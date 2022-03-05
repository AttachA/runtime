#pragma once
#include <unordered_map>
#include <chrono>
#include <thread>
#include <vector>
#include <string>
#include <atomic>
#include <mutex>
#include "../libray/exceptions.hpp"
#include "../libray/list_array.hpp"
#include "attacha_abi.hpp"
#include "dynamic_call.hpp"

class ValueEnvironment {
	std::unordered_map<std::string, ValueEnvironment*> enviropments;
public:
	FuncRes value;
	ValueEnvironment*& joinEnviropment(const std::string& str) {
		return enviropments[str];
	}
	bool hasEnviropment(const std::string& str) {
		return enviropments.contains(str);
	}
	void removeEnviropment(const std::string& str) {
		delete enviropments[str];
		enviropments.erase(str);
	}
};


extern ValueEnvironment enviropments;
extern thread_local ValueEnvironment thread_local_enviropments;
class FuncEnviropment {
public:
	enum class FuncType : uint32_t {
		own,
		native,
		native_own_abi,
		python,
		csharp,
		java
	};
private:
	static std::unordered_map<std::string, typed_lgr<FuncEnviropment>> enviropments;
	std::mutex compile_lock;
	DynamicCall::FunctionTemplate nat_templ;
	std::vector<typed_lgr<FuncEnviropment>> used_envs;
	std::vector<uint8_t> cross_code;
	list_array<std::string> strings;
	std::string name;
	std::atomic_uint64_t current_runners = 0;
	Enviropment curr_func = nullptr;
	uint8_t* frame = nullptr;
	uint32_t max_values : 16;
	FuncType type : 3;
	uint32_t need_compile : 1 = true;
	uint32_t in_debug : 1 = false;
	uint32_t can_be_unloaded : 1 = true;
	void Compile();

	FuncEnviropment(const std::vector<uint8_t>& code, uint16_t values_count,bool cbu) {
		cross_code = code;
		max_values = values_count;
		can_be_unloaded = cbu;
		type = FuncType::own;
	}
	FuncEnviropment(DynamicCall::PROC proc, const DynamicCall::FunctionTemplate& templ,bool cbu) {
		nat_templ = templ;
		type = FuncType::native;
		curr_func = (Enviropment)proc;
		need_compile = false;
		can_be_unloaded = cbu;
	}
	FuncEnviropment(AttachACXX proc, bool cbu) {
		type = FuncType::native_own_abi;
		curr_func = (Enviropment)proc;
		need_compile = false;
		can_be_unloaded = cbu;
	}
	void funcComp() {
		std::lock_guard lguard(compile_lock);
		if (need_compile) {
			while (current_runners)
				std::this_thread::sleep_for(std::chrono::milliseconds(5));
			Compile();
			need_compile = false;
		}
	}

public:
	FuncRes* NativeProxy_DynamicToStatic(list_array<ArrItem>*);
	FuncRes* initAndCall(list_array<ArrItem>*);
	FuncEnviropment() { 
		need_compile = false;
		type = FuncType::own;
	}
	FuncEnviropment(FuncEnviropment&& move) noexcept {
		operator=(std::move(move));
	}
	~FuncEnviropment();
	FuncEnviropment& operator=(FuncEnviropment&& move) noexcept {
		used_envs = std::move(move.used_envs);
		cross_code = std::move(move.cross_code);
		name = std::move(move.name);
		nat_templ = std::move(move.nat_templ);
		current_runners = move.current_runners.load();
		curr_func = move.curr_func;
		frame = move.frame;
		max_values = move.max_values;
		type = move.type;
		need_compile = move.need_compile;
		in_debug = move.in_debug;
		can_be_unloaded = move.can_be_unloaded;
		//disable destructor
		move.can_be_unloaded = false;
		return *this;
	}
	FuncRes* syncWrapper(list_array<ArrItem>* arguments);
	
	 
	void hotPath(const std::vector<uint8_t>& new_cross_code) {
		if (type != FuncType::own)
			throw HotPathException("Path fail cause this symbol is not own");
		if(!can_be_unloaded)
			throw HotPathException("Path fail cause this symbol is cannon't be unloaded for path");
		std::lock_guard lguard(compile_lock);
		cross_code = new_cross_code;
		need_compile = true;
	}
	static typed_lgr<FuncEnviropment> enviropment(const std::string& func_name) {
		return enviropments[func_name];
	}
	static FuncRes* CallFunc(const std::string& func_name, list_array<ArrItem>* arguments, bool run_async) {
		if (enviropments.contains(func_name)) {
			if (run_async)
				return asyncCall(enviropments[func_name], arguments);
			else
				return enviropments[func_name]->syncWrapper(arguments);
		}
		throw NotImplementedException();
	}

#include<tuple>
	template<size_t i, class T, class... Args>
	struct ArgumentsBuild {
		ArgumentsBuild(DynamicCall::FunctionTemplate& templ) : t(templ) {
			templ.arguments.push_front(DynamicCall::FunctionTemplate::ValueT::getFromType<T>());
		}
		ArgumentsBuild<i - 1, Args...> t;
	};
	template<class T, class... Args>
	struct ArgumentsBuild<1,T, Args...>{
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

