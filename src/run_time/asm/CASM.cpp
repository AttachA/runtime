// Copyright Danyil Melnytskyi 2022-Present
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <run_time/asm/CASM.hpp>
#include <util/platform.hpp>
#ifdef PLATFORM_WINDOWS
#include <Windows.h>

#include <DbgHelp.h>
#include <dbgeng.h>

namespace art {
    void _______dbgOut(const char* str) {
        OutputDebugStringA(str);
    }
}
#elif PLATFORM_LINUX || PLATFORM_MACOS
#include <cxxabi.h>
#include <execinfo.h>
#include <libunwind.h>
#include <run_time/asm/CASM/linux/dwarf_builder.hpp>


extern "C" void __register_frame(void* fde);
extern "C" void __deregister_frame(void* fde);

#endif
#include <cassert>
#include <string>
#include <unordered_map>
#include <util/threading.hpp>

namespace art {
#define CONV_ASMJIT(to_conv)                                                                        \
    {                                                                                               \
        if (auto tmp = (to_conv))                                                                   \
            throw CompileTimeException("Fail create func asmjit err code: " + std::to_string(tmp)); \
    }

    //return offset from allocated to additional size or code size
    size_t CASM::allocate_and_prepare_code(size_t additional_size_begin, uint8_t*& res, CodeHolder* code, asmjit::JitAllocator* alloc, size_t additional_size_end) {
        res = 0;
        CONV_ASMJIT(code->flatten());
        if (code->hasUnresolvedLinks()) {
            CONV_ASMJIT(code->resolveUnresolvedLinks());
        }

        size_t estimatedCodeSize = code->codeSize();
        if (ASMJIT_UNLIKELY(estimatedCodeSize == 0))
            throw CompileTimeException("Code is empty");

        uint8_t* rx;
        uint8_t* rw;
        CONV_ASMJIT(alloc->alloc((void**)&rx, (void**)&rw, additional_size_begin + estimatedCodeSize + additional_size_end));
        rw += additional_size_begin;
        // Relocate the code.
        Error err = code->relocateToBase(uintptr_t((void*)rx) + additional_size_begin);
        if (ASMJIT_UNLIKELY(err)) {
            alloc->release(rx);
            return err;
        }

        // Recalculate the final code size and shrink the memory we allocated for it
        // in case that some relocations didn't require records in an address table.
        size_t codeSize = code->codeSize();


        for (asmjit::Section* section : code->_sections) {
            size_t offset = size_t(section->offset());
            size_t bufferSize = size_t(section->bufferSize());
            size_t virtualSize = size_t(section->virtualSize());

            assert(offset + bufferSize <= codeSize);
            memcpy(rw + offset, section->data(), bufferSize);

            if (virtualSize > bufferSize) {
                assert(offset + virtualSize <= codeSize);
                memset(rw + offset + bufferSize, 0, virtualSize - bufferSize);
            }
        }
        if (codeSize < estimatedCodeSize)
            alloc->shrink(rx, additional_size_begin + codeSize + additional_size_end);

#if BIT_64
#if COMPILER_GCC
        uint8_t* start = rx;
        uint8_t* end = start + codeSize + additional_size_begin + additional_size_end;
        __builtin___clear_cache(start, end);
#endif
#elif BIT_32
#if COMPILER_MSVC
        // Windows has a built-in support in `kernel32.dll`.
        ::FlushInstructionCache(::GetCurrentProcess(), rx, codeSize + additional_size);
#elif COMPILER_GCC
        char* start = static_cast<char*>(const_cast<void*>(rx));
        char* end = start + codeSize + additional_size;
        __builtin___clear_cache(start, end);
#else
        asmjit::DebugUtils::unused(p, size);
#endif
#endif
        res = rx;
        return codeSize;
    }

    void CASM::release_code(uint8_t* code, asmjit::JitAllocator* alloc) {
        alloc->release(code);
    }

    struct frame_info {
        art::ustring name;
        art::ustring file;
        size_t fun_size = 0;
    };

    struct FrameSymbols {
        std::unordered_map<uint8_t*, frame_info, art::hash<uint8_t*>> map;
        bool destroyed = false;

        FrameSymbols() {}

