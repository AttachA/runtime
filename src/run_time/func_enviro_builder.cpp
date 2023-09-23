// Copyright Danyil Melnytskyi 2022-Present
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <run_time/attacha_abi.hpp>
#include <run_time/func_enviro_builder.hpp>
#include <run_time/util/tools.hpp>
#include <stdint.h>


using namespace art;
#pragma region FuncEnviroBuilder

ValueIndexPos FuncEnviroBuilder::create_constant(const ValueItem& val) {
    if (unify_constants) {
        uint32_t i = 0;
        for (auto& v : all_constants) {
            if (v == val)
                return ValueIndexPos(i, ValuePos::in_constants);
            i++;
        }
    }
    if (constants_values > UINT16_MAX)
        throw CompileTimeException("Unaddressable constant value");
    all_constants.push_front(val);
    if (use_dynamic_values && !strict_mode) {
        flags.run_time_computable = true;
        dynamic_values.push_back(val);
    } else {
        builder::write(code, Command(Opcode::store_constant));
        builder::writeAny(code, const_cast<ValueItem&>(val));
    }
    return ValueIndexPos(constants_values++, ValuePos::in_constants);
}

#pragma region SetRem

void FuncEnviroBuilder::set_stack_any_array(ValueIndexPos val, uint32_t len) {
    builder::write(code, Command(Opcode::create_saarr, false, false));
    builder::writeIndexPos(code, val);
    builder::write(code, len);
    useVal(val);
}

void FuncEnviroBuilder::remove(ValueIndexPos val, ValueMeta m) {
    builder::write(code, Command(Opcode::remove, false, true));
    builder::write(code, m);
    builder::writeIndexPos(code, val);
    useVal(val);
}

void FuncEnviroBuilder::remove(ValueIndexPos val) {
    builder::write(code, Command(Opcode::remove));
    builder::writeIndexPos(code, val);
    useVal(val);
}

#pragma endregion
#pragma region numeric

void FuncEnviroBuilder::sum(ValueIndexPos val0, ValueIndexPos val1) {
    builder::write(code, Command(Opcode::sum));
    builder::writeIndexPos(code, val0);
    builder::writeIndexPos(code, val1);
    useVal(val0);
    useVal(val1);
}

void FuncEnviroBuilder::sum(ValueIndexPos val0, ValueIndexPos val1, ValueMeta m0, ValueMeta m1) {
    builder::write(code, Command(Opcode::sum, false, true));
    builder::writeIndexPos(code, val0);
    builder::write(code, m0);
    builder::writeIndexPos(code, val1);
    builder::write(code, m1);
    useVal(val0);
    useVal(val1);
}

void FuncEnviroBuilder::minus(ValueIndexPos val0, ValueIndexPos val1) {
    builder::write(code, Command(Opcode::minus));
    builder::writeIndexPos(code, val0);
    builder::writeIndexPos(code, val1);
    useVal(val0);
    useVal(val1);
}

void FuncEnviroBuilder::minus(ValueIndexPos val0, ValueIndexPos val1, ValueMeta m0, ValueMeta m1) {
    builder::write(code, Command(Opcode::minus, false, true));
    builder::writeIndexPos(code, val0);
    builder::write(code, m0);
    builder::writeIndexPos(code, val1);
    builder::write(code, m1);
    useVal(val0);
    useVal(val1);
}

void FuncEnviroBuilder::div(ValueIndexPos val0, ValueIndexPos val1) {
    builder::write(code, Command(Opcode::div));
    builder::writeIndexPos(code, val0);
    builder::writeIndexPos(code, val1);
    useVal(val0);
    useVal(val1);
}

void FuncEnviroBuilder::div(ValueIndexPos val0, ValueIndexPos val1, ValueMeta m0, ValueMeta m1) {
    builder::write(code, Command(Opcode::div, false, true));
    builder::writeIndexPos(code, val0);
    builder::write(code, m0);
    builder::writeIndexPos(code, val1);
    builder::write(code, m1);
    useVal(val0);
    useVal(val1);
}

void FuncEnviroBuilder::mul(ValueIndexPos val0, ValueIndexPos val1) {
    builder::write(code, Command(Opcode::mul));
    builder::writeIndexPos(code, val0);
    builder::writeIndexPos(code, val1);
    useVal(val0);
    useVal(val1);
}

void FuncEnviroBuilder::mul(ValueIndexPos val0, ValueIndexPos val1, ValueMeta m0, ValueMeta m1) {
    builder::write(code, Command(Opcode::mul, false, true));
    builder::writeIndexPos(code, val0);
    builder::write(code, m0);
    builder::writeIndexPos(code, val1);
    builder::write(code, m1);
    useVal(val0);
    useVal(val1);
}

void FuncEnviroBuilder::rest(ValueIndexPos val0, ValueIndexPos val1) {
    builder::write(code, Command(Opcode::rest));
    builder::writeIndexPos(code, val0);
    builder::writeIndexPos(code, val1);
    useVal(val0);
    useVal(val1);
}

void FuncEnviroBuilder::rest(ValueIndexPos val0, ValueIndexPos val1, ValueMeta m0, ValueMeta m1) {
    builder::write(code, Command(Opcode::rest, false, true));
    builder::writeIndexPos(code, val0);
    builder::write(code, m0);
    builder::writeIndexPos(code, val1);
    builder::write(code, m1);
    useVal(val0);
    useVal(val1);
}

#pragma endregion
#pragma region bit

void FuncEnviroBuilder::bit_xor(ValueIndexPos val0, ValueIndexPos val1) {
    builder::write(code, Command(Opcode::bit_xor));
    builder::writeIndexPos(code, val0);
    builder::writeIndexPos(code, val1);
    useVal(val0);
    useVal(val1);
}

void FuncEnviroBuilder::bit_xor(ValueIndexPos val0, ValueIndexPos val1, ValueMeta m0, ValueMeta m1) {
    builder::write(code, Command(Opcode::bit_xor, false, true));
    builder::writeIndexPos(code, val0);
    builder::write(code, m0);
    builder::writeIndexPos(code, val1);
    builder::write(code, m1);
    useVal(val0);
    useVal(val1);
}

void FuncEnviroBuilder::bit_or(ValueIndexPos val0, ValueIndexPos val1) {
    builder::write(code, Command(Opcode::bit_or));
    builder::writeIndexPos(code, val0);
    builder::writeIndexPos(code, val1);
    useVal(val0);
    useVal(val1);
}

void FuncEnviroBuilder::bit_or(ValueIndexPos val0, ValueIndexPos val1, ValueMeta m0, ValueMeta m1) {
    builder::write(code, Command(Opcode::bit_or, false, true));
    builder::writeIndexPos(code, val0);
    builder::write(code, m0);
    builder::writeIndexPos(code, val1);
    builder::write(code, m1);
    useVal(val0);
    useVal(val1);
}

