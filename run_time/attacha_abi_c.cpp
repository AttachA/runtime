// Copyright Danyil Melnytskyi 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#include "AttachA_CXX.hpp"
#include <string>
#include <sstream>
#include <mutex>



bool needAlloc(ValueMeta type) {
	if (type.use_gc)
		return true;
	return needAllocType(type.vtype);
}

bool needAllocType(VType type) {
	switch (type) {
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
	case VType::uarr:
	case VType::string:
	case VType::async_res:
	case VType::except_value:
	case VType::faarr:
	case VType::class_:
	case VType::morph:
	case VType::proxy:
	case VType::function:
		return true;
	default:
		return false;
	}
}










bool calc_safe_deph_arr(void* ptr) {
	list_array<ValueItem>& items = *(list_array<ValueItem>*)ptr;
	for (ValueItem& it : items)
		if (it.meta.use_gc)
			if (!((lgr*)it.val)->depth_safety())
				return false;
	return true;
}



void universalFree(void** value, ValueMeta meta) {
	if (!value)
		return;
	if (!*value)
		return;
	if (meta.use_gc)
		goto gc_destruct;
	switch (meta.vtype) {
	case VType::raw_arr_i8:
	case VType::raw_arr_ui8:
		delete[](uint8_t*)* value;
		return;
	case VType::raw_arr_i16:
	case VType::raw_arr_ui16:
		delete[](uint16_t*)* value;
		return;
	case VType::raw_arr_i32:
	case VType::raw_arr_ui32:
	case VType::raw_arr_flo:
		delete[](uint32_t*)* value;
		return;
	case VType::raw_arr_i64:
	case VType::raw_arr_ui64:
	case VType::raw_arr_doub:
		delete[](uint64_t*)* value;
		return;
	case VType::uarr:
		delete (list_array<ValueItem>*)* value;
		return;
	case VType::string:
		delete (std::string*)*value;
		return;
	case VType::async_res:
		delete (typed_lgr<Task>*)* value;
		return;
	case VType::undefined_ptr:
		return;
	case VType::except_value:
		delete (std::exception_ptr*)*value;
		return;
	case VType::faarr:
		delete[](ValueItem*)* value;
		return;
	case VType::class_:
		delete (ClassValue*)*value;
		return;
	case VType::morph:
		delete (MorphValue*)*value;
		return;
	case VType::proxy:
		delete (ProxyClass*)*value;
		return;
	case VType::function:
		delete (typed_lgr<FuncEnviropment>*)*value;
		return;
	default:
		delete *value;
	}
	return;
gc_destruct:
	delete *(lgr**)value;
}
void universalRemove(void** value) {
	ValueMeta& meta = *reinterpret_cast<ValueMeta*>(value + 1);
	if (!value)
		return;
	if (!*value)
		return;
	if (!meta.encoded)
		return;
	if(!meta.as_ref)
		if (needAlloc(meta))
			universalFree(value, meta);
	if (meta.vtype == VType::saarr) {
		ValueItem* begin = (ValueItem*)*value;
		uint32_t count = meta.val_len;
		while (count--)
			universalRemove((void**)begin++);
	}
	meta.encoded = 0;
	*value = nullptr;
}
void universalAlloc(void** value, ValueMeta meta) {
	if (*value)
		universalRemove(value);
	if (needAllocType(meta.vtype)) {
		switch (meta.vtype) {
		case VType::raw_arr_i8:
		case VType::raw_arr_ui8:
			*value = new uint8_t[meta.val_len];
			break;
		case VType::raw_arr_i16:
		case VType::raw_arr_ui16:
			*value = new uint16_t[meta.val_len];
			break;
		case VType::raw_arr_i32:
		case VType::raw_arr_ui32:
		case VType::raw_arr_flo:
			*value = new uint32_t[meta.val_len];
			break;
		case VType::raw_arr_ui64:
		case VType::raw_arr_i64:
		case VType::raw_arr_doub:
			*value = new uint64_t[meta.val_len];
			break;
		case VType::uarr:
			*value = new list_array<ValueItem>();
			break;
		case VType::string:
			*value = new std::string();
			break;
		case VType::except_value:
			try {
				throw AException("Undefined exception", "No description");
			}
			catch (...) {
				*value = new std::exception_ptr(std::current_exception());
				break;
			}
		case VType::faarr:
			*value = new ValueItem[meta.val_len]();
			break;
		case VType::saarr:
			throw InvalidOperation("Fail alocate local stack value");
			break;
		case VType::class_:
			*value = new ClassValue();
			break;
		case VType::morph:
			*value = new MorphValue();
			break;
		}
	}
	if (meta.use_gc) {
		void(*destructor)(void*) = nullptr;
		bool(*deph)(void*) = nullptr;
		switch (meta.vtype)
		{
		case VType::noting:
			break;
		case VType::i8:
		case VType::ui8:
			destructor = defaultDestructor<uint8_t>;
			*value = new uint8_t(0);
			break;
		case VType::i16:
		case VType::ui16:
			destructor = defaultDestructor<uint16_t>;
			*value = new uint16_t(0);
			break;
		case VType::i32:
		case VType::ui32:
		case VType::flo:
			destructor = defaultDestructor<uint32_t>;
			*value = new uint32_t(0);
			break;
		case VType::i64:
		case VType::ui64:
		case VType::undefined_ptr:
		case VType::doub:
		case VType::type_identifier:
			destructor = defaultDestructor<uint64_t>;
			*value = new uint64_t(0);
			break;
		case VType::raw_arr_i8:
		case VType::raw_arr_ui8:
			destructor = arrayDestructor<uint8_t>;
			break;
		case VType::raw_arr_i16:
		case VType::raw_arr_ui16:
			destructor = arrayDestructor<uint16_t>;
			break;
		case VType::raw_arr_i32:
		case VType::raw_arr_ui32:
		case VType::raw_arr_flo:
			destructor = arrayDestructor<uint32_t>;
			break;
		case VType::raw_arr_i64:
		case VType::raw_arr_ui64:
		case VType::raw_arr_doub:
			destructor = arrayDestructor<uint64_t>;
			break;
		case VType::uarr:
			destructor = defaultDestructor<list_array<ValueItem>>;
			deph = calc_safe_deph_arr;
			break;
		case VType::string:
			destructor = defaultDestructor<std::string>;
			break;
		case VType::except_value:
			destructor = defaultDestructor<std::exception_ptr>;
			break;
		case VType::faarr:
			destructor = arrayDestructor<ValueItem>;
			break;
		case VType::saarr:
			break;
		case VType::class_:
			destructor = defaultDestructor<ClassValue>;
			break;
		case VType::morph:
			destructor = defaultDestructor<MorphValue>;
			break;
		default:
			break;
		}
		*value = new lgr(value, deph, destructor);
	}
	*(value + 1) = (void*)meta.encoded;
}

void initEnviropement(void** res, uint32_t vals_count) {
	for (uint32_t i = 0; i < vals_count; i++)
		res[i] = nullptr;
}
void removeEnviropement(void** env, uint16_t vals_count) {
	uint32_t max_vals = uint32_t(vals_count) << 1;
	for (uint32_t i = 0; i < max_vals; i += 2)
		universalRemove(env + i);
}
void removeArgsEnviropement(list_array<ValueItem>* env) {
	delete env;
}
char* getStrBegin(std::string* str) {
	return &(str->operator[](0));
}
void throwInvalidType() {
	throw InvalidType("Requested specifed type but recuived another");
}
ValueItem* getAsyncValueItem(void* val) {
	typed_lgr<Task>& tmp = *(typed_lgr<Task>*)val;
	return Task::get_result(tmp);
}
void getValueItem(void** value, ValueItem* f_res) {
	universalRemove(value);
	if (f_res) {
		*value = f_res->val;
		*(value + 1) = (void*)f_res->meta.encoded;
		f_res->val = nullptr;
		f_res->meta = 0;
		delete f_res;
	}
}
ValueItem* buildRes(void** value) {
	ValueItem* res;
	try {
		res = new ValueItem();
	}
	catch (const std::bad_alloc& ex)
	{
		throw EnviropmentRuinException();
	}
	res->val = *value;
	res->meta = (size_t)*(value + 1);
	*value = *(value + 1) = nullptr;
	return res;
}


void getAsyncResult(void*& value, ValueMeta& meta) {
	while (meta.vtype == VType::async_res) {
		ValueItem* vaal = (ValueItem*)&value;
		ValueItem* res = nullptr;
		try {
			auto res = getAsyncValueItem(meta.use_gc ? ((lgr*)value)->getPtr() : value);
			*vaal = std::move(*res);
		}
		catch (...) {
			if (res)
				delete res;
		}
		if (res)
			delete res;
	}
}
void* copyValue(void*& val, ValueMeta& meta) {
	getAsyncResult(val, meta);

	void* actual_val = val;
	if (meta.use_gc)
		actual_val = ((lgr*)val)->getPtr();
	if (needAllocType(meta.vtype)) {
		switch (meta.vtype) {
		case VType::raw_arr_i8:
		case VType::raw_arr_ui8: {
			uint8_t* cop = new uint8_t[meta.val_len];
			memcpy(cop, actual_val, meta.val_len);
			return cop;
		}
		case VType::raw_arr_i16:
		case VType::raw_arr_ui16: {
			uint16_t* cop = new uint16_t[meta.val_len];
			memcpy(cop, actual_val, size_t(meta.val_len) * 2);
			return cop;
		}
		case VType::raw_arr_i32:
		case VType::raw_arr_ui32:
		case VType::raw_arr_flo: {
			uint32_t* cop = new uint32_t[meta.val_len];
			memcpy(cop, actual_val, size_t(meta.val_len) * 4);
			return cop;
		}
		case VType::raw_arr_i64:
		case VType::raw_arr_ui64:
		case VType::raw_arr_doub: {
			uint64_t* cop = new uint64_t[meta.val_len];
			memcpy(cop, actual_val, size_t(meta.val_len) * 8);
			return cop;
		}
		case VType::uarr:
			return new list_array<ValueItem>(*(list_array<ValueItem>*)actual_val);
		case VType::string:
			return new std::string(*(std::string*)actual_val);
		case VType::async_res:
			return new typed_lgr<Task>(*(typed_lgr<Task>*)actual_val);
		case VType::except_value:
			return new std::exception_ptr(*(std::exception_ptr*)actual_val);
		case VType::faarr: {
			ValueItem* cop = new ValueItem[meta.val_len]();
			for (uint32_t i = 0; i < meta.val_len; i++)
				cop[i] = reinterpret_cast<ValueItem*>(val)[i];
			return cop;
		}
		case VType::class_:
			return new ClassValue(*(ClassValue*)actual_val);
		case VType::morph:
			return new MorphValue(*(MorphValue*)actual_val);
		case VType::proxy:
			return new ProxyClass(*(ProxyClass*)actual_val);
		case VType::function:
			return new typed_lgr<FuncEnviropment>(*(typed_lgr<FuncEnviropment>*)actual_val);
		default:
			throw NotImplementedException();
		}
	}
	else return actual_val;
}


void** preSetValue(void** value, ValueMeta set_meta, bool match_gc_dif) {
	void** res = getValueLink(value);
	ValueMeta& meta = *(ValueMeta*)(value + 1);
	if (!needAlloc(meta)) {
		if (match_gc_dif) {
			if (meta.allow_edit && meta.vtype == set_meta.vtype && meta.use_gc == set_meta.use_gc)
				return res;
		}
		else
			if (meta.allow_edit && meta.vtype == set_meta.vtype)
				return res;
	}
	if(!meta.as_ref)
		universalRemove(value);
	*(value + 1) = (void*)set_meta.encoded;
	return getValueLink(value);
}
void*& getValue(void*& value, ValueMeta& meta) {
	if (meta.vtype == VType::async_res)
		getAsyncResult(value, meta);
	if (meta.use_gc)
		if (((lgr*)value)->is_deleted()) {
			universalRemove(&value);
			meta = ValueMeta(0);
		}
	return meta.use_gc ? (**(lgr*)value) : value;
}
void*& getValue(void** value) {
	ValueMeta& meta = *(ValueMeta*)(value + 1);
	if (meta.vtype == VType::async_res)
		getAsyncResult(*value, meta);
	if (meta.use_gc)
		if (((lgr*)value)->is_deleted()) {
			universalRemove(value);
			meta = 0;
		}
	return meta.use_gc ? (**(lgr*)value) : *value;
}
void* getSpecificValue(void** value, VType typ) {
	ValueMeta& meta = *(ValueMeta*)(value + 1);
	if (meta.vtype == VType::async_res)
		getAsyncResult(*value, meta);
	if (meta.vtype != typ)
		throw InvalidType("Requested specifed type but recuived another");
	if(meta.use_gc)
		if (((lgr*)value)->is_deleted()) {
			universalRemove(value);
			meta = ValueMeta(0);
		}
	return meta.use_gc ? ((lgr*)value)->getPtr() : *value;
}
void** getSpecificValueLink(void** value, VType typ) {
	ValueMeta& meta = *(ValueMeta*)(value + 1);
	if (meta.vtype == VType::async_res)
		getAsyncResult(*value, meta);
	if (meta.vtype != typ)
		throw InvalidType("Requested specifed type but recuived another");
	if (meta.use_gc)
		if (((lgr*)value)->is_deleted()) {
			universalFree(value, meta);
			meta = ValueMeta(0);
		}
	return meta.use_gc ? (&**(lgr*)value) : value;
}
void** getValueLink(void** value) {
	return &getValue(value);
}

