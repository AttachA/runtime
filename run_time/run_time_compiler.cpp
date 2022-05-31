// Copyright Danyil Melnytskyi 2022
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once
#include <bit>
#include <stdint.h>
#include "CASM.hpp"
#include "run_time_compiler.hpp"
#include "tools.hpp"
#include "tasks.hpp"
#include <Windows.h>
#include <stddef.h>
#include <iostream>
using namespace run_time;

asmjit::JitRuntime jrt;
std::unordered_map<std::string, typed_lgr<FuncEnviropment>> FuncEnviropment::enviropments;

ValueEnvironment enviropments;
thread_local ValueEnvironment thread_local_enviropments;

ValueItem* FuncEnviropment::asyncCall(typed_lgr<FuncEnviropment> f, list_array<ValueItem>* args) {
	ValueItem* res = new ValueItem();
	res->meta = ValueMeta(VType::async_res, false, false).encoded;
	res->val = new typed_lgr(new Task(f, new list_array<ValueItem>(*args)));
	Task::start(*(typed_lgr<Task>*)res->val);
	return res;
}
void inlineReleaseUnused(CASM& a,creg64 reg) {
	auto lab = a.newLabel();
	a.test(reg, reg);
	a.jmp_zero(lab);
	BuildCall b(a);
	b.addArg(reg);
	b.finalize(defaultDestructor<ValueItem>);
	a.label_bind(lab);
}






list_array<std::pair<uint64_t, Label>> prepareJumpList(CASM& a, const std::vector<uint8_t>& data, size_t data_len, size_t& to_be_skiped) {
	if (uint8_t size = data[to_be_skiped++]) {
		uint64_t labels = 0;
		switch (size) {
		case 8:
			labels = data[to_be_skiped++];
			labels <<= 8;
			labels |= data[to_be_skiped++];
			labels <<= 8;
			labels |= data[to_be_skiped++];
			labels <<= 8;
			labels |= data[to_be_skiped++];
			labels <<= 8;
			__fallthrough;
		case 4:
			labels |= data[to_be_skiped++];
			labels <<= 8;
			labels |= data[to_be_skiped++];
			labels <<= 8;
			__fallthrough;
		case 2:
			labels |= data[to_be_skiped++];
			labels <<= 8;
			__fallthrough;
		case 1:
			labels |= data[to_be_skiped++];
			break;
		default:
			throw InvalidFunction("Invalid function header, label count size missmatch");
		}
		list_array<std::pair<uint64_t, Label>> res;
		res.resize(labels);
		for (uint64_t i = 0; i < labels; i++)
			res[i] = { readData<uint64_t>(data,data_len,to_be_skiped), a.newLabel() };
		return res;
	}
	return {};
}
ValueItem* AttachACXXCatchCall(AttachACXX fn, list_array<ValueItem>* args) {
	try {
		return fn(args);
	}
	catch (...) {
		return new ValueItem(new std::exception_ptr(std::current_exception()), ValueMeta(VType::except_value, false, true), true);
	}
}
template<bool use_result = true, bool do_cleanup = true>
void compilerFabric_call(CASM& a, const std::vector<uint8_t>& data,size_t data_len, size_t& i, list_array<ValueItem>& values) {
	CallFlags flags;
	flags.encoded = readData<uint8_t>(data, data_len, i);
	BuildCall b(a);
	if (flags.in_memory) {
		uint16_t value_index = readData<uint16_t>(data, data_len, i);
		b.leaEnviro(value_index);
		b.addArg(VType::string);
		b.finalize(getSpecificValue);
		b.addArg(resr);
		b.addArg(arg_ptr);
		b.addArg(flags.async_mode);
		if(flags.except_catch)
			b.finalize(&FuncEnviropment::CallFunc_catch);
		else
			b.finalize(&FuncEnviropment::CallFunc);
	}
	else {
		std::string fnn = readString(data, data_len, i);
		typed_lgr<FuncEnviropment> fn = FuncEnviropment::enviropment(fnn);
		if (fn->canBeUnloaded() || flags.async_mode) {
			values.push_back(fnn);
			b.addArg(((std::string*)values.back().val)->c_str());
			b.addArg(arg_ptr);
			b.addArg(flags.async_mode);
			if (flags.except_catch)
				b.finalize(&FuncEnviropment::CallFunc_catch);
			else
				b.finalize(&FuncEnviropment::CallFunc);
		}
		else {
			switch (fn->Type()) {
				case FuncEnviropment::FuncType::own: {
					b.addArg(fn.getPtr());
					b.addArg(arg_ptr);
					if (flags.except_catch)
						b.finalize(&FuncEnviropment::initAndCall_catch);
					else
						b.finalize(&FuncEnviropment::initAndCall);
					break;
				}
				case FuncEnviropment::FuncType::native: {
					if (!fn->templateCall().arguments.size() && fn->templateCall().result.is_void()) {
						b.finalize(fn->get_func_ptr());
						if constexpr (use_result)
							if (flags.use_result) {
								a.movEnviro(readData<uint16_t>(data, data_len, i), 0);
								return;
							}
						break;
					}
					b.addArg(fn.getPtr());
					b.addArg(arg_ptr);
					if (flags.except_catch)
						b.finalize(&FuncEnviropment::native_proxy_catch);
					else
						b.finalize(&FuncEnviropment::NativeProxy_DynamicToStatic);
					break;
				}
				case FuncEnviropment::FuncType::native_own_abi: {
					if (flags.except_catch) {
						b.addArg(fn->get_func_ptr());
						b.addArg(arg_ptr);
						b.finalize(AttachACXXCatchCall);
					}
					else {
						b.addArg(arg_ptr);
						b.finalize(fn->get_func_ptr());
					}
					break;
				}
				default: {
					b.addArg(fn.getPtr());
					b.addArg(arg_ptr);
					if (flags.except_catch)
						b.finalize(&FuncEnviropment::syncWrapper_catch);
					else
						b.finalize(&FuncEnviropment::syncWrapper);
					break;
				}
			}
		}
	}
	if constexpr (use_result) {
		if (flags.use_result) {
			b.leaEnviro(readData<uint16_t>(data, data_len, i));
			b.addArg(resr);
			b.finalize(getValueItem);
			return;
		}
		else if constexpr (do_cleanup)
			if (!flags.use_result)
				inlineReleaseUnused(a, resr);
	}
	else if constexpr (do_cleanup)
		if (!flags.use_result)
			inlineReleaseUnused(a, resr);
}
template<bool use_result = true, bool do_cleanup = true>
void compilerFabric_call_local(CASM& a, const std::vector<uint8_t>& data, size_t data_len, size_t& i,FuncEnviropment* env) {
	CallFlags flags;
	flags.encoded = readData<uint8_t>(data, data_len, i);
	BuildCall b(a);
	if (flags.in_memory) {
		b.leaEnviro(readData<uint16_t>(data, data_len, i));
		b.finalize(getSize);
		b.addArg(env);
		b.addArg(resr);
		b.addArg(arg_ptr);
		b.addArg(flags.async_mode);
		if (flags.except_catch)
			b.finalize(&FuncEnviropment::localWrapper_catch);
		else
			b.finalize(&FuncEnviropment::localWrapper);
	}
	else {
		uint32_t fnn = readData<uint32_t>(data, data_len, i);
		if (env->localFnSize() >= fnn) {
			throw CompileTimeException("Not found local function");
		}
		if (flags.async_mode) {
			b.addArg(env);
			b.addArg(resr);
			b.addArg(arg_ptr);
			b.addArg(flags.async_mode);
			if (flags.except_catch)
				b.finalize(&FuncEnviropment::localWrapper_catch);
			else
				b.finalize(&FuncEnviropment::localWrapper);
		}
		else {
			typed_lgr<FuncEnviropment> fn = env->localFn(fnn);
			switch (fn->Type()) {
			case FuncEnviropment::FuncType::own: {
				b.addArg(fn.getPtr());
				b.addArg(arg_ptr);
				if(flags.except_catch)
					b.finalize(&FuncEnviropment::initAndCall_catch);
				else
					b.finalize(&FuncEnviropment::initAndCall);
				break;
			}
			case FuncEnviropment::FuncType::native: {
				if (!fn->templateCall().arguments.size() && fn->templateCall().result.is_void()) {
					b.finalize(fn->get_func_ptr());
					if constexpr (use_result)
						if (flags.use_result) {
							a.movEnviro(readData<uint16_t>(data, data_len, i), 0);
							return;
						}
					break;
				}
				b.addArg(fn.getPtr());
				b.addArg(arg_ptr);
				if (flags.except_catch)
					b.finalize(& FuncEnviropment::native_proxy_catch);
				else
					b.finalize(&FuncEnviropment::NativeProxy_DynamicToStatic);
				break;
			}
			case FuncEnviropment::FuncType::native_own_abi: {
				if (flags.except_catch) {
					b.addArg(fn->get_func_ptr());
					b.addArg(arg_ptr);
					b.finalize(AttachACXXCatchCall);
				}
				else {
					b.addArg(arg_ptr);
					b.finalize(fn->get_func_ptr());
				}
				break;
			}
			default: {
				b.addArg(fn.getPtr());
				b.addArg(arg_ptr);
				if (flags.except_catch)
					b.finalize(&FuncEnviropment::syncWrapper_catch);
				else
					b.finalize(&FuncEnviropment::syncWrapper);
				break;
			}
			}
		}
	}
	if constexpr (use_result) {
		if (flags.use_result) {
			b.leaEnviro(readData<uint16_t>(data, data_len, i));
			b.addArg(resr);
			b.finalize(getValueItem);
			return;
		}
		else if constexpr (do_cleanup)
			if (!flags.use_result)
				inlineReleaseUnused(a, resr);
	}
	else if constexpr (do_cleanup)
		if (!flags.use_result)
			inlineReleaseUnused(a, resr);
}



