// Copyright Danyil Melnytskyi 2022-Present
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <run_time/attacha_abi_structs.hpp>
#include <util/cxxException.hpp>
#include <util/platform.hpp>
#ifdef PLATFORM_WINDOWS
#include <dbgeng.h>

    #include <base/run_time.hpp>
    #include <run_time/AttachA_CXX.hpp>
    #include <run_time/asm/CASM.hpp>
    #include <run_time/util/tools.hpp>

namespace art {
    struct CXXExInfo;

    namespace exception {
        using namespace internal;

        struct exception_data {
            CXXExInfo meta;
            std::exception_ptr ptr;
        };

        thread_local exception_data current_ex_info;

        EXCEPTION_DISPOSITION __attacha_handle(
            IN PEXCEPTION_RECORD ExceptionRecord,
            IN ULONG64 EstablisherFrame,
            IN OUT PCONTEXT ContextRecord,
            IN OUT PDISPATCHER_CONTEXT DispatcherContext
        ) {
            auto function_start = (uint8_t*)DispatcherContext->ImageBase;
            void* addr = (void*)DispatcherContext->ControlPc;
            uint8_t* data = (uint8_t*)DispatcherContext->HandlerData;
            bool execute = false;
            bool on_unwind = ExceptionRecord->ExceptionFlags & EXCEPTION_UNWINDING;
            current_ex_info.ptr = std::current_exception(); //internally use same pointer as ExceptionRecord in thread local storage when used EH runtime
            CXXExInfo info;

            if (!on_unwind) {
                getCxxExInfoFromNative1(info, ExceptionRecord);
                current_ex_info.meta = info;
            }

            try {
                while (true) {
                    ScopeAction::Action action = (ScopeAction::Action)*data++;
                    if (action == ScopeAction::Action::not_action)
                        return ExceptionContinueSearch;
                    size_t start_offset = readFromArrayAsValue<size_t>(data);
                    size_t end_offset = readFromArrayAsValue<size_t>(data);
                    execute = addr >= (void*)(function_start + start_offset) && addr < (void*)(function_start + end_offset);
                    switch (action) {
                    case ScopeAction::Action::destruct_stack: {
                        char* stack = (char*)ContextRecord->Rbp;
                        auto destruct = readFromArrayAsValue<void (*)(void*&)>(data);
                        stack += readFromArrayAsValue<uint64_t>(data);
                        if (execute && on_unwind) {
                            ExceptionRecord->ExceptionFlags |= EXCEPTION_NONCONTINUABLE;
                            destruct(*(void**)stack);
                        }
                        break;
                    }
                    case ScopeAction::Action::destruct_register: {
                        auto destruct = readFromArrayAsValue<void (*)(void*&)>(data);
                        if (!execute || !on_unwind) {
                            readFromArrayAsValue<uint32_t>(data);
                            continue;
                        }
                        ExceptionRecord->ExceptionFlags |= EXCEPTION_NONCONTINUABLE;
                        switch (readFromArrayAsValue<uint32_t>(data)) {
                        case asmjit::x86::Gp::kIdAx:
                            destruct(*(void**)ContextRecord->Rax);
                            break;
                        case asmjit::x86::Gp::kIdBx:
                            destruct(*(void**)ContextRecord->Rbx);
                            break;
                        case asmjit::x86::Gp::kIdCx:
                            destruct(*(void**)ContextRecord->Rcx);
                            break;
                        case asmjit::x86::Gp::kIdDx:
                            destruct(*(void**)ContextRecord->Rdx);
                            break;
                        case asmjit::x86::Gp::kIdDi:
                            destruct(*(void**)ContextRecord->Rdi);
                            break;
                        case asmjit::x86::Gp::kIdSi:
                            destruct(*(void**)ContextRecord->Rsi);
                            break;
                        case asmjit::x86::Gp::kIdR8:
                            destruct(*(void**)ContextRecord->R8);
                            break;
                        case asmjit::x86::Gp::kIdR9:
                            destruct(*(void**)ContextRecord->R9);
                            break;
                        case asmjit::x86::Gp::kIdR10:
                            destruct(*(void**)ContextRecord->R10);
                            break;
                        case asmjit::x86::Gp::kIdR11:
                            destruct(*(void**)ContextRecord->R11);
                            break;
                        case asmjit::x86::Gp::kIdR12:
                            destruct(*(void**)ContextRecord->R12);
                            break;
                        case asmjit::x86::Gp::kIdR13:
                            destruct(*(void**)ContextRecord->R13);
                            break;
                        case asmjit::x86::Gp::kIdR14:
                            destruct(*(void**)ContextRecord->R14);
                            break;
                        case asmjit::x86::Gp::kIdR15:
                            destruct(*(void**)ContextRecord->R15);
                            break;
                        default: {
                            ValueItem it{"Invalid register id"};
                            errors.async_notify(it);
                        }
                            return ExceptionContinueSearch;
                        }
                    }
                    case ScopeAction::Action::filter: {
                        auto filter = readFromArrayAsValue<bool (*)(CXXExInfo&, void*&, void*, size_t, void*, uint8_t*)>(data);
                        if (!execute) {
                            skipArray<char>(data);
                            continue;
                        }
                        size_t size = 0;
                        std::unique_ptr<char[]> stack;
                        stack.reset(readFromArrayAsArray<char>(data, size));
                        void* continue_from = nullptr;
                        if (!on_unwind) {
                            if (filter(info, continue_from, stack.get(), size, (void*)ContextRecord->Rsp, function_start)) {
                                ExceptionRecord->ExceptionFlags |= EXCEPTION_NONCONTINUABLE;
                                ExceptionRecord->ExceptionFlags |= EXCEPTION_UNWINDING;
                                info.make_cleanup();
                                current_ex_info = {};

                                RtlUnwindEx(
                                    (void*)EstablisherFrame,
                                    continue_from,
                                    ExceptionRecord,
                                    UlongToPtr(ExceptionRecord->ExceptionCode),
                                    DispatcherContext->ContextRecord,
                                    DispatcherContext->HistoryTable
                                );
                                __debugbreak();
                                ContextRecord->Rip = (uint64_t)continue_from;
                                return ExceptionCollidedUnwind;
                            } else
                                return ExceptionContinueSearch;
                        }
                        break;
                    }
                    case ScopeAction::Action::finally: {
                        auto finally_ = readFromArrayAsValue<void (*)(void*, size_t, void* rsp)>(data);
                        if (execute && on_unwind) {
                            size_t size = 0;
                            std::unique_ptr<char[]> stack;
                            stack.reset(readFromArrayAsArray<char>(data, size));
                            ExceptionRecord->ExceptionFlags |= EXCEPTION_NONCONTINUABLE;
                            finally_(stack.get(), size, (void*)ContextRecord->Rsp);
                        } else {
                            skipArray<char>(data);
                            continue;
                        }
                        break;
                    }
                    case ScopeAction::Action::not_action:
                        return ExceptionContinueSearch;
                    default:
                        throw BadOperationException();
                    }
                }
            } catch (...) {
                switch (exception_on_language_routine_action) {
                case ExceptionOnLanguageRoutineAction::invite_to_debugger: {
                    std::exception_ptr second_ex = std::current_exception();
                    invite_to_debugger("In this program caught exception on language routine handle");
                    break;
                }
                case ExceptionOnLanguageRoutineAction::nest_exception:
                    throw RoutineHandleExceptions(current_ex_info.ptr, std::current_exception());
                case ExceptionOnLanguageRoutineAction::swap_exception:
                    throw;
                case ExceptionOnLanguageRoutineAction::ignore:
                    break;
                }
            }
            return ExceptionContinueSearch;
        }

