// Copyright Danyil Melnytskyi 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once
#include <mutex>
#include <fstream>
#include <list>
#include "link_garbage_remover.hpp"
#include "../library/list_array.hpp"
#include "attacha_abi_structs.hpp"
#include <chrono>
#include "util/enum_helper.hpp"
#pragma push_macro("min")
#undef min

//it do abort when catched, recomended do rethrow manually and catching by const refrence
class TaskCancellation : AttachARuntimeException {
	bool in_landing = false;
	friend void forceCancelCancellation(TaskCancellation& cancel_token);
public:
	TaskCancellation();
	~TaskCancellation() noexcept(false);
	bool _in_landig();
};






class TaskMutex {
	std::timed_mutex no_race;
	std::list<typed_lgr<struct Task>> resume_task;
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
};

ENUM_t(MutexUnifyType, uint8_t,
	(noting)
	(nmut)
	(ntimed)
	(nrec)
	(umut)
	(mmut)
);
#pragma pack (push)
#pragma pack (1)
struct MutexUnify {
	union {
		std::mutex* nmut = nullptr;
		std::timed_mutex* ntimed;
		std::recursive_mutex* nrec;
		TaskMutex* umut;
		struct MultiplyMutex* mmut;
	};
	MutexUnify();
	MutexUnify(const MutexUnify& mut);
	MutexUnify(std::mutex& smut);
	MutexUnify(std::timed_mutex& smut);
	MutexUnify(std::recursive_mutex& smut);
	MutexUnify(TaskMutex& smut);
	MutexUnify(struct MultiplyMutex& mmut);
	MutexUnify(nullptr_t);


	MutexUnify& operator=(const MutexUnify& mut);
	MutexUnify& operator=(std::mutex& smut);
	MutexUnify& operator=(std::timed_mutex& smut);
	MutexUnify& operator=(std::recursive_mutex& smut);
	MutexUnify& operator=(TaskMutex& smut);
	MutexUnify& operator=(struct MultiplyMutex& mmut);
	MutexUnify& operator=(nullptr_t);

	MutexUnifyType type;
	void lock();
	bool try_lock();
	bool try_lock_for(size_t milliseconds);
	bool try_lock_until(std::chrono::high_resolution_clock::time_point time_point);
	void unlock();
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
	std::list<typed_lgr<struct Task>> resume_task;
	std::mutex no_race;
public:
	TaskConditionVariable();
	~TaskConditionVariable();
	void wait(std::unique_lock<MutexUnify>& lock);
	bool wait_for(std::unique_lock<MutexUnify>& lock,size_t milliseconds);
	bool wait_until(std::unique_lock<MutexUnify>& lock, std::chrono::high_resolution_clock::time_point time_point);
	void notify_one();
	void notify_all();
};
struct TaskResult {
	TaskConditionVariable result_notify;
	list_array<ValueItem> results;
	void* context = nullptr;
	bool end_of_life = false;
	ValueItem* getResult(size_t res_num, std::unique_lock<MutexUnify>& l);
	void awaitEnd(std::unique_lock<MutexUnify>& l);
	void yieldResult(ValueItem* res, std::unique_lock<MutexUnify>& l, bool release = true);
	void yieldResult(ValueItem&& res, std::unique_lock<MutexUnify>& l);
	void finalResult(ValueItem* res, std::unique_lock<MutexUnify>& l);
	void finalResult(ValueItem&& res, std::unique_lock<MutexUnify>& l);
	TaskResult();
	TaskResult(TaskResult&& move) noexcept;
	~TaskResult();
};
struct Task {
	static size_t max_running_tasks;
	static size_t max_planned_tasks;


