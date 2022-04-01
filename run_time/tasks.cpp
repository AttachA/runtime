// Copyright Danyil Melnytskyi 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <list>
#include <boost/context/protected_fixedsize_stack.hpp>

#include <boost/context/continuation.hpp>

#include <boost/fiber/all.hpp>
#include <boost/fiber/detail/thread_barrier.hpp>

#include "AttachA_CXX.hpp"
#include "tasks.hpp"
#include "library/exceptions.hpp"
#include "../library/mem_tool.hpp"
#include "../library/string_help.hpp"
#include <iostream>
#include <stack>
#include <queue>
#include <deque>

namespace ctx = boost::context;
std::atomic_size_t alocs = 0;
std::atomic_size_t clears = 0;
class TaskCancellation : AttachARuntimeException {
public:
	bool in_landing = false;
	TaskCancellation() : AttachARuntimeException("This task received cancellation token") {}
	~TaskCancellation() noexcept(false) {
		if (!in_landing)
			throw TaskCancellation();
	}
};
void forceCancelCancellation(TaskCancellation& cancel_token) {
	cancel_token.in_landing = true;
}
TaskResult::~TaskResult() {
	if (context) {
		reinterpret_cast<ctx::continuation&>(context).~continuation();
		size_t i = 0;
		while (!end_of_life)
			getResult(i++);
	}
}


ValueItem* TaskResult::getResult(size_t res_num) {
	if (results.size() >= res_num) {
		while (results.size() >= res_num || end_of_life)
			result_notify.wait();

		if (end_of_life)
			return new ValueItem();
	}
	return new ValueItem(results[res_num]);
}
void TaskResult::awaitEnd() {
	while (!end_of_life)
		result_notify.wait();
}
void TaskResult::yieldResult(ValueItem* res, bool release) {
	if (res)
		results.push_back(std::move(*res));
	else
		results.push_back(ValueItem());
	result_notify.notify_all();
	if (release)
		delete res;
}
void TaskResult::yieldResult(ValueItem&& res) {
	results.push_back(std::move(res));
	result_notify.notify_all();
}
void TaskResult::finalResult(ValueItem* res) {
	if (res)
		results.push_back(std::move(*res));
	else
		results.push_back(ValueItem());
	end_of_life = true;
	result_notify.notify_all();
	delete res;
}
void TaskResult::finalResult(ValueItem&& res) {
	results.push_back(std::move(res));
	end_of_life = true;
	result_notify.notify_all();
}
TaskResult::TaskResult() {}
TaskResult::TaskResult(TaskResult&& move) noexcept {
	results = std::move(move.results);
	end_of_life = move.end_of_life;
	move.end_of_life = true;
}



Task::Task(typed_lgr<class FuncEnviropment> call_func, list_array<ValueItem>* arguments, typed_lgr<class FuncEnviropment> exception_handler) {
	ex_handle = exception_handler;
	func = call_func;
	args = arguments;
	alocs++;
}
Task::~Task() {
	if (args)
		delete args;
	clears++;
}

using timing = std::pair<std::chrono::high_resolution_clock::time_point, typed_lgr<Task>>;
struct {
	TaskConditionVariable no_tasks_notifier;
	TaskConditionVariable no_tasks_execute_notifier;

	std::queue<typed_lgr<Task>> tasks;
	std::deque<timing> timed_tasks;

	std::recursive_mutex task_thread_safety;
	std::recursive_mutex task_timer_safety;

	std::condition_variable tasks_notifier;
	std::condition_variable time_notifier;

	size_t executors = 0;
	size_t in_exec = 0;
	bool time_control_enabled = false;

	bool in_time_swap = false;
	std::atomic_size_t tasks_in_swap = 0;

} glob;

struct {
	ctx::continuation* tmp_current_context = nullptr;
	std::exception_ptr ex_ptr;
	typed_lgr<Task> curr_task = nullptr;
	bool is_task_thread = false;
	bool context_in_swap = false;
} thread_local loc;

void checkCancelation() {
	if (loc.curr_task->make_cancel)
		throw TaskCancellation();
}

#pragma optimize("",off)
void swapCtx() {
	loc.context_in_swap = true;
	++glob.tasks_in_swap;
	*loc.tmp_current_context = std::move(*loc.tmp_current_context).resume();
	--glob.tasks_in_swap;
	loc.context_in_swap = false;
	checkCancelation();
}
void swapCtxRelock(std::mutex& mut) {
	loc.curr_task->relock_mut = &mut;
	swapCtx();
	loc.curr_task->relock_mut = nullptr;
}
void swapCtxRelock(std::mutex& mut, std::timed_mutex& tmut) {
	loc.curr_task->relock_mut = &mut;
	loc.curr_task->relock_timed_mut = &tmut;
	swapCtx();
	loc.curr_task->relock_mut = nullptr;
	loc.curr_task->relock_timed_mut = nullptr;
}
void swapCtxRelock(std::timed_mutex& mut) {
	loc.curr_task->relock_timed_mut = &mut;
	swapCtx();
	loc.curr_task->relock_timed_mut = nullptr;
}
void swapCtxRelock(std::recursive_mutex& mut) {
	loc.curr_task->relock_rec_mut = &mut;
	swapCtx();
	loc.curr_task->relock_rec_mut = nullptr;
}
void swapCtxRelock(std::recursive_mutex& mut, std::timed_mutex& tmut) {
	loc.curr_task->relock_rec_mut = &mut;
	loc.curr_task->relock_timed_mut = &tmut;
	swapCtx();
	loc.curr_task->relock_rec_mut = nullptr;
	loc.curr_task->relock_timed_mut = nullptr;
}


