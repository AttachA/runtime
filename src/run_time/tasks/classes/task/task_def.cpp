// Copyright Danyil Melnytskyi 2022-Present
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <run_time/ValueEnvironment.hpp>
#include <run_time/asm/FuncEnvironment.hpp>
#include <run_time/tasks.hpp>
#include <run_time/tasks/_internal.hpp>
#include <run_time/tasks/util/native_workers_singleton.hpp>
#include <util/platform.hpp>

namespace art {
    ValueItem* _empty_func(ValueItem* /*ignored*/, uint32_t /*ignored*/) {
        return nullptr;
    }

    art::shared_ptr<FuncEnvironment> empty_func(new FuncEnvironment(_empty_func, false, true));

    void put_arguments(ValueItem& args_hold, const ValueItem& arguments) {
        if (arguments.meta.vtype == VType::faarr || arguments.meta.vtype == VType::saarr)
            args_hold = std::move(arguments);
        else if (arguments.meta.vtype != VType::noting)
            args_hold = {std::move(arguments)};
    }

    void put_arguments(ValueItem& args_hold, ValueItem&& arguments) {
        if (arguments.meta.vtype == VType::faarr)
            args_hold = std::move(arguments);
        else if (arguments.meta.vtype == VType::saarr) {
            ValueItem* arr = new ValueItem[arguments.meta.val_len];
            ValueItem* arr2 = (ValueItem*)arguments.val;
            for (size_t i = 0; i < arguments.meta.val_len; ++i)
                arr[i] = std::move(arr2[i]);
            args_hold = ValueItem(arr, arguments.meta.val_len, no_copy);
        } else if (arguments.meta.vtype != VType::noting) {
            ValueItem* arr = new ValueItem[1];
            arr[0] = std::move(arguments);
            args_hold = std::move(ValueItem(arr, 1, no_copy));
        }
    }

    void initTaskContext(TaskResult& fres, TaskPriority priority) {
        fres.context = new task_context;
        switch (priority) {
        case TaskPriority::background:
            fres.context->_priority = task_context::priority::background;
            break;
        case TaskPriority::low:
            fres.context->_priority = task_context::priority::low;
            break;
        case TaskPriority::lower:
            fres.context->_priority = task_context::priority::lower;
            break;
        case TaskPriority::normal:
            fres.context->_priority = task_context::priority::normal;
            break;
        case TaskPriority::higher:
            fres.context->_priority = task_context::priority::higher;
            break;
        case TaskPriority::high:
            fres.context->_priority = task_context::priority::high;
            break;
        case TaskPriority::realtime:
            fres.context->_priority = task_context::priority::semi_realtime;
            break;
        default:
            break;
        }
        fres.context->init_quantum();
    }

    Task::Task(art::shared_ptr<FuncEnvironment> call_func, const ValueItem& arguments, bool used_task_local, art::shared_ptr<FuncEnvironment> exception_handler, std::chrono::high_resolution_clock::time_point task_timeout, TaskPriority priority) {
        ex_handle = exception_handler;
        func = call_func;
        put_arguments(args, arguments);

        timeout = task_timeout;
        if (used_task_local)
            _task_local = new ValueEnvironment();

        if (Task::max_planned_tasks) {
            MutexUnify uni(glob.task_thread_safety);
            art::unique_lock l(uni);
            while (glob.planned_tasks >= Task::max_planned_tasks)
                glob.can_planned_new_notifier.wait(l);
        }
        initTaskContext(fres, priority);
        ++glob.planned_tasks;
    }

    Task::Task(art::shared_ptr<FuncEnvironment> call_func, ValueItem&& arguments, bool used_task_local, art::shared_ptr<FuncEnvironment> exception_handler, std::chrono::high_resolution_clock::time_point task_timeout, TaskPriority priority) {
        ex_handle = exception_handler;
        func = call_func;
        put_arguments(args, std::move(arguments));

        timeout = task_timeout;
        if (used_task_local)
            _task_local = new ValueEnvironment();

        if (Task::max_planned_tasks) {
            MutexUnify uni(glob.task_thread_safety);
            art::unique_lock l(uni);
            while (glob.planned_tasks >= Task::max_planned_tasks)
                glob.can_planned_new_notifier.wait(l);
        }
        initTaskContext(fres, priority);
        ++glob.planned_tasks;
    }

