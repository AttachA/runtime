#pragma once
#include "run_time_compiler.hpp"
#include <cassert>
namespace AttachA {
	class Value {
		ValueItem fres;
		int8_t asI8() {
			return ABI_IMPL::Vcast<int8_t>(fres.val, fres.meta);
		}
		uint8_t asUI8() {
			return ABI_IMPL::Vcast<uint8_t>(fres.val, fres.meta);
		}
		int16_t asI16() {
			return ABI_IMPL::Vcast<int16_t>(fres.val,fres.meta);
		}
		uint16_t asUI16() {
			return ABI_IMPL::Vcast<uint16_t>(fres.val,fres.meta);
		}
		int32_t asI32() {
			return ABI_IMPL::Vcast<int32_t>(fres.val,fres.meta);
		}
		uint32_t asUI32() {
			return ABI_IMPL::Vcast<uint32_t>(fres.val,fres.meta);
		}
		int64_t asI64() {
			return ABI_IMPL::Vcast<int64_t>(fres.val,fres.meta);
		}
		uint64_t asUI64() {
			return ABI_IMPL::Vcast<uint64_t>(fres.val,fres.meta);
		}
		std::string asStr() {
			return ABI_IMPL::Scast(fres.val,fres.meta);
		}
		list_array<Value> asUarr() {
			if (fres.meta.vtype == VType::uarr)
				return ((list_array<ValueItem>*)fres.val)->convert<Value>([](ValueItem& arIt) { return ValueItem(arIt); });
			else
				return { ValueItem(fres) };
		}
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
		

		template<class T = int8_t>
		T as() {
			if constexpr (std::is_same_v<T, int8_t>)
				return asI8();
			else if constexpr (std::is_same_v<T, uint8_t>)
				return asUI8();
			else if constexpr (std::is_same_v<T, int16_t>)
				return asI16();
			else if constexpr (std::is_same_v<T, uint16_t>)
				return asUI16();
			else if constexpr (std::is_same_v<T, int32_t>)
				return asI32();
			else if constexpr (std::is_same_v<T, int32_t>)
				return asUI32();
			else if constexpr (std::is_same_v<T, int64_t>)
				return asI64();
			else if constexpr (std::is_same_v<T, int64_t>)
				return asUI64();
			else if constexpr (std::is_same_v<T, std::string>)
				return asStr();
			else if constexpr (std::is_same_v<T, list_array<Value>>)
				return asUarr();
			else if constexpr (std::is_same_v<T, ValueItem>)
				return fres;
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
	static ValueItem convValue(T& val) {
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
		else if constexpr (std::is_same_v<T, int64_t>)
			return ValueItem((void*)val, ValueMeta(VType::ui64, false, true));
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
			~deleter() {
				delete temp;
			}
		} val;
		val.temp = new list_array<ValueItem>{ convValue(types)... };
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