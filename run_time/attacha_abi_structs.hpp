// Copyright Danyil Melnytskyi 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once
#include <cstdint>
#include <unordered_map>
#include "../library/list_array.hpp"
enum class Opcode : uint8_t {
	noting,
	set,
	remove,
	sum,
	minus,
	div,
	mul,
	bit_xor,
	bit_or,
	bit_and,
	bit_not,
	log_not,
	compare,
	jump,
	arg_set,
	call,
	call_self,
	call_local,
	call_and_ret,
	call_self_and_ret,
	call_local_and_ret,
	ret,
	ret_noting,
	copy,
	move,
	arr_op,
	debug_break,
	force_debug_break,
	throw_ex,
	as,
	is,
	store_bool,//store bool from value for if statements, set false if type is noting, numeric types is zero and containers is empty, if another then true
	load_bool//used if need save equaluty result, set numeric type as 1 or 0
};
enum class JumpCondition {
	no_condition,
	is_equal,
	is_not_equal,
	is_more,
	is_lower,
	is_lower_or_eq,
	is_more_or_eq
};
struct Command {
	Command(uint8_t ini) {
		code = (Opcode)(ini & 0x3F);
		is_gc_mode = ini & 0x40;
		static_mode = ini & 0x80;
	}
	Command(Opcode op, bool gc_mode = false, bool static_mode = false) {
		code = op;
		is_gc_mode = gc_mode;
		static_mode = static_mode;
	}
	Opcode code : 6;
	uint8_t is_gc_mode : 1;
	uint8_t static_mode : 1;

	uint8_t toCmd() {
		uint8_t res = (uint8_t)code;
		res |= is_gc_mode << 5;
		res |= is_gc_mode << 6;
		res |= static_mode << 7;
		return res;
	}
};

union CallFlags {
	struct {
		uint8_t in_memory : 1;
		uint8_t async_mode : 1;
		uint8_t use_result : 1;
		uint8_t except_catch : 1;
		uint8_t unused : 4;
	};
	uint8_t encoded = 0;
};


struct RFLAGS {
	uint16_t unused_000000 : 1;
	uint16_t nt : 1;
	uint16_t iopl : 1;
	uint16_t overflow : 1;
	uint16_t direction : 1;
	uint16_t ief : 1;
	uint16_t tf : 1;
	uint16_t sign_f : 1;
	uint16_t zero : 1;
	uint16_t unused_000001 : 1;
	uint16_t auxiliary_carry : 1;
	uint16_t unused_000002 : 1;
	uint16_t parity : 1;
	uint16_t unused_000003 : 1;
	uint16_t carry : 1;
};

enum class VType : uint8_t {
	noting,
	i8,
	i16,
	i32,
	i64,
	ui8,
	ui16,
	ui32,
	ui64,
	flo,
	doub,
	raw_arr_i8,
	raw_arr_i16,
	raw_arr_i32,
	raw_arr_i64,
	raw_arr_ui8,
	raw_arr_ui16,
	raw_arr_ui32,
	raw_arr_ui64,
	raw_arr_flo,
	raw_arr_doub,
	uarr,
	string,
	async_res,
	undefined_ptr,
	except_value,//default from except call

	// sstructure// ValueItem[] (allocated in stack)
	// hstructure// ValueItem[] (allocated in heap)
	// 
	// class {define ptr, [values]}
	// 
	// morph {[funcs], [values]}
	// proxy {define ptr, {value ptr}}
	// 
};
union ValueMeta {
	size_t encoded;
	struct {
		VType vtype;
		uint8_t use_gc : 1;
		uint8_t allow_edit : 1;
		uint32_t val_len;
	};

	ValueMeta() = default;
	ValueMeta(const ValueMeta& copy) = default;
	ValueMeta(VType ty, bool gc, bool editable, uint32_t length = 0) { vtype = ty; use_gc = gc; allow_edit = editable; val_len = length; }
	ValueMeta(size_t enc) { encoded = enc; }
};

