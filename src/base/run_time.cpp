// Copyright Danyil Melnytskyi 2022-Present
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <util/platform.hpp>
#if PLATFORM_WINDOWS
#pragma comment(lib, "Dbghelp.lib")
#include <Windows.h>

#include <Dbghelp.h>
#include <Psapi.h>

#include <utf8cpp/utf8.h>

#include <configuration/run_time.hpp>
#include <configuration/tasks.hpp>

#include <base/run_time.hpp>
#include <run_time/asm/FuncEnvironment.hpp>
#include <run_time/asm/dynamic_call.hpp>
#include <run_time/tasks.hpp>
#include <run_time/tasks/util/light_stack.hpp>
#include <util/hash.hpp>
#include <util/string_help.hpp>

namespace art {
    size_t page_size = []() {
        SYSTEM_INFO si;
        GetSystemInfo(&si);
        return si.dwPageSize;
    }();
    unsigned long fault_reserved_stack_size = 0; // 524288;
    unsigned long fault_reserved_pages = fault_reserved_stack_size / page_size + (fault_reserved_stack_size % page_size ? 1 : 0);
    thread_local unsigned long stack_size_tmp = 0;
    thread_local bool need_stack_restore = false;
    bool enable_thread_naming = configuration::run_time::enable_thread_naming;
    bool allow_intern_access = configuration::run_time::allow_intern_access;

    FaultAction default_fault_action = (FaultAction)configuration::run_time::default_fault_action;
    BreakPointAction break_point_action = (BreakPointAction)configuration::run_time::break_point_action;
    ExceptionOnLanguageRoutineAction exception_on_language_routine_action = (ExceptionOnLanguageRoutineAction)configuration::run_time::exception_on_language_routine_action;

    bool _set_name_thread_dbg(const art::ustring& name, unsigned long thread_id) {
        if (!enable_thread_naming)
            return false;
        std::u16string wname = (std::u16string)name;
        HANDLE thread = OpenThread(THREAD_SET_LIMITED_INFORMATION, false, thread_id);
        if (!thread)
            return false;
        bool result = SUCCEEDED(SetThreadDescription(thread, (wchar_t*)wname.c_str()));
        CloseHandle(thread);
        return result;
    }

    bool _set_name_thread_dbg(const art::ustring& name) {
        if (!enable_thread_naming)
            return false;
        std::u16string wname = (std::u16string)name;
        return SUCCEEDED(SetThreadDescription(GetCurrentThread(), (wchar_t*)wname.c_str()));
    }

    art::ustring _get_name_thread_dbg(unsigned long thread_id) {
        HANDLE thread = OpenThread(THREAD_QUERY_LIMITED_INFORMATION, false, thread_id);
        if (!thread)
            return "";
        WCHAR* res;
        if (SUCCEEDED(GetThreadDescription(thread, &res))) {
            art::ustring result((const char16_t*)res);
            LocalFree(res);
            CloseHandle(thread);
            return result;
        } else
            return "";
    }

