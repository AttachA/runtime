// Copyright Danyil Melnytskyi 2022-Present
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <base/run_time.hpp>
#include <run_time/asm/FuncEnvironment.hpp>
#include <run_time/tasks.hpp>
#include <run_time/tasks/_internal.hpp>

namespace art {
    TaskRecursiveMutex::~TaskRecursiveMutex() noexcept(false) {
        if (recursive_count)
            throw InvalidOperation("Mutex destroyed while locked");
    }

    void TaskRecursiveMutex::lock() {
        if (loc.is_task_thread) {
            if (mutex.current_task == &*loc.curr_task) {
                recursive_count++;
                if (recursive_count == 0) {
                    recursive_count--;
                    throw InvalidOperation("Recursive mutex overflow");
                }
            } else {
                mutex.lock();
                recursive_count = 1;
            }
        } else {
            if (mutex.current_task == reinterpret_cast<Task*>((size_t)_thread_id() | native_thread_flag)) {
                recursive_count++;
                if (recursive_count == 0) {
                    recursive_count--;
                    throw InvalidOperation("Recursive mutex overflow");
                }
            } else {
                mutex.lock();
                recursive_count = 1;
            }
        }
    }

    bool TaskRecursiveMutex::try_lock() {
        if (loc.is_task_thread) {
            if (mutex.current_task == &*loc.curr_task) {
                recursive_count++;
                if (recursive_count == 0) {
                    recursive_count--;
                    return false;
                }
                return true;
            } else if (mutex.try_lock()) {
                recursive_count = 1;
                return true;
            } else
                return false;
        } else {
            if (mutex.current_task == reinterpret_cast<Task*>((size_t)_thread_id() | native_thread_flag)) {
                recursive_count++;
                if (recursive_count == 0) {
                    recursive_count--;
                    return false;
                }
                return true;
            } else if (mutex.try_lock()) {
                recursive_count = 1;
                return true;
            } else
                return false;
        }
    }

    bool TaskRecursiveMutex::try_lock_for(size_t milliseconds) {
        if (loc.is_task_thread) {
            if (mutex.current_task == &*loc.curr_task) {
                recursive_count++;
                if (recursive_count == 0) {
                    recursive_count--;
                    return false;
                }
                return true;
            } else if (mutex.try_lock_for(milliseconds)) {
                recursive_count = 1;
                return true;
            } else
                return false;
        } else {
            if (mutex.current_task == reinterpret_cast<Task*>((size_t)_thread_id() | native_thread_flag)) {
                recursive_count++;
                if (recursive_count == 0) {
                    recursive_count--;
                    return false;
                }
                return true;
            } else if (mutex.try_lock_for(milliseconds)) {
                recursive_count = 1;
                return true;
            } else
                return false;
        }
    }

    bool TaskRecursiveMutex::try_lock_until(std::chrono::high_resolution_clock::time_point time_point) {
        if (loc.is_task_thread) {
            if (mutex.current_task == &*loc.curr_task) {
                recursive_count++;
                if (recursive_count == 0) {
                    recursive_count--;
                    return false;
                }
                return true;
            } else if (mutex.try_lock_until(time_point)) {
                recursive_count = 1;
                return true;
            } else
                return false;
        } else {
            if (mutex.current_task == reinterpret_cast<Task*>((size_t)_thread_id() | native_thread_flag)) {
                recursive_count++;
                if (recursive_count == 0) {
                    recursive_count--;
                    return false;
                }
                return true;
            } else if (mutex.try_lock_until(time_point)) {
                recursive_count = 1;
                return true;
            } else
                return false;
        }
    }

    void TaskRecursiveMutex::unlock() {
        if (recursive_count) {
            recursive_count--;
            if (!recursive_count)
                mutex.unlock();
        } else
            throw InvalidOperation("Mutex not locked");
    }

    bool TaskRecursiveMutex::is_locked() {
        if (recursive_count)
            return true;
        else
            return false;
    }

    void TaskRecursiveMutex::lifecycle_lock(art::shared_ptr<Task> task) {
        mutex.lifecycle_lock(task);
    }

    void TaskRecursiveMutex::sequence_lock(art::shared_ptr<Task> task) {
        mutex.sequence_lock(task);
    }

    bool TaskRecursiveMutex::is_own() {
        if (loc.is_task_thread) {
            if (mutex.current_task == &*loc.curr_task)
                return true;
        } else if (mutex.current_task == reinterpret_cast<Task*>((size_t)_thread_id() | native_thread_flag))
            return true;
        return false;
    }
}
