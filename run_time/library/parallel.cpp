// Copyright Danyil Melnytskyi 2022-2023
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include "../AttachA_CXX.hpp"
#include "../../configuration/agreement/symbols.hpp"
namespace parallel {
	ProxyClassDefine define_ConditionVariable;
	ProxyClassDefine define_Mutex;
	ProxyClassDefine define_Semaphore;
	ProxyClassDefine define_ConcurentFile;
	ProxyClassDefine define_EventSystem;
	ProxyClassDefine define_TaskLimiter;
	ProxyClassDefine define_TaskQuery;
	ProxyClassDefine define_ValueMonitor;
	ProxyClassDefine define_ValueChangeMonitor;

	template<class Class_>
	inline typed_lgr<Class_> getClass(ValueItem* vals) {
		vals->getAsync();
		if (vals->meta.vtype == VType::proxy)
			return (*(typed_lgr<Class_>*)(((ProxyClass*)vals->getSourcePtr()))->class_ptr);
		else
			throw InvalidOperation("That function used only in proxy class");
	}


#pragma region ConditionVariable
	ValueItem* funs_ConditionVariable_wait(ValueItem* vals, uint32_t len) {
		if (len) {
			if (vals->meta.vtype == VType::proxy) {
				switch (len) {
				case 1: {
					run_time::threading::mutex mt;
					MutexUnify unif(mt);
					std::unique_lock lock(unif);
					getClass<TaskConditionVariable>(vals)->wait(lock);
					return nullptr;
				}
				case 2: {
					if (vals[1].meta.vtype == VType::proxy) {
						if (AttachA::Interface::name(vals[1]) == "mutex") {
							auto tmp = getClass<TaskMutex>(vals + 1);
							MutexUnify unif(*tmp.getPtr());
							ValueItem* ret = nullptr;
							std::unique_lock lock(unif, std::adopt_lock);
							getClass<TaskConditionVariable>(vals)->wait(lock);
							lock.release();
						}
						else
							throw  InvalidArguments("That function recuive [class ptr] and optional [mutex]");
					}
					else {
						run_time::threading::mutex mt;
						MutexUnify unif(mt);
						std::unique_lock lock(unif);
						return new ValueItem(getClass<TaskConditionVariable>(vals)->wait_for(lock,(size_t)vals[1]));
					}
				}
				case 3:
					if (vals[1].meta.vtype == VType::proxy) {
						if (AttachA::Interface::name(vals[1]) == "mutex") {
							auto tmp = getClass<TaskMutex>(vals + 1);

							MutexUnify unif(*tmp.getPtr());
							ValueItem* ret = nullptr;
							{
								std::unique_lock lock(unif, std::adopt_lock);
								ret = new ValueItem(getClass<TaskConditionVariable>(vals)->wait_for(lock, (size_t)vals[2]));
								lock.release();
							}
							return ret;
						}
						else throw InvalidArguments("That function recuive [class ptr], optional [mutex] and optional [milliseconds to timeout]");
					}
					else throw InvalidArguments("That function recuive [class ptr], optional [mutex] and optional [milliseconds to timeout]");
				default:
					throw InvalidArguments("That function recuive [class ptr] and optional [milliseconds to timeout]");
				}
			}
		}
		throw InvalidOperation("That function used only in proxy class");
	}
	ValueItem* funs_ConditionVariable_wait_until(ValueItem* vals, uint32_t len) {
		if (len) {
			if (vals->meta.vtype == VType::proxy) {
				switch (len) {
				case 2: {
					run_time::threading::mutex mt;
					MutexUnify unif(mt);
					std::unique_lock lock(unif);
					return new ValueItem(getClass<TaskConditionVariable>(vals)->wait_until(lock, (std::chrono::high_resolution_clock::time_point)vals[1]));
				}
				case 3:
					if (vals[1].meta.vtype == VType::proxy) {
						if (AttachA::Interface::name(vals[1]) == "mutex") {
							auto tmp = getClass<TaskMutex>(vals + 1);
							MutexUnify unif(*tmp.getPtr());
							ValueItem* ret = nullptr;
							{
								std::unique_lock lock(unif, std::adopt_lock);
								ret = new ValueItem(getClass<TaskConditionVariable>(vals)->wait_until(lock, (std::chrono::high_resolution_clock::time_point)vals[2]));
								lock.release();
							}
							return ret;
						}
						else
							throw  InvalidArguments("That function recuive [class ptr], optional [mutex] and optional  [milliseconds to timeout] ");
					}
				default:
					throw InvalidArguments("That function recuive only [class ptr], [mutex] and [time point value in nanoseconds] or [class ptr] and [time point value in nanoseconds]");
				}
			}
		}
		throw InvalidOperation("That function used only in proxy class");
	}
	ValueItem* funs_ConditionVariable_notify_one(ValueItem* vals, uint32_t len) {
		if (len) {
			if (vals->meta.vtype == VType::proxy) {
				getClass<TaskConditionVariable>(vals)->notify_one();
				return nullptr;
			}
		}
		throw InvalidOperation("That function used only in proxy class");
	}
	ValueItem* funs_ConditionVariable_notify_all(ValueItem* vals, uint32_t len) {
		if (len) {
			if (vals->meta.vtype == VType::proxy) {
				getClass<TaskConditionVariable>(vals)->notify_one();
				return nullptr;
			}
		}
		throw InvalidOperation("That function used only in proxy class");
	}