bool is_integer(VType typ) {
	switch (typ)
	{
	case VType::i8:
	case VType::i16:
	case VType::i32:
	case VType::i64:
	case VType::ui8:
	case VType::ui16:
	case VType::ui32:
	case VType::ui64:
	case VType::flo:
	case VType::doub:
		return true;
	default:
		return false;
	}
}
bool integer_unsigned(VType typ) {
	switch (typ)
	{
	case VType::ui8:
	case VType::ui16:
	case VType::ui32:
	case VType::ui64:
		return true;
	default:
		return false;
	}
}
bool is_raw_array(VType typ) {
	switch (typ)
	{
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
	case VType::faarr:
	case VType::saarr:
		return true;
	default:
		return false;
	}
}
bool has_interface(VType typ) {
	switch (typ) {
	case VType::class_:
	case VType::morph:
	case VType::proxy:
		return true;
	default:
		return false;
	}
}
#pragma warning(push)  
#pragma warning( disable: 4311)
#pragma warning( disable: 4302)



std::pair<bool, bool> compareArrays(ValueMeta cmp1, ValueMeta cmp2, void* val1, void* val2) {
	if (!calc_safe_deph_arr(val1))
		return { false,false };
	else if (!calc_safe_deph_arr(val2))
		return { false,false };
	else {
		auto& arr1 = *(list_array<ValueItem>*)val1;
		auto& arr2 = *(list_array<ValueItem>*)val2;
		if (arr1.size() < arr2.size())
			return { false, true };
		else if (arr1.size() == arr2.size()) {
			auto tmp = arr2.begin();
			for (auto& it : arr1) {
				auto res = compareValue(it.meta, tmp->meta, it.val, tmp->val);
				if (!res.first)
					return res;
				++tmp;
			}
			return { true, false };
		}
		else return { false, false };
	}
}
//uarr and raw_arr_* or faarr/saarr
std::pair<bool, bool> compareUarrARawArr(ValueMeta cmp1, ValueMeta cmp2, void* val1, void* val2, bool flip_args = false) {
	if (!calc_safe_deph_arr(val1))
		return { false,false };
	else {
		auto& arr1 = *(list_array<ValueItem>*)val1;
		if (arr1.size() < cmp2.val_len)
			return { false, true };
		else if (arr1.size() == cmp2.val_len) {
			switch (cmp2.vtype) {
			case VType::raw_arr_i8: {
				auto arr2 = (int8_t*)val2;
				for (auto& it : arr1) {
					auto first = *arr2;
					auto res = compareValue(it.meta, VType::i8, it.val, &first);
					if (!res.first)
						return flip_args ? std::pair<bool, bool>{false, res.second} : res;
					++arr2;
				}
				break;
			}
			case VType::raw_arr_i16: {
				auto arr2 = (int16_t*)val2;
				for (auto& it : arr1) {
					auto first = *arr2;
					auto res = compareValue(it.meta, VType::i16, it.val, &first);
					if (!res.first)
						return flip_args ? std::pair<bool, bool>{false, res.second} : res;
					++arr2;
				}
				break;
			}
			case VType::raw_arr_i32: {
				auto arr2 = (int32_t*)val2;
				for (auto& it : arr1) {
					auto first = *arr2;
					auto res = compareValue(it.meta, VType::i32, it.val, &first);
					if (!res.first)
						return flip_args ? std::pair<bool, bool>{false, res.second} : res;
					++arr2;
				}
				break;
			}
			case VType::raw_arr_i64: {
				auto arr2 = (int64_t*)val2;
				for (auto& it : arr1) {
					auto first = *arr2;
					auto res = compareValue(it.meta, VType::i64, it.val, &first);
					if (!res.first)
						return flip_args ? std::pair<bool, bool>{false, res.second} : res;
					++arr2;
				}
				break;
			}
			case VType::raw_arr_ui8: {
				auto arr2 = (uint8_t*)val2;
				for (auto& it : arr1) {
					auto first = *arr2;
					auto res = compareValue(it.meta, VType::ui8, it.val, &first);
					if (!res.first)
						return flip_args ? std::pair<bool, bool>{false, res.second} : res;
					++arr2;
				}
				break;
			}
			case VType::raw_arr_ui16: {
				auto arr2 = (uint16_t*)val2;
				for (auto& it : arr1) {
					auto first = *arr2;
					auto res = compareValue(it.meta, VType::ui16, it.val, &first);
					if (!res.first)
						return flip_args ? std::pair<bool, bool>{false, res.second} : res;
					++arr2;
				}
				break;
			}
			case VType::raw_arr_ui32: {
				auto arr2 = (uint32_t*)val2;
				for (auto& it : arr1) {
					auto first = *arr2;
					auto res = compareValue(it.meta, VType::ui32, it.val, &first);
					if (!res.first)
						return flip_args ? std::pair<bool, bool>{false, res.second} : res;
					++arr2;
				}
				break;
			}
			case VType::raw_arr_ui64: {
				auto arr2 = (uint64_t*)val2;
				for (auto& it : arr1) {
					auto first = *arr2;
					auto res = compareValue(it.meta, VType::ui64, it.val, &first);
					if (!res.first)
						return flip_args ? std::pair<bool, bool>{false, res.second} : res;
					++arr2;
				}
				break;
			}
			case VType::raw_arr_flo: {
				auto arr2 = (float*)val2;
				for (auto& it : arr1) {
					auto first = *arr2;
					auto res = compareValue(it.meta, VType::flo, it.val, &first);
					if (!res.first)
						return flip_args ? std::pair<bool, bool>{false, res.second} : res;
					++arr2;
				}
				break;
			}
			case VType::raw_arr_doub: {
				auto arr2 = (double*)val2;
				for (auto& it : arr1) {
					auto first = *arr2;
					auto res = compareValue(it.meta, VType::doub, it.val, &first);
					if (!res.first)
						return flip_args ? std::pair<bool, bool>{false, res.second} : res;
					++arr2;
				}
				break;
			}
			case VType::faarr:
			case VType::saarr: {
				auto arr2 = (ValueItem*)val2;
				for (auto& it : arr1) {
					auto res = compareValue(it.meta, arr2->meta, it.val, arr2->val);
					if (!res.first)
						return flip_args ? std::pair<bool, bool>{false, res.second} : res;
					++arr2;
				}
				break;
			}
			default:
				throw InvalidOperation("Wrong compare operation, notify devs via github, reason: used function for compare uarr and raw_arr_* but second operand is actualy " + enum_to_string(cmp2.vtype));
			}
			return { true, false };
		}
		else return { false, false };
	}
}
//uarr and raw_arr_* or faarr/saarr
std::pair<bool, bool> compareUarrAInterface(ValueMeta cmp1, ValueMeta cmp2, void* val1, void* val2, bool flip_args = false) {
	if (!calc_safe_deph_arr(val1))
		return { false,false };
	else {
		auto& arr1 = *(list_array<ValueItem>*)val1;
		uint64_t length;  
		switch (cmp2.vtype) {
		case VType::class_:
			length = (uint64_t)AttachA::Interface::makeCall(ClassAccess::priv,*(ClassValue*)val2,"size");
			break;
		case VType::morph:
			length = (uint64_t)AttachA::Interface::makeCall(ClassAccess::priv, *(MorphValue*)val2, "size");
			break;
		case VType::proxy:
			length = (uint64_t)AttachA::Interface::makeCall(ClassAccess::priv, *(ProxyClass*)val2, "size");
			break;
		default:
			throw AException("Implementation exception", "Wrong function usage compareUarrAInterface");
		}
		if (arr1.size() < length)
			return { false, true };
		else if (arr1.size() == length) {
			auto iter = arr1.begin(); 
			if ((*(ProxyClass*)val2).containsFn("begin")) {
				auto iter2 = AttachA::Interface::makeCall(ClassAccess::priv, *(ProxyClass*)val2, "begin");
				for (uint64_t i = 0; i < length; i++) {
					auto& it1 = *iter;
					auto it2 = AttachA::Interface::makeCall(ClassAccess::priv, iter2, "next");
					auto res = compareValue(it1.meta, it2.meta, it1.val, it2.val);
					if (!res.first)
						return flip_args ? std::pair<bool, bool>{false, res.second} : res;
					++iter;
				}
			}
			else if ((*(ProxyClass*)val2).containsFn("index")) {
				for (uint64_t i = 0; i < length; i++) {
					auto& it1 = *iter;
					auto it2 = AttachA::Interface::makeCall(ClassAccess::priv, *(ProxyClass*)val2, "index", i);
					auto res = compareValue(it1.meta, it2.meta, it1.val, it2.val);
					if (!res.first)
						return flip_args ? std::pair<bool, bool>{false, res.second} : res;
					++iter;
				}
			}
			else
				throw NotImplementedException();

			return { true, false };
		}
		else return { false, false };
	}
}
template<class T,VType T_VType>
std::tuple<bool, bool, bool> compareRawArrAInterface_Worst0(void* val1, void* val2,uint64_t length) {
	auto arr2 = (T*)val2;
	if ((*(ProxyClass*)val2).containsFn("begin")) {
		auto iter2 = AttachA::Interface::makeCall(ClassAccess::priv, *(ProxyClass*)val2, "begin");
		for (uint64_t i = 0; i < length; i++) {
			T first = *arr2;
			ValueItem it2 = AttachA::Interface::makeCall(ClassAccess::priv, iter2, "next");
			auto res = compareValue(T_VType, it2.meta, &first, it2.val);
			if (!res.first)
				return { false, res.second,true };
			++arr2;
		}
	}
	else if ((*(ProxyClass*)val2).containsFn("index")) {
		for (uint64_t i = 0; i < length; i++) {
			T first = *arr2;
			ValueItem it2 = AttachA::Interface::makeCall(ClassAccess::priv, *(ProxyClass*)val2, "index", i);
			auto res = compareValue(T_VType, it2.meta, &first, it2.val);
			if (!res.first)
				return { false, res.second,true };
			++arr2;
		}
	}
	else
		throw NotImplementedException();
	return { false,false,false };
}
std::tuple<bool, bool, bool> compareRawArrAInterface_Worst1(void* val1, void* val2, uint64_t length) {
	auto arr2 = (ValueItem*)val2;
	if ((*(ProxyClass*)val2).containsFn("begin")) {
		auto iter2 = AttachA::Interface::makeCall(ClassAccess::priv, *(ProxyClass*)val2, "begin");
		for (uint64_t i = 0; i < length; i++) {
			ValueItem& first = *arr2;
			ValueItem it2 = AttachA::Interface::makeCall(ClassAccess::priv, iter2, "next");
			auto res = compareValue(first.meta, it2.meta, first.val, it2.val);
			if (!res.first)
				return { false, res.second,true };
			++arr2;
		}
	}
	else if ((*(ProxyClass*)val2).containsFn("index")) {
		for (uint64_t i = 0; i < length; i++) {
			ValueItem& first = *arr2;
			ValueItem it2 = AttachA::Interface::makeCall(ClassAccess::priv, *(ProxyClass*)val2, "index", i);
			auto res = compareValue(first.meta, it2.meta, first.val, it2.val);
			if (!res.first)
				return { false, res.second,true };
			++arr2;
		}
	}
	else
		throw NotImplementedException();
	return { false,false,false };
}
std::pair<bool, bool> compareRawArrAInterface(ValueMeta cmp1, ValueMeta cmp2, void* val1, void* val2, bool flip_args = false) {
	if (!calc_safe_deph_arr(val1))
		return { false,false };
	else {
		auto& arr1 = *(list_array<ValueItem>*)val1;
		uint64_t length;
		switch (cmp2.vtype) {
		case VType::class_:
			length = (uint64_t)AttachA::Interface::makeCall(ClassAccess::priv, *(ClassValue*)val2, "size");
			break;
		case VType::morph:
			length = (uint64_t)AttachA::Interface::makeCall(ClassAccess::priv, *(MorphValue*)val2, "size");
			break;
		case VType::proxy:
			length = (uint64_t)AttachA::Interface::makeCall(ClassAccess::priv, *(ProxyClass*)val2, "size");
			break;
		default:
			throw AException("Implementation exception", "Wrong function usage compareRawArrAInterface");
		}
		if (arr1.size() < length)
			return { false, true };
		else if (arr1.size() == length) {
			std::tuple<bool, bool, bool> res;
			switch (cmp2.vtype) {
			case VType::raw_arr_i8: 
				res = compareRawArrAInterface_Worst0<int8_t, VType::i8>(val1, val2, length);
				break;
			case VType::raw_arr_i16:
				res = compareRawArrAInterface_Worst0<int16_t, VType::i16>(val1, val2, length);
				break;
			case VType::raw_arr_i32:
				res = compareRawArrAInterface_Worst0<int32_t, VType::i32>(val1, val2, length);
				break;
			case VType::raw_arr_i64:
				res = compareRawArrAInterface_Worst0<int64_t, VType::i64>(val1, val2, length);
				break;
			case VType::raw_arr_ui8:
				res = compareRawArrAInterface_Worst0<uint8_t, VType::ui8>(val1, val2, length);
				break;
			case VType::raw_arr_ui16:
				res = compareRawArrAInterface_Worst0<uint16_t, VType::ui16>(val1, val2, length);
				break;
			case VType::raw_arr_ui32:
				res = compareRawArrAInterface_Worst0<uint32_t, VType::ui32>(val1, val2, length);
				break;
			case VType::raw_arr_ui64:
				res = compareRawArrAInterface_Worst0<uint64_t, VType::ui64>(val1, val2, length);
				break;
			case VType::raw_arr_flo:
				res = compareRawArrAInterface_Worst0<float, VType::flo>(val1, val2, length);
				break;
			case VType::raw_arr_doub:
				res = compareRawArrAInterface_Worst0<double, VType::doub>(val1, val2, length);
				break;
			case VType::faarr:
			case VType::saarr:
				res = compareRawArrAInterface_Worst1(val1, val2, length);
				break;
			default:
				throw InvalidOperation("Wrong compare operation, notify devs via github, reason: used function for compare uarr and raw_arr_* but second operand is actualy " + enum_to_string(cmp2.vtype));
			}
			auto& [eq, low, has_res] = res;
			if (has_res)
				return { eq,low };
			else
				return { true, false };
		}
		else return { false, false };
	}
}
//return equal,lower bool result
std::pair<bool, bool> compareValue(ValueMeta cmp1, ValueMeta cmp2, void* val1, void* val2) {
	bool cmp_int;
	if (cmp_int = (is_integer(cmp1.vtype) || cmp1.vtype == VType::undefined_ptr) != (is_integer(cmp2.vtype) || cmp1.vtype == VType::undefined_ptr))
		return { false,false };

	if (cmp_int) {
		if (val1 == val2)
			return { true,false };

		bool temp1 = (integer_unsigned(cmp1.vtype) || cmp1.vtype == VType::undefined_ptr);
		bool temp2 = (integer_unsigned(cmp2.vtype) || cmp2.vtype == VType::undefined_ptr);
		if (temp1 && temp2)
			return { false, uint64_t(val1) < uint64_t(val2) };
		else if (temp1)
			switch (cmp2.vtype) {
			case VType::i8:
				return { false, uint64_t(val1) < int8_t(val2) };
			case VType::i16:
				return { false, uint64_t(val1) < int16_t(val2) };
			case VType::i32:
				return { false, uint64_t(val1) < int32_t(val2) };
			case VType::i64:
				return { false, uint64_t(val1) < int64_t(val2) };
			case VType::flo:
				return { false, uint64_t(val1) < *(float*)&val2 };
			case VType::doub:
				return { false, uint64_t(val1) < *(double*)&val2 };
			}
		else if (temp2)
			switch (cmp1.vtype) {
			case VType::i8:
				return { false, int8_t(val1) < uint64_t(val2) };
			case VType::i16:
				return { false, int16_t(val1) < uint64_t(val2) };
			case VType::i32:
				return { false, int32_t(val1) < uint64_t(val2) };
			case VType::i64:
				return { false, int64_t(val1) < uint64_t(val2) };
			case VType::flo:
				return { false, *(float*)&val1 < uint64_t(val2) };
			case VType::doub:
				return { false, *(double*)&val1 < uint64_t(val2) };
			}
		else
			switch (cmp1.vtype) {
			case VType::i8:
				switch (cmp2.vtype) {
				case VType::i8:
					return { false, int8_t(val1) < int8_t(val2) };
				case VType::i16:
					return { false, int8_t(val1) < int16_t(val2) };
				case VType::i32:
					return { false, int8_t(val1) < int32_t(val2) };
				case VType::i64:
					return { false, int8_t(val1) < int64_t(val2) };
				case VType::flo:
					return { false, int8_t(val1) < *(float*)&(val2) };
				case VType::doub:
					return { false, int8_t(val1) < *(double*)&(val2) };
				}
				break;
			case VType::i16:
				switch (cmp2.vtype) {
				case VType::i8:
					return { false, int16_t(val1) < int8_t(val2) };
				case VType::i16:
					return { false, int16_t(val1) < int16_t(val2) };
				case VType::i32:
					return { false, int16_t(val1) < int32_t(val2) };
				case VType::i64:
					return { false, int16_t(val1) < int64_t(val2) };
				case VType::flo:
					return { false, int16_t(val1) < *(float*)&(val2) };
				case VType::doub:
					return { false, int16_t(val1) < *(double*)&(val2) };
				}
				break;
			case VType::i32:
				switch (cmp2.vtype) {
				case VType::i8:
					return { false, int32_t(val1) < int8_t(val2) };
				case VType::i16:
					return { false, int32_t(val1) < int16_t(val2) };
				case VType::i32:
					return { false, int32_t(val1) < int32_t(val2) };
				case VType::i64:
					return { false, int32_t(val1) < int64_t(val2) };
				case VType::flo:
					return { false, int32_t(val1) < *(float*)&(val2) };
				case VType::doub:
					return { false, int32_t(val1) < *(double*)&(val2) };
				}
				break;
			case VType::i64:
				switch (cmp2.vtype) {
				case VType::i8:
					return { false, int64_t(val1) < int8_t(val2) };
				case VType::i16:
					return { false, int64_t(val1) < int16_t(val2) };
				case VType::i32:
					return { false, int64_t(val1) < int32_t(val2) };
				case VType::i64:
					return { false, int64_t(val1) < int64_t(val2) };
				case VType::flo:
					return { false, int64_t(val1) < *(float*)&(val2) };
				case VType::doub:
					return { false, int64_t(val1) < *(double*)&(val2) };
				}
				break;
			case VType::flo:
				switch (cmp2.vtype) {
				case VType::i8:
					return { false, *(float*)&val1 < int8_t(val2) };
				case VType::i16:
					return { false, *(float*)&val1 < int16_t(val2) };
				case VType::i32:
					return { false, *(float*)&val1 < int32_t(val2) };
				case VType::i64:
					return { false, *(float*)&val1 < int64_t(val2) };
				case VType::flo:
					return { false, *(float*)&val1 < *(float*)&(val2) };
				case VType::doub:
					return { false, *(float*)&val1 < *(double*)&(val2) };
				}
				break;
			case VType::doub:
				switch (cmp2.vtype) {
				case VType::i8:
					return { false, *(double*)&val1 < int8_t(val2) };
				case VType::i16:
					return { false, *(double*)&val1 < int16_t(val2) };
				case VType::i32:
					return { false, *(double*)&val1 < int32_t(val2) };
				case VType::i64:
					return { false, *(double*)&val1 < int64_t(val2) };
				case VType::flo:
					return { false, *(double*)&val1 < *(float*)&(val2) };
				case VType::doub:
					return { false, *(double*)&val1 < *(double*)&(val2) };
				}
			}
		return { false, false };
	}
	else if (cmp1.vtype == VType::string && cmp2.vtype == VType::string) {
		if (*(std::string*)val1 == *(std::string*)val2)
			return { true, false };
		else
			return { false, *(std::string*)val1 < *(std::string*)val2 };
	}
	else if (cmp1.vtype == VType::uarr && cmp2.vtype == VType::uarr) return compareArrays(cmp1, cmp2, val1, val2);
	else if (cmp1.vtype == VType::uarr && is_raw_array(cmp2.vtype)) return compareUarrARawArr(cmp1, cmp2, val1, val2);
	else if (is_raw_array(cmp1.vtype) && cmp2.vtype == VType::uarr) return compareUarrARawArr(cmp2, cmp1, val2, val1, true);
	else if (cmp1.vtype == VType::uarr && has_interface(cmp2.vtype)) return compareUarrAInterface(cmp1, cmp2, val1, val2);
	else if (has_interface(cmp1.vtype) && cmp2.vtype == VType::uarr) return compareUarrAInterface(cmp2, cmp1, val2, val1, true);
	else if (is_raw_array(cmp1.vtype) && has_interface(cmp2.vtype)) return compareRawArrAInterface(cmp1, cmp2, val1, val2);
	else if (has_interface(cmp1.vtype) && is_raw_array(cmp2.vtype)) return compareRawArrAInterface(cmp2, cmp1, val2, val1, true);
	else
		return { cmp1.vtype == cmp2.vtype, false };
}
RFLAGS compare(RFLAGS old, void** value_1, void** value_2) {
	void* val1 = getValue(value_1);
	void* val2 = getValue(value_2);
	ValueMeta cmp1 = *(ValueMeta*)(value_1 + 1);
	ValueMeta cmp2 = *(ValueMeta*)(value_2 + 1);

	old.parity = old.auxiliary_carry = old.sign_f = old.overflow = 0;
	auto res = compareValue(cmp1, cmp2, val1, val2);
	old.zero = res.first;
	old.carry = res.second;
	return old;
}
RFLAGS link_compare(RFLAGS old, void** value_1, void** value_2) {
	void* val1 = getValue(value_1);
	void* val2 = getValue(value_2);
	ValueMeta cmp1 = *(ValueMeta*)(value_1 + 1);
	ValueMeta cmp2 = *(ValueMeta*)(value_2 + 1);

	old.parity = old.auxiliary_carry = old.sign_f = old.overflow = 0;
	if (*value_1 == *value_2) { 
		old.zero = true;
		old.carry = false;
	}
	else {
		old.zero = false;
		old.carry = uint64_t(val1) < uint64_t(val2);
	}
	return old;
}