void FuncEnviroBuilder::bit_and(ValueIndexPos val0, ValueIndexPos val1) {
    builder::write(code, Command(Opcode::bit_and));
    builder::writeIndexPos(code, val0);
    builder::writeIndexPos(code, val1);
    useVal(val0);
    useVal(val1);
}

void FuncEnviroBuilder::bit_and(ValueIndexPos val0, ValueIndexPos val1, ValueMeta m0, ValueMeta m1) {
    builder::write(code, Command(Opcode::bit_and, false, true));
    builder::writeIndexPos(code, val0);
    builder::write(code, m0);
    builder::writeIndexPos(code, val1);
    builder::write(code, m1);
    useVal(val0);
    useVal(val1);
}

void FuncEnviroBuilder::bit_not(ValueIndexPos val0) {
    builder::write(code, Command(Opcode::bit_not));
    builder::writeIndexPos(code, val0);
    useVal(val0);
}

void FuncEnviroBuilder::bit_not(ValueIndexPos val, ValueMeta m) {
    builder::write(code, Command(Opcode::bit_not, false, true));
    builder::writeIndexPos(code, val);
    builder::write(code, m);
    useVal(val);
}

#pragma endregion

void FuncEnviroBuilder::log_not() {
    builder::write(code, Command(Opcode::log_not));
}

void FuncEnviroBuilder::compare(ValueIndexPos val0, ValueIndexPos val1) {
    builder::write(code, Command(Opcode::compare));
    builder::writeIndexPos(code, val0);
    builder::writeIndexPos(code, val1);
    useVal(val0);
    useVal(val1);
}

void FuncEnviroBuilder::compare(ValueIndexPos val0, ValueIndexPos val1, ValueMeta m0, ValueMeta m1) {
    builder::write(code, Command(Opcode::compare, false, true));
    builder::writeIndexPos(code, val0);
    builder::write(code, m0);
    builder::writeIndexPos(code, val1);
    builder::write(code, m1);
    useVal(val0);
    useVal(val1);
}

void FuncEnviroBuilder::jump(JumpCondition cd, const art::ustring& label_name) {
    builder::write(code, Command(Opcode::jump));
    builder::write(code, jumpMap(label_name));
    builder::write(code, cd);
}

void FuncEnviroBuilder::arg_set(ValueIndexPos val0) {
    builder::write(code, Command(Opcode::arg_set));
    builder::writeIndexPos(code, val0);
    useVal(val0);
}

void FuncEnviroBuilder::call(ValueIndexPos fn_mem, bool is_async, bool fn_mem_only_str) {
    builder::write(code, Command(Opcode::call, false, fn_mem_only_str));
    CallFlags f;
    f.async_mode = is_async;
    f.use_result = false;
    code.push_back(f.encoded);
    builder::writeIndexPos(code, fn_mem);
    useVal(fn_mem);
}

void FuncEnviroBuilder::call(ValueIndexPos fn_mem, ValueIndexPos res, bool is_async, bool fn_mem_only_str) {
    builder::write(code, Command(Opcode::call, false, fn_mem_only_str));
    CallFlags f;
    f.async_mode = is_async;
    f.use_result = true;
    code.push_back(f.encoded);
    builder::writeIndexPos(code, fn_mem);
    builder::writeIndexPos(code, res);
    useVal(fn_mem);
    useVal(res);
}

void FuncEnviroBuilder::call_self(bool is_async) {
    builder::write(code, Command(Opcode::call_self));
    CallFlags f;
    f.async_mode = is_async;
    f.use_result = false;
    code.push_back(f.encoded);
}

void FuncEnviroBuilder::call_self(ValueIndexPos res, bool is_async) {
    builder::write(code, Command(Opcode::call_self));
    CallFlags f;
    f.async_mode = is_async;
    f.use_result = true;
    code.push_back(f.encoded);
    builder::writeIndexPos(code, res);
    useVal(res);
}

uint64_t FuncEnviroBuilder::add_local_fn(art::shared_ptr<FuncEnvironment> fn) {
    uint64_t res = local_funs.size();
    if (fn->get_cross_code().empty()) {
        if (strict_mode)
            throw CompileTimeException("local func must have cross code");
        else
            flags.run_time_computable = true;
    }
    local_funs.push_back(fn);
    return res;
}

void FuncEnviroBuilder::call_local(ValueIndexPos in_mem_fn, bool is_async) {
    builder::write(code, Command(Opcode::call_local));
    CallFlags f;
    f.async_mode = is_async;
    f.use_result = false;
    code.push_back(f.encoded);
    builder::writeIndexPos(code, in_mem_fn);
    useVal(in_mem_fn);
}

void FuncEnviroBuilder::call_local(ValueIndexPos in_mem_fn, ValueIndexPos res, bool is_async) {
    builder::write(code, Command(Opcode::call_local));
    CallFlags f;
    f.async_mode = is_async;
    f.use_result = true;
    code.push_back(f.encoded);
    builder::writeIndexPos(code, in_mem_fn);
    builder::writeIndexPos(code, res);
    useVal(in_mem_fn);
    useVal(res);
}

void FuncEnviroBuilder::call_and_ret(ValueIndexPos fn_mem, bool is_async, bool fn_mem_only_str) {
    builder::write(code, Command(Opcode::call_and_ret, false, fn_mem_only_str));
    CallFlags f;
    f.async_mode = is_async;
    f.use_result = false;
    code.push_back(f.encoded);
    builder::writeIndexPos(code, fn_mem);
    useVal(fn_mem);
}

void FuncEnviroBuilder::call_self_and_ret(bool is_async) {
    builder::write(code, Command(Opcode::call_self_and_ret));
    CallFlags f;
    f.async_mode = is_async;
    f.use_result = false;
    code.push_back(f.encoded);
}

void FuncEnviroBuilder::call_local_and_ret(ValueIndexPos in_mem_fn, bool is_async) {
    builder::write(code, Command(Opcode::call_local_and_ret));
    CallFlags f;
    f.async_mode = is_async;
    code.push_back(f.encoded);
    builder::writeIndexPos(code, in_mem_fn);
    useVal(in_mem_fn);
}

void FuncEnviroBuilder::ret(ValueIndexPos val) {
    builder::write(code, Command(Opcode::ret));
    builder::writeIndexPos(code, val);
    useVal(val);
}

void FuncEnviroBuilder::ret_take(ValueIndexPos val) {
    builder::write(code, Command(Opcode::ret_take));
    builder::writeIndexPos(code, val);
    useVal(val);
}

void FuncEnviroBuilder::ret() {
    builder::write(code, Command(Opcode::ret_noting));
}

void FuncEnviroBuilder::copy(ValueIndexPos to, ValueIndexPos from) {
    builder::write(code, Command(Opcode::copy));
    builder::writeIndexPos(code, to);
    builder::writeIndexPos(code, from);
    useVal(to);
    useVal(from);
}

