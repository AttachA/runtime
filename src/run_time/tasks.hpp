// Copyright Danyil Melnytskyi 2022-Present
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once
#ifndef RUN_TIME_TASKS
#include <chrono>
#include <forward_list>
#include <list>

#include <library/list_array.hpp>
#include <run_time/attacha_abi_structs.hpp>
#include <util/enum_helper.hpp>
#include <util/link_garbage_remover.hpp>
#include <util/threading.hpp>

#pragma push_macro("min")
#undef min

namespace art {
    namespace __ {
        struct resume_task {
            art::typed_lgr<Task> task;
            uint16_t awake_check;
        };
    }

    //it do abort when catched, recommended do rethrow manually and catching by const reference
    class TaskCancellation : AttachARuntimeException {
        bool in_landing = false;
        friend void forceCancelCancellation(TaskCancellation& cancel_token);

    public:
        TaskCancellation();
        ~TaskCancellation() noexcept(false);
        bool _in_landing();
    };

    class GeneratorRestart : AttachARuntimeException {
        bool in_landing = false;
        friend void forceCancelCancellation(GeneratorRestart& cancel_token);

    public:
        GeneratorRestart();
        ~GeneratorRestart() noexcept(false);
        bool _in_landing();
    };

#pragma pack(push)
#pragma pack(1)

    class TaskMutex {
        friend class TaskRecursiveMutex;
        std::list<__::resume_task> resume_task;
        art::timed_mutex no_race;
        struct Task* current_task = nullptr;

    public:
        TaskMutex() = default;

        ~TaskMutex();
        void lock();
        bool try_lock();
        bool try_lock_for(size_t milliseconds);
        bool try_lock_until(std::chrono::high_resolution_clock::time_point time_point);
        void unlock();
        bool is_locked();
        //put here child task(not started), and it will lock mutex, and unlock it when it will be finished
        void lifecycle_lock(const art::typed_lgr<Task>& task);
        //put here child task(not started), and it will lock mutex and relock it when received value from child task, and unlock it when it will be finished
        void sequence_lock(const art::typed_lgr<Task>& task);
        bool is_own();
    };

    class TaskRecursiveMutex {
        TaskMutex mutex;
        uint32_t recursive_count = 0;

    public:
        TaskRecursiveMutex() = default;

        ~TaskRecursiveMutex() noexcept(false);
        void lock();
        bool try_lock();
        bool try_lock_for(size_t milliseconds);
        bool try_lock_until(std::chrono::high_resolution_clock::time_point time_point);
        void unlock();
        bool is_locked();
        //put here child task(not started), and it will lock mutex, and unlock it when it will be finished
        void lifecycle_lock(const art::typed_lgr<Task>& task);
        //put here child task(not started), and it will lock mutex and relock it when received value from child task, and unlock it when it will be finished
        void sequence_lock(const art::typed_lgr<Task>& task);
        bool is_own();

        relock_state relock_begin();
        void relock_end(relock_state state);
    };

    class TaskRWMutex {
        friend class TaskRecursiveMutex;
        std::list<__::resume_task> resume_task;
        std::list<Task*> readers;
        art::timed_mutex no_race;
        struct Task* current_writer_task = nullptr;


    public:
        TaskRWMutex() = default;

        ~TaskRWMutex();
        void read_lock();
        bool try_read_lock();
        bool try_read_lock_for(size_t milliseconds);
        bool try_read_lock_until(std::chrono::high_resolution_clock::time_point time_point);
        void read_unlock();
        bool is_read_locked();
        //put here child task(not started), and it will lock mutex, and unlock it when it will be finished
        void lifecycle_read_lock(const art::typed_lgr<Task>& task);
        //put here child task(not started), and it will lock mutex and relock it when received value from child task, and unlock it when it will be finished
        void sequence_read_lock(const art::typed_lgr<Task>& task);

