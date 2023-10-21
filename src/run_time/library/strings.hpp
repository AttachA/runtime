// Copyright Danyil Melnytskyi 2023-Present
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef SRC_RUN_TIME_LIBRARY_STRINGS
#define SRC_RUN_TIME_LIBRARY_STRINGS
#include <cstdint>
#include <util/ustring.hpp>

namespace art {
    struct ValueItem;

    namespace strings {
        art::ustring _format(ValueItem* args, uint32_t len);
        ValueItem* format(ValueItem* args, uint32_t len);
        ValueItem* register_format_operator(ValueItem* args, uint32_t len);
    }
}
#endif /* SRC_RUN_TIME_LIBRARY_STRINGS */
