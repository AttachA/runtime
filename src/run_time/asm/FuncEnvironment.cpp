// Copyright Danyil Melnytskyi 2022-2023
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include "FuncEnvironment.hpp"
#include "../../run_time.hpp"
#include "../attacha_abi.hpp"
#include "CASM.hpp"
#include "../tools.hpp"
#include "../AttachA_CXX.hpp"
#include "../tasks.hpp"
#include "../attacha_abi.hpp"
#include "../threading.hpp"
#include "../../../configuration/agreement/symbols.hpp"


using namespace run_time;
asmjit::JitRuntime jrt;
std::unordered_map<std::string, typed_lgr<FuncEnvironment>> enviropments;
TaskMutex enviropments_lock;


const char* try_resolve_frame(FuncEnvironment* env){
	for(auto& it : enviropments)
		if(it.second.getPtr() == env)
			return it.first.data();
	return "unresolved_attach_a_symbol";
}

FuncEnvironment::~FuncEnvironment() {
	art::lock_guard lguard(compile_lock);
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
	BuildCall b(a, 1);
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
			throw InvalidFunction("Invalid function header, unsupported label size: " + std::to_string(size) + " bytes, supported: 1,2,4,8");
		}
		list_array<std::pair<uint64_t, Label>> res;
		res.resize(labels);
		for (uint64_t i = 0; i < labels; i++)
			res[i] = { readData<uint64_t>(data,data_len,to_be_skiped), a.newLabel() };
		return res;
	}
	return {};
}

std::tuple<list_array<std::pair<uint64_t, Label>>, std::vector<typed_lgr<FuncEnvironment>>, FunctionMetaFlags, uint16_t, uint16_t, uint32_t, uint64_t> decodeFunctionHeader(CASM& a, const std::vector<uint8_t>& data, size_t data_len, size_t& to_be_skiped) {
	FunctionMetaFlags flags = readData<FunctionMetaFlags>(data, data_len, to_be_skiped);
	if(flags.length != data_len)
		throw InvalidFunction("Invalid function header, invalid function length");

	uint16_t used_static_values = 0;
	uint16_t used_enviro_vals = 0;
	uint32_t used_arguments = 0;
	uint64_t constants_values = 0;

	if(flags.used_static)
		used_static_values = readData<uint16_t>(data, data_len, to_be_skiped);
	if(flags.used_enviro_vals)
		used_enviro_vals = readData<uint16_t>(data, data_len, to_be_skiped);
	if(flags.used_arguments)
		used_arguments = readData<uint32_t>(data, data_len, to_be_skiped);
	constants_values = readPackedLen(data, data_len, to_be_skiped);
	std::vector<typed_lgr<FuncEnvironment>> locals;
	if(flags.has_local_functions){
		uint64_t locals_count = readPackedLen(data, data_len, to_be_skiped);
		locals.resize(locals_count);
		for (uint64_t i = 0; i < locals_count; i++) {
			uint64_t local_fn_len = readPackedLen(data, data_len, to_be_skiped);
			uint8_t* local_fn = extractRawArray<uint8_t>(data, data_len, to_be_skiped, local_fn_len);
			std::vector<uint8_t> local_fn_data(local_fn, local_fn + local_fn_len);
			locals[i] = new FuncEnvironment(local_fn_data);
		}
	}
	return { prepareJumpList(a, data, data_len, to_be_skiped), std::move(locals), flags, used_static_values, used_enviro_vals, used_arguments, constants_values};
}


