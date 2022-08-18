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
		value = std::move((ValueItem&)set);
		set = ValueItem();
	}
	ValueMonitor(const ValueMonitor& set) {
		value = set;
	}

	ValueMonitor& operator=(ValueItem&& set) {
		ValueItem args{ value, set };
		value = std::move(set);
		set_notify->notify(args);
	}
	ValueMonitor& operator=(const ValueItem& set) {
		ValueItem args{ value, set };
		value = set;
		set_notify->notify(args);
	}

	ValueMonitor& operator=(ValueMonitor&& set) noexcept(false) {
		ValueItem args{ value, set };
		value = std::move(set);
		set_notify->notify(args);
	}
	ValueMonitor& operator=(const ValueMonitor& set) {
		ValueItem args{ value, set };
		value = set;
		set_notify->notify(args);
	}

	operator ValueItem&() {
		ValueItem args{ value };
		get_notify->notify(args);
		return value;
	}
	operator const ValueItem&() const {
		ValueItem args{ value };
		const_cast<typed_lgr<EventSystem>&>(get_notify)->notify(args);
		return value;
	}
};

class ValueChangeMonitor {
	ValueItem value;
public:
	typed_lgr<EventSystem> set_notify;

	ValueChangeMonitor(ValueItem&& set) noexcept {
		value = std::move(set);
	}
	ValueChangeMonitor(const ValueItem& set) {
		value = set;
	}
	ValueChangeMonitor(ValueChangeMonitor&& set) noexcept(false) {
		value = std::move((ValueItem&)set);
		set = ValueItem();
	}
	ValueChangeMonitor(const ValueChangeMonitor& set) {
		value = set;
	}

	ValueChangeMonitor& operator=(ValueItem&& set) {
		ValueItem args{ value, set };
		value = std::move(set);
		set_notify->notify(args);
	}
	ValueChangeMonitor& operator=(const ValueItem& set) {
		ValueItem args{value, set};
		value = set;
		set_notify->notify(args);
	}

	ValueChangeMonitor& operator=(ValueChangeMonitor&& set) noexcept(false) {
		ValueItem args{ value, set };
		value = std::move(set);
		set_notify->notify(args);
	}
	ValueChangeMonitor& operator=(const ValueChangeMonitor& set) {
		ValueItem args{ value, set };
		value = set;
		set_notify->notify(args);
	}

	operator ValueItem& () {
		return value;
	}
	operator const ValueItem& () const {
		return value;
	}
};