        ~FrameSymbols() {
            map.clear();
            destroyed = true;
        }
    } frame_symbols;
#ifdef _WIN64
    enum UWC {
        UWOP_PUSH_NONVOL = 0,
        UWOP_ALLOC_LARGE = 1,
        UWOP_ALLOC_SMALL = 2,
        UWOP_SET_FPREG = 3,
        UWOP_SAVE_NONVOL = 4,
        UWOP_SAVE_NONVOL_FAR = 5,
        UWOP_SAVE_XMM128 = 8,
        UWOP_SAVE_XMM128_FAR = 9,
        UWOP_PUSH_MACHFRAME = 10,
    };

    union UWCODE {
        struct {
            uint8_t offset;
            uint8_t op : 4;
            uint8_t info : 4;
        };

        uint16_t solid = 0;
        UWCODE() = default;
        UWCODE(const UWCODE& copy) = default;

        UWCODE(uint8_t off, uint8_t oper, uint8_t inf) {
            offset = off;
            op = oper;
            info = inf;
        }
    };

    void BuildProlog::pushReg(creg reg) {
        csm.push(reg.fromTypeAndId(asmjit::RegType::kGp64, reg.id()));
        if ((uint8_t)csm.offset() != csm.offset())
            throw CompileTimeException("prolog too large");
        res.prolog.push_back(UWCODE((uint8_t)csm.offset(), UWC::UWOP_PUSH_NONVOL, reg.id()).solid);
        pushes.push_back({cur_op++, reg});
        stack_align += 8;
    }

    void BuildProlog::stackAlloc(uint32_t size) {
        if (size == 0)
            return;
        //align stack
        size = (size / 8 + ((size % 8) ? 1 : 0)) * 8;
        csm.sub(stack_ptr, size);
        if ((uint8_t)csm.offset() != csm.offset())
            throw CompileTimeException("prolog too large");
        if (size <= 128)
            res.prolog.push_back(UWCODE((uint8_t)csm.offset(), UWC::UWOP_ALLOC_SMALL, size / 8 - 1).solid);
        else if (size <= 524280) {
            //512K - 8
            res.prolog.push_back(size / 8);
            res.prolog.push_back(UWCODE((uint8_t)csm.offset(), UWC::UWOP_ALLOC_LARGE, 0).solid);
        } else if (size <= 4294967288) {
            //4gb - 8
            res.prolog.push_back((uint16_t)(size >> 16));
            res.prolog.push_back((uint16_t)size);
            res.prolog.push_back(UWCODE((uint8_t)csm.offset(), UWC::UWOP_ALLOC_LARGE, 1).solid);
        } else
            throw CompileTimeException("Invalid unwind code, too large stack allocation");
        stack_alloc.push_back({cur_op++, size});
        stack_align += size;
    }

    void BuildProlog::setFrame(uint16_t stack_offset) {
        if (frame_inited)
            throw CompileTimeException("Frame already inited");
        if (stack_offset % 16)
            throw CompileTimeException("Invalid frame offset, it must be aligned by 16");
        if (uint8_t(stack_offset / 16) != stack_offset / 16)
            throw CompileTimeException("Frame offset too large");
        csm.lea(frame_ptr, stack_ptr, stack_offset);
        if ((uint8_t)csm.offset() != csm.offset())
            throw CompileTimeException("prolog too large");
        res.prolog.push_back(UWCODE((uint8_t)csm.offset(), UWC::UWOP_SET_FPREG, 0).solid);
        set_frame.push_back({cur_op++, stack_offset});
        res.head.FrameOffset = stack_offset / 16;
        frame_inited = true;
    }

