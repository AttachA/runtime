// Copyright Danyil Melnytskyi 2022-2023
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include "../AttachA_CXX.hpp"
#include "../../configuration/agreement/symbols.hpp"
namespace parallel {
	AttachAVirtualTable* define_ConditionVariable;
	AttachAVirtualTable* define_Mutex;
	AttachAVirtualTable* define_RecursiveMutex;
	AttachAVirtualTable* define_Semaphore;
	AttachAVirtualTable* define_ConcurentFile;
	AttachAVirtualTable* define_EventSystem;
	AttachAVirtualTable* define_TaskLimiter;
	AttachAVirtualTable* define_TaskQuery;
	AttachAVirtualTable* define_Task;
	AttachAVirtualTable* define_TaskResultIterator;
	AttachAVirtualTable* define_TaskGroup;

	template<size_t args_off>
	void parseArgumentsToTask(ValueItem* args, uint32_t len, typed_lgr<FuncEnvironment>& func, typed_lgr<FuncEnvironment>& fault_func, std::chrono::high_resolution_clock::time_point& timeout, bool& used_task_local, ValueItem& arguments){
		timeout = std::chrono::high_resolution_clock::time_point::min();
		used_task_local = false;

		AttachA::excepted(args[args_off], VType::function);
		func = *args[args_off].funPtr();

		if (len > args_off + 2) if(args[args_off + 2].meta.vtype != VType::noting){
			AttachA::excepted(args[args_off + 2], VType::function);
			fault_func = *args[args_off + 2].funPtr();
		}
		if(len > args_off + 3) if(args[args_off + 3].meta.vtype != VType::noting){
			if(args[args_off + 3].meta.vtype == VType::time_point)
				timeout = (std::chrono::high_resolution_clock::time_point)args[args_off + 3];
			else
				timeout = std::chrono::high_resolution_clock::now() + std::chrono::milliseconds((uint64_t)args[args_off + 3]);
		}
		if(len > args_off + 4) if(args[args_off + 4].meta.vtype != VType::noting)
			used_task_local = (bool)args[args_off + 4];
		arguments = len > 5 ? args[5] : nullptr;
	}

#pragma region ConditionVariable
	AttachAFun(funs_ConditionVariable_wait, 1,{
		auto& class_ = *AttachA::Interface::getExtractAs<typed_lgr<TaskConditionVariable>>(args[0], define_ConditionVariable);
		switch (len) {
		case 1:{
			run_time::threading::mutex mt;
			MutexUnify unif(mt);
			std::unique_lock lock(unif);
			class_.wait(lock);
			break;
		}
		case 2:{
			if (args[1].meta.vtype == VType::struct_) {
				auto& mutex = AttachA::Interface::getExtractAs<typed_lgr<TaskMutex>>(args[1], define_Mutex);
				MutexUnify unif(*mutex);
				std::unique_lock lock(unif, std::adopt_lock);
				class_.wait(lock);
				lock.release();
				break;
			}
			else if(args[1].meta.vtype == VType::time_point){
				run_time::threading::mutex mt;
				MutexUnify unif(mt);
				std::unique_lock lock(unif);
				auto res = class_.wait_until(lock, (std::chrono::high_resolution_clock::time_point)args[1]);
				lock.release();
				return res;
			}
			else{
				run_time::threading::mutex mt;
				MutexUnify unif(mt);
				std::unique_lock lock(unif);
				return class_.wait_for(lock, (size_t)args[1]);
			}
		}
		case 3:
		default:{
			auto& mutex = AttachA::Interface::getExtractAs<typed_lgr<TaskMutex>>(args[1], define_Mutex);
			MutexUnify unif(*mutex);
			std::unique_lock lock(unif, std::adopt_lock);
			bool res;
			if(args[2].meta.vtype == VType::time_point)
				res = class_.wait_until(lock, (std::chrono::high_resolution_clock::time_point)args[2]);
			else
				res = class_.wait_for(lock, (size_t)args[2]);
			lock.release();
			return res;
		}
		}
	})
	
	AttachAFun(funs_ConditionVariable_wait_until, 2,{
		auto& class_ = *AttachA::Interface::getExtractAs<typed_lgr<TaskConditionVariable>>(args[0], define_ConditionVariable);
		switch (len) {
		case 2:{
			run_time::threading::mutex mt;
			MutexUnify unif(mt);
			std::unique_lock lock(unif);
			return class_.wait_until(lock, (std::chrono::high_resolution_clock::time_point)args[1]);
		}
		case 3:
		default:{
			auto& mutex = AttachA::Interface::getExtractAs<typed_lgr<TaskMutex>>(args[1], define_Mutex);
			MutexUnify unif(*mutex);
			std::unique_lock lock(unif, std::adopt_lock);
			auto res = class_.wait_until(lock, (std::chrono::high_resolution_clock::time_point)args[2]);
			lock.release();
			return res;
		}
		}
	})
	AttachAFun(funs_ConditionVariable_notify_one, 1,{
		auto& class_ = *AttachA::Interface::getExtractAs<typed_lgr<TaskConditionVariable>>(args[0], define_ConditionVariable);
		class_.notify_one();
	})
	AttachAFun(funs_ConditionVariable_notify_all, 1,{
		auto& class_ = *AttachA::Interface::getExtractAs<typed_lgr<TaskConditionVariable>>(args[0], define_ConditionVariable);
		class_.notify_all();
	})

	void init_ConditionVariable() {
		define_ConditionVariable = AttachA::Interface::createTable<typed_lgr<TaskConditionVariable>>("condition_variable",
			AttachA::Interface::direct_method("wait", funs_ConditionVariable_wait),
			AttachA::Interface::direct_method("wait_until", funs_ConditionVariable_wait_until),
			AttachA::Interface::direct_method("notify_one", funs_ConditionVariable_notify_one),
			AttachA::Interface::direct_method("notify_all", funs_ConditionVariable_notify_all)
		);
		AttachA::Interface::typeVTable<typed_lgr<TaskConditionVariable>>() = define_ConditionVariable;
	}
#pragma endregion
#pragma region Mutex
	AttachAFun(funs_Mutex_lock, 1,{
		auto& class_ = *AttachA::Interface::getExtractAs<typed_lgr<TaskMutex>>(args[0], define_Mutex);
		class_.lock();
	})
	AttachAFun(funs_Mutex_unlock, 1,{
		auto& class_ = *AttachA::Interface::getExtractAs<typed_lgr<TaskMutex>>(args[0], define_Mutex);
		class_.unlock();
	})
	AttachAFun(funs_Mutex_try_lock, 1,{
		auto& class_ = *AttachA::Interface::getExtractAs<typed_lgr<TaskMutex>>(args[0], define_Mutex);
		if(len == 1)
			return class_.try_lock();
		else
			return class_.try_lock_for((size_t)args[1]);
	})
	AttachAFun(funs_Mutex_try_lock_until, 2,{
		auto& class_ = *AttachA::Interface::getExtractAs<typed_lgr<TaskMutex>>(args[0], define_Mutex);
		return class_.try_lock_until((std::chrono::high_resolution_clock::time_point)args[1]);
	})
	AttachAFun(funs_Mutex_is_locked, 1,{
		auto& class_ = *AttachA::Interface::getExtractAs<typed_lgr<TaskMutex>>(args[0], define_Mutex);
		return class_.is_locked();
	})
	AttachAFun(funs_Mutex_is_own, 1,{
		auto& class_ = *AttachA::Interface::getExtractAs<typed_lgr<TaskMutex>>(args[0], define_Mutex);
		return class_.is_own();
	})
	AttachAFun(funs_Mutex_lifecycle_lock, 2,{
		auto& class_ = *AttachA::Interface::getExtractAs<typed_lgr<TaskMutex>>(args[0], define_Mutex);
		class_.lifecycle_lock((typed_lgr<Task>)args[1]);
	})
	AttachAFun(funs_Mutex_sequence_lock, 2,{
		auto& class_ = *AttachA::Interface::getExtractAs<typed_lgr<TaskMutex>>(args[0], define_Mutex);
		class_.sequence_lock((typed_lgr<Task>)args[1]);
	})
	void init_Mutex() {
		define_Mutex = AttachA::Interface::createTable<typed_lgr<TaskMutex>>("mutex",
			AttachA::Interface::direct_method("lock", funs_Mutex_lock),
			AttachA::Interface::direct_method("unlock", funs_Mutex_unlock),
			AttachA::Interface::direct_method("try_lock", funs_Mutex_try_lock),
			AttachA::Interface::direct_method("try_lock_until", funs_Mutex_try_lock_until),
			AttachA::Interface::direct_method("is_locked", funs_Mutex_is_locked),
			AttachA::Interface::direct_method("is_own", funs_Mutex_is_own),
			AttachA::Interface::direct_method("lifecycle_lock", funs_Mutex_lifecycle_lock),
			AttachA::Interface::direct_method("sequence_lock", funs_Mutex_sequence_lock)
		);
		AttachA::Interface::typeVTable<typed_lgr<TaskMutex>>() = define_Mutex;
	}
#pragma endregion
#pragma region RecursiveMutex
	AttachAFun(funs_RecursiveMutex_lock, 1,{
		auto& class_ = *AttachA::Interface::getExtractAs<typed_lgr<TaskRecursiveMutex>>(args[0], define_RecursiveMutex);
		class_.lock();
	})
	AttachAFun(funs_RecursiveMutex_unlock, 1,{
		auto& class_ = *AttachA::Interface::getExtractAs<typed_lgr<TaskRecursiveMutex>>(args[0], define_RecursiveMutex);
		class_.unlock();
	})
	AttachAFun(funs_RecursiveMutex_try_lock, 1,{
		auto& class_ = *AttachA::Interface::getExtractAs<typed_lgr<TaskRecursiveMutex>>(args[0], define_RecursiveMutex);
		if(len == 1)
			return class_.try_lock();
		else
			return class_.try_lock_for((size_t)args[1]);
	})
	AttachAFun(funs_RecursiveMutex_try_lock_until, 2,{
		auto& class_ = *AttachA::Interface::getExtractAs<typed_lgr<TaskRecursiveMutex>>(args[0], define_RecursiveMutex);
		return class_.try_lock_until((std::chrono::high_resolution_clock::time_point)args[1]);
	})
	AttachAFun(funs_RecursiveMutex_is_locked, 1,{
		auto& class_ = *AttachA::Interface::getExtractAs<typed_lgr<TaskRecursiveMutex>>(args[0], define_RecursiveMutex);
		return class_.is_locked();
	})
	AttachAFun(funs_RecursiveMutex_is_own, 1,{
		auto& class_ = *AttachA::Interface::getExtractAs<typed_lgr<TaskRecursiveMutex>>(args[0], define_RecursiveMutex);
		return class_.is_own();
	})
	AttachAFun(funs_RecursiveMutex_lifecycle_lock, 2,{
		auto& class_ = *AttachA::Interface::getExtractAs<typed_lgr<TaskRecursiveMutex>>(args[0], define_RecursiveMutex);
		class_.lifecycle_lock((typed_lgr<Task>)args[1]);
	})
	AttachAFun(funs_RecursiveMutex_sequence_lock, 2,{
		auto& class_ = *AttachA::Interface::getExtractAs<typed_lgr<TaskRecursiveMutex>>(args[0], define_RecursiveMutex);
		class_.sequence_lock((typed_lgr<Task>)args[1]);
	})

	void init_RecursiveMutex() {
		define_RecursiveMutex = AttachA::Interface::createTable<typed_lgr<TaskRecursiveMutex>>("recursive_mutex",
			AttachA::Interface::direct_method("lock", funs_Mutex_lock),
			AttachA::Interface::direct_method("unlock", funs_Mutex_unlock),
			AttachA::Interface::direct_method("try_lock", funs_Mutex_try_lock),
			AttachA::Interface::direct_method("try_lock_until", funs_Mutex_try_lock_until),
			AttachA::Interface::direct_method("is_locked", funs_Mutex_is_locked),
			AttachA::Interface::direct_method("is_own", funs_RecursiveMutex_is_own),
			AttachA::Interface::direct_method("lifecycle_lock", funs_RecursiveMutex_lifecycle_lock),
			AttachA::Interface::direct_method("sequence_lock", funs_RecursiveMutex_sequence_lock)
		);
		AttachA::Interface::typeVTable<typed_lgr<TaskRecursiveMutex>>() = define_RecursiveMutex;
	}
#pragma endregion
#pragma region Semaphore
	AttachAFun(funs_Semaphore_lock, 1,{
		auto& class_ = *AttachA::Interface::getExtractAs<typed_lgr<TaskSemaphore>>(args[0], define_Semaphore);
		class_.lock();
	})
	AttachAFun(funs_Semaphore_release, 1,{
		auto& class_ = *AttachA::Interface::getExtractAs<typed_lgr<TaskSemaphore>>(args[0], define_Semaphore);
		class_.release();
	})
	AttachAFun(funs_Semaphore_release_all, 1,{
		auto& class_ = *AttachA::Interface::getExtractAs<typed_lgr<TaskSemaphore>>(args[0], define_Semaphore);
		class_.release_all();
	})
	AttachAFun(funs_Semaphore_try_lock, 1,{
		auto& class_ = *AttachA::Interface::getExtractAs<typed_lgr<TaskSemaphore>>(args[0], define_Semaphore);
		if(len == 1)
			return class_.try_lock();
		else
			return class_.try_lock_for((size_t)args[1]);
	})
	AttachAFun(funs_Semaphore_try_lock_until, 2,{
		auto& class_ = *AttachA::Interface::getExtractAs<typed_lgr<TaskSemaphore>>(args[0], define_Semaphore);
		return class_.try_lock_until((std::chrono::high_resolution_clock::time_point)args[1]);
	})
	AttachAFun(funs_Semaphore_is_locked, 1,{
		auto& class_ = *AttachA::Interface::getExtractAs<typed_lgr<TaskSemaphore>>(args[0], define_Semaphore);
		return class_.is_locked();
	})

