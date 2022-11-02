// Copyright Danyil Melnytskyi 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once
#include <bit>
#include <stdint.h>
#include "tools.hpp"
#include "run_time_compiler.hpp"
#include "attacha_abi.hpp"
using namespace run_time;

#pragma region FuncEviroBuilder
#pragma region SetRem
void FuncEviroBuilder::set_constant(uint16_t val, const ValueItem& cv, bool is_dynamic) {
	const_cast<ValueItem&>(cv).getAsync();
	code.push_back(Command(Opcode::set, cv.meta.use_gc, !is_dynamic).toCmd());
	builder::write(code, val);
	builder::writeAny(code, const_cast<ValueItem&>(cv));
	useVal(val);
}
void FuncEviroBuilder::set_stack_any_array(uint16_t val, uint32_t len) {
	code.push_back(Command(Opcode::set_saar, false, false).toCmd());
	builder::write(code, val);
	builder::write(code, len);
	useVal(val);
}
void FuncEviroBuilder::remove(uint16_t val, ValueMeta m) {
	code.push_back(Command(Opcode::remove,false,true).toCmd());
	builder::write(code, m.vtype);
	builder::write(code, val);
	useVal(val);
}
void FuncEviroBuilder::remove(uint16_t val) {
	code.push_back(Command(Opcode::remove).toCmd());
	builder::write(code, val);
	useVal(val);
}
#pragma endregion
#pragma region numeric
void FuncEviroBuilder::sum(uint16_t val0, uint16_t val1) {
	code.push_back(Command(Opcode::sum).toCmd());
	builder::write(code, val0);
	builder::write(code, val1);
	useVal(val0);
	useVal(val1);
}
void FuncEviroBuilder::sum(uint16_t val0, uint16_t val1, ValueMeta m0, ValueMeta m1) {
	sum(val0, val1);//TO-DO
	useVal(val0);
	useVal(val1);
}
void FuncEviroBuilder::minus(uint16_t val0, uint16_t val1) {
	code.push_back(Command(Opcode::minus).toCmd());
	builder::write(code, val0);
	builder::write(code, val1);
	useVal(val0);
	useVal(val1);
}
void FuncEviroBuilder::minus(uint16_t val0, uint16_t val1, ValueMeta m0, ValueMeta m1) {
	minus(val0, val1);//TO-DO
	useVal(val0);
	useVal(val1);
}
void FuncEviroBuilder::div(uint16_t val0, uint16_t val1) {
	code.push_back(Command(Opcode::div).toCmd());
	builder::write(code, val0);
	builder::write(code, val1);
	useVal(val0);
	useVal(val1);
}
void FuncEviroBuilder::div(uint16_t val0, uint16_t val1, ValueMeta m0, ValueMeta m1) {
	div(val0, val1);//TO-DO
	useVal(val0);
	useVal(val1);
}
void FuncEviroBuilder::mul(uint16_t val0, uint16_t val1) {
	code.push_back(Command(Opcode::mul).toCmd());
	builder::write(code, val0);
	builder::write(code, val1);
	useVal(val0);
	useVal(val1);
}
void FuncEviroBuilder::mul(uint16_t val0, uint16_t val1, ValueMeta m0, ValueMeta m1){
	mul(val0, val1);//TO-DO
	useVal(val0);
	useVal(val1);
}
void FuncEviroBuilder::rest(uint16_t val0, uint16_t val1) {
	code.push_back(Command(Opcode::rest).toCmd());
	builder::write(code, val0);
	builder::write(code, val1);
	useVal(val0);
	useVal(val1);
}
void FuncEviroBuilder::rest(uint16_t val0, uint16_t val1, ValueMeta m0, ValueMeta m1) {
	mul(val0, val1);//TO-DO
	useVal(val0);
	useVal(val1);
}
#pragma endregion
#pragma region bit
void FuncEviroBuilder::bit_xor(uint16_t val0, uint16_t val1) {
	code.push_back(Command(Opcode::bit_xor).toCmd());
	builder::write(code, val0);
	builder::write(code, val1);
	useVal(val0);
	useVal(val1);
}
void FuncEviroBuilder::bit_xor(uint16_t val0, uint16_t val1, ValueMeta m0, ValueMeta m1) {
	bit_xor(val0, val1);//TO-DO
	useVal(val0);
	useVal(val1);
}

void FuncEviroBuilder::bit_or(uint16_t val0, uint16_t val1) {
	code.push_back(Command(Opcode::bit_or).toCmd());
	builder::write(code, val0);
	builder::write(code, val1);
	useVal(val0);
	useVal(val1);
}
void FuncEviroBuilder::bit_or(uint16_t val0, uint16_t val1, ValueMeta m0, ValueMeta m1) {
	bit_or(val0, val1);//TO-DO
	useVal(val0);
	useVal(val1);
}

void FuncEviroBuilder::bit_and(uint16_t val0, uint16_t val1) {
	code.push_back(Command(Opcode::bit_and).toCmd());
	builder::write(code, val0);
	builder::write(code, val1);
	useVal(val0);
	useVal(val1);
}
void FuncEviroBuilder::bit_and(uint16_t val0, uint16_t val1, ValueMeta m0, ValueMeta m1) {
	bit_and(val0, val1);//TO-DO
	useVal(val0);
	useVal(val1);
}

void FuncEviroBuilder::bit_not(uint16_t val0) {
	code.push_back(Command(Opcode::bit_not).toCmd());
	builder::write(code, val0);
	useVal(val0);
}
void FuncEviroBuilder::bit_not(uint16_t val, ValueMeta m) {
	bit_not(val);//TO-DO
	useVal(val);
}
#pragma endregion

