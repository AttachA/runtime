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
#include <utf8cpp/utf8.h>
#include "configuration/run_time.hpp"
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
bool enable_thread_naming = configuration::tasks::enable_thread_naming;
bool allow_intern_access = configuration::run_time::allow_intern_access;


FaultAction default_fault_action = (FaultAction)configuration::run_time::default_fault_action;
BreakPointAction break_point_action = (BreakPointAction)configuration::run_time::break_point_action;
ExceptionOnLanguageRoutineAction exception_on_language_routine_action = (ExceptionOnLanguageRoutineAction)configuration::run_time::exception_on_language_routine_action;

bool _set_name_thread_dbg(const std::string& name) {
	std::u16string result;
	utf8::utf8to16(name.begin(), name.end(), std::back_inserter(result));
	return SUCCEEDED(SetThreadDescription(GetCurrentThread(), (wchar_t*)result.c_str()));
}

std::string _get_name_thread_dbg(int thread_id) {
	HANDLE thread = OpenThread(THREAD_QUERY_LIMITED_INFORMATION,false,thread_id);
	if (!thread)
		return "";
	WCHAR* res;
	if (SUCCEEDED(GetThreadDescription(thread, &res))) {
		std::string result;
		utf8::utf16to8(res, res + wcslen(res), std::back_inserter(result));
		LocalFree(res);
		CloseHandle(thread);
		return result;
	}
	else 
		return "";
}

int _thread_id() {
	return GetCurrentThreadId();
}


EventSystem unhandled_exception;
EventSystem ex_fault;
EventSystem errors;
EventSystem warning;
EventSystem info;

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
		case BreakPointAction::invite_to_debugger: 
			invite_to_debugger("Oops!,\n This program caught unhandled breakpoint");
			break;
		case BreakPointAction::throw_exception:
			throw UnusedDebugPointException();
		default:
		case BreakPointAction::ignore:
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
		case FaultAction::show_error:
			show_err(cxx);
			break;
		case FaultAction::dump_and_show_error:
			make_dump(e, &cxx);
			show_err(cxx);
			break;
		case FaultAction::make_dump: 
			make_dump(e, &cxx);
			break;
		case FaultAction::invite_to_debugger: {
			invite_to_debugger("In this program unhandled exception occoured");
			break;
		}
		case FaultAction::system_default:
		default:
			break;
		}
	}
	else {
		ValueItem noting;
		ex_fault.sync_notify(noting);
		switch (default_fault_action) {
		case FaultAction::show_error:
			show_err(e);
			break;
		case FaultAction::dump_and_show_error:
			make_dump(e, nullptr);
			show_err(e);
			break;
		case FaultAction::make_dump:
			make_dump(e, nullptr);
			break;
		case FaultAction::invite_to_debugger: {
			invite_to_debugger("In this program unhandled exception occoured");
			break;
		}
		case FaultAction::system_default:
		default:
			break;
		}
	}
	return EXCEPTION_CONTINUE_SEARCH;
}

