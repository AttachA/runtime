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
    }
}

#define _configuration_tasks_enable_task_naming_modifable true
#define _configuration_tasks_max_running_tasks_modifable true
#define _configuration_tasks_max_planned_tasks_modifable true
#endif /* CONFIGURATION_TASKS */