	void init_ConditionVariable() {
		define_ConditionVariable.copy = AttachA::Interface::special::proxyCopy<TaskConditionVariable, true>;
		define_ConditionVariable.destructor = AttachA::Interface::special::proxyDestruct<TaskConditionVariable, true>;
		define_ConditionVariable.name = "condition_variable";
		define_ConditionVariable.funs["wait"] = { new FuncEnviropment(funs_ConditionVariable_wait,false),false,ClassAccess::pub };
		define_ConditionVariable.funs["wait_until"] = { new FuncEnviropment(funs_ConditionVariable_wait_until,false),false,ClassAccess::pub };
		define_ConditionVariable.funs["notify_one"] = { new FuncEnviropment(funs_ConditionVariable_notify_one,false),false,ClassAccess::pub };
		define_ConditionVariable.funs["notify_all"] = { new FuncEnviropment(funs_ConditionVariable_notify_all,false),false,ClassAccess::pub };
	}
#pragma endregion
#pragma region Mutex
	ValueItem* funs_Mutex_lock(ValueItem* vals, uint32_t len) {
		if (len) {
			if (vals->meta.vtype == VType::proxy) {
				getClass<TaskMutex>(vals)->lock();
				return nullptr;
			}
		}
		throw InvalidOperation("That function used only in proxy class");
	}
	ValueItem* funs_Mutex_unlock(ValueItem* vals, uint32_t len) {
		if (len) {
			if (vals->meta.vtype == VType::proxy) {
				getClass<TaskMutex>(vals)->unlock();
				return nullptr;
			}
		}
		throw InvalidOperation("That function used only in proxy class");
	}
	ValueItem* funs_Mutex_try_lock(ValueItem* vals, uint32_t len) {
		if (len) {
			if (vals->meta.vtype == VType::proxy) {
				if(len == 1)
					return new ValueItem(getClass<TaskMutex>(vals)->try_lock());
				else
					return new ValueItem(getClass<TaskMutex>(vals)->try_lock_for((size_t)vals[1]));
			}
		}
		throw InvalidOperation("That function used only in proxy class");
	}
	ValueItem* funs_Mutex_try_lock_until(ValueItem* vals, uint32_t len) {
		if (len) {
			if (vals->meta.vtype == VType::proxy) {
				if (len >= 2)
					return new ValueItem(getClass<TaskMutex>(vals)->try_lock_until((std::chrono::high_resolution_clock::time_point)vals[1]));
				else
					throw InvalidArguments("That function recuive only [class ptr] and [time point value in nanoseconds]");
			}
		}
		throw InvalidOperation("That function used only in proxy class");
	}
	ValueItem* funs_Mutex_is_locked(ValueItem* vals, uint32_t len) {
		if (len)
			if (vals->meta.vtype == VType::proxy)
				return new ValueItem((uint8_t)getClass<TaskMutex>(vals)->is_locked());
		throw InvalidOperation("That function used only in proxy class");
	}
	void init_Mutex() {
		define_Mutex.copy = AttachA::Interface::special::proxyCopy<TaskMutex, true>;
		define_Mutex.destructor = AttachA::Interface::special::proxyDestruct<TaskMutex, true>;
		define_Mutex.name = "mutex";
		define_Mutex.funs["lock"] = { new FuncEnviropment(funs_Mutex_lock,false),false,ClassAccess::pub };
		define_Mutex.funs["unlock"] = { new FuncEnviropment(funs_Mutex_unlock,false),false,ClassAccess::pub };
		define_Mutex.funs["try_lock"] = { new FuncEnviropment(funs_Mutex_try_lock,false),false,ClassAccess::pub };
		define_Mutex.funs["try_lock_until"] = { new FuncEnviropment(funs_Mutex_try_lock_until,false),false,ClassAccess::pub };
		define_Mutex.funs["is_locked"] = { new FuncEnviropment(funs_Mutex_is_locked,false),false,ClassAccess::pub };
	}
#pragma endregion
#pragma region Semaphore
	ValueItem* funs_Semaphore_lock(ValueItem* vals, uint32_t len) {
		if (len) {
			if (vals->meta.vtype == VType::proxy) {
				getClass<TaskSemaphore>(vals)->lock();
				return nullptr;
			}
		}
		throw InvalidOperation("That function used only in proxy class");
	}
	ValueItem* funs_Semaphore_release(ValueItem* vals, uint32_t len) {
		if (len) {
			if (vals->meta.vtype == VType::proxy) {
				getClass<TaskSemaphore>(vals)->release();
				return nullptr;
			}
		}
		throw InvalidOperation("That function used only in proxy class");
	}
	ValueItem* funs_Semaphore_release_all(ValueItem* vals, uint32_t len) {
		if (len) {
			if (vals->meta.vtype == VType::proxy) {
				getClass<TaskSemaphore>(vals)->release_all();
				return nullptr;
			}
		}
		throw InvalidOperation("That function used only in proxy class");
	}
	ValueItem* funs_Semaphore_try_lock(ValueItem* vals, uint32_t len) {
		if (len) {
			if (vals->meta.vtype == VType::proxy) {
				if(len==1)
					return new ValueItem(getClass<TaskSemaphore>(vals)->try_lock());
				else
					return new ValueItem(getClass<TaskSemaphore>(vals)->try_lock_for((size_t)vals[1]));
			}
		}
		throw InvalidOperation("That function used only in proxy class");
	}
	ValueItem* funs_Semaphore_try_lock_until(ValueItem* vals, uint32_t len) {
		if (len) {
			if (vals->meta.vtype == VType::proxy) {
				if (len >= 2)
					return new ValueItem(getClass<TaskSemaphore>(vals)->try_lock_until((std::chrono::high_resolution_clock::time_point)vals[1]));
				else
					throw InvalidArguments("That function recuive only [class ptr] and [time point value in nanoseconds]");
			}
		}
		throw InvalidOperation("That function used only in proxy class");
	}
	ValueItem* funs_Semaphore_is_locked(ValueItem* vals, uint32_t len) {
		if (len)
			if (vals->meta.vtype == VType::proxy)
				return new ValueItem((uint8_t)getClass<TaskSemaphore>(vals)->is_locked());
		throw InvalidOperation("That function used only in proxy class");
	}

