#pragma once
#include <bit>
#include <stdint.h>
#include "CASM.hpp"
#include "run_time_compiler.hpp"
#include "tasks.hpp"
#include <future>
#include <Windows.h>
#include <stddef.h>
#include <iostream>

asmjit::JitRuntime jrt;
std::unordered_map<std::string, typed_lgr<FuncEnviropment>> FuncEnviropment::enviropments;

ValueEnvironment enviropments;
thread_local ValueEnvironment thread_local_enviropments;

ValueItem* FuncEnviropment::asyncCall(typed_lgr<FuncEnviropment> f, list_array<ValueItem>* args) {
	ValueItem* res = new ValueItem();
	res->meta = ValueMeta(VType::async_res, true, false).encoded;
	res->val = new typed_lgr(new BTask(f, new list_array<ValueItem>(*args)));
	BTask::start(*(typed_lgr<BTask>*)res->val);
	return res;
}

void releaseUnused(ValueItem* r) {
	if(r)
		delete r;
}







template<class T>
T readData(const std::vector<uint8_t>& data, size_t data_len, size_t& i) {
	if (data_len < i + sizeof(T))
		throw InvalidFunction("Function is not full cause in pos " + std::to_string(i) + " try read " + std::to_string(sizeof(T)) + " bytes, but function length is " + std::to_string(data_len) + ", fail compile function");
	uint8_t res[sizeof(T)]{ 0 };
	for (size_t j = 0; j < sizeof(T); j++)
		res[j] = data[i++];
	return *(T*)res;
}
std::string readString(const std::vector<uint8_t>& data, size_t data_len, size_t& i) {
	uint32_t value_len = readData<uint32_t>(data, data_len, i);
	std::string res;
	for (uint32_t j = 0; j < value_len; j++)
		res += readData<char>(data, data_len, i);
	return res;
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

template<bool use_result = true,bool do_cleanup = true>
void compilerFabric_call(CASM& a, const std::vector<uint8_t>& data,size_t data_len, size_t& i, list_array<std::string>& strings) {
	CallFlags flags;
	flags.encoded = readData<uint8_t>(data, data_len, i);
	if (flags.in_memory) {
		uint16_t value_index = readData<uint16_t>(data, data_len, i);
		a.leaEnviro(argr0, value_index);
		a.mov(argr1, VType::string);
		a.call(getSpecificValue);
		a.mov(argr0, resr);
		a.mov(argr1, arg_ptr);
		a.mov(argr2, flags.async_mode);
		a.call(&FuncEnviropment::CallFunc);
	}
	else {
		std::string fnn = readString(data, data_len, i);
		typed_lgr<FuncEnviropment> fn = FuncEnviropment::enviropment(fnn);
		if (fn->canBeUnloaded()) {
			strings.push_back(fnn);
			a.mov(argr0, &strings.back());
			a.mov(argr1, arg_ptr);
			a.mov(argr2, flags.async_mode);
			a.call(&FuncEnviropment::CallFunc);
		}
		else {
			if (flags.async_mode) {
				strings.push_back(fnn);
				a.mov(argr0, &strings.back());
				a.mov(argr1, arg_ptr);
				a.mov(argr2, flags.async_mode);
				a.call(&FuncEnviropment::CallFunc);
			}
			else {
				switch (fn->Type()) {
				case FuncEnviropment::FuncType::own: {
					a.mov(argr0, fn.getPtr());
					a.mov(argr1, arg_ptr);
					a.call(&FuncEnviropment::initAndCall);
					break;
				}
				case FuncEnviropment::FuncType::native: {
					if (!fn->templateCall().arguments.size() && fn->templateCall().result.is_void()) {
						a.call(fn->get_func_ptr());
						if constexpr (use_result)
							if (flags.use_result) {
								a.movEnviro(readData<uint16_t>(data, data_len, i), 0);
								return;
							}
						break;
					}
					a.mov(argr0, fn.getPtr());
					a.mov(argr1, arg_ptr);
					a.call(&FuncEnviropment::NativeProxy_DynamicToStatic);
					break;
				}
				case FuncEnviropment::FuncType::native_own_abi: {
					a.mov(argr0, arg_ptr);
					a.call(fn->get_func_ptr());
					break;
				}
				default: {
					a.mov(argr0, fn.getPtr());
					a.mov(argr1, arg_ptr);
					a.call(&FuncEnviropment::syncWrapper);
					break;
				}
				}


				if constexpr (use_result) {
					if (flags.use_result) {
						a.leaEnviro(argr0, readData<uint16_t>(data, data_len, i));
						a.mov(argr1, resr);
						a.call(getValueItem);
						return;
					}
				}
				if constexpr (do_cleanup) {
					a.mov(argr0, resr);
					a.call(releaseUnused);
				}
			}
		}
	}
	if constexpr (use_result) {
		if (flags.use_result) {
			a.leaEnviro(argr0, readData<uint16_t>(data, data_len, i));
			a.mov(argr1, resr);
			a.call(getValueItem);
			return;
		}
	}
	if constexpr (do_cleanup) {
		a.mov(argr0, resr);
		a.call(releaseUnused);
	}
}



template<char typ>
void IndexArrayCopy(void** value,list_array<ValueItem>* arr, uint64_t pos) {
	universalRemove(value);
	if constexpr (typ == 2) {
		ValueItem temp = ((list_array<ValueItem>*)arr)->atDefault(pos);
		*value = copyValue(temp.val, temp.meta);
		*((size_t*)value) = temp.meta.encoded;
	}
	else {
		ValueItem* res;
		if constexpr (typ == 1)
			res = &((list_array<ValueItem>*)arr)->at(pos);
		else
			res = &((list_array<ValueItem>*)arr)->operator[](pos);
		*value = copyValue(res->val, res->meta);
		*((size_t*)value) = res->meta.encoded;
	}
}
template<char typ>
void IndexArrayMove(void** value, list_array<ValueItem>* arr, uint64_t pos) {
	universalRemove(value);	
	if constexpr (typ == 2) {
		ValueItem temp = ((list_array<ValueItem>*)arr)->atDefault(pos);
		*value = temp.val;
		*((size_t*)(value + 1)) = temp.meta.encoded;
		temp.val = nullptr;
		temp.meta.encoded = 0;
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
void setSize(void** value, size_t res) {
	void*& set = getValue(*value, *(ValueMeta*)(value + 1));
	ValueMeta& meta = *(ValueMeta*)(value + 1);
	switch (meta.vtype) {
	case VType::i8:
		if (((int8_t&)set = res) != res)
			throw NumericUndererflowException();
		break;
	case VType::i16:
		if (((int16_t&)set = res) != res)
			throw NumericUndererflowException();
		break;
	case VType::i32:
		if (((int32_t&)set = res) != res)
			throw NumericUndererflowException();
		break;
	case VType::i64:
		if (((int64_t&)set = res) != res)
			throw NumericUndererflowException();
		break;
	case VType::ui8:
		if (((uint8_t&)set = res) != res)
			throw NumericOverflowException();
		break;
	case VType::ui16:
		if (((uint16_t&)set = res) != res)
			throw NumericOverflowException();
		break;
	case VType::ui32:
		if (((uint32_t&)set = res) != res)
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

template<bool static_mode = false>
void popStart(list_array<ValueItem>* dest, void** insert) {
	if constexpr (!static_mode)
		universalRemove(insert);
	auto& it = dest->front();
	*insert = it.val;
	*reinterpret_cast<ValueMeta*>(insert + 1) = it.meta;
	it.meta.encoded = 0;
	dest->pop_front();
}
template<bool static_mode = false>
void popEnd(list_array<ValueItem>* dest, void** insert) {
	if constexpr (!static_mode)
		universalRemove(insert);
	auto& it = dest->back();
	*insert = it.val;
	*reinterpret_cast<ValueMeta*>(insert + 1) = it.meta;
	it.meta.encoded = 0;
	dest->pop_back();
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
void throwEx(const char* typ, const char* desc) {
	throw AException(typ, desc);
}



void setString(std::string*& str, const char* set) {
	str->operator=(set);
}

//extern "C" static EXCEPTION_DISPOSITION AttachA_specifed_handler(IN PEXCEPTION_RECORD ExceptionRecord, IN ULONG64 EstablisherFrame, IN OUT PCONTEXT ContextRecord, IN OUT PDISPATCHER_CONTEXT DispatcherContext) {
//	std::cout << "Hello from handler" << std::endl;
//
//	return EXCEPTION_DISPOSITION::ExceptionContinueSearch;
//}

void FuncEnviropment::Compile() {
	if (curr_func != nullptr)
		FrameResult::deinit(frame, curr_func, jrt);
	strings.clear();
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
				VType needSet = (VType)readData<uint8_t>(data, data_len, i);
				ValueMeta meta;
				meta.vtype = needSet;
				meta.use_gc = cmd.is_gc_mode;
				meta.allow_edit = true;
				{
					if (needAlloc(needSet)) {
						BuildCall v(a);
						v.leaEnviro(value_index);
						v.addArg(meta.encoded);
						v.addArg(readData<uint8_t>(data, data_len, i));
						v.finalize(preSetValue);
					}
					else
						a.movEnviroMeta(value_index, meta.encoded);
				}
				switch (needSet) {
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
					strings.push_back(readString(data, data_len, i));
					BuildCall b(a);
					b.addArg(resr);
					b.addArg(strings.back().data());
					b.finalize(setString);
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
					a.mov(argr0, readData<uint16_t>(data, data_len, i));
					a.mov(argr1, readData<uint16_t>(data, data_len, i));
					a.call(throwStatEx);
				}
				else {
					auto ex_typ = string_index_seter++;
					strings[ex_typ] = readString(data, data_len, i);
					auto ex_desc = string_index_seter++;
					strings[ex_desc] = readString(data, data_len, i);
					a.mov(argr0, strings[ex_typ].c_str());
					a.mov(argr1, strings[ex_desc].c_str());
					a.call(throwEx);
				}
				break;
			}
			case Opcode::arr_op: {
				uint16_t arr = readData<uint16_t>(data, data_len, i);
				switch (readData<uint8_t>(data, data_len, i)) {
				case  0://move
					a.leaEnviro(argr0, readData<uint16_t>(data, data_len, i));
					a.leaEnviro(argr1, arr);
					a.mov(argr2, readData<uint64_t>(data, data_len, i));
					a.call(IndexArrayMove<0>);
					break;
				case  1://checked move
					a.leaEnviro(argr0, readData<uint16_t>(data, data_len, i));
					a.leaEnviro(argr1, arr);
					a.mov(argr2, readData<uint64_t>(data, data_len, i));
					a.call(IndexArrayMove<1>);
					break;
				case  2://checked nothrow move
					a.leaEnviro(argr0, readData<uint16_t>(data, data_len, i));
					a.leaEnviro(argr1, arr);
					a.mov(argr2, readData<uint64_t>(data, data_len, i));
					a.call(IndexArrayMove<2>);
					break;
				case  3://get
					a.leaEnviro(argr0, readData<uint16_t>(data, data_len, i));
					a.leaEnviro(argr1, arr);
					a.mov(argr2, readData<uint64_t>(data, data_len, i));
					a.call(IndexArrayCopy<0>);
					break;
				case  4://get checked
					a.leaEnviro(argr0, readData<uint16_t>(data, data_len, i));
					a.leaEnviro(argr1, arr);
					a.mov(argr2, readData<uint64_t>(data, data_len, i));
					a.call(IndexArrayCopy<1>);
					break;
				case  5://get checked nothrow
					a.leaEnviro(argr0, readData<uint16_t>(data, data_len, i));
					a.leaEnviro(argr1, arr);
					a.mov(argr2, readData<uint64_t>(data, data_len, i));
					a.call(IndexArrayCopy<2>);
					break;
				case  6://move by val
					a.leaEnviro(argr0, readData<uint16_t>(data, data_len, i));
					a.call(getSize);
					a.leaEnviro(argr0, readData<uint16_t>(data, data_len, i));
					a.leaEnviro(argr1, arr);
					a.mov(argr2, resr);
					a.call(IndexArrayMove<0>);
					break;
				case  7://checked move by val
					a.leaEnviro(argr0, readData<uint16_t>(data, data_len, i));
					a.call(getSize);
					a.leaEnviro(argr0, readData<uint16_t>(data, data_len, i));
					a.leaEnviro(argr1, arr);
					a.mov(argr2, resr);
					a.call(IndexArrayMove<1>);
					break;
				case  8://checked nothrow move by val
					a.leaEnviro(argr0, readData<uint16_t>(data, data_len, i));
					a.call(getSize);
					a.leaEnviro(argr0, readData<uint16_t>(data, data_len, i));
					a.leaEnviro(argr1, arr);
					a.mov(argr2, resr);
					a.call(IndexArrayMove<2>);
					break;
				case  9://get by val
					a.leaEnviro(argr0, readData<uint16_t>(data, data_len, i));
					a.call(getSize);
					a.leaEnviro(argr0, readData<uint16_t>(data, data_len, i));
					a.leaEnviro(argr1, arr);
					a.mov(argr2, resr);
					a.call(IndexArrayCopy<0>);
					break;
				case 10://get checked by val
					a.leaEnviro(argr0, readData<uint16_t>(data, data_len, i));
					a.call(getSize);
					a.leaEnviro(argr0, readData<uint16_t>(data, data_len, i));
					a.leaEnviro(argr1, arr);
					a.mov(argr2, resr);
					a.call(IndexArrayCopy<1>);
					break;
				case 11://get checked nothrow by val
					a.leaEnviro(argr0, readData<uint16_t>(data, data_len, i));
					a.call(getSize);
					a.leaEnviro(argr0, readData<uint16_t>(data, data_len, i));
					a.leaEnviro(argr1, arr);
					a.mov(argr2, resr);
					a.call(IndexArrayCopy<2>);
					break;
				case 12://resize
					a.movEnviro(argr0, arr);
					a.mov(argr1, readData<uint64_t>(data, data_len, i));
					a.call((void(list_array<ValueItem>::*)(size_t))&list_array<ValueItem>::resize);
					break;
				case 13://resize with default val
					a.movEnviro(argr0, arr);
					a.mov(argr1, readData<uint64_t>(data, data_len, i));
					a.leaEnviro(argr2, readData<uint16_t>(data, data_len, i));
					a.call((void(list_array<ValueItem>::*)(size_t, const ValueItem&))&list_array<ValueItem>::resize);
					break;
				case 14://resize by val
					a.leaEnviro(argr0, readData<uint16_t>(data, data_len, i));
					a.call(getSize);
					a.leaEnviro(argr0, arr);
					a.mov(argr1, resr);
					a.call((void(list_array<ValueItem>::*)(size_t))&list_array<ValueItem>::resize);
					break;
				case 15://resize by val with default val
					a.leaEnviro(argr0, readData<uint16_t>(data, data_len, i));
					a.call(getSize);
					a.movEnviro(argr0, arr);
					a.mov(argr1, resr);
					a.leaEnviro(argr2, readData<uint16_t>(data, data_len, i));
					a.call((void(list_array<ValueItem>::*)(size_t, const ValueItem&))&list_array<ValueItem>::resize);
					break;
				case  16://get size
					a.movEnviro(argr0, arr);
					a.call(&list_array<ValueItem>::size);
					a.leaEnviro(argr0, readData<uint16_t>(data, data_len, i));
					a.mov(argr1, resr);
					a.call(setSize);
					break;
				case 17://insert range to another arr pos
					a.movEnviro(argr0, arr);
					a.mov(argr1, readData<uint64_t>(data, data_len, i));
					a.leaEnviro(argr2, readData<uint16_t>(data, data_len, i));
					a.mov(argr3, readData<uint64_t>(data, data_len, i));
					a.push(readData<uint64_t>(data, data_len, i));
					a.call((void(list_array<ValueItem>::*)(size_t, const list_array<ValueItem>&,size_t,size_t)) &list_array<ValueItem>::insert);
					a.pop();
					break;
				case 18://push end
					a.movEnviro(argr0, arr);
					a.leaEnviro(argr1, readData<uint16_t>(data, data_len, i));
					a.call((void (list_array<ValueItem>::*)(const ValueItem& copyer))&list_array<ValueItem>::push_back);
					break;
				case 19://push start
					a.movEnviro(argr0, arr);
					a.leaEnviro(argr1, readData<uint16_t>(data, data_len, i));
					a.call((void (list_array<ValueItem>::*)(const ValueItem & copyer))&list_array<ValueItem>::push_front);
					break;
				case 20://pop end
					a.leaEnviro(argr0, arr);
					a.leaEnviro(argr1, readData<uint16_t>(data, data_len, i));
					a.call(popEnd<true>);
					break;
				case 21://pop start
					a.leaEnviro(argr0, arr);
					a.leaEnviro(argr1, readData<uint16_t>(data, data_len, i));
					a.call(popStart<true>);
					break;
				case 22://reserve push end
					a.movEnviro(argr0, arr);
					a.movEnviro(argr1, readData<uint64_t>(data, data_len, i));
					a.call(&list_array<ValueItem>::reserve_push_back);
					break;
				case 23://reserve push start
					a.movEnviro(argr0, arr);
					a.movEnviro(argr1, readData<uint64_t>(data, data_len, i));
					a.call(&list_array<ValueItem>::reserve_push_front);
					break;
				case 24://remove item
					a.movEnviro(argr0, arr);
					a.movEnviro(argr1, readData<uint64_t>(data, data_len, i));
					a.call((void(list_array<ValueItem>::*)(size_t pos))&list_array<ValueItem>::remove);
					break;
				case 25://remove range
					a.movEnviro(argr0, arr);
					a.movEnviro(argr1, readData<uint64_t>(data, data_len, i));
					a.movEnviro(argr2, readData<uint64_t>(data, data_len, i));
					a.call((void(list_array<ValueItem>::*)(size_t,size_t)) & list_array<ValueItem>::remove);
					break;
				case 26://optimize (commit)
					a.movEnviro(argr0, arr);
					a.call(&list_array<ValueItem>::commit);
					break;
				case 27://optimize (decommit)
					a.movEnviro(argr0, arr);
					a.mov(argr1, readData<uint64_t>(data, data_len, i));
					a.call(&list_array<ValueItem>::decommit);
					break;
				case 28://remove reserved
					a.movEnviro(argr0, arr);
					a.call(&list_array<ValueItem>::shrink_to_fit);
					break;
				default:
					throw CompileTimeException("Invalid array operation");
				}
				break;
				}
			}
		};

		auto dynamic_fablric = [&](Command cmd) {
			switch (cmd.code) {
			case Opcode::noting:
				a.noting();
				break;
			case Opcode::set: {
				uint16_t value_index = readData<uint16_t>(data, data_len, i);
				VType set_type = (VType)readData<uint8_t>(data, data_len, i);
				{
					BuildCall v(a);
					ValueMeta meta;
					meta.vtype = set_type;
					meta.use_gc = cmd.is_gc_mode;
					meta.allow_edit = true;
					v.leaEnviro(value_index);
					v.addArg(meta.encoded);
					v.addArg(readData<uint8_t>(data, data_len, i));
					v.finalize(preSetValue);
				}
				
				switch (set_type) {
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
				case VType::string: {
					strings.push_back(readString(data, data_len, i));
					BuildCall b(a);
					b.addArg(resr);
					b.addArg(strings.back().data());
					b.finalize(setString);
					break;
				}
				default:
					break;
				}
				break;
			}
			case Opcode::remove:
				a.leaEnviro(argr0, readData<uint16_t>(data, data_len, i));
				a.call(universalRemove);
				break;
			case Opcode::sum:
				a.leaEnviro(argr0, readData<uint16_t>(data, data_len, i));
				a.leaEnviro(argr1, readData<uint16_t>(data, data_len, i));
				a.call(DynSum);
				break;
			case Opcode::minus:
				a.leaEnviro(argr0, readData<uint16_t>(data, data_len, i));
				a.leaEnviro(argr1, readData<uint16_t>(data, data_len, i));
				a.call(DynMinus);
				break;
			case Opcode::div:
				a.leaEnviro(argr0, readData<uint16_t>(data, data_len, i));
				a.leaEnviro(argr1, readData<uint16_t>(data, data_len, i));
				a.call(DynDiv);
				break;
			case Opcode::mul:
				a.leaEnviro(argr0, readData<uint16_t>(data, data_len, i));
				a.leaEnviro(argr1, readData<uint16_t>(data, data_len, i));
				a.call(DynMul);
				break;
			case Opcode::bit_xor:
				a.leaEnviro(argr0, readData<uint16_t>(data, data_len, i));
				a.leaEnviro(argr1, readData<uint16_t>(data, data_len, i));
				a.call(DynBitXor);
				break;
			case Opcode::bit_or:
				a.leaEnviro(argr0, readData<uint16_t>(data, data_len, i));
				a.leaEnviro(argr1, readData<uint16_t>(data, data_len, i));
				a.call(DynBitOr);
				break;
			case Opcode::bit_and:
				a.leaEnviro(argr0, readData<uint16_t>(data, data_len, i));
				a.leaEnviro(argr1, readData<uint16_t>(data, data_len, i));
				a.call(DynBitAnd);
				break;
			case Opcode::bit_not:
				a.leaEnviro(argr0, readData<uint16_t>(data, data_len, i));
				a.call(DynBitNot);
				break;
			case Opcode::log_not: {
				a.load_flag8h();
				a.xor_(resr_8h, 65);//flip zero and carry flags //'>=' -> '<'  '>' -> '<='
				a.store_flag8h();
				break;
			}
			case Opcode::compare: {
				a.push_flags();
				a.pop(argr0_16);
				a.leaEnviro(argr1, readData<uint16_t>(data, data_len, i));
				a.leaEnviro(argr2, readData<uint16_t>(data, data_len, i));
				a.call(compare);
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
			case Opcode::arg_set:
				a.leaEnviro(argr0, readData<uint16_t>(data, data_len, i));
				a.call(AsArg);
				a.mov(arg_ptr, resr);
				break;
			case Opcode::call: {
				compilerFabric_call<true>(a, data, data_len, i, strings);
				break;
			}
			case Opcode::ret: {
				a.leaEnviro(argr0, readData<uint16_t>(data, data_len, i));
				a.call(buildRes);
				do_jump_to_ret = true;
				break;
			}
			case Opcode::call_and_ret: {
				compilerFabric_call<false,false>(a, data, data_len, i, strings);
				do_jump_to_ret = true;
				break;
			}
			case Opcode::throw_ex: {
				bool in_memory = readData<bool>(data, data_len, i);
				if (in_memory) {
					a.mov(argr0, readData<uint16_t>(data, data_len, i));
					a.mov(argr1, readData<uint16_t>(data, data_len, i));
					a.call(throwDirEx);
				}
				else {
					auto ex_typ = string_index_seter++;
					strings[ex_typ] = readString(data, data_len, i);
					auto ex_desc = string_index_seter++;
					strings[ex_desc] = readString(data, data_len, i);
					a.mov(argr0, strings[ex_typ].c_str());
					a.mov(argr1, strings[ex_desc].c_str());
					a.call(throwEx);
				}
				break;
			}
			case Opcode::copy: {
				uint16_t from = readData<uint16_t>(data, data_len, i);
				uint16_t to = readData<uint16_t>(data, data_len, i);
				if (from != to) {
					a.leaEnviro(argr0, to);
					a.call(universalRemove);
					a.movEnviro(argr0, from);
					a.movEnviroMeta(argr1, from);
					a.call(copyValue);
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
					a.leaEnviro(argr0, to);
					a.call(universalRemove);
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
				a.leaEnviro(argr0, arr);
				a.call(AsArr);
				switch (readData<uint8_t>(data, data_len, i)) {
				case  0://move
					a.leaEnviro(argr0, readData<uint16_t>(data, data_len, i));
					a.leaEnviro(argr1, arr);
					a.mov(argr2, readData<uint64_t>(data, data_len, i));
					a.call(IndexArrayMove<0>);
					break;
				case  1://checked move
					a.leaEnviro(argr0, readData<uint16_t>(data, data_len, i));
					a.leaEnviro(argr1, arr);
					a.mov(argr2, readData<uint64_t>(data, data_len, i));
					a.call(IndexArrayMove<1>);
					break;
				case  2://checked nothrow move
					a.leaEnviro(argr0, readData<uint16_t>(data, data_len, i));
					a.leaEnviro(argr1, arr);
					a.mov(argr2, readData<uint64_t>(data, data_len, i));
					a.call(IndexArrayMove<2>);
					break;
				case  3://get
					a.leaEnviro(argr0, readData<uint16_t>(data, data_len, i));
					a.leaEnviro(argr1, arr);
					a.mov(argr2, readData<uint64_t>(data, data_len, i));
					a.call(IndexArrayCopy<0>);
					break;
				case  4://get checked
					a.leaEnviro(argr0, readData<uint16_t>(data, data_len, i));
					a.leaEnviro(argr1, arr);
					a.mov(argr2, readData<uint64_t>(data, data_len, i));
					a.call(IndexArrayCopy<1>);
					break;
				case  5://get checked nothrow
					a.leaEnviro(argr0, readData<uint16_t>(data, data_len, i));
					a.leaEnviro(argr1, arr);
					a.mov(argr2, readData<uint64_t>(data, data_len, i));
					a.call(IndexArrayCopy<2>);
					break;
				case  6://move by val
					a.leaEnviro(argr0, readData<uint16_t>(data, data_len, i));
					a.call(getSize);
					a.leaEnviro(argr0, readData<uint16_t>(data, data_len, i));
					a.leaEnviro(argr1, arr);
					a.mov(argr2, resr);
					a.call(IndexArrayMove<0>);
					break;
				case  7://checked move by val
					a.leaEnviro(argr0, readData<uint16_t>(data, data_len, i));
					a.call(getSize);
					a.leaEnviro(argr0, readData<uint16_t>(data, data_len, i));
					a.leaEnviro(argr1, arr);
					a.mov(argr2, resr);
					a.call(IndexArrayMove<1>);
					break;
				case  8://checked nothrow move by val
					a.leaEnviro(argr0, readData<uint16_t>(data, data_len, i));
					a.call(getSize);
					a.leaEnviro(argr0, readData<uint16_t>(data, data_len, i));
					a.leaEnviro(argr1, arr);
					a.mov(argr2, resr);
					a.call(IndexArrayMove<2>);
					break;
				case  9://get by val
					a.leaEnviro(argr0, readData<uint16_t>(data, data_len, i));
					a.call(getSize);
					a.leaEnviro(argr0, readData<uint16_t>(data, data_len, i));
					a.leaEnviro(argr1, arr);
					a.mov(argr2, resr);
					a.call(IndexArrayCopy<0>);
					break;
				case 10://get checked by val
					a.leaEnviro(argr0, readData<uint16_t>(data, data_len, i));
					a.call(getSize);
					a.leaEnviro(argr0, readData<uint16_t>(data, data_len, i));
					a.leaEnviro(argr1, arr);
					a.mov(argr2, resr);
					a.call(IndexArrayCopy<1>);
					break;
				case 11://get checked nothrow by val
					a.leaEnviro(argr0, readData<uint16_t>(data, data_len, i));
					a.call(getSize);
					a.leaEnviro(argr0, readData<uint16_t>(data, data_len, i));
					a.leaEnviro(argr1, arr);
					a.mov(argr2, resr);
					a.call(IndexArrayCopy<2>);
					break;
				case 12://resize
					a.movEnviro(argr0, arr);
					a.mov(argr1, readData<uint64_t>(data, data_len, i));
					a.call((void(list_array<ValueItem>::*)(size_t)) & list_array<ValueItem>::resize);
					break;
				case 13://resize with default val
					a.movEnviro(argr0, arr);
					a.mov(argr1, readData<uint64_t>(data, data_len, i));
					a.leaEnviro(argr2, readData<uint16_t>(data, data_len, i));
					a.call((void(list_array<ValueItem>::*)(size_t, const ValueItem&)) & list_array<ValueItem>::resize);
					break;
				case 14://resize by val
					a.leaEnviro(argr0, readData<uint16_t>(data, data_len, i));
					a.call(getSize);
					a.leaEnviro(argr0, arr);
					a.mov(argr1, resr);
					a.call((void(list_array<ValueItem>::*)(size_t)) & list_array<ValueItem>::resize);
					break;
				case 15://resize by val with default val
					a.leaEnviro(argr0, readData<uint16_t>(data, data_len, i));
					a.call(getSize);
					a.movEnviro(argr0, arr);
					a.mov(argr1, resr);
					a.leaEnviro(argr2, readData<uint16_t>(data, data_len, i));
					a.call((void(list_array<ValueItem>::*)(size_t, const ValueItem&)) & list_array<ValueItem>::resize);
					break;
				case  16://get size
					a.movEnviro(argr0, arr);
					a.call(&list_array<ValueItem>::size);
					a.leaEnviro(argr0, readData<uint16_t>(data, data_len, i));
					a.mov(argr1, resr);
					a.call(setSize);
					break;
				case 17://insert range to another arr pos
				{
					uint16_t arr1 = readData<uint16_t>(data, data_len, i);
					a.leaEnviro(argr0, arr1);
					a.call(AsArr);
					a.movEnviro(argr0, arr);
					a.mov(argr1, readData<uint64_t>(data, data_len, i));
					a.leaEnviro(argr2, arr1);
					a.mov(argr3, readData<uint64_t>(data, data_len, i));
					a.push(readData<uint64_t>(data, data_len, i));
					a.call((void(list_array<ValueItem>::*)(size_t, const list_array<ValueItem>&, size_t, size_t)) & list_array<ValueItem>::insert);
					a.pop();
					break;
				}
				case 18://push end
					a.movEnviro(argr0, arr);
					a.leaEnviro(argr1, readData<uint16_t>(data, data_len, i));
					a.call((void (list_array<ValueItem>::*)(const ValueItem & copyer)) & list_array<ValueItem>::push_back);
					break;
				case 19://push start
					a.movEnviro(argr0, arr);
					a.leaEnviro(argr1, readData<uint16_t>(data, data_len, i));
					a.call((void (list_array<ValueItem>::*)(const ValueItem & copyer)) & list_array<ValueItem>::push_front);
					break;
				case 20://pop end
					a.leaEnviro(argr0, arr);
					a.leaEnviro(argr1, readData<uint16_t>(data, data_len, i));
					a.call(popEnd<false>);
					break;
				case 21://pop start
					a.leaEnviro(argr0, arr);
					a.leaEnviro(argr1, readData<uint16_t>(data, data_len, i));
					a.call(popStart<false>);
					break;
				case 22://reserve push end
					a.leaEnviro(argr0, arr);
					a.movEnviro(argr1, readData<uint64_t>(data, data_len, i));
					a.call(&list_array<ValueItem>::reserve_push_back);
					break;
				case 23://reserve push start
					a.leaEnviro(argr0, arr);
					a.movEnviro(argr1, readData<uint64_t>(data, data_len, i));
					a.call(&list_array<ValueItem>::reserve_push_front);
					break;
				case 24://remove item
					a.movEnviro(argr0, arr);
					a.movEnviro(argr1, readData<uint64_t>(data, data_len, i));
					a.call((void(list_array<ValueItem>::*)(size_t pos)) & list_array<ValueItem>::remove);
					break;
				case 25://remove range
					a.movEnviro(argr0, arr);
					a.movEnviro(argr1, readData<uint64_t>(data, data_len, i));
					a.movEnviro(argr2, readData<uint64_t>(data, data_len, i));
					a.call((void(list_array<ValueItem>::*)(size_t, size_t)) & list_array<ValueItem>::remove);
					break;
				case 26://optimize (commit)
					a.movEnviro(argr0, arr);
					a.call(&list_array<ValueItem>::commit);
					break;
				case 27://optimize (decommit)
					a.movEnviro(argr0, arr);
					a.mov(argr1, readData<uint64_t>(data, data_len, i));
					a.call(&list_array<ValueItem>::decommit);
					break;
				case 28://remove reserved
					a.movEnviro(argr0, arr);
					a.call(&list_array<ValueItem>::shrink_to_fit);
					break;
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

	strings.push_back("AttachA symbol: " + name);
	curr_func = (Enviropment)tmp.init(frame, a.code(), jrt, strings.back().data());
}

struct EnviroHold {
	void** envir;
	uint16_t _vals;
	EnviroHold(uint16_t vals) :_vals(vals) {
		if (vals) {
			envir = new void* [size_t(vals) << 1];
			memset(envir, 0, sizeof(void*) * (size_t(vals) << 1));
		}
		else
			envir = nullptr;
	}
	~EnviroHold() {
		removeEnviropement(envir, _vals);
		if (envir)
			delete[] envir;
	}
};

ValueItem* FuncEnviropment::initAndCall(list_array<ValueItem>* arguments) {
	ValueItem* res = nullptr;
	EnviroHold env(max_values);
	try {
		res = curr_func(env.envir,arguments);
	}
	catch(const StackOverflowException&){
		if (!need_restore_stack_fault()) {
			--current_runners;
			throw;
		}
	}
	catch(...) {
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
	return type == FuncType::own ? initAndCall(args) : NativeProxy_DynamicToStatic(args);
}

#pragma warning(push)
#pragma warning(disable: 4311)
#pragma warning(disable: 4302)
ValueItem* FuncEnviropment::NativeProxy_DynamicToStatic(list_array<ValueItem>* arguments) {
	DynamicCall::FunctionCall call((DynamicCall::PROC)curr_func, nat_templ, true);
	if (arguments) {
		for (auto& it : *arguments) {
			auto to_add = call.ToAddArgument();
			if (to_add.ptype == DynamicCall::FunctionTemplate::ValueT::PlaceType::as_ptr && to_add.vtype == DynamicCall::FunctionTemplate::ValueT::ValueType::integer && !to_add.vsize)
				break;
			ValueMeta meta = it.meta;
			void*& arg = getValue(it.val, it.meta);
			switch (to_add.vtype) {
			case DynamicCall::FunctionTemplate::ValueT::ValueType::integer:
			case DynamicCall::FunctionTemplate::ValueT::ValueType::signed_integer:
			case DynamicCall::FunctionTemplate::ValueT::ValueType::floating:
				if (to_add.ptype == DynamicCall::FunctionTemplate::ValueT::PlaceType::as_value) {
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
					case VType::string:
						call.AddValueArgument(((std::string*)arg)->data());
						break;
					default:
						throw InvalidType("Required integer or floating family type but requived another");
					}
				}
				break;

			case DynamicCall::FunctionTemplate::ValueT::ValueType::pointer:
				switch (meta.vtype) {
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
			case DynamicCall::FunctionTemplate::ValueT::ValueType::_class:
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
	catch (...) {
		--current_runners;
		throw;
	}
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
#pragma warning(pop)

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
	delete FuncEnviropment::CallFunc(symbol_name, &empty, run_async);
}

#include "../libray/console.hpp"

extern "C" void initStandardFunctions() {
#pragma region Console
	FuncEnviropment::AddNative(console::printLine, "console printLine", false);
	FuncEnviropment::AddNative(console::print, "console print", false);
	FuncEnviropment::AddNative(console::resetModifiers, "console resetModifiers", false);
	FuncEnviropment::AddNative(console::boldText, "console boldText", false);
	FuncEnviropment::AddNative(console::italicText, "console italicText", false);
	FuncEnviropment::AddNative(console::underlineText, "console underlineText", false);
	FuncEnviropment::AddNative(console::slowBlink, "console slowBlink", false);
	FuncEnviropment::AddNative(console::rapidBlink, "console rapidBlink", false);
	FuncEnviropment::AddNative(console::invertColors, "console invertColors", false);
	FuncEnviropment::AddNative(console::notBoldText, "console notBoldText", false);
	FuncEnviropment::AddNative(console::notUnderlinedText, "console notUnderlinedText", false);
	FuncEnviropment::AddNative(console::notBlinkText, "console notBlinkText", false);

	FuncEnviropment::AddNative(console::resetTextColor, "console resetTextColor", false);
	FuncEnviropment::AddNative(console::resetBgColor, "console resetBgColor", false);
	FuncEnviropment::AddNative(console::setTextColor, "console setTextColor", false);
	FuncEnviropment::AddNative(console::setBgColor, "console setBgColor", false);
	FuncEnviropment::AddNative(console::setPos, "console setPos", false);
	FuncEnviropment::AddNative(console::saveCurPos, "console saveCurPos", false);
	FuncEnviropment::AddNative(console::loadCurPos, "console loadCurPos", false);
	FuncEnviropment::AddNative(console::setLine, "console setLine", false);
	FuncEnviropment::AddNative(console::showCursor, "console showCursor", false);
	FuncEnviropment::AddNative(console::hideCursor, "console hideCursor", false);
#pragma endregion
#pragma region File

#pragma endregion

}

