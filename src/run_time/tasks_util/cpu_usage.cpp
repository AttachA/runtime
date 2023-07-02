// Copyright Danyil Melnytskyi 2022-2023
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#include "cpu_usage.hpp"

static double calculate_cpu_load(run_time::tasks::cpu::usage_prev_stat& prev_stat, uint64_t idle_ticks, uint64_t total_ticks) {
    uint64_t total_ticks_since_last_time = total_ticks - prev_stat.total_ticks;
    uint64_t idle_ticks_since_last_time  = idle_ticks - prev_stat.idle_ticks;

    double result = 1.0-
        (
            (total_ticks_since_last_time > 0) 
            ? ((double)idle_ticks_since_last_time)/total_ticks_since_last_time 
            : 0
        );

    prev_stat.total_ticks = total_ticks;
    prev_stat.idle_ticks = idle_ticks;
    return result;
}
#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>

namespace run_time{
    namespace tasks{
        namespace cpu{
            static uint64_t file_time_to_int64(const FILETIME& ft) {return (((uint64_t)(ft.dwHighDateTime))<<32)|((uint64_t)ft.dwLowDateTime);}

            double get_usage(usage_prev_stat& prev_stat){
                FILETIME  idle_time, kernel_time, user_time;
                if(GetSystemTimes(&idle_time, &kernel_time, &user_time))
                    return calculate_cpu_load(prev_stat,file_time_to_int64(idle_time), file_time_to_int64(kernel_time)+file_time_to_int64(user_time));
                else
                    return -1.0f;
            }
            
            double get_usage_percents(usage_prev_stat& prev_stat){
                auto tmp = get_usage(prev_stat);
                return tmp < 0 ? tmp : tmp*100;
            }
        }
    }
}
#elif defined(__linux__) || defined(__linux) || defined(linux) || defined(__gnu_linux__)
#include "sysinfo.h"
namespace run_time{
    namespace tasks{
        namespace cpu{
            double get_usage(usage_prev_stat& prev_stat){
                uint64_t cpu_use =0;
                uint64_t cpu_nic =0;
                uint64_t cpu_sys =0;
                uint64_t cpu_idl =0;
                uint64_t cpu_iow =0;
                uint64_t cpu_xxx =0;
                uint64_t cpu_yyy =0;
                uint64_t cpu_zzz =0;
                unsigned long pgpgin =0;
                unsigned long pgpgout =0;
                unsigned long pswpin =0;
                unsigned long pswpout =0;
                unsigned intr =0;
                unsigned ctxt =0;
                unsigned int running =0;
                unsigned int blocked =0;
                unsigned int dummy_1 =0;
                unsigned int dummy_2 =0;

                getstat(&cpu_use,&cpu_nic, &cpu_sys, &cpu_idl, &cpu_iow, &cpu_xxx, &cpu_yyy, &cpu_zzz, &pgpgin, &pgpgout, &pswpin, &pswpout, &intr, &ctxt, &running,&blocked, &dummy_1, &dummy_2);

                return calculate_cpu_load(prev_stat, cpu_idl, cpu_use+cpu_nic+cpu_sys+cpu_idl);
            }
            double get_usage_percents(usage_prev_stat& prev_stat){
                return get_usage(prev_stat) * 100;
            }
        }
    }
}
#endif