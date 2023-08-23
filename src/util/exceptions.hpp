// Copyright Danyil Melnytskyi 2022-2023
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef SRC_RUN_TIME_EXCEPTIONS
#define SRC_RUN_TIME_EXCEPTIONS

#include <string>
#include <exception>
#include <library/list_array.hpp>
#include <util/string_help.hpp>
#include <util/ustring.hpp>
namespace art {
#pragma region AnyTimeExceptions
	class AttachARuntimeException {
		art::ustring message;
		std::exception_ptr inner_exception;
	public:
		AttachARuntimeException() { message = ""; }
		AttachARuntimeException(std::exception_ptr inner_exception) : inner_exception(inner_exception) { message = ""; }
		AttachARuntimeException(const art::ustring& msq) : message(msq) {}
		AttachARuntimeException(const art::ustring& msq, std::exception_ptr inner_exception) : inner_exception(inner_exception), message(msq) {}
		virtual ~AttachARuntimeException() noexcept(false) {}
		const art::ustring& what() const {
			return message;
		}
		virtual art::ustring full_info() const;
		virtual const char* name() const {
			return "attach_a_runtime_exception";
		}
		void throw_inner_exception() const  {
			std::rethrow_exception(inner_exception);
		}
		std::exception_ptr get_inner_exception() const  {
			return inner_exception;
		}
	};
	class InvalidCast : public virtual AttachARuntimeException {
	public:
		InvalidCast(const art::ustring& msq) : AttachARuntimeException(msq) {}
		InvalidCast(const art::ustring& msq, std::exception_ptr inner_exception) : AttachARuntimeException(msq, inner_exception) {}
		const char* name() const override {
			return "invalid_cast";
		}
	};
	class UnmodifiableValue : public AttachARuntimeException {
	public:
		UnmodifiableValue() : AttachARuntimeException() {}
		UnmodifiableValue(std::exception_ptr inner_exception) : AttachARuntimeException(inner_exception) {}
		const char* name() const override {
			return "unmodifiable_value";
		}
	};
	class InvalidOperation : public virtual AttachARuntimeException {
	public:
		InvalidOperation(const art::ustring& msq) : AttachARuntimeException(msq) {}
		InvalidOperation(const art::ustring& msq, std::exception_ptr inner_exception) : AttachARuntimeException(msq, inner_exception) {}
		const char* name() const override {
			return "invalid_operation";
		}
	};
	class InvalidArguments : public AttachARuntimeException {
	public:
		InvalidArguments(const art::ustring& msq) : AttachARuntimeException(msq) {}
		InvalidArguments(const art::ustring& msq, std::exception_ptr inner_exception) : AttachARuntimeException(msq, inner_exception) {}
		const char* name() const override {
			return "invalid_arguments";
		}
	};
	class InvalidLock : public InvalidOperation {
	public:
		InvalidLock(const art::ustring& msq) : InvalidOperation(msq) {}
		InvalidLock(const art::ustring& msq, std::exception_ptr inner_exception) : InvalidOperation(msq, inner_exception) {}
		const char* name() const override {
			return "invalid_lock";
		}
	};
	class InvalidUnlock : public InvalidOperation {
	public:
		InvalidUnlock(const art::ustring& msq) : InvalidOperation(msq) {}
		InvalidUnlock(const art::ustring& msq, std::exception_ptr inner_exception) : InvalidOperation(msq, inner_exception) {}
		const char* name() const override {
			return "invalid_unlock";
		}
	};
	class InvalidInput : public AttachARuntimeException {
	public:
		InvalidInput(const art::ustring& msq) : AttachARuntimeException(msq) {}
		InvalidInput(const art::ustring& msq, std::exception_ptr inner_exception) : AttachARuntimeException(msq, inner_exception) {}
		const char* name() const override {
			return "invalid_input";
		}
	};
	class NotImplementedException : public virtual AttachARuntimeException {
	public:
		NotImplementedException() : AttachARuntimeException("Entered to non implemented region") {}
		NotImplementedException(std::exception_ptr inner_exception) : AttachARuntimeException("Entered to non implemented region", inner_exception) {}
		const char* name() const override {
			return "attach_a_runtime_exception";
		}
	};
	class UnsupportedOperation : public NotImplementedException, public InvalidOperation {
	public:
		UnsupportedOperation() : InvalidOperation("Caught unsupported Operation") {}
		UnsupportedOperation(std::exception_ptr inner_exception) : InvalidOperation("Caught unsupported Operation", inner_exception) {}
		UnsupportedOperation(const art::ustring& msq) : InvalidOperation(msq) {}
		UnsupportedOperation(const art::ustring& msq, std::exception_ptr inner_exception) : InvalidOperation(msq, inner_exception) {}
		const char* name() const override {
			return "unsupported_operation";
		}
	};
	class OutOfRange : public AttachARuntimeException {
	public:
		OutOfRange() : AttachARuntimeException("Out of range") {}
		OutOfRange(std::exception_ptr inner_exception) : AttachARuntimeException("Out of range", inner_exception) {}
		OutOfRange(const art::ustring& str) : AttachARuntimeException(str) {}
		OutOfRange(const art::ustring& str, std::exception_ptr inner_exception) : AttachARuntimeException(str, inner_exception) {}
		const char* name() const override {
			return "out_of_range";
		}
	};
	class InvalidClassDeclarationException : public AttachARuntimeException {
	public:
		InvalidClassDeclarationException() : AttachARuntimeException("Invalid Class Declaration Exception") {}
		InvalidClassDeclarationException(const art::ustring& desc) : AttachARuntimeException("Invalid Class Declaration Exception: " + desc) {}
		const char* name() const override {
			return "invalid_class_declaration_exception";
		}
	};
	class LibraryNotFoundException : public AttachARuntimeException {
	public:
		LibraryNotFoundException() : AttachARuntimeException("library not found") {}
		LibraryNotFoundException(std::exception_ptr inner_exception) : AttachARuntimeException("library not found", inner_exception) {}
		LibraryNotFoundException(const art::ustring& desc) : AttachARuntimeException(desc) {}
		LibraryNotFoundException(const art::ustring& desc, std::exception_ptr inner_exception) : AttachARuntimeException(desc, inner_exception) {}
		const char* name() const override {
			return "library_not_found_exception";
		}
	};
	class LibraryFunctionNotFoundException : public AttachARuntimeException {
	public:
		LibraryFunctionNotFoundException() : AttachARuntimeException("library function not found") {}
		LibraryFunctionNotFoundException(std::exception_ptr inner_exception) : AttachARuntimeException("library function not found", inner_exception) {}
		LibraryFunctionNotFoundException(const art::ustring& desc) : AttachARuntimeException(desc) {}
		LibraryFunctionNotFoundException(const art::ustring& desc, std::exception_ptr inner_exception) : AttachARuntimeException(desc, inner_exception) {}
		const char* name() const override {
			return "library_function_not_found_exception";
		}
	};
	class EnvironmentRuinException : public AttachARuntimeException {
	public:
		EnvironmentRuinException() : AttachARuntimeException("Environment ruin exception") {}
		EnvironmentRuinException(std::exception_ptr inner_exception) : AttachARuntimeException("Environment ruin exception", inner_exception) {}
		EnvironmentRuinException(const art::ustring& desc) : AttachARuntimeException("EnvironmentRuinException: " + desc) {}
		EnvironmentRuinException(const art::ustring& desc, std::exception_ptr inner_exception) : AttachARuntimeException(desc, inner_exception) {}
		const char* name() const override {
			return "environment_ruin_exception";
		}
	};
	class InvalidArchitectureException : public AttachARuntimeException {
	public:
		InvalidArchitectureException() : AttachARuntimeException("Invalid architecture") {}
		InvalidArchitectureException(std::exception_ptr inner_exception) : AttachARuntimeException("Invalid architecture", inner_exception) {}
		const char* name() const override {
			return "invalid_architecture_exception";
		}
	};
	class StackOverflowException : public AttachARuntimeException {
	public:
		StackOverflowException() {}
		StackOverflowException(std::exception_ptr inner_exception) : AttachARuntimeException("Stack overflow", inner_exception) {}
		const char* name() const override {
			return "stack_overflow_exception";
		}
	};
	class UnusedDebugPointException : public AttachARuntimeException {
	public:
		UnusedDebugPointException() : AttachARuntimeException("Unused debug breakpoint") {}
		UnusedDebugPointException(std::exception_ptr inner_exception) : AttachARuntimeException("Unused debug breakpoint",inner_exception) {}
		const char* name() const override {
			return "unused_debug_point_exception";
		}
	};
	class DivideByZeroException : public AttachARuntimeException {
	public:
		DivideByZeroException() : AttachARuntimeException("Number divided by zero") {}
		DivideByZeroException(std::exception_ptr inner_exception) : AttachARuntimeException("Number divided by zero", inner_exception) {}
		const char* name() const override {
			return "divide_by_zero_exception";
		}
	};
	class BadInstructionException : public AttachARuntimeException {
	public:
		BadInstructionException() : AttachARuntimeException("This instruction undefined") {}
		BadInstructionException(std::exception_ptr inner_exception) : AttachARuntimeException("This instruction undefined", inner_exception) {}
		BadInstructionException(const art::ustring& msq) : AttachARuntimeException(msq) {}
		BadInstructionException(const art::ustring& msq, std::exception_ptr inner_exception) : AttachARuntimeException(msq, inner_exception) {}
		const char* name() const override {
			return "bad_instruction_exception";
		}
	};
	class NumericOverflowException : public AttachARuntimeException {
	public:
		NumericOverflowException() : AttachARuntimeException("Caught numeric overflow") {}
		NumericOverflowException(std::exception_ptr inner_exception) : AttachARuntimeException("Caught numeric overflow", inner_exception) {}
		const char* name() const override {
			return "numeric_overflow_exception";
		}
	};
	class NumericUndererflowException : public AttachARuntimeException {
	public:
		NumericUndererflowException() : AttachARuntimeException("Caught numeric underflow") {}
		NumericUndererflowException(std::exception_ptr inner_exception) : AttachARuntimeException("Caught numeric underflow", inner_exception) {}
		const char* name() const override {
			return "numeric_undererflow_exception";
		}
	};
	class SegmentationFaultException : public AttachARuntimeException {
	public:
		SegmentationFaultException() : AttachARuntimeException("Thread try get access to non mapped region") {}
		SegmentationFaultException(std::exception_ptr inner_exception) : AttachARuntimeException("Thread try get access to non mapped region", inner_exception) {}
		SegmentationFaultException(const art::ustring& text) : AttachARuntimeException(text) {}
		SegmentationFaultException(const art::ustring& text, std::exception_ptr inner_exception) : AttachARuntimeException(text, inner_exception) {}
		const char* name() const override {
			return "segmentation_fault_exception";
		}
	};
	class NullPointerException : public SegmentationFaultException {
	public:
		NullPointerException() : SegmentationFaultException("Thread try get access to null pointer region") {}
		NullPointerException(std::exception_ptr inner_exception) : SegmentationFaultException("Thread try get access to null pointer region", inner_exception) {}
		NullPointerException(const art::ustring& text) : SegmentationFaultException(text) {}
		NullPointerException(const art::ustring& text, std::exception_ptr inner_exception) : SegmentationFaultException(text, inner_exception) {}
		const char* name() const override {
			return "null_pointer_exception";
		}
	};
	class NoMemoryException : public AttachARuntimeException {
	public:
		NoMemoryException() : AttachARuntimeException("No memory") {}
		NoMemoryException(std::exception_ptr inner_exception) : AttachARuntimeException("No memory", inner_exception) {}
		const char* name() const override {
			return "no_memory_exception";
		}
	};

