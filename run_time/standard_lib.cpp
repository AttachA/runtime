#include "standard_lib.hpp"
#include "library/console.hpp"
#include "AttachA_CXX.hpp"
#include <cmath>
#include <math.h>

ValueItem math_abs_impl(ValueItem& val) {
	if(val.meta.vtype == VType::async_res)
		getAsyncResult(val.val, val.meta);
	switch (val.meta.vtype) {
	case VType::i8:
		return ValueItem((void*)(std::abs(int8_t(val.val))), val.meta);
	case VType::i16:
		return ValueItem((void*)(std::abs(int16_t(val.val))), val.meta);
	case VType::i32:
		return ValueItem((void*)(std::abs(int32_t(val.val))), val.meta);
	case VType::i64:
		return ValueItem((void*)(std::abs(int64_t(val.val))), val.meta);
	case VType::flo: {
		union {
			float abs;
			void* res = nullptr;
		};
		abs = std::abs(*(float*)val.val);
		return ValueItem(res, val.meta);
		
	}
	case VType::doub: {
		union {
			double abs;
			void* res = nullptr;
		};
		abs = +(*(double*)val.val);
		return ValueItem(res, val.meta);
	}
	default:
		return val;
	}
}
ValueItem* math_abs(list_array<ValueItem>* args) {
	if (args) {
		if (args->size() == 1)
			return new ValueItem(math_abs_impl(args->operator[](0)));
		else {
			list_array<ValueItem>* res = new list_array<ValueItem>;
			res->reserve_push_back(args->size());
			for (auto& it : *args)
				res->push_back(math_abs_impl(it));
			return new ValueItem(res,ValueMeta(VType::uarr,false,true));
		}
	}
	return nullptr;
}
ValueItem* math_max(list_array<ValueItem>* args) {
	if (args) {
		if (args->size() == 1) {
			if (args->operator[](0).meta.vtype == VType::uarr)
				return math_max((list_array<ValueItem>*)args->operator[](0).val);
			else
				return new ValueItem(args->operator[](0));
		}
		else {
			bool ist_first = true;
			ValueItem* max = nullptr;
			for (auto& it : *args) {
				if (ist_first) {
					max = &it;
					ist_first = false;
					continue;
				}
				if (*max < it)
					max = &it;
			}
			if (max)
				return new ValueItem(*max);
		}
	}
	return nullptr;
}
ValueItem* math_min(list_array<ValueItem>* args) {
	if (args) {
		if (args->size() == 1) {
			if (args->operator[](0).meta.vtype == VType::uarr)
				return math_min((list_array<ValueItem>*)args->operator[](0).val);
			else
				return new ValueItem(args->operator[](0));
		}
		else {
			bool ist_first = true;
			ValueItem* min = nullptr;
			for (auto& it : *args) {
				if (ist_first) {
					min = &it;
					ist_first = false;
					continue;
				}
				if (*min > it)
					min = &it;
			}
			if (min)
				return new ValueItem(*min);
		}
	}
	return nullptr;
}

