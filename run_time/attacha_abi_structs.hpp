// Copyright Danyil Melnytskyi 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once
#include <cstdint>
#include <unordered_map>
#include <chrono>
#include "../library/list_array.hpp"
#include "library/exceptions.hpp"
#include "link_garbage_remover.hpp"
#include "util/enum_helper.hpp"

ENUM_t(Opcode,uint8_t,
	(noting)
	(set)
	(set_saar)
	(remove)
	(sum)
	(minus)
	(div)
	(mul)
	(rest)
	(bit_xor)
	(bit_or)
	(bit_and)
	(bit_not)
	(log_not)
	(compare)
	(jump)
	(arg_set)
	(call)
	(call_self)
	(call_local)
	(call_and_ret)
	(call_self_and_ret)
	(call_local_and_ret)
	(ret)
	(ret_noting)
	(copy)
	(move)
	(arr_op)
	(debug_break)
	(force_debug_break)
	(throw_ex)
	(as)
	(is)
	(store_bool)//store bool from value for if statements, set false if type is noting, numeric types is zero and containers is empty, if another then true
	(load_bool)//used if need save equaluty result, set numeric type as 1 or 0

	(inline_native)
	(call_value_function)
	(call_value_function_and_ret)
	(static_call_value_function)
	(static_call_value_function_and_ret)
	(set_structure_value)
	(get_structure_value)
	(explicit_await)
	(generator_get)
)


ENUM_t( OpcodeArray,uint8_t,
	(set)
	(insert)
	(push_end)
	(push_start)
	(insert_range)

	(get)
	(take)
	(take_end)
	(take_start)
	(get_range)
	(take_range)


	(pop_end)
	(pop_start)
	(remove_item)
	(remove_range)

	(resize)
	(resize_default)


	(reserve_push_end)
	(reserve_push_start)
	(commit)
	(decommit)
	(remove_reserved)
	(size)
)
ENUM_t(ArrCheckMode, uint8_t,
	(no_check)
	(check)
	(no_throw_check)
)
union OpArrFlags {
	struct {
		uint8_t move_mode : 1;
		ArrCheckMode checked : 2;
		uint8_t by_val_mode : 1;
	};
	uint8_t raw;
};

ENUM(JumpCondition,
	(no_condition)
	(is_equal)
	(is_not_equal)
	(is_more)
	(is_lower)
	(is_lower_or_eq)
	(is_more_or_eq)
)

struct Command {
	Command(uint8_t ini) {
		code = (Opcode)(ini & 0x3F);
		is_gc_mode = (ini & 0x40) >> 6;
		static_mode = (ini & 0x80) >> 7;
	}
	Command(Opcode op, bool gc_mode = false, bool set_static_mode = false) {
		code = op;
		is_gc_mode = gc_mode;
		static_mode = set_static_mode;
	}
	Opcode code : 6;
	uint8_t is_gc_mode : 1;
	uint8_t static_mode : 1;

	uint8_t toCmd() {
		uint8_t res = (uint8_t)code;
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
		uint8_t compiletime_constant : 1;//invalid when in_memory true, if true async_mode ignored
		uint8_t : 3;
	};
	uint8_t encoded = 0;
};

struct RFLAGS {
	uint16_t : 1;
	uint16_t nt : 1;
	uint16_t iopl : 1;
	uint16_t overflow : 1;
	uint16_t direction : 1;
	uint16_t ief : 1;
	uint16_t tf : 1;
	uint16_t sign_f : 1;
	uint16_t zero : 1;
	uint16_t : 1;
	uint16_t auxiliary_carry : 1;
	uint16_t : 1;
	uint16_t parity : 1;
	uint16_t : 1;
	uint16_t carry : 1;
	struct off_left {
		static constexpr uint8_t nt = 13;
		static constexpr uint8_t iopl = 12;
		static constexpr uint8_t overflow = 11;
		static constexpr uint8_t direction = 10;
		static constexpr uint8_t ief = 9;
		static constexpr uint8_t tf = 8;
		static constexpr uint8_t sign_f = 7;
		static constexpr uint8_t zero = 6;
		static constexpr uint8_t auxiliary_carry = 4;
		static constexpr uint8_t parity = 2;
		static constexpr uint8_t carry = 0;
	};
	struct bit {
		static constexpr uint16_t nt = 0x2000;
		static constexpr uint16_t iopl = 0x1000;
		static constexpr uint16_t overflow = 0x800;
		static constexpr uint16_t direction = 0x400;
		static constexpr uint16_t ief = 0x200;
		static constexpr uint16_t tf = 0x100;
		static constexpr uint16_t sign_f = 0x80;
		static constexpr uint16_t zero = 0x40;
		static constexpr uint16_t auxiliary_carry = 0x10;
		static constexpr uint16_t parity = 0x4;
		static constexpr uint16_t carry = 0x1;
	};
};

