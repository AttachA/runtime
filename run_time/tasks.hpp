#pragma once
#include <mutex>
#include "../libray/list_array.hpp"
#include "attacha_abi_structs.hpp"



struct Task {
	struct TaskResult* fres = nullptr;
	class FuncEnviropment* ex_handle;//if ex_handle is nullptr then exception will be stored in fres
	class FuncEnviropment* func;
	list_array<ArrItem>* args;
	bool detached;
	bool started = false;
	bool is_yield_mode = false;
	Task(class FuncEnviropment* call_func, list_array<ArrItem>* arguments, bool detached = false, class FuncEnviropment* exception_handler = nullptr);
	Task(Task&& mov) {
		fres = mov.fres;
		ex_handle = mov.ex_handle;
		func = mov.func;
		args = mov.args;
		detached = mov.detached;
		started = mov.started;
		is_yield_mode = mov.is_yield_mode;
		mov.fres = nullptr;
		mov.ex_handle = nullptr;
		mov.func = nullptr;
		mov.args = nullptr;
		mov.detached = false;
		mov.started = false;
		mov.is_yield_mode = false;
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

	static void createExecutor(size_t count = 1);
	static size_t totalExecutors();
	static void reduceExceutor(size_t count = 1);
	static void becomeTaskExecutor();
	static void awaitNoTasks(bool be_executor = false);
	static void awaitEndTasks();
};
class TaskMutex {
	list_array<typed_lgr<Task>> resume_task;
	std::mutex no_race;
	typed_lgr<Task> current_task = nullptr;
public:
	TaskMutex() {}
	void lock();
	bool try_lock();
	void unlock();
};
class TaskConditionVariable {
	list_array<typed_lgr<Task>> resume_task;
	std::condition_variable cd;
public:
	TaskConditionVariable() {}
	void wait();
	void notify_one();
	void notify_all();
};