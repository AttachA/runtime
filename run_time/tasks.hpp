#pragma once
#include <mutex>
#include "link_garbage_remover.hpp"
#include "../libray/list_array.hpp"
#include "attacha_abi_structs.hpp"


//Task only in typed_lgr
struct Task {
	std::mutex no_race;
	struct TaskResult* fres = nullptr;
	class FuncEnviropment* ex_handle;//if ex_handle is nullptr then exception will be stored in fres
	class FuncEnviropment* func;
	list_array<ArrItem>* args;
	bool time_end_flag = false;
	bool awaked = false;
	bool started = false;
	bool is_yield_mode = false;
	Task(class FuncEnviropment* call_func, list_array<ArrItem>* arguments, class FuncEnviropment* exception_handler = nullptr);
	Task(Task&& mov) noexcept {
		fres = mov.fres;
		ex_handle = mov.ex_handle;
		func = mov.func;
		args = mov.args;
		time_end_flag = mov.time_end_flag;
		awaked = mov.awaked;
		started = mov.started;
		is_yield_mode = mov.is_yield_mode;
		mov.fres = nullptr;
		mov.args = nullptr;
	}

	~Task() {
		if (fres)
			delete fres;
		if (args)
			delete args;
	}
	static void start(typed_lgr<Task>&& lgr_task) {
		start(lgr_task);
	}
	static FuncRes* getResult(typed_lgr<Task>&& lgr_task) {
		return getResult(lgr_task);
	}
	static void start(typed_lgr<Task>& lgr_task);
	static FuncRes* getResult(typed_lgr<Task>& lgr_task);
	static FuncRes* getAwaitResult(typed_lgr<Task>& lgr_task);
	static FuncRes* getYieldResult(typed_lgr<Task>& lgr_task);

	static void createExecutor(size_t count = 1);
	static size_t totalExecutors();
	static void reduceExceutor(size_t count = 1);
	static void becomeTaskExecutor();
	static void awaitNoTasks(bool be_executor = false);
	static void awaitEndTasks();
	static void sleep(size_t milliseconds);
	static void sleep_until(std::chrono::high_resolution_clock::time_point time_point);
};
class TaskMutex {
	list_array<typed_lgr<Task>> resume_task;
	std::timed_mutex no_race;
	typed_lgr<Task> current_task = nullptr;
public:
	TaskMutex() {}
	void lock();
	bool try_lock();
	bool try_lock_for(size_t milliseconds);
	bool try_lock_until(std::chrono::high_resolution_clock::time_point time_point);
	void unlock();
	bool is_locked();
};
class TaskConditionVariable {
	list_array<typed_lgr<Task>> resume_task;
	std::condition_variable cd;
public:
	TaskConditionVariable() {}
	void wait();
	bool wait_for(size_t milliseconds);
	bool wait_until(std::chrono::high_resolution_clock::time_point time_point);
	void notify_one();
	void notify_all();
};
class TaskSemaphore {
	list_array<typed_lgr<Task>> resume_task;
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
