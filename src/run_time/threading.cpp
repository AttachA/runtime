// Copyright Danyil Melnytskyi 2022-2023
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#include "threading.hpp"
#include <Windows.h>
namespace run_time {
    namespace threading {
#ifdef _WIN32
        mutex::mutex() {
            _mutex = SRWLOCK_INIT;
        }
        mutex::~mutex() {

            
        }
        void mutex::lock() {
            AcquireSRWLockExclusive((PSRWLOCK)&_mutex);
        }
        void mutex::unlock() {
            ReleaseSRWLockExclusive((PSRWLOCK)&_mutex);
        }
        bool mutex::try_lock() {
            return TryAcquireSRWLockExclusive((PSRWLOCK)&_mutex);
        }

            rw_mutex::rw_mutex(){
                _mutex = SRWLOCK_INIT;
            }
            rw_mutex::~rw_mutex(){

            }
            void rw_mutex::lock(){
                AcquireSRWLockExclusive((PSRWLOCK)&_mutex);
            }
            void rw_mutex::unlock(){
                ReleaseSRWLockExclusive((PSRWLOCK)&_mutex);
            }
            bool rw_mutex::try_lock(){
                return TryAcquireSRWLockExclusive((PSRWLOCK)&_mutex);
            }

            void rw_mutex::lock_shared(){
                AcquireSRWLockShared((PSRWLOCK)&_mutex);
            }
            void rw_mutex::unlock_shared(){
                ReleaseSRWLockShared((PSRWLOCK)&_mutex);
            }


        recursive_mutex::recursive_mutex() {
            count = 0;
            owner = art::thread::id();
        }
        recursive_mutex::~recursive_mutex() {
            
        }
        void recursive_mutex::lock() {
            if (owner == art::this_thread::get_id()) {
                count++;
                return;
            }
            actual_mutex.lock();
            owner = art::this_thread::get_id();
            count = 1;
        }
        void recursive_mutex::unlock() {
            if (owner != art::this_thread::get_id()) {
                return;
            }
            count--;
            if (count == 0) {
                owner = art::thread::id();
                actual_mutex.unlock();
            }
        }
        bool recursive_mutex::try_lock() {
            if (owner == art::this_thread::get_id()) {
                count++;
                return true;
            }
            if (actual_mutex.try_lock()) {
                owner = art::this_thread::get_id();
                count = 1;
                return true;
            }
            return false;
        }
        
        relock_state recursive_mutex::relock_begin(){
            unsigned int _count = count;
            count = 1;
            return relock_state(_count);
        }
        void recursive_mutex::relock_end(relock_state state){
            count = state._state;
            owner = art::this_thread::get_id();
        }
        timed_mutex::timed_mutex() {
            
        }
        timed_mutex::~timed_mutex() {
            
        }
        void timed_mutex::lock() {
            std::lock_guard<mutex> lock(_mutex);
            while (locked != 0) 
                _cond.wait(_mutex);
            locked = UINT_MAX;
        }
        void timed_mutex::unlock() {
            {
                std::lock_guard<mutex> lock(_mutex);
                locked = 0;
            }
            _cond.notify_one();
        }
        bool timed_mutex::try_lock() {
            std::lock_guard<mutex> lock(_mutex);
            if (locked == 0) {
                locked = UINT_MAX;
                return true;
            }
            else
                return false;
        }
        bool timed_mutex::try_lock_for(std::chrono::milliseconds ms){
            return try_lock_until(std::chrono::high_resolution_clock::now() + ms);
        }
        bool timed_mutex::try_lock_until(std::chrono::high_resolution_clock::time_point time){
            std::lock_guard<mutex> lock(_mutex);
            while(locked != 0)
                if (_cond.wait_until(_mutex,time) == cv_status::timeout)
                    return false;
            locked = UINT_MAX;
            return true;
        }
        condition_variable::condition_variable() {
            _cond = CONDITION_VARIABLE_INIT;
        }
        condition_variable::~condition_variable() {
            
        }
        void condition_variable::notify_one() {
            WakeConditionVariable((PCONDITION_VARIABLE)&_cond);
        }
        void condition_variable::notify_all() {
            WakeAllConditionVariable((PCONDITION_VARIABLE)&_cond);
        }
        
        void condition_variable::wait(mutex& mtx) {
            if(SleepConditionVariableSRW((PCONDITION_VARIABLE)&_cond, (PSRWLOCK)&mtx._mutex, INFINITE, 0)){
                DWORD err = GetLastError();
                if(err)
                    throw SystemException(err);
            }
        }
        cv_status condition_variable::wait_for(mutex& mtx, std::chrono::milliseconds ms) {
            return wait_until(mtx, std::chrono::high_resolution_clock::now() + ms);
        }
        cv_status condition_variable::wait_until(mutex& mtx, std::chrono::high_resolution_clock::time_point time) {
            while(true){
                auto sleep_ms = std::chrono::duration_cast<std::chrono::milliseconds>(time - std::chrono::high_resolution_clock::now());
                if (sleep_ms.count() <= 0) 
                    return cv_status::timeout;
                DWORD wait_time = sleep_ms.count() > 0x7FFFFFFF ? 0x7FFFFFFF : (DWORD)sleep_ms.count();
                if (SleepConditionVariableSRW((PCONDITION_VARIABLE)&_cond, (PSRWLOCK)&mtx._mutex, wait_time, 0)) {
                    auto err = GetLastError();
                    switch(err){
                        case ERROR_TIMEOUT:
                            return cv_status::timeout;
                        case ERROR_SUCCESS:
                            return cv_status::no_timeout;
                        default:
                            throw SystemException(err);
                    }
                }else
                    return cv_status::no_timeout;
            }
        }
        
