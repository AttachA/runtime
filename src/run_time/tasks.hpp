// Copyright Danyil Melnytskyi 2022-2023
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once
#ifndef RUN_TIME_TASKS
#include "threading.hpp"
#include <list>
#include "link_garbage_remover.hpp"
#include "../library/list_array.hpp"
#include "attacha_abi_structs.hpp"
#include <chrono>
#include "util/enum_helper.hpp"
#pragma push_macro("min")
#undef min
namespace __{
	struct resume_task{
		typed_lgr<struct Task> task;
		uint16_t awake_check;
	};
}


//it do abort when catched, recomended do rethrow manually and catching by const refrence
class TaskCancellation : AttachARuntimeException {
	bool in_landing = false;
	friend void forceCancelCancellation(TaskCancellation& cancel_token);
public:
	TaskCancellation();
	~TaskCancellation() noexcept(false);
	bool _in_landig();
};



#pragma pack (push)
#pragma pack (1)
class TaskMutex {
	friend class TaskRecursiveMutex;
	std::list<__::resume_task> resume_task;
	art::timed_mutex no_race;
	struct Task* current_task = nullptr;
public:
	TaskMutex() {}
	~TaskMutex();
	void lock();
	bool try_lock();
	bool try_lock_for(size_t milliseconds);
	bool try_lock_until(std::chrono::high_resolution_clock::time_point time_point);
	void unlock();
	bool is_locked();
	//put here child task(not started), and it will lock mutex, and unlock it when it will be finished
	void lifecycle_lock(typed_lgr<struct Task> task);
	//put here child task(not started), and it will lock mutex and relock it when recived value from child task, and unlock it when it will be finished
	void sequence_lock(typed_lgr<struct Task> task);
	bool is_own();
};
class TaskRecursiveMutex{
	TaskMutex mutex;
	uint32_t recursive_count = 0;
public:
	TaskRecursiveMutex() {}
	~TaskRecursiveMutex();
	void lock();
	bool try_lock();
	bool try_lock_for(size_t milliseconds);
	bool try_lock_until(std::chrono::high_resolution_clock::time_point time_point);
	void unlock();
	bool is_locked();
	//put here child task(not started), and it will lock mutex, and unlock it when it will be finished
	void lifecycle_lock(typed_lgr<struct Task> task);
	//put here child task(not started), and it will lock mutex and relock it when recived value from child task, and unlock it when it will be finished
	void sequence_lock(typed_lgr<struct Task> task);
	bool is_own();
};

ENUM_t(MutexUnifyType, uint8_t,
	(noting)
	(nmut)
	(ntimed)
	(nrec)
	(umut)
	(mmut)
);
struct MutexUnify {
	union {
		art::mutex* nmut = nullptr;
		art::timed_mutex* ntimed;
		art::recursive_mutex* nrec;
		TaskMutex* umut;
		struct MultiplyMutex* mmut;
	};
	MutexUnify();
	MutexUnify(const MutexUnify& mut);
	MutexUnify(art::mutex& smut);
	MutexUnify(art::timed_mutex& smut);
	MutexUnify(art::recursive_mutex& smut);
	MutexUnify(TaskMutex& smut);
	MutexUnify(struct MultiplyMutex& mmut);
	MutexUnify(nullptr_t);


	MutexUnify& operator=(const MutexUnify& mut);
	MutexUnify& operator=(art::mutex& smut);
	MutexUnify& operator=(art::timed_mutex& smut);
	MutexUnify& operator=(art::recursive_mutex& smut);
	MutexUnify& operator=(TaskMutex& smut);
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
	bool wait_for(art::unique_lock<MutexUnify>& lock,size_t milliseconds);
	bool wait_until(art::unique_lock<MutexUnify>& lock, std::chrono::high_resolution_clock::time_point time_point);
	void notify_one();
	void notify_all();

	void dummy_wait(typed_lgr<struct Task> task, art::unique_lock<MutexUnify>& lock);
	//set in arguments result of wait_for
	void dummy_wait_for(typed_lgr<struct Task> task, art::unique_lock<MutexUnify>& lock, size_t milliseconds);
	//set in arguments result of wait_until
	void dummy_wait_until(typed_lgr<struct Task> task, art::unique_lock<MutexUnify>& lock, std::chrono::high_resolution_clock::time_point time_point);

