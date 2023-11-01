#ifndef SRC_RUN_TIME_ASM_ATTACHA_ENVIRONMENT
#define SRC_RUN_TIME_ASM_ATTACHA_ENVIRONMENT
#include <asmjit/asmjit.h>
#include <run_time/ValueEnvironment.hpp>
#include <run_time/tasks.hpp>

namespace art {


    struct frame_info {
        art::ustring name;
        art::ustring file;
        size_t fun_size = 0;
    };

    struct FrameSymbols {
        std::unordered_map<uint8_t*, frame_info, art::hash<uint8_t*>> map;
        bool destroyed = false;

        FrameSymbols() {}

        ~FrameSymbols() {
            map.clear();
            destroyed = true;
        }
    };

    class attacha_environment {
        struct function_globals_handle;

        struct code_gen_handle {
            TaskMutex frame_symbols_lock;
            FrameSymbols frame_symbols;
            asmjit::JitRuntime run_time;
#if PLATFORM_WINDOWS
            art::mutex DbgHelp_lock;
#endif
        };

        TaskRecursiveMutex mutex;
        ValueEnvironment* value_globals;
        function_globals_handle* function_globals;
        code_gen_handle* code_gen;
        static attacha_environment self;
        attacha_environment();

        static function_globals_handle* create_function_globals();

        static void remove_function_globals(function_globals_handle*);


    public:
        ~attacha_environment();
        static function_globals_handle& get_function_globals();
        static ValueEnvironment& get_value_globals();
        static code_gen_handle& get_code_gen();

        static art::shared_ptr<class FuncEnvironment>& create_fun_env(class FuncEnvironment* ptr);
    };
} // namespace art

#endif /* SRC_RUN_TIME_ASM_ATTACHA_ENVIRONMENT */
