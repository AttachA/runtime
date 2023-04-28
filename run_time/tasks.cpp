// Copyright Danyil Melnytskyi 2022-2023
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <list>
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
#include "tasks_util/light_stack.hpp"
#include "library/parallel.hpp"







#undef min
namespace ctx = boost::context;
constexpr size_t native_thread_flag = (SIZE_MAX ^ -1);
ValueItem* _empty_func(ValueItem* /*ignored*/, uint32_t /*ignored*/) {
	return nullptr;
}
typed_lgr<FuncEnviropment> empty_func(new FuncEnviropment(_empty_func, false));
//first arg bool& check
//second arg std::condition_variable_any
ValueItem* _notify_native_thread(ValueItem* args, uint32_t /*ignored*/) {
	*((bool*)args[0].val) = true;
	((std::condition_variable_any*)args[1].val)->notify_one();
	return nullptr;
}
typed_lgr<FuncEnviropment> notify_native_thread(new FuncEnviropment(_notify_native_thread, false));


#pragma region MutexUnify
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
#pragma endregion


#pragma region MultiplyMutex
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
#pragma endregion


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


#pragma region TaskResult
TaskResult::~TaskResult() {
	if (context) 
		reinterpret_cast<ctx::continuation&>(context).~continuation();
}


ValueItem* TaskResult::getResult(size_t res_num, std::unique_lock<MutexUnify>& l) {
	if (results.size() >= res_num) {
		while (results.size() >= res_num && !end_of_life)
			result_notify.wait(l);
		if (results.size() >= res_num)
			return new ValueItem();
	}
	return new ValueItem(results[res_num]);
}
void TaskResult::awaitEnd(std::unique_lock<MutexUnify>& l) {
	while (!end_of_life)
		result_notify.wait(l);
}
void TaskResult::yieldResult(ValueItem* res, std::unique_lock<MutexUnify>& l, bool release) {
	if (res) {
		results.push_back(std::move(*res));
		if (release)
			delete res;
	}
	else
		results.push_back(ValueItem());
	result_notify.notify_all();
}
void TaskResult::yieldResult(ValueItem&& res, std::unique_lock<MutexUnify>& l) {
	results.push_back(std::move(res));
	result_notify.notify_all();
}
void TaskResult::finalResult(ValueItem* res, std::unique_lock<MutexUnify>& l) {
	if (res) {
		results.push_back(std::move(*res));
		delete res;
	}
	else
		results.push_back(ValueItem());
	end_of_life = true;
	result_notify.notify_all();
}
void TaskResult::finalResult(ValueItem&& res, std::unique_lock<MutexUnify>& l) {
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
#pragma endregion
struct timing {
	std::chrono::high_resolution_clock::time_point wait_timepoint; 
	typed_lgr<Task> awake_task; 
	//size_t check_id;
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
	typed_lgr<Generator> on_load_generator_ref = nullptr;

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
		ValueItem* res = loc.curr_task->func->syncWrapper((ValueItem*)loc.curr_task->args.val, loc.curr_task->args.meta.val_len);
		MutexUnify mu(loc.curr_task->no_race);
		std::unique_lock l(mu);
		loc.curr_task->fres.finalResult(res,l);
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
		ValueItem* res = loc.curr_task->ex_handle->syncWrapper((ValueItem*)loc.curr_task->args.val, loc.curr_task->args.meta.val_len);
		MutexUnify mu(loc.curr_task->no_race);
		std::unique_lock l(mu);
		loc.curr_task->fres.finalResult(res, l);
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
#define worker_mode_desk(mode) if(enable_thread_naming)worker_mode_desk_(mode);
void worker_mode_desk_(const std::string& mode) {
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
		std::unique_lock guard(glob.task_thread_safety);
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
			glob.tasks_notifier.wait(guard);
		}
		loc.is_task_thread = true;
		if (loadTask())
			continue;
		guard.unlock();
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
			*loc.tmp_current_context = ctx::callcc(std::allocator_arg, light_stack(1048576/*1 mb*/), context_exec);
		}
		if (loc.ex_ptr) {
			if (loc.curr_task->ex_handle) {
				ValueItem temp(new std::exception_ptr(loc.ex_ptr), VType::except_value, no_copy);
				loc.curr_task->args = ValueItem(&temp, 0);
				loc.ex_ptr = nullptr;
				*loc.tmp_current_context = ctx::callcc(context_ex_handle);

				if (!loc.ex_ptr)
					goto end_task;
			}
			MutexUnify uni(loc.curr_task->no_race);
			std::unique_lock l(uni);
			loc.curr_task->fres.finalResult(ValueItem(new std::exception_ptr(loc.ex_ptr), ValueMeta(VType::except_value), no_copy),l);
			loc.ex_ptr = nullptr;
		}

	end_task:
		if (loc.curr_task) {
			loc.curr_task->relock_0.unlock();
			loc.curr_task->relock_1.unlock();
			loc.curr_task->relock_2.unlock();
		}
		loc.curr_task = nullptr;
		worker_mode_desk("idle");
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
					//if (tmng.check_id != tmng.awake_task->sleep_check) {
					//	glob.timed_tasks.pop_front();
					//	continue;
					//}
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
				glob.timed_tasks.insert(it, timing(t,loc.curr_task/*, ++loc.curr_task->sleep_check */ ));
				i = -1;
				break;
			}
			++it;
		}
		if (i != -1)
			glob.timed_tasks.push_back(timing(t, loc.curr_task/*, ++loc.curr_task->sleep_check*/));
	}
	glob.time_notifier.notify_one();
}