void FuncEnviroBuilder::move(ValueIndexPos to, ValueIndexPos from) {
    builder::write(code, Command(Opcode::move));
    builder::writeIndexPos(code, to);
    builder::writeIndexPos(code, from);
    useVal(to);
    useVal(from);
}

void FuncEnviroBuilder::debug_break() {
    builder::write(code, Command(Opcode::debug_break));
}

void FuncEnviroBuilder::force_debug_break() {
    builder::write(code, Command(Opcode::debug_break));
}

void FuncEnviroBuilder::throw_ex(ValueIndexPos name, ValueIndexPos desc) {
    builder::write(code, Command(Opcode::throw_ex));
    builder::writeIndexPos(code, name);
    builder::writeIndexPos(code, desc);
    useVal(name);
    useVal(desc);
}

void FuncEnviroBuilder::as(ValueIndexPos val, VType meta) {
    builder::write(code, Command(Opcode::as));
    builder::writeIndexPos(code, val);
    builder::write(code, meta);
    useVal(val);
}

void FuncEnviroBuilder::is(ValueIndexPos val, VType meta) {
    builder::write(code, Command(Opcode::is));
    builder::writeIndexPos(code, val);
    builder::write(code, meta);
    useVal(val);
}

void FuncEnviroBuilder::is_gc(ValueIndexPos val) {
    builder::write(code, Command(Opcode::is_gc));
    code.push_back(false);
    builder::writeIndexPos(code, val);
    useVal(val);
}

void FuncEnviroBuilder::is_gc(ValueIndexPos val, ValueIndexPos result) {
    builder::write(code, Command(Opcode::is_gc));
    code.push_back(true);
    builder::writeIndexPos(code, val);
    builder::writeIndexPos(code, result);
    useVal(val);
    useVal(result);
}

void FuncEnviroBuilder::store_bool(ValueIndexPos val) {
    builder::write(code, Command(Opcode::store_bool));
    builder::writeIndexPos(code, val);
    useVal(val);
}

void FuncEnviroBuilder::load_bool(ValueIndexPos val) {
    builder::write(code, Command(Opcode::load_bool));
    builder::writeIndexPos(code, val);
    useVal(val);
}

void FuncEnviroBuilder::inline_native_opcode(uint8_t* opcode, uint32_t len) {
    builder::write(code, Command(Opcode::inline_native));
    builder::write(code, len);
    code.insert(code.end(), opcode, opcode + len);
}

void FuncEnviroBuilder::bind_pos(const art::ustring& name) {
    jump_pos[jumpMap(name)] = code.size();
}

#pragma region arr_op

void default_static_header_arr_op(std::vector<uint8_t>& code, ValueIndexPos arr, ValueMeta arr_meta, bool move = false, ArrCheckMode check_bounds = ArrCheckMode::no_check) {
    builder::write(code, Command(Opcode::arr_op, false, true));
    builder::writeIndexPos(code, arr);
    OpArrFlags flags;
    flags.move_mode = move;
    flags.checked = check_bounds;
    builder::write(code, flags.raw);
    builder::write(code, arr_meta);
}

void FuncEnviroBuilder::_static_arr::set(ValueIndexPos index, ValueIndexPos val, bool move, ArrCheckMode check_bounds) {
    default_static_header_arr_op(build.code, arr, arr_meta, move, check_bounds);
    build.code.push_back((uint8_t)OpcodeArray::set);
    builder::writeIndexPos(build.code, index);
    builder::writeIndexPos(build.code, val);
}

void FuncEnviroBuilder::_static_arr::insert(ValueIndexPos index, ValueIndexPos val, bool move) {
    default_static_header_arr_op(build.code, arr, arr_meta, move);
    build.code.push_back((uint8_t)OpcodeArray::insert);
    builder::writeIndexPos(build.code, index);
    builder::writeIndexPos(build.code, val);
}

void FuncEnviroBuilder::_static_arr::push_end(ValueIndexPos val, bool move) {
    default_static_header_arr_op(build.code, arr, arr_meta, move);
    build.code.push_back((uint8_t)OpcodeArray::push_end);
    builder::writeIndexPos(build.code, val);
}

void FuncEnviroBuilder::_static_arr::push_start(ValueIndexPos val, bool move) {
    default_static_header_arr_op(build.code, arr, arr_meta, move);
    build.code.push_back((uint8_t)OpcodeArray::push_start);
    builder::writeIndexPos(build.code, val);
}

void FuncEnviroBuilder::_static_arr::insert_range(ValueIndexPos arr2, ValueIndexPos arr2_start, ValueIndexPos arr2_end, ValueIndexPos arr_pos, bool move) {
    default_static_header_arr_op(build.code, arr, arr_meta, move);
    build.code.push_back((uint8_t)OpcodeArray::insert_range);
    builder::writeIndexPos(build.code, arr2);
    builder::writeIndexPos(build.code, arr2_start);
    builder::writeIndexPos(build.code, arr2_end);
    builder::writeIndexPos(build.code, arr_pos);
}

void FuncEnviroBuilder::_static_arr::get(ValueIndexPos to, ValueIndexPos index, bool move, ArrCheckMode check_bounds) {
    default_static_header_arr_op(build.code, arr, arr_meta, move, check_bounds);
    build.code.push_back((uint8_t)OpcodeArray::get);
    builder::writeIndexPos(build.code, to);
    builder::writeIndexPos(build.code, index);
}

void FuncEnviroBuilder::_static_arr::take(ValueIndexPos to, ValueIndexPos index, bool move) {
    default_static_header_arr_op(build.code, arr, arr_meta, move);
    build.code.push_back((uint8_t)OpcodeArray::take);
    builder::writeIndexPos(build.code, to);
    builder::writeIndexPos(build.code, index);
}

void FuncEnviroBuilder::_static_arr::take_end(ValueIndexPos to, bool move) {
    default_static_header_arr_op(build.code, arr, arr_meta, move);
    build.code.push_back((uint8_t)OpcodeArray::take_end);
    builder::writeIndexPos(build.code, to);
}

void FuncEnviroBuilder::_static_arr::take_start(ValueIndexPos to, bool move) {
    default_static_header_arr_op(build.code, arr, arr_meta, move);
    build.code.push_back((uint8_t)OpcodeArray::take_start);
    builder::writeIndexPos(build.code, to);
}

void FuncEnviroBuilder::_static_arr::get_range(ValueIndexPos to, ValueIndexPos start, ValueIndexPos end, bool move) {
    default_static_header_arr_op(build.code, arr, arr_meta, move);
    build.code.push_back((uint8_t)OpcodeArray::get_range);
    builder::writeIndexPos(build.code, to);
    builder::writeIndexPos(build.code, start);
    builder::writeIndexPos(build.code, end);
}

