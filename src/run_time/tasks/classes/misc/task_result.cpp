// Copyright Danyil Melnytskyi 2022-Present
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <run_time/tasks.hpp>
#include <run_time/tasks/_internal.hpp>


namespace art{
    TaskResult::~TaskResult() {
		if (context){
			delete context;
			context = nullptr;
		}
	}

	ValueItem* TaskResult::getResult(size_t res_num, art::unique_lock<MutexUnify>& l) {
		if (results.size() >= res_num) {
			while (results.size() >= res_num && !end_of_life)
				result_notify.wait(l);
			if (results.size() >= res_num)
				return new ValueItem();
		}
		return new ValueItem(results[res_num]);
	}

	void TaskResult::awaitEnd(art::unique_lock<MutexUnify>& l) {
		while (!end_of_life)
			result_notify.wait(l);
	}

	void TaskResult::yieldResult(ValueItem* res, art::unique_lock<MutexUnify>& l, bool release) {
		if (res) {
			results.push_back(std::move(*res));
			if (release)
				delete res;
		}
		else
			results.push_back(ValueItem());
		result_notify.notify_all();
	}

	void TaskResult::yieldResult(ValueItem&& res, art::unique_lock<MutexUnify>& l) {
		results.push_back(std::move(res));
		result_notify.notify_all();
	}
    
	void TaskResult::finalResult(ValueItem* res, art::unique_lock<MutexUnify>& l) {
		if (res) {
			results.push_back(std::move(*res));
			delete res;
		}
		else
			results.push_back(ValueItem());
		end_of_life = true;
		result_notify.notify_all();
	}

	void TaskResult::finalResult(ValueItem&& res, art::unique_lock<MutexUnify>& l) {
		results.push_back(std::move(res));
		end_of_life = true;
		result_notify.notify_all();
	}

	TaskResult::TaskResult() {}

	TaskResult::TaskResult(TaskResult&& move) noexcept {
		results = std::move(move.results);
		end_of_life = move.end_of_life;
		move.end_of_life = true;
	}
}
