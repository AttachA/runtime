// Copyright Danyil Melnytskyi 2022-2023
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once
#ifndef RUN_TIME_ATTACHA_ABI_STRUCTS
#define RUN_TIME_ATTACHA_ABI_STRUCTS

#include <cstdint>
#include <cassert>
#include <unordered_map>
#include <unordered_set>
#include <chrono>
#include <exception>
#include "../library/list_array.hpp"
#include "library/exceptions.hpp"
#include "link_garbage_remover.hpp"
#include "util/enum_helper.hpp"

ENUM_t(Opcode,uint8_t,
	(noting)
	(set)
	(set_saarr)
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
	(bit_shift_left)
	(bit_shift_right)
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
	(ret_take)
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
	(call_value_function_id)
	(call_value_function_and_ret)
	(call_value_function_id_and_ret)
	(static_call_value_function)
	(static_call_value_function_and_ret)
	(static_call_value_function_id)
	(static_call_value_function_id_and_ret)
	(set_structure_value)
	(get_structure_value)
	(explicit_await)
	(generator_get)//get value from generator or async task
	(yield)

	(handle_begin)
	(handle_catch)
	(handle_finally)
	(handle_convert)

	(value_hold)
	(value_unhold)
	(is_gc)
	(to_gc)
	(localize_gc)
	(from_gc)
	(table_jump)
	(xarray_slice)//farray and sarray slice by creating reference to original array with moved pointer and new size
	(store_constant)
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
ENUM_t(TableJumpCheckFailAction, uint8_t,
	(jump_specified)
	(throw_exception)
	(unchecked)
)

union OpArrFlags {
	struct {
		uint8_t move_mode : 1;
		ArrCheckMode checked : 2;
		uint8_t by_val_mode : 1;
	};
	uint8_t raw;
};
union TableJumpFlags {
	struct {
		uint8_t is_signed : 1;
		TableJumpCheckFailAction too_large : 2;
		TableJumpCheckFailAction too_small : 2;
	};
	uint8_t raw;
};

ENUM_ta(JumpCondition, uint8_t,
	(no_condition)
	(is_equal)
	(is_not_equal)

	(is_unsigned_more)
	(is_unsigned_lower)
	(is_unsigned_lower_or_eq)
	(is_unsigned_more_or_eq)

	(is_signed_more)
	(is_signed_lower)
	(is_signed_lower_or_eq)
	(is_signed_more_or_eq)

	(is_zero)
	,
	(is_more = is_unsigned_more)
	(is_lower = is_unsigned_lower)
	(is_lower_or_eq = is_unsigned_lower_or_eq)
	(is_more_or_eq = is_unsigned_more_or_eq)
)

struct Command {
	Command(){
		code = Opcode::noting;
		is_gc_mode = false;
		static_mode = false;
	}
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
		uint8_t : 5;
	};
	uint8_t encoded = 0;
};

struct RFLAGS {
	uint16_t carry : 1;
	uint16_t : 1;
	uint16_t parity : 1;
	uint16_t : 1;
	uint16_t auxiliary_carry : 1;
	uint16_t : 1;
	uint16_t zero : 1;
	uint16_t sign_f : 1;
	uint16_t tf : 1;
	uint16_t ief : 1;
	uint16_t direction : 1;
	uint16_t overflow : 1;
	uint16_t iopl : 1;
	uint16_t nt : 1;
	uint16_t : 1;
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
	
	(struct_)//like c++ class, but with dynamic abilities

	(type_identifier)
	(function)
	(map)//unordered_map<any,any>
	(set)//unordered_set<any>
	(time_point)//std::chrono::steady_clock::time_point


	(generator)//holds function context
)

ENUM_t(ValuePos, uint8_t,
	(in_enviro)
	(in_arguments)
	(in_static)
	(in_constants)
)
struct ValueIndexPos{
	uint16_t index;
	ValuePos pos = ValuePos::in_enviro;