        void write_lock();
        bool try_write_lock();
        bool try_write_lock_for(size_t milliseconds);
        bool try_write_lock_until(std::chrono::high_resolution_clock::time_point time_point);
        void write_unlock();
        bool is_write_locked();
        //put here child task(not started), and it will lock mutex, and unlock it when it will be finished
        void lifecycle_write_lock(const art::typed_lgr<Task>& task);
        //put here child task(not started), and it will lock mutex and relock it when received value from child task, and unlock it when it will be finished
        void sequence_write_lock(const art::typed_lgr<Task>& task);

        bool is_own();
    };

    ENUM_t(
        MutexUnifyType,
        uint8_t,
        noting,
        nmut,
        nrwmut_r,
        nrwmut_w,
        ntimed,
        nrec,
        umut,
        urmut,
        urwmut_r,
        urwmut_w,
        mmut
    );

    struct MutexUnify {
        union {
            art::mutex* nmut = nullptr;
            art::rw_mutex* nrwmut;
            art::timed_mutex* ntimed;
            art::recursive_mutex* nrec;
            TaskMutex* umut;
            TaskRWMutex* urwmut;
            TaskRecursiveMutex* urmut;
            struct MultiplyMutex* mmut;
        };

        MutexUnify();
        MutexUnify(const MutexUnify& mut);
        MutexUnify(art::mutex& smut);
        MutexUnify(art::rw_mutex& smut, bool read_write = true);
        MutexUnify(art::timed_mutex& smut);
        MutexUnify(art::recursive_mutex& smut);
        MutexUnify(TaskMutex& smut);
        MutexUnify(TaskRWMutex& smut, bool read_write = true);
        MutexUnify(TaskRecursiveMutex& smut);
        MutexUnify(struct MultiplyMutex& mmut);
        MutexUnify(nullptr_t);

        ~MutexUnify();

        MutexUnify& operator=(const MutexUnify& mut);
        MutexUnify& operator=(art::mutex& smut);
        MutexUnify& operator=(art::rw_mutex& smut);
        MutexUnify& operator=(art::timed_mutex& smut);
        MutexUnify& operator=(art::recursive_mutex& smut);
        MutexUnify& operator=(TaskMutex& smut);
        MutexUnify& operator=(TaskRecursiveMutex& smut);
        MutexUnify& operator=(struct MultiplyMutex& mmut);
        MutexUnify& operator=(nullptr_t);

        MutexUnifyType type;
        art::relock_state state;
        void lock();
        bool try_lock();
        bool try_lock_for(size_t milliseconds);
        bool try_lock_until(std::chrono::high_resolution_clock::time_point time_point);
        void unlock();

        void relock_start();
        void relock_end();

        operator bool();
    };

    struct MultiplyMutex {
        list_array<MutexUnify> mu;
        MultiplyMutex(const std::initializer_list<MutexUnify>& muts);
        void lock();
        bool try_lock();
        bool try_lock_for(size_t milliseconds);
        bool try_lock_until(std::chrono::high_resolution_clock::time_point time_point);
        void unlock();
    };

    class TaskConditionVariable {
        std::list<__::resume_task> resume_task;
        art::mutex no_race;

    public:
        TaskConditionVariable();
        ~TaskConditionVariable();
        void wait(art::unique_lock<MutexUnify>& lock);
        bool wait_for(art::unique_lock<MutexUnify>& lock, size_t milliseconds);
        bool wait_until(art::unique_lock<MutexUnify>& lock, std::chrono::high_resolution_clock::time_point time_point);
        void notify_one();
        void notify_all();

        void dummy_wait(const art::typed_lgr<Task>& task, art::unique_lock<MutexUnify>& lock);
        //set in arguments result of wait_for
        void dummy_wait_for(const art::typed_lgr<Task>& task, art::unique_lock<MutexUnify>& lock, size_t milliseconds);
        //set in arguments result of wait_until
        void dummy_wait_until(const art::typed_lgr<Task>& task, art::unique_lock<MutexUnify>& lock, std::chrono::high_resolution_clock::time_point time_point);

        bool has_waiters();
    };

