// Copyright Danyil Melnytskyi 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once
#include "run_time/library/exceptions.hpp"
#include "library/list_array.hpp"
#include "run_time/tasks.hpp"
#include "library/string_convert.hpp"
#include "run_time/cxxException.hpp"
#include <thread>
#include "run_time/dynamic_call.hpp"
typedef void* (*CALL_FUNC)(...);

thread_local extern bool ex_proxy_enabled;
extern size_t page_size;

extern EventSystem unhandled_exception;
extern EventSystem ex_fault;

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



extern unsigned long fault_reserved_stack_size;
extern FaultActionByDefault default_fault_action;
extern BreakPointActionByDefault break_point_action;
extern bool enable_thread_naming;
bool restore_stack_fault();
bool need_restore_stack_fault();

bool _set_name_thread_dbg(const std::string& name);

std::string _get_name_thread_dbg(int thread_id);

int _thread_id();

void ini_current();


class NativeLib {
	void* hGetProcIDDLL;
	std::unordered_map<std::string, typed_lgr<class FuncEnviropment>> envs;
public:
	NativeLib(const char* libray_path);
	CALL_FUNC get_func(const char* func_name);
	typed_lgr<class FuncEnviropment> get_func_enviro(const char* func_name, const DynamicCall::FunctionTemplate& templ);
	size_t get_pure_func(const char* func_name);
	~NativeLib();
};
