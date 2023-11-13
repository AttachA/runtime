// Copyright Danyil Melnytskyi 2022-Present
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <run_time/asm/FuncEnvironment.hpp>
#include <run_time/tasks.hpp>
#include <run_time/tasks/_internal.hpp>

namespace art {
    TaskConditionVariable::TaskConditionVariable() = default;

    TaskConditionVariable::~TaskConditionVariable() {
        notify_all();
    }

    void TaskConditionVariable::wait(art::unique_lock<MutexUnify>& mut) {
        if (loc.is_task_thread) {
            if (mut.mutex()->nmut == &no_race) {
                resume_task.emplace_back(loc.curr_task, loc.curr_task->awake_check);
                swapCtxRelock(no_race);
            } else {
                art::lock_guard guard(no_race);
                resume_task.emplace_back(loc.curr_task, loc.curr_task->awake_check);
                swapCtxRelock(*mut.mutex(), no_race);
            }
        } else {
            art::condition_variable_any cd;
            bool has_res = false;
            art::typed_lgr<Task> task = Task::cxx_native_bridge(has_res, cd);
            if (mut.mutex()->nmut == &no_race) {
                resume_task.emplace_back(task, task->awake_check);
                while (!has_res)
                    cd.wait(mut);
            } else {
                no_race.lock();
                resume_task.emplace_back(task, task->awake_check);
                no_race.unlock();
                while (!has_res)
                    cd.wait(mut);
            }
        task_not_ended:
            //prevent destruct cd, because it is used in task
            task->no_race.lock();
            if (!task->fres.end_of_life) {
                task->no_race.unlock();
                goto task_not_ended;
            }
            task->no_race.unlock();
        }
    }

    bool TaskConditionVariable::wait_for(art::unique_lock<MutexUnify>& mut, size_t milliseconds) {
        return wait_until(mut, std::chrono::high_resolution_clock::now() + std::chrono::milliseconds(milliseconds));
    }

    bool TaskConditionVariable::wait_until(art::unique_lock<MutexUnify>& mut, std::chrono::high_resolution_clock::time_point time_point) {
        if (loc.is_task_thread) {
            art::lock_guard guard(loc.curr_task->no_race);
            makeTimeWait(time_point);
            {
                art::lock_guard guard(no_race);
                resume_task.emplace_back(loc.curr_task, loc.curr_task->awake_check);
            }
            swapCtxRelock(loc.curr_task->no_race);
            if (loc.curr_task->time_end_flag)
                return false;
        } else {
            art::condition_variable_any cd;
            bool has_res = false;
            art::typed_lgr<Task> task = Task::cxx_native_bridge(has_res, cd);

            if (mut.mutex()->nmut == &no_race) {
                resume_task.emplace_back(task, task->awake_check);
                while (!has_res)
                    cd.wait(mut);
            } else {
                no_race.lock();
                resume_task.emplace_back(task, task->awake_check);
                no_race.unlock();
                while (!has_res)
                    cd.wait(mut);
            }

        task_not_ended:
            //prevent destruct cd, because it is used in task
            task->no_race.lock();
            if (!task->fres.end_of_life) {
                task->no_race.unlock();
                goto task_not_ended;
            }
            task->no_race.unlock();

            return !task->time_end_flag;
        }
        return true;
    }

    void TaskConditionVariable::notify_all() {
        art::unique_lock no_race_guard(no_race);
        std::list<__::resume_task> revive_tasks(std::move(resume_task));
        no_race_guard.unlock();
        if (revive_tasks.empty())
            return;
        bool to_yield = false;
        {
            art::lock_guard guard(glob.task_thread_safety);
            for (auto& resumer : revive_tasks) {
                auto& it = resumer.task;
                art::lock_guard guard_loc(it->no_race);
                if (resumer.task->awake_check != resumer.awake_check)
                    continue;
                if (!it->time_end_flag) {
                    it->awaked = true;
                    transfer_task(it);
                }
            }
            glob.tasks_notifier.notify_one();
            if (Task::max_running_tasks && loc.is_task_thread) {
                if (can_be_scheduled_task_to_hot() && loc.curr_task && !loc.curr_task->fres.end_of_life)
                    to_yield = true;
            }
        }
        if (to_yield)
            Task::yield();
    }

    void TaskConditionVariable::notify_one() {
        art::typed_lgr<Task> tsk;
        {
            art::lock_guard guard(no_race);
            while (resume_task.size()) {
                auto& [cur, awake_check] = resume_task.back();
                cur->no_race.lock();
                if (cur->time_end_flag || cur->awake_check != awake_check) {
                    cur->no_race.unlock();
                    resume_task.pop_back();
                } else {
                    tsk = cur;
                    resume_task.pop_back();
                    break;
                }
            }
            if (resume_task.empty() && !tsk)
                return;
        }
        bool to_yield = false;
        art::lock_guard guard_loc(tsk->no_race, art::adopt_lock);
        {
            tsk->awaked = true;
            art::lock_guard guard(glob.task_thread_safety);
            if (Task::max_running_tasks && loc.is_task_thread) {
                if (can_be_scheduled_task_to_hot() && loc.curr_task && !loc.curr_task->end_of_life)
                    to_yield = true;
            }
            transfer_task(tsk);
        }
        if (to_yield)
            Task::yield();
    }

    void TaskConditionVariable::dummy_wait(const art::typed_lgr<Task>& task, art::unique_lock<MutexUnify>& lock) {
        if (lock.mutex()->nmut == &no_race)
            resume_task.emplace_back(task, task->awake_check);
        else {
            art::lock_guard guard(no_race);
            resume_task.emplace_back(task, task->awake_check);
        }
    }

    void TaskConditionVariable::dummy_wait_for(const art::typed_lgr<Task>& task, art::unique_lock<MutexUnify>& lock, size_t milliseconds) {
        dummy_wait_until(task, lock, std::chrono::high_resolution_clock::now() + std::chrono::milliseconds(milliseconds));
    }

    ValueItem* _TaskConditionVariable_dummy_awaiter(ValueItem* args, uint32_t len) {
        art::typed_lgr<Task>& task = *(art::typed_lgr<Task>*)args[0].val;
        TaskConditionVariable* cv = (TaskConditionVariable*)args[1].val;
        std::chrono::high_resolution_clock::time_point& time_point = (std::chrono::high_resolution_clock::time_point&)args[2];
        art::unique_lock<MutexUnify> lock(*(MutexUnify*)args[3].val, art::defer_lock);
        {
            MutexUnify uni(loc.curr_task->no_race);
            art::unique_lock l(uni);
            ValueItem noting;
            loc.curr_task->fres.yieldResult(&noting, l, false);
            //signal to parent task that it is ready to wait
        }
        if (cv->wait_until(lock, time_point))
            task->args = {true};
        else
            task->args = {false};
        Task::start(task);
        return nullptr;
    }

    void TaskConditionVariable::dummy_wait_until(const art::typed_lgr<Task>& task, art::unique_lock<MutexUnify>& lock, std::chrono::high_resolution_clock::time_point time_point) {
        static art::shared_ptr<FuncEnvironment> TaskConditionVariable_dummy_awaiter(new FuncEnvironment(_TaskConditionVariable_dummy_awaiter, false, false));
        delete Task::get_result(new Task(TaskConditionVariable_dummy_awaiter, ValueItem{ValueItem(new art::typed_lgr<Task>(task), VType::async_res), this, time_point, lock.mutex()}));
    }

    bool TaskConditionVariable::has_waiters() {
        art::lock_guard guard(no_race);
        return !resume_task.empty();
    }
}
