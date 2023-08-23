// Copyright Danyil Melnytskyi 2022-2023
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <run_time/asm/CASM.hpp>
#include <run_time/asm/exception.hpp>
#include <util/enum_class.hpp>
#include <util/cxxException.hpp>
#include <util/ustring.hpp>
#ifdef _WIN64
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif

namespace art{
    art::ustring inner_exception_info(const std::exception_ptr& inner_exception) {
        CXXExInfo ex;
        getCxxExInfoFromException(ex, inner_exception);
        if(ex.ex_ptr == nullptr) {
    #ifdef _WIN64
            switch (ex.native_id) {
            case EXCEPTION_ACCESS_VIOLATION:
                return "\nAccessViolation: NATIVE EXCEPTION";
            case EXCEPTION_STACK_OVERFLOW:
                return "\nStackOverflow: NATIVE EXCEPTION";
            case EXCEPTION_INT_DIVIDE_BY_ZERO:
            case EXCEPTION_FLT_DIVIDE_BY_ZERO:
                return "\nDivideByZero: NATIVE EXCEPTION";
            case EXCEPTION_INT_OVERFLOW:
            case EXCEPTION_FLT_OVERFLOW:
            case EXCEPTION_FLT_STACK_CHECK:
            case EXCEPTION_FLT_UNDERFLOW:
                return "\nNumericOverflow: NATIVE EXCEPTION";
            case EXCEPTION_BREAKPOINT: 
                return "\nBreakpoint: NATIVE EXCEPTION";
            case EXCEPTION_ILLEGAL_INSTRUCTION:
                return "\nIllegalInstruction: NATIVE EXCEPTION";
            case EXCEPTION_PRIV_INSTRUCTION:
                return "\nPrivilegedInstruction: NATIVE EXCEPTION";
            default:
                return "\nUnknownNativeException: NATIVE EXCEPTION";
            }
    #else
            return "\nUnknownNativeException: NATIVE EXCEPTION";
    #endif
        } else {
            try{
                std::rethrow_exception(inner_exception);
            }catch(const AttachARuntimeException& e) {
                return "\n" + e.full_info();
            }catch(const std::exception& e) {
                return "\nC++Exception: " + art::ustring(e.what());
            }catch(...) {
                return "\nC++Exception: UNKNOWN";
            }
        }
    }
        

    art::ustring AttachARuntimeException::full_info() const {
        art::ustring result = name() + art::ustring(": ") + what();
        if (inner_exception) 
            result += inner_exception_info(inner_exception);
        return result;
    }

    InternalException::InternalException(const art::ustring& msq) : AttachARuntimeException(msq) {
        auto tmp_trace = FrameResult::JitCaptureStackChainTrace();
        stack_trace = {tmp_trace.begin(), tmp_trace.end()};
    }
    InternalException::InternalException(const art::ustring& msq, std::exception_ptr inner_exception) : AttachARuntimeException(msq, inner_exception) {
        auto tmp_trace = FrameResult::JitCaptureStackChainTrace();
        stack_trace = {tmp_trace.begin(), tmp_trace.end()};
    }
    art::ustring InternalException::full_info()  const {
        art::ustring result = AttachARuntimeException::full_info();
        result += "\nStackTrace:";
        for (auto& frame : stack_trace) {
            auto r_frame = FrameResult::JitResolveFrame(frame);
            result += "\n\t" + r_frame.fn_name + ": " + std::to_string(r_frame.line);
        }
        return result;
    }
        
    art::ustring RoutineHandleExceptions::full_info() const {
        art::ustring res = name() + art::ustring(": caught two exceptions in routine: ");
        res += inner_exception_info(get_inner_exception());
        res += inner_exception_info(second_exception);
        return res;
    }
}