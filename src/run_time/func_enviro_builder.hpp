// Copyright Danyil Melnytskyi 2022-Present
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#pragma once
#ifndef SRC_RUN_TIME_FUNC_ENVIRO_BUILDER
#define SRC_RUN_TIME_FUNC_ENVIRO_BUILDER
#include <forward_list>

#include <run_time/asm/FuncEnvironment.hpp>

namespace art {
    struct FuncEnviroBuilder_line_info {
        uint64_t begin;
        uint64_t line;
        uint64_t column;
    };

    class FuncEnviroBuilder {
        std::vector<uint8_t> code;
        std::vector<uint64_t> jump_pos;
        std::unordered_map<art::ustring, size_t, art::hash<art::ustring>> jump_pos_map;
        list_array<ValueItem> dynamic_values;
        std::forward_list<ValueItem> all_constants;
        std::vector<art::shared_ptr<FuncEnvironment>> local_funs;
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
            switch (val.pos) {
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
            case ValuePos::in_constants:
                break;
            }
        }

        size_t jumpMap(const art::ustring& name) {
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

        FuncEnviroBuilder() {
            flags.can_be_unloaded = true;
        }

        //strict_mode - if true, then all local functions must has cross_code, by default true
        //use_dynamic_values - if true, then constants can be any type, by default false, option ignored if strict_mode is true(then all constants will be stored at opcodes)
        FuncEnviroBuilder(bool strict_mode, bool use_dynamic_values = false)
            : strict_mode(strict_mode), use_dynamic_values(use_dynamic_values) {}

        ValueIndexPos create_constant(const ValueItem& val);
        void set_stack_any_array(ValueIndexPos val, uint32_t len);
        void remove(ValueIndexPos val, ValueMeta m);
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

        void jump(JumpCondition cd, const art::ustring& label_name);

        void arg_set(ValueIndexPos val0);

        void call(ValueIndexPos fn_mem, bool is_async = false, bool fn_mem_only_str = false);
        void call(ValueIndexPos fn_mem, ValueIndexPos res, bool is_async = false, bool fn_mem_only_str = false);


        void call_self(bool is_async = false);
        void call_self(ValueIndexPos res, bool is_async = false);

        uint64_t add_local_fn(art::shared_ptr<FuncEnvironment> fn);

        void call_local(ValueIndexPos fn, bool is_async = false);
        void call_local(ValueIndexPos fn, ValueIndexPos res, bool is_async = false);

        void call_and_ret(ValueIndexPos fn_mem, bool is_async = false, bool fn_mem_only_str = false);


        void call_self_and_ret(bool is_async = false);


        void call_local_and_ret(ValueIndexPos fn, bool is_async = false);
        void ret(ValueIndexPos val);
        void ret_take(ValueIndexPos val);
        void ret();
        void copy(ValueIndexPos to, ValueIndexPos from);
        void move(ValueIndexPos to, ValueIndexPos from);

        void debug_break();
        void force_debug_break();

        void throw_ex(ValueIndexPos name, ValueIndexPos desc);

        void as(ValueIndexPos val, VType meta);
        void is(ValueIndexPos val, VType meta);
        void is_gc(ValueIndexPos val);
        void is_gc(ValueIndexPos val, ValueIndexPos result);

        void store_bool(ValueIndexPos val);
        void load_bool(ValueIndexPos val);

        void inline_native_opcode(uint8_t* opcode, uint32_t size);

        void bind_pos(const art::ustring& label_name);
#pragma region arr_op

        struct _arr {
            FuncEnviroBuilder& build;
            ValueIndexPos arr;

            _arr(FuncEnviroBuilder& build, ValueIndexPos arr)
                : build(build), arr(arr) {}

            void set(ValueIndexPos index, ValueIndexPos val, bool move = true, ArrCheckMode check_bounds = ArrCheckMode::no_check);
            void insert(ValueIndexPos index, ValueIndexPos val, bool move = true);
            void push_end(ValueIndexPos val, bool move = true);
            void push_start(ValueIndexPos val, bool move = true);
            void insert_range(ValueIndexPos arr2, ValueIndexPos arr2_start, ValueIndexPos arr2_end, ValueIndexPos arr_pos, bool move = true);
            void get(ValueIndexPos to, ValueIndexPos index, bool move = true, ArrCheckMode check_bounds = ArrCheckMode::no_check);
            void take(ValueIndexPos to, ValueIndexPos index, bool move = true);
            void take_end(ValueIndexPos to, bool move = true);
            void take_start(ValueIndexPos to, bool move = true);
            void get_range(ValueIndexPos to, ValueIndexPos start, ValueIndexPos end, bool move = true);
            void take_range(ValueIndexPos to, ValueIndexPos start, ValueIndexPos end, bool move = true);
            void pop_end();
            void pop_start();
            void remove_item(ValueIndexPos in);
            void remove_range(ValueIndexPos start, ValueIndexPos end);
            void resize(ValueIndexPos new_size);
            void resize_default(ValueIndexPos new_size, ValueIndexPos default_init_val);
            void reserve_push_end(ValueIndexPos new_size);
            void reserve_push_start(ValueIndexPos new_size);
            void commit();
            void decommit(ValueIndexPos blocks_count);
            void remove_reserved();
            void size(ValueIndexPos set_to);
        };

        _arr arr(ValueIndexPos arr) {
            return _arr(*this, arr);
        }

        struct _static_arr {
            FuncEnviroBuilder& build;
            ValueIndexPos arr;
            ValueMeta arr_meta;

            _static_arr(FuncEnviroBuilder& build, ValueIndexPos arr, ValueMeta arr_meta)
                : build(build), arr(arr), arr_meta(arr_meta) {}

            void set(ValueIndexPos index, ValueIndexPos val, bool move = true, ArrCheckMode check_bounds = ArrCheckMode::no_check);
            void insert(ValueIndexPos index, ValueIndexPos val, bool move = true);
            void push_end(ValueIndexPos val, bool move = true);
            void push_start(ValueIndexPos val, bool move = true);
            void insert_range(ValueIndexPos arr2, ValueIndexPos arr2_start, ValueIndexPos arr2_end, ValueIndexPos arr_pos, bool move = true);
            void get(ValueIndexPos to, ValueIndexPos index, bool move = true, ArrCheckMode check_bounds = ArrCheckMode::no_check);
            void take(ValueIndexPos to, ValueIndexPos index, bool move = true);
            void take_end(ValueIndexPos to, bool move = true);
            void take_start(ValueIndexPos to, bool move = true);
            void get_range(ValueIndexPos to, ValueIndexPos start, ValueIndexPos end, bool move = true);
            void take_range(ValueIndexPos to, ValueIndexPos start, ValueIndexPos end, bool move = true);
            void pop_end();
            void pop_start();
            void remove_item(ValueIndexPos in);
            void remove_range(ValueIndexPos start, ValueIndexPos end);
            void resize(ValueIndexPos new_size);
            void resize_default(ValueIndexPos new_size, ValueIndexPos default_init_val);
            void reserve_push_end(ValueIndexPos new_size);
            void reserve_push_start(ValueIndexPos new_size);
            void commit();
            void decommit(ValueIndexPos blocks_count);
            void remove_reserved();
            void size(ValueIndexPos set_to);
        };

        _static_arr static_arr(ValueIndexPos arr, ValueMeta arr_meta) {
            return _static_arr(*this, arr, arr_meta);
        }

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
        void xarray_slice(ValueIndexPos result, ValueIndexPos val, ValueIndexPos from);
        void xarray_slice(ValueIndexPos result, ValueIndexPos val, bool unused, ValueIndexPos to);
        void xarray_slice(ValueIndexPos result, ValueIndexPos val, ValueIndexPos from, ValueIndexPos to);
        void table_jump(
            std::vector<art::ustring> table,
            ValueIndexPos index,
            bool is_signed = false,
            TableJumpCheckFailAction too_large = TableJumpCheckFailAction::throw_exception,
            const art::ustring& too_large_label = "",
            TableJumpCheckFailAction too_small = TableJumpCheckFailAction::throw_exception,
            const art::ustring& too_small_label = "");
        void get_reference(ValueIndexPos res, ValueIndexPos val);
        void make_as_const(ValueIndexPos res);
        void remove_const_protect(ValueIndexPos res);
        void copy_un_constant(ValueIndexPos res, ValueIndexPos val);
        void copy_un_reference(ValueIndexPos res, ValueIndexPos val);
        void move_un_reference(ValueIndexPos res, ValueIndexPos val);
        void remove_qualifiers(ValueIndexPos res); //same as ungc and remove_const_protect


        FuncEnviroBuilder& O_flag_can_be_unloaded(bool can_be_unloaded);
        FuncEnviroBuilder& O_flag_is_translated(bool is_translated);
        FuncEnviroBuilder& O_flag_is_cheap(bool is_cheap);
        FuncEnviroBuilder& O_flag_used_vec128(uint8_t index);
        FuncEnviroBuilder& O_flag_is_patchable(bool is_patchable);

        FuncEnviroBuilder_line_info O_line_info_begin();
        void O_line_info_end(FuncEnviroBuilder_line_info line_info);

        art::shared_ptr<FuncEnvironment> O_prepare_func();
        std::vector<uint8_t> O_build_func();
        void O_load_func(const art::ustring& symbol_name);
        void O_patch_func(const art::ustring& symbol_name);
    };
}
#endif /* SRC_RUN_TIME_FUNC_ENVIRO_BUILDER */