    Task::Task(Task&& mov) noexcept
        : fres(std::move(mov.fres)) {
        ex_handle = mov.ex_handle;
        func = mov.func;
        args = std::move(mov.args);
        _task_local = mov._task_local;
        mov._task_local = nullptr;
        time_end_flag = mov.time_end_flag;
        awaked = mov.awaked;
        started = mov.started;
        is_yield_mode = mov.is_yield_mode;
    }

    Task::~Task() {
        if (_task_local && _task_local != (ValueEnvironment*)-1)
            delete _task_local;
        if (_task_local == (ValueEnvironment*)-1)
            delete (task_callback*)args.getSourcePtr();

        if (!started) {
            --glob.planned_tasks;
            if (Task::max_running_tasks)
                glob.can_planned_new_notifier.notify_one();
        }
    }

    void Task::set_auto_bind_worker(bool enable) {
        auto_bind_worker = enable;
        if (enable)
            bind_to_worker_id = -1;
    }

    void Task::set_worker_id(uint16_t id) {
        bind_to_worker_id = id;
        auto_bind_worker = false;
    }

    void Task::schedule(art::typed_lgr<Task>&& task, size_t milliseconds) {
        schedule_until(task, std::chrono::high_resolution_clock::now() + std::chrono::milliseconds(milliseconds));
    }

    void Task::schedule(const art::typed_lgr<Task>& task, size_t milliseconds) {
        schedule_until(task, std::chrono::high_resolution_clock::now() + std::chrono::milliseconds(milliseconds));
    }

    void Task::schedule_until(art::typed_lgr<Task>&& task, std::chrono::high_resolution_clock::time_point time_point) {
        schedule_until(task, time_point);
    }

    void Task::schedule_until(const art::typed_lgr<Task>& task, std::chrono::high_resolution_clock::time_point time_point) {
        art::typed_lgr<Task> lgr_task = task;
        if (lgr_task->started)
            return;
        {
            art::unique_lock guard(glob.task_thread_safety);
            if (lgr_task->started)
                return;
        }
        art::unique_lock guard(glob.task_timer_safety);
        if (can_be_scheduled_task_to_hot())
            unsafe_put_task_to_timed_queue(glob.timed_tasks, time_point, lgr_task);
        else
            unsafe_put_task_to_timed_queue(glob.cold_timed_tasks, time_point, lgr_task);
        lgr_task->started = true;
        glob.tasks_notifier.notify_one();
        guard.unlock();
        if (!glob.time_control_enabled)
            startTimeController();
    }

    void Task::start(list_array<art::typed_lgr<Task>>& lgr_task) {
        for (auto& it : lgr_task)
            start(it);
    }

    void Task::start(art::typed_lgr<Task>&& lgr_task) {
        start(lgr_task);
    }

    bool Task::yield_iterate(art::typed_lgr<Task>& lgr_task) {
        bool res = (!lgr_task->started || lgr_task->is_yield_mode) && lgr_task->_task_local != (ValueEnvironment*)-1;
        if (res)
            Task::start(lgr_task);
        return res;
    }

    ValueItem* Task::get_result(art::typed_lgr<Task>& lgr_task, size_t yield_res) {
        if (!lgr_task->started && lgr_task->_task_local != (ValueEnvironment*)-1)
            Task::start(lgr_task);
        MutexUnify uni(lgr_task->no_race);
        art::unique_lock l(uni);
        if (lgr_task->_task_local == (ValueEnvironment*)-1) {
            l.unlock();
            if (!task_callback::await(*lgr_task)) {
                l.lock();
                lgr_task->fres.awaitEnd(l);
            }
            if (lgr_task->fres.results.size() <= yield_res)
                return new ValueItem();
            return new ValueItem(lgr_task->fres.results[yield_res]);
        } else
            return lgr_task->fres.getResult(yield_res, l);
    }

    ValueItem* Task::get_result(art::typed_lgr<Task>&& lgr_task, size_t yield_res) {
        return get_result(lgr_task, yield_res);
    }

    bool Task::has_result(art::typed_lgr<Task>& lgr_task, size_t yield_res) {
        return lgr_task->fres.results.size() > yield_res;
    }