    unsigned long _thread_id() {
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
                e->ExceptionRecord->ExceptionInformation[0] == 0    // thread attempted to read the inaccessible data
                || e->ExceptionRecord->ExceptionInformation[0] == 1 // thread attempted to write to an inaccessible address
            ) {
                if (Task::is_task()) {
                    if (e->ExceptionRecord->ExceptionInformation[1] < UINT16_MAX)
                        throw NullPointerException("Task 0x" + string_help::hexsstr(Task::task_id()) + " attempted to " + art::ustring(e->ExceptionRecord->ExceptionInformation[0] ? "write" : "read") + " in null pointer region. 0x" + string_help::hexstr(e->ExceptionRecord->ExceptionInformation[1]));
                    else
                        throw SegmentationFaultException("Task 0x" + string_help::hexsstr(Task::task_id()) + " attempted to " + art::ustring(e->ExceptionRecord->ExceptionInformation[0] ? "write" : "read") + " in non mapped region. 0x" + string_help::hexstr(e->ExceptionRecord->ExceptionInformation[1]));

                } else {
                    if (e->ExceptionRecord->ExceptionInformation[1] < UINT16_MAX)
                        throw NullPointerException("Thread " + string_help::hexsstr((size_t)art::this_thread::get_id()) + " attempted to " + art::ustring(e->ExceptionRecord->ExceptionInformation[0] ? "write" : "read") + " in null pointer region. 0x" + string_help::hexstr(e->ExceptionRecord->ExceptionInformation[1]));
                    else
                        throw SegmentationFaultException("Thread " + string_help::hexsstr((size_t)art::this_thread::get_id()) + " attempted to " + art::ustring(e->ExceptionRecord->ExceptionInformation[0] ? "write" : "read") + " in non mapped region. 0x" + string_help::hexstr(e->ExceptionRecord->ExceptionInformation[1]));
                }
            } else
                return EXCEPTION_CONTINUE_SEARCH; // 8 - thread attempted to execute to an inaccessible address
        case EXCEPTION_INT_DIVIDE_BY_ZERO:
        case EXCEPTION_FLT_DIVIDE_BY_ZERO:
            throw DivideByZeroException();
        case EXCEPTION_STACK_OVERFLOW:
            need_stack_restore = true;
            throw StackOverflowException();
        case EXCEPTION_INT_OVERFLOW:
        case EXCEPTION_FLT_OVERFLOW:
        case EXCEPTION_FLT_STACK_CHECK:
        case EXCEPTION_FLT_UNDERFLOW:
            throw NumericOverflowException();
        case EXCEPTION_BREAKPOINT:
            switch (break_point_action) {
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
        art::ustring ex_str;
        if (hasClassInEx(cxx, "AttachARuntimeException")) {
            ex_str = "Caught to unhandled AttachA ";
            ex_str += cxx.ty_arr[0].ty_info->name();
            ex_str += " exception.\n";
            ex_str += ((AttachARuntimeException*)cxx.ex_ptr)->what();
            MessageBoxA(NULL, ex_str.c_str(), "Unhandled AttachA exception", MB_ICONERROR);
        } else {
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
        art::ustring ss;
        ss.reserve(256);
        ss.append("Caught to unhandled seh exception.\n", 36);
        ss.append("Ex code: ", 9);
        ss.append(std::to_string(e->ExceptionRecord->ExceptionCode));
        ss.append("\nEx addr: ", 10);
        ss.append(string_help::hexstr(e->ExceptionRecord->ExceptionAddress));
        DWORD maxi = e->ExceptionRecord->NumberParameters;
        ss.append("\nParams: ", 9);
        ss.append(std::to_string(maxi));
        for (size_t i = 0; i < maxi; i++) {
            ss.append("\n\tp", 3);
            ss.append(std::to_string(i));
            ss.append(": ", 2);
            ss.append(string_help::hexstr(e->ExceptionRecord->ExceptionInformation[i]));
        }
        MessageBoxA(NULL, ss.c_str(), "Unhandled seh exception", MB_ICONERROR);
        std::exit(-1);
    }

    void make_dump(LPEXCEPTION_POINTERS e, CXXExInfo* cxx) {
        std::wstring path;
        {
            constexpr uint16_t limit = 65535;
            wchar_t* tmp;
            try {
                tmp = new wchar_t[limit];
            } catch (...) {
                return;
            }
            DWORD len = GetEnvironmentVariableW(L"AttachA_dump_path", tmp, limit);
            if (!len) {
                DWORD len = GetCurrentDirectoryW(0, NULL);
                if (len > limit) {
                    wchar_t* tmp = new wchar_t[len];
                    GetCurrentDirectoryW(len, tmp);
                    path = std::wstring(tmp, tmp + len);
                    delete[] tmp;
                } else {
                    GetCurrentDirectoryW(len, tmp);
                    path = std::wstring(tmp, tmp + len);
                }
            } else {
                path = std::wstring(tmp, tmp + len);
            }

            len = GetModuleFileNameW(NULL, tmp, limit);
            if (!len) {
                path += L"\\Unresolved_AttachA";
            } else {
                std::wstring tmp2(tmp, tmp + len);
                path += tmp2.substr(tmp2.find_last_of(L"\\") + 1);
            }
            delete[] tmp;
        }
        path += L" exception fault ";
        if (cxx) {
            const char* type_name = cxx->ty_arr[0].ty_info->name();
            size_t len = strlen(type_name);
            wchar_t* tmp = new wchar_t[len];
            size_t wlen = MultiByteToWideChar(CP_UTF8, 0, type_name, len, tmp, len);
            if (!wlen)
                path += L"FAILED_TO_CONVERT_EXCEPTION_NAME";
            else
                path.append(tmp, tmp + wlen);
            delete[] tmp;
        }
        path += L" ";
        {
            LARGE_INTEGER ticks;
            if (!QueryPerformanceCounter(&ticks))
                ticks.QuadPart = -1;
            path += std::to_wstring(art::hash<long long>()(ticks.QuadPart));
        }
        path += L".dmp";
        HANDLE hndl = CreateFileW(path.c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_ARCHIVE, 0);
        if (hndl == INVALID_HANDLE_VALUE)
            return;
        auto flags = MiniDumpWithFullMemory | MiniDumpWithFullMemoryInfo | MiniDumpWithHandleData | MiniDumpWithUnloadedModules | MiniDumpWithThreadInfo;

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
                ValueItem val(&cxx, ValueMeta(VType::undefined_ptr, false, false));
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
                invite_to_debugger("In this program unhandled exception occurred");
                break;
            }
            case FaultAction::system_default:
            default:
                break;
            }
        } else {
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
                invite_to_debugger("In this program unhandled exception occurred");
                break;
            }
            case FaultAction::system_default:
            default:
                break;
            }
        }
        return EXCEPTION_CONTINUE_SEARCH;
    }

    void invite_to_debugger(const art::ustring& reason) {
        art::ustring decorated = reason + ",\n if you wanna debug it, attach to process with id: " + std::to_string(GetCurrentProcessId()) + ",\n then switch to thread id: " + std::to_string(GetCurrentThreadId()) + " and click OK";
        MessageBoxA(NULL, decorated.c_str(), "Debug invite", MB_ICONQUESTION);
        if (IsDebuggerPresent())
            DebugBreak();
    }

    void ini_current() {
        bool cur_siz = SetThreadStackGuarantee(&stack_size_tmp);
        stack_size_tmp += fault_reserved_stack_size;
        bool setted = SetThreadStackGuarantee(&stack_size_tmp);
        auto err = GetLastError();

        AddVectoredExceptionHandler(0, win_exception_handler);
        SetUnhandledExceptionFilter(win_fault_handler);
    }

    thread_local bool ex_proxy_enabled = []() {
        ini_current();
        return true;
    }();

    NativeLib::NativeLib(const art::ustring& library_path) {
        std::u16string res;
        utf8::utf8to16(library_path.begin(), library_path.end(), std::back_inserter(res));

        hGetProcIDDLL = LoadLibraryW((wchar_t*)res.c_str());
        if (!hGetProcIDDLL)
            throw LibraryNotFoundException();
    }

    CALL_FUNC NativeLib::get_func(const art::ustring& func_name) {
        auto tmp = GetProcAddress((HMODULE)hGetProcIDDLL, func_name.c_str());
        if (!tmp)
            throw LibraryFunctionNotFoundException();
        return (CALL_FUNC)tmp;
    }

    art::shared_ptr<class FuncEnvironment> NativeLib::get_func_enviro(const art::ustring& func_name, const DynamicCall::FunctionTemplate& templ) {
        auto& env = envs[func_name];
        if (!env) {
            DynamicCall::PROC tmp = (DynamicCall::PROC)GetProcAddress((HMODULE)hGetProcIDDLL, func_name.c_str());
            if (!tmp)
                throw LibraryFunctionNotFoundException();
            return env = new FuncEnvironment(tmp, templ, true, false);
        }
        return env;
    }

    art::shared_ptr<class FuncEnvironment> NativeLib::get_own_enviro(const art::ustring& func_name) {
        auto& env = envs[func_name];
        if (!env) {
            Environment tmp = (Environment)(DynamicCall::PROC)GetProcAddress((HMODULE)hGetProcIDDLL, func_name.c_str());
            if (!tmp)
                throw LibraryFunctionNotFoundException();
            return env = new FuncEnvironment(tmp, true, false);
        }
        return env;
    }

    size_t NativeLib::get_pure_func(const art::ustring& func_name) {
        size_t tmp = (size_t)GetProcAddress((HMODULE)hGetProcIDDLL, func_name.c_str());
        if (!tmp)
            throw LibraryFunctionNotFoundException();
        return tmp;
    }

    NativeLib::~NativeLib() {
        for (auto& [_, it] : envs)
            if (it)
                it->forceUnload();
        envs = {};
        if (hGetProcIDDLL) {
            FreeLibrary((HMODULE)hGetProcIDDLL);
            hGetProcIDDLL = nullptr;
        }
    }

    bool restore_stack_fault() {
        if (!need_stack_restore)
            return false;

        bool is_successes = _resetstkoflw();
        if (!is_successes)
            std::exit(EXCEPTION_STACK_OVERFLOW);
        need_stack_restore = false;
        return true;
    }

    bool need_restore_stack_fault() {
        return need_stack_restore;
    }
