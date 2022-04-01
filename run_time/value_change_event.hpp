// Copyright Danyil Melnytskyi 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once
#include "AttachA_CXX.hpp"

template<class T = int>
class ValueChangeEvent {
	T value;
public:
	size_t seter_check = 0;
	size_t geter_check = 0;
	typed_lgr<FuncEnviropment> set_notify;
	typed_lgr<FuncEnviropment> get_notify;

	ValueChangeEvent(T&& set) {
		value = std::move(set);
	}
	ValueChangeEvent(const T& set) {
		value = set;
	}

	ValueChangeEvent& operator=(T&& set) {
		value = std::move(set);
		seter_check++;
		set_notify.notify_all();
	}
	ValueChangeEvent& operator=(const T& set) {
		value = set;
		seter_check++;
		set_notify.notify_all();
	}

	operator T& () {
		list_array<AttachA::Value> tt{ AttachA::Value(new ValueItem()) };
		AttachA::convValue(tt);
		AttachA::cxxCall(set_notify, std::string("ssss"));
		geter_check++;
		get_notify.notify_all();
		return value;
	}
};