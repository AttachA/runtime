// Copyright Danyil Melnytskyi 2023-Present
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <algorithm>
#include <base/run_time.hpp>
#include <run_time/asm/FuncEnvironment.hpp>
#include <run_time/asm/attacha_environment.hpp>
#include <run_time/tasks.hpp>
#include <run_time/tasks/_internal.hpp>

namespace art {
    ValueItem* _TaskRWMutex_lock_holder(ValueItem* args, uint32_t len) {
        art::typed_lgr<Task>& task = *(art::typed_lgr<Task>*)args[0].val;

        struct RW_Guard {
            TaskRWMutex* mut;
            bool locked;
            bool lock_sequence;
            bool as_writer;

            RW_Guard(TaskRWMutex* mut, bool lock_sequence, bool as_writer, bool locked = false)
                : mut(mut), lock_sequence(lock_sequence), as_writer(as_writer), locked(locked) {}

            void lock() {
                if (as_writer)
                    mut->write_lock();
                else
                    mut->read_lock();
                locked = true;
            }

            void unlock() {
                if (locked) {
                    if (as_writer)
                        mut->write_unlock();
                    else
                        mut->read_unlock();
                    locked = false;
                }
            }

            ~RW_Guard() {
                unlock();
            }
        } guard(
            (TaskRWMutex*)args[1].val,
            false,
            (bool)args[2].val,
            (bool)args[3].val
        );

        while (!task->fres.end_of_life) {
            guard.lock();
            Task::await_task(task);
            if (!guard.lock_sequence)
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

    art::shared_ptr<FuncEnvironment>& TaskRWMutex_lock_holder = attacha_environment::create_fun_env(new FuncEnvironment(_TaskRWMutex_lock_holder, true, false));

    TaskRWMutex::~TaskRWMutex() {
        art::lock_guard lg(no_race);
        if (current_writer_task || !readers.empty())
            throw InvalidOperation("Mutex destroyed while locked");
    }

    void TaskRWMutex::read_lock() {
        if (loc.is_task_thread) {
            loc.curr_task->awaked = false;
            loc.curr_task->time_end_flag = false;

            art::lock_guard lg(no_race);
            if (std::find(readers.begin(), readers.end(), loc.curr_task.getPtr()) != readers.end())
                throw InvalidLock("Tried lock mutex twice");
            if (current_writer_task == loc.curr_task.getPtr())
                throw InvalidLock("Tried lock write and then read mode");
            while (current_writer_task) {
                resume_task.emplace_back(loc.curr_task, loc.curr_task->awake_check);
                swapCtxRelock(no_race);
            }
            readers.push_back(&*loc.curr_task);
        } else {
            art::unique_lock ul(no_race);
            art::typed_lgr<Task> task;
            Task* self_mask = reinterpret_cast<Task*>((size_t)_thread_id() | native_thread_flag);
            if (std::find(readers.begin(), readers.end(), self_mask) != readers.end())
                throw InvalidLock("Tried lock mutex twice");
            while (current_writer_task) {
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
            readers.push_back(self_mask);
        }
    }

    bool TaskRWMutex::try_read_lock() {
        if (!no_race.try_lock())
            return false;
        art::unique_lock ul(no_race, art::adopt_lock);

        if (current_writer_task)
            return false;
        else {
            Task* self_mask;
            if (loc.is_task_thread || loc.context_in_swap)
                self_mask = &*loc.curr_task;
            else
                self_mask = reinterpret_cast<Task*>((size_t)_thread_id() | native_thread_flag);
            if (std::find(readers.begin(), readers.end(), self_mask) != readers.end())
                return false;
            if (current_writer_task == loc.curr_task.getPtr())
                return false;
            readers.push_back(self_mask);
            return true;
        }
    }

    bool TaskRWMutex::try_read_lock_for(size_t milliseconds) {
        return try_read_lock_until(std::chrono::high_resolution_clock::now() + std::chrono::milliseconds(milliseconds));
    }

    bool TaskRWMutex::try_read_lock_until(std::chrono::high_resolution_clock::time_point time_point) {
        if (!no_race.try_lock_until(time_point))
            return false;
        art::unique_lock ul(no_race, art::adopt_lock);

        if (current_writer_task)
            return false;
        else {
            Task* self_mask;
            if (loc.is_task_thread || loc.context_in_swap)
                self_mask = &*loc.curr_task;
            else
                self_mask = reinterpret_cast<Task*>((size_t)_thread_id() | native_thread_flag);
            if (std::find(readers.begin(), readers.end(), self_mask) != readers.end())
                return false;
            if (current_writer_task == loc.curr_task.getPtr())
                return false;
            readers.push_back(self_mask);
            return true;
        }
    }

    void TaskRWMutex::read_unlock() {
        art::lock_guard lg0(no_race);
        if (readers.empty())
            throw InvalidOperation("Tried unlock non owned mutex");
        else {
            Task* self_mask;
            if (loc.is_task_thread || loc.context_in_swap)
                self_mask = &*loc.curr_task;
            else
                self_mask = reinterpret_cast<Task*>((size_t)_thread_id() | native_thread_flag);
            auto it = std::find(readers.begin(), readers.end(), self_mask);
            if (it == readers.end())
                throw InvalidOperation("Tried unlock non owned mutex");
            readers.erase(it);

            if (resume_task.size() && readers.empty() && !current_writer_task) {
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
    }

    bool TaskRWMutex::is_read_locked() {
        Task* self_mask;
        if (loc.is_task_thread || loc.context_in_swap)
            self_mask = &*loc.curr_task;
        else
            self_mask = reinterpret_cast<Task*>((size_t)_thread_id() | native_thread_flag);
        auto it = std::find(readers.begin(), readers.end(), self_mask);
        return it != readers.end();
    }

    void TaskRWMutex::lifecycle_read_lock(const art::typed_lgr<Task>& task) {
        Task::start(new Task(TaskRWMutex_lock_holder, ValueItem{ValueItem(new art::typed_lgr<Task>(task), VType::async_res), this, false, false}));
    }

    void TaskRWMutex::sequence_read_lock(const art::typed_lgr<Task>& task) {
        Task::start(new Task(TaskRWMutex_lock_holder, ValueItem{ValueItem(new art::typed_lgr<Task>(task), VType::async_res), this, true, false}));
    }

    void TaskRWMutex::write_lock() {
        if (loc.is_task_thread) {
            loc.curr_task->awaked = false;
            loc.curr_task->time_end_flag = false;

            art::lock_guard lg(no_race);
            if (current_writer_task == loc.curr_task.getPtr())
                throw InvalidLock("Tried lock mutex twice");
            if (std::find(readers.begin(), readers.end(), loc.curr_task.getPtr()) != readers.end())
                throw InvalidLock("Tried lock read and then write mode");
            while (current_writer_task) {
                resume_task.emplace_back(loc.curr_task, loc.curr_task->awake_check);
                swapCtxRelock(no_race);
            }
            current_writer_task = &*loc.curr_task;
            while (!readers.empty()) {
                resume_task.emplace_back(loc.curr_task, loc.curr_task->awake_check);
                swapCtxRelock(no_race);
            }
        } else {
            art::unique_lock ul(no_race);
            art::typed_lgr<Task> task;

            if (current_writer_task == reinterpret_cast<Task*>((size_t)_thread_id() | native_thread_flag))
                throw InvalidLock("Tried lock mutex twice");
            while (current_writer_task) {
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
            current_writer_task = reinterpret_cast<Task*>((size_t)_thread_id() | native_thread_flag);
            while (!readers.empty()) {
                art::condition_variable_any cd;
                bool has_res = false;
                task = Task::cxx_native_bridge(has_res, cd);
                resume_task.emplace_back(task, task->awake_check);
                while (!has_res)
                    cd.wait(ul);
            task_not_ended2:
                //prevent destruct cd, because it is used in task
                task->no_race.lock();
                if (!task->fres.end_of_life) {
                    task->no_race.unlock();
                    goto task_not_ended2;
                }
                task->no_race.unlock();
            }
        }
    }

    bool TaskRWMutex::try_write_lock() {
        if (!no_race.try_lock())
            return false;
        art::unique_lock ul(no_race, art::adopt_lock);

        if (current_writer_task || !readers.empty())
            return false;
        else if (loc.is_task_thread || loc.context_in_swap)
            current_writer_task = &*loc.curr_task;
        else
            current_writer_task = reinterpret_cast<Task*>((size_t)art::this_thread::get_id() | native_thread_flag);
        return true;
    }

    bool TaskRWMutex::try_write_lock_for(size_t milliseconds) {
        return try_write_lock_until(std::chrono::high_resolution_clock::now() + std::chrono::milliseconds(milliseconds));
    }

    bool TaskRWMutex::try_write_lock_until(std::chrono::high_resolution_clock::time_point time_point) {
        if (!no_race.try_lock_until(time_point))
            return false;
        art::unique_lock ul(no_race, art::adopt_lock);

        if (loc.is_task_thread && !loc.context_in_swap) {
            while (current_writer_task) {
                art::lock_guard guard(loc.curr_task->no_race);
                makeTimeWait(time_point);
                resume_task.emplace_back(loc.curr_task, loc.curr_task->awake_check);
                swapCtxRelock(loc.curr_task->no_race, no_race);
                if (!loc.curr_task->awaked)
                    return false;
            }
            current_writer_task = &*loc.curr_task;

            while (!readers.empty()) {
                art::lock_guard guard(loc.curr_task->no_race);
                makeTimeWait(time_point);
                resume_task.emplace_back(loc.curr_task, loc.curr_task->awake_check);
                swapCtxRelock(loc.curr_task->no_race, no_race);
                if (!loc.curr_task->awaked) {
                    current_writer_task = nullptr;
                    return false;
                }
            }
            return true;
        } else {
            bool has_res;
            art::condition_variable_any cd;
            while (current_writer_task) {
                has_res = false;
                art::typed_lgr<Task> task = Task::cxx_native_bridge(has_res, cd);
                resume_task.emplace_back(task, task->awake_check);
                while (has_res)
                    cd.wait_until(ul, time_point);
                if (!task->awaked)
                    return false;
            }
            if (!loc.context_in_swap)
                current_writer_task = reinterpret_cast<Task*>((size_t)_thread_id() | native_thread_flag);
            else
                current_writer_task = &*loc.curr_task;

            while (!readers.empty()) {
                has_res = false;
                art::typed_lgr<Task> task = Task::cxx_native_bridge(has_res, cd);
                resume_task.emplace_back(task, task->awake_check);
                while (has_res)
                    cd.wait_until(ul, time_point);
                if (!task->awaked) {
                    current_writer_task = nullptr;
                    return false;
                }
            }
            return true;
        }
    }

    void TaskRWMutex::write_unlock() {
        art::unique_lock ul(no_race);
        Task* self_mask;
        if (loc.is_task_thread || loc.context_in_swap)
            self_mask = &*loc.curr_task;
        else
            self_mask = reinterpret_cast<Task*>((size_t)_thread_id() | native_thread_flag);

        if (current_writer_task == self_mask)
            throw InvalidOperation("Tried unlock non owned mutex");
        current_writer_task = nullptr;
        while (resume_task.size()) {
            art::typed_lgr<Task> it = resume_task.front().task;
            uint16_t awake_check = resume_task.front().awake_check;
            resume_task.pop_front();
            art::lock_guard lg1(it->no_race);
            if (it->awake_check != awake_check)
                continue;
            if (!it->time_end_flag) {
                it->awaked = true;
                transfer_task(it);
            }
        }
    }

    bool TaskRWMutex::is_write_locked() {
        Task* self_mask;
        if (loc.is_task_thread || loc.context_in_swap)
            self_mask = &*loc.curr_task;
        else
            self_mask = reinterpret_cast<Task*>((size_t)_thread_id() | native_thread_flag);
        return current_writer_task == self_mask;
    }

    void TaskRWMutex::lifecycle_write_lock(const art::typed_lgr<Task>& task) {
        Task::start(new Task(TaskRWMutex_lock_holder, ValueItem{ValueItem(new art::typed_lgr<Task>(task), VType::async_res), this, false, true}));
    }

    void TaskRWMutex::sequence_write_lock(const art::typed_lgr<Task>& task) {
        Task::start(new Task(TaskRWMutex_lock_holder, ValueItem{ValueItem(new art::typed_lgr<Task>(task), VType::async_res), this, true, true}));
    }

    bool TaskRWMutex::is_own() {
        if (is_write_locked())
            return true;
        else
            return is_read_locked();
    }
}