#elif PLATFORM_LINUX

#include <configuration/run_time.hpp>
#include <configuration/tasks.hpp>
#include <utf8cpp/utf8.h>

#include <base/run_time.hpp>
#include <dlfcn.h>
#include <run_time/asm/FuncEnvironment.hpp>
#include <run_time/asm/dynamic_call.hpp>
#include <run_time/tasks.hpp>
#include <run_time/tasks/util/light_stack.hpp>
#include <unistd.h>
#include <util/string_help.hpp>

namespace art {
    thread_local bool ex_proxy_enabled;

    EventSystem unhandled_exception;
    EventSystem ex_fault;
    EventSystem errors;
    EventSystem warning;
    EventSystem info;

    size_t page_size = sysconf(_SC_PAGESIZE);
    unsigned long fault_reserved_stack_size = 0;
    unsigned long fault_reserved_pages = fault_reserved_stack_size / page_size + (fault_reserved_stack_size % page_size ? 1 : 0);
    thread_local unsigned long stack_size_tmp = 0;
    thread_local bool need_stack_restore = false;
    bool enable_thread_naming = configuration::run_time::enable_thread_naming;
    bool allow_intern_access = configuration::run_time::allow_intern_access;

    FaultAction default_fault_action = (FaultAction)configuration::run_time::default_fault_action;
    BreakPointAction break_point_action = (BreakPointAction)configuration::run_time::break_point_action;
    ExceptionOnLanguageRoutineAction exception_on_language_routine_action = (ExceptionOnLanguageRoutineAction)configuration::run_time::exception_on_language_routine_action;