	void init_Semaphore() {
		define_Semaphore.copy = AttachA::Interface::special::proxyCopy<TaskSemaphore, true>;
		define_Semaphore.destructor = AttachA::Interface::special::proxyDestruct<TaskSemaphore, true>;
		define_Semaphore.name = "semaphore";
		define_Semaphore.funs["lock"] = { new FuncEnviropment(funs_Semaphore_lock,false),false,ClassAccess::pub };
		define_Semaphore.funs["release"] = { new FuncEnviropment(funs_Semaphore_release,false),false,ClassAccess::pub };
		define_Semaphore.funs["release_all"] = { new FuncEnviropment(funs_Semaphore_release_all,false),false,ClassAccess::pub };
		define_Semaphore.funs["try_lock"] = { new FuncEnviropment(funs_Semaphore_try_lock,false),false,ClassAccess::pub };
		define_Semaphore.funs["try_lock_until"] = { new FuncEnviropment(funs_Semaphore_try_lock_until,false),false,ClassAccess::pub };
		define_Semaphore.funs["is_locked"] = { new FuncEnviropment(funs_Semaphore_is_locked,false),false,ClassAccess::pub };
	}
#pragma endregion
#pragma region EventSystem
	ValueItem* funs_EventSystem_operator_add(ValueItem* vals, uint32_t len) {
		if(len < 2)
			throw InvalidArguments("That function recuive only [class ptr] and [function ptr]");
		if (vals->meta.vtype == VType::proxy) {
			auto fun = vals[1].funPtr();
			if (!fun) 
				throw InvalidArguments("That function recuive only [class ptr] and [function ptr]");
			getClass<EventSystem>(vals)->operator+=(*fun);
			return nullptr;
		}
		throw InvalidOperation("That function used only in proxy class");
	}
	ValueItem* funs_EventSystem_join(ValueItem* vals, uint32_t len) {
		if (len < 2)
			throw InvalidArguments("That function recuive only [class ptr] [function ptr] [optional is_async] and [optional enum Priorithy]");
		if (vals->meta.vtype == VType::proxy) {
			auto fun = vals[1].funPtr();
			bool as_async = len > 2 ? (bool)vals[2] : false;
			EventSystem::Priorithy priorithy = len > 3 ? (EventSystem::Priorithy)(uint8_t)vals[3] : EventSystem::Priorithy::avg;
			if (!fun)
				throw InvalidArguments("That function recuive only [class ptr] [function ptr] [optional is_async] and [optional enum Priorithy]");
			getClass<EventSystem>(vals)->join(*fun, as_async, priorithy);
			return nullptr;
		}
		throw InvalidOperation("That function used only in proxy class");
	}
	ValueItem* funs_EventSystem_leave(ValueItem* vals, uint32_t len) {
		if (len < 2)
			throw InvalidArguments("That function recuive only [class ptr] [function ptr] [optional is_async] and [optional enum Priorithy]");
		if (vals->meta.vtype == VType::proxy) {
			auto fun = vals[1].funPtr();
			bool as_async = len > 2 ? (bool)vals[2] : false;
			EventSystem::Priorithy priorithy = len > 3 ? (EventSystem::Priorithy)(uint8_t)vals[3] : EventSystem::Priorithy::avg;
			if (!fun)
				throw InvalidArguments("That function recuive only [class ptr] [function ptr] [optional is_async] and [optional enum Priorithy]");
			getClass<EventSystem>(vals)->leave(*fun, as_async, priorithy);
			return nullptr;
		}
		throw InvalidOperation("That function used only in proxy class");
	}