    void Task::await_task(art::typed_lgr<Task>& lgr_task, bool make_start) {
        if (!lgr_task->started && make_start && lgr_task->_task_local != (ValueEnvironment*)-1)
            Task::start(lgr_task);
        MutexUnify uni(lgr_task->no_race);
        art::unique_lock l(uni);
        if (lgr_task->_task_local != (ValueEnvironment*)-1)
            lgr_task->fres.awaitEnd(l);
        else {
            l.unlock();
            if (!task_callback::await(*lgr_task)) {
                l.lock();
                lgr_task->fres.awaitEnd(l);
            }
        }
    }

    list_array<ValueItem> Task::await_results(art::typed_lgr<Task>& task) {
        await_task(task);
        return task->fres.results;
    }

    list_array<ValueItem> Task::await_results(list_array<art::typed_lgr<Task>>& tasks) {
        list_array<ValueItem> res;
        for (auto& it : tasks)
            res.push_back(await_results(it));
        return res;
    }

    void Task::start(const art::typed_lgr<Task>& tsk) {
        art::typed_lgr<Task> lgr_task = tsk;
        if (lgr_task->started && !lgr_task->is_yield_mode)
            return;
        {
            art::lock_guard guard(glob.task_thread_safety);
            if (lgr_task->started && !lgr_task->is_yield_mode)
                return;
            if (can_be_scheduled_task_to_hot())
                glob.tasks.push(lgr_task);
            else
                glob.cold_tasks.push(lgr_task);
            lgr_task->started = true;
            glob.tasks_notifier.notify_one();
        }
    }

    uint16_t Task::create_bind_only_executor(uint16_t fixed_count, bool allow_implicit_start) {
        art::lock_guard guard(glob.binded_workers_safety);
        uint16_t try_count = 0;
        uint16_t id = glob.binded_workers.size();
    is_not_id:
        while (glob.binded_workers.contains(id)) {
            if (try_count == UINT16_MAX)
                throw InternalException("Too many binded workers");
            try_count++;
            id++;
        }
        if (id == (uint16_t)-1)
            goto is_not_id;
        glob.binded_workers[id].allow_implicit_start = allow_implicit_start;
        glob.binded_workers[id].fixed_size = (bool)fixed_count;
        for (size_t i = 0; i < fixed_count; i++)
            art::thread(bindedTaskExecutor, id).detach();
        return id;
    }

    void Task::close_bind_only_executor(uint16_t id) {
        MutexUnify unify(glob.binded_workers_safety);
        art::unique_lock guard(unify);
        std::list<art::typed_lgr<Task>> transfer_tasks;
        if (!glob.binded_workers.contains(id)) {
            throw InternalException("Binded worker not found");
        } else {
            auto& context = glob.binded_workers[id];
            art::unique_lock context_lock(context.no_race);
            if (context.in_close)
                return;
            context.in_close = true;
            for (uint16_t i = 0; i < context.executors; i++) {
                context.tasks.emplace_back(new Task(nullptr, ValueItem()));
            }
            context.new_task_notifier.notify_all();
            {
                MultiplyMutex mmut{unify, context.no_race};
                MutexUnify mmut_unify(mmut);
                art::unique_lock re_lock(mmut_unify, art::adopt_lock);
                while (context.executors != 0)
                    context.on_closed_notifier.wait(re_lock);
                re_lock.release();
            }
            std::swap(transfer_tasks, context.tasks);
            context_lock.unlock();
            glob.binded_workers.erase(id);
        }
        for (art::typed_lgr<Task>& task : transfer_tasks) {
            task->bind_to_worker_id = -1;
            transfer_task(task);
        }
    }

    void Task::create_executor(size_t count) {
        for (size_t i = 0; i < count; i++)
            art::thread(taskExecutor, false).detach();
    }

    size_t Task::total_executors() {
        art::lock_guard guard(glob.task_thread_safety);
        return glob.executors;
    }

    void Task::reduce_executor(size_t count) {
        ValueItem noting;
        for (size_t i = 0; i < count; i++) {
            start(new Task(nullptr, noting));
        }
    }

    void Task::become_task_executor() {
        try {
            taskExecutor();
            loc.context_in_swap = false;
            loc.is_task_thread = false;
            loc.curr_task = nullptr;
        } catch (...) {
            loc.context_in_swap = false;
            loc.is_task_thread = false;
            loc.curr_task = nullptr;
        }
    }

