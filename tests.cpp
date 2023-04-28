// Copyright Danyil Melnytskyi 2022-2023
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#include "run_time.hpp"
#include <atomic>
#include <iostream>
#include <thread>
#include <mutex>
#include <vector>
#include <cassert>
#include <unordered_map>
#include "run_time/standard_lib.hpp"
#include "run_time/attacha_abi_structs.hpp"
#include "run_time/run_time_compiler.hpp"
#include "run_time/Tasks.hpp"
#include <stdio.h>
#include <typeinfo>
#include <Windows.h>
#include "run_time/CASM.hpp"
#include "run_time/tasks_util/light_stack.hpp"
struct test_struct {
	uint64_t a, b;
};
#pragma warning(push) 
#pragma warning(disable : 4717)
void SOVER() {
	void* test = alloca(100000);
	SOVER();
}
#pragma warning(pop)


void TestCall() {
	std::cout << "Hello from proxy" << std::endl;
}
void ThrowCall() {
	throw std::exception();
}

TaskMutex tsk_mtx;
size_t executed = 0;
void gvfdasf() {
	for (size_t i = 0; i < 100; i++) {
		tsk_mtx.lock();
		std::cout << "Hello, " << std::flush;
		executed++;
		tsk_mtx.unlock();
	}
}

void sdagfsgsfdg() {
	for (size_t i = 0; i < 100; i++) {
		tsk_mtx.lock();
		std::cout << "World" << std::flush;
		tsk_mtx.unlock();
	}
}
void fvbzxcbxcv() {
	for (size_t i = 0; i < 100; i++) {
		tsk_mtx.lock();
		std::cout << "!\n" << std::flush;
		tsk_mtx.unlock();
	}
}
void a3tgr4at() {
	tsk_mtx.lock();
	Task::sleep(4000);
	tsk_mtx.unlock();
}

size_t idsaDAS = 0;

void cout_test() {
	//std::this_thread::sleep_for(std::chrono::nanoseconds(1));
	++idsaDAS;
	std::cout << idsaDAS << std::endl;
	Task::sleep(1000);
}
void sleep_test() {
	//light_stack::dump_current_out();
	auto started = std::chrono::high_resolution_clock::now();
	Task::sleep(1000);
	std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - started).count() << std::endl;
	
	//light_stack::dump_current_out();
}
struct test_lgr {
	bool depth_safety() const {
		return self.depth_safety();
	}
	typed_lgr<test_lgr> self;
};
void lgr_loop_test() {
	typed_lgr tlgr(new test_lgr());
	tlgr->self = tlgr;
}

ValueItem* paralelize_test_0_0(ValueItem*,uint32_t) {
	for (size_t i = 0; i < 100; i++) {
		tsk_mtx.lock();
		tsk_mtx.unlock();
	}
	return nullptr;
}

ValueItem* paralelize_test_0(ValueItem*, uint32_t) {
	auto started = std::chrono::high_resolution_clock::now();
	list_array<typed_lgr<Task>> tasks;
	typed_lgr<FuncEnviropment> func = new FuncEnviropment(paralelize_test_0_0,true);
	ValueItem noting;
	for (size_t i = 0; i < 10000; i++)
		tasks.push_back(new Task(func, noting));

	Task::await_multiple(tasks);
	std::cout << "lock speed: " << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - started).count() << std::endl;
	return nullptr;
}

ValueItem* paralelize_test_1_0(ValueItem*, uint32_t) {
	Task::sleep(1000);
	return nullptr;
}

ValueItem* paralelize_test_1(ValueItem*, uint32_t) {
	auto started = std::chrono::high_resolution_clock::now();
	list_array<typed_lgr<Task>> tasks;
	typed_lgr<FuncEnviropment> func = new FuncEnviropment(paralelize_test_1_0, true);
	ValueItem noting;
	for (size_t i = 0; i < 50000; i++)
		tasks.push_back(new Task(func, noting));

	Task::await_multiple(tasks);
	std::cout << "sleep 1 sec speed: " << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - started).count() << std::endl;
	return nullptr;
}

