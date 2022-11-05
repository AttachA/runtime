#include "FuncEnviropment.hpp"
#include "../run_time.hpp"
#include "attacha_abi.hpp"
#include "CASM.hpp"
#include "tools.hpp"
#include "AttachA_CXX.hpp"
#include "tasks.hpp"
#include <iostream>
#include "attacha_abi.hpp"
std::unordered_map<std::string, typed_lgr<FuncEnviropment>> FuncEnviropment::enviropments;

ValueItem* FuncEnviropment::async_call(typed_lgr<FuncEnviropment> f, ValueItem* args, uint32_t args_len) {
	ValueItem* res = new ValueItem();
	res->meta = ValueMeta(VType::async_res, false, false).encoded;
	ValueItem temp(args, ValueMeta(VType::saarr, false, true, args_len), true);
	res->val = new typed_lgr(new Task(f, temp));
	Task::start(*(typed_lgr<Task>*)res->val);
	return res;
}

struct EnviroHold {
	void** envir;
	uint16_t _vals;
	EnviroHold(void** env, uint16_t vals) :envir(env), _vals(vals) {
		if (vals)
			memset(envir, 0, sizeof(void*) * (size_t(vals) << 1));
	}
	~EnviroHold() {
		if (_vals) {
			uint32_t max_vals = uint32_t(_vals) << 1;
			for (uint32_t i = 0; i < max_vals; i += 2)
				universalRemove(envir + i);
		}
	}
};
ValueItem* FuncEnviropment::initAndCall(ValueItem* arguments, uint32_t arguments_size) {
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
		res = curr_func(env.envir, arguments, arguments_size);
	}
	catch (const StackOverflowException&) {
		if (!need_restore_stack_fault())
			throw;
	}
	if (restore_stack_fault())
		throw StackOverflowException();
	return res;
}

ValueItem* FuncEnviropment::syncWrapper(ValueItem* args, uint32_t arguments_size) {
	if(_type == FuncEnviropment::FuncType::force_unloaded)
		throw InvalidFunction("Function is force unloaded");
	if (force_unload)
		throw InvalidFunction("Function is force unloaded");
	if (need_compile)
		funcComp();
	switch (_type) {
	case FuncEnviropment::FuncType::own:
		return initAndCall(args, arguments_size);
	case FuncEnviropment::FuncType::native:
		return NativeProxy_DynamicToStatic(args, arguments_size);
	case FuncEnviropment::FuncType::native_own_abi: {
		ValueItem* res;
		try {
			res = ((AttachACXX)curr_func)(args, arguments_size);
		}
		catch (...) {
			if (!need_restore_stack_fault()) 
				throw;
			res = nullptr;
		}
		if (restore_stack_fault())
			throw StackOverflowException();
		return res;
	}
	case FuncEnviropment::FuncType::python:
	case FuncEnviropment::FuncType::csharp:
	case FuncEnviropment::FuncType::java:
	default:
		throw NotImplementedException();
	}
}
ValueItem* FuncEnviropment::syncWrapper_catch(ValueItem* args, uint32_t arguments_size) {
	try {
		return syncWrapper(args, arguments_size);
	}
	catch (...) {
		return new ValueItem(new std::exception_ptr(std::current_exception()), ValueMeta(VType::except_value, false, true), true);
	}
}


