// Copyright Danyil Melnytskyi 2022-2023
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once
#ifndef SRC_RUN_TIME_ATTACHA_ABI_STRUCTS
#define SRC_RUN_TIME_ATTACHA_ABI_STRUCTS

#include <cstdint>
#include <cassert>
#include <unordered_map>
#include <unordered_set>
#include <chrono>
#include <exception>
#include "../library/list_array.hpp"
#include "exceptions.hpp"
#include "link_garbage_remover.hpp"
#include "util/enum_helper.hpp"
namespace art {
	struct ValueItem;
}
namespace std {
	template<>
	struct hash<art::ValueItem> {
		size_t operator()(const art::ValueItem& cit) const;
	};
}
namespace art{
	ENUM_t(Opcode,uint8_t,
		(noting)
		(create_saarr)
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
		(handle_end)

		(value_hold)
		(value_unhold)
		(is_gc)
		(to_gc)
		(localize_gc)
		(from_gc)
		(table_jump)
		(xarray_slice)//farray and sarray slice by creating reference to original array with moved pointer and new size
		(store_constant)
		(get_refrence)
		(make_as_const)
		(remove_const_protect)
		(copy_un_constant)
		(copy_un_refrence)
		(move_un_refrence)
		(remove_qualifiers)
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
		Command(Opcode op, bool gc_mode = false, bool set_static_mode = false) {
			code = op;
			is_gc_mode = gc_mode;
			static_mode = set_static_mode;
		}
		Opcode code;
		uint8_t is_gc_mode : 1;
		uint8_t static_mode : 1;
	};

	union CallFlags {
		struct {
			uint8_t always_dynamic : 1;//prevent holding old function reference
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
		(generator)
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
		bool is_patchable : 1;//define function is patchable or not, if not patchable, in function header and footer excluded atomic usage count modificatiion
		//10bits left
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
		std::string to_string() const{
			std::string ret;
			if(!allow_edit) ret += "const ";
			ret += enum_to_string(vtype);
			if(use_gc) ret += "^";
			if(as_ref) ret += "&";
			if(val_len) ret += "[" + std::to_string(val_len) + "]";
			return ret;
		}
	};
	class Structure;
	struct ValueItem;
	class ValueItemIterator{
		ValueItem& item;
		void* iterator_data;
		ValueItemIterator(ValueItem& item, void* iterator_data):item(item), iterator_data(iterator_data){}
	public:
		using iterator_category = std::forward_iterator_tag;
		ValueItemIterator(ValueItem& item, bool end = false);
		ValueItemIterator(ValueItemIterator&& move):item(move.item){
			iterator_data = move.iterator_data;
			move.iterator_data = nullptr;
		}
		ValueItemIterator(const ValueItemIterator& move);
		~ValueItemIterator();
		
		ValueItemIterator& operator++();
		ValueItemIterator operator++(int);
		ValueItem& operator*();
		ValueItem* operator->();


		operator ValueItem() const;
		ValueItem get() const;
		ValueItemIterator& operator=(const ValueItem& item);
		bool operator==(const ValueItemIterator& compare) const;
		bool operator!=(const ValueItemIterator& compare) const;
	};
	class ValueItemConstIterator{
		ValueItemIterator iterator;
	public:
		using iterator_category = std::forward_iterator_tag;
		ValueItemConstIterator(const ValueItem& item, bool end = false):iterator(const_cast<ValueItem&>(item), end){}
		ValueItemConstIterator(ValueItemConstIterator&& move):iterator(std::move(move.iterator)){}
		ValueItemConstIterator(const ValueItemConstIterator& copy):iterator(copy.iterator){}

		~ValueItemConstIterator(){}
		
		ValueItemConstIterator& operator++(){ iterator.operator++(); return *this;}
		ValueItemConstIterator operator++(int){ ValueItemConstIterator ret(*this); iterator.operator++(); return ret;}
		const ValueItem& operator*() const { return const_cast<ValueItemIterator&>(iterator).operator*();}

