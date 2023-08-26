#include <unordered_map>
#include <asmjit/asmjit.h>
#include <library/list_array.hpp>
#include <run_time/tasks_util/signals.hpp>
#include <run_time/tasks_util/native_workers_singleton.hpp>
#include <util/threading.hpp>
#include <util/platform.hpp>
extern "C" void signal_interrupter_asm_zmm();
extern "C" void signal_interrupter_asm_ymm();
extern "C" void signal_interrupter_asm_xmm();
extern "C" void signal_interrupter_asm_xmm_small();
extern "C" void signal_interrupter_asm();
void(*signal_interrupter_asm_ptr)() = [](){
        //get current cpu abilities, use asmjit
        asmjit::JitRuntime jrt;
        auto& features = jrt.cpuFeatures().x86();
        if(features.hasAVX512_VL())
            return signal_interrupter_asm_zmm;
        if(features.hasAVX())
            return signal_interrupter_asm_ymm;
        if(features.hasSSE())
            return signal_interrupter_asm_xmm;
        return signal_interrupter_asm;
    }();
namespace art{
    namespace signals{
        struct timer_handle{
            void (*sigprof)() = nullptr;
            void (*sigalarm)() = nullptr;
            void (*sigvalarm)() = nullptr;
            itimerval timer_value;
            void* timer_handle_ = nullptr;
            thread::id thread_id;
            std::atomic_size_t guard_zones = 0;
            bool enabled_timers = true;
            ~timer_handle() {
                stop_timers();
            }
        };
        struct timer_data {
            void (*interrupter)();
            timer_handle* thread_timers;
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
    



    namespace signals{
        
        
        

        list_array<timer_data> await_timers;
        art::condition_variable timer_signal;
        art::mutex timer_signal_mutex;

        
        struct HANDLE_CLOSER {
            HANDLE handle = nullptr;
            HANDLE_CLOSER(HANDLE handle) : handle(handle) {}
            ~HANDLE_CLOSER() { 
                if(handle != nullptr)
                    CloseHandle(handle);
                handle = nullptr;
            }
        };
        void init_signals_handler(){
            art::thread([](){
                art::unique_lock<art::mutex> lock(timer_signal_mutex, art::defer_lock);
                while(true){
                    lock.lock();
                    while(await_timers.empty())
                        timer_signal.wait(lock);
                    timer_data data = await_timers.take_front();
                    lock.unlock();
                    if(data.thread_timers != nullptr){
                        //get thread handle
                        HANDLE_CLOSER thread_handle(OpenThread(THREAD_SUSPEND_RESUME | THREAD_QUERY_INFORMATION  | THREAD_GET_CONTEXT | THREAD_SET_CONTEXT, false, data.thread_timers->thread_id._id));
                        if(SuspendThread(thread_handle.handle) == -1)
                            continue;
                        if (!data.thread_timers->enabled_timers) {
                            ResumeThread(thread_handle.handle);
                            continue;
                        }
                        if(data.thread_timers->guard_zones != 0){
                            ResumeThread(thread_handle.handle);
                            lock.lock();
                            await_timers.push_back(data);
                            lock.unlock();
                            continue;
                        }
                        
                        //set thread context
                        CONTEXT context;
                        context.ContextFlags = CONTEXT_CONTROL;
                        if(GetThreadContext(thread_handle.handle, &context) == 0){
                            DWORD error = GetLastError();
                            ResumeThread(thread_handle.handle);
                            continue;
                        }
                        try{
                            auto rsp = context.Rsp;
                            rsp -= sizeof(DWORD64);
                            *(DWORD64*)rsp = context.Rip;//return address
                            rsp -= sizeof(DWORD64);
                            *(DWORD64*)rsp = (DWORD64)data.interrupter;//interrupter
                            context.Rsp = rsp;
                            //set rip to trampoline
                            context.Rip = (DWORD64)signal_interrupter_asm_ptr;
                            SetThreadContext(thread_handle.handle, &context);
                        }catch(...){}
                        ResumeThread(thread_handle.handle);
                    }
                }
            });
        }
        

        VOID NTAPI timer_callback_fun(void* callback, BOOLEAN){
            timer_handle* timer = (timer_handle*)callback;
            if(timer->thread_id == this_thread::get_id())
                return;
            std::unique_lock<art::mutex> lock(timer_signal_mutex);
            await_timers.push_back({timer->sigprof, timer});
            timer_signal.notify_all();
        }

        int sigaction(SIGNAL_TYPE signum, void(*interrupter)()){
            switch(signum){
                case SIGNAL_TYPE::SIGALRM:
                    timer.sigalarm = interrupter;
                    break;
                case SIGNAL_TYPE::SIGVTALRM:
                    timer.sigvalarm = interrupter;
                    break;
                case SIGNAL_TYPE::SIGPROF:
                    timer.sigprof = interrupter;
                    break;
            }
            return 0;
        }
        int setitimer(TIMER_TYPE which, const struct itimerval *new_value, struct itimerval *old_value){
            interrupt_unsafe_region guard;
            if(old_value)
                *old_value = timer.timer_value;
            if(new_value == nullptr)
                return -1;
            
            timer.timer_value = *new_value;
            timer.thread_id = this_thread::get_id();
            auto args = (void (*)())nullptr;
            switch(which){
                case TIMER_TYPE::REALTIME:
                    args = timer.sigalarm;
                    break;
                case TIMER_TYPE::VIRTUAL:
                    args = timer.sigvalarm;
                    break;
                case TIMER_TYPE::PROF:
                    args = timer.sigprof;
                    break;
            }
            if(args == nullptr)
                return -1;
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
                return -1;
            }
            timer.enabled_timers = true;
            return 0;
        }
        void stop_timers(){
            interrupt_unsafe_region region;
            if(timer.timer_handle_ != nullptr){
                DeleteTimerQueueTimer(NULL, timer.timer_handle_, INVALID_HANDLE_VALUE);
                timer.timer_handle_ = nullptr;
            }
            timer.enabled_timers = false;
        }
    }
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

void * operator new(std::size_t n) throw(std::bad_alloc)
{
    art::signals::interrupt_unsafe_region region;
    void* ptr = malloc(n);
    if(ptr == nullptr)
        throw std::bad_alloc();
    return ptr;
}
void operator delete(void * p) throw()
{
    art::signals::interrupt_unsafe_region region;
    free(p);
}
void *operator new[](std::size_t s) throw(std::bad_alloc)
{
    art::signals::interrupt_unsafe_region region;
    void* ptr = malloc(s);
    if(ptr == nullptr)
        throw std::bad_alloc();
    return ptr;
}
void operator delete[](void *p) throw()
{
    art::signals::interrupt_unsafe_region region;
    free(p);
}
#endif