	void init_Semaphore() {
		define_Semaphore = AttachA::Interface::createTable<typed_lgr<TaskSemaphore>>("semaphore",
			AttachA::Interface::direct_method("lock", funs_Semaphore_lock),
			AttachA::Interface::direct_method("release", funs_Semaphore_release),
			AttachA::Interface::direct_method("release_all", funs_Semaphore_release_all),
			AttachA::Interface::direct_method("try_lock", funs_Semaphore_try_lock),
			AttachA::Interface::direct_method("try_lock_until", funs_Semaphore_try_lock_until),
			AttachA::Interface::direct_method("is_locked", funs_Semaphore_is_locked)
		);
		AttachA::Interface::typeVTable<typed_lgr<TaskSemaphore>>() = define_Semaphore;
	}
#pragma endregion
#pragma region EventSystem
	AttachAFun(funs_EventSystem_operator_add, 2,{
		auto& class_ = *AttachA::Interface::getExtractAs<typed_lgr<EventSystem>>(args[0], define_EventSystem);
		AttachA::excepted(args[1], VType::function);
		auto& fun = *args[1].funPtr();
		class_ += fun;
	})
	AttachAFun(funs_EventSystem_join, 2,{
		auto& class_ = *AttachA::Interface::getExtractAs<typed_lgr<EventSystem>>(args[0], define_EventSystem);
		AttachA::excepted(args[1], VType::function);
		auto& fun = *args[1].funPtr();
		bool as_async = len > 2 ? (bool)args[2] : false;
		EventSystem::Priorithy priorithy = len > 3 ? (EventSystem::Priorithy)(uint8_t)args[3] : EventSystem::Priorithy::avg;
		class_.join(fun, as_async, priorithy);
	})
	AttachAFun(funs_EventSystem_leave, 2,{
		auto& class_ = *AttachA::Interface::getExtractAs<typed_lgr<EventSystem>>(args[0], define_EventSystem);
		AttachA::excepted(args[1], VType::function);
		auto& fun = *args[1].funPtr();
		bool as_async = len > 2 ? (bool)args[2] : false;
		EventSystem::Priorithy priorithy = len > 3 ? (EventSystem::Priorithy)(uint8_t)args[3] : EventSystem::Priorithy::avg;
		class_.leave(fun, as_async, priorithy);
	})
	ValueItem __funs_EventSystem_get_values0(ValueItem* vals, uint32_t len) {
		if (len > 2) {
			size_t size = len - 1;
			ValueItem* args = new ValueItem[size]{};
			std::unique_ptr<ValueItem[]> args_holder(args);
			for (uint32_t i = 0; i < size; i++)
				args[i] = vals[i + 1];
			return ValueItem(args_holder.release(), size, no_copy);
		}
		return nullptr;
	}
	AttachAFun(funs_EventSystem_notify, 2,{
		auto& class_ = *AttachA::Interface::getExtractAs<typed_lgr<EventSystem>>(args[0], define_EventSystem);
		if(len == 2)
			return class_.notify(args[1]);
		else{
			ValueItem values = __funs_EventSystem_get_values0(args, len);
			return class_.notify(values);
		}
	})
	AttachAFun(funs_EventSystem_sync_notify, 2,{
		auto& class_ = *AttachA::Interface::getExtractAs<typed_lgr<EventSystem>>(args[0], define_EventSystem);
		if(len == 2)
			return class_.sync_notify(args[1]);
		else{
			ValueItem values = __funs_EventSystem_get_values0(args, len);
			return class_.sync_notify(values);
		}
	})
	AttachAFun(funs_EventSystem_await_notify, 2,{
		auto& class_ = *AttachA::Interface::getExtractAs<typed_lgr<EventSystem>>(args[0], define_EventSystem);
		if(len == 2)
			return class_.await_notify(args[1]);
		else{
			ValueItem values = __funs_EventSystem_get_values0(args, len);
			return class_.await_notify(values);
		}
	})
	AttachAFun(funs_EventSystem_async_notify, 2,{
		auto& class_ = *AttachA::Interface::getExtractAs<typed_lgr<EventSystem>>(args[0], define_EventSystem);
		if(len == 2)
			return class_.async_notify(args[1]);
		else{
			ValueItem values = __funs_EventSystem_get_values0(args, len);
			return class_.async_notify(values);
		}
	})
	void init_EventSystem() {
		define_EventSystem = AttachA::Interface::createTable<typed_lgr<EventSystem>>("event_system",
			AttachA::Interface::direct_method(symbols::structures::add_operator, funs_EventSystem_operator_add),
			AttachA::Interface::direct_method("join", funs_EventSystem_join),
			AttachA::Interface::direct_method("leave", funs_EventSystem_leave),
			AttachA::Interface::direct_method("notify", funs_EventSystem_notify),
			AttachA::Interface::direct_method("sync_notify", funs_EventSystem_sync_notify),
			AttachA::Interface::direct_method("await_notify", funs_EventSystem_await_notify),
			AttachA::Interface::direct_method("async_notify", funs_EventSystem_async_notify)
		);
		AttachA::Interface::typeVTable<typed_lgr<EventSystem>>() = define_EventSystem;
	}
#pragma endregion
#pragma region TaskLimiter
	AttachAFun(funs_TaskLimiter_set_max_treeshold, 2, {
		auto& class_ = *AttachA::Interface::getExtractAs<typed_lgr<TaskLimiter>>(args[0], define_TaskLimiter);
		class_.set_max_treeshold((uint64_t)args[1]);
	})
	AttachAFun(funs_TaskLimiter_lock, 1, {
		auto& class_ = *AttachA::Interface::getExtractAs<typed_lgr<TaskLimiter>>(args[0], define_TaskLimiter);
		class_.lock();
	})
	AttachAFun(funs_TaskLimiter_unlock, 1, {
		auto& class_ = *AttachA::Interface::getExtractAs<typed_lgr<TaskLimiter>>(args[0], define_TaskLimiter);
		class_.unlock();
	})
	AttachAFun(funs_TaskLimiter_try_lock, 1, {
		auto& class_ = *AttachA::Interface::getExtractAs<typed_lgr<TaskLimiter>>(args[0], define_TaskLimiter);
		if(len == 1)
			return class_.try_lock();
		else
			return class_.try_lock_for((size_t)args[1]);
	})
	AttachAFun(funs_TaskLimiter_try_lock_until, 2, {
		auto& class_ = *AttachA::Interface::getExtractAs<typed_lgr<TaskLimiter>>(args[0], define_TaskLimiter);
		return class_.try_lock_until((std::chrono::high_resolution_clock::time_point)args[1]);
	})
	AttachAFun(funs_TaskLimiter_is_locked, 1, {
		auto& class_ = *AttachA::Interface::getExtractAs<typed_lgr<TaskLimiter>>(args[0], define_TaskLimiter);
		return class_.is_locked();
	})
	void init_TaskLimiter() {
		define_TaskLimiter = AttachA::Interface::createTable<typed_lgr<TaskLimiter>>("task_limiter",
			AttachA::Interface::direct_method("set_max_treeshold", funs_TaskLimiter_set_max_treeshold),
			AttachA::Interface::direct_method("lock", funs_TaskLimiter_lock),
			AttachA::Interface::direct_method("unlock", funs_TaskLimiter_unlock),
			AttachA::Interface::direct_method("try_lock", funs_TaskLimiter_try_lock),
			AttachA::Interface::direct_method("try_lock_until", funs_TaskLimiter_try_lock_until),
			AttachA::Interface::direct_method("is_locked", funs_TaskLimiter_is_locked)
		);
		AttachA::Interface::typeVTable<typed_lgr<TaskLimiter>>() = define_TaskLimiter;
	}
#pragma endregion
#pragma region TaskQuery
	AttachAFun(funs_TaskQuery_add_task, 2, {
		auto& class_ = *AttachA::Interface::getExtractAs<typed_lgr<TaskQuery>>(args[0], define_TaskQuery);
		typed_lgr<FuncEnvironment> func;
		typed_lgr<FuncEnvironment> fault_func;
		std::chrono::high_resolution_clock::time_point timeout = std::chrono::high_resolution_clock::time_point::min();
		bool used_task_local = false;
		ValueItem values;
		parseArgumentsToTask<1>(args, len, func, fault_func, timeout, used_task_local, values);
		return class_.add_task(func, values,used_task_local,fault_func, timeout);
	})
	AttachAFun(funs_TaskQuery_enable, 1, {
		auto& class_ = *AttachA::Interface::getExtractAs<typed_lgr<TaskQuery>>(args[0], define_TaskQuery);
		class_.enable();
	})
	AttachAFun(funs_TaskQuery_disable, 1, {
		auto& class_ = *AttachA::Interface::getExtractAs<typed_lgr<TaskQuery>>(args[0], define_TaskQuery);
		class_.disable();
	})
	AttachAFun(funs_TaskQuery_in_query, 2, {
		auto& class_ = *AttachA::Interface::getExtractAs<typed_lgr<TaskQuery>>(args[0], define_TaskQuery);
		AttachA::excepted(args[1], VType::async_res);
		typed_lgr<Task>& task = *(typed_lgr<Task>*)args[1].getSourcePtr();
		return class_.in_query(task);
	})
	AttachAFun(funs_TaskQuery_set_max_at_execution, 2, {
		auto& class_ = *AttachA::Interface::getExtractAs<typed_lgr<TaskQuery>>(args[0], define_TaskQuery);
		class_.set_max_at_execution((size_t)args[1]);
	})
	AttachAFun(funs_TaskQuery_get_max_at_execution, 1, {
		auto& class_ = *AttachA::Interface::getExtractAs<typed_lgr<TaskQuery>>(args[0], define_TaskQuery);
		return class_.get_max_at_execution();
	})