		operator ValueItem() const;
		ValueItem get() const;
		bool operator==(const ValueItemConstIterator& compare) const{ return iterator.operator==(compare.iterator);}
		bool operator!=(const ValueItemConstIterator& compare) const{ return iterator.operator!=(compare.iterator);}
	};
	namespace __{
		template<typename T, bool as_refrence>
		class array{
			T* data;
			uint32_t length;
			friend ValueItem;
		public:
			array():data(nullptr), length(0){}
			array(T* data, uint32_t length) : length(length){
				if constexpr (as_refrence){
					this->data = data;
				}else{
					this->data = new T[length];
					for(uint32_t i = 0; i < length; i++){
						this->data[i] = data[i];
					}
				}
			}
			array(uint32_t length, T* data) : data(data), length(length){}
			array(const array<T,false>& copy) : length(copy.length){
				if constexpr (as_refrence){
					data = copy.data;
				}else{
					data = new T[length];
					for(uint32_t i = 0; i < length; i++){
						data[i] = copy.data[i];
					}
				}
			}
			array(const array<T,true>& copy) : length(copy.length){
				if constexpr (as_refrence){
					data = copy.data;
				}else{
					data = new T[length];
					for(uint32_t i = 0; i < length; i++){
						data[i] = copy.data[i];
					}
				}
			}
			array(array&& move) : data(move.data), length(move.length){
				move.data = nullptr;
				move.length = 0;
			}
			~array(){
				if constexpr(!as_refrence)
					if(data) delete[] data;
			}
			T& operator[](uint32_t index){
				if constexpr (as_refrence){
					return data[index];
				}else{
					return data[index];
				}
			}
			const T& operator[](uint32_t index) const{
				if constexpr (as_refrence){
					return data[index];
				}else{
					return data[index];
				}
			}
			uint32_t size() const{
				return length;
			}
			T* begin(){
				return data;
			}
			T* end(){
				return data + length;
			}
			const T* begin() const{
				return data;
			}
			const T* end() const{
				return data + length;
			}
			void release(){
				data = nullptr;
				length = 0;
			}
		};
	}
	template<typename T>
	using array_t = __::array<T,false>;
	template<typename T>
	using array_ref_t = __::array<T,true>;
	
	struct as_refrence_t {};
	constexpr inline as_refrence_t as_refrence = {};

	struct no_copy_t {};
	constexpr inline no_copy_t no_copy = {};
	struct ValueItem {
		void* val;
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
		ValueItem(long val):ValueItem(uint32_t(val)){}
		ValueItem(const std::string& val);
		ValueItem(std::string&& val);
		ValueItem(const char* str);
		ValueItem(const list_array<ValueItem>& val);
		ValueItem(list_array<ValueItem>&& val);
		ValueItem(ValueItem* vals, uint32_t len);
		ValueItem(ValueItem* vals, uint32_t len, no_copy_t);
		ValueItem(ValueItem* vals, uint32_t len, as_refrence_t);
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