    void invite_to_debugger(const art::ustring& reason) {
        // TODO
        std::terminate();
    }

    bool _set_name_thread_dbg(const art::ustring& name) {
        if (name.size() > 15)
            return false;
        return pthread_setname_np(pthread_self(), name.c_str()) == 0;
    }

    bool _set_name_thread_dbg(const art::ustring& name, unsigned long id) {
        if (name.size() > 15)
            return false;
        return pthread_setname_np(id, name.c_str()) == 0;
    }

    art::ustring _get_name_thread_dbg(unsigned long thread_id) {
        char name[16];
        if (pthread_getname_np(pthread_t(thread_id), name, 16) != 0)
            return "";
        return name;
    }

    unsigned long _thread_id() {
        return pthread_self();
    }

    bool restore_stack_fault() {
        return false;
    }

    bool need_restore_stack_fault() {
        return false;
    }

    void ini_current() {
        // TODO
    }

    NativeLib::NativeLib(const art::ustring& library_path) {
        art::ustring res;
        hGetProcIDDLL = dlopen(res.c_str(), RTLD_LAZY);
        if (!hGetProcIDDLL)
            throw LibraryNotFoundException();
    }

    CALL_FUNC NativeLib::get_func(const art::ustring& func_name) {
        auto tmp = dlsym(hGetProcIDDLL, func_name.c_str());
        if (!tmp)
            throw LibraryFunctionNotFoundException();
        return (CALL_FUNC)tmp;
    }