	TaskResult fres;
	std::mutex no_race;
	typed_lgr<class FuncEnviropment> ex_handle;//if ex_handle is nullptr then exception will be stored in fres
	typed_lgr<class FuncEnviropment> func;
	ValueItem args;
	MutexUnify relock_0;
	MutexUnify relock_1;
	MutexUnify relock_2;
	class ValueEnvironment* _task_local = nullptr;
	//size_t sleep_check = 0;
	std::chrono::high_resolution_clock::time_point timeout = std::chrono::high_resolution_clock::time_point::min();
	bool time_end_flag : 1 = false;
	bool awaked : 1 = false;
	bool started : 1 = false;
	bool is_yield_mode : 1 = false;
	bool end_of_life : 1 = false;
	bool make_cancel : 1 = false;
	Task(typed_lgr<class FuncEnviropment> call_func, const ValueItem& arguments, bool used_task_local = false, typed_lgr<class FuncEnviropment> exception_handler = nullptr, std::chrono::high_resolution_clock::time_point timeout = std::chrono::high_resolution_clock::time_point::min());
	Task(typed_lgr<class FuncEnviropment> call_func, ValueItem&& arguments, bool used_task_local = false, typed_lgr<class FuncEnviropment> exception_handler = nullptr, std::chrono::high_resolution_clock::time_point timeout = std::chrono::high_resolution_clock::time_point::min());
	Task(Task&& mov) noexcept;
	~Task();

	static void start(typed_lgr<Task>&& lgr_task);
	static void start(list_array<typed_lgr<Task>>& lgr_task);
	static void start(const typed_lgr<Task>& lgr_task);

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
	static void await_task(typed_lgr<Task>& lgr_task, bool in_place = false);
	static void await_multiple(list_array<typed_lgr<Task>>& tasks, bool pre_started = false, bool release = false);
	static void await_multiple(typed_lgr<Task>* tasks, size_t len, bool pre_started = false, bool release = false);
	static list_array<ValueItem> await_results(typed_lgr<Task>& task);
	static list_array<ValueItem> await_results(list_array<typed_lgr<Task>>& tasks);
	static class ValueEnvironment* task_local();
	static size_t task_id();
	static void check_cancelation();
	static void self_cancel();
	static bool is_task();


	//clean unused memory, used for debug pruproses, ie memory leak
	//not recomended use in production
	static void clean_up();
};
#pragma pack (pop)
class TaskSemaphore {
	std::list<typed_lgr<Task>> resume_task;
	std::timed_mutex no_race;
	std::condition_variable native_notify;
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
struct ConcurentFile {
	TaskMutex no_race;
	std::fstream stream;
	bool last_op_append = false;
	ConcurentFile(const char* path);
	~ConcurentFile();

	//all pointers below will not be released, it do automatically
	static typed_lgr<Task> read(typed_lgr<ConcurentFile>& file, uint32_t len, uint64_t pos = -1);
	static typed_lgr<Task> write(typed_lgr<ConcurentFile>& file, char* arr, uint32_t len, uint64_t pos = -1);
	static typed_lgr<Task> append(typed_lgr<ConcurentFile>& file, char* arr, uint32_t len);

	static typed_lgr<Task> read_long(typed_lgr<ConcurentFile>& file, uint64_t len, uint64_t pos = -1);
	static typed_lgr<Task> write_long(typed_lgr<ConcurentFile>& file, list_array<uint8_t>*, uint64_t pos = -1);
	static typed_lgr<Task> append_long(typed_lgr<ConcurentFile>& file, list_array<uint8_t>*);

	static bool is_open(typed_lgr<ConcurentFile>& file);
	static void close(typed_lgr<ConcurentFile>& file);
};
class EventSystem {
	friend ValueItem* __async_notify(ValueItem* vals, uint32_t);
	TaskMutex no_race;
	std::list<typed_lgr<class FuncEnviropment>> heigh_priorihty;
	std::list<typed_lgr<class FuncEnviropment>> upper_avg_priorihty;
	std::list<typed_lgr<class FuncEnviropment>> avg_priorihty;
	std::list<typed_lgr<class FuncEnviropment>> lower_avg_priorihty;
	std::list<typed_lgr<class FuncEnviropment>> low_priorihty;

