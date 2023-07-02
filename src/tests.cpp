// Copyright Danyil Melnytskyi 2022-2023
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#include "run_time.hpp"
#include <atomic>
#include <vector>
#include <cassert>
#include <unordered_map>
#include "run_time/standard_lib.hpp"
#include "run_time/attacha_abi_structs.hpp"
#include "run_time/func_enviro_builder.hpp"
#include "run_time/tasks.hpp"
#include <stdio.h>
#include <typeinfo>
#include <Windows.h>
#include "run_time/asm/CASM.hpp"
#include "run_time/tasks_util/light_stack.hpp"
#include "run_time/library/console.hpp"
#include "run_time/AttachA_CXX.hpp"
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
	ValueItem msq("Hello from proxy\n");
	console::printLine(&msq, 1);
}
void ThrowCall() {
	throw std::exception();
}

TaskMutex tsk_mtx;
size_t executed = 0;
void gvfdasf() {
	ValueItem msq("Hello,");

	for (size_t i = 0; i < 100; i++) {
		tsk_mtx.lock();
		console::print(&msq, 1);
		executed++;
		tsk_mtx.unlock();
	}
}

void sdagfsgsfdg() {
	ValueItem msq("World");
	for (size_t i = 0; i < 100; i++) {
		tsk_mtx.lock();
		console::print(&msq, 1);
		tsk_mtx.unlock();
	}
}
void fvbzxcbxcv() {
	ValueItem msq("!\n");
	for (size_t i = 0; i < 100; i++) {
		tsk_mtx.lock();
		console::print(&msq, 1);
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
	Task::sleep(1);
	ValueItem msq(++idsaDAS);
	console::print(&msq, 1);
	Task::sleep(1000);
}
void sleep_test() {
	//light_stack::dump_current_out();
	auto started = std::chrono::high_resolution_clock::now();
	Task::sleep(1000);
	uint64_t time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - started).count();
	ValueItem msq(time);
	console::printLine(&msq, 1);	
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
	typed_lgr<FuncEnvironment> func = new FuncEnvironment(paralelize_test_0_0,true, false);
	ValueItem noting;
	for (size_t i = 0; i < 10000; i++)
		tasks.push_back(new Task(func, noting));

	Task::await_multiple(tasks);
	uint64_t time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - started).count();
	ValueItem msq("lock speed: " + std::to_string(time));
	console::printLine(&msq, 1);
	return nullptr;
}

ValueItem* paralelize_test_1_0(ValueItem*, uint32_t) {
	Task::sleep(1000);
	return nullptr;
}

ValueItem* paralelize_test_1(ValueItem*, uint32_t) {
	auto started = std::chrono::high_resolution_clock::now();
	list_array<typed_lgr<Task>> tasks;
	typed_lgr<FuncEnvironment> func = new FuncEnvironment(paralelize_test_1_0, true, false);
	ValueItem noting;
	for (size_t i = 0; i < 50000; i++)
		tasks.push_back(new Task(func, noting));

	Task::await_multiple(tasks);
	uint64_t time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - started).count();
	ValueItem msq("sleep 1 sec speed: " + std::to_string(time));
	console::printLine(&msq, 1);
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
		uint64_t time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - started).count();
		ValueItem msq("task canceled: " + std::to_string(time));
		console::printLine(&msq, 1);
		throw;
	}
	return nullptr;
}






