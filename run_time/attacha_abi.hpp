// Copyright Danyil Melnytskyi 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once
#include "../run_time.hpp"
#include "link_garbage_remover.hpp"
#include "attacha_abi_structs.hpp"
#include "cxxException.hpp"

bool needAlloc(VType type);











bool calc_safe_deph_arr(void* ptr);

template<class T>
void defaultDestructor(void* a) {
	delete (T*)a;
}
template<class T>
void arrayDestructor(void* a) {
	delete[] (T*)a;
}
template<class T>
void Allocate(void** a) {
	*a = new T();
}




void universalFree(void** value, ValueMeta meta);
void universalRemove(void** value);
void universalAlloc(void** value, ValueMeta meta);

void initEnviropement(void** res, uint32_t vals_count);
void removeEnviropement(void** env, uint16_t vals_count);
void removeArgsEnviropement(list_array<ValueItem>* env);
char* getStrBegin(std::string* str);
void throwInvalidType();

auto gcCall(lgr* gc, list_array<ValueItem>* args, bool async_mode);
ValueItem* getAsyncValueItem(void* val);
void getValueItem(void** value, ValueItem* f_res);
ValueItem* buildRes(void** value);


void getAsyncResult(void*& value, ValueMeta& meta);
void* copyValue(void*& val, ValueMeta& meta);
void** copyEnviropement(void** env, uint16_t env_it_count);


void** preSetValue(void** value, ValueMeta set_meta, bool match_gc_dif);
void*& getValue(void*& value, ValueMeta& meta);
void*& getValue(void** value);
void* getSpecificValue(void** value, VType typ);
void** getSpecificValueLink(void** value, VType typ);

void** getValueLink(void** value);

bool is_integer(VType typ);
bool integer_unsigned(VType typ);

//return equal,lower bool result
std::pair<bool, bool> compareValue(VType cmp1, VType cmp2, void* val1, void* val2);
RFLAGS compare(RFLAGS old, void** value_1, void** value_2);
RFLAGS link_compare(RFLAGS old, void** value_1, void** value_2);

void copyEnviropement(void** env, uint16_t env_it_count, void*** res);





namespace ABI_IMPL {

	template <class T>
	T Vcast(void*& val, ValueMeta& meta) {
		getAsyncResult(val, meta);
		switch (meta.vtype) {
		case VType::noting:
			return T();
		case VType::raw_arr_i8: {
			if constexpr (std::is_pointer_v<T>)
				return (T)val;
			else
				return (T)((int8_t*)val)[0];
		}
		case VType::raw_arr_i16: {
			if constexpr (std::is_pointer_v<T>)
				return (T)val;
			else
				return (T)((int16_t*)val)[0];
		}
		case VType::raw_arr_i32: {
			if constexpr (std::is_pointer_v<T>)
				return (T)val;
			else
				return (T)((int32_t*)val)[0];
		}
		case VType::raw_arr_i64: {
			if constexpr (std::is_pointer_v<T>)
				return (T)val;
			else
				return (T)((int64_t*)val)[0];
		}
		case VType::raw_arr_ui8: {
			if constexpr (std::is_pointer_v<T>)
				return (T)val;
			else
				return (T)((uint8_t*)val)[0];
		}
		case VType::raw_arr_ui16: {
			if constexpr (std::is_pointer_v<T>)
				return (T)val;
			else
				return (T)((uint16_t*)val)[0];
		}
		case VType::raw_arr_ui32: {
			if constexpr (std::is_pointer_v<T>)
				return (T)val;
			else
				return (T)((uint32_t*)val)[0];
		}
		case VType::raw_arr_ui64: {
			if constexpr (std::is_pointer_v<T>)
				return (T)val;
			else
				return (T)((uint64_t*)val)[0];
		}
		case VType::raw_arr_flo: {
			if constexpr (std::is_pointer_v<T>)
				return (T)val;
			else
				return (T)((float*)val)[0];
		}
		case VType::raw_arr_doub: {
			if constexpr (std::is_pointer_v<T>)
				return (T)val;
			else
				return (T)((double*)val)[0];
		}
		case VType::i8:
			return (T)reinterpret_cast<int8_t&>(val);
		case VType::i16:
			return (T)reinterpret_cast<int16_t&>(val);
		case VType::i32:
			return (T)reinterpret_cast<int32_t&>(val);
		case VType::i64:
			return (T)reinterpret_cast<int64_t&>(val);
		case VType::ui8:
			return (T)reinterpret_cast<uint8_t&>(val);
		case VType::ui16:
			return (T)reinterpret_cast<uint16_t&>(val);
		case VType::ui32:
			return (T)reinterpret_cast<uint32_t&>(val);
		case VType::ui64:
			return (T)reinterpret_cast<uint64_t&>(val);
		case VType::flo:
			if constexpr(std::is_same_v<void*,T>)
				return val;
			else
				return (T)reinterpret_cast<float&>(val);
		case VType::doub:
			if constexpr (std::is_same_v<void*, T>)
				return val;
			else
				return (T)reinterpret_cast<double&>(val);
		case VType::uarr:
			if(((list_array<ValueItem>*)val)->size()!=1)
				throw InvalidCast("Fail cast uarr");
			else {
				auto& tmp = (*(list_array<ValueItem>*)val)[0];
				return Vcast<T>(tmp.val, tmp.meta);
			}
		case VType::string:
			try {
				return (T)std::stoll(reinterpret_cast<std::string&>(val));
			}
			catch (const std::exception& ex) {
				throw InvalidCast("Fail cast string to long");
			}
		case VType::undefined_ptr:
			return (T)reinterpret_cast<size_t&>(val);
		default:
			throw InvalidCast("Fail cast undefined type");
		}
	}
	
	std::string Scast(void*& val, ValueMeta& meta);
	ValueItem SBcast(const std::string& str);
}





void DynSum(void** val0, void** val1);
void DynMinus(void** val0, void** val1);
void DynMul(void** val0, void** val1);
void DynDiv(void** val0, void** val1);


void DynBitXor(void** val0, void** val1);
void DynBitOr(void** val0, void** val1);
void DynBitAnd(void** val0, void** val1);
void DynBitNot(void** val0);

void* AsArg(void** val);
void AsArr(void** val);

size_t getSize(void** value);

void asValue(void** val, VType type);
bool isTrueValue(void** value);
void setBoolValue(bool,void** value);

namespace exception_abi {
	bool is_except(void** val);
	void ignore_except(void** val);
	void continue_unwind(void** val);

	//return true if val is actually exception, can be used for finally blocks
	bool call_except_handler(void** val, bool(*func_symbol)(void** val), bool ignore_fault = true);

	typedef ptrdiff_t jump_point;
	struct jump_handle_except {
		std::string type_name;
		jump_point jump_off;
	};
	//for static catch block
	ptrdiff_t switch_jump_handle_except(void** val, jump_handle_except* handlers, size_t handlers_c);
	//for dynamic catch block
	ptrdiff_t switch_jump_handle_except(void** val, list_array<jump_handle_except>* handlers);


	template<class _FN, class ...Args>
	ValueItem* catchCall(_FN func, Args... args) {
		try {
			return func(args...);
		}
		catch (...) {
			try {
				return new ValueItem(new std::exception_ptr(std::current_exception()), ValueMeta(VType::except_value, false, true), true);
			}
			catch (const std::bad_alloc& ex) {
				throw EnviropmentRuinException();
			}
		}
	}
}