		ValueItem(int8_t* vals, uint32_t len, as_refrence_t);
		ValueItem(uint8_t* vals, uint32_t len, as_refrence_t);
		ValueItem(int16_t* vals, uint32_t len, as_refrence_t);
		ValueItem(uint16_t* vals, uint32_t len, as_refrence_t);
		ValueItem(int32_t* vals, uint32_t len, as_refrence_t);
		ValueItem(uint32_t* vals, uint32_t len, as_refrence_t);
		ValueItem(int64_t* vals, uint32_t len, as_refrence_t);
		ValueItem(uint64_t* vals, uint32_t len, as_refrence_t);
		ValueItem(float* vals, uint32_t len, as_refrence_t);
		ValueItem(double* vals, uint32_t len, as_refrence_t);
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

		
		ValueItem(const ValueItem& ref, as_refrence_t);
		ValueItem(const bool& val, as_refrence_t);
		ValueItem(const int8_t& val, as_refrence_t);
		ValueItem(const uint8_t& val, as_refrence_t);
		ValueItem(const int16_t& val, as_refrence_t);
		ValueItem(const uint16_t& val, as_refrence_t);
		ValueItem(const int32_t& val, as_refrence_t);
		ValueItem(const uint32_t& val, as_refrence_t);
		ValueItem(const int64_t& val, as_refrence_t);
		ValueItem(const uint64_t& val, as_refrence_t);
		ValueItem(const float& val, as_refrence_t);
		ValueItem(const double& val, as_refrence_t);
		ValueItem(const class Structure*, as_refrence_t);
		ValueItem(const std::string& val, as_refrence_t);
		ValueItem(const list_array<ValueItem>& val, as_refrence_t);

		ValueItem(const std::exception_ptr&, as_refrence_t);
		ValueItem(const std::chrono::steady_clock::time_point&, as_refrence_t);
		ValueItem(const std::unordered_map<ValueItem, ValueItem>&, as_refrence_t);
		ValueItem(const std::unordered_set<ValueItem>&, as_refrence_t);
		ValueItem(const typed_lgr<struct Task>& task, as_refrence_t);
		ValueItem(const ValueMeta&, as_refrence_t);
		ValueItem(const typed_lgr<class FuncEnvironment>&, as_refrence_t);


		ValueItem(array_t<bool>&& val);
		ValueItem(array_t<int8_t>&& val);
		ValueItem(array_t<uint8_t>&& val);
		ValueItem(array_t<int16_t>&& val);
		ValueItem(array_t<uint16_t>&& val);
		ValueItem(array_t<int32_t>&& val);
		ValueItem(array_t<uint32_t>&& val);
		ValueItem(array_t<int64_t>&& val);
		ValueItem(array_t<uint64_t>&& val);
		ValueItem(array_t<float>&& val);
		ValueItem(array_t<double>&& val);
		ValueItem(array_t<ValueItem>&& val);
		
		ValueItem(const array_t<bool>& val);
		ValueItem(const array_t<int8_t>& val);
		ValueItem(const array_t<uint8_t>& val);
		ValueItem(const array_t<int16_t>& val);
		ValueItem(const array_t<uint16_t>& val);
		ValueItem(const array_t<int32_t>& val);
		ValueItem(const array_t<uint32_t>& val);
		ValueItem(const array_t<int64_t>& val);
		ValueItem(const array_t<uint64_t>& val);
		ValueItem(const array_t<float>& val);
		ValueItem(const array_t<double>& val);
		ValueItem(const array_t<ValueItem>& val);
		
		ValueItem(const array_ref_t<bool>& val);
		ValueItem(const array_ref_t<int8_t>& val);
		ValueItem(const array_ref_t<uint8_t>& val);
		ValueItem(const array_ref_t<int16_t>& val);
		ValueItem(const array_ref_t<uint16_t>& val);
		ValueItem(const array_ref_t<int32_t>& val);
		ValueItem(const array_ref_t<uint32_t>& val);
		ValueItem(const array_ref_t<int64_t>& val);
		ValueItem(const array_ref_t<uint64_t>& val);
		ValueItem(const array_ref_t<float>& val);
		ValueItem(const array_ref_t<double>& val);
		ValueItem(const array_ref_t<ValueItem>& val);


		ValueItem& operator=(const ValueItem& copy);
		ValueItem& operator=(ValueItem&& copy);
		~ValueItem();
		int8_t compare(const ValueItem& cmp) const;
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