template<char typ>
void IndexArrayCopy(void** value,list_array<ValueItem>* arr, uint64_t pos) {
	universalRemove(value);
	if constexpr (typ == 2) {
		ValueItem temp = ((list_array<ValueItem>*)arr)->atDefault(pos);
		*value = temp.val;
		*((size_t*)(value + 1)) = temp.meta.encoded;
		temp.val = nullptr;
	}
	else {
		ValueItem* res;
		if constexpr (typ == 1)
			res = &((list_array<ValueItem>*)arr)->at(pos);
		else
			res = &((list_array<ValueItem>*)arr)->operator[](pos);
		*value = copyValue(res->val, res->meta);
		*((size_t*)(value + 1)) = res->meta.encoded;
	}
}
template<char typ>
void IndexArrayMove(void** value, list_array<ValueItem>* arr, uint64_t pos) {
	universalRemove(value);	
	if constexpr (typ == 2) {
		if (arr->size() > pos) {
			ValueItem& res = ((list_array<ValueItem>*)arr)->operator[](pos);
			*value = res.val;
			*((size_t*)(value + 1)) = res.meta.encoded;
			res.val = nullptr;
			res.meta.encoded = 0;
		}
	}
	else {
		ValueItem* res;
		if constexpr (typ == 1)
			res = &((list_array<ValueItem>*)arr)->at(pos);
		else
			res = &((list_array<ValueItem>*)arr)->operator[](pos);
		*value = res->val;
		*((size_t*)(value + 1)) = res->meta.encoded;
		res->val = nullptr;
		res->meta.encoded = 0;
	}
}


template<char typ>
void IndexArraySetCopy(void** value, list_array<ValueItem>* arr, uint64_t pos) {
	if constexpr (typ == 2) {
		if (((list_array<ValueItem>*)arr)->size() <= pos)
			((list_array<ValueItem>*)arr)->resize(pos + 1);
		((list_array<ValueItem>*)arr)->operator[](pos) = reinterpret_cast<ValueItem&>(value);
	}
	else {
		if constexpr (typ == 1)
			((list_array<ValueItem>*)arr)->at(pos) = reinterpret_cast<ValueItem&>(value);
		else
			((list_array<ValueItem>*)arr)->operator[](pos) = reinterpret_cast<ValueItem&>(value);
	}
}
template<char typ>
void IndexArraySetMove(void** value, list_array<ValueItem>* arr, uint64_t pos) {
	if constexpr (typ == 2) {
		if (((list_array<ValueItem>*)arr)->size() <= pos)
			((list_array<ValueItem>*)arr)->resize(pos + 1);
		((list_array<ValueItem>*)arr)->operator[](pos) = reinterpret_cast<ValueItem&&>(value);
	}
	else {
		if constexpr (typ == 1)
			((list_array<ValueItem>*)arr)->at(pos) = reinterpret_cast<ValueItem&&>(value);
		else
			((list_array<ValueItem>*)arr)->operator[](pos) = reinterpret_cast<ValueItem&&>(value);
	}
}
void setSize(void** value, size_t res) {
	void*& set = getValue(*value, *(ValueMeta*)(value + 1));
	ValueMeta& meta = *(ValueMeta*)(value + 1);
	switch (meta.vtype) {
	case VType::i8:
		if (((int8_t&)set = (int8_t)res) != res)
			throw NumericUndererflowException();
		break;
	case VType::i16:
		if (((int16_t&)set = (int16_t)res) != res)
			throw NumericUndererflowException();
		break;
	case VType::i32:
		if (((int32_t&)set = (int32_t)res) != res)
			throw NumericUndererflowException();
		break;
	case VType::i64:
		if (((int64_t&)set = (int64_t)res) != res)
			throw NumericUndererflowException();
		break;
	case VType::ui8:
		if (((uint8_t&)set = (uint8_t)res) != res)
			throw NumericOverflowException();
		break;
	case VType::ui16:
		if (((uint16_t&)set = (uint16_t)res) != res)
			throw NumericOverflowException();
		break;
	case VType::ui32:
		if (((uint32_t&)set = (uint32_t)res) != res)
			throw NumericOverflowException();
		break;
	case VType::ui64:
		(uint64_t&)set = res;
		break;
	case VType::flo:
		if (size_t((float&)set = (float)res) != res)
			throw NumericOverflowException();
		break;
	case VType::doub:
		if (size_t((double&)set = (double)res) != res)
			throw NumericOverflowException();
		break;
	default:
		throw InvalidType("Need sizable type");
	}
}

void takeStart(list_array<ValueItem>* dest, void** insert) {
	reinterpret_cast<ValueItem&>(insert) = dest->take_front();
}
void takeEnd(list_array<ValueItem>* dest, void** insert) {
	reinterpret_cast<ValueItem&>(insert) = dest->take_back();
}
void take(size_t pos,list_array<ValueItem>* dest, void** insert) {
	reinterpret_cast<ValueItem&>(insert) = dest->take(pos);
}
void getRange(list_array<ValueItem>* dest, void** insert,size_t start, size_t end) {
	auto tmp = dest->range(start, end);
	reinterpret_cast<ValueItem&>(insert) = list_array<ValueItem>(tmp.begin(), tmp.end());
}
void takeRange(list_array<ValueItem>* dest, void** insert, size_t start, size_t end) {
	reinterpret_cast<ValueItem&>(insert) = dest->take(start, end);
}




void throwDirEx(void** typ, void** desc) {
	throw AException(
		*(const std::string*)getSpecificValue(typ, VType::string),
		*(const std::string*)getSpecificValue(desc, VType::string)
	);
}
void throwStatEx(void** typ, void** desc) {
	throw AException(
		*(const std::string*)getValue(typ),
		*(const std::string*)getValue(desc)
	);
}
void throwEx(const std::string* typ, const std::string* desc) {
	throw AException(*typ, *desc);
}



void setValue(void*& val, void* set,ValueMeta meta) {
	val = copyValue(set, meta);
}


struct EnviroHold {
	void** envir;
	uint16_t _vals;
	EnviroHold(void** env, uint16_t vals) :envir(env), _vals(vals) {
		if (vals)
			memset(envir, 0, sizeof(void*) * (size_t(vals) << 1));
	}
	~EnviroHold() {
		if (_vals)
			removeEnviropement(envir, _vals);
	}
};
ValueItem* inlineCatch(Enviropment curr_func, list_array<ValueItem>* arguments, uint16_t max_values) {
	ValueItem* res = nullptr;
	try {
		EnviroHold env(
			(
				max_values ?
				(void**)alloca(sizeof(void*) * (size_t(max_values) << 1)) :
				nullptr
				)
			, max_values
		);
		res = curr_func(env.envir, arguments);
	}
	catch (const StackOverflowException&) {
		if (!need_restore_stack_fault())
			return new ValueItem(new std::exception_ptr(std::current_exception()), ValueMeta(VType::except_value, false, true), true);
	}
	catch (...) {
		return new ValueItem(new std::exception_ptr(std::current_exception()), ValueMeta(VType::except_value, false, true), true);
	}
	if (restore_stack_fault()) {
		try {
			throw StackOverflowException();
		}
		catch (...) {
			return new ValueItem(new std::exception_ptr(std::current_exception()), ValueMeta(VType::except_value, false, true), true);
		}
	}
	return res;
}
ValueItem* inlineAsync(Enviropment curr_func, list_array<ValueItem>* arguments, uint16_t max_values) {

	ValueItem* res = nullptr;
	try {
		EnviroHold env(
			(
				max_values ?
				(void**)alloca(sizeof(void*) * (size_t(max_values) << 1)) :
				nullptr
				)
			, max_values
		);
		res = curr_func(env.envir, arguments);
	}
	catch (const StackOverflowException&) {
		if (!need_restore_stack_fault())
			return new ValueItem(new std::exception_ptr(std::current_exception()), ValueMeta(VType::except_value, false, true), true);
	}
	catch (...) {
		return new ValueItem(new std::exception_ptr(std::current_exception()), ValueMeta(VType::except_value, false, true), true);
	}
	if (restore_stack_fault()) {
		try {
			throw StackOverflowException();
		}
		catch (...) {
			return new ValueItem(new std::exception_ptr(std::current_exception()), ValueMeta(VType::except_value, false, true), true);
		}
	}
	return res;
}

