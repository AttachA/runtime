#ifndef SRC_RUN_TIME_TASKS_UTIL_SIGNALS
#define SRC_RUN_TIME_TASKS_UTIL_SIGNALS

//Virtualized signals for windows and proxy for posix signals
//  implements only timer signals
namespace art{
    namespace signals{
        void init_signals_handler();
        
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


        enum class TIMER_TYPE{
            REALTIME = 0,
            VIRTUAL = 1,
            PROF = 2
        };
        enum class SIGNAL_TYPE{
            SIGALRM = 0,
            SIGVTALRM = 1,
            SIGPROF = 2
        };
        //interrupter must be a function that takes no arguments and returns modifying any registers is not allowed 
        int sigaction(SIGNAL_TYPE signum, void(*interrupter)());
        int setitimer(TIMER_TYPE which, const struct itimerval *new_value, struct itimerval *old_value);    
        void stop_timers();
    }
}

#endif /* SRC_RUN_TIME_TASKS_UTIL_SIGNALS */