    struct TaskResult {
        TaskConditionVariable result_notify;
        list_array<ValueItem> results;
        struct task_context* context = nullptr;
        bool end_of_life = false;
        ValueItem* getResult(size_t res_num, art::unique_lock<MutexUnify>& l);
        void awaitEnd(art::unique_lock<MutexUnify>& l);
        void yieldResult(ValueItem* res, art::unique_lock<MutexUnify>& l, bool release = true);
        void yieldResult(ValueItem&& res, art::unique_lock<MutexUnify>& l);
        void finalResult(ValueItem* res, art::unique_lock<MutexUnify>& l);
        void finalResult(ValueItem&& res, art::unique_lock<MutexUnify>& l);
        TaskResult();
        TaskResult(TaskResult&& move) noexcept;
        ~TaskResult();
    };
    enum class TaskPriority {
        background,
        low,
        lower,
        normal,
        higher,
        high,
        realtime
    };

    struct Task {
        static size_t max_running_tasks;
        static size_t max_planned_tasks;
        static bool enable_task_naming;

        TaskResult fres;
        art::shared_ptr<FuncEnvironment> ex_handle; //if ex_handle is nullptr then exception will be stored in fres
        art::shared_ptr<FuncEnvironment> func;
        std::forward_list<art::shared_ptr<struct TaskEnvironment>> _task_envs; //if _task_envs is empty then task use global task environment
        ValueItem args;
        art::mutex no_race;
        MutexUnify relock_0;
        MutexUnify relock_1;
        MutexUnify relock_2;
        class ValueEnvironment* _task_local = nullptr;
        std::chrono::high_resolution_clock::time_point timeout = std::chrono::high_resolution_clock::time_point::min();
        uint16_t awake_check = 0;
        uint16_t bind_to_worker_id = -1; //-1 - not binded
        bool time_end_flag : 1 = false;
        bool awaked : 1 = false;
        bool started : 1 = false;
        bool is_yield_mode : 1 = false;
        bool end_of_life : 1 = false;
        bool make_cancel : 1 = false;
        bool auto_bind_worker : 1 = false; //can be binded to regular worker
        bool invalid_switch_caught : 1 = false;
        Task(art::shared_ptr<FuncEnvironment> call_func, const ValueItem& arguments, bool used_task_local = false, art::shared_ptr<FuncEnvironment> exception_handler = nullptr, std::chrono::high_resolution_clock::time_point timeout = std::chrono::high_resolution_clock::time_point::min(), TaskPriority priority = TaskPriority::normal);
        Task(art::shared_ptr<FuncEnvironment> call_func, ValueItem&& arguments, bool used_task_local = false, art::shared_ptr<FuncEnvironment> exception_handler = nullptr, std::chrono::high_resolution_clock::time_point timeout = std::chrono::high_resolution_clock::time_point::min(), TaskPriority priority = TaskPriority::normal);
        Task(Task&& mov) noexcept;
        ~Task();
        void set_auto_bind_worker(bool enable = true);
        void set_worker_id(uint16_t id); //disables auto_bind_worker and manually bind task to worker

        static void schedule(art::typed_lgr<Task>&& task, size_t milliseconds);
        static void schedule(const art::typed_lgr<Task>& task, size_t milliseconds);
        static void schedule_until(art::typed_lgr<Task>&& task, std::chrono::high_resolution_clock::time_point time_point);
        static void schedule_until(const art::typed_lgr<Task>& task, std::chrono::high_resolution_clock::time_point time_point);
        static void start(art::typed_lgr<Task>&& lgr_task);
        static void start(list_array<art::typed_lgr<Task>>& lgr_task);
        static void start(const art::typed_lgr<Task>& lgr_task);

        //if count zero then threads count will be dynamically calculated
        static uint16_t create_bind_only_executor(uint16_t fixed_count, bool allow_implicit_start); //return id of executor, this worker can't be used for regular tasks, only for binded tasks
        static void close_bind_only_executor(uint16_t id);

        static void create_executor(size_t count = 1);
        static size_t total_executors();
        static void reduce_executor(size_t count = 1);
        static void become_task_executor();

        static void await_no_tasks(bool be_executor = false);
        static void await_end_tasks(bool be_executor = false);
        static void sleep(size_t milliseconds);
        static void sleep_until(std::chrono::high_resolution_clock::time_point time_point);
        static void result(ValueItem* f_res);
        static void yield();