    void BuildProlog::saveToStack(creg reg, int32_t stack_back_offset) {
        if (reg.isVec()) {
            if (reg.type() == asmjit::RegType::kVec128) {
                if (INT32_MAX > stack_back_offset)
                    throw CompileTimeException("Overflow, fail convert 64 point to 32 point");
                if (UINT16_MAX > stack_back_offset || stack_back_offset % 16) {
                    csm.mov(stack_ptr, stack_back_offset, reg.as<creg128>());
                    if ((uint8_t)csm.offset() != csm.offset())
                        throw CompileTimeException("prolog too large");
                    res.prolog.push_back(stack_back_offset & (UINT32_MAX ^ UINT16_MAX));
                    res.prolog.push_back(stack_back_offset & UINT16_MAX);
                    res.prolog.push_back(UWCODE((uint8_t)csm.offset(), UWC::UWOP_SAVE_XMM128_FAR, reg.id()).solid);
                } else {
                    csm.mov(stack_ptr, stack_back_offset, reg.as<creg128>());
                    if ((uint8_t)csm.offset() != csm.offset())
                        throw CompileTimeException("prolog too large");
                    res.prolog.push_back(uint16_t(stack_back_offset / 16));
                    res.prolog.push_back(UWCODE((uint8_t)csm.offset(), UWC::UWOP_SAVE_XMM128, reg.id()).solid);
                }
            } else
                throw CompileTimeException("Supported only 128 bit vector register");
        } else {
            csm.mov(stack_ptr, stack_back_offset, reg.size(), reg);
            if ((uint8_t)csm.offset() != csm.offset())
                throw CompileTimeException("prolog too large");
            if (stack_back_offset % 8) {
                res.prolog.push_back(stack_back_offset & (UINT32_MAX ^ UINT16_MAX));
                res.prolog.push_back(stack_back_offset & UINT16_MAX);
                res.prolog.push_back(UWCODE((uint8_t)csm.offset(), UWC::UWOP_SAVE_NONVOL_FAR, reg.id()).solid);
            } else {
                res.prolog.push_back(uint16_t(stack_back_offset / 8));
                res.prolog.push_back(UWCODE((uint8_t)csm.offset(), UWC::UWOP_SAVE_NONVOL, reg.id()).solid);
            }
        }
        save_to_stack.push_back({cur_op++, {reg, stack_back_offset}});
    }

    void BuildProlog::end_prolog() {
        if (prolog_preEnd)
            throw CompileTimeException("end_prolog will be used only once");
        if (stack_align & 0xF)
            stackAlloc(8);
        prolog_preEnd = true;
        if ((uint8_t)csm.offset() != csm.offset())
            throw CompileTimeException("prolog too large");
        res.head.SizeOfProlog = (uint8_t)csm.offset();
        if ((uint8_t)res.prolog.size() != res.prolog.size())
            throw CompileTimeException("prolog too large");
        res.head.CountOfUnwindCodes = (uint8_t)res.prolog.size();

        if (res.head.CountOfUnwindCodes & 1)
            res.prolog.push_back(0);
    }

#pragma comment(lib, "Dbghelp.lib")
    mutex DbgHelp_lock;

    struct NativeSymbolResolver {
        std::unordered_map<void*, StackTraceItem, art::hash<void*>> memoized;

        NativeSymbolResolver() {
            SymInitialize(GetCurrentProcess(), nullptr, true);
        }

        ~NativeSymbolResolver() {
            SymCleanup(GetCurrentProcess());
        }

        StackTraceItem GetName(void* frame) {
            {
                auto it = memoized.find(frame);
                if (it != memoized.end())
                    return it->second;
            }
            unsigned char buffer[sizeof(SYMBOL_INFO) + 128];
            PSYMBOL_INFO symbol64 = reinterpret_cast<SYMBOL_INFO*>(buffer);
            memset(symbol64, 0, sizeof(SYMBOL_INFO) + 128);
            symbol64->SizeOfStruct = sizeof(SYMBOL_INFO);
            symbol64->MaxNameLen = 128;

            DWORD64 displacement = 0;
            BOOL result = SymFromAddr(GetCurrentProcess(), (DWORD64)frame, &displacement, symbol64);
            if (result) {
                IMAGEHLP_LINE64 line64;
                DWORD displacement = 0;
                memset(&line64, 0, sizeof(IMAGEHLP_LINE64));
                line64.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
                result = SymGetLineFromAddr64(GetCurrentProcess(), (DWORD64)frame, &displacement, &line64);
                if (memoized.size() > 1000)
                    memoized.erase(memoized.begin());
                if (result)
                    return memoized[frame] = {symbol64->Name, line64.FileName, line64.LineNumber};
                else
                    return memoized[frame] = {symbol64->Name, line64.FileName ? line64.FileName : "UNDEFINED", SIZE_MAX};
            }
            return {"UNDEFINED", "UNDEFINED", SIZE_MAX};
        }
    };

