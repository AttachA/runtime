// Copyright Danyil Melnytskyi 2022-Present
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef SRC_BASE_RUN_TIME
#define SRC_BASE_RUN_TIME
#pragma once

#include <library/list_array.hpp>
#include <run_time/tasks.hpp>
#include <util/cxxException.hpp>
#include <util/enum_helper.hpp>
#include <util/exceptions.hpp>
#include <util/shared_ptr.hpp>
#include <util/ustring.hpp>

namespace art {
    typedef void* (*CALL_FUNC)(...);

    thread_local extern bool ex_proxy_enabled;
    extern size_t page_size;

    extern EventSystem unhandled_exception;
    extern EventSystem ex_fault;
    extern EventSystem errors;

#define DISABLE_RUNTIME_WARNING
    extern EventSystem warning;

#define DISABLE_RUNTIME_INFO
    extern EventSystem info;

    ENUM_ta(
        FaultAction,
        uint8_t,
        (ignore = system_default),
        make_dump,
        show_error,
        dump_and_show_error,
        invite_to_debugger,
        system_default
    );

    ENUM_t(
        BreakPointAction,
        uint8_t,
        invite_to_debugger,
        throw_exception,
        ignore
    );

    ENUM_t(
        ExceptionOnLanguageRoutineAction,
        uint8_t,
        invite_to_debugger,
        nest_exception,
        swap_exception,
        ignore
    );


    extern unsigned long fault_reserved_stack_size;
    extern unsigned long fault_reserved_pages;
    extern FaultAction default_fault_action;
    extern BreakPointAction break_point_action;
    extern ExceptionOnLanguageRoutineAction exception_on_language_routine_action;
    extern bool enable_thread_naming;
    extern bool allow_intern_access;

    bool restore_stack_fault();
    bool need_restore_stack_fault();

    void invite_to_debugger(const art::ustring& reason);
    bool _set_name_thread_dbg(const art::ustring& name);
    art::ustring _get_name_thread_dbg(unsigned long thread_id);
    unsigned long _thread_id();

    void ini_current();
    void modify_run_time_config(const art::ustring& name, const art::ustring& value);
    art::ustring get_run_time_config(const art::ustring& name);

    namespace DynamicCall {
        struct FunctionTemplate;
    }

    class NativeLib {
        void* hGetProcIDDLL;
        std::unordered_map<art::ustring, art::shared_ptr<class FuncEnvironment>, art::hash<art::ustring>> envs;

    public:
        NativeLib(const art::ustring& library_path);
        CALL_FUNC get_func(const art::ustring& func_name);
        art::shared_ptr<class FuncEnvironment> get_func_enviro(const art::ustring& func_name, const DynamicCall::FunctionTemplate& templ);
        art::shared_ptr<class FuncEnvironment> get_own_enviro(const art::ustring& func_name);
        size_t get_pure_func(const art::ustring& func_name);
        ~NativeLib();
    };
}


#endif /* SRC_BASE_RUN_TIME */
