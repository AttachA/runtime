#include "attacha_abi.hpp"
#include "run_time_compiler.hpp"
#include "../libray/list_array.hpp"
#include <future>
#include <string>
#include <sstream>


#include <mutex>

std::list<FuncRes*> to_delete_ignored_async_res;
std::condition_variable to_delete_ignored_async_res_cond;

void ignoredAsyncGC() {
	std::mutex mut;
	std::unique_lock local_lock(mut);
	while (true) {
		while (to_delete_ignored_async_res.empty())
			to_delete_ignored_async_res_cond.wait(local_lock);
		std::list<FuncRes*> tmp;
		std::swap(to_delete_ignored_async_res, tmp);
		for (auto it : (tmp))
			delete it;
	}
}
void ignoredAsync(FuncRes* fres) {
	to_delete_ignored_async_res.push_back(fres);
	to_delete_ignored_async_res_cond.notify_one();
}












bool needAlloc(VType type) {
	switch (type) {
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
	list_array<ArrItem>& items = *(list_array<ArrItem>*)ptr;
	for (ArrItem& it : items)
		if (it.meta.use_gc)
			if (!((lgr*)it.val)->deph_safe())
				return false;
	return true;
}

VType valueType(void** value) {
	return (*(ValueMeta*)value).vtype;
}



void universalFree(void** value, ValueMeta meta) {
	if (!value)
		return;
	if (!*value)
		return;
	if (meta.use_gc)
		goto gc_destruct;
	switch (meta.vtype) {
	case VType::uarr:
		delete (list_array<ArrItem>*)*value;
		return;
	case VType::string:
		delete (std::string*)*value;
		return;
	case VType::async_res: 
		{
			auto tmp = (FuncRes*)((std::future<FuncRes*>*)*value)->get();
			if (tmp)
				delete tmp; 
		}
		delete (std::future<FuncRes*>*)*value;
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
		switch (meta.vtype)
		{
		case VType::uarr:
			Allocate<list_array<ArrItem>>(value);
			break;
		case VType::string:
			Allocate<std::string>(value);
			break;
		}
	}
	if (meta.use_gc) {
		void(*destructor)(void*) = nullptr;
		bool(*deph)(void*) = nullptr;
		switch (meta.vtype)
		{
		case VType::uarr:
			destructor = defaultDestructor<list_array<ArrItem>>;
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
void removeArgsEnviropement(list_array<ArrItem>* env) {
	delete env;
}
char* getStrBegin(std::string* str) {
	return &(str->operator[](0));
}
void throwInvalidType() {
	throw InvalidType("Requested specifed type but recuived another");
}

auto gcCall(lgr* gc, list_array<ArrItem>* args, bool async_mode) {
	return FuncEnviropment::CallFunc(*(std::string*)gc->getPtr(), args, async_mode);
}
FuncRes* getAsyncFuncRes(void* val) {
	return ((std::future<FuncRes*>*)val)->get();
}
void getFuncRes(void** value, FuncRes* f_res) {
	universalRemove(value);
	if (f_res) {
		*value = f_res->value;
		*(value + 1) = (void*)f_res->meta;
		f_res->value = nullptr;
		f_res->meta = 0;
		delete f_res;
	}
}
FuncRes* buildRes(void** value) {
	FuncRes* res;
	try {
		res = new FuncRes();
	}
	catch (const std::bad_alloc& ex)
	{
		throw EnviropmentRuinException();
	}
	res->value = *value;
	res->meta = (size_t) * (value + 1);
	*value = *(value + 1) = nullptr;
	return res;
}


void getAsyncResult(void*& value, ValueMeta& meta) {
	if (meta.vtype == VType::async_res) {
		auto res = getAsyncFuncRes(meta.use_gc ? ((lgr*)value)->getPtr() : value);
		void* moveValue = res->value;
		void* new_meta = (void*)res->meta;
		res->value = nullptr;
		universalFree(&value, meta);
		value = moveValue;
		meta.encoded = (size_t)new_meta;
	}
}
void* copyValue(void*& val, ValueMeta& meta) {
	while (meta.vtype == VType::async_res)
		getAsyncResult(val, meta);

	void* actual_val = val;
	if (meta.use_gc)
		actual_val = ((lgr*)val)->getPtr();
	if (needAlloc(meta.vtype)) {
		switch (meta.vtype)
		{
		case VType::uarr:
			return new list_array<ArrItem>(*(list_array<ArrItem>*)val);
		case VType::string:
			return new std::string(*(std::string*)val);
		case VType::async_res: {
			auto tmp = getAsyncFuncRes(val);
			meta.encoded = tmp->meta;
			val = tmp->value;
			delete tmp;
			return copyValue(val, meta);
		}
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
	if (match_gc_dif) {
		if (meta.allow_edit && meta.vtype == set_meta.vtype && meta.use_gc == set_meta.use_gc)
			return res;
	}
	else
		if (meta.allow_edit && meta.vtype == set_meta.vtype)
			return res;
	universalRemove(value);
	universalAlloc(value, set_meta);
	return getValueLink(value);
}
void*& getValue(void*& value, ValueMeta& meta) {
	if (meta.vtype == VType::async_res)
		getAsyncResult(value, meta);
	return meta.use_gc ? (**(lgr*)value) : value;
}
void*& getValue(void** value) {
	ValueMeta& meta = *(ValueMeta*)(value + 1);
	if (meta.vtype == VType::async_res)
		getAsyncResult(*value, meta);
	return meta.use_gc ? (**(lgr*)value) : *value;
}
void* getSpecificValue(void** value, VType typ) {
	ValueMeta& meta = *(ValueMeta*)(value + 1);
	if (meta.vtype == VType::async_res)
		getAsyncResult(*value, meta);
	if (meta.vtype != typ)
		throw InvalidType("Requested specifed type but recuived another");
	return meta.use_gc ? ((lgr*)value)->getPtr() : *value;
}
void** getSpecificValueLink(void** value, VType typ) {
	ValueMeta& meta = *(ValueMeta*)(value + 1);
	if (meta.vtype == VType::async_res)
		getAsyncResult(*value, meta);
	if (meta.vtype != typ)
		throw InvalidType("Requested specifed type but recuived another");
	return meta.use_gc ? (&**(lgr*)value) : value;
}

void** getValueLink(void** value) {
	ValueMeta& meta = *(ValueMeta*)(value + 1);
	if (meta.vtype == VType::async_res)
		getAsyncResult(*value, meta);
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

//return equal,lower bool result
std::pair<bool, bool> compareValue(VType cmp1, VType cmp2, void* val1, void* val2) {
	bool cmp_int;
	if (cmp_int = (is_integer(cmp1) || cmp1 == VType::undefined_ptr) != (is_integer(cmp2) || cmp1 == VType::undefined_ptr))
		return { false,false };
	if (cmp_int) {
		if (val1 == val2)
			return { true,false };

		bool temp1 = (integer_unsigned(cmp1) || cmp1 == VType::undefined_ptr);
		bool temp2 = (integer_unsigned(cmp2) || cmp1 == VType::undefined_ptr);
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
				break;
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
		if (cmp1 == cmp2)
			return { true,false };
		else if (!calc_safe_deph_arr(val1))
			return { false,false };
		else if (!calc_safe_deph_arr(val2))
			return { false,false };
		else {
			auto& arr1 = *(list_array<ArrItem>*)val1;
			auto& arr2 = *(list_array<ArrItem>*)val2;
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

void copyEnviropement(void** env, uint16_t env_it_count, void*** res) {
	uint32_t it_count = env_it_count;
	it_count <<= 1;
	void**& new_env = *res;
	if (new_env == nullptr)
		throw EnviropmentRuinException();
	for (uint32_t i = 0; i < it_count; i += 2)
		new_env[i] = copyValue(env[i], (ValueMeta&)(new_env[i & 1] = env[i & 1]));
}



ArrItem::ArrItem(void* vall, ValueMeta vmeta) {
	val = copyValue(vall, vmeta);
	meta = vmeta;
}
ArrItem::ArrItem(void* vall, ValueMeta vmeta, bool) {
	val = vall;
	meta = vmeta;
}
ArrItem::ArrItem(const ArrItem& copy) {
	ArrItem& tmp = (ArrItem&)copy;
	val = copyValue(tmp.val, tmp.meta);
	meta = copy.meta;
}

ArrItem& ArrItem::operator=(const ArrItem& copy) {
	ArrItem& tmp = (ArrItem&)copy;
	val = copyValue(tmp.val, tmp.meta);
	meta = copy.meta;
	return *this;
}
ArrItem& ArrItem::operator=(ArrItem&& move) noexcept {
	val = move.val;
	meta = move.meta;
	move.val = nullptr;
	return *this;
}
ArrItem::~ArrItem() {
	if (val)
		if (needAlloc(meta.vtype))
			universalFree(&val, meta);
}


FuncRes::~FuncRes() {
	universalFree(&value, *reinterpret_cast<ValueMeta*>(&meta));
}





namespace ABI_IMPL {
	std::string Scast(void*& val, ValueMeta meta) {
		switch (meta.vtype) {
		case VType::noting:
			return "null";
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
			for (auto& it : *reinterpret_cast<list_array<ArrItem>*>(val)) {
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
			return (std::ostringstream() << val).str();
			break;
		default:
			throw InvalidCast("Fail cast undefined type");
		}
	}
}





void DynSum(void** val0, void** val1) {
	ValueMeta& val0_meta = *((ValueMeta*)val0 + 1);
	ValueMeta& val1_meta = *((ValueMeta*)val1 + 1);
	void*& actual_val0 = *val0;
	void*& actual_val1 = *val1;
	while (val0_meta.vtype == VType::async_res)
		getAsyncResult(actual_val0, val0_meta);
	while (val1_meta.vtype == VType::async_res)
		getAsyncResult(actual_val1, val1_meta);

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
		reinterpret_cast<list_array<ArrItem>&>(actual_val0).push_back(ArrItem(copyValue(actual_val1,val1_meta), val1_meta));
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
	while (val0_meta.vtype == VType::async_res)
		getAsyncResult(actual_val0, val0_meta);
	while (val1_meta.vtype == VType::async_res)
		getAsyncResult(actual_val1, val1_meta);

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
		reinterpret_cast<list_array<ArrItem>&>(actual_val0).push_front(ArrItem(copyValue(actual_val1,val1_meta), val1_meta));
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
	while (val0_meta.vtype == VType::async_res)
		getAsyncResult(actual_val0, val0_meta);
	while (val1_meta.vtype == VType::async_res)
		getAsyncResult(actual_val1, val1_meta);

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
			reinterpret_cast<list_array<ArrItem>&>(actual_val0).insert(reinterpret_cast<list_array<ArrItem>&>(actual_val0).size() - 1, reinterpret_cast<list_array<ArrItem>&>(actual_val1));
		else
			reinterpret_cast<list_array<ArrItem>&>(actual_val0).push_back(ArrItem(copyValue(actual_val1,val1_meta), val1_meta));
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
	while (val0_meta.vtype == VType::async_res)
		getAsyncResult(actual_val0, val0_meta);
	while (val1_meta.vtype == VType::async_res)
		getAsyncResult(actual_val1, val1_meta);

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
			reinterpret_cast<list_array<ArrItem>&>(actual_val0).insert(0, reinterpret_cast<list_array<ArrItem>&>(actual_val1));
		else
			reinterpret_cast<list_array<ArrItem>&>(actual_val0).push_front(ArrItem(copyValue(actual_val1,val1_meta), val1_meta));
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


void DynBitXor(void** val0, void** val1) {
	ValueMeta& val0_meta = *((ValueMeta*)val0 + 1);
	ValueMeta& val1_meta = *((ValueMeta*)val1 + 1);
	void*& actual_val0 = *val0;
	void*& actual_val1 = *val1;
	while (val0_meta.vtype == VType::async_res)
		getAsyncResult(actual_val0, val0_meta);
	while (val1_meta.vtype == VType::async_res)
		getAsyncResult(actual_val1, val1_meta);

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
	while (val0_meta.vtype == VType::async_res)
		getAsyncResult(actual_val0, val0_meta);
	while (val1_meta.vtype == VType::async_res)
		getAsyncResult(actual_val1, val1_meta);

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
	while (val0_meta.vtype == VType::async_res)
		getAsyncResult(actual_val0, val0_meta);
	while (val1_meta.vtype == VType::async_res)
		getAsyncResult(actual_val1, val1_meta);

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
	while (val0_meta.vtype == VType::async_res)
		getAsyncResult(actual_val0, val0_meta);

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
		auto tmp = new list_array<ArrItem>(1);
		tmp->operator[](0) = ArrItem(*val, meta);
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
		auto tmp = new list_array<ArrItem>(1);
		tmp->operator[](0) = ArrItem(*val, meta);
		universalRemove(val);
		*val = tmp;
		meta.allow_edit = true;
		meta.use_gc = false;
		meta.vtype = VType::uarr;
	}
}


namespace exception_abi {
	bool is_except(void** val) {
		return ((ValueMeta*)(val + 1))->vtype == VType::async_res;
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
	void call_except_handler(void** val, bool(*func_symbol)(void** val), bool ignore_fault) {
		try {
			getSpecificValueLink(val, VType::except_value);
			if (func_symbol(val)) {
				universalFree(val, *(ValueMeta*)(val + 1));
				(*val) = (*(val + 1)) = nullptr;
			}
		}
		catch (...) {
			if (!ignore_fault)
				throw;
		}
	}
	ptrdiff_t switch_jump_handle_except(void** val, jump_handle_except* handlers, size_t handlers_c) {
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
				if (size_t pos = info.ty_arr.find_it([handlers, i](const CXXExInfo::Tys& ty) {return ty.ty_info->name() == handlers[i].type_name; }))
					return handlers[i].jump_off;

		}
		return handlers[0].jump_off;
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
	default:
		throw InvalidType("Need sizable type");
	}
	if (sig != actual)
		throw NumericUndererflowException();
	return actual;
}