void FuncEnviroBuilder::_static_arr::take_range(ValueIndexPos to, ValueIndexPos start, ValueIndexPos end, bool move) {
    default_static_header_arr_op(build.code, arr, arr_meta, move);
    build.code.push_back((uint8_t)OpcodeArray::take_range);
    builder::writeIndexPos(build.code, to);
    builder::writeIndexPos(build.code, start);
    builder::writeIndexPos(build.code, end);
}

void FuncEnviroBuilder::_static_arr::pop_end() {
    default_static_header_arr_op(build.code, arr, arr_meta, false);
    build.code.push_back((uint8_t)OpcodeArray::pop_end);
}

void FuncEnviroBuilder::_static_arr::pop_start() {
    default_static_header_arr_op(build.code, arr, arr_meta, false);
    build.code.push_back((uint8_t)OpcodeArray::pop_start);
}

void FuncEnviroBuilder::_static_arr::remove_item(ValueIndexPos in) {
    default_static_header_arr_op(build.code, arr, arr_meta, false);
    build.code.push_back((uint8_t)OpcodeArray::remove_item);
    builder::writeIndexPos(build.code, in);
}

void FuncEnviroBuilder::_static_arr::remove_range(ValueIndexPos start, ValueIndexPos end) {
    default_static_header_arr_op(build.code, arr, arr_meta, false);
    build.code.push_back((uint8_t)OpcodeArray::remove_range);
    builder::writeIndexPos(build.code, start);
    builder::writeIndexPos(build.code, end);
}

void FuncEnviroBuilder::_static_arr::resize(ValueIndexPos new_size) {
    default_static_header_arr_op(build.code, arr, arr_meta, false);
    build.code.push_back((uint8_t)OpcodeArray::resize);
    builder::writeIndexPos(build.code, new_size);
}

void FuncEnviroBuilder::_static_arr::resize_default(ValueIndexPos new_size, ValueIndexPos default_init_val) {
    default_static_header_arr_op(build.code, arr, arr_meta, false);
    build.code.push_back((uint8_t)OpcodeArray::resize_default);
    builder::writeIndexPos(build.code, new_size);
    builder::writeIndexPos(build.code, default_init_val);
}

void FuncEnviroBuilder::_static_arr::reserve_push_end(ValueIndexPos new_size) {
    default_static_header_arr_op(build.code, arr, arr_meta, false);
    build.code.push_back((uint8_t)OpcodeArray::reserve_push_end);
    builder::writeIndexPos(build.code, new_size);
}

void FuncEnviroBuilder::_static_arr::reserve_push_start(ValueIndexPos new_size) {
    default_static_header_arr_op(build.code, arr, arr_meta, false);
    build.code.push_back((uint8_t)OpcodeArray::reserve_push_start);
    builder::writeIndexPos(build.code, new_size);
}

void FuncEnviroBuilder::_static_arr::commit() {
    default_static_header_arr_op(build.code, arr, arr_meta, false);
    build.code.push_back((uint8_t)OpcodeArray::commit);
}

void FuncEnviroBuilder::_static_arr::decommit(ValueIndexPos blocks_count) {
    default_static_header_arr_op(build.code, arr, arr_meta, false);
    build.code.push_back((uint8_t)OpcodeArray::decommit);
    builder::writeIndexPos(build.code, blocks_count);
}

void FuncEnviroBuilder::_static_arr::remove_reserved() {
    default_static_header_arr_op(build.code, arr, arr_meta, false);
    build.code.push_back((uint8_t)OpcodeArray::remove_reserved);
}

void FuncEnviroBuilder::_static_arr::size(ValueIndexPos set_to) {
    default_static_header_arr_op(build.code, arr, arr_meta, false);
    build.code.push_back((uint8_t)OpcodeArray::size);
    builder::writeIndexPos(build.code, set_to);
}

void default_dynamic_header_arr_op(std::vector<uint8_t>& code, ValueIndexPos arr, bool move = false, ArrCheckMode check_bounds = ArrCheckMode::no_check) {
    builder::write(code, Command(Opcode::arr_op, false, false));
    builder::writeIndexPos(code, arr);
    OpArrFlags flags;
    flags.move_mode = move;
    flags.checked = check_bounds;
    builder::write(code, flags.raw);
}

void FuncEnviroBuilder::_arr::set(ValueIndexPos index, ValueIndexPos val, bool move, ArrCheckMode check_bounds) {
    default_dynamic_header_arr_op(build.code, arr, move, check_bounds);
    build.code.push_back((uint8_t)OpcodeArray::set);
    builder::writeIndexPos(build.code, index);
    builder::writeIndexPos(build.code, val);
}

void FuncEnviroBuilder::_arr::insert(ValueIndexPos index, ValueIndexPos val, bool move) {
    default_dynamic_header_arr_op(build.code, arr, move);
    build.code.push_back((uint8_t)OpcodeArray::insert);
    builder::writeIndexPos(build.code, index);
    builder::writeIndexPos(build.code, val);
}

void FuncEnviroBuilder::_arr::push_end(ValueIndexPos val, bool move) {
    default_dynamic_header_arr_op(build.code, arr, move);
    build.code.push_back((uint8_t)OpcodeArray::push_end);
    builder::writeIndexPos(build.code, val);
}

void FuncEnviroBuilder::_arr::push_start(ValueIndexPos val, bool move) {
    default_dynamic_header_arr_op(build.code, arr, move);
    build.code.push_back((uint8_t)OpcodeArray::push_start);
    builder::writeIndexPos(build.code, val);
}

void FuncEnviroBuilder::_arr::insert_range(ValueIndexPos arr2, ValueIndexPos arr2_start, ValueIndexPos arr2_end, ValueIndexPos arr_pos, bool move) {
    default_dynamic_header_arr_op(build.code, arr, move);
    build.code.push_back((uint8_t)OpcodeArray::insert_range);
    builder::writeIndexPos(build.code, arr2);
    builder::writeIndexPos(build.code, arr2_start);
    builder::writeIndexPos(build.code, arr2_end);
    builder::writeIndexPos(build.code, arr_pos);
}

void FuncEnviroBuilder::_arr::get(ValueIndexPos to, ValueIndexPos index, bool move, ArrCheckMode check_bounds) {
    default_dynamic_header_arr_op(build.code, arr, move, check_bounds);
    build.code.push_back((uint8_t)OpcodeArray::get);
    builder::writeIndexPos(build.code, to);
    builder::writeIndexPos(build.code, index);
}

void FuncEnviroBuilder::_arr::take(ValueIndexPos to, ValueIndexPos index, bool move) {
    default_dynamic_header_arr_op(build.code, arr, move);
    build.code.push_back((uint8_t)OpcodeArray::take);
    builder::writeIndexPos(build.code, to);
    builder::writeIndexPos(build.code, index);
}

