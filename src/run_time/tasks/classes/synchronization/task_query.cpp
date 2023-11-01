// Copyright Danyil Melnytskyi 2022-Present
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <run_time/asm/FuncEnvironment.hpp>
#include <run_time/asm/attacha_environment.hpp>
#include <run_time/tasks.hpp>

namespace art {
    struct TaskQueryHandle {
        TaskQuery* tq;
        size_t at_execution_max;
        size_t now_at_execution = 0;
        TaskMutex no_race;
        bool destructed = false;
        TaskConditionVariable end_of_query;
    };

    TaskQuery::TaskQuery(size_t at_execution_max) {
        is_running = false;
        handle = new TaskQueryHandle{this, at_execution_max};
    }

    void __TaskQuery_add_task_leave(TaskQueryHandle* tqh, TaskQuery* tq) {
        art::lock_guard lock(tqh->no_race);
        if (tqh->destructed) {
            if (tqh->at_execution_max == 0)
                delete tqh;
        } else if (!tq->tasks.empty() && tq->is_running) {
            tq->handle->now_at_execution--;
            while (tq->handle->now_at_execution <= tq->handle->at_execution_max) {
                tq->handle->now_at_execution++;
                auto awake_task = tq->tasks.front();
                tq->tasks.pop_front();
                Task::start(awake_task);
            }
        } else {
            tq->handle->now_at_execution--;

            if (tq->handle->now_at_execution == 0 && tq->tasks.empty())
                tq->handle->end_of_query.notify_all();
        }
    }

    ValueItem* __TaskQuery_add_task(ValueItem* args, uint32_t len) {
        TaskQueryHandle* tqh = (TaskQueryHandle*)args[0].val;
        TaskQuery* tq = tqh->tq;
        art::shared_ptr<FuncEnvironment>& call_func = *(art::shared_ptr<FuncEnvironment>*)args[1].val;
        ValueItem& arguments = *(ValueItem*)args[2].val;

        ValueItem* res = nullptr;
        try {
            res = FuncEnvironment::sync_call(call_func, (ValueItem*)arguments.getSourcePtr(), arguments.meta.val_len);
        } catch (...) {
            __TaskQuery_add_task_leave(tqh, tq);
            throw;
        }
        __TaskQuery_add_task_leave(tqh, tq);
        return res;
    }

    art::shared_ptr<FuncEnvironment>& _TaskQuery_add_task = attacha_environment::create_fun_env(new FuncEnvironment(__TaskQuery_add_task, false, false));

    art::typed_lgr<Task> TaskQuery::add_task(art::shared_ptr<FuncEnvironment> call_func, ValueItem& arguments, bool used_task_local, art::shared_ptr<FuncEnvironment> exception_handler, std::chrono::high_resolution_clock::time_point timeout) {
        ValueItem copy;
        if (arguments.meta.vtype == VType::faarr || arguments.meta.vtype == VType::saarr)
            copy = ValueItem((ValueItem*)arguments.getSourcePtr(), arguments.meta.val_len);
        else
            copy = ValueItem({arguments});

        art::typed_lgr<Task> res = new Task(_TaskQuery_add_task, ValueItem{(void*)handle, new art::shared_ptr<FuncEnvironment>(call_func), copy}, used_task_local, exception_handler, timeout);
        art::lock_guard lock(handle->no_race);
        if (is_running && handle->now_at_execution <= handle->at_execution_max) {
            Task::start(res);
            handle->now_at_execution++;
        } else
            tasks.push_back(res);

        return res;
    }

    void TaskQuery::enable() {
        art::lock_guard lock(handle->no_race);
        is_running = true;
        while (handle->now_at_execution < handle->at_execution_max && !tasks.empty()) {
            auto awake_task = tasks.front();
            tasks.pop_front();
            Task::start(awake_task);
            handle->now_at_execution++;
        }
    }

    void TaskQuery::disable() {
        art::lock_guard lock(handle->no_race);
        is_running = false;
    }

    bool TaskQuery::in_query(art::typed_lgr<Task> task) {
        if (task->started)
            return false; //started task can't be in query
        art::lock_guard lock(handle->no_race);
        return std::find(tasks.begin(), tasks.end(), task) != tasks.end();
    }

    void TaskQuery::set_max_at_execution(size_t val) {
        art::lock_guard lock(handle->no_race);
        handle->at_execution_max = val;
    }

    size_t TaskQuery::get_max_at_execution() {
        art::lock_guard lock(handle->no_race);
        return handle->at_execution_max;
    }

    void TaskQuery::wait() {
        MutexUnify unify(handle->no_race);
        art::unique_lock lock(unify);
        while (handle->now_at_execution != 0 && !tasks.empty())
            handle->end_of_query.wait(lock);
    }

    bool TaskQuery::wait_for(size_t milliseconds) {
        return wait_until(std::chrono::high_resolution_clock::now() + std::chrono::milliseconds(milliseconds));
    }

    bool TaskQuery::wait_until(std::chrono::high_resolution_clock::time_point time_point) {
        MutexUnify unify(handle->no_race);
        art::unique_lock lock(unify);
        while (handle->now_at_execution != 0 && !tasks.empty()) {
            if (!handle->end_of_query.wait_until(lock, time_point))
                return false;
        }
        return true;
    }

    TaskQuery::~TaskQuery() {
        handle->destructed = true;
        wait();
        if (handle->now_at_execution == 0)
            delete handle;
    }
}