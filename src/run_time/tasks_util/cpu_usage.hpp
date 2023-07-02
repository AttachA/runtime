// Copyright Danyil Melnytskyi 2022-2023
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef RUN_TIME_TASKS_UTIL_CPU_USAGE
#define RUN_TIME_TASKS_UTIL_CPU_USAGE
#include <cstdint>
namespace run_time{
    namespace tasks{
        namespace cpu{
            struct usage_prev_stat{
                uint64_t total_ticks = 0;
                uint64_t idle_ticks = 0;
            };
            double get_usage(usage_prev_stat& prev_stat);
            double get_usage_percents(usage_prev_stat& prev_stat);
        }
    }
}


#endif /* RUN_TIME_TASKS_UTIL_CPU_USAGE */
