#include <list>
#include <boost/context/protected_fixedsize_stack.hpp>
#include <boost/context/continuation.hpp>
#include "run_time_compiler.hpp"
#include "tasks.hpp"

namespace ctx = boost::context;
Task::Task(class FuncEnviropment* call_func, list_array<ArrItem>* arguments, class FuncEnviropment* exception_handler) {
	ex_handle = exception_handler;
	func = call_func;
	args = arguments;
}
struct TaskResult {
	ctx::continuation tmp_current_context;
	list_array<typed_lgr<Task>> end_of_life_await;//for tasks
	std::condition_variable result_notify;//for non task env
	FuncRes* fres = nullptr;
};


using timing = std::pair<std::chrono::high_resolution_clock::time_point, typed_lgr<Task>>;
struct {
	TaskConditionVariable no_tasks_notifier;
	TaskConditionVariable no_tasks_execute_notifier;

	list_array<typed_lgr<Task>> tasks;
	list_array<timing> timed_tasks;

	std::recursive_mutex task_thread_safety;

	std::condition_variable tasks_notifier;
	std::condition_variable time_notifier;

	size_t executors = 0;
	size_t in_exec = 0;
	bool time_control_enabled = false;
} glob;

struct {
	ctx::continuation* tmp_current_context = nullptr;
	typed_lgr<Task> curr_task = nullptr;
	bool is_task_thread = false;
	bool context_in_swap = false;
} thread_local loc;





