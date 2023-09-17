// Copyright Danyil Melnytskyi 2022-Present
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <configuration/agreement/symbols.hpp>
#include <run_time/AttachA_CXX.hpp>
#include <util/threading.hpp>
#include <run_time/library/parallel.hpp>

namespace art {
    namespace parallel {
        AttachAVirtualTable* define_ConditionVariable;
        AttachAVirtualTable* define_Mutex;
        AttachAVirtualTable* define_RecursiveMutex;
        AttachAVirtualTable* define_Semaphore;
        AttachAVirtualTable* define_ConcurrentFile;
        AttachAVirtualTable* define_EventSystem;
        AttachAVirtualTable* define_TaskLimiter;
        AttachAVirtualTable* define_TaskQuery;
        AttachAVirtualTable* define_Task;
        AttachAVirtualTable* define_TaskResultIterator;
        AttachAVirtualTable* define_TaskGroup;


        AttachAVirtualTable* define_mutex;
        AttachAVirtualTable* define_rw_mutex;
        AttachAVirtualTable* define_timed_mutex;
        AttachAVirtualTable* define_recursive_mutex;

        AttachAVirtualTable* define_condition_variable;
        AttachAVirtualTable* define_thread;

        MutexUnify getMutex(ValueItem& item) {
            CXX::excepted(item, VType::struct_);
            Structure& st = (Structure&)item;
            void* vtable = st.get_vtable();
            if (vtable == define_Mutex)
                return *CXX::Interface::getExtractAs<art::shared_ptr<TaskMutex>>(item, define_Mutex);
            else if (vtable == define_RecursiveMutex)
                return *CXX::Interface::getExtractAs<art::shared_ptr<TaskRecursiveMutex>>(item, define_RecursiveMutex);
            else if (vtable == define_mutex)
                return *CXX::Interface::getExtractAs<art::shared_ptr<art::mutex>>(item, define_mutex);
            else if (vtable == define_rw_mutex)
                return *CXX::Interface::getExtractAs<art::shared_ptr<art::rw_mutex>>(item, define_mutex);
            else if (vtable == define_timed_mutex)
                return *CXX::Interface::getExtractAs<art::shared_ptr<art::timed_mutex>>(item, define_mutex);
            else if (vtable == define_recursive_mutex)
                return *CXX::Interface::getExtractAs<art::shared_ptr<art::recursive_mutex>>(item, define_mutex);
            else
                CXX::Interface::getExtractAs<art::shared_ptr<art::mutex>>(item, define_mutex);
            throw InternalException("Reached unreachable region");
        }


        template <size_t args_off>
        void parseArgumentsToTask(ValueItem* args, uint32_t len, art::shared_ptr<FuncEnvironment>& func, art::shared_ptr<FuncEnvironment>& fault_func, std::chrono::high_resolution_clock::time_point& timeout, bool& used_task_local, ValueItem& arguments) {
            timeout = std::chrono::high_resolution_clock::time_point::min();
            used_task_local = false;

            CXX::excepted(args[args_off], VType::function);
            func = *args[args_off].funPtr();

            if (len > args_off + 2)
                if (args[args_off + 2].meta.vtype != VType::noting) {
                    CXX::excepted(args[args_off + 2], VType::function);
                    fault_func = *args[args_off + 2].funPtr();
                }
            if (len > args_off + 3)
                if (args[args_off + 3].meta.vtype != VType::noting) {
                    if (args[args_off + 3].meta.vtype == VType::time_point)
                        timeout = (std::chrono::high_resolution_clock::time_point)args[args_off + 3];
                    else
                        timeout = std::chrono::high_resolution_clock::now() + std::chrono::milliseconds((uint64_t)args[args_off + 3]);
                }
            if (len > args_off + 4)
                if (args[args_off + 4].meta.vtype != VType::noting)
                    used_task_local = (bool)args[args_off + 4];
            arguments = len > 5 ? args[5] : nullptr;
        }

#pragma region ConditionVariable

        AttachAFun(funs_ConditionVariable_wait, 1, {
            auto& class_ = *CXX::Interface::getExtractAs<typed_lgr<TaskConditionVariable>>(args[0], define_ConditionVariable);
            switch (len) {
            case 1: {
                mutex mt;
                MutexUnify unify(mt);
                unique_lock lock(unify);
                class_.wait(lock);
                break;
            }
            case 2: {
                if (args[1].meta.vtype == VType::struct_) {
                    MutexUnify unify = getMutex(args[1]);
                    unique_lock lock(unify, adopt_lock);
                    class_.wait(lock);
                    lock.release();
                    break;
                } else if (args[1].meta.vtype == VType::time_point) {
                    mutex mt;
                    MutexUnify unify(mt);
                    unique_lock lock(unify);
                    auto res = class_.wait_until(lock, (std::chrono::high_resolution_clock::time_point)args[1]);
                    lock.release();
                    return res;
                } else {
                    mutex mt;
                    MutexUnify unify(mt);
                    unique_lock lock(unify);
                    return class_.wait_for(lock, (size_t)args[1]);
                }
            }
            case 3:
            default: {
                MutexUnify unify = getMutex(args[1]);
                unique_lock lock(unify, adopt_lock);
                bool res;
                if (args[2].meta.vtype == VType::time_point)
                    res = class_.wait_until(lock, (std::chrono::high_resolution_clock::time_point)args[2]);
                else
                    res = class_.wait_for(lock, (size_t)args[2]);
                lock.release();
                return res;
            }
            }
        });

        AttachAFun(funs_ConditionVariable_wait_until, 2, {
            auto& class_ = *CXX::Interface::getExtractAs<typed_lgr<TaskConditionVariable>>(args[0], define_ConditionVariable);
            switch (len) {
            case 2: {
                mutex mt;
                MutexUnify unify(mt);
                unique_lock lock(unify);
                return class_.wait_until(lock, (std::chrono::high_resolution_clock::time_point)args[1]);
            }
            case 3:
            default: {
                MutexUnify unify = getMutex(args[1]);
                unique_lock lock(unify, adopt_lock);
                auto res = class_.wait_until(lock, (std::chrono::high_resolution_clock::time_point)args[2]);
                lock.release();
                return res;
            }
            }
        });
        AttachAFun(funs_ConditionVariable_notify_one, 1, {
            auto& class_ = *CXX::Interface::getExtractAs<typed_lgr<TaskConditionVariable>>(args[0], define_ConditionVariable);
            class_.notify_one();
        });
        AttachAFun(funs_ConditionVariable_notify_all, 1, {
            auto& class_ = *CXX::Interface::getExtractAs<typed_lgr<TaskConditionVariable>>(args[0], define_ConditionVariable);
            class_.notify_all();
        });

        void init_ConditionVariable() {
            define_ConditionVariable = CXX::Interface::createTable<typed_lgr<TaskConditionVariable>>(
                "condition_variable",
                CXX::Interface::direct_method("wait", funs_ConditionVariable_wait),
                CXX::Interface::direct_method("wait_until", funs_ConditionVariable_wait_until),
                CXX::Interface::direct_method("notify_one", funs_ConditionVariable_notify_one),
                CXX::Interface::direct_method("notify_all", funs_ConditionVariable_notify_all)
            );
            CXX::Interface::typeVTable<typed_lgr<TaskConditionVariable>>() = define_ConditionVariable;
        }

#pragma endregion
#pragma region Mutex

        AttachAFun(funs_Mutex_lock, 1, {
            auto& class_ = *CXX::Interface::getExtractAs<typed_lgr<TaskMutex>>(args[0], define_Mutex);
            class_.lock();
        });
        AttachAFun(funs_Mutex_unlock, 1, {
            auto& class_ = *CXX::Interface::getExtractAs<typed_lgr<TaskMutex>>(args[0], define_Mutex);
            class_.unlock();
        });
        AttachAFun(funs_Mutex_try_lock, 1, {
            auto& class_ = *CXX::Interface::getExtractAs<typed_lgr<TaskMutex>>(args[0], define_Mutex);
            if (len == 1)
                return class_.try_lock();
            else
                return class_.try_lock_for((size_t)args[1]);
        });
        AttachAFun(funs_Mutex_try_lock_until, 2, {
            auto& class_ = *CXX::Interface::getExtractAs<typed_lgr<TaskMutex>>(args[0], define_Mutex);
            return class_.try_lock_until((std::chrono::high_resolution_clock::time_point)args[1]);
        });
        AttachAFun(funs_Mutex_is_locked, 1, {
            auto& class_ = *CXX::Interface::getExtractAs<typed_lgr<TaskMutex>>(args[0], define_Mutex);
            return class_.is_locked();
        });
        AttachAFun(funs_Mutex_is_own, 1, {
            auto& class_ = *CXX::Interface::getExtractAs<typed_lgr<TaskMutex>>(args[0], define_Mutex);
            return class_.is_own();
        });
        AttachAFun(funs_Mutex_lifecycle_lock, 2, {
            auto& class_ = *CXX::Interface::getExtractAs<typed_lgr<TaskMutex>>(args[0], define_Mutex);
            class_.lifecycle_lock((art::typed_lgr<Task>)args[1]);
        });
        AttachAFun(funs_Mutex_sequence_lock, 2, {
            auto& class_ = *CXX::Interface::getExtractAs<typed_lgr<TaskMutex>>(args[0], define_Mutex);
            class_.sequence_lock((art::typed_lgr<Task>)args[1]);
        });

        void init_Mutex() {
            define_Mutex = CXX::Interface::createTable<typed_lgr<TaskMutex>>(
                "mutex",
                CXX::Interface::direct_method("lock", funs_Mutex_lock),
                CXX::Interface::direct_method("unlock", funs_Mutex_unlock),
                CXX::Interface::direct_method("try_lock", funs_Mutex_try_lock),
                CXX::Interface::direct_method("try_lock_until", funs_Mutex_try_lock_until),
                CXX::Interface::direct_method("is_locked", funs_Mutex_is_locked),
                CXX::Interface::direct_method("is_own", funs_Mutex_is_own),
                CXX::Interface::direct_method("lifecycle_lock", funs_Mutex_lifecycle_lock),
                CXX::Interface::direct_method("sequence_lock", funs_Mutex_sequence_lock)
            );
            CXX::Interface::typeVTable<typed_lgr<TaskMutex>>() = define_Mutex;
        }

#pragma endregion
#pragma region RecursiveMutex

        AttachAFun(funs_RecursiveMutex_lock, 1, {
            auto& class_ = *CXX::Interface::getExtractAs<typed_lgr<TaskRecursiveMutex>>(args[0], define_RecursiveMutex);
            class_.lock();
        });
        AttachAFun(funs_RecursiveMutex_unlock, 1, {
            auto& class_ = *CXX::Interface::getExtractAs<typed_lgr<TaskRecursiveMutex>>(args[0], define_RecursiveMutex);
            class_.unlock();
        });
        AttachAFun(funs_RecursiveMutex_try_lock, 1, {
            auto& class_ = *CXX::Interface::getExtractAs<typed_lgr<TaskRecursiveMutex>>(args[0], define_RecursiveMutex);
            if (len == 1)
                return class_.try_lock();
            else
                return class_.try_lock_for((size_t)args[1]);
        });
        AttachAFun(funs_RecursiveMutex_try_lock_until, 2, {
            auto& class_ = *CXX::Interface::getExtractAs<typed_lgr<TaskRecursiveMutex>>(args[0], define_RecursiveMutex);
            return class_.try_lock_until((std::chrono::high_resolution_clock::time_point)args[1]);
        });
        AttachAFun(funs_RecursiveMutex_is_locked, 1, {
            auto& class_ = *CXX::Interface::getExtractAs<typed_lgr<TaskRecursiveMutex>>(args[0], define_RecursiveMutex);
            return class_.is_locked();
        });
        AttachAFun(funs_RecursiveMutex_is_own, 1, {
            auto& class_ = *CXX::Interface::getExtractAs<typed_lgr<TaskRecursiveMutex>>(args[0], define_RecursiveMutex);
            return class_.is_own();
        });
        AttachAFun(funs_RecursiveMutex_lifecycle_lock, 2, {
            auto& class_ = *CXX::Interface::getExtractAs<typed_lgr<TaskRecursiveMutex>>(args[0], define_RecursiveMutex);
            class_.lifecycle_lock((art::typed_lgr<Task>)args[1]);
        });
        AttachAFun(funs_RecursiveMutex_sequence_lock, 2, {
            auto& class_ = *CXX::Interface::getExtractAs<typed_lgr<TaskRecursiveMutex>>(args[0], define_RecursiveMutex);
            class_.sequence_lock((art::typed_lgr<Task>)args[1]);
        });

        void init_RecursiveMutex() {
            define_RecursiveMutex = CXX::Interface::createTable<typed_lgr<TaskRecursiveMutex>>(
                "recursive_mutex",
                CXX::Interface::direct_method("lock", funs_Mutex_lock),
                CXX::Interface::direct_method("unlock", funs_Mutex_unlock),
                CXX::Interface::direct_method("try_lock", funs_Mutex_try_lock),
                CXX::Interface::direct_method("try_lock_until", funs_Mutex_try_lock_until),
                CXX::Interface::direct_method("is_locked", funs_Mutex_is_locked),
                CXX::Interface::direct_method("is_own", funs_RecursiveMutex_is_own),
                CXX::Interface::direct_method("lifecycle_lock", funs_RecursiveMutex_lifecycle_lock),
                CXX::Interface::direct_method("sequence_lock", funs_RecursiveMutex_sequence_lock)
            );
            CXX::Interface::typeVTable<typed_lgr<TaskRecursiveMutex>>() = define_RecursiveMutex;
        }

#pragma endregion
#pragma region Semaphore

        AttachAFun(funs_Semaphore_lock, 1, {
            auto& class_ = *CXX::Interface::getExtractAs<typed_lgr<TaskSemaphore>>(args[0], define_Semaphore);
            class_.lock();
        });
        AttachAFun(funs_Semaphore_release, 1, {
            auto& class_ = *CXX::Interface::getExtractAs<typed_lgr<TaskSemaphore>>(args[0], define_Semaphore);
            class_.release();
        });
        AttachAFun(funs_Semaphore_release_all, 1, {
            auto& class_ = *CXX::Interface::getExtractAs<typed_lgr<TaskSemaphore>>(args[0], define_Semaphore);
            class_.release_all();
        });
        AttachAFun(funs_Semaphore_try_lock, 1, {
            auto& class_ = *CXX::Interface::getExtractAs<typed_lgr<TaskSemaphore>>(args[0], define_Semaphore);
            if (len == 1)
                return class_.try_lock();
            else
                return class_.try_lock_for((size_t)args[1]);
        });
        AttachAFun(funs_Semaphore_try_lock_until, 2, {
            auto& class_ = *CXX::Interface::getExtractAs<typed_lgr<TaskSemaphore>>(args[0], define_Semaphore);
            return class_.try_lock_until((std::chrono::high_resolution_clock::time_point)args[1]);
        });
        AttachAFun(funs_Semaphore_is_locked, 1, {
            auto& class_ = *CXX::Interface::getExtractAs<typed_lgr<TaskSemaphore>>(args[0], define_Semaphore);
            return class_.is_locked();
        });