        static bool yield_iterate(art::typed_lgr<Task>& lgr_task);
        static ValueItem* get_result(art::typed_lgr<Task>& lgr_task, size_t yield_res = 0);
        static ValueItem* get_result(art::typed_lgr<Task>&& lgr_task, size_t yield_res = 0);
        static bool has_result(art::typed_lgr<Task>& lgr_task, size_t yield_res = 0);
        static void await_task(art::typed_lgr<Task>& lgr_task, bool make_start = true);
        static void await_multiple(list_array<art::typed_lgr<Task>>& tasks, bool pre_started = false, bool release = false);
        static void await_multiple(art::typed_lgr<Task>* tasks, size_t len, bool pre_started = false, bool release = false);
        static list_array<ValueItem> await_results(art::typed_lgr<Task>& task);
        static list_array<ValueItem> await_results(list_array<art::typed_lgr<Task>>& tasks);
        static void notify_cancel(art::typed_lgr<Task>& task);
        static void notify_cancel(list_array<art::typed_lgr<Task>>& tasks);
        static class ValueEnvironment* task_local();
        static size_t task_id();
        static void check_cancellation();
        static void self_cancel();
        static bool is_task();


        //clean unused memory, used for debug purposes, ie memory leak
        //not recommended use in production
        static void clean_up();

        static art::typed_lgr<Task> dummy_task();

        //unsafe function, checker and cd must be alive during task bridge lifetime
        static art::typed_lgr<Task> cxx_native_bridge(bool& checker, art::condition_variable_any& cd);

        static art::typed_lgr<Task> callback_dummy(ValueItem& dummy_data, void (*on_start)(ValueItem&), void (*on_await)(ValueItem&), void (*on_cancel)(ValueItem&), void (*on_timeout)(ValueItem&), void (*on_destruct)(ValueItem&), std::chrono::high_resolution_clock::time_point timeout = std::chrono::high_resolution_clock::time_point::min());
        static art::typed_lgr<Task> callback_dummy(ValueItem& dummy_data, void (*on_await)(ValueItem&), void (*on_cancel)(ValueItem&), void (*on_timeout)(ValueItem&), void (*on_destruct)(ValueItem&), std::chrono::high_resolution_clock::time_point timeout = std::chrono::high_resolution_clock::time_point::min());

        static art::typed_lgr<Task> fullifed_task(const list_array<ValueItem>& results);
        static art::typed_lgr<Task> fullifed_task(list_array<ValueItem>&& results);
        static art::typed_lgr<Task> fullifed_task(const ValueItem& result);
        static art::typed_lgr<Task> fullifed_task(ValueItem&& result);

        static art::typed_lgr<Task> create_native_task(art::shared_ptr<FuncEnvironment> func);
        static art::typed_lgr<Task> create_native_task(art::shared_ptr<FuncEnvironment> func, const ValueItem& arguments);
        static art::typed_lgr<Task> create_native_task(art::shared_ptr<FuncEnvironment> func, ValueItem&& arguments);
        static art::typed_lgr<Task> create_native_task(art::shared_ptr<FuncEnvironment> func, const ValueItem& arguments, ValueItem& dummy_data, void (*on_await)(ValueItem&), void (*on_cancel)(ValueItem&), void (*on_timeout)(ValueItem&), void (*on_destruct)(ValueItem&), std::chrono::high_resolution_clock::time_point timeout = std::chrono::high_resolution_clock::time_point::min());
        static art::typed_lgr<Task> create_native_task(art::shared_ptr<FuncEnvironment> func, ValueItem&& arguments, ValueItem& dummy_data, void (*on_await)(ValueItem&), void (*on_cancel)(ValueItem&), void (*on_timeout)(ValueItem&), void (*on_destruct)(ValueItem&), std::chrono::high_resolution_clock::time_point timeout = std::chrono::high_resolution_clock::time_point::min());

        static void explicitStartTimer();
        static void shutDown();


        static void become_executor_count_manager(bool leave_after_finish);
        static void start_executor_count_manager();
        static void start_interrupt_handler();
    };

#pragma pack(pop)