ctx::continuation context_exec(ctx::continuation&& sink) {
	*loc.tmp_current_context = std::move(sink);
	try {
		loc.curr_task->fres.finalResult(loc.curr_task->func->syncWrapper(loc.curr_task->args));
		loc.context_in_swap = false;
	}
	catch (TaskCancellation& cancel) {
		cancel.in_landing = true;
	}
	catch (const ctx::detail::forced_unwind& uw) {
		throw;
	}
	catch (...) {
		loc.ex_ptr = std::current_exception();
	}
	loc.curr_task->end_of_life = true;
	return std::move(*loc.tmp_current_context);
}
ctx::continuation context_ex_handle(ctx::continuation&& sink) {
	*loc.tmp_current_context = std::move(sink);
	try {
		loc.curr_task->fres.finalResult(loc.curr_task->ex_handle->syncWrapper(loc.curr_task->args));
		loc.context_in_swap = false;
	}
	catch (TaskCancellation& cancel) {
		cancel.in_landing = true;
	}
	catch (const ctx::detail::forced_unwind&) {
		throw;
	}
	catch (...) {
		loc.ex_ptr = std::current_exception();
	}
	return std::move(*loc.tmp_current_context);
}


void taskNotifyIfEmpty() {
	std::lock_guard guard(glob.task_thread_safety);
	--glob.in_exec;
	if (!glob.in_exec && glob.tasks.empty() && glob.timed_tasks.empty())
		glob.no_tasks_execute_notifier.notify_all();
}
bool loadTask() {
	{
		std::lock_guard guard(glob.task_thread_safety);
		++glob.in_exec;
		size_t len = glob.tasks.size();
		if (!len)
			return true;
		loc.curr_task = glob.tasks.front();
		glob.tasks.pop();
		if (len == 1)
			glob.no_tasks_notifier.notify_all();
	}
	loc.tmp_current_context = &reinterpret_cast<ctx::continuation&>(loc.curr_task->fres.context);
	return false;
}
void taskExecutor(bool end_in_task_out = false) {
	loc.is_task_thread = true;
	std::mutex waiter;
	std::unique_lock waiter_lock(waiter);
	{
		std::lock_guard guard(glob.task_thread_safety);
		++glob.in_exec;
		++glob.executors;
	}
	while (true) {
		loc.context_in_swap = false;
		loc.tmp_current_context = nullptr;
		taskNotifyIfEmpty();
		while (glob.tasks.empty()) {
			if (end_in_task_out) {
				--glob.executors;
				return;
			}
			glob.tasks_notifier.wait(waiter_lock);
		}
		if (loadTask())
			continue;

		//if func is nullptr then this task signal to shutdown executor
		if (!loc.curr_task->func)
			break;

		if (loc.curr_task->end_of_life)
			goto end_task;

		if (*loc.tmp_current_context) {
			if (loc.curr_task->relock_mut)
				loc.curr_task->relock_mut->lock();
			if (loc.curr_task->relock_rec_mut)
				loc.curr_task->relock_rec_mut->lock();
			if (loc.curr_task->relock_timed_mut)
				loc.curr_task->relock_timed_mut->lock();
			*loc.tmp_current_context = std::move(*loc.tmp_current_context).resume();
		}
		else
			*loc.tmp_current_context = ctx::callcc(std::allocator_arg, ctx::protected_fixedsize_stack(128 * 1024), context_exec);
		if (loc.ex_ptr) {
			if (loc.curr_task->ex_handle) {
				if (loc.curr_task->args)
					delete loc.curr_task->args;
				loc.curr_task->args = new list_array<ValueItem>{ ValueItem(new std::exception_ptr(loc.ex_ptr), ValueMeta(VType::except_value, false, false), false) };
				loc.ex_ptr = nullptr;
				*loc.tmp_current_context = ctx::callcc(context_ex_handle);

				if (!loc.ex_ptr)
					goto end_task;
			}
			loc.curr_task->fres.finalResult(ValueItem(new std::exception_ptr(loc.ex_ptr), ValueMeta(VType::except_value, false, false)));
			loc.ex_ptr = nullptr;
		}

	end_task:
		if (loc.curr_task) {
			if (loc.curr_task->relock_mut)
				loc.curr_task->relock_mut->unlock();
			if (loc.curr_task->relock_rec_mut)
				loc.curr_task->relock_rec_mut->unlock();
			if (loc.curr_task->relock_timed_mut)
				loc.curr_task->relock_timed_mut->unlock();
		}
		loc.curr_task = nullptr;
	}
	{
		std::lock_guard guard(glob.task_thread_safety);
		--glob.executors;
	}
	taskNotifyIfEmpty();
}

#pragma optimize("",on)
void taskTimer() {
	std::mutex mtx;
	std::unique_lock ulm(mtx);
	while (true) {
		{
			if (glob.timed_tasks.size()) {
				std::lock_guard guard(glob.task_timer_safety);
				while (glob.timed_tasks.front().first <= std::chrono::high_resolution_clock::now()) {
					std::lock_guard task_guard(glob.timed_tasks.front().second->no_race);
					if (glob.timed_tasks.front().second->awaked) {
						glob.timed_tasks.pop_front();
					}
					else {
						glob.timed_tasks.front().second->time_end_flag = true;
						{
							std::lock_guard guard(glob.task_thread_safety);
							glob.tasks.push(std::move(glob.timed_tasks.front().second));
						}
						glob.timed_tasks.pop_front();
						glob.tasks_notifier.notify_one();
					}
					if (glob.timed_tasks.empty())
						break;

				}
			}
		}
		if (glob.timed_tasks.empty())
			glob.time_notifier.wait(ulm);
		else
			glob.time_notifier.wait_until(ulm, glob.timed_tasks.front().first);
	}
}