void FuncEnviroBuilder::_arr::take_end(ValueIndexPos to, bool move) {
    default_dynamic_header_arr_op(build.code, arr, move);
    build.code.push_back((uint8_t)OpcodeArray::take_end);
    builder::writeIndexPos(build.code, to);
}

void FuncEnviroBuilder::_arr::take_start(ValueIndexPos to, bool move) {
    default_dynamic_header_arr_op(build.code, arr, move);
    build.code.push_back((uint8_t)OpcodeArray::take_start);
    builder::writeIndexPos(build.code, to);
}

void FuncEnviroBuilder::_arr::get_range(ValueIndexPos to, ValueIndexPos start, ValueIndexPos end, bool move) {
    default_dynamic_header_arr_op(build.code, arr, move);
    build.code.push_back((uint8_t)OpcodeArray::get_range);
    builder::writeIndexPos(build.code, to);
    builder::writeIndexPos(build.code, start);
    builder::writeIndexPos(build.code, end);
}

void FuncEnviroBuilder::_arr::take_range(ValueIndexPos to, ValueIndexPos start, ValueIndexPos end, bool move) {
    default_dynamic_header_arr_op(build.code, arr, move);
    build.code.push_back((uint8_t)OpcodeArray::take_range);
    builder::writeIndexPos(build.code, to);
    builder::writeIndexPos(build.code, start);
    builder::writeIndexPos(build.code, end);
}

void FuncEnviroBuilder::_arr::pop_end() {
    default_dynamic_header_arr_op(build.code, arr);
    build.code.push_back((uint8_t)OpcodeArray::pop_end);
}

void FuncEnviroBuilder::_arr::pop_start() {
    default_dynamic_header_arr_op(build.code, arr);
    build.code.push_back((uint8_t)OpcodeArray::pop_start);
}

void FuncEnviroBuilder::_arr::remove_item(ValueIndexPos in) {
    default_dynamic_header_arr_op(build.code, arr);
    build.code.push_back((uint8_t)OpcodeArray::remove_item);
    builder::writeIndexPos(build.code, in);
}

void FuncEnviroBuilder::_arr::remove_range(ValueIndexPos start, ValueIndexPos end) {
    default_dynamic_header_arr_op(build.code, arr);
    build.code.push_back((uint8_t)OpcodeArray::remove_range);
    builder::writeIndexPos(build.code, start);
    builder::writeIndexPos(build.code, end);
}

void FuncEnviroBuilder::_arr::resize(ValueIndexPos new_size) {
    default_dynamic_header_arr_op(build.code, arr);
    build.code.push_back((uint8_t)OpcodeArray::resize);
    builder::writeIndexPos(build.code, new_size);
}

void FuncEnviroBuilder::_arr::resize_default(ValueIndexPos new_size, ValueIndexPos default_init_val) {
    default_dynamic_header_arr_op(build.code, arr);
    build.code.push_back((uint8_t)OpcodeArray::resize_default);
    builder::writeIndexPos(build.code, new_size);
    builder::writeIndexPos(build.code, default_init_val);
}

void FuncEnviroBuilder::_arr::reserve_push_end(ValueIndexPos new_size) {
    default_dynamic_header_arr_op(build.code, arr);
    build.code.push_back((uint8_t)OpcodeArray::reserve_push_end);
    builder::writeIndexPos(build.code, new_size);
}

void FuncEnviroBuilder::_arr::reserve_push_start(ValueIndexPos new_size) {
    default_dynamic_header_arr_op(build.code, arr);
    build.code.push_back((uint8_t)OpcodeArray::reserve_push_start);
    builder::writeIndexPos(build.code, new_size);
}

void FuncEnviroBuilder::_arr::commit() {
    default_dynamic_header_arr_op(build.code, arr);
    build.code.push_back((uint8_t)OpcodeArray::commit);
}

void FuncEnviroBuilder::_arr::decommit(ValueIndexPos blocks_count) {
    default_dynamic_header_arr_op(build.code, arr);
    build.code.push_back((uint8_t)OpcodeArray::decommit);
    builder::writeIndexPos(build.code, blocks_count);
}

void FuncEnviroBuilder::_arr::remove_reserved() {
    default_dynamic_header_arr_op(build.code, arr);
    build.code.push_back((uint8_t)OpcodeArray::remove_reserved);
}

void FuncEnviroBuilder::_arr::size(ValueIndexPos set_to) {
    default_dynamic_header_arr_op(build.code, arr);
    build.code.push_back((uint8_t)OpcodeArray::size);
    builder::writeIndexPos(build.code, set_to);
}

#pragma endregion
//casm,
#pragma region interface

void FuncEnviroBuilder::call_value_interface(ClassAccess access, ValueIndexPos class_val, ValueIndexPos fn_name, bool is_async) {
    builder::write(code, Command(Opcode::call_value_function));
    CallFlags f;
    f.async_mode = is_async;
    f.use_result = false;
    code.push_back(f.encoded);
    builder::writeIndexPos(code, fn_name);
    builder::writeIndexPos(code, class_val);
    builder::write(code, access);
}

void FuncEnviroBuilder::call_value_interface(ClassAccess access, ValueIndexPos class_val, ValueIndexPos fn_name, ValueIndexPos res_val, bool is_async) {
    builder::write(code, Command(Opcode::call_value_function));
    CallFlags f;
    f.async_mode = is_async;
    f.use_result = true;
    code.push_back(f.encoded);
    builder::writeIndexPos(code, fn_name);
    builder::writeIndexPos(code, class_val);
    builder::write(code, access);
    builder::writeIndexPos(code, res_val);
}

void FuncEnviroBuilder::call_value_interface_id(ValueIndexPos class_val, uint64_t class_fun_id, bool is_async) {
    builder::write(code, Command(Opcode::call_value_function));
    CallFlags f;
    f.async_mode = is_async;
    f.use_result = false;
    code.push_back(f.encoded);
    builder::write(code, class_fun_id);
    builder::writeIndexPos(code, class_val);
}

void FuncEnviroBuilder::call_value_interface_id(ValueIndexPos class_val, uint64_t class_fun_id, ValueIndexPos res_val, bool is_async) {
    builder::write(code, Command(Opcode::call_value_function));
    CallFlags f;
    f.async_mode = is_async;
    f.use_result = true;
    code.push_back(f.encoded);
    builder::write(code, class_fun_id);
    builder::writeIndexPos(code, class_val);
    builder::writeIndexPos(code, res_val);
}

void FuncEnviroBuilder::call_value_interface_and_ret(ClassAccess access, ValueIndexPos class_val, ValueIndexPos fn_name, bool is_async) {
    builder::write(code, Command(Opcode::call_value_function_and_ret));
    CallFlags f;
    f.async_mode = is_async;
    f.use_result = false;
    code.push_back(f.encoded);
    builder::writeIndexPos(code, fn_name);
    builder::writeIndexPos(code, class_val);
    builder::write(code, access);
}

