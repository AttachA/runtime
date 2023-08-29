// Copyright Danyil Melnytskyi 2022-Present
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <run_time/tasks.hpp>

namespace art {
    MultiplyMutex::MultiplyMutex(const std::initializer_list<MutexUnify>& muts)
        : mu(muts) {}

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
            } else {
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
            } else {
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
}