	ValueItem __funs_EventSystem_get_values0(ValueItem* vals, uint32_t len) {
		ValueItem values;
		if (len > 2) {
			size_t size = len - 1;
			ValueItem* args = new ValueItem[size]{};
			for (uint32_t i = 0; i < size; i++)
				args[i] = vals[i + 1];
			values = ValueItem(args, size, no_copy);
		}
		return values;
	}
	ValueItem* funs_EventSystem_notify(ValueItem* vals, uint32_t len) {
		if (len < 2)
			throw InvalidArguments("That function recuive [class ptr] [any]...");
		if (vals->meta.vtype == VType::proxy) {
			auto compacted = __funs_EventSystem_get_values0(vals, len);
			return new ValueItem(getClass<EventSystem>(vals)->notify(compacted));
		}
		throw InvalidOperation("That function used only in proxy class");
	}
	ValueItem* funs_EventSystem_sync_notify(ValueItem* vals, uint32_t len) {
		if (len < 2)
			throw InvalidArguments("That function recuive [class ptr] [any]...");
		if (vals->meta.vtype == VType::proxy) {
			auto compacted = __funs_EventSystem_get_values0(vals, len);
			return new ValueItem(getClass<EventSystem>(vals)->sync_notify(compacted));
		}
		throw InvalidOperation("That function used only in proxy class");
	}
	ValueItem* funs_EventSystem_await_notify(ValueItem* vals, uint32_t len) {
		if (len < 2)
			throw InvalidArguments("That function recuive [class ptr] [any]...");
		if (vals->meta.vtype == VType::proxy) {
			auto compacted = __funs_EventSystem_get_values0(vals, len);
			return new ValueItem(getClass<EventSystem>(vals)->await_notify(compacted));
		}
		throw InvalidOperation("That function used only in proxy class");
	}
	ValueItem* funs_EventSystem_async_notify(ValueItem* vals, uint32_t len) {
		if (len < 2)
			throw InvalidArguments("That function recuive [class ptr] [any]...");
		if (vals->meta.vtype == VType::proxy) {
			auto compacted = __funs_EventSystem_get_values0(vals, len);
			return new ValueItem(getClass<EventSystem>(vals)->async_notify(compacted));
		}
		throw InvalidOperation("That function used only in proxy class");
	}