void FuncEnviroBuilder::call_value_interface_id_and_ret(ValueIndexPos class_val, uint64_t class_fun_id, bool is_async) {
    builder::write(code, Command(Opcode::call_value_function_id_and_ret));
    CallFlags f;
    f.async_mode = is_async;
    f.use_result = false;
    code.push_back(f.encoded);
    builder::write(code, class_fun_id);
    builder::writeIndexPos(code, class_val);
}

void FuncEnviroBuilder::static_call_value_interface(ClassAccess access, ValueIndexPos class_val, ValueIndexPos fn_name, bool is_async) {
    builder::write(code, Command(Opcode::static_call_value_function));
    CallFlags f;
    f.async_mode = is_async;
    f.use_result = false;
    code.push_back(f.encoded);
    builder::writeIndexPos(code, fn_name);
    builder::writeIndexPos(code, class_val);
    builder::write(code, access);
}

void FuncEnviroBuilder::static_call_value_interface(ClassAccess access, ValueIndexPos class_val, ValueIndexPos fn_name, ValueIndexPos res_val, bool is_async) {
    builder::write(code, Command(Opcode::static_call_value_function));
    CallFlags f;
    f.async_mode = is_async;
    f.use_result = true;
    code.push_back(f.encoded);
    builder::writeIndexPos(code, fn_name);
    builder::writeIndexPos(code, class_val);
    builder::write(code, access);
    builder::writeIndexPos(code, res_val);
}

void FuncEnviroBuilder::static_call_value_interface_id(ValueIndexPos class_val, uint64_t class_fun_id, bool is_async) {
    builder::write(code, Command(Opcode::static_call_value_function_id));
    CallFlags f;
    f.async_mode = is_async;
    f.use_result = false;
    code.push_back(f.encoded);
    builder::write(code, class_fun_id);
    builder::writeIndexPos(code, class_val);
}

void FuncEnviroBuilder::static_call_value_interface_id(ValueIndexPos class_val, uint64_t class_fun_id, ValueIndexPos res_val, bool is_async) {
    builder::write(code, Command(Opcode::static_call_value_function_id));
    CallFlags f;
    f.async_mode = is_async;
    f.use_result = true;
    code.push_back(f.encoded);
    builder::write(code, class_fun_id);
    builder::writeIndexPos(code, class_val);
    builder::writeIndexPos(code, res_val);
}

void FuncEnviroBuilder::static_call_value_interface_and_ret(ClassAccess access, ValueIndexPos class_val, ValueIndexPos fn_name, bool is_async) {
    builder::write(code, Command(Opcode::static_call_value_function_and_ret));
    CallFlags f;
    f.async_mode = is_async;
    f.use_result = false;
    code.push_back(f.encoded);
    builder::writeIndexPos(code, fn_name);
    builder::writeIndexPos(code, class_val);
    builder::write(code, access);
}

void FuncEnviroBuilder::static_call_value_interface_id_and_ret(ValueIndexPos class_val, uint64_t class_fun_id, bool is_async) {
    builder::write(code, Command(Opcode::static_call_value_function_id_and_ret));
    CallFlags f;
    f.async_mode = is_async;
    f.use_result = false;
    code.push_back(f.encoded);
    builder::write(code, class_fun_id);
    builder::writeIndexPos(code, class_val);
}

void FuncEnviroBuilder::get_interface_value(ClassAccess access, ValueIndexPos class_val, ValueIndexPos val_name, ValueIndexPos res) {
    builder::write(code, Command(Opcode::get_structure_value));
    builder::writeIndexPos(code, val_name);
    builder::write(code, access);
    builder::writeIndexPos(code, class_val);
    builder::writeIndexPos(code, res);
}

void FuncEnviroBuilder::set_interface_value(ClassAccess access, ValueIndexPos class_val, ValueIndexPos val_name, ValueIndexPos set_val) {
    builder::write(code, Command(Opcode::set_structure_value));
    builder::writeIndexPos(code, val_name);
    builder::write(code, access);
    builder::writeIndexPos(code, class_val);
    builder::writeIndexPos(code, set_val);
}

#pragma endregion
#pragma region misc

void FuncEnviroBuilder::explicit_await(ValueIndexPos await_value) {
    builder::write(code, Command(Opcode::explicit_await));
    builder::writeIndexPos(code, await_value);
}

void FuncEnviroBuilder::to_gc(ValueIndexPos val) {
    builder::write(code, Command(Opcode::to_gc));
    builder::writeIndexPos(code, val);
}

void FuncEnviroBuilder::localize_gc(ValueIndexPos val) {
    builder::write(code, Command(Opcode::localize_gc));
    builder::writeIndexPos(code, val);
}

void FuncEnviroBuilder::from_gc(ValueIndexPos val) {
    builder::write(code, Command(Opcode::from_gc));
    builder::writeIndexPos(code, val);
}

union xarray_slice_flags {
    enum class _type : uint8_t {
        all_inlined = 1,
        inl_0_ref_1 = 2,
        ref_0_inl_1 = 3,
        all_ref = 4
    };
    enum class _use_index_pos : uint8_t {
        none = 0,
        from = 1,
        to = 2,
        from_to = 3
    };

    struct {
        _type type : 4;
        _use_index_pos use_index_pos : 4;
    };

    xarray_slice_flags(_type t, _use_index_pos p) {
        type = t;
        use_index_pos = p;
    }

    uint8_t encoded;
};

void FuncEnviroBuilder::xarray_slice(ValueIndexPos result, ValueIndexPos val) {
    builder::write(code, Command(Opcode::xarray_slice));
    builder::writeIndexPos(code, result);
    builder::writeIndexPos(code, val);
    code.push_back(xarray_slice_flags(xarray_slice_flags::_type::all_inlined, xarray_slice_flags::_use_index_pos::none).encoded);
}

void FuncEnviroBuilder::xarray_slice(ValueIndexPos result, ValueIndexPos val, ValueIndexPos from) {
    builder::write(code, Command(Opcode::xarray_slice));
    builder::writeIndexPos(code, result);
    builder::writeIndexPos(code, val);
    code.push_back(xarray_slice_flags(xarray_slice_flags::_type::ref_0_inl_1, xarray_slice_flags::_use_index_pos::from).encoded);
    builder::writeIndexPos(code, from);
}

void FuncEnviroBuilder::xarray_slice(ValueIndexPos result, ValueIndexPos val, bool unused, ValueIndexPos to) {
    builder::write(code, Command(Opcode::xarray_slice));
    builder::writeIndexPos(code, result);
    builder::writeIndexPos(code, val);
    code.push_back(xarray_slice_flags(xarray_slice_flags::_type::inl_0_ref_1, xarray_slice_flags::_use_index_pos::to).encoded);
    builder::writeIndexPos(code, to);
}