void NativeProxy_DynamicToStatic_addValue(DynamicCall::FunctionCall& call, ValueMeta meta, void*& arg) {
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
			call.AddArray((int8_t*)arg, meta.val_len);
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
ValueItem* FuncEnviropment::NativeProxy_DynamicToStatic(ValueItem* arguments, uint32_t arguments_size) {
	using namespace DynamicCall;
	FunctionCall call((DynamicCall::PROC)curr_func, nat_templ, true);
	if (arguments) {
		while (arguments_size--) {
			ValueItem& it = *arguments++;
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
	}
	catch (...) {
		if(!need_restore_stack_fault())
			throw;
	}
	if (restore_stack_fault())
		throw StackOverflowException();

	if (nat_templ.result.is_void())
		return nullptr;
	if (nat_templ.result.ptype == DynamicCall::FunctionTemplate::ValueT::PlaceType::as_ptr)
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

ValueItem* FuncEnviropment::native_proxy_catch(ValueItem* args, uint32_t arguments_size) {
	try {
		return NativeProxy_DynamicToStatic(args, arguments_size);
	}
	catch (...) {
		return new ValueItem(new std::exception_ptr(std::current_exception()), ValueMeta(VType::except_value, false, true), true);
	}
}
ValueItem* FuncEnviropment::initAndCall_catch(ValueItem* args, uint32_t arguments_size) {
	try {
		return initAndCall(args, arguments_size);
	}
	catch (...) {
		return new ValueItem(new std::exception_ptr(std::current_exception()), ValueMeta(VType::except_value, false, true), true);
	}
}


using namespace run_time;
asmjit::JitRuntime jrt;

FuncEnviropment::~FuncEnviropment() {
	std::lock_guard lguard(compile_lock);
	if (can_be_unloaded) {
		if (_type == FuncType::own)
			if (curr_func)
				FrameResult::deinit(frame, curr_func, jrt);
	}
}



void inlineReleaseUnused(CASM& a, creg64 reg) {
	auto lab = a.newLabel();
	a.test(reg, reg);
	a.jmp_zero(lab);
	BuildCall b(a, true);
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
ValueItem* AttachACXXCatchCall(AttachACXX fn, ValueItem* args, uint32_t args_len) {
	try {
		return fn(args, args_len);
	}
	catch (...) {
		return new ValueItem(new std::exception_ptr(std::current_exception()), ValueMeta(VType::except_value, false, true), true);
	}
}
template<bool use_result = true, bool do_cleanup = true>
void compilerFabric_call(CASM& a, const std::vector<uint8_t>& data, size_t data_len, size_t& i, list_array<ValueItem>& values) {
	CallFlags flags;
	flags.encoded = readData<uint8_t>(data, data_len, i);
	BuildCall b(a, true);
	if (flags.in_memory) {
		uint16_t value_index = readData<uint16_t>(data, data_len, i);
		b.leaEnviro(value_index);
		b.addArg(VType::string);
		b.finalize(getSpecificValue);
		b.addArg(resr);
		b.addArg(arg_ptr);
		b.addArg(arg_len_32);
		b.addArg(flags.async_mode);
		if (flags.except_catch)
			b.finalize(&FuncEnviropment::callFunc_catch);
		else
			b.finalize(&FuncEnviropment::callFunc);
	}
	else {
		std::string fnn = readString(data, data_len, i);
		typed_lgr<FuncEnviropment> fn = FuncEnviropment::enviropment(fnn);
		if (fn->canBeUnloaded() || flags.async_mode) {
			values.push_back(fnn);
			b.addArg(((std::string*)values.back().val)->c_str());
			b.addArg(arg_ptr);
			b.addArg(arg_len_32);
			b.addArg(flags.async_mode);
			if (flags.except_catch)
				b.finalize(&FuncEnviropment::callFunc_catch);
			else
				b.finalize(&FuncEnviropment::callFunc);
		}
		else {
			switch (fn->type()) {
			case FuncEnviropment::FuncType::own: {
				b.addArg(fn.getPtr());
				b.addArg(arg_ptr);
				b.addArg(arg_len_32);
				if (flags.except_catch)
					b.finalize(&FuncEnviropment::initAndCall_catch);
				else
					b.finalize(&FuncEnviropment::initAndCall);
				break;
			}
			case FuncEnviropment::FuncType::native: {
				if (!fn->function_template().arguments.size() && fn->function_template().result.is_void()) {
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
				b.addArg(arg_len_32);
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
					b.addArg(arg_len_32);
					b.finalize(AttachACXXCatchCall);
				}
				else {
					b.addArg(arg_ptr);
					b.addArg(arg_len_32);
					b.finalize(fn->get_func_ptr());
				}
				break;
			}
			default: {
				b.addArg(fn.getPtr());
				b.addArg(arg_ptr);
				b.addArg(arg_len_32);
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
void compilerFabric_call_local(CASM& a, const std::vector<uint8_t>& data, size_t data_len, size_t& i, FuncEnviropment* env) {
	CallFlags flags;
	flags.encoded = readData<uint8_t>(data, data_len, i);
	BuildCall b(a, true);
	if (flags.in_memory) {
		b.leaEnviro(readData<uint16_t>(data, data_len, i));
		b.finalize(getSize);
		b.addArg(env);
		b.addArg(resr);
		b.addArg(arg_ptr);
		b.addArg(arg_len_32);
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
			b.addArg(arg_len_32);
			b.addArg(flags.async_mode);
			if (flags.except_catch)
				b.finalize(&FuncEnviropment::localWrapper_catch);
			else
				b.finalize(&FuncEnviropment::localWrapper);
		}
		else {
			typed_lgr<FuncEnviropment> fn = env->localFn(fnn);
			switch (fn->type()) {
			case FuncEnviropment::FuncType::own: {
				b.addArg(fn.getPtr());
				b.addArg(arg_ptr);
				b.addArg(arg_len_32);
				if (flags.except_catch)
					b.finalize(&FuncEnviropment::initAndCall_catch);
				else
					b.finalize(&FuncEnviropment::initAndCall);
				break;
			}
			case FuncEnviropment::FuncType::native: {
				if (!fn->function_template().arguments.size() && fn->function_template().result.is_void()) {
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
				b.addArg(arg_len_32);
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
					b.addArg(arg_len_32);
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
				b.addArg(arg_len_32);
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


template<bool async_call>
void* _valueItemDynamicCall(const std::string& name, ValueItem* class_ptr, ClassAccess access, ValueItem* args, uint32_t len) {
	switch (class_ptr->meta.vtype) {
	case VType::class_:
		if constexpr (async_call)
			return FuncEnviropment::async_call(((ClassValue&)*class_ptr).callFnPtr(name, access), args, len);
		else
			return ((ClassValue&)*class_ptr).callFnPtr(name, access)->syncWrapper(args, len);
		break;
	case VType::morph:
		if constexpr (async_call)
			return FuncEnviropment::async_call(((MorphValue&)*class_ptr).callFnPtr(name, access), args, len);
		else
			return ((MorphValue&)*class_ptr).callFnPtr(name, access)->syncWrapper(args, len);
		break;
	case VType::proxy:
		if constexpr (async_call)
			return FuncEnviropment::async_call(((ProxyClass&)*class_ptr).callFnPtr(name, access), args, len);
		else
			return ((ProxyClass&)*class_ptr).callFnPtr(name, access)->syncWrapper(args, len);
		break;
	default:
		throw NotImplementedException();
	}
}
template<bool ex_catch, bool async_mode>
void* valueItemDynamicCall(const std::string& name, ValueItem* class_ptr, ValueItem* args, uint32_t len, ClassAccess access) {
	if (!class_ptr)
		throw NullPointerException();
	list_array<ValueItem> args_tmp(args, args + len, len);
	if constexpr (async_mode)
		args_tmp.push_front(*class_ptr);
	else
		args_tmp.push_front(ValueItem(class_ptr->val, class_ptr->meta, true, true));
	class_ptr->getAsync();
	if constexpr (ex_catch) {
		try {
			if (async_mode)
				return _valueItemDynamicCall<true>(name, class_ptr, access, args_tmp.data(), len + 1);
			else
				return _valueItemDynamicCall<false>(name, class_ptr, access, args_tmp.data(), len + 1);
		}
		catch (...) {
			try {
				return new ValueItem(new std::exception_ptr(std::current_exception()), VType::except_value, true);
			}
			catch (const std::bad_alloc& ex) {
				throw EnviropmentRuinException();
			}
		}
	}
	else if (async_mode)
		return _valueItemDynamicCall<true>(name, class_ptr, access, args_tmp.data(), len + 1);
	else
		return _valueItemDynamicCall<false>(name, class_ptr, access, args_tmp.data(), len + 1);
}

template<bool use_result = true, bool do_cleanup = true>
void compilerFabric_value_call(CASM& a, const std::vector<uint8_t>& data, size_t data_len, size_t& i, list_array<ValueItem>& values) {
	CallFlags flags;
	flags.encoded = readData<uint8_t>(data, data_len, i);
	BuildCall b(a, true);
	if (flags.in_memory) {
		uint16_t value_index = readData<uint16_t>(data, data_len, i);
		b.leaEnviro(value_index);
		b.addArg(VType::string);
		b.finalize(getSpecificValue);
		b.addArg(resr);
	}
	else {
		std::string fnn = readString(data, data_len, i);
		values.push_back(fnn);
		b.addArg((std::string*)values.back().val);
	}
	//fn_nam

	uint16_t class_ptr = readData<uint16_t>(data, data_len, i);
	b.leaEnviro(class_ptr);
	b.addArg(arg_ptr);
	b.addArg(arg_len_32);
	b.addArg((uint8_t)readData<ClassAccess>(data, data_len, i));
	if (flags.except_catch) {
		if (flags.async_mode)
			b.finalize(valueItemDynamicCall<true, true>);
		else
			b.finalize(valueItemDynamicCall<true, false>);
	}
	else {
		if (flags.async_mode)
			b.finalize(valueItemDynamicCall<false, true>);
		else
			b.finalize(valueItemDynamicCall<false, false>);
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

template<bool ex_catch, bool async_mode>
void* staticValueItemDynamicCall(const std::string& name, ValueItem* class_ptr, ValueItem* args, uint32_t len, ClassAccess access) {
	if (!class_ptr)
		throw NullPointerException();
	class_ptr->getAsync();
	if constexpr (ex_catch) {
		try {
			if (async_mode)
				return _valueItemDynamicCall<true>(name, class_ptr, access, args, len);
			else
				return _valueItemDynamicCall<false>(name, class_ptr, access, args, len);
		}
		catch (...) {
			try {
				return new ValueItem(new std::exception_ptr(std::current_exception()), VType::except_value, true);
			}
			catch (const std::bad_alloc& ex) {
				throw EnviropmentRuinException();
			}
		}
	}
	else if (async_mode)
		return _valueItemDynamicCall<true>(name, class_ptr, access, args, len);
	else
		return _valueItemDynamicCall<false>(name, class_ptr, access, args, len);
}

template<bool use_result = true, bool do_cleanup = true>
void compilerFabric_static_value_call(CASM& a, const std::vector<uint8_t>& data, size_t data_len, size_t& i, list_array<ValueItem>& values) {
	CallFlags flags;
	flags.encoded = readData<uint8_t>(data, data_len, i);
	BuildCall b(a, true);
	if (flags.in_memory) {
		uint16_t value_index = readData<uint16_t>(data, data_len, i);
		b.leaEnviro(value_index);
		b.addArg(VType::string);
		b.finalize(getSpecificValue);
		b.addArg(resr);
	}
	else {
		std::string fnn = readString(data, data_len, i);
		values.push_back(fnn);
		b.addArg((std::string*)values.back().val);
	}
	//fn_nam

	uint16_t class_ptr = readData<uint16_t>(data, data_len, i);
	b.leaEnviro(class_ptr);
	b.addArg(arg_ptr);
	b.addArg(arg_len_32);
	b.addArg((uint8_t)readData<ClassAccess>(data, data_len, i));
	if (flags.except_catch) {
		if (flags.async_mode)
			b.finalize(staticValueItemDynamicCall<true, true>);
		else
			b.finalize(staticValueItemDynamicCall<true, false>);
	}
	else {
		if (flags.async_mode)
			b.finalize(staticValueItemDynamicCall<false, true>);
		else
			b.finalize(staticValueItemDynamicCall<false, false>);
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


#pragma region Static_ArrayAccessors
template<char typ>
void IndexArrayCopyStatic(void** value, list_array<ValueItem>** arr_ref, uint64_t pos) {
	universalRemove(value);
	ValueMeta& meta = ((ValueMeta*)arr_ref)[1];
	list_array<ValueItem>* arr = (list_array<ValueItem>*)getValue(*(void**)arr_ref, meta);
	if constexpr (typ == 2) {
		if (arr->size() > pos) {
			ValueItem temp = ((list_array<ValueItem>*)arr)->atDefault(pos);
			*value = temp.val;
			*((size_t*)(value + 1)) = temp.meta.encoded;
			temp.val = nullptr;
		}
		else {
			*value = nullptr;
			*((size_t*)(value + 1)) = 0;
		}
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
void IndexArrayMoveStatic(void** value, list_array<ValueItem>** arr_ref, uint64_t pos) {
	universalRemove(value);
	ValueMeta& meta = ((ValueMeta*)arr_ref)[1];
	list_array<ValueItem>* arr = (list_array<ValueItem>*)getValue(*(void**)arr_ref, meta);
	if constexpr (typ == 2) {
		if (arr->size() > pos) {
			ValueItem& res = ((list_array<ValueItem>*)arr)->operator[](pos);
			*value = res.val;
			*((size_t*)(value + 1)) = res.meta.encoded;
			res.val = nullptr;
			res.meta.encoded = 0;
		}
		else {
			*value = nullptr;
			*((size_t*)(value + 1)) = 0;
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
void IndexArraySetCopyStatic(void** value, list_array<ValueItem>** arr_ref, uint64_t pos) {
	ValueMeta& meta = ((ValueMeta*)arr_ref)[1];
	list_array<ValueItem>* arr = (list_array<ValueItem>*)getValue(*(void**)arr_ref, meta);
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
void IndexArraySetMoveStatic(void** value, list_array<ValueItem>** arr_ref, uint64_t pos) {
	ValueMeta& meta = ((ValueMeta*)arr_ref)[1];
	list_array<ValueItem>* arr = (list_array<ValueItem>*)getValue(*(void**)arr_ref, meta);
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





template<char typ, class T>
void IndexArrayCopyStatic(void** value, void** arr_ref, uint64_t pos) {
	universalRemove(value);
	ValueMeta& meta = ((ValueMeta*)arr_ref)[1];
	T* arr = (T*)getValue(*arr_ref, meta);
	if constexpr (typ == 2) {
		if (meta.val_len <= pos) {
			*value = nullptr;
			*((size_t*)(value + 1)) = 0;
			return;
		}
	}
	else if constexpr (typ == 1)
		if (meta.val_len > pos)
			throw OutOfRange();

	ValueItem temp = arr[pos];
	*value = temp.val;
	*((size_t*)(value + 1)) = temp.meta.encoded;
	temp.val = nullptr;
}
template<char typ, class T>
void IndexArrayMoveStatic(void** value, void** arr_ref, uint64_t pos) {
	universalRemove(value);
	ValueMeta& meta = ((ValueMeta*)arr_ref)[1];
	T* arr = (T*)getValue(*arr_ref, meta);
	if constexpr (typ == 2) {
		if (meta.val_len <= pos) {
			*value = nullptr;
			*((size_t*)(value + 1)) = 0;
			return;
		}
	}
	else if constexpr (typ == 1)
		if (meta.val_len > pos)
			throw OutOfRange();

	ValueItem temp = std::move(arr[pos]);
	if (!std::is_same_v<T, ValueItem>)
		arr[pos] = T();
	*value = temp.val;
	*((size_t*)(value + 1)) = temp.meta.encoded;
	temp.val = nullptr;
}


template<char typ, class T>
void IndexArraySetCopyStatic(void** value, void** arr_ref, uint32_t pos) {
	ValueMeta& meta = ((ValueMeta*)arr_ref)[1];
	T* arr = (T*)getValue(*arr_ref, meta);
	if constexpr (typ != 0) {
		if (meta.val_len <= pos)
			throw OutOfRange();
	}
	if constexpr (std::is_same_v<T, ValueItem>)
		arr[pos] = *reinterpret_cast<ValueItem*>(value);
	else
		arr[pos] = (T) * reinterpret_cast<ValueItem*>(value);
}
template<char typ, class T>
void IndexArraySetMoveStatic(void** value, void** arr_ref, uint32_t pos) {
	ValueMeta& meta = ((ValueMeta*)arr_ref)[1];
	T* arr = (T*)getValue(*arr_ref, meta);
	if constexpr (typ != 0) {
		if (meta.val_len <= pos)
			throw OutOfRange();
	}
	arr[pos] = (T)reinterpret_cast<ValueItem&&>(value);
	if (!std::is_same_v<T, ValueItem>)
		reinterpret_cast<ValueItem&>(value) = ValueItem();
}


template<char typ>
void IndexArrayStaticInterface(void** value, ValueItem* arr_ref, uint64_t pos) noexcept(false) {
	universalRemove(value);
	size_t length = (size_t)AttachA::Interface::makeCall(ClassAccess::pub, *arr_ref, "size");
	if constexpr (typ == 2) {
		if (length <= pos) {
			*value = nullptr;
			*((size_t*)(value + 1)) = 0;
			return;
		}
	}
	else if constexpr (typ == 1)
		if (length > pos)
			throw OutOfRange();
	ValueItem temp = AttachA::Interface::makeCall(ClassAccess::pub, *arr_ref, "get[]", pos);
	*value = temp.val;
	*((size_t*)(value + 1)) = temp.meta.encoded;
	temp.val = nullptr;
}
template<char typ>
void IndexArraySetStaticInterface(void** value, ValueItem* arr_ref, uint64_t pos) {
	size_t length = (size_t)AttachA::Interface::makeCall(ClassAccess::pub, *arr_ref, "size");
	if constexpr (typ != 0) {
		if (length <= pos)
			throw OutOfRange();
	}
	AttachA::Interface::makeCall(ClassAccess::pub, *arr_ref, "set[]", pos, reinterpret_cast<ValueItem&>(value));
}

template<char typ>
void inlineIndexArraySetCopyStatic(BuildCall& b, VType type) {
	switch (type) {
	case VType::raw_arr_i8:
		b.finalize(IndexArraySetCopyStatic<typ, int8_t>);
		break;
	case VType::raw_arr_i16:
		b.finalize(IndexArraySetCopyStatic<typ, int16_t>);
		break;
	case VType::raw_arr_i32:
		b.finalize(IndexArraySetCopyStatic<typ, int32_t>);
		break;
	case VType::raw_arr_i64:
		b.finalize(IndexArraySetCopyStatic<typ, int64_t>);
		break;
	case VType::raw_arr_ui8:
		b.finalize(IndexArraySetCopyStatic<typ, uint8_t>);
		break;
	case VType::raw_arr_ui16:
		b.finalize(IndexArraySetCopyStatic<typ, uint16_t>);
		break;
	case VType::raw_arr_ui32:
		b.finalize(IndexArraySetCopyStatic<typ, uint32_t>);
		break;
	case VType::raw_arr_ui64:
		b.finalize(IndexArraySetCopyStatic<typ, uint64_t>);
		break;
	case VType::raw_arr_flo:
		b.finalize(IndexArraySetCopyStatic<typ, float>);
		break;
	case VType::raw_arr_doub:
		b.finalize(IndexArraySetCopyStatic<typ, double>);
		break;
	case VType::uarr:
		b.finalize(IndexArraySetCopyStatic<typ>);
		break;
	case VType::faarr:
	case VType::saarr:
		b.finalize(IndexArraySetCopyStatic<typ, ValueItem>);
		break;
	case VType::class_:
	case VType::morph:
	case VType::proxy:
		b.finalize(IndexArraySetStaticInterface<0>);
		break;
	default:
		throw CompileTimeException("Invalid opcode, unsupported static type for this operation");
	}
}
template<char typ>
void inlineIndexArraySetMoveStatic(BuildCall& b, VType type) {
	switch (type) {
	case VType::raw_arr_i8:
		b.finalize(IndexArraySetMoveStatic<typ, int8_t>);
		break;
	case VType::raw_arr_i16:
		b.finalize(IndexArraySetMoveStatic<typ, int16_t>);
		break;
	case VType::raw_arr_i32:
		b.finalize(IndexArraySetMoveStatic<typ, int32_t>);
		break;
	case VType::raw_arr_i64:
		b.finalize(IndexArraySetMoveStatic<typ, int64_t>);
		break;
	case VType::raw_arr_ui8:
		b.finalize(IndexArraySetMoveStatic<typ, uint8_t>);
		break;
	case VType::raw_arr_ui16:
		b.finalize(IndexArraySetMoveStatic<typ, uint16_t>);
		break;
	case VType::raw_arr_ui32:
		b.finalize(IndexArraySetMoveStatic<typ, uint32_t>);
		break;
	case VType::raw_arr_ui64:
		b.finalize(IndexArraySetMoveStatic<typ, uint64_t>);
		break;
	case VType::raw_arr_flo:
		b.finalize(IndexArraySetMoveStatic<typ, float>);
		break;
	case VType::raw_arr_doub:
		b.finalize(IndexArraySetMoveStatic<typ, double>);
		break;
	case VType::uarr:
		b.finalize(IndexArraySetMoveStatic<typ>);
		break;
	case VType::faarr:
	case VType::saarr:
		b.finalize(IndexArraySetMoveStatic<typ, ValueItem>);
		break;
	default:
		throw CompileTimeException("Invalid opcode, unsupported static type for this operation");
	}
}

template<char typ>
void inlineIndexArrayCopyStatic(BuildCall& b, VType type) {
	switch (type) {
	case VType::raw_arr_i8:
		b.finalize(IndexArrayCopyStatic<typ, int8_t>);
		break;
	case VType::raw_arr_i16:
		b.finalize(IndexArrayCopyStatic<typ, int16_t>);
		break;
	case VType::raw_arr_i32:
		b.finalize(IndexArrayCopyStatic<typ, int32_t>);
		break;
	case VType::raw_arr_i64:
		b.finalize(IndexArrayCopyStatic<typ, int64_t>);
		break;
	case VType::raw_arr_ui8:
		b.finalize(IndexArrayCopyStatic<typ, uint8_t>);
		break;
	case VType::raw_arr_ui16:
		b.finalize(IndexArrayCopyStatic<typ, uint16_t>);
		break;
	case VType::raw_arr_ui32:
		b.finalize(IndexArrayCopyStatic<typ, uint32_t>);
		break;
	case VType::raw_arr_ui64:
		b.finalize(IndexArrayCopyStatic<typ, uint64_t>);
		break;
	case VType::raw_arr_flo:
		b.finalize(IndexArrayCopyStatic<typ, float>);
		break;
	case VType::raw_arr_doub:
		b.finalize(IndexArrayCopyStatic<typ, double>);
		break;
	case VType::uarr:
		b.finalize(IndexArrayCopyStatic<typ>);
		break;
	case VType::faarr:
	case VType::saarr:
		b.finalize(IndexArrayCopyStatic<typ, ValueItem>);
		break;
	case VType::class_:
	case VType::morph:
	case VType::proxy:
		b.finalize(IndexArrayStaticInterface<0>);
		break;
	default:
		throw CompileTimeException("Invalid opcode, unsupported static type for this operation");
	}
}
template<char typ>
void inlineIndexArrayMoveStatic(BuildCall& b, VType type) {
	switch (type) {
	case VType::raw_arr_i8:
		b.finalize(IndexArrayMoveStatic<typ, int8_t>);
		break;
	case VType::raw_arr_i16:
		b.finalize(IndexArrayMoveStatic<typ, int16_t>);
		break;
	case VType::raw_arr_i32:
		b.finalize(IndexArrayMoveStatic<typ, int32_t>);
		break;
	case VType::raw_arr_i64:
		b.finalize(IndexArrayMoveStatic<typ, int64_t>);
		break;
	case VType::raw_arr_ui8:
		b.finalize(IndexArrayMoveStatic<typ, uint8_t>);
		break;
	case VType::raw_arr_ui16:
		b.finalize(IndexArrayMoveStatic<typ, uint16_t>);
		break;
	case VType::raw_arr_ui32:
		b.finalize(IndexArrayMoveStatic<typ, uint32_t>);
		break;
	case VType::raw_arr_ui64:
		b.finalize(IndexArrayMoveStatic<typ, uint64_t>);
		break;
	case VType::raw_arr_flo:
		b.finalize(IndexArrayMoveStatic<typ, float>);
		break;
	case VType::raw_arr_doub:
		b.finalize(IndexArrayMoveStatic<typ, double>);
		break;
	case VType::uarr:
		b.finalize(IndexArrayMoveStatic<typ>);
		break;
	case VType::faarr:
	case VType::saarr:
		b.finalize(IndexArrayMoveStatic<typ, ValueItem>);
		break;
	default:
		throw CompileTimeException("Invalid opcode, unsupported static type for this operation");
	}
}

#pragma endregion
#pragma region Dynamic_ArrayAccessors
template<char typ>
void IndexArrayCopyDynamic(void** value, ValueItem* arr, uint64_t pos) {
	switch (arr->meta.vtype) {
	case VType::uarr:
		IndexArrayCopyStatic<typ>(value, (list_array<ValueItem>**)arr, pos);
		break;
	case VType::faarr:
	case VType::saarr:
		IndexArrayCopyStatic<typ, ValueItem>(value, (void**)arr, pos);
		break;
	case VType::raw_arr_i8:
		IndexArrayCopyStatic<typ, int8_t>(value, (void**)arr, pos);
		break;
	case VType::raw_arr_i16:
		IndexArrayCopyStatic<typ, int16_t>(value, (void**)arr, pos);
		break;
	case VType::raw_arr_i32:
		IndexArrayCopyStatic<typ, int32_t>(value, (void**)arr, pos);
		break;
	case VType::raw_arr_i64:
		IndexArrayCopyStatic<typ, int64_t>(value, (void**)arr, pos);
		break;
	case VType::raw_arr_ui8:
		IndexArrayCopyStatic<typ, uint8_t>(value, (void**)arr, pos);
		break;
	case VType::raw_arr_ui16:
		IndexArrayCopyStatic<typ, uint16_t>(value, (void**)arr, pos);
		break;
	case VType::raw_arr_ui32:
		IndexArrayCopyStatic<typ, uint32_t>(value, (void**)arr, pos);
		break;
	case VType::raw_arr_ui64:
		IndexArrayCopyStatic<typ, uint64_t>(value, (void**)arr, pos);
		break;
	case VType::raw_arr_flo:
		IndexArrayCopyStatic<typ, uint64_t>(value, (void**)arr, pos);
		break;
	case VType::raw_arr_doub:
		IndexArrayCopyStatic<typ, uint64_t>(value, (void**)arr, pos);
		break;
	case VType::class_:
	case VType::morph:
	case VType::proxy:
		IndexArrayStaticInterface<typ>(value, arr, pos);
		break;
	default:
		throw NotImplementedException();
	}
}
template<char typ>
void IndexArrayMoveDynamic(void** value, ValueItem* arr, uint64_t pos) {
	switch (arr->meta.vtype) {
	case VType::uarr:
		IndexArrayMoveStatic<typ>(value, (list_array<ValueItem>**)arr, pos);
		break;
	case VType::faarr:
	case VType::saarr:
		IndexArrayMoveStatic<typ, ValueItem>(value, (void**)arr, pos);
		break;
	case VType::raw_arr_i8:
		IndexArrayMoveStatic<typ, int8_t>(value, (void**)arr, pos);
		break;
	case VType::raw_arr_i16:
		IndexArrayMoveStatic<typ, int16_t>(value, (void**)arr, pos);
		break;
	case VType::raw_arr_i32:
		IndexArrayMoveStatic<typ, int32_t>(value, (void**)arr, pos);
		break;
	case VType::raw_arr_i64:
		IndexArrayMoveStatic<typ, int64_t>(value, (void**)arr, pos);
		break;
	case VType::raw_arr_ui8:
		IndexArrayMoveStatic<typ, uint8_t>(value, (void**)arr, pos);
		break;
	case VType::raw_arr_ui16:
		IndexArrayMoveStatic<typ, uint16_t>(value, (void**)arr, pos);
		break;
	case VType::raw_arr_ui32:
		IndexArrayMoveStatic<typ, uint32_t>(value, (void**)arr, pos);
		break;
	case VType::raw_arr_ui64:
		IndexArrayMoveStatic<typ, uint64_t>(value, (void**)arr, pos);
		break;
	case VType::raw_arr_flo:
		IndexArrayMoveStatic<typ, uint64_t>(value, (void**)arr, pos);
		break;
	case VType::raw_arr_doub:
		IndexArrayMoveStatic<typ, uint64_t>(value, (void**)arr, pos);
		break;
	default:
		throw NotImplementedException();
	}
}


template<char typ>
void IndexArraySetCopyDynamic(void** value, ValueItem* arr, uint64_t pos) {
	switch (arr->meta.vtype) {
	case VType::uarr:
		IndexArraySetCopyStatic<typ>(value, (list_array<ValueItem>**)arr, pos);
		break;
	case VType::faarr:
	case VType::saarr:
		IndexArraySetCopyStatic<typ, ValueItem>(value, (void**)arr, (uint32_t)pos);
		break;
	case VType::raw_arr_i8:
		IndexArraySetCopyStatic<typ, int8_t>(value, (void**)arr, (uint32_t)pos);
		break;
	case VType::raw_arr_i16:
		IndexArraySetCopyStatic<typ, int16_t>(value, (void**)arr, (uint32_t)pos);
		break;
	case VType::raw_arr_i32:
		IndexArraySetCopyStatic<typ, int32_t>(value, (void**)arr, (uint32_t)pos);
		break;
	case VType::raw_arr_i64:
		IndexArraySetCopyStatic<typ, int64_t>(value, (void**)arr, (uint32_t)pos);
		break;
	case VType::raw_arr_ui8:
		IndexArraySetCopyStatic<typ, uint8_t>(value, (void**)arr, (uint32_t)pos);
		break;
	case VType::raw_arr_ui16:
		IndexArraySetCopyStatic<typ, uint16_t>(value, (void**)arr, (uint32_t)pos);
		break;
	case VType::raw_arr_ui32:
		IndexArraySetCopyStatic<typ, uint32_t>(value, (void**)arr, (uint32_t)pos);
		break;
	case VType::raw_arr_ui64:
		IndexArraySetCopyStatic<typ, uint64_t>(value, (void**)arr, (uint32_t)pos);
		break;
	case VType::raw_arr_flo:
		IndexArraySetCopyStatic<typ, uint64_t>(value, (void**)arr, (uint32_t)pos);
		break;
	case VType::raw_arr_doub:
		IndexArraySetCopyStatic<typ, uint64_t>(value, (void**)arr, (uint32_t)pos);
		break;
	case VType::class_:
	case VType::morph:
	case VType::proxy:
		IndexArraySetStaticInterface<typ>(value, arr, pos);
		break;
	default:
		throw NotImplementedException();
	}
}
template<char typ>
void IndexArraySetMoveDynamic(void** value, ValueItem* arr, uint64_t pos) {
	switch (arr->meta.vtype) {
	case VType::uarr:
		IndexArraySetMoveStatic<typ>(value, (list_array<ValueItem>**)arr, pos);
		break;
	case VType::faarr:
	case VType::saarr:
		IndexArraySetMoveStatic<typ, ValueItem>(value, (void**)arr, (uint32_t)pos);
		break;
	case VType::raw_arr_i8:
		IndexArraySetMoveStatic<typ, int8_t>(value, (void**)arr, (uint32_t)pos);
		break;
	case VType::raw_arr_i16:
		IndexArraySetMoveStatic<typ, int16_t>(value, (void**)arr, (uint32_t)pos);
		break;
	case VType::raw_arr_i32:
		IndexArraySetMoveStatic<typ, int32_t>(value, (void**)arr, (uint32_t)pos);
		break;
	case VType::raw_arr_i64:
		IndexArraySetMoveStatic<typ, int64_t>(value, (void**)arr, (uint32_t)pos);
		break;
	case VType::raw_arr_ui8:
		IndexArraySetMoveStatic<typ, uint8_t>(value, (void**)arr, (uint32_t)pos);
		break;
	case VType::raw_arr_ui16:
		IndexArraySetMoveStatic<typ, uint16_t>(value, (void**)arr, (uint32_t)pos);
		break;
	case VType::raw_arr_ui32:
		IndexArraySetMoveStatic<typ, uint32_t>(value, (void**)arr, (uint32_t)pos);
		break;
	case VType::raw_arr_ui64:
		IndexArraySetMoveStatic<typ, uint64_t>(value, (void**)arr, (uint32_t)pos);
		break;
	case VType::raw_arr_flo:
		IndexArraySetMoveStatic<typ, uint64_t>(value, (void**)arr, (uint32_t)pos);
		break;
	case VType::raw_arr_doub:
		IndexArraySetMoveStatic<typ, uint64_t>(value, (void**)arr, (uint32_t)pos);
		break;
	default:
		throw NotImplementedException();
	}
}
#pragma endregion










void takeStart(list_array<ValueItem>* dest, void** insert) {
	reinterpret_cast<ValueItem&>(insert) = dest->take_front();
}
void takeEnd(list_array<ValueItem>* dest, void** insert) {
	reinterpret_cast<ValueItem&>(insert) = dest->take_back();
}
void take(size_t pos, list_array<ValueItem>* dest, void** insert) {
	reinterpret_cast<ValueItem&>(insert) = dest->take(pos);
}
void getRange(list_array<ValueItem>* dest, void** insert, size_t start, size_t end) {
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



void setValue(void*& val, void* set, ValueMeta meta) {
	val = copyValue(set, meta);
}
void getInterfaceValue(ClassAccess access, ValueItem* val, const std::string* val_name, ValueItem* res) {
	*res = AttachA::Interface::getValue(access, *val, *val_name);
}
#ifdef _WIN64
#include <dbgeng.h>
EXCEPTION_DISPOSITION __attacha_handle(
	IN PEXCEPTION_RECORD ExceptionRecord,
	IN ULONG64 EstablisherFrame,
	IN OUT PCONTEXT ContextRecord,
	IN OUT PDISPATCHER_CONTEXT DispatcherContext
) {






	printf("hello!");
	return ExceptionContinueSearch;
}
#else
void __attacha_handle(void//not implemented
) {
	return;
}

#endif

void FuncEnviropment::RuntimeCompile() {
	if (curr_func != nullptr)
		FrameResult::deinit(frame, curr_func, jrt);
	values.clear();
	CodeHolder code;
	code.init(jrt.environment());
	CASM a(code);

	BuildProlog bprolog(a);
	bprolog.pushReg(frame_ptr);
	bprolog.pushReg(enviro_ptr);
	if (max_values)
		bprolog.pushReg(arg_ptr);
	bprolog.alignPush();
	bprolog.stackAlloc(0x20);//function vc++ abi
	bprolog.setFrame();
	bprolog.end_prolog();
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
				meta.as_ref = false;
				{
					if (needAlloc(meta)) {
						BuildCall v(a, true);
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
					BuildCall b(a, true);
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
				if (needAlloc(readData<ValueMeta>(data, data_len, i))) {
					BuildCall b(a, true);
					b.leaEnviro(readData<uint16_t>(data, data_len, i));
					b.finalize(universalRemove);
				}
				else
					a.movEnviroMeta(readData<uint16_t>(data, data_len, i), 0);
				break;
			case Opcode::arg_set: {
				uint16_t item = readData<uint16_t>(data, data_len, i);
				a.movEnviro(arg_ptr, item);
				a.getEnviroMetaSize(arg_len_32, item);
				break;
			}
			case Opcode::throw_ex: {
				bool in_memory = readData<bool>(data, data_len, i);
				if (in_memory) {
					BuildCall b(a, true);
					b.addArg(readData<uint16_t>(data, data_len, i));
					b.addArg(readData<uint16_t>(data, data_len, i));
					b.finalize(throwStatEx);
				}
				else {
					auto ex_typ = string_index_seter++;
					values[ex_typ] = readString(data, data_len, i);
					auto ex_desc = string_index_seter++;
					values[ex_desc] = readString(data, data_len, i);
					BuildCall b(a, true);
					b.addArg(values[ex_typ].val);
					b.addArg(values[ex_desc].val);
					b.finalize(throwEx);
				}
				break;
			}
			case Opcode::arr_op: {
				uint16_t arr = readData<uint16_t>(data, data_len, i);
				BuildCall b(a, true);
				auto flags = readData<OpArrFlags>(data, data_len, i);
				switch (readData<OpcodeArray>(data, data_len, i)) {
				case OpcodeArray::set: {
					VType type = readData<VType>(data, data_len, i);
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
						switch (flags.checked) {
						case ArrCheckMode::no_check:
							inlineIndexArraySetMoveStatic<0>(b, type);
							break;
						case ArrCheckMode::check:
							inlineIndexArraySetMoveStatic<1>(b, type);
							break;
						case ArrCheckMode::no_throw_check:
							inlineIndexArraySetMoveStatic<2>(b, type);
							break;
						default:
							break;
						}
					}
					else {
						switch (flags.checked)
						{
						case ArrCheckMode::no_check:
							inlineIndexArraySetCopyStatic<0>(b, type);
							break;
						case ArrCheckMode::check:
							inlineIndexArraySetCopyStatic<1>(b, type);
							break;
						case ArrCheckMode::no_throw_check:
							inlineIndexArraySetCopyStatic<2>(b, type);
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
					VType type = readData<VType>(data, data_len, i);
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
						case ArrCheckMode::no_check:
							inlineIndexArrayMoveStatic<0>(b, type);
							break;
						case ArrCheckMode::check:
							inlineIndexArrayMoveStatic<1>(b, type);
							break;
						case ArrCheckMode::no_throw_check:
							inlineIndexArrayMoveStatic<2>(b, type);
							break;
						default:
							break;
						}
					}
					else {
						switch (flags.checked)
						{
						case ArrCheckMode::no_check:
							inlineIndexArrayCopyStatic<0>(b, type);
							break;
						case ArrCheckMode::check:
							inlineIndexArrayCopyStatic<1>(b, type);
							break;
						case ArrCheckMode::no_throw_check:
							inlineIndexArrayCopyStatic<2>(b, type);
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

						a.pop(argr1);

						b.addArg(set_to);
						b.addArg(argr1);
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

						a.pop(argr1);

						b.addArg(set_to);
						b.addArg(argr1);
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
					b.addArg(readData<uint64_t>(data, data_len, i));
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
					b.addArg(readData<uint64_t>(data, data_len, i));
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
						b.addArg(readData<uint64_t>(data, data_len, i));
					b.finalize(&list_array<ValueItem>::reserve_push_front);
					break;
				}

				case OpcodeArray::commit: {
					BuildCall b(a, true);
					b.movEnviro(arr);
					b.finalize(&list_array<ValueItem>::commit);
					break;
				}
				case OpcodeArray::decommit: {
					BuildCall b(a, true);
					b.movEnviro(arr);
					b.addArg(readData<uint64_t>(data, data_len, i));
					b.finalize(&list_array<ValueItem>::decommit);
					break;
				}
				case OpcodeArray::remove_reserved: {
					BuildCall b(a, true);
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
				meta.as_ref = false;
				uint16_t optional_len = 0;
				{
					BuildCall v(a, true);
					v.leaEnviro(value_index);
					v.addArg(meta.encoded);
					v.addArg(cmd.is_gc_mode);
					v.finalize(preSetValue);
				}
				switch (meta.vtype) {
				case VType::i8:
				case VType::ui8:
				case VType::type_identifier:
					if (!meta.use_gc)
						a.mov(resr, 0, 1, readData<uint8_t>(data, data_len, i));
					else
						values.push_back(ValueItem(new lgr(new uint8_t(readData<uint8_t>(data, data_len, i))), meta, true));
					break;
				case VType::i16:
				case VType::ui16:
					if (!meta.use_gc)
						a.mov(resr, 0, 2, readData<uint16_t>(data, data_len, i));
					else
						values.push_back(ValueItem(new lgr(new uint16_t(readData<uint16_t>(data, data_len, i))), meta, true));
					break;
				case VType::i32:
				case VType::ui32:
				case VType::flo:
					if (!meta.use_gc)
						a.mov(resr, 0, 4, readData<uint32_t>(data, data_len, i));
					else
						values.push_back(ValueItem(new lgr(new uint32_t(readData<uint32_t>(data, data_len, i))), meta, true));
					break;
				case VType::i64:
				case VType::ui64:
				case VType::doub:
				case VType::undefined_ptr:
					if (!meta.use_gc)
						a.mov(resr, 0, 8, readData<uint64_t>(data, data_len, i));
					else
						values.push_back(ValueItem(new lgr(new uint64_t(readData<uint64_t>(data, data_len, i))), meta, true));
					break;
				case VType::raw_arr_i8:
				case VType::raw_arr_ui8: {
					optional_len = readLen(data, data_len, i);
					if (!meta.use_gc)
						values.push_back(ValueItem(readRawArray<int8_t>(data, data_len, i, optional_len), meta, true));
					else
						values.push_back(ValueItem(new lgr(readRawArray<int8_t>(data, data_len, i, optional_len)), meta, true));
					break;
				}
				case VType::raw_arr_i16:
				case VType::raw_arr_ui16: {
					optional_len = readLen(data, data_len, i);
					if (!meta.use_gc)
						values.push_back(ValueItem(readRawArray<int16_t>(data, data_len, i, optional_len), meta, true));
					else
						values.push_back(ValueItem(new lgr(readRawArray<int16_t>(data, data_len, i, optional_len)), meta, true));
					break;
				}
				case VType::raw_arr_i32:
				case VType::raw_arr_ui32:
				case VType::raw_arr_flo: {
					optional_len = readLen(data, data_len, i);
					if (!meta.use_gc)
						values.push_back(ValueItem(readRawArray<int32_t>(data, data_len, i, optional_len), meta, true));
					else
						values.push_back(ValueItem(new lgr(readRawArray<int32_t>(data, data_len, i, optional_len)), meta, true));
					break;
				}
				case VType::raw_arr_i64:
				case VType::raw_arr_ui64:
				case VType::raw_arr_doub: {
					optional_len = readLen(data, data_len, i);
					if (!meta.use_gc)
						values.push_back(ValueItem(readRawArray<int64_t>(data, data_len, i, optional_len), meta, true));
					else
						values.push_back(ValueItem(new lgr(readRawArray<int64_t>(data, data_len, i, optional_len)), meta, true));
					break;
				}
				case VType::uarr: {
					if (!meta.use_gc)
						values.push_back(ValueItem(readAnyUarr(data, data_len, i)));
					else
						values.push_back(ValueItem(new lgr(new list_array<ValueItem>(readAnyUarr(data, data_len, i))), meta));
					break;
				}
				case VType::string: {
					if (!meta.use_gc)
						values.push_back(readString(data, data_len, i));
					else
						values.push_back(ValueItem(new lgr(new std::string(readString(data, data_len, i))), meta));
					break;
				}
				case VType::faarr: {
					optional_len = readLen(data, data_len, i);
					if (!meta.use_gc) {
						meta.val_len = optional_len;
						values.push_back(ValueItem(readRawAny(data, data_len, i, optional_len), meta));
					}
					else
						values.push_back(ValueItem(new lgr(readRawAny(data, data_len, i, optional_len)), meta));
					break;
				}
				default:
					break;
				}

				if (needAlloc(meta)) {
					BuildCall b(a, true);
					b.addArg(resr);
					b.addArg(values.back().val);
					b.addArg(meta.encoded);
					b.finalize(setValue);
					if (is_raw_array(meta.vtype)) {
						a.mov(resr, uint64_t(optional_len) << 32);
						a.or_(resr, enviro_ptr, CASM::enviroMetaOffset(value_index));
						a.movEnviroMeta(value_index, resr);
					}
					break;
				}

				break;
			}
			case Opcode::set_saar: {
				uint16_t value_index = readData<uint16_t>(data, data_len, i);
				uint32_t len = readData<uint32_t>(data, data_len, i);
				BuildCall b(a, true);
				b.leaEnviro(value_index);
				b.addArg(ValueMeta(VType::saarr, false, true, len).encoded);
				b.addArg(cmd.is_gc_mode);
				b.finalize(preSetValue);
				a.lea(argr0, stack_ptr, 0 /*-CASM_REDZONE_SIZE*/);
				a.stackIncrease(len * sizeof(ValueItem));
				a.mov(resr, 0, 8, argr0);
				break;
			}
			case Opcode::remove: {
				BuildCall b(a, true);
				b.leaEnviro(readData<uint16_t>(data, data_len, i));
				b.finalize(universalRemove);
				break;
			}
			case Opcode::sum: {
				BuildCall b(a, true);
				b.leaEnviro(readData<uint16_t>(data, data_len, i));
				b.leaEnviro(readData<uint16_t>(data, data_len, i));
				b.finalize(DynSum);
				break;
			}
			case Opcode::minus: {
				BuildCall b(a, true);
				b.leaEnviro(readData<uint16_t>(data, data_len, i));
				b.leaEnviro(readData<uint16_t>(data, data_len, i));
				b.finalize(DynMinus);
				break;
			}
			case Opcode::div: {
				BuildCall b(a, true);
				b.leaEnviro(readData<uint16_t>(data, data_len, i));
				b.leaEnviro(readData<uint16_t>(data, data_len, i));
				b.finalize(DynDiv);
				break;
			}
			case Opcode::rest: {
				BuildCall b(a, true);
				b.leaEnviro(readData<uint16_t>(data, data_len, i));
				b.leaEnviro(readData<uint16_t>(data, data_len, i));
				b.finalize(DynRest);
				break;
			}
			case Opcode::mul: {
				BuildCall b(a, true);
				b.leaEnviro(readData<uint16_t>(data, data_len, i));
				b.leaEnviro(readData<uint16_t>(data, data_len, i));
				b.finalize(DynMul);
				break;
			}
			case Opcode::bit_xor: {
				BuildCall b(a, true);
				b.leaEnviro(readData<uint16_t>(data, data_len, i));
				b.leaEnviro(readData<uint16_t>(data, data_len, i));
				b.finalize(DynBitXor);
				break;
			}
			case Opcode::bit_or: {
				BuildCall b(a, true);
				b.leaEnviro(readData<uint16_t>(data, data_len, i));
				b.leaEnviro(readData<uint16_t>(data, data_len, i));
				b.finalize(DynBitOr);
				break;
			}
			case Opcode::bit_and: {
				BuildCall b(a, true);
				b.leaEnviro(readData<uint16_t>(data, data_len, i));
				b.leaEnviro(readData<uint16_t>(data, data_len, i));
				b.finalize(DynBitAnd);
				break;
			}
			case Opcode::bit_not: {
				BuildCall b(a, true);
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

				BuildCall b(a, true);
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
				BuildCall b(a, true);
				uint16_t item = readData<uint16_t>(data, data_len, i);
				b.leaEnviro(item);
				b.finalize(AsArg);
				a.mov(arg_ptr, resr);
				a.getEnviroMetaSize(arg_len_32, item);
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
				BuildCall b(a, true);
				b.addArg(this);
				b.addArg(arg_ptr);
				b.addArg(arg_len_32);
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
				BuildCall b(a, true);
				b.addArg(this);
				b.addArg(arg_ptr);
				b.addArg(arg_len_32);
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
				BuildCall b(a, true);
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
					BuildCall b(a, true);
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
					BuildCall b(a, true);
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
				BuildCall b(a, true);
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
						case ArrCheckMode::no_check:
							b.finalize(IndexArraySetMoveDynamic<0>);
							break;
						case ArrCheckMode::check:
							b.finalize(IndexArraySetMoveDynamic<1>);
							break;
						case ArrCheckMode::no_throw_check:
							b.finalize(IndexArraySetMoveDynamic<2>);
							break;
						default:
							break;
						}
					}
					else {
						switch (flags.checked)
						{
						case ArrCheckMode::no_check:
							b.finalize(IndexArraySetCopyDynamic<0>);
							break;
						case ArrCheckMode::check:
							b.finalize(IndexArraySetCopyDynamic<1>);
							break;
						case ArrCheckMode::no_throw_check:
							b.finalize(IndexArraySetCopyDynamic<2>);
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
						case ArrCheckMode::no_check:
							b.finalize(IndexArrayMoveDynamic<0>);
							break;
						case ArrCheckMode::check:
							b.finalize(IndexArrayMoveDynamic<1>);
							break;
						case ArrCheckMode::no_throw_check:
							b.finalize(IndexArrayMoveDynamic<2>);
							break;
						default:
							break;
						}
					}
					else {
						switch (flags.checked)
						{
						case ArrCheckMode::no_check:
							b.finalize(IndexArrayCopyDynamic<0>);
							break;
						case ArrCheckMode::check:
							b.finalize(IndexArrayCopyDynamic<1>);
							break;
						case ArrCheckMode::no_throw_check:
							b.finalize(IndexArrayCopyDynamic<2>);
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

						a.pop(argr1);

						b.addArg(set_to);
						b.addArg(argr1);
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

						a.pop(argr1);

						b.addArg(set_to);
						b.addArg(argr1);
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
					b.addArg(readData<uint64_t>(data, data_len, i));
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
					b.addArg(readData<uint64_t>(data, data_len, i));
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
						b.addArg(readData<uint64_t>(data, data_len, i));
					b.finalize(&list_array<ValueItem>::reserve_push_front);
					break;
				}

				case OpcodeArray::commit: {
					BuildCall b(a, true);
					b.movEnviro(arr);
					b.finalize(&list_array<ValueItem>::commit);
					break;
				}
				case OpcodeArray::decommit: {
					BuildCall b(a, true);
					b.movEnviro(arr);
					b.addArg(readData<uint64_t>(data, data_len, i));
					b.finalize(&list_array<ValueItem>::decommit);
					break;
				}
				case OpcodeArray::remove_reserved: {
					BuildCall b(a, true);
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
					BuildCall b(a, true);
					b.leaEnviro(readData<uint16_t>(data, data_len, i));
					b.leaEnviro(readData<uint16_t>(data, data_len, i));
					b.finalize(throwDirEx);
				}
				else {
					auto ex_typ = string_index_seter++;
					values[ex_typ] = readString(data, data_len, i);
					auto ex_desc = string_index_seter++;
					values[ex_desc] = readString(data, data_len, i);
					BuildCall b(a, true);
					b.addArg(values[ex_typ].val);
					b.addArg(values[ex_desc].val);
					b.finalize(throwEx);
				}
				break;
			}
			case Opcode::as: {
				BuildCall b(a, true);
				b.leaEnviro(readData<uint16_t>(data, data_len, i));
				b.addArg(readData<VType>(data, data_len, i));
				b.finalize(asValue);
				break;
			}
			case Opcode::is: {
				a.movEnviroMeta(resr, readData<uint16_t>(data, data_len, i));
				a.mov(argr0_8l, readData<VType>(data, data_len, i));
				a.cmp(resr_8l, argr0_8l);
				break;
			}
			case Opcode::store_bool: {
				BuildCall b(a, true);
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
				BuildCall b(a, true);
				b.addArg(resr_8h);
				b.leaEnviro(readData<uint16_t>(data, data_len, i));
				b.finalize(setBoolValue);
				break;
			}
			case Opcode::inline_native: {
				uint32_t len = readData<uint32_t>(data, data_len, i);
				auto tmp = readRawArray<uint8_t>(data, data_len, i, len);
				a.insertNative(tmp, len);
				break;
			}
			case Opcode::call_value_function: {
				compilerFabric_value_call<true, true>(a, data, data_len, i, values);
				break;
			}
			case Opcode::call_value_function_and_ret: {
				compilerFabric_value_call<false, false>(a, data, data_len, i, values);
				do_jump_to_ret = true;
				break;
			}
			case Opcode::static_call_value_function: {
				compilerFabric_static_value_call<true, true>(a, data, data_len, i, values);
				break;
			}
			case Opcode::static_call_value_function_and_ret: {
				compilerFabric_static_value_call<false, false>(a, data, data_len, i, values);
				do_jump_to_ret = true;
				break;
			}
			case Opcode::set_structure_value: {
				BuildCall b(a, true);
				if (readData<bool>(data, data_len, i)) {
					uint16_t value_index = readData<uint16_t>(data, data_len, i);
					b.leaEnviro(value_index);
					b.addArg(VType::string);
					b.finalize(getSpecificValue);
					b.addArg(readData<ClassAccess>(data, data_len, i));
					b.leaEnviro(readData<uint16_t>(data, data_len, i));//interface
					b.addArg(resr);
				}
				else {
					std::string fnn = readString(data, data_len, i);
					values.push_back(fnn);
					b.addArg((std::string*)values.back().val);
					b.addArg(readData<ClassAccess>(data, data_len, i));
					b.leaEnviro(readData<uint16_t>(data, data_len, i));//interface
				}
				b.leaEnviro(readData<uint16_t>(data, data_len, i));//value
				b.finalize((void(*)(ClassAccess, ValueItem&, const std::string&, ValueItem&))AttachA::Interface::setValue);
				break;
			}
			case Opcode::get_structure_value: {
				BuildCall b(a, true);
				if (readData<bool>(data, data_len, i)) {
					uint16_t value_index = readData<uint16_t>(data, data_len, i);
					b.leaEnviro(value_index);
					b.addArg(VType::string);
					b.finalize(getSpecificValue);
					b.addArg(readData<ClassAccess>(data, data_len, i));
					b.leaEnviro(readData<uint16_t>(data, data_len, i));//interface
					b.addArg(resr);
				}
				else {
					std::string fnn = readString(data, data_len, i);
					values.push_back(fnn);
					b.addArg((std::string*)values.back().val);
					b.addArg(readData<ClassAccess>(data, data_len, i));
					b.leaEnviro(readData<uint16_t>(data, data_len, i));//interface
				}
				b.leaEnviro(readData<uint16_t>(data, data_len, i));//save to
				b.finalize(getInterfaceValue);
				break;
			}
			case Opcode::explicit_await: {
				BuildCall b(a, true);
				b.leaEnviro(readData<uint16_t>(data, data_len, i));
				b.finalize(&ValueItem::getAsync);
				break;
			}
			case Opcode::generator_get:
				throw NotImplementedException();
			default:
				throw CompileTimeException("Invalid opcode");
			}
		};



		for (; i < data_len; ) {
			for (auto& it : j_list)
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
	auto& tmp = bprolog.finalize_epilog();

	tmp.use_handle = true;
	tmp.exHandleOff = a.offset();
	a.jmp(__attacha_handle);

	curr_func = (Enviropment)tmp.init(frame, a.code(), jrt, "attach_a_symbol");
}

extern "C" void callFunction(const char* symbol_name, bool run_async) {
	ValueItem* res = FuncEnviropment::callFunc(symbol_name, nullptr, 0, run_async);
	if (res)
		delete res;
}