	void init_EventSystem() {
		define_EventSystem.copy = AttachA::Interface::special::proxyCopy<EventSystem, true>;
		define_EventSystem.destructor = AttachA::Interface::special::proxyDestruct<EventSystem, true>;
		define_EventSystem.name = "event_system";

		define_EventSystem.funs[symbols::structures::add_operator] = { new FuncEnviropment(funs_EventSystem_operator_add,false),false,ClassAccess::pub };

		define_EventSystem.funs["join"] = { new FuncEnviropment(funs_EventSystem_join,false),false,ClassAccess::pub };
		define_EventSystem.funs["leave"] = { new FuncEnviropment(funs_EventSystem_leave,false),false,ClassAccess::pub };

		define_EventSystem.funs["notify"] = { new FuncEnviropment(funs_EventSystem_notify,false),false,ClassAccess::pub };
		define_EventSystem.funs["sync_notify"] = { new FuncEnviropment(funs_EventSystem_sync_notify,false),false,ClassAccess::pub };

		define_EventSystem.funs["await_notify"] = { new FuncEnviropment(funs_EventSystem_await_notify,false),false,ClassAccess::pub };
		define_EventSystem.funs["async_notify"] = { new FuncEnviropment(funs_EventSystem_async_notify,false),false,ClassAccess::pub };
	}
#pragma endregion
#pragma region TaskLimiter
	ValueItem* funs_TaskLimiter_set_max_treeshold(ValueItem* vals, uint32_t len) {
		if (len < 2)
			throw InvalidArguments("That function recuive only [class ptr] [count]");
		if (vals->meta.vtype == VType::proxy) {
			getClass<TaskLimiter>(vals)->set_max_treeshold((uint64_t)vals[1]);
			return nullptr;
		}
		throw InvalidOperation("That function used only in proxy class");
	}
	ValueItem* funs_TaskLimiter_lock(ValueItem* vals, uint32_t len) {
		if (len) {
			if (vals->meta.vtype == VType::proxy) {
				getClass<TaskLimiter>(vals)->lock();
				return nullptr;
			}
		}
		throw InvalidOperation("That function used only in proxy class");
	}
	ValueItem* funs_TaskLimiter_unlock(ValueItem* vals, uint32_t len) {
		if (len) {
			if (vals->meta.vtype == VType::proxy) {
				getClass<TaskLimiter>(vals)->unlock();
				return nullptr;
			}
		}
		throw InvalidOperation("That function used only in proxy class");
	}
	ValueItem* funs_TaskLimiter_try_lock(ValueItem* vals, uint32_t len) {
		if (len) {
			if (vals->meta.vtype == VType::proxy) {
				switch (len) {
				case 1:
					return new ValueItem(getClass<TaskLimiter>(vals)->try_lock());
				case 2:
					return new ValueItem(getClass<TaskLimiter>(vals)->try_lock_for((size_t)vals[1]));
				default:
					throw InvalidArguments("That function recuive only [class ptr] and optional [milliseconds to timeout]");
				}
			}
		}
		throw InvalidOperation("That function used only in proxy class");
	}
	ValueItem* funs_TaskLimiter_try_lock_until(ValueItem* vals, uint32_t len) {
		if (len) {
			if (vals->meta.vtype == VType::proxy) {
				if (len >= 2)
					return new ValueItem(getClass<TaskLimiter>(vals)->try_lock_until((std::chrono::high_resolution_clock::time_point)vals[1]));
				else
					throw InvalidArguments("That function recuive only [class ptr] and [time point value in nanoseconds]");
			}
		}
		throw InvalidOperation("That function used only in proxy class");
	}
	ValueItem* funs_TaskLimiter_is_locked(ValueItem* vals, uint32_t len) {
		if (len)
			if (vals->meta.vtype == VType::proxy)
				return new ValueItem(getClass<TaskLimiter>(vals)->is_locked());
		throw InvalidOperation("That function used only in proxy class");
	}
	void init_TaskLimiter() {
		define_TaskLimiter.copy = AttachA::Interface::special::proxyCopy<TaskLimiter, true>;
		define_TaskLimiter.destructor = AttachA::Interface::special::proxyDestruct<TaskLimiter, true>;
		define_TaskLimiter.name = "task_limiter";
		define_TaskLimiter.funs["set_max_treeshold"] = { new FuncEnviropment(funs_TaskLimiter_set_max_treeshold,false),false,ClassAccess::pub };
		define_TaskLimiter.funs["lock"] = { new FuncEnviropment(funs_TaskLimiter_lock,false),false,ClassAccess::pub };
		define_TaskLimiter.funs["unlock"] = { new FuncEnviropment(funs_TaskLimiter_unlock,false),false,ClassAccess::pub };
		define_TaskLimiter.funs["try_lock"] = { new FuncEnviropment(funs_TaskLimiter_try_lock,false),false,ClassAccess::pub };
		define_TaskLimiter.funs["try_lock_until"] = { new FuncEnviropment(funs_TaskLimiter_try_lock_until,false),false,ClassAccess::pub };
		define_TaskLimiter.funs["is_locked"] = { new FuncEnviropment(funs_TaskLimiter_is_locked,false),false,ClassAccess::pub };
	}
#pragma endregion
#pragma region TaskQuery
	ValueItem* funs_TaskQuery_add_task(ValueItem* vals, uint32_t len) {
		if (len < 2)
			throw InvalidArguments("That function recuive [class ptr] [[function], optional [fault function], optional [timeout], optional [use task local]], optional [any args]");
		if (vals->meta.vtype == VType::proxy) {
			ValueItem& val = vals[1];
			typed_lgr<FuncEnviropment> func;
			typed_lgr<FuncEnviropment> fault_func;
			std::chrono::steady_clock::time_point timeout = std::chrono::steady_clock::time_point::min();
			bool used_task_local = false;
			val.getAsync();
			if(val.meta.vtype == VType::faarr || val.meta.vtype == VType::saarr) {
				auto arr = (ValueItem*)val.getSourcePtr();
				if (arr->meta.vtype == VType::function)
					func = *arr->funPtr();
				else
					throw InvalidArguments("That function recuive [class ptr] [[function], optional [fault function], optional [timeout], optional [use task local]], optional [any args]");

				if(val.meta.val_len > 1 && arr[1].meta.vtype == VType::function) 
					fault_func = *arr[1].funPtr();
				if(val.meta.val_len > 2 && arr[1].meta.vtype == VType::time_point)
					timeout = (std::chrono::steady_clock::time_point)arr[2];
				if(val.meta.val_len > 3)
					used_task_local = (bool)arr[3];
			}
			ValueItem args = (len == 3) ? vals[2] : ValueItem();
			return new ValueItem(new typed_lgr(getClass<TaskQuery>(vals)->add_task(func, args,used_task_local,fault_func, timeout)), VType::async_res, no_copy);
		}
		throw InvalidOperation("That function used only in proxy class");
	}

