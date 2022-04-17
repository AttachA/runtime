// Copyright Danyil Melnytskyi 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once
#include "run_time_compiler.hpp"
#include <cassert>
namespace AttachA {
	class Value {
		ValueItem fres;
	public:
		Value() { }
		Value(ValueItem* val) : fres(std::move(*val)) { delete val; }
		Value(const ValueItem& val) : fres(val) {}
		Value(ValueItem&& val) : fres(std::move(val)) {}
		Value(const Value& val) {
			fres = val.fres;
		}
		Value(Value&& val) noexcept {
			fres = std::move(val.fres);
		}
		Value& operator=(Value&& mov) noexcept {
			fres = std::move(mov.fres);
		}
		Value& operator=(const Value& copy) {
			fres = copy.fres;
		}

		Value& operator +=(const Value& op) {
			DynSum(&fres.val, (void**)&op.fres.val);
			return *this;
		}
		Value& operator -=(const Value& op) {
			DynMinus(&fres.val, (void**)&op.fres.val);
			return *this;
		}
		Value& operator *=(const Value& op) {
			DynMul(&fres.val, (void**)&op.fres.val);
			return *this;
		}
		Value& operator /=(const Value& op) {
			DynDiv(&fres.val, (void**)&op.fres.val);
			return *this;
		}
		Value& operator %=(const Value& op) {
			DynRest(&fres.val, (void**)&op.fres.val);
			return *this;
		}
		Value& operator ^=(const Value& op) {
			DynBitXor(&fres.val, (void**)&op.fres.val);
			return *this;
		}
		Value& operator &=(const Value& op) {
			DynBitAnd(&fres.val, (void**)&op.fres.val);
			return *this;
		}
		Value& operator |=(const Value& op) {
			DynBitOr(&fres.val, (void**)&op.fres.val);
			return *this;
		}
		Value& operator !() {
			DynBitNot(&fres.val);
			return *this;
		}

		Value operator +(const Value& op) const {
			return Value(*this) += op;
		}
		Value operator -(const Value& op) const  {
			return Value(*this) -= op;
		}
		Value operator *(const Value& op) const  {
			return Value(*this) *= op;
		}
		Value operator /(const Value& op) const  {
			return Value(*this) /= op;
		}
		Value operator ^(const Value& op) const  {
			return Value(*this) ^= op;
		}
		Value operator &(const Value& op) const  {
			return Value(*this) &= op;
		}
		Value operator |(const Value& op) const  {
			return Value(*this) |= op;
		}

		bool operator==(Value& op) {
			void* val1 = getValue(fres.val, fres.meta);
			void* val2 = getValue(op.fres.val, op.fres.meta);
			return compareValue(fres.meta.vtype, op.fres.meta.vtype, val1, val2).first;
		}
		bool operator!=(Value& op) {
			void* val1 = getValue(fres.val, fres.meta);
			void* val2 = getValue(op.fres.val, op.fres.meta);
			return !compareValue(fres.meta.vtype, op.fres.meta.vtype, val1, val2).first;
		}
		bool operator<(Value& op) {
			void* val1 = getValue(fres.val, fres.meta);
			void* val2 = getValue(op.fres.val, op.fres.meta);
			return compareValue(fres.meta.vtype, op.fres.meta.vtype, val1, val2).second;
		}
		bool operator>(Value& op) {
			void* val1 = getValue(fres.val, fres.meta);
			void* val2 = getValue(op.fres.val, op.fres.meta);
			return !compareValue(fres.meta.vtype, op.fres.meta.vtype, val1, val2).second;
		}
		bool operator<=(Value& op) {
			void* val1 = getValue(fres.val, fres.meta);
			void* val2 = getValue(op.fres.val, op.fres.meta);
			auto tmp = compareValue(fres.meta.vtype, op.fres.meta.vtype, val1, val2);
			return tmp.first || tmp.second;
		}
		bool operator>=(Value& op) {
			void* val1 = getValue(fres.val, fres.meta);
			void* val2 = getValue(op.fres.val, op.fres.meta);
			auto tmp = compareValue(fres.meta.vtype, op.fres.meta.vtype, val1, val2);
			return tmp.first || !tmp.second;
		}