void FuncEviroBuilder::log_not() {
	code.push_back(Command(Opcode::log_not).toCmd());
}

void FuncEviroBuilder::compare(uint16_t val0, uint16_t val1) {
	code.push_back(Command(Opcode::compare).toCmd());
	builder::write(code, val0);
	builder::write(code, val1);
	useVal(val0);
	useVal(val1);
}
void FuncEviroBuilder::compare(uint16_t val0, uint16_t val1, ValueMeta m0, ValueMeta m1) {
	compare(val0, val1);//TO-DO
	useVal(val0);
	useVal(val1);
}

void FuncEviroBuilder::jump(JumpCondition cd, uint64_t pos) {
	code.push_back(Command(Opcode::compare).toCmd());
	builder::write(code, pos);
	builder::write(code, cd);
}

void FuncEviroBuilder::arg_set(uint16_t val0) {
	code.push_back(Command(Opcode::arg_set).toCmd());
	builder::write(code, val0);
	useVal(val0);
}

void FuncEviroBuilder::call(const std::string& fn_name, bool is_async) {
	code.push_back(Command(Opcode::call).toCmd());
	CallFlags f;
	f.in_memory = false;
	f.async_mode = is_async;
	f.use_result = false;
	code.push_back(f.encoded);
	builder::writeString(code, fn_name);
}
void FuncEviroBuilder::call(const std::string& fn_name, uint16_t res, bool catch_ex, bool is_async){
	code.push_back(Command(Opcode::call).toCmd());
	CallFlags f;
	f.in_memory = false;
	f.async_mode = is_async;
	f.use_result = true;
	f.except_catch = catch_ex;
	code.push_back(f.encoded);
	builder::writeString(code, fn_name);
	builder::write(code, res);
	useVal(res);
}

void FuncEviroBuilder::call(uint16_t fn_mem, bool is_async, bool fn_mem_only_str) {
	code.push_back(Command(Opcode::call,false, fn_mem_only_str).toCmd());
	CallFlags f;
	f.in_memory = true;
	f.async_mode = is_async;
	f.use_result = false;
	code.push_back(f.encoded);
	builder::write(code, fn_mem);
	useVal(fn_mem);
}
void FuncEviroBuilder::call(uint16_t fn_mem, uint16_t res, bool catch_ex, bool is_async, bool fn_mem_only_str) {
	code.push_back(Command(Opcode::call, false, fn_mem_only_str).toCmd());
	CallFlags f;
	f.in_memory = true;
	f.async_mode = is_async;
	f.use_result = true;
	f.except_catch = catch_ex;
	code.push_back(f.encoded);
	builder::write(code, fn_mem);
	builder::write(code, res);
	useVal(fn_mem);
	useVal(res);
}


void FuncEviroBuilder::call_self(bool is_async) {
	code.push_back(Command(Opcode::call_self).toCmd());
	CallFlags f;
	f.async_mode = is_async;
	f.use_result = false;
	code.push_back(f.encoded);
}
void FuncEviroBuilder::call_self(uint16_t res, bool catch_ex, bool is_async) {
	code.push_back(Command(Opcode::call_self).toCmd());
	CallFlags f;
	f.async_mode = is_async;
	f.except_catch = catch_ex;
	f.use_result = true;
	code.push_back(f.encoded);
	builder::write(code, res);
	useVal(res);
}


uint32_t FuncEviroBuilder::addLocalFn(typed_lgr<FuncEnviropment> fn) {
	local_funs.push_back(fn);
	uint32_t res = static_cast<uint32_t>(local_funs.size() - 1);
	if (res != (local_funs.size() - 1))
		throw CompileTimeException("too many local funcs");
	return res;
}
void FuncEviroBuilder::call_local(typed_lgr<FuncEnviropment> fn, bool is_async) {
	code.push_back(Command(Opcode::call_local).toCmd());
	CallFlags f;
	f.in_memory = false;
	f.async_mode = is_async;
	f.use_result = false;
	code.push_back(f.encoded);
	builder::write(code, addLocalFn(fn));
}
void FuncEviroBuilder::call_local(typed_lgr<FuncEnviropment> fn, uint16_t res, bool catch_ex, bool is_async) {
	code.push_back(Command(Opcode::call_local).toCmd());
	CallFlags f;
	f.in_memory = false;
	f.async_mode = is_async;
	f.except_catch = catch_ex;
	f.use_result = true;
	code.push_back(f.encoded);
	builder::write(code, addLocalFn(fn));
	builder::write(code, res);
	useVal(res);
}

void FuncEviroBuilder::call_local_in_mem(uint16_t in_mem_fn, bool is_async) {
	code.push_back(Command(Opcode::call_local).toCmd());
	CallFlags f;
	f.in_memory = true;
	f.async_mode = is_async;
	f.use_result = false;
	code.push_back(f.encoded);
	builder::write(code, in_mem_fn);
	useVal(in_mem_fn);
}
void FuncEviroBuilder::call_local_in_mem(uint16_t in_mem_fn, uint16_t res, bool catch_ex, bool is_async) {
	code.push_back(Command(Opcode::call_local).toCmd());
	CallFlags f;
	f.in_memory = true;
	f.async_mode = is_async;
	f.except_catch = catch_ex;
	f.use_result = true;
	code.push_back(f.encoded);
	builder::write(code, in_mem_fn);
	builder::write(code, res);
	useVal(in_mem_fn);
	useVal(res);
}
void FuncEviroBuilder::call_local_idx(uint32_t fn, bool is_async) {
	code.push_back(Command(Opcode::call_local).toCmd());
	CallFlags f;
	f.in_memory = false;
	f.async_mode = is_async;
	f.use_result = false;
	code.push_back(f.encoded);
	builder::write(code, fn);
}
void FuncEviroBuilder::call_local_idx(uint32_t fn, uint16_t res, bool catch_ex, bool is_async) {
	code.push_back(Command(Opcode::call_local).toCmd());
	CallFlags f;
	f.in_memory = false;
	f.async_mode = is_async;
	f.except_catch = catch_ex;
	f.use_result = true;
	code.push_back(f.encoded);
	builder::write(code, fn);
	builder::write(code, res);
	useVal(res);
}