        void init_Semaphore() {
            define_Semaphore = CXX::Interface::createTable<typed_lgr<TaskSemaphore>>(
                "semaphore",
                CXX::Interface::direct_method("lock", funs_Semaphore_lock),
                CXX::Interface::direct_method("release", funs_Semaphore_release),
                CXX::Interface::direct_method("release_all", funs_Semaphore_release_all),
                CXX::Interface::direct_method("try_lock", funs_Semaphore_try_lock),
                CXX::Interface::direct_method("try_lock_until", funs_Semaphore_try_lock_until),
                CXX::Interface::direct_method("is_locked", funs_Semaphore_is_locked)
            );
            CXX::Interface::typeVTable<typed_lgr<TaskSemaphore>>() = define_Semaphore;
        }

#pragma endregion
#pragma region EventSystem

        AttachAFun(funs_EventSystem_operator_add, 2, {
            auto& class_ = *CXX::Interface::getExtractAs<typed_lgr<EventSystem>>(args[0], define_EventSystem);
            CXX::excepted(args[1], VType::function);
            auto& fun = *args[1].funPtr();
            class_ += fun;
        });
        AttachAFun(funs_EventSystem_join, 2, {
            auto& class_ = *CXX::Interface::getExtractAs<typed_lgr<EventSystem>>(args[0], define_EventSystem);
            CXX::excepted(args[1], VType::function);
            auto& fun = *args[1].funPtr();
            bool as_async = len > 2 ? (bool)args[2] : false;
            EventSystem::Priority priority = len > 3 ? (EventSystem::Priority)(uint8_t)args[3] : EventSystem::Priority::avg;
            class_.join(fun, as_async, priority);
        });
        AttachAFun(funs_EventSystem_leave, 2, {
            auto& class_ = *CXX::Interface::getExtractAs<typed_lgr<EventSystem>>(args[0], define_EventSystem);
            CXX::excepted(args[1], VType::function);
            auto& fun = *args[1].funPtr();
            bool as_async = len > 2 ? (bool)args[2] : false;
            EventSystem::Priority priority = len > 3 ? (EventSystem::Priority)(uint8_t)args[3] : EventSystem::Priority::avg;
            class_.leave(fun, as_async, priority);
        });

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

        AttachAFun(funs_EventSystem_notify, 2, {
            auto& class_ = *CXX::Interface::getExtractAs<typed_lgr<EventSystem>>(args[0], define_EventSystem);
            if (len == 2)
                return class_.notify(args[1]);
            else {
                ValueItem values = __funs_EventSystem_get_values0(args, len);
                return class_.notify(values);
            }
        });
        AttachAFun(funs_EventSystem_sync_notify, 2, {
            auto& class_ = *CXX::Interface::getExtractAs<typed_lgr<EventSystem>>(args[0], define_EventSystem);
            if (len == 2)
                return class_.sync_notify(args[1]);
            else {
                ValueItem values = __funs_EventSystem_get_values0(args, len);
                return class_.sync_notify(values);
            }
        });
        AttachAFun(funs_EventSystem_await_notify, 2, {
            auto& class_ = *CXX::Interface::getExtractAs<typed_lgr<EventSystem>>(args[0], define_EventSystem);
            if (len == 2)
                return class_.await_notify(args[1]);
            else {
                ValueItem values = __funs_EventSystem_get_values0(args, len);
                return class_.await_notify(values);
            }
        });
        AttachAFun(funs_EventSystem_async_notify, 2, {
            auto& class_ = *CXX::Interface::getExtractAs<typed_lgr<EventSystem>>(args[0], define_EventSystem);
            if (len == 2)
                return class_.async_notify(args[1]);
            else {
                ValueItem values = __funs_EventSystem_get_values0(args, len);
                return class_.async_notify(values);
            }
        });
        AttachAFun(funs_EventSystem_clear, 1, {
            auto& class_ = *CXX::Interface::getExtractAs<typed_lgr<EventSystem>>(args[0], define_EventSystem);
            class_.clear();
        });

        void init_EventSystem() {
            define_EventSystem = CXX::Interface::createTable<typed_lgr<EventSystem>>(
                "event_system",
                CXX::Interface::direct_method(symbols::structures::add_operator, funs_EventSystem_operator_add),
                CXX::Interface::direct_method("join", funs_EventSystem_join),
                CXX::Interface::direct_method("leave", funs_EventSystem_leave),
                CXX::Interface::direct_method("notify", funs_EventSystem_notify),
                CXX::Interface::direct_method("sync_notify", funs_EventSystem_sync_notify),
                CXX::Interface::direct_method("await_notify", funs_EventSystem_await_notify),
                CXX::Interface::direct_method("async_notify", funs_EventSystem_async_notify),
                CXX::Interface::direct_method("clear", funs_EventSystem_clear)
            );
            CXX::Interface::typeVTable<typed_lgr<EventSystem>>() = define_EventSystem;
        }

#pragma endregion
#pragma region TaskLimiter

        AttachAFun(funs_TaskLimiter_set_max_threshold, 2, {
            auto& class_ = *CXX::Interface::getExtractAs<typed_lgr<TaskLimiter>>(args[0], define_TaskLimiter);
            class_.set_max_threshold((uint64_t)args[1]);
        });
        AttachAFun(funs_TaskLimiter_lock, 1, {
            auto& class_ = *CXX::Interface::getExtractAs<typed_lgr<TaskLimiter>>(args[0], define_TaskLimiter);
            class_.lock();
        });
        AttachAFun(funs_TaskLimiter_unlock, 1, {
            auto& class_ = *CXX::Interface::getExtractAs<typed_lgr<TaskLimiter>>(args[0], define_TaskLimiter);
            class_.unlock();
        });
        AttachAFun(funs_TaskLimiter_try_lock, 1, {
            auto& class_ = *CXX::Interface::getExtractAs<typed_lgr<TaskLimiter>>(args[0], define_TaskLimiter);
            if (len == 1)
                return class_.try_lock();
            else
                return class_.try_lock_for((size_t)args[1]);
        });
        AttachAFun(funs_TaskLimiter_try_lock_until, 2, {
            auto& class_ = *CXX::Interface::getExtractAs<typed_lgr<TaskLimiter>>(args[0], define_TaskLimiter);
            return class_.try_lock_until((std::chrono::high_resolution_clock::time_point)args[1]);
        });
        AttachAFun(funs_TaskLimiter_is_locked, 1, {
            auto& class_ = *CXX::Interface::getExtractAs<typed_lgr<TaskLimiter>>(args[0], define_TaskLimiter);
            return class_.is_locked();
        });

        void init_TaskLimiter() {
            define_TaskLimiter = CXX::Interface::createTable<typed_lgr<TaskLimiter>>(
                "task_limiter",
                CXX::Interface::direct_method("set_max_threshold", funs_TaskLimiter_set_max_threshold),
                CXX::Interface::direct_method("lock", funs_TaskLimiter_lock),
                CXX::Interface::direct_method("unlock", funs_TaskLimiter_unlock),
                CXX::Interface::direct_method("try_lock", funs_TaskLimiter_try_lock),
                CXX::Interface::direct_method("try_lock_until", funs_TaskLimiter_try_lock_until),
                CXX::Interface::direct_method("is_locked", funs_TaskLimiter_is_locked)
            );
            CXX::Interface::typeVTable<typed_lgr<TaskLimiter>>() = define_TaskLimiter;
        }

#pragma endregion
#pragma region TaskQuery

        AttachAFun(funs_TaskQuery_add_task, 2, {
            auto& class_ = *CXX::Interface::getExtractAs<typed_lgr<TaskQuery>>(args[0], define_TaskQuery);
            art::shared_ptr<FuncEnvironment> func;
            art::shared_ptr<FuncEnvironment> fault_func;
            std::chrono::high_resolution_clock::time_point timeout = std::chrono::high_resolution_clock::time_point::min();
            bool used_task_local = false;
            ValueItem values;
            parseArgumentsToTask<1>(args, len, func, fault_func, timeout, used_task_local, values);
            return class_.add_task(func, values, used_task_local, fault_func, timeout);
        });
        AttachAFun(funs_TaskQuery_enable, 1, {
            auto& class_ = *CXX::Interface::getExtractAs<typed_lgr<TaskQuery>>(args[0], define_TaskQuery);
            class_.enable();
        });
        AttachAFun(funs_TaskQuery_disable, 1, {
            auto& class_ = *CXX::Interface::getExtractAs<typed_lgr<TaskQuery>>(args[0], define_TaskQuery);
            class_.disable();
        });
        AttachAFun(funs_TaskQuery_in_query, 2, {
            auto& class_ = *CXX::Interface::getExtractAs<typed_lgr<TaskQuery>>(args[0], define_TaskQuery);
            CXX::excepted(args[1], VType::async_res);
            art::typed_lgr<Task>& task = *(art::typed_lgr<Task>*)args[1].getSourcePtr();
            return class_.in_query(task);
        });
        AttachAFun(funs_TaskQuery_set_max_at_execution, 2, {
            auto& class_ = *CXX::Interface::getExtractAs<typed_lgr<TaskQuery>>(args[0], define_TaskQuery);
            class_.set_max_at_execution((size_t)args[1]);
        });
        AttachAFun(funs_TaskQuery_get_max_at_execution, 1, {
            auto& class_ = *CXX::Interface::getExtractAs<typed_lgr<TaskQuery>>(args[0], define_TaskQuery);
            return class_.get_max_at_execution();
        });

        void init_TaskQuery() {
            define_TaskQuery = CXX::Interface::createTable<typed_lgr<TaskQuery>>(
                "task_query",
                CXX::Interface::direct_method("add_task", funs_TaskQuery_add_task),
                CXX::Interface::direct_method("enable", funs_TaskQuery_enable),
                CXX::Interface::direct_method("disable", funs_TaskQuery_disable),
                CXX::Interface::direct_method("in_query", funs_TaskQuery_in_query),
                CXX::Interface::direct_method("set_max_at_execution", funs_TaskQuery_set_max_at_execution),
                CXX::Interface::direct_method("get_max_at_execution", funs_TaskQuery_get_max_at_execution)
            );
            CXX::Interface::typeVTable<typed_lgr<TaskQuery>>() = define_TaskQuery;
        }

#pragma endregion
#pragma region TaskResultIterator

        struct TaskResultIterator {
            art::typed_lgr<Task> task;
            ptrdiff_t index = -1;

            bool next() {
                if (task->end_of_life) {
                    if (task->fres.results.size() > ++index)
                        return true;

                } else {
                    if (Task::has_result(task, 1 + index)) {
                        ++index;
                        return true;
                    } else {
                        ValueItem* res = Task::get_result(task, 1 + index);
                        delete res;
                        if (Task::has_result(task, 1 + index)) {
                            ++index;
                            return true;
                        }
                    }
                }
                return false;
            }

            bool prev() {
                if (index > 0) {
                    --index;
                    return true;
                }
                return false;
            }

            ValueItem get() {
                return task->fres.results[index];
            }

            TaskResultIterator begin() {
                return TaskResultIterator{task, -1};
            }

            TaskResultIterator end() {
                if (!task->end_of_life)
                    Task::await_task(task);
                return TaskResultIterator{task, (ptrdiff_t)task->fres.results.size()};
            }
        };

        AttachAFun(funs_TaskResultIterator_next, 1, {
            auto& class_ = CXX::Interface::getExtractAs<TaskResultIterator>(args[0], define_TaskResultIterator);
            return class_.next();
        });
        AttachAFun(funs_TaskResultIterator_prev, 1, {
            auto& class_ = CXX::Interface::getExtractAs<TaskResultIterator>(args[0], define_TaskResultIterator);
            return class_.prev();
        });
        AttachAFun(funs_TaskResultIterator_get, 1, {
            auto& class_ = CXX::Interface::getExtractAs<TaskResultIterator>(args[0], define_TaskResultIterator);
            return class_.get();
        });
        AttachAFun(funs_TaskResultIterator_begin, 1, {
            auto& class_ = CXX::Interface::getExtractAs<TaskResultIterator>(args[0], define_TaskResultIterator);
            return ValueItem(CXX::Interface::constructStructure<TaskResultIterator>(define_TaskResultIterator, class_.begin()), no_copy);
        });
        AttachAFun(funs_TaskResultIterator_end, 1, {
            auto& class_ = CXX::Interface::getExtractAs<TaskResultIterator>(args[0], define_TaskResultIterator);
            return ValueItem(CXX::Interface::constructStructure<TaskResultIterator>(define_TaskResultIterator, class_.end()), no_copy);
        });