		template<class T = list_array<Value>>
		T as() {
			if constexpr (std::is_same_v<T, int8_t>)
				return ABI_IMPL::Vcast<int8_t>(fres.val, fres.meta);
			else if constexpr (std::is_same_v<T, uint8_t>)
				return ABI_IMPL::Vcast<uint8_t>(fres.val, fres.meta);
			else if constexpr (std::is_same_v<T, int16_t>)
				return ABI_IMPL::Vcast<int16_t>(fres.val, fres.meta);
			else if constexpr (std::is_same_v<T, uint16_t>)
				return ABI_IMPL::Vcast<uint16_t>(fres.val, fres.meta);
			else if constexpr (std::is_same_v<T, int32_t>)
				return ABI_IMPL::Vcast<int32_t>(fres.val, fres.meta);
			else if constexpr (std::is_same_v<T, uint32_t>)
				return ABI_IMPL::Vcast<uint32_t>(fres.val, fres.meta);
			else if constexpr (std::is_same_v<T, int64_t>)
				return ABI_IMPL::Vcast<int64_t>(fres.val, fres.meta);
			else if constexpr (std::is_same_v<T, uint64_t>)
				return ABI_IMPL::Vcast<uint64_t>(fres.val, fres.meta);
			else if constexpr (std::is_same_v<T, float>)
				return ABI_IMPL::Vcast<float>(fres.val, fres.meta);
			else if constexpr (std::is_same_v<T, double>)
				return ABI_IMPL::Vcast<double>(fres.val, fres.meta);
			else if constexpr (std::is_same_v<T, std::string>)
				return ABI_IMPL::Scast(fres.val, fres.meta);
			else if constexpr (std::is_same_v<T, list_array<Value>>) {
				if (fres.meta.vtype == VType::uarr)
					return ((list_array<ValueItem>*)fres.val)->convert<Value>([](const ValueItem& arIt) { return arIt; });
				else
					return { fres };
			}
			else if constexpr (std::is_same_v<T, ValueItem>)
				return fres;
			else if constexpr (std::is_same_v<T, ValueItem*>)
				return new ValueItem(fres);
			else {
				static_assert(
					!(
						std::is_arithmetic_v<T> ||
						std::is_same_v<T, std::string> ||
						std::is_same_v<T, list_array<Value>> ||
						std::is_same_v<T, ValueItem>
					),
					"Invalid type for convert"
				);
				throw CompileTimeException("Invalid compiler, use correct compiler for compile AttachA, //ignored static_assert//");
			}
		}
		template<class T>
		list_array<T> asSarr() {
			if (reinterpret_cast<ValueMeta&>(fres.meta).vtype == VType::uarr)
				return ((list_array<ValueItem>*)fres.val)->convert<T>([](ValueItem& arIt) { return Value(arIt).as<T>(); });
			else
				return { as<T>() };
		}
	};


	template<class T = int8_t>
	static ValueItem convValue(const T& val) {
		if constexpr (std::is_same_v<T, int8_t>)
			return ValueItem((void*)val, ValueMeta(VType::i8, false, true));
		else if constexpr (std::is_same_v<T, uint8_t>)
			return ValueItem((void*)val, ValueMeta(VType::ui8, false, true));
		else if constexpr (std::is_same_v<T, int16_t>)
			return ValueItem((void*)val, ValueMeta(VType::i16, false, true));
		else if constexpr (std::is_same_v<T, uint16_t>)
			return ValueItem((void*)val, ValueMeta(VType::ui16, false, true));
		else if constexpr (std::is_same_v<T, int32_t>)
			return ValueItem((void*)val, ValueMeta(VType::i32, false, true));
		else if constexpr (std::is_same_v<T, int32_t>)
			return ValueItem((void*)val, ValueMeta(VType::ui32, false, true));
		else if constexpr (std::is_same_v<T, int64_t>)
			return ValueItem((void*)val, ValueMeta(VType::i64, false, true));
		else if constexpr (std::is_same_v<T, uint64_t>)
			return ValueItem((void*)val, ValueMeta(VType::ui64, false, true));
		else if constexpr (std::is_same_v<T, float>)
			return ValueItem(*(void**)&val, ValueMeta(VType::flo, false, true));
		else if constexpr (std::is_same_v<T, double>)
			return ValueItem(*(void**)&val, ValueMeta(VType::doub, false, true));
		else if constexpr (std::is_same_v<T, std::string> || std::is_same_v<T, const char*>)
			return ValueItem(new std::string(val), ValueMeta(VType::string, false, true));
		else if constexpr (std::is_same_v<T, list_array<Value>&>)
			return ValueItem(new list_array<ValueItem>(val.convert<ValueItem>([](Value& val) { return val.as<ValueItem>(); })), ValueMeta(VType::uarr, false, true));
		else {
			static_assert(
				!(
					std::is_arithmetic_v<T> ||
					std::is_same_v<T, std::string> ||
					std::is_same_v<T, list_array<Value>> ||
					std::is_same_v<T, ValueItem>
					),
				"Invalid type for convert"
			);
			throw CompileTimeException("Invalid compiler, use correct compiler for compile AttachA, //ignored static_assert//");
		}
	}



	template<class ...Types>
	Value cxxCall(typed_lgr<FuncEnviropment> func, Types... types) {
		struct deleter {
			list_array<ValueItem>* temp;
			deleter(list_array<ValueItem>* tmp) {
				temp = temp;
			}
			~deleter() {
				delete temp;
			}
		} val(new list_array<ValueItem>{ convValue(types)... });
		return func->syncWrapper(val.temp);
	}
	inline Value cxxCall(typed_lgr<FuncEnviropment> func) {
		return func->syncWrapper(nullptr);
	}
	template<class ...Types>
	Value cxxCall(const std::string& fun_name, Types... types) {
		return cxxCall(FuncEnviropment::enviropment(fun_name), std::forward<Types>(types)...);
	}
	inline Value cxxCall(const std::string& fun_name) {
		return FuncEnviropment::enviropment(fun_name)->syncWrapper(nullptr);
	}
}