    void Task::await_no_tasks(bool be_executor) {
        if (be_executor && !loc.is_task_thread)
            taskExecutor(true);
        else {
            MutexUnify uni(glob.task_thread_safety);
            art::unique_lock l(uni);
            while (glob.tasks.size() || glob.cold_tasks.size() || glob.timed_tasks.size() || glob.cold_timed_tasks.size())
                glob.no_tasks_notifier.wait(l);
        }
    }

    void Task::await_end_tasks(bool be_executor) {
        if (be_executor && !loc.is_task_thread) {
            art::unique_lock l(glob.task_thread_safety);
        binded_workers:
            while (glob.tasks.size() || glob.cold_tasks.size() || glob.timed_tasks.size() || glob.cold_timed_tasks.size() || glob.in_exec || glob.tasks_in_swap) {
                l.unlock();
                try {
                    taskExecutor(true);
                } catch (...) {
                    l.lock();
                    throw;
                }
                l.lock();
            }
            art::lock_guard lock(glob.binded_workers_safety);
            bool binded_tasks_empty = true;
            for (auto& contexts : glob.binded_workers)
                if (contexts.second.tasks.size())
                    binded_tasks_empty = false;
            if (!binded_tasks_empty)
                goto binded_workers;
        } else {
        binded_workers_ : {
            MutexUnify uni(glob.task_thread_safety);
            art::unique_lock l(uni);

            if (loc.is_task_thread)
                while ((glob.tasks.size() || glob.cold_tasks.size() || glob.timed_tasks.size() || glob.cold_timed_tasks.size()) && glob.in_exec != 1 && glob.tasks_in_swap != 1)
                    glob.no_tasks_execute_notifier.wait(l);
            else
                while (glob.tasks.size() || glob.cold_tasks.size() || glob.timed_tasks.size() || glob.cold_timed_tasks.size() || glob.in_exec || glob.tasks_in_swap)
                    glob.no_tasks_execute_notifier.wait(l);
        }
            {
                art::lock_guard lock(glob.binded_workers_safety);
                bool binded_tasks_empty = true;
                for (auto& contexts : glob.binded_workers)
                    if (contexts.second.tasks.size())
                        binded_tasks_empty = false;
                if (binded_tasks_empty)
                    return;
            }
            goto binded_workers_;
        }
    }

    void Task::await_multiple(list_array<art::typed_lgr<Task>>& tasks, bool pre_started, bool release) {
        if (!pre_started) {
            for (auto& it : tasks) {
                if (it->_task_local != (ValueEnvironment*)-1)
                    Task::start(it);
            }
        }
        if (release) {
            for (auto& it : tasks) {
                await_task(it, false);
                it = nullptr;
            }
        } else
            for (auto& it : tasks)
                await_task(it, false);
    }

    void Task::await_multiple(art::typed_lgr<Task>* tasks, size_t len, bool pre_started, bool release) {
        if (!pre_started) {
            art::typed_lgr<Task>* iter = tasks;
            size_t count = len;
            while (count--) {
                if ((*iter)->_task_local != (ValueEnvironment*)-1)
                    Task::start(*iter++);
            }
        }
        if (release) {
            while (len--) {
                await_task(*tasks, false);
                (*tasks++) = nullptr;
            }
        } else
            while (len--)
                await_task(*tasks, false);
    }

    void Task::sleep(size_t milliseconds) {
        sleep_until(std::chrono::high_resolution_clock::now() + std::chrono::milliseconds(milliseconds));
    }

    void Task::result(ValueItem* f_res) {
        if (loc.is_task_thread) {
            MutexUnify uni(loc.curr_task->no_race);
            art::unique_lock l(uni);
            loc.curr_task->fres.yieldResult(f_res, l);
        } else
            throw EnvironmentRuinException("Thread attempt return yield result in non task enviro");
    }

    void Task::check_cancellation() {
        if (loc.is_task_thread)
            checkCancellation();
        else
            throw EnvironmentRuinException("Thread attempted check cancellation in non task enviro");
    }

    void Task::self_cancel() {
        if (loc.is_task_thread)
            throw TaskCancellation();
        else
            throw EnvironmentRuinException("Thread attempted cancel self, like task");
    }