    uint32_t CaptureStackTrace___(uint32_t max_frames, void** out_frames, CONTEXT& context) {
        memset(out_frames, 0, sizeof(void*) * max_frames);

        // RtlCaptureStackBackTrace doesn't support RtlAddFunctionTable..
        //return RtlCaptureStackBackTrace(0, max_frames, out_frames, nullptr);

        //if you wanna port to x32 use RtlCaptureStackBackTrace
        UNWIND_HISTORY_TABLE history;
        memset(&history, 0, sizeof(UNWIND_HISTORY_TABLE));

        ULONG64 establisher_frame = 0;
        PVOID handler_data = nullptr;

        uint32_t frame;
        for (frame = 0; frame < max_frames; frame++) {
            try {
                ULONG64 image_base;
                PRUNTIME_FUNCTION rt_func = RtlLookupFunctionEntry(context.Rip, &image_base, &history);

                KNONVOLATILE_CONTEXT_POINTERS nv_context;
                memset(&nv_context, 0, sizeof(KNONVOLATILE_CONTEXT_POINTERS));
                if (!rt_func) {
                    if (!context.Rsp)
                        break;
                    context.Rip = (ULONG64)(*(PULONG64)context.Rsp);
                    context.Rsp += 8;
                } else
                    RtlVirtualUnwind(UNW_FLAG_NHANDLER, image_base, context.Rip, rt_func, &context, &handler_data, &establisher_frame, &nv_context);


                if (!context.Rip)
                    break;

                out_frames[frame] = (void*)context.Rip;
            } catch (const SegmentationFaultException& e) {
                break;
            }
        }
        return frame;
    }

    uint32_t CaptureStackTrace(uint32_t max_frames, void** out_frames) {
        CONTEXT context;
        RtlCaptureContext(&context);
        return CaptureStackTrace___(max_frames, out_frames, context);
    }

    uint32_t CaptureStackTrace(uint32_t max_frames, void** out_frames, void* rip_frame) {
        CONTEXT context{0};
        context.Rip = (DWORD64)rip_frame;
        return CaptureStackTrace___(max_frames, out_frames, context);
    }

    size_t JITPCToLine(uint8_t* pc, const frame_info* info) {
        //int PCIndex = int(pc - ((uint8_t*)(info->start)));
        //if(info->LineInfo.Size() == 1) return info->LineInfo[0].LineNumber;
        //for (unsigned i = 1; i < info->LineInfo.Size(); i++)
        //{
        //	if(info->LineInfo[i].InstructionIndex >= PCIndex)
        //	{
        //		return info->LineInfo[i - 1].LineNumber;
        //	}
        //}
        return SIZE_MAX;
    }

    template <typename Vec, typename T>
    void pushInVectorAsValue(std::vector<Vec>& vec, T value) {
        size_t _add = sizeof(T) / sizeof(Vec);
        size_t add = _add ? _add : sizeof(Vec) / sizeof(T);
        size_t source_pos = vec.size() * sizeof(Vec);
        vec.resize(vec.size() + add);
        *(T*)(((char*)vec.data()) + source_pos) = value;
    }

    template <typename Vec, typename T>
    void pushInVectorAsArray(std::vector<Vec>& vec, T* value, size_t size) {
        size_t _add = sizeof(T) / sizeof(Vec) * size;
        size_t add = _add ? _add : sizeof(Vec) / sizeof(T) * size;
        size_t source_pos = vec.size() * sizeof(Vec);
        vec.resize(vec.size() + add);
        T* ptr = (T*)(((char*)vec.data()) + source_pos);
        for (size_t i = 0; i < size; i++)
            ptr[i] = value[i];
    }

