// Copyright Danyil Melnytskyi 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <list>
#include <boost/context/protected_fixedsize_stack.hpp>

#include <boost/context/continuation.hpp>

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

size_t Task::max_running_tasks = 0;
size_t Task::max_planned_tasks = 0;
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
	std::atomic_size_t in_run_tasks= 0;
	std::atomic_size_t planned_tasks = 0;

	TaskConditionVariable can_started_new_notifier;
	TaskConditionVariable can_planned_new_notifier;
} glob;

Task::Task(typed_lgr<class FuncEnviropment> call_func, list_array<ValueItem>* arguments, typed_lgr<class FuncEnviropment> exception_handler, bool used_task_local) {
	ex_handle = exception_handler;
	func = call_func;
	args = arguments;
	if (Task::max_planned_tasks)
		while (glob.planned_tasks >= Task::max_planned_tasks)
			glob.can_planned_new_notifier.wait();
	++glob.planned_tasks;

	if (used_task_local)
		task_local = new ValueEnvironment();
	
}
Task::~Task() {
	if (args)
		delete args;
	if (task_local)
		delete task_local;
	if (started) {
		--glob.in_run_tasks;
		if (Task::max_running_tasks)
			glob.can_started_new_notifier.notify_one();
	}
	else {
		--glob.planned_tasks;
		if (Task::max_running_tasks)
			glob.can_planned_new_notifier.notify_one();
	}
}


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

