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
#include "ValueEnvironment.hpp"
#include <iostream>
#include <stack>
#include <queue>
#include <deque>
#include <condition_variable>

#undef min
namespace ctx = boost::context;


void MutexUnify::lock() {
	switch (type) {
	case MutexUnifyType::nmut:
		nmut->lock();
		break;
	case MutexUnifyType::ntimed:
		ntimed->lock();
		break;
	case MutexUnifyType::nrec:
		nrec->lock();
		break;
	case MutexUnifyType::umut:
		umut->lock();
		break;
	case MutexUnifyType::mmut:
		mmut->lock();
		break;
	default:
		break;
	}
}
bool MutexUnify::try_lock() {
	switch (type) {
	case MutexUnifyType::nmut:
		return nmut->try_lock();
	case MutexUnifyType::ntimed:
		return ntimed->try_lock();
	case MutexUnifyType::nrec:
		return nrec->try_lock();
	case MutexUnifyType::umut:
		return umut->try_lock();
	default:
		return false;
	}
}
bool MutexUnify::try_lock_for(size_t milliseconds) {
	switch (type) {
	case MutexUnifyType::noting:
		return false;
	case MutexUnifyType::ntimed:
		return ntimed->try_lock_for(std::chrono::milliseconds(milliseconds));
	case MutexUnifyType::nrec:
		return nrec->try_lock();
	case MutexUnifyType::umut:
		return umut->try_lock_for(milliseconds);
	case MutexUnifyType::mmut:
		return mmut->try_lock_for(milliseconds);
	default:
		return false;
	}
}
bool MutexUnify::try_lock_until(std::chrono::high_resolution_clock::time_point time_point) {
	switch (type) {
	case MutexUnifyType::noting:
		return false;
	case MutexUnifyType::ntimed:
		return ntimed->try_lock_until(time_point);
	case MutexUnifyType::nrec:
		return nrec->try_lock();
	case MutexUnifyType::umut:
		return umut->try_lock_until(time_point);
	case MutexUnifyType::mmut:
		return mmut->try_lock_until(time_point);
	default:
		return false;
	}
}
void MutexUnify::unlock() {
	switch (type) {
	case MutexUnifyType::nmut:
		nmut->unlock();
		break;
	case MutexUnifyType::ntimed:
		ntimed->unlock();
		break;
	case MutexUnifyType::nrec:
		nrec->unlock();
		break;
	case MutexUnifyType::umut:
		umut->unlock();
		break;
	case MutexUnifyType::mmut:
		mmut->unlock();
		break;
	default:
		break;
	}
}


MutexUnify::MutexUnify() {
	type = MutexUnifyType::noting;
}
MutexUnify::MutexUnify(const MutexUnify& mut) {
	type = mut.type;
	nmut = mut.nmut;
}
MutexUnify::MutexUnify(std::mutex& smut) {
	type = MutexUnifyType::nmut;
	nmut = std::addressof(smut);
}
MutexUnify::MutexUnify(std::timed_mutex& smut) {
	type = MutexUnifyType::ntimed;
	ntimed = std::addressof(smut);
}
MutexUnify::MutexUnify(std::recursive_mutex& smut) {
	type = MutexUnifyType::nrec;
	nrec = std::addressof(smut);
}
MutexUnify::MutexUnify(TaskMutex& smut) {
	type = MutexUnifyType::umut;
	umut = std::addressof(smut);
}
MutexUnify::MutexUnify(nullptr_t) {
	type = MutexUnifyType::noting;
}

MutexUnify& MutexUnify::operator=(const MutexUnify& mut) {
	type = mut.type;
	nmut = mut.nmut;
	return *this;
}
MutexUnify& MutexUnify::operator=(std::mutex& smut) {
	type = MutexUnifyType::nmut;
	nmut = std::addressof(smut);
	return *this;
}
MutexUnify& MutexUnify::operator=(std::timed_mutex& smut) {
	type = MutexUnifyType::ntimed;
	ntimed = std::addressof(smut);
	return *this;
}
MutexUnify& MutexUnify::operator=(std::recursive_mutex& smut) {
	type = MutexUnifyType::nrec;
	nrec = std::addressof(smut);
	return *this;
}
MutexUnify& MutexUnify::operator=(TaskMutex& smut) {
	type = MutexUnifyType::umut;
	umut = std::addressof(smut);
	return *this;
}
MutexUnify& MutexUnify::operator=(nullptr_t) {
	type = MutexUnifyType::noting;
	return *this;
}
MutexUnify::operator bool() {
	return type != MutexUnifyType::noting;
}



MultiplyMutex::MultiplyMutex(const std::initializer_list<MutexUnify>& muts):mu(muts) {}
void MultiplyMutex::lock() {
		for (auto& mut : mu)
			mut.lock();
	}
bool MultiplyMutex::try_lock() {
	list_array<MutexUnify> locked;
	for (auto& mut : mu) {
		if (mut.try_lock()) 
			locked.push_back(mut);
		else
			goto fail;
	}
	return true;
fail:
	for (auto& to_unlock : locked.reverse())
		to_unlock.unlock();
	return false;
}
bool MultiplyMutex::try_lock_for(size_t milliseconds) {
	list_array<MutexUnify> locked;
	for (auto& mut : mu) {
		if (mut.type != MutexUnifyType::nrec) {
			if (mut.try_lock_for(milliseconds))
				locked.push_back(mut);
			else
				goto fail;
		}
		else {
			if (mut.try_lock())
				locked.push_back(mut);
			else
				goto fail;
		}
	}
	return true;
fail:
	for (auto& to_unlock : locked.reverse())
		to_unlock.unlock();
	return false;
}
bool MultiplyMutex::try_lock_until(std::chrono::high_resolution_clock::time_point time_point) {
	list_array<MutexUnify> locked;
	for (auto& mut : mu) {
		if (mut.type != MutexUnifyType::nrec) {
			if (mut.try_lock_until(time_point))
				locked.push_back(mut);
			else
				goto fail;
		}
		else {
			if (mut.try_lock())
				locked.push_back(mut);
			else
				goto fail;
		}
	}
	return true;
fail:
	for (auto& to_unlock : locked.reverse())
		to_unlock.unlock();
	return false;
}
void MultiplyMutex::unlock() {
	for (auto& mut : mu.reverse())
		mut.unlock();
}
	


size_t Task::max_running_tasks = 0;
size_t Task::max_planned_tasks = 0;

TaskCancellation::TaskCancellation() : AttachARuntimeException("This task received cancellation token") {}
TaskCancellation::~TaskCancellation() {
	if (!in_landing) {
		abort();
	}
}
bool TaskCancellation::_in_landig() {
	return in_landing;
}