	bool operator==(const ValueIndexPos& compare){
		return index == compare.index && pos == compare.pos;
	}
	bool operator!=(const ValueIndexPos& compare){
		return index != compare.index || pos != compare.pos;
	}
};
inline ValueIndexPos operator""_env(unsigned long long index){
	assert(index <= UINT16_MAX);
	return ValueIndexPos(index,ValuePos::in_enviro);
}
inline ValueIndexPos operator""_arg(unsigned long long index){
	assert(index <= UINT16_MAX);
	return ValueIndexPos(index,ValuePos::in_arguments);
}
inline ValueIndexPos operator""_sta(unsigned long long index){
	assert(index <= UINT16_MAX);
	return ValueIndexPos(index,ValuePos::in_static);
}
inline ValueIndexPos operator""_con(unsigned long long index){
	assert(index <= UINT16_MAX);
	return ValueIndexPos(index,ValuePos::in_constants);
}
struct FunctionMetaFlags{
	uint64_t length;//length including meta
	struct {
		bool vec128_0 : 1;
		bool vec128_1 : 1;
		bool vec128_2 : 1;
		bool vec128_3 : 1;
		bool vec128_4 : 1;
		bool vec128_5 : 1;
		bool vec128_6 : 1;
		bool vec128_7 : 1;
		bool vec128_8 : 1;
		bool vec128_9 : 1;
		bool vec128_10 : 1;
		bool vec128_11 : 1;
		bool vec128_12 : 1;
		bool vec128_13 : 1;
		bool vec128_14 : 1;
		bool vec128_15 : 1;
	} used_vec;
	bool can_be_unloaded : 1;
	bool is_translated : 1;//function that returns another function, used to implement generics, lambdas or dynamic functions
	bool has_local_functions : 1;
	bool has_debug_info : 1;
	bool is_cheap : 1;
	bool used_enviro_vals : 1;
	bool used_arguments : 1;
	bool used_static : 1;
	bool in_debug : 1;
	bool run_time_computable : 1;//in files always false
	//11bits left
};

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
	ValueMeta(VType ty, bool gc = false, bool editable = true, uint32_t length = 0, bool as_ref = false):encoded(0){ vtype = ty; use_gc = gc; allow_edit = editable; val_len = length; as_ref = as_ref; }
	ValueMeta(size_t enc) { encoded = enc; }
};
class Structure;

struct as_refrence_t {};
constexpr inline as_refrence_t as_refrence = {};