void FuncEviroBuilder::call_and_ret(const std::string& fn_name, bool catch_ex, bool is_async) {
	code.push_back(Command(Opcode::call_and_ret).toCmd());
	CallFlags f;
	f.in_memory = false;
	f.async_mode = is_async;
	f.use_result = false;
	f.except_catch = catch_ex;
	code.push_back(f.encoded);
	builder::writeString(code, fn_name);
}
void FuncEviroBuilder::call_and_ret(uint16_t fn_mem, bool catch_ex, bool is_async, bool fn_mem_only_str) {
	code.push_back(Command(Opcode::call_and_ret, false, fn_mem_only_str).toCmd());
	CallFlags f;
	f.in_memory = true;
	f.async_mode = is_async;
	f.use_result = false;
	f.except_catch = catch_ex;
	code.push_back(f.encoded);
	builder::write(code, fn_mem);
	useVal(fn_mem);
}

void FuncEviroBuilder::call_self_and_ret(bool catch_ex, bool is_async) {
	code.push_back(Command(Opcode::call_self_and_ret).toCmd());
	CallFlags f;
	f.async_mode = is_async;
	f.use_result = false;
	f.except_catch = catch_ex;
	code.push_back(f.encoded);
}



void FuncEviroBuilder::call_local_and_ret(typed_lgr<FuncEnviropment> fn, bool catch_ex, bool is_async) {
	code.push_back(Command(Opcode::call_local_and_ret).toCmd());
	CallFlags f;
	f.in_memory = false;
	f.async_mode = is_async;
	f.except_catch = catch_ex;
	code.push_back(f.encoded);
	builder::write(code, addLocalFn(fn));
}
void FuncEviroBuilder::call_local_and_ret_in_mem(uint16_t in_mem_fn, bool catch_ex, bool is_async) {
	code.push_back(Command(Opcode::call_local_and_ret).toCmd());
	CallFlags f;
	f.in_memory = true;
	f.async_mode = is_async;
	f.except_catch = catch_ex;
	code.push_back(f.encoded);
	builder::write(code, in_mem_fn);
	useVal(in_mem_fn);
}
void FuncEviroBuilder::call_local_and_ret_idx(uint32_t fn, bool catch_ex, bool is_async) {
	code.push_back(Command(Opcode::call_local_and_ret).toCmd());
	CallFlags f;
	f.in_memory = false;
	f.async_mode = is_async;
	f.except_catch = catch_ex;
	code.push_back(f.encoded);
	builder::write(code, fn);
}

void FuncEviroBuilder::ret(uint16_t val) {
	code.push_back(Command(Opcode::ret).toCmd());
	builder::write(code, val);
	useVal(val);
}
void FuncEviroBuilder::ret() {
	code.push_back(Command(Opcode::ret_noting).toCmd());
}
void FuncEviroBuilder::copy(uint16_t to, uint16_t from) {
	code.push_back(Command(Opcode::copy).toCmd());
	builder::write(code, to);
	builder::write(code, from);
	useVal(to);
	useVal(from);
}
void FuncEviroBuilder::move(uint16_t to, uint16_t from) {
	code.push_back(Command(Opcode::move).toCmd());
	builder::write(code, to);
	builder::write(code, from);
	useVal(to);
	useVal(from);
}

void FuncEviroBuilder::debug_break() {
	code.push_back(Command(Opcode::debug_break).toCmd());
}
void FuncEviroBuilder::force_debug_reak() {
	code.push_back(Command(Opcode::debug_break).toCmd());
}

void FuncEviroBuilder::throwEx(const std::string& name, const std::string& desck) {
	code.push_back(Command(Opcode::throw_ex).toCmd());
	code.push_back(false);
	builder::writeString(code, name);
	builder::writeString(code, desck);
}
void FuncEviroBuilder::throwEx(uint16_t name, uint16_t desck, bool values_is_only_string) {
	code.push_back(Command(Opcode::throw_ex,false, values_is_only_string).toCmd());
	code.push_back(true);
	builder::write(code, name);
	builder::write(code, desck);
	useVal(name);
	useVal(desck);
}


void FuncEviroBuilder::as(uint16_t val, ValueMeta meta) {
	code.push_back(Command(Opcode::as).toCmd());
	builder::write(code, val);
	builder::write(code, meta);
	useVal(val);
}
void FuncEviroBuilder::is(uint16_t val, ValueMeta meta) {
	code.push_back(Command(Opcode::is).toCmd());
	builder::write(code, val);
	builder::write(code, meta);
	useVal(val);
}


