// Copyright Danyil Melnytskyi 2022-Present
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <base/run_time.hpp>
#include <run_time/asm/FuncEnvironment.hpp>
#include <run_time/tasks.hpp>
#include <run_time/tasks/_internal.hpp>

namespace art {
    TaskMutex::~TaskMutex() {
        art::lock_guard lg(no_race);
        while (!resume_task.empty()) {
            auto& tsk = resume_task.back();
            Task::notify_cancel(tsk.task);
            current_task = nullptr;
            Task::await_task(tsk.task);
            resume_task.pop_back();
        }
    }

#pragma GCC push_options
#pragma GCC optimize("O0")
#pragma optimize("", off)

    void TaskMutex::lock() {
        if (loc.is_task_thread) {
            loc.curr_task->awaked = false;
            loc.curr_task->time_end_flag = false;

            art::lock_guard lg(no_race);
            if (current_task == loc.curr_task.getPtr())
                throw InvalidLock("Tried lock mutex twice");
            while (current_task) {
                resume_task.emplace_back(loc.curr_task, loc.curr_task->awake_check);
                swapCtxRelock(no_race);
            }
            current_task = &*loc.curr_task;
        } else {
            art::unique_lock ul(no_race);
            art::typed_lgr<Task> task;

            if (current_task == reinterpret_cast<Task*>((size_t)_thread_id() | native_thread_flag))
                throw InvalidLock("Tried lock mutex twice");
            while (current_task) {
                art::condition_variable_any cd;
                bool has_res = false;
                task = Task::cxx_native_bridge(has_res, cd);
                resume_task.emplace_back(task, task->awake_check);
                while (!has_res)
                    cd.wait(ul);
            task_not_ended:
                //prevent destruct cd, because it is used in task
                task->no_race.lock();
                if (!task->fres.end_of_life) {
                    task->no_race.unlock();
                    goto task_not_ended;
                }
                task->no_race.unlock();
            }
            current_task = reinterpret_cast<Task*>((size_t)_thread_id() | native_thread_flag);
        }
    }

    bool TaskMutex::try_lock() {
        if (!no_race.try_lock())
            return false;
        art::unique_lock ul(no_race, art::adopt_lock);

        if (current_task)
            return false;
        else if (loc.is_task_thread || loc.context_in_swap)
            current_task = &*loc.curr_task;
        else
            current_task = reinterpret_cast<Task*>((size_t)art::this_thread::get_id() | native_thread_flag);
        return true;
    }

    bool TaskMutex::try_lock_for(size_t milliseconds) {
        return try_lock_until(std::chrono::high_resolution_clock::now() + std::chrono::milliseconds(milliseconds));
    }

    bool TaskMutex::try_lock_until(std::chrono::high_resolution_clock::time_point time_point) {
        if (!no_race.try_lock_until(time_point))
            return false;
        art::unique_lock ul(no_race, art::adopt_lock);

        if (loc.is_task_thread && !loc.context_in_swap) {
            while (current_task) {
                art::lock_guard guard(loc.curr_task->no_race);
                makeTimeWait(time_point);
                resume_task.emplace_back(loc.curr_task, loc.curr_task->awake_check);
                swapCtxRelock(loc.curr_task->no_race, no_race);
                if (!loc.curr_task->awaked)
                    return false;
            }
            current_task = &*loc.curr_task;
            return true;
        } else {
            bool has_res;
            art::condition_variable_any cd;
            while (current_task) {
                has_res = false;
                art::typed_lgr<Task> task = Task::cxx_native_bridge(has_res, cd);
                resume_task.emplace_back(task, task->awake_check);
                while (has_res)
                    cd.wait_until(ul, time_point);
                if (!task->awaked)
                    return false;
            }
            if (!loc.context_in_swap)
                current_task = reinterpret_cast<Task*>((size_t)_thread_id() | native_thread_flag);
            else
                current_task = &*loc.curr_task;
            return true;
        }
    }

#pragma optimize("", on)
#pragma GCC pop_options

    void TaskMutex::unlock() {
        art::lock_guard lg0(no_race);
        if (loc.is_task_thread) {
            if (current_task != &*loc.curr_task)
                throw InvalidOperation("Tried unlock non owned mutex");
        } else if (current_task != reinterpret_cast<Task*>((size_t)_thread_id() | native_thread_flag))
            throw InvalidOperation("Tried unlock non owned mutex");

        current_task = nullptr;
        if (resume_task.size()) {
            art::typed_lgr<Task> it = resume_task.front().task;
            uint16_t awake_check = resume_task.front().awake_check;
            resume_task.pop_front();
            art::lock_guard lg1(it->no_race);
            if (it->awake_check != awake_check)
                return;
            if (!it->time_end_flag) {
                it->awaked = true;
                transfer_task(it);
            }
        }
    }

    bool TaskMutex::is_locked() {
        if (try_lock()) {
            unlock();
            return false;
        }
        return true;
    }

    bool TaskMutex::is_own() {
        art::lock_guard lg0(no_race);
        if (loc.is_task_thread) {
            if (current_task != &*loc.curr_task)
                return false;
        } else if (current_task != reinterpret_cast<Task*>((size_t)_thread_id() | native_thread_flag))
            return false;
        return true;
    }

    ValueItem* _TaskMutex_lock_holder(ValueItem* args, uint32_t len) {
        art::typed_lgr<Task>& task = *(art::typed_lgr<Task>*)args[0].val;
        TaskMutex* mut = (TaskMutex*)args[1].val;
        bool lock_sequence = args[2].val;
        art::unique_lock guard(*mut, art::defer_lock);
        while (!task->fres.end_of_life) {
            guard.lock();
            Task::await_task(task);
            if (!lock_sequence)
                return nullptr;
            {
                art::lock_guard task_guard(task->no_race);
                if (!task->fres.result_notify.has_waiters())
                    task->fres.results.clear();
            }
            guard.unlock();
        }
        return nullptr;
    }

    art::shared_ptr<FuncEnvironment> TaskMutex_lock_holder = new FuncEnvironment(_TaskMutex_lock_holder, false, false);

    void TaskMutex::lifecycle_lock(art::typed_lgr<Task> task) {
        Task::start(new Task(TaskMutex_lock_holder, ValueItem{ValueItem(new art::typed_lgr<Task>(task), VType::async_res), this, false}));
    }

    void TaskMutex::sequence_lock(art::typed_lgr<Task> task) {
        Task::start(new Task(TaskMutex_lock_holder, ValueItem{ValueItem(new art::typed_lgr<Task>(task), VType::async_res), this, true}));
    }
}