	std::list<typed_lgr<class FuncEnviropment>> async_heigh_priorihty;
	std::list<typed_lgr<class FuncEnviropment>> async_upper_avg_priorihty;
	std::list<typed_lgr<class FuncEnviropment>> async_avg_priorihty;
	std::list<typed_lgr<class FuncEnviropment>> async_lower_avg_priorihty;
	std::list<typed_lgr<class FuncEnviropment>> async_low_priorihty;

	static bool removeOne(std::list<typed_lgr<class FuncEnviropment>>& list, const typed_lgr<class FuncEnviropment>& func);
	void async_call(std::list<typed_lgr<class FuncEnviropment>>& list, ValueItem& args);
	bool awaitCall(std::list<typed_lgr<class FuncEnviropment>>& list, ValueItem& args);

	bool sync_call(std::list<typed_lgr<class FuncEnviropment>>& list, ValueItem& args);
public:
	enum class Priorithy {
		heigh,
		upper_avg,
		avg,
		lower_avg,
		low
	};
	void operator+=(const typed_lgr<class FuncEnviropment>& func);
	void join(const typed_lgr<class FuncEnviropment>& func, bool async_mode = false, Priorithy priorithy = Priorithy::avg);
	bool leave(const typed_lgr<class FuncEnviropment>& func, bool async_mode = false, Priorithy priorithy = Priorithy::avg);

	bool await_notify(ValueItem& args);
	bool notify(ValueItem& args);
	bool sync_notify(ValueItem& args);
	typed_lgr<Task> async_notify(ValueItem& args);
};
class TaskLimiter {
	list_array<void*> lock_check;
	std::list<typed_lgr<Task>> resume_task;
	std::timed_mutex no_race;
	std::condition_variable native_notify;
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


class TaskQuery{
	std::list<typed_lgr<Task>> tasks;
	class TaskQueryHandle* handle;
	bool is_running;
	friend void __TaskQuery_add_task_leave(class TaskQueryHandle* tqh, TaskQuery* tq);
public:
	TaskQuery(size_t at_execution_max = 0);
	~TaskQuery();
	typed_lgr<Task> add_task(typed_lgr<class FuncEnviropment> call_func, ValueItem& arguments, bool used_task_local = false, typed_lgr<class FuncEnviropment> exception_handler = nullptr, std::chrono::high_resolution_clock::time_point timeout = std::chrono::high_resolution_clock::time_point::min());
	void enable();
	void disable();
	bool in_query(typed_lgr<Task> task);
	void set_max_at_execution(size_t val);
	size_t get_max_at_execution();
	void wait();
	bool wait_for(size_t milliseconds);
	bool wait_until(std::chrono::high_resolution_clock::time_point time_point);
};

class TcpNetworkTask {
	struct async_handle* handle;
public:
	enum class HandleType{
		in_place,
		task
	};
	//constructor set the function to call when a new connection is made
	//function will return another function to call when data is received
	//if the function returns nullptr, the connection will be closed
	TcpNetworkTask(typed_lgr<class FuncEnviropment>, short port, bool blocking_mode = false, HandleType type = HandleType::task, size_t acceptors = 10);
	//start handling
	static void start(typed_lgr<TcpNetworkTask>&);
	//stop getting new connections, and continue handling connected
	static void pause(typed_lgr<TcpNetworkTask>&);
	//resume getting new connections
	static void resume(typed_lgr<TcpNetworkTask>&);
	//stop and close all connections
	static void stop(typed_lgr<TcpNetworkTask>&);
	//check if the server is running
	static bool is_running(typed_lgr<TcpNetworkTask>&);
	//start handling, and use the current thread to handle connections
	static void mainline(typed_lgr<TcpNetworkTask>&);
	//same as constructor
	static void set_on_connect(typed_lgr<TcpNetworkTask>&, typed_lgr<class FuncEnviropment> func);
	//return status of the server, will be checked after constructor
	static bool is_corrupted(typed_lgr<TcpNetworkTask>&);
};



#pragma pop_macro("min")