		explicit operator Structure& ();
		explicit operator std::unordered_map<ValueItem, ValueItem>&();
		explicit operator std::unordered_set<ValueItem>&();
		explicit operator typed_lgr<struct Task>&();
		explicit operator typed_lgr<class FuncEnvironment>&();

		
		explicit operator bool() const;
		explicit operator int8_t() const;
		explicit operator uint8_t() const;
		explicit operator int16_t() const;
		explicit operator uint16_t() const;
		explicit operator int32_t() const;
		explicit operator uint32_t() const;
		explicit operator int64_t() const;
		explicit operator uint64_t() const;
		explicit operator float() const;
		explicit operator double() const;
		explicit operator long() const { return (long)(uint32_t)*this;}
		explicit operator void*() const;
		explicit operator std::string() const;
		explicit operator list_array<ValueItem>() const;
		explicit operator ValueMeta() const;
		explicit operator std::exception_ptr() const;
		explicit operator std::chrono::steady_clock::time_point() const;
		explicit operator const Structure& () const;
		explicit operator const std::unordered_map<ValueItem, ValueItem>&() const;
		explicit operator const std::unordered_set<ValueItem>&() const;
		explicit operator const typed_lgr<struct Task>&() const;
		explicit operator const typed_lgr<class FuncEnvironment>&() const;
		explicit operator const array_t<bool>() const;
		explicit operator const array_t<int8_t>() const;
		explicit operator const array_t<uint8_t>() const;
		explicit operator const array_t<int16_t>() const;
		explicit operator const array_t<uint16_t>() const;
		explicit operator const array_t<int32_t>() const;
		explicit operator const array_t<uint32_t>() const;
		explicit operator const array_t<int64_t>() const;
		explicit operator const array_t<uint64_t>() const;
		explicit operator const array_t<float>() const;
		explicit operator const array_t<double>() const;
		explicit operator const array_t<long>() const;
		explicit operator const array_t<ValueItem>() const;
		explicit operator const array_ref_t<bool>() const;
		explicit operator const array_ref_t<int8_t>() const;
		explicit operator const array_ref_t<uint8_t>() const;
		explicit operator const array_ref_t<int16_t>() const;
		explicit operator const array_ref_t<uint16_t>() const;
		explicit operator const array_ref_t<int32_t>() const;
		explicit operator const array_ref_t<uint32_t>() const;
		explicit operator const array_ref_t<int64_t>() const;
		explicit operator const array_ref_t<uint64_t>() const;
		explicit operator const array_ref_t<float>() const;
		explicit operator const array_ref_t<double>() const;
		explicit operator const array_ref_t<long>() const;
		explicit operator const array_ref_t<ValueItem>() const;
		explicit operator array_ref_t<bool>();
		explicit operator array_ref_t<int8_t>();
		explicit operator array_ref_t<uint8_t>();
		explicit operator array_ref_t<int16_t>();
		explicit operator array_ref_t<uint16_t>();
		explicit operator array_ref_t<int32_t>();
		explicit operator array_ref_t<uint32_t>();
		explicit operator array_ref_t<int64_t>();
		explicit operator array_ref_t<uint64_t>();
		explicit operator array_ref_t<float>();
		explicit operator array_ref_t<double>();
		explicit operator array_ref_t<long>();
		explicit operator array_ref_t<ValueItem>();
		ValueItem* operator()(ValueItem* arguments, uint32_t arguments_size);
		void getAsync();
		void getGeneratorResult(ValueItem* res, uint64_t result_id);
		void*& getSourcePtr();
		const void*& getSourcePtr() const;
		void*& unRef();
		const void* const & unRef() const;
		typed_lgr<class FuncEnvironment>* funPtr();
		const typed_lgr<class FuncEnvironment>* funPtr() const;
		void make_gc();
		void localize_gc();
		void ungc();
		bool is_gc();
		
		size_t hash() const;
		ValueItem make_slice(uint32_t start, uint32_t end) const;


