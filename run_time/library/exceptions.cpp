#include "exceptions.hpp"
#include "../CASM.hpp"
#include "../cxxException.hpp"
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>




std::string AttachARuntimeException::full_info() const {
    std::string result = name() + std::string(": ") + what();
    if (inner_exception != nullptr) {
        CXXExInfo ex;
        getCxxExInfoFromException(ex, inner_exception);
#ifdef _WIN64
        if(ex.ex_ptr == nullptr) {
            switch (ex.native_id) {
            case EXCEPTION_ACCESS_VIOLATION:
                result += "\nAccessViolation: NATIVE EXCEPTION";
            case EXCEPTION_STACK_OVERFLOW:
                result += "\nStackOverflow: NATIVE EXCEPTION";
            case EXCEPTION_INT_DIVIDE_BY_ZERO:
            case EXCEPTION_FLT_DIVIDE_BY_ZERO:
                result += "\nDivideByZero: NATIVE EXCEPTION";
            case EXCEPTION_INT_OVERFLOW:
            case EXCEPTION_FLT_OVERFLOW:
            case EXCEPTION_FLT_STACK_CHECK:
            case EXCEPTION_FLT_UNDERFLOW:
                result += "\nNumericOverflow: NATIVE EXCEPTION";
            case EXCEPTION_BREAKPOINT: 
                result += "\nBreakpoint: NATIVE EXCEPTION";
            case EXCEPTION_ILLEGAL_INSTRUCTION:
                result += "\nIllegalInstruction: NATIVE EXCEPTION";
            case EXCEPTION_PRIV_INSTRUCTION:
                result += "\nPrivilegedInstruction: NATIVE EXCEPTION";
            default:
                result += "\nUnknownNativeException: NATIVE EXCEPTION";
            }
#else
#endif
	    } else {
            try{
                std::rethrow_exception(inner_exception);
                
            }catch(const AttachARuntimeException& e) {
                result += "\n" + e.full_info();
            }catch(const std::exception& e) {
                result += "\nC++Exception: " + std::string(e.what());
            }catch(...) {
                result += "\nC++Exception: UNKNOWN";
            }
        }
    }
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
    
