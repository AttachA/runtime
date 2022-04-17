// Copyright Danyil Melnytskyi 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once
#include "AttachA_CXX.hpp"
#include "tasks.hpp"

class ValueMonitor {
	ValueItem value;
public:
	typed_lgr<EventSystem> set_notify;
	typed_lgr<EventSystem> get_notify;

	ValueMonitor(ValueItem&& set) {
		value = std::move(set);
	}
	ValueMonitor(const ValueItem& set) {
		value = set;
	}
	ValueMonitor(ValueMonitor&& set) noexcept(false) {
		value = std::move(set);
		set = ValueItem();
	}
	ValueMonitor(const ValueMonitor& set) {
		value = set;
	}

	ValueMonitor& operator=(ValueItem&& set) {
		list_array<ValueItem> args{ value, set };
		value = std::move(set);
		set_notify->notify(&args);
	}
	ValueMonitor& operator=(const ValueItem& set) {
		list_array<ValueItem> args{ value, set };
		value = set;
		set_notify->notify(&args);
	}

	ValueMonitor& operator=(ValueMonitor&& set) noexcept(false) {
		list_array<ValueItem> args{ value, set };
		value = std::move(set);
		set_notify->notify(&args);
	}
	ValueMonitor& operator=(const ValueMonitor& set) {
		list_array<ValueItem> args{ value, set };
		value = set;
		set_notify->notify(&args);
	}

	operator ValueItem&() {
		list_array<AttachA::Value> tt{ AttachA::Value(new ValueItem()) };
		list_array<ValueItem> args{ value };
		get_notify->notify(&args);
		return value;
	}
	operator const ValueItem&() const {
		list_array<AttachA::Value> tt{ AttachA::Value(new ValueItem()) };
		list_array<ValueItem> args{ value };
		const_cast<typed_lgr<EventSystem>&>(get_notify)->notify(&args);
		return value;
	}
};

class ValueChangeEvent {
	ValueItem value;
public:
	typed_lgr<EventSystem> set_notify;

	ValueChangeEvent(ValueItem&& set) noexcept {
		value = std::move(set);
	}
	ValueChangeEvent(const ValueItem& set) {
		value = set;
	}
	ValueChangeEvent(ValueChangeEvent&& set) noexcept(false) {
		value = std::move(set);
		set = ValueItem();
	}
	ValueChangeEvent(const ValueChangeEvent& set) {
		value = set;
	}

	ValueChangeEvent& operator=(ValueItem&& set) {
		list_array<ValueItem> args{ value, set };
		value = std::move(set);
		set_notify->notify(&args);
	}
	ValueChangeEvent& operator=(const ValueItem& set) {
		list_array<ValueItem> args{ value, set };
		value = set;
		set_notify->notify(&args);
	}

	ValueChangeEvent& operator=(ValueChangeEvent&& set) noexcept(false) {
		list_array<ValueItem> args{ value, set };
		value = std::move(set);
		set_notify->notify(&args);
	}
	ValueChangeEvent& operator=(const ValueChangeEvent& set) {
		list_array<ValueItem> args{ value, set };
		value = set;
		set_notify->notify(&args);
	}

	operator ValueItem& () {
		return value;
	}
	operator const ValueItem& () const {
		return value;
	}
};