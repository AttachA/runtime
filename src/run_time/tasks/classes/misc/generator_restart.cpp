// Copyright Danyil Melnytskyi 2022-Present
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <run_time/tasks.hpp>
#include <run_time/tasks/_internal.hpp>

namespace art {
    void forceCancelCancellation(GeneratorRestart& restart) {
        restart.in_landing = true;
    }

    GeneratorRestart::GeneratorRestart()
        : AttachARuntimeException("This generator received restart command") {}

    GeneratorRestart::~GeneratorRestart() noexcept(false) {
        if (!in_landing) {
            abort();
        }
    }

    bool GeneratorRestart::_in_landing() {
        return in_landing;
    }
}