void FuncEnviropment::Compile() {
	if (curr_func != nullptr)
		FrameResult::deinit(frame, curr_func, jrt);
	values.clear();
	CodeHolder code;
	code.init(jrt.environment());
	CASM a(code);

	BuildProlog bprolog(a);
	bprolog.pushReg(frame_ptr);
	bprolog.pushReg(enviro_ptr);
	if(max_values)
		bprolog.pushReg(arg_ptr);
	bprolog.alignPush();
	bprolog.stackAlloc(0x20);//function vc++ abi
	bprolog.setFrame();
	a.stackAlign();

	a.mov(enviro_ptr, argr0);
	if (max_values)
		a.mov(arg_ptr, argr1);
	else
		a.xor_(arg_ptr, arg_ptr);

	Label prolog = a.newLabel();
	size_t to_be_skiped = 0;
	list_array<std::pair<uint64_t, Label>> jump_list = prepareJumpList(a, cross_code, cross_code.size(), to_be_skiped);

	auto compilerFabric = [&](CASM& a, list_array<std::pair<uint64_t, Label>>& j_list, size_t skip) {
		std::vector<uint8_t>& data = cross_code;
		size_t data_len = data.size();
		size_t string_index_seter = 0;
		size_t i = skip;
		size_t instructuion = 0;
		bool do_jump_to_ret = false;

		auto static_fabric = [&](Command cmd) {
			switch (cmd.code) {
			case Opcode::set: {
				uint16_t value_index = readData<uint16_t>(data, data_len, i);
				ValueMeta meta = readData<ValueMeta>(data, data_len, i);
				{
					if (needAlloc(meta.vtype)) {
						BuildCall v(a);
						v.leaEnviro(value_index);
						v.addArg(meta.encoded);
						v.addArg(cmd.is_gc_mode);
						v.finalize(preSetValue);
					}
					else
						a.movEnviroMeta(value_index, meta.encoded);
				}
				switch (meta.vtype) {
				case VType::i8:
				case VType::ui8:
					a.mov(resr, 0, 1, readData<uint8_t>(data, data_len, i));
					break;
				case VType::i16:
				case VType::ui16:
					a.mov(resr, 0, 2, readData<uint16_t>(data, data_len, i));
					break;
				case VType::i32:
				case VType::ui32:
				case VType::flo:
					a.mov(resr, 0, 4, readData<uint32_t>(data, data_len, i));
					break;
				case VType::i64:
				case VType::ui64:
				case VType::doub:
					a.mov(resr, 0, 8, readData<uint64_t>(data, data_len, i));
					break;
				case VType::string: {
					values.push_back(readString(data, data_len, i));
					BuildCall b(a);
					b.addArg(resr);
					b.addArg(values.back().val);
					b.addArg(meta.encoded);
					b.finalize(setValue);
					break;
				}
				default:
					break;
				}
				break;
			}
			case Opcode::remove:
				if (needAlloc((VType)readData<uint8_t>(data, data_len, i))) {
					BuildCall b(a);
					b.leaEnviro(readData<uint16_t>(data, data_len, i));
					b.finalize(universalRemove);
				}
				else
					a.movEnviroMeta(readData<uint16_t>(data, data_len, i), 0);
				break; 
			case Opcode::arg_set:
				a.leaEnviro(arg_ptr, readData<uint16_t>(data, data_len, i));
				break;
			case Opcode::throw_ex: {
				bool in_memory = readData<bool>(data, data_len, i);
				if (in_memory) {
					BuildCall b(a);
					b.addArg(readData<uint16_t>(data, data_len, i));
					b.addArg(readData<uint16_t>(data, data_len, i));
					b.finalize(throwStatEx);
				}
				else {
					auto ex_typ = string_index_seter++;
					values[ex_typ] = readString(data, data_len, i);
					auto ex_desc = string_index_seter++;
					values[ex_desc] = readString(data, data_len, i);
					BuildCall b(a);
					b.addArg(values[ex_typ].val);
					b.addArg(values[ex_desc].val);
					b.finalize(throwEx);
				}
				break;
			}
			case Opcode::arr_op: {
				uint16_t arr = readData<uint16_t>(data, data_len, i);
				BuildCall b(a);
				auto flags = readData<OpArrFlags>(data, data_len, i);
				switch (readData<OpcodeArray>(data, data_len, i)) {
				case OpcodeArray::set: {
					if (flags.by_val_mode) {
						b.leaEnviro(readData<uint16_t>(data, data_len, i));
						b.finalize(getSize);
						b.leaEnviro(readData<uint16_t>(data, data_len, i));
						b.leaEnviro(arr);
						b.addArg(resr);
					}
					else {
						b.leaEnviro(readData<uint16_t>(data, data_len, i));
						b.leaEnviro(arr);
						b.addArg(readData<uint64_t>(data, data_len, i));
					}
					if (flags.move_mode) {
						switch (flags.checked)
						{
						case OpArrFlags::CheckMode::no_check:
							b.finalize(IndexArraySetMove<0>);
							break;
						case OpArrFlags::CheckMode::check:
							b.finalize(IndexArraySetMove<1>);
							break;
						case OpArrFlags::CheckMode::no_throw_check:
							b.finalize(IndexArraySetMove<2>);
							break;
						default:
							break;
						}
					}
					else {
						switch (flags.checked)
						{
						case OpArrFlags::CheckMode::no_check:
							b.finalize(IndexArraySetCopy<0>);
							break;
						case OpArrFlags::CheckMode::check:
							b.finalize(IndexArraySetCopy<1>);
							break;
						case OpArrFlags::CheckMode::no_throw_check:
							b.finalize(IndexArraySetCopy<2>);
							break;
						default:
							break;
						}
					}
					break;
				}
				case OpcodeArray::insert: {
					if (flags.by_val_mode) {
						b.leaEnviro(readData<uint16_t>(data, data_len, i));
						b.finalize(getSize);
						b.leaEnviro(arr);
						b.addArg(resr);
						b.leaEnviro(readData<uint16_t>(data, data_len, i));
					}
					else {
						b.leaEnviro(arr);
						b.addArg(readData<uint64_t>(data, data_len, i));
						b.leaEnviro(readData<uint16_t>(data, data_len, i));
					}
					if (flags.move_mode)
						b.finalize((void (list_array<ValueItem>::*)(size_t, const ValueItem&)) & list_array<ValueItem>::insert);
					else
						b.finalize((void (list_array<ValueItem>::*)(size_t, ValueItem&&)) & list_array<ValueItem>::insert);
					break;
				}
				case OpcodeArray::push_end: {
					b.movEnviro(arr);
					b.leaEnviro(readData<uint16_t>(data, data_len, i));
					if (flags.move_mode)
						b.finalize((void (list_array<ValueItem>::*)(ValueItem&&)) & list_array<ValueItem>::push_back);
					else
						b.finalize((void (list_array<ValueItem>::*)(const ValueItem&)) & list_array<ValueItem>::push_back);
					break;
				}
				case OpcodeArray::push_start: {
					b.movEnviro(arr);
					b.leaEnviro(readData<uint16_t>(data, data_len, i));
					if (flags.move_mode)
						b.finalize((void (list_array<ValueItem>::*)(ValueItem&&)) & list_array<ValueItem>::push_front);
					else
						b.finalize((void (list_array<ValueItem>::*)(const ValueItem&)) & list_array<ValueItem>::push_front);
					break;
				}
				case OpcodeArray::insert_range: {
					uint16_t arr1 = readData<uint16_t>(data, data_len, i);
					b.leaEnviro(arr1);
					b.finalize(AsArr);
					if (flags.by_val_mode) {
						uint16_t val1 = readData<uint16_t>(data, data_len, i);
						uint16_t val2 = readData<uint16_t>(data, data_len, i);
						uint16_t val3 = readData<uint16_t>(data, data_len, i);

						b.leaEnviro(val3);
						b.finalize(getSize);
						a.push(resr);

						b.leaEnviro(val2);
						b.finalize(getSize);
						a.push(resr);

						b.leaEnviro(val1);
						b.finalize(getSize);
						b.movEnviro(arr);
						b.addArg(resr);//1
						b.leaEnviro(arr1);

						a.pop(resr);
						b.addArg(resr);//2
						a.pop(resr);
						b.addArg(resr);//3
					}
					else {
						b.movEnviro(arr);
						b.addArg(readData<uint64_t>(data, data_len, i));//1
						b.leaEnviro(arr1);

						b.addArg(readData<uint64_t>(data, data_len, i));//2
						b.addArg(readData<uint64_t>(data, data_len, i));//3
					}

					b.finalize((void(list_array<ValueItem>::*)(size_t, const list_array<ValueItem>&, size_t, size_t)) & list_array<ValueItem>::insert);
					break;
				}
				case OpcodeArray::get: {
					if (flags.by_val_mode) {
						b.leaEnviro(readData<uint16_t>(data, data_len, i));
						b.finalize(getSize);
						b.leaEnviro(readData<uint16_t>(data, data_len, i));
						b.leaEnviro(arr);
						b.addArg(resr);
					}
					else {
						b.leaEnviro(readData<uint16_t>(data, data_len, i));
						b.leaEnviro(arr);
						b.addArg(readData<uint64_t>(data, data_len, i));
					}
					if (flags.move_mode) {
						switch (flags.checked)
						{
						case OpArrFlags::CheckMode::no_check:
							b.finalize(IndexArrayMove<0>);
							break;
						case OpArrFlags::CheckMode::check:
							b.finalize(IndexArrayMove<1>);
							break;
						case OpArrFlags::CheckMode::no_throw_check:
							b.finalize(IndexArrayMove<2>);
							break;
						default:
							break;
						}
					}
					else {
						switch (flags.checked)
						{
						case OpArrFlags::CheckMode::no_check:
							b.finalize(IndexArrayCopy<0>);
							break;
						case OpArrFlags::CheckMode::check:
							b.finalize(IndexArrayCopy<1>);
							break;
						case OpArrFlags::CheckMode::no_throw_check:
							b.finalize(IndexArrayCopy<2>);
							break;
						default:
							break;
						}
					}
					break;
				}
				case OpcodeArray::take: {
					if (flags.by_val_mode) {
						b.leaEnviro(readData<uint16_t>(data, data_len, i));
						b.finalize(getSize);
						b.addArg(resr);
						b.leaEnviro(arr);
						b.leaEnviro(readData<uint16_t>(data, data_len, i));
					}
					else {
						b.addArg(readData<uint64_t>(data, data_len, i));
						b.leaEnviro(arr);
						b.leaEnviro(readData<uint16_t>(data, data_len, i));
					}
					b.finalize(take);
					break;
				}
				case OpcodeArray::take_end: {
					b.leaEnviro(arr);
					b.leaEnviro(readData<uint16_t>(data, data_len, i));
					b.finalize(takeEnd);
					break;
				}
				case OpcodeArray::take_start: {
					b.leaEnviro(arr);
					b.leaEnviro(readData<uint16_t>(data, data_len, i));
					b.finalize(takeStart);
					break;
				}
				case OpcodeArray::get_range: {
					if (flags.by_val_mode) {
						uint16_t set_to = readData<uint16_t>(data, data_len, i);
						b.leaEnviro(readData<uint16_t>(data, data_len, i));
						b.finalize(getSize);
						a.push(resr);
						b.leaEnviro(readData<uint16_t>(data, data_len, i));
						b.finalize(getSize);
						a.pop(mut_temp_ptr);
						b.addArg(set_to);
						b.addArg(mut_temp_ptr);
						b.addArg(resr);
					}
					else {
						b.leaEnviro(arr);
						b.leaEnviro(readData<uint16_t>(data, data_len, i));
						b.addArg(readData<uint64_t>(data, data_len, i));
						b.addArg(readData<uint64_t>(data, data_len, i));
					}
					b.finalize(getRange);
					break;
				}
				case OpcodeArray::take_range: {
					if (flags.by_val_mode) {
						uint16_t set_to = readData<uint16_t>(data, data_len, i);
						b.leaEnviro(readData<uint16_t>(data, data_len, i));
						b.finalize(getSize);
						a.push(resr);
						b.leaEnviro(readData<uint16_t>(data, data_len, i));
						b.finalize(getSize);
						a.pop(mut_temp_ptr);
						b.addArg(set_to);
						b.addArg(mut_temp_ptr);
						b.addArg(resr);
					}
					else {
						b.leaEnviro(arr);
						b.leaEnviro(readData<uint16_t>(data, data_len, i));
						b.addArg(readData<uint64_t>(data, data_len, i));
						b.addArg(readData<uint64_t>(data, data_len, i));
					}
					b.finalize(takeRange);
					break;
				}
				case OpcodeArray::pop_end: {
					b.leaEnviro(arr);
					b.finalize(&list_array<ValueItem>::pop_back);
					break;
				}
				case OpcodeArray::pop_start: {
					b.leaEnviro(arr);
					b.finalize(&list_array<ValueItem>::pop_front);
					break;
				}
				case OpcodeArray::remove_item: {
					b.movEnviro(arr);
					b.movEnviro(readData<uint64_t>(data, data_len, i));
					b.finalize((void(list_array<ValueItem>::*)(size_t pos)) & list_array<ValueItem>::remove);
					break;
				}
				case OpcodeArray::remove_range: {
					b.movEnviro(arr);
					b.addArg(readData<uint64_t>(data, data_len, i));
					b.addArg(readData<uint64_t>(data, data_len, i));
					b.finalize((void(list_array<ValueItem>::*)(size_t, size_t)) & list_array<ValueItem>::remove);
					break;
				}

				case OpcodeArray::resize: {
					if (flags.by_val_mode) {
						b.leaEnviro(readData<uint16_t>(data, data_len, i));
						b.finalize(getSize);
						b.leaEnviro(arr);
						b.addArg(resr);
					}
					else {
						b.movEnviro(arr);
						b.addArg(readData<uint64_t>(data, data_len, i));
					}
					b.finalize((void(list_array<ValueItem>::*)(size_t)) & list_array<ValueItem>::resize);
					break;
				}
				case OpcodeArray::resize_default: {
					if (flags.by_val_mode) {
						b.leaEnviro(readData<uint16_t>(data, data_len, i));
						b.finalize(getSize);
						b.leaEnviro(arr);
						b.addArg(resr);
					}
					else {
						b.movEnviro(arr);
						b.addArg(readData<uint64_t>(data, data_len, i));
					}
					b.leaEnviro(readData<uint16_t>(data, data_len, i));
					b.finalize((void(list_array<ValueItem>::*)(size_t, const ValueItem&)) & list_array<ValueItem>::resize);
					break;
				}
				case OpcodeArray::reserve_push_end: {
					b.leaEnviro(arr);
					b.movEnviro(readData<uint64_t>(data, data_len, i));
					b.finalize(&list_array<ValueItem>::reserve_push_back);
					break;
				}
				case OpcodeArray::reserve_push_start: {
					b.leaEnviro(arr);
					if (flags.by_val_mode) {
						b.leaEnviro(readData<uint16_t>(data, data_len, i));
						b.finalize(getSize);
						b.addArg(resr);
					}
					else
						b.movEnviro(readData<uint64_t>(data, data_len, i));
					b.finalize(&list_array<ValueItem>::reserve_push_front);
					break;
				}

				case OpcodeArray::commit: {
					BuildCall b(a);
					b.movEnviro(arr);
					b.finalize(&list_array<ValueItem>::commit);
					break;
				}
				case OpcodeArray::decommit: {
					BuildCall b(a);
					b.movEnviro(arr);
					b.addArg(readData<uint64_t>(data, data_len, i));
					b.finalize(&list_array<ValueItem>::decommit);
					break;
				}
				case OpcodeArray::remove_reserved: {
					BuildCall b(a);
					b.movEnviro(arr);
					b.finalize(&list_array<ValueItem>::shrink_to_fit);
					break;
				}
				case OpcodeArray::size: {
					b.movEnviro(arr);
					b.finalize(&list_array<ValueItem>::size);
					b.leaEnviro(readData<uint16_t>(data, data_len, i));
					b.addArg(resr);
					b.finalize(setSize);
					break;
				}
				default:
					throw CompileTimeException("Invalid array operation");
				}
				break;
			}
			default:
				throw CompileTimeException("Invalid opcode");
			}
		};

		auto dynamic_fablric = [&](Command cmd) {
			switch (cmd.code) {
			case Opcode::noting:
				a.noting();
				break;
			case Opcode::set: {
				uint16_t value_index = readData<uint16_t>(data, data_len, i);
				ValueMeta meta = readData<ValueMeta>(data, data_len, i);
				{
					BuildCall v(a);
					v.leaEnviro(value_index);
					v.addArg(meta.encoded);
					v.addArg(cmd.is_gc_mode);
					v.finalize(preSetValue);
				}
				switch (meta.vtype) {
				case VType::i8:
				case VType::ui8:
					a.mov(resr,0,1, readData<uint8_t>(data, data_len, i));
					break;
				case VType::i16:
				case VType::ui16:
					a.mov(resr, 0, 2, readData<uint16_t>(data, data_len, i));
					break;
				case VType::i32:
				case VType::ui32:
				case VType::flo:
					a.mov(resr, 0, 4, readData<uint32_t>(data, data_len, i));
					break;
				case VType::i64:
				case VType::ui64:
				case VType::doub:
					a.mov(resr, 0, 8, readData<uint64_t>(data, data_len, i));
					break;
				case VType::raw_arr_i8: 
				case VType::raw_arr_ui8: {
					uint32_t len = readLen(data, data_len, i);
					values.push_back(ValueItem(readRawArray<int8_t>(data, data_len, i, len), VType::raw_arr_i8, true));
					BuildCall b(a);
					b.addArg(resr);
					b.addArg(values.back().val);
					b.addArg(meta.encoded);
					b.finalize(setValue);
					a.mov(resr, uint64_t(len) << 32);
					a.or_(resr, enviro_ptr, CASM::enviroMetaOffset(value_index));
					a.movEnviroMeta(value_index, resr);
					break;
				}
				case VType::raw_arr_i16:
				case VType::raw_arr_ui16: {
					uint32_t len = readLen(data, data_len, i);
					values.push_back(ValueItem(readRawArray<int16_t>(data, data_len, i, len), VType::raw_arr_i8, true));
					BuildCall b(a);
					b.addArg(resr);
					b.addArg(values.back().val);
					b.addArg(meta.encoded);
					b.finalize(setValue);
					a.mov(resr, uint64_t(len) << 32);
					a.or_(resr, enviro_ptr, CASM::enviroMetaOffset(value_index));
					a.movEnviroMeta(value_index, resr);
					break;
				}
				case VType::raw_arr_i32:
				case VType::raw_arr_ui32:
				case VType::raw_arr_flo: {
					uint32_t len = readLen(data, data_len, i);
					values.push_back(ValueItem(readRawArray<uint32_t>(data, data_len, i, len), VType::raw_arr_i8, true));
					BuildCall b(a);
					b.addArg(resr);
					b.addArg(values.back().val);
					b.addArg(meta.encoded);
					b.finalize(setValue);
					a.mov(resr, uint64_t(len) << 32);
					a.or_(resr, enviro_ptr, CASM::enviroMetaOffset(value_index));
					a.movEnviroMeta(value_index, resr);
					break;
				}
				case VType::raw_arr_i64:
				case VType::raw_arr_ui64:
				case VType::raw_arr_doub: {
					uint32_t len = readLen(data, data_len, i);
					values.push_back(ValueItem(readRawArray<uint64_t>(data, data_len, i, len), VType::raw_arr_i8, true));
					BuildCall b(a);
					b.addArg(resr);
					b.addArg(values.back().val);
					b.addArg(meta.encoded);
					b.finalize(setValue);
					a.mov(resr, uint64_t(len) << 32);
					a.or_(resr, enviro_ptr, CASM::enviroMetaOffset(value_index));
					a.movEnviroMeta(value_index, resr);
					break;
				}
				case VType::string: {
					values.push_back(readString(data, data_len, i));
					BuildCall b(a);
					b.addArg(resr);
					b.addArg(values.back().val);
					b.addArg(meta.encoded);
					b.finalize(setValue);
					break;
				}
				case VType::class_ : {
					values.push_back(readString(data, data_len, i));
					BuildCall b(a);
					b.addArg(resr);
					b.addArg(values.back().val);
					b.addArg(meta.encoded);
					b.finalize(setValue);
					break;
				}
				default:
					break;
				}
				break;
			}
			case Opcode::remove: {
				BuildCall b(a);
				b.leaEnviro(readData<uint16_t>(data, data_len, i));
				b.finalize(universalRemove);
				break;
			}
			case Opcode::sum: {
				BuildCall b(a);
				b.leaEnviro(readData<uint16_t>(data, data_len, i));
				b.leaEnviro(readData<uint16_t>(data, data_len, i));
				b.finalize(DynSum);
				break;
			}
			case Opcode::minus: {
				BuildCall b(a);
				b.leaEnviro(readData<uint16_t>(data, data_len, i));
				b.leaEnviro(readData<uint16_t>(data, data_len, i));
				b.finalize(DynMinus);
				break;
			}
			case Opcode::div: {
				BuildCall b(a);
				b.leaEnviro(readData<uint16_t>(data, data_len, i));
				b.leaEnviro(readData<uint16_t>(data, data_len, i));
				b.finalize(DynDiv);
				break;
			}
			case Opcode::rest: {
				BuildCall b(a);
				b.leaEnviro(readData<uint16_t>(data, data_len, i));
				b.leaEnviro(readData<uint16_t>(data, data_len, i));
				b.finalize(DynRest);
				break;
			}
			case Opcode::mul: {
				BuildCall b(a);
				b.leaEnviro(readData<uint16_t>(data, data_len, i));
				b.leaEnviro(readData<uint16_t>(data, data_len, i));
				b.finalize(DynMul);
				break;
			}
			case Opcode::bit_xor: {
				BuildCall b(a);
				b.leaEnviro(readData<uint16_t>(data, data_len, i));
				b.leaEnviro(readData<uint16_t>(data, data_len, i));
				b.finalize(DynBitXor);
				break;
			}
			case Opcode::bit_or: {
				BuildCall b(a);
				b.leaEnviro(readData<uint16_t>(data, data_len, i));
				b.leaEnviro(readData<uint16_t>(data, data_len, i));
				b.finalize(DynBitOr);
				break;
			}
			case Opcode::bit_and: {
				BuildCall b(a);
				b.leaEnviro(readData<uint16_t>(data, data_len, i));
				b.leaEnviro(readData<uint16_t>(data, data_len, i));
				b.finalize(DynBitAnd);
				break;
			}
			case Opcode::bit_not: {
				BuildCall b(a);
				b.leaEnviro(readData<uint16_t>(data, data_len, i));
				b.finalize(DynBitNot);
				break;
			}
			case Opcode::log_not: {
				a.load_flag8h();
				a.xor_(resr_8h, RFLAGS::bit::zero & RFLAGS::bit::carry);
				a.store_flag8h();
				break;
			}
			case Opcode::compare: {
				a.push_flags();
				a.pop(argr0_16);

				BuildCall b(a);
				b.skip();
				b.leaEnviro(readData<uint16_t>(data, data_len, i));
				b.leaEnviro(readData<uint16_t>(data, data_len, i));
				b.finalize(compare);
				a.push(resr_16);
				a.pop_flags();
				break;
			}
			case Opcode::jump: {
				uint64_t to_find = readData<uint64_t>(data, data_len, i);
				auto found = std::find_if(j_list.begin(), j_list.end(), [i](std::pair<uint64_t, Label>& it) { return it.first == i;  });
				if (found == j_list.end())
					throw InvalidFunction("Invalid function header, not found jump position");
				switch (readData<JumpCondition>(data, data_len, i))
				{
					default:
					case JumpCondition::no_condition: {
						a.jmp(found->second);
						break;
					}
					case JumpCondition::is_equal: {
						a.jmp_eq(found->second);
						break;
					}
					case JumpCondition::is_not_equal: {
						a.jmp_not_eq(found->second);
						break;
					}
					case JumpCondition::is_more: {
						a.jmp_more(found->second);
						break;
					}
					case JumpCondition::is_lower: {
						a.jmp_lower(found->second);
						break;
					}
					case JumpCondition::is_more_or_eq: {
						a.jmp_more_or_eq(found->second);
						break;
					}
					case JumpCondition::is_lower_or_eq: {
						a.jmp_lower_or_eq(found->second);
						break;
					}
				}
				break;
			}
			case Opcode::arg_set: {
				BuildCall b(a);
				b.leaEnviro(readData<uint16_t>(data, data_len, i));
				b.finalize(AsArg);
				a.mov(arg_ptr, resr);
				break;
			}
			case Opcode::call: {
				compilerFabric_call<true>(a, data, data_len, i, values);
				break;
			}
			case Opcode::call_self: {
				CallFlags flags;
				flags.encoded = readData<uint8_t>(data, data_len, i);
				if (flags.async_mode)
					throw CompileTimeException("Fail compile async 'call_self', for asynchonly call self use 'call' command");
				BuildCall b(a);
				b.addArg(this);
				b.addArg(arg_ptr);
				if (flags.except_catch)
					b.finalize(&FuncEnviropment::initAndCall_catch);
				else
					b.finalize(&FuncEnviropment::initAndCall);

				if (flags.use_result) {
					b.leaEnviro(readData<uint16_t>(data, data_len, i));
					b.addArg(resr);
					b.finalize(getValueItem);
				}
				else {
					inlineReleaseUnused(a, resr);
				}
				break;
			}
			case Opcode::call_local: {
				compilerFabric_call_local<true>(a, data, data_len, i, this);
				break;
			}
			case Opcode::call_and_ret: {
				compilerFabric_call<false, false>(a, data, data_len, i, values);
				do_jump_to_ret = true;
				break;
			}
			case Opcode::call_self_and_ret: {
				CallFlags flags;
				flags.encoded = readData<uint8_t>(data, data_len, i);
				if (flags.async_mode)
					throw CompileTimeException("Fail compile async 'call_self', for asynchonly call self use 'call' command");
				BuildCall b(a);
				b.addArg(this);
				b.addArg(arg_ptr);
				if (flags.except_catch)
					b.finalize(&FuncEnviropment::initAndCall_catch);
				else
					b.finalize(&FuncEnviropment::initAndCall);
				do_jump_to_ret = true;
				break;
			}
			case Opcode::call_local_and_ret: {
				compilerFabric_call_local<false, false>(a, data, data_len, i, this);
				do_jump_to_ret = true;
				break;
			}
			case Opcode::ret: {
				BuildCall b(a);
				b.leaEnviro(readData<uint16_t>(data, data_len, i));
				b.finalize(buildRes);
				do_jump_to_ret = true;
				break;
			}
			case Opcode::ret_noting: {
				a.xor_(resr, resr);
				do_jump_to_ret = true;
				break;
			}
			case Opcode::copy: {
				uint16_t from = readData<uint16_t>(data, data_len, i);
				uint16_t to = readData<uint16_t>(data, data_len, i);
				if (from != to) {
					BuildCall b(a);
					b.leaEnviro(to);
					b.finalize(universalRemove);
					b.movEnviro(from);
					b.movEnviroMeta(from);
					b.finalize(copyValue);
					a.movEnviro(to, resr);
					a.movEnviroMeta(resr, from);
					a.movEnviroMeta(to, resr);
				}
				break;
			}
			case Opcode::move: {
				uint16_t from = readData<uint16_t>(data, data_len, i);
				uint16_t to = readData<uint16_t>(data, data_len, i);
				if (from != to) {
					BuildCall b(a);
					b.leaEnviro(to);
					b.finalize(universalRemove);
					a.movEnviro(resr, from);
					a.movEnviro(to, resr);
					a.movEnviroMeta(resr, from);
					a.movEnviroMeta(to, resr);
					a.movEnviroMeta(from, 0);
				}
				break;
			}
			case Opcode::arr_op: {
				uint16_t arr = readData<uint16_t>(data, data_len, i);
				BuildCall b(a);
				b.leaEnviro(arr);
				b.finalize(AsArr);
				auto flags = readData<OpArrFlags>(data, data_len, i);
				switch (readData<OpcodeArray>(data, data_len, i)) {
				case OpcodeArray::set: {
					if (flags.by_val_mode) {
						b.leaEnviro(readData<uint16_t>(data, data_len, i));
						b.finalize(getSize);
						b.leaEnviro(readData<uint16_t>(data, data_len, i));
						b.leaEnviro(arr);
						b.addArg(resr);
					}
					else {
						b.leaEnviro(readData<uint16_t>(data, data_len, i));
						b.leaEnviro(arr);
						b.addArg(readData<uint64_t>(data, data_len, i));
					}
					if (flags.move_mode) {
						switch (flags.checked)
						{
						case OpArrFlags::CheckMode::no_check:
							b.finalize(IndexArraySetMove<0>);
							break;
						case OpArrFlags::CheckMode::check:
							b.finalize(IndexArraySetMove<1>);
							break;
						case OpArrFlags::CheckMode::no_throw_check:
							b.finalize(IndexArraySetMove<2>);
							break;
						default:
							break;
						}
					}
					else {
						switch (flags.checked)
						{
						case OpArrFlags::CheckMode::no_check:
							b.finalize(IndexArraySetCopy<0>);
							break;
						case OpArrFlags::CheckMode::check:
							b.finalize(IndexArraySetCopy<1>);
							break;
						case OpArrFlags::CheckMode::no_throw_check:
							b.finalize(IndexArraySetCopy<2>);
							break;
						default:
							break;
						}
					}
					break;
				}
				case OpcodeArray::insert: {
					if (flags.by_val_mode) {
						b.leaEnviro(readData<uint16_t>(data, data_len, i));
						b.finalize(getSize);
						b.leaEnviro(arr);
						b.addArg(resr);
						b.leaEnviro(readData<uint16_t>(data, data_len, i));
					}
					else {
						b.leaEnviro(arr);
						b.addArg(readData<uint64_t>(data, data_len, i));
						b.leaEnviro(readData<uint16_t>(data, data_len, i));
					}
					if (flags.move_mode)
						b.finalize((void (list_array<ValueItem>::*)(size_t,const ValueItem&)) & list_array<ValueItem>::insert);
					else
						b.finalize((void (list_array<ValueItem>::*)(size_t, ValueItem&&)) & list_array<ValueItem>::insert);
					break;
				}
				case OpcodeArray::push_end: {
					b.movEnviro(arr);
					b.leaEnviro(readData<uint16_t>(data, data_len, i));
					if (flags.move_mode)
						b.finalize((void (list_array<ValueItem>::*)(ValueItem&&)) & list_array<ValueItem>::push_back);
					else
						b.finalize((void (list_array<ValueItem>::*)(const ValueItem&)) & list_array<ValueItem>::push_back);
					break;
				}
				case OpcodeArray::push_start: {
					b.movEnviro(arr);
					b.leaEnviro(readData<uint16_t>(data, data_len, i));
					if (flags.move_mode)
						b.finalize((void (list_array<ValueItem>::*)(ValueItem&&)) & list_array<ValueItem>::push_front);
					else
						b.finalize((void (list_array<ValueItem>::*)(const ValueItem&)) & list_array<ValueItem>::push_front);
					break;
				}
				case OpcodeArray::insert_range: {
					uint16_t arr1 = readData<uint16_t>(data, data_len, i);
					b.leaEnviro(arr1);
					b.finalize(AsArr);
					if (flags.by_val_mode) {
						uint16_t val1 = readData<uint16_t>(data, data_len, i);
						uint16_t val2 = readData<uint16_t>(data, data_len, i);
						uint16_t val3 = readData<uint16_t>(data, data_len, i);

						b.leaEnviro(val3);
						b.finalize(getSize);
						a.push(resr);

						b.leaEnviro(val2);
						b.finalize(getSize);
						a.push(resr);

						b.leaEnviro(val1);
						b.finalize(getSize);
						b.movEnviro(arr);
						b.addArg(resr);//1
						b.leaEnviro(arr1);

						a.pop(resr);
						b.addArg(resr);//2
						a.pop(resr);
						b.addArg(resr);//3
					}
					else {
						b.movEnviro(arr);
						b.addArg(readData<uint64_t>(data, data_len, i));//1
						b.leaEnviro(arr1);

						b.addArg(readData<uint64_t>(data, data_len, i));//2
						b.addArg(readData<uint64_t>(data, data_len, i));//3
					}

					b.finalize((void(list_array<ValueItem>::*)(size_t, const list_array<ValueItem>&, size_t, size_t)) & list_array<ValueItem>::insert);
					break;
				}
				case OpcodeArray::get: {
					if (flags.by_val_mode) {
						b.leaEnviro(readData<uint16_t>(data, data_len, i));
						b.finalize(getSize);
						b.leaEnviro(readData<uint16_t>(data, data_len, i));
						b.leaEnviro(arr);
						b.addArg(resr);
					}
					else {
						b.leaEnviro(readData<uint16_t>(data, data_len, i));
						b.leaEnviro(arr);
						b.addArg(readData<uint64_t>(data, data_len, i));
					}
					if (flags.move_mode) {
						switch (flags.checked)
						{
						case OpArrFlags::CheckMode::no_check:
							b.finalize(IndexArrayMove<0>);
							break;
						case OpArrFlags::CheckMode::check:
							b.finalize(IndexArrayMove<1>);
							break;
						case OpArrFlags::CheckMode::no_throw_check:
							b.finalize(IndexArrayMove<2>);
							break;
						default:
							break;
						}
					}
					else {
						switch (flags.checked)
						{
						case OpArrFlags::CheckMode::no_check:
							b.finalize(IndexArrayCopy<0>);
							break;
						case OpArrFlags::CheckMode::check:
							b.finalize(IndexArrayCopy<1>);
							break;
						case OpArrFlags::CheckMode::no_throw_check:
							b.finalize(IndexArrayCopy<2>);
							break;
						default:
							break;
						}
					}
					break;
				}
				case OpcodeArray::take: {
					if (flags.by_val_mode) {
						b.leaEnviro(readData<uint16_t>(data, data_len, i));
						b.finalize(getSize);
						b.addArg(resr);
						b.leaEnviro(arr);
						b.leaEnviro(readData<uint16_t>(data, data_len, i));
					}
					else {
						b.addArg(readData<uint64_t>(data, data_len, i));
						b.leaEnviro(arr);
						b.leaEnviro(readData<uint16_t>(data, data_len, i));
					}
					b.finalize(take);
					break;
				}
				case OpcodeArray::take_end: {
					b.leaEnviro(arr);
					b.leaEnviro(readData<uint16_t>(data, data_len, i));
					b.finalize(takeEnd);
					break; 
				}
				case OpcodeArray::take_start: {
					b.leaEnviro(arr);
					b.leaEnviro(readData<uint16_t>(data, data_len, i));
					b.finalize(takeStart);
					break; 
				}
				case OpcodeArray::get_range: {
					if (flags.by_val_mode) {
						uint16_t set_to = readData<uint16_t>(data, data_len, i);
						b.leaEnviro(readData<uint16_t>(data, data_len, i));
						b.finalize(getSize);
						a.push(resr);
						b.leaEnviro(readData<uint16_t>(data, data_len, i));
						b.finalize(getSize);
						a.pop(mut_temp_ptr);
						b.addArg(set_to);
						b.addArg(mut_temp_ptr);
						b.addArg(resr);
					}
					else {
						b.leaEnviro(arr);
						b.leaEnviro(readData<uint16_t>(data, data_len, i));
						b.addArg(readData<uint64_t>(data, data_len, i));
						b.addArg(readData<uint64_t>(data, data_len, i));
					}
					b.finalize(getRange);
					break;
				}
				case OpcodeArray::take_range: {
					if (flags.by_val_mode) {
						uint16_t set_to = readData<uint16_t>(data, data_len, i);
						b.leaEnviro(readData<uint16_t>(data, data_len, i));
						b.finalize(getSize);
						a.push(resr);
						b.leaEnviro(readData<uint16_t>(data, data_len, i));
						b.finalize(getSize);
						a.pop(mut_temp_ptr);
						b.addArg(set_to);
						b.addArg(mut_temp_ptr);
						b.addArg(resr);
					}
					else {
						b.leaEnviro(arr);
						b.leaEnviro(readData<uint16_t>(data, data_len, i));
						b.addArg(readData<uint64_t>(data, data_len, i));
						b.addArg(readData<uint64_t>(data, data_len, i));
					}
					b.finalize(takeRange);
					break;
				}
				case OpcodeArray::pop_end: {
					b.leaEnviro(arr);
					b.finalize(&list_array<ValueItem>::pop_back);
					break;
				}
				case OpcodeArray::pop_start: {
					b.leaEnviro(arr);
					b.finalize(&list_array<ValueItem>::pop_front);
					break;
				}
				case OpcodeArray::remove_item: {
					b.movEnviro(arr);
					b.movEnviro(readData<uint64_t>(data, data_len, i));
					b.finalize((void(list_array<ValueItem>::*)(size_t pos)) & list_array<ValueItem>::remove);
					break;
				}
				case OpcodeArray::remove_range: {
					b.movEnviro(arr);
					b.addArg(readData<uint64_t>(data, data_len, i));
					b.addArg(readData<uint64_t>(data, data_len, i));
					b.finalize((void(list_array<ValueItem>::*)(size_t, size_t)) & list_array<ValueItem>::remove);
					break;
				}

				case OpcodeArray::resize: {
					if (flags.by_val_mode) {
						b.leaEnviro(readData<uint16_t>(data, data_len, i));
						b.finalize(getSize);
						b.leaEnviro(arr);
						b.addArg(resr);
					}
					else {
						b.movEnviro(arr);
						b.addArg(readData<uint64_t>(data, data_len, i));
					}
					b.finalize((void(list_array<ValueItem>::*)(size_t)) & list_array<ValueItem>::resize);
					break;
				}
				case OpcodeArray::resize_default: {
					if (flags.by_val_mode) {
						b.leaEnviro(readData<uint16_t>(data, data_len, i));
						b.finalize(getSize);
						b.leaEnviro(arr);
						b.addArg(resr);
					}
					else {
						b.movEnviro(arr);
						b.addArg(readData<uint64_t>(data, data_len, i));
					}
					b.leaEnviro(readData<uint16_t>(data, data_len, i));
					b.finalize((void(list_array<ValueItem>::*)(size_t, const ValueItem&)) & list_array<ValueItem>::resize);
					break;
				}
				case OpcodeArray::reserve_push_end: {
					b.leaEnviro(arr);
					b.movEnviro(readData<uint64_t>(data, data_len, i));
					b.finalize(&list_array<ValueItem>::reserve_push_back);
					break;
				}
				case OpcodeArray::reserve_push_start: {
					b.leaEnviro(arr);
					if (flags.by_val_mode) {
						b.leaEnviro(readData<uint16_t>(data, data_len, i));
						b.finalize(getSize);
						b.addArg(resr);
					}
					else
						b.movEnviro(readData<uint64_t>(data, data_len, i));
					b.finalize(&list_array<ValueItem>::reserve_push_front);
					break;
				}

				case OpcodeArray::commit: {
					BuildCall b(a);
					b.movEnviro(arr);
					b.finalize(&list_array<ValueItem>::commit);
					break;
				}
				case OpcodeArray::decommit: {
					BuildCall b(a);
					b.movEnviro(arr);
					b.addArg(readData<uint64_t>(data, data_len, i));
					b.finalize(&list_array<ValueItem>::decommit);
					break;
				}
				case OpcodeArray::remove_reserved: {
					BuildCall b(a);
					b.movEnviro(arr);
					b.finalize(&list_array<ValueItem>::shrink_to_fit);
					break;
				}
				case OpcodeArray::size: {
					b.movEnviro(arr);
					b.finalize(&list_array<ValueItem>::size);
					b.leaEnviro(readData<uint16_t>(data, data_len, i));
					b.addArg(resr);
					b.finalize(setSize);
					break;
				}
				default:
					throw CompileTimeException("Invalid array operation");
				}
				break;
			}
			case Opcode::debug_break:
				if (!in_debug)
					break;
				[[fallthrough]];
			case Opcode::force_debug_break:
				a.int3();
				break;
			case Opcode::throw_ex: {
				bool in_memory = readData<bool>(data, data_len, i);
				if (in_memory) {
					BuildCall b(a);
					b.leaEnviro(readData<uint16_t>(data, data_len, i));
					b.leaEnviro(readData<uint16_t>(data, data_len, i));
					b.finalize(throwDirEx);
				}
				else {
					auto ex_typ = string_index_seter++;
					values[ex_typ] = readString(data, data_len, i);
					auto ex_desc = string_index_seter++;
					values[ex_desc] = readString(data, data_len, i);
					BuildCall b(a);
					b.addArg(values[ex_typ].val);
					b.addArg(values[ex_desc].val);
					b.finalize(throwEx);
				}
				break;
			}
			case Opcode::as: {
				BuildCall b(a);
				b.leaEnviro(readData<uint16_t>(data, data_len, i));
				b.addArg(readData<VType>(data, data_len, i));
				b.finalize(asValue);
				break;
			}
			case Opcode::is: {
				a.movEnviroMeta(resr, readData<uint16_t>(data, data_len, i));
				a.mov(argr0_8l, readData<VType>(data, data_len, i));
				a.cmp(resr_8l , argr0_8l);
				break;
			}
			case Opcode::store_bool: {
				BuildCall b(a);
				b.leaEnviro(readData<uint16_t>(data, data_len, i));
				b.finalize(isTrueValue);
				a.load_flag8h();
				a.shift_left(resr_8l, RFLAGS::off_left::zero);
				a.or_(resr_8h, resr_8l);
				a.store_flag8h();
				break;
			}
			case Opcode::load_bool: {
				a.load_flag8h();
				a.and_(resr_8h, RFLAGS::bit::zero);
				a.store_flag8h();
				BuildCall b(a);
				b.addArg(resr_8h);
				b.leaEnviro(readData<uint16_t>(data, data_len, i));
				b.finalize(setBoolValue);
				break;
			}
			case Opcode::make_inline_call: {
				InlineCallFlags flags = readData<InlineCallFlags>(data, data_len, i);
				std::string fnn = readString(data, data_len, i);
				auto tmp = FuncEnviropment::enviropment(fnn);
				if (!tmp)
					throw CompileTimeException("Function: \"" + fnn + "\" not found or not inited");
				if (flags.except_catch) {
					//TO-DO
				}
				else {

					//TO-DO
				}
				if (flags.use_result) {
					BuildCall b(a);
					b.leaEnviro(readData<uint16_t>(data, data_len, i));
					b.addArg(resr);
					b.finalize(getValueItem);
				}
				else
					inlineReleaseUnused(a, resr);
				break;
			}
			case Opcode::make_inline_call_and_ret: {
				InlineCallFlags flags = readData<InlineCallFlags>(data, data_len, i);
				std::string fnn = readString(data, data_len, i);
				auto tmp = FuncEnviropment::enviropment(fnn);
				if (!tmp)
					throw CompileTimeException("Function: \"" + fnn + "\" not found or not inited");
				if (flags.except_catch) {
					//TO-DO
				}
				else {

					//TO-DO
				}
				if (flags.use_result) {
					BuildCall b(a);
					b.leaEnviro(readData<uint16_t>(data, data_len, i));
					b.addArg(resr);
					b.finalize(getValueItem);
				}
				else
					inlineReleaseUnused(a, resr);
				break;
			}
			case Opcode::casm: {
				//TO-DO
			}
			case Opcode::inline_native://[len]{data} insert all bytes as instructions in current function
			{
				//TO-DO
			}
			default:
				throw CompileTimeException("Invalid opcode");
			}
		};
		


		for (; i < data_len; ) {
			for(auto& it : j_list) 
				if (it.first == i) 
					a.label_bind(it.second);

			if (do_jump_to_ret)
				a.jmp(prolog);
			do_jump_to_ret = false;
			Command cmd(data[i++]);
			!cmd.static_mode ? dynamic_fablric(cmd) : static_fabric(cmd);
		}
	};
	compilerFabric(a, jump_list, to_be_skiped);
	a.label_bind(prolog);
	auto& tmp = bprolog.finalize();

	curr_func = (Enviropment)tmp.init(frame, a.code(), jrt, "attach_a_symbol");
}