void proxyTest() {

	FuncEviroBuilder build;
	build.call("# chanel chanel", 0_env, false);




	build.set_constant(0_env, "The test text, Current color: r%d,g%d,b%d\n");
	build.set_stack_any_array(1_env, 4);

	build.set_constant(2_env, 12ui8);
	build.arr_set(1_env, 2_env, 0, false, ArrCheckMode::no_check, VType::faarr);

	build.set_constant(2_env, 128ui8);
	build.arr_set(1_env, 2_env, 1, false, ArrCheckMode::no_check, VType::faarr);

	build.set_constant(2_env, 12ui8);
	build.arr_set(1_env, 2_env, 2, false, ArrCheckMode::no_check, VType::faarr);


	build.arg_set(1_env);
	build.call("console setTextColor");

	build.arr_set(1_env, 0_env, 0, false, ArrCheckMode::no_check, VType::faarr);
	build.set_constant(2_env, 12ui8);
	build.arr_set(1_env, 2_env, 1, false, ArrCheckMode::no_check, VType::faarr);

	build.set_constant(2_env, 128ui8);
	build.arr_set(1_env, 2_env, 2, false, ArrCheckMode::no_check, VType::faarr);

	build.set_constant(2_env, 12ui8);
	build.arr_set(1_env, 2_env, 3, false, ArrCheckMode::no_check, VType::faarr);

	build.arg_set(1_env);
	build.call_and_ret("console printf");
	build.remove(1_env);
	build.ret();
	build.O_load_func("start");
}




void __ex_handle_test() {
	ValueItem msq(AttachA::cxxCall("internal stack trace"));
	console::printLine(&msq, 1);
	throw 0;
}
#pragma optimize("",off)
void ex_handle_test() {
	FuncEnvironment::AddNative(__ex_handle_test, "__ex_handle_test", false);
	
	{
		FuncEviroBuilder build;
		build.call_and_ret("__ex_handle_test");
		build.O_load_func("ex_handle_test");
		try {
			callFunction("ex_handle_test", false);
		}
		catch (...) {
			ValueItem msq("Passed ex_handle_test 0");
			console::printLine(&msq, 1);
		}
		FuncEnvironment::ForceUnload("ex_handle_test");
	}
}
#pragma optimize("",on)
void interface_test() {
	FuncEviroBuilder build;
	build.set_constant(0_env, "D:\\helloWorld.bin");
	build.arg_set(0_env);
	build.call("# file file_handle", 1_env, false);
	build.set_constant(0_env, { 123ui8, 45ui8, 67ui8 });
	build.arg_set(0_env);
	build.call_value_interface(ClassAccess::pub, 1_env, "write", 1_env, false);
	build.explicit_await(1_env);
	build.ret();

	build.O_prepare_func()->syncWrapper(nullptr, 0);
}


typedef void (*functs)(...);

void task_query_test(){
	typed_lgr<FuncEnvironment> env = FuncEnvironment::enviropment("sleep_test");
	TaskQuery query(300);
	ValueItem noting;
	//for (size_t i = 0; i < 10000; i++) {
	//	query.add_task(FuncEnvironment::enviropment("start"), noting);
	//	query.add_task(FuncEnvironment::enviropment("1"), noting);
	//	query.add_task(env, noting);
	//}
	query.enable();
	query.wait();
}
void static_test(){
	FuncEviroBuilder build;
	auto noting = build.create_constant(nullptr);
	auto one = build.create_constant(1);
	build.compare(noting, 0_sta);
	build.jump(JumpCondition::is_not_equal, "not_inited");
	build.set_constant(0_sta, 0ui32);
	build.bind_pos("not_inited");
	build.sum(0_sta, one);
	build.ret(0_sta);
	build.O_load_func("static_test");

	ValueItem cache = AttachA::cxxCall("static_test");
	console::printLine(&cache, 1);
	cache = AttachA::cxxCall("static_test");
	console::printLine(&cache, 1);
	cache = AttachA::cxxCall("static_test");
	console::printLine(&cache, 1);

}


