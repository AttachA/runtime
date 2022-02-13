#pragma once
#include "libray/exceptions.hpp"
#include "libray/list_array.hpp"
#include "libray/string_convert.hpp"
#include <thread>
typedef void* (*CALL_FUNC)(...);



thread_local extern bool ex_proxy_enabled;
extern size_t page_size;

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
