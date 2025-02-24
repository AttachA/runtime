// Copyright Danyil Melnytskyi 2022-Present
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef SRC_RUN_TIME_ASM_ATTACHA_ENVIRONMENT
#define SRC_RUN_TIME_ASM_ATTACHA_ENVIRONMENT
#include <asmjit/asmjit.h>
#include <run_time/tasks.hpp>
#include <run_time/types_global.hpp>
#include <run_time/values_global.hpp>

namespace art {


    struct frame_info {
        art::ustring name;
        art::ustring file;
        size_t fun_size = 0;

        struct line_info {
            uint64_t offset_begin;
            uint64_t offset_end;
            art::line_info abstracted;
        };

        list_array<line_info> line_infos;
    };

    struct FrameSymbols {
        std::unordered_map<uint8_t*, frame_info, art::hash<uint8_t*>> map;
        bool destroyed = false;

        FrameSymbols() = default;

        ~FrameSymbols() {
            map.clear();
            destroyed = true;
        }
    };

    class attacha_environment {
        class function_globals_handle : public protected_value<
                                            std::unordered_map<
                                                art::ustring,
                                                art::shared_ptr<FuncEnvironment>,
                                                art::hash<art::ustring>>> {
        };

        struct code_gen_handle {
            TaskMutex frame_symbols_lock;
            FrameSymbols frame_symbols;
            asmjit::JitRuntime run_time;
#if PLATFORM_WINDOWS
            art::mutex DbgHelp_lock;
#endif
        };

        TaskRecursiveMutex mutex;
        typed_lgr<values_global> _value_global;
        typed_lgr<types_global> _types_global;
        typed_lgr<function_globals_handle> function_globals;
        typed_lgr<code_gen_handle> code_gen;
        static attacha_environment self;
        attacha_environment() = default;

    public:
        static function_globals_handle& get_function_globals();
        static typed_lgr<values_global> get_value_globals();
        static typed_lgr<types_global> get_types_global();
        static code_gen_handle& get_code_gen();
        static void clean_up();


        static ValueItem* find_global_value(const art::ustring& str);
        static ValueItem* find_global_value_local(const art::ustring& str);
        static ValueItem* find_global_value_auto_join(const art::ustring& str, const art::ustring& separator);
        static ValueItem* find_global_value_local_auto_join(const art::ustring& str, const art::ustring& separator);

        static typed_lgr<types_global> get_type(std::initializer_list<art::ustring> str);
        static ValueItem& get_value(std::initializer_list<art::ustring> str);
        static ValueItem& get_value_strict(std::initializer_list<art::ustring> str);

        static art::shared_ptr<class FuncEnvironment>& create_fun_env(class FuncEnvironment* ptr);
    };
} // namespace art

#endif /* SRC_RUN_TIME_ASM_ATTACHA_ENVIRONMENT */
