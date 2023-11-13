// Copyright Danyil Melnytskyi 2022-Present
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <base/run_time.hpp>
#include <run_time/ValueEnvironment.hpp>
#include <run_time/asm/FuncEnvironment.hpp>
#include <run_time/asm/exception.hpp>
#include <run_time/tasks.hpp>
#include <run_time/tasks/_internal.hpp>
#include <run_time/tasks/util/interrupt.hpp>
#include <run_time/tasks/util/light_stack.hpp>
#include <util/cxxException.hpp>
#include <util/exceptions.hpp>

namespace art {

    size_t Task::max_running_tasks = configuration::tasks::max_running_tasks;
    size_t Task::max_planned_tasks = configuration::tasks::max_planned_tasks;
    bool Task::enable_task_naming = configuration::tasks::enable_task_naming;
#if _configuration_tasks_enable_preemptive_scheduler_preview && PLATFORM_WINDOWS
    void timer_reinit() {
        std::chrono::nanoseconds interval = loc.current_context->next_quantum();
        interrupt::itimerval timer;
        timer.it_interval.tv_sec = 0;
        timer.it_interval.tv_usec = 0;
        timer.it_value.tv_sec = interval.count() / 1000000000;
        timer.it_value.tv_usec = (interval.count() % 1000000000) / 1000;
        interrupt::setitimer(&timer, nullptr);
    }

    void swapCtx();

    void interruptTask() {
        if (loc.curr_task->bind_to_worker_id != (uint16_t)-1) {
            art::unique_lock guard(glob.binded_workers_safety);
            auto& bind_context = glob.binded_workers[loc.curr_task->bind_to_worker_id];
            guard.unlock();
            if (bind_context.tasks.empty()) {
                if (loc.interrupted_tasks.empty()) {
                    if (glob.cold_tasks.empty()) {
                        timer_reinit();
                        return;
                    } else {
                        if (Task::max_running_tasks && !can_be_scheduled_task_to_hot()) {
                            timer_reinit();
                            return;
                        }
                    }
                }
            }
        } else {
            if (glob.tasks.empty()) {
                if (loc.interrupted_tasks.empty()) {
                    if (glob.cold_tasks.empty()) {
                        timer_reinit();
                        return;
                    } else {
                        if (Task::max_running_tasks && !can_be_scheduled_task_to_hot()) {
                            timer_reinit();
                            return;
                        }
                    }
                }
            }
        }


        loc.interrupted_tasks.push(loc.curr_task);
        loc.current_interrupted = true;
        auto old_relock_0 = loc.curr_task->relock_0;
        auto old_relock_1 = loc.curr_task->relock_1;
        auto old_relock_2 = loc.curr_task->relock_2;
        loc.curr_task->relock_0 = nullptr;
        loc.curr_task->relock_1 = nullptr;
        loc.curr_task->relock_2 = nullptr;
        swapCtx();
        loc.curr_task->relock_0 = old_relock_0;
        loc.curr_task->relock_1 = old_relock_1;
        loc.curr_task->relock_2 = old_relock_2;
    }

    inline bool pre_startup_check() {
        if (loc.current_context->peek_quantum() == std::chrono::nanoseconds(0)) {
            art::unique_lock guard(glob.task_thread_safety);
            glob.tasks.push(std::move(loc.curr_task));
            loc.current_interrupted = true;
            glob.tasks_notifier.notify_one();
            return true;
        }
        return false;
    }

    void set_interruptTask() {
        interrupt::timer_callback(interruptTask);
    }

#define stop_timer() interrupt::stop_timer()
#else
#define timer_reinit()
#define set_interruptTask()
#define stop_timer()
#define pre_startup_check() false
#endif
#pragma optimize("", off)
#if defined(__GNUC__) && !defined(__clang__)
    #pragma GCC push_options
    #pragma GCC optimize("O0")
#endif
#pragma region TaskExecutor