void FuncEviroBuilder::store_bool(uint16_t val) {
	code.push_back(Command(Opcode::store_bool).toCmd());
	builder::write(code, val);
	useVal(val);
}
void FuncEviroBuilder::load_bool(uint16_t val) {
	code.push_back(Command(Opcode::load_bool).toCmd());
	builder::write(code, val);
	useVal(val);
}

void FuncEviroBuilder::inline_native_opcode(uint8_t* opcode, uint32_t len){
	code.push_back(Command(Opcode::inline_native).toCmd());
	builder::write(code, len);
	code.insert(code.end(), opcode, opcode + len);
}

uint64_t FuncEviroBuilder::bind_pos() {
	jump_pos.push_back(code.size());
	return jump_pos.size() - 1;
}
#pragma region arr_op
void FuncEviroBuilder::arr_set(uint16_t arr, uint16_t from, uint64_t to, bool move, ArrCheckMode check_bounds, VType array_type) {
	if (array_type != VType::noting) {
		switch (array_type) {
		case VType::raw_arr_i8:
		case VType::raw_arr_i16:
		case VType::raw_arr_i32:
		case VType::raw_arr_i64:
		case VType::raw_arr_ui8:
		case VType::raw_arr_ui16:
		case VType::raw_arr_ui32:
		case VType::raw_arr_ui64:
		case VType::raw_arr_flo:
		case VType::raw_arr_doub:
		case VType::uarr:
		case VType::faarr:
		case VType::saarr:
		case VType::class_:
		case VType::morph:
		case VType::proxy:
			break;
		default:
			throw InvalidOperation("Unsupported operation to non array type and interface type");
		}
	}
	code.push_back(Command(Opcode::arr_op, false, array_type != VType::noting).toCmd());
	builder::write(code ,arr);
	OpArrFlags flags;
	flags.by_val_mode = false;
	flags.move_mode = move;
	flags.checked = check_bounds;
	code.push_back(flags.raw);
	code.push_back((uint8_t)OpcodeArray::set);
	if(array_type != VType::noting)
		builder::write(code, array_type);
	builder::write(code, from);
	builder::write(code, to);
}
void FuncEviroBuilder::arr_setByVal(uint16_t arr, uint16_t from, uint16_t to, bool move, ArrCheckMode check_bounds, VType array_type) {
	if (array_type != VType::noting) {
		switch (array_type) {
		case VType::raw_arr_i8:
		case VType::raw_arr_i16:
		case VType::raw_arr_i32:
		case VType::raw_arr_i64:
		case VType::raw_arr_ui8:
		case VType::raw_arr_ui16:
		case VType::raw_arr_ui32:
		case VType::raw_arr_ui64:
		case VType::raw_arr_flo:
		case VType::raw_arr_doub:
		case VType::uarr:
		case VType::faarr:
		case VType::saarr:
		case VType::class_:
		case VType::morph:
		case VType::proxy:
			break;
		default:
			throw InvalidOperation("Unsupported operation to non array type and interface type");
		}
	}
	code.push_back(Command(Opcode::arr_op, false, array_type != VType::noting).toCmd());
	builder::write(code, arr);
	OpArrFlags flags;
	flags.by_val_mode = true;
	flags.move_mode = move;
	flags.checked = check_bounds;
	code.push_back(flags.raw);
	code.push_back((uint8_t)OpcodeArray::set);
	if (array_type != VType::noting)
		builder::write(code, array_type);
	builder::write(code, from);
	builder::write(code, to);
}
void FuncEviroBuilder::arr_insert(uint16_t arr, uint16_t from, uint64_t to, bool move, bool static_mode) {
	code.push_back(Command(Opcode::arr_op, false, static_mode).toCmd());
	builder::write(code, arr);
	OpArrFlags flags;
	flags.by_val_mode = false;
	flags.move_mode = move;
	code.push_back(flags.raw);
	code.push_back((uint8_t)OpcodeArray::insert);
	builder::write(code, from);
	builder::write(code, to);
}
void FuncEviroBuilder::arr_insertByVal(uint16_t arr, uint16_t from, uint16_t to, bool move, bool static_mode) {
	code.push_back(Command(Opcode::arr_op, false, static_mode).toCmd());
	builder::write(code, arr);
	OpArrFlags flags;
	flags.by_val_mode = true;
	flags.move_mode = move;
	code.push_back(flags.raw);
	code.push_back((uint8_t)OpcodeArray::insert);
	builder::write(code, from);
	builder::write(code, to);
}
void FuncEviroBuilder::arr_push_end(uint16_t arr, uint16_t from, bool move, bool static_mode) {
	code.push_back(Command(Opcode::arr_op, false, static_mode).toCmd());
	builder::write(code, arr);
	OpArrFlags flags;
	flags.move_mode = move;
	code.push_back(flags.raw);
	code.push_back((uint8_t)OpcodeArray::push_end);
	builder::write(code, from);
}
void FuncEviroBuilder::arr_push_start(uint16_t arr, uint16_t from, bool move, bool static_mode) {
	code.push_back(Command(Opcode::arr_op, false, static_mode).toCmd());
	builder::write(code, arr);
	OpArrFlags flags;
	flags.move_mode = move;
	code.push_back(flags.raw);
	code.push_back((uint8_t)OpcodeArray::push_start);
	builder::write(code, from);
}
void FuncEviroBuilder::arr_insert_range(uint16_t arr, uint16_t arr2, uint64_t arr2_start, uint64_t arr2_end, uint64_t arr_pos, bool move, bool static_mode) {
	code.push_back(Command(Opcode::arr_op, false, static_mode).toCmd());
	builder::write(code, arr);
	OpArrFlags flags;
	flags.by_val_mode = false;
	flags.move_mode = move;
	code.push_back(flags.raw);
	code.push_back((uint8_t)OpcodeArray::insert_range);
	builder::write(code, arr2);
	builder::write(code, arr_pos);
	builder::write(code, arr2_start);
	builder::write(code, arr2_end);
}
void FuncEviroBuilder::arr_insert_rangeByVal(uint16_t arr, uint16_t arr2, uint16_t arr2_start, uint16_t arr2_end, uint16_t arr_pos, bool move, bool static_mode) {
	code.push_back(Command(Opcode::arr_op, false, static_mode).toCmd());
	builder::write(code, arr);
	OpArrFlags flags;
	flags.by_val_mode = true;
	flags.move_mode = move;
	code.push_back(flags.raw);
	code.push_back((uint8_t)OpcodeArray::insert_range);
	builder::write(code, arr2);
	builder::write(code, arr_pos);
	builder::write(code, arr2_start);
	builder::write(code, arr2_end);
}
void FuncEviroBuilder::arr_get(uint16_t arr, uint16_t to, uint64_t from, bool move, ArrCheckMode check_bounds, VType array_type) {
	if (array_type != VType::noting) {
		switch (array_type) {
		case VType::raw_arr_i8:
		case VType::raw_arr_i16:
		case VType::raw_arr_i32:
		case VType::raw_arr_i64:
		case VType::raw_arr_ui8:
		case VType::raw_arr_ui16:
		case VType::raw_arr_ui32:
		case VType::raw_arr_ui64:
		case VType::raw_arr_flo:
		case VType::raw_arr_doub:
		case VType::uarr:
		case VType::faarr:
		case VType::saarr:
		case VType::class_:
		case VType::morph:
		case VType::proxy:
			break;
		default:
			throw InvalidOperation("Unsupported operation to non array type and interface type");
		}
	}
	code.push_back(Command(Opcode::arr_op, false, array_type != VType::noting).toCmd());
	builder::write(code, arr);
	OpArrFlags flags;
	flags.by_val_mode = false;
	flags.move_mode = move;
	flags.checked = check_bounds;
	code.push_back(flags.raw);
	code.push_back((uint8_t)OpcodeArray::get);
	if (array_type != VType::noting)
		builder::write(code, array_type);
	builder::write(code, to);
	builder::write(code, from);
}
void FuncEviroBuilder::arr_getByVal(uint16_t arr, uint16_t to, uint16_t from, bool move, ArrCheckMode check_bounds, VType array_type) {
	if (array_type != VType::noting) {
		switch (array_type) {
		case VType::raw_arr_i8:
		case VType::raw_arr_i16:
		case VType::raw_arr_i32:
		case VType::raw_arr_i64:
		case VType::raw_arr_ui8:
		case VType::raw_arr_ui16:
		case VType::raw_arr_ui32:
		case VType::raw_arr_ui64:
		case VType::raw_arr_flo:
		case VType::raw_arr_doub:
		case VType::uarr:
		case VType::faarr:
		case VType::saarr:
		case VType::class_:
		case VType::morph:
		case VType::proxy:
			break;
		default:
			throw InvalidOperation("Unsupported operation to non array type and interface type");
		}
	}
	code.push_back(Command(Opcode::arr_op, false, array_type != VType::noting).toCmd());
	builder::write(code, arr);
	OpArrFlags flags;
	flags.by_val_mode = true;
	flags.move_mode = move;
	flags.checked = check_bounds;
	code.push_back(flags.raw);
	code.push_back((uint8_t)OpcodeArray::get);
	if (array_type != VType::noting)
		builder::write(code, array_type);
	builder::write(code, to);
	builder::write(code, from);
}
void FuncEviroBuilder::arr_take(uint16_t arr, uint16_t to, uint64_t from, bool move, bool static_mode) {
	code.push_back(Command(Opcode::arr_op, false, static_mode).toCmd());
	builder::write(code, arr);
	OpArrFlags flags;
	flags.by_val_mode = false;
	flags.move_mode = move;
	code.push_back(flags.raw);
	code.push_back((uint8_t)OpcodeArray::take);
	builder::write(code, to);
	builder::write(code, from);
}
void FuncEviroBuilder::arr_takeByVal(uint16_t arr, uint16_t to, uint16_t from, bool move, bool static_mode) {
	code.push_back(Command(Opcode::arr_op, false, static_mode).toCmd());
	builder::write(code, arr);
	OpArrFlags flags;
	flags.by_val_mode = true;
	flags.move_mode = move;
	code.push_back(flags.raw);
	code.push_back((uint8_t)OpcodeArray::take);
	builder::write(code, to);
	builder::write(code, from);
}
void FuncEviroBuilder::arr_take_end(uint16_t arr, uint16_t to, bool move, bool static_mode) {
	code.push_back(Command(Opcode::arr_op, false, static_mode).toCmd());
	builder::write(code, arr);
	OpArrFlags flags;
	flags.by_val_mode = true;
	flags.move_mode = move;
	code.push_back(flags.raw);
	code.push_back((uint8_t)OpcodeArray::take_end);
	builder::write(code, to);
}
void FuncEviroBuilder::arr_take_start(uint16_t arr, uint16_t to, bool move, bool static_mode) {
	code.push_back(Command(Opcode::arr_op, false, static_mode).toCmd());
	builder::write(code, arr);
	OpArrFlags flags;
	flags.by_val_mode = true;
	flags.move_mode = move;
	code.push_back(flags.raw);
	code.push_back((uint8_t)OpcodeArray::take_start);
	builder::write(code, to);
}
void FuncEviroBuilder::arr_get_range(uint16_t arr, uint16_t to, uint64_t start, uint64_t end, bool move, bool static_mode) {
	code.push_back(Command(Opcode::arr_op, false, static_mode).toCmd());
	builder::write(code, arr);
	OpArrFlags flags;
	flags.by_val_mode = false;
	flags.move_mode = move;
	code.push_back(flags.raw);
	code.push_back((uint8_t)OpcodeArray::get_range);
	builder::write(code, to);
	builder::write(code, start);
	builder::write(code, end);
}
void FuncEviroBuilder::arr_get_rangeByVal(uint16_t arr, uint16_t to, uint16_t start, uint16_t end, bool move, bool static_mode) {
	code.push_back(Command(Opcode::arr_op, false, static_mode).toCmd());
	builder::write(code, arr);
	OpArrFlags flags;
	flags.by_val_mode = true;
	flags.move_mode = move;
	code.push_back(flags.raw);
	code.push_back((uint8_t)OpcodeArray::get_range);
	builder::write(code, to);
	builder::write(code, start);
	builder::write(code, end);
}
void FuncEviroBuilder::arr_take_range(uint16_t arr, uint16_t to, uint64_t start, uint64_t end, bool move, bool static_mode) {
	code.push_back(Command(Opcode::arr_op, false, static_mode).toCmd());
	builder::write(code, arr);
	OpArrFlags flags;
	flags.by_val_mode = false;
	flags.move_mode = move;
	code.push_back(flags.raw);
	code.push_back((uint8_t)OpcodeArray::take_range);
	builder::write(code, to);
	builder::write(code, start);
	builder::write(code, end);
}
void FuncEviroBuilder::arr_take_rangeByVal(uint16_t arr, uint16_t to, uint16_t start, uint16_t end, bool move, bool static_mode) {
	code.push_back(Command(Opcode::arr_op, false, static_mode).toCmd());
	builder::write(code, arr);
	OpArrFlags flags;
	flags.by_val_mode = true;
	flags.move_mode = move;
	code.push_back(flags.raw);
	code.push_back((uint8_t)OpcodeArray::take_range);
	builder::write(code, to);
	builder::write(code, start);
	builder::write(code, end);
}
void FuncEviroBuilder::arr_pop_end(uint16_t arr, bool static_mode) {
	code.push_back(Command(Opcode::arr_op, false, static_mode).toCmd());
	builder::write(code, arr);
	code.push_back(0);
	code.push_back((uint8_t)OpcodeArray::pop_end);
}
void FuncEviroBuilder::arr_pop_start(uint16_t arr, bool static_mode) {
	code.push_back(Command(Opcode::arr_op, false, static_mode).toCmd());
	builder::write(code, arr);
	code.push_back(0);
	code.push_back((uint8_t)OpcodeArray::pop_start);
}
void FuncEviroBuilder::arr_remove_item(uint16_t arr, uint64_t in, bool static_mode) {
	code.push_back(Command(Opcode::arr_op, false, static_mode).toCmd());
	builder::write(code, arr);
	OpArrFlags flags;
	flags.by_val_mode = false;
	code.push_back(flags.raw);
	code.push_back((uint8_t)OpcodeArray::remove_item);
	builder::write(code, in);
}
void FuncEviroBuilder::arr_remove_itemByVal(uint16_t arr, uint16_t in, bool static_mode) {
	code.push_back(Command(Opcode::arr_op, false, static_mode).toCmd());
	builder::write(code, arr);
	OpArrFlags flags;
	flags.by_val_mode = true;
	code.push_back(flags.raw);
	code.push_back((uint8_t)OpcodeArray::remove_item);
	builder::write(code, in);
}
void FuncEviroBuilder::arr_remove_range(uint16_t arr, uint64_t start, uint64_t end, bool static_mode) {
	code.push_back(Command(Opcode::arr_op, false, static_mode).toCmd());
	builder::write(code, arr);
	OpArrFlags flags;
	flags.by_val_mode = false;
	code.push_back(flags.raw);
	code.push_back((uint8_t)OpcodeArray::remove_range);
	builder::write(code, start);
	builder::write(code, end);
}
void FuncEviroBuilder::arr_remove_rangeByVal(uint16_t arr, uint16_t start, uint16_t end, bool static_mode) {
	code.push_back(Command(Opcode::arr_op, false, static_mode).toCmd());
	builder::write(code, arr);
	OpArrFlags flags;
	flags.by_val_mode = true;
	code.push_back(flags.raw);
	code.push_back((uint8_t)OpcodeArray::remove_range);
	builder::write(code, start);
	builder::write(code, end);
}
void FuncEviroBuilder::arr_resize(uint16_t arr, uint64_t new_size, bool static_mode) {
	code.push_back(Command(Opcode::arr_op, false, static_mode).toCmd());
	builder::write(code, arr);
	OpArrFlags flags;
	flags.by_val_mode = false;
	code.push_back(flags.raw);
	code.push_back((uint8_t)OpcodeArray::resize);
	builder::write(code, new_size);
}
void FuncEviroBuilder::arr_resizeByVal(uint16_t arr, uint16_t new_size, bool static_mode) {
	code.push_back(Command(Opcode::arr_op, false, static_mode).toCmd());
	builder::write(code, arr);
	OpArrFlags flags;
	flags.by_val_mode = true;
	code.push_back(flags.raw);
	code.push_back((uint8_t)OpcodeArray::resize);
	builder::write(code, new_size);
}
void FuncEviroBuilder::arr_resize_default(uint16_t arr, uint64_t new_size, uint16_t default_init_val, bool static_mode) {
	code.push_back(Command(Opcode::arr_op, false, static_mode).toCmd());
	builder::write(code, arr);
	OpArrFlags flags;
	flags.by_val_mode = false;
	code.push_back(flags.raw);
	code.push_back((uint8_t)OpcodeArray::resize);
	builder::write(code, new_size);
	builder::write(code, default_init_val);
}
void FuncEviroBuilder::arr_resize_defaultByVal(uint16_t arr, uint16_t new_size, uint16_t default_init_val, bool static_mode) {
	code.push_back(Command(Opcode::arr_op, false, static_mode).toCmd());
	builder::write(code, arr);
	OpArrFlags flags;
	flags.by_val_mode = true;
	code.push_back(flags.raw);
	code.push_back((uint8_t)OpcodeArray::resize);
	builder::write(code, new_size);
	builder::write(code, default_init_val);
}
void FuncEviroBuilder::arr_reserve_push_end(uint16_t arr, uint64_t new_size, bool static_mode) {
	code.push_back(Command(Opcode::arr_op, false, static_mode).toCmd());
	builder::write(code, arr);
	OpArrFlags flags;
	flags.by_val_mode = false;
	code.push_back(flags.raw);
	code.push_back((uint8_t)OpcodeArray::reserve_push_end);
	builder::write(code, new_size);
}
void FuncEviroBuilder::arr_reserve_push_endByVal(uint16_t arr, uint16_t new_size, bool static_mode) {
	code.push_back(Command(Opcode::arr_op, false, static_mode).toCmd());
	builder::write(code, arr);
	OpArrFlags flags;
	flags.by_val_mode = true;
	code.push_back(flags.raw);
	code.push_back((uint8_t)OpcodeArray::reserve_push_end);
	builder::write(code, new_size);
}
void FuncEviroBuilder::arr_reserve_push_start(uint16_t arr, uint64_t new_size, bool static_mode) {
	code.push_back(Command(Opcode::arr_op, false, static_mode).toCmd());
	builder::write(code, arr);
	OpArrFlags flags;
	flags.by_val_mode = false;
	code.push_back(flags.raw);
	code.push_back((uint8_t)OpcodeArray::reserve_push_start);
	builder::write(code, new_size);
}
void FuncEviroBuilder::arr_reserve_push_startByVal(uint16_t arr, uint16_t new_size, bool static_mode) {
	code.push_back(Command(Opcode::arr_op, false, static_mode).toCmd());
	builder::write(code, arr);
	OpArrFlags flags;
	flags.by_val_mode = true;
	code.push_back(flags.raw);
	code.push_back((uint8_t)OpcodeArray::reserve_push_start);
	builder::write(code, new_size);
}
void FuncEviroBuilder::arr_commit(uint16_t arr, bool static_mode) {
	code.push_back(Command(Opcode::arr_op, false, static_mode).toCmd());
	builder::write(code, arr);
	code.push_back(0);
	code.push_back((uint8_t)OpcodeArray::commit);
}
void FuncEviroBuilder::arr_decommit(uint16_t arr, uint64_t blocks_count, bool static_mode) {
	code.push_back(Command(Opcode::arr_op, false, static_mode).toCmd());
	builder::write(code, arr);
	OpArrFlags flags;
	flags.by_val_mode = false;
	code.push_back(flags.raw);
	code.push_back((uint8_t)OpcodeArray::remove_item);
	builder::write(code, blocks_count);
}
void FuncEviroBuilder::arr_decommitByVal(uint16_t arr, uint16_t blocks_count, bool static_mode) {
	code.push_back(Command(Opcode::arr_op, false, static_mode).toCmd());
	builder::write(code, arr);
	OpArrFlags flags;
	flags.by_val_mode = true;
	code.push_back(flags.raw);
	code.push_back((uint8_t)OpcodeArray::remove_item);
	builder::write(code, blocks_count);
}
void FuncEviroBuilder::arr_remove_reserved(uint16_t arr, bool static_mode) {
	code.push_back(Command(Opcode::arr_op, false, static_mode).toCmd());
	builder::write(code, arr);
	code.push_back(0);
	code.push_back((uint8_t)OpcodeArray::remove_reserved);
}
void FuncEviroBuilder::arr_size(uint16_t arr, uint16_t set_to, bool static_mode) {
	code.push_back(Command(Opcode::arr_op, false, static_mode).toCmd());
	builder::write(code, arr);
	OpArrFlags flags;
	flags.by_val_mode = true;
	code.push_back(flags.raw);
	code.push_back((uint8_t)OpcodeArray::size);
	builder::write(code, set_to);
}
#pragma endregion
	//casm,