ValueItem* FuncEnviropment::initAndCall(list_array<ValueItem>* arguments) {
	ValueItem* res = nullptr;
	try {
		EnviroHold env(
			(
				max_values ?
				(void**)alloca(sizeof(void*) * (size_t(max_values) << 1)) :
				nullptr
				)
			, max_values
		);
		res = curr_func(env.envir, arguments);
	}
	catch (const StackOverflowException&) {
		if (!need_restore_stack_fault()) {
			--current_runners;
			throw;
		}
	}
	catch (...) {
		--current_runners;
		throw;
	}
	--current_runners;
	if (restore_stack_fault())
		throw StackOverflowException();
	return res;
}

ValueItem* FuncEnviropment::syncWrapper(list_array<ValueItem>* args) {
	if (need_compile)
		funcComp();
	current_runners++;
	switch (type) {
	case FuncEnviropment::FuncType::own:
		return initAndCall(args);
	case FuncEnviropment::FuncType::native:
		return NativeProxy_DynamicToStatic(args);
	case FuncEnviropment::FuncType::native_own_abi:
		return ((AttachACXX)curr_func)(args);
	case FuncEnviropment::FuncType::python:
	case FuncEnviropment::FuncType::csharp:
	case FuncEnviropment::FuncType::java:
	default:
		throw NotImplementedException();
	}
}
ValueItem* FuncEnviropment::syncWrapper_catch(list_array<ValueItem>* args) {
	try {
		return syncWrapper(args);
	}
	catch (...) {
		return new ValueItem(new std::exception_ptr(std::current_exception()), ValueMeta(VType::except_value, false, true), true);
	}
}


