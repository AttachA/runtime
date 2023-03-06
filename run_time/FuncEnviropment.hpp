// Copyright Danyil Melnytskyi 2022
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
class FuncEnviropment {
public:
	enum class FuncType : uint32_t {
		own,
		native,
		python,
		csharp,
		java,
		______,
		_______,
		force_unloaded
	};
private:
	TaskMutex compile_lock;
	std::unordered_map<list_array<ValueItem>, ValueItem> cache_map;
	DynamicCall::FunctionTemplate nat_templ;
	std::vector<typed_lgr<FuncEnviropment>> used_envs;
	std::vector<typed_lgr<FuncEnviropment>> local_funcs;
	std::vector<uint8_t> cross_code;
	list_array<ValueItem> values;
	Enviropment curr_func = nullptr;
	uint8_t* frame = nullptr;
	uint32_t max_values : 16;
	FuncType _type : 3;
	uint32_t need_compile : 1 = true;
	uint32_t in_debug : 1 = false;
	uint32_t can_be_unloaded : 1 = true;
	uint32_t use_cache : 1 = false;
	uint32_t force_unload : 1 = false;
	void RuntimeCompile();
	void funcComp() {
		std::lock_guard lguard(compile_lock);
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
	FuncEnviropment(const std::vector<uint8_t>& code, uint16_t values_count, bool _can_be_unloaded) {
		cross_code = code;
		max_values = values_count;
		can_be_unloaded = _can_be_unloaded;
		_type = FuncType::own;
	}
	FuncEnviropment(const std::vector<uint8_t>& code, const std::vector<typed_lgr<FuncEnviropment>>& local_fns, uint16_t values_count, bool _can_be_unloaded) {
		local_funcs = local_fns;
		cross_code = code;
		max_values = values_count;
		can_be_unloaded = _can_be_unloaded;
		_type = FuncType::own;
	}
	FuncEnviropment(DynamicCall::PROC proc, const DynamicCall::FunctionTemplate& templ, bool _can_be_unloaded) {
		nat_templ = templ;
		_type = FuncType::native;
		curr_func = (Enviropment)proc;
		need_compile = false;
		can_be_unloaded = _can_be_unloaded;
	}
	FuncEnviropment(Enviropment proc, bool _can_be_unloaded) {
		_type = FuncType::own;
		curr_func = (Enviropment)proc;
		need_compile = false;
		can_be_unloaded = _can_be_unloaded;
	}

	ValueItem* NativeProxy_DynamicToStatic(ValueItem*, uint32_t arguments_size);
	FuncEnviropment() {
		need_compile = false;
		_type = FuncType::own;
	}
	FuncEnviropment(FuncEnviropment&& move) noexcept {
		operator=(std::move(move));
	}
	~FuncEnviropment();
	FuncEnviropment& operator=(FuncEnviropment&& move) noexcept {
		used_envs = std::move(move.used_envs);
		cross_code = std::move(move.cross_code);
		nat_templ = std::move(move.nat_templ);
		curr_func = move.curr_func;
		frame = move.frame;
		max_values = move.max_values;
		_type = move._type;
		need_compile = move.need_compile;
		in_debug = move.in_debug;
		can_be_unloaded = move.can_be_unloaded;
		//disable destructor
		move.can_be_unloaded = false;
		return *this;
	}

	typed_lgr<FuncEnviropment> localFn(size_t indx) {
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
	static void fastHotPath(const std::string& func_name, typed_lgr<FuncEnviropment>& new_enviro);
	static typed_lgr<FuncEnviropment> enviropment(const std::string& func_name);
	static ValueItem* callFunc(const std::string& func_name, ValueItem* arguments, uint32_t arguments_size, bool run_async);

#pragma region c++ add native
#include<tuple>
	template<size_t i, class T, class... Args>
	struct ArgumentsBuild {
		ArgumentsBuild(DynamicCall::FunctionTemplate& templ) : t(templ) {
			templ.arguments.push_front(DynamicCall::FunctionTemplate::ValueT::getFromType<T>());
		}
		ArgumentsBuild<i - 1, Args...> t;
	};
	template<class T, class... Args>
	struct ArgumentsBuild<1, T, Args...> {
		ArgumentsBuild(DynamicCall::FunctionTemplate& templ) {
			templ.arguments.push_front(DynamicCall::FunctionTemplate::ValueT::getFromType<T>());
		}
	};

	template<class Ret, class... Args>
	struct StartBuild {
		ArgumentsBuild<sizeof...(Args), Args...> res;
		StartBuild(Ret(*function)(Args...), DynamicCall::FunctionTemplate& templ) : res(templ) {}
	};
	template<class Ret>
	static void AddNative(Ret(*function)(), const std::string& symbol_name, bool can_be_unloaded = true) {
		DynamicCall::FunctionTemplate templ;
		templ.result = DynamicCall::FunctionTemplate::ValueT::getFromType<Ret>();
		AddNative((DynamicCall::PROC)function, templ, symbol_name, can_be_unloaded);
	}

	template<class Ret, typename... Args>
	static void AddNative(Ret(*function)(Args...), const std::string& symbol_name, bool can_be_unloaded = true) {
		DynamicCall::FunctionTemplate templ;
		templ.result = DynamicCall::FunctionTemplate::ValueT::getFromType<Ret>();
		StartBuild tmp(function, templ);
		AddNative((DynamicCall::PROC)function, templ, symbol_name, can_be_unloaded);
	}

	static void AddNative(Enviropment function, const std::string& symbol_name, bool can_be_unloaded = true);
#pragma endregion
	static void AddNative(DynamicCall::PROC proc, const DynamicCall::FunctionTemplate& templ, const std::string& symbol_name, bool can_be_unloaded = true);

	static bool Exists(const std::string& symbol_name);
	static void Load(typed_lgr<FuncEnviropment> fn, const std::string& symbol_name) ;
	static void Load(const std::vector<uint8_t>& func_templ, const std::string& symbol_name, bool can_be_unloaded = true);
	static void Unload(const std::string& func_name);
	static void ForceUnload(const std::string& func_name);
	void ForceUnload() {
		force_unload = true;
	}



	static ValueItem* async_call(typed_lgr<FuncEnviropment> f, ValueItem* args, uint32_t arguments_size);
	static ValueItem* sync_call(typed_lgr<FuncEnviropment> f, ValueItem* args, uint32_t arguments_size) {
		return f->syncWrapper(args, arguments_size);
	}

	bool canBeUnloaded() {
		return can_be_unloaded;
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
};

extern "C" void callFunction(const char* symbol_name, bool run_async);