		ValueItem& operator[](const ValueItem& index);
		const ValueItem& operator[](const ValueItem& index) const;
		ValueItem get(const ValueItem& index) const;
		void set(const ValueItem& index, const ValueItem& value);
		bool has(const ValueItem& index) const;
		ValueItemIterator begin();
		ValueItemIterator end();
		ValueItemConstIterator begin() const;
		ValueItemConstIterator end() const;
		ValueItemConstIterator cbegin() const{ return begin(); }
		ValueItemConstIterator cend() const{ return end(); }
		size_t size() const;
	};
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
		const MethodInfo* getMethodsInfo(uint64_t& size) const;
		MethodInfo& getMethodInfo(uint64_t index);
		MethodInfo& getMethodInfo(const std::string& name, ClassAccess access);
		const MethodInfo& getMethodInfo(uint64_t index) const;
		const MethodInfo& getMethodInfo(const std::string& name, ClassAccess access) const;

		Enviropment* getMethods(uint64_t& size);
		Enviropment getMethod(uint64_t index) const;
		Enviropment getMethod(const std::string& name, ClassAccess access) const;

		uint64_t getMethodIndex(const std::string& name, ClassAccess access) const;
		bool hasMethod(const std::string& name, ClassAccess access) const;

		static AttachAVirtualTable* create(list_array<MethodInfo>& methods, typed_lgr<class FuncEnvironment> destructor, typed_lgr<class FuncEnvironment> copy, typed_lgr<class FuncEnvironment> move, typed_lgr<class FuncEnvironment> compare);
		static void destroy(AttachAVirtualTable* table);

		std::string getName() const;
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
		const AfterMethods* getAfterMethods() const;
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
		const MethodInfo& getMethodInfo(uint64_t index) const ;
		const MethodInfo& getMethodInfo(const std::string& name, ClassAccess access) const ;

		Enviropment getMethod(uint64_t index) const;
		Enviropment getMethod(const std::string& name, ClassAccess access) const;

		void addMethod(const std::string& name, Enviropment method, ClassAccess access, const list_array<ValueMeta>& return_values, const list_array<list_array<ValueMeta>>& arguments, const list_array<MethodTag>& tags, const std::string& owner_name);
		void addMethod(const std::string& name, const typed_lgr<FuncEnvironment>& method, ClassAccess access, const list_array<ValueMeta>& return_values, const list_array<list_array<ValueMeta>>& arguments, const list_array<MethodTag>& tags, const std::string& owner_name);

		void removeMethod(const std::string& name, ClassAccess access);

		void addTag(const std::string& name, const ValueItem& value);
		void addTag(const std::string& name, ValueItem&& value);
		void removeTag(const std::string& name);