namespace ABI_IMPL {

	ValueItem* _Vcast_callFN(void* ptr) {
		return FuncEnviropment::sync_call(*(class typed_lgr<class FuncEnviropment>*)ptr, nullptr, 0);
	}

	std::string Scast(void*& ref_val, ValueMeta& meta) {
		void* val = getValue(ref_val, meta);
		switch (meta.vtype) {
		case VType::noting:
			return "null";
		case VType::raw_arr_i8: {
			std::string res = "*[";
			for (uint32_t i = 0; i < meta.val_len; i++)
				res += std::to_string(reinterpret_cast<int8_t*>(val)[i]) + (i + 1 < meta.val_len ? ',' : ']');
			if (!meta.val_len)
				res += ']';
			return res;
		}
		case VType::raw_arr_i16: {
			std::string res = "*[";
			for (uint32_t i = 0; i < meta.val_len; i++)
				res += std::to_string(reinterpret_cast<int16_t*>(val)[i]) + (i + 1 < meta.val_len ? ',' : ']');
			if (!meta.val_len)
				res += ']';
			return res;
		}
		case VType::raw_arr_i32: {
			std::string res = "*[";
			for (uint32_t i = 0; i < meta.val_len; i++)
				res += std::to_string(reinterpret_cast<int32_t*>(val)[i]) + (i + 1 < meta.val_len ? ',' : ']');
			if (!meta.val_len)
				res += ']';
			return res;
		}
		case VType::raw_arr_i64: {
			std::string res = "*[";
			for (uint32_t i = 0; i < meta.val_len; i++)
				res += std::to_string(reinterpret_cast<int64_t*>(val)[i]) + (i + 1 < meta.val_len ? ',' : ']');
			if (!meta.val_len)
				res += ']';
			return res;
		}
		case VType::raw_arr_ui8: {
			std::string res = "*[";
			for (uint32_t i = 0; i < meta.val_len; i++)
				res += std::to_string(reinterpret_cast<uint8_t*>(val)[i]) + (i + 1 < meta.val_len ? ',' : ']');
			if (!meta.val_len)
				res += ']';
			return res;
		}
		case VType::raw_arr_ui16: {
			std::string res = "*[";
			for (uint32_t i = 0; i < meta.val_len; i++)
				res += std::to_string(reinterpret_cast<uint16_t*>(val)[i]) + (i + 1 < meta.val_len ? ',' : ']');
			if (!meta.val_len)
				res += ']';
			return res;
		}
		case VType::raw_arr_ui32: {
			std::string res = "*[";
			for (uint32_t i = 0; i < meta.val_len; i++)
				res += std::to_string(reinterpret_cast<uint32_t*>(val)[i]) + (i + 1 < meta.val_len ? ',' : ']');
			if (!meta.val_len)
				res += ']';
			return res;
		}
		case VType::raw_arr_ui64: {
			std::string res = "*[";
			for (uint32_t i = 0; i < meta.val_len; i++)
				res += std::to_string(reinterpret_cast<uint64_t*>(val)[i]) + (i + 1 < meta.val_len ? ',' : ']');
			if (!meta.val_len)
				res += ']';
			return res;
		}
		case VType::raw_arr_flo: {
			std::string res = "*[";
			for (uint32_t i = 0; i < meta.val_len; i++)
				res += std::to_string(reinterpret_cast<float*>(val)[i]) + (i + 1 < meta.val_len ? ',' : ']');
			if (!meta.val_len)
				res += ']';
			return res;
		}
		case VType::raw_arr_doub: {
			std::string res = "*[";
			for (uint32_t i = 0; i < meta.val_len; i++)
				res += std::to_string(reinterpret_cast<double*>(val)[i]) + (i + 1 < meta.val_len ? ',' : ']');
			if (!meta.val_len)
				res += ']';
			return res;
		}
		case VType::i8:
			return std::to_string(reinterpret_cast<int8_t&>(val));
		case VType::i16:
			return std::to_string(reinterpret_cast<int16_t&>(val));
			break;
		case VType::i32:
			return std::to_string(reinterpret_cast<int32_t&>(val));
			break;
		case VType::i64:
			return std::to_string(reinterpret_cast<int64_t&>(val));
			break;
		case VType::ui8:
			return std::to_string(reinterpret_cast<uint8_t&>(val));
			break;
		case VType::ui16:
			return std::to_string(reinterpret_cast<uint16_t&>(val));
			break;
		case VType::ui32:
			return std::to_string(reinterpret_cast<uint32_t&>(val));
			break;
		case VType::ui64:
			return std::to_string(reinterpret_cast<uint64_t&>(val));
			break;
		case VType::flo:
			return std::to_string(reinterpret_cast<float&>(val));
			break;
		case VType::doub:
			return std::to_string(reinterpret_cast<double&>(val));
			break;
		case VType::uarr: {
			std::string res("[");
			bool before = false;
			for (auto& it : *reinterpret_cast<list_array<ValueItem>*>(val)) {
				if (before)
					res += ',';
				res += Scast(it.val, it.meta);
				before = true;
			}
			res += ']';
			return res;
		}
		case VType::string:
			return *(std::string*)val;
			break;
		case VType::undefined_ptr:
			return "0x" + (std::ostringstream() << val).str();
			break;
		default:
			throw InvalidCast("Fail cast undefined type");
		}
	}
	ValueItem SBcast(const std::string& str) {
		if (str == "null")
			return ValueItem();
		else if (str.starts_with("0x"))
			return ValueItem((void*)std::stoull(str, nullptr, 16),ValueMeta(VType::undefined_ptr,false,true));
		else if (str.starts_with('[')) {
			//TO-DO
			return ValueItem(new std::string(str), ValueMeta(VType::string, false, true));
		}else if(str.starts_with("*[")) {
			//TO-DO
			return ValueItem(new std::string(str), ValueMeta(VType::string, false, true));
		}
		else {
			try {
				try {
					try {
						try {
							try {
								try {
									int32_t res = std::stoi(str);
									return ValueItem(*(void**)&res, ValueMeta(VType::i32, false, true));
								}
								catch (...) {
									uint32_t res = std::stoul(str);
									return ValueItem(*(void**)&res, ValueMeta(VType::ui32, false, true));
								}
							}
							catch (...) {
								int64_t res = std::stoll(str);
								return ValueItem(*(void**)&res, ValueMeta(VType::i64, false, true));
							}
						}
						catch (...) {
							uint64_t res = std::stoull(str);
							return ValueItem(*(void**)&res, ValueMeta(VType::ui64, false, true));
						}
					}
					catch (...) {
						float res = std::stof(str);
						return ValueItem(*(void**)&res, ValueMeta(VType::flo, false, true));
					}
				}
				catch (...) {
					double res = std::stod(str);
					return ValueItem(*(void**)&res, ValueMeta(VType::doub, false, true));
				}
			}
			catch (...) {
				return ValueItem(new std::string(str), ValueMeta(VType::string, false, true));
			}
		}
	}
	template<class T>
	void setValue(const T& val, void** set_val) {
		universalRemove(set_val);
		if constexpr (std::is_same_v<T, int8_t*>) {
			*reinterpret_cast<int8_t**>(set_val) = val;
			*reinterpret_cast<ValueMeta*>(set_val + 1) = ValueMeta(VType::i8, false, true);
		}
		else if constexpr (std::is_same_v<T, uint8_t*>) {
			*reinterpret_cast<uint8_t**>(set_val) = val;
			*reinterpret_cast<ValueMeta*>(set_val + 1) = ValueMeta(VType::ui8, false, true);
		}
		else if constexpr (std::is_same_v<T, int16_t*>) {
			*reinterpret_cast<int16_t**>(set_val) = val;
			*reinterpret_cast<ValueMeta*>(set_val + 1) = ValueMeta(VType::i16, false, true);
		}
		else if constexpr (std::is_same_v<T, uint16_t*>) {
			*reinterpret_cast<uint16_t**>(set_val) = val;
			*reinterpret_cast<ValueMeta*>(set_val + 1) = ValueMeta(VType::ui16, false, true);
		}
		else if constexpr (std::is_same_v<T, int32_t*>) {
			*reinterpret_cast<int32_t**>(set_val) = val;
			*reinterpret_cast<ValueMeta*>(set_val + 1) = ValueMeta(VType::i32, false, true);
		}
		else if constexpr (std::is_same_v<T, uint32_t*>) {
			*reinterpret_cast<uint32_t**>(set_val) = val;
			*reinterpret_cast<ValueMeta*>(set_val + 1) = ValueMeta(VType::ui32, false, true);
		}
		else if constexpr (std::is_same_v<T, int64_t**>) {
			*reinterpret_cast<int64_t**>(set_val) = val;
			*reinterpret_cast<ValueMeta*>(set_val + 1) = ValueMeta(VType::i64, false, true);
		}
		else if constexpr (std::is_same_v<T, uint64_t*>) {
			*reinterpret_cast<uint64_t**>(set_val) = val;
			*reinterpret_cast<ValueMeta*>(set_val + 1) = ValueMeta(VType::ui64, false, true);
		}
		else if constexpr (std::is_same_v < T, bool> ) {
			*reinterpret_cast<bool*>(set_val) = val;
			*reinterpret_cast<ValueMeta*>(set_val + 1) = ValueMeta(VType::boolean, false, true);
		}
		else if constexpr (std::is_same_v<T, int8_t>) {
			*reinterpret_cast<int8_t*>(set_val) = val;
			*reinterpret_cast<ValueMeta*>(set_val + 1) = ValueMeta(VType::i8, false, true);
		}
		else if constexpr (std::is_same_v<T, uint8_t>) {
			*reinterpret_cast<uint8_t*>(set_val) = val;
			*reinterpret_cast<ValueMeta*>(set_val + 1) = ValueMeta(VType::ui8, false, true);
		}
		else if constexpr (std::is_same_v<T, int16_t>) {
			*reinterpret_cast<int16_t*>(set_val) = val;
			*reinterpret_cast<ValueMeta*>(set_val + 1) = ValueMeta(VType::i16, false, true);
		}
		else if constexpr (std::is_same_v<T, uint16_t>) {
			*reinterpret_cast<uint16_t*>(set_val) = val;
			*reinterpret_cast<ValueMeta*>(set_val + 1) = ValueMeta(VType::ui16, false, true);
		}
		else if constexpr (std::is_same_v<T, int32_t>) {
			*reinterpret_cast<int32_t*>(set_val) = val;
			*reinterpret_cast<ValueMeta*>(set_val + 1) = ValueMeta(VType::i32, false, true);
		}
		else if constexpr (std::is_same_v<T, uint32_t>) {
			*reinterpret_cast<uint32_t*>(set_val) = val;
			*reinterpret_cast<ValueMeta*>(set_val + 1) = ValueMeta(VType::ui32, false, true);
		}
		else if constexpr (std::is_same_v<T, int64_t>) {
			*reinterpret_cast<int64_t*>(set_val) = val;
			*reinterpret_cast<ValueMeta*>(set_val + 1) = ValueMeta(VType::i64, false, true);
		}
		else if constexpr (std::is_same_v<T, uint64_t>) {
			*reinterpret_cast<uint64_t*>(set_val) = val;
			*reinterpret_cast<ValueMeta*>(set_val + 1) = ValueMeta(VType::ui64, false, true);
		}
		else if constexpr (std::is_same_v<T, float>) {
			*reinterpret_cast<float*>(set_val) = val;
			*reinterpret_cast<ValueMeta*>(set_val + 1) = ValueMeta(VType::flo, false, true);
		}
		else if constexpr (std::is_same_v<T, double>) {
			*reinterpret_cast<double*>(set_val) = val;
			*reinterpret_cast<ValueMeta*>(set_val + 1) = ValueMeta(VType::doub, false, true);
		}
		else if constexpr (std::is_same_v<T, std::string>) {
			*reinterpret_cast<std::string**>(set_val) = new std::string(val);
			*reinterpret_cast<ValueMeta*>(set_val + 1) = ValueMeta(VType::string, false, true);
		}
		else if constexpr (std::is_same_v<T, list_array<ValueItem>>) {
			*reinterpret_cast<list_array<ValueItem>**>(set_val) = new list_array<ValueItem >(val);
			*reinterpret_cast<ValueMeta*>(set_val + 1) = ValueMeta(VType::uarr, false, true);
		}
		else if constexpr (std::is_same_v<T, void*>) {
			*reinterpret_cast<void**>(set_val) = val;
			*reinterpret_cast<ValueMeta*>(set_val + 1) = ValueMeta(VType::undefined_ptr, false, true);
		}
		else
			throw NotImplementedException();
	}
}





