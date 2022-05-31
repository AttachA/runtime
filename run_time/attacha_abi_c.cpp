// Copyright Danyil Melnytskyi 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include "attacha_abi.hpp"
#include "run_time_compiler.hpp"
#include "tasks.hpp"
#include <string>
#include <sstream>


#include <mutex>



bool needAlloc(VType type) {
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
	default:
		delete *value;
	}
	return;
gc_destruct:
	delete *(lgr**)value;
}
void universalRemove(void** value) {
	ValueMeta& meta = *reinterpret_cast<ValueMeta*>(value + 1);
	if (!meta.encoded)
		return;
	if (needAlloc(meta.vtype))
		universalFree(value, meta);
	meta.encoded = 0;
}
void universalAlloc(void** value, ValueMeta meta) {
	if (*value)
		universalRemove(value);
	if (needAlloc(meta.vtype)) {
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
		}
	}
	if (meta.use_gc) {
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
		case VType::raw_arr_ui64:
		case VType::raw_arr_i64:
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

auto gcCall(lgr* gc, list_array<ValueItem>* args, bool async_mode) {
	return FuncEnviropment::CallFunc(*(std::string*)gc->getPtr(), args, async_mode);
}
ValueItem* getAsyncValueItem(void* val) {
	typed_lgr<Task>& tmp = *(typed_lgr<Task>*)val;
	return Task::getResult(tmp);
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
		auto res = getAsyncValueItem(value);
		void* moveValue = res->val;
		void* new_meta = (void*)res->meta.encoded;
		res->val = nullptr;
		universalFree(&value, meta);
		value = moveValue;
		meta.encoded = (size_t)new_meta;
	}
}
void* copyValue(void*& val, ValueMeta& meta) {
	getAsyncResult(val, meta);

	void* actual_val = val;
	if (meta.use_gc)
		actual_val = ((lgr*)val)->getPtr();
	if (needAlloc(meta.vtype)) {
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
		default:
			throw NotImplementedException();
		}
	}
	else return actual_val;
}
void copyEnviropement(void** env, uint16_t env_it_count, void*** res);
void** copyEnviropement(void** env, uint16_t env_it_count) {
	uint32_t it_count = env_it_count;
	it_count <<= 1;
	void** new_env = new void* [it_count];
	if (new_env == nullptr)
		throw EnviropmentRuinException();
	for (uint32_t i = 0; i < it_count; i += 2)
		new_env[i] = copyValue(env[i], *(ValueMeta*)(&(new_env[i & 1] = env[i & 1])));
}