ValueItem* math_median(list_array<ValueItem>* args) {
	if (args) {
		if (args->size() == 1) {
			if (args->operator[](0).meta.vtype == VType::uarr)
				return math_median((list_array<ValueItem>*)args->operator[](0).val);
			else
				return new ValueItem(args->operator[](0));
		}
		else {
			args->sort();
			size_t pos = args->size() / 2;
			if(args->size() % 2)
				return new ValueItem(args->operator[](pos));
			else {
				return new ValueItem(
					(args->operator[](pos) + args->operator[](pos + 1))
						/ ValueItem((void*)2, ValueMeta(VType::i64, false, true))
				);
			}
		}
	}
	return nullptr;
}
ValueItem* math_range(list_array<ValueItem>* args) {
	if (args) {
		if (args->size() == 1) {
			if (args->operator[](0).meta.vtype == VType::uarr)
				return math_median((list_array<ValueItem>*)args->operator[](0).val);
			else 
				return new ValueItem(args->operator[](0));
		}
		else {
			bool ist_first = true;
			ValueItem* max = nullptr;
			ValueItem* min = nullptr;
			for (auto& it : *args) {
				if (ist_first) {
					max = &it;
					min = &it;
					ist_first = false;
					continue;
				}
				if (*max < it)
					max = &it;
				if (*min > it)
					min = &it;
			}
			if (max) 
				return new ValueItem(*max - *min);
		}
	}
	return nullptr;
}
ValueItem* math_mode(list_array<ValueItem>* args) {
	if (args) {
		if (args->size() == 1) {
			if (args->operator[](0).meta.vtype == VType::uarr)
				return math_mode((list_array<ValueItem>*)args->operator[](0).val);
			else
				return new ValueItem(args->operator[](0));
		}
		else {
			args->sort();
			size_t max_count = 0;
			ValueItem* max_it = nullptr;
			size_t cur_count = 0;
			ValueItem* cur_it = nullptr;
			for (auto& it : *args) {
				if (!cur_it) {
					cur_it = &it;
					++cur_count;
					continue;
				}
				if (*cur_it != it) {
					if (!max_it) {
						max_it = cur_it;
						max_count = cur_count;
					}
					else if(max_count < cur_count) {
						max_it = cur_it;
						max_count = cur_count;
					}
					cur_it = &it;
					cur_count = 0;
				}
				++cur_count;
			}
			if (!max_it)
				if (!cur_it)
					return nullptr;
				else
					return new ValueItem(*cur_it);
			else
				return new ValueItem(*max_it);
		}
	}
	return nullptr;
}

ValueItem math_round_impl(ValueItem& val) {
	if (val.meta.vtype == VType::async_res)
		getAsyncResult(val.val, val.meta);
	switch (val.meta.vtype) {
	case VType::flo: {
		union {
			float abs;
			void* res = nullptr;
		};
		abs = round((*(float*)val.val));
		return ValueItem(res, val.meta);
	}
	case VType::doub: {
		union {
			double abs;
			void* res = nullptr;
		};
		abs = round((*(double*)val.val));
		return ValueItem(res, val.meta);
	}
	default:
		return val;
	}
}
ValueItem* math_round(list_array<ValueItem>* args) {
	if (args) {
		if (args->size() == 1)
			return new ValueItem(math_round_impl(args->operator[](0)));
		else {
			list_array<ValueItem>* res = new list_array<ValueItem>;
			res->reserve_push_back(args->size());
			for (auto& it : *args)
				res->push_back(math_round_impl(it));

			return new ValueItem(res, ValueMeta(VType::uarr, false, true));
		}
	}
	return nullptr;
}
ValueItem math_floor_impl(ValueItem& val) {
	if (val.meta.vtype == VType::async_res)
		getAsyncResult(val.val, val.meta);
	switch (val.meta.vtype) {
	case VType::flo: {
		union {
			float abs;
			void* res = nullptr;
		};
		abs = floor((*(float*)val.val));
		return ValueItem(res, val.meta);
	}
	case VType::doub: {
		union {
			double abs;
			void* res = nullptr;
		};
		abs = floor((*(double*)val.val));
		return ValueItem(res, val.meta);
	}
	default:
		return val;
	}
}
ValueItem* math_floor(list_array<ValueItem>* args) {
	if (args) {
		if (args->size() == 1)
			return new ValueItem(math_floor_impl(args->operator[](0)));
		else {
			list_array<ValueItem>* res = new list_array<ValueItem>;
			res->reserve_push_back(args->size());
			for (auto& it : *args)
				res->push_back(math_floor_impl(it));

			return new ValueItem(res, ValueMeta(VType::uarr, false, true));
		}
	}
	return nullptr;
}
ValueItem math_ceil_impl(ValueItem& val) {
	if (val.meta.vtype == VType::async_res)
		getAsyncResult(val.val, val.meta);
	switch (val.meta.vtype) {
	case VType::flo: {
		union {
			float abs;
			void* res = nullptr;
		};
		abs = ceil((*(float*)val.val));
		return ValueItem(res, val.meta);
	}
	case VType::doub: {
		union {
			double abs;
			void* res = nullptr;
		};
		abs = ceil((*(double*)val.val));
		return ValueItem(res, val.meta);
	}
	default:
		return val;
	}
}
ValueItem* math_ceil(list_array<ValueItem>* args) {
	if (args) {
		if (args->size() == 1)
			return new ValueItem(math_ceil_impl(args->operator[](0)));
		else {
			list_array<ValueItem>* res = new list_array<ValueItem>;
			res->reserve_push_back(args->size());
			for (auto& it : *args)
				res->push_back(math_ceil_impl(it));

			return new ValueItem(res, ValueMeta(VType::uarr, false, true));
		}
	}
	return nullptr;
}
ValueItem math_fix_impl(ValueItem& val) {
	if (val.meta.vtype == VType::async_res)
		getAsyncResult(val.val, val.meta);
	switch (val.meta.vtype) {
	case VType::flo: {
		union {
			float abs;
			void* res = nullptr;
		};
		abs = trunc((*(float*)val.val));
		return ValueItem(res, val.meta);
	}
	case VType::doub: {
		union {
			double abs;
			void* res = nullptr;
		};
		abs = trunc((*(double*)val.val));
		return ValueItem(res, val.meta);
	}
	default:
		return val;
	}
}
ValueItem* math_fix(list_array<ValueItem>* args) {
	if (args) {
		if (args->size() == 1)
			return new ValueItem(math_fix_impl(args->operator[](0)));
		else {
			list_array<ValueItem>* res = new list_array<ValueItem>;
			res->reserve_push_back(args->size());
			for (auto& it : *args)
				res->push_back(math_fix_impl(it));

			return new ValueItem(res, ValueMeta(VType::uarr, false, true));
		}
	}
	return nullptr;
}



