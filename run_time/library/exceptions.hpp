// Copyright Danyil Melnytskyi 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once
#include <string>
#include "../../library/list_array.hpp"
static inline std::string replace_space(const std::string& str) {
	std::string res;
	res.reserve(str.size());
	for (char it : str)
		res.push_back(it == ' ' ? '_' : it);
	return res;
}
class AttachARuntimeException {
	std::string message;
public:
	AttachARuntimeException() { message = ""; }
	AttachARuntimeException(const char* msq) : message(msq) {}
	AttachARuntimeException(const std::string& msq) : message(msq) {}
	virtual ~AttachARuntimeException() noexcept(false) {}
	const std::string& what() const {
		return message;
	}
	virtual const char* name() const {
		return "AttachARuntimeException";
	}
};
class InvalidCast : public AttachARuntimeException {
public:
	InvalidCast(const std::string& msq) : AttachARuntimeException(msq) {}
	const char* name() const override {
		return "InvalidCast";
	}
};
class UnmodifabeValue : public AttachARuntimeException {
public:
	UnmodifabeValue() : AttachARuntimeException() {}
	const char* name() const override {
		return "UnmodifabeValue";
	}
};
class InvalidOperation : public AttachARuntimeException {
public:
	InvalidOperation(const std::string& msq) : AttachARuntimeException(msq) {}
	const char* name() const override {
		return "InvalidOperation";
	}
};
class InvalidLock : public InvalidOperation {
public:
	InvalidLock(const std::string& msq) : InvalidOperation(msq) {}
	const char* name() const override {
		return "InvalidLock";
	}
};
class InvalidUnlock : public InvalidOperation {
public:
	InvalidUnlock(const std::string& msq) : InvalidOperation(msq) {}
	const char* name() const override {
		return "InvalidUnlock";
	}
};

class NotImplementedException : public AttachARuntimeException {
public:
	NotImplementedException() : AttachARuntimeException("Entered to non implemented region") {}
	const char* name() const override {
		return "AttachARuntimeException";
	}
};
class OutOfRange : public AttachARuntimeException {
public:
	OutOfRange() : AttachARuntimeException("Out of range") {}
	OutOfRange(const std::string& str) : AttachARuntimeException(str) {}
	const char* name() const override {
		return "OutOfRange";
	}
};
class InvalidClassDeclarationException : public AttachARuntimeException {
public:
	InvalidClassDeclarationException() : AttachARuntimeException("Invalid Class Declaration Exception") {}
	const char* name() const override {
		return "InvalidClassDeclarationException";
	}
};
class LibrayNotFoundException : public AttachARuntimeException {
public:
	LibrayNotFoundException() : AttachARuntimeException("Libray not found") {}
	LibrayNotFoundException(const std::string& desc) : AttachARuntimeException(desc) {}
	const char* name() const override {
		return "LibrayNotFoundException";
	}
};
class LibrayFunctionNotFoundException : public AttachARuntimeException {
public:
	LibrayFunctionNotFoundException() : AttachARuntimeException("Libray function not found") {}
	LibrayFunctionNotFoundException(const std::string& desc) : AttachARuntimeException(desc) {}
	const char* name() const override {
		return "LibrayFunctionNotFoundException";
	}
};
class EnviropmentRuinException : public AttachARuntimeException {
public:
	EnviropmentRuinException() : AttachARuntimeException("EnviropmentRuinException") {}
	EnviropmentRuinException(const std::string& desc) : AttachARuntimeException("EnviropmentRuinException: " + desc) {}
	const char* name() const override {
		return "EnviropmentRuinException";
	}
};
class InvalidArchitectureException : public AttachARuntimeException {
public:
	InvalidArchitectureException() : AttachARuntimeException("Invalid archetecture") {}
	const char* name() const override {
		return "InvalidArchitectureException";
	}
};
class StackOverflowException : public AttachARuntimeException {
public:
	StackOverflowException()  {}
	const char* name() const override {
		return "StackOverflowException";
	}
};
class UnusedDebugPointException : public AttachARuntimeException {
public:
	UnusedDebugPointException() : AttachARuntimeException("Unused Debug Breakpoint") {}
	const char* name() const override {
		return "UnusedDebugPointException";
	}
};
class DevideByZeroException : public AttachARuntimeException {
public:
	DevideByZeroException() : AttachARuntimeException("Number devided by zero") {}
	const char* name() const override {
		return "DevideByZeroException";
	}
};
class BadInstructionException : public AttachARuntimeException {
public:
	BadInstructionException() : AttachARuntimeException("Bad Instruction") {}
	const char* name() const override {
		return "BadInstructionException";
	}
};
class NumericOverflowException : public AttachARuntimeException {
public:
	NumericOverflowException() : AttachARuntimeException("Caught numeric overflow") {}
	const char* name() const override {
		return "NumericOverflowException";
	}
};
class NumericUndererflowException : public AttachARuntimeException {
public:
	NumericUndererflowException() : AttachARuntimeException("Caught numeric overflow") {}
	const char* name() const override {
		return "NumericUndererflowException";
	}
};
class SegmentationFaultException : public AttachARuntimeException {
public:
	SegmentationFaultException() : AttachARuntimeException("Thread try get access to non mapped region") {}
	SegmentationFaultException(const char* text) : AttachARuntimeException(text) {}
	SegmentationFaultException(const std::string& text) : AttachARuntimeException(text) {}
	const char* name() const override {
		return "SegmentationFaultException";
	}
};
class NullPointerException : public SegmentationFaultException {
public:
	NullPointerException() : SegmentationFaultException("Thread try get access to null pointer region") {}
	NullPointerException(const char* text) : SegmentationFaultException(text) {}
	NullPointerException(const std::string& text) : SegmentationFaultException(text) {}
	const char* name() const override {
		return "NullPointerException";
	}
};