void startTimeController() {
	std::lock_guard guard(glob.task_thread_safety);
	if (glob.time_control_enabled)
		return;
	std::thread(taskTimer).detach();
	glob.time_control_enabled = true;
}
void makeTimeWait(std::chrono::high_resolution_clock::time_point t) {
	if (!glob.time_control_enabled)
		startTimeController();
	loc.curr_task->awaked = false;
	loc.curr_task->time_end_flag = false;
	size_t i = 0;
	{
		std::lock_guard guard(glob.task_timer_safety);
		auto it = glob.timed_tasks.begin();
		auto end = glob.timed_tasks.end();
		while (it != end) {
			if (it->first > t) {
				glob.timed_tasks.insert(it, timing(t,loc.curr_task));
				i = -1;
				break;
			}
			++it;
		}
		if (i != -1)
			glob.timed_tasks.push_back(timing(t, loc.curr_task));
	}
	glob.time_notifier.notify_one();
}

#pragma optimize("",off)
TaskMutex::~TaskMutex() {
	std::lock_guard lg(no_race);
	while (!resume_task.empty()) {
		auto& tsk = resume_task.back();
		tsk->make_cancel = true;
		current_task = nullptr;
		tsk->fres.awaitEnd();
		resume_task.pop_back();
	}
}
void TaskMutex::lock() {
	if (loc.is_task_thread) {
		loc.curr_task->awaked = false;
		loc.curr_task->time_end_flag = false;

		std::lock_guard lg(no_race);
		while (current_task) {
			resume_task.push_back(loc.curr_task);
			swapCtxRelock(no_race);
		}
		current_task = loc.curr_task.getPtr();
	}
	else
		no_race.lock();
}
bool TaskMutex::try_lock() {
	if (loc.is_task_thread) {
		if (!no_race.try_lock())
			return false;
		if (current_task) {
			no_race.unlock();
			return false;
		}
		else
			current_task = loc.curr_task.getPtr();
		no_race.unlock();
		return true;
	}
	else
		return no_race.try_lock();
}

bool TaskMutex::try_lock_for(size_t milliseconds) {
	return try_lock_until(std::chrono::high_resolution_clock::now() + std::chrono::milliseconds(milliseconds));
}
bool TaskMutex::try_lock_until(std::chrono::high_resolution_clock::time_point time_point) {
	if (loc.is_task_thread) {
	re_try:
		if (!no_race.try_lock_until(time_point))
			return false;
		if (current_task) {
			std::lock_guard guard(loc.curr_task->no_race);
			makeTimeWait(time_point);
			resume_task.push_back(loc.curr_task);
			swapCtxRelock(loc.curr_task->no_race, no_race);
			if (!loc.curr_task->awaked) {
				no_race.unlock();
				return false;
			}
			goto re_try;
		}
		if (!current_task)
			current_task = loc.curr_task.getPtr();
		no_race.unlock();
		return true;
	}
	else
		return no_race.try_lock_until(time_point);
}
void TaskMutex::unlock() {
	if (loc.is_task_thread) {
		std::lock_guard lg0(no_race);
		if (current_task != loc.curr_task.getPtr())
			throw InvalidOperation("Tried unlock non owned mutex");
		current_task = nullptr;
		while (resume_task.size()) {
			typed_lgr<Task> it = resume_task.front();
			resume_task.pop_front();
			std::lock_guard lg1(it->no_race);
			if (!it->time_end_flag) {
				it->awaked = true;
				std::lock_guard lg2(glob.task_thread_safety);
				glob.tasks.push(it);
				glob.tasks_notifier.notify_one();
				return;
			}
		}
	}
	else
		no_race.unlock();
}

bool TaskMutex::is_locked() {
	if (try_lock()) {
		unlock();
		return true;
	}
	return false;
}


void Task::start(typed_lgr<Task>& lgr_task) {
	if (lgr_task->started && !lgr_task->is_yield_mode)
		return;

	std::lock_guard guard(glob.task_thread_safety);
	glob.tasks.push(lgr_task);
	glob.tasks_notifier.notify_one();
	lgr_task->started = true;
}

void Task::createExecutor(size_t count) {
	for (size_t i = 0; i < count; i++)
		std::thread(taskExecutor, false).detach();
}
size_t Task::totalExecutors() {
	std::lock_guard guard(glob.task_thread_safety);
	return glob.executors;
}
void Task::reduceExecutor(size_t count) {
	for (size_t i = 0; i < count; i++)
		start(new Task(nullptr, nullptr));
}
void Task::becomeTaskExecutor() {
	try {
		taskExecutor();
		loc.context_in_swap = false;
		loc.is_task_thread = false;
		loc.curr_task = nullptr;
	}
	catch (...) {
		loc.context_in_swap = false;
		loc.is_task_thread = false;
		loc.curr_task = nullptr;
	}
}

void Task::awaitNoTasks(bool be_executor) {
	if (be_executor && !loc.is_task_thread)
		taskExecutor(true);
	else
		while (glob.tasks.size() || glob.timed_tasks.size())
			glob.no_tasks_notifier.wait();
}
void Task::awaitEndTasks() {
	while (glob.tasks.size() || glob.timed_tasks.size() || glob.in_exec || glob.tasks_in_swap)
		glob.no_tasks_execute_notifier.wait();
}
void Task::sleep(size_t milliseconds) {
	sleep_until(std::chrono::high_resolution_clock::now() + std::chrono::milliseconds(milliseconds));
}
void Task::sleep_until(std::chrono::high_resolution_clock::time_point time_point) {
	if (loc.is_task_thread) {
		std::lock_guard guard(loc.curr_task->no_race);
		makeTimeWait(time_point);
		swapCtxRelock(loc.curr_task->no_race);
	}
	else
		std::this_thread::sleep_until(time_point);
}


