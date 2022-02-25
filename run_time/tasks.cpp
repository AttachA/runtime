#include <list>
#include "link_garbage_remover.hpp"
#include <boost/context/protected_fixedsize_stack.hpp>
#include <boost/context/continuation.hpp>
#include "run_time_compiler.hpp"
#include "tasks.hpp"

namespace ctx = boost::context;
Task::Task(class FuncEnviropment* call_func, list_array<ArrItem>* arguments, bool is_detached, class FuncEnviropment* exception_handler) {
	ex_handle = exception_handler;
	func = call_func;
	args = arguments;
	detached = is_detached;
}
struct TaskResult {
	ctx::continuation tmp_current_context;
	list_array<typed_lgr<Task>> end_of_life_await;//for tasks
	std::condition_variable result_notify;//for non task env
	FuncRes* fres = nullptr;
};


struct {
	list_array<typed_lgr<Task>> tasks;
	std::recursive_mutex task_thread_safety;
	std::condition_variable tasks_notifier;
	TaskConditionVariable no_tasks_notifier;
	TaskConditionVariable no_tasks_execute_notifier;

	size_t executors = 0;
	size_t in_exec = 0;
} glob;

struct {
	ctx::continuation tmp_current_context;
	bool is_task_thread = false;
	bool context_in_swap = false;
	typed_lgr<Task> curr_task = nullptr;
}thread_local loc;



void swapCtx() {
	loc.context_in_swap = true;
	loc.tmp_current_context = std::move(loc.tmp_current_context).resume();
	loc.context_in_swap = false;
}

void TaskMutex::lock() {
	if (loc.is_task_thread) {
	re_try:
		if (!no_race.try_lock()) {
			resume_task.push_back(loc.curr_task);
			loc.context_in_swap = true;
			swapCtx();
			goto re_try;
		}
		if (current_task) {
			resume_task.push_back(loc.curr_task);
			no_race.unlock();
			loc.context_in_swap = true;
			swapCtx();
			goto re_try;
		}
		else
			current_task = loc.curr_task;
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
void TaskMutex::unlock() {
	if (loc.is_task_thread) {
		std::lock_guard lg0(no_race);
		if (current_task != loc.curr_task)
			throw InvalidOperation("Tried unlock non owned mutex");
		std::lock_guard lg1(glob.task_thread_safety);
		current_task = nullptr;
		if(resume_task.size())
			glob.tasks.push_back(resume_task.take(0));
		return;
	}
	else 
		no_race.unlock();
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
			if (!glob.in_exec && glob.tasks.empty())
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
			loc.curr_task = glob.tasks.take_front();
			if (glob.tasks.empty())
				glob.no_tasks_notifier.notify_all();
		}
		if (!loc.curr_task->func && loc.curr_task->detached) {
			std::lock_guard guard(glob.task_thread_safety);
			--glob.executors;
			--glob.in_exec;
			break;
		}
		try {
			if (loc.curr_task->fres->tmp_current_context) {
				loc.tmp_current_context = std::move(loc.curr_task->fres->tmp_current_context);
				loc.tmp_current_context.resume();
			}
			else 
				ctx::callcc(std::allocator_arg, ctx::protected_fixedsize_stack(fault_reserved_stack_size * 3),
					[&](ctx::continuation&& sink) {
						loc.tmp_current_context = std::move(sink);
						loc.curr_task->fres->fres = loc.curr_task->func->FuncWraper(loc.curr_task->args, false);
						loc.context_in_swap = false;
						return std::move(loc.tmp_current_context);
					}
				);
			if (loc.context_in_swap) {
				loc.curr_task->fres->tmp_current_context = std::move(loc.tmp_current_context);
				continue;
			}
			loc.curr_task->fres->result_notify.notify_all();
		}
		catch (...) {
			if (loc.curr_task->ex_handle) {
				list_array<ArrItem> arg;
				arg.push_back(ArrItem(new std::exception_ptr(std::current_exception()), ValueMeta(VType::except_value, false, false), false));
				try {
					loc.tmp_current_context = ctx::continuation(ctx::callcc([&](ctx::continuation&& sink) {
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
		loc.context_in_swap = true;
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
		start(typed_lgr<Task>(new Task(nullptr, nullptr, true)));
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
	while (glob.tasks.size() || glob.in_exec)
		glob.no_tasks_execute_notifier.wait();
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
void TaskConditionVariable::notify_all() {
	std::lock_guard guard(glob.task_thread_safety);
	glob.tasks.push_back(resume_task.take());
	if (glob.tasks.size())
		glob.tasks_notifier.notify_all();
	cd.notify_all();
}
void TaskConditionVariable::notify_one() {
	if (resume_task.size()) {
		std::lock_guard guard(glob.task_thread_safety);
		glob.tasks.push_back(resume_task.take((size_t)0));
		if (glob.tasks.size())
			glob.tasks_notifier.notify_one();
	}
	else
		cd.notify_one();
}
