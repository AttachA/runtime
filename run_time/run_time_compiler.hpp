// Copyright Danyil Melnytskyi 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#pragma once
#include "FuncEnviropment.hpp"

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
	void set_constant(uint16_t val, const ValueItem& cv, bool is_dynamic = true);
	void set_stack_any_array(uint16_t val, uint32_t len);
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

	void as(uint16_t val, ValueMeta meta);
	void is(uint16_t val, ValueMeta meta);


	void store_bool(uint16_t val);
	void load_bool(uint16_t val);

	void inline_native_opcode(uint8_t* opcode, uint32_t size);

	uint64_t bind_pos();
#pragma region arr_op
	///<summary>set array_type to VType::noting if you need dynamic mode or array/interface type to enable static mode</summary>
	void arr_set(uint16_t arr, uint16_t from, uint64_t to, bool move = true, ArrCheckMode check_bounds = ArrCheckMode::no_check, VType array_type = VType::noting);
	///<summary>set array_type to VType::noting if you need dynamic mode or array/interface type to enable static mode</summary>
	void arr_setByVal(uint16_t arr, uint16_t from, uint16_t to, bool move = true, ArrCheckMode check_bounds = ArrCheckMode::no_check, VType array_type = VType::noting);

	void arr_insert(uint16_t arr, uint16_t from, uint64_t to, bool move = true, bool static_mode = false);
	void arr_insertByVal(uint16_t arr, uint16_t from, uint16_t to, bool move = true, bool static_mode = false);

	void arr_push_end(uint16_t arr, uint16_t from, bool move = true, bool static_mode = false);
	void arr_push_start(uint16_t arr, uint16_t from, bool move = true, bool static_mode = false);

	void arr_insert_range(uint16_t arr, uint16_t arr2, uint64_t arr2_start, uint64_t arr2_end, uint64_t arr_pos, bool move = true, bool static_mode = false);
	void arr_insert_rangeByVal(uint16_t arr, uint16_t arr2, uint16_t arr2_start, uint16_t arr2_end, uint16_t arr_pos, bool move = true, bool static_mode = false);

	///<summary>set array_type to VType::noting if you need dynamic mode or array/interface type to enable static mode</summary>
	void arr_get(uint16_t arr, uint16_t to, uint64_t from, bool move = true, ArrCheckMode check_bounds = ArrCheckMode::no_check, VType array_type = VType::noting);
	///<summary>set array_type to VType::noting if you need dynamic mode or array/interface type to enable static mode</summary>
	void arr_getByVal(uint16_t arr, uint16_t to, uint16_t from, bool move = true, ArrCheckMode check_bounds = ArrCheckMode::no_check, VType array_type = VType::noting);

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
	void call_value_interface(ClassAccess access, uint16_t class_val, uint16_t fn_name, bool catch_ex = false, bool is_async = false);
	void call_value_interface(ClassAccess access, uint16_t class_val, uint16_t fn_name, uint16_t res_val, bool catch_ex = false, bool is_async = false);
	void call_value_interface(ClassAccess access, uint16_t class_val, const std::string& fn_name, bool catch_ex = false, bool is_async = false);
	void call_value_interface(ClassAccess access, uint16_t class_val, const std::string& fn_name, uint16_t res_val, bool catch_ex = false, bool is_async = false);


	void call_value_interface_and_ret(ClassAccess access, uint16_t class_val, uint16_t fn_name, bool catch_ex = false, bool is_async = false);
	void call_value_interface_and_ret(ClassAccess access, uint16_t class_val, const std::string& fn_name, bool catch_ex = false, bool is_async = false);

	void get_interface_value(ClassAccess access, uint16_t class_val, uint16_t val_name, uint16_t res);
	void get_interface_value(ClassAccess access, uint16_t class_val, const std::string& val_name, uint16_t res);

	void set_interface_value(ClassAccess access, uint16_t class_val, uint16_t val_name, uint16_t set_val);
	void set_interface_value(ClassAccess access, uint16_t class_val, const std::string& val_name, uint16_t set_val);

	void explicit_await(uint16_t await_value);

	typed_lgr<FuncEnviropment> prepareFunc(bool can_be_unloaded = true);
	void loadFunc(const std::string& symbol_name, bool can_be_unloaded = true);
};

