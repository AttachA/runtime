// Copyright Danyil Melnytskyi 2022-2023
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <list>
#include <boost/context/continuation.hpp>

#include <run_time/AttachA_CXX.hpp>
#include <run_time/tasks.hpp>
#include <util/exceptions.hpp>
#include <util/string_help.hpp>
#include <run_time/ValueEnvironment.hpp>
#include <queue>
#include <deque>
#include <run_time/tasks_util/light_stack.hpp>
#include <run_time/tasks_util/hill_climbing.hpp>
#include <run_time/library/parallel.hpp>
#include <run_time/tasks_util/native_workers_singleton.hpp>
#include <configuration/tasks.hpp>
#include <run_time/asm/exception.hpp>
namespace art{





#undef min
	namespace ctx = boost::context;
	constexpr size_t native_thread_flag = size_t(1) << (sizeof(size_t) * 8 - 1);

	void put_arguments(ValueItem& args_hold, const ValueItem& arguments){
		if (arguments.meta.vtype == VType::faarr || arguments.meta.vtype == VType::saarr)
			args_hold = std::move(arguments);
		else if (arguments.meta.vtype != VType::noting)
			args_hold = {std::move(arguments)};
	}
	void put_arguments(ValueItem& args_hold, ValueItem&& arguments){
		if (arguments.meta.vtype == VType::faarr)
			args_hold = std::move(arguments);
		else if(arguments.meta.vtype == VType::saarr){
			ValueItem *arr = new ValueItem[arguments.meta.val_len];
			ValueItem *arr2 = (ValueItem*)arguments.val;
			for (size_t i = 0; i < arguments.meta.val_len; ++i)
				arr[i] = std::move(arr2[i]);
			args_hold = ValueItem(arr, arguments.meta.val_len, no_copy);
		}
		else if (arguments.meta.vtype != VType::noting){
			ValueItem *arr = new ValueItem[1];
			arr[0] = std::move(arguments);
			args_hold = std::move(ValueItem(arr, 1, no_copy));
		}
	}


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
	MutexUnify::MutexUnify(art::mutex& smut) {
		type = MutexUnifyType::nmut;
		nmut = std::addressof(smut);
	}
	MutexUnify::MutexUnify(art::timed_mutex& smut) {
		type = MutexUnifyType::ntimed;
		ntimed = std::addressof(smut);
	}
	MutexUnify::MutexUnify(art::recursive_mutex& smut) {
		type = MutexUnifyType::nrec;
		nrec = std::addressof(smut);
	}
	MutexUnify::MutexUnify(TaskMutex& smut) {
		type = MutexUnifyType::umut;
		umut = std::addressof(smut);
	}
	MutexUnify::MutexUnify(MultiplyMutex& mmut): mmut(&mmut){
		type = MutexUnifyType::mmut;
	}
	MutexUnify::MutexUnify(nullptr_t) {
		type = MutexUnifyType::noting;
	}

	MutexUnify& MutexUnify::operator=(const MutexUnify& mut) {
		type = mut.type;
		nmut = mut.nmut;
		return *this;
	}
	MutexUnify& MutexUnify::operator=(art::mutex& smut) {
		type = MutexUnifyType::nmut;
		nmut = std::addressof(smut);
		return *this;
	}
	MutexUnify& MutexUnify::operator=(art::timed_mutex& smut) {
		type = MutexUnifyType::ntimed;
		ntimed = std::addressof(smut);
		return *this;
	}
	MutexUnify& MutexUnify::operator=(art::recursive_mutex& smut) {
		type = MutexUnifyType::nrec;
		nrec = std::addressof(smut);
		return *this;
	}
	MutexUnify& MutexUnify::operator=(TaskMutex& smut) {
		type = MutexUnifyType::umut;
		umut = std::addressof(smut);
		return *this;
	}
	MutexUnify& MutexUnify::operator=(MultiplyMutex& smut) {
		type = MutexUnifyType::mmut;
		mmut = std::addressof(smut);
		return *this;
	}
	MutexUnify& MutexUnify::operator=(nullptr_t) {
		type = MutexUnifyType::noting;
		return *this;
	}