struct no_copy_t {};
constexpr inline no_copy_t no_copy = {};
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
	ValueItem(std::string&& val);
	ValueItem(const char* str);
	ValueItem(const list_array<ValueItem>& val);
	ValueItem(list_array<ValueItem>&& val);
	ValueItem(ValueItem* vals, uint32_t len);
	ValueItem(ValueItem* vals, uint32_t len, no_copy_t);
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
	
	ValueItem(int8_t* vals, uint32_t len, no_copy_t);
	ValueItem(uint8_t* vals, uint32_t len, no_copy_t);
	ValueItem(int16_t* vals, uint32_t len, no_copy_t);
	ValueItem(uint16_t* vals, uint32_t len, no_copy_t);
	ValueItem(int32_t* vals, uint32_t len, no_copy_t);
	ValueItem(uint32_t* vals, uint32_t len, no_copy_t);
	ValueItem(int64_t* vals, uint32_t len, no_copy_t);
	ValueItem(uint64_t* vals, uint32_t len, no_copy_t);
	ValueItem(float* vals, uint32_t len, no_copy_t);
	ValueItem(double* vals, uint32_t len, no_copy_t);
	ValueItem(class Structure*, no_copy_t);
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
	ValueItem(const std::unordered_map<ValueItem, ValueItem>& map);
	ValueItem(std::unordered_map<ValueItem, ValueItem>&& map);
	ValueItem(const std::unordered_set<ValueItem>& set);
	ValueItem(std::unordered_set<ValueItem>&& set);



	ValueItem(const typed_lgr<class FuncEnvironment>&);
	ValueItem() {
		val = nullptr;
		meta.encoded = 0;
	}
	ValueItem(ValueItem&& move);
	ValueItem(const void* vall, ValueMeta meta);
	ValueItem(void* vall, ValueMeta meta, as_refrence_t);
	ValueItem(void* vall, ValueMeta meta, no_copy_t);
	ValueItem(VType);
	ValueItem(ValueMeta);
	ValueItem(const ValueItem&);

	ValueItem(ValueItem& ref, as_refrence_t);
	ValueItem(bool& val, as_refrence_t);
	ValueItem(int8_t& val, as_refrence_t);
	ValueItem(uint8_t& val, as_refrence_t);
	ValueItem(int16_t& val, as_refrence_t);
	ValueItem(uint16_t& val, as_refrence_t);
	ValueItem(int32_t& val, as_refrence_t);
	ValueItem(uint32_t& val, as_refrence_t);
	ValueItem(int64_t& val, as_refrence_t);
	ValueItem(uint64_t& val, as_refrence_t);
	ValueItem(float& val, as_refrence_t);
	ValueItem(double& val, as_refrence_t);
	ValueItem(class Structure*, as_refrence_t);
	ValueItem(std::string& val, as_refrence_t);
	ValueItem(list_array<ValueItem>& val, as_refrence_t);

	ValueItem(std::exception_ptr&, as_refrence_t);
	ValueItem(std::chrono::steady_clock::time_point&, as_refrence_t);
	ValueItem(std::unordered_map<ValueItem, ValueItem>&, as_refrence_t);
	ValueItem(std::unordered_set<ValueItem>&, as_refrence_t);
	ValueItem(typed_lgr<struct Task>& task, as_refrence_t);
	ValueItem(ValueMeta&, as_refrence_t);
	ValueItem(typed_lgr<class FuncEnvironment>&, as_refrence_t);


	ValueItem& operator=(const ValueItem& copy);
	ValueItem& operator=(ValueItem&& copy);
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
	ValueItem& operator <<=(const ValueItem& op);
	ValueItem& operator >>=(const ValueItem& op);
	ValueItem& operator ++();
	ValueItem& operator --();
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
	explicit operator ValueMeta();
	explicit operator std::exception_ptr();
	explicit operator std::chrono::steady_clock::time_point();
	explicit operator Structure& ();
	explicit operator std::unordered_map<ValueItem, ValueItem>&();
	explicit operator std::unordered_set<ValueItem>&();
	explicit operator typed_lgr<struct Task>&();
	explicit operator typed_lgr<class FuncEnvironment>&();

	ValueItem* operator()(ValueItem* arguments, uint32_t arguments_size);
	void getAsync();
	void getGeneratorResult(ValueItem* res, uint64_t result_id);
	void*& getSourcePtr();
	const void*& getSourcePtr() const;
	typed_lgr<class FuncEnvironment>* funPtr();
	void make_gc();
	void localize_gc();
	void ungc();
	bool is_gc();
	
	size_t hash() const;
	size_t hash();
	ValueItem make_slice(uint32_t start, uint32_t end) const;
};
namespace std {
	template<>
	struct hash<ValueItem> {
		size_t operator()(const ValueItem& cit) const {
			return cit.hash();
		}
	};
}
typedef ValueItem* (*Enviropment)(ValueItem* args, uint32_t len);


ENUM_t(ClassAccess, uint8_t,
	(pub)//anyone can use
	(priv)//main only
	(prot)//derived or main
	(intern)//internal, derived or main
)

struct StructureTag {
	std::string name;
	ValueItem value;
};

using MethodTag = StructureTag;

struct MethodInfo{
	struct Optional{
		list_array<ValueMeta> return_values;
		list_array<list_array<ValueMeta>> arguments;
		list_array<StructureTag> tags;
	};
	typed_lgr<class FuncEnvironment> ref;
	std::string name;
	std::string owner_name;
	Optional* optional;
	ClassAccess access : 2;
	bool deletable : 1;
	MethodInfo() : ref(nullptr), name(), owner_name(), optional(nullptr), access(ClassAccess::pub), deletable(true) {}
	MethodInfo(const std::string& name, Enviropment method, ClassAccess access, const list_array<ValueMeta>& return_values, const list_array<list_array<ValueMeta>>& arguments, const list_array<MethodTag>& tags, const std::string& owner_name);
	MethodInfo(const std::string& name, typed_lgr<class FuncEnvironment> method, ClassAccess access, const list_array<ValueMeta>& return_values, const list_array<list_array<ValueMeta>>& arguments, const list_array<MethodTag>& tags, const std::string& owner_name);

