// Copyright Danyil Melnytskyi 2022-Present
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once
#include "../attacha_abi_structs.hpp"

namespace art {
    namespace parallel {
        void init();

        namespace constructor {
            ValueItem* createProxy_ConditionVariable(ValueItem*, uint32_t);
            ValueItem* createProxy_Mutex(ValueItem*, uint32_t);
            ValueItem* createProxy_RecursiveMutex(ValueItem*, uint32_t);
            ValueItem* createProxy_Semaphore(ValueItem*, uint32_t);

            ValueItem* createProxy_EventSystem(ValueItem*, uint32_t);
            ValueItem* createProxy_TaskLimiter(ValueItem*, uint32_t);

            //args: [func, (fault handler), (timeout), (used_task_local)], saarr,faarr[args....]
            ValueItem* createProxy_TaskQuery(ValueItem*, uint32_t);
            ValueItem* construct_Task(ValueItem*, uint32_t);
            ValueItem* createProxy_TaskGroup(ValueItem*, uint32_t);
        }

        namespace this_task {
            ValueItem* yield(ValueItem*, uint32_t);
            ValueItem* yield_result(ValueItem*, uint32_t);
            ValueItem* sleep(ValueItem*, uint32_t);
            ValueItem* sleep_until(ValueItem*, uint32_t);
            ValueItem* task_id(ValueItem*, uint32_t);
            ValueItem* check_cancellation(ValueItem*, uint32_t);
            ValueItem* self_cancel(ValueItem*, uint32_t);
            ValueItem* is_task(ValueItem*, uint32_t);
        }

        namespace task_runtime {
            ValueItem* clean_up(ValueItem*, uint32_t);
            ValueItem* create_executor(ValueItem*, uint32_t);
            ValueItem* total_executors(ValueItem*, uint32_t);
            ValueItem* reduce_executor(ValueItem*, uint32_t);
            ValueItem* become_task_executor(ValueItem*, uint32_t);
            ValueItem* await_no_tasks(ValueItem*, uint32_t);
            ValueItem* await_end_tasks(ValueItem*, uint32_t);
            ValueItem* explicitStartTimer(ValueItem*, uint32_t);

            ValueItem* create_bind_only_executor(ValueItem*, uint32_t);
            ValueItem* close_bind_only_executor(ValueItem*, uint32_t);
        }

        namespace atomic {
            namespace constructor {
                ValueItem* createProxy_Bool(ValueItem*, uint32_t);
                ValueItem* createProxy_I8(ValueItem*, uint32_t);
                ValueItem* createProxy_I16(ValueItem*, uint32_t);
                ValueItem* createProxy_I32(ValueItem*, uint32_t);
                ValueItem* createProxy_I64(ValueItem*, uint32_t);
                ValueItem* createProxy_UI8(ValueItem*, uint32_t);
                ValueItem* createProxy_UI16(ValueItem*, uint32_t);
                ValueItem* createProxy_UI32(ValueItem*, uint32_t);
                ValueItem* createProxy_UI64(ValueItem*, uint32_t);
                ValueItem* createProxy_Float(ValueItem*, uint32_t);
                ValueItem* createProxy_Double(ValueItem*, uint32_t);
                ValueItem* createProxy_UndefinedPtr(ValueItem*, uint32_t);
                ValueItem* createProxy_Any(ValueItem*, uint32_t);
            }
        }

        namespace native {
            namespace constructor {
                ValueItem* createProxy_mutex(ValueItem*, uint32_t);
                ValueItem* createProxy_rw_mutex(ValueItem*, uint32_t);
                ValueItem* createProxy_timed_mutex(ValueItem*, uint32_t);
                ValueItem* createProxy_recursive_mutex(ValueItem*, uint32_t);

                ValueItem* createProxy_condition_variable(ValueItem*, uint32_t);

                ValueItem* construct_Thread(ValueItem*, uint32_t);
            }

            namespace this_thread {
                ValueItem* get_id(ValueItem*, uint32_t);
                ValueItem* sleep_for(ValueItem*, uint32_t);
                ValueItem* sleep_until(ValueItem*, uint32_t);
                ValueItem* yield(ValueItem*, uint32_t);
            }
        }

        //make raw thread
        ValueItem* createThread(ValueItem*, uint32_t);

        //uses native::thread::wait
        ValueItem* createThreadAndWait(ValueItem*, uint32_t);

        //returns task that uses native::thread::wait
        ValueItem* createAsyncThread(ValueItem*, uint32_t);

        //returns task, args: [func, (fault handler), (timeout), (used_task_local)], saarr,faarr[args....]
        ValueItem* createTask(ValueItem*, uint32_t);
    }
}