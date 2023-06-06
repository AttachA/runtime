#ifndef RUN_TIME_THREADING
#define RUN_TIME_THREADING
#include <thread>
#include <chrono>
//created to avoid back compatibility problems, ex: big size of std::mutex, slow critical section in windows, etc
namespace run_time {
    namespace threading {
        class mutex {
            friend class condition_variable;
            #ifdef _WIN32
            void* _mutex;
            #else
            pthread_mutex_t _mutex;
            #endif
        public:
            mutex();
            mutex(const mutex& other) = delete;
            mutex(mutex&& other) = delete;
            mutex& operator=(const mutex& other) = delete;
            mutex& operator=(mutex&& other) = delete;
            ~mutex();
            void lock();
            void unlock();
            bool try_lock();
        };
        class recursive_mutex {
            friend class condition_variable;
            mutex actual_mutex;
            #ifdef _WIN32
            size_t count = 0;
            std::thread::id owner;
            #else
            #endif
        public:
            recursive_mutex();
            recursive_mutex(const recursive_mutex& other) = delete;
            recursive_mutex(recursive_mutex&& other) = delete;
            recursive_mutex& operator=(const recursive_mutex& other) = delete;
            recursive_mutex& operator=(recursive_mutex&& other) = delete;
            ~recursive_mutex();
            void lock();
            void unlock();
            bool try_lock();
        };
        class condition_variable {
            #ifdef _WIN32
            void* _cond;
            #else
            pthread_cond_t _cond;
            #endif
        public:
            condition_variable();
            condition_variable(const condition_variable& other) = delete;
            condition_variable(condition_variable&& other) = delete;
            condition_variable& operator=(const condition_variable& other) = delete;
            condition_variable& operator=(condition_variable&& other) = delete;
            ~condition_variable();
            void notify_one();
            void notify_all();
            void wait(mutex& mtx);
            bool wait_for(mutex& mtx, std::chrono::milliseconds ms);
            bool wait_until(mutex& mtx, std::chrono::high_resolution_clock::time_point time);

            void wait(recursive_mutex& mtx);
            bool wait_for(recursive_mutex& mtx, std::chrono::milliseconds ms);
            bool wait_until(recursive_mutex& mtx, std::chrono::high_resolution_clock::time_point time);
        };
        class timed_mutex {
            friend class condition_variable;
            #ifdef _WIN32
            condition_variable _cond;
            mutex _mutex;
            unsigned int locked = 0;
            #else
            pthread_mutex_t _mutex;
            #endif
        public:
            timed_mutex();
            timed_mutex(const timed_mutex& other) = delete;
            timed_mutex(timed_mutex&& other) = delete;
            timed_mutex& operator=(const timed_mutex& other) = delete;
            timed_mutex& operator=(timed_mutex&& other) = delete;
            ~timed_mutex();
            void lock();
            void unlock();
            bool try_lock();
            bool try_lock_for(std::chrono::milliseconds ms);
            bool try_lock_until(std::chrono::high_resolution_clock::time_point time);
        };
        class condition_variable_any {
            condition_variable _cond;
            mutex _mutex;
        public:
            condition_variable_any() = default;
            void notify_one();
            void notify_all();
            template<class mut>
            void wait(mut& mtx){
                {
                    std::lock_guard<mutex> lock(_mutex);
                    mtx.unlock();
                    _cond.wait(_mutex);
                }
                mtx.lock();
            }
            template<class mut>
            bool wait_for(mut& mtx, std::chrono::milliseconds ms){
                return wait_until(mtx, std::chrono::high_resolution_clock::now() + ms);
            }
            template<class mut>
            bool wait_until(mut& mtx, std::chrono::high_resolution_clock::time_point time){
                bool ret;{
                    std::lock_guard<mutex> lock(_mutex);
                    mtx.unlock();
                    ret = _cond.wait_until(_mutex, time);
                }
                mtx.lock();
                return ret;
            }
        };
    }
}

#endif /* RUN_TIME_THREADING */
