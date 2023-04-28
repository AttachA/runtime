// Copyright Danyil Melnytskyi 2022-2023
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifdef _WIN32
#pragma comment(lib,"Dbghelp.lib")
#include <Windows.h>
#include <Dbghelp.h>
#include <Psapi.h>
#include <format>
#include <chrono>
#include "library/string_convert.hpp"
size_t page_size = []() {
	SYSTEM_INFO si;
	GetSystemInfo(&si);
	return si.dwPageSize;
}();

#include "run_time.hpp"
#include <sstream>
#include "library/string_help.hpp"
#include <filesystem>
#include "run_time/FuncEnviropment.hpp"
#include "run_time/tasks.hpp"

unsigned long fault_reserved_stack_size = 0;// 524288;
unsigned long fault_reserved_pages = fault_reserved_stack_size / page_size + (fault_reserved_stack_size % page_size ? 1 : 0);
thread_local unsigned long stack_size_tmp = 0;
thread_local bool need_stack_restore = false;
bool enable_thread_naming = true;

bool _set_name_thread_dbg(const std::string& name) {
	auto data = stringC::utf8::convert(name);
	return SUCCEEDED(SetThreadDescription(GetCurrentThread(), data.c_str()));
}

std::string _get_name_thread_dbg(int thread_id) {
	HANDLE thread = OpenThread(THREAD_QUERY_LIMITED_INFORMATION,false,thread_id);
	if (!thread)
		return "";
	WCHAR* res;
	if (SUCCEEDED(GetThreadDescription(thread, &res))) {
		std::string str = stringC::utf8::convert(res);
		LocalFree(res);
		CloseHandle(thread);
		return str;
	}
	else {
		return "";
	}
}

int _thread_id() {
	return GetCurrentThreadId();
}


EventSystem unhandled_exception;
EventSystem ex_fault;
EventSystem errors;
EventSystem info;
#if _DEBUG
FaultActionByDefault default_fault_action = FaultActionByDefault::invite_to_debugger;
BreakPointActionByDefault break_point_action = BreakPointActionByDefault::invite_to_debugger;
#else 
FaultActionByDefault default_fault_action = FaultActionByDefault::make_dump;
BreakPointActionByDefault break_point_action = BreakPointActionByDefault::throw_exception;
#endif

LONG NTAPI win_exception_handler(LPEXCEPTION_POINTERS e) {
	if (e->ExceptionRecord->ExceptionFlags == EXCEPTION_NONCONTINUABLE)
		return EXCEPTION_CONTINUE_SEARCH;
	switch (e->ExceptionRecord->ExceptionCode) {
	case EXCEPTION_ACCESS_VIOLATION:
		if (
			e->ExceptionRecord->ExceptionInformation[0] == 0 //thread attempted to read the inaccessible data
			||
			e->ExceptionRecord->ExceptionInformation[0] == 1 //thread attempted to write to an inaccessible address
		) {
			if(Task::is_task()){
				if (e->ExceptionRecord->ExceptionInformation[1] < UINT16_MAX)
					throw NullPointerException("Task 0x" + string_help::hexsstr(Task::task_id()) + " attempted to " + std::string(e->ExceptionRecord->ExceptionInformation[0] ? "write" : "read") + " in null pointer region. 0x" + string_help::hexstr(e->ExceptionRecord->ExceptionInformation[1]));
				else 
					throw SegmentationFaultException("Task 0x" + string_help::hexsstr(Task::task_id()) + " attempted to " + std::string(e->ExceptionRecord->ExceptionInformation[0] ? "write" : "read") + " in non mapped region. 0x" + string_help::hexstr(e->ExceptionRecord->ExceptionInformation[1]));
		
			}else{
				if (e->ExceptionRecord->ExceptionInformation[1] < UINT16_MAX)
					throw NullPointerException("Thread " + string_help::hexsstr(std::hash<std::thread::id>()(std::this_thread::get_id())) + " attempted to " + std::string(e->ExceptionRecord->ExceptionInformation[0] ? "write" : "read") + " in null pointer region. 0x" + string_help::hexstr(e->ExceptionRecord->ExceptionInformation[1]));
				else 
					throw SegmentationFaultException("Thread " + string_help::hexsstr(std::hash<std::thread::id>()(std::this_thread::get_id())) + " attempted to " + std::string(e->ExceptionRecord->ExceptionInformation[0] ? "write" : "read") + " in non mapped region. 0x" + string_help::hexstr(e->ExceptionRecord->ExceptionInformation[1]));
			}
		}
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
		switch (break_point_action){
		case BreakPointActionByDefault::invite_to_debugger: {
			auto test = "Oops!,\n This program caught unhandled breakpoint, \nif you need debug program,\n attach to process with id: " + std::to_string(GetCurrentProcessId()) + ",\n then switch to thread id: " + std::to_string(GetCurrentThreadId()) + " and click OK";
			MessageBoxA(NULL, test.c_str(), "Debug invite", MB_ICONQUESTION);
			break;
		}
		case BreakPointActionByDefault::throw_exception:
			throw UnusedDebugPointException();
		default:
		case BreakPointActionByDefault::ignore:
#ifdef _AMD64_
			e->ContextRecord->Rip++;
#else
			e->ContextRecord->Eip++;
#endif
			break;
		}
		break;
	case EXCEPTION_ILLEGAL_INSTRUCTION:
		throw BadInstructionException();
	case EXCEPTION_PRIV_INSTRUCTION:
		throw BadInstructionException("This instruction is privileged");
	default:
		return EXCEPTION_CONTINUE_SEARCH;
	}
	return EXCEPTION_CONTINUE_EXECUTION;
}