        void init_TaskResultIterator() {
            define_TaskResultIterator = CXX::Interface::createTable<TaskResultIterator>(
                "task_result_iterator",
                CXX::Interface::direct_method(symbols::structures::iterable::next, funs_TaskResultIterator_next),
                CXX::Interface::direct_method(symbols::structures::iterable::prev, funs_TaskResultIterator_prev),
                CXX::Interface::direct_method(symbols::structures::iterable::get, funs_TaskResultIterator_get),
                CXX::Interface::direct_method(symbols::structures::iterable::begin, funs_TaskResultIterator_begin),
                CXX::Interface::direct_method(symbols::structures::iterable::end, funs_TaskResultIterator_end)
            );
            CXX::Interface::typeVTable<TaskResultIterator>() = define_TaskResultIterator;
        }

#pragma endregion
#pragma region Task
        AttachAFun(funs_Task_schedule, 2, {
            Task::schedule(CXX::Interface::getExtractAs<art::typed_lgr<Task>>(args[0], define_Task), (size_t)args[1]);
        });
        AttachAFun(funs_Task_schedule_until, 2, {
            Task::schedule_until(CXX::Interface::getExtractAs<art::typed_lgr<Task>>(args[0], define_Task), (std::chrono::high_resolution_clock::time_point)args[1]);
        });
        AttachAFun(funs_Task_start, 1, {
            Task::start(CXX::Interface::getExtractAs<art::typed_lgr<Task>>(args[0], define_Task));
        });
        AttachAFun(funs_Task_yield_iterate, 1, {
            return Task::yield_iterate(CXX::Interface::getExtractAs<art::typed_lgr<Task>>(args[0], define_Task));
        });
        AttachAManagedFun(funs_Task_get_result, 1, {
            if (len >= 2)
                return Task::get_result(CXX::Interface::getExtractAs<art::typed_lgr<Task>>(args[0], define_Task), (size_t)args[1]);
            else
                return Task::get_result(CXX::Interface::getExtractAs<art::typed_lgr<Task>>(args[0], define_Task));
        });
        AttachAFun(funs_Task_has_result, 1, {
            if (len >= 2)
                return Task::has_result(CXX::Interface::getExtractAs<art::typed_lgr<Task>>(args[0], define_Task), (size_t)args[1]);
            else
                return Task::has_result(CXX::Interface::getExtractAs<art::typed_lgr<Task>>(args[0], define_Task));
        });
        AttachAFun(funs_Task_await_task, 1, {
            if (len >= 2)
                Task::await_task(CXX::Interface::getExtractAs<art::typed_lgr<Task>>(args[0], define_Task), (bool)args[1]);
            else
                Task::await_task(CXX::Interface::getExtractAs<art::typed_lgr<Task>>(args[0], define_Task));
        });
        AttachAFun(funs_Task_await_results, 1, {
            return Task::await_results(CXX::Interface::getExtractAs<art::typed_lgr<Task>>(args[0], define_Task));
        });
        AttachAFun(funs_Task_notify_cancel, 1, {
            Task::notify_cancel(CXX::Interface::getExtractAs<art::typed_lgr<Task>>(args[0], define_Task));
        });
        AttachAFun(funs_Task_set_auto_bind_worker, 2, {
            CXX::Interface::getExtractAs<art::typed_lgr<Task>>(args[0], define_Task)->set_auto_bind_worker((bool)args[1]);
        });
        AttachAFun(funs_Task_set_worker_id, 2, {
            CXX::Interface::getExtractAs<art::typed_lgr<Task>>(args[0], define_Task)->set_worker_id((uint16_t)args[1]);
        });


        AttachAFun(funs_Task_size, 1, {
            Task& task = *CXX::Interface::getExtractAs<art::typed_lgr<Task>>(args[0], define_Task);
            lock_guard lock(task.no_race);
            return task.fres.results.size();
        });

        AttachAFun(funs_Task_to_string, 1, {
            auto& task = CXX::Interface::getExtractAs<art::typed_lgr<Task>>(args[0], define_Task);
            Task::await_task(task);
            Task& task_ = *task;
            ValueItem pre_res(task_.fres.results, as_reference);
            return (art::ustring)pre_res;
        });

        ValueItem* funs_Task_to_set(ValueItem* args, uint32_t len) {
            CXX::arguments_range(len, 1);
            auto& task = CXX::Interface::getExtractAs<art::typed_lgr<Task>>(args[0], define_Task);
            Task::await_task(task);
            std::unordered_set<ValueItem, art::hash<ValueItem>> res;
            for (auto& i : task->fres.results)
                res.insert(i);
            return new ValueItem(res);
        }

        template <typename T>
        AttachAFun(funs_Task_to_, 1, {
            auto& task = CXX::Interface::getExtractAs<art::typed_lgr<Task>>(args[0], define_Task);
            if (task->fres.results.size() != 1) {
                ValueItem* res = Task::get_result(task, 0);
                std::unique_ptr<ValueItem> res_(res);
                return (T)*res;
            } else
                return (T)task->fres.results[0];
        });

        template <typename T>
        AttachAFun(funs_Task_array_to_, 1, {
            auto& task = CXX::Interface::getExtractAs<art::typed_lgr<Task>>(args[0], define_Task);
            Task::await_task(task);
            if (task->fres.results.size() > UINT32_MAX)
                throw InvalidCast("Task internal result array is too large to convert to an array");
            else if constexpr (std::is_same_v<T, ValueItem>)
                return ValueItem(task->fres.results.data(), task->fres.results.size());
            else {
                size_t len;
                auto res = task->fres.results.convert<T>([](ValueItem& item) {
                                                 return (T)item;
                                             })
                               .take_raw(len);
                return ValueItem(res, len, no_copy);
            }
        });
        AttachAFun(funs_Task_begin, 1, {
            auto& task = CXX::Interface::getExtractAs<art::typed_lgr<Task>>(args[0], define_Task);
            return CXX::Interface::constructStructure<TaskResultIterator>(define_TaskResultIterator, task, -1);
        });
        AttachAFun(funs_Task_end, 1, {
            auto& task = CXX::Interface::getExtractAs<art::typed_lgr<Task>>(args[0], define_Task);
            Task::await_task(task);
            return CXX::Interface::constructStructure<TaskResultIterator>(define_TaskResultIterator, task, task->fres.results.size());
        });

        void init_Task() {
            define_Task = CXX::Interface::createTable<art::typed_lgr<Task>>(
                "task",
                CXX::Interface::direct_method("schedule", funs_Task_schedule),
                CXX::Interface::direct_method("schedule_until", funs_Task_schedule_until),
                CXX::Interface::direct_method("start", funs_Task_start),
                CXX::Interface::direct_method("yield_iterate", funs_Task_yield_iterate),
                CXX::Interface::direct_method("get_result", funs_Task_get_result),
                CXX::Interface::direct_method(symbols::structures::index_operator, funs_Task_get_result),
                CXX::Interface::direct_method(symbols::structures::size, funs_Task_size),
                CXX::Interface::direct_method("has_result", funs_Task_has_result),
                CXX::Interface::direct_method("await_task", funs_Task_await_task),
                CXX::Interface::direct_method("await_results", funs_Task_await_results),
                CXX::Interface::direct_method("notify_cancel", funs_Task_notify_cancel),
                CXX::Interface::direct_method("set_auto_bind_worker", funs_Task_set_auto_bind_worker),
                CXX::Interface::direct_method("set_worker_id", funs_Task_set_worker_id),
                CXX::Interface::direct_method(symbols::structures::convert::to_uarr, funs_Task_await_results),
                CXX::Interface::direct_method(symbols::structures::convert::to_string, funs_Task_to_string),
                CXX::Interface::direct_method(symbols::structures::convert::to_ui8, funs_Task_to_<uint8_t>),
                CXX::Interface::direct_method(symbols::structures::convert::to_ui16, funs_Task_to_<uint16_t>),
                CXX::Interface::direct_method(symbols::structures::convert::to_ui32, funs_Task_to_<uint32_t>),
                CXX::Interface::direct_method(symbols::structures::convert::to_ui64, funs_Task_to_<uint64_t>),
                CXX::Interface::direct_method(symbols::structures::convert::to_i8, funs_Task_to_<int8_t>),
                CXX::Interface::direct_method(symbols::structures::convert::to_i16, funs_Task_to_<int16_t>),
                CXX::Interface::direct_method(symbols::structures::convert::to_i32, funs_Task_to_<int32_t>),
                CXX::Interface::direct_method(symbols::structures::convert::to_i64, funs_Task_to_<int64_t>),
                CXX::Interface::direct_method(symbols::structures::convert::to_float, funs_Task_to_<float>),
                CXX::Interface::direct_method(symbols::structures::convert::to_double, funs_Task_to_<double>),
                CXX::Interface::direct_method(symbols::structures::convert::to_boolean, funs_Task_to_<bool>),
                CXX::Interface::direct_method(symbols::structures::convert::to_timepoint, funs_Task_to_<std::chrono::high_resolution_clock::time_point>),
                CXX::Interface::direct_method(symbols::structures::convert::to_type_identifier, funs_Task_to_<ValueMeta>),
                CXX::Interface::direct_method(symbols::structures::convert::to_function, funs_Task_to_<art::shared_ptr<FuncEnvironment>&>),
                CXX::Interface::direct_method(symbols::structures::convert::to_map, funs_Task_to_<std::unordered_map<ValueItem, ValueItem, art::hash<ValueItem>>&>),
                CXX::Interface::direct_method(symbols::structures::convert::to_set, funs_Task_to_set),
                CXX::Interface::direct_method(symbols::structures::convert::to_ui8_arr, funs_Task_array_to_<uint8_t>),
                CXX::Interface::direct_method(symbols::structures::convert::to_ui16_arr, funs_Task_array_to_<uint16_t>),
                CXX::Interface::direct_method(symbols::structures::convert::to_ui32_arr, funs_Task_array_to_<uint32_t>),
                CXX::Interface::direct_method(symbols::structures::convert::to_ui64_arr, funs_Task_array_to_<uint64_t>),
                CXX::Interface::direct_method(symbols::structures::convert::to_i8_arr, funs_Task_array_to_<int8_t>),
                CXX::Interface::direct_method(symbols::structures::convert::to_i16_arr, funs_Task_array_to_<int16_t>),
                CXX::Interface::direct_method(symbols::structures::convert::to_i32_arr, funs_Task_array_to_<int32_t>),
                CXX::Interface::direct_method(symbols::structures::convert::to_i64_arr, funs_Task_array_to_<int64_t>),
                CXX::Interface::direct_method(symbols::structures::convert::to_float_arr, funs_Task_array_to_<float>),
                CXX::Interface::direct_method(symbols::structures::convert::to_double_arr, funs_Task_array_to_<double>),
                CXX::Interface::direct_method(symbols::structures::convert::to_faarr, funs_Task_array_to_<ValueItem>),
                CXX::Interface::direct_method(symbols::structures::iterable::begin, funs_Task_begin),
                CXX::Interface::direct_method(symbols::structures::iterable::end, funs_Task_end)
            );
            CXX::Interface::typeVTable<art::typed_lgr<Task>>() = define_Task;
        }

#pragma endregion
#pragma region TaskGroup

        AttachAFun(funs_TaskGroup_start, 1, {
            Task::start(CXX::Interface::getExtractAs<list_array<art::typed_lgr<Task>>>(args[0], define_TaskGroup));
        });
        AttachAFun(funs_TaskGroup_await_multiple, 1, {
            switch (len) {
            case 1:
                Task::await_multiple(CXX::Interface::getExtractAs<list_array<art::typed_lgr<Task>>>(args[0], define_TaskGroup));
                break;
            case 2:
                Task::await_multiple(CXX::Interface::getExtractAs<list_array<art::typed_lgr<Task>>>(args[0], define_TaskGroup), (bool)args[1]);
                break;
            case 3:
            default: {
                bool release = (bool)args[2];
                Task::await_multiple(CXX::Interface::getExtractAs<list_array<art::typed_lgr<Task>>>(args[0], define_TaskGroup), (bool)args[1], release);
                if (release)
                    CXX::Interface::getExtractAs<list_array<art::typed_lgr<Task>>>(args[0], define_TaskGroup).clear();
                break;
            }
            }
        });
        AttachAFun(funs_TaskGroup_await_results, 1, {
            return Task::await_results(CXX::Interface::getExtractAs<list_array<art::typed_lgr<Task>>>(args[0], define_TaskGroup));
        });
        AttachAFun(funs_TaskGroup_notify_cancel, 1, {
            Task::notify_cancel(CXX::Interface::getExtractAs<list_array<art::typed_lgr<Task>>>(args[0], define_TaskGroup));
        });

        template <typename T>
        AttachAFun(funs_TaskGroup_array_to_, 1, {
            auto results = Task::await_results(CXX::Interface::getExtractAs<list_array<art::typed_lgr<Task>>>(args[0], define_TaskGroup));

            if (results.size() > UINT32_MAX)
                throw InvalidCast("Task internal result array is too large to convert to an array");
            else if constexpr (std::is_same_v<T, ValueItem>)
                return ValueItem(results.data(), results.size());
            else {
                size_t len;
                auto res = results.convert<T>([](ValueItem& item) { return (T)item; }).take_raw(len);
                return ValueItem(res, len, no_copy);
            }
        });

        ValueItem* funs_TaskGroup_to_set(ValueItem* args, uint32_t len) {
            CXX::arguments_range(len, 1);
            std::unordered_set<ValueItem, art::hash<ValueItem>> res;
            for (auto& i : Task::await_results(CXX::Interface::getExtractAs<list_array<art::typed_lgr<Task>>>(args[0], define_TaskGroup)))
                res.insert(i);
            return new ValueItem(res);
        }

        void ___createProxy_TaskGroup__push_item(list_array<art::typed_lgr<Task>>& tasks, ValueItem& item) {
            switch (item.meta.vtype) {
            case VType::async_res:
                tasks.push_back((art::typed_lgr<Task>&)item);
                break;
            case VType::struct_: {
                Structure& str = (Structure&)item;
                if (str.get_vtable() == define_Task) {
                    tasks.push_back(CXX::Interface::getAs<art::typed_lgr<Task>>(str));
                    break;
                } else if (str.get_vtable() == define_TaskGroup) {
                    tasks.push_back(CXX::Interface::getAs<list_array<art::typed_lgr<Task>>>(str));
                    break;
                } else
                    break;
            }
            case VType::uarr: {
                list_array<ValueItem>& arr = *(list_array<ValueItem>*)item.getSourcePtr();
                for (auto& it : arr)
                    ___createProxy_TaskGroup__push_item(tasks, it);
                break;
            }
            case VType::faarr:
            case VType::saarr: {
                ValueItem* arr = (ValueItem*)item.getSourcePtr();
                uint32_t len = item.meta.val_len;
                for (uint32_t i = 0; i < len; i++)
                    ___createProxy_TaskGroup__push_item(tasks, arr[i]);
                break;
            }
            case VType::set: {
                std::unordered_set<ValueItem, art::hash<ValueItem>>& set = (std::unordered_set<ValueItem, art::hash<ValueItem>>&)item;
                for (auto& it : set)
                    ___createProxy_TaskGroup__push_item(tasks, const_cast<ValueItem&>(it));
                break;
            }
            case VType::map: {
                std::unordered_map<ValueItem, ValueItem, art::hash<ValueItem>>& map = (std::unordered_map<ValueItem, ValueItem, art::hash<ValueItem>>&)item;
                for (auto& it : map)
                    ___createProxy_TaskGroup__push_item(tasks, it.second);
                break;
            }
            default:
                break;
            }
        }

