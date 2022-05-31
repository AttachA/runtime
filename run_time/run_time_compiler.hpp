// Copyright Danyil Melnytskyi 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once
#include <unordered_map>
#include <chrono>
#include <thread>
#include <vector>
#include <string>
#include <atomic>
#include <mutex>
#include "library/exceptions.hpp"
#include "../library/list_array.hpp"
#include "attacha_abi.hpp"
#include "dynamic_call.hpp"
#include "tasks.hpp"

class ValueEnvironment {
	std::unordered_map<std::string, ValueEnvironment*> enviropments;
public:
	ValueItem value;
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
	TaskMutex compile_lock;
	std::unordered_map<list_array<ValueItem>, ValueItem> cache_map;
	DynamicCall::FunctionTemplate nat_templ;
	std::vector<typed_lgr<FuncEnviropment>> used_envs;
	std::vector<typed_lgr<FuncEnviropment>> local_funcs;
	std::vector<uint8_t> cross_code;
	list_array<ValueItem> values;
	Enviropment curr_func = nullptr;
	uint8_t* frame = nullptr;
	std::atomic_size_t current_runners = 0;
	uint32_t max_values : 16;
	FuncType type : 3;
	uint32_t need_compile : 1 = true;
	uint32_t in_debug : 1 = false;
	uint32_t can_be_unloaded : 1 = true;
	uint32_t use_cache : 1 = false;
	void Compile();
	void funcComp() {
		std::lock_guard lguard(compile_lock);
		if (need_compile) {
			while (current_runners)
				Task::sleep(5);
			Compile();
			need_compile = false;
		}
	}

public:
	void preCompile() {
		if(need_compile)
			funcComp();
	}
	FuncEnviropment(const std::vector<uint8_t>& code, uint16_t values_count, bool _can_be_unloaded) {
		cross_code = code;
		max_values = values_count;
		can_be_unloaded = _can_be_unloaded;
		type = FuncType::own;
	}
	FuncEnviropment(const std::vector<uint8_t>& code,const std::vector<typed_lgr<FuncEnviropment>>& local_fns, uint16_t values_count, bool _can_be_unloaded) {
		local_funcs = local_fns;
		cross_code = code;
		max_values = values_count;
		can_be_unloaded = _can_be_unloaded;
		type = FuncType::own;
	}
	FuncEnviropment(DynamicCall::PROC proc, const DynamicCall::FunctionTemplate& templ, bool _can_be_unloaded) {
		nat_templ = templ;
		type = FuncType::native;
		curr_func = (Enviropment)proc;
		need_compile = false;
		can_be_unloaded = _can_be_unloaded;
	}
	FuncEnviropment(AttachACXX proc, bool _can_be_unloaded) {
		type = FuncType::native_own_abi;
		curr_func = (Enviropment)proc;
		need_compile = false;
		can_be_unloaded = _can_be_unloaded;
	}

	ValueItem* NativeProxy_DynamicToStatic(list_array<ValueItem>*);
	ValueItem* initAndCall(list_array<ValueItem>*);

	ValueItem* native_proxy_catch(list_array<ValueItem>*);
	ValueItem* initAndCall_catch(list_array<ValueItem>*);
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
	
	typed_lgr<FuncEnviropment> localFn(size_t indx) {
		return local_funcs[indx];
	}
	size_t localFnSize() {
		return local_funcs.size();
	}
	ValueItem* localWrapper(size_t indx, list_array<ValueItem>* arguments, bool run_async) {
		if (local_funcs.size() <= indx)
			throw NotImplementedException();
		if (run_async)
			return asyncCall(local_funcs[indx], arguments);
		else
			return local_funcs[indx]->syncWrapper(arguments);
	}
	ValueItem* localWrapper_catch(size_t indx, list_array<ValueItem>* arguments, bool run_async) {
		try {
			return localWrapper(indx, arguments, run_async);
		}
		catch (...) {
			return new ValueItem(new std::exception_ptr(std::current_exception()), ValueMeta(VType::except_value, false, true), true);
		}
	}

	ValueItem* syncWrapper(list_array<ValueItem>* arguments);
	ValueItem* syncWrapper_catch(list_array<ValueItem>* arguments);

