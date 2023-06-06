// Copyright Danyil Melnytskyi 2022-2023
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)


#include "run_time.hpp"
#include <atomic>
#include <thread>
#include <vector>
#include <cassert>
#include <unordered_map>
#include "run_time/standard_lib.hpp"
#include "run_time/attacha_abi_structs.hpp"
#include "run_time/run_time_compiler.hpp"
#include "run_time/tasks.hpp"
#include <stdio.h>
#include <typeinfo>
#include <Windows.h>
#include "run_time/CASM.hpp"
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
	//std::this_thread::sleep_for(std::chrono::nanoseconds(1));
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
	typed_lgr<FuncEnviropment> func = new FuncEnviropment(paralelize_test_0_0,true);
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
	typed_lgr<FuncEnviropment> func = new FuncEnviropment(paralelize_test_1_0, true);
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
	ValueItem msq(AttachA::cxxCall("internal stack trace"));
	console::printLine(&msq, 1);
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
			ValueItem msq("Passed ex_handle_test 0");
			console::printLine(&msq, 1);
		}
		FuncEnviropment::ForceUnload("ex_handle_test");
	}
}
#pragma optimize("",on)
void interface_test() {
	FuncEviroBuilder build;
	build.set_constant(0, "D:\\helloWorld.bin");
	build.arg_set(0);
	build.call("# file file_handle", 1, false);
	build.set_constant(0, { 123ui8, 45ui8, 67ui8 });
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
	ValueItem msq("test");
	console::print(&msq, 1);


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
	ValueItem msq;
	msq = light_stack::used_size();
	console::printLine(&msq, 1);
	alloca(100000);
	msq = light_stack::used_size();
	console::printLine(&msq, 1);
}
int main(){
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
	ProxyClass& proxy = *(ProxyClass*)args[0].getSourcePtr();
	ProxyClass& client_ip = *(ProxyClass*)args[1].getSourcePtr();
	ProxyClass& local_ip = *(ProxyClass*)args[2].getSourcePtr();
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
	ProxyClass& proxy = *(ProxyClass*)args[0].getSourcePtr();
	if(!AttachA::Interface::makeCall(ClassAccess::pub, proxy, "is_closed")){
		while(AttachA::Interface::makeCall(ClassAccess::pub, proxy, "data_available"))
			AttachA::Interface::makeCall(ClassAccess::pub, proxy, "read_available_ref");
		ValueItem file = AttachA::cxxCall("# file file_handle", "D:\\sample_hello_world_http_response.txt");
		ProxyClass& file_handle = (ProxyClass&)file;
		uint64_t file_size = (uint64_t)AttachA::Interface::makeCall(ClassAccess::pub, file_handle, "size");
		ValueItem readed = AttachA::Interface::makeCall(ClassAccess::pub, file_handle, "read", file_size);
		readed.getAsync();
		AttachA::Interface::makeCall(ClassAccess::pub, proxy, "write", readed);
	}
	return nullptr;
}
ValueItem file;

ValueItem* test_fast_server_http(ValueItem* args, uint32_t argc){
	ProxyClass& proxy = *(ProxyClass*)args[0].getSourcePtr();
	if(!AttachA::Interface::makeCall(ClassAccess::pub, proxy, "is_closed")){
		while(AttachA::Interface::makeCall(ClassAccess::pub, proxy, "data_available"))
			AttachA::Interface::makeCall(ClassAccess::pub, proxy, "read_available_ref");
		AttachA::Interface::makeCall(ClassAccess::pub, proxy, "write_file", file);
	}
	return nullptr;
}

#include "run_time/networking.hpp"
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
int smain(){
	unhandled_exception.join(new FuncEnviropment(logger<_FATAL>, false));
	errors.join(new FuncEnviropment(logger<_ERROR>, false));
	warning.join(new FuncEnviropment(logger<_WARN>, false));
	info.join(new FuncEnviropment(logger<_INFO>, false));

	Task::create_executor(1);
	init_networking();
	initStandardLib_file();
		file = AttachA::cxxCall("# file file_handle", "D:\\sample_hello_world_http_response.txt");


	ValueItem arg("0.0.0.0:1234");
	ValueItem arg2("0.0.0.0:1235");
	TcpNetworkServer server(new FuncEnviropment(test_slow_server_http, false), arg, TcpNetworkServer::ManageType::write_delayed,20);
	TcpNetworkServer server2(new FuncEnviropment(test_fast_server_http, false), arg2, TcpNetworkServer::ManageType::write_delayed,20);
	server.start();
	server2.start();
	server._await();
	server2._await();
	return 0;
}



ValueItem* table_jump(int8_t default_c){
	FuncEviroBuilder builder;
	builder.set_constant(0, ValueItem(default_c));
	builder.table_jump(
		{
			0,1,2,3
		},
		0,
		true,
		TableJumpCheckFailAction::jump_specified,
		4,
		TableJumpCheckFailAction::jump_specified,
		5
	);
	builder.bind_pos();//0
	builder.ret();
	builder.bind_pos();//1
	builder.set_constant(0, ValueItem(7));
	builder.ret(0);
	builder.bind_pos();//2
	builder.set_constant(0, ValueItem(6));
	builder.ret(0);
	builder.bind_pos();//3
	builder.set_constant(0, ValueItem(5));
	builder.ret(0);
	builder.bind_pos();//4
	builder.set_constant(0, ValueItem(10));
	builder.ret(0);
	builder.bind_pos();//5
	builder.set_constant(0, ValueItem(30));
	builder.ret(0);
	return FuncEnviropment::sync_call(builder.prepareFunc(), nullptr, 0);
}
int amain(){
	ValueItem* res = table_jump(33);
	console::printLine(res, 1);
	if(res)
		delete res;
	res = table_jump(-33);
	console::printLine(res, 1);
	if (res)
		delete res;
	return 0;
}