ENUM_t(VType, uint8_t,
	(noting)
	(boolean)
	(i8)
	(i16)
	(i32)
	(i64)
	(ui8)
	(ui16)
	(ui32)
	(ui64)
	(flo)
	(doub)
	(raw_arr_i8)
	(raw_arr_i16)
	(raw_arr_i32)
	(raw_arr_i64)
	(raw_arr_ui8)
	(raw_arr_ui16)
	(raw_arr_ui32)
	(raw_arr_ui64)
	(raw_arr_flo)
	(raw_arr_doub)
	(uarr)
	(string)
	(async_res)
	(undefined_ptr)
	(except_value)//default from except call
	(faarr)//fixed any array
	(saarr)//stack fixed any array //only local, cannont returned, cannont be used with lgr, cannont be passed as arguments
	
	//class, morph, proxy, struct_ is too like types but has diferent memory management
	(class_)//class_ is regular attacha class, values can be dynamicaly defined and undefined at runtime
	(morph)//morph is same as class_ but functions can also be defined at runtime
	(proxy)//proxy is just proxy to another value, and values can be changed by getters and setters, and proxy functions, regulary can be recuived from library
	//(struct_)//struct_ is same as class_ but values placed in one memory block, can be used as C union or C struct, and defined functions can be used as C++ methods

	(type_identifier)
	(function)
	(class_define)//used to construct class_ or morph values,
	(map)//unordered_map<any,any>
	(set)//unordered_set<any>
	(time_point)//std::chrono::steady_clock::time_point
)
union ValueMeta {
	size_t encoded;
	struct {
		VType vtype;
		uint8_t use_gc : 1;
		uint8_t allow_edit : 1;
		uint8_t as_ref : 1;
		uint32_t val_len;
	};

	ValueMeta() = default;
	ValueMeta(const ValueMeta& copy) = default;
	ValueMeta(VType ty, bool gc = false, bool editable = true, uint32_t length = 0, bool as_ref = false) :as_ref(as_ref) { vtype = ty; use_gc = gc; allow_edit = editable; val_len = length; }
	ValueMeta(size_t enc) { encoded = enc; }
};

struct ClassValue;
struct MorphValue;
struct ProxyClass;
struct ValueItem {
	void* val = nullptr;
	ValueMeta meta;
	ValueItem(nullptr_t);
	ValueItem(bool val);
	ValueItem(int8_t val);
	ValueItem(uint8_t val);
	ValueItem(int16_t val);
	ValueItem(uint16_t val);
	ValueItem(int32_t val);
	ValueItem(uint32_t val);
	ValueItem(int64_t val);
	ValueItem(uint64_t val);
	ValueItem(float val);
	ValueItem(double val);
	ValueItem(const std::string& val);
	ValueItem(const char* str);
	ValueItem(const list_array<ValueItem>& val);
	ValueItem(list_array<ValueItem>&& val);
	ValueItem(ValueItem* vals, uint32_t len);
	ValueItem(void* undefined_ptr);