#pragma region interface
void FuncEviroBuilder::call_value_interface(ClassAccess access, uint16_t class_val, uint16_t fn_name, bool catch_ex, bool is_async) {
	code.push_back(Command(Opcode::call_value_function).toCmd());
	CallFlags f;
	f.in_memory = true;
	f.async_mode = is_async;
	f.use_result = false;
	f.except_catch = catch_ex;
	code.push_back(f.encoded);
	builder::write(code, fn_name);
	builder::write(code, class_val);
	builder::write(code, access);
}
void FuncEviroBuilder::call_value_interface(ClassAccess access, uint16_t class_val, uint16_t fn_name, uint16_t res_val, bool catch_ex, bool is_async) {
	code.push_back(Command(Opcode::call_value_function).toCmd());
	CallFlags f;
	f.in_memory = true;
	f.async_mode = is_async;
	f.use_result = true;
	f.except_catch = catch_ex;
	code.push_back(f.encoded);
	builder::write(code, fn_name);
	builder::write(code, class_val);
	builder::write(code, access);
	builder::write(code, res_val);
}
void FuncEviroBuilder::call_value_interface(ClassAccess access, uint16_t class_val, const std::string& fn_name, bool catch_ex, bool is_async) {
	code.push_back(Command(Opcode::call_value_function).toCmd());
	CallFlags f;
	f.in_memory = false;
	f.async_mode = is_async;
	f.use_result = false;
	f.except_catch = catch_ex;
	code.push_back(f.encoded);
	builder::writeString(code, fn_name);
	builder::write(code, class_val);
	builder::write(code, access);
}
void FuncEviroBuilder::call_value_interface(ClassAccess access, uint16_t class_val, const std::string& fn_name, uint16_t res_val, bool catch_ex, bool is_async) {
	code.push_back(Command(Opcode::call_value_function).toCmd());
	CallFlags f;
	f.in_memory = false;
	f.async_mode = is_async;
	f.use_result = true;
	f.except_catch = catch_ex;
	code.push_back(f.encoded);
	builder::writeString(code, fn_name);
	builder::write(code, class_val);
	builder::write(code, access);
	builder::write(code, res_val);
}
void FuncEviroBuilder::call_value_interface_and_ret(ClassAccess access, uint16_t class_val, uint16_t fn_name, bool catch_ex, bool is_async) {
	code.push_back(Command(Opcode::call_value_function_and_ret).toCmd());
	CallFlags f;
	f.in_memory = true;
	f.async_mode = is_async;
	f.use_result = false;
	f.except_catch = catch_ex;
	code.push_back(f.encoded);
	builder::write(code, fn_name);
	builder::write(code, class_val);
	builder::write(code, access);
}
void FuncEviroBuilder::call_value_interface_and_ret(ClassAccess access, uint16_t class_val, const std::string& fn_name, bool catch_ex, bool is_async) {
	code.push_back(Command(Opcode::call_value_function_and_ret).toCmd());
	CallFlags f;
	f.in_memory = false;
	f.async_mode = is_async;
	f.use_result = false;
	f.except_catch = catch_ex;
	code.push_back(f.encoded);
	builder::writeString(code, fn_name);
	builder::write(code, class_val);
	builder::write(code, access);
}
void FuncEviroBuilder::get_interface_value(ClassAccess access, uint16_t class_val, uint16_t val_name, uint16_t res) {
	code.push_back(Command(Opcode::get_structure_value).toCmd());
	code.push_back(0);
	builder::write(code, val_name);
	builder::write(code, class_val);
	builder::write(code, res);
}
void FuncEviroBuilder::get_interface_value(ClassAccess access, uint16_t class_val, const std::string& val_name, uint16_t res) {
	code.push_back(Command(Opcode::get_structure_value).toCmd());
	code.push_back(1);
	builder::writeString(code, val_name);
	builder::write(code, class_val);
	builder::write(code, res);
}
void FuncEviroBuilder::set_interface_value(ClassAccess access, uint16_t class_val, uint16_t val_name, uint16_t set_val) {
	code.push_back(Command(Opcode::set_structure_value).toCmd());
	code.push_back(0);
	builder::write(code, val_name);
	builder::write(code, class_val);
	builder::write(code, set_val);
}
void FuncEviroBuilder::set_interface_value(ClassAccess access, uint16_t class_val, const std::string& val_name, uint16_t set_val) {
	code.push_back(Command(Opcode::set_structure_value).toCmd());
	code.push_back(1);
	builder::writeString(code, val_name);
	builder::write(code, class_val);
	builder::write(code, set_val);
}
#pragma endregion
#pragma region misc
void FuncEviroBuilder::explicit_await(uint16_t await_value) {
	code.push_back(Command(Opcode::explicit_await).toCmd());
	builder::write(code, await_value);
}
#pragma endregion
#pragma endregion

