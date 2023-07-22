// Copyright Danyil Melnytskyi 2022-2023
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include "exceptions.hpp"
#include "asm/CASM.hpp"
#include "cxxException.hpp"
#include "asm/exception.hpp"
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

namespace art{
    std::string inner_exception_info(const std::exception_ptr& inner_exception) {
        CXXExInfo ex;
        getCxxExInfoFromException(ex, inner_exception);
    #ifdef _WIN64
        if(ex.ex_ptr == nullptr) {
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
    #endif
        } else {
            try{
                std::rethrow_exception(inner_exception);
            }catch(const AttachARuntimeException& e) {
                return "\n" + e.full_info();
            }catch(const std::exception& e) {
                return "\nC++Exception: " + std::string(e.what());
            }catch(...) {
                return "\nC++Exception: UNKNOWN";
            }
        }
    }
        

    std::string AttachARuntimeException::full_info() const {
        std::string result = name() + std::string(": ") + what();
        if (inner_exception) 
            result += inner_exception_info(inner_exception);
        return result;
    }

    InternalException::InternalException(const std::string& msq) : AttachARuntimeException(msq) {
        auto tmp_trace = FrameResult::JitCaptureStackChainTrace();
        stack_trace = {tmp_trace.begin(), tmp_trace.end()};
    }
    InternalException::InternalException(const std::string& msq, std::exception_ptr inner_exception) : AttachARuntimeException(msq, inner_exception) {
        auto tmp_trace = FrameResult::JitCaptureStackChainTrace();
        stack_trace = {tmp_trace.begin(), tmp_trace.end()};
    }
    std::string InternalException::full_info()  const {
        std::string result = AttachARuntimeException::full_info();
        result += "\nStackTrace:";
        for (auto& frame : stack_trace) {
            auto rframe = FrameResult::JitResolveFrame(frame);
            result += "\n\t" + rframe.fn_name + ": " + std::to_string(rframe.line);
        }
        return result;
    }
        
    std::string RoutineHandleExceptions::full_info() const {
        std::string res = name() + std::string(": caught two exceptions in routine: ");
        res += inner_exception_info(get_iner_exception());
        res += inner_exception_info(second_exception);
        return res;
    }
}