    void swapCtx() {
        stop_timer();

        if (loc.is_task_thread) {
            loc.context_in_swap = true;
            ++glob.tasks_in_swap;
            if (exception::has_exception()) {
                void* cxx = exception::take_current_exception();
                try {
                    *loc.stack_current_context = std::move(*loc.stack_current_context).resume();
                } catch (const boost::context::detail::forced_unwind&) {
                    auto relock_state_0 = loc.curr_task->relock_0;
                    auto relock_state_1 = loc.curr_task->relock_1;
                    auto relock_state_2 = loc.curr_task->relock_2;
                    loc.curr_task->relock_0 = nullptr;
                    loc.curr_task->relock_1 = nullptr;
                    loc.curr_task->relock_2 = nullptr;
                    relock_state_0.relock_end();
                    relock_state_1.relock_end();
                    relock_state_2.relock_end();
                    exception::load_current_exception(cxx);
                    throw;
                }
                exception::load_current_exception(cxx);
            } else {
                *loc.stack_current_context = std::move(*loc.stack_current_context).resume();
            }
            loc.context_in_swap = true;
            auto relock_state_0 = loc.curr_task->relock_0;
            auto relock_state_1 = loc.curr_task->relock_1;
            auto relock_state_2 = loc.curr_task->relock_2;
            loc.curr_task->relock_0 = nullptr;
            loc.curr_task->relock_1 = nullptr;
            loc.curr_task->relock_2 = nullptr;
            relock_state_0.relock_end();
            relock_state_1.relock_end();
            relock_state_2.relock_end();
            loc.curr_task->awake_check++;
            --glob.tasks_in_swap;
            loc.context_in_swap = false;
            if (!loc.current_interrupted)
                checkCancellation();
            if (loc.curr_task->invalid_switch_caught) {
                loc.curr_task->invalid_switch_caught = false;
                throw InvalidContextSwitchException("Caught task that switched context but not scheduled or finalized self");
            }
        } else
            throw InternalException("swapCtx() not allowed call in non-task thread or in dispatcher");
        timer_reinit();
    }

    void swapCtxRelock(const MutexUnify& mut0) {
        loc.curr_task->relock_0 = mut0;
        swapCtx();
    }

    void swapCtxRelock(const MutexUnify& mut0, const MutexUnify& mut1, const MutexUnify& mut2) {
        loc.curr_task->relock_0 = mut0;
        loc.curr_task->relock_1 = mut1;
        loc.curr_task->relock_2 = mut2;
        swapCtx();
    }

    void swapCtxRelock(const MutexUnify& mut0, const MutexUnify& mut1) {
        loc.curr_task->relock_0 = mut0;
        loc.curr_task->relock_1 = mut1;
        swapCtx();
    }

    void warmUpTheTasks() {
        if (!Task::max_running_tasks && glob.tasks.empty()) {
            std::swap(glob.tasks, glob.cold_tasks);
        } else {
            //TODO: put to warm task asynchroniously, i.e. when task reach end of life state, push new task to warm
            size_t placed = glob.in_run_tasks;
            size_t max_tasks = std::min(Task::max_running_tasks - placed, glob.cold_tasks.size());
            for (size_t i = 0; i < max_tasks; ++i) {
                glob.tasks.push(std::move(glob.cold_tasks.front()));
                glob.cold_tasks.pop();
            }
            if (Task::max_running_tasks > placed && glob.cold_tasks.empty())
                glob.can_started_new_notifier.notify_all();
        }
    }