uint64_t math_factorial_impl__FAST_FACTOR(uint64_t m) {
	switch (m) {
	case 0:return 0;
	case 1:return 1;
	case 2:return 2;
	case 3:return 6;
	case 4:return 24;
	case 5:return 120;
	case 6:return 720;
	case 7:return 5040;
	case 8:return 40320;
	case 9:return 362880;
	case 10:return 3628800;
	default:
		return 0;
	}

}
uint64_t math_factorial_impl_FACTOR(uint64_t m) {
	if (m < 0)
		return -1;
	if (m < 10)
		return math_factorial_impl__FAST_FACTOR(m);
	uint64_t res = 39916800;
	for (uint64_t i = 12; i < m; i++)
		res *= i;
	res *= m;
	return res;
}
int64_t math_factorial_impl_FACTOR(int64_t m) {
	if (m < 0)
		return -1;
	if (m < 10)
		return math_factorial_impl__FAST_FACTOR((uint64_t)m);
	int64_t res = 39916800;
	for (int64_t i = 12; i < m; i++)
		res *= i;
	res *= m;
	return res;
}
ValueItem math_factorial_impl(ValueItem& val) {
	if (val.meta.vtype == VType::async_res)
		getAsyncResult(val.val, val.meta);
	switch (val.meta.vtype) {
	case VType::i8:
		return ValueItem((void*)math_factorial_impl_FACTOR((int64_t)(int8_t)val.val), val.meta);
	case VType::ui8:
		return ValueItem((void*)math_factorial_impl_FACTOR((uint64_t)(uint8_t)val.val), val.meta);
	case VType::i16:
		return ValueItem((void*)math_factorial_impl_FACTOR((int64_t)(int16_t)val.val), val.meta);
	case VType::ui16:
		return ValueItem((void*)math_factorial_impl_FACTOR((uint64_t)(uint16_t)val.val), val.meta);
	case VType::i32:
		return ValueItem((void*)math_factorial_impl_FACTOR((int64_t)(int32_t)val.val), val.meta);
	case VType::ui32:
		return ValueItem((void*)math_factorial_impl_FACTOR((uint64_t)(uint32_t)val.val), val.meta);
	case VType::i64:
		return ValueItem((void*)math_factorial_impl_FACTOR((int64_t)val.val), val.meta);
	case VType::ui64:
		return ValueItem((void*)math_factorial_impl_FACTOR((uint64_t)val.val), val.meta);
	case VType::undefined_ptr:
		return ValueItem((void*)math_factorial_impl_FACTOR((uint64_t)(size_t)val.val), val.meta);
	case VType::flo: {
		union {
			float x;
			void* res = nullptr;
		};
		x = (float)std::tgamma((double)(*(float*)val.val) + 1);
		return ValueItem(res, val.meta);
	}
	case VType::doub: {
		union {
			double x;
			void* res = nullptr;
		};
		x = std::tgamma((*(double*)val.val) + 1);
		return ValueItem(res, val.meta);
	}
	default:
		return val;
	}
}
ValueItem* math_factorial(list_array<ValueItem>* args) {
	if (args) {
		if (args->size() == 1)
			return new ValueItem(math_factorial_impl(args->operator[](0)));
		else {
			list_array<ValueItem>* res = new list_array<ValueItem>;
			res->reserve_push_back(args->size());
			for (auto& it : *args)
				res->push_back(math_factorial_impl(it));

			return new ValueItem(res, ValueMeta(VType::uarr, false, true));
		}
	}
	return nullptr;
}




