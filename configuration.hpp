#pragma once
#include "run_time.hpp"
namespace configuration {
	extern auto& fault_reserved_stack_size = ::fault_reserved_stack_size;
	extern auto& default_fault_action = ::default_fault_action;
	extern auto& break_point_action = ::break_point_action;
	extern auto& enable_thread_naming = ::enable_thread_naming;

	extern size_t& max_running_tasks;
	extern size_t& max_planned_tasks;
}