    boost::context::continuation context_exec(boost::context::continuation&& sink) {
        *loc.stack_current_context = std::move(sink);
        try {
            checkCancellation();

            timer_reinit();
            ValueItem* res = loc.curr_task->func->syncWrapper((ValueItem*)loc.curr_task->args.val, loc.curr_task->args.meta.val_len);
            MutexUnify mu(loc.curr_task->no_race);
            art::unique_lock l(mu);
            loc.curr_task->fres.finalResult(res, l);
            loc.context_in_swap = false;
        } catch (TaskCancellation& cancel) {
            forceCancelCancellation(cancel);
        } catch (const boost::context::detail::forced_unwind&) {
            stop_timer();
            throw;
        } catch (...) {
            loc.ex_ptr = std::current_exception();
        }
        try {
            loc.curr_task->args = nullptr;
            if (loc.curr_task->_task_local)
                delete loc.curr_task->_task_local;
        } catch (...) {
        };
        art::lock_guard l(loc.curr_task->no_race);
        loc.curr_task->end_of_life = true;
        loc.curr_task->fres.result_notify.notify_all();
        --glob.in_run_tasks;
        if (Task::max_running_tasks)
            glob.can_started_new_notifier.notify_one();
        stop_timer();
        return std::move(*loc.stack_current_context);
    }

    boost::context::continuation context_ex_handle(boost::context::continuation&& sink) {
        *loc.stack_current_context = std::move(sink);
        try {
            checkCancellation();
            timer_reinit();
            ValueItem* res = loc.curr_task->ex_handle->syncWrapper((ValueItem*)loc.curr_task->args.val, loc.curr_task->args.meta.val_len);
            MutexUnify mu(loc.curr_task->no_race);
            art::unique_lock l(mu);
            loc.curr_task->fres.finalResult(res, l);
            loc.context_in_swap = false;
        } catch (TaskCancellation& cancel) {
            forceCancelCancellation(cancel);
        } catch (const boost::context::detail::forced_unwind&) {
            stop_timer();
            throw;
        } catch (...) {
            loc.ex_ptr = std::current_exception();
        }
        try {
            loc.curr_task->args = nullptr;
            if (loc.curr_task->_task_local)
                delete loc.curr_task->_task_local;
        } catch (...) {
        };

        MutexUnify uni(loc.curr_task->no_race);
        art::unique_lock l(uni);
        loc.curr_task->end_of_life = true;
        loc.curr_task->fres.result_notify.notify_all();
        --glob.in_run_tasks;
        if (Task::max_running_tasks)
            glob.can_started_new_notifier.notify_one();
        stop_timer();
        return std::move(*loc.stack_current_context);
    }

    void transfer_task(art::typed_lgr<Task>& task) {
        if (task->bind_to_worker_id == (uint16_t)-1) {
            art::lock_guard guard(glob.task_thread_safety);
            glob.tasks.push(std::move(task));
            glob.tasks_notifier.notify_one();
        } else {
            art::unique_lock initializer_guard(glob.binded_workers_safety);
            if (!glob.binded_workers.contains(task->bind_to_worker_id)) {
                initializer_guard.unlock();
                invite_to_debugger("Binded worker context " + std::to_string(task->bind_to_worker_id) + " not found");
                std::abort();
            }
            binded_context& extern_context = glob.binded_workers[task->bind_to_worker_id];
            initializer_guard.unlock();
            if (extern_context.in_close) {
                invite_to_debugger("Binded worker context " + std::to_string(task->bind_to_worker_id) + " is closed");
                std::abort();
            }
            art::lock_guard guard(extern_context.no_race);
            extern_context.tasks.emplace_back(std::move(task));
            extern_context.new_task_notifier.notify_one();
        }
    }

    void awake_task(art::typed_lgr<Task>& task) {
        if (task->bind_to_worker_id == (uint16_t)-1) {
            if (task->auto_bind_worker) {
                art::unique_lock guard(glob.binded_workers_safety);
                for (auto& [id, context] : glob.binded_workers) {
                    if (context.allow_implicit_start) {
                        if (context.in_close)
                            continue;
                        guard.unlock();
                        art::unique_lock context_guard(context.no_race);
                        task->bind_to_worker_id = id;
                        context.tasks.push_back(std::move(task));
                        context.new_task_notifier.notify_one();
                        return;
                    }
                }
                throw InternalException("No binded workers available");
            }
        }
        transfer_task(task);
    }