        void condition_variable::wait(recursive_mutex& mtx) {
            auto state = mtx.relock_begin();
            if(SleepConditionVariableSRW((PCONDITION_VARIABLE)&_cond, (PSRWLOCK)&mtx.actual_mutex, INFINITE, 0)){
                mtx.relock_end(state);
                DWORD err = GetLastError();
                if(err)
                    throw SystemException(err);
                else
                    return;
            }
            mtx.relock_end(state);
        }
        cv_status condition_variable::wait_for(recursive_mutex& mtx, std::chrono::milliseconds ms) {
            return wait_until(mtx, std::chrono::high_resolution_clock::now() + ms);
        }
        cv_status condition_variable::wait_until(recursive_mutex& mtx, std::chrono::high_resolution_clock::time_point time) {
            auto state = mtx.relock_begin();
            while (true) {
                auto sleep_ms = std::chrono::duration_cast<std::chrono::milliseconds>(time - std::chrono::high_resolution_clock::now());
                if (sleep_ms.count() <= 0)
                    return cv_status::timeout;
                DWORD wait_time = sleep_ms.count() > 0x7FFFFFFF ? 0x7FFFFFFF : (DWORD)sleep_ms.count();
                bool res = SleepConditionVariableSRW((PCONDITION_VARIABLE)&_cond, (PSRWLOCK)&mtx.actual_mutex, wait_time, 0);
                cv_status status = cv_status::no_timeout;
                mtx.relock_end(state);
                if (res) {
                    auto err = GetLastError();
                    switch (err) {
                    case ERROR_TIMEOUT:
                        status = cv_status::timeout;
                        break;
                    default:
                        throw SystemException(err);

                    case ERROR_SUCCESS:
                        break;
                    }
                }
                return status;
            }
        }

        
        void* thread::create(void(*function)(void*), void* arg, unsigned int& id, size_t stack_size,bool stack_reservation, int& error_code){
            error_code = 0;
            void* handle = CreateThread(nullptr, stack_size, (LPTHREAD_START_ROUTINE)function, arg, stack_reservation?STACK_SIZE_PARAM_IS_A_RESERVATION:0 , (LPDWORD)&id);
            if(!handle){
                error_code = GetLastError();
                return nullptr;
            }
            return handle;
        }
        [[nodiscard]] unsigned int thread::hardware_concurrency() noexcept{
            SYSTEM_INFO sysinfo;
            GetSystemInfo(&sysinfo);
            int numCPU = sysinfo.dwNumberOfProcessors;
            if(numCPU < 1)
                numCPU = 1;
            return (unsigned int)numCPU;
        }
        void thread::join(){
            if(_thread){
                WaitForSingleObject(_thread, INFINITE);
                CloseHandle(_thread);
                _thread = nullptr;
            }
        }
        void thread::detach(){
            if(_thread){
                CloseHandle(_thread);
                _thread = nullptr;
            }
        }
        namespace this_thread{
            thread::id get_id() noexcept{
                return thread::id(GetCurrentThreadId());
            }
            void yield() noexcept{
                SwitchToThread();
            }
            void sleep_for(std::chrono::milliseconds ms){
                Sleep((DWORD)ms.count());
            }
            void sleep_until(std::chrono::high_resolution_clock::time_point time){
                auto diff = time - std::chrono::high_resolution_clock::now();
                while(diff.count() > 0){
                    Sleep((DWORD)diff.count());
                    diff = time - std::chrono::high_resolution_clock::now();
                }
            }
        }
#else
        mutex::mutex() {
            pthread_mutex_init(&_mutex, nullptr);
        }
        mutex::~mutex() {
            pthread_mutex_destroy(&_mutex);
        }
        void mutex::lock() {
            pthread_mutex_lock(&_mutex);
        }
        void mutex::unlock() {
            pthread_mutex_unlock(&_mutex);
        }
        bool mutex::try_lock() {
            return pthread_mutex_trylock(&_mutex) == 0;
        }
        recursive_mutex::recursive_mutex() {
            pthread_mutexattr_t attr;
            pthread_mutexattr_init(&attr);
            pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
            pthread_mutex_init(&_mutex, &attr);
            pthread_mutexattr_destroy(&attr);
        }
        recursive_mutex::~recursive_mutex() {
            pthread_mutex_destroy(&_mutex);
        }
        void recursive_mutex::lock() {
            pthread_mutex_lock(&_mutex);
        }
        void recursive_mutex::unlock() {
            pthread_mutex_unlock(&_mutex);
        }
        bool recursive_mutex::try_lock() {
            return pthread_mutex_trylock(&_mutex) == 0;
        }
        timed_mutex::timed_mutex() {
            pthread_mutex_init(&_mutex, nullptr);
        }
        timed_mutex::~timed_mutex() {
            pthread_mutex_destroy(&_mutex);
        }
        void timed_mutex::lock() {
            pthread_mutex_lock(&_mutex);
        }
        void timed_mutex::unlock() {
            pthread_mutex_unlock(&_mutex);
        }
        bool timed_mutex::try_lock() {
            return pthread_mutex_trylock(&_mutex) == 0;
        }
        bool timed_mutex::try_lock_for(std::chrono::milliseconds ms) {
            return try_lock_until(std::chrono::high_resolution_clock::now() + ms);
        }
        bool timed_mutex::try_lock_until(std::chrono::high_resolution_clock::time_point time) {
            while (true) {
                auto sleep_ms = std::chrono::duration_cast<std::chrono::milliseconds>(time - std::chrono::high_resolution_clock::now());
                if (sleep_ms.count() <= 0) 
                    return false;
                timespec ts;
                ts.tv_sec = sleep_ms.count() / 1000;
                ts.tv_nsec = (sleep_ms.count() % 1000) * 1000000;
                if (pthread_mutex_timedlock(&_mutex, &ts) == 0) 
                    return true;
            }
        }

