// Copyright Danyil Melnytskyi 2022-2023
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#include <base/run_time.hpp>
#include <run_time/AttachA_CXX.hpp>
#include <run_time/standard_lib.hpp>

#include <run_time/library/console.hpp>
using namespace art;

void sleep_test() {
	auto started = std::chrono::high_resolution_clock::now();
	Task::sleep(1000);
	uint64_t time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - started).count();
	ValueItem msq(time);
	console::printLine(&msq, 1);	
}





ValueItem* attacha_main(ValueItem* args, uint32_t argc) {
	ValueItem noting;

	console::setBgColor(123, 21, 2);
	console::setTextColor(0, 230,0);
	ValueItem msq("test");
	console::print(&msq, 1);


	FuncEnviroBuilder build;
	auto fn_console_set_text_color = build.create_constant("console set_text_color");
	auto fn_console_printf = build.create_constant("console printf");
	auto text = build.create_constant("The test text, Current color: r%d,g%d,b%d\n");
	auto num12 = build.create_constant((uint8_t)12);
	auto num128 = build.create_constant((uint8_t)128);
	auto num0 = build.create_constant((uint8_t)0);
	auto num1 = build.create_constant((uint8_t)1);
	auto num2 = build.create_constant((uint8_t)2);
	auto num3 = build.create_constant((uint8_t)3);


	build.set_stack_any_array(0_env, 4);
	build.arg_set(0_env);

	build.static_arr(0_env, VType::saarr).set(num12, num0, false, ArrCheckMode::no_check);
	build.static_arr(0_env, VType::saarr).set(num128, num1, false, ArrCheckMode::no_check);
	build.static_arr(0_env, VType::saarr).set(num12, num2, false, ArrCheckMode::no_check);
	build.call(fn_console_set_text_color);

	build.static_arr(0_env, VType::saarr).set(text, num0, false, ArrCheckMode::no_check);
	build.static_arr(0_env, VType::saarr).set(num12, num1, false, ArrCheckMode::no_check);
	build.static_arr(0_env, VType::saarr).set(num128, num2, false, ArrCheckMode::no_check);
	build.static_arr(0_env, VType::saarr).set(num12, num3, false, ArrCheckMode::no_check);
	build.arg_set(0_env);
	build.call(fn_console_printf);

	build.remove(0_env);
	build.ret();
	build.O_load_func("start");
	CXX::cxxCall("start");


	//for (size_t i = 0; i < 1000000; i++)
	//	CXX::cxxCall("start");



	{
		FuncEnviroBuilder build;
		auto fn_Yay = build.create_constant("Yay");
		build.call_and_ret(fn_Yay);
		build.O_load_func("Yay");
	}
	art::shared_ptr<FuncEnvironment> env = FuncEnvironment::environment("sleep_test");
	////Task::start(new Task(FuncEnvironment::environment("4"), nullptr));
	//Task::start(new Task(FuncEnvironment::environment("3"), nullptr));
	//Task::start(new Task(FuncEnvironment::environment("2"), nullptr));
	//Task::start(new Task(FuncEnvironment::environment("1"), nullptr));
	
	Task::await_end_tasks(true);
	try {
		CXX::cxxCall("start");
	}
	catch (const std::exception&) {
		msq = "Catched!\n";
		console::print(&msq, 1);
	}
	catch (const StackOverflowException&) {
		msq = "Catched!\n";
		console::print(&msq, 1);
	}

	//msq = "Hello!\n";
	//console::print(&msq, 1);
	int e = 0;
	Task::await_end_tasks(true);
	{
		Task::start(new Task(env, noting));
		Task::await_end_tasks(true);
	}

	list_array<art::shared_ptr<Task>> tasks;

	//for (size_t i = 0; i < 10000; i++) {
	//	tasks.push_back(new Task(FuncEnvironment::environment("start"), noting));
	//	tasks.push_back(new Task(FuncEnvironment::environment("1"), noting));
	//	tasks.push_back(new Task(env, noting));
	//}
	//
	//Task::await_multiple(tasks);
	//Task::clean_up();
	//tasks.clear();
	///for (size_t i = 0; i < 10000; i++)
	///	tasks.push_back(new Task(env, noting));
	///Task::await_multiple(tasks);
	///Task::clean_up();
	///tasks.clear();
	///for (size_t i = 0; i < 10000; i++)
	///	tasks.push_back(new Task(env, noting));
	///Task::await_multiple(tasks);
	///tasks.clear();
	for (size_t i = 0; i < 1000000; i++)
		tasks.push_back(new Task(env, noting));
	Task::await_multiple(tasks);
	tasks.clear();
	Task::clean_up();

	console::resetBgColor();
	Task::sleep(1000);
	return new ValueItem(e);
}


template<const char* prefix>
ValueItem* logger(ValueItem* args, uint32_t argc) {
	art::ustring output(prefix);
	output += ": [";
	for(uint32_t i = 0; i < argc; i++){
		output += (art::ustring)args[i];
		if(i != argc - 1)
			output += ", ";
	}
	output += "]";
	ValueItem msq(output);
	console::printLine(&msq, 1);
	return nullptr;
}
const char _FATAL[] = "FATAL";
const char _ERROR[] = "ERROR";
const char _WARN[] = "WARN";
const char _INFO[] = "INFO";
#include <run_time/ValueEnvironment.hpp>
int main(){
	unhandled_exception.join(new FuncEnvironment(logger<_FATAL>, false, false));
	errors.join(new FuncEnvironment(logger<_ERROR>, false, false));
	warning.join(new FuncEnvironment(logger<_WARN>, false, false));
	info.join(new FuncEnvironment(logger<_INFO>, false, false));
	auto timer_start = std::chrono::high_resolution_clock::now();
	initStandardLib();
	FuncEnvironment::AddNative(sleep_test, "sleep_test", false);
	enable_thread_naming = true;
	Task::max_running_tasks = 20000;
	Task::max_planned_tasks = 0;

	art::shared_ptr<Task> main_task = new Task(new FuncEnvironment(attacha_main, false, false), {});
	main_task->bind_to_worker_id = Task::create_bind_only_executor(0, true);
	Task::start(main_task);
	_Task_unsafe::become_executor_count_manager(true);
	ValueItem* res = Task::get_result(main_task);
	if(res != nullptr)
		res->getAsync();
	Task::shutDown();
	Task::clean_up();
	auto timer_end = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double, std::milli> elapsed = timer_end - timer_start;
	ValueItem msq("Time: " + std::to_string(elapsed.count()) + "ms");
	console::printLine(&msq, 1);
	if (res != nullptr){
		try{
			console::printLine(res, 1);
		}catch(...){ return -1;}
		delete res;
	}
	else
		return 0;
}