#pragma region Task
Task::Task(typed_lgr<class FuncEnviropment> call_func, const ValueItem& arguments, bool used_task_local, typed_lgr<class FuncEnviropment> exception_handler, std::chrono::high_resolution_clock::time_point task_timeout) {
	ex_handle = exception_handler;
	func = call_func;
	if (arguments.meta.vtype == VType::faarr || arguments.meta.vtype == VType::saarr)
		args = arguments;
	else if (arguments.meta.vtype != VType::noting)
		args = {arguments};
	
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
Task::Task(typed_lgr<class FuncEnviropment> call_func, ValueItem&& arguments, bool used_task_local, typed_lgr<class FuncEnviropment> exception_handler, std::chrono::high_resolution_clock::time_point task_timeout) {
	ex_handle = exception_handler;
	func = call_func;
	if (arguments.meta.vtype == VType::faarr || arguments.meta.vtype == VType::saarr)
		args = std::move(arguments);
	else if (arguments.meta.vtype != VType::noting){
		ValueItem *arr = new ValueItem[1];
		arr[0] = std::move(arguments);
		args = std::move(ValueItem(arr, 1, no_copy));
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
	MutexUnify uni(lgr_task->no_race);
	std::unique_lock l(uni);
	return lgr_task->fres.getResult(yield_res, l);
}
ValueItem* Task::get_result(typed_lgr<Task>&& lgr_task, size_t yield_res) {
	if (!lgr_task->started)
		Task::start(lgr_task);

	MutexUnify uni(lgr_task->no_race);
	std::unique_lock l(uni);
	return lgr_task->fres.getResult(yield_res,l);
}
bool Task::has_result(typed_lgr<Task>& lgr_task, size_t yield_res){
	return lgr_task->fres.results.size() > yield_res;
}
void Task::await_task(typed_lgr<Task>& lgr_task, bool make_start) {
	if (!lgr_task->started && make_start)
		Task::start(lgr_task);
	MutexUnify uni(lgr_task->no_race);
	std::unique_lock l(uni);
	lgr_task->fres.awaitEnd(l);
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
	if (Task::max_running_tasks > glob.in_run_tasks || !Task::max_running_tasks)
		glob.tasks.push(lgr_task);
	else
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
	if (be_executor && !loc.is_task_thread) {
		std::unique_lock l(glob.task_thread_safety);
		while (glob.tasks.size() || glob.cold_tasks.size() || glob.timed_tasks.size() || glob.in_exec || glob.tasks_in_swap) {
			l.unlock();
			try {
				taskExecutor(true);
			}
			catch (...) {
				l.lock();
				throw;
			}
			l.lock();
		}
	}
	else {
		MutexUnify uni(glob.task_thread_safety);
		std::unique_lock l(uni);
		if(loc.is_task_thread)
			while ((glob.tasks.size() || glob.cold_tasks.size() || glob.timed_tasks.size()) && glob.in_exec != 1 && glob.tasks_in_swap != 1)
				glob.no_tasks_execute_notifier.wait(l);
		else
			while (glob.tasks.size() || glob.cold_tasks.size() || glob.timed_tasks.size() || glob.in_exec  || glob.tasks_in_swap)
				glob.no_tasks_execute_notifier.wait(l);
	}
}
void Task::await_multiple(list_array<typed_lgr<Task>>& tasks, bool pre_started, bool release) {
	if (!pre_started) {
		for (auto& it : tasks)
			Task::start(it);
	}
	if (release) {
		for (auto& it : tasks) {
			MutexUnify uni(it->no_race);
			std::unique_lock l(uni);
			it->fres.awaitEnd(l);
			l.unlock();
			it = nullptr;
		}
	}
	else 
		for (auto& it : tasks) {
			MutexUnify uni(it->no_race);
			std::unique_lock l(uni);
			it->fres.awaitEnd(l);
		}
}
void Task::await_multiple(typed_lgr<Task>* tasks, size_t len, bool pre_started, bool release) {
	if (!pre_started) {
		typed_lgr<Task>* iter = tasks;
		size_t count = len;
		while (count--)
			Task::start(*iter++);
	}
	if (release) {
		while (len--) {
			MutexUnify uni((*tasks)->no_race);
			std::unique_lock l(uni);
			(*tasks)->fres.awaitEnd(l);
			l.unlock();
			(*tasks++) = nullptr;
		}
	}
	else 
		while (len--) {
			MutexUnify uni((*tasks)->no_race);
			std::unique_lock l(uni);
			(*tasks++)->fres.awaitEnd(l);
		}
}
void Task::sleep(size_t milliseconds) {
	sleep_until(std::chrono::high_resolution_clock::now() + std::chrono::milliseconds(milliseconds));
}



void Task::result(ValueItem* f_res) {
	if (loc.is_task_thread) {
		MutexUnify uni(loc.curr_task->no_race);
		std::unique_lock l(uni);
		loc.curr_task->fres.yieldResult(f_res, l);
	}
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
	else if (loc.curr_task->_task_local) 
		return loc.curr_task->_task_local;
	else
		return loc.curr_task->_task_local = new ValueEnvironment();
}
size_t Task::task_id() {
	if (!loc.is_task_thread)
		return 0;
	else
		return std::hash<size_t>()(reinterpret_cast<size_t>(loc.curr_task.getPtr()));
}

bool Task::is_task(){
	return loc.is_task_thread;
}


void Task::clean_up() {
	Task::await_no_tasks();
	std::queue<typed_lgr<Task>> e0;
	std::queue<typed_lgr<Task>> e1;
	glob.tasks.swap(e0);
	glob.cold_tasks.swap(e1);
	glob.timed_tasks.shrink_to_fit();
}


typed_lgr<Task> Task::dummy_task(){
	return new Task(empty_func, ValueItem());
}
typed_lgr<Task> Task::cxx_native_bridge(bool& checker, std::condition_variable_any& cd){
	return new Task(notify_native_thread, ValueItem{ ValueItem(&checker, VType::undefined_ptr), ValueItem(std::addressof(cd), VType::undefined_ptr) });
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
		MutexUnify uni(tsk->no_race);
		std::unique_lock l(uni);
		tsk->fres.awaitEnd(l);
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
	else {
		std::unique_lock ul(no_race);
		std::condition_variable_any cd;
		bool has_res = false;
		typed_lgr<Task> task; 
		while (current_task) {
			task = Task::cxx_native_bridge(has_res, cd);
			resume_task.push_back(task);
			while (!has_res)
				cd.wait(ul);
		}
		current_task = reinterpret_cast<Task*>((size_t)_thread_id() | native_thread_flag);
		
		if(!task)
			return;
	task_not_ended:
		//prevent destruct cd, because it is used in task
		if (!task->fres.end_of_life)
			goto task_not_ended;
	}
}
bool TaskMutex::try_lock() {
	if (!no_race.try_lock())
		return false;
	std::unique_lock ul(no_race, std::adopt_lock);

	if (current_task)
		return false;
	else if(loc.is_task_thread)
		current_task = loc.curr_task.getPtr();
	else
		current_task = reinterpret_cast<Task*>((size_t)_thread_id() | native_thread_flag);
	return true;
}

bool TaskMutex::try_lock_for(size_t milliseconds) {
	return try_lock_until(std::chrono::high_resolution_clock::now() + std::chrono::milliseconds(milliseconds));
}
bool TaskMutex::try_lock_until(std::chrono::high_resolution_clock::time_point time_point) {
	if (!no_race.try_lock_until(time_point))
		return false;
	std::unique_lock ul(no_race, std::adopt_lock);

	if (loc.is_task_thread) {
		while (current_task) {
			std::lock_guard guard(loc.curr_task->no_race);
			makeTimeWait(time_point);
			resume_task.push_back(loc.curr_task);
			swapCtxRelock(loc.curr_task->no_race, no_race);
			if (!loc.curr_task->awaked) 
				return false;
		}
		current_task = loc.curr_task.getPtr();
		return true;
	}
	else {
		bool has_res;
		std::condition_variable_any cd;
		while (current_task) {
			has_res = false;
			typed_lgr task = Task::cxx_native_bridge(has_res, cd);
			resume_task.push_back(task);
			while (has_res)
				cd.wait_until(ul, time_point);
			if (!task->awaked)
				return false;
		}
		current_task = loc.curr_task.getPtr();
		return true;
	}
}
void TaskMutex::unlock() {
	std::lock_guard lg0(no_race);
	if (loc.is_task_thread) {
		if (current_task != loc.curr_task.getPtr())
			throw InvalidOperation("Tried unlock non owned mutex");
	}
	else if (current_task != reinterpret_cast<Task*>((size_t)_thread_id() | native_thread_flag))
		throw InvalidOperation("Tried unlock non owned mutex");

	current_task = nullptr;
	if (resume_task.size()) {
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

bool TaskMutex::is_locked() {
	if (try_lock()) {
		unlock();
		return false;
	}
	return true;
}

ValueItem* _TaskMutex_lock_holder(ValueItem* args, uint32_t len){
	typed_lgr<Task>& task = *(typed_lgr<Task>*)args[0].val;
	TaskMutex* mut = (TaskMutex*)args[1].val;
	bool lock_sequence = args[2].val;
	std::unique_lock guard(*mut, std::defer_lock);
	while(!task->fres.end_of_life){
		guard.lock();
		Task::await_task(task);
		if(!lock_sequence)
			return nullptr;
		{
			std::lock_guard task_guard(task->no_race);
			if(!task->fres.result_notify.has_waiters())
				task->fres.results.clear();
		}
		guard.unlock();
	}
	return nullptr;
}
typed_lgr<FuncEnviropment> TaskMutex_lock_holder = new FuncEnviropment(_TaskMutex_lock_holder, false);
void TaskMutex::lifecycle_lock(typed_lgr<struct Task> task){
	Task::start(new Task(TaskMutex_lock_holder, ValueItem{ValueItem(new typed_lgr(task), VType::async_res), this, false}));
}
void TaskMutex::sequence_lock(typed_lgr<struct Task> task){
	Task::start(new Task(TaskMutex_lock_holder, ValueItem{ValueItem(new typed_lgr(task), VType::async_res), this, true}));
}


#pragma endregion

#pragma region TaskConditionVariable
TaskConditionVariable::TaskConditionVariable() {}

TaskConditionVariable::~TaskConditionVariable() {
	notify_all();
}

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
		typed_lgr task = Task::cxx_native_bridge(has_res, cd);
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
	task_not_ended:
		//prevent destruct cd, because it is used in task
		if (!task->fres.end_of_life) 
			goto task_not_ended;
	}
}


bool TaskConditionVariable::wait_for(std::unique_lock<MutexUnify>& mut, size_t milliseconds) {
	return wait_until(mut, std::chrono::high_resolution_clock::now() + std::chrono::milliseconds(milliseconds));
}

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
		bool has_res = false;
		typed_lgr task = Task::cxx_native_bridge(has_res, cd);
	task_not_ended:
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

		//prevent destruct cd, because it is used in task
		if(!task->fres.end_of_life)
			goto task_not_ended;

		return !task->time_end_flag;
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
void TaskConditionVariable::dummy_wait(typed_lgr<struct Task> task, std::unique_lock<MutexUnify>& lock){
	if(lock.mutex()->nmut == &no_race)
		resume_task.push_back(task);
	else{
		std::lock_guard guard(no_race);
		resume_task.push_back(task);
	}
}
void TaskConditionVariable::dummy_wait_for(typed_lgr<struct Task> task, std::unique_lock<MutexUnify>& lock, size_t milliseconds){
	dummy_wait_until(task, lock, std::chrono::high_resolution_clock::now() + std::chrono::milliseconds(milliseconds));
}

ValueItem* _TaskConditionVariable_dummy_awaiter(ValueItem* args, uint32_t len){
	typed_lgr<Task>& task = *(typed_lgr<Task>*)args[0].val;
	TaskConditionVariable* cv = (TaskConditionVariable*)args[1].val;
	std::chrono::high_resolution_clock::time_point& time_point = (std::chrono::high_resolution_clock::time_point&)args[2];
	std::unique_lock<MutexUnify> lock(*(MutexUnify*)args[3].val, std::defer_lock);
	{
		MutexUnify uni(loc.curr_task->no_race);
		std::unique_lock l(uni);
		ValueItem noting;
		loc.curr_task->fres.yieldResult(&noting, l,false);
		//signal to parent task that it is ready to wait
	}
	if (cv->wait_until(lock, time_point))
		task->args = {true};
	else 
		task->args = {false};
	Task::start(task);
	return nullptr;
}
void TaskConditionVariable::dummy_wait_until(typed_lgr<struct Task> task, std::unique_lock<MutexUnify>& lock, std::chrono::high_resolution_clock::time_point time_point){
	static typed_lgr<FuncEnviropment> TaskConditionVariable_dummy_awaiter = new FuncEnviropment(_TaskConditionVariable_dummy_awaiter, false);
	delete Task::get_result(new Task(TaskConditionVariable_dummy_awaiter, ValueItem{ValueItem(new typed_lgr(task), VType::async_res), this, time_point, lock.mutex()}));
}

bool TaskConditionVariable::has_waiters(){
	std::lock_guard guard(no_race);
	return !resume_task.empty();
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
		throw InvalidUnlock("Invalid unlock. Task try unlock already unlocked task limiter");
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
			return new ValueItem((void*)new std::exception_ptr(std::current_exception()), VType::except_value, no_copy);
		}
	}
	else {
		char* res = new char[len];
		pfile->stream.read(res, len);
		return new ValueItem(res, ValueMeta(VType::raw_arr_ui8, false, true, (uint32_t)len), no_copy);
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
		return new ValueItem(new AException("system fault file eof", "No more bytes aviable, end of file"), VType::except_value, no_copy);
	else if (stream.fail())
		return new ValueItem(new AException("system fault file fail", "Fail extract bytes"), VType::except_value, no_copy);

	else if (stream.bad())
		return new ValueItem(new AException("system fault file bad", "Stream maybe corrupted"), VType::except_value, no_copy);
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
		return new ValueItem(new AException("system fault file eof", "No more bytes aviable, end of file"), VType::except_value, no_copy);
	else if (stream.fail())
		return new ValueItem(new AException("system fault file fail", "Fail extract bytes"), VType::except_value, no_copy);

	else if (stream.bad())
		return new ValueItem(new AException("system fault file bad", "Stream maybe corrupted"), VType::except_value, no_copy);
	else
		return (ValueItem*)nullptr;
}
typed_lgr<FuncEnviropment> ConcurentWriterLong(new FuncEnviropment(concurentWriterLong, false));
typed_lgr<Task> ConcurentFile::read(typed_lgr<ConcurentFile>& file, uint32_t len, uint64_t pos) {
	ValueItem tmp[]{
				ValueItem(new typed_lgr<ConcurentFile>(file),VType::undefined_ptr, no_copy),
				ValueItem((void*)(0ull + len),VType::ui64, no_copy),
				ValueItem((void*)pos,VType::ui64, no_copy)
	};
	ValueItem args(tmp);
	typed_lgr<Task> aa(new Task(ConcurentReader, args));
	Task::start(aa);
	return aa;
}
typed_lgr<Task> ConcurentFile::write(typed_lgr<ConcurentFile>& file, char* arr, uint32_t len, uint64_t pos) {
	ValueItem tmp[]{
				ValueItem(new typed_lgr<ConcurentFile>(file),VType::undefined_ptr, no_copy),
				ValueItem(arr,ValueMeta(VType::raw_arr_ui8,false,true,len), no_copy),
				ValueItem((void*)pos,VType::ui64, no_copy),
				ValueItem((void*)false,VType::ui8, no_copy)
	};
	ValueItem args(tmp);
	typed_lgr<Task> aa(new Task(ConcurentWriter, args));
	Task::start(aa);
	return aa;
}
typed_lgr<Task> ConcurentFile::append(typed_lgr<ConcurentFile>& file, char* arr, uint32_t len) {
	ValueItem tmp[]{
				ValueItem(new typed_lgr<ConcurentFile>(file),VType::undefined_ptr, no_copy),
				ValueItem(arr, ValueMeta(VType::raw_arr_ui8,false,true,len), no_copy),
				ValueItem((void*)-1,VType::ui64, no_copy),
				ValueItem((void*)false,VType::ui8, no_copy)
	};
	ValueItem args(tmp);
	typed_lgr<Task> aa(new Task(ConcurentWriter, args));
	Task::start(aa);
	return aa;
}

typed_lgr<Task> ConcurentFile::read_long(typed_lgr<ConcurentFile>& file, uint64_t len, uint64_t pos) {
	ValueItem tmp[]{
				ValueItem(new typed_lgr<ConcurentFile>(file),VType::undefined_ptr, no_copy),
				ValueItem((void*)len ,VType::ui64, no_copy),
				ValueItem((void*)pos,VType::ui64, no_copy)
	};
	ValueItem args(tmp);
	typed_lgr<Task> aa(new Task(ConcurentReaderLong, args));
	Task::start(aa);
	return aa;
}
typed_lgr<Task> ConcurentFile::write_long(typed_lgr<ConcurentFile>& file, list_array<uint8_t>* arr, uint64_t pos) {
	ValueItem tmp[]{
				ValueItem(new typed_lgr<ConcurentFile>(file),VType::undefined_ptr, no_copy),
				ValueItem(arr,VType::undefined_ptr, no_copy),
				ValueItem((void*)pos,VType::ui64, no_copy),
				ValueItem((void*)false,VType::ui8, no_copy)
	};
	ValueItem args(tmp);
	typed_lgr<Task> aa(new Task(ConcurentWriterLong, args));
	Task::start(aa);
	return aa;
}
typed_lgr<Task> ConcurentFile::append_long(typed_lgr<ConcurentFile>& file, list_array<uint8_t>* arr) {
	ValueItem tmp[]{
				ValueItem(new typed_lgr<ConcurentFile>(file),VType::undefined_ptr, no_copy),
				ValueItem(arr, VType::undefined_ptr, no_copy),
				ValueItem((void*)-1,VType::ui64, no_copy),
				ValueItem((void*)true,VType::ui8, no_copy)
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
	ValueItem& args = vals[1];
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
	ValueItem vals{ ValueItem(this,VType::undefined_ptr), args };
	typed_lgr<Task> res = new Task(_async_notify, vals);
	Task::start(res);
	return res;
}
#pragma endregion

#pragma region TaskQuery
struct TaskQueryHandle{
	TaskQuery* tq;
	size_t at_execution_max;
	size_t now_at_execution = 0;
	TaskMutex no_race;
	bool destructed = false;
	TaskConditionVariable end_of_query;
};

TaskQuery::TaskQuery(size_t at_execution_max){	
	is_running = false;
	handle = new TaskQueryHandle{this, at_execution_max};
}

void __TaskQuery_add_task_leave(TaskQueryHandle* tqh, TaskQuery* tq){
	std::lock_guard lock(tqh->no_race);
	if(tqh->destructed){
		if(tqh->at_execution_max == 0)
			delete tqh;
	}
	else if(!tq->tasks.empty() && tq->is_running){
		tq->handle->now_at_execution--;
		while (tq->handle->now_at_execution <= tq->handle->at_execution_max) {
			tq->handle->now_at_execution++;
			auto awake_task = tq->tasks.front();
			tq->tasks.pop_front();
			Task::start(awake_task);
		}
	}
	else {
		tq->handle->now_at_execution--;
		
		if(tq->handle->now_at_execution == 0 && tq->tasks.empty())
			tq->handle->end_of_query.notify_all();
	}
}
ValueItem* __TaskQuery_add_task(ValueItem* args, uint32_t len){
	TaskQueryHandle* tqh = (TaskQueryHandle*)args[0].val;
	TaskQuery* tq = tqh->tq;
	typed_lgr<FuncEnviropment>& call_func = *(typed_lgr<FuncEnviropment>*)args[1].val;
	ValueItem& arguments = *(ValueItem*)args[2].val;

	ValueItem* res = nullptr;
	try{
		res = FuncEnviropment::sync_call(call_func,(ValueItem*)arguments.getSourcePtr(),arguments.meta.val_len);
	}
	catch(...){
		__TaskQuery_add_task_leave(tqh, tq);
		throw;
	}
	__TaskQuery_add_task_leave(tqh, tq);
	return res;
}
typed_lgr<FuncEnviropment> _TaskQuery_add_task(new FuncEnviropment(__TaskQuery_add_task,false));
typed_lgr<Task> TaskQuery::add_task(typed_lgr<class FuncEnviropment> call_func, ValueItem& arguments, bool used_task_local, typed_lgr<class FuncEnviropment> exception_handler, std::chrono::high_resolution_clock::time_point timeout) {
	ValueItem copy;
	if (arguments.meta.vtype == VType::faarr || arguments.meta.vtype == VType::saarr)
		copy = ValueItem((ValueItem*)arguments.getSourcePtr(), arguments.meta.val_len);
	else
		copy = std::initializer_list{ arguments };

	typed_lgr<Task> res = new Task(_TaskQuery_add_task, ValueItem{(void*)handle, new typed_lgr<class FuncEnviropment>(call_func), copy }, used_task_local, exception_handler, timeout);
	std::lock_guard lock(handle->no_race);
	if(is_running && handle->now_at_execution <= handle->at_execution_max){
		Task::start(res);
		handle->now_at_execution++;
	}
	else tasks.push_back(res);

	return res;
}
void TaskQuery::enable(){
	std::lock_guard lock(handle->no_race);
	is_running = true;
	while(handle->now_at_execution < handle->at_execution_max && !tasks.empty()){
		auto awake_task = tasks.front();
		tasks.pop_front();
		Task::start(awake_task);
		handle->now_at_execution++;
	}
}
void TaskQuery::disable(){
	std::lock_guard lock(handle->no_race);
	is_running = false;
}
bool TaskQuery::in_query(typed_lgr<Task> task){
	if(task->started)
		return false;//started task can't be in query
	std::lock_guard lock(handle->no_race);
	return std::find(tasks.begin(), tasks.end(), task) != tasks.end();
}
void TaskQuery::set_max_at_execution(size_t val){
	std::lock_guard lock(handle->no_race);
	handle->at_execution_max = val;
}
size_t TaskQuery::get_max_at_execution(){
	std::lock_guard lock(handle->no_race);
	return handle->at_execution_max;
}


void TaskQuery::wait(){
	MutexUnify unify(handle->no_race);
	std::unique_lock lock(unify);
	while(handle->now_at_execution != 0 && !tasks.empty())
		handle->end_of_query.wait(lock);
}
bool TaskQuery::wait_for(size_t milliseconds){
	return wait_until(std::chrono::high_resolution_clock::now() + std::chrono::milliseconds(milliseconds));
}
bool TaskQuery::wait_until(std::chrono::high_resolution_clock::time_point time_point){
	MutexUnify unify(handle->no_race);
	std::unique_lock lock(unify);
	while(handle->now_at_execution != 0 && !tasks.empty()){
		if(!handle->end_of_query.wait_until(lock,time_point))
			return false;
	}
	return true;
}
TaskQuery::~TaskQuery(){
	handle->destructed = true;
	wait();
	if(handle->now_at_execution == 0)
		delete handle;
}
#pragma endregion


#pragma region Generator

	void prepare_generator(ValueItem& args,typed_lgr<FuncEnviropment>& func, typed_lgr<FuncEnviropment>& ex_handler, Generator*& weak_ref){
		weak_ref = loc.on_load_generator_ref.getPtr();
		func = loc.on_load_generator_ref->func;
		ex_handler = loc.on_load_generator_ref->ex_handle;
		list_array<ValueItem> args_list;
		args_list.push_back(ValueItem(weak_ref));
		if(args.meta.vtype == VType::faarr || args.meta.vtype == VType::saarr){
			ValueItem* args_ptr = (ValueItem*)args.getSourcePtr();
			for(uint32_t i = 0; i < args.meta.val_len; i++)
				args_list.push_back(std::move(args_ptr[i]));
		}
		else
			args_list.push_back(std::move(args));

		size_t len = 0;
		ValueItem* extracted = args_list.take_raw(len);
		args = ValueItem(extracted, len, no_copy);
		weak_ref->args = nullptr;
		loc.on_load_generator_ref = nullptr;
	}
	ctx::continuation generator_execute(ctx::continuation&& sink){
		ValueItem args;
		typed_lgr<FuncEnviropment> func;
		typed_lgr<FuncEnviropment> ex_handler;
		Generator* weak_ref;
		prepare_generator(args, func, ex_handler, weak_ref);
		try{
			Generator::return_(weak_ref, func->syncWrapper((ValueItem*)args.getSourcePtr(), args.meta.val_len));
		}catch(...){
			std::exception_ptr except = std::current_exception();
			try{
				ValueItem ex = except;
				Generator::return_(weak_ref, ex_handler->syncWrapper(&ex, 1));
				return sink;
			}catch(...){}
			Generator::back_unwind(weak_ref, std::move(except));
		}
		return sink;
	}
	

	Generator::Generator(typed_lgr<class FuncEnviropment> call_func, const ValueItem& arguments, bool used_generator_local, typed_lgr<class FuncEnviropment> exception_handler){
		args = arguments;
		func = call_func;
		if(used_generator_local)
			_generator_local = new ValueEnvironment();
		else
			_generator_local = nullptr;
		
		ex_handle = exception_handler;
	}
	Generator::Generator(typed_lgr<class FuncEnviropment> call_func, ValueItem&& arguments, bool used_generator_local, typed_lgr<class FuncEnviropment> exception_handler){
		args = std::move(arguments);
		func = call_func;
		if(used_generator_local)
			_generator_local = new ValueEnvironment();
		else
			_generator_local = nullptr;
		
		ex_handle = exception_handler;
	}
	Generator::Generator(Generator&& mov) noexcept{
		func = mov.func;
		_generator_local = mov._generator_local;
		ex_handle = mov.ex_handle;
		context = mov.context;
		
		mov._generator_local = nullptr;
		mov.context = nullptr;
	}
	Generator::~Generator(){
		if(_generator_local)
			delete _generator_local;
		if(context)
			reinterpret_cast<ctx::continuation&>(context).~continuation();
	}

	bool Generator::yield_iterate(typed_lgr<Generator>& generator){
		if(generator->context == nullptr) {
			*reinterpret_cast<ctx::continuation*>(&generator->context) = ctx::callcc(std::allocator_arg, light_stack(1048576/*1 mb*/), generator_execute);
			if(generator->ex_ptr)
				std::rethrow_exception(generator->ex_ptr);
			return true;
		}
		else if(!generator->end_of_life) {
			*reinterpret_cast<ctx::continuation*>(&generator->context) = reinterpret_cast<ctx::continuation*>(&generator->context)->resume();
			if(generator->ex_ptr)
				std::rethrow_exception(generator->ex_ptr);
			return true;
		}
		else return false;
	}
	ValueItem* Generator::get_result(typed_lgr<Generator>& generator){
		if(!generator->results.empty())
			return generator->results.take_front();
		if(generator->end_of_life)
			return nullptr;
		if(generator->context == nullptr) {
			loc.on_load_generator_ref = generator;
			*reinterpret_cast<ctx::continuation*>(&generator->context) = ctx::callcc(std::allocator_arg, light_stack(1048576/*1 mb*/), generator_execute);
			if(generator->ex_ptr)
				std::rethrow_exception(generator->ex_ptr);
			if(!generator->results.empty())
				return generator->results.take_front();
			return nullptr;
		}
		else {
			*reinterpret_cast<ctx::continuation*>(&generator->context) = reinterpret_cast<ctx::continuation*>(&generator->context)->resume();
			if(generator->ex_ptr)
				std::rethrow_exception(generator->ex_ptr);
			if(!generator->results.empty())
				return generator->results.take_front();
			return nullptr;
		}
	}
	bool Generator::has_result(typed_lgr<Generator>& generator){
		return !generator->results.empty() || (generator->context != nullptr && !generator->end_of_life);
	}
	list_array<ValueItem*> Generator::await_results(typed_lgr<Generator>& generator){
		list_array<ValueItem*> results;
		while(!generator->end_of_life)
			results.push_back(get_result(generator));
		return results;
	}
	list_array<ValueItem*> Generator::await_results(list_array<typed_lgr<Generator>>& generators){
		list_array<ValueItem*> results;
		for(auto& generator : generators){
			auto result = await_results(generator);
			results.push_back(result);
		}
		return results;
	}
	

	class ValueEnvironment* Generator::generator_local(Generator* generator_weak_ref){
		return generator_weak_ref->_generator_local;
	}
	void Generator::yield(Generator* generator_weak_ref, ValueItem* result){
		generator_weak_ref->results.push_back(result);
		*reinterpret_cast<ctx::continuation*>(&generator_weak_ref->context) = reinterpret_cast<ctx::continuation*>(&generator_weak_ref->context)->resume();
	}
	void Generator::result(Generator* generator_weak_ref, ValueItem* result){
		generator_weak_ref->results.push_back(result);
	}

	void Generator::back_unwind(Generator* generator_weak_ref, std::exception_ptr&& except){
		generator_weak_ref->ex_ptr = std::move(except);
		generator_weak_ref->end_of_life = true;
	}
	void Generator::return_(Generator* generator_weak_ref, ValueItem* result){
		generator_weak_ref->results.push_back(result);
		generator_weak_ref->end_of_life = true;
	}
#pragma endregion



namespace _Task_unsafe{
	void ctxSwap(){
		swapCtx();
	}
	void ctxSwapRelock(const MutexUnify& lock0){
		swapCtxRelock(lock0);
	}
	void ctxSwapRelock(const MutexUnify& lock0, const MutexUnify& lock1){
		swapCtxRelock(lock0, lock1);
	}
	void ctxSwapRelock(const MutexUnify& lock0, const MutexUnify& lock1, const MutexUnify& lock2){
		swapCtxRelock(lock0, lock1, lock2);
	}
}