	void init_TaskQuery() {
		define_TaskQuery = AttachA::Interface::createTable<typed_lgr<TaskQuery>>("task_query",
			AttachA::Interface::direct_method("add_task", funs_TaskQuery_add_task),
			AttachA::Interface::direct_method("enable", funs_TaskQuery_enable),
			AttachA::Interface::direct_method("disable", funs_TaskQuery_disable),
			AttachA::Interface::direct_method("in_query", funs_TaskQuery_in_query),
			AttachA::Interface::direct_method("set_max_at_execution", funs_TaskQuery_set_max_at_execution),
			AttachA::Interface::direct_method("get_max_at_execution", funs_TaskQuery_get_max_at_execution)
		);
		AttachA::Interface::typeVTable<typed_lgr<TaskQuery>>() = define_TaskQuery;
	}
#pragma endregion
#pragma region TaskResultIterator
	struct TaskResultIterator {
		typed_lgr<Task> task;
		ptrdiff_t index = -1;
		bool next(){
			if(task->end_of_life){
				if(task->fres.results.size() > ++index)
					return true;
				
			}else{
				if(Task::has_result(task, 1 + index)){
					++index;
					return true;
				}else{
					ValueItem* res = Task::get_result(task, 1 + index);
					delete res;
					if(Task::has_result(task, 1 + index)){
						++index;
						return true;
					}
				}
			}
			return false;
		}
		bool prev(){
			if(index > 0){
				--index;
				return true;
			}
			return false;
		}
		ValueItem get(){
			return task->fres.results[index];
		}
		TaskResultIterator begin(){
			return TaskResultIterator{task, -1};
		}
		TaskResultIterator end(){
			if(!task->end_of_life)
				Task::await_task(task);
			return TaskResultIterator{task, (ptrdiff_t)task->fres.results.size()};
		}
	};
	AttachAFun(funs_TaskResultIterator_next, 1, {
		auto& class_ = AttachA::Interface::getExtractAs<TaskResultIterator>(args[0], define_TaskResultIterator);
		return class_.next();
	})
	AttachAFun(funs_TaskResultIterator_prev, 1, {
		auto& class_ = AttachA::Interface::getExtractAs<TaskResultIterator>(args[0], define_TaskResultIterator);
		return class_.prev();
	})
	AttachAFun(funs_TaskResultIterator_get, 1, {
		auto& class_ = AttachA::Interface::getExtractAs<TaskResultIterator>(args[0], define_TaskResultIterator);
		return class_.get();
	})
	AttachAFun(funs_TaskResultIterator_begin, 1, {
		auto& class_ = AttachA::Interface::getExtractAs<TaskResultIterator>(args[0], define_TaskResultIterator);
		return ValueItem(AttachA::Interface::constructStructure<TaskResultIterator>(define_TaskResultIterator, class_.begin()), no_copy);
	})
	AttachAFun(funs_TaskResultIterator_end, 1, {
		auto& class_ = AttachA::Interface::getExtractAs<TaskResultIterator>(args[0], define_TaskResultIterator);
		return ValueItem(AttachA::Interface::constructStructure<TaskResultIterator>(define_TaskResultIterator, class_.end()), no_copy);
	})
	void init_TaskResultIterator() {
		define_TaskResultIterator = AttachA::Interface::createTable<TaskResultIterator>("task_result_iterator",
			AttachA::Interface::direct_method(symbols::structures::iterable::next, funs_TaskResultIterator_next),
			AttachA::Interface::direct_method(symbols::structures::iterable::prev, funs_TaskResultIterator_prev),
			AttachA::Interface::direct_method(symbols::structures::iterable::get, funs_TaskResultIterator_get),
			AttachA::Interface::direct_method(symbols::structures::iterable::begin, funs_TaskResultIterator_begin),
			AttachA::Interface::direct_method(symbols::structures::iterable::end, funs_TaskResultIterator_end)
		);
		AttachA::Interface::typeVTable<TaskResultIterator>() = define_TaskResultIterator;
	}
#pragma endregion
#pragma region Task
	AttachAFun(funs_Task_start, 1, {
		Task::start(AttachA::Interface::getExtractAs<typed_lgr<Task>>(args[0], define_Task));
	})
	AttachAFun(funs_Task_yield_iterate, 1, {
		return Task::yield_iterate(AttachA::Interface::getExtractAs<typed_lgr<Task>>(args[0], define_Task));
	})
	AttachAManagedFun(funs_Task_get_result, 1, {
		if(len >= 2)
			return Task::get_result(AttachA::Interface::getExtractAs<typed_lgr<Task>>(args[0], define_Task), (size_t)args[1]);
		else
			return Task::get_result(AttachA::Interface::getExtractAs<typed_lgr<Task>>(args[0], define_Task));
	})
	AttachAFun(funs_Task_has_result, 1, {
		if(len >= 2)
			return Task::has_result(AttachA::Interface::getExtractAs<typed_lgr<Task>>(args[0], define_Task), (size_t)args[1]);
		else
			return Task::has_result(AttachA::Interface::getExtractAs<typed_lgr<Task>>(args[0], define_Task));
	})
	AttachAFun(funs_Task_await_task, 1, {
		if(len >= 2)
			Task::await_task(AttachA::Interface::getExtractAs<typed_lgr<Task>>(args[0], define_Task), (bool)args[1]);
		else
			Task::await_task(AttachA::Interface::getExtractAs<typed_lgr<Task>>(args[0], define_Task));
	})
	AttachAFun(funs_Task_await_results, 1, {
		return Task::await_results(AttachA::Interface::getExtractAs<typed_lgr<Task>>(args[0], define_Task));
	})
	AttachAFun(funs_Task_notify_cancel, 1, {
		Task::notify_cancel(AttachA::Interface::getExtractAs<typed_lgr<Task>>(args[0], define_Task));
	})
	AttachAFun(funs_Task_size, 1, {
		Task& task = *AttachA::Interface::getExtractAs<typed_lgr<Task>>(args[0], define_Task);
		std::lock_guard lock(task.no_race);
		return task.fres.results.size();
	})

	AttachAFun(funs_Task_to_string, 1, {
		auto& task = AttachA::Interface::getExtractAs<typed_lgr<Task>>(args[0], define_Task);
		Task::await_task(task);
		Task& task_ = *task;
		ValueItem pre_res(task_.fres.results, as_refrence);
		return (std::string)pre_res;
	})
	
	AttachAFun(funs_Task_to_set, 1, {
		auto& task = AttachA::Interface::getExtractAs<typed_lgr<Task>>(args[0], define_Task);
		Task::await_task(task);
		std::unordered_set<ValueItem> res;
		for(auto& i : task->fres.results)
			res.insert(i);
		return res;
	})

	template<typename T>
	AttachAFun(funs_Task_to_, 1, {
		auto& task = AttachA::Interface::getExtractAs<typed_lgr<Task>>(args[0], define_Task);
		if(task->fres.results.size() != 1){
			ValueItem* res = Task::get_result(task, 0);
			std::unique_ptr<ValueItem> res_(res);
			return (T)*res;
		}else
			return (T)task->fres.results[0];
	})

	template<typename T>
	AttachAFun(funs_Task_array_to_, 1, {
		auto& task = AttachA::Interface::getExtractAs<typed_lgr<Task>>(args[0], define_Task);
		Task::await_task(task);
		if(task->fres.results.size() > UINT32_MAX)
			throw InvalidCast("Task internal result array is too large to convert to an array");
		else if constexpr(std::is_same_v<T, ValueItem>)
			return ValueItem(task->fres.results.data(), task->fres.results.size());
		else{
			size_t len;
			auto res = task->fres.results.convert<T>([](ValueItem& item){
				return (T)item;
			}).take_raw(len);
			return ValueItem(res, len, no_copy);
		}
	})
	AttachAFun(funs_Task_begin, 1, {
		auto& task = AttachA::Interface::getExtractAs<typed_lgr<Task>>(args[0], define_Task);
		return AttachA::Interface::constructStructure<TaskResultIterator>(define_TaskResultIterator, task, -1);
	})
	AttachAFun(funs_Task_end, 1, {
		auto& task = AttachA::Interface::getExtractAs<typed_lgr<Task>>(args[0], define_Task);
		Task::await_task(task);
		return AttachA::Interface::constructStructure<TaskResultIterator>(define_TaskResultIterator, task, task->fres.results.size());
	})

	void init_Task(){
		define_Task = AttachA::Interface::createTable<typed_lgr<Task>>("task",
			AttachA::Interface::direct_method("start", funs_Task_start),
			AttachA::Interface::direct_method("yield_iterate", funs_Task_yield_iterate),
			AttachA::Interface::direct_method("get_result", funs_Task_get_result),
			AttachA::Interface::direct_method(symbols::structures::index_operator, funs_Task_get_result),
			AttachA::Interface::direct_method(symbols::structures::size, funs_Task_size),
			AttachA::Interface::direct_method("has_result", funs_Task_has_result),
			AttachA::Interface::direct_method("await_task", funs_Task_await_task),
			AttachA::Interface::direct_method("await_results", funs_Task_await_results),
			AttachA::Interface::direct_method(symbols::structures::convert::to_uarr, funs_Task_await_results),
			AttachA::Interface::direct_method(symbols::structures::convert::to_string, funs_Task_to_string),
			AttachA::Interface::direct_method(symbols::structures::convert::to_ui8, funs_Task_to_<uint8_t>),
			AttachA::Interface::direct_method(symbols::structures::convert::to_ui16, funs_Task_to_<uint16_t>),
			AttachA::Interface::direct_method(symbols::structures::convert::to_ui32, funs_Task_to_<uint32_t>),
			AttachA::Interface::direct_method(symbols::structures::convert::to_ui64, funs_Task_to_<uint64_t>),
			AttachA::Interface::direct_method(symbols::structures::convert::to_i8, funs_Task_to_<int8_t>),
			AttachA::Interface::direct_method(symbols::structures::convert::to_i16, funs_Task_to_<int16_t>),
			AttachA::Interface::direct_method(symbols::structures::convert::to_i32, funs_Task_to_<int32_t>),
			AttachA::Interface::direct_method(symbols::structures::convert::to_i64, funs_Task_to_<int64_t>),
			AttachA::Interface::direct_method(symbols::structures::convert::to_float, funs_Task_to_<float>),
			AttachA::Interface::direct_method(symbols::structures::convert::to_double, funs_Task_to_<double>),
			AttachA::Interface::direct_method(symbols::structures::convert::to_boolean, funs_Task_to_<bool>),
			AttachA::Interface::direct_method(symbols::structures::convert::to_timepoint, funs_Task_to_<std::chrono::high_resolution_clock::time_point>),
			AttachA::Interface::direct_method(symbols::structures::convert::to_type_identifier, funs_Task_to_<ValueMeta>),
			AttachA::Interface::direct_method(symbols::structures::convert::to_function, funs_Task_to_<typed_lgr<FuncEnvironment>&>),
			AttachA::Interface::direct_method(symbols::structures::convert::to_map, funs_Task_to_<std::unordered_map<ValueItem,ValueItem>&>),
			AttachA::Interface::direct_method(symbols::structures::convert::to_set, funs_Task_to_set),
			AttachA::Interface::direct_method(symbols::structures::convert::to_ui8_arr, funs_Task_array_to_<uint8_t>),
			AttachA::Interface::direct_method(symbols::structures::convert::to_ui16_arr, funs_Task_array_to_<uint16_t>),
			AttachA::Interface::direct_method(symbols::structures::convert::to_ui32_arr, funs_Task_array_to_<uint32_t>),
			AttachA::Interface::direct_method(symbols::structures::convert::to_ui64_arr, funs_Task_array_to_<uint64_t>),
			AttachA::Interface::direct_method(symbols::structures::convert::to_i8_arr, funs_Task_array_to_<int8_t>),
			AttachA::Interface::direct_method(symbols::structures::convert::to_i16_arr, funs_Task_array_to_<int16_t>),
			AttachA::Interface::direct_method(symbols::structures::convert::to_i32_arr, funs_Task_array_to_<int32_t>),
			AttachA::Interface::direct_method(symbols::structures::convert::to_i64_arr, funs_Task_array_to_<int64_t>),
			AttachA::Interface::direct_method(symbols::structures::convert::to_float_arr, funs_Task_array_to_<float>),
			AttachA::Interface::direct_method(symbols::structures::convert::to_double_arr, funs_Task_array_to_<double>),
			AttachA::Interface::direct_method(symbols::structures::convert::to_farr, funs_Task_array_to_<ValueItem>),
			AttachA::Interface::direct_method(symbols::structures::iterable::begin, funs_Task_begin),
			AttachA::Interface::direct_method(symbols::structures::iterable::end, funs_Task_end),
			AttachA::Interface::direct_method("notify_cancel", funs_Task_notify_cancel)
		);
		AttachA::Interface::typeVTable<typed_lgr<Task>>() = define_Task;
	}
	
#pragma endregion
#pragma region TaskGroup
	AttachAFun(funs_TaskGroup_start, 1, {
		Task::start(AttachA::Interface::getExtractAs<list_array<typed_lgr<Task>>>(args[0], define_TaskGroup));
	})
	AttachAFun(funs_TaskGroup_await_multiple, 1, {
		switch (len) {
		case 1:
			Task::await_multiple(AttachA::Interface::getExtractAs<list_array<typed_lgr<Task>>>(args[0], define_TaskGroup));
			break;
		case 2:
			Task::await_multiple(AttachA::Interface::getExtractAs<list_array<typed_lgr<Task>>>(args[0], define_TaskGroup), (bool)args[1]);
			break;
		case 3:
		default:{
			bool release = (bool)args[2];
			Task::await_multiple(AttachA::Interface::getExtractAs<list_array<typed_lgr<Task>>>(args[0], define_TaskGroup), (bool)args[1], release);
			if(release)
				AttachA::Interface::getExtractAs<list_array<typed_lgr<Task>>>(args[0], define_TaskGroup).clear();
			break;
		}
		}
	})
	AttachAFun(funs_TaskGroup_await_results, 1, {
		return Task::await_results(AttachA::Interface::getExtractAs<list_array<typed_lgr<Task>>>(args[0], define_TaskGroup));
	})
	AttachAFun(funs_TaskGroup_notify_cancel, 1, {
		Task::notify_cancel(AttachA::Interface::getExtractAs<list_array<typed_lgr<Task>>>(args[0], define_TaskGroup));
	})