void NativeProxy_DynamicToStatic_addValue(DynamicCall::FunctionCall& call, ValueMeta meta,void*& arg) {
	if (!meta.allow_edit && meta.vtype == VType::string) {
		switch (meta.vtype) {
		case VType::noting:
			call.AddValueArgument((void*)0);
			break;
		case VType::i8:
			call.AddValueArgument((int8_t)arg);
			break;
		case VType::i16:
			call.AddValueArgument((int16_t)arg);
			break;
		case VType::i32:
			call.AddValueArgument((int32_t)arg);
			break;
		case VType::i64:
			call.AddValueArgument((int64_t)arg);
			break;
		case VType::ui8:
			call.AddValueArgument((uint8_t)arg);
			break;
		case VType::ui16:
			call.AddValueArgument((int16_t)arg);
			break;
		case VType::ui32:
			call.AddValueArgument((uint32_t)arg);
			break;
		case VType::ui64:
			call.AddValueArgument((uint64_t)arg);
			break;
		case VType::flo:
			call.AddValueArgument(*(float*)&arg);
			break;
		case VType::doub:
			call.AddValueArgument(*(double*)&arg);
			break;
		case VType::raw_arr_i8:
			call.AddArray((int8_t*)arg,meta.val_len);
			break;
		case VType::raw_arr_i16:
			call.AddArray((int16_t*)arg, meta.val_len);
			break;
		case VType::raw_arr_i32:
			call.AddArray((int32_t*)arg, meta.val_len);
			break;
		case VType::raw_arr_i64:
			call.AddArray((int64_t*)arg, meta.val_len);
			break;
		case VType::raw_arr_ui8:
			call.AddArray((uint8_t*)arg, meta.val_len);
			break;
		case VType::raw_arr_ui16:
			call.AddArray((int16_t*)arg, meta.val_len);
			break;
		case VType::raw_arr_ui32:
			call.AddArray((uint32_t*)arg, meta.val_len);
			break;
		case VType::raw_arr_ui64:
			call.AddArray((uint64_t*)arg, meta.val_len);
			break;
		case VType::raw_arr_flo:
			call.AddArray((float*)arg, meta.val_len);
			break;
		case VType::raw_arr_doub:
			call.AddArray((double*)arg, meta.val_len);
			break;
		case VType::uarr:
			call.AddPtrArgument((const list_array<ValueItem>*)arg);
			break;
		case VType::string:
			call.AddArray(((std::string*)arg)->data(), ((std::string*)arg)->size());
			break;
		case VType::undefined_ptr:
			call.AddPtrArgument(arg);
			break;
		default:
			throw NotImplementedException();
		}
	}
	else {
		switch (meta.vtype) {
		case VType::noting:
			call.AddValueArgument((void*)0);
			break;
		case VType::i8:
			call.AddValueArgument((int8_t)arg);
			break;
		case VType::i16:
			call.AddValueArgument((int16_t)arg);
			break;
		case VType::i32:
			call.AddValueArgument((int32_t)arg);
			break;
		case VType::i64:
			call.AddValueArgument((int64_t)arg);
			break;
		case VType::ui8:
			call.AddValueArgument((uint8_t)arg);
			break;
		case VType::ui16:
			call.AddValueArgument((int16_t)arg);
			break;
		case VType::ui32:
			call.AddValueArgument((uint32_t)arg);
			break;
		case VType::ui64:
			call.AddValueArgument((uint64_t)arg);
			break;
		case VType::flo:
			call.AddValueArgument(*(float*)&arg);
			break;
		case VType::doub:
			call.AddValueArgument(*(double*)&arg);
			break;
		case VType::raw_arr_i8:
			call.AddPtrArgument((int8_t*)arg);
			break;
		case VType::raw_arr_i16:
			call.AddPtrArgument((int16_t*)arg);
			break;
		case VType::raw_arr_i32:
			call.AddPtrArgument((int32_t*)arg);
			break;
		case VType::raw_arr_i64:
			call.AddPtrArgument((int64_t*)arg);
			break;
		case VType::raw_arr_ui8:
			call.AddPtrArgument((uint8_t*)arg);
			break;
		case VType::raw_arr_ui16:
			call.AddPtrArgument((int16_t*)arg);
			break;
		case VType::raw_arr_ui32:
			call.AddPtrArgument((uint32_t*)arg);
			break;
		case VType::raw_arr_ui64:
			call.AddPtrArgument((uint64_t*)arg);
			break;
		case VType::raw_arr_flo:
			call.AddPtrArgument((float*)arg);
			break;
		case VType::raw_arr_doub:
			call.AddPtrArgument((double*)arg);
			break;
		case VType::uarr:
			call.AddPtrArgument((list_array<ValueItem>*)arg);
			break;
		case VType::string:
			call.AddPtrArgument(((std::string*)arg)->data());
			break;
		case VType::undefined_ptr:
			call.AddPtrArgument(arg);
			break;
		default:
			throw NotImplementedException();
		}
	}
}
ValueItem* FuncEnviropment::NativeProxy_DynamicToStatic(list_array<ValueItem>* arguments) {
	using namespace DynamicCall;
	FunctionCall call((DynamicCall::PROC)curr_func, nat_templ, true);
	if (arguments) {
		for (ValueItem& it : *arguments) {
			auto to_add = call.ToAddArgument();
			if (to_add.is_void()) {
				if (call.is_variadic()) {
					void*& val = getValue(it.val, it.meta);
					NativeProxy_DynamicToStatic_addValue(call, it.meta, val);
					continue;
				}
				else
					break;
			}
			void*& arg = getValue(it.val, it.meta);
			ValueMeta meta = it.meta;
			switch (to_add.vtype) {
			case FunctionTemplate::ValueT::ValueType::integer:
			case FunctionTemplate::ValueT::ValueType::signed_integer:
			case FunctionTemplate::ValueT::ValueType::floating:
				if (to_add.ptype == FunctionTemplate::ValueT::PlaceType::as_value) {
					switch (meta.vtype) {
					case VType::i8:
						call.AddValueArgument((int8_t)arg);
						break;
					case VType::i16:
						call.AddValueArgument((int16_t)arg);
						break;
					case VType::i32:
						call.AddValueArgument((int32_t)arg);
						break;
					case VType::i64:
						call.AddValueArgument((int64_t)arg);
						break;
					case VType::ui8:
						call.AddValueArgument((uint8_t)arg);
						break;
					case VType::ui16:
						call.AddValueArgument((uint16_t)arg);
						break;
					case VType::ui32:
						call.AddValueArgument((uint32_t)arg);
						break;
					case VType::ui64:
						call.AddValueArgument((uint64_t)arg);
						break;
					case VType::flo:
						call.AddValueArgument(*(float*)&arg);
						break;
					case VType::doub:
						call.AddValueArgument(*(double*)&arg);
						break;
					default:
						throw InvalidType("Required integer or floating family type but requived another");
					}
				}
				else {
					switch (meta.vtype) {
					case VType::i8:
						call.AddValueArgument((int8_t*)&arg);
						break;
					case VType::i16:
						call.AddValueArgument((int16_t*)&arg);
						break;
					case VType::i32:
						call.AddValueArgument((int32_t*)&arg);
						break;
					case VType::i64:
						call.AddValueArgument((int64_t*)&arg);
						break;
					case VType::ui8:
						call.AddValueArgument((uint8_t*)&arg);
						break;
					case VType::ui16:
						call.AddValueArgument((uint16_t*)&arg);
						break;
					case VType::ui32:
						call.AddValueArgument((uint32_t*)&arg);
						break;
					case VType::ui64:
						call.AddValueArgument((uint64_t*)&arg);
						break;
					case VType::flo:
						call.AddValueArgument((float*)&arg);
						break;
					case VType::doub:
						call.AddValueArgument((double*)&arg);
						break; 
					case VType::raw_arr_i8:
						call.AddValueArgument((int8_t*)arg);
						break;
					case VType::raw_arr_i16:
						call.AddValueArgument((int16_t*)arg);
						break;
					case VType::raw_arr_i32:
						call.AddValueArgument((int32_t*)arg);
						break;
					case VType::raw_arr_i64:
						call.AddValueArgument((int64_t*)arg);
						break;
					case VType::raw_arr_ui8:
						call.AddValueArgument((uint8_t*)arg);
						break;
					case VType::raw_arr_ui16:
						call.AddValueArgument((uint16_t*)arg);
						break;
					case VType::raw_arr_ui32:
						call.AddValueArgument((uint32_t*)arg);
						break;
					case VType::raw_arr_ui64:
						call.AddValueArgument((uint64_t*)arg);
						break;
					case VType::raw_arr_flo:
						call.AddValueArgument((float*)arg);
						break;
					case VType::raw_arr_doub:
						call.AddValueArgument((double*)arg);
						break;
					case VType::string:
						call.AddValueArgument(((std::string*)arg)->data());
						break;
					default:
						throw InvalidType("Required integer or floating family type but requived another");
					}
				}
				break;
			case FunctionTemplate::ValueT::ValueType::pointer:
				switch (meta.vtype) {
				case VType::raw_arr_i8:
					call.AddValueArgument((int8_t*)arg);
					break;
				case VType::raw_arr_i16:
					call.AddValueArgument((int16_t*)arg);
					break;
				case VType::raw_arr_i32:
					call.AddValueArgument((int32_t*)arg);
					break;
				case VType::raw_arr_i64:
					call.AddValueArgument((int64_t*)arg);
					break;
				case VType::raw_arr_ui8:
					call.AddValueArgument((uint8_t*)arg);
					break;
				case VType::raw_arr_ui16:
					call.AddValueArgument((uint16_t*)arg);
					break;
				case VType::raw_arr_ui32:
					call.AddValueArgument((uint32_t*)arg);
					break;
				case VType::raw_arr_ui64:
					call.AddValueArgument((uint64_t*)arg);
					break;
				case VType::raw_arr_flo:
					call.AddValueArgument((float*)arg);
					break;
				case VType::raw_arr_doub:
					call.AddValueArgument((double*)arg);
					break;
				case VType::string:
					if (to_add.vsize == 1) {
						if (to_add.is_modifable)
							call.AddPtrArgument(((std::string*)arg)->data());
						else
							call.AddArray(((std::string*)arg)->c_str(), ((std::string*)arg)->size());
					}
					else if (to_add.vsize == 2) {


					}
					else if (to_add.vsize == sizeof(std::string))
						call.AddPtrArgument((std::string*)arg);
					break;
				case VType::undefined_ptr:
					call.AddPtrArgument(arg);
					break;
				default:
					throw InvalidType("Required pointer family type but requived another");
				}
				break;
			case FunctionTemplate::ValueT::ValueType::_class:
			default:
				break;
			}
		}
	}
	void* res = nullptr;
	try {
		res = call.Call();
		--current_runners;
	}
	catch (const StackOverflowException&) {
		--current_runners;
	}
	catch (...) {
		--current_runners;
		throw;
	}
	if (restore_stack_fault())
		throw StackOverflowException();

	if (nat_templ.result.is_void())
		return nullptr;
	if(nat_templ.result.ptype == DynamicCall::FunctionTemplate::ValueT::PlaceType::as_ptr)
		return new ValueItem(res, ValueMeta(VType::undefined_ptr, false, true));
	switch (nat_templ.result.vtype) {
	case DynamicCall::FunctionTemplate::ValueT::ValueType::integer:
		switch (nat_templ.result.vsize) {
		case 1:
			return new ValueItem(res, ValueMeta(VType::ui8, false, true));
		case 2:
			return new ValueItem(res, ValueMeta(VType::ui16, false, true));
		case 4:
			return new ValueItem(res, ValueMeta(VType::ui32, false, true));
		case 8:
			return new ValueItem(res, ValueMeta(VType::ui64, false, true));
		default:
			throw InvalidCast("Invalid type for convert");
		}
	case DynamicCall::FunctionTemplate::ValueT::ValueType::signed_integer:
		switch (nat_templ.result.vsize) {
		case 1:
			return new ValueItem(res, ValueMeta(VType::i8, false, true));
		case 2:
			return new ValueItem(res, ValueMeta(VType::i16, false, true));
		case 4:
			return new ValueItem(res, ValueMeta(VType::i32, false, true));
		case 8:
			return new ValueItem(res, ValueMeta(VType::i64, false, true));
		default:
			throw InvalidCast("Invalid type for convert");
		}
	case DynamicCall::FunctionTemplate::ValueT::ValueType::floating:		
		switch (nat_templ.result.vsize) {
		case 1:
			return new ValueItem(res, ValueMeta(VType::ui8, false, true));
		case 2:
			return new ValueItem(res, ValueMeta(VType::ui16, false, true));
		case 4:
			return new ValueItem(res, ValueMeta(VType::flo, false, true));
		case 8:
			return new ValueItem(res, ValueMeta(VType::doub, false, true));
		default:
			throw InvalidCast("Invalid type for convert");
		}
	case DynamicCall::FunctionTemplate::ValueT::ValueType::pointer:
		return new ValueItem(res, ValueMeta(VType::undefined_ptr, false, true));
	case DynamicCall::FunctionTemplate::ValueT::ValueType::_class:
	default:
		throw NotImplementedException();
	}
}