    void Task::notify_cancel(art::typed_lgr<Task>& lgr_task) {
        if (lgr_task->_task_local == (ValueEnvironment*)-1)
            task_callback::cancel(*lgr_task);
        else
            lgr_task->make_cancel = true;
    }

    void Task::notify_cancel(list_array<art::typed_lgr<Task>>& tasks) {
        for (auto& it : tasks)
            notify_cancel(it);
    }

    class ValueEnvironment* Task::task_local() {
        if (!loc.is_task_thread)
            return nullptr;
        else if (loc.curr_task->_task_local)
            return loc.curr_task->_task_local;
        else
            return loc.curr_task->_task_local = new ValueEnvironment();
    }

    size_t Task::task_id() {
        if (!loc.is_task_thread)
            return 0;
        else
            return art::hash<size_t>()(reinterpret_cast<size_t>(&*loc.curr_task));
    }

    bool Task::is_task() {
        return loc.is_task_thread;
    }

    void Task::clean_up() {
        Task::await_no_tasks();
        std::queue<art::typed_lgr<Task>> e0;
        std::queue<art::typed_lgr<Task>> e1;
        glob.tasks.swap(e0);
        glob.cold_tasks.swap(e1);
        glob.timed_tasks.shrink_to_fit();
    }

    art::typed_lgr<Task> Task::dummy_task() {
        return new Task(empty_func, ValueItem());
    }

    //first arg bool& check
    //second arg art::condition_variable_any
    ValueItem* _notify_native_thread(ValueItem* args, uint32_t /*ignored*/) {
        *((bool*)args[0].val) = true;
        {
            art::lock_guard l(glob.task_thread_safety);
            glob.in_exec--;
        }
        loc.in_exec_decreased = true;
        ((art::condition_variable_any*)args[1].val)->notify_one();
        return nullptr;
    }

    art::shared_ptr<FuncEnvironment> notify_native_thread(new FuncEnvironment(_notify_native_thread, false, true));

    art::typed_lgr<Task> Task::cxx_native_bridge(bool& checker, art::condition_variable_any& cd) {
        return new Task(notify_native_thread, ValueItem{ValueItem(&checker, VType::undefined_ptr), ValueItem(std::addressof(cd), VType::undefined_ptr)});
    }

    art::typed_lgr<Task> Task::fullifed_task(const list_array<ValueItem>& results) {
        auto task = new Task(empty_func, nullptr);
        task->func = nullptr;
        task->fres.results = results;
        task->end_of_life = true;
        task->started = true;
        task->fres.end_of_life = true;
        return task;
    }

    art::typed_lgr<Task> Task::fullifed_task(list_array<ValueItem>&& results) {
        auto task = new Task(empty_func, nullptr);
        task->func = nullptr;
        task->fres.results = std::move(results);
        task->end_of_life = true;
        task->started = true;
        task->fres.end_of_life = true;
        return task;
    }

    art::typed_lgr<Task> Task::fullifed_task(const ValueItem& result) {
        auto task = new Task(empty_func, nullptr);
        task->func = nullptr;
        task->fres.results = {result};
        task->end_of_life = true;
        task->started = true;
        task->fres.end_of_life = true;
        return task;
    }

    art::typed_lgr<Task> Task::fullifed_task(ValueItem&& result) {
        auto task = new Task(empty_func, nullptr);
        task->func = nullptr;
        task->fres.results = {std::move(result)};
        task->end_of_life = true;
        task->started = true;
        task->fres.end_of_life = true;
        return task;
    }

    class native_task_schedule : public NativeWorkerHandle, public NativeWorkerManager {
        art::shared_ptr<FuncEnvironment> func;
        ValueItem args;
        bool started = false;

    public:
        art::typed_lgr<Task> task;

        native_task_schedule(art::shared_ptr<FuncEnvironment> func)
            : NativeWorkerHandle(this) {
            this->func = func;
            task = Task::dummy_task();
            task->started = true;
        }

        native_task_schedule(art::shared_ptr<FuncEnvironment> func, ValueItem&& arguments)
            : NativeWorkerHandle(this) {
            this->func = func;
            task = Task::dummy_task();
            task->started = true;
            put_arguments(args, std::move(arguments));
        }