	static void AddNative(AttachACXX function, const std::string& symbol_name, bool can_be_unloaded = true) {
		if (enviropments.contains(symbol_name))
			throw SymbolException("Fail alocate symbol: \"" + symbol_name + "\" cause them already exists");
		enviropments[symbol_name] = typed_lgr(new FuncEnviropment(function, can_be_unloaded));
	}


	static void AddNative(DynamicCall::PROC proc, const DynamicCall::FunctionTemplate& templ, const std::string& symbol_name, bool can_be_unloaded = true) {
		if (enviropments.contains(symbol_name))
			throw SymbolException("Fail alocate symbol: \"" + symbol_name + "\" cause them already exists");
		enviropments[symbol_name] = typed_lgr(new FuncEnviropment( proc,templ, can_be_unloaded));
	}

	static bool Exists(const std::string& symbol_name) {
		return enviropments.contains(symbol_name);
	}
	static void Load(const std::vector<uint8_t>& func_templ, const std::string& symbol_name, bool can_be_unloaded = true) {
		if (enviropments.contains(symbol_name))
			throw SymbolException("Fail alocate symbol: \"" + symbol_name + "\" cause them already exists");
		if(func_templ.size() < 2)
			throw SymbolException("Fail alocate symbol: \"" + symbol_name + "\" cause them emplty");
		uint16_t max_vals = func_templ[1];
		max_vals <<= 8;
		max_vals |= func_templ[0];
		enviropments[symbol_name] = typed_lgr(new FuncEnviropment{ { func_templ.begin() + 2, func_templ.end()}, max_vals, can_be_unloaded });
	}
	static void Unload(const std::string& symbol_name) {
		if (enviropments.contains(symbol_name))
			throw SymbolException("Fail release symbol: \"" + symbol_name + "\" cause them not exists");
		enviropments.erase(symbol_name);
	}

	const char* getString(size_t ind) {
		return strings[ind].c_str();
	}

	bool canBeUnloaded() {
		return can_be_unloaded;
	}
	FuncType Type() {
		return type;
	}
	const DynamicCall::FunctionTemplate& templateCall() {
		return nat_templ;
	}
	void* get_func_ptr() {
		return (void*)curr_func;
	}



	static FuncRes* asyncCall(typed_lgr<FuncEnviropment> f, list_array<ArrItem>* args);
	static FuncRes* syncCall(typed_lgr<FuncEnviropment> f, list_array<ArrItem>* args) {
		return f->syncWrapper(args);
	}
};



extern "C" void callFunction(const char* symbol_name, bool run_async);
extern "C" void initStandardFunctions();