ValueItem* timeout_test(ValueItem*, uint32_t) {
	auto started = std::chrono::high_resolution_clock::now();
	try {
		while (true) {
			Task::yield();
		}
	}
	catch (const TaskCancellation&) {
		std::cout << "task canceled: " << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - started).count() << std::endl;
		throw;
	}
	return nullptr;
}


#include "run_time/library/console.hpp"
#include "run_time/AttachA_CXX.hpp"




void proxyTest() {

	FuncEviroBuilder build;
	build.call("# chanel chanel", 0, false);




	build.set_constant(0, "The test text, Current color: r%d,g%d,b%d\n");
	build.set_stack_any_array(1, 4);

	build.set_constant(2, 12ui8);
	build.arr_set(1, 2, 0, false, ArrCheckMode::no_check, VType::faarr);

	build.set_constant(2, 128ui8);
	build.arr_set(1, 2, 1, false, ArrCheckMode::no_check, VType::faarr);

	build.set_constant(2, 12ui8);
	build.arr_set(1, 2, 2, false, ArrCheckMode::no_check, VType::faarr);


	build.arg_set(1);
	build.call("console setTextColor");

	build.arr_set(1, 0, 0, false, ArrCheckMode::no_check, VType::faarr);
	build.set_constant(2, 12ui8);
	build.arr_set(1, 2, 1, false, ArrCheckMode::no_check, VType::faarr);

	build.set_constant(2, 128ui8);
	build.arr_set(1, 2, 2, false, ArrCheckMode::no_check, VType::faarr);

	build.set_constant(2, 12ui8);
	build.arr_set(1, 2, 3, false, ArrCheckMode::no_check, VType::faarr);

	build.arg_set(1);
	build.call_and_ret("console printf");
	build.remove(1);
	build.ret();
	build.loadFunc("start");
}




void __ex_handle_test() {
	std::cout << (std::string)AttachA::cxxCall("internal stack trace") << std::endl;
	throw 0;
}
#pragma optimize("",off)
void ex_handle_test() {
	FuncEnviropment::AddNative(__ex_handle_test, "__ex_handle_test", false);
	
	{
		FuncEviroBuilder build;
		build.call_and_ret("__ex_handle_test");
		build.loadFunc("ex_handle_test");
		try {
			callFunction("ex_handle_test", false);
		}
		catch (...) {
			std::cout << "Passed ex_handle_test 0" << std::endl;
		}
		FuncEnviropment::ForceUnload("ex_handle_test");
	}
}
#pragma optimize("",on)
void interface_test() {
	FuncEviroBuilder build;
	build.set_constant(0, "D:\\helloWorld.bin");
	build.arg_set(0);
	build.call("# parallel concurent_file", 1, false);
	build.set_constant(0, { 123ui16, 212ui16, 4213ui16 });
	build.arg_set(0);
	build.call_value_interface(ClassAccess::pub, 1, "write", 1, false, false);
	build.explicit_await(1);
	build.ret();

	build.prepareFunc()->syncWrapper(nullptr, 0);
}


typedef void (*functs)(...);

void task_query_test(){
	typed_lgr<FuncEnviropment> env = FuncEnviropment::enviropment("sleep_test");
	TaskQuery query(300);
	ValueItem noting;
	//for (size_t i = 0; i < 10000; i++) {
	//	query.add_task(FuncEnviropment::enviropment("start"), noting);
	//	query.add_task(FuncEnviropment::enviropment("1"), noting);
	//	query.add_task(env, noting);
	//}
	query.enable();
	query.wait();
}