	bool has_waiters();
};
struct TaskResult {
	TaskConditionVariable result_notify;
	list_array<ValueItem> results;
	void* context = nullptr;
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
struct Task {
	static size_t max_running_tasks;
	static size_t max_planned_tasks;
	static bool enable_task_naming;

	TaskResult fres;
	typed_lgr<class FuncEnvironment> ex_handle;//if ex_handle is nullptr then exception will be stored in fres
	typed_lgr<class FuncEnvironment> func;
	std::forward_list<typed_lgr<class TaskEnvironment>> _task_envs;//if _task_envs is empty then task use global task enviropment
	ValueItem args;
	art::mutex no_race;
	MutexUnify relock_0;
	MutexUnify relock_1;
	MutexUnify relock_2;
	class ValueEnvironment* _task_local = nullptr;
	std::chrono::high_resolution_clock::time_point timeout = std::chrono::high_resolution_clock::time_point::min();
	uint16_t awake_check = 0;
	uint16_t bind_to_worker_id = -1;//-1 - not binded
	bool time_end_flag : 1 = false;
	bool awaked : 1 = false;
	bool started : 1 = false;
	bool is_yield_mode : 1 = false;
	bool end_of_life : 1 = false;
	bool make_cancel : 1 = false;
	bool auto_bind_worker : 1 = false;//can be binded to regular worker
	Task(typed_lgr<class FuncEnvironment> call_func, const ValueItem& arguments, bool used_task_local = false, typed_lgr<class FuncEnvironment> exception_handler = nullptr, std::chrono::high_resolution_clock::time_point timeout = std::chrono::high_resolution_clock::time_point::min());
	Task(typed_lgr<class FuncEnvironment> call_func, ValueItem&& arguments, bool used_task_local = false, typed_lgr<class FuncEnvironment> exception_handler = nullptr, std::chrono::high_resolution_clock::time_point timeout = std::chrono::high_resolution_clock::time_point::min());
	Task(Task&& mov) noexcept;
	~Task();
	void auto_bind_worker_enable(bool enable = true);
	void set_worker_id(uint16_t id);//disables auto_bind_worker and manualy bind task to worker



	static void start(typed_lgr<Task>&& lgr_task);
	static void start(list_array<typed_lgr<Task>>& lgr_task);
	static void start(const typed_lgr<Task>& lgr_task);
	
	//if count zero then threads count will be dynamicaly calculated
	static uint16_t create_bind_only_executor(uint16_t fixed_count, bool allow_implicit_start);//return id of executor, this worker can't be used for regular tasks, only for binded tasks
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


	static bool yield_iterate(typed_lgr<Task>& lgr_task);
	static ValueItem* get_result(typed_lgr<Task>& lgr_task, size_t yield_res = 0);
	static ValueItem* get_result(typed_lgr<Task>&& lgr_task, size_t yield_res = 0);
	static bool has_result(typed_lgr<Task>& lgr_task, size_t yield_res = 0);
	static void await_task(typed_lgr<Task>& lgr_task, bool make_start = true);
	static void await_multiple(list_array<typed_lgr<Task>>& tasks, bool pre_started = false, bool release = false);
	static void await_multiple(typed_lgr<Task>* tasks, size_t len, bool pre_started = false, bool release = false);
	static list_array<ValueItem> await_results(typed_lgr<Task>& task);
	static list_array<ValueItem> await_results(list_array<typed_lgr<Task>>& tasks);
	static void notify_cancel(typed_lgr<Task>& task);
	static void notify_cancel(list_array<typed_lgr<Task>>& tasks);
	static class ValueEnvironment* task_local();
	static size_t task_id();
	static void check_cancelation();
	static void self_cancel();
	static bool is_task();


	//clean unused memory, used for debug pruproses, ie memory leak
	//not recomended use in production
	static void clean_up();

	static typed_lgr<Task> dummy_task();

	//unsafe function, checker and cd must be alive during task bridge lifetime
	static typed_lgr<Task> cxx_native_bridge(bool& checker, art::condition_variable_any& cd);
	