void Task::result(ValueItem* f_res) {
	if (loc.is_task_thread)
		loc.curr_task->fres.yieldResult(f_res);
	else
		throw EnviropmentRuinException("Thread attempt return yield result in non task enviro");
}
void Task::yield() {
	if (loc.is_task_thread)
		swapCtx();
	else
		throw EnviropmentRuinException("Thread attempt return yield task in non task enviro");
}
void TaskConditionVariable::wait() {
	if (loc.is_task_thread) {
		std::lock_guard guard(glob.task_thread_safety);
		resume_task.push_back(loc.curr_task);
		swapCtxRelock(glob.task_thread_safety);
	}
	else {
		std::mutex mtx;
		std::unique_lock ulm(mtx);
		cd.wait(ulm);
	}
}
bool TaskConditionVariable::wait_for(size_t milliseconds) {
	return wait_until(std::chrono::high_resolution_clock::now() + std::chrono::milliseconds(milliseconds));
}
bool TaskConditionVariable::wait_until(std::chrono::high_resolution_clock::time_point time_point) {
	if (loc.is_task_thread) {
		std::lock_guard guard(loc.curr_task->no_race);
		makeTimeWait(time_point);
		{
			std::lock_guard guard(no_race);
			resume_task.push_back(loc.curr_task);
		}
		swapCtxRelock(loc.curr_task->no_race);
		if (loc.curr_task->time_end_flag)
			return false;
	}
	else {
		std::mutex mtx;
		std::unique_lock ulm(mtx);
		auto wait_until_check = time_point;
		while (time_point > std::chrono::high_resolution_clock::now())
			cd.wait_until(ulm, time_point);
	}
	return true;
}
void TaskConditionVariable::notify_all() {
	std::lock_guard guard(glob.task_thread_safety);
	while (resume_task.size()) {
		std::lock_guard guard_loc(resume_task.back()->no_race);
		if (!resume_task.back()->time_end_flag) {
			resume_task.back()->awaked = true;
			glob.tasks.push(resume_task.front());
			resume_task.pop_front();
		}
		else
			resume_task.pop_front();
	}
	cd.notify_all();
}
void TaskConditionVariable::notify_one() {
	if (resume_task.size()) {
		std::lock_guard guard(glob.task_thread_safety);
		std::lock_guard guard_loc(resume_task.back()->no_race);
		if (!resume_task.back()->time_end_flag) {
			resume_task.back()->awaked = true;
			glob.tasks.push(resume_task.front());
			resume_task.pop_front();
		}
		else
			resume_task.pop_front();
		glob.tasks_notifier.notify_one();
	}
	else
		cd.notify_one();
}

TaskAwaiter::TaskAwaiter() {}
void TaskAwaiter::wait() {
	no_race.lock();
	if (allow_wait) {
		no_race.unlock();
		cd.wait();
	}
	else
		no_race.unlock();
}
bool TaskAwaiter::wait_for(size_t milliseconds) {
	no_race.lock();
	if (allow_wait) {
		no_race.unlock();
		return cd.wait_for(milliseconds);
	}
	else
		no_race.unlock();
	return false;
}
bool TaskAwaiter::wait_until(std::chrono::high_resolution_clock::time_point time_point) {
	no_race.lock();
	if (allow_wait) {
		no_race.unlock();
		return cd.wait_until(time_point);
	}
	else
		no_race.unlock();
	return false;
}
void TaskAwaiter::endLife() {
	no_race.lock();
	allow_wait = false;
	no_race.unlock();
	cd.notify_all();
}

void TaskSemaphore::setMaxTreeshold(size_t val) {
	std::lock_guard guard(no_race);
	release_all();
	max_treeshold = val;
	allow_treeshold = max_treeshold;
}
void TaskSemaphore::lock() {
	loc.curr_task->awaked = false;
	loc.curr_task->time_end_flag = false;
re_try:
	no_race.lock();
	if (!allow_treeshold) {
		no_race.unlock();
		if (loc.is_task_thread) {
			std::lock_guard guard(glob.task_thread_safety);
			resume_task.push_back(loc.curr_task);
			swapCtxRelock(glob.task_thread_safety);
		}
		else {
			std::mutex mtx;
			std::unique_lock ulm(mtx);
			native_notify.wait(ulm);
		}
		goto re_try;
	}
	else
		--allow_treeshold;
	no_race.unlock();
	return;
}
bool TaskSemaphore::try_lock() {
	if (!no_race.try_lock())
		return false;
	if (!allow_treeshold) {
		no_race.unlock();
		return false;
	}
	else
		--allow_treeshold;
	no_race.unlock();
	return true;
}