	~MethodInfo();
	MethodInfo(const MethodInfo& copy);
	MethodInfo(MethodInfo&& move);
	MethodInfo& operator=(const MethodInfo& copy);
	MethodInfo& operator=(MethodInfo&& move);
};

struct AttachAVirtualTable {
	Enviropment destructor;//args: Structure* structure
	Enviropment copy;//args: Structure* dst, Structure* src, bool at_construct
	Enviropment move;//args: Structure* dst, Structure* src, bool at_construct
	Enviropment compare;//args: Structure* first, Structure* second, return: -1 if first < second, 0 if first == second, 1 if first > second
	uint64_t table_size;
	char data[];

	//{
	//  Enviropment[table_size] table;
	//  MethodInfo [table_size] table_additional_info;
	//	typed_lgr<class FuncEnvironment> holder_destructor;
	//	typed_lgr<class FuncEnvironment> holder_copy;
	//	typed_lgr<class FuncEnvironment> holder_move;
	//	typed_lgr<class FuncEnvironment> holder_compare;
	//  std::string name;
	//	list_array<StructureTag>* tags;//can be null
	//}
	list_array<StructureTag>* getStructureTags();
	list_array<MethodTag>* getMethodTags(uint64_t index);
	list_array<MethodTag>* getMethodTags(const std::string& name, ClassAccess access);

	list_array<list_array<ValueMeta>>* getMethodArguments(uint64_t index);
	list_array<list_array<ValueMeta>>* getMethodArguments(const std::string& name, ClassAccess access);

	list_array<ValueMeta>* getMethodReturnValues(uint64_t index);
	list_array<ValueMeta>* getMethodReturnValues(const std::string& name, ClassAccess access);

	MethodInfo* getMethodsInfo(uint64_t& size);
	MethodInfo& getMethodInfo(uint64_t index);
	MethodInfo& getMethodInfo(const std::string& name, ClassAccess access);

	Enviropment* getMethods(uint64_t& size);
	Enviropment getMethod(uint64_t index);
	Enviropment getMethod(const std::string& name, ClassAccess access);

	uint64_t getMethodIndex(const std::string& name, ClassAccess access);
	bool hasMethod(const std::string& name, ClassAccess access);

	static AttachAVirtualTable* create(list_array<MethodInfo>& methods, typed_lgr<class FuncEnvironment> destructor, typed_lgr<class FuncEnvironment> copy, typed_lgr<class FuncEnvironment> move, typed_lgr<class FuncEnvironment> compare);
	static void destroy(AttachAVirtualTable* table);

	std::string getName();
	void setName(const std::string& name);
private:
	struct AfterMethods{
		typed_lgr<class FuncEnvironment> destructor;
		typed_lgr<class FuncEnvironment> copy;
		typed_lgr<class FuncEnvironment> move;
		typed_lgr<class FuncEnvironment> compare;
		std::string name;
		list_array<StructureTag>* tags;
	};
	AfterMethods* getAfterMethods();
	AttachAVirtualTable(list_array<MethodInfo>& methods, typed_lgr<class FuncEnvironment> destructor, typed_lgr<class FuncEnvironment> copy, typed_lgr<class FuncEnvironment> move, typed_lgr<class FuncEnvironment> compare);
	~AttachAVirtualTable();
};
struct AttachADynamicVirtualTable {
	typed_lgr<class FuncEnvironment> destructor;//args: Structure* structure
	typed_lgr<class FuncEnvironment> copy;//args: Structure* dst, Structure* src, bool at_construct
	typed_lgr<class FuncEnvironment> move;//args: Structure* dst, Structure* src, bool at_construct
	typed_lgr<class FuncEnvironment> compare;//args: Structure* first, Structure* second, return: -1 if first < second, 0 if first == second, 1 if first > second
	list_array<MethodInfo> methods;
	list_array<StructureTag>* tags;
	std::string name;
	AttachADynamicVirtualTable(list_array<MethodInfo>& methods, typed_lgr<class FuncEnvironment> destructor, typed_lgr<class FuncEnvironment> copy, typed_lgr<class FuncEnvironment> move,typed_lgr<class FuncEnvironment> compare);
	~AttachADynamicVirtualTable();
	AttachADynamicVirtualTable(const AttachADynamicVirtualTable&);
	list_array<StructureTag>* getStructureTags();
	list_array<MethodTag>* getMethodTags(uint64_t index);
	list_array<MethodTag>* getMethodTags(const std::string& name, ClassAccess access);