	static typed_lgr<Task> callback_dummy(ValueItem& dummy_data, void(*on_start)(ValueItem&), void(*on_await)(ValueItem&), void(*on_cancel)(ValueItem&), void(*on_timeout)(ValueItem&), void(*on_destruct)(ValueItem&), std::chrono::high_resolution_clock::time_point timeout = std::chrono::high_resolution_clock::time_point::min());
	static typed_lgr<Task> callback_dummy(ValueItem& dummy_data, void(*on_await)(ValueItem&), void(*on_cancel)(ValueItem&), void(*on_timeout)(ValueItem&), void(*on_destruct)(ValueItem&) , std::chrono::high_resolution_clock::time_point timeout = std::chrono::high_resolution_clock::time_point::min());
	
	static typed_lgr<Task> fullifed_task(const list_array<ValueItem>& results);
	static typed_lgr<Task> fullifed_task(list_array<ValueItem>&& results);
	static typed_lgr<Task> fullifed_task(const ValueItem& result);
	static typed_lgr<Task> fullifed_task(ValueItem&& result);

	static typed_lgr<Task> create_native_task(typed_lgr<class FuncEnvironment> func);
	static typed_lgr<Task> create_native_task(typed_lgr<class FuncEnvironment> func, const ValueItem& arguments);
	static typed_lgr<Task> create_native_task(typed_lgr<class FuncEnvironment> func, ValueItem&& arguments);
	static typed_lgr<Task> create_native_task(typed_lgr<class FuncEnvironment> func, const ValueItem& arguments, ValueItem& dummy_data, void(*on_await)(ValueItem&), void(*on_cancel)(ValueItem&), void(*on_timeout)(ValueItem&), void(*on_destruct)(ValueItem&), std::chrono::high_resolution_clock::time_point timeout = std::chrono::high_resolution_clock::time_point::min());
	static typed_lgr<Task> create_native_task(typed_lgr<class FuncEnvironment> func, ValueItem&& arguments, ValueItem& dummy_data, void(*on_await)(ValueItem&), void(*on_cancel)(ValueItem&), void(*on_timeout)(ValueItem&), void(*on_destruct)(ValueItem&), std::chrono::high_resolution_clock::time_point timeout = std::chrono::high_resolution_clock::time_point::min());

	static void explicitStartTimer();
};
#pragma pack (pop)
class TaskSemaphore {
	std::list<__::resume_task> resume_task;
	art::timed_mutex no_race;
	art::condition_variable native_notify;
	size_t allow_treeshold = 0;
	size_t max_treeshold = 0;
public:
	TaskSemaphore() {}
	void setMaxTreeshold(size_t val);
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
	std::list<typed_lgr<class FuncEnvironment>> heigh_priorihty;
	std::list<typed_lgr<class FuncEnvironment>> upper_avg_priorihty;
	std::list<typed_lgr<class FuncEnvironment>> avg_priorihty;
	std::list<typed_lgr<class FuncEnvironment>> lower_avg_priorihty;
	std::list<typed_lgr<class FuncEnvironment>> low_priorihty;

	std::list<typed_lgr<class FuncEnvironment>> async_heigh_priorihty;
	std::list<typed_lgr<class FuncEnvironment>> async_upper_avg_priorihty;
	std::list<typed_lgr<class FuncEnvironment>> async_avg_priorihty;
	std::list<typed_lgr<class FuncEnvironment>> async_lower_avg_priorihty;
	std::list<typed_lgr<class FuncEnvironment>> async_low_priorihty;

	static bool removeOne(std::list<typed_lgr<class FuncEnvironment>>& list, const typed_lgr<class FuncEnvironment>& func);
	void async_call(std::list<typed_lgr<class FuncEnvironment>>& list, ValueItem& args);
	bool awaitCall(std::list<typed_lgr<class FuncEnvironment>>& list, ValueItem& args);

	bool sync_call(std::list<typed_lgr<class FuncEnvironment>>& list, ValueItem& args);
public:
	enum class Priorithy {
		heigh,
		upper_avg,
		avg,
		lower_avg,
		low
	};
	void operator+=(const typed_lgr<class FuncEnvironment>& func);
	void join(const typed_lgr<class FuncEnvironment>& func, bool async_mode = false, Priorithy priorithy = Priorithy::avg);
	bool leave(const typed_lgr<class FuncEnvironment>& func, bool async_mode = false, Priorithy priorithy = Priorithy::avg);