        AttachAFun(funs_TaskGroup_add, 1, {
            auto& tasks = CXX::Interface::getExtractAs<list_array<art::typed_lgr<Task>>>(args[0], define_TaskGroup);
            for (uint32_t i = 0; i < len; i++)
                ___createProxy_TaskGroup__push_item(tasks, args[i]);
        });

        void init_TaskGroup() {
            define_TaskGroup = CXX::Interface::createTable<list_array<art::typed_lgr<Task>>>(
                "task_group",
                CXX::Interface::direct_method("start", funs_TaskGroup_start),
                CXX::Interface::direct_method("await_multiple", funs_TaskGroup_await_multiple),
                CXX::Interface::direct_method("await_results", funs_TaskGroup_await_results),
                CXX::Interface::direct_method("notify_cancel", funs_TaskGroup_notify_cancel),
                CXX::Interface::direct_method(symbols::structures::convert::to_ui8_arr, funs_TaskGroup_array_to_<uint8_t>),
                CXX::Interface::direct_method(symbols::structures::convert::to_ui16_arr, funs_TaskGroup_array_to_<uint16_t>),
                CXX::Interface::direct_method(symbols::structures::convert::to_ui32_arr, funs_TaskGroup_array_to_<uint32_t>),
                CXX::Interface::direct_method(symbols::structures::convert::to_ui64_arr, funs_TaskGroup_array_to_<uint64_t>),
                CXX::Interface::direct_method(symbols::structures::convert::to_i8_arr, funs_TaskGroup_array_to_<int8_t>),
                CXX::Interface::direct_method(symbols::structures::convert::to_i16_arr, funs_TaskGroup_array_to_<int16_t>),
                CXX::Interface::direct_method(symbols::structures::convert::to_i32_arr, funs_TaskGroup_array_to_<int32_t>),
                CXX::Interface::direct_method(symbols::structures::convert::to_i64_arr, funs_TaskGroup_array_to_<int64_t>),
                CXX::Interface::direct_method(symbols::structures::convert::to_float_arr, funs_TaskGroup_array_to_<float>),
                CXX::Interface::direct_method(symbols::structures::convert::to_double_arr, funs_TaskGroup_array_to_<double>),
                CXX::Interface::direct_method(symbols::structures::convert::to_faarr, funs_TaskGroup_array_to_<ValueItem>),
                CXX::Interface::direct_method(symbols::structures::convert::to_set, funs_TaskGroup_to_set),
                CXX::Interface::direct_method(symbols::structures::convert::to_uarr, funs_TaskGroup_await_multiple),
                CXX::Interface::direct_method(symbols::structures::add_operator, funs_TaskGroup_add)
            );
            CXX::Interface::typeVTable<list_array<art::typed_lgr<Task>>>() = define_TaskGroup;
        }

#pragma endregion

        namespace constructor {
            ValueItem* createProxy_ConditionVariable(ValueItem*, uint32_t) {
                return new ValueItem(CXX::Interface::constructStructure<typed_lgr<TaskConditionVariable>>(define_ConditionVariable, new TaskConditionVariable()), no_copy);
            }

            ValueItem* createProxy_Mutex(ValueItem*, uint32_t) {
                return new ValueItem(CXX::Interface::constructStructure<typed_lgr<TaskMutex>>(define_Mutex, new TaskMutex()), no_copy);
            }

            ValueItem* createProxy_RecursiveMutex(ValueItem*, uint32_t) {
                return new ValueItem(CXX::Interface::constructStructure<typed_lgr<TaskRecursiveMutex>>(define_RecursiveMutex, new TaskRecursiveMutex()), no_copy);
            }

            ValueItem* createProxy_Semaphore(ValueItem*, uint32_t) {
                return new ValueItem(CXX::Interface::constructStructure<typed_lgr<TaskSemaphore>>(define_Semaphore, new TaskSemaphore()), no_copy);
            }

            ValueItem* createProxy_EventSystem(ValueItem* val, uint32_t len) {
                return new ValueItem(CXX::Interface::constructStructure<typed_lgr<EventSystem>>(define_EventSystem, new EventSystem()), no_copy);
            }

            ValueItem* createProxy_TaskLimiter(ValueItem* val, uint32_t len) {
                return new ValueItem(CXX::Interface::constructStructure<typed_lgr<TaskLimiter>>(define_TaskLimiter, new TaskLimiter()), no_copy);
            }

            ValueItem* createProxy_TaskQuery(ValueItem* val, uint32_t len) {
                return new ValueItem(CXX::Interface::constructStructure<typed_lgr<TaskQuery>>(define_TaskQuery, new TaskQuery(len ? (size_t)*val : 0)), no_copy);
            }

            AttachAFun(construct_Task, 1, {
                if (args[0].meta.vtype == VType::async_res)
                    return ValueItem(CXX::Interface::constructStructure<art::typed_lgr<Task>>(define_Task, (art::typed_lgr<Task>&)args[0]), no_copy);
                else if (args[0].meta.vtype == VType::function) {
                    art::shared_ptr<FuncEnvironment> func;
                    art::shared_ptr<FuncEnvironment> fault_func;
                    std::chrono::high_resolution_clock::time_point timeout = std::chrono::high_resolution_clock::time_point::min();
                    bool used_task_local = false;
                    ValueItem values;
                    parseArgumentsToTask<0>(args, len, func, fault_func, timeout, used_task_local, values);
                    return ValueItem(CXX::Interface::constructStructure<art::typed_lgr<Task>>(define_Task, new Task(func, values, used_task_local, fault_func, timeout)), no_copy);
                } else
                    return ValueItem(CXX::Interface::constructStructure<art::typed_lgr<Task>>(define_Task, CXX::Interface::getExtractAs<art::typed_lgr<Task>>(args[0], define_Task)), no_copy);
            });

            ValueItem* createProxy_TaskGroup(ValueItem* val, uint32_t len) {
                if (!len)
                    return new ValueItem(CXX::Interface::constructStructure<list_array<art::typed_lgr<Task>>>(define_TaskGroup), no_copy);
                else {
                    list_array<art::typed_lgr<Task>> tasks;
                    tasks.reserve_push_back(len);
                    for (uint32_t i = 0; i < len; i++)
                        ___createProxy_TaskGroup__push_item(tasks, val[i]);
                    return new ValueItem(CXX::Interface::constructStructure<list_array<art::typed_lgr<Task>>>(define_TaskGroup, std::move(tasks)), no_copy);
                }
            }
        }

        ValueItem* createThread(ValueItem* vals, uint32_t len) {
            CXX::arguments_range(len, 1);
            art::shared_ptr<FuncEnvironment> func = *vals->funPtr();

            if (len != 1) {
                ValueItem* movedArgs = new ValueItem[len - 1];
                vals++;
                len--;
                uint32_t i = 0;
                for (; len--; i++)
                    movedArgs[i] = std::move(*vals++);
                art::thread(
                    [](art::shared_ptr<FuncEnvironment> func, ValueItem* args, uint32_t len) {
                        std::unique_ptr<ValueItem[]> args_hold(args);
                        CXX::aCall(func, args, len);
                    },
                    func,
                    movedArgs,
                    i
                )
                    .detach();
            } else {
                art::thread(
                    [](art::shared_ptr<FuncEnvironment> func) {
                        CXX::aCall(func, nullptr, 0);
                    },
                    func
                )
                    .detach();
            }
            return nullptr;
        }

        AttachAFun(createThreadAndWait, 1, {
            ValueItem _thread = CXX::aCall(native::constructor::construct_Thread, args, len);
            return CXX::Interface::makeCall(ClassAccess::pub, _thread, "wait");
        });

        AttachAFun(_createAsyncThread__Awaiter, 1, {
            return CXX::Interface::makeCall(ClassAccess::pub, *args, "wait");
        });

        art::shared_ptr<FuncEnvironment> __createAsyncThread__Awaiter = new FuncEnvironment(_createAsyncThread__Awaiter, false);

        ValueItem* createAsyncThread(ValueItem* args, uint32_t len) {
            ValueItem awaiter_args = CXX::aCall(native::constructor::construct_Thread, args, len);
            return new ValueItem(new typed_lgr(new Task(__createAsyncThread__Awaiter, awaiter_args)), VType::async_res, no_copy);
        }

        ValueItem* createTask(ValueItem* vals, uint32_t len) {
            CXX::arguments_range(len, 1);
            art::shared_ptr<FuncEnvironment> func;
            art::shared_ptr<FuncEnvironment> fault_func;
            std::chrono::high_resolution_clock::time_point timeout;
            bool used_task_local;
            ValueItem args;
            parseArgumentsToTask<0>(vals, len, func, fault_func, timeout, used_task_local, args);
            return new ValueItem(new typed_lgr(new Task(func, args, used_task_local, fault_func, timeout)), VType::async_res, no_copy);
        }

        namespace this_task {
            ValueItem* yield(ValueItem*, uint32_t) {
                Task::yield();
                return nullptr;
            }

            ValueItem* yield_result(ValueItem* result, uint32_t args_len) {
                for (uint32_t i = 0; i < args_len; i++)
                    Task::result(new ValueItem(result[i]));
                return nullptr;
            }

            ValueItem* sleep(ValueItem* args, uint32_t len) {
                Task::sleep((size_t)args[0]);
                return nullptr;
            }

            ValueItem* sleep_until(ValueItem* args, uint32_t len) {
                Task::sleep_until((std::chrono::high_resolution_clock::time_point)args[0]);
                return nullptr;
            }

            ValueItem* task_id(ValueItem*, uint32_t) {
                return new ValueItem(Task::task_id());
            }

            ValueItem* check_cancellation(ValueItem*, uint32_t) {
                Task::check_cancellation();
                return nullptr;
            }

            ValueItem* self_cancel(ValueItem*, uint32_t) {
                Task::self_cancel();
                return nullptr;
            }

            ValueItem* is_task(ValueItem*, uint32_t) {
                return new ValueItem(Task::is_task());
            }
        }

        namespace task_runtime {
            ValueItem* clean_up(ValueItem*, uint32_t) {
                Task::clean_up();
                return nullptr;
            }

            ValueItem* create_executor(ValueItem* args, uint32_t len) {
                Task::create_executor(len ? (size_t)args[0] : 1);
                return nullptr;
            }

            ValueItem* total_executors(ValueItem*, uint32_t) {
                return new ValueItem(Task::total_executors());
            }

            ValueItem* reduce_executor(ValueItem* args, uint32_t len) {
                Task::reduce_executor(len ? (size_t)args[0] : 1);
                return nullptr;
            }

            ValueItem* become_task_executor(ValueItem*, uint32_t) {
                Task::become_task_executor();
                return nullptr;
            }

            ValueItem* await_no_tasks(ValueItem* args, uint32_t len) {
                Task::await_no_tasks(len ? (bool)args[0] : false);
                return nullptr;
            }

            ValueItem* await_end_tasks(ValueItem* args, uint32_t len) {
                Task::await_end_tasks(len ? (bool)args[0] : false);
                return nullptr;
            }

            ValueItem* explicitStartTimer(ValueItem*, uint32_t) {
                Task::explicitStartTimer();
                return nullptr;
            }

            AttachAFun(create_bind_only_executor, 2, {
                return Task::create_bind_only_executor((uint16_t)args[0], (bool)args[1]);
            });
            AttachAFun(close_bind_only_executor, 2, {
                Task::close_bind_only_executor((uint16_t)args[0]);
            });
        }

        namespace atomic {

            template <typename T>
            class AtomicBasic : std::atomic<T> {
            public:
                static inline AttachAVirtualTable* virtual_table = nullptr;

                AtomicBasic(T val)
                    : std::atomic<T>(val) {}

                AtomicBasic()
                    : std::atomic<T>() {}

                AtomicBasic(const AtomicBasic& other)
                    : std::atomic<T>(other.load()) {}

                AtomicBasic& operator=(const AtomicBasic& other) {
                    this->store(other.load());
                    return *this;
                }

                static AttachAFun(__add, 2, {
                    if constexpr ((std::is_integral_v<T> || std::is_floating_point_v<T>)&&!std::is_same_v<T, bool>) {
                        auto& self = CXX::Interface::getExtractAs<AtomicBasic<T>>(args[0], virtual_table);
                        self += (T)args[1];
                    }
                });
                static AttachAFun(__sub, 2, {
                    if constexpr ((std::is_integral_v<T> || std::is_floating_point_v<T>)&&!std::is_same_v<T, bool>) {
                        auto& self = CXX::Interface::getExtractAs<AtomicBasic<T>>(args[0], virtual_table);
                        self -= (T)args[1];
                    }
                });
                static AttachAFun(__and, 2, {
                    if constexpr (std::is_integral_v<T> && !std::is_floating_point_v<T> && !std::is_same_v<T, bool>) {
                        auto& self = CXX::Interface::getExtractAs<AtomicBasic<T>>(args[0], virtual_table);
                        self &= (T)args[1];
                    }
                });
                static AttachAFun(__or, 2, {
                    if constexpr (std::is_integral_v<T> && !std::is_floating_point_v<T> && !std::is_same_v<T, bool>) {
                        auto& self = CXX::Interface::getExtractAs<AtomicBasic<T>>(args[0], virtual_table);
                        self |= (T)args[1];
                    }
                });
                static AttachAFun(__xor, 2, {
                    if constexpr (std::is_integral_v<T> && !std::is_floating_point_v<T> && !std::is_same_v<T, bool>) {
                        auto& self = CXX::Interface::getExtractAs<AtomicBasic<T>>(args[0], virtual_table);
                        self ^= (T)args[1];
                    }
                });

                static AttachAFun(__inc, 1, {
                    if constexpr ((std::is_integral_v<T> || std::is_floating_point_v<T>)&&!std::is_same_v<T, bool>) {
                        auto& self = CXX::Interface::getExtractAs<AtomicBasic<T>>(args[0], virtual_table);
                        self += 1;
                    }
                });
                static AttachAFun(__dec, 1, {
                    if constexpr ((std::is_integral_v<T> || std::is_floating_point_v<T>)&&!std::is_same_v<T, bool>) {
                        auto& self = CXX::Interface::getExtractAs<AtomicBasic<T>>(args[0], virtual_table);
                        self -= 1;
                    }
                });