    //convert FrameResult struct to native unwindInfo
    std::vector<uint16_t> convert(FrameResult& frame) {
        auto& codes = frame.prolog;

        std::vector<uint16_t> info((sizeof(UWINFO_head) >> 1));
        info.reserve(codes.size() + codes.size() & 1);

        frame.head.Flags = frame.use_handle ? UNW_FLAG_EHANDLER | UNW_FLAG_UHANDLER : 0;
        *(UWINFO_head*)(info.data()) = frame.head;

        for (size_t i = codes.size(); i > 0; i--)
            info.push_back(codes[i - 1]);

        if (codes.size() & 1)
            info.push_back(0);

        if (frame.use_handle) {
            pushInVectorAsValue(info, frame.exHandleOff);
            std::vector<uint8_t> handler_info;
            for (size_t i = frame.scope_actions.size(); i > 0; i--) {
                auto& action = frame.scope_actions[i - 1];
                handler_info.push_back((uint8_t)action.action);
                pushInVectorAsValue(handler_info, action.function_begin_off);
                pushInVectorAsValue(handler_info, action.function_end_off);
                switch (action.action) {
                case ScopeAction::Action::destruct_stack:
                    pushInVectorAsValue(handler_info, action.destruct);
                    pushInVectorAsValue(handler_info, action.stack_offset);
                    break;
                case ScopeAction::Action::destruct_register:
                    pushInVectorAsValue(handler_info, action.destruct_register);
                    pushInVectorAsValue(handler_info, action.register_value);
                    break;
                case ScopeAction::Action::filter:
                    pushInVectorAsValue(handler_info, action.filter);
                    pushInVectorAsValue(handler_info, action.filter_data_len);
                    pushInVectorAsArray(handler_info, (char*)action.filter_data, action.filter_data_len);
                    break;
                case ScopeAction::Action::finally:
                    pushInVectorAsValue(handler_info, action.finally);
                    pushInVectorAsValue(handler_info, action.finally_data_len);
                    pushInVectorAsArray(handler_info, (char*)action.finally_data, action.finally_data_len);
                    break;
                default:
                    break;
                }
            }
            pushInVectorAsValue(handler_info, ScopeAction::Action::not_action);
            if (handler_info.size() & 1) {
                handler_info.push_back(0xFFui8);
                pushInVectorAsArray(info, handler_info.data(), handler_info.size());
            } else {
                pushInVectorAsArray(info, handler_info.data(), handler_info.size());
                info.push_back(0xFFFF);
            }
        }
        return info;
    }

    void* FrameResult::init(uint8_t*& frame, CodeHolder* code, asmjit::JitRuntime& runtime, const char* symbol_name, const char* file_path) {
        std::vector<uint16_t> unwindInfo = convert(*this);
        size_t unwindInfoSize = unwindInfo.size() * sizeof(uint16_t);

        uint8_t* baseaddr;
        size_t fun_size = CASM::allocate_and_prepare_code(0, baseaddr, code, runtime.allocator(), unwindInfoSize + sizeof(RUNTIME_FUNCTION));
        if (!baseaddr) {
            const char* err = asmjit::DebugUtils::errorAsString(asmjit::Error(fun_size));
            throw CompileTimeException(err);
        }

        uint8_t* startaddr = baseaddr;
        uint8_t* unwindptr = baseaddr + (((fun_size + 15) >> 4) << 4);
        memcpy(unwindptr, unwindInfo.data(), unwindInfoSize);

        RUNTIME_FUNCTION* table = (RUNTIME_FUNCTION*)(unwindptr + unwindInfoSize);
        frame = (uint8_t*)table;
        table[0].BeginAddress = (DWORD)(ptrdiff_t)(startaddr - baseaddr);
        table[0].EndAddress = (DWORD)(ptrdiff_t)(use_handle ? exHandleOff : fun_size);
        table[0].UnwindData = (DWORD)(ptrdiff_t)(unwindptr - baseaddr);
        BOOLEAN result = RtlAddFunctionTable(table, 1, (DWORD64)baseaddr);

        if (result == 0) {
            runtime.allocator()->release(baseaddr);
            throw CompileTimeException("RtlAddFunctionTable failed");
        }
        auto& tmp = frame_symbols.map[baseaddr];
        tmp.fun_size = fun_size;
        tmp.name = symbol_name;
        tmp.file = file_path;
        return baseaddr;
    }

    bool FrameResult::deinit(uint8_t* frame, void* funct, asmjit::JitRuntime& runtime) {
        if (frame) {
            BOOLEAN result = RtlDeleteFunctionTable((RUNTIME_FUNCTION*)frame);
            auto tmp = runtime.allocator()->release(funct);
            if (!frame_symbols.destroyed)
                frame_symbols.map.erase((uint8_t*)funct);
            return !(result == FALSE || tmp);
        }
        return false;
    };
#else
    ffi_builder& prolog_ffi_builder(void*& ffi_build) {
        if (!ffi_build)
            ffi_build = new ffi_builder();
        return *(ffi_builder*)ffi_build;
    }

