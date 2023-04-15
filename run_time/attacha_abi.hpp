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

bool needAlloc(ValueMeta type);
bool needAllocType(VType type);











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

template<class T>
constexpr VType Type_as_VType() {
	if constexpr (std::is_same<T, void>) return VType::noting;
	else if constexpr (std::is_same<T, bool>) return VType::boolean;
	else if constexpr (std::is_same<T, int8_t>) return VType::i8;
	else if constexpr (std::is_same<T, int16_t>) return VType::i16;
	else if constexpr (std::is_same<T, int32_t>) return VType::i32;
	else if constexpr (std::is_same<T, int64_t>) return VType::i64;
	else if constexpr (std::is_same<T, uint8_t>) return VType::ui8;
	else if constexpr (std::is_same<T, uint16_t>) return VType::ui16;
	else if constexpr (std::is_same<T, uint32_t>) return VType::ui32;
	else if constexpr (std::is_same<T, uint64_t>) return VType::ui64;
	else if constexpr (std::is_same<T, float>) return VType::flo;
	else if constexpr (std::is_same<T, double>) return VType::doub;
	else if constexpr (std::is_same<T, int8_t*>) return VType::raw_arr_i8;
	else if constexpr (std::is_same<T, int16_t*>) return VType::raw_arr_i16;
	else if constexpr (std::is_same<T, int32_t*>) return VType::raw_arr_i32;
	else if constexpr (std::is_same<T, int64_t*>) return VType::raw_arr_i64;
	else if constexpr (std::is_same<T, uint8_t*>) return VType::raw_arr_ui8;
	else if constexpr (std::is_same<T, uint16_t*>) return VType::raw_arr_ui16;
	else if constexpr (std::is_same<T, uint32_t*>) return VType::raw_arr_ui32;
	else if constexpr (std::is_same<T, uint64_t*>) return VType::raw_arr_ui64;
	else if constexpr (std::is_same<T, float*>) return VType::raw_arr_flo;
	else if constexpr (std::is_same<T, double*>) return VType::raw_arr_doub;
	else if constexpr (std::is_same<T, list_array<ValueItem>>) return VType::uarr;
	else if constexpr (std::is_same<T, std::string>) return VType::string;
	else if constexpr (std::is_same<T, typed_lgr<Task>>) return VType::async_res;
	else if constexpr (std::is_same<T, void*>) return VType::undefined_ptr;
	else if constexpr (std::is_same<T, std::exception_ptr*>) return VType::except_value;
	else if constexpr (std::is_same<T, ValueItem*>) return VType::faarr;
	else if constexpr (std::is_same<T, ClassValue>) return VType::class_;
	else if constexpr (std::is_same<T, MorphValue>) return VType::morph;
	else if constexpr (std::is_same<T, ProxyClass>) return VType::proxy;
	else if constexpr (std::is_same<T, ValueMeta>) return VType::type_identifier;
	else if constexpr (std::is_same<T, typed_lgr<class FuncEnviropment>>) return VType::function;
	else
		throw AttachARuntimeException("Invalid c++ type convert");
}


void universalFree(void** value, ValueMeta meta);
void universalRemove(void** value);
void universalAlloc(void** value, ValueMeta meta);

void initEnviropement(void** res, uint32_t vals_count);
void removeEnviropement(void** env, uint16_t vals_count);
char* getStrBegin(std::string* str);
void throwInvalidType();

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
bool is_raw_array(VType typ);
bool has_interface(VType typ);

//return equal,lower bool result
std::pair<bool, bool> compareValue(ValueMeta cmp1, ValueMeta cmp2, void* val1, void* val2);
RFLAGS compare(RFLAGS old, void** value_1, void** value_2);
RFLAGS link_compare(RFLAGS old, void** value_1, void** value_2);

void copyEnviropement(void** env, uint16_t env_it_count, void*** res);





namespace ABI_IMPL {
	ValueItem* _Vcast_callFN(void* ptr);
	template<class T, class A, std::is_convertible<A, T>::value>
	T* AsPointer(void* val) {
		return new T[] {(T)(A)val };
	}
	template<class T, class A>
	T* AsPointer(void* val) {
		throw InvalidCast("Try convert unconvertible types");
	}