        condition_variable::condition_variable() {
            pthread_cond_init(&_cond, nullptr);
        }
        condition_variable::~condition_variable() {
            pthread_cond_destroy(&_cond);
        }
        void condition_variable::notify_one() {
            pthread_cond_signal(&_cond);
        }
        void condition_variable::notify_all() {
            pthread_cond_broadcast(&_cond);
        }
        void condition_variable::wait(mutex& mtx) {
            pthread_cond_wait(&_cond, &mtx._mutex);
        }
        bool condition_variable::wait_for(mutex& mtx, std::chrono::milliseconds ms) {
            return wait_until(mtx, std::chrono::high_resolution_clock::now() + ms);
        }
        bool condition_variable::wait_until(mutex& mtx, std::chrono::high_resolution_clock::time_point time) {
            while (true) {
                auto sleep_ms = std::chrono::duration_cast<std::chrono::milliseconds>(time - std::chrono::high_resolution_clock::now());
                if (sleep_ms.count() <= 0) {
                    return false;
                }
                timespec ts;
                ts.tv_sec = sleep_ms.count() / 1000;
                ts.tv_nsec = (sleep_ms.count() % 1000) * 1000000;
                if (pthread_cond_timedwait(&_cond, &mtx._mutex, &ts) == 0) {
                    return true;
                }
            }
            return false;
        }
        void condition_variable::wait(recursive_mutex& mtx) {
            pthread_cond_wait(&_cond, &mtx.actual_mutex);
        }
        bool condition_variable::wait_for(recursive_mutex& mtx, std::chrono::milliseconds ms) {
            return wait_until(mtx, std::chrono::high_resolution_clock::now() + ms);
        }
        bool condition_variable::wait_until(recursive_mutex& mtx, std::chrono::high_resolution_clock::time_point time) {
            while (true) {
                auto sleep_ms = std::chrono::duration_cast<std::chrono::milliseconds>(time - std::chrono::high_resolution_clock::now());
                if (sleep_ms.count() <= 0) {
                    return false;
                }
                timespec ts;
                ts.tv_sec = sleep_ms.count() / 1000;
                ts.tv_nsec = (sleep_ms.count() % 1000) * 1000000;
                if (pthread_cond_timedwait(&_cond, &mtx.actual_mutex, &ts) == 0) {
                    return true;
                }
            }
            return false;
        }
#endif
        void condition_variable_any::notify_one(){
            std::lock_guard<mutex> lock(_mutex);
            _cond.notify_one();
        }
        void condition_variable_any::notify_all(){
            std::lock_guard<mutex> lock(_mutex);
            _cond.notify_all();
        }


        [[nodiscard]] thread::id thread::get_id() const noexcept{
            return id(_id);
        }
        bool thread::id::operator==(const id& other) const noexcept{
            return _id == other._id;
        }
        bool thread::id::operator!=(const id& other) const noexcept{
            return _id != other._id;
        }
        bool thread::id::operator<(const id& other) const noexcept{
            return _id < other._id;
        }
        bool thread::id::operator<=(const id& other) const noexcept{
            return _id <= other._id;
        }
        bool thread::id::operator>(const id& other) const noexcept{
            return _id > other._id;
        }
        bool thread::id::operator>=(const id& other) const noexcept{
            return _id >= other._id;
        }
            
        [[nodiscard]] bool thread::joinable() const noexcept{
            return _thread != nullptr;
        }
        
    }
}