    void BuildProlog::pushReg(creg reg) {
        auto& ffi = prolog_ffi_builder(res.prolog_data);
        csm.push(reg.fromTypeAndId(asmjit::RegType::kGp64, reg.id()));
        stack_align += 8;
        ffi.advance_loc(offset());
        ffi.stack_offset(stack_align);
        ffi.offset(reg.id(), stack_align);
        pushes.push_back({cur_op++, reg});
    }

    void BuildProlog::stackAlloc(uint32_t size) {
        if (size == 0)
            return;
        auto& ffi = prolog_ffi_builder(res.prolog_data);
        csm.stackIncrease(size);
        stack_align += size;
        ffi.advance_loc(offset());
        ffi.stack_offset(stack_align);
        stack_alloc.push_back({cur_op++, size});
    }

    void BuildProlog::setFrame(uint16_t stack_offset) {
        if (frame_inited)
            throw CompileTimeException("Frame already inited");
        if (!stack_offset)
            csm.mov(frame_ptr, stack_ptr);
        else
            csm.lea(frame_ptr, stack_ptr, stack_offset);
        set_frame.push_back({cur_op++, stack_offset});
        frame_inited = true;
    }

    void BuildProlog::saveToStack(creg reg, int32_t stack_back_offset) {
        auto& ffi = prolog_ffi_builder(res.prolog_data);
        if (reg.isVec()) {
            csm.mov(stack_ptr, stack_back_offset, reg.as<creg128>());
            ffi.advance_loc(offset());
            ffi.offset(17 + reg.id(), stack_back_offset);
        } else {
            csm.mov(stack_ptr, stack_back_offset, reg.size(), reg);
            ffi.advance_loc(offset());
            ffi.offset(reg.id(), stack_back_offset);
        }
        save_to_stack.push_back({cur_op++, {reg, stack_back_offset}});
    }

    void BuildProlog::end_prolog() {
        if (prolog_preEnd)
            throw CompileTimeException("end_prolog will be used only once");
        if (stack_align & 0xF)
            stackAlloc(8);
        prolog_preEnd = true;
    }

    void BuildProlog::cleanup_frame() {
        if (res.prolog_data)
            delete (ffi_builder*)res.prolog_data;
    }

    void _______dbgOut(const char*) {
        //TODO
    }

    struct NativeSymbolResolver {
        StackTraceItem GetName(void* frame) {
            char** strings;
            void* frames[1] = {frame};
            strings = backtrace_symbols(frames, 1);

            char* ptr = strings[0];
            char* filename = ptr;
            const char* function = "";

            while (*ptr) {
                if (*ptr == '(') {
                    *(ptr++) = 0;
                    function = ptr;
                    break;
                }
                ptr++;
            }

            if (function[0]) {
                while (*ptr) {
                    if (*ptr == '+' || *ptr == ')') {
                        *(ptr++) = 0;
                        break;
                    }
                    ptr++;
                }
            }

            int status;
            char* new_function = abi::__cxa_demangle(function, nullptr, nullptr, &status);
            if (new_function)
                function = new_function;


            art::ustring s = "Called from " + art::ustring(function) + " at " + filename + '\n';
            if (new_function)
                free(new_function);
            free(strings);
            return {s, "UNDEFINED", SIZE_MAX};
        }
    };

    size_t JITPCToLine(uint8_t* pc, const frame_info* info) {
        //int PCIndex = int(pc - ((uint8_t*)(info->start)));
        //if(info->LineInfo.Size() == 1) return info->LineInfo[0].LineNumber;
        //for (unsigned i = 1; i < info->LineInfo.Size(); i++)
        //{
        //	if(info->LineInfo[i].InstructionIndex >= PCIndex)
        //	{
        //		return info->LineInfo[i - 1].LineNumber;
        //	}
        //}
        return SIZE_MAX;
    }

    uint32_t CaptureStackTrace(uint32_t max_frames, void** out_frames) {
        if (max_frames != (int32_t)max_frames)
            return 0;
        return (uint32_t)backtrace(out_frames, (int32_t)max_frames);
    }