class AttachedLangException : public AttachARuntimeException {
public:
	AttachedLangException() : AttachARuntimeException("Caught unconvertable external attached langue exception") {}
	const char* name() const override {
		return "AttachedLangException";
	}
};






class CompileTimeException : public AttachARuntimeException {
public:
	CompileTimeException(const std::string& msq) : AttachARuntimeException(msq) {}
	const char* name() const override {
		return "CompileTimeException";
	}
};
class HotPathException : public CompileTimeException {
public:
	HotPathException(const std::string& msq) : CompileTimeException(msq) {}
	const char* name() const override {
		return "HotPathException";
	}
};
class SymbolException : public CompileTimeException {
public:
	SymbolException(const std::string& msq) : CompileTimeException(msq) {}
	const char* name() const override {
		return "SymbolException";
	}
};
class InvalidFunction : public CompileTimeException {
public:
	InvalidFunction(const std::string& msq) : CompileTimeException(msq) {}
	const char* name() const override {
		return "InvalidFunction";
	}
};
class InvalidType : public CompileTimeException {
public:
	InvalidType(const std::string& msq) : CompileTimeException(msq) {}
	const char* name() const override {
		return "InvalidType";
	}
};

class BadOperationException : public CompileTimeException {
public:
	BadOperationException() : CompileTimeException("Bad Operation") {}
	const char* name() const override {
		return "BadOperatinException";
	}
};

class AException : public AttachARuntimeException {
	std::string _name;
public:
	AException(const std::string& ex_name, const std::string& description, void* va = nullptr, size_t ty = 0) : _name(replace_space(ex_name)), AttachARuntimeException(description), v(va), t(ty) {}
	const char* name() const override {
		return _name.c_str();
	}
	void* v;
	size_t t;
};

