bool TaskSemaphore::try_lock_for(size_t milliseconds) {
	return try_lock_until(std::chrono::high_resolution_clock::now() + std::chrono::milliseconds(milliseconds));
}
bool TaskSemaphore::try_lock_until(std::chrono::high_resolution_clock::time_point time_point) {
re_try:
	if (!no_race.try_lock_until(time_point))
		return false;
	if (!allow_treeshold) {
		if (loc.is_task_thread) {
			std::lock_guard guard(glob.task_thread_safety);
			makeTimeWait(time_point);
			resume_task.push_back(loc.curr_task);
			no_race.unlock();
			swapCtxRelock(glob.task_thread_safety);
			if (!loc.curr_task->awaked)
				return false;
		}
		else {
			no_race.unlock();
			std::mutex mtx;
			std::unique_lock ulm(mtx);
			if (native_notify.wait_until(ulm, time_point) == std::cv_status::timeout)
				return false;
		}
		goto re_try;
	}
	if (allow_treeshold)
		--allow_treeshold;
	no_race.unlock();
	return true;

}
void TaskSemaphore::release() {
	std::lock_guard lg0(no_race);
	if (allow_treeshold == max_treeshold)
		return;
	allow_treeshold++;
	native_notify.notify_one();
	while (resume_task.size()) {
		auto& it = resume_task.back();
		std::lock_guard lg2(it->no_race);
		if (!it->time_end_flag) {
			it->awaked = true;
			{
				std::lock_guard lg1(glob.task_thread_safety);
				glob.tasks.push(resume_task.front());
			}
			resume_task.pop_front();
			glob.tasks_notifier.notify_one();
			return;
		}
		else
			resume_task.pop_front();
	}
}
void TaskSemaphore::release_all() {
	std::lock_guard lg0(no_race);
	if (allow_treeshold == max_treeshold)
		return;
	std::lock_guard lg1(glob.task_thread_safety);
	allow_treeshold = max_treeshold;
	native_notify.notify_all();
	while (resume_task.size()) {
		auto& it = resume_task.back();
		std::lock_guard lg2(it->no_race);
		if (!it->time_end_flag) {
			it->awaked = true;
			{
				std::lock_guard lg1(glob.task_thread_safety);
				glob.tasks.push(resume_task.front());
			}
			resume_task.pop_front();
			glob.tasks_notifier.notify_one();
		}
		else
			resume_task.pop_front();
	}
}
bool TaskSemaphore::is_locked() {
	if (try_lock()) {
		release();
		return true;
	}
	return false;
}
#pragma optimize("",on)




typed_lgr<Task> FileQuery::read(uint32_t len, size_t pos) {
	static typed_lgr<FuncEnviropment> read_enviro = new FuncEnviropment(
		(AttachALambda*)new AttachALambdaWrap(
			[this, len, pos](list_array<ValueItem>* arguments) {
				std::lock_guard guard(no_race);
				char* res = new char[len];
				if (pos != size_t(-1)) {
					stream.flush();
					stream.seekg(pos);
				}
				stream.read(res, len);
				if (stream.eof()) {
					delete[] res;
					return new ValueItem(new AException("system fault file eof", "No more bytes aviable, end of file"), ValueMeta(VType::except_value, false, true), true);
				}
				else if (stream.fail()) {
					delete[] res;
					return new ValueItem(new AException("system fault file fail", "Fail extract bytes"), ValueMeta(VType::except_value, false, true), true);
				}
				else if (stream.bad()) {
					delete[] res;
					return new ValueItem(new AException("system fault file bad", "Stream maybe corrupted"), ValueMeta(VType::except_value, false, true), true);
				}
				else
					return new ValueItem(res, ValueMeta(VType::raw_arr_ui8, false, true, len));
			}
		),
		false
	);
	typed_lgr<Task> aa(new Task(read_enviro,nullptr));
	Task::start(aa);
	return aa;
}
typed_lgr<Task> FileQuery::write(char* arr, uint32_t len, size_t pos) {
	typed_lgr<FuncEnviropment> write_enviro = new FuncEnviropment(
		(AttachALambda*)new AttachALambdaWrap(
			[this, len, pos, arr](list_array<ValueItem>* arguments) {
				mem_tool::ArrDeleter deleter(arr);
				std::lock_guard guard(no_race);
				if (pos != size_t(-1)) {
					stream.flush();
					stream.seekp(pos);
				}
				stream.write(arr, len);
				if (stream.eof())
					return new ValueItem(new AException("system fault file eof", "No more bytes aviable, end of file"), ValueMeta(VType::except_value, false, true), true);
				else if (stream.fail())
					return new ValueItem(new AException("system fault file fail", "Fail extract bytes"), ValueMeta(VType::except_value, false, true), true);
				
				else if (stream.bad())
					return new ValueItem(new AException("system fault file bad", "Stream maybe corrupted"), ValueMeta(VType::except_value, false, true), true);
				else
					return (ValueItem*)nullptr;
			}
		),
		false
	);
	typed_lgr<Task> aa(new Task(write_enviro, nullptr));
	Task::start(aa);
	return aa;
}
typed_lgr<Task> FileQuery::append(char* arr, uint32_t len) {
	typed_lgr<FuncEnviropment> append_enviro = new FuncEnviropment(
		(AttachALambda*)new AttachALambdaWrap(
			[this, len, arr](list_array<ValueItem>* arguments) {
				mem_tool::ArrDeleter deleter(arr);
				std::lock_guard guard(no_race);
				stream.flush();
				stream.seekp(0, std::ios_base::end);
				stream.write(arr, len);
				if (stream.eof())
					return new ValueItem(new AException("system fault file eof", "No more bytes aviable, end of file"), ValueMeta(VType::except_value, false, true), true);
				else if (stream.fail())
					return new ValueItem(new AException("system fault file fail", "Fail extract bytes"), ValueMeta(VType::except_value, false, true), true);

				else if (stream.bad())
					return new ValueItem(new AException("system fault file bad", "Stream maybe corrupted"), ValueMeta(VType::except_value, false, true), true);
				else
					return (ValueItem*)nullptr;
			}
		),
		false
	);
	typed_lgr<Task> aa(new Task(append_enviro, nullptr));
	Task::start(aa);
	return aa;
}