template<double(*fn)(double)>
ValueItem math_thrigonomic_impl(ValueItem& val) {
	if (val.meta.vtype == VType::async_res)
		getAsyncResult(val.val, val.meta);
	switch (val.meta.vtype) {
	case VType::i8:
	case VType::ui8:
	case VType::i16:
	case VType::ui16:
	case VType::i32:
	case VType::ui32:
	case VType::i64:
	case VType::ui64:
		double t = ABI_IMPL::Vcast<double>(val.val, val.meta);
		ValueItem tmp((void*)&t, ValueMeta(VType::doub, false, true));
		return math_thrigonomic_impl<fn>(tmp);
	}
	switch (val.meta.vtype) {
	case VType::flo: {
		union {
			float x;
			void* res = nullptr;
		};
		x = fn((*(float*)val.val));
		return ValueItem(res, val.meta);
	}
	case VType::doub: {
		union {
			double x;
			void* res = nullptr;
		};
		x = fn((*(double*)val.val));
		return ValueItem(res, val.meta);
	}
	default:
		return val;
	}
}
template<double(*fn)(double)>
ValueItem* math_thrigonomic(list_array<ValueItem>* args) {
	if (args) {
		if (args->size() == 1) {
			if (args->operator[](0).meta.vtype == VType::uarr)
				return math_mode((list_array<ValueItem>*)args->operator[](0).val);
			else
				return new ValueItem(math_thrigonomic_impl<fn>(args->operator[](0)));
		}
		else {
			list_array<ValueItem>* res = new list_array<ValueItem>;
			res->reserve_push_back(args->size());
			for (auto& it : *args)
				res->push_back(math_thrigonomic_impl<fn>(it));

			return new ValueItem(res, ValueMeta(VType::uarr, false, true));
		}
	}
	return nullptr;
}

double csc(double d) {
	return 1 / sin(d);
}
double acsc(double d) {
	return asin(1/d);
}
double sec(double d) {
	return 1 / cos(d);
}
double asec(double d) {
	return acos(1 / d);
}
double cot(double d) {
	return 1 / tan(d);
}
double acot(double d) {
	return tan(1 / d);
}

double csch(double d) {
	return 1 / sinh(d);
}
double acsch(double d) {
	return asinh(1 / d);
}
double sech(double d) {
	return 1 / cosh(d);
}
double asech(double d) {
	return acosh(1 / d);
}
double coth(double d) {
	return 1 / tanh(d);
}
double acoth(double d) {
	return tanh(1 / d);
}


