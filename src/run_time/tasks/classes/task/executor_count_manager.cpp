// Copyright Danyil Melnytskyi 2022-Present
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <configuration/tasks.hpp>
#include <run_time/tasks.hpp>
#include <run_time/tasks/_internal.hpp>
#include <run_time/tasks/util/hill_climbing.hpp>
#include <run_time/tasks/util/interrupt.hpp>
#include <util/platform.hpp>

namespace art {
    std::pair<uint64_t, int64_t> _become_executor_count_manager_(
        art::unique_lock<art::recursive_mutex>& lock,
        std::chrono::high_resolution_clock::time_point& last_time,
        std::list<uint32_t>& workers_completions,
        art::util::hill_climb& hill_climber) {
        std::chrono::high_resolution_clock::duration elapsed = std::chrono::high_resolution_clock::now() - last_time;
        uint32_t executor_count = workers_completions.size();
        if (executor_count == 0)
            return {100, 1};
        uint64_t sleep_count_avg = 0;
        uint64_t recommended_workers_avg = 0;
        for (auto& completions : workers_completions) {
            auto [recommended_workers_, sleep_count_] = hill_climber.climb(executor_count, std::chrono::duration<double>(elapsed).count(), completions, 1, 10);
            completions = 0;
            sleep_count_avg += sleep_count_;
            recommended_workers_avg += recommended_workers_;
        }
        sleep_count_avg /= executor_count;
        recommended_workers_avg /= executor_count;
        return {sleep_count_avg, int64_t(recommended_workers_avg) - executor_count};
    }

    void Task::become_executor_count_manager(bool leave_after_finish) {
        art::unique_lock lock(glob.task_thread_safety);
        if (glob.executor_manager_in_work)
            return;
        glob.executor_manager_in_work = true;
        std::chrono::high_resolution_clock::time_point last_time = std::chrono::high_resolution_clock::now();
        while (true) {
            uint64_t all_sleep_count_avg;
            auto [sleep_count_avg, workers_diff] = _become_executor_count_manager_(lock, last_time, glob.workers_completions, glob.executor_manager_hill_climb);
            all_sleep_count_avg = sleep_count_avg;
            if (workers_diff == 0)
                ;
            else if (workers_diff > 0) {
                for (uint32_t i = 0; i < workers_diff; i++)
                    art::thread(taskExecutor, false).detach();
            } else {
                art::typed_lgr<Task> task(new Task(nullptr, nullptr));
                for (uint32_t i = 0; i < workers_diff; i++)
                    glob.tasks.push(task);
            }
            lock.unlock();
            {
                art::lock_guard binds(glob.binded_workers_safety);
                for (auto& contexts : glob.binded_workers) {
                    if (contexts.second.fixed_size)
                        continue;
                    art::unique_lock lock(contexts.second.no_race);
                    auto [sleep_count_avg, workers_diff] = _become_executor_count_manager_(lock, last_time, contexts.second.completions, contexts.second.executor_manager_hill_climb);
                    all_sleep_count_avg += sleep_count_avg;
                    if (workers_diff == 0)
                        ;
                    else if (workers_diff > 0) {
                        for (uint32_t i = 0; i < workers_diff; i++)
                            art::thread(bindedTaskExecutor, contexts.first).detach();
                    } else {
                        art::typed_lgr<Task> task(new Task(nullptr, nullptr));
                        task->bind_to_worker_id = contexts.first;
                        for (uint32_t i = 0; i < workers_diff; i++)
                            contexts.second.tasks.push_back(task);
                    }
                }
                all_sleep_count_avg /= glob.binded_workers.size() + 1;
            }


            last_time = std::chrono::high_resolution_clock::now();
            art::this_thread::sleep_for(std::chrono::milliseconds(all_sleep_count_avg));
            lock.lock();

            if (leave_after_finish)
                if (!(!glob.tasks.empty() || !glob.cold_tasks.empty() || !glob.timed_tasks.empty() || glob.in_exec || glob.tasks_in_swap || glob.in_run_tasks)) {
                    art::lock_guard lock(glob.binded_workers_safety);
                    bool binded_tasks_empty = true;
                    for (auto& contexts : glob.binded_workers)
                        if (contexts.second.tasks.size())
                            binded_tasks_empty = false;
                    if (binded_tasks_empty)
                        break;
                }
        }
    }

    void Task::start_executor_count_manager() {
        art::lock_guard lock(glob.task_thread_safety);
        if (glob.executor_manager_in_work)
            return;
        art::thread(&become_executor_count_manager, false).detach();
    }

    void Task::start_interrupt_handler() {
#if _configuration_tasks_enable_preemptive_scheduler_preview && PLATFORM_WINDOWS
        interrupt::init_interrupt_handler();
#endif
    }
}
