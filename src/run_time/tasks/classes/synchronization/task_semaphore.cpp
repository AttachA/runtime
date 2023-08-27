#include <run_time/tasks.hpp>
#include <run_time/tasks/_internal.hpp>
namespace art{
    void TaskSemaphore::setMaxThreshold(size_t val) {
		art::lock_guard guard(no_race);
		release_all();
		max_threshold = val;
		allow_threshold = max_threshold;
	}
	void TaskSemaphore::lock() {
		loc.curr_task->awaked = false;
		loc.curr_task->time_end_flag = false;
	re_try:
		no_race.lock();
		if (!allow_threshold) {
			if (loc.is_task_thread) {
				art::lock_guard guard(glob.task_thread_safety);
				resume_task.emplace_back(loc.curr_task, loc.curr_task->awake_check);
				no_race.unlock();
				swapCtxRelock(glob.task_thread_safety);
			}
			else {
				art::mutex mtx;
				art::unique_lock guard(mtx);
				no_race.unlock();
				native_notify.wait(guard);
			}
			goto re_try;
		}
		else
			--allow_threshold;
		no_race.unlock();
		return;
	}
	bool TaskSemaphore::try_lock() {
		if (!no_race.try_lock())
			return false;
		if (!allow_threshold) {
			no_race.unlock();
			return false;
		}
		else
			--allow_threshold;
		no_race.unlock();
		return true;
	}

	bool TaskSemaphore::try_lock_for(size_t milliseconds) {
		return try_lock_until(std::chrono::high_resolution_clock::now() + std::chrono::milliseconds(milliseconds));
	}
	bool TaskSemaphore::try_lock_until(std::chrono::high_resolution_clock::time_point time_point) {
	re_try:
		if (!no_race.try_lock_until(time_point))
			return false;
		if (!allow_threshold) {
			if (loc.is_task_thread) {
				art::lock_guard guard(glob.task_thread_safety);
				makeTimeWait(time_point);
				resume_task.emplace_back(loc.curr_task, loc.curr_task->awake_check);
				no_race.unlock();
				swapCtxRelock(glob.task_thread_safety);
				if (!loc.curr_task->awaked)
					return false;
			}
			else {
				no_race.unlock();
				art::mutex mtx;
				art::unique_lock guard(mtx);
				if (native_notify.wait_until(guard, time_point) == art::cv_status::timeout)
					return false;
			}
			goto re_try;
		}
		if (allow_threshold)
			--allow_threshold;
		no_race.unlock();
		return true;

	}
	void TaskSemaphore::release() {
		art::lock_guard lg0(no_race);
		if (allow_threshold == max_threshold)
			return;
		allow_threshold++;
		native_notify.notify_one();
		while (resume_task.size()) {
			auto& it = resume_task.front();
			art::lock_guard lg2(it.task->no_race);
			if (!it.task->time_end_flag) {
				if(it.task->awake_check != it.awake_check)
					continue;
				it.task->awaked = true;
				auto task = resume_task.front().task;
				resume_task.pop_front();
				transfer_task(task);
				return;
			}
			else
				resume_task.pop_front();
		}
	}
	void TaskSemaphore::release_all() {
		art::lock_guard lg0(no_race);
		if (allow_threshold == max_threshold)
			return;
		art::lock_guard lg1(glob.task_thread_safety);
		allow_threshold = max_threshold;
		native_notify.notify_all();
		while (resume_task.size()) {
			auto& it = resume_task.back();
			art::lock_guard lg2(it.task->no_race);
			if (!it.task->time_end_flag) {
				if (it.task->awake_check != it.awake_check)
					continue;
				it.task->awaked = true;
				auto task = resume_task.front().task;
				resume_task.pop_front();
				transfer_task(task);
			}
			else
				resume_task.pop_front();
		}
	}
	bool TaskSemaphore::is_locked() {
		if (try_lock()) {
			release();
			return true;
		}
		return false;
	}
}