#pragma region CompilerFabric helpers
template<bool use_result = true, bool do_cleanup = true>
void compilerFabric_call(CASM& a, const std::vector<uint8_t>& data, size_t data_len, size_t& i, list_array<ValueItem>& values, std::vector<ValueItem*> static_map) {
	CallFlags flags;
	flags.encoded = readData<uint8_t>(data, data_len, i);
	BuildCall b(a, 0);
	if (flags.in_memory) {
		b.lea_valindex({static_map, values}, readIndexPos(data, data_len, i));
		ValueIndexPos value_index = readIndexPos(data, data_len, i);
		b.lea_valindex({static_map, values},value_index);
		b.addArg(VType::string);
		b.finalize(getSpecificValue);
		b.addArg(resr);
		b.addArg(arg_ptr);
		b.addArg(arg_len_32);
		b.addArg(flags.async_mode);
		b.finalize(&FuncEnvironment::callFunc);
	}
	else {
		std::string fnn = readString(data, data_len, i);
		typed_lgr<FuncEnvironment> fn = FuncEnvironment::enviropment(fnn);
		if (fn->canBeUnloaded() || flags.async_mode) {
			values.push_back(fnn);
			b.addArg(((std::string*)values.back().val)->c_str());
			b.addArg(arg_ptr);
			b.addArg(arg_len_32);
			b.addArg(flags.async_mode);
			b.finalize(&FuncEnvironment::callFunc);
		}
		else {
			switch (fn->type()) {
			case FuncEnvironment::FuncType::own: {
				b.addArg(arg_ptr);
				b.addArg(arg_len_32);
				b.finalize(fn->get_func_ptr());
				break;
			}
			case FuncEnvironment::FuncType::native: {
				if (!fn->function_template().arguments.size() && fn->function_template().result.is_void()) {
					b.finalize(fn->get_func_ptr());
					if constexpr (use_result)
						if (flags.use_result) {
							a.mov_valindex({static_map, values}, readIndexPos(data, data_len, i), 0);
							return;
						}
					break;
				}
				b.addArg(fn.getPtr());
				b.addArg(arg_ptr);
				b.addArg(arg_len_32);
				b.finalize(&FuncEnvironment::NativeProxy_DynamicToStatic);
				break;
			}
			default: {
				b.addArg(fn.getPtr());
				b.addArg(arg_ptr);
				b.addArg(arg_len_32);
				b.finalize(&FuncEnvironment::syncWrapper);
				break;
			}
			}
		}
	}
	if constexpr (use_result) {
		if (flags.use_result) {
			b.lea_valindex({static_map, values}, readIndexPos(data, data_len, i));
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

void _compilerFabric_call_local_any(CASM& a, FuncEnvironment* env,bool async_mode, uint32_t function_index) {
	BuildCall b(a, 5);
	b.addArg(env);
	b.addArg(function_index);
	b.addArg(arg_ptr);
	b.addArg(arg_len_32);
	b.addArg(async_mode);
	b.finalize(&FuncEnvironment::localWrapper);
}
template<bool use_result = true, bool do_cleanup = true>
void compilerFabric_call_local(CASM& a, const std::vector<uint8_t>& data, size_t data_len, size_t& i, FuncEnvironment* env, list_array<ValueItem>& values,  std::vector<ValueItem*> static_map) {
	CallFlags flags;
	flags.encoded = readData<uint8_t>(data, data_len, i);
	BuildCall b(a, 0);
	if (flags.in_memory) {
		b.lea_valindex({static_map, values}, readIndexPos(data, data_len, i));
		b.finalize(getSize);
		b.setArguments(5);
		b.addArg(env);
		b.addArg(resr);
		b.addArg(arg_ptr);
		b.addArg(arg_len_32);
		b.addArg(flags.async_mode);
		b.finalize(&FuncEnvironment::localWrapper);
		b.setArguments(0);
	}
	else {
		uint32_t fnn = readData<uint32_t>(data, data_len, i);
		if (env->localFnSize() >= fnn) {
			throw InvalidIL("Invalid function index");
		}
		if (flags.async_mode) {
			_compilerFabric_call_local_any(a, env, true, fnn);
		}
		else {
			typed_lgr<FuncEnvironment> fn = env->localFn(fnn);
			switch (fn->type()) {
			case FuncEnvironment::FuncType::own: {
				if(!fn->get_func_ptr()){
					_compilerFabric_call_local_any(a, env, true, fnn);
					break;
				}
				b.addArg(arg_ptr);
				b.addArg(arg_len_32);
				b.finalize(fn->get_func_ptr());
				break;
			}
			case FuncEnvironment::FuncType::native: {
				if (!fn->function_template().arguments.size() && fn->function_template().result.is_void()) {
					b.finalize(fn->get_func_ptr());
					if constexpr (use_result)
						if (flags.use_result) {
							a.mov_valindex({static_map, values}, readIndexPos(data, data_len, i), 0);
							return;
						}
					break;
				}
				b.addArg(fn.getPtr());
				b.addArg(arg_ptr);
				b.addArg(arg_len_32);
				b.finalize(&FuncEnvironment::NativeProxy_DynamicToStatic);
				break;
			}
			default: {
				b.addArg(fn.getPtr());
				b.addArg(arg_ptr);
				b.addArg(arg_len_32);
				b.finalize(&FuncEnvironment::syncWrapper);
				break;
			}
			}
		}
	}
	if constexpr (use_result) {
		if (flags.use_result) {
			b.lea_valindex({static_map, values}, readIndexPos(data, data_len, i));
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
ValueItem* _valueItemDynamicCall(const std::string& name, ValueItem* class_ptr, ClassAccess access, ValueItem* args, uint32_t len) {
	switch (class_ptr->meta.vtype) {
	case VType::struct_:
		if constexpr (async_call)
			return FuncEnvironment::async_call(((Structure&)*class_ptr).get_method_dynamic(name, access), args, len);
		else
			return ((Structure&)*class_ptr).table_get_dynamic(name, access)(args, len);
	default:
		throw NotImplementedException();
	}
}
template<bool async_call>
ValueItem* _valueItemDynamicCall(uint64_t id, ValueItem* class_ptr, ValueItem* args, uint32_t len) {
	switch (class_ptr->meta.vtype) {
	case VType::struct_:
		if constexpr (async_call)
			return FuncEnvironment::async_call(((Structure&)*class_ptr).get_method(id), args, len);
		else
			return ((Structure&)*class_ptr).table_get(id)(args, len);
	default:
		throw NotImplementedException();
	}
}
template<bool async_mode>
ValueItem* valueItemDynamicCall(const std::string& name, ValueItem* class_ptr, ValueItem* args, uint32_t len, ClassAccess access) {
	if (!class_ptr)
		throw NullPointerException();
	class_ptr->getAsync();
	list_array<ValueItem> args_tmp;
	args_tmp.reserve_push_back(len + 1);
	args_tmp.push_back(ValueItem(*class_ptr, as_refrence));
	for(uint32_t i = 0; i < len; i++)
		args_tmp.push_back(ValueItem(args[i], as_refrence));
	return _valueItemDynamicCall<async_mode>(name, class_ptr, access, args_tmp.data(), len + 1);
}

template<bool async_mode>
ValueItem* valueItemDynamicCallId(uint64_t id, ValueItem* class_ptr, ValueItem* args, uint32_t len) {
	if (!class_ptr)
		throw NullPointerException();
	class_ptr->getAsync();
	list_array<ValueItem> args_tmp;
	args_tmp.reserve_push_back(len + 1);
	args_tmp.push_back(ValueItem(*class_ptr, as_refrence));
	for(uint32_t i = 0; i < len; i++)
		args_tmp.push_back(ValueItem(args[i], as_refrence));
		
	return _valueItemDynamicCall<async_mode>(id, class_ptr, args_tmp.data(), len + 1);
}

template<bool use_result = true, bool do_cleanup = true>
void compilerFabric_value_call(CASM& a, const std::vector<uint8_t>& data, size_t data_len, size_t& i, list_array<ValueItem>& values, std::vector<ValueItem*> static_map) {
	CallFlags flags;
	flags.encoded = readData<uint8_t>(data, data_len, i);
	BuildCall b(a, 0);
	if (flags.in_memory) {
		b.lea_valindex({static_map, values},readIndexPos(data, data_len, i));//value
		b.addArg(VType::string);
		b.finalize(getSpecificValue);
		b.setArguments(5);
		b.addArg(resr);
	}
	else {
		std::string fnn = readString(data, data_len, i);
		values.push_back(fnn);
		b.setArguments(5);
		b.addArg((std::string*)values.back().val);
	}

	b.lea_valindex({static_map, values},readIndexPos(data, data_len, i));//class
	b.addArg(arg_ptr);
	b.addArg(arg_len_32);
	b.addArg((uint8_t)readData<ClassAccess>(data, data_len, i));
	if (flags.async_mode)
		b.finalize(valueItemDynamicCall<true>);
	else
		b.finalize(valueItemDynamicCall<false>);

	b.setArguments(0);
	if constexpr (use_result) {
		if (flags.use_result) {
			b.lea_valindex({static_map, values}, readIndexPos(data, data_len, i));
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
void compilerFabric_value_call_id(CASM& a, const std::vector<uint8_t>& data, size_t data_len, size_t& i, list_array<ValueItem>& values, std::vector<ValueItem*> static_map) {
	CallFlags flags;
	flags.encoded = readData<uint8_t>(data, data_len, i);
	BuildCall b(a, 4);
	b.addArg(readData<uint64_t>(data, data_len, i));
	b.lea_valindex({static_map, values}, readIndexPos(data, data_len, i));//class
	b.addArg(arg_ptr);
	b.addArg(arg_len_32);
	if (flags.async_mode)
		b.finalize(valueItemDynamicCallId<true>);
	else
		b.finalize(valueItemDynamicCallId<false>);

	b.setArguments(0);
	if constexpr (use_result) {
		if (flags.use_result) {
			b.lea_valindex({static_map, values}, readIndexPos(data, data_len, i));
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


template<bool async_mode>
void* staticValueItemDynamicCall(const std::string& name, ValueItem* class_ptr, ValueItem* args, uint32_t len, ClassAccess access) {
	if (!class_ptr)
		throw NullPointerException();
	class_ptr->getAsync();
	if constexpr (async_mode)
		return _valueItemDynamicCall<true>(name, class_ptr, access, args, len);
	else
		return _valueItemDynamicCall<false>(name, class_ptr, access, args, len);
}

template<bool use_result = true, bool do_cleanup = true>
void compilerFabric_static_value_call(CASM& a, const std::vector<uint8_t>& data, size_t data_len, size_t& i, list_array<ValueItem>& values, std::vector<ValueItem*> static_map) {
	CallFlags flags;
	flags.encoded = readData<uint8_t>(data, data_len, i);
	BuildCall b(a, 0);
	if (flags.in_memory) {
		b.lea_valindex({static_map, values},readIndexPos(data, data_len, i));//value
		b.addArg(VType::string);
		b.finalize(getSpecificValue);
		b.setArguments(5);
		b.addArg(resr);
	}
	else {
		std::string fnn = readString(data, data_len, i);
		values.push_back(fnn);
		b.setArguments(5);
		b.addArg((std::string*)values.back().val);
	}

	b.lea_valindex({static_map, values},readIndexPos(data, data_len, i));
	b.addArg(arg_ptr);
	b.addArg(arg_len_32);
	b.addArg((uint8_t)readData<ClassAccess>(data, data_len, i));

	if (flags.async_mode)
		b.finalize(staticValueItemDynamicCall<true>);
	else
		b.finalize(staticValueItemDynamicCall<false>);
	
	
	b.setArguments(0);
	if constexpr (use_result) {
		if (flags.use_result) {
			b.lea_valindex({static_map, values}, readIndexPos(data, data_len, i));
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
template<bool async_mode>
void* staticValueItemDynamicCallId(uint64_t id, ValueItem* class_ptr, ValueItem* args, uint32_t len) {
	if (!class_ptr)
		throw NullPointerException();
	class_ptr->getAsync();
	if constexpr (async_mode)
		return _valueItemDynamicCall<true>(id, class_ptr, args, len);
	else
		return _valueItemDynamicCall<false>(id, class_ptr, args, len);
}

template<bool use_result = true, bool do_cleanup = true>
void compilerFabric_static_value_call_id(CASM& a, const std::vector<uint8_t>& data, size_t data_len, size_t& i, list_array<ValueItem>& values, std::vector<ValueItem*> static_map) {
	CallFlags flags;
	flags.encoded = readData<uint8_t>(data, data_len, i);
	BuildCall b(a, 4);
	b.addArg(readData<uint64_t>(data, data_len, i));
	b.lea_valindex({static_map, values},readIndexPos(data, data_len, i));
	b.addArg(arg_ptr);
	b.addArg(arg_len_32);
	if (flags.async_mode)
		b.finalize(staticValueItemDynamicCallId<true>);
	else
		b.finalize(staticValueItemDynamicCallId<false>);
	
	
	b.setArguments(0);
	if constexpr (use_result) {
		if (flags.use_result) {
			b.lea_valindex({static_map, values}, readIndexPos(data, data_len, i));
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
#pragma endregion

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
	size_t length = (size_t)AttachA::Interface::makeCall(ClassAccess::pub, *arr_ref, symbols::structures::size);
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
	ValueItem temp = AttachA::Interface::makeCall(ClassAccess::pub, *arr_ref, symbols::structures::index_operator, pos);
	*value = temp.val;
	*((size_t*)(value + 1)) = temp.meta.encoded;
	temp.val = nullptr;
}
template<char typ>
void IndexArraySetStaticInterface(void** value, ValueItem* arr_ref, uint64_t pos) {
	size_t length = (size_t)AttachA::Interface::makeCall(ClassAccess::pub, *arr_ref, symbols::structures::size);
	if constexpr (typ != 0) {
		if (length <= pos)
			throw OutOfRange();
	}
	AttachA::Interface::makeCall(ClassAccess::pub, *arr_ref, symbols::structures::index_set_operator, pos, reinterpret_cast<ValueItem&>(value));
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
	case VType::struct_:
		b.finalize(IndexArraySetStaticInterface<0>);
		break;
	default:
		throw InvalidIL("Invalid opcode, unsupported static type for this operation");
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
		throw InvalidIL("Invalid opcode, unsupported static type for this operation");
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
	case VType::struct_:
		b.finalize(IndexArrayStaticInterface<0>);
		break;
	default:
		throw InvalidIL("Invalid opcode, unsupported static type for this operation");
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
		throw InvalidIL("Invalid opcode, unsupported static type for this operation");
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
	case VType::struct_:
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
	case VType::struct_:
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
#pragma region Compiler Runtime Helper Functions
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




void throwDirEx(ValueItem* typ, ValueItem* desc) {
	throw AException((std::string)*typ,(std::string)*desc);
}
void throwStatEx(ValueItem* typ, ValueItem* desc) {
	throw AException(*(std::string*)typ->getSourcePtr(),*(std::string*)desc->getSourcePtr());
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
void* prepareStack(void** stack, size_t size) {
	while (size)
		stack[--size] = nullptr;
	return stack;
}

template<typename T>
void valueDestruct(void*& val){
	if (val != nullptr) {
		((T*)val)->~T();
		val = nullptr;
	}
}
void valueDestructDyn(void** val){
	if (val != nullptr) {
		((ValueItem*)val)->~ValueItem();
		val = nullptr;
	}
}
ValueItem* ValueItem_is_gc_proxy(ValueItem* val){
	return new ValueItem(val->meta.use_gc);
}
void ValueItem_xmake_slice00(ValueItem* result, ValueItem* val, uint32_t start, uint32_t end){
	*result = val->make_slice(start, end);
}
void ValueItem_xmake_slice10(ValueItem* result, ValueItem* val, ValueItem* start, uint32_t end){
	*result = val->make_slice((uint32_t)*start, end);
}
void ValueItem_xmake_slice01(ValueItem* result, ValueItem* val, uint32_t start, ValueItem* end){
	*result = val->make_slice(start, (uint32_t)*end);
}
void ValueItem_xmake_slice11(ValueItem* result, ValueItem* val, ValueItem* start, ValueItem* end){
	*result = val->make_slice((uint32_t)*start, (uint32_t)*end);
}
void Unchecked_make_slice00(ValueItem* result, ValueItem* arr, uint32_t start, uint32_t end){
	*result = ValueItem(arr + start, ValueMeta(VType::faarr,false,true, end - start), as_refrence);
}
#pragma endregion
#pragma region ScopeAction
#ifdef _WIN64
#include <dbgeng.h>




template<typename T>
T readFromArrayAsValue(uint8_t*& arr) {
	T& val = *(T*)arr;
	arr += sizeof(T);
	return val;
}
template<typename T>
T* readFromArrayAsArray(uint8_t*& arr, size_t& size) {
	size = readFromArrayAsValue<size_t>(arr);
	T* ret = new T[size];
	for (size_t i = 0; i < size; i++)
		ret[i] = readFromArrayAsValue<T>(arr);
	return ret;
}
template<typename T>
void skipArray(uint8_t*& arr) {
	size_t size = readFromArrayAsValue<size_t>(arr);
	arr += sizeof(T) * size;
}
EXCEPTION_DISPOSITION __attacha_handle(
	IN PEXCEPTION_RECORD ExceptionRecord,
	IN ULONG64 EstablisherFrame,
	IN OUT PCONTEXT ContextRecord,
	IN OUT PDISPATCHER_CONTEXT DispatcherContext
) {
	auto function_start = (uint8_t*)DispatcherContext->ImageBase;
	void* addr = (void*)DispatcherContext->ControlPc;
	uint8_t* data = (uint8_t*)DispatcherContext->HandlerData;
	bool execute = false;
	bool on_unwind = ExceptionRecord->ExceptionFlags & EXCEPTION_UNWINDING;
	std::exception_ptr current_ex = std::current_exception();
	try{
		while(true){
			ScopeAction::Action action = (ScopeAction::Action)*data++;
			if (action == ScopeAction::Action::not_action)
				return ExceptionContinueSearch;
			size_t start_offset = readFromArrayAsValue<size_t>(data);
			size_t end_offset = readFromArrayAsValue<size_t>(data);
			if (addr >= (void*)(function_start + start_offset) && addr < (void*)(function_start + end_offset)) 
				execute = true;
			switch(action) {
				case ScopeAction::Action::destruct_stack: {
					char* stack = (char*)ContextRecord->Rbp;
					auto destruct = readFromArrayAsValue<void(*)(void*&)>(data);
					stack += readFromArrayAsValue<uint64_t>(data);
					if(execute && on_unwind){
						destruct(*(void**)stack);
						ExceptionRecord->ExceptionFlags |= EXCEPTION_NONCONTINUABLE;
					}
					break;
				}
				case ScopeAction::Action::destruct_register: {
					auto destruct = readFromArrayAsValue<void(*)(void*&)>(data);
					if(!execute || !on_unwind){
						readFromArrayAsValue<uint32_t>(data);
						execute = false;
						continue;
					}
					switch (readFromArrayAsValue<uint32_t>(data))
					{
					case asmjit::x86::Gp::kIdAx:
						destruct(*(void**)ContextRecord->Rax);
						break;
					case asmjit::x86::Gp::kIdBx:
						destruct(*(void**)ContextRecord->Rbx);
						break;
					case asmjit::x86::Gp::kIdCx:
						destruct(*(void**)ContextRecord->Rcx);
						break;
					case asmjit::x86::Gp::kIdDx:
						destruct(*(void**)ContextRecord->Rdx);
						break;
					case asmjit::x86::Gp::kIdDi:
						destruct(*(void**)ContextRecord->Rdi);
						break;
					case asmjit::x86::Gp::kIdSi:
						destruct(*(void**)ContextRecord->Rsi);
						break;
					case asmjit::x86::Gp::kIdR8:
						destruct(*(void**)ContextRecord->R8);
						break;
					case asmjit::x86::Gp::kIdR9:
						destruct(*(void**)ContextRecord->R9);
						break;
					case asmjit::x86::Gp::kIdR10:
						destruct(*(void**)ContextRecord->R10);
						break;
					case asmjit::x86::Gp::kIdR11:
						destruct(*(void**)ContextRecord->R11);
						break;
					case asmjit::x86::Gp::kIdR12:
						destruct(*(void**)ContextRecord->R12);
						break;
					case asmjit::x86::Gp::kIdR13:
						destruct(*(void**)ContextRecord->R13);
						break;
					case asmjit::x86::Gp::kIdR14:
						destruct(*(void**)ContextRecord->R14);
						break;
					case asmjit::x86::Gp::kIdR15:
						destruct(*(void**)ContextRecord->R15);
						break;
					default:
						{
							ValueItem it {"Invalid register id"};
							errors.async_notify(it);
						}
						return ExceptionContinueSearch;
					}
				}
				case ScopeAction::Action::filter: {
					auto filter = readFromArrayAsValue<bool(*)(CXXExInfo&, void*&, void*, size_t, void*)>(data);
					if(!execute){
						skipArray<char>(data);
						execute = false;
						continue;
					}
					size_t size = 0;
					std::unique_ptr<char[]> stack;
					stack.reset(readFromArrayAsArray<char>(data, size));
					CXXExInfo info;
					getCxxExInfoFromNative1(info,ExceptionRecord);
					void* continue_from = nullptr;
					if(!on_unwind){
						if (filter(info,continue_from, stack.get(), size, (void*)ContextRecord->Rsp)){
							RtlUnwindEx(
								(void*)EstablisherFrame,
								continue_from,
								ExceptionRecord,
								UlongToPtr(ExceptionRecord->ExceptionCode),
								DispatcherContext->ContextRecord,DispatcherContext->HistoryTable
							);
							__debugbreak();
							ContextRecord->Rip = (uint64_t)continue_from;
							return ExceptionCollidedUnwind;
						}
						else
							return ExceptionContinueSearch;
					}
					break;
				}
				case ScopeAction::Action::converter: {
					auto convert = readFromArrayAsValue<void(*)(void*,size_t, void* rsp)>(data);
					size_t size = 0;
					std::unique_ptr<char[]> stack;
					stack.reset(readFromArrayAsArray<char>(data, size));
					convert(stack.get(), size, (void*)ContextRecord->Rsp);
					ValueItem args{"Exception converter will not return"};
					errors.async_notify(args);
					return ExceptionContinueSearch;
				}
				case ScopeAction::Action::finally:{
					auto finally = readFromArrayAsValue<void(*)(void* rsp)>(data);
					if(!execute || !on_unwind){
						skipArray<char>(data);
						execute = false;
						continue;
					}
					size_t size = 0;
					std::unique_ptr<char[]> stack;
					stack.reset(readFromArrayAsArray<char>(data, size));
					finally(stack.get());
					break;
				}
				case ScopeAction::Action::not_action:
					return ExceptionContinueSearch;
				default:
					throw BadOperationException();
			}
		}
	}catch(...){
		switch(exception_on_language_routine_action){
			case ExceptionOnLanguageRoutineAction::invite_to_debugger:{
				std::exception_ptr second_ex = std::current_exception();
				invite_to_debugger("In this program caught exception on language routine handle");
				break;
			}
			case ExceptionOnLanguageRoutineAction::nest_exception:
				throw RoutineHandleExceptions(current_ex, std::current_exception());
			case ExceptionOnLanguageRoutineAction::swap_exception:
				throw;
			case ExceptionOnLanguageRoutineAction::ignore:
				break;
		}
	}
	return ExceptionContinueSearch;
}
bool _attacha_filter(CXXExInfo& info, void** continue_from, void* data, size_t size, void* enviro) {
	uint8_t* data_info = (uint8_t*)data;
	list_array<std::string> exceptions;
	switch (*data_info++) {
		case 0: {
			uint64_t handle_count = readFromArrayAsValue<uint64_t>(data_info);
			exceptions.reserve_push_back(handle_count);
			for (size_t i = 0; i < handle_count; i++) {
				std::string string;
				size_t len = 0;
				char* str = readFromArrayAsArray<char>(data_info, len);
				string.assign(str, len);
				delete[] str;
				exceptions.push_back(string);
			}	
			break;
		}
		case 1: {
			uint16_t value = readFromArrayAsValue<uint16_t>(data_info);
			ValueItem* item = (ValueItem*)enviro + (uint32_t(value)<<1);
			exceptions.push_back((std::string)*item);
			break;
		}
		case 2: {
			uint64_t handle_count = readFromArrayAsValue<uint64_t>(data_info);
			exceptions.reserve_push_back(handle_count);
			for (size_t i = 0; i < handle_count; i++) {
				uint16_t value = readFromArrayAsValue<uint16_t>(data_info);
				ValueItem* item = (ValueItem*)enviro + (uint32_t(value)<<1);
				exceptions.push_back((std::string)*item);
			}
			break;
		}
		case 3: {
			uint64_t handle_count = readFromArrayAsValue<uint64_t>(data_info);
			exceptions.reserve_push_back(handle_count);
			for (size_t i = 0; i < handle_count; i++) {
				bool is_dynamic = readFromArrayAsValue<bool>(data_info);
				if (!is_dynamic) {
					std::string string;
					size_t len = 0;
					char* str = readFromArrayAsArray<char>(data_info, len);
					string.assign(str, len);
					delete[] str;
					exceptions.push_back(string);
				}
				else {
					uint16_t value = readFromArrayAsValue<uint16_t>(data_info);
					ValueItem* item = (ValueItem*)enviro + (uint32_t(value)<<1);
					exceptions.push_back((std::string)*item);
				}
			}
			break;
		}
		default:
			throw BadOperationException();
	}

	if(info.ex_ptr == nullptr) {
		switch (info.native_id) {
		case EXCEPTION_ACCESS_VIOLATION:
			return exceptions.contains("AccessViolation");
		case EXCEPTION_STACK_OVERFLOW:
			return exceptions.contains("StackOverflow");
		case EXCEPTION_INT_DIVIDE_BY_ZERO:
		case EXCEPTION_FLT_DIVIDE_BY_ZERO:
			return exceptions.contains("DivideByZero");
		case EXCEPTION_INT_OVERFLOW:
		case EXCEPTION_FLT_OVERFLOW:
		case EXCEPTION_FLT_STACK_CHECK:
		case EXCEPTION_FLT_UNDERFLOW:
			return exceptions.contains("NumericOverflow");
		case EXCEPTION_BREAKPOINT: 
			return exceptions.contains("Breakpoint");
		case EXCEPTION_ILLEGAL_INSTRUCTION:
			return exceptions.contains("IllegalInstruction");
		case EXCEPTION_PRIV_INSTRUCTION:
			return exceptions.contains("PrivilegedInstruction");
		default:
			return exceptions.contains("nUnknownNativeException");
		}
	} else {
		return exceptions.contains_one([&info](const std::string& str) {
			return info.ty_arr.contains_one([&str](const CXXExInfo::Tys& ty) {
				if(ty.is_bad_alloc)
					return str == "BadAlloc";
				return str == ty.ty_info->name();
			});
		});
	}
}
bool _attacha_converter(void* data, size_t size, void* rsp){
	uint8_t* data_info = (uint8_t*)data;
	return false;
}


#else
void __attacha_handle(void//not implemented
) {
	return;
}

#endif
#pragma endregion

#pragma region Compiler
struct ScopeManagerMap{
	std::unordered_map<uint64_t, size_t> handle_id_map;
	std::unordered_map<uint64_t, size_t> value_hold_id_map;
	ScopeManager& manager;
	ScopeManagerMap(ScopeManager& manager) :manager(manager) {}
	size_t mapHandle(uint64_t id){
		auto it = handle_id_map.find(id);
		if (it == handle_id_map.end())
			return handle_id_map[id] = manager.createExceptionScope();
		else
			return it->second;
	}
	size_t mapValueHold(uint64_t id, void(*destruct)(void**), uint16_t off){
		auto it = value_hold_id_map.find(id);
		if (it == value_hold_id_map.end())
			return value_hold_id_map[id] = manager.createValueLifetimeScope(destruct, (size_t)off<<1);
		else
			return it->second;
	}
	size_t try_mapHandle(uint64_t id){
		auto it = handle_id_map.find(id);
		if (it == handle_id_map.end())
			return -1;
		else
			return it->second;
	}
	size_t try_mapValueHold(uint64_t id, void(*destruct)(void**), uint16_t off){
		auto it = value_hold_id_map.find(id);
		if (it == value_hold_id_map.end())
			return -1;
		else
			return it->second;
	}
};
struct CompilerFabric{
	Command cmd;
	CASM& a;
	ScopeManager& scope;
	ScopeManagerMap& scope_map;
	Label& prolog;
	Label& self_function;
	std::vector<uint8_t>& data;
	size_t data_len;
	size_t i;
	size_t skip_count;
	std::unordered_map<uint64_t, Label> label_bind_map;
	std::unordered_map<uint64_t, Label*> label_map;
	list_array<ValueItem>& values;
	bool do_jump_to_ret = false;
	bool in_debug;
	FuncEnvironment* build_func;

	std::vector<ValueItem*> static_map;

	CompilerFabric(
		CASM& a,
		ScopeManager& scope,
		ScopeManagerMap& scope_map,
		Label& prolog,
		Label& self_function,
		std::vector<uint8_t>& data,
		size_t data_len,
		size_t start_from,
		list_array<std::pair<uint64_t, Label>>& jump_list,
		list_array<ValueItem>& values,
		bool in_debug,
		FuncEnvironment* build_func
		) : a(a), scope(scope), scope_map(scope_map), prolog(prolog), self_function(self_function), data(data), data_len(data_len), i(start_from),skip_count(start_from), values(values), in_debug(in_debug), build_func(build_func) {
			label_bind_map.reserve(jump_list.size());
			label_map.reserve(jump_list.size());
			size_t i = 0;
			for(auto& it : jump_list){
				label_map[i++] = &(label_bind_map[it.first] = it.second);
			}
		}

	asmjit::Label& resolve_label(uint64_t id) {
		auto label = label_map.find(id);
		if (label == label_map.end())
			throw InvalidFunction("Invalid function header, not found jump position for label: " + std::to_string(id));
		return *label->second;
	}
	
	
	void store_constant(){
		values.push_back(readAny(data, data_len, i));
	}


	
	
#pragma region dynamic opcodes
#pragma region set/remove/move/copy
	void dynamic_set(){
		ValueIndexPos value_index = readIndexPos(data, data_len, i);
		ValueMeta meta = readData<ValueMeta>(data, data_len, i);
		meta.as_ref = false;
		if(!meta.use_gc)
			meta.allow_edit = true;
		uint32_t optional_len = 0;
		{
			BuildCall v(a, 3);
			v.lea_valindex({static_map, values}, value_index);
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
				values.push_back(ValueItem(new lgr(new uint8_t(readData<uint8_t>(data, data_len, i))), meta, no_copy));
			break;
		case VType::i16:
		case VType::ui16:
			if (!meta.use_gc)
				a.mov(resr, 0, 2, readData<uint16_t>(data, data_len, i));
			else
				values.push_back(ValueItem(new lgr(new uint16_t(readData<uint16_t>(data, data_len, i))), meta, no_copy));
			break;
		case VType::i32:
		case VType::ui32:
		case VType::flo:
			if (!meta.use_gc)
				a.mov(resr, 0, 4, readData<uint32_t>(data, data_len, i));
			else
				values.push_back(ValueItem(new lgr(new uint32_t(readData<uint32_t>(data, data_len, i))), meta, no_copy));
			break;
		case VType::i64:
		case VType::ui64:
		case VType::doub:
		case VType::undefined_ptr:
			if (!meta.use_gc)
				a.mov(resr, 0, 8, readData<uint64_t>(data, data_len, i));
			else
				values.push_back(ValueItem(new lgr(new uint64_t(readData<uint64_t>(data, data_len, i))), meta, no_copy));
			break;
		case VType::raw_arr_i8:
		case VType::raw_arr_ui8: {
			optional_len = readLen(data, data_len, i);
			if (!meta.use_gc)
				values.push_back(ValueItem(readRawArray<int8_t>(data, data_len, i, optional_len), meta, no_copy));
			else
				values.push_back(ValueItem(new lgr(readRawArray<int8_t>(data, data_len, i, optional_len)), meta, no_copy));
			break;
		}
		case VType::raw_arr_i16:
		case VType::raw_arr_ui16: {
			optional_len = readLen(data, data_len, i);
			if (!meta.use_gc)
				values.push_back(ValueItem(readRawArray<int16_t>(data, data_len, i, optional_len), meta, no_copy));
			else
				values.push_back(ValueItem(new lgr(readRawArray<int16_t>(data, data_len, i, optional_len)), meta, no_copy));
			break;
		}
		case VType::raw_arr_i32:
		case VType::raw_arr_ui32:
		case VType::raw_arr_flo: {
			optional_len = readLen(data, data_len, i);
			if (!meta.use_gc)
				values.push_back(ValueItem(readRawArray<int32_t>(data, data_len, i, optional_len), meta, no_copy));
			else
				values.push_back(ValueItem(new lgr(readRawArray<int32_t>(data, data_len, i, optional_len)), meta, no_copy));
			break;
		}
		case VType::raw_arr_i64:
		case VType::raw_arr_ui64:
		case VType::raw_arr_doub: {
			optional_len = readLen(data, data_len, i);
			if (!meta.use_gc)
				values.push_back(ValueItem(readRawArray<int64_t>(data, data_len, i, optional_len), meta, no_copy));
			else
				values.push_back(ValueItem(new lgr(readRawArray<int64_t>(data, data_len, i, optional_len)), meta, no_copy));
			break;
		}
		case VType::uarr: {
			if (!meta.use_gc)
				values.push_back(ValueItem(readAnyUarr(data, data_len, i)));
			else
				values.push_back(ValueItem(new lgr(new list_array<ValueItem>(readAnyUarr(data, data_len, i))), meta, no_copy));
			break;
		}
		case VType::string: {
			if (!meta.use_gc)
				values.push_back(readString(data, data_len, i));
			else
				values.push_back(ValueItem(new lgr(new std::string(readString(data, data_len, i))), meta, no_copy));
			break;
		}
		case VType::faarr: {
			optional_len = readLen(data, data_len, i);
			if (!meta.use_gc) {
				meta.val_len = optional_len;
				values.push_back(ValueItem(readRawAny(data, data_len, i, optional_len), meta, no_copy));
			}
			else
				values.push_back(ValueItem(new lgr(readRawAny(data, data_len, i, optional_len)), meta, no_copy));
			break;
		}
		default:
			break;
		}
		if (needAlloc(meta)) {
			BuildCall b(a, 3);
			b.addArg(resr);
			b.addArg(values.back().val);
			b.addArg(meta.encoded);
			b.finalize(setValue);
			if (is_raw_array(meta.vtype)) 
				a.mov_valindex_meta_size({static_map, values}, value_index, optional_len);
		}
	}
	void dynamic_set_saarr() {
		ValueIndexPos value_index = readIndexPos(data, data_len, i);
		uint32_t len = readData<uint32_t>(data, data_len, i);
		BuildCall b(a, 0);
		b.lea_valindex({static_map, values}, value_index);
		b.finalize(valueDestructDyn);
		a.mov(resr, ValueMeta(VType::saarr, false, true, len).encoded);
		a.mov_valindex_meta({static_map, values},value_index, resr);

		a.stackIncrease(len * sizeof(ValueItem));
		b.addArg(stack_ptr);
		b.addArg(len*2);
		b.finalize(prepareStack);
		a.mov_valindex({static_map, values}, value_index, resr);
	}
	void dynamic_remove(){
		BuildCall b(a, 1);
		b.lea_valindex({static_map, values}, readIndexPos(data, data_len, i));
		b.finalize(universalRemove);
	}
	void dynamic_copy(){
		ValueIndexPos from = readIndexPos(data, data_len, i);
		ValueIndexPos to = readIndexPos(data, data_len, i);
		if (from != to) {
			BuildCall b(a, 0);
			b.lea_valindex({static_map, values}, to);
			b.finalize(universalRemove);
			b.mov_valindex({static_map, values}, from);
			b.mov_valindex_meta({static_map, values}, from);
			b.finalize(copyValue);
			a.mov_valindex({static_map, values}, to, resr);
			a.mov_valindex_meta({static_map, values}, resr, from);
			a.mov_valindex_meta({static_map, values}, to, resr);
		}
	}
	void dynamic_move(){
		ValueIndexPos from = readIndexPos(data, data_len, i);
		ValueIndexPos to = readIndexPos(data, data_len, i);
		if (from != to) {
			BuildCall b(a, 1);
			b.lea_valindex({static_map, values}, to);
			b.finalize(universalRemove);

			a.mov_valindex({static_map, values},resr, from);
			a.mov_valindex({static_map, values}, to, resr);
			a.mov_valindex_meta({static_map, values},resr, from);
			a.mov_valindex_meta({static_map, values}, to, resr);
			a.mov_valindex_meta({static_map, values},from, 0);
		}
	}
#pragma endregion
#pragma region dynamic math
	void dynamic_sum(){
		BuildCall b(a, 2);
		b.lea_valindex({static_map, values}, readIndexPos(data, data_len, i));
		b.lea_valindex({static_map, values}, readIndexPos(data, data_len, i));
		b.finalize(DynSum);
	}
	void dynamic_minus(){
		BuildCall b(a, 2);
		b.lea_valindex({static_map, values}, readIndexPos(data, data_len, i));
		b.lea_valindex({static_map, values}, readIndexPos(data, data_len, i));
		b.finalize(DynMinus);
	}
	void dynamic_div(){
		BuildCall b(a, 2);
		b.lea_valindex({static_map, values}, readIndexPos(data, data_len, i));
		b.lea_valindex({static_map, values}, readIndexPos(data, data_len, i));
		b.finalize(DynDiv);
	}
	void dynamic_mul(){
		BuildCall b(a, 2);
		b.lea_valindex({static_map, values}, readIndexPos(data, data_len, i));
		b.lea_valindex({static_map, values}, readIndexPos(data, data_len, i));
		b.finalize(DynMul);
	}
	void dynamic_rest(){
		BuildCall b(a, 2);
		b.lea_valindex({static_map, values}, readIndexPos(data, data_len, i));
		b.lea_valindex({static_map, values}, readIndexPos(data, data_len, i));
		b.finalize(DynRest);
	}
#pragma endregion
#pragma region dynamic bit
	void dynamic_bit_xor(){
		BuildCall b(a, 2);
		b.lea_valindex({static_map, values}, readIndexPos(data, data_len, i));
		b.lea_valindex({static_map, values}, readIndexPos(data, data_len, i));
		b.finalize(DynBitXor);
	}
	void dynamic_bit_or(){
		BuildCall b(a, 2);
		b.lea_valindex({static_map, values}, readIndexPos(data, data_len, i));
		b.lea_valindex({static_map, values}, readIndexPos(data, data_len, i));
		b.finalize(DynBitOr);
	}
	void dynamic_bit_and(){
		BuildCall b(a, 2);
		b.lea_valindex({static_map, values}, readIndexPos(data, data_len, i));
		b.lea_valindex({static_map, values}, readIndexPos(data, data_len, i));
		b.finalize(DynBitAnd);
	}
	void dynamic_bit_not(){
		BuildCall b(a, 1);
		b.lea_valindex({static_map, values}, readIndexPos(data, data_len, i));
		b.finalize(DynBitNot);
	}
	void dynamic_bit_shift_left(){
		BuildCall b(a, 2);
		b.lea_valindex({static_map, values}, readIndexPos(data, data_len, i));
		b.lea_valindex({static_map, values}, readIndexPos(data, data_len, i));
		b.finalize(DynBitShiftLeft);
	}
	void dynamic_bit_shift_right(){
		BuildCall b(a, 2);
		b.lea_valindex({static_map, values}, readIndexPos(data, data_len, i));
		b.lea_valindex({static_map, values}, readIndexPos(data, data_len, i));
		b.finalize(DynBitShiftRight);
	}

#pragma endregion
#pragma region dynamic logic
	void dynamic_log_not(){
		a.load_flag8h();
		a.xor_(resr_8h, RFLAGS::bit::zero & RFLAGS::bit::carry);
		a.store_flag8h();
	}
	void dynamic_compare(){
		a.push_flags();
		a.pop(argr0_16);

		BuildCall b(a, 3);
		b.addArg(argr0_16);
		b.lea_valindex({static_map, values}, readIndexPos(data, data_len, i));
		b.lea_valindex({static_map, values}, readIndexPos(data, data_len, i));
		b.finalize(compare);
		a.push(resr_16);
		a.pop_flags();
	}
	void dynamic_jump(){
		auto& label = resolve_label(readData<uint64_t>(data, data_len, i));
		switch (readData<JumpCondition>(data, data_len, i)) {
		default:
		case JumpCondition::no_condition: 
			a.jmp(label);
			break;
		case JumpCondition::is_zero: 
			a.jmp_zero(label);
			break;
		case JumpCondition::is_equal: 
			a.jmp_equal(label);
			break;
		case JumpCondition::is_not_equal: 
			a.jmp_not_equal(label);
			break;
		case JumpCondition::is_unsigned_more:
			a.jmp_unsigned_more(label);
			break;
		case JumpCondition::is_unsigned_lower:
			a.jmp_unsigned_lower(label);
			break;
		case JumpCondition::is_unsigned_more_or_eq:
			a.jmp_unsigned_more_or_eq(label);
			break;
		case JumpCondition::is_unsigned_lower_or_eq:
			a.jmp_unsigned_lower_or_eq(label);
			break;
		case JumpCondition::is_signed_more: 
			a.jmp_signed_more(label);
			break;
		case JumpCondition::is_signed_lower:
			a.jmp_signed_lower(label);
			break;
		case JumpCondition::is_signed_more_or_eq:
			a.jmp_signed_more_or_eq(label);
			break;
		case JumpCondition::is_signed_lower_or_eq:
			a.jmp_signed_lower_or_eq(label);
			break;
		}
	}
#pragma endregion
#pragma region dynamic call
	void dunamic_arg_set(){
		BuildCall b(a, 1);
		ValueIndexPos item = readIndexPos(data, data_len, i);
		b.lea_valindex({static_map, values}, item);
		b.finalize(AsArg);
		a.mov(arg_ptr, resr);
		a.mov_valindex_meta_size({static_map, values}, arg_len_32, item);
	}
	void dynamic_call_self(){
		CallFlags flags;
		flags.encoded = readData<uint8_t>(data, data_len, i);
		if (flags.async_mode)
			throw InvalidIL("Fail compile async 'call_self', for asynchonly call self use 'call' command");
		BuildCall b(a, 0);
		b.addArg(arg_ptr);
		b.addArg(arg_len_32);
		b.finalize(self_function);

		if (flags.use_result) {
			b.lea_valindex({static_map, values}, readIndexPos(data, data_len, i));
			b.addArg(resr);
			b.finalize(getValueItem);
		}
		else 
			inlineReleaseUnused(a, resr);
	}
	void dynamic_call_self_and_ret(){
		CallFlags flags;
		flags.encoded = readData<uint8_t>(data, data_len, i);
		if (flags.async_mode)
			throw InvalidIL("Fail compile async 'call_self', for asynchonly call self use 'call' command");
		BuildCall b(a, 3);
		b.addArg(this);
		b.addArg(arg_ptr);
		b.addArg(arg_len_32);
		b.finalize(self_function);
		do_jump_to_ret = true;
	}
#pragma endregion
	void dynamic_as(){
		BuildCall b(a, 2);
		b.lea_valindex({static_map, values}, readIndexPos(data, data_len, i));
		b.addArg(readData<VType>(data, data_len, i));
		b.finalize(asValue);
	}
	void dynamic_is(){
		a.mov_valindex_meta({static_map, values}, resr, readIndexPos(data, data_len, i));
		a.mov(argr0_8l, readData<VType>(data, data_len, i));
		a.cmp(resr_8l, argr0_8l);
	}
	void dynamic_store_bool(){
		BuildCall b(a, 1);
		b.lea_valindex({static_map, values}, readIndexPos(data, data_len, i));
		b.finalize(isTrueValue);
		a.load_flag8h();
		a.shift_left(resr_8l, RFLAGS::off_left::zero);
		a.or_(resr_8h, resr_8l);
		a.store_flag8h();
	}
	void dynamic_load_bool(){
		a.load_flag8h();
		a.and_(resr_8h, RFLAGS::bit::zero);
		a.store_flag8h();
		BuildCall b(a, 2);
		b.addArg(resr_8h);
		b.lea_valindex({static_map, values}, readIndexPos(data, data_len, i));
		b.finalize(setBoolValue);
	}

	void dynamic_throw(){
		bool in_memory = readData<bool>(data, data_len, i);
		if (in_memory) {
			BuildCall b(a, 2);
			b.lea_valindex({static_map, values}, readIndexPos(data, data_len, i));
			b.lea_valindex({static_map, values}, readIndexPos(data, data_len, i));
			b.finalize(throwDirEx);
		}
		else {
			values.push_back(readString(data, data_len, i));
			auto& ex_typ = values.back();
			values.push_back(readString(data, data_len, i));
			auto& ex_desc = values.back();
			BuildCall b(a, 2);
			b.addArg(ex_typ.val);
			b.addArg(ex_desc.val);
			b.finalize(throwEx);
		}
	}
	void dynamic_insert_native() {
		uint32_t len = readData<uint32_t>(data, data_len, i);
		auto tmp = extractRawArray<uint8_t>(data, data_len, i, len);
		a.insertNative(tmp, len);
	}
	void dynamic_set_structure_value(){
		BuildCall b(a, 0);
		if (readData<bool>(data, data_len, i)) {
			b.lea_valindex({static_map, values}, readIndexPos(data, data_len, i));//value name
			b.addArg(VType::string);
			b.finalize(getSpecificValue);
			b.addArg(readData<ClassAccess>(data, data_len, i));
			b.lea_valindex({static_map, values}, readIndexPos(data, data_len, i));//interface
			b.addArg(resr);
		}
		else {
			std::string fnn = readString(data, data_len, i);
			values.push_back(fnn);
			b.addArg((std::string*)values.back().val);
			b.addArg(readData<ClassAccess>(data, data_len, i));
			b.lea_valindex({static_map, values}, readIndexPos(data, data_len, i));//interface
		}
		b.lea_valindex({static_map, values}, readIndexPos(data, data_len, i));//value
		b.finalize((void(*)(ClassAccess, ValueItem&, const std::string&, ValueItem&))AttachA::Interface::setValue);
	}
	void dynamic_get_structure_value() {
		BuildCall b(a, 0);
		if (readData<bool>(data, data_len, i)) {
			b.lea_valindex({static_map, values}, readIndexPos(data, data_len, i));//value name
			b.addArg(VType::string);
			b.finalize(getSpecificValue);
			b.addArg(readData<ClassAccess>(data, data_len, i));
			b.lea_valindex({static_map, values}, readIndexPos(data, data_len, i));//interface
			b.addArg(resr);
		}
		else {
			std::string fnn = readString(data, data_len, i);
			values.push_back(fnn);
			b.addArg((std::string*)values.back().val);
			b.addArg(readData<ClassAccess>(data, data_len, i));
			b.lea_valindex({static_map, values}, readIndexPos(data, data_len, i));//interface
		}
		b.lea_valindex({static_map, values}, readIndexPos(data, data_len, i));//save to
		b.finalize(getInterfaceValue);
	}
	void dynamic_explicit_await(){
		BuildCall b(a, 1);
		b.lea_valindex({static_map, values}, readIndexPos(data, data_len, i));
		b.finalize(&ValueItem::getAsync);
	}
	void dynamic_generator_get(){
		BuildCall b(a, 3);
		b.lea_valindex({static_map, values}, readIndexPos(data, data_len, i));
		b.lea_valindex({static_map, values}, readIndexPos(data, data_len, i));
		b.addArg(readData<uint64_t>(data, data_len, i));
		b.finalize(&ValueItem::getGeneratorResult);
	}
	void dynamic_yield(){
		BuildCall b(a, 2);
		b.lea_valindex({static_map, values}, readIndexPos(data, data_len, i));
		b.finalize(Task::result);//generators actually not implemented yet, use task
	}

	void dynamic_ret() {
		BuildCall b(a, 1);
		b.lea_valindex({static_map, values}, readIndexPos(data, data_len, i));
		b.finalize(buildRes);
		do_jump_to_ret = true;
	}
	void dynamic_ret_take() {
		BuildCall b(a, 1);
		b.lea_valindex({static_map, values}, readIndexPos(data, data_len, i));
		b.finalize(buildResTake);
		do_jump_to_ret = true;
	}

	void dynamic_arr_op(){
		ValueIndexPos arr = readIndexPos(data, data_len, i);
		BuildCall b(a, 0);
		b.lea_valindex({static_map, values}, arr);
		b.finalize(AsArr);
		auto flags = readData<OpArrFlags>(data, data_len, i);
		switch (readData<OpcodeArray>(data, data_len, i)) {
		case OpcodeArray::set: {
			if (flags.by_val_mode) {
				b.lea_valindex({static_map, values}, readIndexPos(data, data_len, i));
				b.finalize(getSize);
				b.lea_valindex({static_map, values}, readIndexPos(data, data_len, i));
				b.lea_valindex({static_map, values}, arr);
				b.addArg(resr);
			}
			else {
				b.lea_valindex({static_map, values}, readIndexPos(data, data_len, i));
				b.lea_valindex({static_map, values}, arr);
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
				b.lea_valindex({static_map, values}, readIndexPos(data, data_len, i));
				b.finalize(getSize);
				b.lea_valindex({static_map, values},arr);
				b.addArg(resr);
				b.lea_valindex({static_map, values}, readIndexPos(data, data_len, i));
			}
			else {
				b.lea_valindex({static_map, values},arr);
				b.addArg(readData<uint64_t>(data, data_len, i));
				b.lea_valindex({static_map, values}, readIndexPos(data, data_len, i));
			}
			if (flags.move_mode)
				b.finalize((void (list_array<ValueItem>::*)(size_t, const ValueItem&)) & list_array<ValueItem>::insert);
			else
				b.finalize((void (list_array<ValueItem>::*)(size_t, ValueItem&&)) & list_array<ValueItem>::insert);
			break;
		}
		case OpcodeArray::push_end: {
			b.lea_valindex({static_map, values},arr);
			b.lea_valindex({static_map, values}, readIndexPos(data, data_len, i));
			if (flags.move_mode)
				b.finalize((void (list_array<ValueItem>::*)(ValueItem&&)) & list_array<ValueItem>::push_back);
			else
				b.finalize((void (list_array<ValueItem>::*)(const ValueItem&)) & list_array<ValueItem>::push_back);
			break;
		}
		case OpcodeArray::push_start: {
			b.lea_valindex({static_map, values},arr);
			b.lea_valindex({static_map, values}, readIndexPos(data, data_len, i));
			if (flags.move_mode)
				b.finalize((void (list_array<ValueItem>::*)(ValueItem&&)) & list_array<ValueItem>::push_front);
			else
				b.finalize((void (list_array<ValueItem>::*)(const ValueItem&)) & list_array<ValueItem>::push_front);
			break;
		}
		case OpcodeArray::insert_range: {
			ValueIndexPos arr1 = readIndexPos(data, data_len, i);
			b.lea_valindex({static_map, values},arr1);
			b.finalize(AsArr);
			if (flags.by_val_mode) {
				ValueIndexPos val1 = readIndexPos(data, data_len, i);
				ValueIndexPos val2 = readIndexPos(data, data_len, i);
				ValueIndexPos val3 = readIndexPos(data, data_len, i);
				b.lea_valindex({static_map, values},val3);
				b.finalize(getSize);
				a.push(resr);
				b.lea_valindex({static_map, values},val2);
				b.finalize(getSize);
				a.push(resr);
				b.lea_valindex({static_map, values},val1);
				b.finalize(getSize);
				b.mov_valindex({static_map, values},arr);
				b.addArg(resr);//1
				b.lea_valindex({static_map, values},arr1);
				a.pop(resr);
				b.addArg(resr);//2
				a.pop(resr);
				b.addArg(resr);//3
			}
			else {
				b.mov_valindex({static_map, values},arr);
				b.addArg(readData<uint64_t>(data, data_len, i));//1
				b.lea_valindex({static_map, values},arr1);
				b.addArg(readData<uint64_t>(data, data_len, i));//2
				b.addArg(readData<uint64_t>(data, data_len, i));//3
			}
			b.finalize((void(list_array<ValueItem>::*)(size_t, const list_array<ValueItem>&, size_t, size_t)) & list_array<ValueItem>::insert);
			break;
		}
		case OpcodeArray::get: {
			if (flags.by_val_mode) {
				b.lea_valindex({static_map, values}, readIndexPos(data, data_len, i));
				b.finalize(getSize);
				b.lea_valindex({static_map, values}, readIndexPos(data, data_len, i));
				b.lea_valindex({static_map, values},arr);
				b.addArg(resr);
			}
			else {
				b.lea_valindex({static_map, values}, readIndexPos(data, data_len, i));
				b.lea_valindex({static_map, values},arr);
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
				b.lea_valindex({static_map, values}, readIndexPos(data, data_len, i));
				b.finalize(getSize);
				b.addArg(resr);
				b.lea_valindex({static_map, values},arr);
				b.lea_valindex({static_map, values}, readIndexPos(data, data_len, i));
			}
			else {
				b.addArg(readData<uint64_t>(data, data_len, i));
				b.lea_valindex({static_map, values},arr);
				b.lea_valindex({static_map, values}, readIndexPos(data, data_len, i));
			}
			b.finalize(take);
			break;
		}
		case OpcodeArray::take_end: {
			b.lea_valindex({static_map, values},arr);
			b.lea_valindex({static_map, values}, readIndexPos(data, data_len, i));
			b.finalize(takeEnd);
			break;
		}
		case OpcodeArray::take_start: {
			b.lea_valindex({static_map, values},arr);
			b.lea_valindex({static_map, values}, readIndexPos(data, data_len, i));
			b.finalize(takeStart);
			break;
		}
		case OpcodeArray::get_range: {
			if (flags.by_val_mode) {
				ValueIndexPos set_to = readIndexPos(data, data_len, i);
				b.lea_valindex({static_map, values}, readIndexPos(data, data_len, i));
				b.finalize(getSize);
				a.push(resr);
				b.lea_valindex({static_map, values}, readIndexPos(data, data_len, i));
				b.finalize(getSize);
				a.pop(argr2);
				b.lea_valindex({static_map, values},arr);
				b.lea_valindex({static_map, values},set_to);
				b.addArg(argr2);
				b.addArg(resr);
			}
			else {
				b.lea_valindex({static_map, values},arr);
				b.lea_valindex({static_map, values}, readIndexPos(data, data_len, i));
				b.addArg(readData<uint64_t>(data, data_len, i));
				b.addArg(readData<uint64_t>(data, data_len, i));
			}
			b.finalize(getRange);
			break;
		}
		case OpcodeArray::take_range: {
			if (flags.by_val_mode) {
				ValueIndexPos set_to = readIndexPos(data, data_len, i);
				b.lea_valindex({static_map, values}, readIndexPos(data, data_len, i));
				b.finalize(getSize);
				a.push(resr);
				b.lea_valindex({static_map, values}, readIndexPos(data, data_len, i));
				b.finalize(getSize);
				a.pop(argr2);
				b.lea_valindex({static_map, values},arr);
				b.lea_valindex({static_map, values},set_to);
				b.addArg(argr2);
				b.addArg(resr);
			}
			else {
				b.lea_valindex({static_map, values},arr);
				b.lea_valindex({static_map, values}, readIndexPos(data, data_len, i));
				b.addArg(readData<uint64_t>(data, data_len, i));
				b.addArg(readData<uint64_t>(data, data_len, i));
			}
			b.finalize(takeRange);
			break;
		}
		case OpcodeArray::pop_end: {
			b.lea_valindex({static_map, values},arr);
			b.finalize(&list_array<ValueItem>::pop_back);
			break;
		}
		case OpcodeArray::pop_start: {
			b.lea_valindex({static_map, values},arr);
			b.finalize(&list_array<ValueItem>::pop_front);
			break;
		}
		case OpcodeArray::remove_item: {
			b.mov_valindex({static_map, values},arr);
			b.addArg(readData<uint64_t>(data, data_len, i));
			b.finalize((void(list_array<ValueItem>::*)(size_t pos)) & list_array<ValueItem>::remove);
			break;
		}
		case OpcodeArray::remove_range: {
			b.mov_valindex({static_map, values},arr);
			b.addArg(readData<uint64_t>(data, data_len, i));
			b.addArg(readData<uint64_t>(data, data_len, i));
			b.finalize((void(list_array<ValueItem>::*)(size_t, size_t)) & list_array<ValueItem>::remove);
			break;
		}
		case OpcodeArray::resize: {
			if (flags.by_val_mode) {
				b.lea_valindex({static_map, values}, readIndexPos(data, data_len, i));
				b.finalize(getSize);
				b.lea_valindex({static_map, values},arr);
				b.addArg(resr);
			}
			else {
				b.mov_valindex({static_map, values},arr);
				b.addArg(readData<uint64_t>(data, data_len, i));
			}
			b.finalize((void(list_array<ValueItem>::*)(size_t)) & list_array<ValueItem>::resize);
			break;
		}
		case OpcodeArray::resize_default: {
			if (flags.by_val_mode) {
				b.lea_valindex({static_map, values}, readIndexPos(data, data_len, i));
				b.finalize(getSize);
				b.lea_valindex({static_map, values},arr);
				b.addArg(resr);
			}
			else {
				b.mov_valindex({static_map, values},arr);
				b.addArg(readData<uint64_t>(data, data_len, i));
			}
			b.lea_valindex({static_map, values}, readIndexPos(data, data_len, i));
			b.finalize((void(list_array<ValueItem>::*)(size_t, const ValueItem&)) & list_array<ValueItem>::resize);
			break;
		}
		case OpcodeArray::reserve_push_end: {
			b.lea_valindex({static_map, values},arr);
			b.addArg(readData<uint64_t>(data, data_len, i));
			b.finalize(&list_array<ValueItem>::reserve_push_back);
			break;
		}
		case OpcodeArray::reserve_push_start: {
			b.lea_valindex({static_map, values},arr);
			if (flags.by_val_mode) {
				b.lea_valindex({static_map, values}, readIndexPos(data, data_len, i));
				b.finalize(getSize);
				b.addArg(resr);
			}
			else
				b.addArg(readData<uint64_t>(data, data_len, i));
			b.finalize(&list_array<ValueItem>::reserve_push_front);
			break;
		}
		case OpcodeArray::commit: {
			b.mov_valindex({static_map, values},arr);
			b.finalize(&list_array<ValueItem>::commit);
			break;
		}
		case OpcodeArray::decommit: {
			b.mov_valindex({static_map, values},arr);
			b.addArg(readData<uint64_t>(data, data_len, i));
			b.finalize(&list_array<ValueItem>::decommit);
			break;
		}
		case OpcodeArray::remove_reserved: {
			b.mov_valindex({static_map, values},arr);
			b.finalize(&list_array<ValueItem>::shrink_to_fit);
			break;
		}
		case OpcodeArray::size: {
			b.mov_valindex({static_map, values},arr);
			b.finalize(&list_array<ValueItem>::size);
			b.lea_valindex({static_map, values}, readIndexPos(data, data_len, i));
			b.addArg(resr);
			b.finalize(setSize);
			break;
		}
		default:
			throw InvalidIL("Invalid array operation");
		}
	}

	void dynamic_handle_catch(){
		size_t handle = scope_map.try_mapHandle(readData<uint64_t>(data, data_len, i));
		if (handle == -1)
			throw InvalidIL("Undefined handle");
		ScopeAction* scope_action = scope.setExceptionHandle(handle, _attacha_filter);
		//switch (readData<char>(data, data_len, i)) {
		//case 0:
		//	{
		//		struct _filter_data {
		//			typed_lgr<FuncEnvironment> catch
		//			std::string message;
		//		};
		//	}
		//	break;
		//
		//}
		
		//char* data = new char[10];


		//data[0] =0;
		//scope_action->filter_data = data;
		//scope_action->filter_data_len = 10;
		//scope_action->cleanup_filter_data = [](ScopeAction* data) {
		//	delete[] (char*)data->filter_data;
		//	data->filter_data = nullptr;
		//};
	}

	void dynamic_is_gc(){
		BuildCall b(a, 0);
		bool use_result = readData<bool>(data, data_len, i);
		b.mov_valindex({static_map, values}, readIndexPos(data, data_len, i));
		if(use_result){
			b.finalize(ValueItem_is_gc_proxy);
			b.lea_valindex({static_map, values}, readIndexPos(data, data_len, i));
			b.addArg(resr);
			b.finalize(getValueItem);
		}else{
			b.finalize(&ValueItem::is_gc);
			a.test(resr_8l, resr_8l);
		}
	}
	void dynamic_to_gc(){
		BuildCall b(a, 1);
		b.mov_valindex({static_map, values},readIndexPos(data, data_len, i));
		b.finalize(&ValueItem::make_gc);
	}
	void dynamic_from_gc(){
		BuildCall b(a, 1);
		b.mov_valindex({static_map, values},readIndexPos(data, data_len, i));
		b.finalize(&ValueItem::ungc);
	}
	void dynamic_localize_gc(){
		BuildCall b(a, 1);
		b.mov_valindex({static_map, values},readIndexPos(data, data_len, i));
		b.finalize(&ValueItem::localize_gc);
	}
	template<bool direction>
	void _dynamic_table_jump_bound_check(TableJumpFlags flags, TableJumpCheckFailAction action, uint32_t table_size, uint64_t index, const char* action_name) {
		static const std::string exception_name( "IndexError");
		static const std::string exception_description("Index out of range");

		switch (action) {
		case TableJumpCheckFailAction::jump_specified:
			if constexpr (direction) {
				a.cmp(resr, table_size);
				if (flags.is_signed) 
					a.jmp_signed_more_or_eq(resolve_label(index));
				else 
					a.jmp_unsigned_more_or_eq(resolve_label(index));
			}
			else {
				assert(flags.is_signed);
				a.cmp(resr, 0);
				a.jmp_signed_lower(resolve_label(index));
			}
			break;
		case TableJumpCheckFailAction::throw_exception: {
			asmjit::Label no_exception_label = a.newLabel();
			if constexpr (direction) {
				a.cmp(resr, table_size);
				if (flags.is_signed)
					a.jmp_signed_lower(no_exception_label);
				else
					a.jmp_unsigned_lower(no_exception_label);
			}
			else {
				assert(flags.is_signed);
				a.cmp(resr, 0);
				a.jmp_signed_more_or_eq(no_exception_label);
			}
			BuildCall b(a, 2);
			b.addArg(&exception_name);
			b.addArg(&exception_description);
			b.finalize(throwEx);
			a.label_bind(no_exception_label);
			break;
		}
		case TableJumpCheckFailAction::unchecked:
			return;
		default:
			throw InvalidIL(std::string("Invalid opcode, unsupported table jump check fail action for ") + action_name + ": " + enum_to_string(action));
		}
	}
	void dynamic_table_jump() {
		std::vector<asmjit::Label> table;
		TableJumpFlags flags = readData<TableJumpFlags>(data, data_len, i);

		uint64_t fail_too_large = 0;
		uint64_t fail_too_small = 0;
		if (flags.too_large == TableJumpCheckFailAction::jump_specified)
			fail_too_large = readData<uint64_t>(data, data_len, i);
		if(flags.too_small == TableJumpCheckFailAction::jump_specified && flags.is_signed)
			fail_too_small = readData<uint64_t>(data, data_len, i);

		ValueIndexPos value = readIndexPos(data, data_len, i);
		uint32_t table_size = readData<uint32_t>(data, data_len, i);
		table.reserve(table_size);
		for (uint32_t j = 0; j < table_size; j++)
			table.push_back(resolve_label(readData<uint64_t>(data, data_len, i)));
		
		auto table_label = a.add_table(table);
		BuildCall b(a, 1);
		b.lea_valindex({static_map, values},value);
		if(flags.is_signed)
			b.finalize(&ValueItem::operator int64_t);
		else
			b.finalize(&ValueItem::operator uint64_t);


		_dynamic_table_jump_bound_check<true>(flags, flags.too_large, table_size, fail_too_large, "too_large");
		if (flags.is_signed)
			_dynamic_table_jump_bound_check<false>(flags, flags.too_small, table_size, fail_too_small, "too_small");
		a.lea(argr0, table_label);
		a.mov(resr, argr0, resr, 3,0,8);
		a.jmp(resr);
	}
	void dynamic_xarray_slice(){
		ValueIndexPos result_index = readIndexPos(data, data_len, i);
		ValueIndexPos slice_index = readIndexPos(data, data_len, i);
		uint8_t slice_flags = readData<uint8_t>(data, data_len, i);

		uint8_t slice_type = slice_flags & 0x0F;
		uint8_t slice_offset_used = slice_flags >> 4;
		BuildCall b(a, 4);
		b.mov_valindex({static_map, values},result_index);
		b.mov_valindex({static_map, values},slice_index);
		switch(slice_type){
			case 1: {
				if(slice_offset_used & 1)
					b.addArg(readData<uint32_t>(data, data_len, i));
				else
					b.addArg(0);
				if(slice_offset_used & 2)
					b.addArg(readData<uint32_t>(data, data_len, i));
				else
					b.addArg(0);
				b.finalize(ValueItem_xmake_slice00);
				break;
			}
			case 2: {
				if(slice_offset_used & 1)
					b.addArg(readData<uint32_t>(data, data_len, i));
				else
					b.addArg(0);
				b.mov_valindex({static_map, values},readIndexPos(data, data_len, i));
				b.finalize(ValueItem_xmake_slice01);
				break;
			}
			case 3: {
				b.mov_valindex({static_map, values},readIndexPos(data, data_len, i));
				if(slice_offset_used & 2)
					b.addArg(readData<uint32_t>(data, data_len, i));
				else
					b.addArg(0);
				b.finalize(ValueItem_xmake_slice10);
				break;
			}
			case 4: {
				b.mov_valindex({static_map, values},readIndexPos(data, data_len, i));
				b.mov_valindex({static_map, values},readIndexPos(data, data_len, i));
				b.finalize(ValueItem_xmake_slice11);
				break;
			}
			default:
				throw InvalidIL("Invalid opcode, unsupported slice type");
		}
	}
#pragma endregion
#pragma region static opcodes

#pragma endregion

	void static_build(){
		switch (cmd.code) {
			case Opcode::set: {
				ValueIndexPos value_index = readIndexPos(data, data_len, i);
				ValueMeta meta = readData<ValueMeta>(data, data_len, i);
				meta.as_ref = false;
				{
					if (needAlloc(meta)) {
						BuildCall v(a, 3);
						v.lea_valindex({static_map, values},value_index);
						v.addArg(meta.encoded);
						v.addArg(cmd.is_gc_mode);
						v.finalize(preSetValue);
					}
					else
						a.mov_valindex_meta({static_map, values},value_index, meta.encoded);
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
					BuildCall b(a, 3);
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
					BuildCall b(a, 1);
					b.lea_valindex({static_map, values}, readIndexPos(data, data_len, i));
					b.finalize(universalRemove);
				}
				else
					a.mov_valindex_meta({static_map, values}, readIndexPos(data, data_len, i), 0);
				break;
			case Opcode::arg_set: {
				ValueIndexPos item = readIndexPos(data, data_len, i);
				a.mov_valindex({static_map, values}, arg_ptr, item);
				a.mov_valindex_meta_size({static_map, values}, arg_len_32, item);
				break;
			}
			case Opcode::throw_ex: {
				bool in_memory = readData<bool>(data, data_len, i);
				if (in_memory) {
					BuildCall b(a, 2);
					b.lea_valindex({static_map, values},readIndexPos(data, data_len, i));
					b.lea_valindex({static_map, values},readIndexPos(data, data_len, i));
					b.finalize(throwStatEx);
				}
				else {
					values.push_back(readString(data, data_len, i));
					auto& ex_typ = values.back();
					values.push_back(readString(data, data_len, i));
					auto& ex_desc = values.back();
					BuildCall b(a, 2);
					b.addArg(ex_typ.val);
					b.addArg(ex_desc.val);
					b.finalize(throwEx);
				}
				break;
			}
			case Opcode::arr_op: {
				ValueIndexPos arr = readIndexPos(data, data_len, i);
				BuildCall b(a, 0);
				auto flags = readData<OpArrFlags>(data, data_len, i);
				switch (readData<OpcodeArray>(data, data_len, i)) {
				case OpcodeArray::set: {
					VType type = readData<VType>(data, data_len, i);
					if (flags.by_val_mode) {
						b.lea_valindex({static_map, values}, readIndexPos(data, data_len, i));
						b.finalize(getSize);
						b.lea_valindex({static_map, values}, readIndexPos(data, data_len, i));
						b.lea_valindex({static_map, values},arr);
						b.addArg(resr);
					}
					else {
						b.lea_valindex({static_map, values}, readIndexPos(data, data_len, i));
						b.lea_valindex({static_map, values},arr);
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
						b.lea_valindex({static_map, values}, readIndexPos(data, data_len, i));
						b.finalize(getSize);
						b.lea_valindex({static_map, values},arr);
						b.addArg(resr);
						b.lea_valindex({static_map, values}, readIndexPos(data, data_len, i));
					}
					else {
						b.lea_valindex({static_map, values},arr);
						b.addArg(readData<uint64_t>(data, data_len, i));
						b.lea_valindex({static_map, values}, readIndexPos(data, data_len, i));
					}
					if (flags.move_mode)
						b.finalize((void (list_array<ValueItem>::*)(size_t, const ValueItem&)) & list_array<ValueItem>::insert);
					else
						b.finalize((void (list_array<ValueItem>::*)(size_t, ValueItem&&)) & list_array<ValueItem>::insert);
					break;
				}
				case OpcodeArray::push_end: {
					b.mov_valindex({static_map, values},arr);
					b.lea_valindex({static_map, values}, readIndexPos(data, data_len, i));
					if (flags.move_mode)
						b.finalize((void (list_array<ValueItem>::*)(ValueItem&&)) & list_array<ValueItem>::push_back);
					else
						b.finalize((void (list_array<ValueItem>::*)(const ValueItem&)) & list_array<ValueItem>::push_back);
					break;
				}
				case OpcodeArray::push_start: {
					b.mov_valindex({static_map, values},arr);
					b.lea_valindex({static_map, values}, readIndexPos(data, data_len, i));
					if (flags.move_mode)
						b.finalize((void (list_array<ValueItem>::*)(ValueItem&&)) & list_array<ValueItem>::push_front);
					else
						b.finalize((void (list_array<ValueItem>::*)(const ValueItem&)) & list_array<ValueItem>::push_front);
					break;
				}
				case OpcodeArray::insert_range: {
					ValueIndexPos arr1 = readIndexPos(data, data_len, i);
					b.lea_valindex({static_map, values},arr1);
					b.finalize(AsArr);
					if (flags.by_val_mode) {
						ValueIndexPos val1 = readIndexPos(data, data_len, i);
						ValueIndexPos val2 = readIndexPos(data, data_len, i);
						ValueIndexPos val3 = readIndexPos(data, data_len, i);

						b.lea_valindex({static_map, values},val3);
						b.finalize(getSize);
						a.push(resr);
						a.push(0);//align

						b.lea_valindex({static_map, values},val2);
						b.finalize(getSize);
						a.pop();//align
						a.push(resr);

						b.lea_valindex({static_map, values},val1);
						b.finalize(getSize);


						b.setArguments(5);
						b.mov_valindex({static_map, values},arr);
						b.addArg(resr);//1
						b.lea_valindex({static_map, values},arr1);
						a.pop(resr);
						b.addArg(resr);//2
						a.pop(resr);
						b.addArg(resr);//3
					}
					else {
						b.setArguments(5);
						b.mov_valindex({static_map, values},arr);
						b.addArg(readData<uint64_t>(data, data_len, i));//1
						b.lea_valindex({static_map, values},arr1);

						b.addArg(readData<uint64_t>(data, data_len, i));//2
						b.addArg(readData<uint64_t>(data, data_len, i));//3
					}

					b.finalize((void(list_array<ValueItem>::*)(size_t, const list_array<ValueItem>&, size_t, size_t)) & list_array<ValueItem>::insert);
					break;
				}
				case OpcodeArray::get: {
					VType type = readData<VType>(data, data_len, i);
					if (flags.by_val_mode) {
						b.lea_valindex({static_map, values}, readIndexPos(data, data_len, i));
						b.finalize(getSize);
						b.lea_valindex({static_map, values}, readIndexPos(data, data_len, i));
						b.lea_valindex({static_map, values},arr);
						b.addArg(resr);
					}
					else {
						b.lea_valindex({static_map, values}, readIndexPos(data, data_len, i));
						b.lea_valindex({static_map, values},arr);
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
						b.lea_valindex({static_map, values}, readIndexPos(data, data_len, i));
						b.finalize(getSize);
						b.addArg(resr);
						b.lea_valindex({static_map, values},arr);
						b.lea_valindex({static_map, values}, readIndexPos(data, data_len, i));
					}
					else {
						b.addArg(readData<uint64_t>(data, data_len, i));
						b.lea_valindex({static_map, values},arr);
						b.lea_valindex({static_map, values}, readIndexPos(data, data_len, i));
					}
					b.finalize(take);
					break;
				}
				case OpcodeArray::take_end: {
					b.lea_valindex({static_map, values},arr);
					b.lea_valindex({static_map, values}, readIndexPos(data, data_len, i));
					b.finalize(takeEnd);
					break;
				}
				case OpcodeArray::take_start: {
					b.lea_valindex({static_map, values},arr);
					b.lea_valindex({static_map, values}, readIndexPos(data, data_len, i));
					b.finalize(takeStart);
					break;
				}
				case OpcodeArray::get_range: {
					if (flags.by_val_mode) {
						ValueIndexPos set_to = readIndexPos(data, data_len, i);
						b.lea_valindex({static_map, values}, readIndexPos(data, data_len, i));
						b.finalize(getSize);
						a.push(resr);
						b.lea_valindex({static_map, values}, readIndexPos(data, data_len, i));
						b.finalize(getSize);

						a.pop(argr2);

						b.lea_valindex({static_map, values},arr);
						b.lea_valindex({static_map, values}, set_to);
						b.addArg(argr2);
						b.addArg(resr);
					}
					else {
						b.lea_valindex({static_map, values},arr);
						b.lea_valindex({static_map, values}, readIndexPos(data, data_len, i));
						b.addArg(readData<uint64_t>(data, data_len, i));
						b.addArg(readData<uint64_t>(data, data_len, i));
					}
					b.finalize(getRange);
					break;
				}
				case OpcodeArray::take_range: {
					if (flags.by_val_mode) {
						ValueIndexPos set_to = readIndexPos(data, data_len, i);
						b.lea_valindex({static_map, values}, readIndexPos(data, data_len, i));
						b.finalize(getSize);
						a.push(resr);
						b.lea_valindex({static_map, values}, readIndexPos(data, data_len, i));
						b.finalize(getSize);

						a.pop(argr2);

						b.lea_valindex({static_map, values},arr);
						b.lea_valindex({static_map, values},set_to);
						b.addArg(argr2);
						b.addArg(resr);
					}
					else {
						b.lea_valindex({static_map, values},arr);
						b.lea_valindex({static_map, values}, readIndexPos(data, data_len, i));
						b.addArg(readData<uint64_t>(data, data_len, i));
						b.addArg(readData<uint64_t>(data, data_len, i));
					}
					b.finalize(takeRange);
					break;
				}
				case OpcodeArray::pop_end: {
					b.lea_valindex({static_map, values},arr);
					b.finalize(&list_array<ValueItem>::pop_back);
					break;
				}
				case OpcodeArray::pop_start: {
					b.lea_valindex({static_map, values},arr);
					b.finalize(&list_array<ValueItem>::pop_front);
					break;
				}
				case OpcodeArray::remove_item: {
					b.mov_valindex({static_map, values},arr);
					b.addArg(readData<uint64_t>(data, data_len, i));
					b.finalize((void(list_array<ValueItem>::*)(size_t pos)) & list_array<ValueItem>::remove);
					break;
				}
				case OpcodeArray::remove_range: {
					b.mov_valindex({static_map, values},arr);
					b.addArg(readData<uint64_t>(data, data_len, i));
					b.addArg(readData<uint64_t>(data, data_len, i));
					b.finalize((void(list_array<ValueItem>::*)(size_t, size_t)) & list_array<ValueItem>::remove);
					break;
				}

				case OpcodeArray::resize: {
					if (flags.by_val_mode) {
						b.lea_valindex({static_map, values}, readIndexPos(data, data_len, i));
						b.finalize(getSize);
						b.lea_valindex({static_map, values},arr);
						b.addArg(resr);
					}
					else {
						b.mov_valindex({static_map, values},arr);
						b.addArg(readData<uint64_t>(data, data_len, i));
					}
					b.finalize((void(list_array<ValueItem>::*)(size_t)) & list_array<ValueItem>::resize);
					break;
				}
				case OpcodeArray::resize_default: {
					if (flags.by_val_mode) {
						b.lea_valindex({static_map, values}, readIndexPos(data, data_len, i));
						b.finalize(getSize);
						b.lea_valindex({static_map, values},arr);
						b.addArg(resr);
					}
					else {
						b.mov_valindex({static_map, values},arr);
						b.addArg(readData<uint64_t>(data, data_len, i));
					}
					b.lea_valindex({static_map, values}, readIndexPos(data, data_len, i));
					b.finalize((void(list_array<ValueItem>::*)(size_t, const ValueItem&)) & list_array<ValueItem>::resize);
					break;
				}
				case OpcodeArray::reserve_push_end: {
					b.lea_valindex({static_map, values},arr);
					b.addArg(readData<uint64_t>(data, data_len, i));
					b.finalize(&list_array<ValueItem>::reserve_push_back);
					break;
				}
				case OpcodeArray::reserve_push_start: {
					b.lea_valindex({static_map, values},arr);
					if (flags.by_val_mode) {
						b.lea_valindex({static_map, values}, readIndexPos(data, data_len, i));
						b.finalize(getSize);
						b.addArg(resr);
					}
					else
						b.addArg(readData<uint64_t>(data, data_len, i));
					b.finalize(&list_array<ValueItem>::reserve_push_front);
					break;
				}
				case OpcodeArray::commit: {
					b.mov_valindex({static_map, values},arr);
					b.finalize(&list_array<ValueItem>::commit);
					break;
				}
				case OpcodeArray::decommit: {
					b.mov_valindex({static_map, values},arr);
					b.addArg(readData<uint64_t>(data, data_len, i));
					b.finalize(&list_array<ValueItem>::decommit);
					break;
				}
				case OpcodeArray::remove_reserved: {
					b.mov_valindex({static_map, values},arr);
					b.finalize(&list_array<ValueItem>::shrink_to_fit);
					break;
				}
				case OpcodeArray::size: {
					b.mov_valindex({static_map, values},arr);
					b.finalize(&list_array<ValueItem>::size);
					b.lea_valindex({static_map, values}, readIndexPos(data, data_len, i));
					b.addArg(resr);
					b.finalize(setSize);
					break;
				}
				default:
					throw InvalidIL("Invalid array operation");
				}
				break;
			}
			default:
				throw InvalidIL("Invalid opcode");
		}
	}
	

	void dynamic_build(){
		switch (cmd.code) {
			case Opcode::noting: a.noting(); break;
			case Opcode::set: dynamic_set(); break;
			case Opcode::set_saarr: dynamic_set_saarr(); break;
			case Opcode::remove: dynamic_remove(); break;
			case Opcode::sum: dynamic_sum(); break;
			case Opcode::minus: dynamic_minus(); break;
			case Opcode::div: dynamic_div(); break;
			case Opcode::rest: dynamic_rest(); break;
			case Opcode::mul: dynamic_mul(); break;
			case Opcode::bit_xor: dynamic_bit_xor(); break;
			case Opcode::bit_or: dynamic_bit_or(); break;
			case Opcode::bit_and: dynamic_bit_and(); break;
			case Opcode::bit_not: dynamic_bit_not(); break;
			case Opcode::bit_shift_left: dynamic_bit_shift_left(); break;
			case Opcode::bit_shift_right: dynamic_bit_shift_right(); break;
			case Opcode::log_not: dynamic_log_not(); break;
			case Opcode::compare: dynamic_compare(); break;
			case Opcode::jump: dynamic_jump(); break;
			case Opcode::arg_set: dunamic_arg_set(); break;
			case Opcode::call: compilerFabric_call<true>(a, data, data_len, i, values, static_map); break;
			case Opcode::call_self: dynamic_call_self(); break;
			case Opcode::call_local: compilerFabric_call_local<true>(a, data, data_len, i, build_func, values, static_map);
			case Opcode::call_and_ret: {
				compilerFabric_call<false, false>(a, data, data_len, i, values, static_map);
				do_jump_to_ret = true;
				break;
			}
			case Opcode::call_self_and_ret: dynamic_call_self_and_ret(); break;
			case Opcode::call_local_and_ret: {
				compilerFabric_call_local<false, false>(a, data, data_len, i, build_func, values, static_map);
				do_jump_to_ret = true;
				break;
			}
			case Opcode::ret: dynamic_ret(); break;
			case Opcode::ret_take: dynamic_ret_take(); break;
			case Opcode::ret_noting: {
				a.xor_(resr, resr);
				do_jump_to_ret = true;
				break;
			}
			case Opcode::copy: dynamic_copy(); break;
			case Opcode::move: dynamic_move(); break;
			case Opcode::arr_op: dynamic_arr_op(); break;
			case Opcode::debug_break:
				if (!in_debug)
					break;
				[[fallthrough]];
			case Opcode::force_debug_break:
				a.int3();
				break;
			case Opcode::throw_ex: dynamic_throw();
			case Opcode::as: dynamic_as(); break;
			case Opcode::is: dynamic_is(); break;
			case Opcode::store_bool: dynamic_store_bool(); break;
			case Opcode::load_bool: dynamic_load_bool(); break;
			case Opcode::inline_native: dynamic_insert_native(); break;
			case Opcode::call_value_function: compilerFabric_value_call<true, true>(a, data, data_len, i, values, static_map); break;
			case Opcode::call_value_function_id: compilerFabric_value_call_id<true, true>(a, data, data_len, i, values, static_map); break;
			case Opcode::call_value_function_and_ret: {
				compilerFabric_value_call<false, false>(a, data, data_len, i, values, static_map);
				do_jump_to_ret = true;
				break;
			}
			case Opcode::call_value_function_id_and_ret: {
				compilerFabric_value_call_id<false, false>(a, data, data_len, i, values, static_map);
				do_jump_to_ret = true;
				break;
			}
			case Opcode::static_call_value_function:  compilerFabric_static_value_call<true, true>(a, data, data_len, i, values, static_map); break;
			case Opcode::static_call_value_function_id:  compilerFabric_static_value_call_id<true, true>(a, data, data_len, i, values, static_map); break;
			case Opcode::static_call_value_function_and_ret: {
				compilerFabric_static_value_call<false, false>(a, data, data_len, i, values, static_map);
				do_jump_to_ret = true;
				break;
			}
			case Opcode::static_call_value_function_id_and_ret: {
				compilerFabric_static_value_call_id<false, false>(a, data, data_len, i, values, static_map);
				do_jump_to_ret = true;
				break;
			}
			case Opcode::set_structure_value: dynamic_set_structure_value(); break;
			case Opcode::get_structure_value: dynamic_get_structure_value(); break;
			case Opcode::explicit_await: dynamic_explicit_await(); break;
			case Opcode::generator_get: dynamic_generator_get(); break;
			case Opcode::yield: dynamic_yield(); break;
			case Opcode::handle_begin:
				scope_map.mapHandle(readData<uint64_t>(data, data_len, i));
				break;
			case Opcode::handle_catch:dynamic_handle_catch(); break;
			
			case Opcode::handle_finally:
			case Opcode::handle_convert:
			case Opcode::value_hold:
			case Opcode::value_unhold:
				throw NotImplementedException();

			
			
			case Opcode::is_gc: dynamic_is_gc(); break;
			case Opcode::to_gc: dynamic_to_gc(); break;
			case Opcode::localize_gc: dynamic_localize_gc(); break;
			case Opcode::from_gc: dynamic_from_gc(); break;
			case Opcode::table_jump: dynamic_table_jump(); break;
			case Opcode::xarray_slice: dynamic_xarray_slice(); break;
			case Opcode::store_constant: store_constant(); break;
			default:
				throw InvalidIL("Invalid opcode");
		}
	}

	void build(){
		for (; i < data_len; ) {
			if (do_jump_to_ret)
				a.jmp(prolog);
			auto label = label_bind_map.find(i - skip_count);
			if (label != label_bind_map.end()) 
				a.label_bind(label->second);

			do_jump_to_ret = false;
			cmd = Command(data[i++]);
			!cmd.static_mode ? dynamic_build() : static_build();
		}
	}
};

class RuntimeCompileException : public asmjit::ErrorHandler {
public:
	void handleError(Error err, const char* message, asmjit::BaseEmitter* origin) override {
		throw CompileTimeException(asmjit::DebugUtils::errorAsString(err) +  std::string(message));
	}
};

void FuncEnvironment::RuntimeCompile() {
	if (curr_func != nullptr)
		FrameResult::deinit(frame, curr_func, jrt);
	values.clear();
	RuntimeCompileException error_handler;
	CodeHolder code;
	code.setErrorHandler(&error_handler);
	code.init(jrt.environment());
	CASM a(code);
	Label self_function = a.newLabel();
	a.label_bind(self_function);

	size_t to_be_skiped = 0;
	auto&& [jump_list, function_locals, flags, used_static_values, used_enviro_vals, used_arguments, constants_values] = decodeFunctionHeader(a, cross_code, cross_code.size(), to_be_skiped);
	uint32_t to_alloc_statics = flags.used_static ? uint32_t(used_static_values) + 1 : 0;
	if(flags.run_time_computable){
		//local_funcs already contains all local functions
		values.reserve_push_front(to_alloc_statics);
		for(uint32_t i = 0; i < to_alloc_statics; i++)
			values.push_front(nullptr);
	}else{
		local_funcs = std::move(function_locals);
		values.resize(to_alloc_statics, nullptr);
	}
	constants_values += to_alloc_statics;
	values.reserve_push_back(std::clamp<uint32_t>(constants_values, 0, UINT32_MAX));
	uint32_t max_values = flags.used_enviro_vals ? uint32_t(used_enviro_vals) + 1 : 0;


	//OS dependent prolog begin
	BuildProlog bprolog(a);
	bprolog.pushReg(frame_ptr);
	if(max_values)
		bprolog.pushReg(enviro_ptr);
	bprolog.pushReg(arg_ptr);
	bprolog.pushReg(arg_len);
	bprolog.alignPush();
	bprolog.stackAlloc(0x20);//c++ abi
	bprolog.setFrame();
	bprolog.end_prolog();
	//OS dependent prolog end 

	//Init enviropment
	a.mov(arg_ptr, argr0);
	a.mov(arg_len_32, argr1_32);
	a.mov(enviro_ptr, stack_ptr);
	if(max_values)
		a.stackIncrease(CASM::alignStackBytes(max_values<<1));
	a.stackAlign();

	if(flags.used_arguments){
		Label correct = a.newLabel();
		a.cmp(arg_len_32, uint32_t(used_arguments));
		a.jmp_unsigned_more_or_eq(correct);
		BuildCall b(a, 2);
		b.addArg(arg_len_32);
		b.addArg(uint32_t(used_arguments));
		b.finalize((void(*)(uint32_t,uint32_t))AttachA::arguments_range);
		a.label_bind(correct);
	}

	//Clean enviropment
	ScopeManager scope(bprolog);
	ScopeManagerMap scope_map(scope);
	{
		std::vector<ValueItem*> empty_static_map;
		ValueIndexPos ipos;
		ipos.pos = ValuePos::in_enviro;
		for(size_t i=0;i<max_values;i++){
			ipos.index = i;
			a.mov_valindex({empty_static_map, values} ,ipos, 0);
			a.mov_valindex_meta({empty_static_map, values} ,ipos, 0);
			scope.createValueLifetimeScope(valueDestructDyn, i<<1);
		}
	}
	Label prolog = a.newLabel();
	CompilerFabric fabric(a,scope,scope_map,prolog, self_function, cross_code, cross_code.size(), to_be_skiped, jump_list, values, flags.in_debug, this);
	fabric.build();

	a.label_bind(prolog);
	a.push(resr);
	a.push(0);
	{
		BuildCall b(a, 1);
		ValueIndexPos ipos;
		ipos.pos = ValuePos::in_enviro;
		for(size_t i=0;i<max_values;i++){
			ipos.index = i;
			b.lea_valindex({fabric.static_map, values},ipos);
			b.finalize(valueDestructDyn);
			scope.endValueLifetime(i);
		}
	}
	a.pop();
	a.pop(resr);




	
	auto& tmp = bprolog.finalize_epilog();
	tmp.use_handle = true;
	tmp.exHandleOff = a.offset() <= UINT32_MAX ? (uint32_t)a.offset() : throw InvalidFunction("Too big function");
	a.jmp((uint64_t)__attacha_handle);
	a.finalize();
	curr_func = (Enviropment)tmp.init(frame, a.code(), jrt, try_resolve_frame(this));
}
#pragma endregion
#pragma region FuncEnvironment
ValueItem* FuncEnvironment::async_call(typed_lgr<FuncEnvironment> f, ValueItem* args, uint32_t args_len) {
	ValueItem* res = new ValueItem();
	res->meta = ValueMeta(VType::async_res, false, false).encoded;
	res->val = new typed_lgr(new Task(f, ValueItem(args, ValueMeta(VType::saarr, false, true, args_len), no_copy)));
	Task::start(*(typed_lgr<Task>*)res->val);
	return res;
}

ValueItem* FuncEnvironment::syncWrapper(ValueItem* args, uint32_t arguments_size) {
	if(_type == FuncEnvironment::FuncType::force_unloaded)
		throw InvalidFunction("Function is force unloaded");
	if (force_unload)
		throw InvalidFunction("Function is force unloaded");
	if (need_compile)
		funcComp();
	switch (_type) {
	case FuncEnvironment::FuncType::native:
		return NativeProxy_DynamicToStatic(args, arguments_size);
	case FuncEnvironment::FuncType::own:{
		ValueItem* res;
		try {
			res = ((Enviropment)curr_func)(args, arguments_size);
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
	case FuncEnvironment::FuncType::python:
	case FuncEnvironment::FuncType::csharp:
	case FuncEnvironment::FuncType::java:
	default:
		throw NotImplementedException();
	}
}

void NativeProxy_DynamicToStatic_addValue(DynamicCall::FunctionCall& call, ValueMeta meta, void*& arg) {
	if (!meta.allow_edit && meta.vtype == VType::string) {
		switch (meta.vtype) {
		case VType::noting:
			call.AddValueArgument((void*)0);
			break;
		case VType::i8:
			call.AddValueArgument(*(int8_t*)&arg);
			break;
		case VType::i16:
			call.AddValueArgument(*(int16_t*)&arg);
			break;
		case VType::i32:
			call.AddValueArgument(*(int32_t*)&arg);
			break;
		case VType::i64:
			call.AddValueArgument(*(int64_t*)&arg);
			break;
		case VType::ui8:
			call.AddValueArgument(*(uint8_t*)&arg);
			break;
		case VType::ui16:
			call.AddValueArgument(*(int16_t*)&arg);
			break;
		case VType::ui32:
			call.AddValueArgument(*(uint32_t*)&arg);
			break;
		case VType::ui64:
			call.AddValueArgument(*(uint64_t*)&arg);
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
			call.AddValueArgument(*(int8_t*)&arg);
			break;
		case VType::i16:
			call.AddValueArgument(*(int16_t*)&arg);
			break;
		case VType::i32:
			call.AddValueArgument(*(int32_t*)&arg);
			break;
		case VType::i64:
			call.AddValueArgument(*(int64_t*)&arg);
			break;
		case VType::ui8:
			call.AddValueArgument(*(uint8_t*)&arg);
			break;
		case VType::ui16:
			call.AddValueArgument(*(int16_t*)&arg);
			break;
		case VType::ui32:
			call.AddValueArgument(*(uint32_t*)&arg);
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
ValueItem* NativeProxy_DynamicToStatic(DynamicCall::FunctionCall& call, DynamicCall::FunctionTemplate& nat_templ, ValueItem* arguments, uint32_t arguments_size) {
	using namespace DynamicCall;
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
						call.AddValueArgument(*(int8_t*)&arg);
						break;
					case VType::i16:
						call.AddValueArgument(*(int16_t*)&arg);
						break;
					case VType::i32:
						call.AddValueArgument(*(int32_t*)&arg);
						break;
					case VType::i64:
						call.AddValueArgument(*(int64_t*)&arg);
						break;
					case VType::ui8:
						call.AddValueArgument(*(uint8_t*)&arg);
						break;
					case VType::ui16:
						call.AddValueArgument(*(int16_t*)&arg);
						break;
					case VType::ui32:
						call.AddValueArgument(*(uint32_t*)&arg);
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
		return new ValueItem(res, VType::undefined_ptr);
	switch (nat_templ.result.vtype) {
	case DynamicCall::FunctionTemplate::ValueT::ValueType::integer:
		switch (nat_templ.result.vsize) {
		case 1:
			return new ValueItem(res, VType::ui8);
		case 2:
			return new ValueItem(res, VType::ui16);
		case 4:
			return new ValueItem(res, VType::ui32);
		case 8:
			return new ValueItem(res, VType::ui64);
		default:
			throw InvalidCast("Invalid type for convert");
		}
	case DynamicCall::FunctionTemplate::ValueT::ValueType::signed_integer:
		switch (nat_templ.result.vsize) {
		case 1:
			return new ValueItem(res, VType::i8);
		case 2:
			return new ValueItem(res, VType::i16);
		case 4:
			return new ValueItem(res, VType::i32);
		case 8:
			return new ValueItem(res, VType::i64);
		default:
			throw InvalidCast("Invalid type for convert");
		}
	case DynamicCall::FunctionTemplate::ValueT::ValueType::floating:
		switch (nat_templ.result.vsize) {
		case 1:
			return new ValueItem(res, VType::ui8);
		case 2:
			return new ValueItem(res, VType::ui16);
		case 4:
			return new ValueItem(res, VType::flo);
		case 8:
			return new ValueItem(res, VType::doub);
		default:
			throw InvalidCast("Invalid type for convert");
		}
	case DynamicCall::FunctionTemplate::ValueT::ValueType::pointer:
		return new ValueItem(res, VType::undefined_ptr);
	case DynamicCall::FunctionTemplate::ValueT::ValueType::_class:
	default:
		throw NotImplementedException();
	}
}
ValueItem* FuncEnvironment::NativeProxy_DynamicToStatic(ValueItem* arguments, uint32_t arguments_size) {
	DynamicCall::FunctionCall call((DynamicCall::PROC)curr_func, nat_templ, true);
	return ::NativeProxy_DynamicToStatic(call, nat_templ, arguments, arguments_size);
}

std::string FuncEnvironment::to_string(){
	if(!curr_func)
		return "fn(unknown)@0";
	for(auto& it : enviropments)
		if(it.second.getPtr() == this)
			return "fn(" + it.first + ")@" + string_help::hexstr((ptrdiff_t)curr_func);
	return "fn(" + FrameResult::JitResolveFrame(curr_func,true).fn_name + ")@" + string_help::hexstr((ptrdiff_t)curr_func);
}
const std::vector<uint8_t>& FuncEnvironment::get_cross_code(){
	return cross_code;
}

void FuncEnvironment::fastHotPath(const std::string& func_name, const std::vector<uint8_t>& new_cross_code) {
	auto& tmp = enviropments[func_name];
	if (tmp) {
		if (!tmp->can_be_unloaded)
			throw HotPathException("Path fail cause this symbol is cannon't be unloaded for path");
		tmp->force_unload = true;
	}
	tmp = new FuncEnvironment(new_cross_code);
	
}
void FuncEnvironment::fastHotPath(const std::string& func_name, typed_lgr<FuncEnvironment>& new_enviro) {
	auto& tmp = enviropments[func_name];
	if (tmp) {
		if (!tmp->can_be_unloaded)
			throw HotPathException("Path fail cause this symbol is cannon't be unloaded for path");
		tmp->force_unload = true;
	}
	tmp = new_enviro;
}
typed_lgr<FuncEnvironment> FuncEnvironment::enviropment(const std::string& func_name) {
	return enviropments[func_name];
}
ValueItem* FuncEnvironment::callFunc(const std::string& func_name, ValueItem* arguments, uint32_t arguments_size, bool run_async) {
	if (enviropments.contains(func_name)) {
		if (run_async)
			return async_call(enviropments[func_name], arguments, arguments_size);
		else
			return enviropments[func_name]->syncWrapper(arguments, arguments_size);
	}
	throw NotImplementedException();
}
void FuncEnvironment::AddNative(Enviropment function, const std::string& symbol_name, bool can_be_unloaded, bool is_cheap) {
	if (enviropments.contains(symbol_name))
		throw SymbolException("Fail alocate symbol: \"" + symbol_name + "\" cause them already exists");
	enviropments[symbol_name] = typed_lgr(new FuncEnvironment(function, can_be_unloaded, is_cheap));
}

void FuncEnvironment::AddNative(DynamicCall::PROC proc, const DynamicCall::FunctionTemplate& templ, const std::string& symbol_name, bool can_be_unloaded, bool is_cheap) {
	if (enviropments.contains(symbol_name))
		throw SymbolException("Fail alocate symbol: \"" + symbol_name + "\" cause them already exists");
	enviropments[symbol_name] = typed_lgr(new FuncEnvironment(proc, templ, can_be_unloaded, is_cheap));
}

bool FuncEnvironment::Exists(const std::string& symbol_name) {
	return enviropments.contains(symbol_name);
}
void FuncEnvironment::Load(typed_lgr<FuncEnvironment> fn, const std::string& symbol_name) {
	if (enviropments.contains(symbol_name))
		throw SymbolException("Fail load symbol: \"" + symbol_name + "\" cause them already exists");
	enviropments[symbol_name] = fn;
}
void FuncEnvironment::Load(const std::vector<uint8_t>& func_templ, const std::string& symbol_name) {
	if (enviropments.contains(symbol_name))
		throw SymbolException("Fail load symbol: \"" + symbol_name + "\" cause them already exists");
	if (func_templ.size() < 2)
		throw SymbolException("Fail load symbol: \"" + symbol_name + "\" cause them emplty");
	uint16_t max_vals = func_templ[1];
	max_vals <<= 8;
	max_vals |= func_templ[0];
	enviropments[symbol_name] = new FuncEnvironment(func_templ);
}
void FuncEnvironment::Unload(const std::string& func_name) {
	art::lock_guard guard(enviropments_lock);
	if (enviropments.contains(func_name))
		if (enviropments[func_name]->can_be_unloaded)
			enviropments.erase(func_name);
		else
			throw SymbolException("Fail unload symbol: \"" + func_name + "\" cause them cannont be unloaded");
}
void FuncEnvironment::ForceUnload(const std::string& func_name) {
	if (enviropments.contains(func_name)) {
		enviropments[func_name]->force_unload = true;
		enviropments.erase(func_name);
	}
}
#pragma endregion




extern "C" void callFunction(const char* symbol_name, bool run_async) {
	ValueItem* res = FuncEnvironment::callFunc(symbol_name, nullptr, 0, run_async);
	if (res)
		delete res;
}