    art::shared_ptr<FuncEnvironment> NativeLib::get_func_enviro(const art::ustring& func_name, const DynamicCall::FunctionTemplate& templ) {
        auto& env = envs[func_name];
        if (!env) {
            void* tmp = (void*)(DynamicCall::PROC)dlsym(hGetProcIDDLL, func_name.c_str());
            if (!tmp)
                throw LibraryFunctionNotFoundException();
            return env = new FuncEnvironment(tmp, templ, true, false);
        }
        return env;
    }

    art::shared_ptr<FuncEnvironment> NativeLib::get_own_enviro(const art::ustring& func_name) {
        auto& env = envs[func_name];
        if (!env) {
            Environment tmp = (Environment)dlsym(hGetProcIDDLL, func_name.c_str());
            if (!tmp)
                throw LibraryFunctionNotFoundException();
            return env = new FuncEnvironment(tmp, true, false);
        }
        return env;
    }

    size_t NativeLib::get_pure_func(const art::ustring& func_name) {
        auto tmp = dlsym(hGetProcIDDLL, func_name.c_str());
        if (!tmp)
            throw LibraryFunctionNotFoundException();
        return (size_t)tmp;
    }

    NativeLib::~NativeLib() {
        for (auto& [_, it] : envs)
            if (it)
                it->forceUnload();
        envs = {};
        if (hGetProcIDDLL) {
            dlclose(hGetProcIDDLL);
            hGetProcIDDLL = nullptr;
        }
    }
#endif

    std::unordered_map<art::ustring, art::ustring, art::hash<art::ustring>> run_time_configuration;