void FuncEnviroBuilder::xarray_slice(ValueIndexPos result, ValueIndexPos val, ValueIndexPos from, ValueIndexPos to) {
    builder::write(code, Command(Opcode::xarray_slice));
    builder::writeIndexPos(code, result);
    builder::writeIndexPos(code, val);
    code.push_back(xarray_slice_flags(xarray_slice_flags::_type::all_ref, xarray_slice_flags::_use_index_pos::from_to).encoded);
    builder::writeIndexPos(code, from);
    builder::writeIndexPos(code, to);
}

void FuncEnviroBuilder::table_jump(
    std::vector<art::ustring> table,
    ValueIndexPos index,
    bool is_signed,
    TableJumpCheckFailAction too_large,
    const art::ustring& too_large_label,
    TableJumpCheckFailAction too_small,
    const art::ustring& too_small_label) {
    if ((uint32_t)table.size() != table.size())
        throw CompileTimeException("table size is too big");
    builder::write(code, Command(Opcode::table_jump));
    TableJumpFlags f;
    f.is_signed = is_signed;
    f.too_large = too_large;
    f.too_small = too_small;
    builder::write(code, f.raw);
    if (too_large == TableJumpCheckFailAction::jump_specified)
        builder::write(code, jumpMap(too_large_label));
    if (too_small == TableJumpCheckFailAction::jump_specified && is_signed)
        builder::write(code, jumpMap(too_small_label));
    builder::writeIndexPos(code, index);
    builder::write(code, (uint32_t)table.size());
    for (auto& i : table)
        builder::write(code, jumpMap(i));
}

void FuncEnviroBuilder::get_reference(ValueIndexPos res, ValueIndexPos val) {
    builder::write(code, Command(Opcode::get_reference));
    builder::writeIndexPos(code, res);
    builder::writeIndexPos(code, val);
}

void FuncEnviroBuilder::make_as_const(ValueIndexPos res) {
    builder::write(code, Command(Opcode::make_as_const));
    builder::writeIndexPos(code, res);
}

void FuncEnviroBuilder::remove_const_protect(ValueIndexPos res) {
    builder::write(code, Command(Opcode::remove_const_protect));
    builder::writeIndexPos(code, res);
}

void FuncEnviroBuilder::copy_un_constant(ValueIndexPos res, ValueIndexPos val) {
    builder::write(code, Command(Opcode::copy_un_constant));
    builder::writeIndexPos(code, res);
    builder::writeIndexPos(code, val);
}

void FuncEnviroBuilder::copy_un_reference(ValueIndexPos res, ValueIndexPos val) {
    builder::write(code, Command(Opcode::copy_un_reference));
    builder::writeIndexPos(code, res);
    builder::writeIndexPos(code, val);
}

void FuncEnviroBuilder::move_un_reference(ValueIndexPos res, ValueIndexPos val) {
    builder::write(code, Command(Opcode::move_un_reference));
    builder::writeIndexPos(code, res);
    builder::writeIndexPos(code, val);
}

void FuncEnviroBuilder::remove_qualifiers(ValueIndexPos res) {
    builder::write(code, Command(Opcode::remove_qualifiers));
    builder::writeIndexPos(code, res);
}

#pragma endregion
#pragma region except

FuncEnviroBuilder::_except_build::_except_build(FuncEnviroBuilder& build)
    : build(build) {}

void FuncEnviroBuilder::_except_build::handle_begin(uint64_t id) {
    if (build.except_ids_watch.contains(id))
        throw InvalidOperation("This exception handle id already defined and in use");
    builder::write(build.code, Command(Opcode::handle_begin));
    builder::write(build.code, id);
    build.except_ids_watch.emplace(id);
}

void FuncEnviroBuilder::_except_build::handle_catch(uint64_t id, const std::initializer_list<ValueIndexPos>& filter_strings) {
    if (!build.except_ids_watch.contains(id))
        throw InvalidOperation("This exception handle id is not defined");
    bool is_mixed = false;
    bool on_constants = false;
    bool on_enviro = false;
    for (auto& i : filter_strings) {
        if (i.pos == ValuePos::in_constants)
            on_constants = true;
        else if (i.pos != ValuePos::in_enviro)
            throw InvalidArguments("All filter strings must be in environment or constants");
        else
            on_enviro = true;

        if (on_constants && on_enviro) {
            is_mixed = true;
            break;
        }
    }
    builder::write(build.code, Command(Opcode::handle_catch));
    builder::write(build.code, id);

    if (is_mixed) {
        builder::write<char>(build.code, 3);
        builder::writePackedLen(build.code, filter_strings.size());
        for (auto& i : filter_strings) {
            builder::write<bool>(build.code, i.pos == ValuePos::in_enviro);
            if (i.pos == ValuePos::in_enviro)
                builder::write(build.code, i.index);
            else
                builder::writeString(build.code, (art::ustring)build.all_constants[i.index]);
        }
    } else {
        if (on_enviro) {
            if (filter_strings.size() == 1) {
                builder::write<char>(build.code, 1);
                builder::write(build.code, filter_strings.begin()->index);
            }
            builder::write<char>(build.code, 2);
            builder::writePackedLen(build.code, filter_strings.size());
            for (auto& i : filter_strings)
                builder::write(build.code, i.index);
        } else { //on_constants
            builder::write<char>(build.code, 0);
            builder::writePackedLen(build.code, filter_strings.size());
            for (auto& i : filter_strings)
                builder::writeString(build.code, (art::ustring)build.all_constants[i.index]);
        }
    }
}

void FuncEnviroBuilder::_except_build::handle_catch_all(uint64_t id) {
    if (!build.except_ids_watch.contains(id))
        throw InvalidOperation("This exception handle id is not defined");
    builder::write(build.code, Command(Opcode::handle_catch));
    builder::write(build.code, id);
    builder::write<char>(build.code, 4);
}

void FuncEnviroBuilder::_except_build::handle_catch_filter(uint64_t id, ValueIndexPos filter_fn_mem, uint16_t filter_enviro_slice_begin, uint16_t filter_enviro_slice_end) {
    if (!build.except_ids_watch.contains(id))
        throw InvalidOperation("This exception handle id is not defined");
    builder::write(build.code, Command(Opcode::handle_catch));
    builder::write(build.code, id);
    builder::write<char>(build.code, 5);
    builder::writeIndexPos(build.code, filter_fn_mem);
    builder::write(build.code, filter_enviro_slice_begin);
    builder::write(build.code, filter_enviro_slice_end);
}

void FuncEnviroBuilder::_except_build::handle_finally(uint64_t id, ValueIndexPos fn_mem) {
    if (!build.except_ids_watch.contains(id))
        throw InvalidOperation("This exception handle id is not defined");
    builder::write(build.code, Command(Opcode::handle_finally));
    builder::writeIndexPos(build.code, fn_mem);
    builder::write(build.code, id);
}

