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


extern unsigned long fault_reserved_stack_size;
extern FaultActionByDefault default_fault_action;
bool restore_stack_fault();
bool need_restore_stack_fault();

void ini_current();

class NativeLib {
	void* hGetProcIDDLL;
public:
	NativeLib(const char* libray_path);
	CALL_FUNC get_func(const char* func_name);
	size_t get_pure_func(const char* func_name);
	~NativeLib();
};
