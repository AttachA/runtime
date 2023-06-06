// Copyright Danyil Melnytskyi 2022-2023
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#include "AttachA_CXX.hpp"
#include "../configuration/agreement/symbols.hpp"
#include <string>



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
void removeArgsEnviropement(list_array<ValueItem>* env) {
	delete env;
}
ValueItem* getAsyncValueItem(void* val) {
	if(!val)
		return nullptr;
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
	catch (const std::bad_alloc&)
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
			res = getAsyncValueItem(meta.use_gc ? ((lgr*)value)->getPtr() : value);
			if(res){
				*vaal = std::move(*res);
				delete res;
			}
		}
		catch (...) {
			if (res)
				delete res;
		}
	}
}
void* copyValue(void*& val, ValueMeta& meta) {
	getAsyncResult(val, meta);
	if(meta.as_ref)
		return val;
	void* actual_val = val;
	if (meta.use_gc)
		actual_val = ((lgr*)val)->getPtr();
	if(actual_val == nullptr)
		return nullptr;
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
		case VType::saarr:
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
	void** res = &getValue(value);
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
	return &getValue(value);
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
			length = (uint64_t)AttachA::Interface::makeCall(ClassAccess::priv,*(ClassValue*)val2, symbols::structures::size);
			break;
		case VType::morph:
			length = (uint64_t)AttachA::Interface::makeCall(ClassAccess::priv, *(MorphValue*)val2, symbols::structures::size);
			break;
		case VType::proxy:
			length = (uint64_t)AttachA::Interface::makeCall(ClassAccess::priv, *(ProxyClass*)val2, symbols::structures::size);
			break;
		default:
			throw AException("Implementation exception", "Wrong function usage compareUarrAInterface");
		}
		if (arr1.size() < length)
			return { false, true };
		else if (arr1.size() == length) {
			auto iter = arr1.begin(); 
			if ((*(ProxyClass*)val2).containsFn(symbols::structures::iterable::begin)) {
				auto iter2 = AttachA::Interface::makeCall(ClassAccess::priv, *(ProxyClass*)val2, symbols::structures::iterable::begin);
				for (uint64_t i = 0; i < length; i++) {
					auto& it1 = *iter;
					auto it2 = AttachA::Interface::makeCall(ClassAccess::priv, iter2, symbols::structures::iterable::next);
					auto res = compareValue(it1.meta, it2.meta, it1.val, it2.val);
					if (!res.first)
						return flip_args ? std::pair<bool, bool>{false, res.second} : res;
					++iter;
				}
			}
			else if ((*(ProxyClass*)val2).containsFn(symbols::structures::index_operator)) {
				for (uint64_t i = 0; i < length; i++) {
					auto& it1 = *iter;
					auto it2 = AttachA::Interface::makeCall(ClassAccess::priv, *(ProxyClass*)val2, symbols::structures::index_operator, i);
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
	if ((*(ProxyClass*)val2).containsFn(symbols::structures::iterable::begin)) {
		auto iter2 = AttachA::Interface::makeCall(ClassAccess::priv, *(ProxyClass*)val2, symbols::structures::iterable::begin);
		for (uint64_t i = 0; i < length; i++) {
			T first = *arr2;
			ValueItem it2 = AttachA::Interface::makeCall(ClassAccess::priv, iter2, symbols::structures::iterable::next);
			auto res = compareValue(T_VType, it2.meta, &first, it2.val);
			if (!res.first)
				return { false, res.second,true };
			++arr2;
		}
	}
	else if ((*(ProxyClass*)val2).containsFn(symbols::structures::index_operator)) {
		for (uint64_t i = 0; i < length; i++) {
			T first = *arr2;
			ValueItem it2 = AttachA::Interface::makeCall(ClassAccess::priv, *(ProxyClass*)val2, symbols::structures::index_operator, i);
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
	if ((*(ProxyClass*)val2).containsFn(symbols::structures::iterable::begin)) {
		auto iter2 = AttachA::Interface::makeCall(ClassAccess::priv, *(ProxyClass*)val2, symbols::structures::iterable::begin);
		for (uint64_t i = 0; i < length; i++) {
			ValueItem& first = *arr2;
			ValueItem it2 = AttachA::Interface::makeCall(ClassAccess::priv, iter2, symbols::structures::iterable::next);
			auto res = compareValue(first.meta, it2.meta, first.val, it2.val);
			if (!res.first)
				return { false, res.second,true };
			++arr2;
		}
	}
	else if ((*(ProxyClass*)val2).containsFn(symbols::structures::index_operator)) {
		for (uint64_t i = 0; i < length; i++) {
			ValueItem& first = *arr2;
			ValueItem it2 = AttachA::Interface::makeCall(ClassAccess::priv, *(ProxyClass*)val2, symbols::structures::index_operator, i);
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
			length = (uint64_t)AttachA::Interface::makeCall(ClassAccess::priv, *(ClassValue*)val2, symbols::structures::size);
			break;
		case VType::morph:
			length = (uint64_t)AttachA::Interface::makeCall(ClassAccess::priv, *(MorphValue*)val2, symbols::structures::size);
			break;
		case VType::proxy:
			length = (uint64_t)AttachA::Interface::makeCall(ClassAccess::priv, *(ProxyClass*)val2, symbols::structures::size);
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
	if ((cmp_int = (is_integer(cmp1.vtype) || cmp1.vtype == VType::undefined_ptr) != (is_integer(cmp2.vtype) || cmp1.vtype == VType::undefined_ptr)))
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
				return { false, uint64_t(val1) < int8_t((ptrdiff_t)val2) };
			case VType::i16:
				return { false, uint64_t(val1) < int16_t((ptrdiff_t)val2) };
			case VType::i32:
				return { false, uint64_t(val1) < int32_t((ptrdiff_t)val2) };
			case VType::i64:
				return { false, int64_t(val2) < 0 ? false : uint64_t(val1) < uint64_t(val2) };
			case VType::flo:
				return { false, uint64_t(val1) < *(float*)&val2 };
			case VType::doub:
				return { false, uint64_t(val1) < *(double*)&val2 };
				default:
					break;
			}
		else if (temp2)
			switch (cmp1.vtype) {
			case VType::i8:
				return { false, int8_t((ptrdiff_t)val1) < uint64_t(val2) };
			case VType::i16:
				return { false, int16_t((ptrdiff_t)val1) < uint64_t(val2) };
			case VType::i32:
				return { false, int32_t((ptrdiff_t)val1) < uint64_t(val2) };
			case VType::i64:
				return { false, int64_t(val1) < 0 ? true : uint64_t(val1) < uint64_t(val2) };
			case VType::flo:
				return { false, *(float*)&val1 < uint64_t(val2) };
			case VType::doub:
				return { false, *(double*)&val1 < uint64_t(val2) };
			default:
				break;
			}
		else
			switch (cmp1.vtype) {
			case VType::i8:
				switch (cmp2.vtype) {
				case VType::i8:
					return { false, int8_t((ptrdiff_t)val1) < int8_t((ptrdiff_t)val2) };
				case VType::i16:
					return { false, int8_t((ptrdiff_t)val1) < int16_t((ptrdiff_t)val2) };
				case VType::i32:
					return { false, int8_t((ptrdiff_t)val1) < int32_t((ptrdiff_t)val2) };
				case VType::i64:
					return { false, int8_t((ptrdiff_t)val1) < int64_t((ptrdiff_t)val2) };
				case VType::flo:
					return { false, int8_t((ptrdiff_t)val1) < *(float*)&(val2) };
				case VType::doub:
					return { false, int8_t((ptrdiff_t)val1) < *(double*)&(val2) };
				default:
					break;
				}
				break;
			case VType::i16:
				switch (cmp2.vtype) {
				case VType::i8:
					return { false, int16_t((ptrdiff_t)val1) < int8_t((ptrdiff_t)val2) };
				case VType::i16:
					return { false, int16_t((ptrdiff_t)val1) < int16_t((ptrdiff_t)val2) };
				case VType::i32:
					return { false, int16_t((ptrdiff_t)val1) < int32_t((ptrdiff_t)val2) };
				case VType::i64:
					return { false, int16_t((ptrdiff_t)val1) < int64_t((ptrdiff_t)val2) };
				case VType::flo:
					return { false, int16_t((ptrdiff_t)val1) < *(float*)&(val2) };
				case VType::doub:
					return { false, int16_t((ptrdiff_t)val1) < *(double*)&(val2) };
				default:
					break;
				}
				break;
			case VType::i32:
				switch (cmp2.vtype) {
				case VType::i8:
					return { false, int32_t((ptrdiff_t)val1) < int8_t((ptrdiff_t)val2) };
				case VType::i16:
					return { false, int32_t((ptrdiff_t)val1) < int16_t((ptrdiff_t)val2) };
				case VType::i32:
					return { false, int32_t((ptrdiff_t)val1) < int32_t((ptrdiff_t)val2) };
				case VType::i64:
					return { false, int32_t((ptrdiff_t)val1) < int64_t((ptrdiff_t)val2) };
				case VType::flo:
					return { false, int32_t((ptrdiff_t)val1) < *(float*)&(val2) };
				case VType::doub:
					return { false, int32_t((ptrdiff_t)val1) < *(double*)&(val2) };
				default:
					break;
				}
				
				break;
			case VType::i64:
				switch (cmp2.vtype) {
				case VType::i8:
					return { false, int64_t(val1) < int8_t((ptrdiff_t)val2) };
				case VType::i16:
					return { false, int64_t(val1) < int16_t((ptrdiff_t)val2) };
				case VType::i32:
					return { false, int64_t(val1) < int32_t((ptrdiff_t)val2) };
				case VType::i64:
					return { false, int64_t(val1) < int64_t((ptrdiff_t)val2) };
				case VType::flo:
					return { false, int64_t(val1) < *(float*)&(val2) };
				case VType::doub:
					return { false, int64_t(val1) < *(double*)&(val2) };
				default:
					break;
				}
				break;
			case VType::flo:
				switch (cmp2.vtype) {
				case VType::i8:
					return { false, *(float*)&val1 < int8_t((ptrdiff_t)val2) };
				case VType::i16:
					return { false, *(float*)&val1 < int16_t((ptrdiff_t)val2) };
				case VType::i32:
					return { false, *(float*)&val1 < int32_t((ptrdiff_t)val2) };
				case VType::i64:
					return { false, *(float*)&val1 < int64_t((ptrdiff_t)val2) };
				case VType::flo:
					return { false, *(float*)&val1 < *(float*)&(val2) };
				case VType::doub:
					return { false, *(float*)&val1 < *(double*)&(val2) };
				default:
					break;
				}
				break;
			case VType::doub:
				switch (cmp2.vtype) {
				case VType::i8:
					return { false, *(double*)&val1 < int8_t((ptrdiff_t)val2) };
				case VType::i16:
					return { false, *(double*)&val1 < int16_t((ptrdiff_t)val2) };
				case VType::i32:
					return { false, *(double*)&val1 < int32_t((ptrdiff_t)val2) };
				case VType::i64:
					return { false, *(double*)&val1 < int64_t((ptrdiff_t)val2) };
				case VType::flo:
					return { false, *(double*)&val1 < *(float*)&(val2) };
				case VType::doub:
					return { false, *(double*)&val1 < *(double*)&(val2) };
				default:
					break;
				}
			
			default:
				break;
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
	

	template<class T>
	std::string raw_arr_to_string(void* arr, size_t size) {
		std::string res = "*[";
		for (size_t i = 0; i < size; i++) {
			res += std::to_string(reinterpret_cast<T*>(arr)[i]);
			if (i != size - 1) res += ", ";
		}
		res += "]";
		return res;
	}
	
	std::string Scast(void*& ref_val, ValueMeta& meta) {
		void* val = getValue(ref_val, meta);
		switch (meta.vtype) {
		case VType::noting: return "noting";
		case VType::boolean: return *(bool*)val ? "true" : "false";
		case VType::i8: return std::to_string(*reinterpret_cast<int8_t*>(&val));
		case VType::i16: return std::to_string(*reinterpret_cast<int16_t*>(&val));
		case VType::i32: return std::to_string(*reinterpret_cast<int32_t*>(&val));
		case VType::i64: return std::to_string(*reinterpret_cast<int64_t*>(&val));
		case VType::ui8: return std::to_string(*reinterpret_cast<uint8_t*>(&val));
		case VType::ui16: return std::to_string(*reinterpret_cast<uint16_t*>(&val));
		case VType::ui32: return std::to_string(*reinterpret_cast<uint32_t*>(&val));
		case VType::ui64: return std::to_string(*reinterpret_cast<uint64_t*>(&val));
		case VType::flo: return std::to_string(*reinterpret_cast<float*>(&val));
		case VType::doub: return std::to_string(*reinterpret_cast<double*>(&val));
		case VType::raw_arr_i8: return raw_arr_to_string<int8_t>(val, meta.val_len);
		case VType::raw_arr_i16: return raw_arr_to_string<int16_t>(val, meta.val_len);
		case VType::raw_arr_i32: return raw_arr_to_string<int32_t>(val, meta.val_len);
		case VType::raw_arr_i64: return raw_arr_to_string<int64_t>(val, meta.val_len);
		case VType::raw_arr_ui8: return raw_arr_to_string<uint8_t>(val, meta.val_len);
		case VType::raw_arr_ui16: return raw_arr_to_string<uint16_t>(val, meta.val_len);
		case VType::raw_arr_ui32: return raw_arr_to_string<uint32_t>(val, meta.val_len);
		case VType::raw_arr_ui64: return raw_arr_to_string<uint64_t>(val, meta.val_len);
		case VType::raw_arr_flo: return raw_arr_to_string<float>(val, meta.val_len);
		case VType::raw_arr_doub: return raw_arr_to_string<double>(val, meta.val_len);
		case VType::faarr:
		case VType::saarr:{
			std::string res = "*[";
			for (uint32_t i = 0; i < meta.val_len; i++){
				ValueItem& it = reinterpret_cast<ValueItem*>(val)[i];
				res += Scast(it.val, it.meta) + (i + 1 < meta.val_len ? ',' : ']');
			}
			if (!meta.val_len)
				res += ']';
			return res;
		}
		case VType::string: return *reinterpret_cast<std::string*>(val);
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
		case VType::undefined_ptr: return "0x" + string_help::hexstr(val);
		case VType::type_identifier: return enum_to_string(*(VType*)&val);
		case VType::function: return reinterpret_cast<FuncEnviropment*>(val)->to_string();
		case VType::map:{
			std::string res("{");
			bool before = false;
			for (auto& it : *reinterpret_cast<std::unordered_map<ValueItem, ValueItem>*>(val)) {
				if (before)
					res += ',';
				ValueItem& key = const_cast<ValueItem&>(it.first);
				ValueItem& item = it.second;
				res += Scast(key.val, key.meta) + ':' + Scast(item.val, item.meta);
				before = true;
			}
			res += '}';
			return res;
		}
		case VType::set:{
			std::string res("(");
			bool before = false;
			for (auto& it : *reinterpret_cast<std::unordered_set<ValueItem>*>(val)) {
				if (before)
					res += ',';
				ValueItem& item = const_cast<ValueItem&>(it);
				res += Scast(item.val, item.meta);
				before = true;
			}
			res += ')';
			return res;
		}
		case VType::time_point: return "t(" + std::to_string(reinterpret_cast<std::chrono::steady_clock::time_point*>(val)->time_since_epoch().count()) + ')';
		case VType::class_:
			return (std::string)AttachA::Interface::makeCall(ClassAccess::pub, *reinterpret_cast<ClassValue*>(val), symbols::structures::convert::to_string);
		case VType::morph:
			return (std::string)AttachA::Interface::makeCall(ClassAccess::pub, *reinterpret_cast<MorphValue*>(val), symbols::structures::convert::to_string);
		case VType::proxy: 
			return (std::string)AttachA::Interface::makeCall(ClassAccess::pub, *reinterpret_cast<ProxyClass*>(val), symbols::structures::convert::to_string);
		case VType::class_define:
			throw InvalidCast("Fail cast class define");
		default:
			throw InvalidCast("Fail cast undefined type");
		}
	}

	list_array<ValueItem> string_to_array(const std::string& str, uint32_t start){
		list_array<ValueItem> res;
		std::string tmp;
		bool in_str = false;
		for (uint32_t i = start; i < str.size(); i++) {
			if (str[i] == '"' && str[i - 1] != '\\')
				in_str = !in_str;
			if (str[i] == ',' && !in_str) {
				res.push_back(SBcast(tmp));
				tmp.clear();
			}
			else
				tmp += str[i];
		}
		if (tmp.size())
			res.push_back(SBcast(tmp));
		return res;	
	}
	ValueItem SBcast(const std::string& str) {
		if (str == "noting" || str == "null" || str == "undefined")
			return nullptr;
		else if (str == "true")
			return true;
		else if (str == "false")
			return false;
		else if (str.starts_with('"') && str.ends_with('"'))
			return str.substr(1, str.size() - 2);
		else if (str.starts_with('\'') && str.ends_with('\'') && str.size() > 2)
			return str.substr(1, str.size() - 2);
		else if(str == "\'\\\'\'")//'\'' -> '
			return ValueItem("\'");
		else if (str.starts_with("0x"))
			return ValueItem((void*)std::stoull(str, nullptr, 16),VType::undefined_ptr);
		else if (str.starts_with('[')) 
			return ValueItem(string_to_array(str,1));	
		else if(str.starts_with("*[")) {
			auto tmp = string_to_array(str, 2);
			ValueItem res;
			res.meta.vtype = VType::faarr;
			res.meta.allow_edit = true;
			size_t size = 0;
			res.val = tmp.take_raw(size);
			res.meta.val_len = size;
			return res;
		}
		else if(str.starts_with('{')){
			std::unordered_map<ValueItem, ValueItem> res;
			std::string key;
			std::string value;
			bool in_str = false;
			bool is_key = true;
			for (uint32_t i = 1; i < str.size(); i++) {
				if (str[i] == '"' && str[i - 1] != '\\')
					in_str = !in_str;
				if (str[i] == ':' && !in_str) {
					is_key = false;
				}
				else if (str[i] == ',' && !in_str) {
					res[SBcast(key)] = SBcast(value);
					key.clear();
					value.clear();
					is_key = true;
				}
				else {
					if (is_key)
						key += str[i];
					else
						value += str[i];
				}
			}
			if (key.size())
				res[SBcast(key)] = SBcast(value);
			return ValueItem(std::move(res));
		}
		else if (str.starts_with('(')) {
			std::unordered_set<ValueItem> res;
			std::string tmp;
			bool in_str = false;
			for (uint32_t i = 1; i < str.size(); i++) {
				if (str[i] == '"' && str[i - 1] != '\\')
					in_str = !in_str;
				if (str[i] == ',' && !in_str) {
					res.insert(SBcast(tmp));
					tmp.clear();
				}
				else
					tmp += str[i];
			}
			if (tmp.size())
				res.insert(SBcast(tmp));
			return ValueItem(std::move(res));
		}
		else if (str.starts_with("t(")) {
			std::string tmp;
			for (uint32_t i = 11; i < str.size(); i++) {
				if(str[i] == ')')
					break;
				tmp += str[i];
			}
			auto res = std::chrono::steady_clock::time_point(std::chrono::nanoseconds(std::stoull(tmp)));
			return ValueItem(*(void**)&res, VType::time_point);
		}
		else {
			try {
				try {
					try {
						try {
							try {
								try {
									int32_t res = std::stoi(str);
									return ValueItem(*(void**)&res, VType::i32);
								}
								catch (...) {
									uint32_t res = std::stoul(str);
									return ValueItem(*(void**)&res, VType::ui32);
								}
							}
							catch (...) {
								int64_t res = std::stoll(str);
								return ValueItem(*(void**)&res, VType::i64);
							}
						}
						catch (...) {
							uint64_t res = std::stoull(str);
							return ValueItem(*(void**)&res, VType::ui64);
						}
					}
					catch (...) {
						float res = std::stof(str);
						return ValueItem(*(void**)&res, VType::flo);
					}
				}
				catch (...) {
					double res = std::stod(str);
					return ValueItem(*(void**)&res, VType::doub);
				}
			}
			catch (...) {
				return ValueItem(new std::string(str), VType::string);
			}
		}
	}
	template<class T>
	void setValue(const T& val, void** set_val) {
		universalRemove(set_val);
		if constexpr (std::is_same_v<T, int8_t*>) {
			*reinterpret_cast<int8_t**>(set_val) = val;
			*reinterpret_cast<ValueMeta*>(set_val + 1) = VType::i8;
		}
		else if constexpr (std::is_same_v<T, uint8_t*>) {
			*reinterpret_cast<uint8_t**>(set_val) = val;
			*reinterpret_cast<ValueMeta*>(set_val + 1) =VType::ui8;
		}
		else if constexpr (std::is_same_v<T, int16_t*>) {
			*reinterpret_cast<int16_t**>(set_val) = val;
			*reinterpret_cast<ValueMeta*>(set_val + 1) = VType::i16;
		}
		else if constexpr (std::is_same_v<T, uint16_t*>) {
			*reinterpret_cast<uint16_t**>(set_val) = val;
			*reinterpret_cast<ValueMeta*>(set_val + 1) = VType::ui16;
		}
		else if constexpr (std::is_same_v<T, int32_t*>) {
			*reinterpret_cast<int32_t**>(set_val) = val;
			*reinterpret_cast<ValueMeta*>(set_val + 1) = VType::i32;
		}
		else if constexpr (std::is_same_v<T, uint32_t*>) {
			*reinterpret_cast<uint32_t**>(set_val) = val;
			*reinterpret_cast<ValueMeta*>(set_val + 1) = VType::ui32;
		}
		else if constexpr (std::is_same_v<T, int64_t**>) {
			*reinterpret_cast<int64_t**>(set_val) = val;
			*reinterpret_cast<ValueMeta*>(set_val + 1) = VType::i64;
		}
		else if constexpr (std::is_same_v<T, uint64_t*>) {
			*reinterpret_cast<uint64_t**>(set_val) = val;
			*reinterpret_cast<ValueMeta*>(set_val + 1) = VType::ui64;
		}
		else if constexpr (std::is_same_v < T, bool> ) {
			*reinterpret_cast<bool*>(set_val) = val;
			*reinterpret_cast<ValueMeta*>(set_val + 1) = VType::boolean;
		}
		else if constexpr (std::is_same_v<T, int8_t>) {
			*reinterpret_cast<int8_t*>(set_val) = val;
			*reinterpret_cast<ValueMeta*>(set_val + 1) = VType::i8;
		}
		else if constexpr (std::is_same_v<T, uint8_t>) {
			*reinterpret_cast<uint8_t*>(set_val) = val;
			*reinterpret_cast<ValueMeta*>(set_val + 1) = VType::ui8;
		}
		else if constexpr (std::is_same_v<T, int16_t>) {
			*reinterpret_cast<int16_t*>(set_val) = val;
			*reinterpret_cast<ValueMeta*>(set_val + 1) = VType::i16;
		}
		else if constexpr (std::is_same_v<T, uint16_t>) {
			*reinterpret_cast<uint16_t*>(set_val) = val;
			*reinterpret_cast<ValueMeta*>(set_val + 1) = VType::ui16;
		}
		else if constexpr (std::is_same_v<T, int32_t>) {
			*reinterpret_cast<int32_t*>(set_val) = val;
			*reinterpret_cast<ValueMeta*>(set_val + 1) = VType::i32;
		}
		else if constexpr (std::is_same_v<T, uint32_t>) {
			*reinterpret_cast<uint32_t*>(set_val) = val;
			*reinterpret_cast<ValueMeta*>(set_val + 1) = VType::ui32;
		}
		else if constexpr (std::is_same_v<T, int64_t>) {
			*reinterpret_cast<int64_t*>(set_val) = val;
			*reinterpret_cast<ValueMeta*>(set_val + 1) = VType::i64;
		}
		else if constexpr (std::is_same_v<T, uint64_t>) {
			*reinterpret_cast<uint64_t*>(set_val) = val;
			*reinterpret_cast<ValueMeta*>(set_val + 1) = VType::ui64;
		}
		else if constexpr (std::is_same_v<T, float>) {
			*reinterpret_cast<float*>(set_val) = val;
			*reinterpret_cast<ValueMeta*>(set_val + 1) = VType::flo;
		}
		else if constexpr (std::is_same_v<T, double>) {
			*reinterpret_cast<double*>(set_val) = val;
			*reinterpret_cast<ValueMeta*>(set_val + 1) = VType::doub;
		}
		else if constexpr (std::is_same_v<T, std::string>) {
			*reinterpret_cast<std::string**>(set_val) = new std::string(val);
			*reinterpret_cast<ValueMeta*>(set_val + 1) = VType::string;
		}
		else if constexpr (std::is_same_v<T, list_array<ValueItem>>) {
			*reinterpret_cast<list_array<ValueItem>**>(set_val) = new list_array<ValueItem >(val);
			*reinterpret_cast<ValueMeta*>(set_val + 1) = VType::uarr;
		}
		else if constexpr (std::is_same_v<T, void*>) {
			*reinterpret_cast<void**>(set_val) = val;
			*reinterpret_cast<ValueMeta*>(set_val + 1) = VType::undefined_ptr;
		}
		else
			throw NotImplementedException();
	}
}





void DynSum(void** val0, void** val1) {
	ValueItem& val0_r = *reinterpret_cast<ValueItem*>(val0);
	ValueItem& val1_r = *reinterpret_cast<ValueItem*>(val1);
	val0_r.getAsync();
	val1_r.getAsync();
	void*& actual_val0 = val0_r.getSourcePtr();
	void*& actual_val1 = val1_r.getSourcePtr();

	if (!val1_r.meta.allow_edit)
		throw UnmodifabeValue();

	switch (val0_r.meta.vtype) {
	case VType::noting: val0_r = val1_r;break;
	case VType::i8:
		reinterpret_cast<int8_t&>(actual_val0) += (int8_t)val1_r;
		break;
	case VType::i16:
		reinterpret_cast<int16_t&>(actual_val0) += (int16_t)val1_r;
		break;
	case VType::i32:
		reinterpret_cast<int32_t&>(actual_val0) += (int32_t)val1_r;
		break;
	case VType::i64:
		reinterpret_cast<int64_t&>(actual_val0) += (int64_t)val1_r;
		break;
	case VType::ui8:
		reinterpret_cast<uint8_t&>(actual_val0) += (uint8_t)val1_r;
		break;
	case VType::ui16:
		reinterpret_cast<uint16_t&>(actual_val0) += (uint16_t)val1_r;
		break;
	case VType::ui32:
		reinterpret_cast<uint32_t&>(actual_val0) += (uint32_t)val1_r;
		break;
	case VType::ui64:
		reinterpret_cast<uint64_t&>(actual_val0) += (uint64_t)val1_r;
		break;
	case VType::flo:
		reinterpret_cast<float&>(actual_val0) += (float)val1_r;
		break;
	case VType::doub:
		reinterpret_cast<double&>(actual_val0) += (double)val1_r;
		break;
	case VType::uarr:
		reinterpret_cast<list_array<ValueItem>&>(actual_val0).push_back(val1_r);
		break;
	case VType::string:
		reinterpret_cast<std::string&>(actual_val0) += (std::string)val1_r;
		break;
	case VType::undefined_ptr:
		reinterpret_cast<size_t&>(actual_val0) += (size_t)val1_r;
		break;
	case VType::time_point:{
		auto& val0_time = reinterpret_cast<std::chrono::steady_clock::time_point&>(actual_val0);
		auto& val1_time = reinterpret_cast<std::chrono::steady_clock::time_point&>(actual_val1);
		val0_time = val0_time + (std::chrono::nanoseconds)val1_time.time_since_epoch();
		break;
	}
	case VType::proxy:
	case VType::morph:
	case VType::class_:
		AttachA::Interface::makeCall(ClassAccess::pub,val0_r, symbols::structures::add_operator, val1_r);
		break;
	default:
		throw InvalidCast("Fail cast value for add operation, cause value type is unsupported");
	}
}
void DynMinus(void** val0, void** val1) {
	ValueItem& val0_r = *reinterpret_cast<ValueItem*>(val0);
	ValueItem& val1_r = *reinterpret_cast<ValueItem*>(val1);
	val0_r.getAsync();
	val1_r.getAsync();
	void*& actual_val0 = val0_r.getSourcePtr();
	void*& actual_val1 = val1_r.getSourcePtr();

	if (!val1_r.meta.allow_edit)
		throw UnmodifabeValue();

	switch (val0_r.meta.vtype) {
	case VType::noting: val0_r = val1_r;break;
	case VType::i8:
		reinterpret_cast<int8_t&>(actual_val0) -= (int8_t)val1_r;
		break;
	case VType::i16:
		reinterpret_cast<int16_t&>(actual_val0) -= (int16_t)val1_r;
		break;
	case VType::i32:
		reinterpret_cast<int32_t&>(actual_val0) -= (int32_t)val1_r;
		break;
	case VType::i64:
		reinterpret_cast<int64_t&>(actual_val0) -= (int64_t)val1_r;
		break;
	case VType::ui8:
		reinterpret_cast<uint8_t&>(actual_val0) -= (uint8_t)val1_r;
		break;
	case VType::ui16:
		reinterpret_cast<uint16_t&>(actual_val0) -= (uint16_t)val1_r;
		break;
	case VType::ui32:
		reinterpret_cast<uint32_t&>(actual_val0) -= (uint32_t)val1_r;
		break;
	case VType::ui64:
		reinterpret_cast<uint64_t&>(actual_val0) -= (uint64_t)val1_r;
		break;
	case VType::flo:
		reinterpret_cast<float&>(actual_val0) -= (float)val1_r;
		break;
	case VType::doub:
		reinterpret_cast<double&>(actual_val0) -= (double)val1_r;
		break;
	case VType::uarr:
		reinterpret_cast<list_array<ValueItem>&>(actual_val0).push_back(val1_r);
		break;
	case VType::string:
		reinterpret_cast<std::string&>(actual_val0) = (std::string)val1_r + reinterpret_cast<std::string&>(actual_val0);
		break;
	case VType::undefined_ptr:
		reinterpret_cast<size_t&>(actual_val0) -= (size_t)val1_r;
		break;
	case VType::time_point:{
		auto& val0_time = reinterpret_cast<std::chrono::steady_clock::time_point&>(actual_val0);
		auto& val1_time = reinterpret_cast<std::chrono::steady_clock::time_point&>(actual_val1);
		val0_time = val0_time - (std::chrono::nanoseconds)val1_time.time_since_epoch();
		break;
	}
	case VType::proxy:
	case VType::morph:
	case VType::class_:
		AttachA::Interface::makeCall(ClassAccess::pub, val0_r, symbols::structures::subtract_operator, val1_r);
		break;
	default:
		throw InvalidCast("Fail cast value for minus operation, cause value type is unsupported");
	}
}
void DynMul(void** val0, void** val1) {
	ValueItem& val0_r = *reinterpret_cast<ValueItem*>(val0);
	ValueItem& val1_r = *reinterpret_cast<ValueItem*>(val1);
	val0_r.getAsync();
	val1_r.getAsync();
	void*& actual_val0 = val0_r.getSourcePtr();
	void*& actual_val1 = val1_r.getSourcePtr();

	if (!val1_r.meta.allow_edit)
		throw UnmodifabeValue();

	switch (val0_r.meta.vtype) {
	case VType::noting: val0_r = val1_r;break;
	case VType::i8:
		reinterpret_cast<int8_t&>(actual_val0) *= (int8_t)val1_r;
		break;
	case VType::i16:
		reinterpret_cast<int16_t&>(actual_val0) *= (int16_t)val1_r;
		break;
	case VType::i32:
		reinterpret_cast<int32_t&>(actual_val0) *= (int32_t)val1_r;
		break;
	case VType::i64:
		reinterpret_cast<int64_t&>(actual_val0) *= (int64_t)val1_r;
		break;
	case VType::ui8:
		reinterpret_cast<uint8_t&>(actual_val0) *= (uint8_t)val1_r;
		break;
	case VType::ui16:
		reinterpret_cast<uint16_t&>(actual_val0) *= (uint16_t)val1_r;
		break;
	case VType::ui32:
		reinterpret_cast<uint32_t&>(actual_val0) *= (uint32_t)val1_r;
		break;
	case VType::ui64:
		reinterpret_cast<uint64_t&>(actual_val0) *= (uint64_t)val1_r;
		break;
	case VType::flo:
		reinterpret_cast<float&>(actual_val0) *= (float)val1_r;
		break;
	case VType::doub:
		reinterpret_cast<double&>(actual_val0) *= (double)val1_r;
		break;
	case VType::uarr:
		reinterpret_cast<list_array<ValueItem>&>(actual_val0).push_back(val1_r);
		break;
	case VType::undefined_ptr:
		reinterpret_cast<size_t&>(actual_val0) *= (size_t)val1_r;
		break;
	case VType::proxy:
	case VType::morph:
	case VType::class_:
		AttachA::Interface::makeCall(ClassAccess::pub, val0_r, symbols::structures::multiply_operator, val1_r);
		break;
	default:
		throw InvalidCast("Fail cast value for multiply operation, cause value type is unsupported");
	}

}
void DynDiv(void** val0, void** val1) {
	ValueItem& val0_r = *reinterpret_cast<ValueItem*>(val0);
	ValueItem& val1_r = *reinterpret_cast<ValueItem*>(val1);
	val0_r.getAsync();
	val1_r.getAsync();
	void*& actual_val0 = val0_r.getSourcePtr();
	void*& actual_val1 = val1_r.getSourcePtr();

	if (!val1_r.meta.allow_edit)
		throw UnmodifabeValue();

	switch (val0_r.meta.vtype) {
	case VType::noting: val0_r = val1_r;break;
	case VType::i8:
		reinterpret_cast<int8_t&>(actual_val0) /= (int8_t)val1_r;
		break;
	case VType::i16:
		reinterpret_cast<int16_t&>(actual_val0) /= (int16_t)val1_r;
		break;
	case VType::i32:
		reinterpret_cast<int32_t&>(actual_val0) /= (int32_t)val1_r;
		break;
	case VType::i64:
		reinterpret_cast<int64_t&>(actual_val0) /= (int64_t)val1_r;
		break;
	case VType::ui8:
		reinterpret_cast<uint8_t&>(actual_val0) /= (uint8_t)val1_r;
		break;
	case VType::ui16:
		reinterpret_cast<uint16_t&>(actual_val0) /= (uint16_t)val1_r;
		break;
	case VType::ui32:
		reinterpret_cast<uint32_t&>(actual_val0) /= (uint32_t)val1_r;
		break;
	case VType::ui64:
		reinterpret_cast<uint64_t&>(actual_val0) /= (uint64_t)val1_r;
		break;
	case VType::flo:
		reinterpret_cast<float&>(actual_val0) /= (float)val1_r;
		break;
	case VType::doub:
		reinterpret_cast<double&>(actual_val0) /= (double)val1_r;
		break;
	case VType::undefined_ptr:
		reinterpret_cast<size_t&>(actual_val0) /= (size_t)val1_r;
		break;
	case VType::proxy:
	case VType::morph:
	case VType::class_:
		AttachA::Interface::makeCall(ClassAccess::pub, val0_r, symbols::structures::divide_operator, val1_r);
		break;
	default:
		throw InvalidCast("Fail cast value for divide operation, cause value type is unsupported");
	}
}
void DynRest(void** val0, void** val1) {
	ValueItem& val0_r = *reinterpret_cast<ValueItem*>(val0);
	ValueItem& val1_r = *reinterpret_cast<ValueItem*>(val1);
	val0_r.getAsync();
	val1_r.getAsync();
	void*& actual_val0 = val0_r.getSourcePtr();
	void*& actual_val1 = val1_r.getSourcePtr();

	if (!val1_r.meta.allow_edit)
		throw UnmodifabeValue();

	switch (val0_r.meta.vtype) {
	case VType::noting: val0_r = val1_r;break;
	case VType::i8:
		reinterpret_cast<int8_t&>(actual_val0) %= (int8_t)val1_r;
		break;
	case VType::i16:
		reinterpret_cast<int16_t&>(actual_val0) %= (int16_t)val1_r;
		break;
	case VType::i32:
		reinterpret_cast<int32_t&>(actual_val0) %= (int32_t)val1_r;
		break;
	case VType::i64:
		reinterpret_cast<int64_t&>(actual_val0) %= (int64_t)val1_r;
		break;
	case VType::ui8:
		reinterpret_cast<uint8_t&>(actual_val0) %= (uint8_t)val1_r;
		break;
	case VType::ui16:
		reinterpret_cast<uint16_t&>(actual_val0) %= (uint16_t)val1_r;
		break;
	case VType::ui32:
		reinterpret_cast<uint32_t&>(actual_val0) %= (uint32_t)val1_r;
		break;
	case VType::ui64:
		reinterpret_cast<uint64_t&>(actual_val0) %= (uint64_t)val1_r;
		break;
	case VType::undefined_ptr:
		reinterpret_cast<size_t&>(actual_val0) %= (size_t)val1_r;
		break;
	case VType::proxy:
	case VType::morph:
	case VType::class_:
		AttachA::Interface::makeCall(ClassAccess::pub, val0_r, symbols::structures::modulo_operator, val1_r);
		break;
	default:
		throw InvalidCast("Fail cast value for rest operation, cause value type is unsupported");
	}
}
void DynInc(void** val0){
	ValueItem& val0_r = *reinterpret_cast<ValueItem*>(val0);
	val0_r.getAsync();
	void*& actual_val0 = val0_r.getSourcePtr();

	if (!val0_r.meta.allow_edit)
		throw UnmodifabeValue();

	switch (val0_r.meta.vtype) {
	case VType::noting: val0_r = 1;break;
	case VType::i8:
		reinterpret_cast<int8_t&>(actual_val0)++;
		break;
	case VType::i16:
		reinterpret_cast<int16_t&>(actual_val0)++;
		break;
	case VType::i32:
		reinterpret_cast<int32_t&>(actual_val0)++;
		break;
	case VType::i64:
		reinterpret_cast<int64_t&>(actual_val0)++;
		break;
	case VType::ui8:
		reinterpret_cast<uint8_t&>(actual_val0)++;
		break;
	case VType::ui16:
		reinterpret_cast<uint16_t&>(actual_val0)++;
		break;
	case VType::ui32:
		reinterpret_cast<uint32_t&>(actual_val0)++;
		break;
	case VType::ui64:
		reinterpret_cast<uint64_t&>(actual_val0)++;
		break;
	case VType::flo:
		reinterpret_cast<float&>(actual_val0)++;
		break;
	case VType::doub:
		reinterpret_cast<double&>(actual_val0)++;
		break;
	case VType::undefined_ptr:
		reinterpret_cast<size_t&>(actual_val0)++;
		break;
	case VType::proxy:
	case VType::morph:
	case VType::class_:
		AttachA::Interface::makeCall(ClassAccess::pub, val0_r, symbols::structures::increment_operator);
		break;
	default:
		throw InvalidCast("Fail cast value for increment operation, cause value type is unsupported");
	}
}
void DynDec(void** val0) {
	ValueItem& val0_r = *reinterpret_cast<ValueItem*>(val0);
	val0_r.getAsync();
	void*& actual_val0 = val0_r.getSourcePtr();

	if (!val0_r.meta.allow_edit)
		throw UnmodifabeValue();

	switch (val0_r.meta.vtype) {
	case VType::noting: val0_r = -1;break;
	case VType::i8:
		reinterpret_cast<int8_t&>(actual_val0)--;
		break;
	case VType::i16:
		reinterpret_cast<int16_t&>(actual_val0)--;
		break;
	case VType::i32:
		reinterpret_cast<int32_t&>(actual_val0)--;
		break;
	case VType::i64:
		reinterpret_cast<int64_t&>(actual_val0)--;
		break;
	case VType::ui8:
		reinterpret_cast<uint8_t&>(actual_val0)--;
		break;
	case VType::ui16:
		reinterpret_cast<uint16_t&>(actual_val0)--;
		break;
	case VType::ui32:
		reinterpret_cast<uint32_t&>(actual_val0)--;
		break;
	case VType::ui64:
		reinterpret_cast<uint64_t&>(actual_val0)--;
		break;
	case VType::flo:
		reinterpret_cast<float&>(actual_val0)--;
		break;
	case VType::doub:
		reinterpret_cast<double&>(actual_val0)--;
		break;
	case VType::undefined_ptr:
		reinterpret_cast<size_t&>(actual_val0)--;
		break;
	case VType::proxy:
	case VType::morph:
	case VType::class_:
		AttachA::Interface::makeCall(ClassAccess::pub, val0_r, symbols::structures::decrement_operator);
		break;
	default:
		throw InvalidCast("Fail cast value for decrement operation, cause value type is unsupported");
	}
}

void DynBitXor(void** val0, void** val1) {
	ValueItem& val0_r = *reinterpret_cast<ValueItem*>(val0);
	ValueItem& val1_r = *reinterpret_cast<ValueItem*>(val1);
	val0_r.getAsync();
	val1_r.getAsync();
	void*& actual_val0 = val0_r.getSourcePtr();
	void*& actual_val1 = val1_r.getSourcePtr();

	if (!val0_r.meta.allow_edit)
		throw UnmodifabeValue();
	
	switch (val0_r.meta.vtype) {
	case VType::noting: val0_r = val1_r;break;
	case VType::i8:
		reinterpret_cast<int8_t&>(actual_val0) ^= (int8_t)val1_r;
		break;
	case VType::i16:
		reinterpret_cast<int16_t&>(actual_val0) ^= (int16_t)val1_r;
		break;
	case VType::i32:
		reinterpret_cast<int32_t&>(actual_val0) ^= (int32_t)val1_r;
		break;
	case VType::i64:
		reinterpret_cast<int64_t&>(actual_val0) ^= (int64_t)val1_r;
		break;
	case VType::ui8:
		reinterpret_cast<uint8_t&>(actual_val0) ^= (uint8_t)val1_r;
		break;
	case VType::ui16:
		reinterpret_cast<uint16_t&>(actual_val0) ^= (uint16_t)val1_r;
		break;
	case VType::ui32:
		reinterpret_cast<uint32_t&>(actual_val0) ^= (uint32_t)val1_r;
		break;
	case VType::ui64:
		reinterpret_cast<uint64_t&>(actual_val0) ^= (uint64_t)val1_r;
		break;
	case VType::undefined_ptr:
		reinterpret_cast<size_t&>(actual_val0) ^= (size_t)val1_r;
		break;
	case VType::proxy:
	case VType::morph:
	case VType::class_:
		AttachA::Interface::makeCall(ClassAccess::pub, val0_r, symbols::structures::bitwise_xor_operator, val1_r);
		break;
	default:
		throw InvalidCast("Fail cast value for xor operation, cause value type is unsupported");
	}
}
void DynBitOr(void** val0, void** val1) {
	ValueItem& val0_r = *reinterpret_cast<ValueItem*>(val0);
	ValueItem& val1_r = *reinterpret_cast<ValueItem*>(val1);
	val0_r.getAsync();
	val1_r.getAsync();
	void*& actual_val0 = val0_r.getSourcePtr();
	void*& actual_val1 = val1_r.getSourcePtr();

	if (!val0_r.meta.allow_edit)
		throw UnmodifabeValue();

	switch (val0_r.meta.vtype) {
	case VType::noting: val0_r = val1_r;break;
	case VType::i8:
		reinterpret_cast<int8_t&>(actual_val0) |= (int8_t)val1_r;
		break;
	case VType::i16:
		reinterpret_cast<int16_t&>(actual_val0) |= (int16_t)val1_r;
		break;
	case VType::i32:
		reinterpret_cast<int32_t&>(actual_val0) |= (int32_t)val1_r;
		break;
	case VType::i64:
		reinterpret_cast<int64_t&>(actual_val0) |= (int64_t)val1_r;
		break;
	case VType::ui8:
		reinterpret_cast<uint8_t&>(actual_val0) |= (uint8_t)val1_r;
		break;
	case VType::ui16:
		reinterpret_cast<uint16_t&>(actual_val0) |= (uint16_t)val1_r;
		break;
	case VType::ui32:
		reinterpret_cast<uint32_t&>(actual_val0) |= (uint32_t)val1_r;
		break;
	case VType::ui64:
		reinterpret_cast<uint64_t&>(actual_val0) |= (uint64_t)val1_r;
		break;
	case VType::undefined_ptr:
		reinterpret_cast<size_t&>(actual_val0) |= (size_t)val1_r;
		break;
	case VType::proxy:
	case VType::morph:
	case VType::class_:
		AttachA::Interface::makeCall(ClassAccess::pub, val0_r, symbols::structures::bitwise_or_operator, val1_r);
		break;
	default:
		throw InvalidCast("Fail cast value for or operation, cause value type is unsupported");
	}
}
void DynBitAnd(void** val0, void** val1) {
	ValueItem& val0_r = *reinterpret_cast<ValueItem*>(val0);
	ValueItem& val1_r = *reinterpret_cast<ValueItem*>(val1);
	val0_r.getAsync();
	val1_r.getAsync();
	void*& actual_val0 = val0_r.getSourcePtr();
	void*& actual_val1 = val1_r.getSourcePtr();

	if (!val0_r.meta.allow_edit)
		throw UnmodifabeValue();

	switch (val0_r.meta.vtype) {
	case VType::noting: val0_r = val1_r;break;
	case VType::i8:
		reinterpret_cast<int8_t&>(actual_val0) &= (int8_t)val1_r;
		break;
	case VType::i16:
		reinterpret_cast<int16_t&>(actual_val0) &= (int16_t)val1_r;
		break;
	case VType::i32:
		reinterpret_cast<int32_t&>(actual_val0) &= (int32_t)val1_r;
		break;
	case VType::i64:
		reinterpret_cast<int64_t&>(actual_val0) &= (int64_t)val1_r;
		break;
	case VType::ui8:
		reinterpret_cast<uint8_t&>(actual_val0) &= (uint8_t)val1_r;
		break;
	case VType::ui16:
		reinterpret_cast<uint16_t&>(actual_val0) &= (uint16_t)val1_r;
		break;
	case VType::ui32:
		reinterpret_cast<uint32_t&>(actual_val0) &= (uint32_t)val1_r;
		break;
	case VType::ui64:
		reinterpret_cast<uint64_t&>(actual_val0) &= (uint64_t)val1_r;
		break;
	case VType::undefined_ptr:
		reinterpret_cast<size_t&>(actual_val0) &= (size_t)val1_r;
		break;
	case VType::proxy:
	case VType::morph:
	case VType::class_:
		AttachA::Interface::makeCall(ClassAccess::pub, val0_r, symbols::structures::bitwise_and_operator, val1_r);
		break;
	default:
		throw InvalidCast("Fail cast value for and operation, cause value type is unsupported");
	}
}
void DynBitShiftRight(void** val0, void** val1){
	ValueItem& val0_r = *reinterpret_cast<ValueItem*>(val0);
	ValueItem& val1_r = *reinterpret_cast<ValueItem*>(val1);
	val0_r.getAsync();
	val1_r.getAsync();
	void*& actual_val0 = val0_r.getSourcePtr();
	void*& actual_val1 = val1_r.getSourcePtr();

	if (!val0_r.meta.allow_edit)
		throw UnmodifabeValue();

	switch (val0_r.meta.vtype) {
	case VType::noting: val0_r = val1_r;break;
	case VType::i8:
		reinterpret_cast<int8_t&>(actual_val0) >>= (int8_t)val1_r;
		break;
	case VType::i16:
		reinterpret_cast<int16_t&>(actual_val0) >>= (int16_t)val1_r;
		break;
	case VType::i32:
		reinterpret_cast<int32_t&>(actual_val0) >>= (int32_t)val1_r;
		break;
	case VType::i64:
		reinterpret_cast<int64_t&>(actual_val0) >>= (int64_t)val1_r;
		break;
	case VType::ui8:
		reinterpret_cast<uint8_t&>(actual_val0) >>= (uint8_t)val1_r;
		break;
	case VType::ui16:
		reinterpret_cast<uint16_t&>(actual_val0) >>= (uint16_t)val1_r;
		break;
	case VType::ui32:
		reinterpret_cast<uint32_t&>(actual_val0) >>= (uint32_t)val1_r;
		break;
	case VType::ui64:
		reinterpret_cast<uint64_t&>(actual_val0) >>= (uint64_t)val1_r;
		break;
	case VType::undefined_ptr:
		reinterpret_cast<size_t&>(actual_val0) >>= (size_t)val1_r;
		break;
	case VType::proxy:
	case VType::morph:
	case VType::class_:
		AttachA::Interface::makeCall(ClassAccess::pub, val0_r, symbols::structures::bitwise_shift_right_operator, val1_r);
		break;
	default:
		throw InvalidCast("Fail cast value for shift right operation, cause value type is unsupported");
	}
}
void DynBitShiftLeft(void** val0, void** val1) {
	ValueItem& val0_r = *reinterpret_cast<ValueItem*>(val0);
	ValueItem& val1_r = *reinterpret_cast<ValueItem*>(val1);
	val0_r.getAsync();
	val1_r.getAsync();
	void*& actual_val0 = val0_r.getSourcePtr();
	void*& actual_val1 = val1_r.getSourcePtr();

	if (!val0_r.meta.allow_edit)
		throw UnmodifabeValue();

	switch (val0_r.meta.vtype) {
	case VType::noting: val0_r = val1_r;break;
	case VType::i8:
		reinterpret_cast<int8_t&>(actual_val0) <<= (int8_t)val1_r;
		break;
	case VType::i16:
		reinterpret_cast<int16_t&>(actual_val0) <<= (int16_t)val1_r;
		break;
	case VType::i32:
		reinterpret_cast<int32_t&>(actual_val0) <<= (int32_t)val1_r;
		break;
	case VType::i64:
		reinterpret_cast<int64_t&>(actual_val0) <<= (int64_t)val1_r;
		break;
	case VType::ui8:
		reinterpret_cast<uint8_t&>(actual_val0) <<= (uint8_t)val1_r;
		break;
	case VType::ui16:
		reinterpret_cast<uint16_t&>(actual_val0) <<= (uint16_t)val1_r;
		break;
	case VType::ui32:
		reinterpret_cast<uint32_t&>(actual_val0) <<= (uint32_t)val1_r;
		break;
	case VType::ui64:
		reinterpret_cast<uint64_t&>(actual_val0) <<= (uint64_t)val1_r;
		break;
	case VType::undefined_ptr:
		reinterpret_cast<size_t&>(actual_val0) <<= (size_t)val1_r;
		break;
	case VType::proxy:
	case VType::morph:
	case VType::class_:
		AttachA::Interface::makeCall(ClassAccess::pub, val0_r, symbols::structures::bitwise_shift_left_operator, val1_r);
		break;
	default:
		throw InvalidCast("Fail cast value for shift left operation, cause value type is unsupported");
	}
}

void DynBitNot(void** val0) {
	ValueItem& val0_r = *reinterpret_cast<ValueItem*>(val0);
	val0_r.getAsync();
	void*& actual_val0 = val0_r.getSourcePtr();

	if (!val0_r.meta.allow_edit)
		throw UnmodifabeValue();

	switch (val0_r.meta.vtype) {
	case VType::noting: val0_r = ValueItem(0);break;
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
	case VType::proxy:
	case VType::morph:
	case VType::class_:
		AttachA::Interface::makeCall(ClassAccess::pub, val0_r, symbols::structures::bitwise_not_operator);
		break;
	default:
		throw InvalidCast("Fail cast value for not operation, cause value type is unsupported");
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
		*value = ValueItem(std::initializer_list<ValueItem>{ std::move(*(ValueItem*)(val)) });
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
			catch (AException&) {
				universalRemove(val);
				*val = new std::exception_ptr(std::current_exception());
				meta = VType::except_value;
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
		meta = VType::boolean;
		*value = (void*)1;
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
ValueItem::ValueItem(ValueItem&& move) {
	if(move.meta.vtype == VType::saarr && !move.meta.as_ref){
		ValueItem* farrr = new ValueItem[move.meta.val_len];
		ValueItem* src = (ValueItem*)move.getSourcePtr();
		for (size_t i = 0; i < move.meta.val_len; i++)
			farrr[i] = std::move(src[i]);
			
		val = farrr;
		meta = move.meta;
		meta.vtype = VType::faarr;
		move.val = nullptr;
	}

	val = move.val;
	meta = move.meta;
	move.val = nullptr;
}

#pragma region ValueItem constructors
ValueItem::ValueItem(const void* vall, ValueMeta vmeta) : val(0) {
	auto tmp = (void*)vall;
	val = copyValue(tmp, vmeta);
	meta = vmeta;
}
ValueItem::ValueItem(void* vall, ValueMeta vmeta, no_copy_t) {
	val = vall;
	meta = vmeta;
}
ValueItem::ValueItem(void* vall, ValueMeta vmeta, as_refrence_t) {
	val = vall;
	meta = vmeta;
	meta.as_ref = true;
}
ValueItem::ValueItem(const ValueItem& copy) : val(0) {
	ValueItem& tmp = (ValueItem&)copy;
	val = copyValue(tmp.val, tmp.meta);
	meta = copy.meta;
}
ValueItem::ValueItem(ValueItem& ref, as_refrence_t){
	val = ref.val;
	meta = ref.meta;
	meta.as_ref = true;
}

ValueItem::ValueItem(nullptr_t) : val(0), meta(0) {}
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
ValueItem::ValueItem(const std::string& set) {
	val = new std::string(set);
	meta = VType::string;
}
ValueItem::ValueItem(std::string&& set){
	val = new std::string(std::move(set));
	meta = VType::string;
}

ValueItem::ValueItem(const char* str) : val(0) {
	val = new std::string(str);
	meta = VType::string;
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
ValueItem::ValueItem(ValueItem* vals, uint32_t len, no_copy_t){
	*this = ValueItem(vals, ValueMeta(VType::faarr, false, true, len), no_copy);
}

ValueItem::ValueItem(void* undefined_ptr) {
	val = undefined_ptr;
	meta = VType::undefined_ptr;
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

ValueItem::ValueItem(int8_t* vals, uint32_t len, no_copy_t){
	*this = ValueItem(vals, ValueMeta(VType::raw_arr_i8, false, true, len), no_copy);
}
ValueItem::ValueItem(uint8_t* vals, uint32_t len, no_copy_t){
	*this = ValueItem(vals, ValueMeta(VType::raw_arr_ui8, false, true, len), no_copy);
}
ValueItem::ValueItem(int16_t* vals, uint32_t len, no_copy_t){
	*this = ValueItem(vals, ValueMeta(VType::raw_arr_i16, false, true, len), no_copy);
}
ValueItem::ValueItem(uint16_t* vals, uint32_t len, no_copy_t){
	*this = ValueItem(vals, ValueMeta(VType::raw_arr_ui16, false, true, len), no_copy);
}
ValueItem::ValueItem(int32_t* vals, uint32_t len, no_copy_t){
	*this = ValueItem(vals, ValueMeta(VType::raw_arr_i32, false, true, len), no_copy);
}
ValueItem::ValueItem(uint32_t* vals, uint32_t len, no_copy_t){
	*this = ValueItem(vals, ValueMeta(VType::raw_arr_ui32, false, true, len), no_copy);
}
ValueItem::ValueItem(int64_t* vals, uint32_t len, no_copy_t){
	*this = ValueItem(vals, ValueMeta(VType::raw_arr_i64, false, true, len), no_copy);
}
ValueItem::ValueItem(uint64_t* vals, uint32_t len, no_copy_t){
	*this = ValueItem(vals, ValueMeta(VType::raw_arr_ui64, false, true, len), no_copy);
}
ValueItem::ValueItem(float* vals, uint32_t len, no_copy_t){
	*this = ValueItem(vals, ValueMeta(VType::raw_arr_flo, false, true, len), no_copy);
}
ValueItem::ValueItem(double* vals, uint32_t len, no_copy_t){
	*this = ValueItem(vals, ValueMeta(VType::raw_arr_doub, false, true, len), no_copy);
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
		ValueItem* res = new ValueItem[len];
		ValueItem* copy = const_cast<ValueItem*>(args.begin());
		for(uint32_t i = 0;i< len;i++)
			res[i] = std::move(copy[i]);
		val = res;
	}
}
ValueItem::ValueItem(const std::exception_ptr& ex) {
	val = new std::exception_ptr(ex);
	meta = VType::except_value;
}
ValueItem::ValueItem(const std::chrono::steady_clock::time_point& time) {
	static_assert(sizeof(std::chrono::steady_clock::time_point) <= sizeof(val), "Time point is too large");
	*reinterpret_cast<std::chrono::steady_clock::time_point*>(&val) = time;
	meta = VType::time_point;
}
ValueItem::ValueItem(const std::unordered_map<ValueItem, ValueItem>& map): val(0){
	*this = ValueItem(new std::unordered_map<ValueItem, ValueItem>(map), VType::map);
}
ValueItem::ValueItem(std::unordered_map<ValueItem, ValueItem>&& map): val(0){
	*this = ValueItem(new std::unordered_map<ValueItem, ValueItem>(std::move(map)), VType::map);
}
ValueItem::ValueItem(const std::unordered_set<ValueItem>& set): val(0){
	*this = ValueItem(new std::unordered_set<ValueItem>(set), VType::set);
}
ValueItem::ValueItem(std::unordered_set<ValueItem>&& set): val(0){
	*this = ValueItem(new std::unordered_set<ValueItem>(std::move(set)), VType::set);
}


ValueItem::ValueItem(typed_lgr<class FuncEnviropment>& fun){
	val = new typed_lgr(fun);
	meta = VType::function;
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
	case VType::time_point:
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
	val = (void*)type.encoded;
	meta = VType::type_identifier;
}
#pragma endregion

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
ValueItem& ValueItem::operator=(ValueItem&& move) {
	if (val)
		if (!meta.as_ref)
			if (needAlloc(meta))
				universalFree(&val, meta);
	val = move.val;
	meta = move.meta;
	move.val = nullptr;
	return *this;
}
#pragma region ValueItem operators
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
ValueItem& ValueItem::operator<<=(const ValueItem& op) {
	DynBitShiftLeft(&val, (void**)&op.val);
	return *this;
}
ValueItem& ValueItem::operator>>=(const ValueItem& op) {
	DynBitShiftRight(&val, (void**)&op.val);
	return *this;
}
ValueItem& ValueItem::operator ++() {
	DynInc(&val);
	return *this;
}
ValueItem& ValueItem::operator --() {
	DynDec(&val);
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
#pragma endregion

#pragma region ValueItem cast operators
ValueItem::operator bool() {
	return ABI_IMPL::Vcast<bool>(val, meta);
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
	if(meta.vtype == VType::string)
		return *(std::string*)getSourcePtr();
	else
		return ABI_IMPL::Scast(val, meta);
}
ValueItem::operator void*() {
	return ABI_IMPL::Vcast<void*>(val, meta);
}
ValueItem::operator list_array<ValueItem>() {
	return ABI_IMPL::Vcast<list_array<ValueItem>>(val, meta);
}
ValueItem::operator ClassValue&() {
	if(meta.vtype == VType::async_res)
		getAsync();
	if (meta.vtype == VType::class_)
		return *(ClassValue*)getSourcePtr();
	else
		throw InvalidCast("This type is not class");
}
ValueItem::operator MorphValue&() {
	if(meta.vtype == VType::async_res)
		getAsync();
	if (meta.vtype == VType::morph)
		return *(MorphValue*)getSourcePtr();
	else
		throw InvalidCast("This type is not morph");
}
ValueItem::operator ProxyClass&() {
	if(meta.vtype == VType::async_res)
		getAsync();
	if (meta.vtype == VType::proxy)
		return *(ProxyClass*)getSourcePtr();
	else
		throw InvalidCast("This type is not proxy");
}

ValueItem::operator ValueMeta(){
	if(meta.vtype == VType::async_res)
		getAsync();
	if(meta.vtype == VType::type_identifier)
		return *(ValueMeta*)getSourcePtr();
	else
		return meta.vtype;
}
ValueItem::operator std::exception_ptr() {
	if(meta.vtype == VType::async_res)
		getAsync();
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
ValueItem::operator std::chrono::steady_clock::time_point(){
	if(meta.vtype == VType::async_res)
		getAsync();
	if (meta.vtype == VType::time_point)
		return *(std::chrono::steady_clock::time_point*)getSourcePtr();
	else
		throw InvalidCast("This type is not time_point");
}
ValueItem::operator std::unordered_map<ValueItem, ValueItem>&(){
	if(meta.vtype == VType::async_res)
		getAsync();
	if (meta.vtype == VType::map)
		return *(std::unordered_map<ValueItem, ValueItem>*)getSourcePtr();
	else
		throw InvalidCast("This type is not map");
}
ValueItem::operator std::unordered_set<ValueItem>&(){
	if(meta.vtype == VType::async_res)
		getAsync();
	if (meta.vtype == VType::set)
		return *(std::unordered_set<ValueItem>*)getSourcePtr();
	else
		throw InvalidCast("This type is not set");
}
#pragma endregion

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
const void*& ValueItem::getSourcePtr() const{
	return const_cast<const void*&>(getValue(*const_cast<void**>(&val), const_cast<ValueMeta&>(meta)));
}
typed_lgr<FuncEnviropment>* ValueItem::funPtr() {
	if (meta.vtype == VType::function)
		return (typed_lgr<FuncEnviropment>*)getValue(val, meta);
	return nullptr;
}
template<typename T>
size_t array_hash(T* arr, size_t len) {
	size_t hash = 0;
	for (size_t i = 0; i < len; i++)
		hash ^= std::hash<T>()(arr[i]) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
	return hash;
}
void ValueItem::make_gc(){
	if(meta.use_gc)
		return;
	if(needAllocType(meta.vtype)){
		void(*destructor)(void*) = nullptr;
		bool(*deph)(void*) = nullptr;
		switch (meta.vtype) {
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
		case VType::class_:
			destructor = defaultDestructor<ClassValue>;
			break;
		case VType::morph:
			destructor = defaultDestructor<MorphValue>;
			break;
		default:
			break;
		}
		val = new lgr(val, deph, destructor);
	}else{
		void(*destructor)(void*) = nullptr;
		void* new_val = nullptr;
		switch(meta.vtype){
		case VType::noting:
			break;
		case VType::i8:
			new_val = new int8_t(*(int8_t*)val);
			destructor = defaultDestructor<uint8_t>;
			break;
		case VType::ui8:
			new_val = new uint8_t(*(uint8_t*)val);
			destructor = defaultDestructor<uint8_t>;
			break;
		case VType::i16:
			new_val = new int16_t(*(int16_t*)val);
			destructor = defaultDestructor<uint16_t>;
			break;
		case VType::ui16:
			new_val = new uint16_t(*(uint16_t*)val);
			destructor = defaultDestructor<uint16_t>;
			break;
		case VType::i32:
			new_val = new int32_t(*(int32_t*)val);
			destructor = defaultDestructor<uint32_t>;
			break;
		case VType::ui32:
			new_val = new uint32_t(*(uint32_t*)val);
			destructor = defaultDestructor<uint32_t>;
			break;
		case VType::flo:
			new_val = new float(*(float*)val);
			destructor = defaultDestructor<uint32_t>;
			break;
		case VType::i64:
			new_val = new int64_t(*(int64_t*)val);
			destructor = defaultDestructor<uint64_t>;
			break;
		case VType::ui64:
			new_val = new uint64_t(*(uint64_t*)val);
			destructor = defaultDestructor<uint64_t>;
			break;
		case VType::undefined_ptr:
			new_val = new void*(*(void**)val);
			destructor = defaultDestructor<uint64_t>;
			break;
		case VType::doub:
			new_val = new double(*(double*)val);
			destructor = defaultDestructor<uint64_t>;
			break;
		case VType::type_identifier:
			new_val = new ValueMeta(*(ValueMeta*)val);
			destructor = defaultDestructor<uint64_t>;
			break;
		case VType::saarr:
			*this = ValueItem((ValueItem*)val,meta.val_len);
			make_gc();
			return;
		}
		val = new lgr(new_val, nullptr, destructor);
	}
}
void ValueItem::localize_gc(){
	if(meta.use_gc && val != nullptr) {
		ValueMeta fake_meta = meta;
		fake_meta.use_gc = false;
		void* new_val = copyValue(getSourcePtr(), fake_meta);
		new_val = new lgr(new_val, ((lgr*)val)->getCalcDepth(),((lgr*)val)->getDestructor());
		delete (lgr*)val;
		val = new_val;
	}
}
void ValueItem::ungc(){
	if(meta.use_gc && val != nullptr) {
		meta.use_gc = false;
		lgr* tmp = (lgr*)val;
		if(tmp->is_deleted()) {
			meta.encoded = 0;
			val = nullptr;
			delete tmp;
			return;
		}
		void* new_val = tmp->try_take_ptr();
		if(new_val == nullptr){
			void* new_val = copyValue(getSourcePtr(), meta);
			delete (lgr*)val;
			val = new_val;
		}
		
	}
}
bool ValueItem::is_gc(){
	return meta.use_gc;
}
ValueItem ValueItem::make_slice(uint32_t start, uint32_t end) const {
	if(meta.val_len < end) end = meta.val_len;
	if(start > end) start = end;
	ValueMeta res_meta = meta;
	res_meta.val_len = end - start;
	if(end == start) return ValueItem(nullptr, res_meta, as_refrence);
	switch (meta.vtype) {
	case VType::saarr:
	case VType::faarr:
		return ValueItem((ValueItem*)val + start, res_meta, as_refrence);
	case VType::raw_arr_ui8:
	case VType::raw_arr_i8:
		return ValueItem((uint8_t*)val + start, res_meta, as_refrence);
	case VType::raw_arr_ui16:
	case VType::raw_arr_i16:
		return ValueItem((uint16_t*)val + start, res_meta, as_refrence);
	case VType::raw_arr_ui32:
	case VType::raw_arr_i32:
	case VType::raw_arr_flo:
		return ValueItem((uint32_t*)val + start, res_meta, as_refrence);
	case VType::raw_arr_ui64:
	case VType::raw_arr_i64:
	case VType::raw_arr_doub:
		return ValueItem((uint64_t*)val + start, res_meta, as_refrence);
	
	default:
		throw InvalidOperation("Can't make slice of this type: " + enum_to_string(meta.vtype));
	}
}
size_t ValueItem::hash() const{
	return const_cast<ValueItem&>(*this).hash();
}
size_t ValueItem::hash() {
	getAsync();
	switch (meta.vtype) {
	case VType::noting:return 0;
	case VType::type_identifier:
	case VType::boolean:
	case VType::i8:return std::hash<int8_t>()((int8_t)*this);
	case VType::i16:return std::hash<int16_t>()((int16_t)*this);
	case VType::i32:return std::hash<int32_t>()((int32_t)*this);
	case VType::i64:return std::hash<int64_t>()((int64_t)*this);
	case VType::ui8:return std::hash<uint8_t>()((uint8_t)*this);
	case VType::ui16:return std::hash<uint16_t>()((uint16_t)*this);
	case VType::ui32:return std::hash<uint32_t>()((uint32_t)*this);
	case VType::undefined_ptr: return std::hash<uint32_t>()((size_t)*this);
	case VType::ui64: return std::hash<uint64_t>()((uint64_t)*this);
	case VType::flo: return std::hash<float>()((float)*this);
	case VType::doub: return std::hash<double>()((double)*this);
	case VType::string: return std::hash<std::string>()(*(std::string*)getSourcePtr());
	case VType::uarr: return std::hash<list_array<ValueItem>>()(*(list_array<ValueItem>*)getSourcePtr());
	case VType::raw_arr_i8: return array_hash((int8_t*)getSourcePtr(), meta.val_len);
	case VType::raw_arr_i16: return array_hash((int16_t*)getSourcePtr(), meta.val_len);
	case VType::raw_arr_i32: return array_hash((int32_t*)getSourcePtr(), meta.val_len);
	case VType::raw_arr_i64: return array_hash((int64_t*)getSourcePtr(), meta.val_len);
	case VType::raw_arr_ui8: return array_hash((uint8_t*)getSourcePtr(), meta.val_len);
	case VType::raw_arr_ui16: return array_hash((uint16_t*)getSourcePtr(), meta.val_len);
	case VType::raw_arr_ui32: return array_hash((uint32_t*)getSourcePtr(), meta.val_len);
	case VType::raw_arr_ui64: return array_hash((uint64_t*)getSourcePtr(), meta.val_len);
	case VType::raw_arr_flo: return array_hash((float*)getSourcePtr(), meta.val_len);
	case VType::raw_arr_doub: return array_hash((double*)getSourcePtr(), meta.val_len);

	case VType::saarr:
	case VType::faarr: return array_hash((ValueItem*)getSourcePtr(), meta.val_len);

	case VType::class_: 
	case VType::morph:
	case VType::proxy:
		{
			if(AttachA::Interface::hasImplement(*this, "hash"))
				return  (size_t)AttachA::Interface::makeCall(ClassAccess::pub, *this, "hash");
			else
				return std::hash<void*>()(getSourcePtr());
		}
	case VType::set: {
		size_t hash = 0;
		for (auto& i : operator std::unordered_set<ValueItem>&())
			hash ^= const_cast<ValueItem&>(i).hash() + 0x9e3779b9 + (hash << 6) + (hash >> 2);
		return hash;
	}
	case VType::map:{
		size_t hash = 0;
		for (auto& i : operator std::unordered_map<ValueItem, ValueItem>&())
			hash ^= const_cast<ValueItem&>(i.first).hash() + 0x9e3779b9 + (hash << 6) + (hash >> 2) + const_cast<ValueItem&>(i.second).hash() + 0x9e3779b9 + (hash << 6) + (hash >> 2);
		return hash;
	}
	case VType::function: {
		auto fn = funPtr();
		if (fn)
			return std::hash<void*>()(fn->getPtr());
		else
			return 0;
	}
	case VType::async_res:  throw InternalException("getAsync() not work in hash function");
	case VType::except_value: throw InvalidOperation("Hash function for exception not available", *(std::exception_ptr*)getSourcePtr());
	default:
		throw NotImplementedException();
		break;
	}
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
				if (tmp.access != ClassAccess::intern)
					return tmp.fn;
				break;
			case ClassAccess::prot:
				if (tmp.access == ClassAccess::pub || tmp.access == ClassAccess::prot)
					return tmp.fn;
				break;
			case ClassAccess::intern:
				if(allow_intern_access) return tmp.fn;
				else throw InvalidOperation("Internal access not allowed by configuration");
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
			if (tmp.access != ClassAccess::intern)
				return tmp.val;
			break;
		case ClassAccess::prot:
			if (tmp.access == ClassAccess::pub || tmp.access == ClassAccess::prot)
				return tmp.val;
			break;
		case ClassAccess::intern:
			if(allow_intern_access) return tmp.val;
			else throw InvalidOperation("Internal access not allowed by configuration");
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
			if (tmp.access != ClassAccess::intern)
				return tmp.fn;
			break;
		case ClassAccess::prot:
			if (tmp.access == ClassAccess::pub || tmp.access == ClassAccess::prot)
				return tmp.fn;
			break;
		case ClassAccess::intern:
			if(allow_intern_access) return tmp.fn;
			else throw InvalidOperation("Internal access not allowed by configuration");
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
			if (tmp.access != ClassAccess::intern)
				return tmp.val;
			break;
		case ClassAccess::prot:
			if (tmp.access == ClassAccess::pub || tmp.access == ClassAccess::prot)
				return tmp.val;
			break;
		case ClassAccess::intern:			
			if(allow_intern_access) return tmp.val;
			else throw InvalidOperation("Internal access not allowed by configuration");
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
			if (tmp.access != ClassAccess::intern) {
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
		case ClassAccess::intern:
			if(allow_intern_access)tmp.val = set_val;
			else throw InvalidOperation("Internal access not allowed by configuration");
			return;
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




ProxyClassDefine::ProxyClassDefine() :name("") {}
ProxyClassDefine::ProxyClassDefine(const std::string& name) :name(name) { }
ProxyClass::ProxyClass() {
	class_ptr = nullptr;
	declare_ty = nullptr;
}

ProxyClass::ProxyClass(ProxyClass& other){
	if (other.declare_ty)
		if(other.declare_ty->copy)
			class_ptr = other.declare_ty->copy(other.class_ptr);
		else
			throw NotImplementedException();
	declare_ty = other.declare_ty;
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
		if(declare_ty->destructor)
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
				if (tmp.access != ClassAccess::intern)
					return tmp.fn;
				break;
			case ClassAccess::prot:
				if (tmp.access == ClassAccess::pub || tmp.access == ClassAccess::prot)
					return tmp.fn;
				break;
			case ClassAccess::intern:
				if(allow_intern_access) return tmp.fn;
				else throw InvalidOperation("Internal access not allowed by configuration");				
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