	class AttachedLangException : public AttachARuntimeException {
	public:
		AttachedLangException() : AttachARuntimeException("Caught unconvertible external attached langue exception") {}
		AttachedLangException(std::exception_ptr inner_exception) : AttachARuntimeException("Caught unconvertible external attached langue exception", inner_exception) {}
		const char* name() const override {
			return "attached_lang_exception";
		}
	};
	class DeprecatedException : public AttachARuntimeException {
	public:
		DeprecatedException() : AttachARuntimeException("This function deprecated") {}
		DeprecatedException(std::exception_ptr inner_exception) : AttachARuntimeException("This function deprecated", inner_exception) {}
		const char* name() const override {
			return "deprecated_exception";
		}
	};
	class SystemException : public AttachARuntimeException {
	public:
		SystemException(uint32_t error_code) : AttachARuntimeException("System error: " + std::to_string(error_code)) {}
		SystemException(uint32_t error_code, std::exception_ptr inner_exception) : AttachARuntimeException("System error: "+ std::to_string(error_code), inner_exception) {}
		const char* name() const override {
			return "system_exception";
		}
	};
	class AllocationException : public AttachARuntimeException {
	public:
		AllocationException(const art::ustring& msq) : AttachARuntimeException(msq) {}
		AllocationException(const art::ustring& msq, std::exception_ptr inner_exception) : AttachARuntimeException(msq, inner_exception) {}
		const char* name() const override {
			return "allocation_exception";
		}
	};