	bool await_notify(ValueItem& args);
	bool notify(ValueItem& args);
	bool sync_notify(ValueItem& args);
	typed_lgr<Task> async_notify(ValueItem& args);
};
class TaskLimiter {
	list_array<void*> lock_check;
	std::list<__::resume_task> resume_task;
	art::timed_mutex no_race;
	art::condition_variable_any native_notify;
	size_t allow_treeshold = 0;
	size_t max_treeshold = 1;
	bool locked = false;
	void unchecked_unlock();
public:
	TaskLimiter() {}
	void set_max_treeshold(size_t val);
	void lock();
	bool try_lock();
	bool try_lock_for(size_t milliseconds);
	bool try_lock_until(std::chrono::high_resolution_clock::time_point time_point);
	void unlock();
	bool is_locked();
};
struct TaskEnvironment{
	art::mutex no_race;
	std::list<typed_lgr<Task>> awake_list;
	class ValueEnvironment* env = nullptr;
	bool cancelation_token = false;
	bool disabled = false;
	size_t max_work = 0;
	size_t in_work = 0;
	~TaskEnvironment();
	TaskEnvironment() = default;
	bool check_cancelation();
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

class TaskQuery{
	std::list<typed_lgr<Task>> tasks;
	class TaskQueryHandle* handle;
	bool is_running;
	friend void __TaskQuery_add_task_leave(class TaskQueryHandle* tqh, TaskQuery* tq);
public:
	TaskQuery(size_t at_execution_max = 0);
	~TaskQuery();
	typed_lgr<Task> add_task(typed_lgr<class FuncEnvironment> call_func, ValueItem& arguments, bool used_task_local = false, typed_lgr<class FuncEnvironment> exception_handler = nullptr, std::chrono::high_resolution_clock::time_point timeout = std::chrono::high_resolution_clock::time_point::min());
	void enable();
	void disable();
	bool in_query(typed_lgr<Task> task);
	void set_max_at_execution(size_t val);
	size_t get_max_at_execution();
	void wait();
	bool wait_for(size_t milliseconds);
	bool wait_until(std::chrono::high_resolution_clock::time_point time_point);
};

//task unsafe, TO-DO: compatible with task sync classes
class Generator {
	friend void prepare_generator(ValueItem& args,typed_lgr<FuncEnvironment>& func, typed_lgr<FuncEnvironment>& ex_handler, Generator*& weak_ref);
	list_array<ValueItem*> results;
	typed_lgr<class FuncEnvironment> ex_handle;//if ex_handle is nullptr then exception will be unrolled to caller
	typed_lgr<class FuncEnvironment> func;
	ValueItem args;
	class ValueEnvironment* _generator_local = nullptr;
	std::exception_ptr ex_ptr = nullptr;
	void* context = nullptr;
	bool end_of_life : 1 = false;
public:
	Generator(typed_lgr<class FuncEnvironment> call_func, const ValueItem& arguments, bool used_generator_local = false, typed_lgr<class FuncEnvironment> exception_handler = nullptr);
	Generator(typed_lgr<class FuncEnvironment> call_func, ValueItem&& arguments, bool used_generator_local = false, typed_lgr<class FuncEnvironment> exception_handler = nullptr);
	Generator(Generator&& mov) noexcept;
	~Generator();

	static bool yield_iterate(typed_lgr<Generator>& lgr_task);
	static ValueItem* get_result(typed_lgr<Generator>& lgr_task);
	static bool has_result(typed_lgr<Generator>& lgr_task);
	static list_array<ValueItem*> await_results(typed_lgr<Generator>& task);
	static list_array<ValueItem*> await_results(list_array<typed_lgr<Generator>>& tasks);


	//in generators use
	static class ValueEnvironment* generator_local(Generator* generator_weak_ref);
	static void yield(Generator* generator_weak_ref, ValueItem* result);
	static void result(Generator* generator_weak_ref, ValueItem* result);

	//internal
	static void back_unwind(Generator* generator_weak_ref, std::exception_ptr&& ex_ptr);
	static void return_(Generator* generator_weak_ref, ValueItem* result);
};




//internal
namespace _Task_unsafe{
	void ctxSwap();
	void ctxSwapRelock(const MutexUnify& lock0);
	void ctxSwapRelock(const MutexUnify& lock0, const MutexUnify& lock1);
	void ctxSwapRelock(const MutexUnify& lock0, const MutexUnify& lock1, const MutexUnify& lock2);
	typed_lgr<Task> get_self();
	void become_executor_count_manager(bool leave_after_finish);
	void start_executor_count_manager();
	
}
#pragma pop_macro("min")
#endif