// Copyright Danyil Melnytskyi 2022-2023
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once
#include <run_time/attacha_abi_structs.hpp>
namespace art {
	namespace exceptions{
		ValueItem* get_current_exception_name(ValueItem*, uint32_t);
		ValueItem* get_current_exception_description(ValueItem*, uint32_t);
		ValueItem* get_current_exception_full_description(ValueItem*, uint32_t);
		ValueItem* has_current_exception_inner_exception(ValueItem*, uint32_t);
		ValueItem* unpack_current_exception(ValueItem*, uint32_t);
		ValueItem* current_exception_catched(ValueItem*, uint32_t);
		ValueItem* in_exception(ValueItem*, uint32_t);
	}
}