typed_lgr<FuncEnviropment> FuncEviroBuilder::prepareFunc(bool can_be_unloaded) {
	//create header
	std::vector<uint8_t> fn;
	if (jump_pos.size() == 0) {
		builder::write(fn, 0ui8);
	}
	else if (jump_pos.size() >= UINT8_MAX) {
		builder::write(fn, 1ui8);
		builder::write(fn, (uint8_t)jump_pos.size());
	}
	else if (jump_pos.size() >= UINT16_MAX) {
		builder::write(fn, 2ui8);
		builder::write(fn, (uint16_t)jump_pos.size());
	}
	else if (jump_pos.size() >= UINT32_MAX) {
		builder::write(fn, 4ui8);
		builder::write(fn, (uint32_t)jump_pos.size());
	}
	else if (jump_pos.size() >= UINT64_MAX) {
		builder::write(fn, 8ui8);
		builder::write(fn, (uint64_t)jump_pos.size());
	}
	for(uint64_t it : jump_pos)
		builder::write(fn, it);

	fn.insert(fn.end(),code.begin(), code.end());

	return new FuncEnviropment(fn, local_funs, max_values, can_be_unloaded);
}
void FuncEviroBuilder::loadFunc(const std::string& str,bool can_be_unloaded) {
	FuncEnviropment::Load(prepareFunc(can_be_unloaded), str);
}