	class InternalException : public AttachARuntimeException {
		list_array<void*> stack_trace;
	public:
		InternalException(const art::ustring& msq);
		InternalException(const art::ustring& msq, std::exception_ptr inner_exception);
		const char* name() const override {
			return "internal_exception";
		}
		art::ustring full_info() const override;
	};
	class RoutineHandleExceptions : public AttachARuntimeException {
		std::exception_ptr second_exception;
	public:
		RoutineHandleExceptions(std::exception_ptr first_exception, std::exception_ptr second_exception) : AttachARuntimeException(first_exception), second_exception(second_exception) {}
		const char* name() const override {
			return "multiple_exceptions";
		}
		art::ustring full_info() const override;
	};

	class MissingDependencyException : public AttachARuntimeException {
	public:
		MissingDependencyException(const art::ustring& msq) : AttachARuntimeException(msq) {}
		MissingDependencyException(const art::ustring& msq, std::exception_ptr inner_exception) : AttachARuntimeException(msq, inner_exception) {}
		const char* name() const override {
			return "missing_dependency_exception";
		}
	};

	class InvalidEncodingException : public AttachARuntimeException {
	public:
		InvalidEncodingException(const art::ustring& msq) : AttachARuntimeException(msq) {}
		InvalidEncodingException(const art::ustring& msq, std::exception_ptr inner_exception) : AttachARuntimeException(msq, inner_exception) {}
		const char* name() const override {
			return "invalid_encoding_exception";
		}
	};
#pragma endregion




#pragma region CompileTimeExceptions
	class CompileTimeException : public AttachARuntimeException {
	public:
		CompileTimeException(const art::ustring& msq) : AttachARuntimeException(msq) {}
		CompileTimeException(const art::ustring& msq, std::exception_ptr inner_exception) : AttachARuntimeException(msq, inner_exception) {}
		const char* name() const override {
			return "compile_time_exception";
		}
	};
	class HotPathException : public CompileTimeException {
	public:
		HotPathException(const art::ustring& msq) : CompileTimeException(msq) {}
		HotPathException(const art::ustring& msq, std::exception_ptr inner_exception) : CompileTimeException(msq, inner_exception) {}
		const char* name() const override {
			return "hot_path_exception";
		}
	};
	class SymbolException : public CompileTimeException {
	public:
		SymbolException(const art::ustring& msq) : CompileTimeException(msq) {}
		SymbolException(const art::ustring& msq, std::exception_ptr inner_exception) : CompileTimeException(msq, inner_exception) {}
		const char* name() const override {
			return "symbol_exception";
		}
	};
	class InvalidFunction : public CompileTimeException {
	public:
		InvalidFunction(const art::ustring& msq) : CompileTimeException(msq) {}
		InvalidFunction(const art::ustring& msq, std::exception_ptr inner_exception) : CompileTimeException(msq, inner_exception) {}
		const char* name() const override {
			return "invalid_function";
		}
	};
	class InvalidIL : public InvalidFunction {
	public:
		InvalidIL(const art::ustring& msq) : InvalidFunction(msq) {}
		InvalidIL(const art::ustring& msq, std::exception_ptr inner_exception) : InvalidFunction(msq, inner_exception) {}
		const char* name() const override {
			return "invalid_il";
		}
	};
	class InvalidType : public CompileTimeException {
	public:
		InvalidType(const art::ustring& msq) : CompileTimeException(msq) {}
		InvalidType(const art::ustring& msq, std::exception_ptr inner_exception) : CompileTimeException(msq, inner_exception) {}
		const char* name() const override {
			return "invalid_type";
		}
	};
	class BadOperationException : public CompileTimeException {
	public:
		BadOperationException() : CompileTimeException("Bad Operation") {}
		BadOperationException(std::exception_ptr inner_exception) : CompileTimeException("Bad Operation", inner_exception) {}
		const char* name() const override {
			return "bad_operation_exception";
		}
	};
#pragma endregion


	//this exception can be throw from attacha runtime
	class AException : public AttachARuntimeException {
		art::ustring _name;
	public:
		AException(const art::ustring& ex_name, const art::ustring& description, void* va = nullptr, size_t ty = 0) : _name(string_help::replace_space(ex_name)), AttachARuntimeException(description), v(va), t(ty) {}
		const char* name() const override {
			return _name.c_str();
		}
		void* v;
		size_t t;
	};
}


#endif /* SRC_RUN_TIME_EXCEPTIONS */