        void* __get_internal_handler() {
            return (void*)__attacha_handle;
        }

        ValueItem* get_current_exception_name() {
            if (current_ex_info.meta.ex_ptr == nullptr) {
                if (current_ex_info.meta.native_id == 0)
                    return nullptr;
                switch (current_ex_info.meta.native_id) {
                case EXCEPTION_ACCESS_VIOLATION:
                    if (current_ex_info.meta.ex_data_0 != 0 && current_ex_info.meta.ex_data_0 != 1)
                        return new ValueItem("execution_violation");
                    if (current_ex_info.meta.ex_data_1 < UINT16_MAX)
                        return new ValueItem("null_pointer_access");
                    return new ValueItem("access_violation");
                case EXCEPTION_STACK_OVERFLOW:
                    return new ValueItem("stack_overflow");
                case EXCEPTION_INT_DIVIDE_BY_ZERO:
                case EXCEPTION_FLT_DIVIDE_BY_ZERO:
                    return new ValueItem("divide_by_zero");
                case EXCEPTION_INT_OVERFLOW:
                case EXCEPTION_FLT_OVERFLOW:
                case EXCEPTION_FLT_STACK_CHECK:
                case EXCEPTION_FLT_UNDERFLOW:
                    return new ValueItem("numeric_overflow");
                case EXCEPTION_BREAKPOINT:
                    return new ValueItem("breakpoint");
                case EXCEPTION_ILLEGAL_INSTRUCTION:
                    return new ValueItem("illegal_instruction");
                case EXCEPTION_PRIV_INSTRUCTION:
                    return new ValueItem("privileged_instruction");
                default:
                    return new ValueItem("unknown_native_exception");
                }
            } else {
                if (current_ex_info.meta.ty_arr.contains_one([](const CXXExInfo::Tys& ty) { return ty.is_bad_alloc; }))
                    return new ValueItem("bad_alloc");
                else {
                    if (current_ex_info.meta.ty_arr.contains_one([](const CXXExInfo::Tys& ty) { return ty.ty_info->name() == typeid(AttachARuntimeException).name(); })) {
                        return new ValueItem(((AttachARuntimeException*)current_ex_info.meta.ex_ptr)->name());
                    } else
                        return new ValueItem(current_ex_info.meta.ty_arr[0].ty_info->name());
                }
            }
        }