ValueItem math_pow_impl(ValueItem& val, ValueItem& m) {
	if (val.meta.vtype == VType::async_res)
		getAsyncResult(val.val, val.meta);
	double x = (double)m;

	double t = 0;
	switch (val.meta.vtype) {
	case VType::i8:
		t = pow((int8_t)val.val,x);
		break;
	case VType::ui8:
		t = pow((uint8_t)val.val, x);
		break;
	case VType::i16:
		t = pow((int16_t)val.val, x);
		break;
	case VType::ui16:
		t = pow((uint16_t)val.val, x);
		break;
	case VType::i32:
		t = pow((int32_t)val.val, x);
		break;
	case VType::ui32:
		t = pow((uint32_t)val.val, x);
		break;
	case VType::i64:
		t = pow((int64_t)val.val, x);
		break;
	case VType::ui64:
		t = pow((uint64_t)val.val, x);
		break;
	case VType::flo:
		t = pow(*(float*)&val.val, x);
		break;
	case VType::doub:
		t = pow(*(double*)&val.val, x);
		break;
	}

	switch (val.meta.vtype) {
	case VType::flo: {
		float f = t;
		return ValueItem((void*)&f, ValueMeta(VType::flo, false, true));
	}
	default:
		return ValueItem((void*)&t, ValueMeta(VType::doub, false, true));
	}
}
ValueItem* math_pow(list_array<ValueItem>* args) {
	if (args) {
		static ValueItem default_pow((void*)2, ValueMeta(VType::i32, false, true));
		if (args->size() == 1)
			return new ValueItem(math_pow_impl(args->operator[](0), default_pow));
		else if (args->size() == 2) {
			return new ValueItem(math_pow_impl(args->operator[](0), args->operator[](1)));
		} 
		else{
			list_array<ValueItem>* res = new list_array<ValueItem>;
			res->reserve_push_back(args->size());
			for (auto& it : *args)
				res->push_back(math_pow_impl(it, default_pow));

			return new ValueItem(res, ValueMeta(VType::uarr, false, true));
		}
	}
	return nullptr;
}


ValueItem math_frrt_impl(ValueItem& val) {
	if (val.meta.vtype == VType::async_res)
		getAsyncResult(val.val, val.meta);
	constexpr double x = 1. / 4;
	double t = 0;
	switch (val.meta.vtype) {
	case VType::i8:
		t = pow((int8_t)val.val, x);
		break;
	case VType::ui8:
		t = pow((uint8_t)val.val, x);
		break;
	case VType::i16:
		t = pow((int16_t)val.val, x);
		break;
	case VType::ui16:
		t = pow((uint16_t)val.val, x);
		break;
	case VType::i32:
		t = pow((int32_t)val.val, x);
		break;
	case VType::ui32:
		t = pow((uint32_t)val.val, x);
		break;
	case VType::i64:
		t = pow((int64_t)val.val, x);
		break;
	case VType::ui64:
		t = pow((uint64_t)val.val, x);
		break;
	case VType::flo:
		t = pow(*(float*)&val.val, x);
		break;
	case VType::doub:
		t = pow(*(double*)&val.val, x);
		break;
	}

	switch (val.meta.vtype) {
	case VType::flo: {
		float f = t;
		return ValueItem((void*)&f, ValueMeta(VType::flo, false, true));
	}
	default:
		return ValueItem((void*)&t, ValueMeta(VType::doub, false, true));
	}
}
ValueItem* math_frrt(list_array<ValueItem>* args) {
	if (args) {
		if (args->size() == 1)
			return new ValueItem(math_frrt_impl(args->operator[](0)));
		else {
			list_array<ValueItem>* res = new list_array<ValueItem>;
			res->reserve_push_back(args->size());
			for (auto& it : *args)
				res->push_back(math_frrt_impl(it));

			return new ValueItem(res, ValueMeta(VType::uarr, false, true));
		}
	}
	return nullptr;
}