                static AttachAFun(__not_equal, 2, {
                    auto& self = CXX::Interface::getExtractAs<AtomicBasic<T>>(args[0], virtual_table);
                    return self.load() != (T)args[1];
                });
                static AttachAFun(__equal, 2, {
                    auto& self = CXX::Interface::getExtractAs<AtomicBasic<T>>(args[0], virtual_table);
                    return self.load() == (T)args[1];
                });
                static AttachAFun(__less, 2, {
                    auto& self = CXX::Interface::getExtractAs<AtomicBasic<T>>(args[0], virtual_table);
                    return self.load() < (T)args[1];
                });
                static AttachAFun(__greater, 2, {
                    auto& self = CXX::Interface::getExtractAs<AtomicBasic<T>>(args[0], virtual_table);
                    return self.load() > (T)args[1];
                });
                static AttachAFun(__less_or_equal, 2, {
                    auto& self = CXX::Interface::getExtractAs<AtomicBasic<T>>(args[0], virtual_table);
                    return self.load() <= (T)args[1];
                });
                static AttachAFun(__greater_or_equal, 2, {
                    auto& self = CXX::Interface::getExtractAs<AtomicBasic<T>>(args[0], virtual_table);
                    return self.load() >= (T)args[1];
                });

                static AttachAFun(__not, 1, {
                    auto& self = CXX::Interface::getExtractAs<AtomicBasic<T>>(args[0], virtual_table);
                    return !self.load();
                });
                static AttachAFun(__bitwise_not, 1, {
                    if constexpr (!std::is_floating_point_v<T>) {
                        auto& self = CXX::Interface::getExtractAs<AtomicBasic<T>>(args[0], virtual_table);
                        if constexpr (std::is_integral_v<T> && !std::is_same_v<T, bool>)
                            return ~self.load();
                        else
                            return !self.load();
                    }
                });
                static AttachAFun(__to_string, 1, {
                    auto& self = CXX::Interface::getExtractAs<AtomicBasic<T>>(args[0], virtual_table);
                    return (art::ustring)std::to_string(self.load());
                });
                static AttachAFun(__to_ui8, 1, {
                    auto& self = CXX::Interface::getExtractAs<AtomicBasic<T>>(args[0], virtual_table);
                    return (uint8_t)self.load();
                });
                static AttachAFun(__to_ui16, 1, {
                    auto& self = CXX::Interface::getExtractAs<AtomicBasic<T>>(args[0], virtual_table);
                    return (uint16_t)self.load();
                });
                static AttachAFun(__to_ui32, 1, {
                    auto& self = CXX::Interface::getExtractAs<AtomicBasic<T>>(args[0], virtual_table);
                    return (uint32_t)self.load();
                });
                static AttachAFun(__to_ui64, 1, {
                    auto& self = CXX::Interface::getExtractAs<AtomicBasic<T>>(args[0], virtual_table);
                    return (uint64_t)self.load();
                });
                static AttachAFun(__to_i8, 1, {
                    auto& self = CXX::Interface::getExtractAs<AtomicBasic<T>>(args[0], virtual_table);
                    return (int8_t)self.load();
                });
                static AttachAFun(__to_i16, 1, {
                    auto& self = CXX::Interface::getExtractAs<AtomicBasic<T>>(args[0], virtual_table);
                    return (int16_t)self.load();
                });
                static AttachAFun(__to_i32, 1, {
                    auto& self = CXX::Interface::getExtractAs<AtomicBasic<T>>(args[0], virtual_table);
                    return (int32_t)self.load();
                });
                static AttachAFun(__to_i64, 1, {
                    auto& self = CXX::Interface::getExtractAs<AtomicBasic<T>>(args[0], virtual_table);
                    return (int64_t)self.load();
                });
                static AttachAFun(__to_float, 1, {
                    auto& self = CXX::Interface::getExtractAs<AtomicBasic<T>>(args[0], virtual_table);
                    return (float)self.load();
                });
                static AttachAFun(__to_double, 1, {
                    auto& self = CXX::Interface::getExtractAs<AtomicBasic<T>>(args[0], virtual_table);
                    return (double)self.load();
                });
                static AttachAFun(__to_boolean, 1, {
                    auto& self = CXX::Interface::getExtractAs<AtomicBasic<T>>(args[0], virtual_table);
                    return (bool)self.load();
                });
                static AttachAFun(__to_timepoint, 1, {
                    auto& self = CXX::Interface::getExtractAs<AtomicBasic<T>>(args[0], virtual_table);
                    return std::chrono::high_resolution_clock::time_point(
                        std::chrono::duration_cast<std::chrono::high_resolution_clock::duration>(
                            std::chrono::duration<T>(self.load())
                        )
                    );
                });
                static AttachAFun(__to_type_identifier, 0, {
                    return Type_as_ValueMeta<T>();
                });

                static AttachAFun(__get, 1, {
                    auto& self = CXX::Interface::getExtractAs<AtomicBasic<T>>(args[0], virtual_table);
                    return self.load();
                });

                static AttachAFun(__set, 2, {
                    auto& self = CXX::Interface::getExtractAs<AtomicBasic<T>>(args[0], virtual_table);
                    self.store((T)args[1]);
                    return args[1];
                });

                static void init() {
                    if constexpr (std::is_integral_v<T> && !std::is_floating_point_v<T> && !std::is_same_v<T, bool>) {
                        art::ustring type_name;
                        if constexpr (std::is_same_v<T, int8_t>)
                            type_name = "atomic_i8";
                        else if constexpr (std::is_same_v<T, int16_t>)
                            type_name = "atomic_i16";
                        else if constexpr (std::is_same_v<T, int32_t>)
                            type_name = "atomic_i32";
                        else if constexpr (std::is_same_v<T, int64_t>)
                            type_name = "atomic_i64";
                        else if constexpr (std::is_same_v<T, uint8_t>)
                            type_name = "atomic_ui8";
                        else if constexpr (std::is_same_v<T, uint16_t>)
                            type_name = "atomic_ui16";
                        else if constexpr (std::is_same_v<T, uint32_t>)
                            type_name = "atomic_ui32";
                        else if constexpr (std::is_same_v<T, uint64_t>)
                            type_name = "atomic_ui64";
                        else
                            type_name = "atomic_x";
                        virtual_table = CXX::Interface::createTable<AtomicBasic<T>>(type_name, CXX::Interface::direct_method(symbols::structures::add_operator, __add), CXX::Interface::direct_method(symbols::structures::subtract_operator, __sub), CXX::Interface::direct_method(symbols::structures::bitwise_and_operator, __and), CXX::Interface::direct_method(symbols::structures::bitwise_or_operator, __or), CXX::Interface::direct_method(symbols::structures::bitwise_xor_operator, __xor), CXX::Interface::direct_method(symbols::structures::increment_operator, __inc), CXX::Interface::direct_method(symbols::structures::decrement_operator, __dec), CXX::Interface::direct_method(symbols::structures::not_equal_operator, __not_equal), CXX::Interface::direct_method(symbols::structures::equal_operator, __equal), CXX::Interface::direct_method(symbols::structures::less_operator, __less), CXX::Interface::direct_method(symbols::structures::greater_operator, __greater), CXX::Interface::direct_method(symbols::structures::less_or_equal_operator, __less_or_equal), CXX::Interface::direct_method(symbols::structures::greater_or_equal_operator, __greater_or_equal), CXX::Interface::direct_method(symbols::structures::not_operator, __not), CXX::Interface::direct_method(symbols::structures::bitwise_not_operator, __bitwise_not), CXX::Interface::direct_method(symbols::structures::convert::to_string, __to_string), CXX::Interface::direct_method(symbols::structures::convert::to_ui8, __to_ui8), CXX::Interface::direct_method(symbols::structures::convert::to_ui16, __to_ui16), CXX::Interface::direct_method(symbols::structures::convert::to_ui32, __to_ui32), CXX::Interface::direct_method(symbols::structures::convert::to_ui64, __to_ui64), CXX::Interface::direct_method(symbols::structures::convert::to_i8, __to_i8), CXX::Interface::direct_method(symbols::structures::convert::to_i16, __to_i16), CXX::Interface::direct_method(symbols::structures::convert::to_i32, __to_i32), CXX::Interface::direct_method(symbols::structures::convert::to_i64, __to_i64), CXX::Interface::direct_method(symbols::structures::convert::to_float, __to_float), CXX::Interface::direct_method(symbols::structures::convert::to_double, __to_double), CXX::Interface::direct_method(symbols::structures::convert::to_boolean, __to_boolean), CXX::Interface::direct_method(symbols::structures::convert::to_timepoint, __to_timepoint), CXX::Interface::direct_method(symbols::structures::convert::to_type_identifier, __to_type_identifier), CXX::Interface::direct_method("get", __get), CXX::Interface::direct_method("set", __set));
                    } else if constexpr (std::is_floating_point_v<T>) {
                        virtual_table = CXX::Interface::createTable<AtomicBasic<T>>(std::is_same_v<T, float> ? "atomic_float" : "atomic_double", CXX::Interface::direct_method(symbols::structures::add_operator, __add), CXX::Interface::direct_method(symbols::structures::subtract_operator, __sub), CXX::Interface::direct_method(symbols::structures::increment_operator, __inc), CXX::Interface::direct_method(symbols::structures::decrement_operator, __dec), CXX::Interface::direct_method(symbols::structures::not_equal_operator, __not_equal), CXX::Interface::direct_method(symbols::structures::equal_operator, __equal), CXX::Interface::direct_method(symbols::structures::less_operator, __less), CXX::Interface::direct_method(symbols::structures::greater_operator, __greater), CXX::Interface::direct_method(symbols::structures::less_or_equal_operator, __less_or_equal), CXX::Interface::direct_method(symbols::structures::greater_or_equal_operator, __greater_or_equal), CXX::Interface::direct_method(symbols::structures::not_operator, __not), CXX::Interface::direct_method(symbols::structures::convert::to_string, __to_string), CXX::Interface::direct_method(symbols::structures::convert::to_ui8, __to_ui8), CXX::Interface::direct_method(symbols::structures::convert::to_ui16, __to_ui16), CXX::Interface::direct_method(symbols::structures::convert::to_ui32, __to_ui32), CXX::Interface::direct_method(symbols::structures::convert::to_ui64, __to_ui64), CXX::Interface::direct_method(symbols::structures::convert::to_i8, __to_i8), CXX::Interface::direct_method(symbols::structures::convert::to_i16, __to_i16), CXX::Interface::direct_method(symbols::structures::convert::to_i32, __to_i32), CXX::Interface::direct_method(symbols::structures::convert::to_i64, __to_i64), CXX::Interface::direct_method(symbols::structures::convert::to_float, __to_float), CXX::Interface::direct_method(symbols::structures::convert::to_double, __to_double), CXX::Interface::direct_method(symbols::structures::convert::to_boolean, __to_boolean), CXX::Interface::direct_method(symbols::structures::convert::to_timepoint, __to_timepoint), CXX::Interface::direct_method(symbols::structures::convert::to_type_identifier, __to_type_identifier), CXX::Interface::direct_method("get", __get), CXX::Interface::direct_method("set", __set));
                    } else {
                        virtual_table = CXX::Interface::createTable<AtomicBasic<T>>("atomic_boolean", CXX::Interface::direct_method(symbols::structures::not_equal_operator, __not_equal), CXX::Interface::direct_method(symbols::structures::equal_operator, __equal), CXX::Interface::direct_method(symbols::structures::not_operator, __not), CXX::Interface::direct_method(symbols::structures::bitwise_not_operator, __bitwise_not), CXX::Interface::direct_method(symbols::structures::convert::to_string, __to_string), CXX::Interface::direct_method(symbols::structures::convert::to_ui8, __to_ui8), CXX::Interface::direct_method(symbols::structures::convert::to_ui16, __to_ui16), CXX::Interface::direct_method(symbols::structures::convert::to_ui32, __to_ui32), CXX::Interface::direct_method(symbols::structures::convert::to_ui64, __to_ui64), CXX::Interface::direct_method(symbols::structures::convert::to_i8, __to_i8), CXX::Interface::direct_method(symbols::structures::convert::to_i16, __to_i16), CXX::Interface::direct_method(symbols::structures::convert::to_i32, __to_i32), CXX::Interface::direct_method(symbols::structures::convert::to_i64, __to_i64), CXX::Interface::direct_method(symbols::structures::convert::to_float, __to_float), CXX::Interface::direct_method(symbols::structures::convert::to_double, __to_double), CXX::Interface::direct_method(symbols::structures::convert::to_boolean, __to_boolean), CXX::Interface::direct_method(symbols::structures::convert::to_timepoint, __to_timepoint), CXX::Interface::direct_method(symbols::structures::convert::to_type_identifier, __to_type_identifier), CXX::Interface::direct_method("get", __get), CXX::Interface::direct_method("set", __set));
                    }
                    CXX::Interface::typeVTable<AtomicBasic<T>>() = virtual_table;
                }
            };

            struct AtomicObject {
                static inline AttachAVirtualTable* virtual_table = nullptr;
                ValueItem value;
                TaskRecursiveMutex mutex;

                AtomicObject(const ValueItem& v) {
                    *this = v;
                }

                AtomicObject(ValueItem&& v) {
                    *this = std::move(v);
                }

                AtomicObject(const AtomicObject& other) {
                    *this = other;
                }

                AtomicObject(AtomicObject&& other) {
                    *this = std::move(other);
                }

                AtomicObject& operator=(const AtomicObject& other) {
                    unique_lock<TaskRecursiveMutex> object_lock(const_cast<AtomicObject&>(other).mutex);
                    ValueItem get(other.value);
                    object_lock.unlock();
                    lock_guard<TaskRecursiveMutex> lock(mutex);
                    value = std::move(get);
                    return *this;
                }

                AtomicObject& operator=(AtomicObject&& other) {
                    unique_lock<TaskRecursiveMutex> object_lock(other.mutex);
                    ValueItem get(std::move(other.value));
                    object_lock.unlock();
                    lock_guard<TaskRecursiveMutex> lock(mutex);
                    value = std::move(get);
                    return *this;
                }

                AtomicObject& operator=(const ValueItem& other) {
                    lock_guard<TaskRecursiveMutex> lock(mutex);
                    value = other;
                    return *this;
                }

                AtomicObject& operator=(ValueItem&& other) {
                    lock_guard<TaskRecursiveMutex> lock(mutex);
                    value = std::move(other);
                    return *this;
                }