		uint64_t getMethodIndex(const std::string& name, ClassAccess access) const;
		bool hasMethod(const std::string& name, ClassAccess access) const;

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
		const Item* getPtr(const std::string& name) const;
		const Item* getPtr(size_t index) const;
		template<typename T>
		ValueItem getRawArray(const Item* item) {
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
		ValueItem getRawArray(const Item* item) const {
			ValueItem result;
			if(item->inlined){
				if(item->type.as_ref)
					result = ValueItem((T*)&static_value_get_ref<T*>(item->offset, 0, 0), item->type.val_len, as_refrence);
				else
					result = ValueItem((T*)&static_value_get_ref<T*>(item->offset, 0, 0), item->type.val_len);
			}else{
				if(item->type.as_ref)
					result = ValueItem(static_value_get<T*>(item->offset, 0, item->bit_offset), item->type.val_len, as_refrence);
				else
					result = ValueItem(static_value_get<T*>(item->offset, 0, item->bit_offset), item->type.val_len);
			}
			result.meta.allow_edit = false;
			return result;
		}
		template<typename T>
		ValueItem getType(const Item* item) const {
			if(item->type.as_ref)
				return ValueItem(static_value_get_ref<T>(item->offset, 0, 0), as_refrence);
			else
				return ValueItem(static_value_get_ref<T>(item->offset, 0, 0));
		}
		template<typename T>
		ValueItem getRawArrayRef(const Item* item) {
			if(item->inlined)
				return ValueItem((T*)&static_value_get_ref<T*>(item->offset, 0, 0), item->type.val_len, as_refrence);
			else
				return ValueItem(static_value_get<T*>(item->offset, 0, item->bit_offset), item->type.val_len, as_refrence);
		}
		template<typename T>
		ValueItem getTypeRef(const Item* item){
			return ValueItem(static_value_get_ref<T>(item->offset, 0, 0), as_refrence);
		}

		ValueItem _static_value_get(const Item* item) const;
		ValueItem _static_value_get_ref(const Item* item);
		void _static_value_set(Item* item, ValueItem& set);
		Structure(size_t structure_size, Item* items, size_t count, void* vtable,VTableMode table_mode );
		~Structure() noexcept(false);
	public:
		
		template<typename T>
		T static_value_get(size_t offset, uint16_t bit_used, uint8_t bit_offset) const {
			if(sizeof(T) * 8 < bit_used && bit_used)
				throw InvalidArguments("bit_used is too big for type");
			
			const char* ptr = raw_data + count * sizeof(Item);
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
		const T& static_value_get_ref(size_t offset, uint16_t bit_used, uint8_t bit_offset) const {
			if((bit_used / 8 == sizeof(T) && bit_used) || bit_offset % sizeof(T) == 0)
				throw InvalidArguments("bit_used is not aligned for type");
			const char* ptr = raw_data + count * sizeof(Item);
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
			const char* ptr = raw_data + count * sizeof(Item);
			ptr += offset;
			ptr += bit_offset / 8;
			*(T*)ptr = value;
		}

		ValueItem static_value_get(size_t value_data_index) const;
		ValueItem static_value_get_ref(size_t value_data_index);
		void static_value_set(size_t value_data_index, ValueItem value);
		ValueItem dynamic_value_get(const std::string& name) const;
		ValueItem dynamic_value_get_ref(const std::string& name);
		void dynamic_value_set(const std::string& name, ValueItem value);

		uint64_t table_get_id(const std::string& name, ClassAccess access) const;
		Enviropment table_get(uint64_t fn_id) const;
		Enviropment table_get_dynamic(const std::string& name, ClassAccess access) const;//table_get(table_get_id(name, access))
		
		void add_method(const std::string& name, Enviropment method, ClassAccess access, const list_array<ValueMeta>& return_values, const list_array<list_array<ValueMeta>>& arguments, const list_array<MethodTag>& tags, const std::string& owner_name);//only for AttachADynamicVirtualTable
		void add_method(const std::string& name, const typed_lgr<FuncEnvironment>& method, ClassAccess access, const list_array<ValueMeta>& return_values, const list_array<list_array<ValueMeta>>& arguments, const list_array<MethodTag>& tags, const std::string& owner_name);//only for AttachADynamicVirtualTable

		bool has_method(const std::string& name, ClassAccess access) const;
		void remove_method(const std::string& name, ClassAccess access);
		typed_lgr<FuncEnvironment> get_method(uint64_t fn_id) const;
		typed_lgr<FuncEnvironment> get_method_dynamic(const std::string& name, ClassAccess access) const;

		void table_derive(void* vtable, VTableMode vtable_mode);//only for AttachADynamicVirtualTable
		void change_table(void* vtable, VTableMode vtable_mode);//only for AttachADynamicVirtualTable, destroy old vtable and use new one
		
		VTableMode get_vtable_mode() const;
		void* get_vtable();
		const void* get_vtable() const;
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

		std::string get_name() const;
	};
}
#ifndef FFDSSC
#define FFDSSC

namespace std {
	inline size_t hash<art::ValueItem>::operator()(const art::ValueItem& cit) const {
		return cit.hash();
	}
}
#endif // !1
#endif /* SRC_RUN_TIME_ATTACHA_ABI_STRUCTS */
