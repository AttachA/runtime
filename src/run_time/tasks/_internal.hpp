// Copyright Danyil Melnytskyi 2022-Present
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef SRC_RUN_TIME_TASKS_INTERNAL
#define SRC_RUN_TIME_TASKS_INTERNAL


#include <exception>
#include <queue>
#include <boost/context/continuation.hpp>
#include <configuration/tasks.hpp>
#include <run_time/tasks.hpp>
#include <run_time/tasks/util/hill_climbing.hpp>

namespace art{
    struct task_context{
		boost::context::continuation context;
		enum class priority {
			background,		//| max 15ms	| basic 5ms
			low,			//| max 30ms	| basic 15ms
			lower,			//| max 50ms	| basic 25ms
			normal,			//| max 80ms	| basic 40ms
			higher,			//| max 100ms	| basic 60ms
			high,			//| max 200ms	| basic 100ms
			semi_realtime,	//| no limit	| no limit
		} _priority;
		
		inline static constexpr std::chrono::nanoseconds priority_quantum_basic[] = {
			std::chrono::nanoseconds(configuration::tasks::scheduler::background_basic_quantum_ns),
			std::chrono::nanoseconds(configuration::tasks::scheduler::low_basic_quantum_ns),
			std::chrono::nanoseconds(configuration::tasks::scheduler::lower_basic_quantum_ns),
			std::chrono::nanoseconds(configuration::tasks::scheduler::normal_basic_quantum_ns),
			std::chrono::nanoseconds(configuration::tasks::scheduler::higher_basic_quantum_ns),
			std::chrono::nanoseconds(configuration::tasks::scheduler::high_basic_quantum_ns),
			std::chrono::nanoseconds::min()
		};
		inline static constexpr std::chrono::nanoseconds priority_quantum_max[] = {
			std::chrono::nanoseconds(configuration::tasks::scheduler::background_max_quantum_ns),
			std::chrono::nanoseconds(configuration::tasks::scheduler::low_max_quantum_ns),
			std::chrono::nanoseconds(configuration::tasks::scheduler::lower_max_quantum_ns),
			std::chrono::nanoseconds(configuration::tasks::scheduler::normal_max_quantum_ns),
			std::chrono::nanoseconds(configuration::tasks::scheduler::higher_max_quantum_ns),
			std::chrono::nanoseconds(configuration::tasks::scheduler::high_max_quantum_ns),
			std::chrono::nanoseconds::min()
		};
		//per task has n quantum(ms) to execute depends on priority
		//if task spend it all it will be suspended
		//if task not spend it all, unused quantum will be added to next task quantum(ms limited by priority)
		//after resume if quantum is not more basic quantum, limit will be set to basic quantum
		//semi_realtime tasks has no limits


		std::chrono::nanoseconds current_available_quantum;
		
		

		
		//std::chrono::nanoseconds::min(); means no limit
		//std::chrono::nanoseconds(0); means no quantum, task last time spend more quantum than it has
		std::chrono::nanoseconds next_quantum(){
			if(_priority == priority::semi_realtime)
				return std::chrono::nanoseconds::min();
			

			current_available_quantum += priority_quantum_basic[(size_t)_priority];
			if(current_available_quantum > priority_quantum_max[(size_t)_priority])
				current_available_quantum = priority_quantum_max[(size_t)_priority];
			return current_available_quantum > std::chrono::nanoseconds(0) ? current_available_quantum : std::chrono::nanoseconds(0);
		}
		std::chrono::nanoseconds peek_quantum() {
			if (_priority == priority::semi_realtime)
				return std::chrono::nanoseconds::min();
			auto current_available_quantum = this->current_available_quantum + priority_quantum_basic[(size_t)_priority];
			if (current_available_quantum > priority_quantum_max[(size_t)_priority])
				current_available_quantum = priority_quantum_max[(size_t)_priority];
			return current_available_quantum > std::chrono::nanoseconds(0) ? current_available_quantum : std::chrono::nanoseconds(0);
		}
		void task_switch(std::chrono::nanoseconds elapsed){
			if(_priority == priority::semi_realtime)
				return;
			current_available_quantum -= elapsed;
		}
		void init_quantum(){
			current_available_quantum = priority_quantum_basic[(size_t)_priority];
		}
	};

    struct executors_local {
        art::shared_ptr<Generator> on_load_generator_ref = nullptr;

        boost::context::continuation* stack_current_context = nullptr;
        struct task_context* current_context = nullptr;
        std::exception_ptr ex_ptr;
        std::queue<art::shared_ptr<Task>> interrupted_tasks;
        art::shared_ptr<Task> curr_task = nullptr;
        bool is_task_thread = false;
        bool context_in_swap = false;