        ValueItem* get_current_exception_description() {
            art::ustring description;
            if (current_ex_info.meta.ex_ptr == nullptr) {
                if (current_ex_info.meta.native_id == 0)
                    return nullptr;

                switch (current_ex_info.meta.native_id) {
                case EXCEPTION_ACCESS_VIOLATION:
                    description =
                        "Program attempted to " + art::ustring(current_ex_info.meta.ex_data_0 ? "write" : "read") + (current_ex_info.meta.ex_data_1 < UINT16_MAX ? " in null pointer region. 0x" : " in non mapped region. 0x") + string_help::hexstr(current_ex_info.meta.ex_data_1);
                    break;
                case EXCEPTION_STACK_OVERFLOW:
                    description = "Caught stack overflow exception";
                    break;
                case EXCEPTION_INT_DIVIDE_BY_ZERO:
                case EXCEPTION_FLT_DIVIDE_BY_ZERO:
                    description = "Number divided by zero";
                    break;
                case EXCEPTION_INT_OVERFLOW:
                case EXCEPTION_FLT_OVERFLOW:
                case EXCEPTION_FLT_STACK_CHECK:
                case EXCEPTION_FLT_UNDERFLOW:
                    description = "Trapped integer overflow";
                    break;
                case EXCEPTION_BREAKPOINT:
                    description = "Caught breakpoint exception, no debugger attached";
                    break;
                case EXCEPTION_ILLEGAL_INSTRUCTION:
                    description = "Huh, what are you doing?";
                    break;
                case EXCEPTION_PRIV_INSTRUCTION:
                    description = "You have no power here";
                    break;
                default:
                    description = "Caught unknown native exception: 0x" + string_help::hexstr(current_ex_info.meta.native_id);
                    break;
                }
            } else {
                if (current_ex_info.meta.ty_arr.contains_one([](const CXXExInfo::Tys& ty) { return ty.is_bad_alloc; }))
                    description = "Out of memory";
                else {
                    if (current_ex_info.meta.ty_arr.contains_one([](const CXXExInfo::Tys& ty) { return ty.ty_info->name() == typeid(AttachARuntimeException).name(); }))
                        description = ((AttachARuntimeException*)current_ex_info.meta.ex_ptr)->what();
                    else if (current_ex_info.meta.ty_arr.contains_one([](const CXXExInfo::Tys& ty) { return ty.ty_info->name() == typeid(std::exception).name(); })) {
                        description = art::ustring(current_ex_info.meta.ty_arr[0].ty_info->name()) + ": " + ((std::exception*)current_ex_info.meta.ex_ptr)->what();
                    } else
                        description = "Can not decode exception: " + art::ustring(current_ex_info.meta.ty_arr[0].ty_info->name());
                }
            }
            return new ValueItem(description);
        }