extern "C" void initStandardFunctions() {
#pragma region Console
	FuncEnviropment::AddNative(console::printLine, "console printLine", false);
	FuncEnviropment::AddNative(console::print, "console print", false);
	{
		DynamicCall::FunctionTemplate templ;
		templ.is_variadic = true;
		templ.arguments.push_back(DynamicCall::FunctionTemplate::ValueT::getFromType<const char*>());
		FuncEnviropment::AddNative((DynamicCall::PROC)printf, templ, "console printf", false);
	}
	FuncEnviropment::AddNative(console::resetModifiers, "console resetModifiers", false);
	FuncEnviropment::AddNative(console::boldText, "console boldText", false);
	FuncEnviropment::AddNative(console::italicText, "console italicText", false);
	FuncEnviropment::AddNative(console::underlineText, "console underlineText", false);
	FuncEnviropment::AddNative(console::slowBlink, "console slowBlink", false);
	FuncEnviropment::AddNative(console::rapidBlink, "console rapidBlink", false);
	FuncEnviropment::AddNative(console::invertColors, "console invertColors", false);
	FuncEnviropment::AddNative(console::notBoldText, "console notBoldText", false);
	FuncEnviropment::AddNative(console::notUnderlinedText, "console notUnderlinedText", false);
	FuncEnviropment::AddNative(console::notBlinkText, "console notBlinkText", false);

	FuncEnviropment::AddNative(console::resetTextColor, "console resetTextColor", false);
	FuncEnviropment::AddNative(console::resetBgColor, "console resetBgColor", false);
	FuncEnviropment::AddNative(console::setTextColor, "console setTextColor", false);
	FuncEnviropment::AddNative(console::setBgColor, "console setBgColor", false);
	FuncEnviropment::AddNative(console::setPos, "console setPos", false);
	FuncEnviropment::AddNative(console::saveCurPos, "console saveCurPos", false);
	FuncEnviropment::AddNative(console::loadCurPos, "console loadCurPos", false);
	FuncEnviropment::AddNative(console::setLine, "console setLine", false);
	FuncEnviropment::AddNative(console::showCursor, "console showCursor", false);
	FuncEnviropment::AddNative(console::hideCursor, "console hideCursor", false);

	FuncEnviropment::AddNative(console::readWord, "console readWord", false);
	FuncEnviropment::AddNative(console::readLine, "console readLine", false);
	FuncEnviropment::AddNative(console::readInput, "console readInput", false);
	FuncEnviropment::AddNative(console::readValue, "console readValue", false);
#pragma endregion
#pragma region Math
	FuncEnviropment::AddNative(math_abs, "math abs", false);

	FuncEnviropment::AddNative(math_min, "math min", false);
	FuncEnviropment::AddNative(math_max, "math max", false);
	FuncEnviropment::AddNative(math_median, "math median", false);
	FuncEnviropment::AddNative(math_range, "math range", false);
	FuncEnviropment::AddNative(math_mode, "math mode", false);

	FuncEnviropment::AddNative(math_round, "math round", false);
	FuncEnviropment::AddNative(math_floor, "math floor", false);
	FuncEnviropment::AddNative(math_ceil, "math ceil", false);
	FuncEnviropment::AddNative(math_fix, "math fix", false);

	FuncEnviropment::AddNative(math_factorial, "math factorial", false);
	FuncEnviropment::AddNative(math_thrigonomic<sin>, "math sin", false);
	FuncEnviropment::AddNative(math_thrigonomic<asin>, "math asin", false);
	FuncEnviropment::AddNative(math_thrigonomic<cos>, "math cos", false);
	FuncEnviropment::AddNative(math_thrigonomic<acos>, "math acos", false);
	FuncEnviropment::AddNative(math_thrigonomic<tan>, "math tan", false);
	FuncEnviropment::AddNative(math_thrigonomic<atan>, "math atan", false);
	FuncEnviropment::AddNative(math_thrigonomic<csc>, "math csc", false);
	FuncEnviropment::AddNative(math_thrigonomic<acsc>, "math acsc", false);
	FuncEnviropment::AddNative(math_thrigonomic<sec>, "math sec", false);
	FuncEnviropment::AddNative(math_thrigonomic<asec>, "math asec", false);
	FuncEnviropment::AddNative(math_thrigonomic<cot>, "math cot", false);
	FuncEnviropment::AddNative(math_thrigonomic<acot>, "math acot", false);
	FuncEnviropment::AddNative(math_thrigonomic<sinh>, "math sinh", false);
	FuncEnviropment::AddNative(math_thrigonomic<asinh>, "math asinh", false);
	FuncEnviropment::AddNative(math_thrigonomic<cosh>, "math cosh", false);
	FuncEnviropment::AddNative(math_thrigonomic<acosh>, "math acosh", false);
	FuncEnviropment::AddNative(math_thrigonomic<tanh>, "math tanh", false);
	FuncEnviropment::AddNative(math_thrigonomic<atanh>, "math atanh", false);
	FuncEnviropment::AddNative(math_thrigonomic<csch>, "math csch", false);
	FuncEnviropment::AddNative(math_thrigonomic<acsch>, "math acsch", false);
	FuncEnviropment::AddNative(math_thrigonomic<sech>, "math sech", false);
	FuncEnviropment::AddNative(math_thrigonomic<asech>, "math asech", false);
	FuncEnviropment::AddNative(math_thrigonomic<coth>, "math coth", false);
	FuncEnviropment::AddNative(math_thrigonomic<acoth>, "math acoth", false);

	FuncEnviropment::AddNative(math_thrigonomic_impl<sqrt>, "math sqrt", false);
	FuncEnviropment::AddNative(math_thrigonomic_impl<cbrt>, "math cbrt", false);
	FuncEnviropment::AddNative(math_pow, "math pow", false);
	FuncEnviropment::AddNative(math_thrigonomic_impl<log>, "math log", false);
	FuncEnviropment::AddNative(math_thrigonomic_impl<log2>, "math log2", false);
	FuncEnviropment::AddNative(math_thrigonomic_impl<log10>, "math log10", false);




	
#pragma endregion
#pragma region File

#pragma endregion
}



