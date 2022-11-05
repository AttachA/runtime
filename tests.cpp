// Copyright Danyil Melnytskyi 2022
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

#include "run_time/attacha_abi_structs.hpp"
#include "run_time/run_time_compiler.hpp"
#include "run_time/Tasks.hpp"
#include <stdio.h>
#include <typeinfo>
#include <Windows.h>
#include "run_time/CASM.hpp"
struct test_struct {
	uint64_t a, b;
};


void SOVER() {
	void* test = alloca(100000);
	SOVER();
}


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
	auto started = std::chrono::high_resolution_clock::now();
	Task::sleep(1000);
	std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - started).count() << std::endl;
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
	catch (const TaskCancellation& c) {
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
	throw 0;
}

void ex_handle_test() {
	FuncEnviropment::AddNative(__ex_handle_test, "__ex_handle_test", false);
	FuncEviroBuilder build;
	build.call_and_ret("__ex_handle_test");
	try {
		build.prepareFunc()->syncWrapper(nullptr,0);
	}
	catch (...) {
		std::cout << "Cathced" << std::endl;
	}
}
void interface_test() {
	FuncEviroBuilder build;
	build.set_constant(0, "/helloWorld.bin");
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
int main() {
	initStandardFunctions();
	Task::max_running_tasks = 4000;
	Task::max_planned_tasks = 0;
	Task::create_executor(1);
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
	build.call("console setTextColor");

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


	//for (size_t i = 0; i < 1000000; i++)
	//	callFunction("start", false);


	FuncEnviropment::AddNative(TestCall, "test");
	FuncEnviropment::AddNative(ThrowCall, "throwcall");
	FuncEnviropment::AddNative(SOVER, "stack_owerflower");


	FuncEnviropment::AddNative(gvfdasf, "1");
	FuncEnviropment::AddNative(sdagfsgsfdg, "2");
	FuncEnviropment::AddNative(fvbzxcbxcv, "3");
	FuncEnviropment::AddNative(a3tgr4at, "4");
	FuncEnviropment::AddNative(cout_test, "cout_test");
	FuncEnviropment::AddNative(sleep_test, "sleep_test");

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


	for (size_t i = 0; i < 10000; i++) {
		//tasks.push_back(new Task(FuncEnviropment::enviropment("start"), noting));
		//tasks.push_back(new Task(FuncEnviropment::enviropment("1"), noting));
		tasks.push_back(new Task(env, noting));
	}

	Task::await_multiple(tasks);
	for (size_t i = 0; i < 10000; i++)
		Task::start(new Task(env, noting));
	Task::await_end_tasks(true);
	for (size_t i = 0; i < 10000; i++)
		Task::start(new Task(env, noting));
	Task::await_end_tasks(true);
	Task::sleep(100000000000);

	console::resetBgColor();
	return e;
}