                static AttachAFun(__less, 2, {
                    auto& self = CXX::Interface::getExtractAs<AtomicObject>(args[0], virtual_table);
                    lock_guard<TaskRecursiveMutex> lock(self.mutex);
                    auto& cmp = args[1];
                    return self.value < cmp;
                });

                static AttachAFun(__greater, 2, {
                    auto& self = CXX::Interface::getExtractAs<AtomicObject>(args[0], virtual_table);
                    lock_guard<TaskRecursiveMutex> lock(self.mutex);
                    auto& cmp = args[1];
                    return self.value > cmp;
                });

                static AttachAFun(__equal, 2, {
                    auto& self = CXX::Interface::getExtractAs<AtomicObject>(args[0], virtual_table);
                    lock_guard<TaskRecursiveMutex> lock(self.mutex);
                    auto& cmp = args[1];
                    return self.value == cmp;
                });

                static AttachAFun(__not_equal, 2, {
                    auto& self = CXX::Interface::getExtractAs<AtomicObject>(args[0], virtual_table);
                    lock_guard<TaskRecursiveMutex> lock(self.mutex);
                    auto& cmp = args[1];
                    return self.value != cmp;
                });

                static AttachAFun(__greater_equal, 2, {
                    auto& self = CXX::Interface::getExtractAs<AtomicObject>(args[0], virtual_table);
                    lock_guard<TaskRecursiveMutex> lock(self.mutex);
                    auto& cmp = args[1];
                    return self.value >= cmp;
                });

                static AttachAFun(__less_equal, 2, {
                    auto& self = CXX::Interface::getExtractAs<AtomicObject>(args[0], virtual_table);
                    lock_guard<TaskRecursiveMutex> lock(self.mutex);
                    auto& cmp = args[1];
                    return self.value <= cmp;
                });


                static AttachAFun(__add, 2, {
                    auto& self = CXX::Interface::getExtractAs<AtomicObject>(args[0], virtual_table);
                    lock_guard<TaskRecursiveMutex> lock(self.mutex);
                    auto& set = args[1];
                    self.value += set;
                });

                static AttachAFun(__sub, 2, {
                    auto& self = CXX::Interface::getExtractAs<AtomicObject>(args[0], virtual_table);
                    lock_guard<TaskRecursiveMutex> lock(self.mutex);
                    auto& set = args[1];
                    self.value -= set;
                });

                static AttachAFun(__mul, 2, {
                    auto& self = CXX::Interface::getExtractAs<AtomicObject>(args[0], virtual_table);
                    lock_guard<TaskRecursiveMutex> lock(self.mutex);
                    auto& set = args[1];
                    self.value *= set;
                });

                static AttachAFun(__div, 2, {
                    auto& self = CXX::Interface::getExtractAs<AtomicObject>(args[0], virtual_table);
                    lock_guard<TaskRecursiveMutex> lock(self.mutex);
                    auto& set = args[1];
                    self.value /= set;
                });

                static AttachAFun(__mod, 2, {
                    auto& self = CXX::Interface::getExtractAs<AtomicObject>(args[0], virtual_table);
                    auto& set = args[1];
                    lock_guard<TaskRecursiveMutex> lock(self.mutex);
                    self.value %= set;
                });

                static AttachAFun(__xor, 2, {
                    auto& self = CXX::Interface::getExtractAs<AtomicObject>(args[0], virtual_table);
                    lock_guard<TaskRecursiveMutex> lock(self.mutex);
                    auto& set = args[1];
                    self.value ^= set;
                });

                static AttachAFun(__and, 2, {
                    auto& self = CXX::Interface::getExtractAs<AtomicObject>(args[0], virtual_table);
                    auto& set = args[1];
                    lock_guard<TaskRecursiveMutex> lock(self.mutex);
                    self.value &= set;
                });

                static AttachAFun(__or, 2, {
                    auto& self = CXX::Interface::getExtractAs<AtomicObject>(args[0], virtual_table);
                    auto& set = args[1];
                    lock_guard<TaskRecursiveMutex> lock(self.mutex);
                    self.value |= set;
                });

                static AttachAFun(__lshift, 2, {
                    auto& self = CXX::Interface::getExtractAs<AtomicObject>(args[0], virtual_table);
                    auto& set = args[1];
                    lock_guard<TaskRecursiveMutex> lock(self.mutex);
                    self.value <<= set;
                });

                static AttachAFun(__rshift, 2, {
                    auto& self = CXX::Interface::getExtractAs<AtomicObject>(args[0], virtual_table);
                    auto& set = args[1];
                    lock_guard<TaskRecursiveMutex> lock(self.mutex);
                    self.value >>= set;
                });

                static AttachAFun(__inc, 1, {
                    auto& self = CXX::Interface::getExtractAs<AtomicObject>(args[0], virtual_table);
                    lock_guard<TaskRecursiveMutex> lock(self.mutex);
                    ++self.value;
                });

                static AttachAFun(__dec, 1, {
                    auto& self = CXX::Interface::getExtractAs<AtomicObject>(args[0], virtual_table);
                    lock_guard<TaskRecursiveMutex> lock(self.mutex);
                    --self.value;
                });

                static AttachAFun(__not, 1, {
                    auto& self = CXX::Interface::getExtractAs<AtomicObject>(args[0], virtual_table);
                    lock_guard<TaskRecursiveMutex> lock(self.mutex);
                    return !self.value;
                });


                static AttachAFun(__to_boolean, 1, {
                    auto& self = CXX::Interface::getExtractAs<AtomicObject>(args[0], virtual_table);
                    lock_guard<TaskRecursiveMutex> lock(self.mutex);
                    return (bool)self.value;
                });

                static AttachAFun(__to_i8, 1, {
                    auto& self = CXX::Interface::getExtractAs<AtomicObject>(args[0], virtual_table);
                    lock_guard<TaskRecursiveMutex> lock(self.mutex);
                    return (int8_t)self.value;
                });
                static AttachAFun(__to_i16, 1, {
                    auto& self = CXX::Interface::getExtractAs<AtomicObject>(args[0], virtual_table);
                    lock_guard<TaskRecursiveMutex> lock(self.mutex);
                    return (int16_t)self.value;
                });

                static AttachAFun(__to_i32, 1, {
                    auto& self = CXX::Interface::getExtractAs<AtomicObject>(args[0], virtual_table);
                    lock_guard<TaskRecursiveMutex> lock(self.mutex);
                    return (int32_t)self.value;
                });

                static AttachAFun(__to_i64, 1, {
                    auto& self = CXX::Interface::getExtractAs<AtomicObject>(args[0], virtual_table);
                    lock_guard<TaskRecursiveMutex> lock(self.mutex);
                    return (int64_t)self.value;
                });

                static AttachAFun(__to_u8, 1, {
                    auto& self = CXX::Interface::getExtractAs<AtomicObject>(args[0], virtual_table);
                    lock_guard<TaskRecursiveMutex> lock(self.mutex);
                    return (uint8_t)self.value;
                });

                static AttachAFun(__to_u16, 1, {
                    auto& self = CXX::Interface::getExtractAs<AtomicObject>(args[0], virtual_table);
                    lock_guard<TaskRecursiveMutex> lock(self.mutex);
                    return (uint16_t)self.value;
                });

                static AttachAFun(__to_u32, 1, {
                    auto& self = CXX::Interface::getExtractAs<AtomicObject>(args[0], virtual_table);
                    lock_guard<TaskRecursiveMutex> lock(self.mutex);
                    return (uint32_t)self.value;
                });

                static AttachAFun(__to_u64, 1, {
                    auto& self = CXX::Interface::getExtractAs<AtomicObject>(args[0], virtual_table);
                    lock_guard<TaskRecursiveMutex> lock(self.mutex);
                    return (uint64_t)self.value;
                });

                static AttachAFun(__to_float, 1, {
                    auto& self = CXX::Interface::getExtractAs<AtomicObject>(args[0], virtual_table);
                    lock_guard<TaskRecursiveMutex> lock(self.mutex);
                    return (float)self.value;
                });

                static AttachAFun(__to_double, 1, {
                    auto& self = CXX::Interface::getExtractAs<AtomicObject>(args[0], virtual_table);
                    lock_guard<TaskRecursiveMutex> lock(self.mutex);
                    return (double)self.value;
                });

                static AttachAFun(__to_i8_arr, 1, {
                    auto& self = CXX::Interface::getExtractAs<AtomicObject>(args[0], virtual_table);
                    lock_guard<TaskRecursiveMutex> lock(self.mutex);
                    return (array_t<int8_t>)self.value;
                });

                static AttachAFun(__to_i16_arr, 1, {
                    auto& self = CXX::Interface::getExtractAs<AtomicObject>(args[0], virtual_table);
                    lock_guard<TaskRecursiveMutex> lock(self.mutex);
                    return (array_t<int16_t>)self.value;
                });

                static AttachAFun(__to_i32_arr, 1, {
                    auto& self = CXX::Interface::getExtractAs<AtomicObject>(args[0], virtual_table);
                    lock_guard<TaskRecursiveMutex> lock(self.mutex);
                    return (array_t<int32_t>)self.value;
                });

                static AttachAFun(__to_i64_arr, 1, {
                    auto& self = CXX::Interface::getExtractAs<AtomicObject>(args[0], virtual_table);
                    lock_guard<TaskRecursiveMutex> lock(self.mutex);
                    return (array_t<int64_t>)self.value;
                });

                static AttachAFun(__to_ui8_arr, 1, {
                    auto& self = CXX::Interface::getExtractAs<AtomicObject>(args[0], virtual_table);
                    lock_guard<TaskRecursiveMutex> lock(self.mutex);
                    return (array_t<uint8_t>)self.value;
                });

                static AttachAFun(__to_ui16_arr, 1, {
                    auto& self = CXX::Interface::getExtractAs<AtomicObject>(args[0], virtual_table);
                    lock_guard<TaskRecursiveMutex> lock(self.mutex);
                    return (array_t<uint16_t>)self.value;
                });

                static AttachAFun(__to_ui32_arr, 1, {
                    auto& self = CXX::Interface::getExtractAs<AtomicObject>(args[0], virtual_table);
                    lock_guard<TaskRecursiveMutex> lock(self.mutex);
                    return (array_t<uint32_t>)self.value;
                });

                static AttachAFun(__to_ui64_arr, 1, {
                    auto& self = CXX::Interface::getExtractAs<AtomicObject>(args[0], virtual_table);
                    lock_guard<TaskRecursiveMutex> lock(self.mutex);
                    return (array_t<uint64_t>)self.value;
                });

                static AttachAFun(__to_float_arr, 1, {
                    auto& self = CXX::Interface::getExtractAs<AtomicObject>(args[0], virtual_table);
                    lock_guard<TaskRecursiveMutex> lock(self.mutex);
                    return (array_t<float>)self.value;
                });

                static AttachAFun(__to_double_arr, 1, {
                    auto& self = CXX::Interface::getExtractAs<AtomicObject>(args[0], virtual_table);
                    lock_guard<TaskRecursiveMutex> lock(self.mutex);
                    return (array_t<double>)self.value;
                });

                static AttachAFun(__to_faarr, 1, {
                    auto& self = CXX::Interface::getExtractAs<AtomicObject>(args[0], virtual_table);
                    lock_guard<TaskRecursiveMutex> lock(self.mutex);
                    return (array_t<ValueItem>)self.value;
                });

                static AttachAFun(__to_undefined_pointer, 1, {
                    auto& self = CXX::Interface::getExtractAs<AtomicObject>(args[0], virtual_table);
                    lock_guard<TaskRecursiveMutex> lock(self.mutex);
                    return (void*)self.value;
                });

                static AttachAFun(__to_string, 1, {
                    auto& self = CXX::Interface::getExtractAs<AtomicObject>(args[0], virtual_table);
                    lock_guard<TaskRecursiveMutex> lock(self.mutex);
                    return (art::ustring)self.value;
                });

                static AttachAFun(__to_uarr, 1, {
                    auto& self = CXX::Interface::getExtractAs<AtomicObject>(args[0], virtual_table);
                    lock_guard<TaskRecursiveMutex> lock(self.mutex);
                    return (list_array<ValueItem>)self.value;
                });

                static AttachAFun(__to_type_identifier, 1, {
                    auto& self = CXX::Interface::getExtractAs<AtomicObject>(args[0], virtual_table);
                    lock_guard<TaskRecursiveMutex> lock(self.mutex);
                    return (ValueMeta)self.value;
                });

                static AttachAFun(__to_timepoint, 1, {
                    auto& self = CXX::Interface::getExtractAs<AtomicObject>(args[0], virtual_table);
                    lock_guard<TaskRecursiveMutex> lock(self.mutex);
                    return (std::chrono::high_resolution_clock::time_point)self.value;
                });

                static AttachAFun(__to_map, 1, {
                    auto& self = CXX::Interface::getExtractAs<AtomicObject>(args[0], virtual_table);
                    lock_guard<TaskRecursiveMutex> lock(self.mutex);
                    return (std::unordered_map<ValueItem, ValueItem, art::hash<ValueItem>>&)self.value;
                });

                static AttachAFun(__to_set, 1, {
                    auto& self = CXX::Interface::getExtractAs<AtomicObject>(args[0], virtual_table);
                    lock_guard<TaskRecursiveMutex> lock(self.mutex);
                    return (std::unordered_set<ValueItem, art::hash<ValueItem>>&)self.value;
                });

                static AttachAFun(__to_function, 1, {
                    auto& self = CXX::Interface::getExtractAs<AtomicObject>(args[0], virtual_table);
                    lock_guard<TaskRecursiveMutex> lock(self.mutex);
                    return (art::shared_ptr<FuncEnvironment>&)self.value;
                });

                static AttachAFun(__explicit_await, 1, {
                    auto& self = CXX::Interface::getExtractAs<AtomicObject>(args[0], virtual_table);
                    lock_guard<TaskRecursiveMutex> lock(self.mutex);
                    self.value.getAsync();
                });

                static AttachAFun(__make_gc, 1, {
                    auto& self = CXX::Interface::getExtractAs<AtomicObject>(args[0], virtual_table);
                    lock_guard<TaskRecursiveMutex> lock(self.mutex);
                    self.value.make_gc();
                });

                static AttachAFun(__localize_gc, 1, {
                    auto& self = CXX::Interface::getExtractAs<AtomicObject>(args[0], virtual_table);
                    lock_guard<TaskRecursiveMutex> lock(self.mutex);
                    self.value.localize_gc();
                });

