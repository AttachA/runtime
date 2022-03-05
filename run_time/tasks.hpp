#pragma once
#include <mutex>
#include "link_garbage_remover.hpp"
#include "../libray/list_array.hpp"
#include "attacha_abi_structs.hpp"
#include <boost/fiber/timed_mutex.hpp>
#include <boost/fiber/condition_variable.hpp>
#include <boost/fiber/operations.hpp>











//Task only in typed_lgr
struct Task {
	std::mutex no_race;
	struct TaskResult* fres = nullptr;
	typed_lgr<class FuncEnviropment> ex_handle;//if ex_handle is nullptr then exception will be stored in fres
	typed_lgr<class FuncEnviropment> func;
	list_array<ArrItem>* args;
	bool time_end_flag = false;
	bool awaked = false;
	bool started = false;
	bool is_yield_mode = false;
	bool end_of_life = false;
	Task(typed_lgr<class FuncEnviropment> call_func, list_array<ArrItem>* arguments, typed_lgr<class FuncEnviropment> exception_handler = nullptr);
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
	~Task();

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
	static void sleep(size_t milliseconds);
	static void sleep_until(std::chrono::high_resolution_clock::time_point time_point);

	static bool yieldIterate(typed_lgr<Task>& lgr_task) {
		bool res = !lgr_task->started || lgr_task->is_yield_mode;
		if (res)
			Task::start(lgr_task);
		return res;
	}
	static FuncRes* getCurrentResult(typed_lgr<Task>& lgr_task);
};
class TaskMutex {
	std::list<typed_lgr<Task>> resume_task;
	std::timed_mutex no_race;
	Task* current_task = nullptr;
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
	std::list<typed_lgr<Task>> resume_task;
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




using BTaskMutex = boost::fibers::timed_mutex;
using BTaskConditionVariable = boost::fibers::condition_variable;

struct BTaskResult {
	list_array<ArrItem> results;
	BTaskConditionVariable result_notify;
	bool end_of_life = false;
	FuncRes* getResult(size_t res_num) {
		if (results.size() >= res_num) {
			boost::fibers::mutex mtx;
			std::unique_lock ul(mtx);
			result_notify.wait(ul, [&]() { return !(results.size() >= res_num || end_of_life); });

			if (end_of_life)
				return new FuncRes();
		}
		return new FuncRes(results[res_num]);
	}
	~BTaskResult() {
		size_t i = 0;
		while (!end_of_life)
			getResult(i++);
	}

};
class BTask {
	BTaskResult* fres = nullptr;
	typed_lgr<class FuncEnviropment> func;
	list_array<ArrItem>* args;
public:
	BTask(typed_lgr<class FuncEnviropment> call_func, list_array<ArrItem>* arguments) : func(call_func), args(arguments) {}
	BTask(BTask&& mov) noexcept {
		func = mov.func;

		fres = mov.fres;
		args = mov.args;
		mov.fres = nullptr;
		mov.args = nullptr;
	}

	~BTask() {
		if (fres)
			delete fres;
		if (args)
			delete args;
	}


	static void createExecutor(uint32_t count = 1);
	static void start(typed_lgr<BTask> lgr_task);
	static void sleep(size_t milliseconds) {
		boost::this_fiber::sleep_for(std::chrono::milliseconds(milliseconds));
	}
	static void sleep_until(std::chrono::high_resolution_clock::time_point time_point) {
		boost::this_fiber::sleep_until(time_point);
	}
	static void awaitEndTasks();
	static void threadEnviroConfig();
	static void result(FuncRes* f_res);

	static FuncRes* getResult(typed_lgr<BTask> lgr_task, size_t yield_res = 0) {
		if (!lgr_task->fres)
			start(lgr_task);
		return lgr_task->fres->getResult(yield_res);
	}
};

class FileQuery {
	struct rq {
		typed_lgr<Task> tsk;
		size_t len;
		size_t id;
	};
	struct wq {
		typed_lgr<Task> tsk;
		const char* arr;
		size_t len;
		size_t id;
	};
	list_array<rq> read_query;
	list_array<wq> write_query;
	union {
		int i;
		long long l;
		unsigned int ui;
		unsigned long long ul;
	} descriptor;
	size_t id_l = 0;
public:
	FileQuery(const char* path);
	

};
