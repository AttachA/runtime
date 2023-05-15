#ifndef CONFIGURATION_RUN_TIME
#define CONFIGURATION_RUN_TIME
namespace configuration {
    namespace run_time {
        enum class FaultActionByDefault {
            make_dump,
            show_error,
            dump_and_show_error,
            invite_to_debugger,
            system_default,
            ignore = system_default
        };
        enum class BreakPointActionByDefault {
            invite_to_debugger,
            throw_exception,
            ignore
        };
        enum class ExceptionOnLanguageRoutineActionByDefault {
            invite_to_debugger,
            nest_exception,
            swap_exception,
            ignore
        };

        
        #if defined(_DEBUG) || defined(DEBUG) || !defined(NDEBUG)
        constexpr FaultActionByDefault default_fault_action = FaultActionByDefault::invite_to_debugger;
        constexpr BreakPointActionByDefault break_point_action = BreakPointActionByDefault::invite_to_debugger;
        constexpr ExceptionOnLanguageRoutineActionByDefault exception_on_language_routine_action = ExceptionOnLanguageRoutineActionByDefault::invite_to_debugger;

        #else 
        constexpr FaultActionByDefault default_fault_action = FaultActionByDefault::make_dump;
        constexpr BreakPointActionByDefault break_point_action = BreakPointActionByDefault::throw_exception;
        constexpr ExceptionOnLanguageRoutineActionByDefault exception_on_language_routine_action = ExceptionOnLanguageRoutineActionByDefault::nest_exception;
        #endif

        constexpr bool allow_intern_access = false;
    }
}
#define _configuration_run_time_fault_action_modifable true
#define _configuration_run_time_break_point_action_modifable true
#define _configuration_run_time_exception_on_language_routine_action_modifable true
#define _configuration_run_time_allow_intern_access_modifable false
#endif /* CONFIGURATION_RUN_TIME */