	ValueItem* funs_TaskQuery_enable(ValueItem* vals, uint32_t len) {
		if (len == 1)
			if (vals->meta.vtype == VType::proxy){
				getClass<TaskQuery>(vals)->enable();
				return nullptr;
			}
		throw InvalidOperation("That function used only in proxy class");
	}
	ValueItem* funs_TaskQuery_disable(ValueItem* vals, uint32_t len) {
		if (len == 1)
			if (vals->meta.vtype == VType::proxy){
				getClass<TaskQuery>(vals)->disable();
				return nullptr;
			}
		throw InvalidOperation("That function used only in proxy class");
	}
	ValueItem* funs_TaskQuery_in_query(ValueItem* vals, uint32_t len) {
		if (len == 2)
			if (vals->meta.vtype == VType::proxy){
				if(vals[1].meta.vtype != VType::async_res)
					throw InvalidArguments("That function recuive [class ptr] and [async result (task)]");
				return new ValueItem(getClass<TaskQuery>(vals)->in_query(*(typed_lgr<Task>*)vals[1].getSourcePtr()));
			}
		throw InvalidOperation("That function used only in proxy class");
	}
	ValueItem* funs_TaskQuery_set_max_at_execution(ValueItem* vals, uint32_t len) {
		if (len == 2)
			if (vals->meta.vtype == VType::proxy){
				getClass<TaskQuery>(vals)->set_max_at_execution((size_t)vals[1]);
				return nullptr;
			}
		throw InvalidOperation("That function used only in proxy class");
	}
	ValueItem* funs_TaskQuery_get_max_at_execution(ValueItem* vals, uint32_t len) {
		if (len == 1)
			if (vals->meta.vtype == VType::proxy){
				getClass<TaskQuery>(vals)->get_max_at_execution();
				return nullptr;
			}
		throw InvalidOperation("That function used only in proxy class");
	}

