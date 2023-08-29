#ifndef CONFIGURATION_TASKS
#define CONFIGURATION_TASKS
#include <cstddef>

namespace configuration {
    namespace tasks {
#if defined(_DEBUG) || defined(DEBUG) || !defined(NDEBUG)
        constexpr bool enable_task_naming = true;
#else
        constexpr bool enable_task_naming = false;
#endif

        constexpr size_t max_running_tasks = 0;
        constexpr size_t max_planned_tasks = 0;

        namespace light_stack {
            constexpr size_t inital_buffer_size = 1; //compile time only
            constexpr bool flush_used_stacks = false;
            constexpr size_t max_buffer_size = 20; //0 for unlimited, -1 for disabled

            static_assert(inital_buffer_size <= max_buffer_size || !max_buffer_size, "inital_buffer_size must be less or equal max_buffer_size");
        }

        namespace scheduler {
            constexpr long long background_basic_quantum_ns = 15 * 1000000;
            constexpr long long low_basic_quantum_ns = 30 * 1000000;
            constexpr long long lower_basic_quantum_ns = 40 * 1000000;
            constexpr long long normal_basic_quantum_ns = 80 * 1000000;
            constexpr long long higher_basic_quantum_ns = 90 * 1000000;
            constexpr long long high_basic_quantum_ns = 120 * 1000000;


            constexpr long long background_max_quantum_ns = 30 * 1000000;
            constexpr long long low_max_quantum_ns = 60 * 1000000;
            constexpr long long lower_max_quantum_ns = 80 * 1000000;
            constexpr long long normal_max_quantum_ns = 160 * 1000000;
            constexpr long long higher_max_quantum_ns = 180 * 1000000;
            constexpr long long high_max_quantum_ns = 240 * 1000000;
        }
    }
}

#define _configuration_tasks_enable_task_naming_modifiable true
#define _configuration_tasks_max_running_tasks_modifiable true
#define _configuration_tasks_max_planned_tasks_modifiable true
#define _configuration_tasks_light_stack_flush_used_stacks_modifiable false
#define _configuration_tasks_light_stack_max_buffer_size_modifiable true

#define _configuration_tasks_enable_debug_mode false
#define _configuration_tasks_enable_preemptive_scheduler_preview true
#endif /* CONFIGURATION_TASKS */