ValueItem* attacha_main(ValueItem* args, uint32_t argc) {
	light_stack::dump_current_out();

	ex_handle_test();
	interface_test();
	Task::create_executor(1);
	//Task::sleep(1000);
	//Task::create_executor(10);
	//Task::sleep(1000);


	ValueItem noting;
	//Task::start(new Task(new FuncEnviropment(paralelize_test_0, true), noting));
	//Task::start(new Task(new FuncEnviropment(paralelize_test_1, true), noting));
	//Task::start(new Task(new FuncEnviropment(timeout_test, true), noting,false,nullptr, std::chrono::high_resolution_clock::now() + std::chrono::seconds(30)));
	//Task::await_end_tasks(true);
	//Task::sleep(1000);

	console::setBgColor(123, 21, 2);
	console::setTextColor(0, 230,0);
	std::cout << "test";


	FuncEviroBuilder build;
	build.set_constant(0, "The test text, Current color: r%d,g%d,b%d\n");
	build.set_stack_any_array(1, 4);
	//build.set_constant(1, ValueItem(new ValueItem[4]{}, 4));

	build.set_constant(2, 12ui8);
	build.arr_set(1, 2, 0,false, ArrCheckMode::no_check, VType::faarr);

	build.set_constant(2, 128ui8);
	build.arr_set(1, 2, 1, false, ArrCheckMode::no_check, VType::faarr);

	build.set_constant(2, 12ui8);
	build.arr_set(1, 2, 2, false, ArrCheckMode::no_check, VType::faarr);


	build.arg_set(1);
	build.call("console set_text_color");

	build.arr_set(1, 0, 0, false, ArrCheckMode::no_check, VType::faarr);
	build.set_constant(2, 12ui8);
	build.arr_set(1, 2, 1, false, ArrCheckMode::no_check, VType::faarr);

	build.set_constant(2, 128ui8);
	build.arr_set(1, 2, 2, false, ArrCheckMode::no_check, VType::faarr);

	build.set_constant(2, 12ui8);
	build.arr_set(1, 2, 3, false, ArrCheckMode::no_check, VType::faarr);

	build.arg_set(1);
	build.call("console printf");
	build.remove(1);
	build.ret();
	build.loadFunc("start");
	callFunction("start", false);

	task_query_test();


	//for (size_t i = 0; i < 1000000; i++)
	//	callFunction("start", false);



	{
		FuncEviroBuilder build;
		build.call_and_ret("Yay");
		build.loadFunc("Yay");
	}
	typed_lgr<FuncEnviropment> env = FuncEnviropment::enviropment("sleep_test");
	////Task::start(new Task(FuncEnviropment::enviropment("4"), nullptr));
	//Task::start(new Task(FuncEnviropment::enviropment("3"), nullptr));
	//Task::start(new Task(FuncEnviropment::enviropment("2"), nullptr));
	//Task::start(new Task(FuncEnviropment::enviropment("1"), nullptr));
	
	Task::await_end_tasks(true);
	try {
		callFunction("start", false);
	}
	catch (const std::exception&) {
		std::cout << "Catched!\n";
	}
	catch (const StackOverflowException&) {
		std::cout << "Catched!\n";
	}

	//std::cout << "Hello!\n";
	int e = 0;
	Task::await_end_tasks(true);
	{
		Task::start(new Task(env, noting));
		Task::await_end_tasks(true);
	}

	list_array<typed_lgr<Task>> tasks;


	//for (size_t i = 0; i < 10000; i++) {
	//	tasks.push_back(new Task(FuncEnviropment::enviropment("start"), noting));
	//	tasks.push_back(new Task(FuncEnviropment::enviropment("1"), noting));
	//	tasks.push_back(new Task(env, noting));
	//}
	//
	//Task::await_multiple(tasks);
	//Task::clean_up();
	//tasks.clear();
	for (size_t i = 0; i < 10000; i++)
		tasks.push_back(new Task(env, noting));
	Task::await_multiple(tasks);
	Task::clean_up();
	tasks.clear();
	for (size_t i = 0; i < 10000; i++)
		tasks.push_back(new Task(env, noting));
	Task::await_multiple(tasks);
	tasks.clear();

	console::resetBgColor();
	return new ValueItem(e);
}