        ValueItem* get_current_exception_full_description() {
            if (current_ex_info.meta.ex_ptr != nullptr) {
                if (current_ex_info.meta.ty_arr.contains_one([](const CXXExInfo::Tys& ty) { return ty.ty_info->name() == typeid(AttachARuntimeException).name(); })) {
                    return new ValueItem(((AttachARuntimeException*)current_ex_info.meta.ex_ptr)->full_info());
                }
            }
            return get_current_exception_description();
        }

        ValueItem* has_current_exception_inner_exception() {
            if (current_ex_info.meta.ex_ptr != nullptr) {
                if (current_ex_info.meta.ty_arr.contains_one([](const CXXExInfo::Tys& ty) { return ty.ty_info->name() == typeid(AttachARuntimeException).name(); })) {
                    return new ValueItem((bool)((AttachARuntimeException*)current_ex_info.meta.ex_ptr)->get_inner_exception());
                }
            }
            if (current_ex_info.meta.native_id == 0)
                return nullptr;
            return new ValueItem(false);
        }

        void unpack_current_exception() {
            if (current_ex_info.meta.ex_ptr != nullptr) {
                if (current_ex_info.meta.ty_arr.contains_one([](const CXXExInfo::Tys& ty) { return ty.ty_info->name() == typeid(AttachARuntimeException).name(); })) {
                    ((AttachARuntimeException*)current_ex_info.meta.ex_ptr)->throw_inner_exception();
                }
            }
        }

        void current_exception_catched() {
            current_ex_info = {};
        }

        void* take_current_exception() {
            return new exception_data(std::move(current_ex_info));
        }

        CXXExInfo& lookup_meta() {
            return current_ex_info.meta;
        }

        bool try_catch_all(CXXExInfo& cxx) {
            //.NET CLR exception, we not support it
            return cxx.native_id != 0xE0434F4D && cxx.native_id != 0XE0434352;
        }

        void load_current_exception(void* cxx) {
            current_ex_info = std::move(*(exception_data*)cxx);
            delete (exception_data*)cxx;
        }

        bool has_exception() {
            return current_ex_info.meta.ex_ptr != nullptr || current_ex_info.meta.native_id != 0;
        }