	template<typename T>
	AttachAFun(funs_TaskGroup_array_to_, 1, {
		auto results = Task::await_results(AttachA::Interface::getExtractAs<list_array<typed_lgr<Task>>>(args[0], define_TaskGroup));
		
		if(results.size() > UINT32_MAX)
			throw InvalidCast("Task internal result array is too large to convert to an array");
		else if constexpr(std::is_same_v<T, ValueItem>)
			return ValueItem(results.data(), results.size());
		else{
			size_t len;
			auto res = results.convert<T>([](ValueItem& item){return (T)item;}).take_raw(len);
			return ValueItem(res, len, no_copy);
		}
	})
	AttachAFun(funs_TaskGroup_to_set, 1, {
		std::unordered_set<ValueItem> res;
		for(auto& i : Task::await_results(AttachA::Interface::getExtractAs<list_array<typed_lgr<Task>>>(args[0], define_TaskGroup)))
			res.insert(i);
		return res;
	})
	void ___createProxy_TaskGroup__push_item(list_array<typed_lgr<Task>>& tasks, ValueItem& item){
		switch(item.meta.vtype){
			case VType::async_res:
				tasks.push_back((typed_lgr<Task>&)item);
				break;
			case VType::struct_:
				{
					Structure& str = (Structure&)item;
					if(str.get_vtable() == define_Task){
						tasks.push_back(AttachA::Interface::getAs<typed_lgr<Task>>(str));
						break;
					}else if (str.get_vtable() == define_TaskGroup){
						tasks.push_back(AttachA::Interface::getAs<list_array<typed_lgr<Task>>>(str));
						break;
					}else
						break;
				}
			case VType::uarr:{
				list_array<ValueItem>& arr = *(list_array<ValueItem>*)item.getSourcePtr();
				for(auto& it : arr)
					___createProxy_TaskGroup__push_item(tasks, it);
				break;
			}
			case VType::faarr:
			case VType::saarr:{
				ValueItem* arr = (ValueItem*)item.getSourcePtr();
				uint32_t len = item.meta.val_len;
				for(uint32_t i = 0; i < len; i++)
					___createProxy_TaskGroup__push_item(tasks, arr[i]);
				break;
			}
			case VType::set:{
				std::unordered_set<ValueItem>& set = (std::unordered_set<ValueItem>&)item;
				for(auto& it : set)
					___createProxy_TaskGroup__push_item(tasks, const_cast<ValueItem&>(it));
				break;
			}
			case VType::map:{
				std::unordered_map<ValueItem, ValueItem>& map = (std::unordered_map<ValueItem, ValueItem>&)item;
				for(auto& it : map)
					___createProxy_TaskGroup__push_item(tasks, it.second);
				break;
			}
			default:
				break;
		}
	}
	AttachAFun(funs_TaskGroup_add, 1, {
		auto& tasks = AttachA::Interface::getExtractAs<list_array<typed_lgr<Task>>>(args[0], define_TaskGroup);
		for(uint32_t i = 0; i < len; i++)
			___createProxy_TaskGroup__push_item(tasks, args[i]);
	})
	void init_TaskGroup() {
		define_TaskGroup = AttachA::Interface::createTable<list_array<typed_lgr<Task>>>("task_group",
			AttachA::Interface::direct_method("start", funs_TaskGroup_start),
			AttachA::Interface::direct_method("await_multiple", funs_TaskGroup_await_multiple),
			AttachA::Interface::direct_method("await_results", funs_TaskGroup_await_results),
			AttachA::Interface::direct_method("notify_cancel", funs_TaskGroup_notify_cancel),
			AttachA::Interface::direct_method(symbols::structures::convert::to_ui8_arr, funs_TaskGroup_array_to_<uint8_t>),
			AttachA::Interface::direct_method(symbols::structures::convert::to_ui16_arr, funs_TaskGroup_array_to_<uint16_t>),
			AttachA::Interface::direct_method(symbols::structures::convert::to_ui32_arr, funs_TaskGroup_array_to_<uint32_t>),
			AttachA::Interface::direct_method(symbols::structures::convert::to_ui64_arr, funs_TaskGroup_array_to_<uint64_t>),
			AttachA::Interface::direct_method(symbols::structures::convert::to_i8_arr, funs_TaskGroup_array_to_<int8_t>),
			AttachA::Interface::direct_method(symbols::structures::convert::to_i16_arr, funs_TaskGroup_array_to_<int16_t>),
			AttachA::Interface::direct_method(symbols::structures::convert::to_i32_arr, funs_TaskGroup_array_to_<int32_t>),
			AttachA::Interface::direct_method(symbols::structures::convert::to_i64_arr, funs_TaskGroup_array_to_<int64_t>),
			AttachA::Interface::direct_method(symbols::structures::convert::to_float_arr, funs_TaskGroup_array_to_<float>),
			AttachA::Interface::direct_method(symbols::structures::convert::to_double_arr, funs_TaskGroup_array_to_<double>),
			AttachA::Interface::direct_method(symbols::structures::convert::to_farr, funs_TaskGroup_array_to_<ValueItem>),
			AttachA::Interface::direct_method(symbols::structures::convert::to_set, funs_TaskGroup_to_set),
			AttachA::Interface::direct_method(symbols::structures::convert::to_uarr, funs_TaskGroup_await_multiple),
			AttachA::Interface::direct_method(symbols::structures::add_operator, funs_TaskGroup_add)
		);
		AttachA::Interface::typeVTable<list_array<typed_lgr<Task>>>() = define_TaskGroup;
	}
#pragma endregion







	namespace constructor {
		ValueItem* createProxy_ConditionVariable(ValueItem*, uint32_t) {
			return new ValueItem(AttachA::Interface::constructStructure<typed_lgr<TaskConditionVariable>>(define_ConditionVariable, new TaskConditionVariable()), no_copy);
		}
		ValueItem* createProxy_Mutex(ValueItem*, uint32_t) {
			return new ValueItem(AttachA::Interface::constructStructure<typed_lgr<TaskMutex>>(define_Mutex, new TaskMutex()), no_copy);
		}
		ValueItem* createProxy_RecursiveMutex(ValueItem*, uint32_t) {
			return new ValueItem(AttachA::Interface::constructStructure<typed_lgr<TaskRecursiveMutex>>(define_RecursiveMutex, new TaskRecursiveMutex()), no_copy);
		}
		ValueItem* createProxy_Semaphore(ValueItem*, uint32_t) {
			return new ValueItem(AttachA::Interface::constructStructure<typed_lgr<TaskSemaphore>>(define_Semaphore, new TaskSemaphore()), no_copy);
		}

		ValueItem* createProxy_EventSystem(ValueItem* val, uint32_t len) {
			return new ValueItem(AttachA::Interface::constructStructure<typed_lgr<EventSystem>>(define_EventSystem, new EventSystem()), no_copy);
		}

		ValueItem* createProxy_TaskLimiter(ValueItem* val, uint32_t len) {
			return new ValueItem(AttachA::Interface::constructStructure<typed_lgr<TaskLimiter>>(define_TaskLimiter, new TaskLimiter()), no_copy);
		}

		ValueItem* createProxy_TaskQuery(ValueItem* val, uint32_t len){
			return new ValueItem(AttachA::Interface::constructStructure<typed_lgr<TaskQuery>>(define_TaskQuery, new TaskQuery(len? (size_t)*val : 0)), no_copy);
		}
		AttachAFun(construct_Task, 1, {
			if(args[0].meta.vtype == VType::async_res)
				return ValueItem(AttachA::Interface::constructStructure<typed_lgr<Task>>(define_Task, (typed_lgr<Task>&)args[0]), no_copy);
			else if(args[0].meta.vtype == VType::function){
				typed_lgr<FuncEnvironment> func;
				typed_lgr<FuncEnvironment> fault_func;
				std::chrono::high_resolution_clock::time_point timeout = std::chrono::high_resolution_clock::time_point::min();
				bool used_task_local = false;
				ValueItem values;
				parseArgumentsToTask<0>(args, len, func, fault_func, timeout, used_task_local, values);
				return ValueItem(AttachA::Interface::constructStructure<typed_lgr<Task>>(define_Task, new Task(func,values, used_task_local,fault_func, timeout)), no_copy);
			}
			else
				return ValueItem(AttachA::Interface::constructStructure<typed_lgr<Task>>(define_Task, AttachA::Interface::getExtractAs<typed_lgr<Task>>(args[0],define_Task)), no_copy);
		})
		ValueItem* createProxy_TaskGroup(ValueItem* val, uint32_t len) {
			if(!len)
				return new ValueItem(AttachA::Interface::constructStructure<list_array<typed_lgr<Task>>>(define_TaskGroup), no_copy);
			else{
				list_array<typed_lgr<Task>> tasks;
				tasks.reserve_push_back(len);
				for(uint32_t i = 0; i < len; i++)
					___createProxy_TaskGroup__push_item(tasks, val[i]);
				return new ValueItem(AttachA::Interface::constructStructure<list_array<typed_lgr<Task>>>(define_TaskGroup, std::move(tasks)), no_copy);
			}
		}
	}
	
	ValueItem* createThread(ValueItem* vals, uint32_t len) {
		AttachA::arguments_range(len, 1);

		if (len != 1) {
			typed_lgr<FuncEnvironment> func = *vals->funPtr();
			ValueItem* copyArgs = new ValueItem[len - 1];
			vals++;
			len--;
			uint32_t i = 0;
			while (len--)
				copyArgs[i++] = *vals++;
			run_time::threading::thread([](typed_lgr<FuncEnvironment> func, ValueItem* args, uint32_t len) {
				auto tmp = FuncEnvironment::sync_call(func, args, len);
				if (tmp)
					delete tmp;
				delete[] args;
			}, func, copyArgs, i).detach();
		}
		else {
			run_time::threading::thread([](typed_lgr<FuncEnvironment> func) {
				auto tmp = FuncEnvironment::sync_call(func, nullptr, 0);
				if (tmp)
					delete tmp;
			}, *vals->funPtr()).detach();
		}
		return nullptr;
	}
	ValueItem* createThreadAndWait(ValueItem* vals, uint32_t len) {
		AttachA::arguments_range(len, 1);

		TaskConditionVariable cv;
		TaskMutex mtx;
		MutexUnify unif(mtx);
		std::unique_lock ul(unif);
		bool end = false;
		ValueItem* res = nullptr;
		typed_lgr<FuncEnvironment> func = *vals->funPtr();
		if (len != 1) {
			run_time::threading::thread([&end, &res, &mtx, &cv](typed_lgr<FuncEnvironment> func, ValueItem* args, uint32_t len) {
				try{
					auto tmp = FuncEnvironment::sync_call(func, args, len);
					std::unique_lock ul(mtx);
					res = tmp;
					end = true;
					cv.notify_all();
				}catch(...){
					std::unique_lock ul(mtx);
					try{
						res = new ValueItem(std::current_exception());
					}catch(...){
						end = true;
						cv.notify_all();
					}
					end = true;
					cv.notify_all();
				}
			}, func, vals, len).detach();

		}
		else {
			run_time::threading::thread([&end, &res, &mtx, &cv](typed_lgr<FuncEnvironment> func) {
				try{
					auto tmp = FuncEnvironment::sync_call(func, nullptr, 0);
					std::unique_lock ul(mtx);
					res = tmp;
					end = true;
					cv.notify_all();
				}catch(...){
					std::unique_lock ul(mtx);
					try{
						res = new ValueItem(std::current_exception());
					}catch(...){
						end = true;
						cv.notify_all();
					}
					end = true;
					cv.notify_all();
				}
			}, func).detach();
		}
		while(!end)
			cv.wait(ul);
		return res;
	}

	struct _createAsyncThread_awaiter_struct {
		TaskConditionVariable cv;
		TaskMutex mtx;
		MutexUnify unif;
		bool end = false;
		ValueItem* res = nullptr;
		_createAsyncThread_awaiter_struct(){
			unif = MutexUnify(mtx);
		}
		~_createAsyncThread_awaiter_struct(){
			if(!end){
				std::unique_lock ul(mtx);
				end = true;
				cv.notify_all();
			}
		}
	};
	
	ValueItem* _createAsyncThread__Awaiter(ValueItem* val, uint32_t len){
		_createAsyncThread_awaiter_struct* awaiter = (_createAsyncThread_awaiter_struct*)val->getSourcePtr();
		std::unique_lock<MutexUnify> ul(awaiter->unif);
		try{
			while(!awaiter->end)
				awaiter->cv.wait(ul);
		}catch(...){
			ul.unlock();
			delete awaiter;
			throw;
		}
		ul.unlock();
		delete awaiter;
		return awaiter->res;
	}
	typed_lgr<FuncEnvironment> __createAsyncThread__Awaiter = new FuncEnvironment(_createAsyncThread__Awaiter, false);