    void taskNotifyIfEmpty(art::unique_lock<art::recursive_mutex>& re_lock) {
        if (!loc.in_exec_decreased)
            --glob.in_exec;
        loc.in_exec_decreased = false;
        if (!glob.in_exec && glob.tasks.empty() && glob.timed_tasks.empty())
            glob.no_tasks_execute_notifier.notify_all();
    }

    bool loadTask() {
        ++glob.in_exec;

#if _configuration_tasks_enable_preemptive_scheduler_preview
        bool interrupt = loc.current_interrupted && !glob.tasks.empty();
        if (!interrupt)
            interrupt = can_be_scheduled_task_to_hot();
        if (interrupt || loc.interrupted_tasks.empty()) {
#endif
            loc.current_interrupted = false;
            size_t len = glob.tasks.size();
            if (!len)
                return true;
            auto tmp = std::move(glob.tasks.front());
            glob.tasks.pop();
            if (len == 1)
                glob.no_tasks_notifier.notify_all();
            loc.curr_task = std::move(tmp);

            if (Task::max_running_tasks) {
                if (can_be_scheduled_task_to_hot()) {
                    if (!glob.cold_tasks.empty()) {
                        glob.tasks.push(std::move(glob.cold_tasks.front()));
                        glob.cold_tasks.pop();
                    }
                }
            } else {
                while (!glob.cold_tasks.empty()) {
                    glob.tasks.push(std::move(glob.cold_tasks.front()));
                    glob.cold_tasks.pop();
                }
            }

#if _configuration_tasks_enable_preemptive_scheduler_preview
        } else {
            auto tmp = std::move(loc.interrupted_tasks.front());
            loc.interrupted_tasks.pop();
            loc.curr_task = std::move(tmp);
        }
#endif
        loc.current_context = loc.curr_task->fres.context;
        loc.stack_current_context = &loc.current_context->context;
        return false;
    }

#define worker_mode_desk(old_name, mode) \
    if (Task::enable_task_naming)        \
        worker_mode_desk_(old_name, mode);

    void worker_mode_desk_(const art::ustring& old_name, const art::ustring& mode) {
        if (old_name.empty())
            _set_name_thread_dbg("Worker " + std::to_string(_thread_id()) + ": " + mode);
        else
            _set_name_thread_dbg(old_name + " | (Temporal worker) " + std::to_string(_thread_id()) + ": " + mode);
    }

    void pseudo_task_handle(const art::ustring& old_name, bool& caught_ex) {
        loc.is_task_thread = false;
        caught_ex = false;
        try {
            if (loc.curr_task->_task_local == (ValueEnvironment*)-1) {
                worker_mode_desk(old_name, " executing callback")
                    task_callback::start(*loc.curr_task);
            } else {
                //worker_mode_desk(old_name, " executing function - " + loc.curr_task->func->to_string())
                ValueItem* res = FuncEnvironment::sync_call(loc.curr_task->func, (ValueItem*)loc.curr_task->args.getSourcePtr(), loc.curr_task->args.meta.val_len);
                MutexUnify mu(loc.curr_task->no_race);
                art::unique_lock l(mu);
                loc.curr_task->fres.finalResult(res, l);
            }

        } catch (...) {
            loc.is_task_thread = true;
            loc.ex_ptr = std::current_exception();
            caught_ex = true;

            if (!need_restore_stack_fault())
                return;
        }
        if (caught_ex)
            restore_stack_fault();
        worker_mode_desk(old_name, "idle");
        loc.is_task_thread = true;
    }