//namespace AttachA {
//	enum class ExType {
//		system,
//		processor,
//		compile_time,
//		external,//extern lang except c++
//		own
//	};
//	class Exception {
//		const char* message = "UNDEFINED EXCEPTION";
//		const char* _name = "UNDEFINED";
//		const void* add_data = "UNDEFINED";
//		size_t t_hash = 0;
//		ExType _type;
//	public:
//		Exception() {}
//		Exception(const char* msq) : message(msq) { }
//		Exception(const std::string& ex_name, const std::string& description, const void* v) : _name(ex_name.c_str()), message(description.c_str()), data(v) { t_hash = std::hash<std::string>{}(_name); }
//		const char* what() {
//			return message;
//		}
//		const char* name() {
//			return _name;
//		}
//		Type type() {
//			return _type;
//		}
//		const void* additionalData() {
//			return nullptr;
//		}
//		size_t hash() {
//			return t_hash;
//		}
//	};
//	namespace Ex {
//		
//
//		class ExecuteTimeException : public BaseEx {
//			const char* name;
//		public:
//			ExecuteTimeException(const std::string& ex_name, const std::string& description) : name(ex_name.c_str()), BaseEx(description) { t_hash = std::hash<std::string>{}(name); }
//			const char* name() const override {
//				return name;
//			}
//		};
//	}
//
//
//
//
//}
//
//
//
//
//class NotImplementedException : public AttachARuntimeException {
//public:
//	NotImplementedException() : AttachARuntimeException("Entered to non implemented region") {}
//	const char* name() const override {
//		return "AttachARuntimeException";
//	}
//};
//class OutOfRange : public AttachARuntimeException {
//public:
//	OutOfRange() : AttachARuntimeException("Out of range") {}
//	const char* name() const override {
//		return "OutOfRange";
//	}
//};
//class InvalidClassDeclarationException : public AttachARuntimeException {
//public:
//	InvalidClassDeclarationException() : AttachARuntimeException("Invalid Class Declaration Exception") {}
//	const char* name() const override {
//		return "InvalidClassDeclarationException";
//	}
//};
//class LibrayNotFoundException : public AttachARuntimeException {
//public:
//	LibrayNotFoundException() : AttachARuntimeException("Libray not found") {}
//	LibrayNotFoundException(const std::string& desc) : AttachARuntimeException(desc) {}
//	const char* name() const override {
//		return "LibrayNotFoundException";
//	}
//};
//class LibrayFunctionNotFoundException : public AttachARuntimeException {
//public:
//	LibrayFunctionNotFoundException() : AttachARuntimeException("Libray function not found") {}
//	LibrayFunctionNotFoundException(const std::string& desc) : AttachARuntimeException(desc) {}
//	const char* name() const override {
//		return "LibrayFunctionNotFoundException";
//	}
//};
//class EnviropmentRuinException : public AttachARuntimeException {
//public:
//	EnviropmentRuinException() : AttachARuntimeException() {}
//	const char* name() const override {
//		return "EnviropmentRuinException";
//	}
//};
//class InvalidArchitectureException : public AttachARuntimeException {
//public:
//	InvalidArchitectureException() : AttachARuntimeException("Invalid archetecture") {}
//	const char* name() const override {
//		return "InvalidArchitectureException";
//	}
//};
//class StackOverflowException : public AttachARuntimeException {
//public:
//	StackOverflowException() : AttachARuntimeException("Stack overflow exception") {}
//	const char* name() const override {
//		return "StackOverflowException";
//	}
//}; 
//class UnusedDebugPointException : public AttachARuntimeException {
//public:
//	UnusedDebugPointException() : AttachARuntimeException("Unused Debug Breakpoint") {}
//	const char* name() const override {
//		return "UnusedDebugPointException";
//	}
//}; 
//class DevideByZeroException : public AttachARuntimeException {
//public:
//	DevideByZeroException() : AttachARuntimeException("Number devided by zero") {}
//	const char* name() const override {
//		return "DevideByZeroException";
//	}
//};
//class BadInstructionException : public AttachARuntimeException {
//public:
//	BadInstructionException() : AttachARuntimeException("Bad Instruction") {}
//	const char* name() const override {
//		return "BadInstructionException";
//	}
//};
//class NumericOverflowException : public AttachARuntimeException {
//public:
//	NumericOverflowException() : AttachARuntimeException("Caught numeric overflow") {}
//	const char* name() const override {
//		return "NumericOverflowException";
//	}
//};
//class NumericUndererflowException : public AttachARuntimeException {
//public:
//	NumericUndererflowException() : AttachARuntimeException("Caught numeric overflow") {}
//	const char* name() const override {
//		return "NumericUndererflowException";
//	}
//};
//class SegmentationFaultException : public AttachARuntimeException {
//public:
//	SegmentationFaultException() : AttachARuntimeException("Thread try get access to non mapped region") {}
//	const char* name() const override {
//		return "SegmentationFaultException";
//	}
//};




