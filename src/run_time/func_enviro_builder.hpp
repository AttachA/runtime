// Copyright Danyil Melnytskyi 2022-2023
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#pragma once
#ifndef RUN_TIME_RUN_TIME_COMPILER
#define RUN_TIME_RUN_TIME_COMPILER
#include "asm/FuncEnvironment.hpp"

namespace art{
	struct FuncEviroBuilder_line_info {
		uint64_t begin;
		uint64_t line;
		uint64_t column;
	};

	class FuncEviroBuilder {
		std::vector<uint8_t> code;
		std::vector<uint64_t> jump_pos;
		std::unordered_map<std::string, size_t> jump_pos_map;
		list_array<ValueItem> dynamic_values;
		std::forward_list<ValueItem> all_constants;
		std::vector<typed_lgr<FuncEnvironment>> local_funs;
		uint64_t cop = 0;
		uint16_t values = 0;
		uint16_t static_values = 0;
		uint32_t args_values = 0;
		uint64_t constants_values = 0;
		FunctionMetaFlags flags{0};
		bool use_dynamic_values = false;
		bool strict_mode = true;
		bool unify_constants = true;
		
		void useVal(ValueIndexPos val) {
			switch (val.pos){
				case ValuePos::in_enviro:
					if (values < val.index)
						values = val.index;
					flags.used_enviro_vals = true;
					break;
				case ValuePos::in_arguments:
					if (args_values < val.index)
						args_values = val.index;
					flags.used_arguments = true;
					break;
				case ValuePos::in_static:
					if (static_values < val.index)
						static_values = val.index;
					flags.used_static = true;
					break;
			}
		}
		size_t jumpMap(const std::string& name) {
			auto it = jump_pos_map.find(name);
			if (it == jump_pos_map.end()) {
				jump_pos_map[name] = jump_pos.size();
				jump_pos.push_back(0);
				return jump_pos.size() - 1;
			}
			return it->second;
		}
	public:
		bool get_unify_constants() const {
			return unify_constants;
		}
		void set_unify_constants(bool unify_constants) {
			this->unify_constants = unify_constants;
		}
		FuncEviroBuilder(){
			flags.can_be_unloaded = true;
		}
		//strict_mode - if true, then all local functions must has cross_code, by default true
		//use_dynamic_values - if true, then constants can be any type, by default false, option ignored if strict_mode is true(then all constants will be stored at opcodes)
		FuncEviroBuilder(bool strict_mode,bool use_dynamic_values = false) : strict_mode(strict_mode), use_dynamic_values(use_dynamic_values) {}
		ValueIndexPos create_constant(const ValueItem& val);
		void set_stack_any_array(ValueIndexPos val, uint32_t len);
		void remove(ValueIndexPos val,ValueMeta m);
		void remove(ValueIndexPos val);
		void sum(ValueIndexPos val0, ValueIndexPos val1);
		void sum(ValueIndexPos val0, ValueIndexPos val1, ValueMeta m0, ValueMeta m1);

		void minus(ValueIndexPos val0, ValueIndexPos val1);
		void minus(ValueIndexPos val0, ValueIndexPos val1, ValueMeta m0, ValueMeta m1);

		void div(ValueIndexPos val0, ValueIndexPos val1);
		void div(ValueIndexPos val0, ValueIndexPos val1, ValueMeta m0, ValueMeta m1);

		void mul(ValueIndexPos val0, ValueIndexPos val1);
		void mul(ValueIndexPos val0, ValueIndexPos val1, ValueMeta m0, ValueMeta m1);

		void rest(ValueIndexPos val0, ValueIndexPos val1);
		void rest(ValueIndexPos val0, ValueIndexPos val1, ValueMeta m0, ValueMeta m1);

		void bit_xor(ValueIndexPos val0, ValueIndexPos val1);
		void bit_xor(ValueIndexPos val0, ValueIndexPos val1, ValueMeta m0, ValueMeta m1);

		void bit_or(ValueIndexPos val0, ValueIndexPos val1);
		void bit_or(ValueIndexPos val0, ValueIndexPos val1, ValueMeta m0, ValueMeta m1);

		void bit_and(ValueIndexPos val0, ValueIndexPos val1);
		void bit_and(ValueIndexPos val0, ValueIndexPos val1, ValueMeta m0, ValueMeta m1);

