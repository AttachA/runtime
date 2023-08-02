// Copyright Danyil Melnytskyi 2022-2023
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include "exceptions.hpp"
#include "../asm/exception.hpp"
namespace art{
    namespace exceptions{
        ValueItem* get_current_exception_name(ValueItem*, uint32_t){
            return exception::get_current_exception_name();
        }
        ValueItem* get_current_exception_description(ValueItem*, uint32_t){
            return exception::get_current_exception_description();
        }
        ValueItem* get_current_exception_full_description(ValueItem*, uint32_t){
            return exception::get_current_exception_full_description();
        }
        ValueItem* has_current_exception_inner_exception(ValueItem*, uint32_t){
            return exception::has_current_exception_inner_exception();
        }
        ValueItem* unpack_current_exception(ValueItem*, uint32_t){
            exception::unpack_current_exception();
            return nullptr;
        }
        ValueItem* current_exception_catched(ValueItem*, uint32_t){
            exception::current_exception_catched();
            return nullptr;
        }
        ValueItem* in_exception(ValueItem*, uint32_t){
            return new ValueItem(exception::has_exception());
        }
    }
}