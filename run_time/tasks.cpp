#include <list>
//#include <boost/context/protected_fixedsize_stack.hpp>

#include <boost/context/continuation.hpp>
#include <boost/fiber/all.hpp>
#include <boost/fiber/detail/thread_barrier.hpp>

#include "AttachA_CXX.hpp"
#include "tasks.hpp"
#include <iostream>

namespace ctx = boost::context;
struct TaskResult {
	ctx::continuation tmp_current_context;
	std::list<typed_lgr<Task>> end_of_life_await;//for tasks
	std::condition_variable result_notify;//for non task env
	ValueItem* fres = nullptr;
};
Task::Task(typed_lgr<class FuncEnviropment> call_func, list_array<ValueItem>* arguments, typed_lgr<class FuncEnviropment> exception_handler) {
	ex_handle = exception_handler;
	func = call_func;
	args = arguments;
	fres = new TaskResult();
}
Task::~Task() {
	if (fres)
		delete fres;
	if (args)
		delete args;
}

using timing = std::pair<std::chrono::high_resolution_clock::time_point, typed_lgr<Task>>;
struct {
	TaskConditionVariable no_tasks_notifier;
	TaskConditionVariable no_tasks_execute_notifier;

	std::list<typed_lgr<Task>> tasks;
	std::list<timing> timed_tasks;

	std::recursive_mutex task_thread_safety;
	std::recursive_mutex task_timer_safety;

	std::condition_variable tasks_notifier;
	std::condition_variable time_notifier;

	size_t executors = 0;
	size_t in_exec = 0;
	bool time_control_enabled = false;

	bool in_time_swap = false;
} glob;

struct {
	ctx::continuation* tmp_current_context = nullptr;
	std::exception_ptr ex_ptr;
	typed_lgr<Task> curr_task = nullptr;
	bool is_task_thread = false;
	bool context_in_swap = false;
} thread_local loc;





void inline swapCtx() {
	loc.context_in_swap = true;
	*loc.tmp_current_context = std::move(*loc.tmp_current_context).resume();
	loc.context_in_swap = false;
}