struct ValueItem {
	void* val;
	ValueMeta meta;
	ValueItem() {
		val = nullptr;
		meta.encoded = 0;
	}
	ValueItem(ValueItem&& move) noexcept {
		val = move.val;
		meta = move.meta;
		move.val = nullptr;
	}
	ValueItem(void* vall, ValueMeta meta);
	ValueItem(void* vall, ValueMeta meta, bool no_copy);
	ValueItem(const ValueItem&);
	ValueItem& operator=(const ValueItem& copy);
	ValueItem& operator=(ValueItem&& copy) noexcept;
	~ValueItem();
	bool operator<(const ValueItem& cmp) const;
	bool operator>(const ValueItem& cmp) const;
	bool operator==(const ValueItem& cmp) const;
	bool operator!=(const ValueItem& cmp) const;
	bool operator>=(const ValueItem& cmp) const;
	bool operator<=(const ValueItem& cmp) const;
	ValueItem& operator +=(const ValueItem& op);
	ValueItem& operator -=(const ValueItem& op);
	ValueItem& operator *=(const ValueItem& op);
	ValueItem& operator /=(const ValueItem& op);
	ValueItem& operator %=(const ValueItem& op);
	ValueItem& operator ^=(const ValueItem& op);
	ValueItem& operator &=(const ValueItem& op);
	ValueItem& operator |=(const ValueItem& op);
	ValueItem& operator !();

	ValueItem operator +(const ValueItem& op) const;
	ValueItem operator -(const ValueItem& op) const;
	ValueItem operator *(const ValueItem& op) const;
	ValueItem operator /(const ValueItem& op) const;
	ValueItem operator ^(const ValueItem& op) const;
	ValueItem operator &(const ValueItem& op) const;
	ValueItem operator |(const ValueItem& op) const;
};

typedef ValueItem* (*Enviropment)(void** enviro, list_array<ValueItem>* args);
typedef ValueItem* (*AttachACXX)(list_array<ValueItem>* arguments);






enum class ClassAccess : uint8_t {
	pub,//anyone can use
	priv,//main only
	prot,//derived or main
	deriv//derived only
};
struct ClassFnDefine {
	typed_lgr <class FuncEnviropment > fn = nullptr;
	uint8_t deletable : 1 = true;
	ClassAccess access : 2 = ClassAccess::pub;
};
struct ClassValDefine {
	ValueItem val;
	ClassAccess access : 2 = ClassAccess::pub;
};
struct ClassValue {
	std::unordered_map<std::string, ClassValDefine> val;
	std::unordered_map<std::string, ClassFnDefine>* funs;
	typed_lgr<class FuncEnviropment> callFnPtr(const std::string& str, ClassAccess acces) {
		if (funs) {
			if (funs->contains(str)) {
				auto& tmp = funs->operator[](str);
				switch (acces) {
				case ClassAccess::pub:
					if (tmp.access == ClassAccess::pub)
						return tmp.fn;
					break;
				case ClassAccess::priv:
					if (tmp.access != ClassAccess::deriv)
						return tmp.fn;
					break;
				case ClassAccess::prot:
					if (tmp.access == ClassAccess::pub || tmp.access == ClassAccess::prot)
						return tmp.fn;
					break;
				case ClassAccess::deriv:
					if (tmp.access != ClassAccess::priv)
						return tmp.fn;
					break;
				default:
					throw NotImplementedException();
				}
				throw InvalidFunction("Try access to private function");
			}
		}
		throw NotImplementedException();
	}
	auto* getFnMeta(const std::string& str) {
		if (funs) {
			if (funs->contains(str))
				return &funs->operator[](str);
		}
		throw NotImplementedException();
	}
	ValueItem& getValue(const std::string& str, ClassAccess acces) {
		if (val.contains(str)) {
			auto& tmp = val[str];
			switch (acces) {
			case ClassAccess::pub:
				if (tmp.access == ClassAccess::pub)
					return tmp.val;
				break;
			case ClassAccess::priv:
				if (tmp.access != ClassAccess::deriv)
					return tmp.val;
				break;
			case ClassAccess::prot:
				if (tmp.access == ClassAccess::pub || tmp.access == ClassAccess::prot)
					return tmp.val;
				break;
			case ClassAccess::deriv:
				if (tmp.access != ClassAccess::priv)
					return tmp.val;
				break;
			default:
				throw NotImplementedException();
			}
			throw InvalidFunction("Try access to non public value");
		}
		throw NotImplementedException();
	}
	ValueItem copyValue(const std::string& str, ClassAccess acces) {
		if (val.contains(str)) {
			auto& tmp = val[str];
			switch (acces) {
			case ClassAccess::pub:
				if (tmp.access == ClassAccess::pub)
					return tmp.val;
				break;
			case ClassAccess::priv:
				if (tmp.access != ClassAccess::deriv)
					return tmp.val;
				break;
			case ClassAccess::prot:
				if (tmp.access == ClassAccess::pub || tmp.access == ClassAccess::prot)
					return tmp.val;
				break;
			case ClassAccess::deriv:
				if (tmp.access != ClassAccess::priv)
					return tmp.val;
				break;
			default:
				throw NotImplementedException();
			}
			throw InvalidFunction("Try access to non public value");
		}
		return ValueItem();
	}
};

