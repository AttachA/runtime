#ifndef CONFIGURATION_TASKS
#define CONFIGURATION_TASKS
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
            constexpr size_t inital_buffer_size = 1;//compile time only
            constexpr bool flush_used_stacks = false;
            constexpr size_t max_buffer_size = 20;//0 for unlimited, -1 for disabled

            static_assert(inital_buffer_size <= max_buffer_size || !max_buffer_size, "inital_buffer_size must be less or equal max_buffer_size");
        }
    }
}

#define _configuration_tasks_enable_task_naming_modifiable true
#define _configuration_tasks_max_running_tasks_modifiable true
#define _configuration_tasks_max_planned_tasks_modifiable true
#define _configuration_tasks_light_stack_flush_used_stacks_modifiable false
#define _configuration_tasks_light_stack_max_buffer_size_modifiable true

#define _configuration_tasks_enable_debug_mode false
#endif /* CONFIGURATION_TASKS */