ctx::continuation context_exec(ctx::continuation&& sink) {
	*loc.tmp_current_context = std::move(sink);
	try {
		loc.curr_task->fres->fres = loc.curr_task->func->syncWrapper(loc.curr_task->args);
		loc.context_in_swap = false;
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
		loc.curr_task->fres->fres = loc.curr_task->ex_handle->syncWrapper(loc.curr_task->args);
		loc.context_in_swap = false;
	}
	catch (const ctx::detail::forced_unwind&) {
		throw;
	}
	catch (...) {
		loc.ex_ptr = std::current_exception();
	}
	return std::move(sink);
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
		{
			{
				std::lock_guard guard(glob.task_thread_safety);
				--glob.in_exec;
				if (!glob.in_exec && glob.tasks.empty() && glob.timed_tasks.empty())
					glob.no_tasks_execute_notifier.notify_all();
			}
			while (glob.tasks.empty()) {
				if (end_in_task_out) {
					--glob.executors;
					return;
				}
				glob.tasks_notifier.wait(waiter_lock);
			}
			std::lock_guard guard(glob.task_thread_safety);
			++glob.in_exec;
			if (glob.tasks.empty())
				continue;

			loc.curr_task = glob.tasks.front();
			glob.tasks.pop_front();
			if (loc.curr_task->end_of_life)
				continue;
			loc.tmp_current_context = &loc.curr_task->fres->tmp_current_context;
			if (glob.tasks.empty())
				glob.no_tasks_notifier.notify_all();
			//if func is nullptr then this task signal to shutdown executor
			if (!loc.curr_task->func) {
				--glob.executors;
				--glob.in_exec;
				break;
			}
		}


		if (*loc.tmp_current_context)
			*loc.tmp_current_context = std::move(*loc.tmp_current_context).resume();
		else 
			*loc.tmp_current_context = ctx::callcc(context_exec);

		if (loc.ex_ptr) {
			if (loc.curr_task->ex_handle) {
				if (loc.curr_task->args)
					delete loc.curr_task->args;
				loc.curr_task->args = new list_array<ValueItem>{ValueItem(new std::exception_ptr(loc.ex_ptr), ValueMeta(VType::except_value, false, false), false)};
				loc.ex_ptr = nullptr;
				*loc.tmp_current_context = ctx::callcc(context_ex_handle);

				if (!loc.ex_ptr) {
					if (loc.context_in_swap)
						continue;
					loc.curr_task->fres->result_notify.notify_all();
					goto task_end;
				}
			}
			loc.curr_task->fres->fres = new ValueItem(new std::exception_ptr(loc.ex_ptr), ValueMeta(VType::except_value, false, false));
			loc.ex_ptr = nullptr;
			loc.curr_task->fres->result_notify.notify_all();
		}
		else {
			if (loc.context_in_swap)
				continue;
			loc.curr_task->fres->result_notify.notify_all();
		}
	task_end:
		{
			std::lock_guard guard(glob.task_thread_safety);
			std::list<typed_lgr<Task>> swp;
			swp.swap(loc.curr_task->fres->end_of_life_await);
			glob.tasks.insert(glob.tasks.end(), swp.begin(), swp.end());
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
			if (glob.timed_tasks.size()) {
				std::lock_guard guard(glob.task_thread_safety);
				while (glob.timed_tasks.front().first <= std::chrono::high_resolution_clock::now()) {
					std::lock_guard task_guard(glob.timed_tasks.front().second->no_race);
					if (glob.timed_tasks.front().second->awaked) {
						glob.timed_tasks.pop_front();
					}
					else {
						glob.timed_tasks.front().second->time_end_flag = true;
						glob.tasks.push_back(std::move(glob.timed_tasks.front().second));
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
void makeTimeWait(timing&& t) {
	if (!glob.time_control_enabled)
		startTimeController();
	loc.curr_task->awaked = false;
	loc.curr_task->time_end_flag = false;
	std::lock_guard guard(glob.task_thread_safety);
	size_t i = 0;
	auto it = glob.timed_tasks.begin();
	auto end = glob.timed_tasks.end();
	while (it != end) {
		if (it->first > t.first) {
			glob.timed_tasks.insert(it, std::move(t));
			i = -1;
			break;
		}
		++it;
	}
	if (i != -1)
		glob.timed_tasks.push_back(std::move(t));
	glob.time_notifier.notify_one();
}
void TaskMutex::lock() {
	if (loc.is_task_thread) {
		loc.curr_task->awaked = false;
		loc.curr_task->time_end_flag = false;

		no_race.lock();
		while (current_task) {
			resume_task.push_back(loc.curr_task);
			no_race.unlock();
			swapCtx();
			no_race.lock();
		}
		current_task = loc.curr_task.getPtr();
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
			makeTimeWait(timing(time_point, loc.curr_task));
			resume_task.push_back(loc.curr_task);
			no_race.unlock();
			swapCtx();
			if (!loc.curr_task->awaked)
				return false;
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
		std::lock_guard lg1(glob.task_thread_safety);
		while (resume_task.size()) {
			auto& it = resume_task.front();
			std::lock_guard lg2(it->no_race);
			if (!it->time_end_flag) {
				it->awaked = true;
				glob.tasks.push_back(it);
				resume_task.pop_front();
				glob.tasks_notifier.notify_one();
				return;
			}
			else
				resume_task.pop_front();
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
	glob.tasks.push_back(lgr_task);
	glob.tasks_notifier.notify_one();
	lgr_task->started = true;
}
ValueItem* Task::getResult(typed_lgr<Task>& lgr_task) {
	if (loc.is_task_thread) {
		if (!lgr_task->started || lgr_task->is_yield_mode) {
			lgr_task->fres->end_of_life_await.push_back(loc.curr_task);
			start(lgr_task);
			swapCtx();
		}
		return lgr_task->fres->fres;
	}
	else {
		std::mutex mtx;
		std::unique_lock ulm(mtx);
		if (!lgr_task->started || lgr_task->is_yield_mode) {
			lgr_task->fres->fres = nullptr;
			start(lgr_task);
			while (!lgr_task->fres->fres)
				lgr_task->fres->result_notify.wait(ulm);
		}
		return lgr_task->fres->fres;
	}
}
ValueItem* Task::getCurrentResult(typed_lgr<Task>& lgr_task) {
	if (loc.is_task_thread) {
		if (!lgr_task->started) {
			lgr_task->fres->end_of_life_await.push_back(loc.curr_task);
			start(lgr_task);
			swapCtx();
		}
		return lgr_task->fres->fres;
	}
	else {
		std::mutex mtx;
		std::unique_lock ulm(mtx);
		if (!lgr_task->started) {
			lgr_task->fres->fres = nullptr;
			start(lgr_task);
			while (!lgr_task->fres->fres)
				lgr_task->fres->result_notify.wait(ulm);
		}
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
			glob.tasks.push_back(resume_task.front());
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
			glob.tasks.push_back(resume_task.front());
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
			glob.tasks.push_back(resume_task.front());
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
		throw InvalidOperation("Tried release fully released semaphore");
	std::lock_guard lg1(glob.task_thread_safety);
	allow_treeshold = max_treeshold;
	native_notify.notify_all();
	while (resume_task.size()) {
		auto& it = resume_task.back();
		std::lock_guard lg2(it->no_race);
		if (!it->time_end_flag) {
			it->awaked = true;
			glob.tasks.push_back(resume_task.front());
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






boost::fibers::condition_variable_any sleep_task{};
std::condition_variable awake_task{};

std::atomic_size_t inited_count = 0;
std::condition_variable ini_end;

std::atomic_size_t tasks_count = 0;
boost::fibers::condition_variable_any task_end{};

void aathread() {
	boost::fibers::use_scheduling_algorithm<boost::fibers::algo::shared_work>();
	++inited_count;
	ini_end.notify_one();
	std::mutex mtx_count{};
	std::unique_lock lk(mtx_count);
	size_t no_tasks_count = 0;

	boost::fibers::scheduler* cur_sched = boost::fibers::context::active()->get_scheduler();
	while (true) {
		awake_task.wait(lk, []() { return (bool)tasks_count; });
		sleep_task.wait(lk, []() { return !tasks_count; });
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
	while(tasks_count)
		task_end.wait(lk);
}

void BTask::threadEnviroConfig() {
	boost::fibers::use_scheduling_algorithm<boost::fibers::algo::shared_work>();
}