	ValueItem* createAsyncThread(ValueItem* vals, uint32_t len){
		AttachA::arguments_range(len, 1);
		typed_lgr<FuncEnvironment> func = *vals->funPtr();
		_createAsyncThread_awaiter_struct* awaiter = nullptr;
		try{
			awaiter = new _createAsyncThread_awaiter_struct();

			if (len != 1) {
				ValueItem* copyArgs = new ValueItem[len - 1];
				vals++;
				len--;
				uint32_t i = 0;
				while (len--)
					copyArgs[i++] = *vals++;

				run_time::threading::thread([awaiter](typed_lgr<FuncEnvironment> func, ValueItem* args, uint32_t len) {
					try{
						auto tmp = FuncEnvironment::sync_call(func, args, len);
						std::unique_lock ul(awaiter->mtx);
						awaiter->res = tmp;
						awaiter->end = true;
						awaiter->cv.notify_all();
					}catch(...){
						std::unique_lock ul(awaiter->mtx);
						try{
							awaiter->res = new ValueItem(std::current_exception());
						}catch(...){
							awaiter->end = true;
							awaiter->cv.notify_all();
						}
						awaiter->end = true;
						awaiter->cv.notify_all();
					}
				}, func, vals, len).detach();

			}
			else {
				run_time::threading::thread([awaiter](typed_lgr<FuncEnvironment> func) {
					try{
						auto tmp = FuncEnvironment::sync_call(func, nullptr, 0);
						std::unique_lock ul(awaiter->mtx);
						awaiter->res = tmp;
						awaiter->end = true;
						awaiter->cv.notify_all();
					}catch(...){
						std::unique_lock ul(awaiter->mtx);
						try{
							awaiter->res = new ValueItem(std::current_exception());
						}catch(...){
							awaiter->end = true;
							awaiter->cv.notify_all();
						}
						awaiter->end = true;
						awaiter->cv.notify_all();
					}
				}, func).detach();
			}
		}
		catch(...) {
			if(awaiter)
				delete awaiter;
			throw;
		}
		ValueItem awaiter_args(awaiter);
		return new ValueItem(new typed_lgr(new Task(__createAsyncThread__Awaiter, awaiter_args)), VType::async_res, no_copy);
	}

	ValueItem* createTask(ValueItem* vals, uint32_t len){
		AttachA::arguments_range(len, 1);
		typed_lgr<FuncEnvironment> func;
		typed_lgr<FuncEnvironment> fault_func;
		std::chrono::steady_clock::time_point timeout;
		bool used_task_local;
		ValueItem args;
		parseArgumentsToTask<0>(vals, len, func,fault_func, timeout, used_task_local, args);
		return new ValueItem(new typed_lgr(new Task(func, args, used_task_local, fault_func, timeout)), VType::async_res, no_copy);
	}


	
	namespace this_task{
		ValueItem* yield(ValueItem*, uint32_t){
			Task::yield();
			return nullptr;
		}
		ValueItem* yield_result(ValueItem* result, uint32_t args_len){
			for(uint32_t i = 0; i < args_len; i++)
				Task::result(new ValueItem(result[i]));
			return nullptr;
		}
		ValueItem* sleep(ValueItem* args, uint32_t len){
			Task::sleep((size_t)args[0]);
			return nullptr;

		}
		ValueItem* sleep_until(ValueItem* args, uint32_t len){
			Task::sleep_until((std::chrono::high_resolution_clock::time_point)args[0]);
			return nullptr;
		}
		ValueItem* task_id(ValueItem*, uint32_t){
			return new ValueItem(Task::task_id());
		}
		ValueItem* check_cancelation(ValueItem*, uint32_t){
			Task::check_cancelation();
			return nullptr;
		}
		ValueItem* self_cancel(ValueItem*, uint32_t){
			Task::self_cancel();
			return nullptr;
		}
		ValueItem* is_task(ValueItem*, uint32_t){
			return new ValueItem(Task::is_task());
		}
	}
	namespace task_runtime{
		ValueItem* clean_up(ValueItem*, uint32_t){
			Task::clean_up();
			return nullptr;
		}
		ValueItem* create_executor(ValueItem* args, uint32_t len){
			Task::create_executor(len ? (size_t)args[0] : 1);
			return nullptr;
		}
		ValueItem* total_executors(ValueItem*, uint32_t){
			return new ValueItem(Task::total_executors());
		}
		ValueItem* reduce_executor(ValueItem* args, uint32_t len){
			Task::reduce_executor(len ? (size_t)args[0] : 1);
			return nullptr;
		}
		ValueItem* become_task_executor(ValueItem*, uint32_t){
			Task::become_task_executor();
			return nullptr;
		}
		ValueItem* await_no_tasks(ValueItem* args, uint32_t len){
			Task::await_no_tasks(len ? (bool)args[0] : false);
			return nullptr;
		}
		ValueItem* await_end_tasks(ValueItem* args, uint32_t len){
			Task::await_end_tasks(len ? (bool)args[0] : false);
			return nullptr;
		}
		ValueItem* explicitStartTimer(ValueItem*, uint32_t){
			Task::explicitStartTimer();
			return nullptr;
		}
	}

	namespace atomic{

		template<typename T>
		class AtomicBasic : std::atomic<T>{
		public:
			static inline AttachAVirtualTable* virtual_table = nullptr;
			AtomicBasic(T val) : std::atomic<T>(val){}
			AtomicBasic() : std::atomic<T>(){}
			AtomicBasic(const AtomicBasic& other) : std::atomic<T>(other.load()){}
			AtomicBasic& operator=(const AtomicBasic& other){
				this->store(other.load());
				return *this;
			}


			static AttachAFun(__add,2,{
				if constexpr ((std::is_integral_v<T> || std::is_floating_point_v<T>) && !std::is_same_v<T,bool>){
					auto& self = AttachA::Interface::getExtractAs<AtomicBasic<T>>(args[0], virtual_table);
					self += (T)args[1];
				}
			})
			static AttachAFun(__sub,2,{
				if constexpr ((std::is_integral_v<T> || std::is_floating_point_v<T>) && !std::is_same_v<T,bool>){
					auto& self = AttachA::Interface::getExtractAs<AtomicBasic<T>>(args[0], virtual_table);
					self -= (T)args[1];
				}
			})
			static AttachAFun(__and,2,{
				if constexpr (std::is_integral_v<T> && !std::is_floating_point_v<T> && !std::is_same_v<T,bool>){
					auto& self = AttachA::Interface::getExtractAs<AtomicBasic<T>>(args[0], virtual_table);
					self &= (T)args[1];
				}
			})
			static AttachAFun(__or,2,{
				if constexpr (std::is_integral_v<T> && !std::is_floating_point_v<T> && !std::is_same_v<T,bool>){
					auto& self = AttachA::Interface::getExtractAs<AtomicBasic<T>>(args[0], virtual_table);
					self |= (T)args[1];
				}
			})
			static AttachAFun(__xor,2,{
				if constexpr (std::is_integral_v<T> && !std::is_floating_point_v<T> && !std::is_same_v<T,bool>){
					auto& self = AttachA::Interface::getExtractAs<AtomicBasic<T>>(args[0], virtual_table);
					self ^= (T)args[1];
				}
			})

			static AttachAFun(__inc,1,{
				if constexpr ((std::is_integral_v<T> || std::is_floating_point_v<T>) && !std::is_same_v<T,bool>){
					auto& self = AttachA::Interface::getExtractAs<AtomicBasic<T>>(args[0], virtual_table);
					self+=1;
				}
			})
			static AttachAFun(__dec,1,{
				if constexpr ((std::is_integral_v<T> || std::is_floating_point_v<T>) && !std::is_same_v<T,bool>){
					auto& self = AttachA::Interface::getExtractAs<AtomicBasic<T>>(args[0], virtual_table);
					self-=1;
				}
			})

			static AttachAFun(__not_equal,2,{
				auto& self = AttachA::Interface::getExtractAs<AtomicBasic<T>>(args[0], virtual_table);
				return self.load() != (T)args[1];
			})
			static AttachAFun(__equal,2,{
				auto& self = AttachA::Interface::getExtractAs<AtomicBasic<T>>(args[0], virtual_table);
				return self.load() == (T)args[1];
			})
			static AttachAFun(__less,2,{
				auto& self = AttachA::Interface::getExtractAs<AtomicBasic<T>>(args[0], virtual_table);
				return self.load() < (T)args[1];
			})
			static AttachAFun(__greater,2,{
				auto& self = AttachA::Interface::getExtractAs<AtomicBasic<T>>(args[0], virtual_table);
				return self.load() > (T)args[1];
			})
			static AttachAFun(__less_or_equal,2,{
				auto& self = AttachA::Interface::getExtractAs<AtomicBasic<T>>(args[0], virtual_table);
				return self.load() <= (T)args[1];
			})
			static AttachAFun(__greater_or_equal,2,{
				auto& self = AttachA::Interface::getExtractAs<AtomicBasic<T>>(args[0], virtual_table);
				return self.load() >= (T)args[1];
			})

			static AttachAFun(__not,1,{
				auto& self = AttachA::Interface::getExtractAs<AtomicBasic<T>>(args[0], virtual_table);
				return !self.load();
			})
			static AttachAFun(__bitwise_not,1,{
				if constexpr (!std::is_floating_point_v<T>){
					auto& self = AttachA::Interface::getExtractAs<AtomicBasic<T>>(args[0], virtual_table);
					return ~self.load();
				}
			})
			static AttachAFun(__to_string, 1, {
				auto& self = AttachA::Interface::getExtractAs<AtomicBasic<T>>(args[0], virtual_table);
				return std::to_string(self.load());
			})
			static AttachAFun(__to_ui8, 1, {
				auto& self = AttachA::Interface::getExtractAs<AtomicBasic<T>>(args[0], virtual_table);
				return (uint8_t)self.load();
			})
			static AttachAFun(__to_ui16, 1, {
				auto& self = AttachA::Interface::getExtractAs<AtomicBasic<T>>(args[0], virtual_table);
				return (uint16_t)self.load();
			})
			static AttachAFun(__to_ui32, 1, {
				auto& self = AttachA::Interface::getExtractAs<AtomicBasic<T>>(args[0], virtual_table);
				return (uint32_t)self.load();
			})
			static AttachAFun(__to_ui64, 1, {
				auto& self = AttachA::Interface::getExtractAs<AtomicBasic<T>>(args[0], virtual_table);
				return (uint64_t)self.load();
			})
			static AttachAFun(__to_i8, 1, {
				auto& self = AttachA::Interface::getExtractAs<AtomicBasic<T>>(args[0], virtual_table);
				return (int8_t)self.load();
			})
			static AttachAFun(__to_i16, 1, {
				auto& self = AttachA::Interface::getExtractAs<AtomicBasic<T>>(args[0], virtual_table);
				return (int16_t)self.load();
			})
			static AttachAFun(__to_i32, 1, {
				auto& self = AttachA::Interface::getExtractAs<AtomicBasic<T>>(args[0], virtual_table);
				return (int32_t)self.load();
			})
			static AttachAFun(__to_i64, 1, {
				auto& self = AttachA::Interface::getExtractAs<AtomicBasic<T>>(args[0], virtual_table);
				return (int64_t)self.load();
			})
			static AttachAFun(__to_float, 1, {
				auto& self = AttachA::Interface::getExtractAs<AtomicBasic<T>>(args[0], virtual_table);
				return (float)self.load();
			})
			static AttachAFun(__to_double, 1, {
				auto& self = AttachA::Interface::getExtractAs<AtomicBasic<T>>(args[0], virtual_table);
				return (double)self.load();
			})
			static AttachAFun(__to_boolean, 1, {
				auto& self = AttachA::Interface::getExtractAs<AtomicBasic<T>>(args[0], virtual_table);
				return (bool)self.load();
			})
			static AttachAFun(__to_timepoint, 1, {
				auto& self = AttachA::Interface::getExtractAs<AtomicBasic<T>>(args[0], virtual_table);
				return std::chrono::steady_clock::time_point(
					std::chrono::duration_cast<std::chrono::steady_clock::duration>(
						std::chrono::duration<T>(self.load())
					)
				);
			})
			static AttachAFun(__to_type_identifier, 0, {
				return Type_as_ValueMeta<T>();
			})

			static AttachAFun(__get, 1, {
				auto& self = AttachA::Interface::getExtractAs<AtomicBasic<T>>(args[0], virtual_table);
				return self.load();
			})
			static AttachAFun(__set, 2, {
				auto& self = AttachA::Interface::getExtractAs<AtomicBasic<T>>(args[0], virtual_table);
				self.store((T)args[1]);
				return args[1];
			})