	list_array<list_array<ValueMeta>>* getMethodArguments(uint64_t index);
	list_array<list_array<ValueMeta>>* getMethodArguments(const std::string& name, ClassAccess access);

	list_array<ValueMeta>* getMethodReturnValues(uint64_t index);
	list_array<ValueMeta>* getMethodReturnValues(const std::string& name, ClassAccess access);

	MethodInfo* getMethodsInfo(uint64_t& size);
	MethodInfo& getMethodInfo(uint64_t index);
	MethodInfo& getMethodInfo(const std::string& name, ClassAccess access);

	Enviropment* getMethods(uint64_t& size);
	Enviropment getMethod(uint64_t index);
	Enviropment getMethod(const std::string& name, ClassAccess access);

	void addMethod(const std::string& name, Enviropment method, ClassAccess access, const list_array<ValueMeta>& return_values, const list_array<list_array<ValueMeta>>& arguments, const list_array<MethodTag>& tags, const std::string& owner_name);
	void addMethod(const std::string& name, const typed_lgr<FuncEnvironment>& method, ClassAccess access, const list_array<ValueMeta>& return_values, const list_array<list_array<ValueMeta>>& arguments, const list_array<MethodTag>& tags, const std::string& owner_name);

	void removeMethod(const std::string& name, ClassAccess access);

	void addTag(const std::string& name, const ValueItem& value);
	void addTag(const std::string& name, ValueItem&& value);
	void removeTag(const std::string& name);

	uint64_t getMethodIndex(const std::string& name, ClassAccess access);
	bool hasMethod(const std::string& name, ClassAccess access);

	void derive(AttachADynamicVirtualTable& parent);
	void derive(AttachAVirtualTable& parent);
};

//static values can be implemented by builder, allocate somewhere in memory and put refrences to functions, not structure 
class Structure{
public:
	//return true if allowed
	static bool checkAccess(ClassAccess access, ClassAccess access_to_check);
	struct Item{
		std::string name;
		size_t offset;
		ValueMeta type;
		uint16_t bit_used;
		uint8_t bit_offset : 7;
		bool inlined:1;
	};
	enum class VTableMode : uint8_t{
		disabled = 0,
		AttachAVirtualTable = 1,
		AttachADynamicVirtualTable = 2,//destructor will delete the vtable
		CXX = 3
	};

	static AttachAVirtualTable* createAAVTable(list_array<MethodInfo>& methods, typed_lgr<class FuncEnvironment> destructor, typed_lgr<class FuncEnvironment> copy, typed_lgr<class FuncEnvironment> move, typed_lgr<class FuncEnvironment> compare,const list_array<std::tuple<void*,VTableMode>>& derive_vtables);
	static AttachADynamicVirtualTable* createAADVTable(list_array<MethodInfo>& methods, typed_lgr<class FuncEnvironment> destructor, typed_lgr<class FuncEnvironment> copy, typed_lgr<class FuncEnvironment> move, typed_lgr<class FuncEnvironment> compare,const list_array<std::tuple<void*,VTableMode>>& derive_vtables);
	static void destroyVTable(void* table, VTableMode mode);
private:
	size_t struct_size;//vtable + sizeof(structure)
	VTableMode vtable_mode : 2 = VTableMode::disabled;
public:
	size_t fully_constructed : 1 = false;
private:
	size_t count : 61 = 0;
	char raw_data[];//Item[count], char data[struct_size]



