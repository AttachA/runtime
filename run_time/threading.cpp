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
        recursive_mutex::recursive_mutex() {
            count = 0;
            owner = std::thread::id();
        }
        recursive_mutex::~recursive_mutex() {
            
        }
        void recursive_mutex::lock() {
            if (owner == std::this_thread::get_id()) {
                count++;
                return;
            }
            actual_mutex.lock();
            owner = std::this_thread::get_id();
            count = 1;
        }
        void recursive_mutex::unlock() {
            if (owner != std::this_thread::get_id()) {
                return;
            }
            count--;
            if (count == 0) {
                owner = std::thread::id();
                actual_mutex.unlock();
            }
        }
        bool recursive_mutex::try_lock() {
            if (owner == std::this_thread::get_id()) {
                count++;
                return true;
            }
            if (actual_mutex.try_lock()) {
                owner = std::this_thread::get_id();
                count = 1;
                return true;
            }
            return false;
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
                if (!_cond.wait_until(_mutex,time))
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
            SleepConditionVariableSRW((PCONDITION_VARIABLE)&_cond, (PSRWLOCK)&mtx._mutex, INFINITE, 0);
        }
        bool condition_variable::wait_for(mutex& mtx, std::chrono::milliseconds ms) {
            return wait_until(mtx, std::chrono::high_resolution_clock::now() + ms);
        }
        bool condition_variable::wait_until(mutex& mtx, std::chrono::high_resolution_clock::time_point time) {
            while(true){
                auto sleep_ms = std::chrono::duration_cast<std::chrono::milliseconds>(time - std::chrono::high_resolution_clock::now());
                if (sleep_ms.count() <= 0) {
                    return false;
                }
                DWORD wait_time = sleep_ms.count() > 0x7FFFFFFF ? 0x7FFFFFFF : (DWORD)sleep_ms.count();
                if (SleepConditionVariableSRW((PCONDITION_VARIABLE)&_cond, (PSRWLOCK)&mtx._mutex, wait_time, 0)) {
                    return true;
                }
            }
            return false;
        }
        
        void condition_variable::wait(recursive_mutex& mtx) {
            size_t count = mtx.count;
            SleepConditionVariableSRW((PCONDITION_VARIABLE)&_cond, (PSRWLOCK)&mtx.actual_mutex, INFINITE, 0);
            mtx.owner = std::this_thread::get_id();
            mtx.count = count;
        }
        bool condition_variable::wait_for(recursive_mutex& mtx, std::chrono::milliseconds ms) {
            return wait_until(mtx, std::chrono::high_resolution_clock::now() + ms);
        }
        bool condition_variable::wait_until(recursive_mutex& mtx, std::chrono::high_resolution_clock::time_point time) {
            size_t count = mtx.count;
            while (true) {
                auto sleep_ms = std::chrono::duration_cast<std::chrono::milliseconds>(time - std::chrono::high_resolution_clock::now());
                if (sleep_ms.count() <= 0)
                    return false;
                DWORD wait_time = sleep_ms.count() > 0x7FFFFFFF ? 0x7FFFFFFF : (DWORD)sleep_ms.count();
                if (SleepConditionVariableSRW((PCONDITION_VARIABLE)&_cond, (PSRWLOCK)&mtx.actual_mutex, wait_time, 0)) {
                    mtx.owner = std::this_thread::get_id();
                    mtx.count = count;
                    return true;
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
    }
}