    uint32_t CaptureStackTrace(uint32_t max_frames, void** out_frames, void* rip) {
        return 0;
    }

    DWARF build_dwarf(void*& prolog_data, bool use_handle, uint32_t exHandleOff) {
        auto& ffi = prolog_ffi_builder(prolog_data);
        cfi_builder cfi;
        cfi.def_cfa(stack_ptr.id(), 8);
        cfi.offset(16, 8);
        CIE_entry cie_entry;
        cie_entry.id = 0;
        cie_entry.version = 1;
        cie_entry.return_address_register = 16;
        cie_entry.personality.enabled = use_handle;
        cie_entry.personality.plt_entry = use_handle ? exHandleOff : 0;
        cie_entry.code_alignment_factor = 1;
        cie_entry.data_alignment_factor = -1;
        cie_entry.use_fde.enabled = true;
        cie_entry.lsda.enabled = false;
        cie_entry.augmentation_remainder.enabled = true;

        FDE_entry fde_entry;
        return complete_dwarf(
            make_CIE(cie_entry, {0}, cfi.data),
            fde_entry,
            {},
            ffi.data);
    }

    void* FrameResult::init(uint8_t*& frame, CodeHolder* code, asmjit::JitRuntime& runtime, const char* symbol_name, const char* file_path) {
        uint8_t* baseaddr;
        DWARF unwindInfo = build_dwarf(prolog_data, use_handle, exHandleOff);
        unwindInfo.data.commit();
        size_t fun_size = CASM::allocate_and_prepare_code(0, baseaddr, code, runtime.allocator(), unwindInfo.data.size());
        if (!baseaddr) {
            const char* err = asmjit::DebugUtils::errorAsString(asmjit::Error(fun_size));
            throw CompileTimeException(err);
        }
        auto fde = baseaddr + fun_size;
        unwindInfo.patch_function_address((uint64_t)baseaddr);
        unwindInfo.patch_function_size(fun_size);
        memcpy(fde, unwindInfo.data.data(), unwindInfo.data.size());
        __register_frame(fde); //libgcc
        //TO-DO register frames for:
        // libunwind _U_dyn_register							https://www.nongnu.org/libunwind/man/_U_dyn_register(3).html
        // GDB __jit_debug_register_code 						https://sourceware.org/gdb/onlinedocs/gdb/JIT-Interface.html
        // oprofile  op_write_native_code    					https://oprofile.sourceforge.io/doc/devel/jit-interface.html
        // perf /tmp/perf-%d.map  "START SIZE symbolname\n"		https://github.com/torvalds/linux/blob/master/tools/perf/Documentation/jit-interface.txt

        //TO-DO
        //also support android http://blog.httrack.com/blog/2013/08/23/catching-posix-signals-on-android/

        auto& tmp = frame_symbols.map[baseaddr];
        tmp.fun_size = fun_size;
        tmp.name = symbol_name;
        tmp.file = file_path;
        frame = fde;
        return baseaddr;
    }

    bool FrameResult::deinit(uint8_t* frame, void* funct, asmjit::JitRuntime& runtime) {
        __deregister_frame(frame);
        if (!frame_symbols.destroyed)
            frame_symbols.map.erase((uint8_t*)funct);
        return !(runtime._release(funct));
    };
#endif


    StackTraceItem JitGetStackFrameName(NativeSymbolResolver* nativeSymbols, void* pc) {
        if (!frame_symbols.destroyed) {
            for (auto& it : frame_symbols.map) {
                auto& info = it.second;
                if (pc >= it.first && pc < (it.first + info.fun_size))
                    return {info.name, info.file, JITPCToLine((uint8_t*)pc, &info)};
            }
        }
        return nativeSymbols ? nativeSymbols->GetName(pc) : StackTraceItem("UNDEFINED", "UNDEFINED", SIZE_MAX);
    }