                static AttachAFun(__ungc, 1, {
                    auto& self = CXX::Interface::getExtractAs<AtomicObject>(args[0], virtual_table);
                    lock_guard<TaskRecursiveMutex> lock(self.mutex);
                    self.value.ungc();
                });

                static AttachAFun(__is_gc, 1, {
                    auto& self = CXX::Interface::getExtractAs<AtomicObject>(args[0], virtual_table);
                    lock_guard<TaskRecursiveMutex> lock(self.mutex);
                    return self.value.is_gc();
                });

                static AttachAFun(__hash, 1, {
                    auto& self = CXX::Interface::getExtractAs<AtomicObject>(args[0], virtual_table);
                    lock_guard<TaskRecursiveMutex> lock(self.mutex);
                    return self.value.hash();
                });

                static AttachAFun(__make_slice, 2, {
                    auto& self = CXX::Interface::getExtractAs<AtomicObject>(args[0], virtual_table);
                    lock_guard<TaskRecursiveMutex> lock(self.mutex);
                    if (len == 2)
                        return self.value.make_slice((uint32_t)args[1], self.value.meta.val_len);
                    else
                        return self.value.make_slice((uint32_t)args[1], (uint32_t)args[2]);
                });

                static AttachAFun(__size, 1, {
                    auto& self = CXX::Interface::getExtractAs<AtomicObject>(args[0], virtual_table);
                    lock_guard<TaskRecursiveMutex> lock(self.mutex);
                    return self.value.meta.val_len;
                });

                static AttachAFun(__get, 1, {
                    auto& self = CXX::Interface::getExtractAs<AtomicObject>(args[0], virtual_table);
                    lock_guard<TaskRecursiveMutex> lock(self.mutex);
                    return self.value;
                });

                static AttachAFun(__set, 2, {
                    auto& self = CXX::Interface::getExtractAs<AtomicObject>(args[0], virtual_table);
                    lock_guard<TaskRecursiveMutex> lock(self.mutex);
                    self.value = args[1];
                });

                static void init() {
                    virtual_table = CXX::Interface::createTable<AtomicObject>("AtomicAny", CXX::Interface::direct_method(symbols::structures::add_operator, __add), CXX::Interface::direct_method(symbols::structures::subtract_operator, __sub), CXX::Interface::direct_method(symbols::structures::multiply_operator, __mul), CXX::Interface::direct_method(symbols::structures::divide_operator, __div), CXX::Interface::direct_method(symbols::structures::modulo_operator, __mod), CXX::Interface::direct_method(symbols::structures::bitwise_and_operator, __and), CXX::Interface::direct_method(symbols::structures::bitwise_or_operator, __or), CXX::Interface::direct_method(symbols::structures::bitwise_xor_operator, __xor), CXX::Interface::direct_method(symbols::structures::bitwise_shift_left_operator, __lshift), CXX::Interface::direct_method(symbols::structures::bitwise_shift_right_operator, __rshift), CXX::Interface::direct_method(symbols::structures::not_equal_operator, __not_equal), CXX::Interface::direct_method(symbols::structures::equal_operator, __equal), CXX::Interface::direct_method(symbols::structures::less_operator, __less), CXX::Interface::direct_method(symbols::structures::greater_operator, __greater), CXX::Interface::direct_method(symbols::structures::less_or_equal_operator, __less_equal), CXX::Interface::direct_method(symbols::structures::greater_or_equal_operator, __greater_equal), CXX::Interface::direct_method(symbols::structures::increment_operator, __inc), CXX::Interface::direct_method(symbols::structures::decrement_operator, __dec), CXX::Interface::direct_method(symbols::structures::not_operator, __not), CXX::Interface::direct_method(symbols::structures::bitwise_not_operator, __not), CXX::Interface::direct_method(symbols::structures::convert::to_boolean, __to_boolean), CXX::Interface::direct_method(symbols::structures::convert::to_string, __to_string), CXX::Interface::direct_method(symbols::structures::convert::to_ui8, __to_u8), CXX::Interface::direct_method(symbols::structures::convert::to_ui16, __to_u16), CXX::Interface::direct_method(symbols::structures::convert::to_ui32, __to_u32), CXX::Interface::direct_method(symbols::structures::convert::to_ui64, __to_u64), CXX::Interface::direct_method(symbols::structures::convert::to_i8, __to_i8), CXX::Interface::direct_method(symbols::structures::convert::to_i16, __to_i16), CXX::Interface::direct_method(symbols::structures::convert::to_i32, __to_i32), CXX::Interface::direct_method(symbols::structures::convert::to_i64, __to_i64), CXX::Interface::direct_method(symbols::structures::convert::to_float, __to_float), CXX::Interface::direct_method(symbols::structures::convert::to_double, __to_double), CXX::Interface::direct_method(symbols::structures::convert::to_ui8_arr, __to_ui8_arr), CXX::Interface::direct_method(symbols::structures::convert::to_ui16_arr, __to_ui16_arr), CXX::Interface::direct_method(symbols::structures::convert::to_ui32_arr, __to_ui32_arr), CXX::Interface::direct_method(symbols::structures::convert::to_ui64_arr, __to_ui64_arr), CXX::Interface::direct_method(symbols::structures::convert::to_i8_arr, __to_i8_arr), CXX::Interface::direct_method(symbols::structures::convert::to_i16_arr, __to_i16_arr), CXX::Interface::direct_method(symbols::structures::convert::to_i32_arr, __to_i32_arr), CXX::Interface::direct_method(symbols::structures::convert::to_i64_arr, __to_i64_arr), CXX::Interface::direct_method(symbols::structures::convert::to_float_arr, __to_float_arr), CXX::Interface::direct_method(symbols::structures::convert::to_double_arr, __to_double_arr), CXX::Interface::direct_method(symbols::structures::convert::to_faarr, __to_faarr), CXX::Interface::direct_method(symbols::structures::convert::to_timepoint, __to_timepoint), CXX::Interface::direct_method(symbols::structures::convert::to_type_identifier, __to_type_identifier), CXX::Interface::direct_method(symbols::structures::convert::to_function, __to_function), CXX::Interface::direct_method(symbols::structures::convert::to_map, __to_map), CXX::Interface::direct_method(symbols::structures::convert::to_set, __to_set), CXX::Interface::direct_method(symbols::structures::convert::to_uarr, __to_uarr), CXX::Interface::direct_method("explicit_await", __explicit_await), CXX::Interface::direct_method("make_gc", __make_gc), CXX::Interface::direct_method("localize_gc", __localize_gc), CXX::Interface::direct_method("ungc", __ungc), CXX::Interface::direct_method("is_gc", __is_gc), CXX::Interface::direct_method("hash", __hash), CXX::Interface::direct_method("make_slice", __make_slice), CXX::Interface::direct_method("size", __size), CXX::Interface::direct_method("get", __get), CXX::Interface::direct_method("set", __set));
                }
            };

            namespace constructor {
                ValueItem* createProxy_Bool(ValueItem* args, uint32_t len) {
                    bool set = len == 0 ? false : (bool)args[0];
                    return new ValueItem(CXX::Interface::constructStructure<AtomicBasic<bool>>(AtomicBasic<bool>::virtual_table, set), no_copy);
                }

                ValueItem* createProxy_I8(ValueItem* args, uint32_t len) {
                    int8_t set = len == 0 ? 0 : (int8_t)args[0];
                    return new ValueItem(CXX::Interface::constructStructure<AtomicBasic<int8_t>>(AtomicBasic<int8_t>::virtual_table, set), no_copy);
                }

                ValueItem* createProxy_I16(ValueItem* args, uint32_t len) {
                    int16_t set = len == 0 ? 0 : (int16_t)args[0];
                    return new ValueItem(CXX::Interface::constructStructure<AtomicBasic<int16_t>>(AtomicBasic<int16_t>::virtual_table, set), no_copy);
                }

                ValueItem* createProxy_I32(ValueItem* args, uint32_t len) {
                    int32_t set = len == 0 ? 0 : (int32_t)args[0];
                    return new ValueItem(CXX::Interface::constructStructure<AtomicBasic<int32_t>>(AtomicBasic<int32_t>::virtual_table, set), no_copy);
                }

                ValueItem* createProxy_I64(ValueItem* args, uint32_t len) {
                    int64_t set = len == 0 ? 0 : (int64_t)args[0];
                    return new ValueItem(CXX::Interface::constructStructure<AtomicBasic<int64_t>>(AtomicBasic<int64_t>::virtual_table, set), no_copy);
                }

                ValueItem* createProxy_UI8(ValueItem* args, uint32_t len) {
                    uint8_t set = len == 0 ? 0 : (uint8_t)args[0];
                    return new ValueItem(CXX::Interface::constructStructure<AtomicBasic<uint8_t>>(AtomicBasic<uint8_t>::virtual_table, set), no_copy);
                }

                ValueItem* createProxy_UI16(ValueItem* args, uint32_t len) {
                    uint16_t set = len == 0 ? 0 : (uint16_t)args[0];
                    return new ValueItem(CXX::Interface::constructStructure<AtomicBasic<uint16_t>>(AtomicBasic<uint16_t>::virtual_table, set), no_copy);
                }

                ValueItem* createProxy_UI32(ValueItem* args, uint32_t len) {
                    uint32_t set = len == 0 ? 0 : (uint32_t)args[0];
                    return new ValueItem(CXX::Interface::constructStructure<AtomicBasic<uint32_t>>(AtomicBasic<uint32_t>::virtual_table, set), no_copy);
                }

                ValueItem* createProxy_UI64(ValueItem* args, uint32_t len) {
                    uint64_t set = len == 0 ? 0 : (uint64_t)args[0];
                    return new ValueItem(CXX::Interface::constructStructure<AtomicBasic<uint64_t>>(AtomicBasic<uint64_t>::virtual_table, set), no_copy);
                }

                ValueItem* createProxy_Float(ValueItem* args, uint32_t len) {
                    float set = len == 0 ? 0 : (float)args[0];
                    return new ValueItem(CXX::Interface::constructStructure<AtomicBasic<float>>(AtomicBasic<float>::virtual_table, set), no_copy);
                }

                ValueItem* createProxy_Double(ValueItem* args, uint32_t len) {
                    double set = len == 0 ? 0 : (double)args[0];
                    return new ValueItem(CXX::Interface::constructStructure<AtomicBasic<double>>(AtomicBasic<double>::virtual_table, set), no_copy);
                }

                ValueItem* createProxy_UndefinedPtr(ValueItem* args, uint32_t len) {
                    void* set = len == 0 ? nullptr : (void*)args[0];
                    return new ValueItem(CXX::Interface::constructStructure<AtomicBasic<size_t>>(AtomicBasic<size_t>::virtual_table, (size_t)set), no_copy);
                }

                ValueItem* createProxy_Any(ValueItem* args, uint32_t len) {
                    return new ValueItem(CXX::Interface::constructStructure<AtomicObject>(AtomicObject::virtual_table, len ? args[0] : nullptr), no_copy);
                }
            }
        }

        namespace native {

#pragma region define_mutex
            AttachAFun(funs_mutex_lock, 1, {
                auto& self = CXX::Interface::getExtractAs<art::shared_ptr<art::mutex>>(args[0], define_mutex);
                self->lock();
            });
            AttachAFun(funs_mutex_unlock, 1, {
                auto& self = CXX::Interface::getExtractAs<art::shared_ptr<art::mutex>>(args[0], define_mutex);
                self->unlock();
            });
            AttachAFun(funs_mutex_try_lock, 1, {
                auto& self = CXX::Interface::getExtractAs<art::shared_ptr<art::mutex>>(args[0], define_mutex);
                return self->try_lock();
            });
#pragma endregion

#pragma region define_rw_mutex
            AttachAFun(funs_rw_mutex_lock, 1, {
                auto& self = CXX::Interface::getExtractAs<art::shared_ptr<art::rw_mutex>>(args[0], define_mutex);
                self->lock();
            });
            AttachAFun(funs_rw_mutex_unlock, 1, {
                auto& self = CXX::Interface::getExtractAs<art::shared_ptr<art::rw_mutex>>(args[0], define_mutex);
                self->unlock();
            });
            AttachAFun(funs_rw_mutex_try_lock, 1, {
                auto& self = CXX::Interface::getExtractAs<art::shared_ptr<art::rw_mutex>>(args[0], define_mutex);
                return self->try_lock();
            });

            AttachAFun(funs_rw_mutex_lock_shared, 1, {
                auto& self = CXX::Interface::getExtractAs<art::shared_ptr<art::rw_mutex>>(args[0], define_mutex);
                self->lock_shared();
            });
            AttachAFun(funs_rw_mutex_unlock_shared, 1, {
                auto& self = CXX::Interface::getExtractAs<art::shared_ptr<art::rw_mutex>>(args[0], define_mutex);
                self->unlock_shared();
            });
#pragma endregion

#pragma region define_timed_mutex
            AttachAFun(funs_timed_mutex_lock, 1, {
                auto& self = CXX::Interface::getExtractAs<art::shared_ptr<art::timed_mutex>>(args[0], define_mutex);
                self->lock();
            });
            AttachAFun(funs_timed_mutex_unlock, 1, {
                auto& self = CXX::Interface::getExtractAs<art::shared_ptr<art::timed_mutex>>(args[0], define_mutex);
                self->unlock();
            });
            AttachAFun(funs_timed_mutex_try_lock, 1, {
                auto& self = CXX::Interface::getExtractAs<art::shared_ptr<art::timed_mutex>>(args[0], define_mutex);
                return self->try_lock();
            });

            AttachAFun(funs_timed_mutex_try_lock_for, 2, {
                auto& self = CXX::Interface::getExtractAs<art::shared_ptr<art::timed_mutex>>(args[0], define_mutex);
                return self->try_lock_for((std::chrono::milliseconds)(uint64_t)args[1]);
            });
            AttachAFun(funs_timed_mutex_try_lock_until, 1, {
                auto& self = CXX::Interface::getExtractAs<art::shared_ptr<art::timed_mutex>>(args[0], define_mutex);
                return self->try_lock_until((std::chrono::high_resolution_clock::time_point)args[1]);
            });
#pragma endregion

#pragma region define_recursive_mutex
            AttachAFun(funs_recursive_mutex_lock, 1, {
                auto& self = CXX::Interface::getExtractAs<art::shared_ptr<art::recursive_mutex>>(args[0], define_mutex);
                self->lock();
            });
            AttachAFun(funs_recursive_mutex_unlock, 1, {
                auto& self = CXX::Interface::getExtractAs<art::shared_ptr<art::recursive_mutex>>(args[0], define_mutex);
                self->unlock();
            });
            AttachAFun(funs_recursive_mutex_try_lock, 1, {
                auto& self = CXX::Interface::getExtractAs<art::shared_ptr<art::recursive_mutex>>(args[0], define_mutex);
                return self->try_lock();
            });
#pragma endregion

#pragma region define_condition_variable
            AttachAFun(funs_condition_variable_wait, 2, {
                auto& self = CXX::Interface::getExtractAs<art::shared_ptr<art::condition_variable_any>>(args[0], define_condition_variable);
                MutexUnify unify = getMutex(args[1]);
                self->wait(unify);
            });