	static void fastHotPath(const std::string& func_name,const std::vector<uint8_t>& new_cross_code) {
		if (!enviropments.contains(func_name)) 
			Load(new_cross_code, func_name, true);
		else {
			auto& tmp = enviropments[func_name];
			if(!tmp->can_be_unloaded)
				throw HotPathException("Path fail cause this symbol is cannon't be unloaded for path");

			uint16_t max_vals = new_cross_code[1];
			max_vals <<= 8;
			max_vals |= new_cross_code[0];
			tmp = typed_lgr(new FuncEnviropment{ { new_cross_code.begin() + 2, new_cross_code.end()}, max_vals, true });
		}
	}
	static typed_lgr<FuncEnviropment> enviropment(const std::string& func_name) {
		return enviropments[func_name];
	}
	static ValueItem* CallFunc(const std::string& func_name, list_array<ValueItem>* arguments, bool run_async) {
		if (enviropments.contains(func_name)) {
			if (run_async)
				return asyncCall(enviropments[func_name], arguments);
			else
				return enviropments[func_name]->syncWrapper(arguments);
		}
		throw NotImplementedException();
	}
	static ValueItem* CallFunc_catch(const std::string& func_name, list_array<ValueItem>* arguments, bool run_async) {
		try {
			return CallFunc(func_name, arguments,run_async);
		}
		catch (...) {
			return new ValueItem(new std::exception_ptr(std::current_exception()), ValueMeta(VType::except_value, false, true), true);
		}
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
	static void Load(typed_lgr<FuncEnviropment> fn, const std::string& symbol_name) {
		if (enviropments.contains(symbol_name))
			throw SymbolException("Fail load symbol: \"" + symbol_name + "\" cause them already exists");
		enviropments[symbol_name] = fn;
	}
	static void Load(const std::vector<uint8_t>& func_templ, const std::string& symbol_name, bool can_be_unloaded = true) {
		if (enviropments.contains(symbol_name))
			throw SymbolException("Fail load symbol: \"" + symbol_name + "\" cause them already exists");
		if(func_templ.size() < 2)
			throw SymbolException("Fail load symbol: \"" + symbol_name + "\" cause them emplty");
		uint16_t max_vals = func_templ[1];
		max_vals <<= 8;
		max_vals |= func_templ[0];
		enviropments[symbol_name] = typed_lgr(new FuncEnviropment{ { func_templ.begin() + 2, func_templ.end()}, max_vals, can_be_unloaded });
	}
	static void Unload(const std::string& symbol_name) {
		auto beg = enviropments.begin();
		auto end = enviropments.begin();
		while (beg != end) {
			if (beg->first == symbol_name) {
				if (beg->second->can_be_unloaded) {
					enviropments.erase(beg);
					return;
				}
				else
					throw SymbolException("Fail unload symbol: \"" + symbol_name + "\" cause them cannont be unloaded");
			}
		}
		throw SymbolException("Fail unload symbol: \"" + symbol_name + "\" cause them not exists");
	}




	static ValueItem* asyncCall(typed_lgr<FuncEnviropment> f, list_array<ValueItem>* args);
	static ValueItem* syncCall(typed_lgr<FuncEnviropment> f, list_array<ValueItem>* args) {
		return f->syncWrapper(args);
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
};

class FuncEviroBuilder {
	std::vector<uint8_t> code;
	std::vector<uint64_t> jump_pos;
	std::vector<typed_lgr<FuncEnviropment>> local_funs;
	uint64_t cop = 0;
	uint16_t max_values = 0;
	void useVal(uint16_t val) {
		if (max_values < val + 1)
			max_values = val + 1;
	}
public:
	void setConstant(uint16_t val, const ValueItem& cv, bool is_dynamic = true);
	void remove(uint16_t val,ValueMeta m);
	void remove(uint16_t val);
	void sum(uint16_t val0, uint16_t val1);
	void sum(uint16_t val0, uint16_t val1, ValueMeta m0, ValueMeta m1);

	void minus(uint16_t val0, uint16_t val1);
	void minus(uint16_t val0, uint16_t val1, ValueMeta m0, ValueMeta m1);

	void div(uint16_t val0, uint16_t val1);
	void div(uint16_t val0, uint16_t val1, ValueMeta m0, ValueMeta m1);

	void mul(uint16_t val0, uint16_t val1);
	void mul(uint16_t val0, uint16_t val1, ValueMeta m0, ValueMeta m1);

	void rest(uint16_t val0, uint16_t val1);
	void rest(uint16_t val0, uint16_t val1, ValueMeta m0, ValueMeta m1);

	void bit_xor(uint16_t val0, uint16_t val1);
	void bit_xor(uint16_t val0, uint16_t val1, ValueMeta m0, ValueMeta m1);

	void bit_or(uint16_t val0, uint16_t val1);
	void bit_or(uint16_t val0, uint16_t val1, ValueMeta m0, ValueMeta m1);

	void bit_and(uint16_t val0, uint16_t val1);
	void bit_and(uint16_t val0, uint16_t val1, ValueMeta m0, ValueMeta m1);

	void bit_not(uint16_t val0);
	void bit_not(uint16_t val, ValueMeta m);

	void log_not();

	void compare(uint16_t val0, uint16_t val1);
	void compare(uint16_t val0, uint16_t val1, ValueMeta m0, ValueMeta m1);

	void jump(JumpCondition cd, uint64_t pos);

	void arg_set(uint16_t val0);

	void call(const std::string& fn_name, bool is_async = false);
	void call(const std::string& fn_name,uint16_t res, bool catch_ex = false, bool is_async = false);

	void call(uint16_t fn_mem, bool is_async = false, bool fn_mem_only_str = false);
	void call(uint16_t fn_mem, uint16_t res, bool catch_ex = false, bool is_async = false, bool fn_mem_only_str = false);


	void call_self(bool is_async = false);
	void call_self(uint16_t res, bool catch_ex = false, bool is_async = false);

	uint32_t addLocalFn(typed_lgr<FuncEnviropment> fn);
	void call_local(typed_lgr<FuncEnviropment> fn, bool is_async = false);
	void call_local(typed_lgr<FuncEnviropment> fn, uint16_t res, bool catch_ex = false, bool is_async = false);

	void call_local_in_mem(uint16_t in_mem_fn, bool is_async = false);
	void call_local_in_mem(uint16_t in_mem_fn, uint16_t res, bool catch_ex = false, bool is_async = false);
	void call_local_idx(uint32_t fn, bool is_async = false);
	void call_local_idx(uint32_t fn, uint16_t res, bool catch_ex = false, bool is_async = false);
	
	void call_and_ret(const std::string& fn_name, bool catch_ex = false, bool is_async = false);
	void call_and_ret(uint16_t fn_mem, bool catch_ex = false, bool is_async = false, bool fn_mem_only_str = false);


	void call_self_and_ret(bool catch_ex = false, bool is_async = false);


	void call_local_and_ret(typed_lgr<FuncEnviropment> fn, bool catch_ex = false, bool is_async = false);
	void call_local_and_ret_in_mem(uint16_t in_mem_fn, bool catch_ex = false, bool is_async = false);
	void call_local_and_ret_idx(uint32_t fn, bool catch_ex = false, bool is_async = false);
	void ret(uint16_t val);
	void ret();
	void copy(uint16_t to, uint16_t from);
	void move(uint16_t to, uint16_t from);

	void debug_break();
	void force_debug_reak();

	void throwEx(const std::string& name, const std::string& desck);
	void throwEx(uint16_t name, uint16_t desck,bool values_is_only_string = false);


	//void inlineNative(const uint8_t* raw, size_t len);
	//void inlineNative(const int8_t* raw, size_t len);
	//void inlineNative(const char* raw, size_t len);


	//void inline_call(const std::string& fn_name);
	//void inline_call(const std::string& fn_name, uint16_t res, bool catch_ex = false);
	//
	//void inline_call_and_ret(const std::string& fn_name,bool catch_ex = false);

	void as(uint16_t val, ValueMeta meta);
	void is(uint16_t val, ValueMeta meta);


	void store_bool(uint16_t val);
	void load_bool(uint16_t val);



	uint64_t bind_pos();
#pragma region arr_op
	void arr_set(uint16_t arr, uint16_t from, uint64_t to, bool move = true, OpArrFlags::CheckMode check_bounds = OpArrFlags::CheckMode::no_check, bool static_mode = false);
	void arr_setByVal(uint16_t arr, uint16_t from, uint16_t to, bool move = true, OpArrFlags::CheckMode check_bounds = OpArrFlags::CheckMode::no_check, bool static_mode = false);

	void arr_insert(uint16_t arr, uint16_t from, uint64_t to, bool move = true, bool static_mode = false);
	void arr_insertByVal(uint16_t arr, uint16_t from, uint16_t to, bool move = true, bool static_mode = false);

	void arr_push_end(uint16_t arr, uint16_t from, bool move = true, bool static_mode = false);
	void arr_push_start(uint16_t arr, uint16_t from, bool move = true, bool static_mode = false);

	void arr_insert_range(uint16_t arr, uint16_t arr2, uint64_t arr2_start, uint64_t arr2_end, uint64_t arr_pos, bool move = true, bool static_mode = false);
	void arr_insert_rangeByVal(uint16_t arr, uint16_t arr2, uint16_t arr2_start, uint16_t arr2_end, uint16_t arr_pos, bool move = true, bool static_mode = false);


	void arr_get(uint16_t arr, uint16_t to, uint64_t from, bool move = true, OpArrFlags::CheckMode check_bounds = OpArrFlags::CheckMode::no_check, bool static_mode = false);
	void arr_getByVal(uint16_t arr, uint16_t to, uint16_t from, bool move = true, OpArrFlags::CheckMode check_bounds = OpArrFlags::CheckMode::no_check, bool static_mode = false);

	void arr_take(uint16_t arr, uint16_t to, uint64_t from, bool move = true, bool static_mode = false);
	void arr_takeByVal(uint16_t arr, uint16_t to, uint16_t from, bool move = true, bool static_mode = false);

	void arr_take_end(uint16_t arr, uint16_t to, bool move = true, bool static_mode = false);
	void arr_take_start(uint16_t arr, uint16_t to, bool move = true, bool static_mode = false);

	void arr_get_range(uint16_t arr, uint16_t to, uint64_t start, uint64_t end, bool move = true, bool static_mode = false);
	void arr_get_rangeByVal(uint16_t arr, uint16_t to, uint16_t start, uint16_t end, bool move = true, bool static_mode = false);

	void arr_take_range(uint16_t arr, uint16_t to, uint64_t start, uint64_t end, bool move = true, bool static_mode = false);
	void arr_take_rangeByVal(uint16_t arr, uint16_t to, uint16_t start, uint16_t end, bool move = true, bool static_mode = false);


	void arr_pop_end(uint16_t arr, bool static_mode = false);
	void arr_pop_start(uint16_t arr, bool static_mode = false);

	void arr_remove_item(uint16_t arr, uint64_t in, bool static_mode = false);
	void arr_remove_itemByVal(uint16_t arr, uint16_t in, bool static_mode = false);

	void arr_remove_range(uint16_t arr, uint64_t start, uint64_t end, bool static_mode = false);
	void arr_remove_rangeByVal(uint16_t arr, uint16_t start, uint16_t end, bool static_mode = false);

	void arr_resize(uint16_t arr, uint64_t new_size, bool static_mode = false);
	void arr_resizeByVal(uint16_t arr, uint16_t new_size, bool static_mode = false);

	void arr_resize_default(uint16_t arr, uint64_t new_size, uint16_t default_init_val, bool static_mode = false);
	void arr_resize_defaultByVal(uint16_t arr, uint16_t new_size, uint16_t default_init_val, bool static_mode = false);



	void arr_reserve_push_end(uint16_t arr, uint64_t new_size, bool static_mode = false);
	void arr_reserve_push_endByVal(uint16_t arr, uint16_t new_size, bool static_mode = false);

	void arr_reserve_push_start(uint16_t arr, uint64_t new_size, bool static_mode = false);
	void arr_reserve_push_startByVal(uint16_t arr, uint16_t new_size, bool static_mode = false);

	void arr_commit(uint16_t arr, bool static_mode = false);

	void arr_decommit(uint16_t arr, uint64_t blocks_count, bool static_mode = false);
	void arr_decommitByVal(uint16_t arr, uint16_t blocks_count, bool static_mode = false);

	void arr_remove_reserved(uint16_t arr, bool static_mode = false);

	void arr_size(uint16_t arr, uint16_t set_to, bool static_mode = false);
#pragma endregion
	//casm,




	typed_lgr<FuncEnviropment> prepareFunc(bool can_be_unloaded = true);
	void loadFunc(const std::string& symbol_name, bool can_be_unloaded = true);
};


extern "C" void callFunction(const char* symbol_name, bool run_async);
extern "C" void initStandardFunctions();