struct MorphValue {
	std::unordered_map<std::string, ClassValDefine> val;
	std::unordered_map<std::string, ClassFnDefine>* funs;
	ClassFnDefine funcs_def;
	typed_lgr<class FuncEnviropment> callFnPtr(const std::string& str, ClassAccess acces) {
		if (funs->contains(str)) {
			auto& tmp = funs->operator[](str);
			switch (acces) {
			case ClassAccess::pub:
				if (tmp.access == ClassAccess::pub)
					return tmp.fn;
				break;
			case ClassAccess::priv:
				if (tmp.access != ClassAccess::deriv)
					return tmp.fn;
				break;
			case ClassAccess::prot:
				if (tmp.access == ClassAccess::pub || tmp.access == ClassAccess::prot)
					return tmp.fn;
				break;
			case ClassAccess::deriv:
				if (tmp.access != ClassAccess::priv)
					return tmp.fn;
				break;
			default:
				throw NotImplementedException();
			}
			throw InvalidFunction("Try access to private function");
		}
		throw NotImplementedException();
	}
	auto* getFnMeta(const std::string& str) {
		if (funs->contains(str))
			return &funs->operator[](str);
		throw NotImplementedException();
	}
	ValueItem& getValue(const std::string& str, ClassAccess acces) {
		if (val.contains(str)) {
			auto& tmp = val[str];
			switch (acces) {
			case ClassAccess::pub:
				if (tmp.access == ClassAccess::pub)
					return tmp.val;
				break;
			case ClassAccess::priv:
				if (tmp.access != ClassAccess::deriv)
					return tmp.val;
				break;
			case ClassAccess::prot:
				if (tmp.access == ClassAccess::pub || tmp.access == ClassAccess::prot)
					return tmp.val;
				break;
			case ClassAccess::deriv:
				if (tmp.access != ClassAccess::priv)
					return tmp.val;
				break;
			default:
				throw NotImplementedException();
			}
			throw InvalidFunction("Try access to non public value");
		}
		throw NotImplementedException();
	}
	ValueItem copyValue(const std::string& str, ClassAccess acces) {
		if (val.contains(str)) {
			auto& tmp = val[str];
			switch (acces) {
			case ClassAccess::pub:
				if (tmp.access == ClassAccess::pub)
					return tmp.val;
				break;
			case ClassAccess::priv:
				if (tmp.access != ClassAccess::deriv)
					return tmp.val;
				break;
			case ClassAccess::prot:
				if (tmp.access == ClassAccess::pub || tmp.access == ClassAccess::prot)
					return tmp.val;
				break;
			case ClassAccess::deriv:
				if (tmp.access != ClassAccess::priv)
					return tmp.val;
				break;
			default:
				throw NotImplementedException();
			}
			throw InvalidFunction("Try access to non public value");
		}
		return ValueItem();
	}
};



using ProxyClassGetter = ValueItem(*)(void*);
using ProxyClassSeter = void(*)(void*, ValueItem&);
using ProxyClassDestructor = void(*)(void*);

struct ProxyClassDeclare {
	std::unordered_map<std::string, ProxyClassGetter> value_geter;
	std::unordered_map<std::string, ProxyClassSeter> value_seter;
	std::unordered_map<std::string, class FuncEnviropment*> public_fun;
	ProxyClassDestructor destructor;
};
struct ProxyClassValueLgrItem {
	ProxyClassDeclare* declare_ty = nullptr;
	void* class_ptr = nullptr;
};