void show_err(CXXExInfo& cxx) {
	std::string ex_str;
	if (hasClassInEx(cxx,"AttachARuntimeException")) {
		ex_str = "Caught to unhandled AttachA ";
		ex_str += cxx.ty_arr[0].ty_info->name();
		ex_str += " exception.\n";
		ex_str += ((AttachARuntimeException*)cxx.ex_ptr)->what();
		MessageBoxA(NULL, ex_str.c_str(), "Unhandled AttachA exception", MB_ICONERROR);
	}
	else {
		ex_str = "Caught to unhandled c++ ";
		ex_str += cxx.ty_arr[0].ty_info->name();
		ex_str += " exception.\n";
		if (hasClassInEx(cxx, "std::exception"))
			ex_str += ((std::exception*)cxx.ex_ptr)->what();
		MessageBoxA(NULL, ex_str.c_str(), "Unhandled C++ exception", MB_ICONERROR);
	}
	std::exit(-1);
}
void show_err(LPEXCEPTION_POINTERS e) {
	std::stringstream ss;
	ss << "Caught to unhandled seh exception.\n";
	ss << "Ex code: "  << e->ExceptionRecord->ExceptionCode <<'\n';
	ss << "Ex addr: " << e->ExceptionRecord->ExceptionAddress << '\n';
	DWORD maxi = e->ExceptionRecord->NumberParameters;
	ss << "Params: " << maxi << '\n';
	for (size_t i = 0; i < maxi; i++) 
		ss << "\tp" << i << ':' << e->ExceptionRecord->ExceptionInformation[i] << '\n';

	MessageBoxA(NULL, ss.str().c_str(), "Unhandled seh exception", MB_ICONERROR);
	std::exit(-1);
}


void make_dump(LPEXCEPTION_POINTERS e, CXXExInfo* cxx) {
	std::filesystem::path path;
	{
		constexpr uint16_t limit = 65535;
		wchar_t* tmp;
		try {
			tmp = new wchar_t[limit];
		}
		catch (...) {
			return;
		}
		DWORD len = GetEnvironmentVariableW(L"AttachA_dump_path", tmp, limit);
		if (!len) {
			path = std::filesystem::current_path();
		}
		else {
			path = std::wstring(tmp, tmp + len);
		}
		len = GetModuleFileNameExW(GetCurrentProcess(), nullptr, tmp, limit);
		if (!len) {
			path /= "AttachA";
		}
		else {
			path /= std::filesystem::path(std::wstring(tmp, tmp + len)).stem();
		}
		delete[] tmp;
	}
	path += " exception fault ";
	if(cxx)
		path += cxx->ty_arr[0].ty_info->name();
	path += std::format(" {}.dmp", (uint16_t)std::hash<long long>()(std::chrono::system_clock::now().time_since_epoch().count()));
	HANDLE hndl = CreateFileW(path.native().c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_ARCHIVE, 0);
	if (hndl == INVALID_HANDLE_VALUE)
		return;
	auto flags =
		MiniDumpWithFullMemory |
		MiniDumpWithFullMemoryInfo |
		MiniDumpWithHandleData |
		MiniDumpWithUnloadedModules |
		MiniDumpWithThreadInfo;

	MINIDUMP_EXCEPTION_INFORMATION info;
	info.ClientPointers = false;
	info.ExceptionPointers = e;
	info.ThreadId = GetCurrentThreadId();

	MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hndl, (MINIDUMP_TYPE)flags, &info, NULL, NULL);
	CloseHandle(hndl);
}