typed_lgr<Task> FileQuery::read_long(uint64_t len, size_t pos) {
	typed_lgr<FuncEnviropment> read_enviro = new FuncEnviropment(
		(AttachALambda*)new AttachALambdaWrap(
			[this, len, pos](list_array<ValueItem>* arguments) {
				std::lock_guard guard(no_race);
				list_array<char> res(len);
				if (pos != size_t(-1)) {
					stream.flush();
					stream.seekg(pos);
				}
				stream.read(res.data(), len);
				if (stream.eof())
					return new ValueItem(new AException("system fault file eof", "No more bytes aviable, end of file"), ValueMeta(VType::except_value, false, true), true);
				else if (stream.fail())
					return new ValueItem(new AException("system fault file fail", "Fail extract bytes"), ValueMeta(VType::except_value, false, true), true);
				
				else if (stream.bad()) 
					return new ValueItem(new AException("system fault file bad", "Stream maybe corrupted"), ValueMeta(VType::except_value, false, true), true);
				else
					return new ValueItem(
						new list_array<ValueItem>(
							res.convert<ValueItem>([](char it) { return ValueItem((void*)it, ValueMeta(VType::ui8, false, true)); })
						), 
						ValueMeta(VType::raw_arr_ui8, false, true, len)
					);
			}
		),
		false
	);
	typed_lgr<Task> aa(new Task(read_enviro, nullptr));
	Task::start(aa);
	return aa;
}
typed_lgr<Task> FileQuery::write_long(list_array<uint8_t>* arr, size_t pos) {
	typed_lgr<FuncEnviropment> write_enviro = new FuncEnviropment(
		(AttachALambda*)new AttachALambdaWrap(
			[this, arr, pos](list_array<ValueItem>* arguments) {
				mem_tool::ValDeleter deleter(arr);
				std::lock_guard guard(no_race);
				if (pos != size_t(-1)) {
					stream.flush();
					stream.seekp(pos);
				}
				stream.write((char*)arr->data(), arr->size());
				if (stream.eof())
					return new ValueItem(new AException("system fault file eof", "No more bytes aviable, end of file"), ValueMeta(VType::except_value, false, true), true);
				else if (stream.fail())
					return new ValueItem(new AException("system fault file fail", "Fail extract bytes"), ValueMeta(VType::except_value, false, true), true);

				else if (stream.bad())
					return new ValueItem(new AException("system fault file bad", "Stream maybe corrupted"), ValueMeta(VType::except_value, false, true), true);
				else
					return (ValueItem*)nullptr;
			}
		),
		false
	);
	typed_lgr<Task> aa(new Task(write_enviro, nullptr));
	Task::start(aa);
	return aa;
}
typed_lgr<Task> FileQuery::append_long(list_array<uint8_t>* arr) {
	typed_lgr<FuncEnviropment> append_enviro = new FuncEnviropment(
		(AttachALambda*)new AttachALambdaWrap(
			[this, arr](list_array<ValueItem>* arguments) {
				mem_tool::ValDeleter deleter(arr);
				std::lock_guard guard(no_race);
				stream.flush();
				stream.seekp(0, std::ios_base::end);
				stream.write((char*)arr->data(), arr->size());
				if (stream.eof())
					return new ValueItem(new AException("system fault file eof", "No more bytes aviable, end of file"), ValueMeta(VType::except_value, false, true), true);
				else if (stream.fail())
					return new ValueItem(new AException("system fault file fail", "Fail extract bytes"), ValueMeta(VType::except_value, false, true), true);

				else if (stream.bad())
					return new ValueItem(new AException("system fault file bad", "Stream maybe corrupted"), ValueMeta(VType::except_value, false, true), true);
				else
					return (ValueItem*)nullptr;
			}
		),
		false
				);
	typed_lgr<Task> aa(new Task(append_enviro, nullptr));
	Task::start(aa);
	return aa;
}
#ifdef _WIN64
#include <Windows.h>


static std::string _format_msq(DWORD err) {
	std::string res;
	const char* lpMsgBuf = nullptr;
	FormatMessageA(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		err,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPSTR)&lpMsgBuf,
		0, NULL);
	res = lpMsgBuf;
	LocalFree((LPVOID)lpMsgBuf);
	return res;
}

typedef typed_lgr<class Overlap> lpOverlap;
struct Overlap : OVERLAPPED {
	TaskAwaiter completion_notify;
	lgr fileHandle;
	uint32_t err_code = 0;
	bool completed = false;
public:
	uint32_t transfered_len = 0;
	char* read_result = nullptr;
	Overlap(lgr& file, size_t off) {
		std::memset(static_cast<OVERLAPPED*>(this), 0, sizeof(OVERLAPPED));
		Pointer = reinterpret_cast<void*>(off);
		fileHandle = file;
	}
	~Overlap() {
		if(read_result)
			delete[] read_result;
	}
	size_t get_offset() const { return size_t(Pointer); }
	static void callback(DWORD errorCode, DWORD numberOfBytesTransferred, Overlap* overlapped) {
		overlapped->transfered_len = numberOfBytesTransferred;
		overlapped->err_code = errorCode;
		overlapped->completed = true;
		overlapped->completion_notify.endLife();
	}
	uint32_t get_err_code() {
		if (!completed)
			wait();
		return err_code;
	}
	char* get() {
		if (!completed)
			wait();
		return read_result;
	}
	void wait() {
		while (!completed)
			completion_notify.wait();
	}
};