    std::vector<StackTraceItem> FrameResult::JitCaptureStackTrace(uint32_t framesToSkip, bool includeNativeFrames, uint32_t max_frames) {
#ifdef _WIN64
        art::lock_guard lg(DbgHelp_lock); //in windows NativeSymbolResolver class and CaptureStackTrace function use single thread DbgHelp functions
#endif
        if (max_frames + 1 == 0)
            throw std::bad_array_new_length();
        max_frames += 1;
        std::unique_ptr<void*, std::default_delete<void*[]>> frames_buffer(new void*[max_frames]);
        void** frame = frames_buffer.get();
        uint32_t num_frames = CaptureStackTrace(max_frames, frame);

        std::unique_ptr<NativeSymbolResolver> nativeSymbols;
        if (includeNativeFrames)
            nativeSymbols.reset(new NativeSymbolResolver());

        std::vector<StackTraceItem> stack_trace;
        for (uint32_t i = framesToSkip + 1; i < num_frames; i++)
            stack_trace.push_back(JitGetStackFrameName(nativeSymbols.get(), frame[i]));
        return stack_trace;
    }

    std::vector<void*> FrameResult::JitCaptureStackChainTrace(uint32_t framesToSkip, bool includeNativeFrames, uint32_t max_frames) {
#ifdef _WIN64
        art::lock_guard lg(DbgHelp_lock); //in windows NativeSymbolResolver class and CaptureStackTrace function use single thread DbgHelp functions
#endif
        if (max_frames + 1 == 0)
            throw std::bad_array_new_length();
        max_frames += 1;
        std::unique_ptr<void*, std::default_delete<void*[]>> frames_buffer(new void*[max_frames]);
        void** frame = frames_buffer.get();
        uint32_t num_frames = CaptureStackTrace(max_frames, frame);
        if (framesToSkip >= num_frames)
            return {};
        else
            return std::vector<void*>(frame + framesToSkip, frame + num_frames);
    }

    std::vector<StackTraceItem> FrameResult::JitCaptureExternStackTrace(void* rip, uint32_t framesToSkip, bool includeNativeFrames, uint32_t max_frames) {
#ifdef _WIN64
        art::lock_guard lg(DbgHelp_lock); //in windows NativeSymbolResolver class and CaptureStackTrace function use single thread DbgHelp functions
#endif
        if (max_frames + 1 == 0)
            throw std::bad_array_new_length();
        max_frames += 1;
        std::unique_ptr<void*, std::default_delete<void*[]>> frames_buffer(new void*[max_frames]);
        void** frame = frames_buffer.get();
        uint32_t num_frames = CaptureStackTrace(max_frames, frame, rip);

        std::unique_ptr<NativeSymbolResolver> nativeSymbols;
        if (includeNativeFrames)
            nativeSymbols.reset(new NativeSymbolResolver());

        std::vector<StackTraceItem> stack_trace;
        for (uint32_t i = framesToSkip + 1; i < num_frames; i++)
            stack_trace.push_back(JitGetStackFrameName(nativeSymbols.get(), frame[i]));
        return stack_trace;
    }

    std::vector<void*> FrameResult::JitCaptureExternStackChainTrace(void* rip, uint32_t framesToSkip, bool includeNativeFrames, uint32_t max_frames) {
#ifdef _WIN64
        art::lock_guard lg(DbgHelp_lock); //in windows NativeSymbolResolver class and CaptureStackTrace function use single thread DbgHelp functions
#endif
        if (max_frames + 1 == 0)
            throw std::bad_array_new_length();
        max_frames += 1;
        std::unique_ptr<void*, std::default_delete<void*[]>> frames_buffer(new void*[max_frames]);
        void** frame = frames_buffer.get();
        uint32_t num_frames = CaptureStackTrace(max_frames, frame, rip);
        if (framesToSkip >= num_frames)
            return {};
        else
            return std::vector<void*>(frame + framesToSkip, frame + num_frames);
    }

    StackTraceItem FrameResult::JitResolveFrame(void* rip, bool include_native) {
        if (include_native) {
#ifdef _WIN64
            art::lock_guard lg(DbgHelp_lock); //in windows NativeSymbolResolver class and CaptureStackTrace function use single thread DbgHelp functions
#endif
            std::unique_ptr<NativeSymbolResolver> nativeSymbols;
            nativeSymbols.reset(new NativeSymbolResolver());
            return JitGetStackFrameName(nativeSymbols.get(), rip);
        } else
            return JitGetStackFrameName(nullptr, rip);
    }
}