	std::string Scast(void*& val, ValueMeta& meta);
	ValueItem SBcast(const std::string& str);
	template<class T = int8_t>
	ValueItem BVcast(const T& val) {
		if constexpr (std::is_same_v<std::remove_cvref_t<T>, nullptr_t>)
			return ValueItem();
		if constexpr (std::is_same_v<std::remove_cvref_t<T>, bool>)
			return ValueItem((void*)(0ull + val), VType::boolean);
		else if constexpr (std::is_same_v<std::remove_cvref_t<T>, int8_t>)
			return ValueItem((void*)(0ll + val), VType::i8);
		else if constexpr (std::is_same_v<std::remove_cvref_t<T>, uint8_t>)
			return ValueItem((void*)(0ull + val), VType::ui8);
		else if constexpr (std::is_same_v<std::remove_cvref_t<T>, int16_t>)
			return ValueItem((void*)(0ll + val), VType::i16);
		else if constexpr (std::is_same_v<std::remove_cvref_t<T>, uint16_t>)
			return ValueItem((void*)(0ull + val), VType::ui16);
		else if constexpr (std::is_same_v<std::remove_cvref_t<T>, int32_t>)
			return ValueItem((void*)(0ll + val), VType::i32);
		else if constexpr (std::is_same_v<std::remove_cvref_t<T>, uint32_t>)
			return ValueItem((void*)(0ull + val), VType::ui32);
		else if constexpr (std::is_same_v<std::remove_cvref_t<T>, int64_t>)
			return ValueItem((void*)val, VType::i64);
		else if constexpr (std::is_same_v<std::remove_cvref_t<T>, uint64_t>)
			return ValueItem((void*)val, VType::ui64);
		else if constexpr (std::is_same_v<std::remove_cvref_t<T>, float>)
			return ValueItem(*(void**)&val, VType::flo);
		else if constexpr (std::is_same_v<std::remove_cvref_t<T>, double>)
			return ValueItem(*(void**)&val, VType::doub);
		else if constexpr (std::is_same_v<std::remove_cvref_t<T>, std::string> || std::is_same_v<T, const char*>)
			return ValueItem(new std::string(val), VType::string);
		else if constexpr (std::is_same_v<std::remove_cvref_t<T>, list_array<ValueItem>>)
			return ValueItem(new list_array<ValueItem>(val), VType::uarr);
		else if constexpr (std::is_same_v<std::remove_cvref_t<T>, ValueMeta>)
			return ValueItem(*(void**)&val, VType::type_identifier);
		else if constexpr (std::is_same_v<std::remove_cvref_t<T>, ClassValue>)
			return ValueItem(new ClassValue(val), VType::class_);
		else if constexpr (std::is_same_v<std::remove_cvref_t<T>, MorphValue>)
			return ValueItem(new MorphValue(val), VType::morph);
		else if constexpr (std::is_same_v<std::remove_cvref_t<T>, ProxyClass>)
			return ValueItem(new ProxyClass(val), VType::proxy);
		else if constexpr (std::is_same_v<std::remove_cvref_t<T>, typed_lgr<class FuncEnviropment>>)
			return ValueItem(new typed_lgr<class FuncEnviropment>(val), VType::function);
		else if constexpr (std::is_same_v<std::remove_cvref_t<T>, ValueItem>)
			return val;
		else if constexpr (std::is_same_v<std::remove_cvref_t<T>, void*>)
			return val;
		else {
			static_assert(
				(
					std::is_arithmetic_v<std::remove_cvref_t<T>> ||
					std::is_same_v<std::remove_cvref_t<T>, std::string> ||
					std::is_same_v<std::remove_cvref_t<T>, ValueItem> ||
					std::is_same_v<std::remove_cvref_t<T>, ValueMeta> ||
					std::is_same_v<std::remove_cvref_t<T>, ClassValue> ||
					std::is_same_v<std::remove_cvref_t<T>, MorphValue> ||
					std::is_same_v<std::remove_cvref_t<T>, ProxyClass> ||
					std::is_same_v<std::remove_cvref_t<T>, ValueItem> ||
					std::is_same_v<std::remove_cvref_t<T>, list_array<ValueItem>> ||
					std::is_same_v<std::remove_cvref_t<T>, nullptr_t> ||
					std::is_same_v<std::remove_cvref_t<T>, bool> ||
					std::is_same_v<std::remove_cvref_t<T>, void*>
					),
				"Invalid type for convert"
				);
			throw CompileTimeException("Invalid compiler, use correct compiler for compile AttachA, //ignored static_assert//");
		}
	}
	template <class T>
	T Vcast(void*& ref_val, ValueMeta& meta) {
		getAsyncResult(ref_val, meta);
		void* val = getValue(ref_val, meta);
		if constexpr (std::is_same_v<T, std::string>) {
			return Scast(val, meta);
		}
		else {
			switch (meta.vtype) {
			case VType::noting:
				return T();
			case VType::boolean:
			case VType::i8: {
				if constexpr (std::is_same_v<T, void*>)
					return val;
				else if constexpr (std::is_same_v<T, list_array<ValueItem>>)
					return { ValueItem(val,meta) };
				else if constexpr (std::is_arithmetic_v<T>)
					return (T)(int8_t)val;
				else if constexpr (std::is_arithmetic_v<std::remove_pointer_t<T>>)
					return AsPointer<T, int8_t>(val);
				else
					return (T)val;
				break;
			}
			case VType::i16: {
				if constexpr (std::is_same_v<T, void*>)
					return val;
				else if constexpr (std::is_same_v<T, list_array<ValueItem>>)
					return { ValueItem(val,meta) };
				else if constexpr (std::is_arithmetic_v<T>)
					return (T)(int16_t)val;
				else if constexpr (std::is_arithmetic_v<std::remove_pointer_t<T>>)
					return AsPointer<T, int16_t>(val);
				else
					return (T)val;
				break;
			}
			case VType::i32: {
				if constexpr (std::is_same_v<T, void*>)
					return val;
				else if constexpr (std::is_same_v<T, list_array<ValueItem>>)
					return { ValueItem(val,meta) };
				else if constexpr (std::is_arithmetic_v<T>)
					return (T)(int32_t)val;
				else if constexpr (std::is_arithmetic_v<std::remove_pointer_t<T>>)
					return AsPointer<T, int32_t>(val);
				else
					return (T)val;
				break;
			}
			case VType::i64: {
				if constexpr (std::is_same_v<T, void*>)
					return val;
				else if constexpr (std::is_same_v<T, list_array<ValueItem>>)
					return { ValueItem(val,meta) };
				else if constexpr (std::is_arithmetic_v<T>)
					return (T)(int64_t)val;
				else if constexpr (std::is_arithmetic_v<std::remove_pointer_t<T>>)
					return AsPointer<T, int64_t>(val);
				else
					return (T)val;
				break;
			}
			case VType::ui8: {
				if constexpr (std::is_same_v<T, void*>)
					return val;
				else if constexpr (std::is_same_v<T, list_array<ValueItem>>)
					return { ValueItem(val,meta) };
				else if constexpr (std::is_arithmetic_v<T>)
					return (T)(uint8_t)val;
				else if constexpr (std::is_arithmetic_v<std::remove_pointer_t<T>>)
					return AsPointer<T, uint8_t>(val);
				else
					return (T)val;
				break;
			}
			case VType::ui16: {
				if constexpr (std::is_same_v<T, void*>)
					return val;
				else if constexpr (std::is_same_v<T, list_array<ValueItem>>)
					return { ValueItem(val,meta) };
				else if constexpr (std::is_arithmetic_v<T>)
					return (T)(uint16_t)val;
				else if constexpr (std::is_arithmetic_v<std::remove_pointer_t<T>>)
					return AsPointer<T, uint16_t>(val);
				else
					return (T)val;
				break;
			}
			case VType::ui32: {
				if constexpr (std::is_same_v<T, void*>)
					return val;
				else if constexpr (std::is_same_v<T, list_array<ValueItem>>)
					return { ValueItem(val,meta) };
				else if constexpr (std::is_arithmetic_v<T>)
					return (T)(uint32_t)val;
				else if constexpr (std::is_arithmetic_v<std::remove_pointer_t<T>>)
					return AsPointer<T, uint32_t>(val);
				else
					return (T)val;
				break;
			}
			case VType::ui64: {
				if constexpr (std::is_same_v<T, void*>)
					return val;
				else if constexpr (std::is_same_v<T, list_array<ValueItem>>)
					return { ValueItem(val,meta) };
				else if constexpr (std::is_arithmetic_v<T>)
					return (T)(uint64_t)val;
				else if constexpr (std::is_arithmetic_v<std::remove_pointer_t<T>>)
					return AsPointer<T, uint64_t>(val);
				else
					return (T)val;
				break;
			}
			case VType::flo: {
				if constexpr (std::is_same_v<T, void*>)
					return val;
				else if constexpr (std::is_same_v<T, list_array<ValueItem>>)
					return { ValueItem(val,meta) };
				else if constexpr (std::is_arithmetic_v<T>)
					return (T) * (float*)&val;
				else if constexpr (std::is_arithmetic_v<std::remove_pointer_t<T>>)
					return AsPointer<T, float>(val);
				else
					return (T)val;
				break;
			}
			case VType::doub: {
				if constexpr (std::is_same_v<T, void*>)
					return val;
				else if constexpr (std::is_same_v<T, list_array<ValueItem>>)
					return { ValueItem(val,meta) };
				else if constexpr (std::is_arithmetic_v<T>)
					return (T) * (double*)&val;
				else if constexpr (std::is_arithmetic_v<std::remove_pointer_t<T>>)
					return AsPointer<T, double>(val);
				else
					return (T)val;
				break;
			}
			case VType::raw_arr_i8: {
				if constexpr (std::is_same_v<T, void*>)
					return val;
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
				break;
			}
			case VType::raw_arr_i16: {
				if constexpr (std::is_same_v<T, void*>)
					return val;
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
				break;
			}
			case VType::raw_arr_i32: {
				if constexpr (std::is_same_v<T, void*>)
					return val;
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
				break;
			}
			case VType::raw_arr_i64: {
				if constexpr (std::is_same_v<T, void*>)
					return val;
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
				break;
			}
			case VType::raw_arr_ui8: {
				if constexpr (std::is_same_v<T, void*>)
					return val;
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
				break;
			}
			case VType::raw_arr_ui16: {
				if constexpr (std::is_same_v<T, void*>)
					return val;
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
				break;
			}
			case VType::raw_arr_ui32: {
				if constexpr (std::is_same_v<T, void*>)
					return val;
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
				break;
			}
			case VType::raw_arr_ui64: {
				if constexpr (std::is_same_v<T, void*>)
					return val;
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
				break;
			}
			case VType::raw_arr_flo: {
				if constexpr (std::is_same_v<T, void*>)
					return val;
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
							res[i] = ValueItem(*(void**)&reinterpret_cast<float*>(val)[i], VType::flo);
						return res;
					}
					else return { ValueItem(val,meta) };
				}
				else throw InvalidCast("Fail cast raw_arr_flo");
				break;
			}
			case VType::raw_arr_doub: {
				if constexpr (std::is_same_v<T, void*>)
					return val;
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
							res[i] = ValueItem(*(void**)&(reinterpret_cast<double*>(val)[i]), VType::doub);
						return res;
					}
					else return { ValueItem(val,meta) };
				}
				else throw InvalidCast("Fail cast raw_arr_doub");
				break;
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
						return (T)tmp;
					}
				}
				else if constexpr (std::is_arithmetic_v<std::remove_pointer_t<T>>) {
					if (meta.val_len) {
						list_array<ValueItem>& ref = reinterpret_cast<list_array<ValueItem>&>(val);
						std::remove_pointer_t<T>* res = new std::remove_pointer_t<T>[meta.val_len];
						for (uint32_t i = 0; i < meta.val_len; i++) {
							ValueItem& tmp = ref[i];
							res[i] = (std::remove_pointer_t<T>)tmp;
						}
						return res;
					}
					else throw InvalidCast("Fail cast uarr");
				}
				else if constexpr (std::is_same_v<T, list_array<ValueItem>>) {
					return reinterpret_cast<list_array<ValueItem>&>(val);
				}
				else throw InvalidCast("Fail cast uarr");
				break;
			}
			case VType::string: {
				if constexpr (std::is_same_v<T, void*>)
					return val;
				else {
					ValueItem tmp = SBcast(reinterpret_cast<const std::string&>(val));
					if (tmp.meta.vtype == VType::string) {
						if constexpr (std::is_same_v<T, list_array<ValueItem>>)
							return { ValueItem(val,meta) };
						else throw InvalidCast("Fail cast string");
					}
					else
						return (T)tmp;
				}
				break;
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
				break;
			}
			case VType::type_identifier: {
				if constexpr (std::is_same_v<T, list_array<ValueItem>>)
					return { ValueItem(val,meta) };
				else if constexpr (std::is_arithmetic_v<T>)
					return (T)meta.vtype;
				else if constexpr (std::is_same_v<T, VType>)
					return meta.vtype;
				else
					throw InvalidCast("Fail cast type_identifier");
				break;
			}
			case VType::faarr: {
				if constexpr (std::is_same_v<T, list_array<ValueItem>>)
					return list_array<ValueItem>((ValueItem*)val, ((ValueItem*)val) + meta.val_len);
				else if constexpr (std::is_arithmetic_v<T>)
					return (T)meta.vtype;
				else if constexpr (std::is_same_v<T, VType>)
					return meta.vtype;
				else
					throw InvalidCast("Fail cast type_identifier");
				break;
			}
			case VType::class_: {
				if constexpr (std::is_same_v<T, ClassValue>)
					return (ClassValue&)val;
				else if constexpr (std::is_same_v<T, void*>) {
					return val;
				}
				else {
					ValueItem tmp(val, meta, no_copy);
					ValueItem* res = ((ClassValue&)val).callFnPtr("()", ClassAccess::pub)->syncWrapper(&tmp,1);
					tmp.val = 0;
					ValueItem m(std::move(*res));
					delete res;
					return (T)m;
				}
				break;
			}
			case VType::morph: {
				if constexpr (std::is_same_v<T, MorphValue>)
					return (MorphValue&)val;
				else if constexpr (std::is_same_v<T, void*>) {
					return val;
				}
				else {
					ValueItem tmp(val, meta, no_copy);
					ValueItem* res = ((MorphValue&)val).callFnPtr("()", ClassAccess::pub)->syncWrapper(&tmp,1);
					tmp.val = 0;
					ValueItem m(std::move(*res));
					delete res;
					return Vcast<T>(m.val, m.meta);
				}
				break;
			}
			case VType::proxy: {
				if constexpr (std::is_same_v<T, ClassValue>)
					return (ClassValue&)val;
				else if constexpr (std::is_same_v<T, void*>) {
					return val;
				}
				else {
					ValueItem tmp(val, meta, no_copy);
					ValueItem* res = ((ProxyClass&)val).callFnPtr("()", ClassAccess::pub)->syncWrapper(&tmp,1);
					ValueItem m(std::move(*res));
					delete res;
					return Vcast<T>(m.val, m.meta);
				}
				break;
			}
			case VType::function: {
				if constexpr (std::is_same_v<T, void*>) {
					return val;
				}
				else {
					auto tmp = _Vcast_callFN(val);
					T res;
					try {
						res = (T)*tmp;
					}
					catch (...) {
						delete tmp;
						throw;
					}
					delete tmp;
					return res;
				}
				break;
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
void DynBitShiftLeft(void** val0, void** val1);
void DynBitShiftRight(void** val0, void** val1);


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
			return func(std::forward(args)...);
		}
		catch (...) {
			try {
				return new ValueItem(new std::exception_ptr(std::current_exception()), VType::except_value, no_copy);
			}
			catch (const std::bad_alloc& ex) {
				throw EnviropmentRuinException();
			}
		}
	}
}
