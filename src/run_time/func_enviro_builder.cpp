// Copyright Danyil Melnytskyi 2022-2023
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <stdint.h>
#include "util/tools.hpp"
#include "func_enviro_builder.hpp"
#include "attacha_abi.hpp"
using namespace art;
#pragma region FuncEnviroBuilder

ValueIndexPos FuncEnviroBuilder::create_constant(const ValueItem& val){
	if(unify_constants){
		uint32_t i = 0;
		for (auto& v : all_constants) {
			if (v == val)
				return ValueIndexPos(i, ValuePos::in_constants);
			i++;
		}
	}
	if(constants_values > UINT16_MAX)
		throw CompileTimeException("unaddressable constant value");
	all_constants.push_front(val);
	if(use_dynamic_values && !strict_mode){
		flags.run_time_computable = true;
		dynamic_values.push_back(val);
	}
	else{
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
	builder::write(code, Command(Opcode::remove,false,true));
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
	sum(val0, val1);//TO-DO
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
	minus(val0, val1);//TO-DO
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
	div(val0, val1);//TO-DO
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
void FuncEnviroBuilder::mul(ValueIndexPos val0, ValueIndexPos val1, ValueMeta m0, ValueMeta m1){
	mul(val0, val1);//TO-DO
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
	mul(val0, val1);//TO-DO
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
	bit_xor(val0, val1);//TO-DO
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
	bit_or(val0, val1);//TO-DO
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
	bit_and(val0, val1);//TO-DO
	useVal(val0);
	useVal(val1);
}

void FuncEnviroBuilder::bit_not(ValueIndexPos val0) {
	builder::write(code, Command(Opcode::bit_not));
	builder::writeIndexPos(code, val0);
	useVal(val0);
}
void FuncEnviroBuilder::bit_not(ValueIndexPos val, ValueMeta m) {
	bit_not(val);//TO-DO
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
	compare(val0, val1);//TO-DO
	useVal(val0);
	useVal(val1);
}

void FuncEnviroBuilder::jump(JumpCondition cd, const std::string& label_name) {
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
	builder::write(code, Command(Opcode::call,false, fn_mem_only_str));
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


uint32_t FuncEnviroBuilder::add_local_fn(typed_lgr<FuncEnvironment> fn) {
	uint32_t res = static_cast<uint32_t>(local_funs.size());
	if (res != local_funs.size())
		throw CompileTimeException("too many local funcs");
	if(fn->get_cross_code().empty()){
		if(strict_mode)
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
void FuncEnviroBuilder::call_local_idx(uint32_t fn, bool is_async) {
	call_local(create_constant(fn), is_async);
}
void FuncEnviroBuilder::call_local_idx(uint32_t fn, ValueIndexPos res, bool is_async) {
	call_local(create_constant(fn), res, is_async);
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
void FuncEnviroBuilder::call_local_and_ret_idx(uint32_t fn, bool is_async) {
	call_local_and_ret(create_constant(fn), is_async);
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
void FuncEnviroBuilder::is_gc(ValueIndexPos val){
	builder::write(code, Command(Opcode::is_gc));
	code.push_back(false);
	builder::writeIndexPos(code, val);
	useVal(val);
}
void FuncEnviroBuilder::is_gc(ValueIndexPos val, ValueIndexPos result){
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

void FuncEnviroBuilder::inline_native_opcode(uint8_t* opcode, uint32_t len){
	builder::write(code, Command(Opcode::inline_native));
	builder::write(code, len);
	code.insert(code.end(), opcode, opcode + len);
}

void FuncEnviroBuilder::bind_pos(const std::string& name) {
	jump_pos[jumpMap(name)] = code.size();
}
#pragma region arr_op
void FuncEnviroBuilder::arr_set(ValueIndexPos arr, ValueIndexPos from, uint64_t to, bool move, ArrCheckMode check_bounds, VType array_type) {
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
		case VType::struct_:
			break;
		default:
			throw InvalidOperation("Unsupported operation to non array type and interface type");
		}
	}
	builder::write(code, Command(Opcode::arr_op, false, array_type != VType::noting));
	builder::writeIndexPos(code ,arr);
	OpArrFlags flags;
	flags.by_val_mode = false;
	flags.move_mode = move;
	flags.checked = check_bounds;
	code.push_back(flags.raw);
	code.push_back((uint8_t)OpcodeArray::set);
	if(array_type != VType::noting)
		builder::write(code, array_type);
	builder::writeIndexPos(code, from);
	builder::write(code, to);
}
void FuncEnviroBuilder::arr_setByVal(ValueIndexPos arr, ValueIndexPos from, ValueIndexPos to, bool move, ArrCheckMode check_bounds, VType array_type) {
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
		case VType::struct_:
			break;
		default:
			throw InvalidOperation("Unsupported operation to non array type and interface type");
		}
	}
	builder::write(code, Command(Opcode::arr_op, false, array_type != VType::noting));
	builder::writeIndexPos(code, arr);
	OpArrFlags flags;
	flags.by_val_mode = true;
	flags.move_mode = move;
	flags.checked = check_bounds;
	code.push_back(flags.raw);
	code.push_back((uint8_t)OpcodeArray::set);
	if (array_type != VType::noting)
		builder::write(code, array_type);
	builder::writeIndexPos(code, from);
	builder::writeIndexPos(code, to);
}
void FuncEnviroBuilder::arr_insert(ValueIndexPos arr, ValueIndexPos from, uint64_t to, bool move, bool static_mode) {
	builder::write(code, Command(Opcode::arr_op, false, static_mode));
	builder::writeIndexPos(code, arr);
	OpArrFlags flags;
	flags.by_val_mode = false;
	flags.move_mode = move;
	code.push_back(flags.raw);
	code.push_back((uint8_t)OpcodeArray::insert);
	builder::writeIndexPos(code, from);
	builder::write(code, to);
}
void FuncEnviroBuilder::arr_insertByVal(ValueIndexPos arr, ValueIndexPos from, ValueIndexPos to, bool move, bool static_mode) {
	builder::write(code, Command(Opcode::arr_op, false, static_mode));
	builder::writeIndexPos(code, arr);
	OpArrFlags flags;
	flags.by_val_mode = true;
	flags.move_mode = move;
	code.push_back(flags.raw);
	code.push_back((uint8_t)OpcodeArray::insert);
	builder::writeIndexPos(code, from);
	builder::writeIndexPos(code, to);
}
void FuncEnviroBuilder::arr_push_end(ValueIndexPos arr, ValueIndexPos from, bool move, bool static_mode) {
	builder::write(code, Command(Opcode::arr_op, false, static_mode));
	builder::writeIndexPos(code, arr);
	OpArrFlags flags;
	flags.move_mode = move;
	code.push_back(flags.raw);
	code.push_back((uint8_t)OpcodeArray::push_end);
	builder::writeIndexPos(code, from);
}
void FuncEnviroBuilder::arr_push_start(ValueIndexPos arr, ValueIndexPos from, bool move, bool static_mode) {
	builder::write(code, Command(Opcode::arr_op, false, static_mode));
	builder::writeIndexPos(code, arr);
	OpArrFlags flags;
	flags.move_mode = move;
	code.push_back(flags.raw);
	code.push_back((uint8_t)OpcodeArray::push_start);
	builder::writeIndexPos(code, from);
}
void FuncEnviroBuilder::arr_insert_range(ValueIndexPos arr, ValueIndexPos arr2, uint64_t arr2_start, uint64_t arr2_end, uint64_t arr_pos, bool move, bool static_mode) {
	builder::write(code, Command(Opcode::arr_op, false, static_mode));
	builder::writeIndexPos(code, arr);
	OpArrFlags flags;
	flags.by_val_mode = false;
	flags.move_mode = move;
	code.push_back(flags.raw);
	code.push_back((uint8_t)OpcodeArray::insert_range);
	builder::writeIndexPos(code, arr2);
	builder::write(code, arr_pos);
	builder::write(code, arr2_start);
	builder::write(code, arr2_end);
}
void FuncEnviroBuilder::arr_insert_rangeByVal(ValueIndexPos arr, ValueIndexPos arr2, ValueIndexPos arr2_start, ValueIndexPos arr2_end, ValueIndexPos arr_pos, bool move, bool static_mode) {
	builder::write(code, Command(Opcode::arr_op, false, static_mode));
	builder::writeIndexPos(code, arr);
	OpArrFlags flags;
	flags.by_val_mode = true;
	flags.move_mode = move;
	code.push_back(flags.raw);
	code.push_back((uint8_t)OpcodeArray::insert_range);
	builder::writeIndexPos(code, arr2);
	builder::writeIndexPos(code, arr_pos);
	builder::writeIndexPos(code, arr2_start);
	builder::writeIndexPos(code, arr2_end);
}
void FuncEnviroBuilder::arr_get(ValueIndexPos arr, ValueIndexPos to, uint64_t from, bool move, ArrCheckMode check_bounds, VType array_type) {
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
		case VType::struct_:
			break;
		default:
			throw InvalidOperation("Unsupported operation to non array type and interface type");
		}
	}
	builder::write(code, Command(Opcode::arr_op, false, array_type != VType::noting));
	builder::writeIndexPos(code, arr);
	OpArrFlags flags;
	flags.by_val_mode = false;
	flags.move_mode = move;
	flags.checked = check_bounds;
	code.push_back(flags.raw);
	code.push_back((uint8_t)OpcodeArray::get);
	if (array_type != VType::noting)
		builder::write(code, array_type);
	builder::writeIndexPos(code, to);
	builder::write(code, from);
}
void FuncEnviroBuilder::arr_getByVal(ValueIndexPos arr, ValueIndexPos to, ValueIndexPos from, bool move, ArrCheckMode check_bounds, VType array_type) {
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
		case VType::struct_:
			break;
		default:
			throw InvalidOperation("Unsupported operation to non array type and interface type");
		}
	}
	builder::write(code, Command(Opcode::arr_op, false, array_type != VType::noting));
	builder::writeIndexPos(code, arr);
	OpArrFlags flags;
	flags.by_val_mode = true;
	flags.move_mode = move;
	flags.checked = check_bounds;
	code.push_back(flags.raw);
	code.push_back((uint8_t)OpcodeArray::get);
	if (array_type != VType::noting)
		builder::write(code, array_type);
	builder::writeIndexPos(code, to);
	builder::writeIndexPos(code, from);
}
void FuncEnviroBuilder::arr_take(ValueIndexPos arr, ValueIndexPos to, uint64_t from, bool move, bool static_mode) {
	builder::write(code, Command(Opcode::arr_op, false, static_mode));
	builder::writeIndexPos(code, arr);
	OpArrFlags flags;
	flags.by_val_mode = false;
	flags.move_mode = move;
	code.push_back(flags.raw);
	code.push_back((uint8_t)OpcodeArray::take);
	builder::writeIndexPos(code, to);
	builder::write(code, from);
}
void FuncEnviroBuilder::arr_takeByVal(ValueIndexPos arr, ValueIndexPos to, ValueIndexPos from, bool move, bool static_mode) {
	builder::write(code, Command(Opcode::arr_op, false, static_mode));
	builder::writeIndexPos(code, arr);
	OpArrFlags flags;
	flags.by_val_mode = true;
	flags.move_mode = move;
	code.push_back(flags.raw);
	code.push_back((uint8_t)OpcodeArray::take);
	builder::writeIndexPos(code, to);
	builder::writeIndexPos(code, from);
}
void FuncEnviroBuilder::arr_take_end(ValueIndexPos arr, ValueIndexPos to, bool move, bool static_mode) {
	builder::write(code, Command(Opcode::arr_op, false, static_mode));
	builder::writeIndexPos(code, arr);
	OpArrFlags flags;
	flags.by_val_mode = true;
	flags.move_mode = move;
	code.push_back(flags.raw);
	code.push_back((uint8_t)OpcodeArray::take_end);
	builder::writeIndexPos(code, to);
}
void FuncEnviroBuilder::arr_take_start(ValueIndexPos arr, ValueIndexPos to, bool move, bool static_mode) {
	builder::write(code, Command(Opcode::arr_op, false, static_mode));
	builder::writeIndexPos(code, arr);
	OpArrFlags flags;
	flags.by_val_mode = true;
	flags.move_mode = move;
	code.push_back(flags.raw);
	code.push_back((uint8_t)OpcodeArray::take_start);
	builder::writeIndexPos(code, to);
}
void FuncEnviroBuilder::arr_get_range(ValueIndexPos arr, ValueIndexPos to, uint64_t start, uint64_t end, bool move, bool static_mode) {
	builder::write(code, Command(Opcode::arr_op, false, static_mode));
	builder::writeIndexPos(code, arr);
	OpArrFlags flags;
	flags.by_val_mode = false;
	flags.move_mode = move;
	code.push_back(flags.raw);
	code.push_back((uint8_t)OpcodeArray::get_range);
	builder::writeIndexPos(code, to);
	builder::write(code, start);
	builder::write(code, end);
}
void FuncEnviroBuilder::arr_get_rangeByVal(ValueIndexPos arr, ValueIndexPos to, ValueIndexPos start, ValueIndexPos end, bool move, bool static_mode) {
	builder::write(code, Command(Opcode::arr_op, false, static_mode));
	builder::writeIndexPos(code, arr);
	OpArrFlags flags;
	flags.by_val_mode = true;
	flags.move_mode = move;
	code.push_back(flags.raw);
	code.push_back((uint8_t)OpcodeArray::get_range);
	builder::writeIndexPos(code, to);
	builder::writeIndexPos(code, start);
	builder::writeIndexPos(code, end);
}
void FuncEnviroBuilder::arr_take_range(ValueIndexPos arr, ValueIndexPos to, uint64_t start, uint64_t end, bool move, bool static_mode) {
	builder::write(code, Command(Opcode::arr_op, false, static_mode));
	builder::writeIndexPos(code, arr);
	OpArrFlags flags;
	flags.by_val_mode = false;
	flags.move_mode = move;
	code.push_back(flags.raw);
	code.push_back((uint8_t)OpcodeArray::take_range);
	builder::writeIndexPos(code, to);
	builder::write(code, start);
	builder::write(code, end);
}
void FuncEnviroBuilder::arr_take_rangeByVal(ValueIndexPos arr, ValueIndexPos to, ValueIndexPos start, ValueIndexPos end, bool move, bool static_mode) {
	builder::write(code, Command(Opcode::arr_op, false, static_mode));
	builder::writeIndexPos(code, arr);
	OpArrFlags flags;
	flags.by_val_mode = true;
	flags.move_mode = move;
	code.push_back(flags.raw);
	code.push_back((uint8_t)OpcodeArray::take_range);
	builder::writeIndexPos(code, to);
	builder::writeIndexPos(code, start);
	builder::writeIndexPos(code, end);
}
void FuncEnviroBuilder::arr_pop_end(ValueIndexPos arr, bool static_mode) {
	builder::write(code, Command(Opcode::arr_op, false, static_mode));
	builder::writeIndexPos(code, arr);
	code.push_back(0);
	code.push_back((uint8_t)OpcodeArray::pop_end);
}
void FuncEnviroBuilder::arr_pop_start(ValueIndexPos arr, bool static_mode) {
	builder::write(code, Command(Opcode::arr_op, false, static_mode));
	builder::writeIndexPos(code, arr);
	code.push_back(0);
	code.push_back((uint8_t)OpcodeArray::pop_start);
}
void FuncEnviroBuilder::arr_remove_item(ValueIndexPos arr, uint64_t in, bool static_mode) {
	builder::write(code, Command(Opcode::arr_op, false, static_mode));
	builder::writeIndexPos(code, arr);
	OpArrFlags flags;
	flags.by_val_mode = false;
	code.push_back(flags.raw);
	code.push_back((uint8_t)OpcodeArray::remove_item);
	builder::write(code, in);
}
void FuncEnviroBuilder::arr_remove_itemByVal(ValueIndexPos arr, ValueIndexPos in, bool static_mode) {
	builder::write(code, Command(Opcode::arr_op, false, static_mode));
	builder::writeIndexPos(code, arr);
	OpArrFlags flags;
	flags.by_val_mode = true;
	code.push_back(flags.raw);
	code.push_back((uint8_t)OpcodeArray::remove_item);
	builder::writeIndexPos(code, in);
}
void FuncEnviroBuilder::arr_remove_range(ValueIndexPos arr, uint64_t start, uint64_t end, bool static_mode) {
	builder::write(code, Command(Opcode::arr_op, false, static_mode));
	builder::writeIndexPos(code, arr);
	OpArrFlags flags;
	flags.by_val_mode = false;
	code.push_back(flags.raw);
	code.push_back((uint8_t)OpcodeArray::remove_range);
	builder::write(code, start);
	builder::write(code, end);
}
void FuncEnviroBuilder::arr_remove_rangeByVal(ValueIndexPos arr, ValueIndexPos start, ValueIndexPos end, bool static_mode) {
	builder::write(code, Command(Opcode::arr_op, false, static_mode));
	builder::writeIndexPos(code, arr);
	OpArrFlags flags;
	flags.by_val_mode = true;
	code.push_back(flags.raw);
	code.push_back((uint8_t)OpcodeArray::remove_range);
	builder::writeIndexPos(code, start);
	builder::writeIndexPos(code, end);
}
void FuncEnviroBuilder::arr_resize(ValueIndexPos arr, uint64_t new_size, bool static_mode) {
	builder::write(code, Command(Opcode::arr_op, false, static_mode));
	builder::writeIndexPos(code, arr);
	OpArrFlags flags;
	flags.by_val_mode = false;
	code.push_back(flags.raw);
	code.push_back((uint8_t)OpcodeArray::resize);
	builder::write(code, new_size);
}
void FuncEnviroBuilder::arr_resizeByVal(ValueIndexPos arr, ValueIndexPos new_size, bool static_mode) {
	builder::write(code, Command(Opcode::arr_op, false, static_mode));
	builder::writeIndexPos(code, arr);
	OpArrFlags flags;
	flags.by_val_mode = true;
	code.push_back(flags.raw);
	code.push_back((uint8_t)OpcodeArray::resize);
	builder::writeIndexPos(code, new_size);
}
void FuncEnviroBuilder::arr_resize_default(ValueIndexPos arr, uint64_t new_size, ValueIndexPos default_init_val, bool static_mode) {
	builder::write(code, Command(Opcode::arr_op, false, static_mode));
	builder::writeIndexPos(code, arr);
	OpArrFlags flags;
	flags.by_val_mode = false;
	code.push_back(flags.raw);
	code.push_back((uint8_t)OpcodeArray::resize_default);
	builder::write(code, new_size);
	builder::writeIndexPos(code, default_init_val);
}
void FuncEnviroBuilder::arr_resize_defaultByVal(ValueIndexPos arr, ValueIndexPos new_size, ValueIndexPos default_init_val, bool static_mode) {
	builder::write(code, Command(Opcode::arr_op, false, static_mode));
	builder::writeIndexPos(code, arr);
	OpArrFlags flags;
	flags.by_val_mode = true;
	code.push_back(flags.raw);
	code.push_back((uint8_t)OpcodeArray::resize_default);
	builder::writeIndexPos(code, new_size);
	builder::writeIndexPos(code, default_init_val);
}
void FuncEnviroBuilder::arr_reserve_push_end(ValueIndexPos arr, uint64_t new_size, bool static_mode) {
	builder::write(code, Command(Opcode::arr_op, false, static_mode));
	builder::writeIndexPos(code, arr);
	OpArrFlags flags;
	flags.by_val_mode = false;
	code.push_back(flags.raw);
	code.push_back((uint8_t)OpcodeArray::reserve_push_end);
	builder::write(code, new_size);
}
void FuncEnviroBuilder::arr_reserve_push_endByVal(ValueIndexPos arr, ValueIndexPos new_size, bool static_mode) {
	builder::write(code, Command(Opcode::arr_op, false, static_mode));
	builder::writeIndexPos(code, arr);
	OpArrFlags flags;
	flags.by_val_mode = true;
	code.push_back(flags.raw);
	code.push_back((uint8_t)OpcodeArray::reserve_push_end);
	builder::writeIndexPos(code, new_size);
}
void FuncEnviroBuilder::arr_reserve_push_start(ValueIndexPos arr, uint64_t new_size, bool static_mode) {
	builder::write(code, Command(Opcode::arr_op, false, static_mode));
	builder::writeIndexPos(code, arr);
	OpArrFlags flags;
	flags.by_val_mode = false;
	code.push_back(flags.raw);
	code.push_back((uint8_t)OpcodeArray::reserve_push_start);
	builder::write(code, new_size);
}
void FuncEnviroBuilder::arr_reserve_push_startByVal(ValueIndexPos arr, ValueIndexPos new_size, bool static_mode) {
	builder::write(code, Command(Opcode::arr_op, false, static_mode));
	builder::writeIndexPos(code, arr);
	OpArrFlags flags;
	flags.by_val_mode = true;
	code.push_back(flags.raw);
	code.push_back((uint8_t)OpcodeArray::reserve_push_start);
	builder::writeIndexPos(code, new_size);
}
void FuncEnviroBuilder::arr_commit(ValueIndexPos arr, bool static_mode) {
	builder::write(code, Command(Opcode::arr_op, false, static_mode));
	builder::writeIndexPos(code, arr);
	code.push_back(0);
	code.push_back((uint8_t)OpcodeArray::commit);
}
void FuncEnviroBuilder::arr_decommit(ValueIndexPos arr, uint64_t blocks_count, bool static_mode) {
	builder::write(code, Command(Opcode::arr_op, false, static_mode));
	builder::writeIndexPos(code, arr);
	OpArrFlags flags;
	flags.by_val_mode = false;
	code.push_back(flags.raw);
	code.push_back((uint8_t)OpcodeArray::remove_item);
	builder::write(code, blocks_count);
}
void FuncEnviroBuilder::arr_decommitByVal(ValueIndexPos arr, ValueIndexPos blocks_count, bool static_mode) {
	builder::write(code, Command(Opcode::arr_op, false, static_mode));
	builder::writeIndexPos(code, arr);
	OpArrFlags flags;
	flags.by_val_mode = true;
	code.push_back(flags.raw);
	code.push_back((uint8_t)OpcodeArray::remove_item);
	builder::writeIndexPos(code, blocks_count);
}
void FuncEnviroBuilder::arr_remove_reserved(ValueIndexPos arr, bool static_mode) {
	builder::write(code, Command(Opcode::arr_op, false, static_mode));
	builder::writeIndexPos(code, arr);
	code.push_back(0);
	code.push_back((uint8_t)OpcodeArray::remove_reserved);
}
void FuncEnviroBuilder::arr_size(ValueIndexPos arr, ValueIndexPos set_to, bool static_mode) {
	builder::write(code, Command(Opcode::arr_op, false, static_mode));
	builder::writeIndexPos(code, arr);
	OpArrFlags flags;
	flags.by_val_mode = true;
	code.push_back(flags.raw);
	code.push_back((uint8_t)OpcodeArray::size);
	builder::writeIndexPos(code, set_to);
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
void FuncEnviroBuilder::call_value_interface_id(ValueIndexPos class_val, uint64_t class_fun_id, bool is_async){
	builder::write(code, Command(Opcode::call_value_function));
	CallFlags f;
	f.async_mode = is_async;
	f.use_result = false;
	code.push_back(f.encoded);
	builder::write(code, class_fun_id);
	builder::writeIndexPos(code, class_val);
}
void FuncEnviroBuilder::call_value_interface_id(ValueIndexPos class_val, uint64_t class_fun_id, ValueIndexPos res_val, bool is_async){
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
void FuncEnviroBuilder::call_value_interface_id_and_ret(ValueIndexPos class_val, uint64_t class_fun_id, bool is_async){
	builder::write(code, Command(Opcode::call_value_function_id_and_ret));
	CallFlags f;
	f.async_mode = is_async;
	f.use_result = false;
	code.push_back(f.encoded);
	builder::write(code, class_fun_id);
	builder::writeIndexPos(code, class_val);
}


void FuncEnviroBuilder::static_call_value_interface(ClassAccess access, ValueIndexPos class_val, ValueIndexPos fn_name, bool is_async){
	builder::write(code, Command(Opcode::static_call_value_function));
	CallFlags f;
	f.async_mode = is_async;
	f.use_result = false;
	code.push_back(f.encoded);
	builder::writeIndexPos(code, fn_name);
	builder::writeIndexPos(code, class_val);
	builder::write(code, access);
}
void FuncEnviroBuilder::static_call_value_interface(ClassAccess access, ValueIndexPos class_val, ValueIndexPos fn_name, ValueIndexPos res_val, bool is_async){
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
void FuncEnviroBuilder::static_call_value_interface_id(ValueIndexPos class_val, uint64_t class_fun_id, bool is_async){
	builder::write(code, Command(Opcode::static_call_value_function_id));
	CallFlags f;
	f.async_mode = is_async;
	f.use_result = false;
	code.push_back(f.encoded);
	builder::write(code, class_fun_id);
	builder::writeIndexPos(code, class_val);
}
void FuncEnviroBuilder::static_call_value_interface_id(ValueIndexPos class_val, uint64_t class_fun_id, ValueIndexPos res_val, bool is_async){
	builder::write(code, Command(Opcode::static_call_value_function_id));
	CallFlags f;
	f.async_mode = is_async;
	f.use_result = true;
	code.push_back(f.encoded);
	builder::write(code, class_fun_id);
	builder::writeIndexPos(code, class_val);
	builder::writeIndexPos(code, res_val);
}


void FuncEnviroBuilder::static_call_value_interface_and_ret(ClassAccess access, ValueIndexPos class_val, ValueIndexPos fn_name, bool is_async){
	builder::write(code, Command(Opcode::static_call_value_function_and_ret));
	CallFlags f;
	f.async_mode = is_async;
	f.use_result = false;
	code.push_back(f.encoded);
	builder::writeIndexPos(code, fn_name);
	builder::writeIndexPos(code, class_val);
	builder::write(code, access);
}
void FuncEnviroBuilder::static_call_value_interface_id_and_ret(ValueIndexPos class_val, uint64_t class_fun_id, bool is_async){
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

void FuncEnviroBuilder::to_gc(ValueIndexPos val){
	builder::write(code, Command(Opcode::to_gc));
	builder::writeIndexPos(code, val);
}
void FuncEnviroBuilder::localize_gc(ValueIndexPos val){
	builder::write(code, Command(Opcode::localize_gc));
	builder::writeIndexPos(code, val);
}
void FuncEnviroBuilder::from_gc(ValueIndexPos val){
	builder::write(code, Command(Opcode::from_gc));
	builder::writeIndexPos(code, val);
}
union xarray_slice_flags{
	enum class _type : uint8_t {
		all_inlined = 1,
		inl_0_ref_1 = 2,
		ref_0_inl_1 = 3,
		all_ref = 4
	};
	enum class _use_index_pos: uint8_t {
		none = 0,
		from = 1,
		to = 2,
		from_to = 3
	};
	struct{
		_type type : 4;
		_use_index_pos use_index_pos : 4;
	};
	xarray_slice_flags(_type t, _use_index_pos p) {
		type = t;
		use_index_pos = p;
	}
	uint8_t encoded;
};

void FuncEnviroBuilder::xarray_slice(ValueIndexPos result, ValueIndexPos val){
	builder::write(code, Command(Opcode::xarray_slice));
	builder::writeIndexPos(code, result);
	builder::writeIndexPos(code, val);
	code.push_back(xarray_slice_flags(xarray_slice_flags::_type::all_inlined, xarray_slice_flags::_use_index_pos::none).encoded);
}
void FuncEnviroBuilder::xarray_slice(ValueIndexPos result,ValueIndexPos val, uint32_t from){
	builder::write(code, Command(Opcode::xarray_slice));
	builder::writeIndexPos(code, result);
	builder::writeIndexPos(code, val);
	code.push_back(xarray_slice_flags(xarray_slice_flags::_type::all_inlined, xarray_slice_flags::_use_index_pos::from).encoded);
	builder::write(code, from);
}
void FuncEnviroBuilder::xarray_slice(ValueIndexPos result,ValueIndexPos val, ValueIndexPos from){
	builder::write(code, Command(Opcode::xarray_slice));
	builder::writeIndexPos(code, result);
	builder::writeIndexPos(code, val);
	code.push_back(xarray_slice_flags(xarray_slice_flags::_type::ref_0_inl_1, xarray_slice_flags::_use_index_pos::from).encoded);
	builder::writeIndexPos(code, from);
}
void FuncEnviroBuilder::xarray_slice(ValueIndexPos result,ValueIndexPos val, bool unused, uint32_t to){
	builder::write(code, Command(Opcode::xarray_slice));
	builder::writeIndexPos(code, result);
	builder::writeIndexPos(code, val);
	code.push_back(xarray_slice_flags(xarray_slice_flags::_type::all_inlined, xarray_slice_flags::_use_index_pos::to).encoded);
	builder::write(code, to);
}
void FuncEnviroBuilder::xarray_slice(ValueIndexPos result,ValueIndexPos val, bool unused, ValueIndexPos to){
	builder::write(code, Command(Opcode::xarray_slice));
	builder::writeIndexPos(code, result);
	builder::writeIndexPos(code, val);
	code.push_back(xarray_slice_flags(xarray_slice_flags::_type::inl_0_ref_1, xarray_slice_flags::_use_index_pos::to).encoded);
	builder::writeIndexPos(code, to);
}
void FuncEnviroBuilder::xarray_slice(ValueIndexPos result,ValueIndexPos val, uint32_t from, uint32_t to){
	builder::write(code, Command(Opcode::xarray_slice));
	builder::writeIndexPos(code, result);
	builder::writeIndexPos(code, val);
	code.push_back(xarray_slice_flags(xarray_slice_flags::_type::all_inlined, xarray_slice_flags::_use_index_pos::from_to).encoded);
	builder::write(code, from);
	builder::write(code, to);
}
void FuncEnviroBuilder::xarray_slice(ValueIndexPos result,ValueIndexPos val, uint32_t from, ValueIndexPos to){
	builder::write(code, Command(Opcode::xarray_slice));
	builder::writeIndexPos(code, result);
	builder::writeIndexPos(code, val);
	code.push_back(xarray_slice_flags(xarray_slice_flags::_type::inl_0_ref_1, xarray_slice_flags::_use_index_pos::from_to).encoded);
	builder::write(code, from);
	builder::writeIndexPos(code, to);
}
void FuncEnviroBuilder::xarray_slice(ValueIndexPos result,ValueIndexPos val, ValueIndexPos from, uint32_t to){
	builder::write(code, Command(Opcode::xarray_slice));
	builder::writeIndexPos(code, result);
	builder::writeIndexPos(code, val);
	code.push_back(xarray_slice_flags(xarray_slice_flags::_type::ref_0_inl_1, xarray_slice_flags::_use_index_pos::from_to).encoded);
	builder::writeIndexPos(code, from);
	builder::write(code, to);
}
void FuncEnviroBuilder::xarray_slice(ValueIndexPos result,ValueIndexPos val, ValueIndexPos from, ValueIndexPos to){
	builder::write(code, Command(Opcode::xarray_slice));
	builder::writeIndexPos(code, result);
	builder::writeIndexPos(code, val);
	code.push_back(xarray_slice_flags(xarray_slice_flags::_type::all_ref, xarray_slice_flags::_use_index_pos::from_to).encoded);
	builder::writeIndexPos(code, from);
	builder::writeIndexPos(code, to);
}

void FuncEnviroBuilder::table_jump(
	std::vector<std::string> table, 
	ValueIndexPos index,
	bool is_signed,
	TableJumpCheckFailAction too_large,
	const std::string& too_large_label,
	TableJumpCheckFailAction too_small,
	const std::string& too_small_label
){
	if((uint32_t)table.size() != table.size())
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

void FuncEnviroBuilder::get_reference(ValueIndexPos res, ValueIndexPos val){
	builder::write(code, Command(Opcode::get_reference));
	builder::writeIndexPos(code, res);
	builder::writeIndexPos(code, val);
}
void FuncEnviroBuilder::make_as_const(ValueIndexPos res){
	builder::write(code, Command(Opcode::make_as_const));
	builder::writeIndexPos(code, res);
}
void FuncEnviroBuilder::remove_const_protect(ValueIndexPos res){
	builder::write(code, Command(Opcode::remove_const_protect));
	builder::writeIndexPos(code, res);
}
void FuncEnviroBuilder::copy_un_constant(ValueIndexPos res, ValueIndexPos val){
	builder::write(code, Command(Opcode::copy_un_constant));
	builder::writeIndexPos(code, res);
	builder::writeIndexPos(code, val);
}
void FuncEnviroBuilder::copy_un_reference(ValueIndexPos res, ValueIndexPos val){
	builder::write(code, Command(Opcode::copy_un_reference));
	builder::writeIndexPos(code, res);
	builder::writeIndexPos(code, val);
}
void FuncEnviroBuilder::move_un_reference(ValueIndexPos res, ValueIndexPos val){
	builder::write(code, Command(Opcode::move_un_reference));
	builder::writeIndexPos(code, res);
	builder::writeIndexPos(code, val);
}
void FuncEnviroBuilder::remove_qualifiers(ValueIndexPos res){
	builder::write(code, Command(Opcode::remove_qualifiers));
	builder::writeIndexPos(code, res);
}
#pragma endregion
#pragma endregion

typed_lgr<FuncEnvironment> FuncEnviroBuilder::O_prepare_func() {
	if(!flags.run_time_computable)
		return new FuncEnvironment(O_build_func(), flags.can_be_unloaded, flags.is_cheap);
	else{
		std::vector<uint8_t> fn;
		builder::write(fn, flags);
		if(flags.used_static)
			builder::write(fn, static_values);
		if(flags.used_enviro_vals)
			builder::write(fn, values);
		if(flags.used_arguments)
			builder::write(fn, args_values);
		builder::writePackedLen(fn, constants_values);
		
		builder::writePackedLen(fn, jump_pos.size());
		for(uint64_t it : jump_pos)
			builder::write(fn, it);

		fn.insert(fn.end(),code.begin(), code.end());
		*(uint64_t*)(fn.data()) = fn.size();
		all_constants.clear();
		return new FuncEnvironment(std::move(fn), std::move(dynamic_values), std::move(local_funs), flags.can_be_unloaded, flags.is_cheap);
	}
}
FuncEnviroBuilder& FuncEnviroBuilder::O_flag_can_be_unloaded(bool can_be_unloaded){
	flags.can_be_unloaded = can_be_unloaded;
	return *this;
}
FuncEnviroBuilder& FuncEnviroBuilder::O_flag_is_translated(bool is_translated){
	flags.is_translated = is_translated;
	return *this;
}
FuncEnviroBuilder& FuncEnviroBuilder::O_flag_is_cheap(bool is_cheap){
	flags.is_cheap = is_cheap;
	return *this;
}
FuncEnviroBuilder& FuncEnviroBuilder::O_flag_used_vec128(uint8_t index){
	switch (index) {
	case 0: flags.used_vec.vec128_0 = true; break;
	case 1: flags.used_vec.vec128_1 = true; break;
	case 2: flags.used_vec.vec128_2 = true; break;
	case 3: flags.used_vec.vec128_3 = true; break;
	case 4: flags.used_vec.vec128_4 = true; break;
	case 5: flags.used_vec.vec128_5 = true; break;
	case 6: flags.used_vec.vec128_6 = true; break;
	case 7: flags.used_vec.vec128_7 = true; break;
	case 8: flags.used_vec.vec128_8 = true; break;
	case 9: flags.used_vec.vec128_9 = true; break;
	case 10: flags.used_vec.vec128_10 = true; break;
	case 11: flags.used_vec.vec128_11 = true; break;
	case 12: flags.used_vec.vec128_12 = true; break;
	case 13: flags.used_vec.vec128_13 = true; break;
	case 14: flags.used_vec.vec128_14 = true; break;
	case 15: flags.used_vec.vec128_15 = true; break;
	default:
		throw InvalidArguments("Used vector index must be in range 0-15");
	}
	return *this;
}
FuncEnviroBuilder& FuncEnviroBuilder::O_flag_is_patchable(bool is_patchabele){
	flags.is_patchable = is_patchabele;
	return *this;
}


FuncEnviroBuilder_line_info FuncEnviroBuilder::O_line_info_begin(){
	FuncEnviroBuilder_line_info ret;
	ret.begin = code.size();
	return ret;
}
void FuncEnviroBuilder::O_line_info_end(FuncEnviroBuilder_line_info line_info){
	//TODO: add line info
}



std::vector<uint8_t> FuncEnviroBuilder::O_build_func() {
	if(flags.run_time_computable)
		throw CompileTimeException("to build function, function must not use run time computable functions, ie dynamic");
	std::vector<uint8_t> fn;
	flags.has_local_functions = local_funs.size() > 0;
	builder::write(fn, flags);
	if(flags.used_static)
		builder::write(fn, static_values);
	if(flags.used_enviro_vals)
		builder::write(fn, values);
	if(flags.used_arguments)
		builder::write(fn, args_values);
	builder::writePackedLen(fn, constants_values);
	

	if(flags.has_local_functions)
		builder::writePackedLen(fn, local_funs.size());
	for(auto& locals : local_funs){
		const auto& loc = locals->get_cross_code();
		if(loc.empty())
			throw CompileTimeException("local function must be with opcodes");
		else{
			builder::writePackedLen(fn, loc.size());
			fn.insert(fn.end(), loc.begin(), loc.end());
		}
	}

	builder::writePackedLen(fn, jump_pos.size());
	for(uint64_t it : jump_pos)
		builder::write(fn, it);

	fn.insert(fn.end(),code.begin(), code.end());
	*(uint64_t*)(fn.data()) = fn.size();
	return fn;
}
void FuncEnviroBuilder::O_load_func(const std::string& str) {
	FuncEnvironment::Load(O_prepare_func(), str);
}
void FuncEnviroBuilder::O_patch_func(const std::string& symbol_name){
	FuncEnvironment::fastHotPatch(symbol_name, new FuncHandle::inner_handle(O_build_func(),flags.is_cheap));
}