    class TaskSemaphore {
        std::list<__::resume_task> resume_task;
        art::timed_mutex no_race;
        art::condition_variable native_notify;
        size_t allow_threshold = 0;
        size_t max_threshold = 0;

    public:
        TaskSemaphore() = default;

        void setMaxThreshold(size_t val);
        void lock();
        bool try_lock();
        bool try_lock_for(size_t milliseconds);
        bool try_lock_until(std::chrono::high_resolution_clock::time_point time_point);
        void release();
        void release_all();
        bool is_locked();
    };

    class EventSystem {
        friend ValueItem* __async_notify(ValueItem* vals, uint32_t);
        TaskMutex no_race;
        std::list<art::shared_ptr<FuncEnvironment>> heigh_priority;
        std::list<art::shared_ptr<FuncEnvironment>> upper_avg_priority;
        std::list<art::shared_ptr<FuncEnvironment>> avg_priority;
        std::list<art::shared_ptr<FuncEnvironment>> lower_avg_priority;
        std::list<art::shared_ptr<FuncEnvironment>> low_priority;

        std::list<art::shared_ptr<FuncEnvironment>> async_heigh_priority;
        std::list<art::shared_ptr<FuncEnvironment>> async_upper_avg_priority;
        std::list<art::shared_ptr<FuncEnvironment>> async_avg_priority;
        std::list<art::shared_ptr<FuncEnvironment>> async_lower_avg_priority;
        std::list<art::shared_ptr<FuncEnvironment>> async_low_priority;

        static bool removeOne(std::list<art::shared_ptr<FuncEnvironment>>& list, const art::shared_ptr<FuncEnvironment>& func);
        void async_call(std::list<art::shared_ptr<FuncEnvironment>>& list, ValueItem& args);
        bool awaitCall(std::list<art::shared_ptr<FuncEnvironment>>& list, ValueItem& args);

        bool sync_call(std::list<art::shared_ptr<FuncEnvironment>>& list, ValueItem& args);

    public:
        enum class Priority {
            heigh,
            upper_avg,
            avg,
            lower_avg,
            low
        };
        void operator+=(const art::shared_ptr<FuncEnvironment>& func);
        void join(const art::shared_ptr<FuncEnvironment>& func, bool async_mode = false, Priority priority = Priority::avg);
        bool leave(const art::shared_ptr<FuncEnvironment>& func, bool async_mode = false, Priority priority = Priority::avg);

        bool await_notify(ValueItem& args);
        bool notify(ValueItem& args);
        bool sync_notify(ValueItem& args);
        art::typed_lgr<Task> async_notify(ValueItem& args);

        void clear() {
            heigh_priority.clear();
            upper_avg_priority.clear();
            avg_priority.clear();
            lower_avg_priority.clear();
            low_priority.clear();

            async_heigh_priority.clear();
            async_upper_avg_priority.clear();
            async_avg_priority.clear();
            async_lower_avg_priority.clear();
            async_low_priority.clear();
        }
    };

    class TaskLimiter {
        list_array<void*> lock_check;
        std::list<__::resume_task> resume_task;
        art::timed_mutex no_race;
        art::condition_variable_any native_notify;
        size_t allow_threshold = 0;
        size_t max_threshold = 1;
        bool locked = false;
        void unchecked_unlock();

    public:
        TaskLimiter() = default;

        void set_max_threshold(size_t val);
        void lock();
        bool try_lock();
        bool try_lock_for(size_t milliseconds);
        bool try_lock_until(std::chrono::high_resolution_clock::time_point time_point);
        void unlock();
        bool is_locked();
    };

    struct TaskEnvironment {
        art::mutex no_race;
        std::list<art::typed_lgr<Task>> awake_list;
        class ValueEnvironment* env = nullptr;
        bool cancellation_token = false;
        bool disabled = false;
        size_t max_work = 0;
        size_t in_work = 0;
        ~TaskEnvironment();
        TaskEnvironment() = default;
        bool check_cancellation();
        void _awake();
        bool can_i_work();
        void set_max_work(size_t new_max_work);

    private:
        bool _can_i_work();
        void if_i_can_awoke_me();
        void update_awoke_list();
        void awoke_item();
        void awoke_all();
    };