		void bit_not(ValueIndexPos val0);
		void bit_not(ValueIndexPos val, ValueMeta m);

		void log_not();

		void compare(ValueIndexPos val0, ValueIndexPos val1);
		void compare(ValueIndexPos val0, ValueIndexPos val1, ValueMeta m0, ValueMeta m1);

		void jump(JumpCondition cd, const std::string& label_name);

		void arg_set(ValueIndexPos val0);

		void call(ValueIndexPos fn_mem, bool is_async = false, bool fn_mem_only_str = false);
		void call(ValueIndexPos fn_mem, ValueIndexPos res, bool is_async = false, bool fn_mem_only_str = false);


		void call_self(bool is_async = false);
		void call_self(ValueIndexPos res, bool is_async = false);

		uint32_t add_local_fn(typed_lgr<FuncEnvironment> fn);

		void call_local(ValueIndexPos in_mem_fn, bool is_async = false);
		void call_local(ValueIndexPos in_mem_fn, ValueIndexPos res, bool is_async = false);
		void call_local_idx(uint32_t fn, bool is_async = false);
		void call_local_idx(uint32_t fn, ValueIndexPos res, bool is_async = false);
		
		void call_and_ret(ValueIndexPos fn_mem, bool is_async = false, bool fn_mem_only_str = false);


		void call_self_and_ret(bool is_async = false);


		void call_local_and_ret(ValueIndexPos in_mem_fn, bool is_async = false);
		void call_local_and_ret_idx(uint32_t fn, bool is_async = false);
		void ret(ValueIndexPos val);
		void ret_take(ValueIndexPos val);
		void ret();
		void copy(ValueIndexPos to, ValueIndexPos from);
		void move(ValueIndexPos to, ValueIndexPos from);

		void debug_break();
		void force_debug_reak();

		void throw_ex(ValueIndexPos name, ValueIndexPos desck);

		void as(ValueIndexPos val, VType meta);
		void is(ValueIndexPos val, VType meta);
		void is_gc(ValueIndexPos val);
		void is_gc(ValueIndexPos val, ValueIndexPos result);
		
		void store_bool(ValueIndexPos val);
		void load_bool(ValueIndexPos val);

		void inline_native_opcode(uint8_t* opcode, uint32_t size);

		void bind_pos(const std::string& label_name);
	#pragma region arr_op
		///<summary>set array_type to VType::noting if you need dynamic mode or array/interface type to enable static mode</summary>
		void arr_set(ValueIndexPos arr, ValueIndexPos from, uint64_t to, bool move = true, ArrCheckMode check_bounds = ArrCheckMode::no_check, VType array_type = VType::noting);
		///<summary>set array_type to VType::noting if you need dynamic mode or array/interface type to enable static mode</summary>
		void arr_setByVal(ValueIndexPos arr, ValueIndexPos from, ValueIndexPos to, bool move = true, ArrCheckMode check_bounds = ArrCheckMode::no_check, VType array_type = VType::noting);

		void arr_insert(ValueIndexPos arr, ValueIndexPos from, uint64_t to, bool move = true, bool static_mode = false);
		void arr_insertByVal(ValueIndexPos arr, ValueIndexPos from, ValueIndexPos to, bool move = true, bool static_mode = false);

		void arr_push_end(ValueIndexPos arr, ValueIndexPos from, bool move = true, bool static_mode = false);
		void arr_push_start(ValueIndexPos arr, ValueIndexPos from, bool move = true, bool static_mode = false);

		void arr_insert_range(ValueIndexPos arr, ValueIndexPos arr2, uint64_t arr2_start, uint64_t arr2_end, uint64_t arr_pos, bool move = true, bool static_mode = false);
		void arr_insert_rangeByVal(ValueIndexPos arr, ValueIndexPos arr2, ValueIndexPos arr2_start, ValueIndexPos arr2_end, ValueIndexPos arr_pos, bool move = true, bool static_mode = false);