ValueItem* attacha_main(ValueItem* args, uint32_t argc) {
	static_test();
	light_stack::dump_current_out();

	ex_handle_test();
	interface_test();
	Task::create_executor(1);
	//Task::sleep(1000);
	//Task::create_executor(10);
	//Task::sleep(1000);


	ValueItem noting;
	//Task::start(new Task(new FuncEnvironment(paralelize_test_0, true), noting));
	//Task::start(new Task(new FuncEnvironment(paralelize_test_1, true), noting));
	//Task::start(new Task(new FuncEnvironment(timeout_test, true), noting,false,nullptr, std::chrono::high_resolution_clock::now() + std::chrono::seconds(30)));
	//Task::await_end_tasks(true);
	//Task::sleep(1000);

	console::setBgColor(123, 21, 2);
	console::setTextColor(0, 230,0);
	ValueItem msq("test");
	console::print(&msq, 1);


	FuncEviroBuilder build;
	build.set_constant(0_env, "The test text, Current color: r%d,g%d,b%d\n");
	build.set_stack_any_array(1_env, 4);
	//build.set_constant(1, ValueItem(new ValueItem[4]{}, 4));

	build.set_constant(2_env, 12ui8);
	build.arr_set(1_env, 2_env, 0,false, ArrCheckMode::no_check, VType::faarr);

	build.set_constant(2_env, 128ui8);
	build.arr_set(1_env, 2_env, 1, false, ArrCheckMode::no_check, VType::faarr);

	build.set_constant(2_env, 12ui8);
	build.arr_set(1_env, 2_env, 2, false, ArrCheckMode::no_check, VType::faarr);


	build.arg_set(1_env);
	build.call("console set_text_color");

	build.arr_set(1_env, 0_env, 0, false, ArrCheckMode::no_check, VType::faarr);
	build.set_constant(2_env, 12ui8);
	build.arr_set(1_env, 2_env, 1, false, ArrCheckMode::no_check, VType::faarr);

	build.set_constant(2_env, 128ui8);
	build.arr_set(1_env, 2_env, 2, false, ArrCheckMode::no_check, VType::faarr);

	build.set_constant(2_env, 12ui8);
	build.arr_set(1_env, 2_env, 3, false, ArrCheckMode::no_check, VType::faarr);

	build.arg_set(1_env);
	build.call("console printf");
	build.remove(1_env);
	build.ret();
	build.O_load_func("start");
	callFunction("start", false);

	task_query_test();


	//for (size_t i = 0; i < 1000000; i++)
	//	callFunction("start", false);



	{
		FuncEviroBuilder build;
		build.call_and_ret("Yay");
		build.O_load_func("Yay");
	}
	typed_lgr<FuncEnvironment> env = FuncEnvironment::enviropment("sleep_test");
	////Task::start(new Task(FuncEnvironment::enviropment("4"), nullptr));
	//Task::start(new Task(FuncEnvironment::enviropment("3"), nullptr));
	//Task::start(new Task(FuncEnvironment::enviropment("2"), nullptr));
	//Task::start(new Task(FuncEnvironment::enviropment("1"), nullptr));
	
	Task::await_end_tasks(true);
	try {
		callFunction("start", false);
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

	list_array<typed_lgr<Task>> tasks;

	//for (size_t i = 0; i < 10000; i++) {
	//	tasks.push_back(new Task(FuncEnvironment::enviropment("start"), noting));
	//	tasks.push_back(new Task(FuncEnvironment::enviropment("1"), noting));
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
	for (size_t i = 0; i < 10000; i++)
		tasks.push_back(new Task(env, noting));
	Task::await_multiple(tasks);
	tasks.clear();
	Task::clean_up();

	console::resetBgColor();
	Task::sleep(1000);
	return new ValueItem(e);
}

ValueItem* empty_fun(ValueItem* args, uint32_t argc) {
	Task::sleep(1000);
	Task::yield();
	return nullptr;
}
void test_stack(){
	ValueItem msq;
	msq = light_stack::used_size();
	console::printLine(&msq, 1);
	alloca(100000);
	msq = light_stack::used_size();
	console::printLine(&msq, 1);
}
int mmain(){
	ValueItem msq;
	test_stack();
	light_stack::shrink_current();
	msq = light_stack::used_size();
	console::printLine(&msq, 1);

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
	FuncEnvironment::AddNative(TestCall, "test", false);
	FuncEnvironment::AddNative(ThrowCall, "throwcall", false);
	FuncEnvironment::AddNative(SOVER, "stack_owerflower", false);


	FuncEnvironment::AddNative(gvfdasf, "1", false);
	FuncEnvironment::AddNative(sdagfsgsfdg, "2", false);
	FuncEnvironment::AddNative(fvbzxcbxcv, "3", false);
	FuncEnvironment::AddNative(a3tgr4at, "4", false);
	FuncEnvironment::AddNative(cout_test, "cout_test", false);
	FuncEnvironment::AddNative(sleep_test, "sleep_test", false);
	enable_thread_naming = true;
	Task::max_running_tasks = 200000;
	Task::max_planned_tasks = 0;

	//Task::create_executor(5);

	ValueItem noting;
	typed_lgr<FuncEnvironment> main_env = new FuncEnvironment(attacha_main, false, false);
	typed_lgr<FuncEnvironment> empty_env = new FuncEnvironment(empty_fun, false, false);


	//typed_lgr<Task> empty_task = new Task(empty_env, noting);
	//Task::start(empty_task);
	//Task::await_end_tasks(false);

	//attacha_main(nullptr, 0);
	
	typed_lgr<Task> main_task = new Task(main_env, noting);
	main_task->bind_to_worker_id = Task::create_bind_only_executor(0, true);
	Task::start(main_task);
	_Task_unsafe::become_executor_count_manager(true);
	ValueItem* res = Task::get_result(main_task);
	Task::clean_up();
	if (res != nullptr)
		{
			int int_res = (int)*res;
			delete res;
			return int_res;
		}
	else
		return 0;
}

std::vector<uint8_t> to_var_int(int integer){
	std::vector<uint8_t> var_int;
	while (true){
		uint8_t byte = integer & 0b01111111;
		integer >>= 7;
		if (integer != 0){
			byte |= 0b10000000;
		}
		var_int.push_back(byte);
		if (integer == 0){
			break;
		}
	}
	return var_int;
}

std::vector<uint8_t> handshake_fn(){
	std::vector<uint8_t> handshake;
	handshake.push_back(0x00);
	char handshake_data[] = "{\"version\": {\"name\": \"1.19.7\",\"protocol\": 762},"
		"\"players\": {\"max\": 100,\"online\": 5,\"sample\":"
		"[{\"name\": \"thinkofdeath\",\"id\": \"4566e69f-c907-48ee-8d71-d7ba5aa00d20\"}]},"
		"\"description\": {\"text\": \"Hello world\"},\"favicon\": \"data:image/png;base64,<data>\"}";
	std::vector<uint8_t> handshake_data_len = to_var_int(sizeof(handshake_data) - 1);
	handshake.insert(handshake.end(), handshake_data_len.begin(), handshake_data_len.end());
	handshake.insert(handshake.end(), handshake_data, handshake_data + sizeof(handshake_data) - 1);
	
	handshake_data_len = to_var_int(handshake.size());
	handshake.insert(handshake.begin(), handshake_data_len.begin(), handshake_data_len.end());
	return handshake;
}







ValueItem* test_server_func(ValueItem* args, uint32_t argc) {
	Structure& proxy = (Structure&)args[0];
	Structure& client_ip = (Structure&)args[1];
	Structure& local_ip = (Structure&)args[2];
	ValueItem msq;
	msq = ((std::string)AttachA::Interface::makeCall(ClassAccess::pub, client_ip, "to_string")) + " " + (std::string)AttachA::Interface::makeCall(ClassAccess::pub, client_ip, "port");
	console::printLine(&msq, 1);
	msq = ((std::string)AttachA::Interface::makeCall(ClassAccess::pub, local_ip, "to_string")) + " " + (std::string)AttachA::Interface::makeCall(ClassAccess::pub, local_ip, "port");
	console::printLine(&msq, 1);
	bool sended = false;

	auto handshake = handshake_fn();
	//send handshake

	if(!AttachA::Interface::makeCall(ClassAccess::pub, proxy, "is_closed")){
		msq = AttachA::Interface::makeCall(ClassAccess::pub, proxy, "read_available_ref");
		console::printLine(&msq, 1);
		if(AttachA::Interface::makeCall(ClassAccess::pub, proxy, "data_available")){
			msq = AttachA::Interface::makeCall(ClassAccess::pub, proxy, "read_available_ref");
			console::printLine(&msq, 1);
		}

		AttachA::Interface::makeCall(ClassAccess::pub, proxy, "write", ValueItem(handshake.data(), ValueMeta(VType::raw_arr_ui8, false, false, handshake.size())));

		AttachA::Interface::makeCall(ClassAccess::pub, proxy, "force_write");
		ValueItem ping = AttachA::Interface::makeCall(ClassAccess::pub, proxy, "read_available_ref");
		AttachA::Interface::makeCall(ClassAccess::pub, proxy, "write", ping);


	}
	AttachA::Interface::makeCall(ClassAccess::pub, proxy, "close");

	return nullptr;
}
ValueItem* test_slow_server_http(ValueItem* args, uint32_t argc){
	Structure& proxy = (Structure&)args[0];
	if(!AttachA::Interface::makeCall(ClassAccess::pub, proxy, "is_closed")){
		while(AttachA::Interface::makeCall(ClassAccess::pub, proxy, "data_available"))
			AttachA::Interface::makeCall(ClassAccess::pub, proxy, "read_available_ref");
		ValueItem file = AttachA::cxxCall("# file file_handle", "D:\\sample_hello_world_http_response.txt");
		Structure& file_handle = (Structure&)file;
		uint64_t file_size = (uint64_t)AttachA::Interface::makeCall(ClassAccess::pub, file_handle, "size");
		ValueItem readed = AttachA::Interface::makeCall(ClassAccess::pub, file_handle, "read", file_size);
		readed.getAsync();
		AttachA::Interface::makeCall(ClassAccess::pub, proxy, "write", readed);
	}
	return nullptr;
}
ValueItem file;

ValueItem* test_fast_server_http(ValueItem* args, uint32_t argc){
	Structure& proxy = (Structure&)args[0];
	if(!AttachA::Interface::makeCall(ClassAccess::pub, proxy, "is_closed")){
		while(AttachA::Interface::makeCall(ClassAccess::pub, proxy, "data_available"))
			AttachA::Interface::makeCall(ClassAccess::pub, proxy, "read_available_ref");
		AttachA::Interface::makeCall(ClassAccess::pub, proxy, "write_file", file);
	}
	return nullptr;
}

#include "run_time/cxx_library/networking.hpp"
template<const char* prefix>
ValueItem* logger(ValueItem* args, uint32_t argc) {
	std::string output(prefix);
	output += ": [";
	for(uint32_t i = 0; i < argc; i++){
		output += (std::string)args[i];
		if(i != argc - 1)
			output += ", ";
	}
	output += "]";
	ValueItem msq(output);
	console::printLine(&msq, 1);
	return nullptr;
}
constexpr int ii = '\r';
const char _FATAL[] = "FATAL";
const char _ERROR[] = "ERROR";
const char _WARN[] = "WARN";
const char _INFO[] = "INFO";
int nmain(){
	unhandled_exception.join(new FuncEnvironment(logger<_FATAL>, false, false));
	errors.join(new FuncEnvironment(logger<_ERROR>, false, false));
	warning.join(new FuncEnvironment(logger<_WARN>, false, false));
	info.join(new FuncEnvironment(logger<_INFO>, false, false));

	Task::create_executor(1);
	init_networking();
	initStandardLib_file();
		file = AttachA::cxxCall("# file file_handle", "D:\\sample_hello_world_http_response.txt");


	ValueItem arg("0.0.0.0:1234");
	ValueItem arg2("0.0.0.0:1235");
	TcpNetworkServer server(new FuncEnvironment(test_slow_server_http, false, false), arg, TcpNetworkServer::ManageType::write_delayed,20);
	TcpNetworkServer server2(new FuncEnvironment(test_fast_server_http, false, false), arg2, TcpNetworkServer::ManageType::write_delayed,20);
	
	server.start();
	server2.start();
	_Task_unsafe::become_executor_count_manager(false);
	server._await();
	server2._await();
	return 0;
}



void table_jump(){
	FuncEviroBuilder builder;
	builder.table_jump(
		{ "0", "1", "2", "3" },
		0_arg,
		true,
		TableJumpCheckFailAction::jump_specified,
		"too_big",
		TableJumpCheckFailAction::jump_specified,
		"too_small"
	);
	builder.bind_pos("0");
	builder.ret();
	builder.bind_pos("1");
	builder.set_constant(0_env, ValueItem(7));
	builder.ret(0_env);
	builder.bind_pos("2");
	builder.set_constant(0_env, ValueItem(6));
	builder.ret(0_env);
	builder.bind_pos("3");
	builder.set_constant(0_env, ValueItem(5));
	builder.ret(0_env);
	builder.bind_pos("too_big");
	builder.set_constant(0_env, ValueItem(10));
	builder.ret(0_env);
	builder.bind_pos("too_small");
	builder.set_constant(0_env, ValueItem(30));
	builder.ret(0_env);
	builder.O_load_func("table_jump_test");
}
void table_jump_2(){
	FuncEviroBuilder builder;
	builder.table_jump(
		{ "0", "1", "2", "3" },
		0_arg,
		true,
		TableJumpCheckFailAction::jump_specified,
		"default",
		TableJumpCheckFailAction::jump_specified,
		"default"
	);
	builder.bind_pos("0");
	builder.ret();
	builder.bind_pos("1");
	builder.set_constant(0_env, ValueItem(7));
	builder.ret(0_env);
	builder.bind_pos("2");
	builder.set_constant(0_env, ValueItem(6));
	builder.ret(0_env);
	builder.bind_pos("3");
	builder.set_constant(0_env, ValueItem(5));
	builder.ret(0_env);
	builder.bind_pos("default");
	builder.set_constant(0_env, ValueItem(10));
	builder.ret(0_env);
	builder.O_load_func("table_jump_test");
}
int omain(){
	table_jump_2();
	ValueItem res = AttachA::cxxCall("table_jump_test", 0);
	console::printLine(&res, 1);
	res = AttachA::cxxCall("table_jump_test", 1);
	console::printLine(&res, 1);
	res = AttachA::cxxCall("table_jump_test", 2);
	console::printLine(&res, 1);
	res = AttachA::cxxCall("table_jump_test", 3);
	console::printLine(&res, 1);
	res = AttachA::cxxCall("table_jump_test", 4);
	console::printLine(&res, 1);
	res = AttachA::cxxCall("table_jump_test", -1);
	console::printLine(&res, 1);
	res = AttachA::cxxCall("table_jump_test", -5);
	console::printLine(&res, 1);
	return 0;
}


//ValueItem* _test_destructor(ValueItem* args, uint32_t argc){
//	Structure* str = (Structure*)args[0].getSourcePtr();
//	((std::string*)str->get_data_no_vtable())->~basic_string();
//	return nullptr;
//}
//ValueItem* _test_copy(ValueItem* args, uint32_t argc){
//	Structure* dest = (Structure*)args[0].getSourcePtr();
//	Structure* src = (Structure*)args[1].getSourcePtr();
//	bool in_constructor = (bool)args[2];
//	if(!in_constructor)
//		_test_destructor(args, 1);
//	new(dest->get_data_no_vtable()) std::string(*(std::string*)src->get_data_no_vtable());
//	return nullptr;
//}
//ValueItem* _test_move(ValueItem* args, uint32_t argc){
//	Structure* dest = (Structure*)args[0].getSourcePtr();
//	Structure* src = (Structure*)args[1].getSourcePtr();
//	bool in_constructor = (bool)args[2];
//	if (!in_constructor)
//		_test_destructor(args, 1);
//	new(dest->get_data_no_vtable()) std::string(std::move(*(std::string*)src->get_data_no_vtable()));
//	return nullptr;
//}
//ValueItem* _test_compare(ValueItem* args, uint32_t argc){
//	Structure* a = (Structure*)args[0].getSourcePtr();
//	Structure* b = (Structure*)args[1].getSourcePtr();
//	auto res = (*(std::string*)a->get_data_no_vtable() <=> *(std::string*)b->get_data_no_vtable());
//	if(res < 0)
//		return new ValueItem((int8_t)-1);
//	else if(res > 0)
//		return new ValueItem((int8_t)1);
//	else return nullptr;
//}
ValueItem* _test_print(ValueItem* args, uint32_t argc){
	if(argc != 1)
		return nullptr;
	Structure& str = (Structure&)args[0];
	ValueItem ref(*(std::string*)str.get_data_no_vtable(), as_refrence);
	console::printLine(&ref, 1);
	return nullptr;
}
ValueItem* _test_set_string(ValueItem* args, uint32_t argc){
	if (argc != 2)
		return nullptr;
	Structure* str = (Structure*)args[0].getSourcePtr();
	*(std::string*)str->get_data_no_vtable() = (std::string)args[1];
	return nullptr;
}
int xmain(){
	auto check_res = AttachA::Interface::createProxyTable<std::string>(
		AttachA::Interface::make_method<std::string>("ttt", (const char&(std::string::*)() const)&std::string::back)
	);
	//list_array<MethodInfo> methods;
	//methods.push_back(MethodInfo("print", _test_print,ClassAccess::pub,{},{},{}, "String"));
	//methods.push_back(MethodInfo("set_string", _test_set_string, ClassAccess::pub, { }, {}, {}, "String"));
	//auto table = Structure::createAAVTable(methods, 
	//	new FuncEnvironment(_test_destructor,false),
	//	new FuncEnvironment(_test_copy,false),
	//	new FuncEnvironment(_test_move,false),
	//	new FuncEnvironment(_test_compare,false),
	//	{}
	//);
	auto table = AttachA::Interface::createDTable<std::string>(".",
		AttachA::Interface::direct_method("print", _test_print),
		AttachA::Interface::direct_method("set_string", _test_set_string)
	);
	Structure* str = AttachA::Interface::constructStructure<std::string>(table);
	Structure* str2 = AttachA::Interface::constructStructure<std::string>(table, "Hello Worlz!");
	ValueItem set_stiring_test{ValueItem(str, as_refrence), "Hello World!"};
	str->table_get_dynamic("print", ClassAccess::pub)((ValueItem*)set_stiring_test.val, 1);
	str->table_get_dynamic("set_string", ClassAccess::pub)((ValueItem*)set_stiring_test.val, 2);
	ValueItem ref = Structure::compare(str, str2);
	console::printLine(&ref, 1);
	str->table_get_dynamic("print", ClassAccess::pub)((ValueItem*)set_stiring_test.val, 1);
	Structure::destruct(str);
	return 0;
}

ValueItem* testHandler(ValueItem* args, uint32_t len){
	return nullptr;
}
int main(){
	initStandardLib_file();
	initStandardLib_paralel();
	Task::create_executor(1);
	ValueItem res = AttachA::cxxCall("# file folder_changes_monitor", "D:\\", true);
	ValueItem create_file_event = AttachA::Interface::makeCall(ClassAccess::pub, res, "get_event_file_creation");
	AttachA::Interface::makeCall(ClassAccess::pub, create_file_event, symbols::structures::add_operator, &testHandler);
	AttachA::Interface::makeCall(ClassAccess::pub, res, "start");
	_Task_unsafe::become_executor_count_manager(false);
}