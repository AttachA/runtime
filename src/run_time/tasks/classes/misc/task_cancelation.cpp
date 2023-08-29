// Copyright Danyil Melnytskyi 2022-Present
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <run_time/tasks.hpp>
#include <run_time/tasks/_internal.hpp>

namespace art {
    TaskCancellation::TaskCancellation()
        : AttachARuntimeException("This task received cancellation token") {}

    TaskCancellation::~TaskCancellation() {
        if (!in_landing) {
            abort();
        }
    }

    bool TaskCancellation::_in_landing() {
        return in_landing;
    }

    void forceCancelCancellation(TaskCancellation& cancel_token) {
        cancel_token.in_landing = true;
    }

    void checkCancellation() {
        if (loc.curr_task->make_cancel)
            throw TaskCancellation();
        if (loc.curr_task->timeout != std::chrono::high_resolution_clock::time_point::min())
            if (loc.curr_task->timeout <= std::chrono::high_resolution_clock::now())
                throw TaskCancellation();
    }
}