		///<summary>set array_type to VType::noting if you need dynamic mode or array/interface type to enable static mode</summary>
		void arr_get(ValueIndexPos arr, ValueIndexPos to, uint64_t from, bool move = true, ArrCheckMode check_bounds = ArrCheckMode::no_check, VType array_type = VType::noting);
		///<summary>set array_type to VType::noting if you need dynamic mode or array/interface type to enable static mode</summary>
		void arr_getByVal(ValueIndexPos arr, ValueIndexPos to, ValueIndexPos from, bool move = true, ArrCheckMode check_bounds = ArrCheckMode::no_check, VType array_type = VType::noting);

		void arr_take(ValueIndexPos arr, ValueIndexPos to, uint64_t from, bool move = true, bool static_mode = false);
		void arr_takeByVal(ValueIndexPos arr, ValueIndexPos to, ValueIndexPos from, bool move = true, bool static_mode = false);

		void arr_take_end(ValueIndexPos arr, ValueIndexPos to, bool move = true, bool static_mode = false);
		void arr_take_start(ValueIndexPos arr, ValueIndexPos to, bool move = true, bool static_mode = false);

		void arr_get_range(ValueIndexPos arr, ValueIndexPos to, uint64_t start, uint64_t end, bool move = true, bool static_mode = false);
		void arr_get_rangeByVal(ValueIndexPos arr, ValueIndexPos to, ValueIndexPos start, ValueIndexPos end, bool move = true, bool static_mode = false);

		void arr_take_range(ValueIndexPos arr, ValueIndexPos to, uint64_t start, uint64_t end, bool move = true, bool static_mode = false);
		void arr_take_rangeByVal(ValueIndexPos arr, ValueIndexPos to, ValueIndexPos start, ValueIndexPos end, bool move = true, bool static_mode = false);


		void arr_pop_end(ValueIndexPos arr, bool static_mode = false);
		void arr_pop_start(ValueIndexPos arr, bool static_mode = false);

		void arr_remove_item(ValueIndexPos arr, uint64_t in, bool static_mode = false);
		void arr_remove_itemByVal(ValueIndexPos arr, ValueIndexPos in, bool static_mode = false);

		void arr_remove_range(ValueIndexPos arr, uint64_t start, uint64_t end, bool static_mode = false);
		void arr_remove_rangeByVal(ValueIndexPos arr, ValueIndexPos start, ValueIndexPos end, bool static_mode = false);

		void arr_resize(ValueIndexPos arr, uint64_t new_size, bool static_mode = false);
		void arr_resizeByVal(ValueIndexPos arr, ValueIndexPos new_size, bool static_mode = false);

		void arr_resize_default(ValueIndexPos arr, uint64_t new_size, ValueIndexPos default_init_val, bool static_mode = false);
		void arr_resize_defaultByVal(ValueIndexPos arr, ValueIndexPos new_size, ValueIndexPos default_init_val, bool static_mode = false);



		void arr_reserve_push_end(ValueIndexPos arr, uint64_t new_size, bool static_mode = false);
		void arr_reserve_push_endByVal(ValueIndexPos arr, ValueIndexPos new_size, bool static_mode = false);

		void arr_reserve_push_start(ValueIndexPos arr, uint64_t new_size, bool static_mode = false);
		void arr_reserve_push_startByVal(ValueIndexPos arr, ValueIndexPos new_size, bool static_mode = false);

		void arr_commit(ValueIndexPos arr, bool static_mode = false);

		void arr_decommit(ValueIndexPos arr, uint64_t blocks_count, bool static_mode = false);
		void arr_decommitByVal(ValueIndexPos arr, ValueIndexPos blocks_count, bool static_mode = false);

		void arr_remove_reserved(ValueIndexPos arr, bool static_mode = false);

		void arr_size(ValueIndexPos arr, ValueIndexPos set_to, bool static_mode = false);
	#pragma endregion
		//casm,
		void call_value_interface(ClassAccess access, ValueIndexPos class_val, ValueIndexPos fn_name, bool is_async = false);
		void call_value_interface(ClassAccess access, ValueIndexPos class_val, ValueIndexPos fn_name, ValueIndexPos res_val, bool is_async = false);
		void call_value_interface_id(ValueIndexPos class_val, uint64_t class_fun_id, bool is_async = false);
		void call_value_interface_id(ValueIndexPos class_val, uint64_t class_fun_id, ValueIndexPos res_val, bool is_async = false);