        native_task_schedule(art::shared_ptr<FuncEnvironment> func, const ValueItem& arguments)
            : NativeWorkerHandle(this) {
            this->func = func;
            task = Task::dummy_task();
            task->started = true;
            put_arguments(args, arguments);
        }

        native_task_schedule(art::shared_ptr<FuncEnvironment> func, const ValueItem& arguments, ValueItem& dummy_data, void (*on_await)(ValueItem&), void (*on_cancel)(ValueItem&), void (*on_timeout)(ValueItem&), void (*on_destruct)(ValueItem&), std::chrono::high_resolution_clock::time_point timeout)
            : NativeWorkerHandle(this) {
            this->func = func;
            task = Task::callback_dummy(dummy_data, on_await, on_cancel, on_timeout, on_destruct, timeout);
            task->started = true;
            put_arguments(args, arguments);
        }

        native_task_schedule(art::shared_ptr<FuncEnvironment> func, ValueItem&& arguments, ValueItem& dummy_data, void (*on_await)(ValueItem&), void (*on_cancel)(ValueItem&), void (*on_timeout)(ValueItem&), void (*on_destruct)(ValueItem&), std::chrono::high_resolution_clock::time_point timeout)
            : NativeWorkerHandle(this) {
            this->func = func;
            task = Task::callback_dummy(dummy_data, on_await, on_cancel, on_timeout, on_destruct, timeout);
            task->started = true;
            put_arguments(args, std::move(arguments));
        }
#ifdef PLATFORM_WINDOWS
        void handle(void* unused0, NativeWorkerHandle* unused1, unsigned long unused2, bool unused3) {
            actual_handle();
        }
#elif defined(PLATFORM_LINUX)
        void handle(NativeWorkerHandle* unused1, io_uring_cqe* unused) {
            actual_handle();
        }
#else
#error Unsupported platform for native tasks

        void handle() {}
#endif

        void actual_handle() {
            ValueItem* result = nullptr;
            try {
                result = FuncEnvironment::sync_call(func, (ValueItem*)args.val, args.meta.val_len);
            } catch (...) {
                MutexUnify mtx(task->no_race);
                art::unique_lock<MutexUnify> ulock(mtx);
                task->fres.finalResult(std::current_exception(), ulock);
                task->end_of_life = true;
                return;
            }
            MutexUnify mtx(task->no_race);
            art::unique_lock<MutexUnify> ulock(mtx);
            task->fres.finalResult(result, ulock);
            task->end_of_life = true;
            delete this;
        }

        bool start() {
            if (started)
                return false;
#ifdef PLATFORM_WINDOWS
            return started = NativeWorkersSingleton::post_work(this);
#elif defined(PLATFORM_LINUX)
            NativeWorkersSingleton::post_yield(this);
            return started = true;
#else
#error Unsupported platform for native tasks
#endif
        }
    };

    art::typed_lgr<Task> Task::create_native_task(art::shared_ptr<FuncEnvironment> func) {
        native_task_schedule* schedule = new native_task_schedule(func);
        if (!schedule->start()) {
            delete schedule;
            throw AttachARuntimeException("Failed to start native task");
        }
        return schedule->task;
    }

    art::typed_lgr<Task> Task::create_native_task(art::shared_ptr<FuncEnvironment> func, const ValueItem& arguments) {
        native_task_schedule* schedule = new native_task_schedule(func, arguments);
        if (!schedule->start()) {
            delete schedule;
            throw AttachARuntimeException("Failed to start native task");
        }
        return schedule->task;
    }

    art::typed_lgr<Task> Task::create_native_task(art::shared_ptr<FuncEnvironment> func, ValueItem&& arguments) {
        native_task_schedule* schedule = new native_task_schedule(func, std::move(arguments));
        if (!schedule->start()) {
            delete schedule;
            throw AttachARuntimeException("Failed to start native task");
        }
        return schedule->task;
    }

    art::typed_lgr<Task> Task::create_native_task(art::shared_ptr<FuncEnvironment> func, const ValueItem& arguments, ValueItem& dummy_data, void (*on_await)(ValueItem&), void (*on_cancel)(ValueItem&), void (*on_timeout)(ValueItem&), void (*on_destruct)(ValueItem&), std::chrono::high_resolution_clock::time_point timeout) {
        native_task_schedule* schedule = new native_task_schedule(func, arguments, dummy_data, on_await, on_cancel, on_timeout, on_destruct, timeout);
        if (!schedule->start()) {
            delete schedule;
            throw AttachARuntimeException("Failed to start native task");
        }
        return schedule->task;
    }