			static void init(){
				if constexpr (std::is_integral_v<T> && !std::is_floating_point_v<T> && !std::is_same_v<T,bool>){
					std::string type_name;
					if constexpr (std::is_same_v<T, int8_t>) type_name = "atomic_i8";
					else if constexpr(std::is_same_v<T,int16_t>) type_name = "atomic_i16";
					else if constexpr(std::is_same_v<T,int32_t>) type_name = "atomic_i32";
					else if constexpr(std::is_same_v<T,int64_t>) type_name = "atomic_i64";
					else if constexpr(std::is_same_v<T,uint8_t>) type_name = "atomic_ui8";
					else if constexpr(std::is_same_v<T,uint16_t>) type_name = "atomic_ui16";
					else if constexpr(std::is_same_v<T,uint32_t>) type_name = "atomic_ui32";
					else if constexpr(std::is_same_v<T,uint64_t>) type_name = "atomic_ui64";
					else type_name = "atomic_x";
					virtual_table = AttachA::Interface::createTable<AtomicBasic<T>>(type_name,
						AttachA::Interface::direct_method(symbols::structures::add_operator, __add),
						AttachA::Interface::direct_method(symbols::structures::subtract_operator, __sub),
						AttachA::Interface::direct_method(symbols::structures::bitwise_and_operator, __and),
						AttachA::Interface::direct_method(symbols::structures::bitwise_or_operator, __or),
						AttachA::Interface::direct_method(symbols::structures::bitwise_xor_operator, __xor),
						AttachA::Interface::direct_method(symbols::structures::increment_operator, __inc),
						AttachA::Interface::direct_method(symbols::structures::decrement_operator, __dec),
						AttachA::Interface::direct_method(symbols::structures::not_equal_operator, __not_equal),
						AttachA::Interface::direct_method(symbols::structures::equal_operator, __equal),
						AttachA::Interface::direct_method(symbols::structures::less_operator, __less),
						AttachA::Interface::direct_method(symbols::structures::greater_operator, __greater),
						AttachA::Interface::direct_method(symbols::structures::less_or_equal_operator, __less_or_equal),
						AttachA::Interface::direct_method(symbols::structures::greater_or_equal_operator, __greater_or_equal),
						AttachA::Interface::direct_method(symbols::structures::not_operator, __not),
						AttachA::Interface::direct_method(symbols::structures::bitwise_not_operator, __bitwise_not),
						AttachA::Interface::direct_method(symbols::structures::convert::to_string, __to_string),
						AttachA::Interface::direct_method(symbols::structures::convert::to_ui8, __to_ui8),
						AttachA::Interface::direct_method(symbols::structures::convert::to_ui16, __to_ui16),
						AttachA::Interface::direct_method(symbols::structures::convert::to_ui32, __to_ui32),
						AttachA::Interface::direct_method(symbols::structures::convert::to_ui64, __to_ui64),
						AttachA::Interface::direct_method(symbols::structures::convert::to_i8, __to_i8),
						AttachA::Interface::direct_method(symbols::structures::convert::to_i16, __to_i16),
						AttachA::Interface::direct_method(symbols::structures::convert::to_i32, __to_i32),
						AttachA::Interface::direct_method(symbols::structures::convert::to_i64, __to_i64),
						AttachA::Interface::direct_method(symbols::structures::convert::to_float, __to_float),
						AttachA::Interface::direct_method(symbols::structures::convert::to_double, __to_double),
						AttachA::Interface::direct_method(symbols::structures::convert::to_boolean, __to_boolean),
						AttachA::Interface::direct_method(symbols::structures::convert::to_timepoint, __to_timepoint),
						AttachA::Interface::direct_method(symbols::structures::convert::to_type_identifier, __to_type_identifier),
						AttachA::Interface::direct_method("get", __get),
						AttachA::Interface::direct_method("set", __set)
					);
				}
				else if constexpr (std::is_floating_point_v<T>){
					virtual_table = AttachA::Interface::createTable<AtomicBasic<T>>(std::is_same_v<T, float> ? "atomic_float" : "atomic_double",
						AttachA::Interface::direct_method(symbols::structures::add_operator, __add),
						AttachA::Interface::direct_method(symbols::structures::subtract_operator, __sub),
						AttachA::Interface::direct_method(symbols::structures::increment_operator, __inc),
						AttachA::Interface::direct_method(symbols::structures::decrement_operator, __dec),
						AttachA::Interface::direct_method(symbols::structures::not_equal_operator, __not_equal),
						AttachA::Interface::direct_method(symbols::structures::equal_operator, __equal),
						AttachA::Interface::direct_method(symbols::structures::less_operator, __less),
						AttachA::Interface::direct_method(symbols::structures::greater_operator, __greater),
						AttachA::Interface::direct_method(symbols::structures::less_or_equal_operator, __less_or_equal),
						AttachA::Interface::direct_method(symbols::structures::greater_or_equal_operator, __greater_or_equal),
						AttachA::Interface::direct_method(symbols::structures::not_operator, __not),
						AttachA::Interface::direct_method(symbols::structures::convert::to_string, __to_string),
						AttachA::Interface::direct_method(symbols::structures::convert::to_ui8, __to_ui8),
						AttachA::Interface::direct_method(symbols::structures::convert::to_ui16, __to_ui16),
						AttachA::Interface::direct_method(symbols::structures::convert::to_ui32, __to_ui32),
						AttachA::Interface::direct_method(symbols::structures::convert::to_ui64, __to_ui64),
						AttachA::Interface::direct_method(symbols::structures::convert::to_i8, __to_i8),
						AttachA::Interface::direct_method(symbols::structures::convert::to_i16, __to_i16),
						AttachA::Interface::direct_method(symbols::structures::convert::to_i32, __to_i32),
						AttachA::Interface::direct_method(symbols::structures::convert::to_i64, __to_i64),
						AttachA::Interface::direct_method(symbols::structures::convert::to_float, __to_float),
						AttachA::Interface::direct_method(symbols::structures::convert::to_double, __to_double),
						AttachA::Interface::direct_method(symbols::structures::convert::to_boolean, __to_boolean),
						AttachA::Interface::direct_method(symbols::structures::convert::to_timepoint, __to_timepoint),
						AttachA::Interface::direct_method(symbols::structures::convert::to_type_identifier, __to_type_identifier),
						AttachA::Interface::direct_method("get", __get),
						AttachA::Interface::direct_method("set", __set)
					);
				}else{
					virtual_table = AttachA::Interface::createTable<AtomicBasic<T>>("atomic_boolean",
						AttachA::Interface::direct_method(symbols::structures::not_equal_operator, __not_equal),
						AttachA::Interface::direct_method(symbols::structures::equal_operator, __equal),
						AttachA::Interface::direct_method(symbols::structures::not_operator, __not),
						AttachA::Interface::direct_method(symbols::structures::bitwise_not_operator, __bitwise_not),
						AttachA::Interface::direct_method(symbols::structures::convert::to_string, __to_string),
						AttachA::Interface::direct_method(symbols::structures::convert::to_ui8, __to_ui8),
						AttachA::Interface::direct_method(symbols::structures::convert::to_ui16, __to_ui16),
						AttachA::Interface::direct_method(symbols::structures::convert::to_ui32, __to_ui32),
						AttachA::Interface::direct_method(symbols::structures::convert::to_ui64, __to_ui64),
						AttachA::Interface::direct_method(symbols::structures::convert::to_i8, __to_i8),
						AttachA::Interface::direct_method(symbols::structures::convert::to_i16, __to_i16),
						AttachA::Interface::direct_method(symbols::structures::convert::to_i32, __to_i32),
						AttachA::Interface::direct_method(symbols::structures::convert::to_i64, __to_i64),
						AttachA::Interface::direct_method(symbols::structures::convert::to_float, __to_float),
						AttachA::Interface::direct_method(symbols::structures::convert::to_double, __to_double),
						AttachA::Interface::direct_method(symbols::structures::convert::to_boolean, __to_boolean),
						AttachA::Interface::direct_method(symbols::structures::convert::to_timepoint, __to_timepoint),
						AttachA::Interface::direct_method(symbols::structures::convert::to_type_identifier, __to_type_identifier),
						AttachA::Interface::direct_method("get", __get),
						AttachA::Interface::direct_method("set", __set)
					);
				}
				AttachA::Interface::typeVTable<AtomicBasic<T>>() = virtual_table;
			}
		};
		

		struct AtomicObject{
			static inline AttachAVirtualTable* virtual_table = nullptr;
			ValueItem value;
			TaskRecursiveMutex mutex;
			AtomicObject(const ValueItem& v) { *this = v;}
			AtomicObject(ValueItem&& v) { *this = std::move(v);}
			AtomicObject(const AtomicObject& other) { *this = other; }
			AtomicObject(AtomicObject&& other) { *this = std::move(other); }
			AtomicObject& operator=(const AtomicObject& other){
				std::unique_lock<TaskRecursiveMutex> olock(const_cast<AtomicObject&>(other).mutex);
				ValueItem get(other.value);
				olock.unlock();
				std::lock_guard<TaskRecursiveMutex> lock(mutex);
				value = std::move(get);
				return *this;
			}
			AtomicObject& operator=(AtomicObject&& other){
				std::unique_lock<TaskRecursiveMutex> olock(other.mutex);
				ValueItem get(std::move(other.value));
				olock.unlock();
				std::lock_guard<TaskRecursiveMutex> lock(mutex);
				value = std::move(get);
				return *this;
			}
			AtomicObject& operator=(const ValueItem& other){
				std::lock_guard<TaskRecursiveMutex> lock(mutex);
				value = other;
				return *this;
			}
			AtomicObject& operator=(ValueItem&& other){
				std::lock_guard<TaskRecursiveMutex> lock(mutex);
				value = std::move(other);
				return *this;
			}


	
			static AttachAFun(__less, 2,{
				auto& self = AttachA::Interface::getExtractAs<AtomicObject>(args[0], virtual_table);
				std::lock_guard<TaskRecursiveMutex> lock(self.mutex);
				auto& cmp = args[1];
				return self.value < cmp;
			})
			static AttachAFun(__greater, 2,{
				auto& self = AttachA::Interface::getExtractAs<AtomicObject>(args[0], virtual_table);
				std::lock_guard<TaskRecursiveMutex> lock(self.mutex);
				auto& cmp = args[1];
				return self.value > cmp;
			})
			static AttachAFun(__equal, 2,{
				auto& self = AttachA::Interface::getExtractAs<AtomicObject>(args[0], virtual_table);
				std::lock_guard<TaskRecursiveMutex> lock(self.mutex);
				auto& cmp = args[1];
				return self.value == cmp;
			})
			static AttachAFun(__not_equal, 2,{
				auto& self = AttachA::Interface::getExtractAs<AtomicObject>(args[0], virtual_table);
				std::lock_guard<TaskRecursiveMutex> lock(self.mutex);
				auto& cmp = args[1];
				return self.value != cmp;
			})
			static AttachAFun(__greater_equal, 2,{
				auto& self = AttachA::Interface::getExtractAs<AtomicObject>(args[0], virtual_table);
				std::lock_guard<TaskRecursiveMutex> lock(self.mutex);
				auto& cmp = args[1];
				return self.value >= cmp;
			})
			static AttachAFun(__less_equal, 2,{
				auto& self = AttachA::Interface::getExtractAs<AtomicObject>(args[0], virtual_table);
				std::lock_guard<TaskRecursiveMutex> lock(self.mutex);
				auto& cmp = args[1];
				return self.value <= cmp;
			})