    bool execute_task(const art::ustring& old_name) {
        bool pseudo_handle_caught_ex = false;
        if (!loc.curr_task->func)
            return true;
        if (loc.curr_task->_task_local == (ValueEnvironment*)-1 || loc.curr_task->func->isCheap()) {
            pseudo_task_handle(old_name, pseudo_handle_caught_ex);
            if (pseudo_handle_caught_ex)
                goto caught_ex;
            goto end_task;
        }
        if (loc.curr_task->end_of_life)
            goto end_task;

        worker_mode_desk(old_name, "process task - " + std::to_string(loc.curr_task->task_id()));
        if (*loc.stack_current_context) {
            if (pre_startup_check())
                return false;
            *loc.stack_current_context = std::move(*loc.stack_current_context).resume();
            loc.curr_task->relock_0.relock_start();
            loc.curr_task->relock_1.relock_start();
            loc.curr_task->relock_2.relock_start();
        } else {
            ++glob.in_run_tasks;
            --glob.planned_tasks;
            if (Task::max_planned_tasks)
                glob.can_planned_new_notifier.notify_one();
            if (pre_startup_check())
                return false;
            *loc.stack_current_context = boost::context::callcc(std::allocator_arg, light_stack(1048576 /*1 mb*/), context_exec);
            loc.curr_task->relock_0.relock_start();
            loc.curr_task->relock_1.relock_start();
            loc.curr_task->relock_2.relock_start();
        }
    caught_ex:
        if (loc.ex_ptr) {
            if (loc.curr_task->ex_handle) {
                ValueItem temp(new std::exception_ptr(loc.ex_ptr), VType::except_value, no_copy);
                loc.curr_task->args = ValueItem(&temp, 0);
                loc.ex_ptr = nullptr;
                *loc.stack_current_context = boost::context::callcc(context_ex_handle);
                if (!loc.ex_ptr)
                    goto end_task;
            }
            MutexUnify uni(loc.curr_task->no_race);
            art::unique_lock l(uni);
            loc.curr_task->fres.finalResult(ValueItem(new std::exception_ptr(loc.ex_ptr), ValueMeta(VType::except_value), no_copy), l);
            loc.ex_ptr = nullptr;
        }
    end_task:
        loc.is_task_thread = false;
        if (!loc.curr_task->fres.end_of_life && loc.curr_task.totalLinks() == 1) {
            loc.curr_task->invalid_switch_caught = true;
            glob.tasks.push(loc.curr_task);
        }
        loc.curr_task = nullptr;
        worker_mode_desk(old_name, "idle");
        return false;
    }

    bool taskExecutor_check_next(art::unique_lock<art::recursive_mutex>& guard, bool end_in_task_out) {
        loc.context_in_swap = false;
        loc.current_context = nullptr;
        loc.stack_current_context = nullptr;
        taskNotifyIfEmpty(guard);
        loc.is_task_thread = false;
        while (glob.tasks.empty()) {
            if (!glob.cold_tasks.empty()) {
                if (can_be_scheduled_task_to_hot()) {
                    warmUpTheTasks();
                    break;
                }
            }

            if (end_in_task_out)
                return true;
            glob.tasks_notifier.wait(guard);
        }
        loc.is_task_thread = true;
        return false;
    }

    void taskExecutor(bool end_in_task_out) {
        set_interruptTask();
        art::ustring old_name = end_in_task_out ? _get_name_thread_dbg(_thread_id()) : "";

        if (old_name.empty())
            _set_name_thread_dbg("Worker " + std::to_string(_thread_id()));
        else
            _set_name_thread_dbg(old_name + " | (Temporal worker) " + std::to_string(_thread_id()));

        art::unique_lock<art::recursive_mutex> guard(glob.task_thread_safety);
        glob.workers_completions.push_front(0);
        auto to_remove_after_death = glob.workers_completions.begin();
        uint32_t& completions = glob.workers_completions.front();
        ++glob.in_exec;
        ++glob.executors;
        bool termination_state = false;

        while (true) {
            if (termination_state) {
                if (loc.interrupted_tasks.empty())
                    break;
                else {
                    loc.curr_task = std::move(loc.interrupted_tasks.front());
                    loc.interrupted_tasks.pop();
                    loc.current_context = loc.curr_task->fres.context;
                    loc.stack_current_context = &loc.current_context->context;
                }
            } else {
                if (taskExecutor_check_next(guard, end_in_task_out)) {
                    termination_state = true;
                    guard.unlock();
                } else {
                    if (loadTask())
                        continue;
                    glob.executor_manager_task_taken.notify_one();
                    guard.unlock();
                    if (loc.curr_task->bind_to_worker_id != (uint16_t)-1) {
                        transfer_task(loc.curr_task);
                        guard.lock();
                        continue;
                    }
                }
            }
            //if func is nullptr then this task signal to shutdown executor
            bool shut_down_signal = execute_task(old_name);
            if (shut_down_signal) {
                termination_state = true;
                continue;
            }
            guard.lock();
            completions += 1;
        }
        --glob.executors;
        glob.workers_completions.erase(to_remove_after_death);
        taskNotifyIfEmpty(guard);
        glob.executor_shutdown_notifier.notify_all();
    }

