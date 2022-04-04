// Copyright Danyil Melnytskyi 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once
#include <mutex>
#include <fstream>
#include <filesystem>
#include "link_garbage_remover.hpp"
#include "../library/list_array.hpp"
#include "attacha_abi_structs.hpp"


//it do automaticaly self rethrow, but still recomended do rethrow manually and catching by const refrence
class TaskCancellation;
void forceCancelCancellation(TaskCancellation& cancel_token);






class TaskConditionVariable {
	std::list<typed_lgr<class Task>> resume_task;
	std::condition_variable cd;
	std::mutex no_race;
public:
	TaskConditionVariable() {}
	~TaskConditionVariable() {
		notify_all();
	}
	void wait();
	bool wait_for(size_t milliseconds);
	bool wait_until(std::chrono::high_resolution_clock::time_point time_point);
	void notify_one();
	void notify_all();
};
struct TaskResult {
	list_array<ValueItem> results;
	TaskConditionVariable result_notify;
	void* context = nullptr;
	bool end_of_life = false;
	ValueItem* getResult(size_t res_num);
	void awaitEnd();
	void yieldResult(ValueItem* res, bool release = true);
	void yieldResult(ValueItem&& res);
	void finalResult(ValueItem* res);
	void finalResult(ValueItem&& res);
	TaskResult();
	TaskResult(TaskResult&& move) noexcept;
	~TaskResult();
};

//Task only in typed_lgr
struct Task {
	static size_t max_running_tasks;
	static size_t max_planned_tasks;


	TaskResult fres;
	std::mutex no_race;
	typed_lgr<class FuncEnviropment> ex_handle;//if ex_handle is nullptr then exception will be stored in fres
	typed_lgr<class FuncEnviropment> func;
	list_array<ValueItem>* args;
	std::mutex* relock_mut = nullptr;
	std::timed_mutex* relock_timed_mut = nullptr;
	std::recursive_mutex* relock_rec_mut = nullptr;
	class ValueEnvironment* task_local = nullptr;
	bool time_end_flag = false;
	bool awaked = false;
	bool started = false;
	bool is_yield_mode = false;
	bool end_of_life = false;
	bool make_cancel = false;
	bool as_attacha_native = false;
	Task(typed_lgr<class FuncEnviropment> call_func, list_array<ValueItem>* arguments, typed_lgr<class FuncEnviropment> exception_handler = nullptr, bool used_task_local = false);
	Task(Task&& mov) noexcept : fres(std::move(mov.fres)) {
		ex_handle = mov.ex_handle;
		func = mov.func;
		args = mov.args;
		task_local = mov.task_local;
		time_end_flag = mov.time_end_flag;
		awaked = mov.awaked;
		started = mov.started;
		is_yield_mode = mov.is_yield_mode;
		mov.args = nullptr;
		mov.task_local = nullptr;
	}
	~Task();

	static void start(typed_lgr<Task>&& lgr_task) {
		start(lgr_task);
	}
	static void start(typed_lgr<Task>& lgr_task);

	static void createExecutor(size_t count = 1);
	static size_t totalExecutors();
	static void reduceExecutor(size_t count = 1);
	static void becomeTaskExecutor();
	static void awaitNoTasks(bool be_executor = false);
	static void awaitEndTasks();
	static void sleep(size_t milliseconds);
	static void sleep_until(std::chrono::high_resolution_clock::time_point time_point);
	static void result(ValueItem* f_res);
	static void yield();


	static bool yieldIterate(typed_lgr<Task>& lgr_task) {
		bool res = !lgr_task->started || lgr_task->is_yield_mode;
		if (res)
			Task::start(lgr_task);
		return res;
	}
	static ValueItem* getResult(typed_lgr<Task>& lgr_task, size_t yield_res = 0) {
		return lgr_task->fres.getResult(yield_res);
	}
	static ValueItem* getResult(typed_lgr<Task>&& lgr_task, size_t yield_res = 0) {
		return lgr_task->fres.getResult(yield_res);
	}

};
class TaskMutex {
	std::list<typed_lgr<Task>> resume_task;
	std::timed_mutex no_race;
	Task* current_task = nullptr;
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

	//all pointers will not be released, it do automatically

	//all pointers below will not be released, it do automatically
	static typed_lgr<Task> read(typed_lgr<ConcurentFile>& file, uint32_t len, size_t pos = -1);
	static typed_lgr<Task> write(typed_lgr<ConcurentFile>& file, char* arr, uint32_t len, size_t pos = -1);
	static typed_lgr<Task> append(typed_lgr<ConcurentFile>& file, char* arr, uint32_t len);

	static typed_lgr<Task> read_long(typed_lgr<ConcurentFile>& file, uint64_t len, size_t pos = -1);
	static typed_lgr<Task> write_long(typed_lgr<ConcurentFile>& file, list_array<uint8_t>*, size_t pos = -1);
	static typed_lgr<Task> append_long(typed_lgr<ConcurentFile>& file, list_array<uint8_t>*);
};