	Item* getPtr(const std::string& name);
	Item* getPtr(size_t index);
	template<typename T>
	ValueItem getRawArray(Item* item) {
		if(item->inlined){
			if(item->type.as_ref)
				return ValueItem((T*)&static_value_get_ref<T*>(item->offset, 0, 0), item->type.val_len, as_refrence);
			else
				return ValueItem((T*)&static_value_get_ref<T*>(item->offset, 0, 0), item->type.val_len);
		}else{
			if(item->type.as_ref)
				return ValueItem(static_value_get<T*>(item->offset, 0, item->bit_offset), item->type.val_len, as_refrence);
			else
				return ValueItem(static_value_get<T*>(item->offset, 0, item->bit_offset), item->type.val_len);
		}
	}
	template<typename T>
	ValueItem getType(Item* item){
		if(item->type.as_ref)
			return ValueItem(static_value_get_ref<T>(item->offset, 0, 0), as_refrence);
		else
			return ValueItem(static_value_get_ref<T>(item->offset, 0, 0));
	}
	template<typename T>
	ValueItem getRawArrayRef(Item* item) {
		if(item->inlined)
			return ValueItem((T*)&static_value_get_ref<T*>(item->offset, 0, 0), item->type.val_len, as_refrence);
		else
			return ValueItem(static_value_get<T*>(item->offset, 0, item->bit_offset), item->type.val_len, as_refrence);
	}
	template<typename T>
	ValueItem getTypeRef(Item* item){
		return ValueItem(static_value_get_ref<T>(item->offset, 0, 0), as_refrence);
	}

	ValueItem _static_value_get(Item* item);
	ValueItem _static_value_get_ref(Item* item);
	void _static_value_set(Item* item, ValueItem& set);
	Structure(size_t structure_size, Item* items, size_t count, void* vtable,VTableMode table_mode );
	~Structure() noexcept(false);
public:
	
	template<typename T>
	T static_value_get(size_t offset, uint16_t bit_used, uint8_t bit_offset) {
		if(sizeof(T) * 8 < bit_used && bit_used)
			throw InvalidArguments("bit_used is too big for type");
		
		char* ptr = raw_data + count * sizeof(Item);
		ptr += offset;
		ptr += bit_offset / 8;
		
		if((bit_used / 8 == sizeof(T) && bit_used) || bit_offset % sizeof(T) == 0)
			return *(T*)ptr;
		uint8_t bit_offset2 = bit_offset % 8;
		
		uint16_t used_bytes = bit_used ? bit_used / 8 : sizeof(T);
		uint8_t used_bits = bit_used ? bit_used % 8 : 0;
		
		char buffer[sizeof(T)]{0};
		for(uint8_t i = 0; i < used_bytes-1; i++)
			buffer[i] = (ptr[i] >> bit_offset2) | (ptr[i + 1] << (8 - bit_offset2));

		buffer[used_bytes - 1] = buffer[used_bytes - 1] >> bit_offset2;
		buffer[used_bytes - 1] &= (1 << used_bits) - 1;
		return *(T*)buffer;
	}
	template<typename T>
	T& static_value_get_ref(size_t offset, uint16_t bit_used, uint8_t bit_offset) {
		if((bit_used / 8 == sizeof(T) && bit_used) || bit_offset % sizeof(T) == 0)
			throw InvalidArguments("bit_used is not aligned for type");
		char* ptr = raw_data + count * sizeof(Item);
		ptr += offset;
		ptr += bit_offset / 8;
		return *(T*)ptr;
	}
	template<typename T>
	void static_value_set(size_t offset, uint16_t bit_used, uint8_t bit_offset, T value) {
		if(sizeof(T) * 8 < bit_used && bit_used)
			throw InvalidArguments("bit_used is too big for type");
		
		char* ptr = raw_data + count * sizeof(Item);
		ptr += offset;
		ptr += bit_offset / 8;
		if((bit_used / 8 == sizeof(T) && bit_used) || bit_offset % sizeof(T) == 0)
			*(T*)ptr = value;
		uint8_t bit_offset2 = bit_offset % 8;
		
		uint8_t used_bits = bit_used ? bit_used % 8 : 0;
		uint16_t used_bytes = bit_used ? bit_used / 8 : sizeof(T) + (used_bits ? 1 : 0);
		
		char buffer[sizeof(T)]{0};
		(*(T*)buffer) = value;

		for(uint8_t i = 0; i < used_bytes-1; i++)
			buffer[i] = (buffer[i] << bit_offset2) | (buffer[i + 1] >> (8 - bit_offset2));
		
		buffer[used_bytes - 1] = buffer[used_bytes - 1] << bit_offset2;
		buffer[used_bytes - 1] &= (1 << used_bits) - 1;
		for(uint8_t i = 0; i < used_bytes; i++)
			ptr[i] = (ptr[i] & ~(buffer[i] << bit_offset2)) | (buffer[i] << bit_offset2);
	}
	template<typename T>
	void static_value_set_ref(size_t offset, uint16_t bit_used, uint8_t bit_offset, T value) {
		if((bit_used / 8 == sizeof(T) && bit_used) || bit_offset % sizeof(T) == 0)
			throw InvalidArguments("bit_used is not aligned for type");
		char* ptr = raw_data + count * sizeof(Item);
		ptr += offset;
		ptr += bit_offset / 8;
		*(T*)ptr = value;
	}

