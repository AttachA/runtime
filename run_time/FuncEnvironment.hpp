// Copyright Danyil Melnytskyi 2022-2023
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#pragma once
#include <unordered_map>
#include <vector>
#include <string>
#include "attacha_abi_structs.hpp"
#include "dynamic_call.hpp"
#include "library/exceptions.hpp"
#include "tasks.hpp"
class FuncEnvironment {
public:
	enum class FuncType : uint32_t {
		own,
		native,
		native_lambda_wrapper,
		python,
		csharp,
		java,
		_,
		__,
		___,
		____,
		_____,
		______,
		_______,
		________,
		_________,
		force_unloaded
	};
private:
	std::unordered_map<list_array<ValueItem>, ValueItem> cache_map;
	DynamicCall::FunctionTemplate nat_templ;
	TaskMutex compile_lock;//52 bytes
	FuncType _type : 4;
	uint32_t need_compile : 1 = true;
	uint32_t can_be_unloaded : 1 = true;
	uint32_t force_unload : 1 = false;
	uint32_t is_cheap : 1 = false;//function without context switchs and with fast code
	std::vector<typed_lgr<FuncEnvironment>> used_envs;
	std::vector<typed_lgr<FuncEnvironment>> local_funcs;
	std::vector<uint8_t> cross_code;
	list_array<ValueItem> values;
	Enviropment curr_func = nullptr;
	uint8_t* frame = nullptr;
	void RuntimeCompile();
	void funcComp() {
		sizeof(TaskRecursiveMutex);
		std::lock_guard<TaskMutex> lguard(compile_lock);
		if (need_compile) {
			RuntimeCompile();
			need_compile = false;
		}
	}
public:
	void preCompile() {
		if (need_compile)
			funcComp();
	}
	FuncEnvironment(const std::vector<uint8_t>& code) {
		cross_code = code;
		_type = FuncType::own;
	}
	FuncEnvironment(std::vector<uint8_t>&& code) {
		cross_code = std::move(code);
		_type = FuncType::own;
	}
	FuncEnvironment(list_array<ValueItem>&& dynamic_constants, std::vector<typed_lgr<FuncEnvironment>>&& dynamic_local_funcs, std::vector<uint8_t>&& code) {
		values = std::move(dynamic_constants);
		local_funcs = std::move(dynamic_local_funcs);
		cross_code = std::move(code);
		_type = FuncType::own;
		RuntimeCompile();
		need_compile = false;
		cross_code.clear();
	}
	FuncEnvironment(DynamicCall::PROC proc, const DynamicCall::FunctionTemplate& templ, bool _can_be_unloaded, bool is_cheap = false): is_cheap(is_cheap) {
		nat_templ = templ;
		_type = FuncType::native;
		curr_func = (Enviropment)proc;
		need_compile = false;
		can_be_unloaded = _can_be_unloaded;
	}
	FuncEnvironment(Enviropment proc, bool _can_be_unloaded, bool is_cheap = false) : is_cheap(is_cheap) {
		_type = FuncType::own;
		curr_func = (Enviropment)proc;
		need_compile = false;
		can_be_unloaded = _can_be_unloaded;
	}

	ValueItem* NativeProxy_DynamicToStatic(ValueItem*, uint32_t arguments_size);
	FuncEnvironment() {
		need_compile = false;
		_type = FuncType::own;
	}
	FuncEnvironment(FuncEnvironment&& move) noexcept {
		operator=(std::move(move));
	}
	~FuncEnvironment();
	FuncEnvironment& operator=(FuncEnvironment&& move) noexcept {
		used_envs = std::move(move.used_envs);
		cross_code = std::move(move.cross_code);
		nat_templ = std::move(move.nat_templ);
		curr_func = move.curr_func;
		frame = move.frame;
		_type = move._type;
		need_compile = move.need_compile;
		can_be_unloaded = move.can_be_unloaded;
		//disable destructor
		move.can_be_unloaded = false;
		return *this;
	}

	typed_lgr<FuncEnvironment> localFn(size_t indx) {
		return local_funcs[indx];
	}
	size_t localFnSize() {
		return local_funcs.size();
	}
	ValueItem* localWrapper(size_t indx, ValueItem* arguments, uint32_t arguments_size, bool run_async) {
		if (local_funcs.size() <= indx)
			throw NotImplementedException();
		if (run_async)
			return async_call(local_funcs[indx], arguments, arguments_size);
		else
			return local_funcs[indx]->syncWrapper(arguments, arguments_size);
	}

	ValueItem* syncWrapper(ValueItem* arguments, uint32_t arguments_size);

	static void fastHotPath(const std::string& func_name, const std::vector<uint8_t>& new_cross_code);
	static void fastHotPath(const std::string& func_name, typed_lgr<FuncEnvironment>& new_enviro);
	static typed_lgr<FuncEnvironment> enviropment(const std::string& func_name);
	static ValueItem* callFunc(const std::string& func_name, ValueItem* arguments, uint32_t arguments_size, bool run_async);

	template<class Ret>
	static void AddNative(Ret(*function)(), const std::string& symbol_name, bool can_be_unloaded = true, bool is_cheap = false) {
		DynamicCall::FunctionTemplate templ;
		templ.result = DynamicCall::FunctionTemplate::ValueT::getFromType<Ret>();
		AddNative((DynamicCall::PROC)function, templ, symbol_name, can_be_unloaded);
	}
	template<class Ret, typename... Args>
	static void AddNative(Ret(*function)(Args...), const std::string& symbol_name, bool can_be_unloaded = true, bool is_cheap = false) {
		DynamicCall::FunctionTemplate templ;
		DynamicCall::StartBuildTemplate<Ret,Args...>(function, templ);
		AddNative((DynamicCall::PROC)function, templ, symbol_name, can_be_unloaded);
	}
	static void AddNative(Enviropment function, const std::string& symbol_name, bool can_be_unloaded = true, bool is_cheap = false);
	static void AddNative(DynamicCall::PROC proc, const DynamicCall::FunctionTemplate& templ, const std::string& symbol_name, bool can_be_unloaded = true, bool is_cheap = false);

	static bool Exists(const std::string& symbol_name);
	static void Load(typed_lgr<FuncEnvironment> fn, const std::string& symbol_name) ;
	static void Load(const std::vector<uint8_t>& func_templ, const std::string& symbol_name);
	static void Unload(const std::string& func_name);
	static void ForceUnload(const std::string& func_name);
	void ForceUnload() {
		force_unload = true;
	}



	static ValueItem* async_call(typed_lgr<FuncEnvironment> f, ValueItem* args, uint32_t arguments_size);
	static ValueItem* sync_call(typed_lgr<FuncEnvironment> f, ValueItem* args, uint32_t arguments_size) {
		return f->syncWrapper(args, arguments_size);
	}

	bool canBeUnloaded() {
		return can_be_unloaded;
	}
	bool isCheap() {
		return is_cheap;
	}
	FuncType type() {
		return _type;
	}
	const DynamicCall::FunctionTemplate& function_template() {
		return nat_templ;
	}
	void* get_func_ptr() {
		return (void*)curr_func;
	}
	std::string to_string();
	const std::vector<uint8_t>& get_cross_code();
};
void NativeProxy_DynamicToStatic_addValue(DynamicCall::FunctionCall& call, ValueMeta meta, void*& arg);
ValueItem* NativeProxy_DynamicToStatic(DynamicCall::FunctionCall& call, DynamicCall::FunctionTemplate& nat_templ, ValueItem* arguments, uint32_t arguments_size);
extern "C" void callFunction(const char* symbol_name, bool run_async);