class F_IO_A_N_Controll {
	list_array<std::thread> threads;
	HANDLE completion_port;
public:
	F_IO_A_N_Controll() {
		completion_port = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 0);
	}
	~F_IO_A_N_Controll() {
		disable();
		CloseHandle(completion_port);
	}
	void enable(uint16_t count) {
		if (threads.empty()) {
			threads.reserve_push_back(count);
			while (count--) {
				threads.push_back(std::thread(&F_IO_A_N_Controll::dispatch, this));
				threads.back().detach();
			}
		}
	}
	void disable() {
		size_t len = threads.size();
		while (len--)
			PostQueuedCompletionStatus(completion_port, 0, 0, 0);
		threads.clear();
	}
	void attach(HANDLE file_handle) {
		if (!CreateIoCompletionPort(file_handle, completion_port, reinterpret_cast<ULONG_PTR>(file_handle), 0))
			throw std::exception("Failed to attach handle to I/O completion port");
	}
	size_t dispatched_count = 0;
	void dispatch() {
		while (true) {
			DWORD bytesTransferred = 0;
			ULONG_PTR completionKey = 0;
			LPOVERLAPPED ioCb = nullptr;
			SetLastError(0);
			bool ok = GetQueuedCompletionStatus(completion_port, &bytesTransferred, &completionKey, &ioCb, INFINITE);

			if (!ok) 
				throw AException("system fault async", "GetQueuedCompletionStatus: " + _format_msq(GetLastError()));

			if (completionKey == 0)
				return;
			

			if (!ioCb)
				throw AException("system fault async", "GetQueuedCompletionStatus overlapped is nullptr");
			
			Overlap* res = reinterpret_cast<Overlap*>(ioCb);
			Overlap::callback(GetLastError(), bytesTransferred, res);
			dispatched_count++;
		}
	}
	bool started() {
		return threads.size();
	}
};




F_IO_A_N_Controll global_control;



typed_lgr<Task> read(lgr& file, uint32_t tlen, size_t pos, AsyncFile::ReadMode rm) {
	typed_lgr<FuncEnviropment> read_enviro = new FuncEnviropment(
		(AttachALambda*)new AttachALambdaWrap(
			[file, tlen, pos, rm](list_array<ValueItem>* arguments) {
				lgr pfile = file;
				uint32_t len = tlen;
				LARGE_INTEGER lgi;
				if (!GetFileSizeEx(pfile.getPtr(), &lgi))
					return new ValueItem(new AException("system fault async", "GetFileSizeEx failed with error " + _format_msq(GetLastError()) + " for handle " + string_help::n2hexstr(file.getPtr())), ValueMeta(VType::except_value, false, true), true);
				
				if (rm == AsyncFile::ReadMode::full) {
					size_t checkp = len + pos;
					if(
						checkp < len ||
						pos < len ||
						(
							checkp == size_t(INT64_MIN) &&
							lgi.QuadPart != INT64_MIN
						) ||
						(
							checkp != size_t(INT64_MIN) &&
							lgi.QuadPart < checkp
						)
					)
						return new ValueItem(new AException("system fault async", "ReadMode::full, position and length is too large for handle " + string_help::n2hexstr(file.getPtr())), ValueMeta(VType::except_value, false, true), true);
				}
				else {
					size_t checkp = len + pos;
					if (
						checkp < len ||
						pos < len ||
						(
							checkp == size_t(INT64_MIN) &&
							lgi.QuadPart != INT64_MIN
						) ||
						(
							checkp != size_t(INT64_MIN) &&
							lgi.QuadPart < checkp
						)
					)
						len = pos - lgi.QuadPart;
				}

				if(!len)
					return new ValueItem(nullptr, ValueMeta(VType::raw_arr_ui8, false, true, 0), true);

				DWORD error;
				lpOverlap overlap(new Overlap(pfile,pos));
				overlap->read_result = new char[len];
				DWORD numberOfBytesRead = 0;
				const auto result = ::ReadFile(pfile.getPtr(), overlap->read_result, len, &numberOfBytesRead, overlap.getPtr());
				if (result)
					goto end;

				error = GetLastError();
				switch (error) {
				case ERROR_SUCCESS:
				case ERROR_IO_PENDING:
					break;
				default:
					return new ValueItem(new AException("system fault async", "ReadFile failed with error " + _format_msq(error) + " for handle " + string_help::n2hexstr(file.getPtr())), ValueMeta(VType::except_value, false, true), true);
				}
				overlap->wait();
			end:
				char* res = overlap->read_result;
				overlap->read_result = nullptr;
				if (pfile.totalLinks() != 1)
					goto end;
				return new ValueItem(res, ValueMeta(VType::raw_arr_ui8, false, true, overlap->transfered_len), true);
			}
		),
		false
	);
	if (!global_control.started())
		global_control.enable(1);
	typed_lgr<Task> aa(new Task(read_enviro, nullptr));
	Task::start(aa);
	return aa;
}
typed_lgr<Task> write(lgr file, char* arr, uint32_t len, size_t pos) {
	typed_lgr<FuncEnviropment> read_enviro = new FuncEnviropment(
		(AttachALambda*)new AttachALambdaWrap(
			[file, arr, len, pos](list_array<ValueItem>* arguments) {
				lgr pfile = file;
				lpOverlap overlap(new Overlap(pfile, pos));
				overlap->read_result = arr;
				DWORD numberOfBytesWriten = 0;
				DWORD error;
				WriteFile(*pfile, overlap->read_result, len, &numberOfBytesWriten, overlap.getPtr());
				error = GetLastError();
				switch (error) {
				case ERROR_SUCCESS:
				case ERROR_IO_PENDING:
					break;
				default:
					return new ValueItem(new AException("system fault async", "WriteFile failed with error " + _format_msq(error) + " for handle " + string_help::n2hexstr(file.getPtr())), ValueMeta(VType::except_value, false, true), true);
				}
				overlap->wait();
				return new ValueItem((void*)overlap->transfered_len, ValueMeta(VType::ui32, false, true));
			}
		),
		false
	);
	if (!global_control.started())
		global_control.enable(1);

	typed_lgr<Task> aa(new Task(read_enviro, nullptr));
	Task::start(aa);
	return aa;
}