	ValueItem(const int8_t* vals, uint32_t len);
	ValueItem(const uint8_t* vals, uint32_t len);
	ValueItem(const int16_t* vals, uint32_t len);
	ValueItem(const uint16_t* vals, uint32_t len);
	ValueItem(const int32_t* vals, uint32_t len);
	ValueItem(const uint32_t* vals, uint32_t len);
	ValueItem(const int64_t* vals, uint32_t len);
	ValueItem(const uint64_t* vals, uint32_t len);
	ValueItem(const float* vals, uint32_t len);
	ValueItem(const double* vals, uint32_t len);
	template<size_t len>
	ValueItem(ValueItem(&vals)[len]) : ValueItem(vals, len) {}
	ValueItem(typed_lgr<struct Task> task);
	ValueItem(const std::initializer_list<int8_t>& args) : ValueItem(args.begin(), args.size() <= UINT32_MAX ? (uint32_t)args.size() : UINT32_MAX){};
	ValueItem(const std::initializer_list<uint8_t>& args) : ValueItem(args.begin(), args.size() <= UINT32_MAX ? (uint32_t)args.size() : UINT32_MAX){};
	ValueItem(const std::initializer_list<int16_t>& args) : ValueItem(args.begin(), args.size() <= UINT32_MAX ? (uint32_t)args.size() : UINT32_MAX){};
	ValueItem(const std::initializer_list<uint16_t>& args) : ValueItem(args.begin(), args.size() <= UINT32_MAX ? (uint32_t)args.size() : UINT32_MAX){};
	ValueItem(const std::initializer_list<int32_t>& args) : ValueItem(args.begin(), args.size() <= UINT32_MAX ? (uint32_t)args.size() : UINT32_MAX){};
	ValueItem(const std::initializer_list<uint32_t>& args) : ValueItem(args.begin(), args.size() <= UINT32_MAX ? (uint32_t)args.size() : UINT32_MAX){};
	ValueItem(const std::initializer_list<int64_t>& args) : ValueItem(args.begin(), args.size() <= UINT32_MAX ? (uint32_t)args.size() : UINT32_MAX) {};
	ValueItem(const std::initializer_list<uint64_t>& args) : ValueItem(args.begin(), args.size() <= UINT32_MAX ? (uint32_t)args.size() : UINT32_MAX) {};
	ValueItem(const std::initializer_list<float>& args) : ValueItem(args.begin(), args.size() <= UINT32_MAX ? (uint32_t)args.size() : UINT32_MAX) {};
	ValueItem(const std::initializer_list<double>& args) : ValueItem(args.begin(), args.size() <= UINT32_MAX ? (uint32_t)args.size() : UINT32_MAX) {};
	ValueItem(const std::initializer_list<ValueItem>& args);
	ValueItem(const std::exception_ptr&);
	ValueItem(const std::chrono::steady_clock::time_point&);
	ValueItem() {
		val = nullptr;
		meta.encoded = 0;
	}
	ValueItem(ValueItem&& move) noexcept {
		val = move.val;
		meta = move.meta;
		move.val = nullptr;
	}
	ValueItem(const void* vall, ValueMeta meta);
	ValueItem(void* vall, ValueMeta meta, bool no_copy, bool as_ref = false);
	ValueItem(VType);
	ValueItem(ValueMeta);
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


	explicit operator bool();
	explicit operator int8_t();
	explicit operator uint8_t();
	explicit operator int16_t();
	explicit operator uint16_t();
	explicit operator int32_t();
	explicit operator uint32_t();
	explicit operator int64_t();
	explicit operator uint64_t();
	explicit operator float();
	explicit operator double();
	explicit operator void*();
	explicit operator std::string();
	explicit operator list_array<ValueItem>();
	explicit operator ClassValue& ();
	explicit operator MorphValue& ();
	explicit operator ProxyClass& ();
	explicit operator std::exception_ptr();
	explicit operator std::chrono::steady_clock::time_point();

	ValueItem* operator()(ValueItem* arguments, uint32_t arguments_size);
	void getAsync();
	void*& getSourcePtr();
	const void*& getSourcePtr() const;
	typed_lgr<class FuncEnviropment>* funPtr();
};
typedef ValueItem* (*Enviropment)(void** enviro, ValueItem* args, uint32_t len);
typedef ValueItem* (*AttachACXX)(ValueItem* arguments, uint32_t len);




ENUM_t(ClassAccess, uint8_t,
	(pub)//anyone can use
	(priv)//main only
	(prot)//derived or main
	(deriv)//derived only
)
struct ClassFnDefine {
	typed_lgr<class FuncEnviropment> fn = nullptr;
	uint8_t deletable : 1 = true;
	ClassAccess access : 2 = ClassAccess::pub;
};
struct ClassDefine {
	std::unordered_map<std::string, ClassFnDefine> funs;
	std::string name;
	ClassDefine();
	ClassDefine(const std::string& name);
};