    void modify_run_time_config(const art::ustring& name, const art::ustring& value) {
        if (name == "default_fault_action") {
#if _configuration_run_time_fault_action_modifiable
            if (value == "make_dump" || value == "0")
                default_fault_action = FaultAction::make_dump;
            else if (value == "show_error" || value == "1")
                default_fault_action = FaultAction::show_error;
            else if (value == "dump_and_show_error" || value == "2")
                default_fault_action = FaultAction::dump_and_show_error;
            else if (value == "invite_to_debugger" || value == "3")
                default_fault_action = FaultAction::invite_to_debugger;
            else if (value == "system_default" || value == "4")
                default_fault_action = FaultAction::system_default;
            else if (value == "ignore" || value == "4")
                default_fault_action = FaultAction::ignore;
            else
                throw InvalidArguments("unrecognized value for default_fault_action");
#else
            throw AttachARuntimeException("default_fault_action is not modifiable")
#endif

        } else if (name == "break_point_action") {
#if _configuration_run_time_break_point_action_modifiable
            if (value == "invite_to_debugger" || value == "0")
                break_point_action = BreakPointAction::invite_to_debugger;
            if (value == "throw_exception" || value == "1")
                break_point_action = BreakPointAction::throw_exception;
            if (value == "ignore" || value == "2")
                break_point_action = BreakPointAction::ignore;
            else
                throw InvalidArguments("unrecognized value for break_point_action");
#else
            throw AttachARuntimeException("break_point_action is not modifiable")
#endif
        } else if (name == "exception_on_language_routine_action") {
#if _configuration_run_time_exception_on_language_routine_action_modifiable
            if (value == "invite_to_debugger" || value == "0")
                exception_on_language_routine_action = ExceptionOnLanguageRoutineAction::invite_to_debugger;
            if (value == "nest_exception" || value == "1")
                exception_on_language_routine_action = ExceptionOnLanguageRoutineAction::nest_exception;
            if (value == "swap_exception" || value == "2")
                exception_on_language_routine_action = ExceptionOnLanguageRoutineAction::swap_exception;
            if (value == "ignore" || value == "3")
                exception_on_language_routine_action = ExceptionOnLanguageRoutineAction::ignore;
            else
                throw InvalidArguments("unrecognized value for exception_on_language_routine_action");
#else
            throw AttachARuntimeException("exception_on_language_routine_action is not modifiable")
#endif
        } else if (name == "enable_thread_naming") {
#if _configuration_tasks_enable_thread_naming_modifiable
            if (value == "true" || value == "1")
                enable_thread_naming = true;
            else if (value == "false" || value == "0")
                enable_thread_naming = false;
            else
                throw InvalidArguments("unrecognized value for enable_thread_naming");
#else
            throw AttachARuntimeException("enable_thread_naming is not modifiable");
#endif
        } else if (name == "enable_task_naming") {
#if _configuration_tasks_enable_task_naming_modifiable
            if (value == "true" || value == "1")
                Task::enable_task_naming = true;
            else if (value == "false" || value == "0")
                Task::enable_task_naming = false;
            else
                throw InvalidArguments("unrecognized value for enable_task_naming");
#else
            throw AttachARuntimeException("enable_task_naming is not modifiable");
#endif
        } else if (name == "allow_intern_access") {
#if _configuration_run_time_allow_intern_access_modifiable
            if (value == "true" || value == "1")
                allow_intern_access = true;
            else if (value == "false" || value == "0")
                allow_intern_access = false;
            else
                throw InvalidArguments("unrecognized value for allow_intern_access");
#else
            throw AttachARuntimeException("allow_intern_access is not modifiable");
#endif
        } else if (name == "max_running_tasks") {
#if _configuration_tasks_max_running_tasks_modifiable
            if (value == "0")
                Task::max_running_tasks = 0;
            else {
                try {
                    Task::max_running_tasks = std::stoull(value);
                } catch (...) {
                    throw InvalidArguments("unrecognized value for max_running_tasks");
                }
            }
#else
            throw AttachARuntimeException("max_running_tasks is not modifiable");
#endif
        } else if (name == "max_planned_tasks") {
#if _configuration_tasks_max_planned_tasks_modifiable
            if (value == "0")
                Task::max_planned_tasks = 0;
            else {
                try {
                    Task::max_planned_tasks = std::stoull(value);
                } catch (...) {
                    throw InvalidArguments("unrecognized value for max_planned_tasks");
                }
            }
#else
            throw AttachARuntimeException("max_planned_tasks is not modifiable");
#endif
        } else if (name == "light_stack_max_buffer_size") {
#if _configuration_tasks_light_stack_max_buffer_size_modifiable
            if (value == "0")
                light_stack::max_buffer_size = 0;
            else {
                try {
                    light_stack::max_buffer_size = std::stoull(value);
                } catch (...) {
                    throw InvalidArguments("unrecognized value for light_stack_max_buffer_size");
                }
            }
#else
            throw AttachARuntimeException("light_stack_max_buffer_size is not modifiable");
#endif
        } else if (name == "light_stack_flush_used_stacks") {
#if _configuration_tasks_light_stack_flush_used_stacks_modifiable
            if (value == "true" || value == "1")
                light_stack::flush_used_stacks = true;
            else if (value == "false" || value == "0")
                light_stack::flush_used_stacks = false;
            else
                throw InvalidArguments("unrecognized value for light_stack_flush_used_stacks");
#else
            throw AttachARuntimeException("light_stack_flush_used_stacks is not modifiable");
#endif
        } else {
            if (value.empty())
                run_time_configuration.erase(name);
            else {
                run_time_configuration[name] = value;
            }
        }
    }

    art::ustring get_run_time_config(const art::ustring& name) {
        if (name == "default_fault_action")
            return enum_to_string(default_fault_action);
        else if (name == "break_point_action")
            return enum_to_string(break_point_action);
        else if (name == "exception_on_language_routine_action")
            return enum_to_string(exception_on_language_routine_action);
        else if (name == "enable_thread_naming")
            return enable_thread_naming ? "true" : "false";
        else if (name == "enable_task_naming")
            return Task::enable_task_naming ? "true" : "false";
        else if (name == "allow_intern_access")
            return allow_intern_access ? "true" : "false";
        else if (name == "max_running_tasks")
            return std::to_string(Task::max_running_tasks);
        else if (name == "max_planned_tasks")
            return std::to_string(Task::max_planned_tasks);
        else if (name == "light_stack_max_buffer_size")
            return std::to_string(light_stack::max_buffer_size);
        else if (name == "light_stack_flush_used_stacks")
            return light_stack::flush_used_stacks ? "true" : "false";
        else {
            auto it = run_time_configuration.find(name);
            if (it == run_time_configuration.end())
                return "";
            return it->second;
        }
    }
}