            AttachAFun(funs_condition_variable_wait_for, 3, {
                auto& self = CXX::Interface::getExtractAs<art::shared_ptr<art::condition_variable_any>>(args[0], define_condition_variable);
                MutexUnify unify = getMutex(args[1]);
                auto milliseconds = (std::chrono::milliseconds)(uint64_t)args[2];
                return self->wait_for(unify, milliseconds) == art::cv_status::no_timeout;
            });

            AttachAFun(funs_condition_variable_wait_until, 3, {
                auto& self = CXX::Interface::getExtractAs<art::shared_ptr<art::condition_variable_any>>(args[0], define_condition_variable);
                MutexUnify unify = getMutex(args[1]);
                auto point = (std::chrono::high_resolution_clock::time_point)args[2];
                return self->wait_until(unify, point) == art::cv_status::no_timeout;
            });

            AttachAFun(funs_condition_variable_notify_one, 3, {
                auto& self = CXX::Interface::getExtractAs<art::shared_ptr<art::condition_variable_any>>(args[0], define_condition_variable);
                self->notify_one();
            });

            AttachAFun(funs_condition_variable_notify_all, 3, {
                auto& self = CXX::Interface::getExtractAs<art::shared_ptr<art::condition_variable_any>>(args[0], define_condition_variable);
                self->notify_all();
            });
#pragma endregion

#pragma region define_thread

            struct Thread {
                struct await_handle {
                    TaskConditionVariable cv;
                    TaskMutex mtx;
                    ValueItem* res = nullptr;
                    bool end = false;
                    bool on_exception = false;
                    bool result_got = false;
                    bool on_wait = false;
                };

                art::shared_ptr<await_handle> handle;
                unsigned long _id;

                ValueItem wait(bool unwind_exception) {
                    await_handle* _handle = handle.get();
                    MutexUnify unify(_handle->mtx);

                    art::unique_lock<MutexUnify> lock(unify);
                    if (_handle->result_got)
                        throw InvalidOperation("Tried to get already got result");
                    if (_handle->on_wait)
                        throw InvalidOperation("This thread already on wait by another thread/task");
                    _handle->on_wait = true;
                    while (_handle->end)
                        _handle->cv.wait(lock);

                    _handle->result_got = true;
                    if (_handle->on_exception && unwind_exception) {
                        if (_handle->res) {
                            auto ex = (std::exception_ptr)*_handle->res;
                            delete _handle->res;
                            std::rethrow_exception(ex);
                        } else
                            throw NullPointerException();
                    } else {
                        if (_handle->res) {
                            ValueItem res = std::move(*_handle->res);
                            delete _handle->res;
                            _handle->res = nullptr;
                            return res;
                        } else
                            return nullptr;
                    }
                }

                //returns [bool success, any result]
                ValueItem wait_for(uint64_t milliseconds, bool unwind_exception) {
                    return wait_until(std::chrono::high_resolution_clock::now() + std::chrono::milliseconds(milliseconds), unwind_exception);
                }

                ValueItem wait_until(std::chrono::high_resolution_clock::time_point point, bool unwind_exception) {
                    await_handle* _handle = handle.get();
                    MutexUnify unify(_handle->mtx);

                    art::unique_lock<MutexUnify> lock(unify);
                    if (_handle->result_got)
                        throw InvalidOperation("Tried to get already got result");
                    if (_handle->on_wait)
                        throw InvalidOperation("This thread already on wait by another thread/task");
                    _handle->on_wait = true;
                    while (_handle->end) {
                        if (!_handle->cv.wait_until(lock, point)) {
                            _handle->on_wait = false;
                            return {false, nullptr};
                        }
                    }

                    _handle->result_got = true;
                    if (_handle->on_exception && unwind_exception) {
                        if (_handle->res) {
                            auto ex = (std::exception_ptr)*_handle->res;
                            delete _handle->res;
                            std::rethrow_exception(ex);
                        } else
                            throw NullPointerException();
                    } else {
                        auto res = _handle->res;
                        _handle->res = nullptr;
                        return {true, res};
                    }
                }

                bool waitable() {
                    return !handle->on_wait;
                }

                unsigned long id() {
                    return _id;
                }

                art::ustring getName() {
                    return _get_name_thread_dbg(_id);
                }

                void setName(const art::ustring& name) {
                    _set_name_thread_dbg(name, _id);
                }
            };

            AttachAFun(funcs_Thread_wait, 1, {
                auto& self = CXX::Interface::getExtractAs<art::shared_ptr<Thread>>(args[0], define_thread);
                bool unwind_exception = true;
                if (len >= 2)
                    unwind_exception = (bool)args[1];
                return self->wait(unwind_exception);
            });
            AttachAFun(funcs_Thread_wait_for, 2, {
                auto& self = CXX::Interface::getExtractAs<art::shared_ptr<Thread>>(args[0], define_thread);
                uint64_t until = (uint64_t)args[1];
                bool unwind_exception = true;
                if (len >= 3)
                    unwind_exception = (bool)args[2];
                return self->wait_for(until, unwind_exception);
            });
            AttachAFun(funcs_Thread_wait_until, 2, {
                auto& self = CXX::Interface::getExtractAs<art::shared_ptr<Thread>>(args[0], define_thread);
                auto until = (std::chrono::high_resolution_clock::time_point)args[1];
                bool unwind_exception = true;
                if (len >= 3)
                    unwind_exception = (bool)args[2];
                return self->wait_until(until, unwind_exception);
            });
            AttachAFun(funcs_Thread_waitable, 1, {
                auto& self = CXX::Interface::getExtractAs<art::shared_ptr<Thread>>(args[0], define_thread);
                return self->waitable();
            });
            AttachAFun(funcs_Thread_id, 1, {
                auto& self = CXX::Interface::getExtractAs<art::shared_ptr<Thread>>(args[0], define_thread);
                return (uint64_t)self->id();
            });
            AttachAFun(funcs_Thread_get_name, 1, {
                auto& self = CXX::Interface::getExtractAs<art::shared_ptr<Thread>>(args[0], define_thread);
                return self->getName();
            });
            AttachAFun(funcs_Thread_set_name, 2, {
                auto& self = CXX::Interface::getExtractAs<art::shared_ptr<Thread>>(args[0], define_thread);
                self->setName((art::ustring)args[1]);
            });
#pragma endregion

            namespace constructor {
                ValueItem* createProxy_mutex(ValueItem*, uint32_t) {
                    return new ValueItem(CXX::Interface::constructStructure<art::shared_ptr<art::mutex>>(define_mutex, new art::mutex()), no_copy);
                }

                ValueItem* createProxy_rw_mutex(ValueItem*, uint32_t) {
                    return new ValueItem(CXX::Interface::constructStructure<art::shared_ptr<art::rw_mutex>>(define_rw_mutex, new art::rw_mutex()), no_copy);
                }

                ValueItem* createProxy_timed_mutex(ValueItem*, uint32_t) {
                    return new ValueItem(CXX::Interface::constructStructure<art::shared_ptr<art::timed_mutex>>(define_timed_mutex, new art::timed_mutex()), no_copy);
                }

                ValueItem* createProxy_recursive_mutex(ValueItem*, uint32_t) {
                    return new ValueItem(CXX::Interface::constructStructure<art::shared_ptr<art::recursive_mutex>>(define_recursive_mutex, new art::recursive_mutex()), no_copy);
                }

                ValueItem* createProxy_condition_variable(ValueItem*, uint32_t) {
                    return new ValueItem(CXX::Interface::constructStructure<art::shared_ptr<art::condition_variable_any>>(define_condition_variable, new art::condition_variable_any()), no_copy);
                }

                ValueItem* construct_Thread(ValueItem* vals, uint32_t len) {
                    CXX::arguments_range(len, 1);
                    art::shared_ptr<FuncEnvironment> func = *vals->funPtr();
                    Thread _thread;
                    art::shared_ptr<Thread::await_handle> handle = new Thread::await_handle();
                    _thread.handle = handle;
                    if (len != 1) {
                        ValueItem* movedArgs = new ValueItem[len - 1];
                        vals++;
                        len--;
                        uint32_t i = 0;
                        while (len--)
                            movedArgs[i++] = std::move(*vals++);

                        art::thread curr_thread(
                            [handle](art::shared_ptr<FuncEnvironment> func, ValueItem* args, uint32_t len) {
                                std::unique_ptr<ValueItem[]> args_hold(args);
                                try {
                                    auto tmp = FuncEnvironment::sync_call(func, args, len);
                                    unique_lock ul(handle->mtx);
                                    handle->res = tmp;
                                    handle->end = true;
                                    handle->cv.notify_all();
                                } catch (...) {
                                    unique_lock ul(handle->mtx);
                                    try {
                                        handle->res = new ValueItem(std::current_exception());
                                    } catch (...) {
                                        handle->end = true;
                                        handle->on_exception = true;
                                        handle->cv.notify_all();
                                    }
                                    handle->on_exception = true;
                                    handle->end = true;
                                    handle->cv.notify_all();
                                }
                            },
                            func,
                            movedArgs,
                            i
                        );
                        _thread._id = curr_thread.get_id();
                        curr_thread.detach();
                    } else {
                        art::thread curr_thread(
                            [handle](art::shared_ptr<FuncEnvironment> func) {
                                try {
                                    auto tmp = FuncEnvironment::sync_call(func, nullptr, 0);
                                    unique_lock ul(handle->mtx);
                                    handle->res = tmp;
                                    handle->end = true;
                                    handle->cv.notify_all();
                                } catch (...) {
                                    unique_lock ul(handle->mtx);
                                    try {
                                        handle->res = new ValueItem(std::current_exception());
                                    } catch (...) {
                                        handle->end = true;
                                        handle->on_exception = true;
                                        handle->cv.notify_all();
                                    }
                                    handle->on_exception = true;
                                    handle->end = true;
                                    handle->cv.notify_all();
                                }
                            },
                            func
                        );
                        _thread._id = curr_thread.get_id();
                        curr_thread.detach();
                    }

                    return new ValueItem(CXX::Interface::constructStructure<art::shared_ptr<Thread>>(define_thread, new Thread()), no_copy);
                }
            }

            namespace this_thread {
                AttachAFun(get_id, 0, {
                    return (size_t)art::this_thread::get_id();
                });

                AttachAFun(sleep_for, 1, {
                    art::this_thread::sleep_for((std::chrono::milliseconds)(uint64_t)args[0]);
                });

                AttachAFun(sleep_until, 1, {
                    art::this_thread::sleep_until((std::chrono::high_resolution_clock::time_point)args[0]);
                });

                ValueItem* yield(ValueItem*, uint32_t) {
                    art::this_thread::yield();
                    return nullptr;
                }
            }

            void init() {
                define_mutex = CXX::Interface::createTable<art::shared_ptr<art::mutex>>(
                    "native_mutex",
                    CXX::Interface::direct_method("lock", funs_mutex_lock),
                    CXX::Interface::direct_method("unlock", funs_mutex_unlock),
                    CXX::Interface::direct_method("try_lock", funs_mutex_try_lock)
                );
                CXX::Interface::typeVTable<art::shared_ptr<art::mutex>>() = define_mutex;

                define_rw_mutex = CXX::Interface::createTable<art::shared_ptr<art::rw_mutex>>(
                    "native_rw_mutex",
                    CXX::Interface::direct_method("lock", funs_rw_mutex_lock),
                    CXX::Interface::direct_method("unlock", funs_rw_mutex_unlock),
                    CXX::Interface::direct_method("try_lock", funs_rw_mutex_try_lock),
                    CXX::Interface::direct_method("lock_shared", funs_rw_mutex_lock_shared),
                    CXX::Interface::direct_method("unlock_shared", funs_rw_mutex_unlock_shared)
                );
                CXX::Interface::typeVTable<art::shared_ptr<art::rw_mutex>>() = define_rw_mutex;

                define_timed_mutex = CXX::Interface::createTable<art::shared_ptr<art::timed_mutex>>(
                    "native_timed_mutex",
                    CXX::Interface::direct_method("lock", funs_timed_mutex_lock),
                    CXX::Interface::direct_method("unlock", funs_timed_mutex_unlock),
                    CXX::Interface::direct_method("try_lock", funs_timed_mutex_try_lock),
                    CXX::Interface::direct_method("try_lock_for", funs_timed_mutex_try_lock_for),
                    CXX::Interface::direct_method("try_lock_until", funs_timed_mutex_try_lock_until)
                );
                CXX::Interface::typeVTable<art::shared_ptr<art::timed_mutex>>() = define_timed_mutex;

                define_recursive_mutex = CXX::Interface::createTable<art::shared_ptr<art::recursive_mutex>>(
                    "native_recursive_mutex",
                    CXX::Interface::direct_method("lock", funs_recursive_mutex_lock),
                    CXX::Interface::direct_method("unlock", funs_recursive_mutex_unlock),
                    CXX::Interface::direct_method("try_lock", funs_recursive_mutex_try_lock)
                );
                CXX::Interface::typeVTable<art::shared_ptr<art::recursive_mutex>>() = define_recursive_mutex;


                define_condition_variable = CXX::Interface::createTable<art::shared_ptr<art::condition_variable_any>>(
                    "native_condition_variable",
                    CXX::Interface::direct_method("wait", funs_condition_variable_wait),
                    CXX::Interface::direct_method("wait_until", funs_condition_variable_wait_until),
                    CXX::Interface::direct_method("notify_one", funs_condition_variable_notify_one),
                    CXX::Interface::direct_method("notify_all", funs_condition_variable_notify_all)
                );
                CXX::Interface::typeVTable<art::shared_ptr<art::condition_variable_any>>() = define_condition_variable;
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
            native::init();
        }
    }
}