void forceCancelCancellation(TaskCancellation& cancel_token) {
	cancel_token.in_landing = true;
}
TaskResult::~TaskResult() {
	if (context) {
		size_t i = 0;
		//maybe race condition, TO-DO: check destructor and fix
		while (!end_of_life)
			getResult(i++);//for yield task
		reinterpret_cast<ctx::continuation&>(context).~continuation();
	}
}


ValueItem* TaskResult::getResult(size_t res_num) {
	MutexUnify uni(no_race);
	std::unique_lock l(uni);
	if (results.size() >= res_num) {
		while (results.size() >= res_num && !end_of_life)
			result_notify.wait(l);
		if (results.size() >= res_num)
			return new ValueItem();
	}
	return new ValueItem(results[res_num]);
}
void TaskResult::awaitEnd() {
	MutexUnify uni(no_race);
	std::unique_lock l(uni);
	while (!end_of_life)
		result_notify.wait(l);
}
void TaskResult::yieldResult(ValueItem* res, bool release) {
	MutexUnify uni(no_race);
	std::unique_lock l(uni);
	if (res) {
		results.push_back(std::move(*res));
		if (release)
			delete res;
	}
	else
		results.push_back(ValueItem());
	result_notify.notify_all();
}
void TaskResult::yieldResult(ValueItem&& res) {
	MutexUnify uni(no_race);
	std::unique_lock l(uni);
	results.push_back(std::move(res));
	result_notify.notify_all();
}
void TaskResult::finalResult(ValueItem* res) {
	MutexUnify uni(no_race);
	std::unique_lock l(uni);
	if (res) {
		results.push_back(std::move(*res));
		delete res;
	}
	else
		results.push_back(ValueItem());
	end_of_life = true;
	result_notify.notify_all();
}
void TaskResult::finalResult(ValueItem&& res) {
	MutexUnify uni(no_race);
	std::unique_lock l(uni);
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

struct timing {
	std::chrono::high_resolution_clock::time_point wait_timepoint; 
	typed_lgr<Task> awake_task; 
	size_t check_id;
};
struct {
	TaskConditionVariable no_tasks_notifier;
	TaskConditionVariable no_tasks_execute_notifier;

	std::queue<typed_lgr<Task>> tasks;
	std::queue<typed_lgr<Task>> cold_tasks;
	std::deque<timing> timed_tasks;

	std::recursive_mutex task_thread_safety;
	std::recursive_mutex task_timer_safety;

	std::condition_variable_any tasks_notifier;
	std::condition_variable_any time_notifier;

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
struct {
	ctx::continuation* tmp_current_context = nullptr;
	std::exception_ptr ex_ptr;
	list_array<typed_lgr<Task>> tasks_buffer;
	typed_lgr<Task> curr_task = nullptr;
	bool is_task_thread = false;
	bool context_in_swap = false;
} thread_local loc;

void checkCancelation() {
	if (loc.curr_task->make_cancel)
		throw TaskCancellation();
	if (loc.curr_task->timeout != std::chrono::high_resolution_clock::time_point::min())
		if (loc.curr_task->timeout <= std::chrono::high_resolution_clock::now())
			throw TaskCancellation();
}
#pragma optimize("",off)
#pragma region TaskExecutor
void swapCtx() {
	loc.context_in_swap = true;
	++glob.tasks_in_swap;
	*loc.tmp_current_context = std::move(*loc.tmp_current_context).resume();
	--glob.tasks_in_swap;
	loc.context_in_swap = false;
	checkCancelation();
}

void swapCtxRelock(const MutexUnify& mut0) {
	loc.curr_task->relock_0 = mut0;
	try {
		swapCtx();
	}
	catch (...) {
		loc.curr_task->relock_0 = nullptr;
		throw;
	}
	loc.curr_task->relock_0 = nullptr;
}
void swapCtxRelock(const MutexUnify& mut0, const MutexUnify& mut1, const MutexUnify& mut2) {
	loc.curr_task->relock_0 = mut0;
	loc.curr_task->relock_1 = mut1;
	loc.curr_task->relock_2 = mut2;
	try {
		swapCtx();
	}
	catch (...) {
		loc.curr_task->relock_0 = nullptr;
		loc.curr_task->relock_1 = nullptr;
		loc.curr_task->relock_2 = nullptr;
		throw;
	}
	loc.curr_task->relock_0 = nullptr;
	loc.curr_task->relock_1 = nullptr;
	loc.curr_task->relock_2 = nullptr;
}
void swapCtxRelock(const MutexUnify& mut0, const MutexUnify& mut1) {
	loc.curr_task->relock_0 = mut0;
	loc.curr_task->relock_1 = mut1;
	try {
		swapCtx();
	}
	catch (...) {
		loc.curr_task->relock_0 = nullptr;
		loc.curr_task->relock_1 = nullptr;
		throw;
	}
	loc.curr_task->relock_0 = nullptr;
	loc.curr_task->relock_1 = nullptr;
}

void warmUpTheTasks() {
	std::lock_guard guard(glob.task_thread_safety);
	if (!Task::max_running_tasks && glob.tasks.empty()) {
		std::swap(glob.tasks, glob.cold_tasks);
	}
	else {
		size_t placed = glob.in_run_tasks;
		while (!glob.cold_tasks.empty() && Task::max_running_tasks > placed++) {
			glob.tasks.push(std::move(glob.cold_tasks.front()));
			glob.cold_tasks.pop();
		}
		if (Task::max_running_tasks > placed && glob.cold_tasks.empty())
			glob.can_started_new_notifier.notify_all();
	}
}

ctx::continuation context_exec(ctx::continuation&& sink) {
	*loc.tmp_current_context = std::move(sink);
	try {
		checkCancelation();
		loc.curr_task->fres.finalResult(loc.curr_task->func->syncWrapper((ValueItem*)loc.curr_task->args.val, loc.curr_task->args.meta.val_len));
		loc.context_in_swap = false;
	}
	catch (TaskCancellation& cancel) {
		forceCancelCancellation(cancel);
	}
	catch (const ctx::detail::forced_unwind&) {
		throw;
	}
	catch (...) {
		loc.ex_ptr = std::current_exception();
	}
	MutexUnify uni(loc.curr_task->no_race);
	std::unique_lock l(uni);
	loc.curr_task->end_of_life = true;
	loc.curr_task->fres.result_notify.notify_all();
	--glob.in_run_tasks;
	if (Task::max_running_tasks)
		glob.can_started_new_notifier.notify_one();
	return std::move(*loc.tmp_current_context);
}
ctx::continuation context_ex_handle(ctx::continuation&& sink) {
	*loc.tmp_current_context = std::move(sink);
	try {
		checkCancelation();
		loc.curr_task->fres.finalResult(loc.curr_task->ex_handle->syncWrapper((ValueItem*)loc.curr_task->args.val, loc.curr_task->args.meta.val_len));
		loc.context_in_swap = false;
	}
	catch (TaskCancellation& cancel) {
		forceCancelCancellation(cancel);
	}
	catch (const ctx::detail::forced_unwind&) {
		throw;
	}
	catch (...) {
		loc.ex_ptr = std::current_exception();
	}
	
	MutexUnify uni(loc.curr_task->no_race);
	std::unique_lock l(uni);
	loc.curr_task->end_of_life = true;
	loc.curr_task->fres.result_notify.notify_all();
	--glob.in_run_tasks;
	if (Task::max_running_tasks)
		glob.can_started_new_notifier.notify_one();
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
		auto tmp = std::move(glob.tasks.front());
		glob.tasks.pop();
		if (len == 1)
			glob.no_tasks_notifier.notify_all();
		loc.curr_task = std::move(tmp);
	}
	loc.tmp_current_context = &reinterpret_cast<ctx::continuation&>(loc.curr_task->fres.context);
	return false;
}

void worker_mode_desk(const std::string& mode) {
	if (enable_thread_naming)
		_set_name_thread_dbg("Worker " + std::to_string(_thread_id()) + ": " + mode);
}

void taskExecutor(bool end_in_task_out = false) {
	if(!end_in_task_out)
		worker_mode_desk("idle");
	loc.is_task_thread = true;
	{
		std::lock_guard guard(glob.task_thread_safety);
		++glob.in_exec;
		++glob.executors;
	}
	while (true) {
		loc.context_in_swap = false;
		loc.tmp_current_context = nullptr;
		taskNotifyIfEmpty();
		loc.is_task_thread = false;
		while (glob.tasks.empty()) {
			if (!glob.cold_tasks.empty()) {
				if (!Task::max_running_tasks) {
					warmUpTheTasks();
					break;
				}
				else if (Task::max_running_tasks > glob.in_run_tasks) {
					warmUpTheTasks();
					break;
				}
			}

			if (end_in_task_out) {
				--glob.executors;
				return;
			}
			worker_mode_desk("idle");
			std::mutex mtx;
			std::unique_lock ulc(mtx);
			glob.tasks_notifier.wait_for(ulc,std::chrono::milliseconds(1000));
		}
		loc.is_task_thread = true;
		if (loadTask())
			continue;

		//if func is nullptr then this task signal to shutdown executor
		if (!loc.curr_task->func)
			break;
		if (!end_in_task_out)
			worker_mode_desk("process task - " + std::to_string(loc.curr_task->task_id()));
		if (loc.curr_task->end_of_life)
			goto end_task;

		if (*loc.tmp_current_context) {
			loc.curr_task->relock_0.lock();
			loc.curr_task->relock_1.lock();
			loc.curr_task->relock_2.lock();
			*loc.tmp_current_context = std::move(*loc.tmp_current_context).resume();
		}
		else {
			++glob.in_run_tasks;
			--glob.planned_tasks;
			if (Task::max_planned_tasks)
				glob.can_planned_new_notifier.notify_one();
			*loc.tmp_current_context = ctx::callcc(std::allocator_arg, ctx::protected_fixedsize_stack(128 * 1024), context_exec);
		}
		if (loc.ex_ptr) {
			if (loc.curr_task->ex_handle) {
				ValueItem temp(new std::exception_ptr(loc.ex_ptr), ValueMeta(VType::except_value, false, false), false);
				loc.curr_task->args = ValueItem(&temp, 0);
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
			loc.curr_task->relock_0.unlock();
			loc.curr_task->relock_1.unlock();
			loc.curr_task->relock_2.unlock();
		}
		loc.curr_task = nullptr;
	}
	{
		std::lock_guard guard(glob.task_thread_safety);
		--glob.executors;
	}
	taskNotifyIfEmpty();
}
#pragma endregion
#pragma optimize("",on)
void taskTimer() {
	if (enable_thread_naming)
		_set_name_thread_dbg("Task time controller");

	std::mutex mtx;
	std::unique_lock ulm(mtx);
	while (true) {
		{
			if (glob.timed_tasks.size()) {
				std::lock_guard guard(glob.task_timer_safety);
				while (glob.timed_tasks.front().wait_timepoint <= std::chrono::high_resolution_clock::now()) {
					timing& tmng = glob.timed_tasks.front();
					if (tmng.check_id != tmng.awake_task->sleep_check) {
						glob.timed_tasks.pop_front();
						continue;
					}
					std::lock_guard task_guard(tmng.awake_task->no_race);
					if (tmng.awake_task->awaked) {
						glob.timed_tasks.pop_front();
					}
					else {
						tmng.awake_task->time_end_flag = true;
						{
							std::lock_guard guard(glob.task_thread_safety);
							glob.tasks.push(std::move(tmng.awake_task));
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
			glob.time_notifier.wait_until(ulm, glob.timed_tasks.front().wait_timepoint);
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
			if (it->wait_timepoint >= t) {
				glob.timed_tasks.insert(it, timing(t,loc.curr_task, ++loc.curr_task->sleep_check));
				i = -1;
				break;
			}
			++it;
		}
		if (i != -1)
			glob.timed_tasks.push_back(timing(t, loc.curr_task, ++loc.curr_task->sleep_check));
	}
	glob.time_notifier.notify_one();
}

#pragma region Task
Task::Task(typed_lgr<class FuncEnviropment> call_func, ValueItem& arguments, bool used_task_local, typed_lgr<class FuncEnviropment> exception_handler, std::chrono::high_resolution_clock::time_point task_timeout) {
	ex_handle = exception_handler;
	func = call_func;
	if (arguments.meta.vtype == VType::async_res)
		arguments.getAsync();
	if (arguments.meta.vtype == VType::faarr || arguments.meta.vtype == VType::saarr) {
		if (!arguments.meta.use_gc)
			args = arguments;
		else
			args = ValueItem((ValueItem*)arguments.getSourcePtr(), arguments.meta.val_len);
	}
	else {
		if (arguments.meta.vtype != VType::noting)
			args = ValueItem(&arguments, 1);
	}
	timeout = task_timeout;
	if (used_task_local)
		_task_local = new ValueEnvironment();

	if (Task::max_planned_tasks) {
		MutexUnify uni(glob.task_thread_safety);
		std::unique_lock l(uni);
		while (glob.planned_tasks >= Task::max_planned_tasks)
			glob.can_planned_new_notifier.wait(l);
	}
	++glob.planned_tasks;

}
Task::Task(Task&& mov) noexcept : fres(std::move(mov.fres)) {
	ex_handle = mov.ex_handle;
	func = mov.func;
	args = std::move(mov.args);
	_task_local = mov._task_local;
	mov._task_local = nullptr;
	time_end_flag = mov.time_end_flag;
	awaked = mov.awaked;
	started = mov.started;
	is_yield_mode = mov.is_yield_mode;
}
Task::~Task() {
	if (_task_local)
		delete _task_local;
	if (!started) {
		--glob.planned_tasks;
		if (Task::max_running_tasks)
			glob.can_planned_new_notifier.notify_one();
	}
}
void Task::start(list_array<typed_lgr<Task>>& lgr_task) {
	for (auto& it : lgr_task)
		start(it);
}


void Task::start(typed_lgr<Task>&& lgr_task) {
	start(lgr_task);
}

bool Task::yield_iterate(typed_lgr<Task>& lgr_task) {
	bool res = !lgr_task->started || lgr_task->is_yield_mode;
	if (res)
		Task::start(lgr_task);
	return res;
}
ValueItem* Task::get_result(typed_lgr<Task>& lgr_task, size_t yield_res) {
	if (!lgr_task->started)
		Task::start(lgr_task);
	return lgr_task->fres.getResult(yield_res);
}
ValueItem* Task::get_result(typed_lgr<Task>&& lgr_task, size_t yield_res) {
	if (!lgr_task->started)
		Task::start(lgr_task);
	return lgr_task->fres.getResult(yield_res);
}
void Task::await_task(typed_lgr<Task>& lgr_task, bool in_place) {
	if (!lgr_task->started)
		Task::start(lgr_task);
	lgr_task->fres.awaitEnd();
}
list_array<ValueItem> Task::await_results(typed_lgr<Task>& task) {
	await_task(task);
	return task->fres.results;
}
list_array<ValueItem> Task::await_results(list_array<typed_lgr<Task>>& tasks) {
	list_array<ValueItem> res;
	for (auto& it : tasks)
		res.push_back(await_results(it));
	return res;
}

void Task::start(const typed_lgr<Task>& tsk) {
	typed_lgr<Task> lgr_task = tsk;
	if (lgr_task->started && !lgr_task->is_yield_mode)
		return;

	std::lock_guard guard(glob.task_thread_safety);
	if (lgr_task->started && !lgr_task->is_yield_mode)
		return;
	glob.cold_tasks.push(lgr_task);
	glob.tasks_notifier.notify_one();
	lgr_task->started = true;
}

void Task::create_executor(size_t count) {
	for (size_t i = 0; i < count; i++)
		std::thread(taskExecutor, false).detach();
}
size_t Task::total_executors() {
	std::lock_guard guard(glob.task_thread_safety);
	return glob.executors;
}
void Task::reduce_executor(size_t count) {
	ValueItem noting;
	for (size_t i = 0; i < count; i++)
		start(new Task(nullptr, noting));
}
void Task::become_task_executor() {
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

void Task::await_no_tasks(bool be_executor) {
	if (be_executor && !loc.is_task_thread)
		taskExecutor(true);
	else {
		MutexUnify uni(glob.task_thread_safety);
		std::unique_lock l(uni);
		while (glob.tasks.size() || glob.cold_tasks.size() || glob.timed_tasks.size())
			glob.no_tasks_notifier.wait(l);
	}
}
void Task::await_end_tasks(bool be_executor) {
	MutexUnify uni(glob.task_thread_safety);
	std::unique_lock l(uni);
	if (be_executor && !loc.is_task_thread) {
		while (glob.tasks.size() || glob.cold_tasks.size() || glob.timed_tasks.size() || glob.in_exec || glob.tasks_in_swap) {
			if (glob.tasks.size() || glob.cold_tasks.size()) {
				l.unlock();
				taskExecutor(true);
				l.lock();
			}
			else if (glob.timed_tasks.size())
				glob.tasks_notifier.wait(l);
			else if (glob.in_exec || glob.tasks_in_swap)
				glob.no_tasks_execute_notifier.wait(l);
			else
				return;
		}
	}
	else {
		while (glob.tasks.size() || glob.cold_tasks.size() || glob.timed_tasks.size() || glob.in_exec || glob.tasks_in_swap) 
			glob.no_tasks_execute_notifier.wait(l);
	}
}
void Task::await_multiple(list_array<typed_lgr<Task>>& tasks, bool pre_started) {
	if (!pre_started) {
		for (auto& it : tasks)
			Task::start(it);
	}
	for (auto& it : tasks)
		it->fres.awaitEnd();
}
void Task::await_multiple(typed_lgr<Task>* tasks, size_t len, bool pre_started) {
	if (!pre_started) {
		typed_lgr<Task>* iter = tasks;
		size_t count = len;
		while (count--)
			Task::start(*iter++);
	}
	while (len--)
		(*tasks++)->fres.awaitEnd();
}
void Task::sleep(size_t milliseconds) {
	sleep_until(std::chrono::high_resolution_clock::now() + std::chrono::milliseconds(milliseconds));
}



void Task::result(ValueItem* f_res) {
	if (loc.is_task_thread)
		loc.curr_task->fres.yieldResult(f_res);
	else
		throw EnviropmentRuinException("Thread attempt return yield result in non task enviro");
}

void Task::check_cancelation() {
	if (loc.is_task_thread)
		checkCancelation();
	else
		throw EnviropmentRuinException("Thread attempted check cancelation in non task enviro");
}
void Task::self_cancel() {
	if (loc.is_task_thread)
		throw TaskCancellation();
	else
		throw EnviropmentRuinException("Thread attempted cancel self, like task");
}


class ValueEnvironment* Task::task_local() {
	if (!loc.is_task_thread)
		return nullptr;
	else if (loc.curr_task->_task_local) {
		return loc.curr_task->_task_local;
	}
	else {
		return loc.curr_task->_task_local = new ValueEnvironment();
	}
}
size_t Task::task_id() {
	if (!loc.is_task_thread)
		return 0;
	else
		return std::hash<size_t>()(reinterpret_cast<size_t>(loc.curr_task.getPtr()));
}
#pragma endregion
#pragma optimize("",off)
#pragma region Task: contexts swap
void Task::sleep_until(std::chrono::high_resolution_clock::time_point time_point) {
	if (loc.is_task_thread) {
		std::lock_guard guard(loc.curr_task->no_race);
		makeTimeWait(time_point);
		swapCtxRelock(loc.curr_task->no_race);
	}
	else
		std::this_thread::sleep_until(time_point);
}
void Task::yield() {
	if (loc.is_task_thread) {
		std::lock_guard guard(glob.task_thread_safety);
		glob.tasks.push(loc.curr_task);
		swapCtxRelock(glob.task_thread_safety);
	}
	else
		throw EnviropmentRuinException("Thread attempt return yield task in non task enviro");
}

#pragma endregion

#pragma region TaskMutex
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
		return false;
	}
	return true;
}

#pragma endregion

#pragma region TaskConditionVariable
TaskConditionVariable::TaskConditionVariable() {}

TaskConditionVariable::~TaskConditionVariable() {
	notify_all();
}

ValueItem* _tcv_wait(ValueItem* args, uint32_t len) {
	(*(bool*)args[0].val)=true;
	((std::condition_variable_any*)args[1].val)->notify_all();
	return nullptr;
}
typed_lgr<FuncEnviropment> tcv_wait(new FuncEnviropment(_tcv_wait, false));


void TaskConditionVariable::wait(std::unique_lock<MutexUnify>& mut) {
	if (loc.is_task_thread) {
		if (mut.mutex()->nmut == &no_race) {
			resume_task.push_back(loc.curr_task);
			swapCtxRelock(no_race);
		}
		else {
			std::lock_guard guard(no_race);
			resume_task.push_back(loc.curr_task);
			swapCtxRelock(no_race, *mut.mutex());
		}
	}
	else {
		std::condition_variable_any cd;
		bool has_res = false;
		ValueItem tmp{ ValueItem(&has_res, VType::undefined_ptr), ValueItem(&cd, VType::undefined_ptr) };
		typed_lgr task = new Task(tcv_wait, tmp);
		if (mut.mutex()->nmut == &no_race) {
			resume_task.push_back(task);
			while (!has_res)
				cd.wait(mut);
		}
		else {
			no_race.lock();
			resume_task.push_back(task);
			no_race.unlock();
			while (!has_res)
				cd.wait(mut);
		}
	}
}


bool TaskConditionVariable::wait_for(std::unique_lock<MutexUnify>& mut, size_t milliseconds) {
	return wait_until(mut, std::chrono::high_resolution_clock::now() + std::chrono::milliseconds(milliseconds));
}
ValueItem* _tcv_wait_until(ValueItem* args, uint32_t len) {
	(*(std::pair<bool,bool>*)args[4].val)={true,!loc.curr_task->time_end_flag};
	((std::condition_variable_any*)args[3].val)->notify_all();
	return nullptr;
}
typed_lgr<FuncEnviropment> tcv_wait_until(new FuncEnviropment(_tcv_wait_until, false));
bool TaskConditionVariable::wait_until(std::unique_lock<MutexUnify>& mut, std::chrono::high_resolution_clock::time_point time_point) {
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
		std::condition_variable_any cd;
		std::pair<bool,bool> has_res{false,false};
		ValueItem tmp{ ValueItem(this, VType::undefined_ptr),ValueItem(&mut, VType::undefined_ptr), ValueItem(&cd, VType::undefined_ptr), ValueItem(&has_res, VType::undefined_ptr), time_point.time_since_epoch().count()};
		typed_lgr task = new Task(tcv_wait_until, tmp);
		resume_task.push_back(task);
		if (mut.mutex()->nmut == &no_race) {
			resume_task.push_back(task);
			while (!has_res.first)
				cd.wait(mut);
		}
		else {
			no_race.lock();
			resume_task.push_back(task);
			no_race.unlock();
			while (!has_res.first)
				cd.wait(mut);
		}
		return has_res.second;
	}
	return true;
}

void TaskConditionVariable::notify_all() {
	std::list<typed_lgr<struct Task>> revive_tasks;
	{
		std::lock_guard guard(no_race);
		std::swap(revive_tasks, resume_task);
	}
	bool to_yield = false;
	{
		std::lock_guard guard(glob.task_thread_safety);
		for (auto& it : revive_tasks) {
			std::lock_guard guard_loc(it->no_race);
			if (!it->time_end_flag) {
				it->awaked = true;
				glob.tasks.push(it);
			}
		}
		if (Task::max_running_tasks && loc.is_task_thread) {
			if (Task::max_running_tasks <= glob.in_run_tasks && loc.curr_task && !loc.curr_task->end_of_life)
				to_yield = true;
		}
	}
	if (to_yield)
		Task::yield();
}
void TaskConditionVariable::notify_one() {
	typed_lgr<struct Task> tsk;
	{
		std::lock_guard guard(no_race);
		while (resume_task.size()) {
			resume_task.back()->no_race.lock();
			if (resume_task.back()->time_end_flag) {
				resume_task.back()->no_race.unlock();
				resume_task.pop_back();
			}
			else {
				tsk = resume_task.back();
				resume_task.pop_back();
				break;
			}
		}
		if (resume_task.empty())
			return;
	}
	bool to_yield = false;
	std::lock_guard guard_loc(tsk->no_race, std::adopt_lock);
	{
		std::lock_guard guard(glob.task_thread_safety);
		resume_task.back()->awaked = true;
		if (Task::max_running_tasks && loc.is_task_thread) {
			if (Task::max_running_tasks <= glob.in_run_tasks && loc.curr_task && !loc.curr_task->end_of_life)
				to_yield = true;
		}
		glob.tasks.push(resume_task.front());
	}
	glob.tasks_notifier.notify_one();
	if(to_yield)
		Task::yield();
}
#pragma endregion

#pragma region TaskSemaphore
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
		if (loc.is_task_thread) {
			std::lock_guard guard(glob.task_thread_safety);
			resume_task.push_back(loc.curr_task);
			no_race.unlock();
			swapCtxRelock(glob.task_thread_safety);
		}
		else {
			std::mutex mtx;
			std::unique_lock ulm(mtx);
			no_race.unlock();
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
#pragma endregion

#pragma region TaskLimiter

void TaskLimiter::set_max_treeshold(size_t val) {
	std::lock_guard guard(no_race);
	if (val < 1)
		val = 1;
	if (max_treeshold == val)
		return;
	if (max_treeshold > val) {
		if(allow_treeshold > max_treeshold - val)
			allow_treeshold -= max_treeshold - val;
		else {
			locked = true;
			allow_treeshold = 0;
		}
		max_treeshold = val;
		return;
	}
	else {
		if (!allow_treeshold) {
			size_t unlocks = max_treeshold;
			max_treeshold = val;
			if (allow_treeshold >= 1)
				locked = false;
			while (unlocks-- >= 1)
				unchecked_unlock();
		}
		else 
			allow_treeshold += val - max_treeshold;
	}
}
void TaskLimiter::lock() {
re_try:
	no_race.lock();
	if (!locked) {
		if (loc.is_task_thread) {
			loc.curr_task->awaked = false;
			loc.curr_task->time_end_flag = false;
			std::lock_guard guard(glob.task_thread_safety);
			resume_task.push_back(loc.curr_task);
			no_race.unlock();
			swapCtxRelock(glob.task_thread_safety);
		}
		else {
			std::mutex mtx;
			std::unique_lock ulm(mtx);
			no_race.unlock();
			native_notify.wait(ulm);
		}
		goto re_try;
	}
	else if (--allow_treeshold == 0)
			locked = true;

	if (lock_check.contains(loc.curr_task.getPtr())) {
		if (++allow_treeshold != 0)
			locked = false;
		no_race.unlock();
		throw InvalidLock("Dead lock. Task try lock already locked task limiter");
	}
	else 
		lock_check.push_back(loc.curr_task.getPtr());
	no_race.unlock();
	return;
}
bool TaskLimiter::try_lock() {
	if (!no_race.try_lock())
		return false;
	if (!locked) {
		no_race.unlock();
		return false;
	}
	else if (--allow_treeshold <= 0)
		locked = true;

	if (lock_check.contains(loc.curr_task.getPtr())) {
		if (++allow_treeshold != 0)
			locked = false;
		no_race.unlock();
		throw InvalidLock("Dead lock. Task try lock already locked task limiter");
	}
	else
		lock_check.push_back(loc.curr_task.getPtr());
	no_race.unlock();
	return true;
}

bool TaskLimiter::try_lock_for(size_t milliseconds) {
	return try_lock_until(std::chrono::high_resolution_clock::now() + std::chrono::milliseconds(milliseconds));
}
bool TaskLimiter::try_lock_until(std::chrono::high_resolution_clock::time_point time_point) {
re_try:
	if (!no_race.try_lock_until(time_point))
		return false;
	if (!locked) {
		if (loc.is_task_thread) {
			loc.curr_task->awaked = false;
			loc.curr_task->time_end_flag = false;
			std::lock_guard guard(glob.task_thread_safety);
			makeTimeWait(time_point);
			resume_task.push_back(loc.curr_task);
			no_race.unlock();
			swapCtxRelock(glob.task_thread_safety);
			if (!loc.curr_task->awaked)
				return false;
		}
		else {
			std::mutex mtx;
			std::unique_lock ulm(mtx);
			no_race.unlock();
			if (native_notify.wait_until(ulm, time_point) == std::cv_status::timeout)
				return false;
		}
		goto re_try;
	}
	else if (--allow_treeshold <= 0)
		locked = true;

	if (lock_check.contains(loc.curr_task.getPtr())) {
		if (++allow_treeshold != 0)
			locked = false;
		no_race.unlock();
		throw InvalidLock("Dead lock. Task try lock already locked task limiter");
	}
	else
		lock_check.push_back(loc.curr_task.getPtr());
	no_race.unlock();
	return true;

}
void TaskLimiter::unlock() {
	std::lock_guard lg0(no_race);
	if (!lock_check.contains(loc.curr_task.getPtr()))
		throw InvalidLock("Invalid unlock. Task try unlock already unlocked task limiter");
	else
		lock_check.erase(loc.curr_task.getPtr());
	unchecked_unlock();
}
void TaskLimiter::unchecked_unlock() {
	if (allow_treeshold >= max_treeshold)
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

bool TaskLimiter::is_locked() {
	return locked;
}
#pragma endregion
#pragma optimize("",on)

#pragma region ConcurentFile
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
ValueItem* concurentReader(ValueItem* arguments, uint32_t) {
	typed_lgr<ConcurentFile> pfile = std::move(*(typed_lgr<ConcurentFile>*)arguments[0].val);
	delete (typed_lgr<ConcurentFile>*)arguments[0].val;

	std::fstream& stream = pfile->stream;
	size_t len = (size_t)arguments[1].val;
	size_t pos = (size_t)arguments[2].val;
	std::lock_guard guard(pfile->no_race);
	if (!stream.is_open())
		Task::self_cancel();
	if (pos != size_t(-1)) {
		pfile->last_op_append = false;
		stream.flush();
		stream.seekg(pos);
	}
	if (len >= UINT32_MAX) {
		auto pos = pfile->stream.tellg();
		pos += len;
		pfile->stream.seekg(pos);
		try {
			throw OutOfRange("Array length is too large for store in raw array type");
		}
		catch (...) {
			return new ValueItem((void*)new std::exception_ptr(std::current_exception()), VType::except_value, true);
		}
	}
	else {
		char* res = new char[len];
		pfile->stream.read(res, len);
		return new ValueItem(res, ValueMeta(VType::raw_arr_ui8, false, true, (uint32_t)len), true);
	}
}
typed_lgr<FuncEnviropment> ConcurentReader(new FuncEnviropment(concurentReader, false));
ValueItem* concurentReaderLong(ValueItem* arguments, uint32_t) {
	typed_lgr<ConcurentFile> pfile = std::move(*(typed_lgr<ConcurentFile>*)arguments[0].val);
	delete (typed_lgr<ConcurentFile>*)arguments[0].val;

	std::fstream& stream = pfile->stream;
	size_t len = (size_t)arguments[1].val;
	size_t pos = (size_t)arguments[2].val;
	std::lock_guard guard(pfile->no_race);
	if (!stream.is_open())
		Task::self_cancel();
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
		VType::uarr
	);
}
typed_lgr<FuncEnviropment> ConcurentReaderLong(new FuncEnviropment(concurentReaderLong, false));

ValueItem* concurentWriter(ValueItem* arguments, uint32_t) {
	typed_lgr<ConcurentFile> pfile = std::move(*(typed_lgr<ConcurentFile>*)arguments[0].val);
	delete (typed_lgr<ConcurentFile>*)arguments[0].val;

	char* value = (char*)arguments[1].val;
	uint32_t len = (uint32_t)arguments[1].meta.val_len;
	size_t pos = (size_t)arguments[2].val;
	bool is_append = (size_t)arguments[3].val;
	std::fstream& stream = pfile->stream;

	std::lock_guard guard(pfile->no_race);
	if (!stream.is_open())
		Task::self_cancel();
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
ValueItem* concurentWriterLong(ValueItem* arguments, uint32_t) {
	typed_lgr<ConcurentFile> pfile = std::move(*(typed_lgr<ConcurentFile>*)arguments[0].val);
	delete (typed_lgr<ConcurentFile>*)arguments[0].val;

	list_array<uint8_t>* value = (list_array<uint8_t>*)arguments[1].val;
	size_t len = value->size();
	size_t pos = (size_t)arguments[2].val;
	bool is_append = (size_t)arguments[3].val;
	mem_tool::ValDeleter deleter0(value);
	std::fstream& stream = pfile->stream;

	std::lock_guard guard(pfile->no_race);
	if (!stream.is_open())
		Task::self_cancel();
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
typed_lgr<Task> ConcurentFile::read(typed_lgr<ConcurentFile>& file, uint32_t len, uint64_t pos) {
	ValueItem tmp[]{
				ValueItem(new typed_lgr<ConcurentFile>(file),ValueMeta(VType::undefined_ptr,false,true), true),
				ValueItem((void*)(0ull + len),ValueMeta(VType::ui64,false,true), true),
				ValueItem((void*)pos,ValueMeta(VType::ui64,false,true), true)
	};
	ValueItem args(tmp);
	typed_lgr<Task> aa(new Task(ConcurentReader, args));
	Task::start(aa);
	return aa;
}
typed_lgr<Task> ConcurentFile::write(typed_lgr<ConcurentFile>& file, char* arr, uint32_t len, uint64_t pos) {
	ValueItem tmp[]{
				ValueItem(new typed_lgr<ConcurentFile>(file),ValueMeta(VType::undefined_ptr,false,true), true),
				ValueItem(arr,ValueMeta(VType::raw_arr_ui8,false,true,len), true),
				ValueItem((void*)pos,ValueMeta(VType::ui64,false,true), true),
				ValueItem((void*)false,ValueMeta(VType::ui8,false,true), true)
	};
	ValueItem args(tmp);
	typed_lgr<Task> aa(new Task(ConcurentWriter, args));
	Task::start(aa);
	return aa;
}
typed_lgr<Task> ConcurentFile::append(typed_lgr<ConcurentFile>& file, char* arr, uint32_t len) {
	ValueItem tmp[]{
				ValueItem(new typed_lgr<ConcurentFile>(file),ValueMeta(VType::undefined_ptr,false,true), true),
				ValueItem(arr, ValueMeta(VType::raw_arr_ui8,false,true,len), true),
				ValueItem((void*)-1,ValueMeta(VType::ui64,false,true), true),
				ValueItem((void*)false,ValueMeta(VType::ui8,false,true), true)
	};
	ValueItem args(tmp);
	typed_lgr<Task> aa(new Task(ConcurentWriter, args));
	Task::start(aa);
	return aa;
}

typed_lgr<Task> ConcurentFile::read_long(typed_lgr<ConcurentFile>& file, uint64_t len, uint64_t pos) {
	ValueItem tmp[]{
				ValueItem(new typed_lgr<ConcurentFile>(file),ValueMeta(VType::undefined_ptr,false,true), true),
				ValueItem((void*)len ,ValueMeta(VType::ui64,false,true), true),
				ValueItem((void*)pos,ValueMeta(VType::ui64,false,true), true)
	};
	ValueItem args(tmp);
	typed_lgr<Task> aa(new Task(ConcurentReaderLong, args));
	Task::start(aa);
	return aa;
}
typed_lgr<Task> ConcurentFile::write_long(typed_lgr<ConcurentFile>& file, list_array<uint8_t>* arr, uint64_t pos) {
	ValueItem tmp[]{
				ValueItem(new typed_lgr<ConcurentFile>(file),ValueMeta(VType::undefined_ptr,false,true), true),
				ValueItem(arr,ValueMeta(VType::undefined_ptr,false,true), true),
				ValueItem((void*)pos,ValueMeta(VType::ui64,false,true), true),
				ValueItem((void*)false,ValueMeta(VType::ui8,false,true), true)
	};
	ValueItem args(tmp);
	typed_lgr<Task> aa(new Task(ConcurentWriterLong, args));
	Task::start(aa);
	return aa;
}
typed_lgr<Task> ConcurentFile::append_long(typed_lgr<ConcurentFile>& file, list_array<uint8_t>* arr) {
	ValueItem tmp[]{
				ValueItem(new typed_lgr<ConcurentFile>(file),ValueMeta(VType::undefined_ptr,false,true), true),
				ValueItem(arr, ValueMeta(VType::undefined_ptr,false,true), true),
				ValueItem((void*)-1,ValueMeta(VType::ui64,false,true), true),
				ValueItem((void*)true,ValueMeta(VType::ui8,false,true), true)
	};
	ValueItem args(tmp);
	typed_lgr<Task> aa(new Task(ConcurentWriterLong, args));
	Task::start(aa);
	return aa;
}

bool ConcurentFile::is_open(typed_lgr<ConcurentFile>& file) {
	std::lock_guard guard(file->no_race);
	return file->stream.is_open();
}
void ConcurentFile::close(typed_lgr<ConcurentFile>& file) {
	std::lock_guard guard(file->no_race);
	file->stream.close();
}
#pragma endregion



#pragma region EventSystem
bool EventSystem::removeOne(std::list<typed_lgr<FuncEnviropment>>& list, const typed_lgr<FuncEnviropment>& func) {
	auto iter = list.begin();
	auto end = list.begin();
	while (iter != end) {
		if (*iter == func) {
			list.erase(iter);
			return true;
		}
	}
	return false;
}
void EventSystem::async_call(std::list<typed_lgr<FuncEnviropment>>& list, ValueItem& args) {
	std::lock_guard guard(no_race);
	for (auto& it : list)
		Task::start(typed_lgr<Task>(new Task(it, args)));
}
bool EventSystem::awaitCall(std::list<typed_lgr<FuncEnviropment>>& list, ValueItem& args) {
	std::list<typed_lgr<Task>> wait_tasks;
	{
		std::lock_guard guard(no_race);
		for (auto& it : list) {
			typed_lgr<Task> tsk(new Task(it, args));
			wait_tasks.push_back(tsk);
			Task::start(tsk);
		}
	}
	bool need_cancel = false;
	for (auto& it : wait_tasks) {
		if (need_cancel) {
			it->make_cancel = true;
			continue;
		}
		auto res = Task::get_result(it);
		if (res) {
			if (isTrueValue(&res->val))
				need_cancel = true;
			delete res;
		}
	}
	return need_cancel;
}
bool EventSystem::sync_call(std::list<typed_lgr<FuncEnviropment>>& list, ValueItem& args) {
	std::lock_guard guard(no_race);
	if (args.meta.vtype == VType::async_res)
		args.getAsync();
	if (args.meta.vtype == VType::noting)
		for (typed_lgr<FuncEnviropment>& it : list) {
			auto res = it->syncWrapper(nullptr, 0);
			if (res)
				if (isTrueValue(&res->val)) {
					delete res;
					return true;
				}
		}
	else
		for (typed_lgr<FuncEnviropment>& it : list) {
			ValueItem copyArgs;
			if (args.meta.vtype == VType::faarr || args.meta.vtype == VType::saarr) {
				if (!args.meta.use_gc)
					copyArgs = args;
				else
					copyArgs = ValueItem((ValueItem*)args.getSourcePtr(), args.meta.val_len);
			}
			else {
				if (args.meta.vtype != VType::noting)
					copyArgs = ValueItem(&args, 1);
			}
			auto res = it->syncWrapper((ValueItem*)copyArgs.val, copyArgs.meta.val_len);
			if (res) {
				if (isTrueValue(&res->val)) {
					delete res;
					return true;
				}
			}
		}
		return false;
}

void EventSystem::operator+=(const typed_lgr<FuncEnviropment>& func) {
	std::lock_guard guard(no_race);
	avg_priorihty.push_back(func);
}
void EventSystem::join(const typed_lgr<FuncEnviropment>& func, bool async_mode, Priorithy priorithy) {
	std::lock_guard guard(no_race);
	if (async_mode) {
		switch (priorithy) {
		case Priorithy::heigh:
			async_heigh_priorihty.push_back(func);
			break;
		case Priorithy::upper_avg:
			async_upper_avg_priorihty.push_back(func);
			break;
		case Priorithy::avg:
			async_avg_priorihty.push_back(func);
			break;
		case Priorithy::lower_avg:
			async_lower_avg_priorihty.push_back(func);
			break;
		case Priorithy::low:
			async_low_priorihty.push_back(func);
			break;
		default:
			break;
		}
	}
	else {
		switch (priorithy) {
		case Priorithy::heigh:
			heigh_priorihty.push_back(func);
			break;
		case Priorithy::upper_avg:
			upper_avg_priorihty.push_back(func);
			break;
		case Priorithy::avg:
			avg_priorihty.push_back(func);
			break;
		case Priorithy::lower_avg:
			lower_avg_priorihty.push_back(func);
			break;
		case Priorithy::low:
			low_priorihty.push_back(func);
			break;
		default:
			break;
		}
	}
}
bool EventSystem::leave(const typed_lgr<FuncEnviropment>& func, bool async_mode, Priorithy priorithy) {
	std::lock_guard guard(no_race);
	if (async_mode) {
		switch (priorithy) {
		case Priorithy::heigh:
			return removeOne(async_heigh_priorihty, func);
		case Priorithy::upper_avg:
			return removeOne(async_upper_avg_priorihty, func);
		case Priorithy::avg:
			return removeOne(async_avg_priorihty, func);
		case Priorithy::lower_avg:
			return removeOne(async_lower_avg_priorihty, func);
		case Priorithy::low:
			return removeOne(async_low_priorihty, func);
		default:
			return false;
		}
	}
	else {
		switch (priorithy) {
		case Priorithy::heigh:
			return removeOne(heigh_priorihty, func);
		case Priorithy::upper_avg:
			return removeOne(upper_avg_priorihty, func);
		case Priorithy::avg:
			return removeOne(avg_priorihty, func);
		case Priorithy::lower_avg:
			return removeOne(lower_avg_priorihty, func);
		case Priorithy::low:
			return removeOne(low_priorihty, func);
		default:
			return false;
		}
	}
}

bool EventSystem::await_notify(ValueItem& it) {
	if(sync_call(heigh_priorihty, it)	) return true;
	if(sync_call(upper_avg_priorihty, it)) return true;
	if(sync_call(avg_priorihty, it)		) return true;
	if(sync_call(lower_avg_priorihty, it)) return true;
	if(sync_call(low_priorihty, it)		) return true;

	if(awaitCall(async_heigh_priorihty, it)		) return true;
	if(awaitCall(async_upper_avg_priorihty, it)	) return true;
	if(awaitCall(async_avg_priorihty, it)		) return true;
	if(awaitCall(async_lower_avg_priorihty, it)	) return true;
	if(awaitCall(async_low_priorihty, it)		) return true;
	return false;
}
bool EventSystem::notify(ValueItem& it) {
	if (sync_call(heigh_priorihty, it)) return true;
	if (sync_call(upper_avg_priorihty, it)) return true;
	if (sync_call(avg_priorihty, it)) return true;
	if (sync_call(lower_avg_priorihty, it)) return true;
	if (sync_call(low_priorihty, it)) return true;

	async_call(async_heigh_priorihty, it);
	async_call(async_upper_avg_priorihty, it);
	async_call(async_avg_priorihty, it);
	async_call(async_lower_avg_priorihty, it);
	async_call(async_low_priorihty, it);
	return false;
}
bool EventSystem::sync_notify(ValueItem& it) {
	if(sync_call(heigh_priorihty, it)	) return true;
	if(sync_call(upper_avg_priorihty, it)) return true;
	if(sync_call(avg_priorihty, it)		) return true;
	if(sync_call(lower_avg_priorihty, it)) return true;
	if(sync_call(low_priorihty, it)		) return true;
	if(sync_call(async_heigh_priorihty, it)		) return true;
	if(sync_call(async_upper_avg_priorihty, it)	) return true;
	if(sync_call(async_avg_priorihty, it)		) return true;
	if(sync_call(async_lower_avg_priorihty, it)	) return true;
	if(sync_call(async_low_priorihty, it)		) return true;
	return false;
}

ValueItem* __async_notify(ValueItem* vals, uint32_t) {
	EventSystem* es = (EventSystem*)vals->val;
	ValueItem& args = *(ValueItem*)vals[1].val;
	if (es->sync_call(es->heigh_priorihty, args)) return new ValueItem(true);
	if (es->sync_call(es->upper_avg_priorihty, args)) return new ValueItem(true);
	if (es->sync_call(es->avg_priorihty, args)) return new ValueItem(true);
	if (es->sync_call(es->lower_avg_priorihty, args)) return new ValueItem(true);
	if (es->sync_call(es->low_priorihty, args)) return new ValueItem(true);
	if (es->awaitCall(es->async_heigh_priorihty, args)) return new ValueItem(true);
	if (es->awaitCall(es->async_upper_avg_priorihty, args)) return new ValueItem(true);
	if (es->awaitCall(es->async_avg_priorihty, args)) return new ValueItem(true);
	if (es->awaitCall(es->async_lower_avg_priorihty, args)) return new ValueItem(true);
	if (es->awaitCall(es->async_low_priorihty, args)) return new ValueItem(true);
	return new ValueItem(false);
}
typed_lgr<FuncEnviropment> _async_notify(new FuncEnviropment(__async_notify,false));

typed_lgr<Task> EventSystem::async_notify(ValueItem& args) {
	ValueItem vals(new ValueItem[2]{ ValueItem(this,VType::undefined_ptr), args });
	typed_lgr<Task> res = new Task(_async_notify, vals);
	Task::start(res);
	return res;
}
#pragma endregion