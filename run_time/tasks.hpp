// Copyright Danyil Melnytskyi 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once
#include <mutex>
#include <fstream>
#include "link_garbage_remover.hpp"
#include "../library/list_array.hpp"
#include "attacha_abi_structs.hpp"
#include <boost/fiber/timed_mutex.hpp>
#include <boost/fiber/condition_variable.hpp>
#include <boost/fiber/operations.hpp>
#include <boost/fiber/future.hpp>


//it do automaticaly self rethrow, but still recomended do rethrow manually and catching by const refrence
class TaskCancellation;
void forceCancelCancellation(TaskCancellation& cancel_token);






class TaskConditionVariable {
	std::list<typed_lgr<class Task>> resume_task;
	std::condition_variable cd;
	std::mutex no_race;
public:
	TaskConditionVariable() {}
	void wait();
	bool wait_for(size_t milliseconds);
	bool wait_until(std::chrono::high_resolution_clock::time_point time_point);
	void notify_one();
	void notify_all();
};
class TaskAwaiter {
	TaskConditionVariable cd;
	std::mutex no_race;
	bool allow_wait = true;
public:
	TaskAwaiter();
	void wait();
	bool wait_for(size_t milliseconds);
	bool wait_until(std::chrono::high_resolution_clock::time_point time_point);
	void endLife();
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
	TaskResult fres;
	std::mutex no_race;
	typed_lgr<class FuncEnviropment> ex_handle;//if ex_handle is nullptr then exception will be stored in fres
	typed_lgr<class FuncEnviropment> func;
	list_array<ValueItem>* args;
	std::mutex* relock_mut = nullptr;
	std::timed_mutex* relock_timed_mut = nullptr;
	std::recursive_mutex* relock_rec_mut = nullptr;
	bool time_end_flag = false;
	bool awaked = false;
	bool started = false;
	bool is_yield_mode = false;
	bool end_of_life = false;
	bool make_cancel = false;
	bool as_attacha_native = false;
	Task(typed_lgr<class FuncEnviropment> call_func, list_array<ValueItem>* arguments, typed_lgr<class FuncEnviropment> exception_handler = nullptr);
	Task(Task&& mov) noexcept : fres(std::move(mov.fres)) {
		ex_handle = mov.ex_handle;
		func = mov.func;
		args = mov.args;
		time_end_flag = mov.time_end_flag;
		awaked = mov.awaked;
		started = mov.started;
		is_yield_mode = mov.is_yield_mode;
		mov.args = nullptr;
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
class FileQuery {
	TaskMutex no_race;
	std::fstream stream;
public:
	FileQuery(const char* path) : stream(path, std::ios_base::in | std::ios_base::out | std::ios_base::binary) {}
	~FileQuery() {
		std::lock_guard guard(no_race);
		stream.close();
	}

	//all pointers will not be released, it do automatically

	//all pointers below will not be released, it do automatically
	typed_lgr<Task> read(uint32_t len, size_t pos = -1);
	typed_lgr<Task> write(char* arr, uint32_t len, size_t pos = -1);
	typed_lgr<Task> append(char* arr, uint32_t len);

	typed_lgr<Task> read_long(uint64_t len, size_t pos = -1);
	typed_lgr<Task> write_long(list_array<uint8_t>*, size_t pos = -1);
	typed_lgr<Task> append_long(list_array<uint8_t>*);
};
class AsyncFile {
	TaskMutex no_race;
	lgr descriptor;
	size_t r_pos;
	size_t w_pos;
public:
	enum class OpenMode {
		create_new = 1,
		create_always = 2,
		open_existing = 3,
		open_always = 4
	};
	enum class ReadMode {
		full,//if reached EOF, exception will throw
		no_care//if reached EOF, length argument will be ignored and readed rest bytes
	};
	AsyncFile(const char* path, OpenMode open_mode);
	~AsyncFile();
	typed_lgr<Task> seek(size_t pos) {
		std::lock_guard guard(no_race);
		r_pos = pos;
		w_pos = pos;
	}
	typed_lgr<Task> seekR(size_t pos) {
		std::lock_guard guard(no_race);
		r_pos = pos;
	}
	typed_lgr<Task> seekW(size_t pos) {
		std::lock_guard guard(no_race);
		w_pos = pos;
	}
	size_t length();
	//all pointers below will not be released, it do automatically
	typed_lgr<Task> read(uint32_t len, size_t pos, ReadMode);
	typed_lgr<Task> read(uint32_t len, ReadMode);
	typed_lgr<Task> write(char* arr, uint32_t len, size_t pos);
	typed_lgr<Task> write(char* arr, uint32_t len);
	typed_lgr<Task> append(char* arr, uint32_t len);
};


using BTaskMutex = boost::fibers::timed_mutex;
using BTaskLMutex = boost::fibers::mutex;
using BTaskConditionVariable = boost::fibers::condition_variable;

struct BTaskResult {
	list_array<ValueItem> results;
	BTaskConditionVariable result_notify;
	bool end_of_life = false;
	ValueItem* getResult(size_t res_num) {
		if (results.size() >= res_num) {
			BTaskLMutex mtx;
			std::unique_lock ul(mtx);
			result_notify.wait(ul, [&]() { return !(results.size() >= res_num || end_of_life); });

			if (end_of_life)
				return new ValueItem();
		}
		return new ValueItem(results[res_num]);
	}
	void awaitEnd() {
		BTaskLMutex mtx;
		std::unique_lock ul(mtx);
		while (end_of_life)
			result_notify.wait(ul);
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
	list_array<ValueItem>* args;
public:
	BTask(typed_lgr<class FuncEnviropment> call_func, list_array<ValueItem>* arguments) : func(call_func), args(arguments) {}
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
	static void reduceExecutor(uint32_t count = 1);
	static void start(typed_lgr<BTask> lgr_task);
	static void sleep(size_t milliseconds) {
		boost::this_fiber::sleep_for(std::chrono::milliseconds(milliseconds));
	}
	static void sleep_until(std::chrono::high_resolution_clock::time_point time_point) {
		boost::this_fiber::sleep_until(time_point);
	}
	static void awaitEndTasks();
	static void threadEnviroConfig();
	static void result(ValueItem* f_res);

	static ValueItem* getResult(typed_lgr<BTask> lgr_task, size_t yield_res = 0) {
		if (!lgr_task->fres)
			start(lgr_task);
		return lgr_task->fres->getResult(yield_res);
	}
};
class BFileQuery {
	BTaskLMutex no_race;
	std::fstream stream;
public:
	BFileQuery(const char* path) : stream(path, std::ios_base::in | std::ios_base::out | std::ios_base::binary) {}
	~BFileQuery() {
		std::lock_guard guard(no_race);
		stream.close();
	}
	boost::fibers::future<char*> read(size_t len, size_t pos) {
		return boost::fibers::async([this, len, pos]() {
			std::lock_guard guard(no_race);
			char* res = new char[len];
			stream.flush();
			stream.seekg(pos);
			stream.read(res, len);
			return res;
		});
	}
	boost::fibers::future<char*> read(size_t len) {
		return boost::fibers::async([this, len]() {
			std::lock_guard guard(no_race);
			char* res = new char[len];
			stream.read(res, len);
			return res;
		});
	}
	void write(char* arr,size_t len, size_t pos) {
		boost::fibers::async([this, arr, len, pos]() {
			std::lock_guard guard(no_race);
			stream.flush();
			stream.seekp(pos);
			stream.write(arr, len);
		});
	}
	void write(char* arr, size_t len) {
		boost::fibers::async([this, arr, len]() {
			std::lock_guard guard(no_race);
			stream.write(arr, len);
		});
	}
	void append(char* arr,size_t len) {
		boost::fibers::async([this, arr, len]() {
			std::lock_guard guard(no_race);
			char* res = new char[len];
			stream.flush();
			stream.seekp(0, std::ios_base::end);
			stream.write(arr, len);
		});
	}
};



#define CTask Task
#define CTaskMutex TaskMutex
#define CTaskConditionVarible TaskConditionVariable