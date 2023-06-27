// Copyright Danyil Melnytskyi 2022-2023
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once
#include "run_time/library/exceptions.hpp"
#include "library/list_array.hpp"
#include "run_time/tasks.hpp"
#include "run_time/cxxException.hpp"
#include "run_time/util/enum_helper.hpp"
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

ENUM_ta(FaultAction,uint8_t, 
	(make_dump)
	(show_error)
	(dump_and_show_error)
	(invite_to_debugger)
	(system_default)
	,
	(ignore = system_default)
);
ENUM_t(BreakPointAction, uint8_t,
	(invite_to_debugger)
	(throw_exception)
	(ignore)
);
ENUM_t(ExceptionOnLanguageRoutineAction, uint8_t,
    (invite_to_debugger)
    (nest_exception)
    (swap_exception)
    (ignore)
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

void invite_to_debugger(const std::string& reason);
bool _set_name_thread_dbg(const std::string& name);
std::string _get_name_thread_dbg(int thread_id);
int _thread_id();

void ini_current();
void modify_run_time_config(const std::string& name, const std::string& value);
std::string get_run_time_config(const std::string& name);
namespace DynamicCall {
	class FunctionTemplate;
}

class NativeLib {
	void* hGetProcIDDLL;
	std::unordered_map<std::string, typed_lgr<class FuncEnvironment>> envs;
public:
	NativeLib(const std::string& libray_path);
	CALL_FUNC get_func(const std::string& func_name);
	typed_lgr<class FuncEnvironment> get_func_enviro(const std::string& func_name, const DynamicCall::FunctionTemplate& templ);
	size_t get_pure_func(const std::string& func_name);
	~NativeLib();
};