        list_array<art::ustring> map_native_exception_names(CXXExInfo& info) {
            if (info.ex_ptr == nullptr) {
                switch (info.native_id) {
                case EXCEPTION_ACCESS_VIOLATION:
                    if (info.ex_data_0 != 0 && info.ex_data_0 != 1)
                        return {"execution_violation"};
                    if (info.ex_data_1 < UINT16_MAX)
                        return {"null_pointer_access", "access_violation"};
                    return {"access_violation"};
                case EXCEPTION_STACK_OVERFLOW:
                    return {"stack_overflow"};
                case EXCEPTION_INT_DIVIDE_BY_ZERO:
                case EXCEPTION_FLT_DIVIDE_BY_ZERO:
                    return {"divide_by_zero"};
                case EXCEPTION_INT_OVERFLOW:
                case EXCEPTION_FLT_OVERFLOW:
                case EXCEPTION_FLT_STACK_CHECK:
                case EXCEPTION_FLT_UNDERFLOW:
                    return {"numeric_overflow"};
                case EXCEPTION_BREAKPOINT:
                    return {"breakpoint"};
                case EXCEPTION_ILLEGAL_INSTRUCTION:
                    return {"illegal_instruction"};
                case EXCEPTION_PRIV_INSTRUCTION:
                    return {"privileged_instruction"};
                case 0xE0434F4D: //old CLR exception
                case 0XE0434352: //CLR exception v4 and above
                    return {};
                default:
                    return {"unknown_native_exception"};
                }
            } else {
                return info.ty_arr.convert<art::ustring>([&info](const CXXExInfo::Tys& ty) -> list_array<art::ustring> {
                    if (ty.is_bad_alloc)
                        return {"bad_alloc", "allocation_exception"};
                    if (ty.ty_info->name() == typeid(AttachARuntimeException).name())
                        return {((AttachARuntimeException*)info.ex_ptr)->name()};
                    else
                        return {ty.ty_info->name()};
                });
            }
        }

        bool _attacha_filter(CXXExInfo& info, void** continue_from, void* data, size_t size, void* enviro, uint8_t* image_base) {
            uint8_t* data_info = (uint8_t*)data;
            list_array<art::ustring> exceptions;
            *continue_from = image_base + readFromArrayAsValue<size_t>(data_info);
            switch (*data_info++) {
            case 0: {
                uint64_t handle_count = internal::readFromArrayAsValue<uint64_t>(data_info);
                exceptions.reserve_push_back(handle_count);
                for (size_t i = 0; i < handle_count; i++) {
                    size_t len = 0;
                    char* str = internal::readFromArrayAsArray<char>(data_info, len);
                    art::ustring string(str, len);
                    delete[] str;
                    exceptions.push_back(string);
                }
                break;
            }
            case 1: {
                uint16_t value = internal::readFromArrayAsValue<uint16_t>(data_info);
                ValueItem* item = (ValueItem*)enviro + (uint32_t(value) << 1);
                exceptions.push_back((art::ustring)*item);
                break;
            }
            case 2: {
                uint64_t handle_count = internal::readFromArrayAsValue<uint64_t>(data_info);
                exceptions.reserve_push_back(handle_count);
                for (size_t i = 0; i < handle_count; i++) {
                    uint16_t value = internal::readFromArrayAsValue<uint16_t>(data_info);
                    ValueItem* item = (ValueItem*)enviro + (uint32_t(value) << 1);
                    exceptions.push_back((art::ustring)*item);
                }
                break;
            }
            case 3: {
                uint64_t handle_count = internal::readFromArrayAsValue<uint64_t>(data_info);
                exceptions.reserve_push_back(handle_count);
                for (size_t i = 0; i < handle_count; i++) {
                    bool is_dynamic = internal::readFromArrayAsValue<bool>(data_info);
                    if (!is_dynamic) {
                        size_t len = 0;
                        char* str = internal::readFromArrayAsArray<char>(data_info, len);
                        art::ustring string(str, len);
                        delete[] str;
                        exceptions.push_back(string);
                    } else {
                        uint16_t value = internal::readFromArrayAsValue<uint16_t>(data_info);
                        ValueItem* item = (ValueItem*)enviro + (uint32_t(value) << 1);
                        exceptions.push_back((art::ustring)*item);
                    }
                }
                break;
            }
            case 4: //catch all
                //prevent catch CLR exception
                return exception::try_catch_all(info);
            case 5: { //attacha filter function
                Environment env_filter = internal::readFromArrayAsValue<Environment>(data_info);
                uint16_t filter_enviro_slice_begin = internal::readFromArrayAsValue<uint16_t>(data_info);
                uint16_t filter_enviro_slice_end = internal::readFromArrayAsValue<uint16_t>(data_info);
                if (filter_enviro_slice_begin > filter_enviro_slice_end)
                    throw InvalidIL("Invalid environment slice");
                uint16_t filter_enviro_size = filter_enviro_slice_end - filter_enviro_slice_begin;
                return (bool)CXX::aCall(env_filter, (ValueItem*)enviro + filter_enviro_slice_begin, filter_enviro_size);
            }
            default:
                throw BadOperationException();
            }
            return exception::map_native_exception_names(info).contains_one([&exceptions](const art::ustring& str) {
                return exceptions.contains(str);
            });
        }

