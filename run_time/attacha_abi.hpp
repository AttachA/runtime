// Copyright Danyil Melnytskyi 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once
#include "../run_time.hpp"
#include "../library/string_help.hpp"
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
	ValueItem* _Vcast_callFN(void* ptr);

	std::string Scast(void*& val, ValueMeta& meta);
	ValueItem SBcast(const std::string& str);
	template<class T = int8_t>
	ValueItem BVcast(const T& val) {
		if constexpr (std::is_same_v<std::remove_cvref_t<T>, int8_t>)
			return ValueItem((void*)val, ValueMeta(VType::i8, false, true));
		else if constexpr (std::is_same_v<std::remove_cvref_t<T>, uint8_t>)
			return ValueItem((void*)val, ValueMeta(VType::ui8, false, true));
		else if constexpr (std::is_same_v<std::remove_cvref_t<T>, int16_t>)
			return ValueItem((void*)val, ValueMeta(VType::i16, false, true));
		else if constexpr (std::is_same_v<std::remove_cvref_t<T>, uint16_t>)
			return ValueItem((void*)val, ValueMeta(VType::ui16, false, true));
		else if constexpr (std::is_same_v<std::remove_cvref_t<T>, int32_t>)
			return ValueItem((void*)val, ValueMeta(VType::i32, false, true));
		else if constexpr (std::is_same_v<std::remove_cvref_t<T>, int32_t>)
			return ValueItem((void*)val, ValueMeta(VType::ui32, false, true));
		else if constexpr (std::is_same_v<std::remove_cvref_t<T>, int64_t>)
			return ValueItem((void*)val, ValueMeta(VType::i64, false, true));
		else if constexpr (std::is_same_v<std::remove_cvref_t<T>, uint64_t>)
			return ValueItem((void*)val, ValueMeta(VType::ui64, false, true));
		else if constexpr (std::is_same_v<std::remove_cvref_t<T>, float>)
			return ValueItem(*(void**)&val, ValueMeta(VType::flo, false, true));
		else if constexpr (std::is_same_v<std::remove_cvref_t<T>, double>)
			return ValueItem(*(void**)&val, ValueMeta(VType::doub, false, true));
		else if constexpr (std::is_same_v<std::remove_cvref_t<T>, std::string> || std::is_same_v<T, const char*>)
			return ValueItem(new std::string(val), ValueMeta(VType::string, false, true));
		else if constexpr (std::is_same_v<std::remove_cvref_t<T>, list_array<ValueItem>>)
			return ValueItem(new list_array<ValueItem>(val), ValueMeta(VType::uarr, false, true));
		else {
			static_assert(
				(
					std::is_arithmetic_v<std::remove_cvref_t<T>> ||
					std::is_same_v<std::remove_cvref_t<T>, std::string> ||
					std::is_same_v<std::remove_cvref_t<T>, ValueItem> ||
					std::is_same_v<std::remove_cvref_t<T>, list_array<ValueItem>>
					),
				"Invalid type for convert"
				);
			throw CompileTimeException("Invalid compiler, use correct compiler for compile AttachA, //ignored static_assert//");
		}
	}
	template <class T>
	T Vcast(void*& val, ValueMeta& meta) {
		getAsyncResult(val, meta);
		if constexpr (std::is_same_v<T, std::string>) {
			return Scast(val, meta);
		}
		else {
			switch (meta.vtype) {
			case VType::noting:
				return T();
			case VType::i8: {
				if constexpr (std::is_same_v<T, void*>)
					return val;
				else if constexpr (std::is_same_v<T, list_array<ValueItem>>)
					return { ValueItem(val,meta) };
				else if constexpr (std::is_arithmetic_v<T>)
					return (T)(int8_t)val;
				else if constexpr (std::is_arithmetic_v<std::remove_pointer_t<T>>)
					return new std::remove_pointer_t<T>[] { (std::remove_pointer_t<T>)(int8_t)val };
				else if constexpr (std::is_same_v<T, list_array<ValueItem>>)
					return { ValueItem(val,meta) };
				else
					return (T)val;
			}
			case VType::i16: {
				if constexpr (std::is_same_v<T, void*>)
					return val;
				else if constexpr (std::is_same_v<T, list_array<ValueItem>>)
					return { ValueItem(val,meta) };
				else if constexpr (std::is_arithmetic_v<T>)
					return (T)(int16_t)val;
				else if constexpr (std::is_arithmetic_v<std::remove_pointer_t<T>>)
					return new std::remove_pointer_t<T>[] { (std::remove_pointer_t<T>)(int16_t)val };
				else if constexpr (std::is_same_v<T, list_array<ValueItem>>)
					return { ValueItem(val,meta) };
				else
					return (T)val;
			}
			case VType::i32: {
				if constexpr (std::is_same_v<T, void*>)
					return val;
				else if constexpr (std::is_same_v<T, list_array<ValueItem>>)
					return { ValueItem(val,meta) };
				else if constexpr (std::is_arithmetic_v<T>)
					return (T)(int32_t)val;
				else if constexpr (std::is_arithmetic_v<std::remove_pointer_t<T>>)
					return new std::remove_pointer_t<T>[] { (std::remove_pointer_t<T>)(int32_t)val };
				else if constexpr (std::is_same_v<T, list_array<ValueItem>>)
					return { ValueItem(val,meta) };
				else
					return (T)val;
			}
			case VType::i64: {
				if constexpr (std::is_same_v<T, void*>)
					return val;
				else if constexpr (std::is_same_v<T, list_array<ValueItem>>)
					return { ValueItem(val,meta) };
				else if constexpr (std::is_arithmetic_v<T>)
					return (T)(int64_t)val;
				else if constexpr (std::is_arithmetic_v<std::remove_pointer_t<T>>)
					return new std::remove_pointer_t<T>[] { (std::remove_pointer_t<T>)(int64_t)val };
				else if constexpr (std::is_same_v<T, list_array<ValueItem>>)
					return { ValueItem(val,meta) };
				else
					return (T)val;
			}
			case VType::ui8: {
				if constexpr (std::is_same_v<T, void*>)
					return val;
				else if constexpr (std::is_same_v<T, list_array<ValueItem>>)
					return { ValueItem(val,meta) };
				else if constexpr (std::is_arithmetic_v<T>)
					return (T)(uint8_t)val;
				else if constexpr (std::is_arithmetic_v<std::remove_pointer_t<T>>)
					return new std::remove_pointer_t<T>[] { (std::remove_pointer_t<T>)(uint8_t)val };
				else if constexpr (std::is_same_v<T, list_array<ValueItem>>)
					return { ValueItem(val,meta) };
				else
					return (T)val;
			}
			case VType::ui16: {
				if constexpr (std::is_same_v<T, void*>)
					return val;
				else if constexpr (std::is_same_v<T, list_array<ValueItem>>)
					return { ValueItem(val,meta) };
				else if constexpr (std::is_arithmetic_v<T>)
					return (T)(uint16_t)val;
				else if constexpr (std::is_arithmetic_v<std::remove_pointer_t<T>>)
					return new std::remove_pointer_t<T>[] { (std::remove_pointer_t<T>)(uint16_t)val };
				else if constexpr (std::is_same_v<T, list_array<ValueItem>>)
					return { ValueItem(val,meta) };
				else
					return (T)val;
			}
			case VType::ui32: {
				if constexpr (std::is_same_v<T, void*>)
					return val;
				else if constexpr (std::is_same_v<T, list_array<ValueItem>>)
					return { ValueItem(val,meta) };
				else if constexpr (std::is_arithmetic_v<T>)
					return (T)(uint32_t)val;
				else if constexpr (std::is_arithmetic_v<std::remove_pointer_t<T>>)
					return new std::remove_pointer_t<T>[] { (std::remove_pointer_t<T>)(uint32_t)val };
				else if constexpr (std::is_same_v<T, list_array<ValueItem>>)
					return { ValueItem(val,meta) };
				else
					return (T)val;
			}
			case VType::ui64: {
				if constexpr (std::is_same_v<T, void*>)
					return val;
				else if constexpr (std::is_same_v<T, list_array<ValueItem>>)
					return { ValueItem(val,meta) };
				else if constexpr (std::is_arithmetic_v<T>)
					return (T)(uint64_t)val;
				else if constexpr (std::is_arithmetic_v<std::remove_pointer_t<T>>)
					return new std::remove_pointer_t<T>[] { (std::remove_pointer_t<T>)(uint64_t)val };
				else if constexpr (std::is_same_v<T, list_array<ValueItem>>)
					return { ValueItem(val,meta) };
				else
					return (T)val;
			}
			case VType::flo: {
				if constexpr (std::is_same_v<T, void*>)
					return val;
				else if constexpr (std::is_same_v<T, list_array<ValueItem>>)
					return { ValueItem(val,meta) };
				else if constexpr (std::is_arithmetic_v<T>)
					return (T) * (float*)&val;
				else if constexpr (std::is_arithmetic_v<std::remove_pointer_t<T>>)
					return new std::remove_pointer_t<T>[] { (std::remove_pointer_t<T>)* (float*)&val };
				else if constexpr (std::is_same_v<T, list_array<ValueItem>>)
					return { ValueItem(val,meta) };
				else
					return (T)val;
			}
			case VType::doub: {
				if constexpr (std::is_same_v<T, void*>)
					return val;
				else if constexpr (std::is_same_v<T, list_array<ValueItem>>)
					return { ValueItem(val,meta) };
				else if constexpr (std::is_arithmetic_v<T>)
					return (T) * (double*)&val;
				else if constexpr (std::is_arithmetic_v<std::remove_pointer_t<T>>)
					return new std::remove_pointer_t<T>[] { (std::remove_pointer_t<T>)* (double*)&val };
				else if constexpr (std::is_same_v<T, list_array<ValueItem>>)
					return { ValueItem(val,meta) };
				else
					return (T)val;
			}
			case VType::raw_arr_i8: {
				if constexpr (std::is_same_v<T, void*>)
					return val;
				else if constexpr (std::is_same_v<T, list_array<ValueItem>>)
					return { ValueItem(val,meta) };
				else if constexpr (std::is_arithmetic_v<T>)
					return (T) * reinterpret_cast<int8_t*>(val);
				else if constexpr (std::is_arithmetic_v<std::remove_pointer_t<T>>) {
					if (meta.val_len) {
						std::remove_pointer_t<T>* tmp = new std::remove_pointer_t<T>[meta.val_len];
						for (uint32_t i = 0; i < meta.val_len; i++)
							tmp[i] = (std::remove_pointer_t<T>)reinterpret_cast<int8_t*>(val)[i];
						return tmp;
					}
					else return new std::remove_pointer_t<T>[] { (std::remove_pointer_t<T>)* reinterpret_cast<int8_t*>(val) };
				}
				else if constexpr (std::is_same_v<T, list_array<ValueItem>>) {
					if (meta.val_len) {
						list_array<ValueItem> res;
						res.resize(meta.val_len);
						for (uint32_t i = 0; i < meta.val_len; i++)
							res[i] = ValueItem((void*)reinterpret_cast<int8_t*>(val)[i], VType::i8);
						return res;
					}
					else return { ValueItem(val,meta) };
				}
				else throw InvalidCast("Fail cast raw_arr_i8");
			}
			case VType::raw_arr_i16: {
				if constexpr (std::is_same_v<T, void*>)
					return val;
				else if constexpr (std::is_same_v<T, list_array<ValueItem>>)
					return { ValueItem(val,meta) };
				else if constexpr (std::is_arithmetic_v<T>)
					return (T) * reinterpret_cast<int16_t*>(val);
				else if constexpr (std::is_arithmetic_v<std::remove_pointer_t<T>>) {
					if (meta.val_len) {
						std::remove_pointer_t<T>* tmp = new std::remove_pointer_t<T>[meta.val_len];
						for (uint32_t i = 0; i < meta.val_len; i++)
							tmp[i] = (std::remove_pointer_t<T>)reinterpret_cast<int16_t*>(val)[i];
						return tmp;
					}
					else return new std::remove_pointer_t<T>[] { (std::remove_pointer_t<T>)* reinterpret_cast<int16_t*>(val) };
				}
				else if constexpr (std::is_same_v<T, list_array<ValueItem>>) {
					if (meta.val_len) {
						list_array<ValueItem> res;
						res.resize(meta.val_len);
						for (uint32_t i = 0; i < meta.val_len; i++)
							res[i] = ValueItem((void*)reinterpret_cast<int16_t*>(val)[i], VType::i16);
						return res;
					}
					else return { ValueItem(val,meta) };
				}
				else throw InvalidCast("Fail cast raw_arr_i16");
			}
			case VType::raw_arr_i32: {
				if constexpr (std::is_same_v<T, void*>)
					return val;
				else if constexpr (std::is_same_v<T, list_array<ValueItem>>)
					return { ValueItem(val,meta) };
				else if constexpr (std::is_arithmetic_v<T>)
					return (T) * reinterpret_cast<int32_t*>(val);
				else if constexpr (std::is_arithmetic_v<std::remove_pointer_t<T>>) {
					if (meta.val_len) {
						std::remove_pointer_t<T>* tmp = new std::remove_pointer_t<T>[meta.val_len];
						for (uint32_t i = 0; i < meta.val_len; i++)
							tmp[i] = (std::remove_pointer_t<T>)reinterpret_cast<int32_t*>(val)[i];
						return tmp;
					}
					else return new std::remove_pointer_t<T>[] { (std::remove_pointer_t<T>)* reinterpret_cast<int32_t*>(val) };
				}
				else if constexpr (std::is_same_v<T, list_array<ValueItem>>) {
					if (meta.val_len) {
						list_array<ValueItem> res;
						res.resize(meta.val_len);
						for (uint32_t i = 0; i < meta.val_len; i++)
							res[i] = ValueItem((void*)reinterpret_cast<int32_t*>(val)[i], VType::i32);
						return res;
					}
					else return { ValueItem(val,meta) };
				}
				else throw InvalidCast("Fail cast raw_arr_i32");
			}
			case VType::raw_arr_i64: {
				if constexpr (std::is_same_v<T, void*>)
					return val;
				else if constexpr (std::is_same_v<T, list_array<ValueItem>>)
					return { ValueItem(val,meta) };
				else if constexpr (std::is_arithmetic_v<T>)
					return (T) * (int64_t*)val;
				else if constexpr (std::is_arithmetic_v<std::remove_pointer_t<T>>) {
					if (meta.val_len) {
						std::remove_pointer_t<T>* tmp = new std::remove_pointer_t<T>[meta.val_len];
						for (uint32_t i = 0; i < meta.val_len; i++)
							tmp[i] = (std::remove_pointer_t<T>)reinterpret_cast<int64_t*>(val)[i];
						return tmp;
					}
					else return new std::remove_pointer_t<T>[] { (std::remove_pointer_t<T>)* reinterpret_cast<int64_t*>(val) };
				}
				else if constexpr (std::is_same_v<T, list_array<ValueItem>>) {
					if (meta.val_len) {
						list_array<ValueItem> res;
						res.resize(meta.val_len);
						for (uint32_t i = 0; i < meta.val_len; i++)
							res[i] = ValueItem((void*)reinterpret_cast<int64_t*>(val)[i], VType::i64);
						return res;
					}
					else return { ValueItem(val,meta) };
				}
				else throw InvalidCast("Fail cast raw_arr_i64");
			}
			case VType::raw_arr_ui8: {
				if constexpr (std::is_same_v<T, void*>)
					return val;
				else if constexpr (std::is_same_v<T, list_array<ValueItem>>)
					return { ValueItem(val,meta) };
				else if constexpr (std::is_arithmetic_v<T>)
					return (T) * reinterpret_cast<uint8_t*>(val);
				else if constexpr (std::is_arithmetic_v<std::remove_pointer_t<T>>) {
					if (meta.val_len) {
						std::remove_pointer_t<T>* tmp = new std::remove_pointer_t<T>[meta.val_len];
						for (uint32_t i = 0; i < meta.val_len; i++)
							tmp[i] = (std::remove_pointer_t<T>)reinterpret_cast<uint8_t*>(val)[i];
						return tmp;
					}
					else return new std::remove_pointer_t<T>[] { (std::remove_pointer_t<T>)* reinterpret_cast<uint8_t*>(val) };
				}
				else if constexpr (std::is_same_v<T, list_array<ValueItem>>) {
					if (meta.val_len) {
						list_array<ValueItem> res;
						res.resize(meta.val_len);
						for (uint32_t i = 0; i < meta.val_len; i++)
							res[i] = ValueItem((void*)reinterpret_cast<uint8_t*>(val)[i], VType::ui8);
						return res;
					}
					else return { ValueItem(val,meta) };
				}
				else throw InvalidCast("Fail cast raw_arr_ui8");
			}
			case VType::raw_arr_ui16: {
				if constexpr (std::is_same_v<T, void*>)
					return val;
				else if constexpr (std::is_same_v<T, list_array<ValueItem>>)
					return { ValueItem(val,meta) };
				else if constexpr (std::is_arithmetic_v<T>)
					return (T) * reinterpret_cast<uint16_t*>(val);
				else if constexpr (std::is_arithmetic_v<std::remove_pointer_t<T>>) {
					if (meta.val_len) {
						std::remove_pointer_t<T>* tmp = new std::remove_pointer_t<T>[meta.val_len];
						for (uint32_t i = 0; i < meta.val_len; i++)
							tmp[i] = (std::remove_pointer_t<T>)reinterpret_cast<uint16_t*>(val)[i];
						return tmp;
					}
					else return new std::remove_pointer_t<T>[] { (std::remove_pointer_t<T>)* reinterpret_cast<uint16_t*>(val) };
				}
				else if constexpr (std::is_same_v<T, list_array<ValueItem>>) {
					if (meta.val_len) {
						list_array<ValueItem> res;
						res.resize(meta.val_len);
						for (uint32_t i = 0; i < meta.val_len; i++)
							res[i] = ValueItem((void*)reinterpret_cast<uint16_t*>(val)[i], VType::ui16);
						return res;
					}
					else return { ValueItem(val,meta) };
				}
				else throw InvalidCast("Fail cast raw_arr_ui16");
			}
			case VType::raw_arr_ui32: {
				if constexpr (std::is_same_v<T, void*>)
					return val;
				else if constexpr (std::is_same_v<T, list_array<ValueItem>>)
					return { ValueItem(val,meta) };
				else if constexpr (std::is_arithmetic_v<T>)
					return (T) * reinterpret_cast<uint32_t*>(val);
				else if constexpr (std::is_arithmetic_v<std::remove_pointer_t<T>>) {
					if (meta.val_len) {
						std::remove_pointer_t<T>* tmp = new std::remove_pointer_t<T>[meta.val_len];
						for (uint32_t i = 0; i < meta.val_len; i++)
							tmp[i] = (std::remove_pointer_t<T>)reinterpret_cast<uint32_t*>(val)[i];
						return tmp;
					}
					else return new std::remove_pointer_t<T>[] { (std::remove_pointer_t<T>)* reinterpret_cast<uint32_t*>(val) };
				}
				else if constexpr (std::is_same_v<T, list_array<ValueItem>>) {
					if (meta.val_len) {
						list_array<ValueItem> res;
						res.resize(meta.val_len);
						for (uint32_t i = 0; i < meta.val_len; i++)
							res[i] = ValueItem((void*)reinterpret_cast<uint32_t*>(val)[i], VType::ui32);
						return res;
					}
					else return { ValueItem(val,meta) };
				}
				else throw InvalidCast("Fail cast raw_arr_ui32");
			}
			case VType::raw_arr_ui64: {
				if constexpr (std::is_same_v<T, void*>)
					return val;
				else if constexpr (std::is_same_v<T, list_array<ValueItem>>)
					return { ValueItem(val,meta) };
				else if constexpr (std::is_arithmetic_v<T>)
					return (T) * reinterpret_cast<uint64_t*>(val);
				else if constexpr (std::is_arithmetic_v<std::remove_pointer_t<T>>) {
					if (meta.val_len) {
						std::remove_pointer_t<T>* tmp = new std::remove_pointer_t<T>[meta.val_len];
						for (uint32_t i = 0; i < meta.val_len; i++)
							tmp[i] = (std::remove_pointer_t<T>)reinterpret_cast<uint64_t*>(val)[i];
						return tmp;
					}
					else return new std::remove_pointer_t<T>[] { (std::remove_pointer_t<T>)* reinterpret_cast<uint64_t*>(val) };
				}
				else if constexpr (std::is_same_v<T, list_array<ValueItem>>) {
					if (meta.val_len) {
						list_array<ValueItem> res;
						res.resize(meta.val_len);
						for (uint32_t i = 0; i < meta.val_len; i++)
							res[i] = ValueItem((void*)reinterpret_cast<uint64_t*>(val)[i], VType::ui64);
						return res;
					}
					else return { ValueItem(val,meta) };
				}
				else throw InvalidCast("Fail cast raw_arr_ui64");
			}
			case VType::raw_arr_flo: {
				if constexpr (std::is_same_v<T, void*>)
					return val;
				else if constexpr (std::is_same_v<T, list_array<ValueItem>>)
					return { ValueItem(val,meta) };
				else if constexpr (std::is_arithmetic_v<T>)
					return (T) * reinterpret_cast<float*>(val);
				else if constexpr (std::is_arithmetic_v<std::remove_pointer_t<T>>) {
					if (meta.val_len) {
						std::remove_pointer_t<T>* tmp = new std::remove_pointer_t<T>[meta.val_len];
						for (uint32_t i = 0; i < meta.val_len; i++)
							tmp[i] = (std::remove_pointer_t<T>)reinterpret_cast<float*>(val)[i];
						return tmp;
					}
					else return new std::remove_pointer_t<T>[] { (std::remove_pointer_t<T>)* reinterpret_cast<float*>(val) };
				}
				else if constexpr (std::is_same_v<T, list_array<ValueItem>>) {
					if (meta.val_len) {
						list_array<ValueItem> res;
						res.resize(meta.val_len);
						for (uint32_t i = 0; i < meta.val_len; i++)
							res[i] = ValueItem((void*)reinterpret_cast<float*>(val)[i], VType::flo);
						return res;
					}
					else return { ValueItem(val,meta) };
				}
				else throw InvalidCast("Fail cast raw_arr_flo");
			}
			case VType::raw_arr_doub: {
				if constexpr (std::is_same_v<T, void*>)
					return val;
				else if constexpr (std::is_same_v<T, list_array<ValueItem>>)
					return { ValueItem(val,meta) };
				else if constexpr (std::is_arithmetic_v<T>)
					return (T) * reinterpret_cast<double*>(val);
				else if constexpr (std::is_arithmetic_v<std::remove_pointer_t<T>>) {
					if (meta.val_len) {
						std::remove_pointer_t<T>* tmp = new std::remove_pointer_t<T>[meta.val_len];
						for (uint32_t i = 0; i < meta.val_len; i++)
							tmp[i] = (std::remove_pointer_t<T>)reinterpret_cast<double*>(val)[i];
						return tmp;
					}
					else return new std::remove_pointer_t<T>[] { (std::remove_pointer_t<T>)* reinterpret_cast<double*>(val) };
				}
				else if constexpr (std::is_same_v<T, list_array<ValueItem>>) {
					if (meta.val_len) {
						list_array<ValueItem> res;
						res.resize(meta.val_len);
						for (uint32_t i = 0; i < meta.val_len; i++)
							res[i] = ValueItem((void*)reinterpret_cast<double*>(val)[i], VType::doub);
						return res;
					}
					else return { ValueItem(val,meta) };
				}
				else throw InvalidCast("Fail cast raw_arr_doub");
			}
			case VType::uarr: {
				if constexpr (std::is_same_v<T, void*>)
					return val;
				else if constexpr (std::is_same_v<T, list_array<ValueItem>>)
					return reinterpret_cast<list_array<ValueItem>&>(val);
				else if constexpr (std::is_arithmetic_v<T>) {
					if (((list_array<ValueItem>*)val)->size() != 1)
						throw InvalidCast("Fail cast uarr");
					else {
						auto& tmp = (*(list_array<ValueItem>*)val)[0];
						return Vcast<T>(tmp.val, tmp.meta);
					}
				}
				else if constexpr (std::is_arithmetic_v<std::remove_pointer_t<T>>) {
					if (meta.val_len) {
						list_array<ValueItem>& ref = reinterpret_cast<list_array<ValueItem>&>(val);
						std::remove_pointer_t<T>* res = new std::remove_pointer_t<T>[meta.val_len];
						for (uint32_t i = 0; i < meta.val_len; i++) {
							ValueItem& tmp = ref[i];
							res[i] = Vcast<std::remove_pointer_t<T>>(tmp.val, tmp.meta);
						}
						return res;
					}
					else throw InvalidCast("Fail cast uarr");
				}
				else if constexpr (std::is_same_v<T, list_array<ValueItem>>) {
					return reinterpret_cast<list_array<ValueItem>&>(val);
				}
				else throw InvalidCast("Fail cast uarr");
			}
			case VType::string: {
				ValueItem tmp = SBcast(reinterpret_cast<const std::string&>(val));
				if (tmp.meta.vtype == VType::string) {
					if constexpr (std::is_same_v<T, list_array<ValueItem>>)
						return { ValueItem(val,meta) };
					else throw InvalidCast("Fail cast string");
				}
				else
					return Vcast<T>(tmp.val, tmp.meta);
			}
			case VType::undefined_ptr:{
				if constexpr (std::is_same_v<T, void*>)
					return val;
				else if constexpr (std::is_same_v<T, list_array<ValueItem>>)
					return { ValueItem(val,meta) };
				else if constexpr (std::is_arithmetic_v<T>)
					return *reinterpret_cast<T*>(&val);
				else if constexpr (std::is_pointer_v<T>)
					return (T)val;
				else throw InvalidCast("Fail cast undefined_ptr");
			}
			case VType::type_identifier:
				if constexpr (std::is_same_v<T, list_array<ValueItem>>)
					return { ValueItem(val,meta) };
				else if constexpr (std::is_arithmetic_v<T>)
					return (T)meta.vtype;
				else if constexpr (std::is_same_v<T, VType>)
					return meta.vtype;
				else
					throw InvalidCast("Fail cast type_identifier");
			case VType::function: {
				auto tmp = _Vcast_callFN(val);
				T res;
				try {
					res = std::move(Vcast<T>(tmp->val, tmp->meta));
				}
				catch (...) {
					delete tmp;
					throw;
				}
				delete tmp;
				return res;
			}
			default:
				throw InvalidCast("Fail cast undefined type");
			}
		}
	}
}





void DynSum(void** val0, void** val1);
void DynMinus(void** val0, void** val1);
void DynMul(void** val0, void** val1);
void DynDiv(void** val0, void** val1);
void DynRest(void** val0, void** val1);


void DynBitXor(void** val0, void** val1);
void DynBitOr(void** val0, void** val1);
void DynBitAnd(void** val0, void** val1);
void DynBitNot(void** val0);

void* AsArg(void** val);
void AsArr(void** val);

size_t getSize(void** value);

void asValue(void** val, VType type);
bool isValue(void** val, VType type);
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