ValueItem* cmath_frexp(list_array<ValueItem>* args) {
	double doub;
	if (args)
		doub = (double)args->operator[](0);
	else
		doub = 0;
	list_array<ValueItem>* rs = new list_array<ValueItem>(2);
	int res1;
	rs->operator[](0) = std::frexp(doub, &res1);
	rs->operator[](1) = res1;
	return new ValueItem(rs, ValueMeta(VType::uarr, false, true));
}
ValueItem* cmath_modf(list_array<ValueItem>* args) {
	double doub;
	if (args)
		doub = (double)args->operator[](0);
	else
		doub = 0;
	list_array<ValueItem>* rs = new list_array<ValueItem>(2);
	double res1;
	rs->operator[](0) = std::modf(doub, &res1);
	rs->operator[](1) = res1;
	return new ValueItem(rs, ValueMeta(VType::uarr, false, true));
}
ValueItem* cmath_remquo(list_array<ValueItem>* args) {
	double doub0;
	double doub1;
	if (args) {
		doub0 = (double)args->operator[](0);
		doub1 = (double)args->operator[](1);
	}
	else {
		doub0 = 0;
		doub1 = 0;
	}
	list_array<ValueItem>* rs = new list_array<ValueItem>(2);
	int res1;
	rs->operator[](0) = std::remquo(doub0, doub1, &res1);
	rs->operator[](1) = res1;
	return new ValueItem(rs, ValueMeta(VType::uarr, false, true));
}
ValueItem* cmath_nexttoward(list_array<ValueItem>* args) {
	float doub0;
	long double doub1;
	if (args) {
		doub0 = (double)args->operator[](0);
		doub1 = (double)args->operator[](1);
	}
	else {
		doub0 = 0;
		doub1 = 0;
	}
	list_array<ValueItem>* rs = new list_array<ValueItem>(2);
	rs->operator[](0) = std::nexttoward(doub0, doub1);
	return new ValueItem(rs, ValueMeta(VType::uarr, false, true));
}
extern "C" void initCMathLib() {
	FuncEnviropment::AddNative((double(*)(double))std::acos, "cmath acos", false);
	FuncEnviropment::AddNative((double(*)(double))std::acosh, "cmath acosh", false);
	FuncEnviropment::AddNative((double(*)(double))std::asin, "cmath asin", false);
	FuncEnviropment::AddNative((double(*)(double))std::asinh, "cmath asinh", false);
	FuncEnviropment::AddNative((double(*)(double))std::atan, "cmath atan", false);
	FuncEnviropment::AddNative((double(*)(double))std::atanh, "cmath atanh", false);
	FuncEnviropment::AddNative((double(*)(double, double))std::atan2, "cmath atan2", false);
	FuncEnviropment::AddNative((double(*)(double))std::cbrt, "cmath cbrt", false);
	FuncEnviropment::AddNative((double(*)(double))std::ceil, "cmath ceil", false);
	FuncEnviropment::AddNative((double(*)(double, double))std::copysign, "cmath copysign", false);
	FuncEnviropment::AddNative((double(*)(double))std::cos, "cmath cos", false);
	FuncEnviropment::AddNative((double(*)(double))std::cosh, "cmath cosh", false);
	FuncEnviropment::AddNative((double(*)(double))std::erf, "cmath erf", false);
	FuncEnviropment::AddNative((double(*)(double))std::erfc, "cmath erfc", false);
	FuncEnviropment::AddNative((double(*)(double))std::exp, "cmath exp", false);
	FuncEnviropment::AddNative((double(*)(double))std::exp2, "cmath exp2", false);
	FuncEnviropment::AddNative((double(*)(double))std::expm1, "cmath expm1", false);
	FuncEnviropment::AddNative((double(*)(double))std::fabs, "cmath fabs", false);
	FuncEnviropment::AddNative((double(*)(double, double))std::fdim, "cmath fdim", false);
	FuncEnviropment::AddNative((double(*)(double))std::floor, "cmath floor", false);
	FuncEnviropment::AddNative((double(*)(double, double, double))std::fma, "cmath fma", false);
	FuncEnviropment::AddNative((double(*)(double, double))std::fmax, "cmath fmax", false);
	FuncEnviropment::AddNative((double(*)(double, double))std::fmin, "cmath fmin", false);
	FuncEnviropment::AddNative((double(*)(double, double))std::fmod, "cmath fmod", false);
	FuncEnviropment::AddNative(cmath_frexp, "cmath frexp", false);
	FuncEnviropment::AddNative((double(*)(double, double))std::hypot, "cmath hypot", false);
	FuncEnviropment::AddNative((int(*)(double))std::ilogb, "cmath ilogb", false);
	FuncEnviropment::AddNative((double(*)(double, int))std::ldexp, "cmath ldexp", false);
	FuncEnviropment::AddNative((double(*)(double))std::lgamma, "cmath lgamma", false);
	FuncEnviropment::AddNative((long long(*)(double))std::llrint, "cmath llrint", false);
	FuncEnviropment::AddNative((long long(*)(double))std::llround, "cmath llround", false);
	FuncEnviropment::AddNative((double(*)(double))std::log, "cmath log", false);
	FuncEnviropment::AddNative((double(*)(double))std::log10, "cmath log10", false);
	FuncEnviropment::AddNative((double(*)(double))std::log1p, "cmath log1p", false);
	FuncEnviropment::AddNative((double(*)(double))std::log2, "cmath log2", false);
	FuncEnviropment::AddNative((double(*)(double))std::logb, "cmath logb", false);
	FuncEnviropment::AddNative((long(*)(double))std::lrint, "cmath lrint", false);
	FuncEnviropment::AddNative((long(*)(double))std::lround, "cmath lround", false);
	FuncEnviropment::AddNative(cmath_modf, "cmath modf", false);
	FuncEnviropment::AddNative((double(*)(double))std::nearbyint, "cmath nearbyint", false);
	FuncEnviropment::AddNative((double(*)(double, double))std::nextafter, "cmath nextafter", false);
	FuncEnviropment::AddNative(cmath_nexttoward, "cmath nexttoward", false);
	FuncEnviropment::AddNative((double(*)(double, double))std::pow, "cmath pow", false);
	FuncEnviropment::AddNative((double(*)(double, double))std::remainder, "cmath remainder", false);
	FuncEnviropment::AddNative(cmath_remquo, "math sqrt", false);
	FuncEnviropment::AddNative((double(*)(double))std::round, "cmath round", false);
	FuncEnviropment::AddNative((double(*)(double, long))std::scalbln, "cmath scalbln", false);
	FuncEnviropment::AddNative((double(*)(double, int))std::scalbn, "cmath scalbn", false);
	FuncEnviropment::AddNative((double(*)(double))std::sin, "cmath sin", false);
	FuncEnviropment::AddNative((double(*)(double))std::sinh, "cmath sinh", false);
	FuncEnviropment::AddNative((double(*)(double))std::sqrt, "cmath sqrt", false);
	FuncEnviropment::AddNative((double(*)(double))std::tan, "cmath tan", false);
	FuncEnviropment::AddNative((double(*)(double))std::tanh, "cmath tanh", false);
	FuncEnviropment::AddNative((double(*)(double))std::tgamma, "cmath tgamma", false);
	FuncEnviropment::AddNative((double(*)(double))std::trunc, "cmath trunc", false);
}