ValueItem* empty_fun(ValueItem* args, uint32_t argc) {

	std::this_thread::sleep_for(std::chrono::seconds(1));
	Task::yield();
	return nullptr;
}
void test_stack(){
	std::cout <<light_stack::used_size() << std::endl;
	alloca(100000);
	std::cout <<light_stack::used_size() << std::endl;
}
int main(){
	test_stack();
	light_stack::shrink_current();
	std::cout <<light_stack::used_size() << std::endl;

	//std::cout << 1 << light_stack::free_size() << " " << light_stack::allocated_size() << " " << light_stack::unused_size() << " " << light_stack::used_size() << std::endl;
	//light_stack::dump_current_out();
	//std::cout << light_stack::prepare(4096*100) << " " << light_stack::free_size() << " " << light_stack::allocated_size() << " " << light_stack::unused_size() << " " << light_stack::used_size() << std::endl;
	//light_stack::dump_current_out();
	//std::cout << light_stack::shrink_current(4096*20) << " " << light_stack::free_size() << " " << light_stack::allocated_size() << " " << light_stack::unused_size() << " " << light_stack::used_size() << std::endl;
	//light_stack::dump_current_out();
	//std::cout << light_stack::prepare() << " " << light_stack::free_size() << " " << light_stack::allocated_size() << " " << light_stack::unused_size() << " " << light_stack::used_size() << std::endl;
	//light_stack::dump_current_out();
	//std::cout << light_stack::shrink_current(4096*20) << " " << light_stack::free_size() << " " << light_stack::allocated_size() << " " << light_stack::unused_size() << " " << light_stack::used_size() << std::endl;
	//light_stack::dump_current_out();
	//std::cout << light_stack::prepare() << " " << light_stack::free_size() << " " << light_stack::allocated_size() << " " << light_stack::unused_size() << " " << light_stack::used_size() << std::endl;
	//light_stack::dump_current_out();
	//std::cout << light_stack::shrink_current(0) << " " << light_stack::free_size() << " " << light_stack::allocated_size() << " " << light_stack::unused_size() << " " << light_stack::used_size() << std::endl;
	//light_stack::dump_current_out();
	initStandardLib();
	FuncEnviropment::AddNative(TestCall, "test", false);
	FuncEnviropment::AddNative(ThrowCall, "throwcall", false);
	FuncEnviropment::AddNative(SOVER, "stack_owerflower", false);


	FuncEnviropment::AddNative(gvfdasf, "1", false);
	FuncEnviropment::AddNative(sdagfsgsfdg, "2", false);
	FuncEnviropment::AddNative(fvbzxcbxcv, "3", false);
	FuncEnviropment::AddNative(a3tgr4at, "4", false);
	FuncEnviropment::AddNative(cout_test, "cout_test", false);
	FuncEnviropment::AddNative(sleep_test, "sleep_test", false);
	enable_thread_naming = false;
	Task::max_running_tasks = 0;
	Task::max_planned_tasks = 0;

	Task::create_executor(5);

	ValueItem noting;
	typed_lgr<FuncEnviropment> main_env = new FuncEnviropment(attacha_main, false);
	typed_lgr<FuncEnviropment> empty_env = new FuncEnviropment(empty_fun, false);


	//typed_lgr<Task> empty_task = new Task(empty_env, noting);
	//Task::start(empty_task);
	//Task::await_end_tasks(false);

	//attacha_main(nullptr, 0);

	typed_lgr<Task> main_task = new Task(main_env, noting);
	Task::start(main_task);
	Task::await_end_tasks(false);
	ValueItem* res = Task::get_result(main_task);
	if (res != nullptr)
		{
			int int_res = (int)*res;
			delete res;
			return int_res;
		}
	else
		return 0;
}