void invite_to_debugger(const std::string& reason){
	std::string decorated = 
					reason 
					+ ",\n if you wanna debug it, attach to process with id: " 
					+ std::to_string(GetCurrentProcessId()) 
					+ ",\n then switch to thread id: " 
					+ std::to_string(GetCurrentThreadId()) 
					+ " and click OK";
	MessageBoxA(NULL, decorated.c_str(), "Debug invite", MB_ICONQUESTION);
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


NativeLib::NativeLib(const std::string& libray_path) {
	std::u16string res;
	utf8::utf8to16(libray_path.begin(), libray_path.end(), std::back_inserter(res));

	hGetProcIDDLL = LoadLibraryW((wchar_t*)res.c_str());
	if (!hGetProcIDDLL)
		throw LibrayNotFoundException();
}
CALL_FUNC NativeLib::get_func(const std::string& func_name) {
	auto tmp = GetProcAddress((HMODULE)hGetProcIDDLL, func_name.c_str());
	if (!tmp)
		throw LibrayFunctionNotFoundException();
	return (CALL_FUNC)tmp;
}
typed_lgr<FuncEnviropment> NativeLib::get_func_enviro(const std::string& func_name, const DynamicCall::FunctionTemplate& templ) {
	auto& env = envs[func_name];
	if (!env) {
		DynamicCall::PROC tmp = (DynamicCall::PROC)GetProcAddress((HMODULE)hGetProcIDDLL, func_name.c_str());
		if (!tmp)
			throw LibrayFunctionNotFoundException();
		return env = new FuncEnviropment(tmp, templ, true);
	}
	return env;
}
size_t NativeLib::get_pure_func(const std::string& func_name) {
	size_t tmp = (size_t)GetProcAddress((HMODULE)hGetProcIDDLL, func_name.c_str());
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

std::unordered_map<std::string, std::string> run_time_configuration;
#include "configuration/tasks.hpp"
void modify_run_time_config(const std::string& name, const std::string& value){
	if(name == "default_fault_action"){
#if _configuration_run_time_fault_action_modifable
		if(value == "make_dump" || value == "0")
			default_fault_action = FaultAction::make_dump;
		else if(value == "show_error" || value == "1")
			default_fault_action = FaultAction::show_error;
		else if(value == "dump_and_show_error" || value == "2")
			default_fault_action = FaultAction::dump_and_show_error;
		else if(value == "invite_to_debugger" || value == "3")
			default_fault_action = FaultAction::invite_to_debugger;
		else if(value == "system_default" || value == "4")
			default_fault_action = FaultAction::system_default;
		else if(value == "ignore" || value == "4")
			default_fault_action = FaultAction::ignore;
		else
			throw InvalidArguments("unrecognized value for default_fault_action");
#else
		throw AttachARuntimeException("default_fault_action is not modifable")
#endif

	}else if(name == "break_point_action"){
#if _configuration_run_time_break_point_action_modifable
		if(value == "invite_to_debugger" || value == "0")
			break_point_action = BreakPointAction::invite_to_debugger;
		if(value == "throw_exception" || value == "1")
			break_point_action = BreakPointAction::throw_exception;
		if(value == "ignore" || value == "2")
			break_point_action = BreakPointAction::ignore;
		else
			throw InvalidArguments("unrecognized value for break_point_action");
#else
		throw AttachARuntimeException("break_point_action is not modifable")
#endif
	}else if(name == "exception_on_language_routine_action"){
#if _configuration_run_time_exception_on_language_routine_action_modifable
		if(value == "invite_to_debugger" || value == "0")
			exception_on_language_routine_action = ExceptionOnLanguageRoutineAction::invite_to_debugger;
		if(value == "nest_exception" || value == "1")
			exception_on_language_routine_action = ExceptionOnLanguageRoutineAction::nest_exception;
		if(value == "swap_exception" || value == "2")
			exception_on_language_routine_action = ExceptionOnLanguageRoutineAction::swap_exception;
		if(value == "ignore" || value == "3")
			exception_on_language_routine_action = ExceptionOnLanguageRoutineAction::ignore;
		else
			throw InvalidArguments("unrecognized value for exception_on_language_routine_action");
#else
		throw AttachARuntimeException("exception_on_language_routine_action is not modifable")
#endif
	}else if(name == "enable_thread_naming"){
#if _configuration_tasks_enable_thread_naming_modifable
		if(value == "true" || value == "1")
			enable_thread_naming = true;
		else if(value == "false" || value == "0")
			enable_thread_naming = false;
		else
			throw InvalidArguments("unrecognized value for enable_thread_naming");
#else
		throw AttachARuntimeException("enable_thread_naming is not modifable");
#endif
	}else if(name == "allow_intern_access"){
#if _configuration_run_time_allow_intern_access_modifable
		if(value == "true" || value == "1")
			allow_intern_access = true;
		else if(value == "false" || value == "0")
			allow_intern_access = false;
		else
			throw InvalidArguments("unrecognized value for allow_intern_access");
#else
		throw AttachARuntimeException("allow_intern_access is not modifable");
#endif
	}else if(name == "max_running_tasks"){
#if _configuration_tasks_max_running_tasks_modifable
		if(value == "0")
			Task::max_running_tasks = 0;
		else{
			try{
				Task::max_running_tasks = std::stoull(value);
			}catch(...){
				throw InvalidArguments("unrecognized value for max_running_tasks");
			}
		}
#else
		throw AttachARuntimeException("max_running_tasks is not modifable");
#endif
	}else if(name == "max_planned_tasks"){
#if _configuration_tasks_max_planned_tasks_modifable
		if(value == "0")
			Task::max_planned_tasks = 0;
		else{
			try{
				Task::max_planned_tasks = std::stoull(value);
			}catch(...){
				throw InvalidArguments("unrecognized value for max_planned_tasks");
			}
		}
#else
		throw AttachARuntimeException("max_planned_tasks is not modifable");
#endif
	}else{
		if(value.empty())
			run_time_configuration.erase(name);
		else{
			run_time_configuration[name] = value;
		}
	}
}
std::string get_run_time_config(const std::string& name){
	if(name == "default_fault_action")
		return enum_to_string(default_fault_action);
	else if(name == "break_point_action")
		return enum_to_string(break_point_action);
	else if(name == "exception_on_language_routine_action")
		return enum_to_string(exception_on_language_routine_action);
	else if(name == "enable_thread_naming")
		return enable_thread_naming ? "true" : "false";
	else if(name == "allow_intern_access")
		return allow_intern_access ? "true" : "false";
	else if(name == "max_running_tasks")
		return std::to_string(Task::max_running_tasks);
	else if(name == "max_planned_tasks")
		return std::to_string(Task::max_planned_tasks);
	else{
		auto it = run_time_configuration.find(name);
		if(it == run_time_configuration.end())
			return "";
		return it->second;
	}
	
}