	void init_TaskQuery() {
		define_TaskQuery.copy = AttachA::Interface::special::proxyCopy<TaskQuery, true>;
		define_TaskQuery.destructor = AttachA::Interface::special::proxyDestruct<TaskQuery, true>;
		define_TaskQuery.name = "task_query";
		define_TaskQuery.funs["add_task"] = { new FuncEnviropment(funs_TaskQuery_add_task,false),false,ClassAccess::pub };
		define_TaskQuery.funs["enable"] = { new FuncEnviropment(funs_TaskQuery_enable,false),false,ClassAccess::pub };
		define_TaskQuery.funs["disable"] = { new FuncEnviropment(funs_TaskQuery_disable,false),false,ClassAccess::pub };
		define_TaskQuery.funs["in_query"] = { new FuncEnviropment(funs_TaskQuery_in_query,false),false,ClassAccess::pub };
		define_TaskQuery.funs["set_max_at_execution"] = { new FuncEnviropment(funs_TaskQuery_set_max_at_execution,false),false,ClassAccess::pub };
		define_TaskQuery.funs["get_max_at_execution"] = { new FuncEnviropment(funs_TaskQuery_get_max_at_execution,false),false,ClassAccess::pub };
	}
#pragma endregion



	void init() {
		init_ConditionVariable();
		init_Mutex();
		init_Semaphore();
		init_EventSystem();
		init_TaskLimiter();
		init_TaskQuery();
	}





	namespace constructor {
		ValueItem* createProxy_ConditionVariable(ValueItem*, uint32_t) {
			return new ValueItem(new ProxyClass(new typed_lgr(new TaskConditionVariable()), &define_ConditionVariable), VType::proxy, no_copy);
		}
		ValueItem* createProxy_Mutex(ValueItem*, uint32_t) {
			return new ValueItem(new ProxyClass(new typed_lgr(new TaskMutex()), &define_ConditionVariable), VType::proxy, no_copy);
		}
		ValueItem* createProxy_Semaphore(ValueItem*, uint32_t) {
			return new ValueItem(new ProxyClass(new typed_lgr(new TaskSemaphore()), &define_Semaphore), VType::proxy, no_copy);
		}

		ValueItem* createProxy_EventSystem(ValueItem* val, uint32_t len) {
			return new ValueItem(new ProxyClass(new typed_lgr(new EventSystem()), &define_EventSystem), VType::proxy, no_copy);
		}

		ValueItem* createProxy_TaskLimiter(ValueItem* val, uint32_t len) {
			return new ValueItem(new ProxyClass(new typed_lgr(new TaskLimiter()), &define_TaskLimiter), VType::proxy, no_copy);
		}

		ValueItem* createProxy_TaskQuery(ValueItem* val, uint32_t len){
			return new ValueItem(new ProxyClass(new typed_lgr(new TaskQuery(len ? (size_t)*val : 0)), &define_TaskQuery), VType::proxy, no_copy);
		}