			static AttachAFun(__add, 2,{
				auto& self = AttachA::Interface::getExtractAs<AtomicObject>(args[0], virtual_table);
				std::lock_guard<TaskRecursiveMutex> lock(self.mutex);
				auto& set = args[1];
				self.value += set;
			})
			static AttachAFun(__sub, 2,{
				auto& self = AttachA::Interface::getExtractAs<AtomicObject>(args[0], virtual_table);
				std::lock_guard<TaskRecursiveMutex> lock(self.mutex);
				auto& set = args[1];
				self.value -= set;
			})
			static AttachAFun(__mul, 2,{
				auto& self = AttachA::Interface::getExtractAs<AtomicObject>(args[0], virtual_table);
				std::lock_guard<TaskRecursiveMutex> lock(self.mutex);
				auto& set = args[1];
				self.value *= set;
			})
			static AttachAFun(__div, 2,{
				auto& self = AttachA::Interface::getExtractAs<AtomicObject>(args[0], virtual_table);
				std::lock_guard<TaskRecursiveMutex> lock(self.mutex);
				auto& set = args[1];
				self.value /= set;
			})
			static AttachAFun(__mod, 2,{
				auto& self = AttachA::Interface::getExtractAs<AtomicObject>(args[0], virtual_table);
				auto& set = args[1];
				std::lock_guard<TaskRecursiveMutex> lock(self.mutex);
				self.value %= set;
			})
			static AttachAFun(__xor, 2,{
				auto& self = AttachA::Interface::getExtractAs<AtomicObject>(args[0], virtual_table);
				std::lock_guard<TaskRecursiveMutex> lock(self.mutex);
				auto& set = args[1];
				self.value ^= set;
			})
			static AttachAFun(__and, 2,{
				auto& self = AttachA::Interface::getExtractAs<AtomicObject>(args[0], virtual_table);
				auto& set = args[1];
				std::lock_guard<TaskRecursiveMutex> lock(self.mutex);
				self.value &= set;
			})
			static AttachAFun(__or, 2,{
				auto& self = AttachA::Interface::getExtractAs<AtomicObject>(args[0], virtual_table);
				auto& set = args[1];
				std::lock_guard<TaskRecursiveMutex> lock(self.mutex);
				self.value |= set;
			})
			static AttachAFun(__lshift, 2,{
				auto& self = AttachA::Interface::getExtractAs<AtomicObject>(args[0], virtual_table);
				auto& set = args[1];
				std::lock_guard<TaskRecursiveMutex> lock(self.mutex);
				self.value <<= set;
			})
			static AttachAFun(__rshift, 2,{
				auto& self = AttachA::Interface::getExtractAs<AtomicObject>(args[0], virtual_table);
				auto& set = args[1];
				std::lock_guard<TaskRecursiveMutex> lock(self.mutex);
				self.value >>= set;
			})
			static AttachAFun(__inc, 1,{
				auto& self = AttachA::Interface::getExtractAs<AtomicObject>(args[0], virtual_table);
				std::lock_guard<TaskRecursiveMutex> lock(self.mutex);
				++self.value;
			})
			static AttachAFun(__dec, 1,{
				auto& self = AttachA::Interface::getExtractAs<AtomicObject>(args[0], virtual_table);
				std::lock_guard<TaskRecursiveMutex> lock(self.mutex);
				--self.value;
			})
			static AttachAFun(__not, 1,{
				auto& self = AttachA::Interface::getExtractAs<AtomicObject>(args[0], virtual_table);
				std::lock_guard<TaskRecursiveMutex> lock(self.mutex);
				return !self.value;
			})

			static AttachAFun(__to_boolean, 1,{
				auto& self = AttachA::Interface::getExtractAs<AtomicObject>(args[0], virtual_table);
				std::lock_guard<TaskRecursiveMutex> lock(self.mutex);
				return (bool)self.value;
			})
			static AttachAFun(__to_i8, 1,{
				auto& self = AttachA::Interface::getExtractAs<AtomicObject>(args[0], virtual_table);
				std::lock_guard<TaskRecursiveMutex> lock(self.mutex);
				return (int8_t)self.value;
			})
			static AttachAFun(__to_i16, 1,{
				auto& self = AttachA::Interface::getExtractAs<AtomicObject>(args[0], virtual_table);
				std::lock_guard<TaskRecursiveMutex> lock(self.mutex);
				return (int16_t)self.value;
			})
			static AttachAFun(__to_i32, 1,{
				auto& self = AttachA::Interface::getExtractAs<AtomicObject>(args[0], virtual_table);
				std::lock_guard<TaskRecursiveMutex> lock(self.mutex);
				return (int32_t)self.value;
			})
			static AttachAFun(__to_i64, 1,{
				auto& self = AttachA::Interface::getExtractAs<AtomicObject>(args[0], virtual_table);
				std::lock_guard<TaskRecursiveMutex> lock(self.mutex);
				return (int64_t)self.value;
			})
			static AttachAFun(__to_u8, 1,{
				auto& self = AttachA::Interface::getExtractAs<AtomicObject>(args[0], virtual_table);
				std::lock_guard<TaskRecursiveMutex> lock(self.mutex);
				return (uint8_t)self.value;
			})
			static AttachAFun(__to_u16, 1,{
				auto& self = AttachA::Interface::getExtractAs<AtomicObject>(args[0], virtual_table);
				std::lock_guard<TaskRecursiveMutex> lock(self.mutex);
				return (uint16_t)self.value;
			})
			static AttachAFun(__to_u32, 1,{
				auto& self = AttachA::Interface::getExtractAs<AtomicObject>(args[0], virtual_table);
				std::lock_guard<TaskRecursiveMutex> lock(self.mutex);
				return (uint32_t)self.value;
			})
			static AttachAFun(__to_u64, 1,{
				auto& self = AttachA::Interface::getExtractAs<AtomicObject>(args[0], virtual_table);
				std::lock_guard<TaskRecursiveMutex> lock(self.mutex);
				return (uint64_t)self.value;
			})
			static AttachAFun(__to_float, 1,{
				auto& self = AttachA::Interface::getExtractAs<AtomicObject>(args[0], virtual_table);
				std::lock_guard<TaskRecursiveMutex> lock(self.mutex);
				return (float)self.value;
			})
			static AttachAFun(__to_double, 1,{
				auto& self = AttachA::Interface::getExtractAs<AtomicObject>(args[0], virtual_table);
				std::lock_guard<TaskRecursiveMutex> lock(self.mutex);
				return (double)self.value;
			})
			static AttachAFun(__to_i8_arr, 1,{
				auto& self = AttachA::Interface::getExtractAs<AtomicObject>(args[0], virtual_table);
				std::lock_guard<TaskRecursiveMutex> lock(self.mutex);
				int8_t* arr = ABI_IMPL::Vcast<int8_t*>(self.value.val, self.value.meta);
				return ValueItem(arr, self.value.meta.val_len, no_copy);
			})
			static AttachAFun(__to_i16_arr, 1, {
				auto& self = AttachA::Interface::getExtractAs<AtomicObject>(args[0], virtual_table);
				std::lock_guard<TaskRecursiveMutex> lock(self.mutex);				
				int16_t* arr = ABI_IMPL::Vcast<int16_t*>(self.value.val, self.value.meta);
				return ValueItem(arr, self.value.meta.val_len, no_copy);
			})
			static AttachAFun(__to_i32_arr, 1, {
				auto& self = AttachA::Interface::getExtractAs<AtomicObject>(args[0], virtual_table);
				std::lock_guard<TaskRecursiveMutex> lock(self.mutex);				
				int32_t* arr = ABI_IMPL::Vcast<int32_t*>(self.value.val, self.value.meta);
				return ValueItem(arr, self.value.meta.val_len, no_copy);
			})
			static AttachAFun(__to_i64_arr, 1, {
				auto& self = AttachA::Interface::getExtractAs<AtomicObject>(args[0], virtual_table);
				std::lock_guard<TaskRecursiveMutex> lock(self.mutex);				
				int64_t* arr = ABI_IMPL::Vcast<int64_t*>(self.value.val, self.value.meta);
				return ValueItem(arr, self.value.meta.val_len, no_copy);
			})
			static AttachAFun(__to_ui8_arr, 1, {
				auto& self = AttachA::Interface::getExtractAs<AtomicObject>(args[0], virtual_table);
				std::lock_guard<TaskRecursiveMutex> lock(self.mutex);				
				uint8_t* arr = ABI_IMPL::Vcast<uint8_t*>(self.value.val, self.value.meta);
				return ValueItem(arr, self.value.meta.val_len, no_copy);
			})
			static AttachAFun(__to_ui16_arr, 1, {
				auto& self = AttachA::Interface::getExtractAs<AtomicObject>(args[0], virtual_table);
				std::lock_guard<TaskRecursiveMutex> lock(self.mutex);				
				uint16_t* arr = ABI_IMPL::Vcast<uint16_t*>(self.value.val, self.value.meta);
				return ValueItem(arr, self.value.meta.val_len, no_copy);
			})
			static AttachAFun(__to_ui32_arr, 1, {
				auto& self = AttachA::Interface::getExtractAs<AtomicObject>(args[0], virtual_table);
				std::lock_guard<TaskRecursiveMutex> lock(self.mutex);				
				uint32_t* arr = ABI_IMPL::Vcast<uint32_t*>(self.value.val, self.value.meta);
				return ValueItem(arr, self.value.meta.val_len, no_copy);
			})
			static AttachAFun(__to_ui64_arr, 1, {
				auto& self = AttachA::Interface::getExtractAs<AtomicObject>(args[0], virtual_table);
				std::lock_guard<TaskRecursiveMutex> lock(self.mutex);				
				uint64_t* arr = ABI_IMPL::Vcast<uint64_t*>(self.value.val, self.value.meta);
				return ValueItem(arr, self.value.meta.val_len, no_copy);
			})
			static AttachAFun(__to_float_arr, 1, {
				auto& self = AttachA::Interface::getExtractAs<AtomicObject>(args[0], virtual_table);
				std::lock_guard<TaskRecursiveMutex> lock(self.mutex);				
				float* arr = ABI_IMPL::Vcast<float*>(self.value.val, self.value.meta);
				return ValueItem(arr, self.value.meta.val_len, no_copy);
			})
			static AttachAFun(__to_double_arr, 1, {
				auto& self = AttachA::Interface::getExtractAs<AtomicObject>(args[0], virtual_table);
				std::lock_guard<TaskRecursiveMutex> lock(self.mutex);				
				double* arr = ABI_IMPL::Vcast<double*>(self.value.val, self.value.meta);
				return ValueItem(arr, self.value.meta.val_len, no_copy);
			})
			static AttachAFun(__to_farr, 1, {
				auto& self = AttachA::Interface::getExtractAs<AtomicObject>(args[0], virtual_table);
				std::lock_guard<TaskRecursiveMutex> lock(self.mutex);				
				ValueItem* arr = ABI_IMPL::Vcast<ValueItem*>(self.value.val, self.value.meta);
				return ValueItem(arr, self.value.meta.val_len, no_copy);
			})
			static AttachAFun(__to_undefined_pointer, 1, {
				auto& self = AttachA::Interface::getExtractAs<AtomicObject>(args[0], virtual_table);
				std::lock_guard<TaskRecursiveMutex> lock(self.mutex);
				return (void*)self.value;
			})
			static AttachAFun(__to_string, 1, {
				auto& self = AttachA::Interface::getExtractAs<AtomicObject>(args[0], virtual_table);
				std::lock_guard<TaskRecursiveMutex> lock(self.mutex);
				return (std::string)self.value;
			})
			static AttachAFun(__to_uarr, 1, {
				auto& self = AttachA::Interface::getExtractAs<AtomicObject>(args[0], virtual_table);
				std::lock_guard<TaskRecursiveMutex> lock(self.mutex);
				return (list_array<ValueItem>)self.value;
			})
			static AttachAFun(__to_type_identifier, 1, {
				auto& self = AttachA::Interface::getExtractAs<AtomicObject>(args[0], virtual_table);
				std::lock_guard<TaskRecursiveMutex> lock(self.mutex);
				return (ValueMeta)self.value;
			})
			static AttachAFun(__to_timepoint, 1, {
				auto& self = AttachA::Interface::getExtractAs<AtomicObject>(args[0], virtual_table);
				std::lock_guard<TaskRecursiveMutex> lock(self.mutex);
				return (std::chrono::steady_clock::time_point)self.value;
			})
			static AttachAFun(__to_map, 1, {
				auto& self = AttachA::Interface::getExtractAs<AtomicObject>(args[0], virtual_table);
				std::lock_guard<TaskRecursiveMutex> lock(self.mutex);
				return (std::unordered_map<ValueItem, ValueItem>&)self.value;
			})
			static AttachAFun(__to_set, 1, {
				auto& self = AttachA::Interface::getExtractAs<AtomicObject>(args[0], virtual_table);
				std::lock_guard<TaskRecursiveMutex> lock(self.mutex);
				return (std::unordered_set<ValueItem>&)self.value;
			})
			static AttachAFun(__to_function, 1, {
				auto& self = AttachA::Interface::getExtractAs<AtomicObject>(args[0], virtual_table);
				std::lock_guard<TaskRecursiveMutex> lock(self.mutex);
				return (typed_lgr<class FuncEnvironment>&)self.value;
			})

