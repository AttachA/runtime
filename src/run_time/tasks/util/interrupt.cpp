#include <unordered_map>
#include <asmjit/asmjit.h>
#include <library/list_array.hpp>
#include <run_time/tasks/util/interrupt.hpp>
#include <run_time/tasks/util/native_workers_singleton.hpp>
#include <util/threading.hpp>
#include <util/platform.hpp>
void uninstall_timer_handle_local();

namespace art{
    namespace interrupt{
        struct timer_handle{
            void (*interrupt)();
            itimerval timer_value;
            void* timer_handle_ = nullptr;
            thread::id thread_id;
            std::atomic_size_t guard_zones = 0;
            bool enabled_timers = true;
            ~timer_handle() {
                uninstall_timer_handle_local();
            }
        };
        thread_local timer_handle timer;
        

        
        
        interrupt_unsafe_region::interrupt_unsafe_region(){
            lock();
        }
        interrupt_unsafe_region::~interrupt_unsafe_region(){
            unlock();
        }
        void interrupt_unsafe_region::lock(){
            timer.guard_zones++;

        }
        void interrupt_unsafe_region::unlock(){
            timer.guard_zones--;
        }
    }
}
#if PLATFORM_WINDOWS
#include <windows.h>
namespace art{
    namespace interrupt {
        
        
        

        list_array<timer_handle*> await_timers;
        art::condition_variable timer_signal;
        art::mutex timer_signal_mutex;

        

        void init_interrupt_handler(){
            art::thread([](){
                art::unique_lock<art::mutex> lock(timer_signal_mutex, art::defer_lock);
                while(true){
                    lock.lock();
                    while(await_timers.empty())
                        timer_signal.wait(lock);
                    timer_handle* data = await_timers.take_front();
                    lock.unlock();
                    if(data != nullptr){
                        //get thread handle
                        auto id = data->thread_id;
                        if(art::thread::suspend(id) == false)
                            continue;

                        if (!data->enabled_timers) {
                            art::thread::resume(id);
                            continue;
                        }
                        if(data->guard_zones != 0){
                            art::thread::resume(id);
                            lock.lock();
                            await_timers.push_back(data);
                            lock.unlock();
                            continue;
                        }
                        art::thread::insert_context(id, (void(*)(void*))data->interrupt, nullptr);
                        art::thread::resume(id);
                    }
                }
            });
        }
        

        VOID NTAPI timer_callback_fun(void* callback, BOOLEAN){
            timer_handle* timer = (timer_handle*)callback;
            if(timer->thread_id == this_thread::get_id())
                return;
            std::unique_lock<art::mutex> lock(timer_signal_mutex);
            await_timers.push_back(timer);
            timer_signal.notify_all();
        }

        bool timer_callback(void(*interrupter)()){
            timer.interrupt = interrupter;
            return true;
        }
        bool setitimer(const struct itimerval *new_value, struct itimerval *old_value){
            interrupt_unsafe_region guard;
            if(old_value)
                *old_value = timer.timer_value;
            if(new_value == nullptr)
                return false;
            
            timer.timer_value = *new_value;
            timer.thread_id = this_thread::get_id();
            if(timer.timer_handle_ != nullptr){
                DeleteTimerQueueTimer(NULL, timer.timer_handle_, INVALID_HANDLE_VALUE);
                timer.timer_handle_ = nullptr;
            }
            if(CreateTimerQueueTimer(
                        &timer.timer_handle_,
                        NULL,
                        timer_callback_fun, 
                        &timer,
                        new_value->it_value.tv_sec * 1000 + new_value->it_value.tv_usec / 1000,
                        new_value->it_interval.tv_sec * 1000 + new_value->it_interval.tv_usec / 1000,
                        WT_EXECUTEINTIMERTHREAD
            ) == 0
            ){
                return false;
            }
            timer.enabled_timers = true;
            return true;
        }
        void stop_timer(){
            interrupt_unsafe_region region;
            if(timer.timer_handle_ != nullptr){
                DeleteTimerQueueTimer(NULL, timer.timer_handle_, INVALID_HANDLE_VALUE);
                timer.timer_handle_ = nullptr;
            }
            timer.enabled_timers = false;
        }

    }
}
void uninstall_timer_handle_local(){
    art::interrupt::stop_timer();
    art::lock_guard<art::mutex> lock(art::interrupt::timer_signal_mutex);
    art::interrupt::await_timers.remove_if([](art::interrupt::timer_handle* timer){
        return timer->thread_id == art::this_thread::get_id();
    });
}
#else
#include <signal.h>
#include <sys/time.h>
namespace art{
    namespace signals{
        void install_on_stack(){
            
        }

        void init_signals_handler(){}
        int sigaction(SIGNAL_TYPE signum, void(*interrupter)()){
            timer.sigprof = interrupter;
            struct sigaction action;
            action.sa_handler = interrupter;
            sigemptyset(&action.sa_mask);
            action.sa_flags = 0;
            switch(signum){
                case SIGNAL_TYPE::SIGALRM:
                    timer.sigprof = interrupter;
                    return ::sigaction(SIGALRM, &action, nullptr);
                case SIGNAL_TYPE::SIGVTALRM:
                    return ::sigaction(SIGVTALRM, &action, nullptr);
                case SIGNAL_TYPE::SIGPROF:
                    return ::sigaction(SIGPROF, &action, nullptr);
            }
            return -1;
        }
        int setitimer(TIMER_TYPE which, const struct itimerval *new_value, struct itimerval *old_value){
            return ::setitimer((int)which, new_value, old_value);
        }
        void stop_timers(){
            struct itimerval timer;
            timer.it_interval.tv_sec = 0;
            timer.it_interval.tv_usec = 0;
            timer.it_value.tv_sec = 0;
            timer.it_value.tv_usec = 0;
            ::setitimer(ITIMER_PROF, &timer, nullptr);
            ::setitimer(ITIMER_REAL, &timer, nullptr);
            ::setitimer(ITIMER_VIRTUAL, &timer, nullptr);
        }
    }
}

#endif

void * operator new(std::size_t n) throw(std::bad_alloc)
{
    art::interrupt::interrupt_unsafe_region region;
    void* ptr = malloc(n);
    if(ptr == nullptr)
        throw std::bad_alloc();
    return ptr;
}
void operator delete(void * p) throw()
{
    art::interrupt::interrupt_unsafe_region region;
    free(p);
}
void *operator new[](std::size_t s) throw(std::bad_alloc)
{
    art::interrupt::interrupt_unsafe_region region;
    void* ptr = malloc(s);
    if(ptr == nullptr)
        throw std::bad_alloc();
    return ptr;
}
void operator delete[](void *p) throw()
{
    art::interrupt::interrupt_unsafe_region region;
    free(p);
}