void DynSum(void** val0, void** val1) {
	ValueMeta& val0_meta = *((ValueMeta*)val0 + 1);
	ValueMeta& val1_meta = *((ValueMeta*)val1 + 1);
	void*& actual_val0 = *val0;
	void*& actual_val1 = *val1;
	if(val0_meta.vtype == VType::async_res) getAsyncResult(actual_val0, val0_meta);
	if(val1_meta.vtype == VType::async_res) getAsyncResult(actual_val1, val1_meta);

	if (!val0_meta.allow_edit)
		throw UnmodifabeValue();

	switch (val0_meta.vtype) {
	case VType::noting: {
		actual_val0 = copyValue(actual_val1, val1_meta);
		val0_meta = val1_meta;
		break;
	}
	case VType::i8:
		reinterpret_cast<int8_t&>(actual_val0) += ABI_IMPL::Vcast<int8_t>(actual_val1, val1_meta);
		break;
	case VType::i16:
		reinterpret_cast<int16_t&>(actual_val0) += ABI_IMPL::Vcast<int16_t>(actual_val1, val1_meta);
		break;
	case VType::i32:
		reinterpret_cast<int32_t&>(actual_val0) += ABI_IMPL::Vcast<int32_t>(actual_val1, val1_meta);
		break;
	case VType::i64:
		reinterpret_cast<int64_t&>(actual_val0) += ABI_IMPL::Vcast<int64_t>(actual_val1, val1_meta);
		break;
	case VType::ui8:
		reinterpret_cast<uint8_t&>(actual_val0) += ABI_IMPL::Vcast<uint8_t>(actual_val1, val1_meta);
		break;
	case VType::ui16:
		reinterpret_cast<uint16_t&>(actual_val0) += ABI_IMPL::Vcast<uint16_t>(actual_val1, val1_meta);
		break;
	case VType::ui32:
		reinterpret_cast<uint32_t&>(actual_val0) += ABI_IMPL::Vcast<uint32_t>(actual_val1, val1_meta);
		break;
	case VType::ui64:
		reinterpret_cast<uint64_t&>(actual_val0) += ABI_IMPL::Vcast<uint64_t>(actual_val1, val1_meta);
		break;
	case VType::flo:
		reinterpret_cast<float&>(actual_val0) += ABI_IMPL::Vcast<float>(actual_val1, val1_meta);
		break;
	case VType::doub:
		reinterpret_cast<double&>(actual_val0) += ABI_IMPL::Vcast<double>(actual_val1, val1_meta);
		break;
	case VType::uarr:
		reinterpret_cast<list_array<ValueItem>&>(actual_val0).push_back(ValueItem(copyValue(actual_val1,val1_meta), val1_meta));
		break;
	case VType::string:
		reinterpret_cast<std::string&>(actual_val0) += ABI_IMPL::Scast(actual_val1, val1_meta);
		break;
	case VType::undefined_ptr:
		reinterpret_cast<size_t&>(actual_val0) += ABI_IMPL::Vcast<size_t>(actual_val1, val1_meta);
		break;
	default:
		throw InvalidCast("Fail cast value for add operation, cause value type is undefined");
	}
}
void DynMinus(void** val0, void** val1) {
	ValueMeta& val0_meta = *((ValueMeta*)val0 + 1);
	ValueMeta& val1_meta = *((ValueMeta*)val1 + 1);
	void*& actual_val0 = *val0;
	void*& actual_val1 = *val1;
	if(val0_meta.vtype == VType::async_res) getAsyncResult(actual_val0, val0_meta);
	if(val1_meta.vtype == VType::async_res) getAsyncResult(actual_val1, val1_meta);

	if (!val0_meta.allow_edit)
		throw UnmodifabeValue();

	switch (val0_meta.vtype) {
	case VType::noting: {
		actual_val0 = copyValue(actual_val1, val1_meta);
		val0_meta = val1_meta;
		break;
	}
	case VType::i8:
		reinterpret_cast<int8_t&>(actual_val0) -= ABI_IMPL::Vcast<int8_t>(actual_val1, val1_meta);
		break;
	case VType::i16:
		reinterpret_cast<int16_t&>(actual_val0) -= ABI_IMPL::Vcast<int16_t>(actual_val1, val1_meta);
		break;
	case VType::i32:
		reinterpret_cast<int32_t&>(actual_val0) -= ABI_IMPL::Vcast<int32_t>(actual_val1, val1_meta);
		break;
	case VType::i64:
		reinterpret_cast<int64_t&>(actual_val0) -= ABI_IMPL::Vcast<int64_t>(actual_val1, val1_meta);
		break;
	case VType::ui8:
		reinterpret_cast<uint8_t&>(actual_val0) -= ABI_IMPL::Vcast<uint8_t>(actual_val1, val1_meta);
		break;
	case VType::ui16:
		reinterpret_cast<uint16_t&>(actual_val0) -= ABI_IMPL::Vcast<uint16_t>(actual_val1, val1_meta);
		break;
	case VType::ui32:
		reinterpret_cast<uint32_t&>(actual_val0) -= ABI_IMPL::Vcast<uint32_t>(actual_val1, val1_meta);
		break;
	case VType::ui64:
		reinterpret_cast<uint64_t&>(actual_val0) -= ABI_IMPL::Vcast<uint64_t>(actual_val1, val1_meta);
		break;
	case VType::flo:
		reinterpret_cast<float&>(actual_val0) -= ABI_IMPL::Vcast<float>(actual_val1, val1_meta);
		break;
	case VType::doub:
		reinterpret_cast<double&>(actual_val0) -= ABI_IMPL::Vcast<double>(actual_val1, val1_meta);
		break;
	case VType::uarr:
		reinterpret_cast<list_array<ValueItem>&>(actual_val0).push_front(ValueItem(copyValue(actual_val1,val1_meta), val1_meta));
		break;
	case VType::string:
		reinterpret_cast<std::string&>(actual_val0) = ABI_IMPL::Scast(actual_val1, val1_meta) + reinterpret_cast<std::string&>(actual_val0);
		break;
	case VType::undefined_ptr:
		reinterpret_cast<size_t&>(actual_val0) -= ABI_IMPL::Vcast<size_t>(actual_val1, val1_meta);
		break;
	default:
		throw InvalidCast("Fail cast value for minus operation, cause value type is undefined");
	}
}
void DynMul(void** val0, void** val1) {
	ValueMeta& val0_meta = *((ValueMeta*)val0 + 1);
	ValueMeta& val1_meta = *((ValueMeta*)val1 + 1);
	void*& actual_val0 = *val0;
	void*& actual_val1 = *val1;
	if(val0_meta.vtype == VType::async_res) getAsyncResult(actual_val0, val0_meta);
	if(val1_meta.vtype == VType::async_res) getAsyncResult(actual_val1, val1_meta);

	if (!val0_meta.allow_edit)
		throw UnmodifabeValue();

	switch (val0_meta.vtype) {
	case VType::noting: {
		actual_val0 = copyValue(actual_val1, val1_meta);
		val0_meta = val1_meta;
		break;
	}
	case VType::i8:
		reinterpret_cast<int8_t&>(actual_val0) *= ABI_IMPL::Vcast<int8_t>(actual_val1, val1_meta);
		break;
	case VType::i16:
		reinterpret_cast<int16_t&>(actual_val0) *= ABI_IMPL::Vcast<int16_t>(actual_val1, val1_meta);
		break;
	case VType::i32:
		reinterpret_cast<int32_t&>(actual_val0) *= ABI_IMPL::Vcast<int32_t>(actual_val1, val1_meta);
		break;
	case VType::i64:
		reinterpret_cast<int64_t&>(actual_val0) *= ABI_IMPL::Vcast<int64_t>(actual_val1, val1_meta);
		break;
	case VType::ui8:
		reinterpret_cast<uint8_t&>(actual_val0) *= ABI_IMPL::Vcast<uint8_t>(actual_val1, val1_meta);
		break;
	case VType::ui16:
		reinterpret_cast<uint16_t&>(actual_val0) *= ABI_IMPL::Vcast<uint16_t>(actual_val1, val1_meta);
		break;
	case VType::ui32:
		reinterpret_cast<uint32_t&>(actual_val0) *= ABI_IMPL::Vcast<uint32_t>(actual_val1, val1_meta);
		break;
	case VType::ui64:
		reinterpret_cast<uint64_t&>(actual_val0) *= ABI_IMPL::Vcast<uint64_t>(actual_val1, val1_meta);
		break;
	case VType::flo:
		reinterpret_cast<float&>(actual_val0) *= ABI_IMPL::Vcast<float>(actual_val1, val1_meta);
		break;
	case VType::doub:
		reinterpret_cast<double&>(actual_val0) *= ABI_IMPL::Vcast<double>(actual_val1, val1_meta);
		break;
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
	case VType::uarr:
		if (val1_meta.vtype == VType::uarr)
			reinterpret_cast<list_array<ValueItem>&>(actual_val0).insert(reinterpret_cast<list_array<ValueItem>&>(actual_val0).size() - 1, reinterpret_cast<list_array<ValueItem>&>(actual_val1));
		else
			reinterpret_cast<list_array<ValueItem>&>(actual_val0).push_back(ValueItem(copyValue(actual_val1, val1_meta), val1_meta));
		break;
	case VType::string:
		throw InvalidOperation("for strings multiply operation is not defined");
	case VType::undefined_ptr:
		reinterpret_cast<size_t&>(actual_val0) *= ABI_IMPL::Vcast<size_t>(actual_val1, val1_meta);
		break;
	case VType::faarr:
	case VType::saarr: {




		break;
	}
	case VType::class_:
		AttachA::Interface::makeCall(ClassAccess::priv, reinterpret_cast<ClassValue&>(actual_val0), "operator *", reinterpret_cast<ValueItem&>(val1));
		break;
	case VType::morph:
		AttachA::Interface::makeCall(ClassAccess::priv, reinterpret_cast<MorphValue&>(actual_val0), "operator *", reinterpret_cast<ValueItem&>(val1));
		break;
	case VType::proxy:
		AttachA::Interface::makeCall(ClassAccess::priv, reinterpret_cast<ProxyClass&>(actual_val0), "operator *", reinterpret_cast<ValueItem&>(val1));
		break;
	case VType::type_identifier:
		throw InvalidCast("Fail use value for mul operation, cause value type is type_identifier");
	case VType::function:
		throw InvalidCast("Fail use value for mul operation, cause value type is function");
		break;
	default:
		throw InvalidCast("Fail cast value for mul operation, cause value type is undefined");
	}
}
void DynDiv(void** val0, void** val1) {
	ValueMeta& val0_meta = *((ValueMeta*)val0 + 1);
	ValueMeta& val1_meta = *((ValueMeta*)val1 + 1);
	void*& actual_val0 = *val0;
	void*& actual_val1 = *val1;
	if (val0_meta.vtype == VType::async_res) getAsyncResult(actual_val0, val0_meta);
	if (val1_meta.vtype == VType::async_res) getAsyncResult(actual_val1, val1_meta);

	if (!val0_meta.allow_edit)
		throw UnmodifabeValue();

	switch (val0_meta.vtype) {
	case VType::noting: {
		actual_val0 = copyValue(actual_val1, val1_meta);
		val0_meta = val1_meta;
		break;
	}
	case VType::i8:
		reinterpret_cast<int8_t&>(actual_val0) /= ABI_IMPL::Vcast<int8_t>(actual_val1, val1_meta);
		break;
	case VType::i16:
		reinterpret_cast<int16_t&>(actual_val0) /= ABI_IMPL::Vcast<int16_t>(actual_val1, val1_meta);
		break;
	case VType::i32:
		reinterpret_cast<int32_t&>(actual_val0) /= ABI_IMPL::Vcast<int32_t>(actual_val1, val1_meta);
		break;
	case VType::i64:
		reinterpret_cast<int64_t&>(actual_val0) /= ABI_IMPL::Vcast<int64_t>(actual_val1, val1_meta);
		break;
	case VType::ui8:
		reinterpret_cast<uint8_t&>(actual_val0) /= ABI_IMPL::Vcast<uint8_t>(actual_val1, val1_meta);
		break;
	case VType::ui16:
		reinterpret_cast<uint16_t&>(actual_val0) /= ABI_IMPL::Vcast<uint16_t>(actual_val1, val1_meta);
		break;
	case VType::ui32:
		reinterpret_cast<uint32_t&>(actual_val0) /= ABI_IMPL::Vcast<uint32_t>(actual_val1, val1_meta);
		break;
	case VType::ui64:
		reinterpret_cast<uint64_t&>(actual_val0) /= ABI_IMPL::Vcast<uint64_t>(actual_val1, val1_meta);
		break;
	case VType::flo:
		reinterpret_cast<float&>(actual_val0) /= ABI_IMPL::Vcast<float>(actual_val1, val1_meta);
		break;
	case VType::doub:
		reinterpret_cast<double&>(actual_val0) /= ABI_IMPL::Vcast<double>(actual_val1, val1_meta);
		break;
	case VType::uarr:
		if (val1_meta.vtype == VType::uarr)
			reinterpret_cast<list_array<ValueItem>&>(actual_val0).push_front(reinterpret_cast<list_array<ValueItem>&>(actual_val1));
		else
			reinterpret_cast<list_array<ValueItem>&>(actual_val0).push_front(ValueItem(copyValue(actual_val1,val1_meta), val1_meta));
		break;
	case VType::string:
		throw InvalidOperation("for strings divide operation is not defined");
	case VType::undefined_ptr:
		reinterpret_cast<size_t&>(actual_val0) /= ABI_IMPL::Vcast<size_t>(actual_val1, val1_meta);
		break;
	default:
		throw InvalidCast("Fail cast value for div operation, cause value type is undefined");
	}
}
void DynRest(void** val0, void** val1) {
	ValueMeta& val0_meta = *((ValueMeta*)val0 + 1);
	ValueMeta& val1_meta = *((ValueMeta*)val1 + 1);
	void*& actual_val0 = *val0;
	void*& actual_val1 = *val1;
	if (val0_meta.vtype == VType::async_res) getAsyncResult(actual_val0, val0_meta);
	if (val1_meta.vtype == VType::async_res) getAsyncResult(actual_val1, val1_meta);

	if (!val0_meta.allow_edit)
		throw UnmodifabeValue();

	switch (val0_meta.vtype) {
	case VType::noting: {
		actual_val0 = copyValue(actual_val1, val1_meta);
		val0_meta = val1_meta;
		break;
	}
	case VType::i8:
		reinterpret_cast<int8_t&>(actual_val0) %= ABI_IMPL::Vcast<int8_t>(actual_val1, val1_meta);
		break;
	case VType::i16:
		reinterpret_cast<int16_t&>(actual_val0) %= ABI_IMPL::Vcast<int16_t>(actual_val1, val1_meta);
		break;
	case VType::i32:
		reinterpret_cast<int32_t&>(actual_val0) %= ABI_IMPL::Vcast<int32_t>(actual_val1, val1_meta);
		break;
	case VType::i64:
		reinterpret_cast<int64_t&>(actual_val0) %= ABI_IMPL::Vcast<int64_t>(actual_val1, val1_meta);
		break;
	case VType::ui8:
		reinterpret_cast<uint8_t&>(actual_val0) %= ABI_IMPL::Vcast<uint8_t>(actual_val1, val1_meta);
		break;
	case VType::ui16:
		reinterpret_cast<uint16_t&>(actual_val0) %= ABI_IMPL::Vcast<uint16_t>(actual_val1, val1_meta);
		break;
	case VType::ui32:
		reinterpret_cast<uint32_t&>(actual_val0) %= ABI_IMPL::Vcast<uint32_t>(actual_val1, val1_meta);
		break;
	case VType::ui64:
		reinterpret_cast<uint64_t&>(actual_val0) %= ABI_IMPL::Vcast<uint64_t>(actual_val1, val1_meta);
		break;
	case VType::flo:
		reinterpret_cast<float&>(actual_val0) = std::fmod(reinterpret_cast<float&>(actual_val0),ABI_IMPL::Vcast<float>(actual_val1, val1_meta));
		break;
	case VType::doub:
		reinterpret_cast<double&>(actual_val0) = std::fmod(reinterpret_cast<double&>(actual_val0), ABI_IMPL::Vcast<double>(actual_val1, val1_meta));
		break;
	case VType::uarr:
		if (val1_meta.vtype == VType::uarr)
			reinterpret_cast<list_array<ValueItem>&>(actual_val0).push_back(reinterpret_cast<list_array<ValueItem>&>(actual_val1));
		else
			reinterpret_cast<list_array<ValueItem>&>(actual_val0).push_back(ValueItem(copyValue(actual_val1, val1_meta), val1_meta));
		break;
	case VType::string:
		throw InvalidOperation("for strings divide operation is not defined");
	case VType::undefined_ptr:
		reinterpret_cast<size_t&>(actual_val0) %= ABI_IMPL::Vcast<size_t>(actual_val1, val1_meta);
		break;
	default:
		throw InvalidCast("Fail cast value for div operation, cause value type is undefined");
	}
}