struct ClassValDefine {
	ValueItem val;
	ClassAccess access : 2 = ClassAccess::pub;
};
struct ClassValue {
	std::unordered_map<std::string, ClassValDefine> val;
	ClassDefine* define = nullptr;
	typed_lgr<class FuncEnviropment> callFnPtr(const std::string& str, ClassAccess acces);
	ClassFnDefine& getFnMeta(const std::string& str);
	void setFnMeta(const std::string& str, ClassFnDefine& fn_decl);
	bool containsFn(const std::string& str);
	ValueItem& getValue(const std::string& str, ClassAccess acces);
	ValueItem copyValue(const std::string& str, ClassAccess acces);
	bool containsValue(const std::string& str);
};

struct MorphValue {
	std::unordered_map<std::string, ClassValDefine> val;
	ClassDefine define;
	typed_lgr<class FuncEnviropment> callFnPtr(const std::string& str, ClassAccess acces);
	ClassFnDefine& getFnMeta(const std::string& str);
	void setFnMeta(const std::string& str, ClassFnDefine& fn_decl);
	bool containsFn(const std::string& str);
	ValueItem& getValue(const std::string& str, ClassAccess acces);
	ValueItem copyValue(const std::string& str, ClassAccess acces);
	bool containsValue(const std::string& str);
	void setValue(const std::string& str, ClassAccess acces, ValueItem& val);
};



using ProxyClassGetter = ValueItem*(*)(void*);
using ProxyClassSeter = void(*)(void*, ValueItem&);
using ProxyClassDestructor = void(*)(void*);
using ProxyClassCopy = void*(*)(void*);

struct ProxyClassDefine {
	std::unordered_map<std::string, ProxyClassGetter> value_geter;
	std::unordered_map<std::string, ProxyClassSeter> value_seter;
	std::unordered_map<std::string, ClassFnDefine> funs;
	ProxyClassDestructor destructor = nullptr;
	ProxyClassCopy copy = nullptr;
	std::string name;
	ProxyClassDefine();
	ProxyClassDefine(const std::string& name);
};
struct ProxyClass {
	ProxyClassDefine* declare_ty;
	void* class_ptr;
	ProxyClass();
	ProxyClass(void* val);
	ProxyClass(void* val, ProxyClassDefine* def);
	~ProxyClass();

	typed_lgr<class FuncEnviropment> callFnPtr(const std::string& str, ClassAccess acces);
	ClassFnDefine& getFnMeta(const std::string& str);
	void setFnMeta(const std::string& str, ClassFnDefine& fn_decl);
	bool containsFn(const std::string& str);
	ValueItem* getValue(const std::string& str);
	void setValue(const std::string& str, ValueItem& it);
	bool containsValue(const std::string& str);
};


namespace std {
	template<>
	struct hash<ValueItem> {
		size_t operator()(const ValueItem& cit) {
			ValueItem& it = const_cast<ValueItem&>(cit);
			it.getAsync();
			switch (it.meta.vtype) {
			case VType::noting:return 0;
			case VType::type_identifier:
			case VType::boolean:
			case VType::i8:return hash<int8_t>()((int8_t)it);
			case VType::i16:return hash<int16_t>()((int16_t)it);
			case VType::i32:return hash<int32_t>()((int32_t)it);
			case VType::i64:return hash<int64_t>()((int64_t)it);
			case VType::ui8:return hash<uint8_t>()((uint8_t)it);
			case VType::ui16:return hash<uint16_t>()((uint16_t)it);
			case VType::ui32:return hash<uint32_t>()((uint32_t)it);
			case VType::undefined_ptr:
			case VType::ui64:return hash<uint64_t>()((uint64_t)it);
			case VType::flo:return hash<float>()((float)it);
			case VType::doub:return hash<double>()((double)it);
			case VType::string:return hash<string>()((string)it);
			case VType::uarr: return hash<list_array<ValueItem>>()((list_array<ValueItem>)it);
			case VType::raw_arr_i8:
			case VType::raw_arr_i16:
			case VType::raw_arr_i32:
			case VType::raw_arr_i64:
			case VType::raw_arr_ui8:
			case VType::raw_arr_ui16:
			case VType::raw_arr_ui32:
			case VType::raw_arr_ui64:
			case VType::raw_arr_flo:
			case VType::raw_arr_doub:
			case VType::async_res:
			case VType::except_value:
			case VType::faarr:
			case VType::class_:
			case VType::morph:
			case VType::proxy:
			default:
				//TO-DO add impl
				throw NotImplementedException();
				break;
			}
		}
	};
}
