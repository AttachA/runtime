#include <Windows.h>
#include "run_time.hpp"

#ifdef _WIN32
#include <windows.h>
size_t page_size = []() {
	SYSTEM_INFO si;
	GetSystemInfo(&si);
	return si.dwPageSize;
}();
#else
#include <unistd.h>
size_t page_size = sysconf(_SC_PAGESIZE);
#endif

unsigned long stack_overflow_case_stack_reserve_size = 524288;
thread_local unsigned long stack_size_tmp = 0;
thread_local bool need_stack_restore = false;

LONG NTAPI win_exception_handler(LPEXCEPTION_POINTERS e) {
	if (e->ExceptionRecord->ExceptionFlags == EXCEPTION_NONCONTINUABLE)
		return EXCEPTION_CONTINUE_SEARCH;
	switch (e->ExceptionRecord->ExceptionCode) {
	case EXCEPTION_ACCESS_VIOLATION:
		if (
			e->ExceptionRecord->ExceptionInformation[0] == 0 //thread attempted to read the inaccessible data
			||
			e->ExceptionRecord->ExceptionInformation[0] == 1 //thread attempted to write to an inaccessible address
			) throw SegmentationFaultException();
		else 
			return EXCEPTION_CONTINUE_SEARCH;//8 - thread attempted to execute to an inaccessible address
	case EXCEPTION_INT_DIVIDE_BY_ZERO:
	case EXCEPTION_FLT_DIVIDE_BY_ZERO:
		throw DevideByZeroException();
	case EXCEPTION_STACK_OVERFLOW: 
		need_stack_restore = true;
		throw StackOverflowException();
	case EXCEPTION_INT_OVERFLOW:
	case EXCEPTION_FLT_OVERFLOW:
	case EXCEPTION_FLT_STACK_CHECK:
	case EXCEPTION_FLT_UNDERFLOW:
		throw NumericOverflowException();
	case EXCEPTION_BREAKPOINT:
		throw UnusedDebugPointException();
	default:
		return EXCEPTION_CONTINUE_SEARCH;
	}
	return EXCEPTION_CONTINUE_EXECUTION;
}





thread_local bool ex_proxy_enabled = []() {
	ini_current();
	return true;
}();

void ini_current() {
	bool cur_siz = SetThreadStackGuarantee(&stack_size_tmp);
	stack_size_tmp += stack_overflow_case_stack_reserve_size;
	bool seted = SetThreadStackGuarantee(&stack_size_tmp);
	auto err = GetLastError();

	AddVectoredExceptionHandler(0, win_exception_handler);
}

NativeLib::NativeLib(const char* libray_path) {
	hGetProcIDDLL = LoadLibraryW(stringC::utf8::convert(libray_path).c_str());
	if (!hGetProcIDDLL)
		throw LibrayNotFoundException();
}
CALL_FUNC NativeLib::get_func(const char* func_name) {
	auto tmp = GetProcAddress((HMODULE)hGetProcIDDLL, func_name);
	if (!tmp)
		throw LibrayFunctionNotFoundException();
	return (CALL_FUNC)tmp;
}
size_t NativeLib::get_pure_func(const char* func_name) {
	size_t tmp = (size_t)GetProcAddress((HMODULE)hGetProcIDDLL, func_name);
	if (!tmp)
		throw LibrayFunctionNotFoundException();
	return tmp;
}
NativeLib::~NativeLib() {
	if (hGetProcIDDLL)
		FreeLibrary((HMODULE)hGetProcIDDLL);
}


bool restore_stack_fault() {
	if(!need_stack_restore)
		return false;

	bool is_successs = _resetstkoflw();
	if (!is_successs)
		std::exit(EXCEPTION_STACK_OVERFLOW);
	need_stack_restore = false;
	return true;
}
bool need_restore_stack_fault() {
	return need_stack_restore;
}