        void _attacha_finally(void* data, size_t size, void* enviro) {
            uint8_t* data_info = (uint8_t*)data;
            Environment env_finalizer = internal::readFromArrayAsValue<Environment>(data_info);
            uint16_t finalizer_enviro_slice_begin = internal::readFromArrayAsValue<uint32_t>(data_info);
            uint16_t finalizer_enviro_slice_end = internal::readFromArrayAsValue<uint32_t>(data_info);
            if (finalizer_enviro_slice_begin >= finalizer_enviro_slice_end)
                throw InvalidIL("Invalid environment slice");
            uint16_t finalizer_enviro_size = finalizer_enviro_slice_end - finalizer_enviro_slice_begin;
            CXX::aCall(env_finalizer, (ValueItem*)enviro + finalizer_enviro_slice_begin, finalizer_enviro_size);
        }
    }
}
#else
#include <base/run_time.hpp>
#include <run_time/asm/CASM.hpp>
#include <run_time/util/tools.hpp>

namespace art {
    struct CXXExInfo;

    namespace exception {
        using namespace internal;
        thread_local CXXExInfo current_ex_info;

        void __attacha_handle() {
            throw 11;
        }

        void* __get_internal_handler() {
            return (void*)__attacha_handle;
        }

        ValueItem* get_current_exception_name() {
            if (current_ex_info.ex_ptr == nullptr) {
                if (current_ex_info.native_id == 0)
                    return nullptr;
                if (current_ex_info.ty_arr.contains_one([](const CXXExInfo::Tys& ty) { return ty.is_bad_alloc; }))
                    return new ValueItem("bad_alloc");
                else {
                    if (current_ex_info.ty_arr.contains_one([](const CXXExInfo::Tys& ty) { return ty.ty_info->name() == typeid(AttachARuntimeException).name(); }))
                        return new ValueItem(((AttachARuntimeException*)current_ex_info.ex_ptr)->name());
                    else
                        return new ValueItem(current_ex_info.ty_arr[0].ty_info->name());
                }
            } else {
                if (current_ex_info.ty_arr.contains_one([](const CXXExInfo::Tys& ty) { return ty.ty_info->name() == typeid(AttachARuntimeException).name(); }))
                    return new ValueItem(((AttachARuntimeException*)current_ex_info.ex_ptr)->name());
                else
                    return new ValueItem(current_ex_info.ty_arr[0].ty_info->name());
            }
        }