void makeTimeWait(std::chrono::high_resolution_clock::time_point t);

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
			glob.tasks_notifier.wait_for(waiter_lock,std::chrono::milliseconds(1000));
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
		else {
			if (Task::max_running_tasks) {
				if (Task::max_running_tasks <= glob.in_run_tasks) {
					makeTimeWait(std::chrono::high_resolution_clock::now() + std::chrono::milliseconds(100));
					continue;
				}
				++glob.in_run_tasks;
				--glob.planned_tasks;
				if (Task::max_planned_tasks)
					glob.can_planned_new_notifier.notify_one();
			}
			*loc.tmp_current_context = ctx::callcc(std::allocator_arg, ctx::protected_fixedsize_stack(128 * 1024), context_exec);
		}
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
			if (it->first >= t) {
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
		if (!no_race.try_lock_until(time_point))
			return false;
	re_try:
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


ConcurentFile::ConcurentFile(const char* path) {
	stream.open(path, std::fstream::in | std::fstream::out | std::fstream::binary);
	if (!stream.is_open()) {
		stream.open(path, std::fstream::in | std::fstream::out | std::fstream::binary | std::fstream::trunc);
		last_op_append = true;
	}
}
ConcurentFile::~ConcurentFile() {
	std::lock_guard guard(no_race);
	stream.close();
}
ValueItem* concurentReader(list_array<ValueItem>* arguments) {
	typed_lgr<ConcurentFile> pfile = *(typed_lgr<ConcurentFile>*)(*arguments)[0].val;
	std::fstream& stream = pfile->stream;
	delete (typed_lgr<ConcurentFile>*)(*arguments)[0].val;
	size_t len = (size_t)(*arguments)[1].val;
	size_t pos = (size_t)(*arguments)[2].val;
	std::lock_guard guard(pfile->no_race);
	if (pos != size_t(-1)) {
		pfile->last_op_append = false;
		stream.flush();
		stream.seekg(pos);
	}
	if (len >= UINT32_MAX) {
		list_array<char> res(len);
		pfile->stream.read(res.data(), len);
		return new ValueItem((void*)new OutOfRange("Array length is too large for store in raw array type"), ValueMeta(VType::except_value, false, true, len), true);
	}
	else {
		char* res = new char[len];
		pfile->stream.read(res, len);
		return new ValueItem(res, ValueMeta(VType::raw_arr_ui8, false, true, len), true);
	}
}
typed_lgr<FuncEnviropment> ConcurentReader(new FuncEnviropment(concurentReader, false));
ValueItem* concurentReaderLong(list_array<ValueItem>* arguments) {
	typed_lgr<ConcurentFile> pfile = *(typed_lgr<ConcurentFile>*)(*arguments)[0].val;
	std::fstream& stream = pfile->stream;
	delete (typed_lgr<ConcurentFile>*)(*arguments)[0].val;
	size_t len = (size_t)(*arguments)[1].val;
	size_t pos = (size_t)(*arguments)[2].val;
	std::lock_guard guard(pfile->no_race);
	if (pos != size_t(-1)) {
		pfile->last_op_append = false;
		stream.flush();
		stream.seekg(pos);
	}
	list_array<char> res(len);
	pfile->stream.read(res.data(), len);
	return new ValueItem(
		new list_array<ValueItem>(
			res.convert<ValueItem>([](char it) { return ValueItem((void*)it, ValueMeta(VType::ui8, false, true)); })
			),
		ValueMeta(VType::uarr, false, true, len)
	);
}
typed_lgr<FuncEnviropment> ConcurentReaderLong(new FuncEnviropment(concurentReaderLong, false));

ValueItem* concurentWriter(list_array<ValueItem>* arguments) {
	typed_lgr<ConcurentFile> pfile = *(typed_lgr<ConcurentFile>*)(*arguments)[0].val;
	char* value = (char*)(*arguments)[1].val;
	uint32_t len = (uint32_t)(*arguments)[2].val;
	size_t pos = (size_t)(*arguments)[3].val;
	bool is_append = (size_t)(*arguments)[4].val;
	mem_tool::ArrDeleter deleter(value);
	delete (typed_lgr<ConcurentFile>*)(*arguments)[0].val;
	std::fstream& stream = pfile->stream;

	std::lock_guard guard(pfile->no_race);
	if (pos != size_t(-1) || (!pfile->last_op_append && is_append)) {
		pfile->last_op_append = false;
		stream.flush();
		if(is_append)
			stream.seekg(0,std::fstream::end);
		else
			stream.seekg(pos);
	}
	stream.write(value, len);
	if (stream.eof())
		return new ValueItem(new AException("system fault file eof", "No more bytes aviable, end of file"), ValueMeta(VType::except_value, false, true), true);
	else if (stream.fail())
		return new ValueItem(new AException("system fault file fail", "Fail extract bytes"), ValueMeta(VType::except_value, false, true), true);

	else if (stream.bad())
		return new ValueItem(new AException("system fault file bad", "Stream maybe corrupted"), ValueMeta(VType::except_value, false, true), true);
	else
		return (ValueItem*)nullptr;
}
typed_lgr<FuncEnviropment> ConcurentWriter(new FuncEnviropment(concurentWriter, false));
ValueItem* concurentWriterLong(list_array<ValueItem>* arguments) {
	typed_lgr<ConcurentFile> pfile = *(typed_lgr<ConcurentFile>*)(*arguments)[0].val;
	list_array<uint8_t>* value = (list_array<uint8_t>*)(*arguments)[1].val;
	size_t len = value->size();
	size_t pos = (size_t)(*arguments)[3].val;
	bool is_append = (size_t)(*arguments)[4].val;
	mem_tool::ArrDeleter deleter0(value);
	mem_tool::ValDeleter deleter1((typed_lgr<ConcurentFile>*)(*arguments)[0].val);
	std::fstream& stream = pfile->stream;

	std::lock_guard guard(pfile->no_race);
	if (pos != size_t(-1) || (!pfile->last_op_append && is_append)) {
		pfile->last_op_append = false;
		stream.flush();
		if (is_append)
			stream.seekg(0, std::fstream::end);
		else
			stream.seekg(pos);
	}
	
	stream.write((const char*)value->data(), len);
	if (stream.eof())
		return new ValueItem(new AException("system fault file eof", "No more bytes aviable, end of file"), ValueMeta(VType::except_value, false, true), true);
	else if (stream.fail())
		return new ValueItem(new AException("system fault file fail", "Fail extract bytes"), ValueMeta(VType::except_value, false, true), true);

	else if (stream.bad())
		return new ValueItem(new AException("system fault file bad", "Stream maybe corrupted"), ValueMeta(VType::except_value, false, true), true);
	else
		return (ValueItem*)nullptr;
}
typed_lgr<FuncEnviropment> ConcurentWriterLong(new FuncEnviropment(concurentWriterLong, false));
typed_lgr<Task> ConcurentFile::read(typed_lgr<ConcurentFile>& file, uint32_t len, size_t pos) {
	typed_lgr<Task> aa(
		new Task(
			ConcurentReader,
			new list_array<ValueItem>{
				ValueItem(new typed_lgr<ConcurentFile>(file),ValueMeta(VType::undefined_ptr,false,true), true),
				ValueItem((void*)len ,ValueMeta(VType::ui64,false,true), true),
				ValueItem((void*)pos,ValueMeta(VType::ui64,false,true), true)
			}
		)
	);
	Task::start(aa);
	return aa;
}
typed_lgr<Task> ConcurentFile::write(typed_lgr<ConcurentFile>& file, char* arr, uint32_t len, size_t pos) {
	typed_lgr<Task> aa(
		new Task(
			ConcurentWriter,
			new list_array<ValueItem>{
				ValueItem(new typed_lgr<ConcurentFile>(file),ValueMeta(VType::undefined_ptr,false,true), true),
				ValueItem(arr,ValueMeta(VType::undefined_ptr,false,true), true),
				ValueItem((void*)len ,ValueMeta(VType::ui32,false,true), true),
				ValueItem((void*)pos,ValueMeta(VType::ui64,false,true), true),
				ValueItem((void*)false,ValueMeta(VType::ui8,false,true), true)
			}
		)
	);
	Task::start(aa);
	return aa;
}
typed_lgr<Task> ConcurentFile::append(typed_lgr<ConcurentFile>& file, char* arr, uint32_t len) {
	typed_lgr<Task> aa(
		new Task(
			ConcurentWriter,
			new list_array<ValueItem>{
				ValueItem(new typed_lgr<ConcurentFile>(file),ValueMeta(VType::undefined_ptr,false,true), true),
				ValueItem(arr,ValueMeta(VType::undefined_ptr,false,true), true),
				ValueItem((void*)len ,ValueMeta(VType::ui32,false,true), true),
				ValueItem((void*)-1,ValueMeta(VType::ui64,false,true), true),
				ValueItem((void*)true,ValueMeta(VType::ui8,false,true), true)
			}
		)
	);
	Task::start(aa);
	return aa;
}

typed_lgr<Task> ConcurentFile::read_long(typed_lgr<ConcurentFile>& file, uint64_t len, size_t pos) {
	typed_lgr<Task> aa(
		new Task(
			ConcurentReaderLong,
			new list_array<ValueItem>{
				ValueItem(new typed_lgr<ConcurentFile>(file),ValueMeta(VType::undefined_ptr,false,true), true),
				ValueItem((void*)len ,ValueMeta(VType::ui64,false,true), true),
				ValueItem((void*)pos,ValueMeta(VType::ui64,false,true), true)
			}
		)
	);
	Task::start(aa);
	return aa;
}
typed_lgr<Task> ConcurentFile::write_long(typed_lgr<ConcurentFile>& file, list_array<uint8_t>* arr, size_t pos) {
	typed_lgr<Task> aa(
		new Task(
			ConcurentWriter,
			new list_array<ValueItem>{
				ValueItem(new typed_lgr<ConcurentFile>(file),ValueMeta(VType::undefined_ptr,false,true), true),
				ValueItem(arr,ValueMeta(VType::undefined_ptr,false,true), true),
				ValueItem((void*)pos,ValueMeta(VType::ui64,false,true), true),
				ValueItem((void*)false,ValueMeta(VType::ui8,false,true), true)
			}
		)
	);
	Task::start(aa);
	return aa;
}
typed_lgr<Task> ConcurentFile::append_long(typed_lgr<ConcurentFile>& file, list_array<uint8_t>* arr) {
	typed_lgr<Task> aa(
		new Task(
			ConcurentWriterLong,
			new list_array<ValueItem>{
				ValueItem(new typed_lgr<ConcurentFile>(file),ValueMeta(VType::undefined_ptr,false,true), true),
				ValueItem(arr,ValueMeta(VType::undefined_ptr,false,true), true),
				ValueItem((void*)-1,ValueMeta(VType::ui64,false,true), true),
				ValueItem((void*)true,ValueMeta(VType::ui8,false,true), true)
			}
		)
	);
	Task::start(aa);
	return aa;
}