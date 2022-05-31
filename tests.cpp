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

#include "run_time/attacha_abi_structs.hpp"
#include "run_time/run_time_compiler.hpp"
#include "run_time/Tasks.hpp"
#include <stdio.h>
#include <typeinfo>
#include <windows.h>
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
void gvfdasf() {
	for (size_t i = 0; i < 100; i++) {
		tsk_mtx.lock();
		std::cout << "Hello, " << std::flush;
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
	std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - started) << std::endl;
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


//


#include "run_time/library/console.hpp"
#include "run_time/AttachA_CXX.hpp"
typedef void (*functs)(...);
int main() {
	initStandardFunctions();
	Task::createExecutor(16);
	Task::max_running_tasks = 3500;
	Task::max_planned_tasks = 8000;

	list_array<int> test{12,3,4,56,88};
	test.reserve_push_back(10);
	test.reserve_push_front(10);

	console::setBgColor(123, 21, 2);
	console::setTextColor(0, 230,0);
	std::cout << "test";

	FuncEviroBuilder build;
	build.setConstant(0, "The test text, Current color: r%d,g%d,b%d\n");
	build.setConstant(2, 12ui8);
	build.arr_push_end(1, 2);
	build.setConstant(2, 128i8);
	build.arr_push_end(1, 2);
	build.setConstant(2, 12ui8);
	build.arr_push_end(1, 2);
	build.arg_set(1);
	build.call("console setTextColor");
	build.arr_resize(1, 0);

	build.arr_push_end(1, 0);
	build.setConstant(2, 12ui8);
	build.arr_push_end(1, 2);
	build.setConstant(2, 128ui8);
	build.arr_push_end(1, 2);
	build.setConstant(2, 12ui8);
	build.arr_push_end(1, 2);
	build.arg_set(1);
	build.call_and_ret("console printf");
	build.loadFunc("start");

	Task::createExecutor(1);

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
	
	Task::awaitEndTasks();
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
	Task::awaitEndTasks();
	{
		Task::start(new Task(env, nullptr));
		Task::awaitEndTasks();
	}
	for (size_t i = 0; i < 10000; i++) {
		Task::start(new Task(FuncEnviropment::enviropment("start"), nullptr));
		Task::start(new Task(FuncEnviropment::enviropment("1"), nullptr));
		Task::start(new Task(env, nullptr));
	}
	Task::awaitEndTasks();
	for (size_t i = 0; i < 10000; i++)
		Task::start(new Task(env, nullptr));
	Task::awaitEndTasks();
	for (size_t i = 0; i < 10000; i++)
		Task::start(new Task(env, nullptr));
	Task::awaitEndTasks();
	Task::sleep(100000000000);

	console::resetBgColor();
	return e;
}