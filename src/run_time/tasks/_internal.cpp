// Copyright Danyil Melnytskyi 2022-Present
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <run_time/tasks/_internal.hpp>

namespace art {
    thread_local executors_local loc;
    executor_global glob;

    bool can_be_scheduled_task_to_hot() {
        if (Task::max_running_tasks)
            if (Task::max_running_tasks <= (glob.tasks_in_swap + glob.tasks.size()))
                return false;
        return true;
    }
}
