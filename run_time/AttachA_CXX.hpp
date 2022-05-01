// Copyright Danyil Melnytskyi 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once
#include "run_time_compiler.hpp"
namespace AttachA {
	template<class ...Types>
	ValueItem cxxCall(typed_lgr<FuncEnviropment> func, Types... types) {
		struct deleter {
			list_array<ValueItem>* temp;
			deleter(list_array<ValueItem>* tmp) {
				temp = temp;
			}
			~deleter() {
				delete temp;
			}
		} val(new list_array<ValueItem>{ ABI_IMPL::BVcast(types)... });

		ValueItem* res = func->syncWrapper(val.temp);
		ValueItem m(std::move(*res));
		delete res;
		return m;
	}
	inline ValueItem cxxCall(typed_lgr<FuncEnviropment> func) {
		ValueItem* res = func->syncWrapper(nullptr);
		ValueItem m(std::move(*res));
		delete res;
		return m;
	}
	template<class ...Types>
	ValueItem cxxCall(const std::string& fun_name, Types... types) {
		return cxxCall(FuncEnviropment::enviropment(fun_name), std::forward<Types>(types)...);
	}
	inline ValueItem cxxCall(const std::string& fun_name) {
		ValueItem* res = FuncEnviropment::enviropment(fun_name)->syncWrapper(nullptr);
		ValueItem m(std::move(*res));
		delete res;
		return m;
	}
}