void DynBitXor(void** val0, void** val1) {
	ValueMeta& val0_meta = *((ValueMeta*)val0 + 1);
	ValueMeta& val1_meta = *((ValueMeta*)val1 + 1);
	void*& actual_val0 = *val0;
	void*& actual_val1 = *val1;
	if (val0_meta.vtype == VType::async_res) getAsyncResult(actual_val0, val0_meta);
	if (val1_meta.vtype == VType::async_res) getAsyncResult(actual_val1, val1_meta);

	if (!val0_meta.allow_edit)
		throw UnmodifabeValue();

	switch (val0_meta.vtype) {
	case VType::noting: {
		actual_val0 = copyValue(actual_val1, val1_meta);
		val0_meta = val1_meta;
		break;
	}
	case VType::i8:
		reinterpret_cast<int8_t&>(actual_val0) ^= ABI_IMPL::Vcast<int8_t>(actual_val1, val1_meta);
		break;
	case VType::i16:
		reinterpret_cast<int16_t&>(actual_val0) ^= ABI_IMPL::Vcast<int16_t>(actual_val1, val1_meta);
		break;
	case VType::i32:
		reinterpret_cast<int32_t&>(actual_val0) ^= ABI_IMPL::Vcast<int32_t>(actual_val1, val1_meta);
		break;
	case VType::i64:
		reinterpret_cast<int64_t&>(actual_val0) ^= ABI_IMPL::Vcast<int64_t>(actual_val1, val1_meta);
		break;
	case VType::ui8:
		reinterpret_cast<uint8_t&>(actual_val0) ^= ABI_IMPL::Vcast<uint8_t>(actual_val1, val1_meta);
		break;
	case VType::ui16:
		reinterpret_cast<uint16_t&>(actual_val0) ^= ABI_IMPL::Vcast<uint16_t>(actual_val1, val1_meta);
		break;
	case VType::ui32:
		reinterpret_cast<uint32_t&>(actual_val0) ^= ABI_IMPL::Vcast<uint32_t>(actual_val1, val1_meta);
		break;
	case VType::ui64:
		reinterpret_cast<uint64_t&>(actual_val0) ^= ABI_IMPL::Vcast<uint64_t>(actual_val1, val1_meta);
		break;
	case VType::undefined_ptr:
		reinterpret_cast<size_t&>(actual_val0) ^= ABI_IMPL::Vcast<size_t>(actual_val1, val1_meta);
		break;
	default:
		throw InvalidOperation("Invalid operation for non integer or non ptr type");
	}
}
void DynBitOr(void** val0, void** val1) {
	ValueMeta& val0_meta = *((ValueMeta*)val0 + 1);
	ValueMeta& val1_meta = *((ValueMeta*)val1 + 1);
	void*& actual_val0 = *val0;
	void*& actual_val1 = *val1;
	if (val0_meta.vtype == VType::async_res) getAsyncResult(actual_val0, val0_meta);
	if (val1_meta.vtype == VType::async_res) getAsyncResult(actual_val1, val1_meta);

	if (!val0_meta.allow_edit)
		throw UnmodifabeValue();

	switch (val0_meta.vtype) {
	case VType::noting: {
		actual_val0 = copyValue(actual_val1, val1_meta);
		val0_meta = val1_meta;
		break;
	}
	case VType::i8:
		reinterpret_cast<int8_t&>(actual_val0) |= ABI_IMPL::Vcast<int8_t>(actual_val1, val1_meta);
		break;
	case VType::i16:
		reinterpret_cast<int16_t&>(actual_val0) |= ABI_IMPL::Vcast<int16_t>(actual_val1, val1_meta);
		break;
	case VType::i32:
		reinterpret_cast<int32_t&>(actual_val0) |= ABI_IMPL::Vcast<int32_t>(actual_val1, val1_meta);
		break;
	case VType::i64:
		reinterpret_cast<int64_t&>(actual_val0) |= ABI_IMPL::Vcast<int64_t>(actual_val1, val1_meta);
		break;
	case VType::ui8:
		reinterpret_cast<uint8_t&>(actual_val0) |= ABI_IMPL::Vcast<uint8_t>(actual_val1, val1_meta);
		break;
	case VType::ui16:
		reinterpret_cast<uint16_t&>(actual_val0) |= ABI_IMPL::Vcast<uint16_t>(actual_val1, val1_meta);
		break;
	case VType::ui32:
		reinterpret_cast<uint32_t&>(actual_val0) |= ABI_IMPL::Vcast<uint32_t>(actual_val1, val1_meta);
		break;
	case VType::ui64:
		reinterpret_cast<uint64_t&>(actual_val0) |= ABI_IMPL::Vcast<uint64_t>(actual_val1, val1_meta);
		break;
	case VType::undefined_ptr:
		reinterpret_cast<size_t&>(actual_val0) |= ABI_IMPL::Vcast<size_t>(actual_val1, val1_meta);
		break;
	default:
		throw InvalidOperation("Invalid operation for non integer or non ptr type");
	}
}
void DynBitAnd(void** val0, void** val1) {
	ValueMeta& val0_meta = *((ValueMeta*)val0 + 1);
	ValueMeta& val1_meta = *((ValueMeta*)val1 + 1);
	void*& actual_val0 = *val0;
	void*& actual_val1 = *val1;
	if (val0_meta.vtype == VType::async_res) getAsyncResult(actual_val0, val0_meta);
	if (val1_meta.vtype == VType::async_res) getAsyncResult(actual_val1, val1_meta);

	if (!val0_meta.allow_edit)
		throw UnmodifabeValue();

	switch (val0_meta.vtype) {
	case VType::noting: {
		actual_val0 = copyValue(actual_val1, val1_meta);
		val0_meta = val1_meta;
		break;
	}
	case VType::i8:
		reinterpret_cast<int8_t&>(actual_val0) &= ABI_IMPL::Vcast<int8_t>(actual_val1, val1_meta);
		break;
	case VType::i16:
		reinterpret_cast<int16_t&>(actual_val0) &= ABI_IMPL::Vcast<int16_t>(actual_val1, val1_meta);
		break;
	case VType::i32:
		reinterpret_cast<int32_t&>(actual_val0) &= ABI_IMPL::Vcast<int32_t>(actual_val1, val1_meta);
		break;
	case VType::i64:
		reinterpret_cast<int64_t&>(actual_val0) &= ABI_IMPL::Vcast<int64_t>(actual_val1, val1_meta);
		break;
	case VType::ui8:
		reinterpret_cast<uint8_t&>(actual_val0) &= ABI_IMPL::Vcast<uint8_t>(actual_val1, val1_meta);
		break;
	case VType::ui16:
		reinterpret_cast<uint16_t&>(actual_val0) &= ABI_IMPL::Vcast<uint16_t>(actual_val1, val1_meta);
		break;
	case VType::ui32:
		reinterpret_cast<uint32_t&>(actual_val0) &= ABI_IMPL::Vcast<uint32_t>(actual_val1, val1_meta);
		break;
	case VType::ui64:
		reinterpret_cast<uint64_t&>(actual_val0) &= ABI_IMPL::Vcast<uint64_t>(actual_val1, val1_meta);
		break;
	case VType::undefined_ptr:
		reinterpret_cast<size_t&>(actual_val0) &= ABI_IMPL::Vcast<size_t>(actual_val1, val1_meta);
		break;
	default:
		throw InvalidOperation("Invalid operation for non integer or non ptr type");
	}
}
void DynBitNot(void** val0) {
	ValueMeta& val0_meta = *((ValueMeta*)val0 + 1);
	void*& actual_val0 = *val0;
	if (val0_meta.vtype == VType::async_res) getAsyncResult(actual_val0, val0_meta);

	if (!val0_meta.allow_edit)
		throw UnmodifabeValue();

	switch (val0_meta.vtype) {
	case VType::noting:
		break;
	case VType::i8:
		reinterpret_cast<int8_t&>(actual_val0) = ~reinterpret_cast<int8_t&>(actual_val0);
		break;
	case VType::i16:
		reinterpret_cast<int16_t&>(actual_val0) = ~reinterpret_cast<int16_t&>(actual_val0);
		break;
	case VType::i32:
		reinterpret_cast<int32_t&>(actual_val0) = ~reinterpret_cast<int32_t&>(actual_val0);
		break;
	case VType::i64:
		reinterpret_cast<int64_t&>(actual_val0) = ~reinterpret_cast<int64_t&>(actual_val0);
		break;
	case VType::ui8:
		reinterpret_cast<uint8_t&>(actual_val0) = ~reinterpret_cast<uint8_t&>(actual_val0);
		break;
	case VType::ui16:
		reinterpret_cast<uint16_t&>(actual_val0) = ~reinterpret_cast<uint16_t&>(actual_val0);
		break;
	case VType::ui32:
		reinterpret_cast<uint32_t&>(actual_val0) = ~reinterpret_cast<uint32_t&>(actual_val0);
		break;
	case VType::ui64:
		reinterpret_cast<uint64_t&>(actual_val0) = ~reinterpret_cast<uint64_t&>(actual_val0);
		break;
	case VType::undefined_ptr:
		reinterpret_cast<size_t&>(actual_val0) = ~reinterpret_cast<size_t&>(actual_val0);
		break;
	default:
		throw InvalidOperation("Invalid operation for non integer or non ptr type");
	}
}