        ValueItem* get_current_exception_description() {
            art::ustring description;
            if (current_ex_info.ex_ptr == nullptr) {
                if (current_ex_info.native_id == 0)
                    return nullptr;
                else
                    description = "Native exception: " + std::to_string(current_ex_info.native_id);
            } else {
                if (current_ex_info.ty_arr.contains_one([](const CXXExInfo::Tys& ty) { return ty.is_bad_alloc; }))
                    description = "Out of memory";
                else {
                    if (current_ex_info.ty_arr.contains_one([](const CXXExInfo::Tys& ty) { return ty.ty_info->name() == typeid(AttachARuntimeException).name(); }))
                        description = ((AttachARuntimeException*)current_ex_info.ex_ptr)->what();
                    else if (current_ex_info.ty_arr.contains_one([](const CXXExInfo::Tys& ty) { return ty.ty_info->name() == typeid(std::exception).name(); })) {
                        description = art::ustring(current_ex_info.ty_arr[0].ty_info->name()) + ": " + ((std::exception*)current_ex_info.ex_ptr)->what();
                    } else
                        description = "Can not decode exception: " + art::ustring(current_ex_info.ty_arr[0].ty_info->name());
                }
            }
            return new ValueItem(description);
        }

        ValueItem* get_current_exception_full_description() {
            if (current_ex_info.ex_ptr != nullptr) {
                if (current_ex_info.ty_arr.contains_one([](const CXXExInfo::Tys& ty) { return ty.ty_info->name() == typeid(AttachARuntimeException).name(); })) {
                    AttachARuntimeException* ex = (AttachARuntimeException*)current_ex_info.ex_ptr;
                    return new ValueItem(ex->full_info());
                }
            }
            return get_current_exception_description();
        }

        ValueItem* has_current_exception_inner_exception() {
            if (current_ex_info.ex_ptr != nullptr) {
                if (current_ex_info.ty_arr.contains_one([](const CXXExInfo::Tys& ty) { return ty.ty_info->name() == typeid(AttachARuntimeException).name(); })) {
                    AttachARuntimeException* ex = (AttachARuntimeException*)current_ex_info.ex_ptr;
                    return new ValueItem((bool)ex->get_inner_exception());
                }
            }
            if (current_ex_info.native_id == 0)
                return nullptr;
            return new ValueItem(false);
        }

        void unpack_current_exception() {
            if (current_ex_info.ex_ptr != nullptr) {
                if (current_ex_info.ty_arr.contains_one([](const CXXExInfo::Tys& ty) { return ty.ty_info->name() == typeid(AttachARuntimeException).name(); })) {
                    AttachARuntimeException* ex = (AttachARuntimeException*)current_ex_info.ex_ptr;
                    ex->get_inner_exception();
                }
            }
        }

        void current_exception_catched() {
            current_ex_info = CXXExInfo();
        }

        CXXExInfo take_current_exception() {
            CXXExInfo ret = current_ex_info;
            current_ex_info = CXXExInfo();
            return ret;
        }

        CXXExInfo& peek_current_exception() {
            return current_ex_info;
        }

        bool try_catch_all(CXXExInfo& cxx) {
            return true;
        }

        void load_current_exception(CXXExInfo& cxx) {
            if (try_catch_all(cxx))
                current_ex_info = cxx;
            else
                throw InvalidArguments("Can not load CLR exception");
        }

        bool has_exception() {
            return current_ex_info.ex_ptr != nullptr || current_ex_info.native_id != 0;
        }

        list_array<art::ustring> map_native_exception_names(CXXExInfo& info) {
            list_array<art::ustring> ret;
            if (info.ex_ptr == nullptr) {
                switch (info.native_id) {
                default:
                    return {"unknown_native_exception"};
                }
            } else {
                return info.ty_arr.convert<art::ustring>([&info](const CXXExInfo::Tys& ty) -> list_array<art::ustring> {
                    if (ty.is_bad_alloc)
                        return {"bad_alloc", "allocation_exception"};
                    if (ty.ty_info->name() == typeid(AttachARuntimeException).name())
                        return {((AttachARuntimeException*)info.ex_ptr)->name()};
                    else
                        return {ty.ty_info->name()};
                });
            }
        }

        bool _attacha_filter(CXXExInfo& info, void** continue_from, void* data, size_t size, void* enviro, uint8_t* image_base) {
            return false;
        }

        void _attacha_finally(void* data, size_t size, void* enviro) {
        }
    }
}
#endif