    class TaskQuery {
        std::list<art::typed_lgr<Task>> tasks;
        class TaskQueryHandle* handle;
        bool is_running;
        friend void __TaskQuery_add_task_leave(class TaskQueryHandle* tqh, TaskQuery* tq);

    public:
        TaskQuery(size_t at_execution_max = 0);
        ~TaskQuery();
        art::typed_lgr<Task> add_task(art::shared_ptr<FuncEnvironment> call_func, ValueItem& arguments, bool used_task_local = false, art::shared_ptr<FuncEnvironment> exception_handler = nullptr, std::chrono::high_resolution_clock::time_point timeout = std::chrono::high_resolution_clock::time_point::min());
        void enable();
        void disable();
        bool in_query(const art::typed_lgr<Task>& task);
        void set_max_at_execution(size_t val);
        size_t get_max_at_execution();
        void wait();
        bool wait_for(size_t milliseconds);
        bool wait_until(std::chrono::high_resolution_clock::time_point time_point);
    };

    //task unsafe, TO-DO: compatible with task sync classes
    class Generator {
        friend void prepare_generator(ValueItem& args, art::shared_ptr<FuncEnvironment>& func, art::shared_ptr<FuncEnvironment>& ex_handler, Generator*& weak_ref);
        friend void*& __Generator_get_context(Generator* generator_weak_ref);
        list_array<ValueItem*> results;
        art::shared_ptr<FuncEnvironment> ex_handle; //if ex_handle is nullptr then exception will be unrolled to caller
        art::shared_ptr<FuncEnvironment> func;
        ValueItem args;
        class ValueEnvironment* _generator_local = nullptr;
        std::exception_ptr ex_ptr = nullptr;
        void* context = nullptr;
        bool end_of_life : 1 = false;
        bool cancellation_flag : 1 = false;
        bool restart_flag : 1 = false;

        struct iterator {
            art::shared_ptr<Generator> generator;

            iterator(art::shared_ptr<Generator> generator);
            iterator(const iterator& mov);
            iterator(iterator&& mov) noexcept;
            iterator& operator=(const iterator& mov);
            iterator& operator=(iterator&& mov) noexcept;
            bool operator==(const iterator& mov);
            bool operator!=(const iterator& mov);
            iterator& operator++();
            ValueItem operator*();
            iterator begin();
            iterator end();
        };

    public:
        Generator(art::shared_ptr<FuncEnvironment> call_func, const ValueItem& arguments, bool used_generator_local = false, art::shared_ptr<FuncEnvironment> exception_handler = nullptr);
        Generator(art::shared_ptr<FuncEnvironment> call_func, ValueItem&& arguments, bool used_generator_local = false, art::shared_ptr<FuncEnvironment> exception_handler = nullptr);
        Generator(Generator&& mov) noexcept;
        ~Generator();

        static bool yield_iterate(art::shared_ptr<Generator>& gen);
        static bool execute(art::shared_ptr<Generator>& gen);
        static ValueItem* get_result(art::shared_ptr<Generator>& gen);
        static list_array<ValueItem*> get_results(art::shared_ptr<Generator>& gen);
        static bool has_result(art::shared_ptr<Generator>& gen);
        static iterator cxx_iterate(art::shared_ptr<Generator>& gen);
        static list_array<ValueItem> await_results(art::shared_ptr<Generator>& gen);
        static list_array<ValueItem> await_results(list_array<art::shared_ptr<Generator>>& gens);
        static void restart_context(art::shared_ptr<Generator>& gen);


        //in generators use
        static class ValueEnvironment* generator_local(Generator* generator_weak_ref);
        static void yield(Generator* generator_weak_ref, ValueItem* result);
        static void result(Generator* generator_weak_ref, ValueItem* result);

        //internal
        static void back_unwind(Generator* generator_weak_ref, std::exception_ptr&& ex_ptr);
        static void back_cancel(Generator* generator_weak_ref);
        static void return_(Generator* generator_weak_ref, ValueItem* result);
    };

#pragma pop_macro("min")
}
#endif