	ValueItem static_value_get(size_t value_data_index);
	ValueItem static_value_get_ref(size_t value_data_index);
	void static_value_set(size_t value_data_index, ValueItem value);
	ValueItem dynamic_value_get(const std::string& name);
	ValueItem dynamic_value_get_ref(const std::string& name);
	void dynamic_value_set(const std::string& name, ValueItem value);

	uint64_t table_get_id(const std::string& name, ClassAccess access);
	Enviropment table_get(uint64_t fn_id);
	Enviropment table_get_dynamic(const std::string& name, ClassAccess access);//table_get(table_get_id(name, access))
	
	void add_method(const std::string& name, Enviropment method, ClassAccess access, const list_array<ValueMeta>& return_values, const list_array<list_array<ValueMeta>>& arguments, const list_array<MethodTag>& tags, const std::string& owner_name);//only for AttachADynamicVirtualTable
	void add_method(const std::string& name, const typed_lgr<FuncEnvironment>& method, ClassAccess access, const list_array<ValueMeta>& return_values, const list_array<list_array<ValueMeta>>& arguments, const list_array<MethodTag>& tags, const std::string& owner_name);//only for AttachADynamicVirtualTable

	bool has_method(const std::string& name, ClassAccess access);
	void remove_method(const std::string& name, ClassAccess access);
	typed_lgr<FuncEnvironment> get_method(uint64_t fn_id);
	typed_lgr<FuncEnvironment> get_method_dynamic(const std::string& name, ClassAccess access);

	void table_derive(void* vtable, VTableMode vtable_mode);//only for AttachADynamicVirtualTable
	void change_table(void* vtable, VTableMode vtable_mode);//only for AttachADynamicVirtualTable, destroy old vtable and use new one
	
	VTableMode get_vtable_mode();
	void* get_vtable();
	void* get_data(size_t offset = 0);
	void* get_data_no_vtable(size_t offset = 0);


	Item* get_items(size_t& count);
	

	
	static Structure* construct(size_t structure_size, Item* items, size_t count);
	static Structure* construct(size_t structure_size, Item* items, size_t count, void* vtable, VTableMode vtable_mode);
	static void destruct(Structure* structure);
	static void copy(Structure* dst, Structure* src, bool at_construct);
	static Structure* copy(Structure* src);
	static void move(Structure* dst, Structure* src, bool at_construct);
	static Structure* move(Structure* src);
	static int8_t compare(Structure* a, Structure* b);//vtable
	static int8_t compare_refrence(Structure* a, Structure* b);//refrence compare
	static int8_t compare_object(Structure* a, Structure* b);//compare by Item*`s
	static int8_t compare_full(Structure* a, Structure* b);//compare && compare_object



	void* get_raw_data();//can be useful for light proxy clases

	std::string get_name();
};


#endif /* RUN_TIME_ATTACHA_ABI_STRUCTS */