void* AsArg(void** val) {
	ValueMeta& meta = *((ValueMeta*)val + 1);
	ValueItem* value = (ValueItem*)val;
	if (meta.vtype == VType::faarr || meta.vtype == VType::saarr)
		return *val;
	else if (meta.vtype == VType::uarr) {
		list_array<ValueItem>& vil = *reinterpret_cast<list_array<ValueItem>*>(*val);
		if (vil.size() > UINT32_MAX)
			throw InvalidCast("Fail cast uarr to faarr due too large array");
		*value = ValueItem(vil.to_array(), (uint32_t)vil.size());
		return value->getSourcePtr();
	}
	else {
		*value = ValueItem(std::initializer_list{ std::move(*(ValueItem*)(val)) });
		return value->getSourcePtr();
	}
}

void AsArr(void** val) {
	ValueMeta& meta = *((ValueMeta*)val + 1);
	if (meta.vtype == VType::uarr)
		return;
	else {
		auto tmp = new list_array<ValueItem>(ABI_IMPL::Vcast<list_array<ValueItem>>(*val, meta));
		universalRemove(val);
		*val = tmp;
		meta.allow_edit = true;
		meta.use_gc = false;
		meta.vtype = VType::uarr;
	}
}

void asValue(void** val, VType type) {
	ValueMeta& meta = *reinterpret_cast<ValueMeta*>(val + 1);
	getAsyncResult(*val, meta);
	switch (type) {
	case VType::noting:
		universalRemove(val);
		break;
	case VType::i8:
		ABI_IMPL::setValue(ABI_IMPL::Vcast<int8_t>(*val, meta),val);
		break;
	case VType::i16:
		ABI_IMPL::setValue(ABI_IMPL::Vcast<int16_t>(*val, meta), val);
		break;
	case VType::i32:
		ABI_IMPL::setValue(ABI_IMPL::Vcast<int32_t>(*val, meta), val);
		break;
	case VType::i64:
		ABI_IMPL::setValue(ABI_IMPL::Vcast<int64_t>(*val, meta), val);
		break;
	case VType::ui8:
		ABI_IMPL::setValue(ABI_IMPL::Vcast<uint8_t>(*val, meta), val);
		break;
	case VType::ui16:
		ABI_IMPL::setValue(ABI_IMPL::Vcast<uint16_t>(*val, meta), val);
		break;
	case VType::ui32:
		ABI_IMPL::setValue(ABI_IMPL::Vcast<uint32_t>(*val, meta), val);
		break;
	case VType::ui64:
		ABI_IMPL::setValue(ABI_IMPL::Vcast<uint64_t>(*val, meta), val);
		break;
	case VType::flo:
		ABI_IMPL::setValue(ABI_IMPL::Vcast<float>(*val, meta), val);
		break;
	case VType::doub:
		ABI_IMPL::setValue(ABI_IMPL::Vcast<double>(*val, meta), val);
		break;
	case VType::uarr:
		ABI_IMPL::setValue(ABI_IMPL::Vcast<list_array<ValueItem>>(*val, meta), val);
		AsArg(val);
		break;
	case VType::string:
		ABI_IMPL::setValue(ABI_IMPL::Scast(*val, meta), val);
		break;
	case VType::undefined_ptr:
		ABI_IMPL::setValue(ABI_IMPL::Vcast<void*>(*val, meta), val);
		break;
	case VType::except_value:
		if (reinterpret_cast<ValueMeta*>(val + 1)->vtype != VType::except_value) {
			try {
				throw AException("Undefined", ABI_IMPL::Scast(*val, meta),copyValue(*val, meta), meta.encoded);
			}
			catch (AException& ex) {
				universalRemove(val);
				*val = new std::exception_ptr(std::current_exception());
				meta = ValueMeta(VType::except_value, false, true);
			}
		}
		break;
	default:
		throw NotImplementedException();
	}
}
bool isValue(void** val, VType type) {
	ValueMeta& meta = *reinterpret_cast<ValueMeta*>(val + 1);
	getAsyncResult(*val, meta);
	return meta.vtype == type;
}

bool isTrueValue(void** value) {
	getAsyncResult(*value, *reinterpret_cast<ValueMeta*>(value + 1));
	void** val = &(getValue(value));
	switch (reinterpret_cast<ValueMeta*>(value + 1)->vtype) {
	case VType::noting:
		return false;
	case VType::boolean:
	case VType::i8:
	case VType::ui8:
		return (uint8_t)*val;
	case VType::i16:
	case VType::ui16:
		return (uint16_t)*val;
	case VType::i32:
	case VType::ui32:
		return (uint32_t)*value;
	case VType::ui64:
	case VType::i64:
		return (uint64_t)*value;
	case VType::flo:
		return *(float*)value;
	case VType::doub:
		return *(double*)value;
	case VType::uarr:
		return ((list_array<ValueItem>*)value)->size();
	case VType::string:
		return ((std::string*)value)->size();
	case VType::undefined_ptr:
		return *value;
	case VType::except_value:
		std::rethrow_exception(*(std::exception_ptr*)value);
	default:
		return false;
	}
}
void setBoolValue(bool boolean,void** value) {
	if (!boolean) {
		universalRemove(value);
		return;
	}
	ValueMeta& meta = *reinterpret_cast<ValueMeta*>(value + 1);
	switch (meta.vtype) {
	case VType::noting:
	case VType::undefined_ptr:
	case VType::boolean:
	case VType::i8:
	case VType::i16:
	case VType::i32:
	case VType::i64:
	case VType::ui8:
	case VType::ui16:
	case VType::ui32:
	case VType::ui64:
	case VType::flo:
	case VType::doub:
		getValue(value) = (void*)1;
		return;
	default:
		universalRemove(value);
		meta = ValueMeta(VType::boolean, false, true);
		*value = (void*)1;
	}
}