void FuncEnviroBuilder::_except_build::handle_end(uint64_t id) {
    if (!build.except_ids_watch.contains(id))
        throw InvalidOperation("This exception handle id is not defined");
    builder::write(build.code, Command(Opcode::handle_end));
    builder::write(build.code, id);
    build.except_ids_watch.erase(id);
}

FuncEnviroBuilder::_except_build FuncEnviroBuilder::except() {
    return _except_build(*this);
}

#pragma endregion
#pragma region life_time

FuncEnviroBuilder::_life_specifier::_life_specifier(FuncEnviroBuilder& build)
    : build(build) {}

FuncEnviroBuilder::_life_specifier& FuncEnviroBuilder::_life_specifier::hold(uint64_t id, ValueIndexPos enviro_val) {
    if (enviro_val.pos != ValuePos::in_enviro)
        throw InvalidArguments("Function must be in environment");
    builder::write(build.code, Command(Opcode::value_hold));
    builder::write(build.code, id);
    builder::write(build.code, enviro_val.index);
    return *this;
}

FuncEnviroBuilder::_life_specifier& FuncEnviroBuilder::_life_specifier::release(uint64_t id) {
    builder::write(build.code, Command(Opcode::value_unhold));
    builder::write(build.code, id);
    return *this;
}

FuncEnviroBuilder::_life_specifier FuncEnviroBuilder::life_time() {
    return _life_specifier(*this);
}
#pragma endregion
#pragma endregion

art::shared_ptr<FuncEnvironment> FuncEnviroBuilder::O_prepare_func() {
    if (!flags.run_time_computable)
        return new FuncEnvironment(O_build_func(), flags.can_be_unloaded, flags.is_cheap);
    else {
        std::vector<uint8_t> fn;
        builder::writeString(fn, "art_1.0.0");
        size_t skip = fn.size();
        builder::write(fn, flags);
        if (flags.used_static)
            builder::write(fn, static_values);
        if (flags.used_enviro_vals)
            builder::write(fn, values);
        if (flags.used_arguments)
            builder::write(fn, args_values);
        builder::writePackedLen(fn, constants_values);

        builder::writePackedLen(fn, jump_pos.size());
        for (uint64_t it : jump_pos)
            builder::write(fn, it);

        fn.insert(fn.end(), code.begin(), code.end());
        *(uint64_t*)(fn.data() + skip) = fn.size();
        all_constants.clear();
        return new FuncEnvironment(std::move(fn), std::move(dynamic_values), std::move(local_funs), flags.can_be_unloaded, flags.is_cheap);
    }
}

FuncEnviroBuilder& FuncEnviroBuilder::O_flag_can_be_unloaded(bool can_be_unloaded) {
    flags.can_be_unloaded = can_be_unloaded;
    return *this;
}

FuncEnviroBuilder& FuncEnviroBuilder::O_flag_is_translated(bool is_translated) {
    flags.is_translated = is_translated;
    return *this;
}

FuncEnviroBuilder& FuncEnviroBuilder::O_flag_is_cheap(bool is_cheap) {
    flags.is_cheap = is_cheap;
    return *this;
}

FuncEnviroBuilder& FuncEnviroBuilder::O_flag_used_vec128(uint8_t index) {
    switch (index) {
    case 0:
        flags.used_vec.vec128_0 = true;
        break;
    case 1:
        flags.used_vec.vec128_1 = true;
        break;
    case 2:
        flags.used_vec.vec128_2 = true;
        break;
    case 3:
        flags.used_vec.vec128_3 = true;
        break;
    case 4:
        flags.used_vec.vec128_4 = true;
        break;
    case 5:
        flags.used_vec.vec128_5 = true;
        break;
    case 6:
        flags.used_vec.vec128_6 = true;
        break;
    case 7:
        flags.used_vec.vec128_7 = true;
        break;
    case 8:
        flags.used_vec.vec128_8 = true;
        break;
    case 9:
        flags.used_vec.vec128_9 = true;
        break;
    case 10:
        flags.used_vec.vec128_10 = true;
        break;
    case 11:
        flags.used_vec.vec128_11 = true;
        break;
    case 12:
        flags.used_vec.vec128_12 = true;
        break;
    case 13:
        flags.used_vec.vec128_13 = true;
        break;
    case 14:
        flags.used_vec.vec128_14 = true;
        break;
    case 15:
        flags.used_vec.vec128_15 = true;
        break;
    default:
        throw InvalidArguments("Used vector index must be in range 0-15");
    }
    return *this;
}

FuncEnviroBuilder& FuncEnviroBuilder::O_flag_is_patchable(bool is_patchable) {
    flags.is_patchable = is_patchable;
    return *this;
}

FuncEnviroBuilder_line_info FuncEnviroBuilder::O_line_info_begin() {
    FuncEnviroBuilder_line_info ret;
    ret.begin = code.size();
    return ret;
}

void FuncEnviroBuilder::O_line_info_end(FuncEnviroBuilder_line_info line_info) {
    //TODO: add line info
}

std::vector<uint8_t> FuncEnviroBuilder::O_build_func() {
    if (flags.run_time_computable)
        throw CompileTimeException("to build function, function must not use run time computable functions, ie dynamic");
    std::vector<uint8_t> fn;
    builder::writeString(fn, "art_1.0.0");
    size_t skip = fn.size();
    flags.has_local_functions = local_funs.size() > 0;
    builder::write(fn, flags);
    if (flags.used_static)
        builder::write(fn, static_values);
    if (flags.used_enviro_vals)
        builder::write(fn, values);
    if (flags.used_arguments)
        builder::write(fn, args_values);
    builder::writePackedLen(fn, constants_values);


    if (flags.has_local_functions)
        builder::writePackedLen(fn, local_funs.size());
    for (auto& locals : local_funs) {
        const auto& loc = locals->get_cross_code();
        if (loc.empty())
            throw CompileTimeException("local function must be with opcodes");
        else {
            builder::writePackedLen(fn, loc.size());
            fn.insert(fn.end(), loc.begin(), loc.end());
        }
    }

    builder::writePackedLen(fn, jump_pos.size());
    for (uint64_t it : jump_pos)
        builder::write(fn, it);

    fn.insert(fn.end(), code.begin(), code.end());
    *(uint64_t*)(fn.data() + skip) = fn.size();
    return fn;
}

void FuncEnviroBuilder::O_load_func(const art::ustring& str) {
    FuncEnvironment::Load(O_prepare_func(), str);
}

void FuncEnviroBuilder::O_patch_func(const art::ustring& symbol_name) {
    FuncEnvironment::fastHotPatch(symbol_name, new FuncHandle::inner_handle(O_build_func(), flags.is_cheap, nullptr));
}
