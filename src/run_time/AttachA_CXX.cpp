// Copyright Danyil Melnytskyi 2023-Present
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <run_time/AttachA_CXX.hpp>

namespace art {
    namespace CXX {
        namespace _internal_ {
            await_lambda _await_lambda;
            async_lambda _async_lambda;
        }
    }
}