void handleCleanUp(HANDLE hndl) {
	CloseHandle(hndl);
}
AsyncFile::AsyncFile(const char* path, OpenMode open_mode) {
	void* desc =
		CreateFileA(path,
		GENERIC_WRITE | GENERIC_READ,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		nullptr, (int)open_mode,
		FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
		nullptr);
	if (descriptor == INVALID_HANDLE_VALUE) {
		const auto error = GetLastError();
		switch (error) {
		case ERROR_ACCESS_DENIED:
			throw std::exception("Insufficient permissions for file opening");
		case ERROR_ALREADY_EXISTS:
		case ERROR_FILE_EXISTS:
			throw std::exception("File already exists");
		case ERROR_FILE_NOT_FOUND:
			throw std::exception("File not found");
		default:
			throw std::exception("Failed to open file");
		}
	}
	global_control.attach(desc);
	descriptor = lgr(desc,nullptr, handleCleanUp);
}
AsyncFile::~AsyncFile() {}

size_t AsyncFile::length() {
	LARGE_INTEGER lgi;
	if (!GetFileSizeEx(descriptor.getPtr(), &lgi))
		throw AException("system fault async", "GetFileSizeEx failed with error " + _format_msq(GetLastError()) + " for handle " + string_help::n2hexstr(descriptor.getPtr())), ValueMeta(VType::except_value, false, true);
}
typed_lgr<Task> AsyncFile::read(uint32_t len, size_t pos, ReadMode rm) {
	return ::read(descriptor, len, pos, rm);
}
typed_lgr<Task> AsyncFile::read(uint32_t len, ReadMode rm) {
	std::lock_guard guard(no_race);
	r_pos += len;
	return ::read(descriptor, len, r_pos - len, rm);
}
typed_lgr<Task> AsyncFile::write(char* arr, uint32_t len, size_t pos) {
	return ::write(descriptor, arr, len, pos);
}
typed_lgr<Task> AsyncFile::write(char* arr, uint32_t len) {
	return ::write(descriptor, arr, len, w_pos);
}
typed_lgr<Task> AsyncFile::append(char* arr, uint32_t len) {
	return ::write(descriptor, arr, len, -1);
}

#elif false


#endif

boost::fibers::condition_variable_any sleep_task{};
std::condition_variable awake_task{};

std::atomic_size_t inited_count = 0;
std::condition_variable ini_end;

std::atomic_size_t tasks_count = 0;
boost::fibers::condition_variable_any task_end{};
std::mutex disabler;
thread_local size_t to_disable_executors = 0;
void aathread() {
	boost::fibers::use_scheduling_algorithm<boost::fibers::algo::shared_work>();
	++inited_count;
	ini_end.notify_one();
	std::mutex mtx_count{};
	std::unique_lock lk(mtx_count);
	size_t no_tasks_count = 0;

	while (true) {
		if (to_disable_executors) {
			std::lock_guard guard(disabler);
			if (to_disable_executors) {
				--to_disable_executors;
				return;
			}
		}
		awake_task.wait(lk, []() { return (bool)tasks_count || to_disable_executors; });
		sleep_task.wait(lk, []() { return !tasks_count || to_disable_executors; });
	}
}

boost::fibers::fiber_specific_ptr<typed_lgr<BTask>> curr_task;

void BTask::createExecutor(uint32_t c) {
	boost::fibers::use_scheduling_algorithm<boost::fibers::algo::shared_work>(c);
	for (size_t i = inited_count; i < c; i++) {
		std::thread(aathread).detach();
		std::lock_guard guard(glob.task_thread_safety);
	}
	std::mutex sync{};
	std::unique_lock lk(sync);
	while (inited_count >= c)
		ini_end.wait(lk);
}
void BTask::reduceExecutor(uint32_t c) {
	{
		std::lock_guard guard(disabler);
		to_disable_executors += c;
	}
	for (uint32_t i = 0; i < c; i++) {
		awake_task.notify_one();
		sleep_task.notify_one();
	}
}
void BTask::start(typed_lgr<BTask> lgr_task) {
	boost::fibers::mutex mut;
	std::unique_lock lk(mut);
	lgr_task->fres = new BTaskResult();
	//boost::fibers::condition_variable start_notify{};
	++tasks_count;
	boost::fibers::fiber([](typed_lgr<BTask> lgr_task) {
		curr_task.reset(new typed_lgr<BTask>(lgr_task));
		ValueItem* tmp = lgr_task->func->syncWrapper(lgr_task->args);
		delete curr_task.release();

		--tasks_count;
		lgr_task->fres->end_of_life = true;
		if (tmp) {
			lgr_task->fres->results.push_back(std::move(*tmp));
			delete tmp;
		}

		lgr_task->fres->result_notify.notify_all();
		if (!tasks_count)
			task_end.notify_all();
		sleep_task.notify_one();
		}, lgr_task).detach();
		//start_notify.wait(lk);
		awake_task.notify_one();
}
void BTask::result(ValueItem* f_res) {
	if (curr_task.get()) {
		curr_task->getPtr()->fres->results.push_back(*f_res);
		curr_task->getPtr()->fres->result_notify.notify_all();
	}
	else
		throw EnviropmentRuinException("This function not async call, fail return result");
}
void BTask::awaitEndTasks() {
	std::mutex mut;
	std::unique_lock lk(mut);
	while (tasks_count)
		task_end.wait(lk);
}

void BTask::threadEnviroConfig() {
	boost::fibers::use_scheduling_algorithm<boost::fibers::algo::shared_work>();
}