		void call_value_interface_and_ret(ClassAccess access, ValueIndexPos class_val, ValueIndexPos fn_name, bool is_async = false);
		void call_value_interface_id_and_ret(ValueIndexPos class_val, uint64_t class_fun_id, bool is_async = false);


		void static_call_value_interface(ClassAccess access, ValueIndexPos class_val, ValueIndexPos fn_name, bool is_async = false);
		void static_call_value_interface(ClassAccess access, ValueIndexPos class_val, ValueIndexPos fn_name, ValueIndexPos res_val, bool is_async = false);
		void static_call_value_interface_id(ValueIndexPos class_val, uint64_t class_fun_id, bool is_async = false);
		void static_call_value_interface_id(ValueIndexPos class_val, uint64_t class_fun_id, ValueIndexPos res_val, bool is_async = false);


		void static_call_value_interface_and_ret(ClassAccess access, ValueIndexPos class_val, ValueIndexPos fn_name, bool is_async = false);
		void static_call_value_interface_id_and_ret(ValueIndexPos class_val, uint64_t class_fun_id, bool is_async = false);
		
		void get_interface_value(ClassAccess access, ValueIndexPos class_val, ValueIndexPos val_name, ValueIndexPos res);
		void set_interface_value(ClassAccess access, ValueIndexPos class_val, ValueIndexPos val_name, ValueIndexPos set_val);

		void explicit_await(ValueIndexPos await_value);

		
		void to_gc(ValueIndexPos val);
		void localize_gc(ValueIndexPos val);
		void from_gc(ValueIndexPos val);
		void xarray_slice(ValueIndexPos result, ValueIndexPos val);
		void xarray_slice(ValueIndexPos result, ValueIndexPos val, uint32_t from);
		void xarray_slice(ValueIndexPos result, ValueIndexPos val, ValueIndexPos from);
		void xarray_slice(ValueIndexPos result, ValueIndexPos val, bool unused, uint32_t to);
		void xarray_slice(ValueIndexPos result, ValueIndexPos val, bool unused, ValueIndexPos to);
		void xarray_slice(ValueIndexPos result, ValueIndexPos val, uint32_t from, uint32_t to);
		void xarray_slice(ValueIndexPos result, ValueIndexPos val, uint32_t from, ValueIndexPos to);
		void xarray_slice(ValueIndexPos result, ValueIndexPos val, ValueIndexPos from, uint32_t to);
		void xarray_slice(ValueIndexPos result, ValueIndexPos val, ValueIndexPos from, ValueIndexPos to);
		void table_jump(
			std::vector<std::string> table,
			ValueIndexPos index,
			bool is_signed = false,
			TableJumpCheckFailAction too_large = TableJumpCheckFailAction::throw_exception,
			const std::string& too_large_label = "",
			TableJumpCheckFailAction too_small = TableJumpCheckFailAction::throw_exception,
			const std::string& too_small_label = ""
		);
		void get_refrence(ValueIndexPos res, ValueIndexPos val);
		void make_as_const(ValueIndexPos res);
		void remove_const_protect(ValueIndexPos res);
		void copy_un_constant(ValueIndexPos res, ValueIndexPos val);
		void copy_un_refrence(ValueIndexPos res, ValueIndexPos val);
		void move_un_refrence(ValueIndexPos res, ValueIndexPos val);
		void remove_qualifiers(ValueIndexPos res);//same as ungc and remove_const_protect



		

		FuncEviroBuilder& O_flag_can_be_unloaded(bool can_be_unloaded);
		FuncEviroBuilder& O_flag_is_translated(bool is_translated);
		FuncEviroBuilder& O_flag_is_cheap(bool is_cheap);
		FuncEviroBuilder& O_flag_used_vec128(uint8_t index);
		FuncEviroBuilder& O_flag_is_patchable(bool is_patchabele);
		
		FuncEviroBuilder_line_info O_line_info_begin();
		void O_line_info_end(FuncEviroBuilder_line_info line_info);

		typed_lgr<FuncEnvironment> O_prepare_func();
		std::vector<uint8_t> O_build_func();
		void O_load_func(const std::string& symbol_name);
		void O_patch_func(const std::string& symbol_name);
	};
}
#endif /* RUN_TIME_RUN_TIME_COMPILER */