		//ProxyClass createProxy_ValueMonitor() {
		//
		//}
		//ProxyClass createProxy_ValueChangeMonitor() {
		//
		//}
	}
	
	ValueItem* createThread(ValueItem* vals, uint32_t len) {
		if (!len)
			throw InvalidArguments("Excepted at least one value in arguments, excepted arguments: [function] [optional any...]");

		if (len != 1) {
			typed_lgr<FuncEnviropment> func = *vals->funPtr();
			ValueItem* copyArgs = new ValueItem[len - 1];
			vals++;
			len--;
			uint32_t i = 0;
			while (len--)
				copyArgs[i++] = *vals++;
			run_time::threading::thread([](typed_lgr<FuncEnviropment> func, ValueItem* args, uint32_t len) {
				auto tmp = FuncEnviropment::sync_call(func, args, len);
				if (tmp)
					delete tmp;
				delete[] args;
			}, func, copyArgs, i).detach();
		}
		else {
			run_time::threading::thread([](typed_lgr<FuncEnviropment> func) {
				auto tmp = FuncEnviropment::sync_call(func, nullptr, 0);
				if (tmp)
					delete tmp;
			}, *vals->funPtr()).detach();
		}
		return nullptr;
	}
	ValueItem* createThreadAndWait(ValueItem* vals, uint32_t len) {
		if (!len)
			throw InvalidArguments("Excepted at least one value in arguments, excepted arguments: [function] [optional any...]");

		TaskConditionVariable cv;
		TaskMutex mtx;
		MutexUnify unif(mtx);
		std::unique_lock ul(unif);
		bool end = false;
		ValueItem* res = nullptr;
		typed_lgr<FuncEnviropment> func = *vals->funPtr();
		if (len != 1) {
			run_time::threading::thread([&end, &res, &mtx, &cv](typed_lgr<FuncEnviropment> func, ValueItem* args, uint32_t len) {
				try{
					auto tmp = FuncEnviropment::sync_call(func, args, len);
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
			run_time::threading::thread([&end, &res, &mtx, &cv](typed_lgr<FuncEnviropment> func) {
				try{
					auto tmp = FuncEnviropment::sync_call(func, nullptr, 0);
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
	typed_lgr<FuncEnviropment> __createAsyncThread__Awaiter = new FuncEnviropment(_createAsyncThread__Awaiter, false);


	ValueItem* createAsyncThread(ValueItem* vals, uint32_t len){
		if (!len)
			throw InvalidArguments("Excepted at least one value in arguments, excepted arguments: [function] [optional any...]");
		typed_lgr<FuncEnviropment> func = *vals->funPtr();
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

				run_time::threading::thread([awaiter](typed_lgr<FuncEnviropment> func, ValueItem* args, uint32_t len) {
					try{
						auto tmp = FuncEnviropment::sync_call(func, args, len);
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
				run_time::threading::thread([awaiter](typed_lgr<FuncEnviropment> func) {
					try{
						auto tmp = FuncEnviropment::sync_call(func, nullptr, 0);
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
		typed_lgr<FuncEnviropment> func;
		typed_lgr<FuncEnviropment> fault_func;
		std::chrono::steady_clock::time_point timeout = std::chrono::steady_clock::time_point::min();
		bool used_task_local = false;
		auto arr = (ValueItem*)vals->getSourcePtr();
		if (arr->meta.vtype == VType::function)
			func = *arr->funPtr();
		else
			throw InvalidArguments("That function recuive [[function], optional [fault function], optional [timeout], optional [use task local]], optional [any args]");

		if(arr->meta.val_len > 1 && arr[1].meta.vtype == VType::function) 
			fault_func = *arr[1].funPtr();
		if(arr->meta.val_len > 2 && arr[2].meta.vtype == VType::time_point)
			timeout = (std::chrono::steady_clock::time_point)arr[2];
		if(arr->meta.val_len > 3)
			used_task_local = (bool)arr[3];
			
		ValueItem args = (len == 3) ? vals[2] : ValueItem();
		return new ValueItem(new typed_lgr(new Task(func, args, used_task_local, fault_func, timeout)), VType::async_res, no_copy);
	}
}