	void MutexUnify::relock_start(){
		if(type == MutexUnifyType::nrec)
			state = nrec->relock_begin();
		unlock();
	}
	void MutexUnify::relock_end(){
		lock();
		if(type == MutexUnifyType::nrec)
			nrec->relock_end(state);
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


	size_t Task::max_running_tasks = configuration::tasks::max_running_tasks;
	size_t Task::max_planned_tasks = configuration::tasks::max_planned_tasks;
	bool Task::enable_task_naming = configuration::tasks::enable_task_naming;

	TaskCancellation::TaskCancellation() : AttachARuntimeException("This task received cancellation token") {}
	TaskCancellation::~TaskCancellation() {
		if (!in_landing) {
			abort();
		}
	}
	bool TaskCancellation::_in_landing() {
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


	ValueItem* TaskResult::getResult(size_t res_num, art::unique_lock<MutexUnify>& l) {
		if (results.size() >= res_num) {
			while (results.size() >= res_num && !end_of_life)
				result_notify.wait(l);
			if (results.size() >= res_num)
				return new ValueItem();
		}
		return new ValueItem(results[res_num]);
	}
	void TaskResult::awaitEnd(art::unique_lock<MutexUnify>& l) {
		while (!end_of_life)
			result_notify.wait(l);
	}
	void TaskResult::yieldResult(ValueItem* res, art::unique_lock<MutexUnify>& l, bool release) {
		if (res) {
			results.push_back(std::move(*res));
			if (release)
				delete res;
		}
		else
			results.push_back(ValueItem());
		result_notify.notify_all();
	}
	void TaskResult::yieldResult(ValueItem&& res, art::unique_lock<MutexUnify>& l) {
		results.push_back(std::move(res));
		result_notify.notify_all();
	}
	void TaskResult::finalResult(ValueItem* res, art::unique_lock<MutexUnify>& l) {
		if (res) {
			results.push_back(std::move(*res));
			delete res;
		}
		else
			results.push_back(ValueItem());
		end_of_life = true;
		result_notify.notify_all();
	}
	void TaskResult::finalResult(ValueItem&& res, art::unique_lock<MutexUnify>& l) {
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
		art::shared_ptr<Task> awake_task; 
		uint16_t check_id;
	};
	struct binded_context{
		art::util::hill_climb executor_manager_hill_climb;
		std::list<uint32_t> completions;
		std::list<art::shared_ptr<Task>> tasks;
		TaskConditionVariable on_closed_notifier;
		art::recursive_mutex no_race;
		art::condition_variable_any new_task_notifier;
		uint16_t executors = 0;
		bool in_close = false;
		bool allow_implicit_start = false;
		bool fixed_size = false;
	};
	struct {
		TaskConditionVariable no_tasks_notifier;
		TaskConditionVariable no_tasks_execute_notifier;

		std::queue<art::shared_ptr<Task>> tasks;
		std::queue<art::shared_ptr<Task>> cold_tasks;
		std::deque<timing> timed_tasks;

		art::recursive_mutex task_thread_safety;
		art::mutex task_timer_safety;

		art::condition_variable_any tasks_notifier;
		art::condition_variable_any time_notifier;
		art::condition_variable_any executor_shutdown_notifier;

		size_t executors = 0;
		size_t in_exec = 0;
		bool time_control_enabled = false;

		bool in_time_swap = false;
		std::atomic_size_t tasks_in_swap = 0;
		std::atomic_size_t in_run_tasks= 0;
		std::atomic_size_t planned_tasks = 0;

		TaskConditionVariable can_started_new_notifier;
		TaskConditionVariable can_planned_new_notifier;
		
		art::mutex binded_workers_safety;
		std::unordered_map<uint16_t, binded_context,art::hash<uint16_t>> binded_workers;

		bool executor_manager_in_work = false;
		art::condition_variable_any executor_manager_task_taken;
		art::util::hill_climb executor_manager_hill_climb;
		std::list<uint32_t> workers_completions;

		
	#if _configuration_tasks_enable_debug_mode
		art::rw_mutex debug_safety;
		std::list<Task*> alive_tasks;
		std::list<Task*> on_workers_tasks;
	#endif
	} glob;
	struct executors_local {
		art::shared_ptr<Generator> on_load_generator_ref = nullptr;

		ctx::continuation* tmp_current_context = nullptr;
		std::exception_ptr ex_ptr;
		list_array<art::shared_ptr<Task>> tasks_buffer;
		art::shared_ptr<Task> curr_task = nullptr;
		bool is_task_thread = false;
		bool context_in_swap = false;

		bool in_exec_decreased = false;
	};
	thread_local executors_local loc;
	struct TaskCallback {
		static void dummy(ValueItem&){}
		ValueItem args;
		void(*on_start)(ValueItem&);
		void(*on_await)(ValueItem&);
		void(*on_cancel)(ValueItem&);
		void(*on_timeout)(ValueItem&);
		void(*on_destruct)(ValueItem&);
		TaskCallback(ValueItem& args, void(*on_await)(ValueItem&) = dummy, void(*on_cancel)(ValueItem&) = dummy, void(*on_timeout)(ValueItem&) = dummy, void(*on_start)(ValueItem&) = dummy, void(*on_destruct)(ValueItem&) = dummy) : args(args), on_start(on_start), on_await(on_await), on_cancel(on_cancel), on_timeout(on_timeout), on_destruct(on_destruct){}
		~TaskCallback() {
			if(on_destruct)
				on_destruct(args);
		}
		static void start(Task& task){
			auto self = (TaskCallback*)task.args.val;
			if(task.timeout != std::chrono::high_resolution_clock::time_point::min()){
				if(task.timeout <= std::chrono::high_resolution_clock::now())
					self->on_timeout(self->args);
					return;
			}
			if(self->on_start)
				self->on_start(self->args);
		}
		static bool await(Task& task){
			auto self = (TaskCallback*)task.args.val;
			if(self->on_await){
				self->on_await(self->args);
				return true;
			}
			else return false;
		}
		static void cancel(Task& task){
			auto self = (TaskCallback*)task.args.val;
			if(self->on_cancel)
				self->on_cancel(self->args);
		}
		static void timeout(Task& task){
			auto self = (TaskCallback*)task.args.val;
			if(self->on_timeout)
				self->on_timeout(self->args);
		}
	};
	void checkCancellation() {
		if (loc.curr_task->make_cancel)
			throw TaskCancellation();
		if (loc.curr_task->timeout != std::chrono::high_resolution_clock::time_point::min())
			if (loc.curr_task->timeout <= std::chrono::high_resolution_clock::now())
				throw TaskCancellation();
	}
#pragma optimize("",off)
#pragma region TaskExecutor
	void swapCtx() {
		if(loc.is_task_thread){
			loc.context_in_swap = true;
			++glob.tasks_in_swap;
			loc.curr_task->relock_0.relock_start();
			loc.curr_task->relock_1.relock_start();
			loc.curr_task->relock_2.relock_start();
			if(exception::has_exception()){
				CXXExInfo cxx = exception::take_current_exception();
				try{
					*loc.tmp_current_context = std::move(*loc.tmp_current_context).resume();
				}catch(const ctx::detail::forced_unwind&){
					exception::load_current_exception(cxx);
					throw;
				}
				exception::load_current_exception(cxx);
			}else{
				*loc.tmp_current_context = std::move(*loc.tmp_current_context).resume();
			}
			loc.context_in_swap = true;
			loc.curr_task->relock_0.relock_end();
			loc.curr_task->relock_1.relock_end();
			loc.curr_task->relock_2.relock_end();
			loc.curr_task->awake_check++;
			--glob.tasks_in_swap;
			loc.context_in_swap = false;
			checkCancellation();
		}else
			throw InternalException("swapCtx() not allowed call in non-task thread or in dispatcher");
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
			//TODO: put to warm task asynchroniously, i.e. when task reach end of life state, push new task to warm
			size_t placed = glob.in_run_tasks;
			size_t max_tasks = std::min(Task::max_running_tasks - placed, glob.cold_tasks.size());
			for (size_t i = 0; i < max_tasks; ++i) {
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
			checkCancellation();
			ValueItem* res = loc.curr_task->func->syncWrapper((ValueItem*)loc.curr_task->args.val, loc.curr_task->args.meta.val_len);
			MutexUnify mu(loc.curr_task->no_race);
			art::unique_lock l(mu);
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
		try{
			loc.curr_task->args = nullptr;
		}catch(...){};
		art::lock_guard l(loc.curr_task->no_race);
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
			checkCancellation();
			ValueItem* res = loc.curr_task->ex_handle->syncWrapper((ValueItem*)loc.curr_task->args.val, loc.curr_task->args.meta.val_len);
			MutexUnify mu(loc.curr_task->no_race);
			art::unique_lock l(mu);
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
		try{
			loc.curr_task->args = nullptr;
		}catch(...){};
		
		MutexUnify uni(loc.curr_task->no_race);
		art::unique_lock l(uni);
		loc.curr_task->end_of_life = true;
		loc.curr_task->fres.result_notify.notify_all();
		--glob.in_run_tasks;
		if (Task::max_running_tasks)
			glob.can_started_new_notifier.notify_one();
		return std::move(*loc.tmp_current_context);
	}


	void transfer_task(art::shared_ptr<Task>& task){
		if(task->bind_to_worker_id ==  (uint16_t)-1){
			art::lock_guard guard(glob.task_thread_safety);
			glob.tasks.push(std::move(task));
			glob.tasks_notifier.notify_one();
		}else{
			art::unique_lock initializer_guard(glob.binded_workers_safety);
			if(!glob.binded_workers.contains(task->bind_to_worker_id)){
				initializer_guard.unlock();
				invite_to_debugger("Binded worker context " + std::to_string(task->bind_to_worker_id) + " not found");
				std::abort();
			}
			binded_context& extern_context = glob.binded_workers[task->bind_to_worker_id];
			initializer_guard.unlock();
			if(extern_context.in_close){
				invite_to_debugger("Binded worker context " + std::to_string(task->bind_to_worker_id) + " is closed");
				std::abort();
			}
			art::lock_guard guard(extern_context.no_race);
			extern_context.tasks.push_back(std::move(task));
			extern_context.new_task_notifier.notify_one();
		}
	}
	void awake_task(art::shared_ptr<Task>& task){
		if(task->bind_to_worker_id ==  (uint16_t)-1){
			if(task->auto_bind_worker){
				art::unique_lock guard(glob.binded_workers_safety);
				for(auto& [id,context] : glob.binded_workers){
					if(context.allow_implicit_start){
						if(context.in_close)
							continue;
						guard.unlock();
						art::unique_lock context_guard(context.no_race);
						task->bind_to_worker_id = id;
						context.tasks.push_back(std::move(task));
						context.new_task_notifier.notify_one();
						return;
					}
				}
				throw InternalException("No binded workers available");
			}
		}
		transfer_task(task);
	}

	void taskNotifyIfEmpty(art::unique_lock<art::recursive_mutex>& re_lock) {
		if(!loc.in_exec_decreased)
			--glob.in_exec;
		loc.in_exec_decreased = false;
		if (!glob.in_exec && glob.tasks.empty() && glob.timed_tasks.empty()) 
			glob.no_tasks_execute_notifier.notify_all();
	}
	bool loadTask() {
		{
			++glob.in_exec;
			size_t len = glob.tasks.size();
			if (!len)
				return true;
			auto tmp = std::move(glob.tasks.front());
			glob.tasks.pop();
			if (len == 1)
				glob.no_tasks_notifier.notify_all();
			loc.curr_task = std::move(tmp);
			if(Task::max_running_tasks){
				if (Task::max_running_tasks > (glob.in_run_tasks + glob.tasks_in_swap)) {
					if (!glob.cold_tasks.empty()) {
						glob.tasks.push(std::move(glob.cold_tasks.front()));
						glob.cold_tasks.pop();
					}
				}
			}else{
				while(!glob.cold_tasks.empty()){
					glob.tasks.push(std::move(glob.cold_tasks.front()));
					glob.cold_tasks.pop();
				}
			}
		}
		loc.tmp_current_context = &reinterpret_cast<ctx::continuation&>(loc.curr_task->fres.context);
		return false;
	}
#define worker_mode_desk(old_name, mode) if(Task::enable_task_naming)worker_mode_desk_(old_name, mode);
	void worker_mode_desk_(const art::ustring& old_name, const art::ustring& mode) {
		if(old_name.empty())
			_set_name_thread_dbg("Worker " + std::to_string(_thread_id()) + ": " + mode);
		else
			_set_name_thread_dbg(old_name + " | (Temporal worker) " + std::to_string(_thread_id()) + ": " + mode);
	}
	void pseudo_task_handle(const art::ustring& old_name, bool &caught_ex){
		loc.is_task_thread = false;
		caught_ex = false;
		try{
			if(loc.curr_task->_task_local == (ValueEnvironment*)-1 ){
				worker_mode_desk(old_name, " executing callback")
				TaskCallback::start(*loc.curr_task);
			}
			else {
				//worker_mode_desk(old_name, " executing function - " + loc.curr_task->func->to_string())
				ValueItem* res = FuncEnvironment::sync_call(loc.curr_task->func, (ValueItem*)loc.curr_task->args.getSourcePtr(), loc.curr_task->args.meta.val_len);
				MutexUnify mu(loc.curr_task->no_race);
				art::unique_lock l(mu);
				loc.curr_task->fres.finalResult(res, l);
			}
			
		}catch(...){
			loc.is_task_thread = true;
			loc.ex_ptr = std::current_exception();
			caught_ex = true;
			
			if(!need_restore_stack_fault())
				return;
		}
		if(caught_ex)
			restore_stack_fault();
		worker_mode_desk(old_name, "idle");
		loc.is_task_thread = true;
	}

	bool execute_task(const art::ustring& old_name){
		bool pseudo_handle_caught_ex = false;
		if (!loc.curr_task->func)
			return true;
		if(loc.curr_task->_task_local == (ValueEnvironment*)-1 || loc.curr_task->func->isCheap()){
			pseudo_task_handle(old_name, pseudo_handle_caught_ex);
			if (pseudo_handle_caught_ex)
				goto caught_ex;
			goto end_task;
		}
		if (loc.curr_task->end_of_life)
			goto end_task;

		worker_mode_desk(old_name, "process task - " + std::to_string(loc.curr_task->task_id()));
		if (*loc.tmp_current_context) {
			*loc.tmp_current_context = std::move(*loc.tmp_current_context).resume();
		}
		else {
			++glob.in_run_tasks;
			--glob.planned_tasks;
			if (Task::max_planned_tasks)
				glob.can_planned_new_notifier.notify_one();
			*loc.tmp_current_context = ctx::callcc(std::allocator_arg, light_stack(1048576/*1 mb*/), context_exec);
		}
	caught_ex:
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
			art::unique_lock l(uni);
			loc.curr_task->fres.finalResult(ValueItem(new std::exception_ptr(loc.ex_ptr), ValueMeta(VType::except_value), no_copy),l);
			loc.ex_ptr = nullptr;
		}
	end_task:
		loc.is_task_thread = false;
		loc.curr_task = nullptr;
		worker_mode_desk(old_name, "idle");
		return false;
	}
	void taskExecutor(bool end_in_task_out = false) {
		art::ustring old_name = end_in_task_out ? _get_name_thread_dbg(_thread_id()) : "";

		if(old_name.empty())
			_set_name_thread_dbg("Worker " + std::to_string(_thread_id()));
		else
			_set_name_thread_dbg(old_name + " | (Temporal worker) " + std::to_string(_thread_id()));

		art::unique_lock guard(glob.task_thread_safety);
		glob.workers_completions.push_front(0);
		auto to_remove_after_death = glob.workers_completions.begin();
		uint32_t& completions = glob.workers_completions.front();
		++glob.in_exec;
		++glob.executors;
		while (true) {
			loc.context_in_swap = false;
			loc.tmp_current_context = nullptr;
			taskNotifyIfEmpty(guard);
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

				if (end_in_task_out) 
					goto end_worker;
				glob.tasks_notifier.wait(guard);
			}
			loc.is_task_thread = true;
			if (loadTask())
				continue;
			glob.executor_manager_task_taken.notify_one();
			guard.unlock();
			if(loc.curr_task->bind_to_worker_id != (uint16_t)-1){
				transfer_task(loc.curr_task);
				guard.lock();
				continue;
			}
#if _configuration_tasks_enable_debug_mode
			{
				art::shared_lock dbg_guard(glob.debug_safety);
				glob.on_workers_tasks.push_back(&*loc.curr_task);
			}
#endif
			//if func is nullptr then this task signal to shutdown executor
			bool shut_down_signal = execute_task(old_name);
#if _configuration_tasks_enable_debug_mode
			{
				art::shared_lock dbg_guard(glob.debug_safety);
				Task* ptr = &*loc.curr_task;
				auto it = std::find(glob.on_workers_tasks.rbegin(), glob.on_workers_tasks.rend(), ptr);
				if(it != glob.on_workers_tasks.rend())
					glob.on_workers_tasks.erase(std::next(it).base());
			}
#endif

			guard.lock();
			if (shut_down_signal)
				break;
			completions +=1;
		}
	end_worker:
		--glob.executors;
		glob.workers_completions.erase(to_remove_after_death);
		taskNotifyIfEmpty(guard);
		glob.executor_shutdown_notifier.notify_all();
	}

	void bindedTaskExecutor(uint16_t id){
		art::ustring old_name = "Binded";
		art::unique_lock initializer_guard(glob.binded_workers_safety);
		if(!glob.binded_workers.contains(id)){
			invite_to_debugger("Binded worker context " + std::to_string(id) + " not found");
			std::abort();
		}
		binded_context& context = glob.binded_workers[id];
		context.completions.push_front(0);
		auto to_remove_after_death = context.completions.begin();
		uint32_t& completions = context.completions.front();
		initializer_guard.unlock();

		std::list<art::shared_ptr<Task>>& queue = context.tasks;
		art::recursive_mutex& safety = context.no_race;
		art::condition_variable_any& notifier = context.new_task_notifier;
		bool pseudo_handle_caught_ex = false;
		_set_name_thread_dbg("Binded worker " + std::to_string(_thread_id()) + ": " + std::to_string(id));

		art::unique_lock guard(safety);
		context.executors++;
		while(true){
			while(queue.empty())
				notifier.wait(guard);
			loc.curr_task = std::move(queue.front());
			queue.pop_front();
			guard.unlock();
			if (!loc.curr_task->func)
				break;
			if(loc.curr_task->bind_to_worker_id !=  (uint16_t)id){
				transfer_task(loc.curr_task);
				continue;
			}
			loc.is_task_thread = true;
			loc.tmp_current_context = &reinterpret_cast<ctx::continuation&>(loc.curr_task->fres.context);
#if _configuration_tasks_enable_debug_mode
			{
				art::shared_lock dbg_guard(glob.debug_safety);
				glob.on_workers_tasks.push_back(&*loc.curr_task);
			}
#endif
			if(execute_task(old_name))
				break;
#if _configuration_tasks_enable_debug_mode
			{
				art::shared_lock dbg_guard(glob.debug_safety);
				Task* ptr = &*loc.curr_task;
				auto it = std::find(glob.on_workers_tasks.rbegin(), glob.on_workers_tasks.rend(), ptr);
				if(it != glob.on_workers_tasks.rend())
					glob.on_workers_tasks.erase(std::next(it).base());
			}
#endif
			completions +=1;
			guard.lock();
		}
		guard.lock();
		--context.executors;
		if(context.executors == 0){
			if(context.in_close){
				context.on_closed_notifier.notify_all();
				guard.unlock();
			}else{
				invite_to_debugger("Caught executor/s death when context is not closed");
				std::abort();
			}
		}
	}

#pragma endregion
#pragma optimize("",on)
	void taskTimer() {
		glob.time_control_enabled = true;
		_set_name_thread_dbg("Task time controller");

		art::mutex mtx;
		mtx.lock();
		list_array<art::shared_ptr<Task>> cached_wake_ups;
		while (glob.time_control_enabled) {
			{
				art::lock_guard guard(glob.task_timer_safety);
				if (glob.timed_tasks.size()) {
					while (glob.timed_tasks.front().wait_timepoint <= std::chrono::high_resolution_clock::now()) {
						timing& tmng = glob.timed_tasks.front();
						if (tmng.check_id != tmng.awake_task->awake_check) {
							glob.timed_tasks.pop_front();
							if (glob.timed_tasks.empty())
								break;
							else
								continue;
						}
						art::lock_guard task_guard(tmng.awake_task->no_race);
						if (tmng.awake_task->awaked) {
							glob.timed_tasks.pop_front();
						}
						else {
							tmng.awake_task->time_end_flag = true;
							cached_wake_ups.push_back(std::move(tmng.awake_task));
							glob.timed_tasks.pop_front();
						}
						if (glob.timed_tasks.empty())
							break;
					}
				}
			}
			if (!cached_wake_ups.empty()) {
				art::lock_guard guard(glob.task_thread_safety);
				while (!cached_wake_ups.empty()) 
					glob.tasks.push(cached_wake_ups.take_back());
				glob.tasks_notifier.notify_all();
			}
			

			if (glob.timed_tasks.empty())
				glob.time_notifier.wait(mtx);
			else
				glob.time_notifier.wait_until(mtx, glob.timed_tasks.front().wait_timepoint);
		}
	}


	void startTimeController() {
		art::lock_guard guard(glob.task_timer_safety);
		if (glob.time_control_enabled)
			return;
		art::thread(taskTimer).detach();
		glob.time_control_enabled = true;
	}
	void makeTimeWait(std::chrono::high_resolution_clock::time_point t) {
		if (!glob.time_control_enabled)
			startTimeController();
		loc.curr_task->awaked = false;
		loc.curr_task->time_end_flag = false;
		size_t i = 0;
		{
			art::lock_guard guard(glob.task_timer_safety);
			auto it = glob.timed_tasks.begin();
			auto end = glob.timed_tasks.end();
			while (it != end) {
				if (it->wait_timepoint >= t) {
					glob.timed_tasks.insert(it, timing(t,loc.curr_task, loc.curr_task->awake_check ));
					i = -1;
					break;
				}
				++it;
			}
			if (i != -1)
				glob.timed_tasks.push_back(timing(t, loc.curr_task, loc.curr_task->awake_check));
		}
		glob.time_notifier.notify_one();
	}

#pragma region Task
	Task::Task(art::shared_ptr<FuncEnvironment> call_func, const ValueItem& arguments, bool used_task_local, art::shared_ptr<FuncEnvironment> exception_handler, std::chrono::high_resolution_clock::time_point task_timeout) {
		ex_handle = exception_handler;
		func = call_func;
		put_arguments(args, arguments);
		
		timeout = task_timeout;
		if (used_task_local)
			_task_local = new ValueEnvironment();

		if (Task::max_planned_tasks) {
			MutexUnify uni(glob.task_thread_safety);
			art::unique_lock l(uni);
			while (glob.planned_tasks >= Task::max_planned_tasks)
				glob.can_planned_new_notifier.wait(l);
		}
		++glob.planned_tasks;
	}
	Task::Task(art::shared_ptr<FuncEnvironment> call_func, ValueItem&& arguments, bool used_task_local, art::shared_ptr<FuncEnvironment> exception_handler, std::chrono::high_resolution_clock::time_point task_timeout) {
		ex_handle = exception_handler;
		func = call_func;
		put_arguments(args, std::move(arguments));

		timeout = task_timeout;
		if (used_task_local)
			_task_local = new ValueEnvironment();

		if (Task::max_planned_tasks) {
			MutexUnify uni(glob.task_thread_safety);
			art::unique_lock l(uni);
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
		if (_task_local && _task_local != (ValueEnvironment*)-1)
			delete _task_local;
		if (_task_local == (ValueEnvironment*)-1)
			delete (TaskCallback*)args.getSourcePtr();
		
		if (!started) {
			--glob.planned_tasks;
			if (Task::max_running_tasks)
				glob.can_planned_new_notifier.notify_one();
		}
	}

	void Task::auto_bind_worker_enable(bool enable){
		auto_bind_worker = enable;
		if (enable)
			bind_to_worker_id = -1;
	}
	void Task::set_worker_id(uint16_t id){
		bind_to_worker_id = id;
		auto_bind_worker = false;
	}

	void Task::start(list_array<art::shared_ptr<Task>>& lgr_task) {
		for (auto& it : lgr_task)
			start(it);
	}


	void Task::start(art::shared_ptr<Task>&& lgr_task) {
		start(lgr_task);
	}

	bool Task::yield_iterate(art::shared_ptr<Task>& lgr_task) {
		bool res = (!lgr_task->started || lgr_task->is_yield_mode) && lgr_task->_task_local != (ValueEnvironment*)-1;
		if (res)
			Task::start(lgr_task);
		return res;
	}
	ValueItem* Task::get_result(art::shared_ptr<Task>& lgr_task, size_t yield_res) {
		if (!lgr_task->started && lgr_task->_task_local != (ValueEnvironment*)-1)
			Task::start(lgr_task);
		MutexUnify uni(lgr_task->no_race);
		art::unique_lock l(uni);
		if(lgr_task->_task_local == (ValueEnvironment*)-1){
			l.unlock();
			if(!TaskCallback::await(*lgr_task)){
				l.lock();
				lgr_task->fres.awaitEnd(l);
			}
			if(lgr_task->fres.results.size() <= yield_res)
				return new ValueItem();
			return new ValueItem(lgr_task->fres.results[yield_res]);
		}
		else return lgr_task->fres.getResult(yield_res,l);
	}
	ValueItem* Task::get_result(art::shared_ptr<Task>&& lgr_task, size_t yield_res) {
		return get_result(lgr_task, yield_res);
	}
	bool Task::has_result(art::shared_ptr<Task>& lgr_task, size_t yield_res){
		return lgr_task->fres.results.size() > yield_res;
	}
	void Task::await_task(art::shared_ptr<Task>& lgr_task, bool make_start) {
		if (!lgr_task->started && make_start && lgr_task->_task_local != (ValueEnvironment*)-1)
			Task::start(lgr_task);
		MutexUnify uni(lgr_task->no_race);
		art::unique_lock l(uni);
		if(lgr_task->_task_local != (ValueEnvironment*)-1)
			lgr_task->fres.awaitEnd(l);
		else{
			l.unlock();
			if(!TaskCallback::await(*lgr_task)){
				l.lock();
				lgr_task->fres.awaitEnd(l);
			}
		}
	}
	list_array<ValueItem> Task::await_results(art::shared_ptr<Task>& task) {
		await_task(task);
		return task->fres.results;
	}
	list_array<ValueItem> Task::await_results(list_array<art::shared_ptr<Task>>& tasks) {
		list_array<ValueItem> res;
		for (auto& it : tasks)
			res.push_back(await_results(it));
		return res;
	}

	void Task::start(const art::shared_ptr<Task>& tsk) {
		art::shared_ptr<Task> lgr_task = tsk;
		if (lgr_task->started && !lgr_task->is_yield_mode)
			return;
		{
			art::lock_guard guard(glob.task_thread_safety);
			if (lgr_task->started && !lgr_task->is_yield_mode)
				return;
			if (Task::max_running_tasks > (glob.in_run_tasks + glob.tasks.size()) || !Task::max_running_tasks)
				glob.tasks.push(lgr_task);
			else
				glob.cold_tasks.push(lgr_task);
			lgr_task->started = true;
			glob.tasks_notifier.notify_one();
		}
	}

	uint16_t Task::create_bind_only_executor(uint16_t fixed_count, bool allow_implicit_start) {
		art::lock_guard guard(glob.binded_workers_safety);
		uint16_t try_count = 0;
		uint16_t id = glob.binded_workers.size();
	is_not_id:
		while(glob.binded_workers.contains(id)){
			if(try_count == UINT16_MAX)
				throw InternalException("Too many binded workers");
			try_count++;
			id++;
		}
		if(id == -1)
			goto is_not_id;
		glob.binded_workers[id].allow_implicit_start = allow_implicit_start;
		glob.binded_workers[id].fixed_size = (bool)fixed_count;
		for (size_t i = 0; i < fixed_count; i++)
			art::thread(bindedTaskExecutor, id).detach();
		return id;
	}
	void Task::close_bind_only_executor(uint16_t id) {
		MutexUnify unify(glob.binded_workers_safety);
		art::unique_lock guard(unify);
		std::list<art::shared_ptr<Task>> transfer_tasks;
		if(!glob.binded_workers.contains(id)){
			throw InternalException("Binded worker not found");
		}else{
			auto& context = glob.binded_workers[id];
			art::unique_lock context_lock(context.no_race);
			if(context.in_close)
				return;
			context.in_close = true;
			for(uint16_t i = 0; i < context.executors; i++){
				context.tasks.emplace_back(new Task(nullptr, ValueItem()));
			}
			context.new_task_notifier.notify_all();
			{
				MultiplyMutex mmut{unify, context.no_race};
				MutexUnify mmut_unify(mmut);
				art::unique_lock re_lock(mmut_unify, art::adopt_lock);
				while(context.executors != 0)
					context.on_closed_notifier.wait(re_lock);
				re_lock.release();
			}
			std::swap(transfer_tasks, context.tasks);
			context_lock.unlock();
			glob.binded_workers.erase(id);
		}
		for(art::shared_ptr<Task>& task : transfer_tasks){
			task->bind_to_worker_id = -1;
			transfer_task(task);
		}
	}
	void Task::create_executor(size_t count) {
		for (size_t i = 0; i < count; i++)
			art::thread(taskExecutor, false).detach();
	}
	size_t Task::total_executors() {
		art::lock_guard guard(glob.task_thread_safety);
		return glob.executors;
	}
	void Task::reduce_executor(size_t count) {
		ValueItem noting;
		for (size_t i = 0; i < count; i++){
			start(new Task(nullptr, noting));
		}
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
			art::unique_lock l(uni);
			while (glob.tasks.size() || glob.cold_tasks.size() || glob.timed_tasks.size())
				glob.no_tasks_notifier.wait(l);
		}
	}
	void Task::await_end_tasks(bool be_executor) {
		if (be_executor && !loc.is_task_thread) {
			art::unique_lock l(glob.task_thread_safety);
		binded_workers:
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
			std::lock_guard lock(glob.binded_workers_safety);
			bool binded_tasks_empty = true;
			for(auto& contexts : glob.binded_workers)
				if(contexts.second.tasks.size())
					binded_tasks_empty = false;
			if(!binded_tasks_empty)
				goto binded_workers;
		}
		else {
		binded_workers_:
			{
				MutexUnify uni(glob.task_thread_safety);
				art::unique_lock l(uni);
			
				if(loc.is_task_thread)
					while ((glob.tasks.size() || glob.cold_tasks.size() || glob.timed_tasks.size()) && glob.in_exec != 1 && glob.tasks_in_swap != 1)
						glob.no_tasks_execute_notifier.wait(l);
				else
					while (glob.tasks.size() || glob.cold_tasks.size() || glob.timed_tasks.size() || glob.in_exec  || glob.tasks_in_swap)
						glob.no_tasks_execute_notifier.wait(l);
			}
			{
				std::lock_guard lock(glob.binded_workers_safety);
				bool binded_tasks_empty = true;
				for(auto& contexts : glob.binded_workers)
					if(contexts.second.tasks.size())
						binded_tasks_empty = false;
				if(binded_tasks_empty)
					return;
			}
			goto binded_workers_;
		}
	}
	void Task::await_multiple(list_array<art::shared_ptr<Task>>& tasks, bool pre_started, bool release) {
		if (!pre_started) {
			for (auto& it : tasks){
				if(it->_task_local != (ValueEnvironment*)-1)
					Task::start(it);
			}
		}
		if (release) {
			for (auto& it : tasks) {
				await_task(it, false);
				it = nullptr;
			}
		}
		else 
			for (auto& it : tasks) 
				await_task(it, false);
	}
	void Task::await_multiple(art::shared_ptr<Task>* tasks, size_t len, bool pre_started, bool release) {
		if (!pre_started) {
			art::shared_ptr<Task>* iter = tasks;
			size_t count = len;
			while (count--){
				if ((*iter)->_task_local != (ValueEnvironment*)-1)
					Task::start(*iter++);
			}
		}
		if (release) {
			while (len--) {
				await_task(*tasks, false);
				(*tasks++) = nullptr;
			}
		}
		else 
			while (len--) 
				await_task(*tasks, false);
	}
	void Task::sleep(size_t milliseconds) {
		sleep_until(std::chrono::high_resolution_clock::now() + std::chrono::milliseconds(milliseconds));
	}



	void Task::result(ValueItem* f_res) {
		if (loc.is_task_thread) {
			MutexUnify uni(loc.curr_task->no_race);
			art::unique_lock l(uni);
			loc.curr_task->fres.yieldResult(f_res, l);
		}
		else
			throw EnvironmentRuinException("Thread attempt return yield result in non task enviro");
	}

	void Task::check_cancellation() {
		if (loc.is_task_thread)
			checkCancellation();
		else
			throw EnvironmentRuinException("Thread attempted check cancellation in non task enviro");
	}
	void Task::self_cancel() {
		if (loc.is_task_thread)
			throw TaskCancellation();
		else
			throw EnvironmentRuinException("Thread attempted cancel self, like task");
	}
	void Task::notify_cancel(art::shared_ptr<Task>& lgr_task){
		if(lgr_task->_task_local == (ValueEnvironment*)-1)
			TaskCallback::cancel(*lgr_task);
		else
			lgr_task->make_cancel = true;
	}

	void Task::notify_cancel(list_array<art::shared_ptr<Task>>& tasks){
		for (auto& it : tasks) 
			notify_cancel(it);
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
			return art::hash<size_t>()(reinterpret_cast<size_t>(&*loc.curr_task));
	}

	bool Task::is_task(){
		return loc.is_task_thread;
	}


	void Task::clean_up() {
		Task::await_no_tasks();
		std::queue<art::shared_ptr<Task>> e0;
		std::queue<art::shared_ptr<Task>> e1;
		glob.tasks.swap(e0);
		glob.cold_tasks.swap(e1);
		glob.timed_tasks.shrink_to_fit();
	}


	ValueItem* _empty_func(ValueItem* /*ignored*/, uint32_t /*ignored*/) {
		return nullptr;
	}
	art::shared_ptr<FuncEnvironment> empty_func(new FuncEnvironment(_empty_func, false, true));
	art::shared_ptr<Task> Task::dummy_task(){
		return new Task(empty_func, ValueItem());
	}
	//first arg bool& check
	//second arg art::condition_variable_any
	ValueItem* _notify_native_thread(ValueItem* args, uint32_t /*ignored*/) {
		*((bool*)args[0].val) = true;
		{
			art::lock_guard l(glob.task_thread_safety);
			glob.in_exec--;
		}
		loc.in_exec_decreased = true;
		((art::condition_variable_any*)args[1].val)->notify_one();
		return nullptr;
	}
	art::shared_ptr<FuncEnvironment> notify_native_thread(new FuncEnvironment(_notify_native_thread, false, true));
	art::shared_ptr<Task> Task::cxx_native_bridge(bool& checker, art::condition_variable_any& cd){
		return new Task(notify_native_thread, ValueItem{ ValueItem(&checker, VType::undefined_ptr), ValueItem(std::addressof(cd), VType::undefined_ptr) });
	}


	art::shared_ptr<Task> Task::fullifed_task(const list_array<ValueItem>& results){
		auto task = new Task(empty_func, nullptr);
		task->func = nullptr;
		task->fres.results = results;
		task->end_of_life = true;
		task->started = true;
		task->fres.end_of_life = true;
		return task;
	}
	art::shared_ptr<Task> Task::fullifed_task(list_array<ValueItem>&& results){
		auto task = new Task(empty_func, nullptr);
		task->func = nullptr;
		task->fres.results = std::move(results);
		task->end_of_life = true;
		task->started = true;
		task->fres.end_of_life = true;
		return task;
	}
	art::shared_ptr<Task> Task::fullifed_task(const ValueItem& result){
		auto task = new Task(empty_func, nullptr);
		task->func = nullptr;
		task->fres.results = { result };
		task->end_of_life = true;
		task->started = true;
		task->fres.end_of_life = true;
		return task;
	}
	art::shared_ptr<Task> Task::fullifed_task(ValueItem&& result){
		auto task = new Task(empty_func, nullptr);
		task->func = nullptr;
		task->fres.results = { std::move(result) };
		task->end_of_life = true;
		task->started = true;
		task->fres.end_of_life = true;
		return task;
	}


	class native_task_schedule : public NativeWorkerHandle, public NativeWorkerManager {
		art::shared_ptr<FuncEnvironment> func;
		ValueItem args;
		bool started = false;
	public:
		art::shared_ptr<Task> task;
		
		native_task_schedule(art::shared_ptr<FuncEnvironment> func) : NativeWorkerHandle(this){
			this->func = func;
			task = Task::dummy_task();
			task->started = true;
		}
		native_task_schedule(art::shared_ptr<FuncEnvironment> func, ValueItem&& arguments) : NativeWorkerHandle(this){
			this->func = func;
			task = Task::dummy_task();
			task->started = true;
			put_arguments(args, std::move(arguments));
		}
		native_task_schedule(art::shared_ptr<FuncEnvironment> func, const ValueItem& arguments) : NativeWorkerHandle(this){
			this->func = func;
			task = Task::dummy_task();
			task->started = true;
			put_arguments(args, arguments);
		}
		native_task_schedule(art::shared_ptr<FuncEnvironment> func, const ValueItem& arguments, ValueItem& dummy_data, void(*on_await)(ValueItem&), void(*on_cancel)(ValueItem&), void(*on_timeout)(ValueItem&), void(*on_destruct)(ValueItem&), std::chrono::high_resolution_clock::time_point timeout): NativeWorkerHandle(this){
			this->func = func;
			task = Task::callback_dummy(dummy_data, on_await, on_cancel, on_timeout, on_destruct, timeout);
			task->started = true;
			put_arguments(args, arguments);
		}
		native_task_schedule(art::shared_ptr<FuncEnvironment> func, ValueItem&& arguments, ValueItem& dummy_data, void(*on_await)(ValueItem&), void(*on_cancel)(ValueItem&), void(*on_timeout)(ValueItem&), void(*on_destruct)(ValueItem&), std::chrono::high_resolution_clock::time_point timeout): NativeWorkerHandle(this){
			this->func = func;
			task = Task::callback_dummy(dummy_data, on_await, on_cancel, on_timeout, on_destruct, timeout);
			task->started = true;
			put_arguments(args, std::move(arguments));
		}
		#if _WIN32
		void handle(void* unused0, NativeWorkerHandle* unused1, unsigned long unused2, bool unused3){
		#elif defined(__linux__)
		void handle(NativeWorkerHandle* unused1, io_uring_cqe* unused){
		#endif
			ValueItem* result = nullptr;
			try{
				result = FuncEnvironment::sync_call(func, (ValueItem*)args.val, args.meta.val_len);
			}catch(...){
				MutexUnify mtx(task->no_race);
				art::unique_lock<MutexUnify> ulock(mtx);
				task->fres.finalResult(std::current_exception(), ulock);
				task->end_of_life = true;
				return;
			}
			MutexUnify mtx(task->no_race);
			art::unique_lock<MutexUnify> ulock(mtx);
			task->fres.finalResult(result, ulock);
			task->end_of_life = true;
			delete this;
		}

		bool start(){
			if(started)
				return false;
			#if _WIN32
				return started = NativeWorkersSingleton::post_work(this);
			#elif defined(__linux__)
				NativeWorkersSingleton::post_yield(this);
				return started = true;
			#endif
		}
	};

	art::shared_ptr<Task> Task::create_native_task(art::shared_ptr<FuncEnvironment> func){
		native_task_schedule* schedule = new native_task_schedule(func);
		if(!schedule->start()){
			delete schedule;
			throw AttachARuntimeException("Failed to start native task");
		}
		return schedule->task;
	}
	art::shared_ptr<Task> Task::create_native_task(art::shared_ptr<FuncEnvironment> func, const ValueItem& arguments){
		native_task_schedule* schedule = new native_task_schedule(func, arguments);
		if(!schedule->start()){
			delete schedule;
			throw AttachARuntimeException("Failed to start native task");
		}
		return schedule->task;
	}
	art::shared_ptr<Task> Task::create_native_task(art::shared_ptr<FuncEnvironment> func, ValueItem&& arguments){
		native_task_schedule* schedule = new native_task_schedule(func, std::move(arguments));
		if(!schedule->start()){
			delete schedule;
			throw AttachARuntimeException("Failed to start native task");
		}
		return schedule->task;
	}
	art::shared_ptr<Task> Task::create_native_task(art::shared_ptr<FuncEnvironment> func, const ValueItem& arguments, ValueItem& dummy_data, void(*on_await)(ValueItem&), void(*on_cancel)(ValueItem&), void(*on_timeout)(ValueItem&), void(*on_destruct)(ValueItem&), std::chrono::high_resolution_clock::time_point timeout){
		native_task_schedule* schedule = new native_task_schedule(func, arguments, dummy_data, on_await, on_cancel, on_timeout, on_destruct, timeout);
		if(!schedule->start()){
			delete schedule;
			throw AttachARuntimeException("Failed to start native task");
		}
		return schedule->task;
	}
	art::shared_ptr<Task> Task::create_native_task(art::shared_ptr<FuncEnvironment> func, ValueItem&& arguments, ValueItem& dummy_data, void(*on_await)(ValueItem&), void(*on_cancel)(ValueItem&), void(*on_timeout)(ValueItem&), void(*on_destruct)(ValueItem&), std::chrono::high_resolution_clock::time_point timeout){
		native_task_schedule* schedule = new native_task_schedule(func, std::move(arguments), dummy_data, on_await, on_cancel, on_timeout, on_destruct, timeout);
		if(!schedule->start()){
			delete schedule;
			throw AttachARuntimeException("Failed to start native task");
		}
		return schedule->task;
	}
	void Task::explicitStartTimer() {
		startTimeController();
	}
	void Task::shutDown(){
		while(!glob.binded_workers.empty())
			Task::close_bind_only_executor(glob.binded_workers.begin()->first);
		
		MutexUnify mtx(glob.task_thread_safety);
		art::unique_lock guard(mtx);
		size_t executors = glob.executors;
		for(size_t i = 0; i<executors;i++)
			glob.tasks.emplace(new Task(nullptr,{}));
		glob.tasks_notifier.notify_all();
		while(glob.executors)
			glob.executor_shutdown_notifier.wait(guard);
		glob.time_control_enabled = false;
		glob.time_notifier.notify_all();
	}

#pragma endregion
#pragma optimize("",off)
#pragma region Task: contexts swap
	void Task::sleep_until(std::chrono::high_resolution_clock::time_point time_point) {
		if (loc.is_task_thread) {
			art::lock_guard guard(loc.curr_task->no_race);
			makeTimeWait(time_point);
			swapCtxRelock(loc.curr_task->no_race);
		}
		else
			art::this_thread::sleep_until(time_point);
	}
	void Task::yield() {
		if (loc.is_task_thread) {
			art::lock_guard guard(glob.task_thread_safety);
			glob.tasks.push(loc.curr_task);
			swapCtxRelock(glob.task_thread_safety);
		}
		else
			throw EnvironmentRuinException("Thread attempt return yield task in non task enviro");
	}

#pragma endregion

#pragma region Task: TaskCallback


	art::shared_ptr<Task> Task::callback_dummy(ValueItem& dummy_data, void(*on_start)(ValueItem&), void(*on_await)(ValueItem&), void(*on_cancel)(ValueItem&), void(*on_timeout)(ValueItem&), void(*on_destruct)(ValueItem&), std::chrono::high_resolution_clock::time_point timeout){
		art::shared_ptr<Task> tsk = new Task(
			empty_func,
			nullptr,
			false,
			nullptr,
			timeout
		);
		tsk->args = new TaskCallback(dummy_data, on_await, on_cancel, on_timeout, on_start, on_destruct);
		tsk->_task_local = (ValueEnvironment*)-1;
		if(timeout != std::chrono::high_resolution_clock::time_point::min()){
			if(!glob.time_control_enabled)
				startTimeController();
			art::lock_guard guard(glob.task_timer_safety);
			glob.timed_tasks.push_back(timing(timeout,tsk));
		}
		return tsk;
	}
	art::shared_ptr<Task> Task::callback_dummy(ValueItem& dummy_data, void(*on_await)(ValueItem&), void(*on_cancel)(ValueItem&), void(*on_timeout)(ValueItem&), void(*on_destruct)(ValueItem&), std::chrono::high_resolution_clock::time_point timeout){
		art::shared_ptr<Task> tsk = new Task(
			empty_func,
			nullptr,
			false,
			nullptr,
			timeout
		);
		tsk->args = new TaskCallback(dummy_data, on_await, on_cancel, on_timeout, nullptr, on_destruct),
		tsk->_task_local = (ValueEnvironment*)-1;
		
		if(timeout != std::chrono::high_resolution_clock::time_point::min()){
			if(!glob.time_control_enabled)
				startTimeController();
			art::lock_guard guard(glob.task_timer_safety);
			glob.timed_tasks.push_back(timing(timeout,tsk));
		}
		return tsk;
	}

#pragma endregion
#pragma region TaskMutex
	TaskMutex::~TaskMutex() {
		art::lock_guard lg(no_race);
		while (!resume_task.empty()) {
			auto& tsk = resume_task.back();
			Task::notify_cancel(tsk.task);
			current_task = nullptr;
			Task::await_task(tsk.task);
			resume_task.pop_back();
		}
	}
	void TaskMutex::lock() {
		if (loc.is_task_thread) {
			loc.curr_task->awaked = false;
			loc.curr_task->time_end_flag = false;

			art::lock_guard lg(no_race);
			while (current_task) {
				resume_task.emplace_back(loc.curr_task, loc.curr_task->awake_check);
				swapCtxRelock(no_race);
			}
			current_task = &*loc.curr_task;
		}
		else {
			art::unique_lock ul(no_race);
			art::shared_ptr<Task> task; 
			while (current_task) {
				art::condition_variable_any cd;
				bool has_res = false;
				task = Task::cxx_native_bridge(has_res, cd);
				resume_task.emplace_back(task, task->awake_check);
				while (!has_res)
					cd.wait(ul);
				task_not_ended:
				//prevent destruct cd, because it is used in task
				task->no_race.lock();
				if (!task->fres.end_of_life) {
					task->no_race.unlock();
					goto task_not_ended;
				}
				task->no_race.unlock();
			}
			current_task = reinterpret_cast<Task*>((size_t)_thread_id() | native_thread_flag);
		}
	}
	bool TaskMutex::try_lock() {
		if (!no_race.try_lock())
			return false;
		art::unique_lock ul(no_race, art::adopt_lock);

		if (current_task)
			return false;
		else if(loc.is_task_thread || loc.context_in_swap)
			current_task = &*loc.curr_task;
		else
			current_task = reinterpret_cast<Task*>((size_t)art::this_thread::get_id() | native_thread_flag);
		return true;
	}

	bool TaskMutex::try_lock_for(size_t milliseconds) {
		return try_lock_until(std::chrono::high_resolution_clock::now() + std::chrono::milliseconds(milliseconds));
	}
	bool TaskMutex::try_lock_until(std::chrono::high_resolution_clock::time_point time_point) {
		if (!no_race.try_lock_until(time_point))
			return false;
		art::unique_lock ul(no_race, art::adopt_lock);

		if (loc.is_task_thread && !loc.context_in_swap) {
			while (current_task) {
				art::lock_guard guard(loc.curr_task->no_race);
				makeTimeWait(time_point);
				resume_task.emplace_back(loc.curr_task, loc.curr_task->awake_check);
				swapCtxRelock(loc.curr_task->no_race, no_race);
				if (!loc.curr_task->awaked) 
					return false;
			}
			current_task = &*loc.curr_task;
			return true;
		}
		else {
			bool has_res;
			art::condition_variable_any cd;
			while (current_task) {
				has_res = false;
				art::shared_ptr<Task> task = Task::cxx_native_bridge(has_res, cd);
				resume_task.emplace_back(task, task->awake_check);
				while (has_res)
					cd.wait_until(ul, time_point);
				if (!task->awaked)
					return false;
			}
			if(!loc.context_in_swap)
				current_task = reinterpret_cast<Task*>((size_t)_thread_id() | native_thread_flag);
			else
				current_task = &*loc.curr_task;
			return true;
		}
	}
	void TaskMutex::unlock() {
		art::lock_guard lg0(no_race);
		if (loc.is_task_thread) {
			if (current_task != &*loc.curr_task)
				throw InvalidOperation("Tried unlock non owned mutex");
		}
		else if (current_task != reinterpret_cast<Task*>((size_t)_thread_id() | native_thread_flag))
			throw InvalidOperation("Tried unlock non owned mutex");

		current_task = nullptr;
		if (resume_task.size()) {
			art::shared_ptr<Task> it = resume_task.front().task;
			uint16_t awake_check = resume_task.front().awake_check;
			resume_task.pop_front();
			art::lock_guard lg1(it->no_race);
			if(it->awake_check != awake_check)
				return;
			if (!it->time_end_flag) {
				it->awaked = true;
				transfer_task(it);
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
	bool TaskMutex::is_own(){
		art::lock_guard lg0(no_race);
		if (loc.is_task_thread) {
			if (current_task != &*loc.curr_task)
				return false;
		}
		else if (current_task != reinterpret_cast<Task*>((size_t)_thread_id() | native_thread_flag))
			return false;
		return true;
	}

	ValueItem* _TaskMutex_lock_holder(ValueItem* args, uint32_t len){
		art::shared_ptr<Task>& task = *(art::shared_ptr<Task>*)args[0].val;
		TaskMutex* mut = (TaskMutex*)args[1].val;
		bool lock_sequence = args[2].val;
		art::unique_lock guard(*mut, art::defer_lock);
		while(!task->fres.end_of_life){
			guard.lock();
			Task::await_task(task);
			if(!lock_sequence)
				return nullptr;
			{
				art::lock_guard task_guard(task->no_race);
				if(!task->fres.result_notify.has_waiters())
					task->fres.results.clear();
			}
			guard.unlock();
		}
		return nullptr;
	}
	art::shared_ptr<FuncEnvironment> TaskMutex_lock_holder = new FuncEnvironment(_TaskMutex_lock_holder, false, false);
	void TaskMutex::lifecycle_lock(art::shared_ptr<Task> task){
		Task::start(new Task(TaskMutex_lock_holder, ValueItem{ValueItem(new art::shared_ptr(task), VType::async_res), this, false}));
	}
	void TaskMutex::sequence_lock(art::shared_ptr<Task> task){
		Task::start(new Task(TaskMutex_lock_holder, ValueItem{ValueItem(new art::shared_ptr(task), VType::async_res), this, true}));
	}


#pragma endregion
#pragma region TaskRecursiveMutex

	TaskRecursiveMutex::~TaskRecursiveMutex() noexcept(false) {
		if (recursive_count)
			throw InvalidOperation("Mutex destroyed while locked");
	}
	void TaskRecursiveMutex::lock() {
		if(loc.is_task_thread){
			if (mutex.current_task == &*loc.curr_task) {
				recursive_count++;
				if(recursive_count == 0) {
					recursive_count--;
					throw InvalidOperation("Recursive mutex overflow");
				}
			}
			else{
				mutex.lock();
				recursive_count = 1;
			}
		}else{
			if (mutex.current_task == reinterpret_cast<Task*>((size_t)_thread_id() | native_thread_flag)) {
				recursive_count++;
				if(recursive_count == 0) {
					recursive_count--;
					throw InvalidOperation("Recursive mutex overflow");
				}
			}
			else{
				mutex.lock();
				recursive_count = 1;
			}
		}
	}
	bool TaskRecursiveMutex::try_lock() {
		if(loc.is_task_thread){
			if (mutex.current_task == &*loc.curr_task) {
				recursive_count++;
				if(recursive_count == 0) {
					recursive_count--;
					return false;
				}
				return true;
			}
			else if (mutex.try_lock()) {
				recursive_count = 1;
				return true;
			}
			else return false;
		}else{
			if (mutex.current_task == reinterpret_cast<Task*>((size_t)_thread_id() | native_thread_flag)) {
				recursive_count++;
				if(recursive_count == 0) {
					recursive_count--;
					return false;
				}
				return true;
			}
			else if (mutex.try_lock()) {
				recursive_count = 1;
				return true;
			}
			else return false;
		}
	}
	bool TaskRecursiveMutex::try_lock_for(size_t milliseconds) {
		if(loc.is_task_thread){
			if (mutex.current_task == &*loc.curr_task) {
				recursive_count++;
				if(recursive_count == 0) {
					recursive_count--;
					return false;
				}
				return true;
			}
			else if (mutex.try_lock_for(milliseconds)) {
				recursive_count = 1;
				return true;
			}
			else return false;
		}else{
			if (mutex.current_task == reinterpret_cast<Task*>((size_t)_thread_id() | native_thread_flag)) {
				recursive_count++;
				if(recursive_count == 0) {
					recursive_count--;
					return false;
				}
				return true;
			}
			else if (mutex.try_lock_for(milliseconds)) {
				recursive_count = 1;
				return true;
			}
			else return false;
		}
	}
	bool TaskRecursiveMutex::try_lock_until(std::chrono::high_resolution_clock::time_point time_point) {
		if(loc.is_task_thread){
			if (mutex.current_task == &*loc.curr_task) {
				recursive_count++;
				if(recursive_count == 0) {
					recursive_count--;
					return false;
				}
				return true;
			}
			else if (mutex.try_lock_until(time_point)) {
				recursive_count = 1;
				return true;
			}
			else return false;
		}else{
			if (mutex.current_task == reinterpret_cast<Task*>((size_t)_thread_id() | native_thread_flag)) {
				recursive_count++;
				if(recursive_count == 0) {
					recursive_count--;
					return false;
				}
				return true;
			}
			else if (mutex.try_lock_until(time_point)) {
				recursive_count = 1;
				return true;
			}
			else return false;
		}
	}
	void TaskRecursiveMutex::unlock() {
		if (recursive_count) {
			recursive_count--;
			if (!recursive_count)
				mutex.unlock();
		}
		else throw InvalidOperation("Mutex not locked");
	}
	bool TaskRecursiveMutex::is_locked() {
		if(recursive_count)
			return true;
		else
			return false;
	}
	void TaskRecursiveMutex::lifecycle_lock(art::shared_ptr<Task> task){
		mutex.lifecycle_lock(task);
	}
	void TaskRecursiveMutex::sequence_lock(art::shared_ptr<Task> task){
		mutex.sequence_lock(task);
	}
	bool TaskRecursiveMutex::is_own() {
		if(loc.is_task_thread){
			if (mutex.current_task == &*loc.curr_task) 
				return true;
		}else if (mutex.current_task == reinterpret_cast<Task*>((size_t)_thread_id() | native_thread_flag))
			return true;
		return false;
	}
#pragma endregion

#pragma region TaskConditionVariable
	TaskConditionVariable::TaskConditionVariable() {}

	TaskConditionVariable::~TaskConditionVariable() {
		notify_all();
	}

	void TaskConditionVariable::wait(art::unique_lock<MutexUnify>& mut) {
		if (loc.is_task_thread) {
			if (mut.mutex()->nmut == &no_race) {
				resume_task.emplace_back(loc.curr_task, loc.curr_task->awake_check);
				swapCtxRelock(no_race);
			}
			else {
				art::lock_guard guard(no_race);
				resume_task.emplace_back(loc.curr_task, loc.curr_task->awake_check);
				swapCtxRelock(*mut.mutex(),no_race);
			}
		}
		else {
			art::condition_variable_any cd;
			bool has_res = false;
			art::shared_ptr<Task> task = Task::cxx_native_bridge(has_res, cd);
			if (mut.mutex()->nmut == &no_race) {
				resume_task.emplace_back(task, task->awake_check);
				while (!has_res)
					cd.wait(mut);
			}
			else {
				no_race.lock();
				resume_task.emplace_back(task, task->awake_check);
				no_race.unlock();
				while (!has_res)
					cd.wait(mut);
			}
		task_not_ended:
			//prevent destruct cd, because it is used in task
			task->no_race.lock();
			if (!task->fres.end_of_life) {
				task->no_race.unlock();
				goto task_not_ended;
			}
			task->no_race.unlock();
		}
	}


	bool TaskConditionVariable::wait_for(art::unique_lock<MutexUnify>& mut, size_t milliseconds) {
		return wait_until(mut, std::chrono::high_resolution_clock::now() + std::chrono::milliseconds(milliseconds));
	}

	bool TaskConditionVariable::wait_until(art::unique_lock<MutexUnify>& mut, std::chrono::high_resolution_clock::time_point time_point) {
		if (loc.is_task_thread) {
			art::lock_guard guard(loc.curr_task->no_race);
			makeTimeWait(time_point);
			{
				art::lock_guard guard(no_race);
				resume_task.emplace_back(loc.curr_task, loc.curr_task->awake_check);
			}
			swapCtxRelock(loc.curr_task->no_race);
			if (loc.curr_task->time_end_flag)
				return false;
		}
		else {
			art::condition_variable_any cd;
			bool has_res = false;
			art::shared_ptr<Task> task = Task::cxx_native_bridge(has_res, cd);
			
			if (mut.mutex()->nmut == &no_race) {
				resume_task.emplace_back(task, task->awake_check);
				while (!has_res)
					cd.wait(mut);
			}
			else {
				no_race.lock();
				resume_task.emplace_back(task, task->awake_check);
				no_race.unlock();
				while (!has_res)
					cd.wait(mut);
			}

		task_not_ended:
			//prevent destruct cd, because it is used in task
			task->no_race.lock();
			if(!task->fres.end_of_life){
				task->no_race.unlock();
				goto task_not_ended;
			}
			task->no_race.unlock();

			return !task->time_end_flag;
		}
		return true;
	}

	void TaskConditionVariable::notify_all() {
		std::list<__::resume_task> revive_tasks;
		{
			art::lock_guard guard(no_race);
			std::swap(revive_tasks, resume_task);
			if (revive_tasks.empty())
				return;
		}
		bool to_yield = false;
		{
			art::lock_guard guard(glob.task_thread_safety);
			for (auto& resumer : revive_tasks) {
				auto& it = resumer.task;
				art::lock_guard guard_loc(it->no_race);
				if(resumer.awake_check != resumer.awake_check)
					continue;
				if (!it->time_end_flag) {
					it->awaked = true;
					transfer_task(it);
				}
			}
			glob.tasks_notifier.notify_one();
			if (Task::max_running_tasks && loc.is_task_thread) {
				if (Task::max_running_tasks <= glob.in_run_tasks && loc.curr_task && !loc.curr_task->end_of_life)
					to_yield = true;
			}
		}
		if (to_yield)
			Task::yield();
	}
	void TaskConditionVariable::notify_one() {
		art::shared_ptr<Task> tsk;
		{
			art::lock_guard guard(no_race);
			while (resume_task.size()) {
				resume_task.back().task->no_race.lock();
				if (resume_task.back().task->time_end_flag || resume_task.back().awake_check != resume_task.back().awake_check) {
					resume_task.back().task->no_race.unlock();
					resume_task.pop_back();
				}
				else {
					tsk = resume_task.back().task;
					resume_task.pop_back();
					break;
				}
			}
			if (resume_task.empty())
				return;
		}
		bool to_yield = false;
		art::lock_guard guard_loc(tsk->no_race, art::adopt_lock);
		{
			tsk->awaked = true;
			art::lock_guard guard(glob.task_thread_safety);
			if (Task::max_running_tasks && loc.is_task_thread) {
				if (Task::max_running_tasks <= glob.in_run_tasks && loc.curr_task && !loc.curr_task->end_of_life)
					to_yield = true;
			}
			transfer_task(tsk);
		}
		if(to_yield)
			Task::yield();
	}
	void TaskConditionVariable::dummy_wait(art::shared_ptr<Task> task, art::unique_lock<MutexUnify>& lock){
		if(lock.mutex()->nmut == &no_race)
			resume_task.emplace_back(task, task->awake_check);
		else{
			art::lock_guard guard(no_race);
			resume_task.emplace_back(task, task->awake_check);
		}
	}
	void TaskConditionVariable::dummy_wait_for(art::shared_ptr<Task> task, art::unique_lock<MutexUnify>& lock, size_t milliseconds){
		dummy_wait_until(task, lock, std::chrono::high_resolution_clock::now() + std::chrono::milliseconds(milliseconds));
	}

	ValueItem* _TaskConditionVariable_dummy_awaiter(ValueItem* args, uint32_t len){
		art::shared_ptr<Task>& task = *(art::shared_ptr<Task>*)args[0].val;
		TaskConditionVariable* cv = (TaskConditionVariable*)args[1].val;
		std::chrono::high_resolution_clock::time_point& time_point = (std::chrono::high_resolution_clock::time_point&)args[2];
		art::unique_lock<MutexUnify> lock(*(MutexUnify*)args[3].val, art::defer_lock);
		{
			MutexUnify uni(loc.curr_task->no_race);
			art::unique_lock l(uni);
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
	void TaskConditionVariable::dummy_wait_until(art::shared_ptr<Task> task, art::unique_lock<MutexUnify>& lock, std::chrono::high_resolution_clock::time_point time_point){
		static art::shared_ptr<FuncEnvironment> TaskConditionVariable_dummy_awaiter(new FuncEnvironment(_TaskConditionVariable_dummy_awaiter, false, false));
		delete Task::get_result(new Task(TaskConditionVariable_dummy_awaiter, 
			ValueItem{
			ValueItem(new art::shared_ptr<Task>(task), VType::async_res)
			, 
			this,
			 time_point,
			  lock.mutex()
			}));
	}

	bool TaskConditionVariable::has_waiters(){
		art::lock_guard guard(no_race);
		return !resume_task.empty();
	}


#pragma endregion

#pragma region TaskSemaphore
	void TaskSemaphore::setMaxThreshold(size_t val) {
		art::lock_guard guard(no_race);
		release_all();
		max_threshold = val;
		allow_threshold = max_threshold;
	}
	void TaskSemaphore::lock() {
		loc.curr_task->awaked = false;
		loc.curr_task->time_end_flag = false;
	re_try:
		no_race.lock();
		if (!allow_threshold) {
			if (loc.is_task_thread) {
				art::lock_guard guard(glob.task_thread_safety);
				resume_task.emplace_back(loc.curr_task, loc.curr_task->awake_check);
				no_race.unlock();
				swapCtxRelock(glob.task_thread_safety);
			}
			else {
				art::mutex mtx;
				art::unique_lock guard(mtx);
				no_race.unlock();
				native_notify.wait(guard);
			}
			goto re_try;
		}
		else
			--allow_threshold;
		no_race.unlock();
		return;
	}
	bool TaskSemaphore::try_lock() {
		if (!no_race.try_lock())
			return false;
		if (!allow_threshold) {
			no_race.unlock();
			return false;
		}
		else
			--allow_threshold;
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
		if (!allow_threshold) {
			if (loc.is_task_thread) {
				art::lock_guard guard(glob.task_thread_safety);
				makeTimeWait(time_point);
				resume_task.emplace_back(loc.curr_task, loc.curr_task->awake_check);
				no_race.unlock();
				swapCtxRelock(glob.task_thread_safety);
				if (!loc.curr_task->awaked)
					return false;
			}
			else {
				no_race.unlock();
				art::mutex mtx;
				art::unique_lock guard(mtx);
				if (native_notify.wait_until(guard, time_point) == art::cv_status::timeout)
					return false;
			}
			goto re_try;
		}
		if (allow_threshold)
			--allow_threshold;
		no_race.unlock();
		return true;

	}
	void TaskSemaphore::release() {
		art::lock_guard lg0(no_race);
		if (allow_threshold == max_threshold)
			return;
		allow_threshold++;
		native_notify.notify_one();
		while (resume_task.size()) {
			auto& it = resume_task.front();
			art::lock_guard lg2(it.task->no_race);
			if (!it.task->time_end_flag) {
				if(it.task->awake_check != it.awake_check)
					continue;
				it.task->awaked = true;
				auto task = resume_task.front().task;
				resume_task.pop_front();
				transfer_task(task);
				return;
			}
			else
				resume_task.pop_front();
		}
	}
	void TaskSemaphore::release_all() {
		art::lock_guard lg0(no_race);
		if (allow_threshold == max_threshold)
			return;
		art::lock_guard lg1(glob.task_thread_safety);
		allow_threshold = max_threshold;
		native_notify.notify_all();
		while (resume_task.size()) {
			auto& it = resume_task.back();
			art::lock_guard lg2(it.task->no_race);
			if (!it.task->time_end_flag) {
				if (it.task->awake_check != it.awake_check)
					continue;
				it.task->awaked = true;
				auto task = resume_task.front().task;
				resume_task.pop_front();
				transfer_task(task);
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

	void TaskLimiter::set_max_threshold(size_t val) {
		art::lock_guard guard(no_race);
		if (val < 1)
			val = 1;
		if (max_threshold == val)
			return;
		if (max_threshold > val) {
			if(allow_threshold > max_threshold - val)
				allow_threshold -= max_threshold - val;
			else {
				locked = true;
				allow_threshold = 0;
			}
			max_threshold = val;
			return;
		}
		else {
			if (!allow_threshold) {
				size_t unlocks = max_threshold;
				max_threshold = val;
				if (allow_threshold >= 1)
					locked = false;
				while (unlocks-- >= 1)
					unchecked_unlock();
			}
			else 
				allow_threshold += val - max_threshold;
		}
	}
	void TaskLimiter::lock() {
		art::unique_lock guard(no_race);
		while (!locked) {
			if (loc.is_task_thread) {
				loc.curr_task->awaked = false;
				loc.curr_task->time_end_flag = false;
				resume_task.emplace_back(loc.curr_task, loc.curr_task->awake_check);
				swapCtxRelock(*guard.mutex());
			}
			else 
				native_notify.wait(guard);
		}
		if (--allow_threshold == 0)
			locked = true;

		if (lock_check.contains(&*loc.curr_task)) {
			if (++allow_threshold != 0)
				locked = false;
			no_race.unlock();
			throw InvalidLock("Dead lock. Task try lock already locked task limiter");
		}
		else 
			lock_check.push_back(&*loc.curr_task);
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
		else if (--allow_threshold <= 0)
			locked = true;

		if (lock_check.contains(&*loc.curr_task)) {
			if (++allow_threshold != 0)
				locked = false;
			no_race.unlock();
			throw InvalidLock("Dead lock. Task try lock already locked task limiter");
		}
		else
			lock_check.push_back(&*loc.curr_task);
		no_race.unlock();
		return true;
	}

	bool TaskLimiter::try_lock_for(size_t milliseconds) {
		return try_lock_until(std::chrono::high_resolution_clock::now() + std::chrono::milliseconds(milliseconds));
	}
	bool TaskLimiter::try_lock_until(std::chrono::high_resolution_clock::time_point time_point) {
		if (no_race.try_lock_until(time_point))
			return false;
		art::unique_lock guard(no_race, art::adopt_lock);
		while(!locked) {
			if (loc.is_task_thread) {
				loc.curr_task->awaked = false;
				loc.curr_task->time_end_flag = false;
				makeTimeWait(time_point);
				resume_task.emplace_back(loc.curr_task, loc.curr_task->awake_check);
				swapCtxRelock(no_race);
				if (!loc.curr_task->awaked)
					return false;
			}
			else if (native_notify.wait_until(guard, time_point) == art::cv_status::timeout)
				return false;
		}
		if (--allow_threshold <= 0)
			locked = true;

		if (lock_check.contains(&*loc.curr_task)) {
			if (++allow_threshold != 0)
				locked = false;
			no_race.unlock();
			throw InvalidLock("Dead lock. Task try lock already locked task limiter");
		}
		else
			lock_check.push_back(&*loc.curr_task);
		no_race.unlock();
		return true;

	}
	void TaskLimiter::unlock() {
		art::lock_guard lg0(no_race);
		if (!lock_check.contains(&*loc.curr_task))
			throw InvalidUnlock("Invalid unlock. Task try unlock already unlocked task limiter");
		else
			lock_check.erase(&*loc.curr_task);
		unchecked_unlock();
	}
	void TaskLimiter::unchecked_unlock() {
		if (allow_threshold >= max_threshold)
			return;
		allow_threshold++;
		native_notify.notify_one();
		while (resume_task.size()) {
			auto& it = resume_task.back();
			art::lock_guard lg2(it.task->no_race);
			if (!it.task->time_end_flag) {
				if(it.task->awake_check != it.awake_check)
					continue;
				it.task->awaked = true;
				auto task = resume_task.front().task;
				resume_task.pop_front();
				transfer_task(task);
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

#pragma region EventSystem
	bool EventSystem::removeOne(std::list<art::shared_ptr<FuncEnvironment>>& list, const art::shared_ptr<FuncEnvironment>& func) {
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
	void EventSystem::async_call(std::list<art::shared_ptr<FuncEnvironment>>& list, ValueItem& args) {
		art::lock_guard guard(no_race);
		for (auto& it : list)
			Task::start(new Task(it, args));
	}
	bool EventSystem::awaitCall(std::list<art::shared_ptr<FuncEnvironment>>& list, ValueItem& args) {
		std::list<art::shared_ptr<Task>> wait_tasks;
		{
			art::lock_guard guard(no_race);
			for (auto& it : list) {
				art::shared_ptr<Task> tsk(new Task(it, args));
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
	bool EventSystem::sync_call(std::list<art::shared_ptr<FuncEnvironment>>& list, ValueItem& args) {
		art::lock_guard guard(no_race);
		if (args.meta.vtype == VType::async_res)
			args.getAsync();
		if (args.meta.vtype == VType::noting)
			for (art::shared_ptr<FuncEnvironment>& it : list) {
				auto res = it->syncWrapper(nullptr, 0);
				if (res)
					if (isTrueValue(&res->val)) {
						delete res;
						return true;
					}
			}
		else
			for (art::shared_ptr<FuncEnvironment>& it : list) {
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

	void EventSystem::operator+=(const art::shared_ptr<FuncEnvironment>& func) {
		art::lock_guard guard(no_race);
		avg_priority.push_back(func);
	}
	void EventSystem::join(const art::shared_ptr<FuncEnvironment>& func, bool async_mode, Priority priority) {
		art::lock_guard guard(no_race);
		if (async_mode) {
			switch (priority) {
			case Priority::heigh:
				async_heigh_priority.push_back(func);
				break;
			case Priority::upper_avg:
				async_upper_avg_priority.push_back(func);
				break;
			case Priority::avg:
				async_avg_priority.push_back(func);
				break;
			case Priority::lower_avg:
				async_lower_avg_priority.push_back(func);
				break;
			case Priority::low:
				async_low_priority.push_back(func);
				break;
			default:
				break;
			}
		}
		else {
			switch (priority) {
			case Priority::heigh:
				heigh_priority.push_back(func);
				break;
			case Priority::upper_avg:
				upper_avg_priority.push_back(func);
				break;
			case Priority::avg:
				avg_priority.push_back(func);
				break;
			case Priority::lower_avg:
				lower_avg_priority.push_back(func);
				break;
			case Priority::low:
				low_priority.push_back(func);
				break;
			default:
				break;
			}
		}
	}
	bool EventSystem::leave(const art::shared_ptr<FuncEnvironment>& func, bool async_mode, Priority priority) {
		art::lock_guard guard(no_race);
		if (async_mode) {
			switch (priority) {
			case Priority::heigh:
				return removeOne(async_heigh_priority, func);
			case Priority::upper_avg:
				return removeOne(async_upper_avg_priority, func);
			case Priority::avg:
				return removeOne(async_avg_priority, func);
			case Priority::lower_avg:
				return removeOne(async_lower_avg_priority, func);
			case Priority::low:
				return removeOne(async_low_priority, func);
			default:
				return false;
			}
		}
		else {
			switch (priority) {
			case Priority::heigh:
				return removeOne(heigh_priority, func);
			case Priority::upper_avg:
				return removeOne(upper_avg_priority, func);
			case Priority::avg:
				return removeOne(avg_priority, func);
			case Priority::lower_avg:
				return removeOne(lower_avg_priority, func);
			case Priority::low:
				return removeOne(low_priority, func);
			default:
				return false;
			}
		}
	}

	bool EventSystem::await_notify(ValueItem& it) {
		if(sync_call(heigh_priority, it)	) return true;
		if(sync_call(upper_avg_priority, it)) return true;
		if(sync_call(avg_priority, it)		) return true;
		if(sync_call(lower_avg_priority, it)) return true;
		if(sync_call(low_priority, it)		) return true;

		if(awaitCall(async_heigh_priority, it)		) return true;
		if(awaitCall(async_upper_avg_priority, it)	) return true;
		if(awaitCall(async_avg_priority, it)		) return true;
		if(awaitCall(async_lower_avg_priority, it)	) return true;
		if(awaitCall(async_low_priority, it)		) return true;
		return false;
	}
	bool EventSystem::notify(ValueItem& it) {
		if (sync_call(heigh_priority, it)) return true;
		if (sync_call(upper_avg_priority, it)) return true;
		if (sync_call(avg_priority, it)) return true;
		if (sync_call(lower_avg_priority, it)) return true;
		if (sync_call(low_priority, it)) return true;

		async_call(async_heigh_priority, it);
		async_call(async_upper_avg_priority, it);
		async_call(async_avg_priority, it);
		async_call(async_lower_avg_priority, it);
		async_call(async_low_priority, it);
		return false;
	}
	bool EventSystem::sync_notify(ValueItem& it) {
		if(sync_call(heigh_priority, it)	) return true;
		if(sync_call(upper_avg_priority, it)) return true;
		if(sync_call(avg_priority, it)		) return true;
		if(sync_call(lower_avg_priority, it)) return true;
		if(sync_call(low_priority, it)		) return true;
		if(sync_call(async_heigh_priority, it)		) return true;
		if(sync_call(async_upper_avg_priority, it)	) return true;
		if(sync_call(async_avg_priority, it)		) return true;
		if(sync_call(async_lower_avg_priority, it)	) return true;
		if(sync_call(async_low_priority, it)		) return true;
		return false;
	}

	ValueItem* __async_notify(ValueItem* vals, uint32_t) {
		EventSystem* es = (EventSystem*)vals->val;
		ValueItem& args = vals[1];
		if (es->sync_call(es->heigh_priority, args)) return new ValueItem(true);
		if (es->sync_call(es->upper_avg_priority, args)) return new ValueItem(true);
		if (es->sync_call(es->avg_priority, args)) return new ValueItem(true);
		if (es->sync_call(es->lower_avg_priority, args)) return new ValueItem(true);
		if (es->sync_call(es->low_priority, args)) return new ValueItem(true);
		if (es->awaitCall(es->async_heigh_priority, args)) return new ValueItem(true);
		if (es->awaitCall(es->async_upper_avg_priority, args)) return new ValueItem(true);
		if (es->awaitCall(es->async_avg_priority, args)) return new ValueItem(true);
		if (es->awaitCall(es->async_lower_avg_priority, args)) return new ValueItem(true);
		if (es->awaitCall(es->async_low_priority, args)) return new ValueItem(true);
		return new ValueItem(false);
	}
	art::shared_ptr<FuncEnvironment> _async_notify(new FuncEnvironment(__async_notify,false, false));

	art::shared_ptr<Task> EventSystem::async_notify(ValueItem& args) {
		ValueItem vals{ ValueItem(this,VType::undefined_ptr), args };
		art::shared_ptr<Task> res = new Task(_async_notify, vals);
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
		art::lock_guard lock(tqh->no_race);
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
		art::shared_ptr<FuncEnvironment>& call_func = *(art::shared_ptr<FuncEnvironment>*)args[1].val;
		ValueItem& arguments = *(ValueItem*)args[2].val;

		ValueItem* res = nullptr;
		try{
			res = FuncEnvironment::sync_call(call_func,(ValueItem*)arguments.getSourcePtr(),arguments.meta.val_len);
		}
		catch(...){
			__TaskQuery_add_task_leave(tqh, tq);
			throw;
		}
		__TaskQuery_add_task_leave(tqh, tq);
		return res;
	}
	art::shared_ptr<FuncEnvironment> _TaskQuery_add_task(new FuncEnvironment(__TaskQuery_add_task,false, false));
	art::shared_ptr<Task> TaskQuery::add_task(art::shared_ptr<FuncEnvironment> call_func, ValueItem& arguments, bool used_task_local, art::shared_ptr<FuncEnvironment> exception_handler, std::chrono::high_resolution_clock::time_point timeout) {
		ValueItem copy;
		if (arguments.meta.vtype == VType::faarr || arguments.meta.vtype == VType::saarr)
			copy = ValueItem((ValueItem*)arguments.getSourcePtr(), arguments.meta.val_len);
		else
			copy = ValueItem({ arguments });

		art::shared_ptr<Task> res = new Task(_TaskQuery_add_task, ValueItem{(void*)handle, new art::shared_ptr<FuncEnvironment>(call_func), copy }, used_task_local, exception_handler, timeout);
		art::lock_guard lock(handle->no_race);
		if(is_running && handle->now_at_execution <= handle->at_execution_max){
			Task::start(res);
			handle->now_at_execution++;
		}
		else tasks.push_back(res);

		return res;
	}
	void TaskQuery::enable(){
		art::lock_guard lock(handle->no_race);
		is_running = true;
		while(handle->now_at_execution < handle->at_execution_max && !tasks.empty()){
			auto awake_task = tasks.front();
			tasks.pop_front();
			Task::start(awake_task);
			handle->now_at_execution++;
		}
	}
	void TaskQuery::disable(){
		art::lock_guard lock(handle->no_race);
		is_running = false;
	}
	bool TaskQuery::in_query(art::shared_ptr<Task> task){
		if(task->started)
			return false;//started task can't be in query
		art::lock_guard lock(handle->no_race);
		return std::find(tasks.begin(), tasks.end(), task) != tasks.end();
	}
	void TaskQuery::set_max_at_execution(size_t val){
		art::lock_guard lock(handle->no_race);
		handle->at_execution_max = val;
	}
	size_t TaskQuery::get_max_at_execution(){
		art::lock_guard lock(handle->no_race);
		return handle->at_execution_max;
	}


	void TaskQuery::wait(){
		MutexUnify unify(handle->no_race);
		art::unique_lock lock(unify);
		while(handle->now_at_execution != 0 && !tasks.empty())
			handle->end_of_query.wait(lock);
	}
	bool TaskQuery::wait_for(size_t milliseconds){
		return wait_until(std::chrono::high_resolution_clock::now() + std::chrono::milliseconds(milliseconds));
	}
	bool TaskQuery::wait_until(std::chrono::high_resolution_clock::time_point time_point){
		MutexUnify unify(handle->no_race);
		art::unique_lock lock(unify);
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

		void prepare_generator(ValueItem& args,art::shared_ptr<FuncEnvironment>& func, art::shared_ptr<FuncEnvironment>& ex_handler, Generator*& weak_ref){
			weak_ref = &*loc.on_load_generator_ref;
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
			art::shared_ptr<FuncEnvironment> func;
			art::shared_ptr<FuncEnvironment> ex_handler;
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
		

		Generator::Generator(art::shared_ptr<FuncEnvironment> call_func, const ValueItem& arguments, bool used_generator_local, art::shared_ptr<FuncEnvironment> exception_handler){
			args = arguments;
			func = call_func;
			if(used_generator_local)
				_generator_local = new ValueEnvironment();
			else
				_generator_local = nullptr;
			
			ex_handle = exception_handler;
		}
		Generator::Generator(art::shared_ptr<FuncEnvironment> call_func, ValueItem&& arguments, bool used_generator_local, art::shared_ptr<FuncEnvironment> exception_handler){
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

		bool Generator::yield_iterate(art::shared_ptr<Generator>& generator){
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
		ValueItem* Generator::get_result(art::shared_ptr<Generator>& generator){
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
		bool Generator::has_result(art::shared_ptr<Generator>& generator){
			return !generator->results.empty() || (generator->context != nullptr && !generator->end_of_life);
		}
		list_array<ValueItem*> Generator::await_results(art::shared_ptr<Generator>& generator){
			list_array<ValueItem*> results;
			while(!generator->end_of_life)
				results.push_back(get_result(generator));
			return results;
		}
		list_array<ValueItem*> Generator::await_results(list_array<art::shared_ptr<Generator>>& generators){
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

		
		art::shared_ptr<Task> get_self(){
			return loc.curr_task;
		}

		

		std::pair<uint64_t,int64_t> _become_executor_count_manager_(
			art::unique_lock<art::recursive_mutex>& lock,
			std::chrono::high_resolution_clock::time_point& last_time,
			std::list<uint32_t>& workers_completions,
			art::util::hill_climb& hill_climber
			){
			std::chrono::high_resolution_clock::duration elapsed = std::chrono::high_resolution_clock::now() - last_time;
			uint32_t executor_count = workers_completions.size();
			if(executor_count == 0)
				return {100, 1};
			uint64_t sleep_count_avg = 0;
			uint64_t recommended_workers_avg = 0;
			for(auto& completions: workers_completions){
				auto [recommended_workers_,sleep_count_] = hill_climber.climb(executor_count, std::chrono::duration<double>(elapsed).count(), completions, 1, 10);
				completions = 0;
				sleep_count_avg += sleep_count_;
				recommended_workers_avg += recommended_workers_;
			}
			sleep_count_avg /= executor_count;
			recommended_workers_avg /= executor_count;
			return {sleep_count_avg, int64_t(recommended_workers_avg) - executor_count};
		}
		void become_executor_count_manager(bool leave_after_finish){
			art::unique_lock lock(glob.task_thread_safety);
			if(glob.executor_manager_in_work)
				return;
			glob.executor_manager_in_work = true;
			std::chrono::high_resolution_clock::time_point last_time = std::chrono::high_resolution_clock::now();
			while(true){
				uint64_t all_sleep_count_avg;
				auto [sleep_count_avg, workers_diff] = _become_executor_count_manager_(lock, last_time, glob.workers_completions, glob.executor_manager_hill_climb);
				all_sleep_count_avg = sleep_count_avg;
				if(workers_diff == 0);
				else if(workers_diff > 0){
					for(uint32_t i = 0; i < workers_diff; i++)
						art::thread(taskExecutor, false).detach();
				}
				else {
					art::shared_ptr<Task> task(new Task(nullptr, nullptr));
					for(uint32_t i = 0; i < workers_diff; i++)
						glob.tasks.push(task);
				}
				lock.unlock();
				{
					art::lock_guard binds(glob.binded_workers_safety);
					for(auto& contexts : glob.binded_workers){
						if(contexts.second.fixed_size)
							continue;
						art::unique_lock lock(contexts.second.no_race);
						auto [sleep_count_avg, workers_diff] = _become_executor_count_manager_(lock, last_time, contexts.second.completions, contexts.second.executor_manager_hill_climb);
						all_sleep_count_avg += sleep_count_avg;
						if(workers_diff == 0);
						else if(workers_diff > 0){
							for(uint32_t i = 0; i < workers_diff; i++)
								art::thread(bindedTaskExecutor, contexts.first).detach();
						}
						else {
							art::shared_ptr<Task> task(new Task(nullptr, nullptr));
							task->bind_to_worker_id = contexts.first;
							for(uint32_t i = 0; i < workers_diff; i++)
								contexts.second.tasks.push_back(task);
						}
					}
					all_sleep_count_avg /= glob.binded_workers.size() + 1;
				}


				last_time = std::chrono::high_resolution_clock::now();
				art::this_thread::sleep_for(std::chrono::milliseconds(all_sleep_count_avg));
				lock.lock();

				if(leave_after_finish)
					if(!(!glob.tasks.empty() || !glob.cold_tasks.empty() || !glob.timed_tasks.empty() || glob.in_exec || glob.tasks_in_swap || glob.in_run_tasks)){
						std::lock_guard lock(glob.binded_workers_safety);
						bool binded_tasks_empty = true;
						for(auto& contexts : glob.binded_workers)
							if(contexts.second.tasks.size())
								binded_tasks_empty = false;
						if(binded_tasks_empty)
							break;
					}
			}
		}
		void start_executor_count_manager(){
			art::lock_guard lock(glob.task_thread_safety);
			if(glob.executor_manager_in_work)
				return;
			art::thread(become_executor_count_manager, false).detach();
		}
	}
}