			static AttachAFun(__explicit_await, 1, {
				auto& self = AttachA::Interface::getExtractAs<AtomicObject>(args[0], virtual_table);
				std::lock_guard<TaskRecursiveMutex> lock(self.mutex);
				self.value.getAsync();
			})
			static AttachAFun(__make_gc, 1, {
				auto& self = AttachA::Interface::getExtractAs<AtomicObject>(args[0], virtual_table);
				std::lock_guard<TaskRecursiveMutex> lock(self.mutex);
				self.value.make_gc();
			})
			static AttachAFun(__localize_gc, 1, {
				auto& self = AttachA::Interface::getExtractAs<AtomicObject>(args[0], virtual_table);
				std::lock_guard<TaskRecursiveMutex> lock(self.mutex);
				self.value.localize_gc();
			})
			static AttachAFun(__ungc, 1, {
				auto& self = AttachA::Interface::getExtractAs<AtomicObject>(args[0], virtual_table);
				std::lock_guard<TaskRecursiveMutex> lock(self.mutex);
				self.value.ungc();
			})
			static AttachAFun(__is_gc, 1, {
				auto& self = AttachA::Interface::getExtractAs<AtomicObject>(args[0], virtual_table);
				std::lock_guard<TaskRecursiveMutex> lock(self.mutex);
				return self.value.is_gc();
			})
			static AttachAFun(__hash, 1, {
				auto& self = AttachA::Interface::getExtractAs<AtomicObject>(args[0], virtual_table);
				std::lock_guard<TaskRecursiveMutex> lock(self.mutex);
				return self.value.hash();
			})
			static AttachAFun(__make_slice, 2, {
				auto& self = AttachA::Interface::getExtractAs<AtomicObject>(args[0], virtual_table);
				std::lock_guard<TaskRecursiveMutex> lock(self.mutex);
				if(len == 2)
					return self.value.make_slice((uint32_t)args[1], self.value.meta.val_len);
				else 
					return self.value.make_slice((uint32_t)args[1], (uint32_t)args[2]);
			})
			static AttachAFun(__size, 1, {
				auto& self = AttachA::Interface::getExtractAs<AtomicObject>(args[0], virtual_table);
				std::lock_guard<TaskRecursiveMutex> lock(self.mutex);
				return self.value.meta.val_len;
			})
			static AttachAFun(__get, 1, {
				auto& self = AttachA::Interface::getExtractAs<AtomicObject>(args[0], virtual_table);
				std::lock_guard<TaskRecursiveMutex> lock(self.mutex);
				return self.value;
			})
			static AttachAFun(__set, 2, {
				auto& self = AttachA::Interface::getExtractAs<AtomicObject>(args[0], virtual_table);
				std::lock_guard<TaskRecursiveMutex> lock(self.mutex);
				self.value = args[1];
			})






			static void init(){
				virtual_table = AttachA::Interface::createTable<AtomicObject>("AtomicAny",
					AttachA::Interface::direct_method(symbols::structures::add_operator, __add),
					AttachA::Interface::direct_method(symbols::structures::subtract_operator, __sub),
					AttachA::Interface::direct_method(symbols::structures::multiply_operator, __mul),
					AttachA::Interface::direct_method(symbols::structures::divide_operator, __div),
					AttachA::Interface::direct_method(symbols::structures::modulo_operator, __mod),
					AttachA::Interface::direct_method(symbols::structures::bitwise_and_operator, __and),
					AttachA::Interface::direct_method(symbols::structures::bitwise_or_operator, __or),
					AttachA::Interface::direct_method(symbols::structures::bitwise_xor_operator, __xor),
					AttachA::Interface::direct_method(symbols::structures::bitwise_shift_left_operator, __lshift),
					AttachA::Interface::direct_method(symbols::structures::bitwise_shift_right_operator, __rshift),
					AttachA::Interface::direct_method(symbols::structures::not_equal_operator, __not_equal),
					AttachA::Interface::direct_method(symbols::structures::equal_operator, __equal),
					AttachA::Interface::direct_method(symbols::structures::less_operator, __less),
					AttachA::Interface::direct_method(symbols::structures::greater_operator, __greater),
					AttachA::Interface::direct_method(symbols::structures::less_or_equal_operator, __less_equal),
					AttachA::Interface::direct_method(symbols::structures::greater_or_equal_operator, __greater_equal),
					AttachA::Interface::direct_method(symbols::structures::increment_operator, __inc),
					AttachA::Interface::direct_method(symbols::structures::decrement_operator, __dec),
					AttachA::Interface::direct_method(symbols::structures::not_operator, __not),
					AttachA::Interface::direct_method(symbols::structures::bitwise_not_operator, __not),
					AttachA::Interface::direct_method(symbols::structures::convert::to_boolean, __to_boolean),
					AttachA::Interface::direct_method(symbols::structures::convert::to_string, __to_string),
					AttachA::Interface::direct_method(symbols::structures::convert::to_ui8, __to_u8),
					AttachA::Interface::direct_method(symbols::structures::convert::to_ui16, __to_u16),
					AttachA::Interface::direct_method(symbols::structures::convert::to_ui32, __to_u32),
					AttachA::Interface::direct_method(symbols::structures::convert::to_ui64, __to_u64),
					AttachA::Interface::direct_method(symbols::structures::convert::to_i8, __to_i8),
					AttachA::Interface::direct_method(symbols::structures::convert::to_i16, __to_i16),
					AttachA::Interface::direct_method(symbols::structures::convert::to_i32, __to_i32),
					AttachA::Interface::direct_method(symbols::structures::convert::to_i64, __to_i64),
					AttachA::Interface::direct_method(symbols::structures::convert::to_float, __to_float),
					AttachA::Interface::direct_method(symbols::structures::convert::to_double, __to_double),
					AttachA::Interface::direct_method(symbols::structures::convert::to_ui8_arr, __to_ui8_arr),
					AttachA::Interface::direct_method(symbols::structures::convert::to_ui16_arr, __to_ui16_arr),
					AttachA::Interface::direct_method(symbols::structures::convert::to_ui32_arr, __to_ui32_arr),
					AttachA::Interface::direct_method(symbols::structures::convert::to_ui64_arr, __to_ui64_arr),
					AttachA::Interface::direct_method(symbols::structures::convert::to_i8_arr, __to_i8_arr),
					AttachA::Interface::direct_method(symbols::structures::convert::to_i16_arr, __to_i16_arr),
					AttachA::Interface::direct_method(symbols::structures::convert::to_i32_arr, __to_i32_arr),
					AttachA::Interface::direct_method(symbols::structures::convert::to_i64_arr, __to_i64_arr),
					AttachA::Interface::direct_method(symbols::structures::convert::to_float_arr, __to_float_arr),
					AttachA::Interface::direct_method(symbols::structures::convert::to_double_arr, __to_double_arr),
					AttachA::Interface::direct_method(symbols::structures::convert::to_farr, __to_farr),
					AttachA::Interface::direct_method(symbols::structures::convert::to_timepoint, __to_timepoint),
					AttachA::Interface::direct_method(symbols::structures::convert::to_type_identifier, __to_type_identifier),
					AttachA::Interface::direct_method(symbols::structures::convert::to_function, __to_function),
					AttachA::Interface::direct_method(symbols::structures::convert::to_map, __to_map),
					AttachA::Interface::direct_method(symbols::structures::convert::to_set, __to_set),
					AttachA::Interface::direct_method(symbols::structures::convert::to_uarr, __to_uarr),
					AttachA::Interface::direct_method("explicit_await", __explicit_await),
					AttachA::Interface::direct_method("make_gc", __make_gc),
					AttachA::Interface::direct_method("localize_gc", __localize_gc),
					AttachA::Interface::direct_method("ungc", __ungc),
					AttachA::Interface::direct_method("is_gc", __is_gc),
					AttachA::Interface::direct_method("hash", __hash),
					AttachA::Interface::direct_method("make_slice", __make_slice),
					AttachA::Interface::direct_method("size", __size),
					AttachA::Interface::direct_method("get", __get),
					AttachA::Interface::direct_method("set", __set)
				);
			}




		};
		namespace constructor {
			ValueItem* createProxy_Bool(ValueItem* args, uint32_t len){
				bool set = len == 0 ? false : (bool)args[0];
				return new ValueItem(AttachA::Interface::constructStructure<AtomicBasic<bool>>(AtomicBasic<bool>::virtual_table, set), no_copy);
			}
			ValueItem* createProxy_I8(ValueItem* args, uint32_t len){
				int8_t set = len == 0 ? 0 : (int8_t)args[0];
				return new ValueItem(AttachA::Interface::constructStructure<AtomicBasic<int8_t>>(AtomicBasic<int8_t>::virtual_table, set), no_copy);
			}
			ValueItem* createProxy_I16(ValueItem* args, uint32_t len){
				int16_t set = len == 0 ? 0 : (int16_t)args[0];
				return new ValueItem(AttachA::Interface::constructStructure<AtomicBasic<int16_t>>(AtomicBasic<int16_t>::virtual_table, set), no_copy);
			}
			ValueItem* createProxy_I32(ValueItem* args, uint32_t len){
				int32_t set = len == 0 ? 0 : (int32_t)args[0];
				return new ValueItem(AttachA::Interface::constructStructure<AtomicBasic<int32_t>>(AtomicBasic<int32_t>::virtual_table, set), no_copy);
			}
			ValueItem* createProxy_I64(ValueItem* args, uint32_t len){
				int64_t set = len == 0 ? 0 : (int64_t)args[0];
				return new ValueItem(AttachA::Interface::constructStructure<AtomicBasic<int64_t>>(AtomicBasic<int64_t>::virtual_table, set), no_copy);
			}
			ValueItem* createProxy_UI8(ValueItem* args, uint32_t len){
				uint8_t set = len == 0 ? 0 : (uint8_t)args[0];
				return new ValueItem(AttachA::Interface::constructStructure<AtomicBasic<uint8_t>>(AtomicBasic<uint8_t>::virtual_table, set), no_copy);
			}
			ValueItem* createProxy_UI16(ValueItem* args, uint32_t len){
				uint16_t set = len == 0 ? 0 : (uint16_t)args[0];
				return new ValueItem(AttachA::Interface::constructStructure<AtomicBasic<uint16_t>>(AtomicBasic<uint16_t>::virtual_table, set), no_copy);
			}
			ValueItem* createProxy_UI32(ValueItem* args, uint32_t len){
				uint32_t set = len == 0 ? 0 : (uint32_t)args[0];
				return new ValueItem(AttachA::Interface::constructStructure<AtomicBasic<uint32_t>>(AtomicBasic<uint32_t>::virtual_table, set), no_copy);
			}
			ValueItem* createProxy_UI64(ValueItem* args, uint32_t len){
				uint64_t set = len == 0 ? 0 : (uint64_t)args[0];
				return new ValueItem(AttachA::Interface::constructStructure<AtomicBasic<uint64_t>>(AtomicBasic<uint64_t>::virtual_table, set), no_copy);
			}
			ValueItem* createProxy_Float(ValueItem* args, uint32_t len){
				float set = len == 0 ? 0 : (float)args[0];
				return new ValueItem(AttachA::Interface::constructStructure<AtomicBasic<float>>(AtomicBasic<float>::virtual_table, set), no_copy);
			}
			ValueItem* createProxy_Double(ValueItem* args, uint32_t len){
				double set = len == 0 ? 0 : (double)args[0];
				return new ValueItem(AttachA::Interface::constructStructure<AtomicBasic<double>>(AtomicBasic<double>::virtual_table, set), no_copy);
			}
			ValueItem* createProxy_UndefinedPtr(ValueItem* args, uint32_t len){
				void* set = len == 0 ? nullptr : (void*)args[0];
				return new ValueItem(AttachA::Interface::constructStructure<AtomicBasic<size_t>>(AtomicBasic<size_t>::virtual_table, (size_t)set), no_copy);
			}
			ValueItem* createProxy_Any(ValueItem* args, uint32_t len){
				return new ValueItem(AttachA::Interface::constructStructure<AtomicObject>(AtomicObject::virtual_table, len ? args[0] : nullptr), no_copy);
			}
		}
	}

	
	void init() {
		init_ConditionVariable();
		init_Mutex();
		init_RecursiveMutex();
		init_Semaphore();
		init_EventSystem();
		init_TaskLimiter();
		init_TaskQuery();
		init_TaskResultIterator();
		init_Task();
		init_TaskGroup();
		atomic::AtomicObject::init();
		atomic::AtomicBasic<bool>::init();
		atomic::AtomicBasic<int8_t>::init();
		atomic::AtomicBasic<int16_t>::init();
		atomic::AtomicBasic<int32_t>::init();
		atomic::AtomicBasic<int64_t>::init();
		atomic::AtomicBasic<uint8_t>::init();
		atomic::AtomicBasic<uint16_t>::init();
		atomic::AtomicBasic<uint32_t>::init();
		atomic::AtomicBasic<uint64_t>::init();
		atomic::AtomicBasic<float>::init();
		atomic::AtomicBasic<double>::init();
		//already initialized in atomic::AtomicBasic<uint64_t> or atomic::AtomicBasic<uint32_t>
		//atomic::AtomicBasic<size_t>::init();
	}
}