void inline swapCtx() {
	loc.context_in_swap = true;
	*loc.tmp_current_context = std::move(*loc.tmp_current_context).resume();
	loc.context_in_swap = false;
}
void taskExecutor(bool end_in_task_out = false) {
	loc.is_task_thread = true;
	std::mutex waiter;
	std::unique_lock waiter_lock(waiter);
	{
		std::lock_guard guard(glob.task_thread_safety);
		++glob.in_exec;
	}
	while (true) {
		{
			std::lock_guard guard(glob.task_thread_safety);
			--glob.in_exec;
			if (!glob.in_exec && glob.tasks.empty() && glob.timed_tasks.empty())
				glob.no_tasks_execute_notifier.notify_all();
		}
		while (glob.tasks.empty()) {
			if (end_in_task_out)
				return;
			glob.tasks_notifier.wait(waiter_lock);
		}
		{
			std::lock_guard guard(glob.task_thread_safety);
			if (glob.tasks.empty())
				continue;
			++glob.in_exec;
			loc.curr_task = glob.tasks.take_back();
			loc.tmp_current_context = &loc.curr_task->fres->tmp_current_context;
			if (glob.tasks.empty())
				glob.no_tasks_notifier.notify_all();
		}
		if (!loc.curr_task->func) {
			std::lock_guard guard(glob.task_thread_safety);
			--glob.executors;
			--glob.in_exec;
			break;
		}
		try {
			ctx::continuation cont;
			if (loc.curr_task->fres->tmp_current_context)
				*loc.tmp_current_context = std::move(loc.curr_task->fres->tmp_current_context).resume();
			else
				*loc.tmp_current_context = ctx::callcc(std::allocator_arg, ctx::protected_fixedsize_stack(fault_reserved_stack_size * 3),
					[&](ctx::continuation&& sink) {
						*loc.tmp_current_context = std::move(sink);
						loc.curr_task->fres->fres = loc.curr_task->func->FuncWraper(loc.curr_task->args, false);
						loc.context_in_swap = false;
						return std::move(*loc.tmp_current_context);
					}
			);
			if (loc.context_in_swap)
				continue;
			loc.curr_task->fres->result_notify.notify_all();
		}
		catch (...) {
			if (loc.curr_task->ex_handle) {
				list_array<ArrItem> arg;
				arg.push_back(ArrItem(new std::exception_ptr(std::current_exception()), ValueMeta(VType::except_value, false, false), false));
				try {
					*loc.tmp_current_context = ctx::continuation(ctx::callcc([&](ctx::continuation&& sink) {
						loc.curr_task->fres->fres = loc.curr_task->ex_handle->FuncWraper(&arg, false);
						loc.context_in_swap = false;
						return std::move(sink);
						}));
					swapCtx();
					if (loc.context_in_swap)
						continue;
					loc.curr_task->fres->result_notify.notify_all();
				}
				catch (...) {
					loc.curr_task->fres->fres = new FuncRes(ValueMeta(VType::except_value, false, false), new std::exception_ptr(std::current_exception()));
					loc.curr_task->fres->result_notify.notify_all();
				}
				goto task_end;
			}
			loc.curr_task->fres->fres = new FuncRes(ValueMeta(VType::except_value, false, false), new std::exception_ptr(std::current_exception()));
			loc.curr_task->fres->result_notify.notify_all();
		}
	task_end:
		{
			std::lock_guard guard(glob.task_thread_safety);
			glob.tasks.push_back(loc.curr_task->fres->end_of_life_await.take());
			if (glob.tasks.size())
				glob.tasks_notifier.notify_all();
		}
	}
}
void taskTimer() {
	std::mutex mtx;
	std::unique_lock ulm(mtx);
	while (true) {
		{
			std::lock_guard guard(glob.task_thread_safety);
			if (glob.timed_tasks.size()) {
				while (glob.timed_tasks.back().first <= std::chrono::high_resolution_clock::now()) {
					std::lock_guard task_guard(glob.timed_tasks.back().second->no_race);
					if (glob.timed_tasks.back().second->awaked) {
						glob.timed_tasks.take_back();
					}
					else {
						glob.timed_tasks.back().second->time_end_flag = true;
						glob.tasks.push_back(glob.timed_tasks.take_back().second);
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
			glob.time_notifier.wait_until(ulm, glob.timed_tasks.back().first);
	}
}
void startTimeController() {
	if (glob.time_control_enabled)
		return;
	std::lock_guard guard(glob.task_thread_safety);
	if (glob.time_control_enabled)
		return;
	std::thread(taskTimer).detach();
	glob.time_control_enabled = true;
}
void makeTimeWait(timing&& t) {
	startTimeController();
	loc.curr_task->awaked = false;
	loc.curr_task->time_end_flag = false;
	std::lock_guard guard(glob.task_thread_safety);
	glob.timed_tasks.ordered_insert(std::move(t), [](timing& a, timing& b) { return a.first > b.first; });
	glob.time_notifier.notify_one();
}


void TaskMutex::lock() {
	if (loc.is_task_thread) {
		loc.curr_task->awaked = false;
		loc.curr_task->time_end_flag = false;
	re_try:
		no_race.lock();
		if (current_task) {
			resume_task.push_back(loc.curr_task);
			no_race.unlock();
			swapCtx();
			goto re_try;
		}
		else
			current_task = loc.curr_task;
		resume_task.shrink_to_fit();
		no_race.unlock();
		return;
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
			current_task = loc.curr_task;
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
			makeTimeWait(timing(time_point, loc.curr_task));
			resume_task.push_back(loc.curr_task);
			no_race.unlock();
			swapCtx();
			if (!loc.curr_task->awaked)
				return false;
			goto re_try;
		}
		if (!current_task)
			current_task = loc.curr_task;
		no_race.unlock();
		return true;
	}
	else
		return no_race.try_lock_until(time_point);
}
void TaskMutex::unlock() {
	if (loc.is_task_thread) {
		std::lock_guard lg0(no_race);
		if (current_task != loc.curr_task)
			throw InvalidOperation("Tried unlock non owned mutex");
		std::lock_guard lg1(glob.task_thread_safety);
		current_task = nullptr;
		while (resume_task.size()) {
			auto& it = resume_task.back();
			std::lock_guard lg2(it->no_race);
			if (!it->time_end_flag) {
				it->awaked = true;
				glob.tasks.push_back(resume_task.take_back());
				glob.tasks_notifier.notify_one();
				return;
			}
			else
				resume_task.take_back();
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
	if (!lgr_task->fres)
		lgr_task->fres = new TaskResult();
	lgr_task->fres->fres = nullptr;
	glob.tasks.push_back(lgr_task);
	glob.tasks_notifier.notify_one();
	lgr_task->started = true;
}
FuncRes* Task::getResult(typed_lgr<Task>& lgr_task) {
	if (loc.is_task_thread) {
		lgr_task->fres->end_of_life_await.push_back(loc.curr_task);
		start(lgr_task);
		swapCtx();
		return lgr_task->fres->fres;
	}
	else {
		std::mutex mtx;
		std::unique_lock ulm(mtx);
		start(lgr_task);
		while (!lgr_task->fres->fres)
			lgr_task->fres->result_notify.wait(ulm);
		return lgr_task->fres->fres;
	}
}
FuncRes* Task::getAwaitResult(typed_lgr<Task>& lgr_task) {
	if (loc.is_task_thread) {
		lgr_task->fres->end_of_life_await.push_back(loc.curr_task);
		if(!lgr_task->started)
			start(lgr_task);
		swapCtx();
		return lgr_task->fres->fres;
	}
	else {
		std::mutex mtx;
		std::unique_lock ulm(mtx);
		if (!lgr_task->started)
			start(lgr_task);
		while (!lgr_task->fres->fres)
			lgr_task->fres->result_notify.wait(ulm);
		return lgr_task->fres->fres;
	}
}
FuncRes* Task::getYieldResult(typed_lgr<Task>& lgr_task) {
	if (loc.is_task_thread) {
		lgr_task->fres->end_of_life_await.push_back(loc.curr_task);
		if (!lgr_task->started || lgr_task->is_yield_mode)
			start(lgr_task);
		swapCtx();
		return lgr_task->fres->fres;
	}
	else {
		std::mutex mtx;
		std::unique_lock ulm(mtx);
		if (!lgr_task->started || lgr_task->is_yield_mode)
			start(lgr_task);
		while (!lgr_task->fres->fres)
			lgr_task->fres->result_notify.wait(ulm);
		return lgr_task->fres->fres;
	}
}


void Task::createExecutor(size_t count) {
	for (size_t i = 0; i < count; i++) {
		std::thread(taskExecutor, false).detach();
		std::lock_guard guard(glob.task_thread_safety);
		++glob.executors;
	}
}
size_t Task::totalExecutors() {
	std::lock_guard guard(glob.task_thread_safety);
	return glob.executors;
}
void Task::reduceExceutor(size_t count) {
	for (size_t i = 0; i < count; i++)
		start(typed_lgr<Task>(new Task(nullptr, nullptr)));
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
		while (glob.tasks.size())
			glob.no_tasks_notifier.wait();
}
void Task::awaitEndTasks(){
	while (glob.tasks.size() || glob.timed_tasks.size() || glob.in_exec)
		glob.no_tasks_execute_notifier.wait();
}
void Task::sleep(size_t milliseconds) {
	sleep_until(std::chrono::high_resolution_clock::now() + std::chrono::milliseconds(milliseconds));
}
void Task::sleep_until(std::chrono::high_resolution_clock::time_point time_point) {
	if (loc.is_task_thread) {
		makeTimeWait(timing(time_point, loc.curr_task));
		swapCtx();
	}
	else 
		std::this_thread::sleep_until(time_point);
}

void TaskConditionVariable::wait() {
	if (loc.is_task_thread) {
		resume_task.push_back(loc.curr_task);
		swapCtx();
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
		makeTimeWait(timing(time_point, loc.curr_task));
		resume_task.push_back(loc.curr_task);
		swapCtx();
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
			glob.tasks.push_back(resume_task.take_back());
		}
		else
			resume_task.take_back();
	}
	cd.notify_all();
}
void TaskConditionVariable::notify_one() {
	if (resume_task.size()) {
		std::lock_guard guard(glob.task_thread_safety);
		std::lock_guard guard_loc(resume_task.back()->no_race);
		if (!resume_task.back()->time_end_flag) {
			resume_task.back()->awaked = true;
			glob.tasks.push_back(resume_task.take_back());
		}
		else
			resume_task.take_back();
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
		if (loc.is_task_thread) {
			resume_task.push_back(loc.curr_task);
			no_race.unlock();
			swapCtx();
		}
		else {
			no_race.unlock();
			std::mutex mtx;
			std::unique_lock ulm(mtx);
			native_notify.wait(ulm);
		}
		goto re_try;
	}
	else
		--allow_treeshold;
	resume_task.shrink_to_fit();
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
			makeTimeWait(timing(time_point, loc.curr_task));
			resume_task.push_back(loc.curr_task);
			no_race.unlock();
			swapCtx();
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
		throw InvalidOperation("Tried release fully released semaphore");
	std::lock_guard lg1(glob.task_thread_safety);
	allow_treeshold++;
	native_notify.notify_one();
	while (resume_task.size()) {
		auto& it = resume_task.back();
		std::lock_guard lg2(it->no_race);
		if (!it->time_end_flag) {
			it->awaked = true;
			glob.tasks.push_back(resume_task.take_back());
			glob.tasks_notifier.notify_one();
			return;
		}
		else
			resume_task.take_back();
	}
}
void TaskSemaphore::release_all() {
	std::lock_guard lg0(no_race);
	if (allow_treeshold == max_treeshold)
		throw InvalidOperation("Tried release fully released semaphore");
	std::lock_guard lg1(glob.task_thread_safety);
	allow_treeshold = max_treeshold;
	native_notify.notify_all();
	while (resume_task.size()) {
		auto& it = resume_task.back();
		std::lock_guard lg2(it->no_race);
		if (!it->time_end_flag) {
			it->awaked = true;
			glob.tasks.push_back(resume_task.take_back());
			glob.tasks_notifier.notify_one();
		}
		else
			resume_task.take_back();
	}
}
bool TaskSemaphore::is_locked() {
	if (try_lock()) {
		release();
		return true;
	}
	return false;
}