ValueItem* FuncEnviropment::native_proxy_catch(list_array<ValueItem>* args) {
	try {
		return NativeProxy_DynamicToStatic(args);
	}
	catch (...) {
		return new ValueItem(new std::exception_ptr(std::current_exception()), ValueMeta(VType::except_value, false, true), true);
	}
}
ValueItem* FuncEnviropment::initAndCall_catch(list_array<ValueItem>* args) {
	try {
		return initAndCall(args);
	}
	catch (...) {
		return new ValueItem(new std::exception_ptr(std::current_exception()), ValueMeta(VType::except_value, false, true), true);
	}
}

FuncEnviropment::~FuncEnviropment() {
	std::lock_guard lguard(compile_lock);
	if (can_be_unloaded) {
		while (current_runners)
			std::this_thread::sleep_for(std::chrono::milliseconds(5));
		if(type == FuncType::own)
			if (curr_func)
				FrameResult::deinit(frame, curr_func, jrt);
	}
}

extern "C" void callFunction(const char* symbol_name, bool run_async) {
	list_array<ValueItem> empty;
	ValueItem* res = FuncEnviropment::CallFunc(symbol_name, &empty, run_async);
	if (res)
		delete res;
}





#pragma region FuncEviroBuilder
void FuncEviroBuilder::setConstant(uint16_t val, const ValueItem& cv, bool is_dynamic) {
	const_cast<ValueItem&>(cv).getAsync();
	code.push_back(Command(Opcode::set, cv.meta.use_gc, is_dynamic).toCmd());
	builder::write(code, val);
	builder::writeAny(code, const_cast<ValueItem&>(cv));
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


//void FuncEviroBuilder::inlineNative(const uint8_t* raw, size_t len);
//void FuncEviroBuilder::inlineNative(const int8_t* raw, size_t len);
//void FuncEviroBuilder::inlineNative(const char* raw, size_t len);
//
//
//void FuncEviroBuilder::inline_call(const std::string& fn_name);
//void FuncEviroBuilder::inline_call(const std::string& fn_name, uint16_t res, bool catch_ex);
//
//void FuncEviroBuilder::inline_call_and_ret(const std::string& fn_name, bool catch_ex);
//
void FuncEviroBuilder::as(uint16_t val, ValueMeta meta) {
	code.push_back(Command(Opcode::as).toCmd());

	useVal(val);
}
void FuncEviroBuilder::is(uint16_t val, ValueMeta meta) {

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



uint64_t FuncEviroBuilder::bind_pos() {
	jump_pos.push_back(code.size());
	return jump_pos.size() - 1;
}
#pragma region arr_op
void FuncEviroBuilder::arr_set(uint16_t arr, uint16_t from, uint64_t to, bool move, OpArrFlags::CheckMode check_bounds, bool static_mode) {
	code.push_back(Command(Opcode::arr_op, false, static_mode).toCmd());
	builder::write(code ,arr);
	OpArrFlags flags;
	flags.by_val_mode = false;
	flags.move_mode = move;
	flags.checked = check_bounds;
	code.push_back(flags.raw);
	code.push_back((uint8_t)OpcodeArray::set);
	builder::write(code, from);
	builder::write(code, to);
}
void FuncEviroBuilder::arr_setByVal(uint16_t arr, uint16_t from, uint16_t to, bool move, OpArrFlags::CheckMode check_bounds, bool static_mode) {
	code.push_back(Command(Opcode::arr_op, false, static_mode).toCmd());
	builder::write(code, arr);
	OpArrFlags flags;
	flags.by_val_mode = true;
	flags.move_mode = move;
	flags.checked = check_bounds;
	code.push_back(flags.raw);
	code.push_back((uint8_t)OpcodeArray::set);
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


void FuncEviroBuilder::arr_get(uint16_t arr, uint16_t to, uint64_t from, bool move, OpArrFlags::CheckMode check_bounds, bool static_mode) {
	code.push_back(Command(Opcode::arr_op, false, static_mode).toCmd());
	builder::write(code, arr);
	OpArrFlags flags;
	flags.by_val_mode = false;
	flags.move_mode = move;
	flags.checked = check_bounds;
	code.push_back(flags.raw);
	code.push_back((uint8_t)OpcodeArray::get);
	builder::write(code, to);
	builder::write(code, from);
}
void FuncEviroBuilder::arr_getByVal(uint16_t arr, uint16_t to, uint16_t from, bool move, OpArrFlags::CheckMode check_bounds, bool static_mode) {
	code.push_back(Command(Opcode::arr_op, false, static_mode).toCmd());
	builder::write(code, arr);
	OpArrFlags flags;
	flags.by_val_mode = true;
	flags.move_mode = move;
	flags.checked = check_bounds;
	code.push_back(flags.raw);
	code.push_back((uint8_t)OpcodeArray::get);
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
	OpArrFlags flags;
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