    void bindedTaskExecutor(uint16_t id) {
        set_interruptTask();
        art::ustring old_name = "Binded";
        art::unique_lock initializer_guard(glob.binded_workers_safety);
        if (!glob.binded_workers.contains(id)) {
            invite_to_debugger("Binded worker context " + std::to_string(id) + " not found");
            std::abort();
        }
        binded_context& context = glob.binded_workers[id];
        context.completions.push_front(0);
        auto to_remove_after_death = context.completions.begin();
        uint32_t& completions = context.completions.front();
        initializer_guard.unlock();

        std::list<art::typed_lgr<Task>>& queue = context.tasks;
        art::recursive_mutex& safety = context.no_race;
        art::condition_variable_any& notifier = context.new_task_notifier;
        bool pseudo_handle_caught_ex = false;
        _set_name_thread_dbg("Binded worker " + std::to_string(_thread_id()) + ": " + std::to_string(id));

        art::unique_lock guard(safety);
        context.executors++;
        bool termination_state = false;
        while (true) {
            if (termination_state) {
                if (loc.interrupted_tasks.empty())
                    break;
                else {
                    loc.curr_task = std::move(loc.interrupted_tasks.front());
                    loc.interrupted_tasks.pop();
                }
            } else {
                if ((loc.current_interrupted && !queue.empty()) || loc.interrupted_tasks.empty()) {
                    loc.current_interrupted = false;
                    while (queue.empty()) {
                        notifier.wait(guard);
                    }
                    loc.curr_task = std::move(queue.front());
                    queue.pop_front();
                } else {
                    loc.curr_task = std::move(loc.interrupted_tasks.front());
                    loc.interrupted_tasks.pop();
                }
            }

            guard.unlock();
            if (!loc.curr_task->func) {
                termination_state = true;
                continue;
            }
            if (loc.curr_task->bind_to_worker_id != (uint16_t)id) {
                transfer_task(loc.curr_task);
                continue;
            }
            loc.is_task_thread = true;
            loc.current_context = loc.curr_task->fres.context;
            loc.stack_current_context = &loc.current_context->context;
            if (execute_task(old_name))
                break;
            completions += 1;
            guard.lock();
        }
        guard.lock();
        --context.executors;
        if (context.executors == 0) {
            if (context.in_close) {
                context.on_closed_notifier.notify_all();
                guard.unlock();
            } else {
                invite_to_debugger("Caught executor/s death when context is not closed");
                std::abort();
            }
        }

        initializer_guard.lock();
        context.completions.erase(to_remove_after_death);
    }

#pragma endregion