        bool in_exec_decreased = false;
        bool current_interrupted = false;
    };
    
    struct timing {
		std::chrono::high_resolution_clock::time_point wait_timepoint; 
		art::shared_ptr<Task> awake_task; 
		uint16_t check_id;
	};
	
    struct binded_context{
		art::util::hill_climb executor_manager_hill_climb;
		std::list<uint32_t> completions;
		std::list<art::shared_ptr<Task>> tasks;
		TaskConditionVariable on_closed_notifier;
		art::recursive_mutex no_race;
		art::condition_variable_any new_task_notifier;
		uint16_t executors = 0;
		bool in_close = false;
		bool allow_implicit_start = false;
		bool fixed_size = false;
	};
	
    struct executor_global {
		TaskConditionVariable no_tasks_notifier;
		TaskConditionVariable no_tasks_execute_notifier;

		std::queue<art::shared_ptr<Task>> tasks;
		std::queue<art::shared_ptr<Task>> cold_tasks;
		std::deque<timing> timed_tasks;

		art::recursive_mutex task_thread_safety;
		art::mutex task_timer_safety;

		art::condition_variable_any tasks_notifier;
		art::condition_variable time_notifier;
		art::condition_variable_any executor_shutdown_notifier;

		size_t executors = 0;
		size_t in_exec = 0;
		bool time_control_enabled = false;

		bool in_time_swap = false;
		std::atomic_size_t tasks_in_swap = 0;
		std::atomic_size_t in_run_tasks= 0;
		std::atomic_size_t planned_tasks = 0;

		TaskConditionVariable can_started_new_notifier;
		TaskConditionVariable can_planned_new_notifier;
		
		art::mutex binded_workers_safety;
		std::unordered_map<uint16_t, binded_context,art::hash<uint16_t>> binded_workers;

		bool executor_manager_in_work = false;
		art::condition_variable_any executor_manager_task_taken;
		art::util::hill_climb executor_manager_hill_climb;
		std::list<uint32_t> workers_completions;

		
	#if _configuration_tasks_enable_debug_mode
		art::rw_mutex debug_safety;
		std::list<Task*> alive_tasks;
		std::list<Task*> on_workers_tasks;
	#endif
	};
    
    struct task_callback {
		static void dummy(ValueItem&){}
		ValueItem args;
		void(*on_start)(ValueItem&);
		void(*on_await)(ValueItem&);
		void(*on_cancel)(ValueItem&);
		void(*on_timeout)(ValueItem&);
		void(*on_destruct)(ValueItem&);
		task_callback(ValueItem& args, void(*on_await)(ValueItem&) = dummy, void(*on_cancel)(ValueItem&) = dummy, void(*on_timeout)(ValueItem&) = dummy, void(*on_start)(ValueItem&) = dummy, void(*on_destruct)(ValueItem&) = dummy) : args(args), on_start(on_start), on_await(on_await), on_cancel(on_cancel), on_timeout(on_timeout), on_destruct(on_destruct){}
		~task_callback() {
			if(on_destruct)
				on_destruct(args);
		}
		static void start(Task& task){
			auto self = (task_callback*)task.args.val;
			if(task.timeout != std::chrono::high_resolution_clock::time_point::min()){
				if(task.timeout <= std::chrono::high_resolution_clock::now())
					self->on_timeout(self->args);
					return;
			}
			if(self->on_start)
				self->on_start(self->args);
		}
		static bool await(Task& task){
			auto self = (task_callback*)task.args.val;
			if(self->on_await){
				self->on_await(self->args);
				return true;
			}
			else return false;
		}
		static void cancel(Task& task){
			auto self = (task_callback*)task.args.val;
			if(self->on_cancel)
				self->on_cancel(self->args);
		}
		static void timeout(Task& task){
			auto self = (task_callback*)task.args.val;
			if(self->on_timeout)
				self->on_timeout(self->args);
		}
	};
    
    //scheduler functions
	void startTimeController();
    void swapCtx();
    void checkCancellation();
	void swapCtxRelock(const MutexUnify& mut0);
	void swapCtxRelock(const MutexUnify& mut0, const MutexUnify& mut1, const MutexUnify& mut2);
	void swapCtxRelock(const MutexUnify& mut0, const MutexUnify& mut1);
    void transfer_task(art::shared_ptr<Task>& task);
    void makeTimeWait(std::chrono::high_resolution_clock::time_point t);
    void taskExecutor(bool end_in_task_out = false);
    void bindedTaskExecutor(uint16_t id);


    extern thread_local executors_local loc;
    extern executor_global glob;
	constexpr size_t native_thread_flag = size_t(1) << (sizeof(size_t) * 8 - 1);
}

#endif /* SRC_RUN_TIME_TASKS_INTERNAL */