LONG WINAPI win_fault_handler(LPEXCEPTION_POINTERS e) {
	if (e->ExceptionRecord->ExceptionCode == 0xe06d7363) {
		CXXExInfo cxx;
		getCxxExInfoFromNative(cxx, e);
		{
			ValueItem val(&cxx,ValueMeta(VType::undefined_ptr,false,false));
			unhandled_exception.sync_notify(val);
		}
		switch (default_fault_action) {
		case FaultActionByDefault::show_error:
			show_err(cxx);
			break;
		case FaultActionByDefault::dump_and_show_error:
			make_dump(e, &cxx);
			show_err(cxx);
			break;
		case FaultActionByDefault::make_dump: 
			make_dump(e, &cxx);
			break;
		case FaultActionByDefault::invite_to_debugger: {
			auto test = "Hello! In this program unhandled exception occoured,\n if you wanna debug it, attach to process with id: " + std::to_string(GetCurrentProcessId()) + ",\n then switch to thread id: " + std::to_string(GetCurrentThreadId()) + " and click OK";
			MessageBoxA(NULL, test.c_str(), "Debug invite", MB_ICONQUESTION);
			break;
		}
		case FaultActionByDefault::system_default:
		default:
			break;
		}
	}
	else {
		ValueItem noting;
		ex_fault.sync_notify(noting);
		switch (default_fault_action) {
		case FaultActionByDefault::show_error:
			show_err(e);
			break;
		case FaultActionByDefault::dump_and_show_error:
			make_dump(e, nullptr);
			show_err(e);
			break;
		case FaultActionByDefault::make_dump:
			make_dump(e, nullptr);
			break;
		case FaultActionByDefault::invite_to_debugger: {
			auto test = "Hello! In this program unhandled exception occoured,\n if you wanna debug it, attach to process with id: " + std::to_string(GetCurrentProcessId()) + ",\n then switch to thread id: " + std::to_string(GetCurrentThreadId()) + " and click to yes to look callstack";
			MessageBoxA(NULL, test.c_str(), "Debug invite", MB_YESNO | MB_ICONQUESTION);
			break;
		}
		case FaultActionByDefault::system_default:
		default:
			break;
		}
	}
	return EXCEPTION_CONTINUE_SEARCH;
}


void ini_current() {
	bool cur_siz = SetThreadStackGuarantee(&stack_size_tmp);
	stack_size_tmp += fault_reserved_stack_size;
	bool seted = SetThreadStackGuarantee(&stack_size_tmp);
	auto err = GetLastError();

	AddVectoredExceptionHandler(0, win_exception_handler);
	SetUnhandledExceptionFilter(win_fault_handler);
}

thread_local bool ex_proxy_enabled = []() {
	ini_current();
	return true;
}();


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
typed_lgr<FuncEnviropment> NativeLib::get_func_enviro(const char* func_name, const DynamicCall::FunctionTemplate& templ) {
	auto& env = envs[func_name];
	if (!env) {
		DynamicCall::PROC tmp = (DynamicCall::PROC)GetProcAddress((HMODULE)hGetProcIDDLL, func_name);
		if (!tmp)
			throw LibrayFunctionNotFoundException();
		return env = new FuncEnviropment(tmp, templ, true);
	}
	return env;
}
size_t NativeLib::get_pure_func(const char* func_name) {
	size_t tmp = (size_t)GetProcAddress((HMODULE)hGetProcIDDLL, func_name);
	if (!tmp)
		throw LibrayFunctionNotFoundException();
	return tmp;
}
NativeLib::~NativeLib() {
	for (auto&[_, it] : envs)
		it->ForceUnload();
	envs = {};
	if (hGetProcIDDLL){
		FreeLibrary((HMODULE)hGetProcIDDLL);
		hGetProcIDDLL = nullptr;
	}
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
#else
#include <unistd.h>
size_t page_size = sysconf(_SC_PAGESIZE);












#endif