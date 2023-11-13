// Copyright Danyil Melnytskyi 2022-Present
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <run_time/tasks.hpp>
#include <run_time/tasks/_internal.hpp>

namespace art {
    void TaskLimiter::set_max_threshold(size_t val) {
        art::lock_guard guard(no_race);
        if (val < 1)
            val = 1;
        if (max_threshold == val)
            return;
        if (max_threshold > val) {
            if (allow_threshold > max_threshold - val)
                allow_threshold -= max_threshold - val;
            else {
                locked = true;
                allow_threshold = 0;
            }
            max_threshold = val;
            return;
        } else {
            if (!allow_threshold) {
                size_t unlocks = max_threshold;
                max_threshold = val;
                while (unlocks-- >= 1)
                    unchecked_unlock();
            } else
                allow_threshold += val - max_threshold;
        }
    }

    void TaskLimiter::lock() {
        art::unique_lock guard(no_race);
        while (locked) {
            if (loc.is_task_thread) {
                loc.curr_task->awaked = false;
                loc.curr_task->time_end_flag = false;
                resume_task.emplace_back(loc.curr_task, loc.curr_task->awake_check);
                swapCtxRelock(*guard.mutex());
            } else
                native_notify.wait(guard);
        }
        if (--allow_threshold == 0)
            locked = true;

        if (lock_check.contains(&*loc.curr_task)) {
            if (++allow_threshold != 0)
                locked = false;
            no_race.unlock();
            throw InvalidLock("Dead lock. Task try lock already locked task limiter");
        } else
            lock_check.push_back(&*loc.curr_task);
        no_race.unlock();
        return;
    }

    bool TaskLimiter::try_lock() {
        if (!no_race.try_lock())
            return false;
        if (!locked) {
            no_race.unlock();
            return false;
        } else if (--allow_threshold <= 0)
            locked = true;

        if (lock_check.contains(&*loc.curr_task)) {
            if (++allow_threshold != 0)
                locked = false;
            no_race.unlock();
            throw InvalidLock("Dead lock. Task try lock already locked task limiter");
        } else
            lock_check.push_back(&*loc.curr_task);
        no_race.unlock();
        return true;
    }

    bool TaskLimiter::try_lock_for(size_t milliseconds) {
        return try_lock_until(std::chrono::high_resolution_clock::now() + std::chrono::milliseconds(milliseconds));
    }

    bool TaskLimiter::try_lock_until(std::chrono::high_resolution_clock::time_point time_point) {
        if (no_race.try_lock_until(time_point))
            return false;
        art::unique_lock guard(no_race, art::adopt_lock);
        while (locked) {
            if (loc.is_task_thread) {
                loc.curr_task->awaked = false;
                loc.curr_task->time_end_flag = false;
                makeTimeWait(time_point);
                resume_task.emplace_back(loc.curr_task, loc.curr_task->awake_check);
                swapCtxRelock(no_race);
                if (!loc.curr_task->awaked)
                    return false;
            } else if (native_notify.wait_until(guard, time_point) == art::cv_status::timeout)
                return false;
        }
        if (--allow_threshold <= 0)
            locked = true;

        if (lock_check.contains(&*loc.curr_task)) {
            if (++allow_threshold != 0)
                locked = false;
            no_race.unlock();
            throw InvalidLock("Dead lock. Task try lock already locked task limiter");
        } else
            lock_check.push_back(&*loc.curr_task);
        no_race.unlock();
        return true;
    }

    void TaskLimiter::unlock() {
        art::lock_guard lg0(no_race);
        if (!lock_check.contains(&*loc.curr_task))
            throw InvalidUnlock("Invalid unlock. Task try unlock already unlocked task limiter");
        else
            lock_check.erase(&*loc.curr_task);
        unchecked_unlock();
    }

    void TaskLimiter::unchecked_unlock() {
        if (allow_threshold >= max_threshold)
            return;
        allow_threshold++;
        native_notify.notify_one();
        while (resume_task.size()) {
            auto& it = resume_task.back();
            art::lock_guard lg2(it.task->no_race);
            if (!it.task->time_end_flag) {
                if (it.task->awake_check != it.awake_check)
                    continue;
                it.task->awaked = true;
                auto task = resume_task.front().task;
                resume_task.pop_front();
                transfer_task(task);
                return;
            } else
                resume_task.pop_front();
        }
    }

    bool TaskLimiter::is_locked() {
        return locked;
    }
}