namespace exception_abi {
	bool is_except(void** val) {
		return ((ValueMeta*)(val + 1))->vtype == VType::except_value;
	}
	void ignore_except(void** val) {
		if (!is_except(val))
			return;
		universalRemove(val);
		(*val) = (*(val + 1)) = nullptr;
	}
	void continue_unwind(void** val) {
		std::rethrow_exception(*((std::exception_ptr*)getSpecificValueLink(val, VType::except_value)));
	}
	bool call_except_handler(void** val, bool(*func_symbol)(void** val), bool ignore_fault) {
		try {
			if (func_symbol(getSpecificValueLink(val, VType::except_value))) {
				universalRemove(val);
				(*val) = (*(val + 1)) = nullptr;
			}
			return true;
		}
		catch (...) {
			if (!ignore_fault)
				throw;
		}
		return false;
	}

	//for static catch block
	jump_point switch_jump_handle_except(void** val, jump_handle_except* handlers, size_t handlers_c) {
		if (!handlers_c)
			continue_unwind(val);

		auto& ex = *((std::exception_ptr*)getSpecificValueLink(val, VType::except_value));
		if (handlers_c != 1) {
			try {
				std::rethrow_exception(ex);
			}
			catch (const AttachARuntimeException& ex) {
				std::string str = ex.name();
				for (size_t i = 1; i < handlers_c; i++)
					if (str == handlers[i].type_name)
						return handlers[i].jump_off;
			}
			catch (...) {}
			CXXExInfo info;
			getCxxExInfoFromException(info, ex);
			for (size_t i = 1; i < handlers_c; i++)
				if (info.ty_arr.contains_one([handlers, i](const CXXExInfo::Tys& ty) {return ty.ty_info->name() == handlers[i].type_name; }))
					return handlers[i].jump_off;
		}
		return handlers[0].jump_off;
	}
	//for dynamic catch block
	jump_point switch_jump_handle_except(void** val, list_array<jump_handle_except>* handlers) {
		if(!handlers)
			continue_unwind(val);
		if (!handlers->size())
			continue_unwind(val);

		auto& ex = *((std::exception_ptr*)getSpecificValueLink(val, VType::except_value));
		if (handlers->size() != 1) {
			try {
				std::rethrow_exception(ex);
			}
			catch (const AttachARuntimeException& ex) {
				std::string str = ex.name();
				size_t jmp = handlers->find_it([&str](const jump_handle_except& it) { return it.type_name == str; }, 1);
				if (jmp != list_array<jump_handle_except>::npos)
					return handlers->operator[](jmp).jump_off;
			}
			catch (...) {}
			CXXExInfo info;
			getCxxExInfoFromException(info, ex);
			for (auto& it : handlers->range(1, handlers->size())) 
				if (info.ty_arr.contains_one([&it](const CXXExInfo::Tys& ty) { return ty.ty_info->name() == it.type_name; }))
					return it.jump_off;
		}
		return handlers->operator[](0).jump_off;
	}
}





size_t getSize(void** value) {
	void* res = getValue(*value, *(ValueMeta*)(value + 1));
	ValueMeta& meta = *(ValueMeta*)(value + 1);
	int64_t sig;
	size_t actual;
	switch (meta.vtype) {
	case VType::i8:
		actual = sig = (int8_t)res;
		break;
	case VType::i16:
		actual = sig = (int16_t)res;
		break;
	case VType::i32:
		actual = sig = (int32_t)res;
		break;
	case VType::i64:
		actual = sig = (int64_t)res;
		break;
	case VType::ui8:
		return (uint8_t)res;
	case VType::ui16:
		return (uint16_t)res;
	case VType::ui32:
		return (uint32_t)res;
	case VType::ui64:
		return (uint64_t)res;
	case VType::flo: {
		float tmp = *(float*)&res;
		actual = (size_t)tmp;
		if (tmp != actual)
			throw NumericUndererflowException();
		return actual;
	}
	case VType::doub: {
		double tmp = *(double*)&res;
		actual = (size_t)tmp;
		if (tmp != actual)
			throw NumericUndererflowException();
		return actual;
	}
	default:
		throw InvalidType("Need sizable type");
	}
	if (sig != actual)
		throw NumericUndererflowException();
	return actual;
}



ValueItem::ValueItem(const void* vall, ValueMeta vmeta) : val(0) {
	auto tmp = (void*)vall;
	val = copyValue(tmp, vmeta);
	meta = vmeta;
}
ValueItem::ValueItem(void* vall, ValueMeta vmeta, bool,bool as_ref) {
	val = vall;
	meta = vmeta;
	meta.as_ref = as_ref;
}
ValueItem::ValueItem(const ValueItem& copy) : val(0) {
	ValueItem& tmp = (ValueItem&)copy;
	val = copyValue(tmp.val, tmp.meta);
	meta = copy.meta;
}
ValueItem::ValueItem(bool val) : val(0) {
	*this = ABI_IMPL::BVcast((uint8_t)val);
}
ValueItem::ValueItem(int8_t val) : val(0) {
	*this = ABI_IMPL::BVcast(val);
}
ValueItem::ValueItem(uint8_t val) : val(0) {
	*this = ABI_IMPL::BVcast(val);
}
ValueItem::ValueItem(int16_t val) : val(0) {
	*this = ABI_IMPL::BVcast(val);
}
ValueItem::ValueItem(uint16_t val) : val(0) {
	*this = ABI_IMPL::BVcast(val);
}
ValueItem::ValueItem(int32_t val) : val(0) {
	*this = ABI_IMPL::BVcast(val);
}
ValueItem::ValueItem(uint32_t val) : val(0) {
	*this = ABI_IMPL::BVcast(val);
}
ValueItem::ValueItem(int64_t val) : val(0) {
	*this = ABI_IMPL::BVcast(val);
}
ValueItem::ValueItem(uint64_t val) : val(0) {
	*this = ABI_IMPL::BVcast(val);
}
ValueItem::ValueItem(float val) : val(0) {
	*this = ABI_IMPL::BVcast(val);
}
ValueItem::ValueItem(double val) : val(0) {
	*this = ABI_IMPL::BVcast(val);
}
ValueItem::ValueItem(const std::string& val) : val(0) {
	*this = ABI_IMPL::BVcast(val);
}
ValueItem::ValueItem(const char* str) : val(0) {
	*this = ABI_IMPL::BVcast(std::string(str));
}
ValueItem::ValueItem(const list_array<ValueItem>& val) : val(0) {
	*this = ABI_IMPL::BVcast(val);
}
ValueItem::ValueItem(list_array<ValueItem>&& val) : val(new list_array<ValueItem>(std::move(val))) {
	meta = VType::uarr;
}
ValueItem::ValueItem(ValueItem* vals, uint32_t len) : val(0) {
	*this = ValueItem(vals, ValueMeta(VType::faarr, false, true, len));
}
ValueItem::ValueItem(const int8_t* vals, uint32_t len) {
	*this = ValueItem(vals, ValueMeta(VType::raw_arr_i8, false, true, len));
}
ValueItem::ValueItem(const uint8_t* vals, uint32_t len) {
	*this = ValueItem(vals, ValueMeta(VType::raw_arr_ui8, false, true, len));
}
ValueItem::ValueItem(const int16_t* vals, uint32_t len) {
	*this = ValueItem(vals, ValueMeta(VType::raw_arr_i16, false, true, len));
}
ValueItem::ValueItem(const uint16_t* vals, uint32_t len) {
	*this = ValueItem(vals, ValueMeta(VType::raw_arr_ui16, false, true, len));
}
ValueItem::ValueItem(const int32_t* vals, uint32_t len) {
	*this = ValueItem(vals, ValueMeta(VType::raw_arr_i32, false, true, len));
}
ValueItem::ValueItem(const uint32_t* vals, uint32_t len) {
	*this = ValueItem(vals, ValueMeta(VType::raw_arr_ui32, false, true, len));
}
ValueItem::ValueItem(const int64_t* vals, uint32_t len) {
	*this = ValueItem(vals, ValueMeta(VType::raw_arr_i64, false, true, len));
}
ValueItem::ValueItem(const uint64_t* vals, uint32_t len) {
	*this = ValueItem(vals, ValueMeta(VType::raw_arr_ui64, false, true, len));
}
ValueItem::ValueItem(const float* vals, uint32_t len) {
	*this = ValueItem(vals, ValueMeta(VType::raw_arr_flo, false, true, len));
}
ValueItem::ValueItem(const double* vals, uint32_t len) {
	*this = ValueItem(vals, ValueMeta(VType::raw_arr_doub, false, true, len));
}
ValueItem::ValueItem(typed_lgr<struct Task> task) {
	val = new typed_lgr(task);
	meta = VType::async_res;
}
ValueItem::ValueItem(const std::initializer_list<ValueItem>& args) : val(0) {
	if (args.size() > (size_t)UINT32_MAX)
		throw OutOfRange("Too large array");
	uint32_t len = (uint32_t)args.size();
	meta = ValueMeta(VType::faarr, false, true, len);
	if (args.size()) {
		ValueItem* res = new ValueItem[args.size()];
		const ValueItem* copy = args.begin();
		for(uint32_t i = 0;i< len;i++)
			res[i] = std::move(copy[i]);
		val = res;
	}
}
ValueItem::ValueItem(const std::exception_ptr& ex) {
	val = new std::exception_ptr(ex);
	meta = VType::except_value;
}
ValueItem::ValueItem(VType type) {
	meta = type;
	switch (type) {
	case VType::noting:
	case VType::async_res:
	case VType::undefined_ptr:
	case VType::type_identifier:
	case VType::i8:
	case VType::i16:
	case VType::i32:
	case VType::i64:
	case VType::ui8:
	case VType::ui16:
	case VType::ui32:
	case VType::ui64:
	case VType::flo:
	case VType::doub:
		val = nullptr;
		break;
	case VType::raw_arr_i8:
	case VType::raw_arr_ui8:
		val = new uint8_t[1]{0};
		meta.val_len = 1;
		break;
	case VType::raw_arr_i16:
	case VType::raw_arr_ui16:
		val = new uint16_t[1]{ 0 };
		meta.val_len = 1;
		break;
	case VType::raw_arr_i32:
	case VType::raw_arr_ui32:
	case VType::raw_arr_flo:
		val = new uint32_t[1]{ 0 };
		meta.val_len = 1;
		break;
	case VType::raw_arr_i64:
	case VType::raw_arr_ui64:
	case VType::raw_arr_doub:
		val = new uint64_t[1]{ 0 };
		meta.val_len = 1;
		break;
	case VType::uarr:
		val = new list_array<ValueItem>();
		break;
	case VType::string:
		val = new std::string();
		break;
	case VType::except_value:
		try {
			throw AException("Undefined exception","No description");
		}
		catch(...) {
			val = new std::exception_ptr(std::current_exception());
		}
		break;
	case VType::faarr:
		val = new ValueItem[1]{};
		meta.val_len = 1;
		break;
	case VType::class_:
		val = new ClassValue();
		break;
	case VType::morph:
		val = new MorphValue();
		break;
	case VType::proxy:
		val = new ProxyClass();
		break;
	case VType::function:
		val = new typed_lgr<FuncEnviropment>();
		break;
	default:
		throw NotImplementedException();
	}
}
ValueItem::ValueItem(ValueMeta type) {
	val = &type;
	meta = VType::type_identifier;
}
ValueItem::~ValueItem() {
	if (val)
		if(!meta.as_ref)
			if (needAlloc(meta))
				universalFree(&val, meta);
}

ValueItem& ValueItem::operator=(const ValueItem& copy) {
	ValueItem& tmp = (ValueItem&)copy;
	if (val)
		if(!meta.as_ref)
			if (needAlloc(meta))
				universalFree(&val, meta);
	val = copyValue(tmp.val, tmp.meta);
	meta = copy.meta;
	return *this;
}
ValueItem& ValueItem::operator=(ValueItem&& move) noexcept {
	if (val)
		if (!meta.as_ref)
			if (needAlloc(meta))
				universalFree(&val, meta);
	val = move.val;
	meta = move.meta;
	move.val = nullptr;
	return *this;
}

