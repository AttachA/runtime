#ifndef SRC_RUN_TIME_TASKS_UTIL_SIGNALS
#define SRC_RUN_TIME_TASKS_UTIL_SIGNALS

//Virtualized signals for windows and proxy for posix signals
//  implements only timer signals
namespace art{
    namespace interrupt {
        void init_interrupt_handler();
        
        struct interrupt_unsafe_region{
            interrupt_unsafe_region();
            ~interrupt_unsafe_region();
            static void lock();
            static void unlock();
        };



        struct timeval{
            long tv_sec;
            long tv_usec;
        };
        struct itimerval{
            struct timeval it_interval;
            struct timeval it_value;
        };
        
        bool timer_callback(void(*interrupter)());
        bool setitimer(const struct itimerval *new_value, struct itimerval *old_value);    
        void stop_timer();
    }
}

#endif /* SRC_RUN_TIME_TASKS_UTIL_SIGNALS */
