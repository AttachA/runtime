// Copyright Danyil Melnytskyi 2022-Present
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <run_time/tasks.hpp>

namespace art {

    void MutexUnify::lock() {
        switch (type) {
        case MutexUnifyType::nmut:
            nmut->lock();
            break;
        case MutexUnifyType::nrwmut:
            nrwmut->lock();
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
        case MutexUnifyType::urmut:
            urmut->lock();
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
        case MutexUnifyType::nrwmut:
            return nrwmut->try_lock();
        case MutexUnifyType::ntimed:
            return ntimed->try_lock();
        case MutexUnifyType::nrec:
            return nrec->try_lock();
        case MutexUnifyType::umut:
            return umut->try_lock();
        case MutexUnifyType::urmut:
            return urmut->try_lock();
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
        case MutexUnifyType::urmut:
            return urmut->try_lock_for(milliseconds);
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
        case MutexUnifyType::urmut:
            return urmut->try_lock_until(time_point);
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
        case MutexUnifyType::nrwmut:
            nrwmut->unlock();
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
        case MutexUnifyType::urmut:
            urmut->unlock();
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
        state = {0};
    }

    MutexUnify::MutexUnify(const MutexUnify& mut) {
        type = mut.type;
        nmut = mut.nmut;
        state = mut.state;
        state = {0};
    }

    MutexUnify::MutexUnify(art::mutex& smut) {
        type = MutexUnifyType::nmut;
        nmut = std::addressof(smut);
        state = {0};
    }

    MutexUnify::MutexUnify(art::rw_mutex& smut) {
        type = MutexUnifyType::nrwmut;
        nrwmut = std::addressof(smut);
        state = {0};
    }

    MutexUnify::MutexUnify(art::timed_mutex& smut) {
        type = MutexUnifyType::ntimed;
        ntimed = std::addressof(smut);
    }

    MutexUnify::MutexUnify(art::recursive_mutex& smut) {
        type = MutexUnifyType::nrec;
        nrec = std::addressof(smut);
        state = {0};
    }

    MutexUnify::MutexUnify(TaskMutex& smut) {
        type = MutexUnifyType::umut;
        umut = std::addressof(smut);
        state = {0};
    }

    MutexUnify::MutexUnify(TaskRecursiveMutex& smut) {
        type = MutexUnifyType::urmut;
        urmut = std::addressof(smut);
        state = {0};
    }

    MutexUnify::MutexUnify(MultiplyMutex& mmut)
        : mmut(&mmut) {
        type = MutexUnifyType::mmut;
        state = {0};
    }

    MutexUnify::MutexUnify(nullptr_t) {
        type = MutexUnifyType::noting;
        state = {0};
    }

    MutexUnify::~MutexUnify() {
        type = MutexUnifyType::noting;
    }

    MutexUnify& MutexUnify::operator=(const MutexUnify& mut) {
        type = mut.type;
        nmut = mut.nmut;
        state = mut.state;
        return *this;
    }

    MutexUnify& MutexUnify::operator=(art::mutex& smut) {
        type = MutexUnifyType::nmut;
        nmut = std::addressof(smut);
        state = {0};
        return *this;
    }

    MutexUnify& MutexUnify::operator=(art::rw_mutex& smut) {
        type = MutexUnifyType::nrwmut;
        nrwmut = std::addressof(smut);
        return *this;
    }

    MutexUnify& MutexUnify::operator=(art::timed_mutex& smut) {
        type = MutexUnifyType::ntimed;
        ntimed = std::addressof(smut);
        state = {0};
        return *this;
    }

    MutexUnify& MutexUnify::operator=(art::recursive_mutex& smut) {
        type = MutexUnifyType::nrec;
        nrec = std::addressof(smut);
        state = {0};
        return *this;
    }

    MutexUnify& MutexUnify::operator=(TaskMutex& smut) {
        type = MutexUnifyType::umut;
        umut = std::addressof(smut);
        state = {0};
        return *this;
    }

    MutexUnify& MutexUnify::operator=(TaskRecursiveMutex& smut) {
        type = MutexUnifyType::urmut;
        urmut = std::addressof(smut);
        state = {0};
        return *this;
    }

    MutexUnify& MutexUnify::operator=(MultiplyMutex& smut) {
        type = MutexUnifyType::mmut;
        mmut = std::addressof(smut);
        state = {0};
        return *this;
    }

    MutexUnify& MutexUnify::operator=(nullptr_t) {
        type = MutexUnifyType::noting;
        state = {0};
        return *this;
    }

    void MutexUnify::relock_start() {
        if (type == MutexUnifyType::nrec)
            state = nrec->relock_begin();
        else if (type == MutexUnifyType::urmut)
            state = urmut->relock_begin();
        else
            state = {0};
        unlock();
    }

    void MutexUnify::relock_end() {
        lock();
        if (type == MutexUnifyType::nrec)
            nrec->relock_end(state);
        if (type == MutexUnifyType::urmut)
            urmut->relock_end(state);
    }

    MutexUnify::operator bool() {
        return type != MutexUnifyType::noting;
    }

    full_state_relock_guard<class MutexUnify>::full_state_relock_guard(class MutexUnify& ref)
        : ref(ref) {
        ref.relock_start();
    }

    full_state_relock_guard<class MutexUnify>::~full_state_relock_guard() {
        ref.relock_end();
    }
}