bool ValueItem::operator<(const ValueItem& cmp) const {
	void* val1 = getValue(const_cast<void*&>(val), const_cast<ValueMeta&>(meta));
	void* val2 = getValue(const_cast<void*&>(cmp.val), const_cast<ValueMeta&>(cmp.meta));
	return compareValue(meta.vtype, cmp.meta.vtype, val1, val2).second;
}
bool ValueItem::operator>(const ValueItem& cmp) const {
	void* val1 = getValue(const_cast<void*&>(val), const_cast<ValueMeta&>(meta));
	void* val2 = getValue(const_cast<void*&>(cmp.val), const_cast<ValueMeta&>(cmp.meta));
	return !compareValue(meta.vtype, cmp.meta.vtype, val1, val2).second;
}
bool ValueItem::operator==(const ValueItem& cmp) const {
	void* val1 = getValue(const_cast<void*&>(val), const_cast<ValueMeta&>(meta));
	void* val2 = getValue(const_cast<void*&>(cmp.val), const_cast<ValueMeta&>(cmp.meta));
	return compareValue(meta.vtype, cmp.meta.vtype, val1, val2).first;
}
bool ValueItem::operator!=(const ValueItem& cmp) const {
	void* val1 = getValue(const_cast<void*&>(val), const_cast<ValueMeta&>(meta));
	void* val2 = getValue(const_cast<void*&>(cmp.val), const_cast<ValueMeta&>(cmp.meta));
	return !compareValue(meta.vtype, cmp.meta.vtype, val1, val2).first;
}
bool ValueItem::operator>=(const ValueItem& cmp) const {
	void* val1 = getValue(const_cast<void*&>(val), const_cast<ValueMeta&>(meta));
	void* val2 = getValue(const_cast<void*&>(cmp.val), const_cast<ValueMeta&>(cmp.meta));
	auto tmp = compareValue(meta.vtype, cmp.meta.vtype, val1, val2);
	return tmp.first || !tmp.second;
}
bool ValueItem::operator<=(const ValueItem& cmp) const {
	void* val1 = getValue(const_cast<void*&>(val), const_cast<ValueMeta&>(meta));
	void* val2 = getValue(const_cast<void*&>(cmp.val), const_cast<ValueMeta&>(cmp.meta));
	auto tmp = compareValue(meta.vtype, cmp.meta.vtype, val1, val2);
	return tmp.first || tmp.second;
}		
ValueItem& ValueItem::operator +=(const ValueItem& op) {
	DynSum(&val, (void**)&op.val);
	return *this;
}
ValueItem& ValueItem::operator -=(const ValueItem& op) {
	DynMinus(&val, (void**)&op.val);
	return *this;
}
ValueItem& ValueItem::operator *=(const ValueItem& op) {
	DynMul(&val, (void**)&op.val);
	return *this;
}
ValueItem& ValueItem::operator /=(const ValueItem& op) {
	DynDiv(&val, (void**)&op.val);
	return *this;
}
ValueItem& ValueItem::operator %=(const ValueItem& op) {
	DynRest(&val, (void**)&op.val);
	return *this;
}
ValueItem& ValueItem::operator ^=(const ValueItem& op) {
	DynBitXor(&val, (void**)&op.val);
	return *this;
}
ValueItem& ValueItem::operator &=(const ValueItem& op) {
	DynBitAnd(&val, (void**)&op.val);
	return *this;
}
ValueItem& ValueItem::operator |=(const ValueItem& op) {
	DynBitOr(&val, (void**)&op.val);
	return *this;
}
ValueItem& ValueItem::operator !() {
	DynBitNot(&val);
	return *this;
}

ValueItem ValueItem::operator +(const ValueItem& op) const {
	return ValueItem(*this) += op;
}
ValueItem ValueItem::operator -(const ValueItem& op) const {
	return ValueItem(*this) -= op;
}
ValueItem ValueItem::operator *(const ValueItem& op) const {
	return ValueItem(*this) *= op;
}
ValueItem ValueItem::operator /(const ValueItem& op) const {
	return ValueItem(*this) /= op;
}
ValueItem ValueItem::operator ^(const ValueItem& op) const {
	return ValueItem(*this) ^= op;
}
ValueItem ValueItem::operator &(const ValueItem& op) const {
	return ValueItem(*this) &= op;
}
ValueItem ValueItem::operator |(const ValueItem& op) const {
	return ValueItem(*this) |= op;
}


ValueItem::operator bool() {
	return (bool)ABI_IMPL::Vcast<uint8_t>(val, meta);
}
ValueItem::operator int8_t() {
	return ABI_IMPL::Vcast<int8_t>(val, meta);
}
ValueItem::operator uint8_t() {
	return ABI_IMPL::Vcast<uint8_t>(val, meta);
}
ValueItem::operator int16_t() {
	return ABI_IMPL::Vcast<int16_t>(val, meta);
}
ValueItem::operator uint16_t() {
	return ABI_IMPL::Vcast<uint16_t>(val, meta);
}
ValueItem::operator int32_t() {
	return ABI_IMPL::Vcast<int32_t>(val, meta);
}
ValueItem::operator uint32_t() {
	return ABI_IMPL::Vcast<uint32_t>(val, meta);
}
ValueItem::operator int64_t() {
	return ABI_IMPL::Vcast<int64_t>(val, meta);
}
ValueItem::operator uint64_t() {
	return ABI_IMPL::Vcast<uint64_t>(val, meta);
}
ValueItem::operator float() {
	return ABI_IMPL::Vcast<float>(val, meta);
}
ValueItem::operator double() {
	return ABI_IMPL::Vcast<double>(val, meta);
}
ValueItem::operator std::string() {
	return ABI_IMPL::Scast(val, meta);
}
ValueItem::operator void*() {
	return ABI_IMPL::Vcast<void*>(val, meta);
}
ValueItem::operator list_array<ValueItem>() {
	return ABI_IMPL::Vcast<list_array<ValueItem>>(val, meta);
}
ValueItem::operator ClassValue&() {
	if (meta.vtype == VType::class_)
		return *(ClassValue*)getSourcePtr();
	else
		throw InvalidCast("This type is not class");
}
ValueItem::operator MorphValue&() {
	if (meta.vtype == VType::morph)
		return *(MorphValue*)getSourcePtr();
	else
		throw InvalidCast("This type is not morph");
}
ValueItem::operator ProxyClass&() {
	if (meta.vtype == VType::proxy)
		return *(ProxyClass*)getSourcePtr();
	else
		throw InvalidCast("This type is not proxy");
}
ValueItem::operator std::exception_ptr() {
	if (meta.vtype == VType::except_value)
		return *(std::exception_ptr*)getSourcePtr();
	else {
		try {
			throw InvalidCast("This type is not proxy");
		}catch(...){
			return std::current_exception();
		}
	}
}
ValueItem* ValueItem::operator()(ValueItem* args,uint32_t len) {
	if (meta.vtype == VType::function)
		return FuncEnviropment::sync_call((*(class typed_lgr<class FuncEnviropment>*)getValue(val, meta)),args,len);
	else
		return new ValueItem(*this);
}

void ValueItem::getAsync() {
	if(val)
		while (meta.vtype == VType::async_res)
			getAsyncResult(val, meta);
}

void*& ValueItem::getSourcePtr() {
	return getValue(val, meta);
}
typed_lgr<FuncEnviropment>* ValueItem::funPtr() {
	if (meta.vtype == VType::function)
		return (typed_lgr<FuncEnviropment>*)getValue(val, meta);
	return nullptr;
}




ClassDefine::ClassDefine():name("Unnamed") { }
ClassDefine::ClassDefine(const std::string& name) { this->name = name; }

typed_lgr<class FuncEnviropment> ClassValue::callFnPtr(const std::string & str, ClassAccess access) {
	if (define) {
		if (define->funs.contains(str)) {
			auto& tmp = define->funs[str];
			switch (access) {
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
				throw InvalidOperation("Undefined access type: " + enum_to_string(access));
			}
			throw InvalidOperation("Try access from " + enum_to_string(access) + "region to " + enum_to_string(tmp.access) + " function");
		}
	}
	throw NotImplementedException();
}
ClassFnDefine& ClassValue::getFnMeta(const std::string & str) {
	if (define) {
			if (define->funs.contains(str))
				return define->funs[str];
		}
	throw NotImplementedException();
}
void ClassValue::setFnMeta(const std::string& str, ClassFnDefine& fn_decl) {
	if (define) {
		if (define->funs.contains(str))
			if (!define->funs[str].deletable)
				throw InvalidOperation("This class has non modifable function declaration");
	}
	else
		define = new ClassDefine();
	define->funs[str] = fn_decl;
}
bool ClassValue::containsFn(const std::string& str) {
	if (!define)
		return false;
	return define->funs.contains(str);
}
ValueItem& ClassValue::getValue(const std::string& str, ClassAccess access) {
	if (val.contains(str)) {
		auto& tmp = val[str];
		switch (access) {
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
			throw InvalidOperation("Undefined access type: " + enum_to_string(access));
		}
		throw InvalidOperation("Try access from " + enum_to_string(access) + "region to " + enum_to_string(tmp.access) + " value");
	}
	throw NotImplementedException();
}
ValueItem ClassValue::copyValue(const std::string& str, ClassAccess access) {
	if (val.contains(str)) {
		return getValue(str, access);
	}
	return ValueItem();
}
bool ClassValue::containsValue(const std::string& str) {
	return val.contains(str);
}


typed_lgr<class FuncEnviropment> MorphValue::callFnPtr(const std::string & str, ClassAccess access) {
	if (define.funs.contains(str)) {
		auto& tmp = define.funs[str];
		switch (access) {
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
			throw InvalidOperation("Undefined access type: " + enum_to_string(access));
		}
		throw InvalidOperation("Try access from " + enum_to_string(access) + "region to " + enum_to_string(tmp.access) + " function");
	}
	throw NotImplementedException();
}
ClassFnDefine& MorphValue::getFnMeta(const std::string& str) {
	if (define.funs.contains(str))
		return define.funs[str];
	throw NotImplementedException();
}
void MorphValue::setFnMeta(const std::string& str, ClassFnDefine& fn_decl) {
	if (define.funs.contains(str))
		if (!define.funs[str].deletable)
			throw InvalidOperation("This class has non modifable function declaration");
	define.funs[str] = fn_decl;
}
bool MorphValue::containsFn(const std::string& str) {
	return define.funs.contains(str);
}
ValueItem& MorphValue::getValue(const std::string& str, ClassAccess access) {
	if (val.contains(str)) {
		auto& tmp = val[str];
		switch (access) {
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
			throw InvalidOperation("Undefined access type: " + enum_to_string(access));
		}
		throw InvalidOperation("Try access from " + enum_to_string(access) + "region to " + enum_to_string(tmp.access) + " value");
	}
	throw NotImplementedException();
}
ValueItem MorphValue::copyValue(const std::string& str, ClassAccess access) {
	if (val.contains(str)) {
		return getValue(str, access);
	}
	return ValueItem();
}
void MorphValue::setValue(const std::string& str, ClassAccess access, ValueItem& set_val) {
	if (val.contains(str)) {
		auto& tmp = val[str];
		switch (access) {
		case ClassAccess::pub:
			if (tmp.access == ClassAccess::pub) {
				tmp.val = set_val;
				return;
			}
			else break;
		case ClassAccess::priv:
			if (tmp.access != ClassAccess::deriv) {
				tmp.val = set_val;
				return;
			}
			else break;
		case ClassAccess::prot:
			if (tmp.access == ClassAccess::pub || tmp.access == ClassAccess::prot) {
				tmp.val = set_val;
				return;
			}
			else break;
		case ClassAccess::deriv:
			if (tmp.access != ClassAccess::priv) {
				tmp.val = set_val;
				return;
			}
			else break;
		default:
			throw InvalidOperation("Undefined access type: " + enum_to_string(access));
		}
		throw InvalidOperation("Try access from " + enum_to_string(access) + "region to " + enum_to_string(tmp.access) + " value");
	}
	throw NotImplementedException();
}
bool MorphValue::containsValue(const std::string& str) {
	return val.contains(str);
}




ProxyClassDefine::ProxyClassDefine() :name("Unnamed") {}
ProxyClassDefine::ProxyClassDefine(const std::string& name) :name(name) { }
ProxyClass::ProxyClass() {
	class_ptr = nullptr;
	declare_ty = nullptr;
}
ProxyClass::ProxyClass(void* val) {
	class_ptr = val;
	declare_ty = nullptr;
}
ProxyClass::ProxyClass(void* val, ProxyClassDefine* def) {
	class_ptr = val;
	declare_ty = def;
}
ProxyClass::~ProxyClass() {
	if (declare_ty)
		declare_ty->destructor(class_ptr);
}

typed_lgr<class FuncEnviropment> ProxyClass::callFnPtr(const std::string& str, ClassAccess access) {
	if (declare_ty) {
		auto& define = *declare_ty;
		if (define.funs.contains(str)) {
			auto& tmp = define.funs[str];
			switch (access) {
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
				throw InvalidOperation("Undefined access type: " + enum_to_string(access));
			}
			throw InvalidOperation("Try access from " + enum_to_string(access) + "region to " + enum_to_string(tmp.access) + " function");
		}
	}
	throw NotImplementedException();
}
ClassFnDefine& ProxyClass::getFnMeta(const std::string& str) {
	if (declare_ty) {
		auto& define = *declare_ty;
		if (define.funs.contains(str))
			return define.funs[str];
	}
	throw NotImplementedException();
}
void ProxyClass::setFnMeta(const std::string& str, ClassFnDefine& fn_decl) {
	if (declare_ty) {
		if (declare_ty->funs.contains(str))
			if (!declare_ty->funs[str].deletable)
				throw InvalidOperation("This class has non modifable function declaration");
	}
	else
		declare_ty = new ProxyClassDefine();
	declare_ty->funs[str] = fn_decl;
}
bool ProxyClass::containsFn(const std::string& str) {
	if (declare_ty)
		return declare_ty->funs.contains(str);
	else
		return false;
}
ValueItem* ProxyClass::getValue(const std::string& str) {
	if (declare_ty) {
		if (declare_ty->value_geter.contains(str)) {
			auto& tmp = declare_ty->value_geter[str];
			return tmp(class_ptr);
		}
	}
	throw NotImplementedException();
}
void ProxyClass::setValue(const std::string& str, ValueItem& it) {
	if (declare_ty) {
		if (declare_ty->value_seter.contains(str)) {
			auto& tmp = declare_ty->value_seter[str];
			tmp(class_ptr, it);
		}
	}
	throw NotImplementedException();
}
bool ProxyClass::containsValue(const std::string& str) {
	return declare_ty ? declare_ty->value_seter.contains(str) && declare_ty->value_geter.contains(str) : false;
}