    void taskTimer() {
        glob.time_control_enabled = true;
        _set_name_thread_dbg("Task time controller");

        art::unique_lock guard(glob.task_timer_safety);
        list_array<art::typed_lgr<Task>> cached_wake_ups;
        list_array<art::typed_lgr<Task>> cached_cold;
        while (glob.time_control_enabled) {
            if (glob.timed_tasks.size()) {
                while (glob.timed_tasks.front().wait_timepoint <= std::chrono::high_resolution_clock::now()) {
                    timing& tmng = glob.timed_tasks.front();
                    if (tmng.check_id != tmng.awake_task->awake_check) {
                        glob.timed_tasks.pop_front();
                        if (glob.timed_tasks.empty())
                            break;
                        else
                            continue;
                    }
                    art::lock_guard task_guard(tmng.awake_task->no_race);
                    if (tmng.awake_task->awaked) {
                        glob.timed_tasks.pop_front();
                    } else {
                        tmng.awake_task->time_end_flag = true;
                        cached_wake_ups.push_back(std::move(tmng.awake_task));
                        glob.timed_tasks.pop_front();
                    }
                    if (glob.timed_tasks.empty())
                        break;
                }
            }
            if (glob.cold_timed_tasks.size()) {
                while (glob.cold_timed_tasks.front().wait_timepoint <= std::chrono::high_resolution_clock::now()) {
                    timing& tmng = glob.cold_timed_tasks.front();
                    if (tmng.check_id != tmng.awake_task->awake_check) {
                        glob.cold_timed_tasks.pop_front();
                        if (glob.cold_timed_tasks.empty())
                            break;
                        else
                            continue;
                    }
                    cached_cold.push_back(std::move(tmng.awake_task));
                    glob.cold_timed_tasks.pop_front();
                    if (glob.cold_timed_tasks.empty())
                        break;
                }
            }
            guard.unlock();
            if (!cached_wake_ups.empty() || !cached_cold.empty()) {
                art::lock_guard guard(glob.task_thread_safety);
                if (!cached_wake_ups.empty())
                    while (!cached_wake_ups.empty())
                        glob.tasks.push(std::move(cached_wake_ups.take_back()));
                if (!cached_cold.empty())
                    while (!cached_cold.empty())
                        glob.cold_tasks.push(std::move(cached_cold.take_back()));
                glob.tasks_notifier.notify_all();
            }
            guard.lock();
            if (glob.timed_tasks.empty() && glob.cold_timed_tasks.empty())
                glob.time_notifier.wait(guard);
            else if (glob.timed_tasks.size() && glob.cold_timed_tasks.size()) {
                if (glob.timed_tasks.front().wait_timepoint < glob.cold_timed_tasks.front().wait_timepoint)
                    glob.time_notifier.wait_until(guard, glob.timed_tasks.front().wait_timepoint);
                else
                    glob.time_notifier.wait_until(guard, glob.cold_timed_tasks.front().wait_timepoint);
            } else if (glob.timed_tasks.size())
                glob.time_notifier.wait_until(guard, glob.timed_tasks.front().wait_timepoint);
            else
                glob.time_notifier.wait_until(guard, glob.cold_timed_tasks.front().wait_timepoint);
        }
    }

#if defined(__GNUC__) && !defined(__clang__)
    #pragma GCC pop_options
#endif
#pragma optimize("", on)
    void startTimeController() {
        art::lock_guard guard(glob.task_timer_safety);
        if (glob.time_control_enabled)
            return;
        art::thread(taskTimer).detach();
        glob.time_control_enabled = true;
    }

    void unsafe_put_task_to_timed_queue(std::deque<timing>& queue, std::chrono::high_resolution_clock::time_point t, art::typed_lgr<Task>& task) {
        size_t i = 0;
        auto it = queue.begin();
        auto end = queue.end();
        while (it != end) {
            if (it->wait_timepoint >= t) {
                queue.emplace(it, timing(t, task, task->awake_check));
                i = -1;
                break;
            }
            ++it;
        }
        if (i != -1)
            queue.emplace_back(timing(t, task, task->awake_check));
    }

    void makeTimeWait(std::chrono::high_resolution_clock::time_point t) {
        if (!glob.time_control_enabled)
            startTimeController();
        loc.curr_task->awaked = false;
        loc.curr_task->time_end_flag = false;

        art::lock_guard guard(glob.task_timer_safety);
        unsafe_put_task_to_timed_queue(glob.timed_tasks, t, loc.curr_task);
        glob.time_notifier.notify_one();
    }
}