    art::typed_lgr<Task> Task::create_native_task(art::shared_ptr<FuncEnvironment> func, ValueItem&& arguments, ValueItem& dummy_data, void (*on_await)(ValueItem&), void (*on_cancel)(ValueItem&), void (*on_timeout)(ValueItem&), void (*on_destruct)(ValueItem&), std::chrono::high_resolution_clock::time_point timeout) {
        native_task_schedule* schedule = new native_task_schedule(func, std::move(arguments), dummy_data, on_await, on_cancel, on_timeout, on_destruct, timeout);
        if (!schedule->start()) {
            delete schedule;
            throw AttachARuntimeException("Failed to start native task");
        }
        return schedule->task;
    }

    void Task::explicitStartTimer() {
        startTimeController();
    }

    void Task::shutDown() {
        while (!glob.binded_workers.empty())
            Task::close_bind_only_executor(glob.binded_workers.begin()->first);

        art::unique_lock guard(glob.task_thread_safety);
        size_t executors = glob.executors;
        for (size_t i = 0; i < executors; i++)
            glob.tasks.emplace(new Task(nullptr, {}));
        glob.tasks_notifier.notify_all();
        while (glob.executors)
            glob.executor_shutdown_notifier.wait(guard);
        glob.time_control_enabled = false;
        glob.time_notifier.notify_all();
    }

    art::typed_lgr<Task> Task::callback_dummy(ValueItem& dummy_data, void (*on_start)(ValueItem&), void (*on_await)(ValueItem&), void (*on_cancel)(ValueItem&), void (*on_timeout)(ValueItem&), void (*on_destruct)(ValueItem&), std::chrono::high_resolution_clock::time_point timeout) {
        art::typed_lgr<Task> tsk = new Task(
            empty_func,
            nullptr,
            false,
            nullptr,
            timeout
        );
        tsk->args = new task_callback(dummy_data, on_await, on_cancel, on_timeout, on_start, on_destruct);
        tsk->_task_local = (ValueEnvironment*)-1;
        if (timeout != std::chrono::high_resolution_clock::time_point::min()) {
            if (!glob.time_control_enabled)
                startTimeController();
            art::lock_guard guard(glob.task_timer_safety);
            glob.timed_tasks.push_back(timing(timeout, tsk));
        }
        return tsk;
    }

    art::typed_lgr<Task> Task::callback_dummy(ValueItem& dummy_data, void (*on_await)(ValueItem&), void (*on_cancel)(ValueItem&), void (*on_timeout)(ValueItem&), void (*on_destruct)(ValueItem&), std::chrono::high_resolution_clock::time_point timeout) {
        art::typed_lgr<Task> tsk = new Task(
            empty_func,
            nullptr,
            false,
            nullptr,
            timeout
        );
        tsk->args = new task_callback(dummy_data, on_await, on_cancel, on_timeout, nullptr, on_destruct),
        tsk->_task_local = (ValueEnvironment*)-1;

        if (timeout != std::chrono::high_resolution_clock::time_point::min()) {
            if (!glob.time_control_enabled)
                startTimeController();
            art::lock_guard guard(glob.task_timer_safety);
            glob.timed_tasks.push_back(timing(timeout, tsk));
        }
        return tsk;
    }

#pragma optimize("", off)
#pragma GCC push_options
#pragma GCC optimize("O0")

    void Task::sleep_until(std::chrono::high_resolution_clock::time_point time_point) {
        if (loc.is_task_thread) {
            art::lock_guard guard(loc.curr_task->no_race);
            makeTimeWait(time_point);
            swapCtxRelock(loc.curr_task->no_race);
        } else
            art::this_thread::sleep_until(time_point);
    }

    void Task::yield() {
        if (loc.is_task_thread) {
            art::lock_guard guard(glob.task_thread_safety);
            glob.tasks.push(loc.curr_task);
            swapCtxRelock(glob.task_thread_safety);
        } else
            throw EnvironmentRuinException("Thread attempt return yield task in non task enviro");
    }

#pragma GCC pop_options
#pragma optimize("", on)
}