void** preSetValue(void** value, ValueMeta set_meta, bool match_gc_dif) {
	void** res = getValueLink(value);
	ValueMeta& meta = *(ValueMeta*)(value + 1);
	if (!needAlloc(meta.vtype)) {
		if (match_gc_dif) {
			if (meta.allow_edit && meta.vtype == set_meta.vtype && meta.use_gc == set_meta.use_gc)
				return res;
		}
		else
			if (meta.allow_edit && meta.vtype == set_meta.vtype)
				return res;
	}
	universalRemove(value);
	universalAlloc(value, set_meta);
	return getValueLink(value);
}
void*& getValue(void*& value, ValueMeta& meta) {
	if (meta.vtype == VType::async_res)
		getAsyncResult(value, meta);
	if (meta.use_gc)
		if (((lgr*)value)->is_deleted()) {
			universalFree(&value, meta);
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
			universalFree(value, meta);
			meta = ValueMeta(0);
		}
	return meta.use_gc ? (**(lgr*)value) : *value;
}
void* getSpecificValue(void** value, VType typ) {
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
#pragma warning(push)  
#pragma warning( disable: 4311)
#pragma warning( disable: 4302)
//return equal,lower bool result
std::pair<bool, bool> compareValue(VType cmp1, VType cmp2, void* val1, void* val2) {
	bool cmp_int;
	if (cmp_int = (is_integer(cmp1) || cmp1 == VType::undefined_ptr) != (is_integer(cmp2) || cmp1 == VType::undefined_ptr))
		return { false,false };
	if (cmp_int) {
		if (val1 == val2)
			return { true,false };

		bool temp1 = (integer_unsigned(cmp1) || cmp1 == VType::undefined_ptr);
		bool temp2 = (integer_unsigned(cmp2) || cmp2 == VType::undefined_ptr);
		if (temp1 && temp2)
			return { false, uint64_t(val1) < uint64_t(val2) };
		else if (temp1)
			switch (cmp2) {
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
			switch (cmp1) {
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
			switch (cmp1) {
			case VType::i8:
				switch (cmp1) {
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
				switch (cmp1) {
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
				switch (cmp1) {
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
				switch (cmp1) {
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
				switch (cmp1) {
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
				switch (cmp1) {
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
	else if (cmp1 == VType::string && cmp2 == VType::string) {
		if (*(std::string*)val1 == *(std::string*)val2)
			return { true, false };
		else
			return { false, *(std::string*)val1 < *(std::string*)val2 };
	}
	else if (cmp1 == VType::uarr && cmp2 == VType::uarr) {
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
				auto tmp = arr1.begin();
				for (auto& it : arr1) {
					auto& first = *tmp;
					void* val1 = getValue(first.val, first.meta);
					void* val2 = getValue(it.val, it.meta);
					auto res = compareValue(first.meta.vtype, it.meta.vtype, first.val, it.val);
					if (!res.first)
						return res;
					++tmp;
				}
				return { true, false };
			}
			else return { false, false };
		}
	}
	else
		return { cmp1 == cmp2, false };
}
RFLAGS compare(RFLAGS old, void** value_1, void** value_2) {
	void* val1 = getValue(value_1);
	void* val2 = getValue(value_2);
	ValueMeta cmp1 = *(ValueMeta*)(value_1 + 1);
	ValueMeta cmp2 = *(ValueMeta*)(value_2 + 1);

	old.parity = old.auxiliary_carry = old.sign_f = old.overflow = 0;
	auto res = compareValue(cmp1.vtype, cmp2.vtype, val1, val2);
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
	auto res = compareValue(cmp1.vtype, cmp2.vtype, val1, val2);
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

void copyEnviropement(void** env, uint16_t env_it_count, void*** res) {
	uint32_t it_count = env_it_count;
	it_count <<= 1;
	void**& new_env = *res;
	if (new_env == nullptr)
		throw EnviropmentRuinException();
	for (uint32_t i = 0; i < it_count; i += 2)
		new_env[i] = copyValue(env[i], (ValueMeta&)(new_env[i & 1] = env[i & 1]));
}







namespace ABI_IMPL {

	ValueItem* _Vcast_callFN(void* ptr) {
		return FuncEnviropment::syncCall(*(typed_lgr<FuncEnviropment>*)ptr, nullptr);
	}

	std::string Scast(void*& val, ValueMeta& meta) {
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
			return reinterpret_cast<std::string&>(val);
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
	case VType::uarr:
		if (val1_meta.vtype == VType::uarr)
			reinterpret_cast<list_array<ValueItem>&>(actual_val0).insert(reinterpret_cast<list_array<ValueItem>&>(actual_val0).size() - 1, reinterpret_cast<list_array<ValueItem>&>(actual_val1));
		else
			reinterpret_cast<list_array<ValueItem>&>(actual_val0).push_back(ValueItem(copyValue(actual_val1,val1_meta), val1_meta));
		break;
	case VType::string:
		throw InvalidOperation("for strings multiply operation is not defined");
	case VType::undefined_ptr:
		reinterpret_cast<size_t&>(actual_val0) *= ABI_IMPL::Vcast<size_t>(actual_val1, val1_meta);
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
	if (meta.vtype == VType::uarr)
		return *val;
	else {
		auto tmp = new list_array<ValueItem>(1);
		tmp->operator[](0) = ValueItem(*val, meta);
		universalRemove(val);
		*val = tmp;
		meta.allow_edit = true;
		meta.use_gc = false;
		meta.vtype = VType::uarr;
		return tmp;
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
	switch (reinterpret_cast<ValueMeta*>(value + 1)->vtype) {
	case VType::noting:
		return false;
	case VType::i8:
	case VType::ui8:
		return (uint8_t)*value;
	case VType::i16:
	case VType::ui16:
		return (uint16_t)*value;
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

	if (meta.use_gc) {
		switch (meta.vtype) {
		case VType::undefined_ptr:
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
		}
	}
	switch (meta.vtype) {
	case VType::noting:
		meta = ValueMeta(VType::i8, false, true);
		__fallthrough;
	case VType::undefined_ptr:
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
		*value = (void*)1;
		break;
	default:
		universalRemove(value);
		meta = ValueMeta(VType::i8, false, true);
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
		universalFree(val, *(ValueMeta*)(val + 1));
		(*val) = (*(val + 1)) = nullptr;
	}
	void continue_unwind(void** val) {
		std::rethrow_exception(*((std::exception_ptr*)getSpecificValueLink(val, VType::except_value)));
	}
	bool call_except_handler(void** val, bool(*func_symbol)(void** val), bool ignore_fault) {
		try {
			if (func_symbol(getSpecificValueLink(val, VType::except_value))) {
				universalFree(val, *(ValueMeta*)(val + 1));
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
		actual = tmp;
		if (tmp != actual)
			throw NumericUndererflowException();
		return actual;
	}
	case VType::doub: {
		double tmp = *(double*)&res;
		actual = tmp;
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



ValueItem::ValueItem(void* vall, ValueMeta vmeta) : val(0) {
	val = copyValue(vall, vmeta);
	meta = vmeta;
}
ValueItem::ValueItem(void* vall, ValueMeta vmeta, bool) {
	val = vall;
	meta = vmeta;
}
ValueItem::ValueItem(const ValueItem& copy) : val(0) {
	ValueItem& tmp = (ValueItem&)copy;
	val = copyValue(tmp.val, tmp.meta);
	meta = copy.meta;
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
ValueItem::ValueItem(void* vall, VType type) : val(0) {
	ValueMeta vmeta(type);
	val = copyValue(vall, vmeta);
	meta = vmeta;
}
ValueItem::ValueItem(void* vall, VType type, bool no_copy) : val(0) {
	val = vall;
	meta = type;
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
		catch(AException&ex) {
			val = new std::exception_ptr(std::make_exception_ptr(ex));
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
ValueItem::~ValueItem() {
	if (val)
		if (needAlloc(meta.vtype))
			universalFree(&val, meta);
}

ValueItem& ValueItem::operator=(const ValueItem& copy) {
	ValueItem& tmp = (ValueItem&)copy;
	if (val)
		if (needAlloc(meta.vtype))
			universalFree(&val, meta);
	val = copyValue(tmp.val, tmp.meta);
	meta = copy.meta;
	return *this;
}
ValueItem& ValueItem::operator=(ValueItem&& move) noexcept {
	if (val)
		if (needAlloc(meta.vtype))
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
ValueItem::operator list_array<ValueItem>() {
	return ABI_IMPL::Vcast<list_array<ValueItem>>(val, meta);
}
ValueItem* ValueItem::operator()(list_array<ValueItem>* args) {
	if (meta.vtype == VType::function)
		return FuncEnviropment::syncCall((*(typed_lgr<FuncEnviropment>*)getValue(val, meta)),args);
	else
		return new ValueItem(*this);
}

void ValueItem::getAsync() {
	while (meta.vtype == VType::async_res)
		getAsyncResult(val, meta);
}

void*& ValueItem::getSourcePtr() {
	return getValue(val, meta);
}




ClassDefine::ClassDefine():name("Unnamed") { }
ClassDefine::ClassDefine(const std::string& name) { this->name = name; }

typed_lgr<class FuncEnviropment> ClassValue::callFnPtr(const std::string & str, ClassAccess acces) {
	if (define) {
		if (define->funs.contains(str)) {
			auto& tmp = define->funs[str];
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
ValueItem& ClassValue::getValue(const std::string& str, ClassAccess acces) {
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
ValueItem ClassValue::copyValue(const std::string& str, ClassAccess acces) {
	if (val.contains(str)) {
		return getValue(str, acces);
	}
	return ValueItem();
}


typed_lgr<class FuncEnviropment> MorphValue::callFnPtr(const std::string & str, ClassAccess acces) {
	if (define.funs.contains(str)) {
		auto& tmp = define.funs[str];
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
ValueItem& MorphValue::getValue(const std::string& str, ClassAccess acces) {
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
ValueItem MorphValue::copyValue(const std::string& str, ClassAccess acces) {
	if (val.contains(str)) {
		return getValue(str, acces);
	}
	return ValueItem();
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

typed_lgr<class FuncEnviropment> ProxyClass::callFnPtr(const std::string& str, ClassAccess acces) {
	if (declare_ty) {
		auto& define = *declare_ty;
		if (define.funs.contains(str)) {
			auto& tmp = define.funs[str];
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
ValueItem ProxyClass::getValue(const std::string& str) {
	if (declare_ty) {
		if (declare_ty->value_geter.contains(str)) {
			auto& tmp = declare_ty->value_geter[str];
			return tmp(class_ptr);
		}
	}
	throw NotImplementedException();
}
void* ProxyClass::setValue(const std::string& str, ValueItem& it) {
	if (declare_ty) {
		if (declare_ty->value_seter.contains(str)) {
			auto& tmp = declare_ty->value_seter[str];
			tmp(class_ptr, it);
		}
	}
	throw NotImplementedException();
}