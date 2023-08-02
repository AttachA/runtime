// Copyright Danyil Melnytskyi 2022-2023
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include "FuncEnvironment.hpp"
#include "../../run_time.hpp"
#include "../attacha_abi.hpp"
#include "CASM.hpp"
#include "../util/tools.hpp"
#include "../AttachA_CXX.hpp"
#include "../tasks.hpp"
#include "../attacha_abi.hpp"
#include "../threading.hpp"
#include "../../../configuration/agreement/symbols.hpp"
#include "dynamic_call_proxy.hpp"
#include "exception.hpp"
namespace art{
	using namespace reader;
	std::shared_ptr<asmjit::JitRuntime> art = std::make_shared<asmjit::JitRuntime>();
	std::unordered_map<std::string, typed_lgr<FuncEnvironment>> environments;
	TaskMutex environments_lock;


	std::string try_resolve_frame(FuncHandle::inner_handle* env){
		for(auto& it : environments){
			if(it.second){
				if(it.second->inner_handle() == env)
					return it.first.data();
			}
		}
		void* fn_ptr;
		switch (env->_type) {
		case FuncHandle::inner_handle::FuncType::own:
			fn_ptr = env->env;
			break;
		case FuncHandle::inner_handle::FuncType::native_c:
			fn_ptr = env->frame;
			break;
		case FuncHandle::inner_handle::FuncType::static_native_c:
			fn_ptr = (void*)env->values[0];
			break;
		default:
			fn_ptr = nullptr;
		}
		if(fn_ptr != nullptr)
			return "fn(" + FrameResult::JitResolveFrame(fn_ptr,true).fn_name + ")@" + string_help::hexstr((ptrdiff_t)fn_ptr);
		else
			return "unresolved_attach_a_symbol";
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
	std::string* _compilerFabric_get_constant_string(ValueIndexPos& value_index, list_array<ValueItem>& values, std::vector<ValueItem*> static_map){
		ValueItem& value = values[value_index.index + static_map.size()];
		if(value.meta.vtype == VType::string)
			 return (std::string*)value.getSourcePtr();
		else{
			values.push_back((std::string)value);
			return (std::string*)values.back().getSourcePtr();
		}
	}
	void _compilerFabric_call_fun_string(CASM& a, std::string& fnn, bool is_async, list_array<typed_lgr<FuncEnvironment>>& used_environs){
		typed_lgr<FuncEnvironment> fn = FuncEnvironment::environment(fnn);
		used_environs.push_back(fn);
		if (is_async) {
			BuildCall b(a, 4);
			b.addArg(fnn.c_str());
			b.addArg(arg_ptr);
			b.addArg(arg_len_32);
			b.addArg(true);
			b.finalize(&FuncEnvironment::callFunc);
		}
		else {
			BuildCall b(a, 2);
			b.addArg(arg_ptr);
			b.addArg(arg_len_32);
			b.finalize(fn->get_func_ptr());
		}
	}
	template<bool use_result = true, bool do_cleanup = true>
	void compilerFabric_call(CASM& a, const std::vector<uint8_t>& data, size_t data_len, size_t& i, list_array<ValueItem>& values, std::vector<ValueItem*> static_map, list_array<typed_lgr<FuncEnvironment>>& used_environs) {
		CallFlags flags;
		flags.encoded = readData<uint8_t>(data, data_len, i);
		BuildCall b(a, 0);
		ValueIndexPos value_index = readIndexPos(data, data_len, i);
		if(value_index.pos == ValuePos::in_constants){
			if(flags.always_dynamic){
				b.setArguments(5);
				b.addArg(_compilerFabric_get_constant_string(value_index, values, static_map));
				b.addArg(arg_ptr);
				b.addArg(arg_len_32);
				b.addArg(flags.async_mode);
				b.finalize(&FuncEnvironment::callFunc);
			}
			else{
				_compilerFabric_call_fun_string(
					a,
					*_compilerFabric_get_constant_string(value_index, values, static_map),
					flags.async_mode,
					used_environs
				);
			}
		}
		else{
			b.setArguments(2);
			b.lea_valindex({static_map, values},value_index);
			b.addArg(VType::string);
			b.finalize(getSpecificValue);
			b.setArguments(5);
			b.addArg(resr);
			b.addArg(arg_ptr);
			b.addArg(arg_len_32);
			b.addArg(flags.async_mode);
			b.finalize(&FuncEnvironment::callFunc);
		}
		if constexpr (use_result) {
			if (flags.use_result) {
				b.setArguments(2);
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


	void _compilerFabric_call_local(CASM& a, FuncHandle::inner_handle* env, uint32_t fn_index, bool is_async){
		if (env->localFnSize() >= fn_index) 
			throw InvalidIL("Invalid function index");
		auto& fn = env->localFn(fn_index);
		if (is_async) {
			BuildCall b(a, 3);
			b.addArg(&fn);
			b.addArg(arg_ptr);
			b.addArg(arg_len_32);
			b.finalize(&FuncEnvironment::asyncWrapper);
		}
		else {
			BuildCall b(a, 2);
			b.addArg(arg_ptr);
			b.addArg(arg_len_32);
			b.finalize(fn->get_func_ptr());
		}
	}
	template<bool use_result = true, bool do_cleanup = true>
	void compilerFabric_call_local(CASM& a, const std::vector<uint8_t>& data, size_t data_len, size_t& i, FuncHandle::inner_handle* env, list_array<ValueItem>& values,  std::vector<ValueItem*> static_map) {
		CallFlags flags;
		flags.encoded = readData<uint8_t>(data, data_len, i);
		auto value_index = readIndexPos(data, data_len, i);
		if(value_index.pos == ValuePos::in_constants){
			uint32_t index = (uint32_t)values[value_index.index + static_map.size()];
			_compilerFabric_call_local(a, env, index, flags.async_mode);
		}else{
			BuildCall b(a, 1);
			b.lea_valindex({static_map, values}, value_index);
			b.finalize(getSize);
			b.setArguments(5);
			b.addArg(env);
			b.addArg(resr);
			b.addArg(arg_ptr);
			b.addArg(arg_len_32);
			b.addArg(flags.async_mode);
			b.finalize(&FuncHandle::inner_handle::localWrapper);
			b.setArguments(0);
		}
		if constexpr (use_result) {
			if (flags.use_result) {
				BuildCall b(a, 2);
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
		args_tmp.push_back(ValueItem(*class_ptr, as_reference));
		for(uint32_t i = 0; i < len; i++)
			args_tmp.push_back(ValueItem(args[i], as_reference));
		return _valueItemDynamicCall<async_mode>(name, class_ptr, access, args_tmp.data(), len + 1);
	}

	template<bool async_mode>
	ValueItem* valueItemDynamicCallId(uint64_t id, ValueItem* class_ptr, ValueItem* args, uint32_t len) {
		if (!class_ptr)
			throw NullPointerException();
		class_ptr->getAsync();
		list_array<ValueItem> args_tmp;
		args_tmp.reserve_push_back(len + 1);
		args_tmp.push_back(ValueItem(*class_ptr, as_reference));
		for(uint32_t i = 0; i < len; i++)
			args_tmp.push_back(ValueItem(args[i], as_reference));
			
		return _valueItemDynamicCall<async_mode>(id, class_ptr, args_tmp.data(), len + 1);
	}

	template<bool use_result = true, bool do_cleanup = true>
	void compilerFabric_value_call(CASM& a, const std::vector<uint8_t>& data, size_t data_len, size_t& i, list_array<ValueItem>& values, std::vector<ValueItem*> static_map) {
		CallFlags flags;
		flags.encoded = readData<uint8_t>(data, data_len, i);
		BuildCall b(a, 0);
		auto value_index = readIndexPos(data, data_len, i);
		if(value_index.pos == ValuePos::in_constants){
			b.setArguments(5);
			b.addArg(_compilerFabric_get_constant_string(value_index, values, static_map));
		}
		else {
			b.lea_valindex({static_map, values},value_index);
			b.addArg(VType::string);
			b.finalize(getSpecificValue);
			b.setArguments(5);
			b.addArg(resr);
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
		auto value_index = readIndexPos(data, data_len, i);
		if (value_index.pos == ValuePos::in_constants) {
			b.setArguments(5);
			b.addArg(_compilerFabric_get_constant_string(value_index, values, static_map));
		}
		else {
			b.lea_valindex({static_map, values},readIndexPos(data, data_len, i));//value
			b.addArg(VType::string);
			b.finalize(getSpecificValue);
			b.setArguments(5);
			b.addArg(resr);
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
		size_t length = (size_t)CXX::Interface::makeCall(ClassAccess::pub, *arr_ref, symbols::structures::size);
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
		ValueItem temp = CXX::Interface::makeCall(ClassAccess::pub, *arr_ref, symbols::structures::index_operator, pos);
		*value = temp.val;
		*((size_t*)(value + 1)) = temp.meta.encoded;
		temp.val = nullptr;
	}
	template<char typ>
	void IndexArraySetStaticInterface(void** value, ValueItem* arr_ref, uint64_t pos) {
		size_t length = (size_t)CXX::Interface::makeCall(ClassAccess::pub, *arr_ref, symbols::structures::size);
		if constexpr (typ != 0) {
			if (length <= pos)
				throw OutOfRange();
		}
		CXX::Interface::makeCall(ClassAccess::pub, *arr_ref, symbols::structures::index_set_operator, pos, reinterpret_cast<ValueItem&>(value));
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



	void throwEx(ValueItem* typ, ValueItem* desc) {
		static ValueItem noting;
		if(desc == nullptr)
			desc = &noting;
		if(typ == nullptr)
			typ = &noting;
		if(typ->meta.vtype == VType::string && desc->meta.vtype == VType::string)
			throw AException(*(std::string*)typ->getSourcePtr(),*(std::string*)desc->getSourcePtr());
		else if (typ->meta.vtype == VType::string)
			throw AException(*(std::string*)typ->getSourcePtr(), (std::string)*desc);
		else if (desc->meta.vtype == VType::string)
			throw AException((std::string)*typ, *(std::string*)desc->getSourcePtr());
		else
			throw AException((std::string)*typ, (std::string)*desc);
	}


	void setValue(void*& val, void* set, ValueMeta meta) {
		val = copyValue(set, meta);
	}
	void getInterfaceValue(ClassAccess access, ValueItem* val, const std::string* val_name, ValueItem* res) {
		*res = art::CXX::Interface::getValue(access, *val, *val_name);
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
		*result = ValueItem(arr + start, ValueMeta(VType::faarr,false,true, end - start), as_reference);
	}

	void ValueItem_make_ref(ValueItem* result, ValueItem* source){
		*result = ValueItem(*source, as_reference);
	}
	void ValueItem_copy_unref(ValueItem* result, ValueItem* source){
		ValueMeta meta = source->meta;
		meta.as_ref = false;
		*result = ValueItem(source->unRef(), meta);
	}
	void ValueItem_move_unref(ValueItem* result, ValueItem* source){
		ValueMeta meta = source->meta;
		meta.as_ref = false;
		*result = ValueItem(source->unRef(), meta, no_copy);
		*source = nullptr;
	}


#pragma endregion
#pragma region ScopeAction

	bool _attacha_filter(CXXExInfo& info, void** continue_from, void* data, size_t size, void* enviro) {
		uint8_t* data_info = (uint8_t*)data;
		list_array<std::string> exceptions;
		*continue_from = internal::readFromArrayAsValue<void*>(data_info);
		switch (*data_info++) {
			case 0: {
				uint64_t handle_count = internal::readFromArrayAsValue<uint64_t>(data_info);
				exceptions.reserve_push_back(handle_count);
				for (size_t i = 0; i < handle_count; i++) {
					std::string string;
					size_t len = 0;
					char* str = internal::readFromArrayAsArray<char>(data_info, len);
					string.assign(str, len);
					delete[] str;
					exceptions.push_back(string);
				}
				break;
			}
			case 1: {
				uint16_t value = internal::readFromArrayAsValue<uint16_t>(data_info);
				ValueItem* item = (ValueItem*)enviro + (uint32_t(value)<<1);
				exceptions.push_back((std::string)*item);
				break;
			}
			case 2: {
				uint64_t handle_count = internal::readFromArrayAsValue<uint64_t>(data_info);
				exceptions.reserve_push_back(handle_count);
				for (size_t i = 0; i < handle_count; i++) {
					uint16_t value = internal::readFromArrayAsValue<uint16_t>(data_info);
					ValueItem* item = (ValueItem*)enviro + (uint32_t(value)<<1);
					exceptions.push_back((std::string)*item);
				}
				break;
			}
			case 3: {
				uint64_t handle_count = internal::readFromArrayAsValue<uint64_t>(data_info);
				exceptions.reserve_push_back(handle_count);
				for (size_t i = 0; i < handle_count; i++) {
					bool is_dynamic = internal::readFromArrayAsValue<bool>(data_info);
					if (!is_dynamic) {
						std::string string;
						size_t len = 0;
						char* str = internal::readFromArrayAsArray<char>(data_info, len);
						string.assign(str, len);
						delete[] str;
						exceptions.push_back(string);
					}
					else {
						uint16_t value = internal::readFromArrayAsValue<uint16_t>(data_info);
						ValueItem* item = (ValueItem*)enviro + (uint32_t(value)<<1);
						exceptions.push_back((std::string)*item);
					}
				}
				break;
			}
			case 4://catch all
				//prevent catch CLR exception
				return exception::try_catch_all(info);
			case 5:{//attacha filter function
				Environment env_filter = internal::readFromArrayAsValue<Environment>(data_info);
				uint16_t filter_enviro_slice_begin = internal::readFromArrayAsValue<uint32_t>(data_info);
				uint16_t filter_enviro_slice_end = internal::readFromArrayAsValue<uint32_t>(data_info);
				if(filter_enviro_slice_begin >= filter_enviro_slice_end)
					throw InvalidIL("Invalid environment slice");
				uint16_t filter_enviro_size = filter_enviro_slice_end - filter_enviro_slice_begin;
				auto env_res = env_filter((ValueItem*)enviro + filter_enviro_slice_begin, filter_enviro_size);
				if(env_res == nullptr)
					return false;
				else{
					bool res = (bool)*env_res;
					delete env_res;
					return res;
				}
			}
			default:
				throw BadOperationException();
		}
		return exception::map_native_exception_names(info).contains_one([&exceptions](const std::string& str) {
			return exceptions.contains(str);
		});
	}
	void _attacha_finally(void* data, size_t size, void* enviro){
		uint8_t* data_info = (uint8_t*)data;
		Environment env_finalizer = internal::readFromArrayAsValue<Environment>(data_info);
		uint16_t finalizer_enviro_slice_begin = internal::readFromArrayAsValue<uint32_t>(data_info);
		uint16_t finalizer_enviro_slice_end = internal::readFromArrayAsValue<uint32_t>(data_info);
		if(finalizer_enviro_slice_begin >= finalizer_enviro_slice_end)
			throw InvalidIL("Invalid environment slice");
		uint16_t finalizer_enviro_size = finalizer_enviro_slice_end - finalizer_enviro_slice_begin;
		auto tmp = env_finalizer((ValueItem*)enviro + finalizer_enviro_slice_begin, finalizer_enviro_size);
		if(tmp != nullptr)
			delete tmp;
	}


	
	void _inner_handle_finalizer(void* data, size_t size, void* rsp){
		(*(FuncHandle::inner_handle**)data)->reduce_usage();
	}
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
				return value_hold_id_map[id] = manager.createValueLifetimeScope(destruct, size_t(off)<<1);
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
		size_t try_mapValueHold(uint64_t id){
			auto it = value_hold_id_map.find(id);
			if (it == value_hold_id_map.end())
				return -1;
			else
				return it->second;
		}
		bool unmapHandle(uint64_t id){
			auto it = handle_id_map.find(id);
			if (it != handle_id_map.end()){
				manager.endValueLifetime(it->second);
				handle_id_map.erase(it);
				return true;
			}
			return false;
		}
		bool unmapValueHold(uint64_t id){
			auto it = value_hold_id_map.find(id);
			if (it != value_hold_id_map.end()){
				manager.endValueLifetime(it->second);
				value_hold_id_map.erase(it);
				return true;
			}
			return false;
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
		list_array<typed_lgr<FuncEnvironment>>& used_environs;
		bool do_jump_to_ret = false;
		bool in_debug;
		FuncHandle::inner_handle* build_func;

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
			FuncHandle::inner_handle* build_func,
			uint16_t static_values,
			list_array<typed_lgr<FuncEnvironment>>& used_environs
			) : a(a), scope(scope), scope_map(scope_map), prolog(prolog), self_function(self_function), data(data), data_len(data_len), i(start_from),skip_count(start_from), values(values), in_debug(in_debug), build_func(build_func), used_environs(used_environs) {
				label_bind_map.reserve(jump_list.size());
				label_map.reserve(jump_list.size());
				size_t i = 0;
				for(auto& it : jump_list){
					label_map[i++] = &(label_bind_map[it.first] = it.second);
				}
				for(uint16_t j =0;j<static_values;j++)
					static_map.push_back(&values[j]);
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
		void dynamic_create_saarr() {
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
		void dynamic_arg_set(){
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
				throw InvalidIL("Fail compile async 'call_self', for asynchronous call self use 'call' command");
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
				throw InvalidIL("Fail compile async 'call_self', for asynchronous call self use 'call' command");
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
			BuildCall b(a, 2);
			b.lea_valindex({static_map, values}, readIndexPos(data, data_len, i));
			b.lea_valindex({static_map, values}, readIndexPos(data, data_len, i));
			b.finalize(throwEx);
		}
		void dynamic_insert_native() {
			uint32_t len = readData<uint32_t>(data, data_len, i);
			auto tmp = extractRawArray<uint8_t>(data, data_len, i, len);
			a.insertNative(tmp, len);
		}
		void dynamic_set_structure_value(){
			BuildCall b(a, 0);
			auto value_index = readIndexPos(data, data_len, i);
			if (value_index.pos == ValuePos::in_constants) {
				b.addArg(readData<ClassAccess>(data, data_len, i));
				b.lea_valindex({static_map, values}, readIndexPos(data, data_len, i));//interface
				b.addArg(_compilerFabric_get_constant_string(value_index, values, static_map));
			}
			else {
				b.lea_valindex({static_map, values}, value_index);//value name
				b.addArg(VType::string);
				b.finalize(getSpecificValue);
				b.addArg(readData<ClassAccess>(data, data_len, i));
				b.lea_valindex({static_map, values}, readIndexPos(data, data_len, i));//interface
				b.addArg(resr);
			}
			b.lea_valindex({static_map, values}, readIndexPos(data, data_len, i));//value
			b.finalize((void(*)(ClassAccess, ValueItem&, const std::string&, const ValueItem&))art::CXX::Interface::setValue);
		}
		void dynamic_get_structure_value() {
			BuildCall b(a, 0);
			auto value_index = readIndexPos(data, data_len, i);
			if (value_index.pos == ValuePos::in_constants) {
				b.addArg(readData<ClassAccess>(data, data_len, i));
				b.lea_valindex({static_map, values}, readIndexPos(data, data_len, i));//interface
				b.addArg(_compilerFabric_get_constant_string(value_index, values, static_map));
			}
			else {
				b.lea_valindex({static_map, values}, value_index);//value name
				b.addArg(VType::string);
				b.finalize(getSpecificValue);
				b.addArg(readData<ClassAccess>(data, data_len, i));
				b.lea_valindex({static_map, values}, readIndexPos(data, data_len, i));//interface
				b.addArg(resr);
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
			std::vector<uint8_t> handler_data;
			handler_data.reserve(40);
			char command = readData<char>(data, data_len, i);
			handler_data.push_back(command);
			switch (command) {
			case 0:{
				uint64_t len = readPackedLen(data, data_len, i);
				builder::write(handler_data,len);
				for(uint64_t i = 0; i < len; i++){
					std::string name = readString(data, data_len, i);
					builder::write(handler_data, name.size());//size_t
					handler_data.insert(handler_data.end(), name.begin(), name.end());
				}
				break;
			}
			case 1:
				builder::write(handler_data,readData<uint16_t>(data, data_len, i));
				break;
			case 2:{
				uint64_t len = readPackedLen(data, data_len, i);
				builder::write(handler_data,len);
				for(uint64_t i = 0; i < len; i++)
					builder::write(handler_data,readData<uint16_t>(data, data_len, i));
				break;
			}
			case 3:{
				uint64_t len = readPackedLen(data, data_len, i);
				builder::write(handler_data,len);
				for(uint64_t i = 0; i < len; i++){
					bool is_dynamic = readData<bool>(data, data_len, i);
					builder::write(handler_data,is_dynamic);
					if(is_dynamic){
						builder::write(handler_data,readData<uint16_t>(data, data_len, i));
					}else{
						std::string name = readString(data, data_len, i);
						builder::write(handler_data, name.size());//size_t
						handler_data.insert(handler_data.end(), name.begin(), name.end());
					}
				}
				break;
			}
			case 4: break;//catch all
			case 5:{
				if(readData<bool>(data, data_len, i))//as local
					builder::write(handler_data, 
						build_func->localFn(
							readData<uint32_t>(data, data_len, i)
						)->get_func_ptr()
					);
				else
					builder::write(handler_data, 
						FuncEnvironment::environment(
							readString(data, data_len, i)
						)->get_func_ptr()
					);
				uint16_t enviro_slice_begin = readData<uint16_t>(data, data_len, i);
				uint16_t enviro_slice_end = readData<uint16_t>(data, data_len, i);
				builder::write(handler_data, enviro_slice_begin);
				builder::write(handler_data, enviro_slice_end);
				break;
			}
			default:
				throw InvalidIL("Invalid catch command");
			}
			scope.setExceptionHandle(handle, _attacha_filter, handler_data.data(), handler_data.size());
		}
		void dynamic_handle_finally(){
			size_t handle = scope_map.try_mapHandle(readData<uint64_t>(data, data_len, i));
			if (handle == -1)
				throw InvalidIL("Undefined handle");
			std::vector<uint8_t> handler_data;
			handler_data.reserve(sizeof(Environment) + sizeof(uint16_t) * 2);
			if(readData<bool>(data, data_len, i))//as local
				builder::write(handler_data, 
					build_func->localFn(
						readData<uint32_t>(data, data_len, i)
					)->get_func_ptr()
				);
			else
				builder::write(handler_data, 
					FuncEnvironment::environment(
						readString(data, data_len, i)
					)->get_func_ptr()
				);
			uint16_t enviro_slice_begin = readData<uint16_t>(data, data_len, i);
			uint16_t enviro_slice_end = readData<uint16_t>(data, data_len, i);
			builder::write(handler_data, enviro_slice_begin);
			builder::write(handler_data, enviro_slice_end);
		}
		void dynamic_handle_end(){
			bool removed = scope_map.unmapHandle(readData<uint64_t>(data, data_len, i));
			if (!removed)
				throw InvalidIL("Undefined handle");
		}

		void dynamic_value_hold(){
			uint64_t hold_id = readData<uint64_t>(data, data_len, i);
			uint16_t value_index = readData<uint16_t>(data, data_len, i);
			scope_map.mapValueHold(hold_id, universalRemove, value_index);
		}
		void dynamic_value_unhold(){
			uint64_t hold_id = readData<uint64_t>(data, data_len, i);
			if (!scope_map.unmapValueHold(hold_id))
				throw InvalidIL("Undefined hold");
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
				b.finalize(&ValueItem::operator uint64_t );


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
		void dynamic_get_reference(){
			BuildCall b(a, 2);
			b.lea_valindex({static_map, values}, readIndexPos(data, data_len, i));
			b.lea_valindex({static_map, values}, readIndexPos(data, data_len, i));
			b.finalize(ValueItem_make_ref);
		}
		void dynamic_make_as_const(){
			ValueIndexPos result_index = readIndexPos(data, data_len, i);
			a.mov_valindex_meta({static_map, values}, resr, result_index);
			a.and_(resr,~ValueMeta(VType::noting,false,true,0,false).encoded);
			a.mov_valindex_meta({static_map, values}, result_index, resr, argr0);
		}
		void dynamic_remove_const_protect(){
			ValueIndexPos result_index = readIndexPos(data, data_len, i);
			a.mov_valindex_meta({static_map, values}, resr, result_index);
			a.or_(resr,ValueMeta(VType::noting,false,true,0,false).encoded);
			a.mov_valindex_meta({static_map, values}, result_index, resr, argr0);
		}
		void dynamic_copy_un_constant(){
			ValueIndexPos result_index = readIndexPos(data, data_len, i);
			ValueIndexPos source_index = readIndexPos(data, data_len, i);

			BuildCall b(a, 2);
			b.lea_valindex({static_map, values}, result_index);
			b.lea_valindex({static_map, values}, source_index);
			b.finalize((ValueItem&(ValueItem::*)(const ValueItem&))&ValueItem::operator=);
			a.mov_valindex_meta({static_map, values}, resr, result_index);
			a.or_(resr,ValueMeta(VType::noting,false,true,0,false).encoded);
			a.mov_valindex_meta({static_map, values}, result_index, resr, argr0);
		}
		void dynamic_copy_un_reference(){
			ValueIndexPos result_index = readIndexPos(data, data_len, i);
			ValueIndexPos source_index = readIndexPos(data, data_len, i);
			BuildCall b(a, 2);
			b.lea_valindex({static_map, values}, result_index);
			b.lea_valindex({static_map, values}, source_index);
			b.finalize(ValueItem_copy_unref);
		}
		void dynamic_move_un_reference(){
			ValueIndexPos result_index = readIndexPos(data, data_len, i);
			ValueIndexPos source_index = readIndexPos(data, data_len, i);
			BuildCall b(a, 2);
			b.lea_valindex({static_map, values}, result_index);
			b.lea_valindex({static_map, values}, source_index);
			b.finalize(ValueItem_move_unref);
		}
		void dynamic_remove_qualifiers(){
			ValueIndexPos result_index = readIndexPos(data, data_len, i);
			BuildCall b(a, 2);
			b.lea_valindex({static_map, values}, result_index);
			b.finalize(&ValueItem::ungc);
			a.mov_valindex_meta({static_map, values}, resr, result_index);
			a.or_(resr,ValueMeta(VType::noting,false,true,0,false).encoded);
			a.mov_valindex_meta({static_map, values}, result_index, resr, argr0);
		}
#pragma endregion
#pragma region static opcodes

#pragma endregion





		void static_build(){
			switch (cmd.code) {
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
					BuildCall b(a, 2);
					b.lea_valindex({static_map, values},readIndexPos(data, data_len, i));
					b.lea_valindex({static_map, values},readIndexPos(data, data_len, i));
					b.finalize(throwEx);
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
				case Opcode::create_saarr: dynamic_create_saarr(); break;
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
				case Opcode::arg_set: dynamic_arg_set(); break;
				case Opcode::call: compilerFabric_call<true>(a, data, data_len, i, values, static_map, used_environs); break;
				case Opcode::call_self: dynamic_call_self(); break;
				case Opcode::call_local: compilerFabric_call_local<true>(a, data, data_len, i, build_func, values, static_map);
				case Opcode::call_and_ret: {
					compilerFabric_call<false, false>(a, data, data_len, i, values, static_map, used_environs);
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
				case Opcode::handle_catch: dynamic_handle_catch(); break;
				case Opcode::handle_finally: dynamic_handle_finally(); break;
				case Opcode::handle_end: dynamic_handle_end(); break;
				case Opcode::value_hold: dynamic_value_hold(); break;
				case Opcode::value_unhold: dynamic_value_unhold(); break;
				case Opcode::is_gc: dynamic_is_gc(); break;
				case Opcode::to_gc: dynamic_to_gc(); break;
				case Opcode::localize_gc: dynamic_localize_gc(); break;
				case Opcode::from_gc: dynamic_from_gc(); break;
				case Opcode::table_jump: dynamic_table_jump(); break;
				case Opcode::xarray_slice: dynamic_xarray_slice(); break;
				case Opcode::store_constant: store_constant(); break;
				
				case Opcode::get_reference: dynamic_get_reference(); break;
				case Opcode::make_as_const: dynamic_make_as_const(); break;
				case Opcode::remove_const_protect: dynamic_remove_const_protect(); break;
				case Opcode::copy_un_constant: dynamic_copy_un_constant(); break;
				case Opcode::copy_un_reference:dynamic_copy_un_reference(); break;
				case Opcode::move_un_reference: dynamic_move_un_reference(); break;
				case Opcode::remove_qualifiers: dynamic_remove_qualifiers(); break;
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
				cmd = readData<Command>(data, data_len, i);
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



	FuncHandle::inner_handle::inner_handle(Environment env, bool is_cheap) : is_cheap(is_cheap){
		_type = FuncType::own;
		this->env = env;
	}
	FuncHandle::inner_handle::inner_handle(void* func, const DynamicCall::FunctionTemplate& template_func, bool is_cheap) : is_cheap(is_cheap) {
		_type = FuncType::native_c;
		values.push_back(new DynamicCall::FunctionTemplate(template_func));
		frame = (uint8_t*)func;
	}
	FuncHandle::inner_handle::inner_handle(void* func, FuncHandle::inner_handle::ProxyFunction proxy_func, bool is_cheap) : is_cheap(is_cheap){
		_type = FuncType::static_native_c;
		values.push_back(func);
		frame = (uint8_t*)proxy_func;
	}
	FuncHandle::inner_handle::inner_handle(const std::vector<uint8_t>& code, bool is_cheap) : is_cheap(is_cheap){
		_type = FuncType::own;
		this->cross_code = code;
	}
	FuncHandle::inner_handle::inner_handle(const std::vector<uint8_t>& code, const list_array<ValueItem>& values, bool is_cheap) : is_cheap(is_cheap){
		_type = FuncType::own;
		this->cross_code = code;
		this->values = values;
	}
	FuncHandle::inner_handle::inner_handle(const std::vector<uint8_t>& code, const list_array<ValueItem>& values, const std::vector<typed_lgr<FuncEnvironment>>& local_funcs, bool is_cheap) : is_cheap(is_cheap){
		_type = FuncType::own;
		this->cross_code = code;
		this->values = values;
		this->local_funcs = local_funcs;
	}

	FuncHandle::inner_handle::inner_handle(std::vector<uint8_t>&& code, bool is_cheap) : is_cheap(is_cheap){
		_type = FuncType::own;
		this->cross_code = std::move(code);
	}
	FuncHandle::inner_handle::inner_handle(std::vector<uint8_t>&& code, list_array<ValueItem>&& values, bool is_cheap) : is_cheap(is_cheap){
		_type = FuncType::own;
		this->cross_code = std::move(code);
		this->values = std::move(values);
	}
	FuncHandle::inner_handle::inner_handle(std::vector<uint8_t>&& code, list_array<ValueItem>&& values, std::vector<typed_lgr<FuncEnvironment>>&& local_funcs, bool is_cheap) : is_cheap(is_cheap){
		_type = FuncType::own;
		this->cross_code = std::move(code);
		this->values = std::move(values);
		this->local_funcs = std::move(local_funcs);
	}
	ValueItem* FuncHandle::inner_handle::localWrapper(size_t indx, ValueItem* arguments, uint32_t arguments_size, bool run_async){
		if (indx < local_funcs.size()){
			if(run_async)
				return FuncEnvironment::async_call(local_funcs[indx], arguments, arguments_size);
			else
				return local_funcs[indx]->syncWrapper(arguments, arguments_size);
		}
		else
			throw InvalidArguments("Invalid local function index, index is out of range");
	}
	FuncHandle::inner_handle::~inner_handle(){
		if (frame != nullptr && _type == FuncType::own){
			if(!FrameResult::deinit(frame, env, *art)){
				ValueItem result{ "Failed unload function:", frame };
				errors.async_notify(result);
			}
		}
		if (_type == FuncType::native_c) {
			delete (DynamicCall::FunctionTemplate*)(void*)values[0];
		}
	}







	void FuncHandle::inner_handle::compile() {
		if (frame != nullptr)
			throw InvalidOperation("Function already compiled");
		used_environs.clear();
		RuntimeCompileException error_handler;
		CodeHolder code;
		code.setErrorHandler(&error_handler);
		code.init(art->environment());
		CASM a(code);
		BuildProlog b_prolog(a);
		ScopeManager scope(b_prolog);
		ScopeManagerMap scope_map(scope);



		Label self_function = a.newLabel();
		a.label_bind(self_function);

		size_t to_be_skiped = 0;
		auto&& [jump_list, function_locals, flags, used_static_values, used_enviro_vals, used_arguments, constants_values] = decodeFunctionHeader(a, cross_code, cross_code.size(), to_be_skiped);
		uint32_t to_alloc_statics = flags.used_static ? uint32_t(used_static_values) + 1 : 0;
		if(flags.run_time_computable){
			//local_funcs already contains all local functions
			if(values.size() > to_alloc_statics)
				values.remove(to_alloc_statics, values.size());
			else{
				values.reserve_push_front(to_alloc_statics - values.size());
				for(uint32_t i = values.size(); i < to_alloc_statics; i++)
					values.push_front(nullptr);
			}
		}else{
			local_funcs = std::move(function_locals);
			values.resize(to_alloc_statics, nullptr);
		}
		constants_values += to_alloc_statics;
		values.reserve_push_back(std::clamp<uint32_t>(constants_values, 0, UINT32_MAX));
		uint32_t max_values = flags.used_enviro_vals ? uint32_t(used_enviro_vals) + 1 : 0;


		//OS dependent prolog begin
		size_t is_patchable_exception_finalizer = 0;
		if(flags.is_patchable){
			a.atomic_increase(&ref_count);//increase usage count
			is_patchable_exception_finalizer = scope.createExceptionScope();
			auto self = this;
			scope.setExceptionFinal(is_patchable_exception_finalizer, _inner_handle_finalizer, &self, sizeof(self));
		}
		b_prolog.pushReg(frame_ptr);
		if(max_values)
			b_prolog.pushReg(enviro_ptr);
		b_prolog.pushReg(arg_ptr);
		b_prolog.pushReg(arg_len);
		b_prolog.alignPush();
		b_prolog.stackAlloc(0x20);//c++ abi
		b_prolog.setFrame();
		b_prolog.end_prolog();
		//OS dependent prolog end 

		//Init environment
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
			b.finalize((void(*)(uint32_t,uint32_t))art::CXX::arguments_range);
			a.label_bind(correct);
		}

		//Clean environment
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
		CompilerFabric fabric(a,scope,scope_map,prolog, self_function, cross_code, cross_code.size(), to_be_skiped, jump_list, values, flags.in_debug, this, to_alloc_statics, used_environs);
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
		auto& tmp = b_prolog.finalize_epilog();
		
		if(flags.is_patchable){
			scope.endExceptionScope(is_patchable_exception_finalizer);
			a.mov(argr0, -1);
			a.atomic_fetch_add(&ref_count, argr0);
			a.cmp(argr0, 1);//if old ref_count == 1

			Label last_usage = a.newLabel();
			a.jmp_equal(last_usage);
			a.ret();
			a.label_bind(last_usage);
			a.mov(argr0, this);
			a.mov(argr1, resr);
			auto func_ptr = &FuncHandle::inner_handle::last_usage_env;
			Label last_usage_function = a.add_data((char*)reinterpret_cast<void*&>(func_ptr), 8);
			a.jmp_in_label(last_usage_function);
		}else{
			a.ret();
		}

		tmp.use_handle = true;
		tmp.exHandleOff = a.offset() <= UINT32_MAX ? (uint32_t)a.offset() : throw InvalidFunction("Too big function");
		a.jmp((size_t)exception::__get_internal_handler());
		a.finalize();
		auto resolved_frame =try_resolve_frame(this);
		env = (Environment)tmp.init(frame, a.code(), *art, resolved_frame.data());
		//remove self from used_environs
		auto my_trampoline = parent ? parent->get_trampoline_code() : nullptr;
		used_environs.remove_if([my_trampoline](typed_lgr<FuncEnvironment>& a){return a->get_func_ptr() == my_trampoline;});
	}

	
	ValueItem* FuncHandle::inner_handle::dynamic_call_helper(ValueItem* arguments, uint32_t arguments_size){
		auto template_func = (DynamicCall::FunctionTemplate*)(void*)values[0];
		DynamicCall::FunctionCall call((DynamicCall::PROC)frame, *template_func, true);
		return __attacha___::NativeProxy_DynamicToStatic(call, *template_func, arguments, arguments_size);
	}
	ValueItem* FuncHandle::inner_handle::static_call_helper(ValueItem* arguments, uint32_t arguments_size){
		FuncHandle::inner_handle::ProxyFunction proxy = (FuncHandle::inner_handle::ProxyFunction)frame;
		void* func = (void*)values[0];
		return proxy(func, arguments, arguments_size);
	}

#pragma endregion

#pragma region FuncHandle
	FuncHandle* FuncHandle::make_func_handle(inner_handle* handle) {
		if(handle ? handle->parent : false)
			throw InvalidArguments("Handle already in use");
		RuntimeCompileException error_handler;
		CodeHolder trampoline_code;
		trampoline_code.setErrorHandler(&error_handler);
		trampoline_code.init(art->environment());
		CASM a(trampoline_code);
		char fake_data[8]{ 0xFFi8 };
		Label compile_call_label = a.add_data(fake_data, 8);
		Label handle_label = a.add_data(fake_data, 8);

		Label not_compiled_code_fallback = a.newLabel();
		Label data_label;
		if(handle ? handle->env : false)
			data_label = a.add_data((char*)handle->env, sizeof(handle->env));
		else
			data_label = a.add_label_ptr(not_compiled_code_fallback);
		a.jmp_in_label(data_label);										//jmp [env | not_compiled_code_fallback]
		a.label_bind(not_compiled_code_fallback);						//not_compiled_code_fallback:
		a.mov(argr2,argr1);												//mov argr2, argr1
		a.mov(argr1, argr0);											//mov argr1, argr0
		a.mov_long(argr0, handle_label, 0);								//mov argr0, [FuncHandle]
		a.jmp_in_label(compile_call_label);								//jmp [FuncHandle::compile_call]
		a.finalize();
		size_t code_size = trampoline_code.textSection()->realSize();
		code_size = code_size + asmjit::Support::alignUp(code_size, trampoline_code.textSection()->alignment());
		FuncHandle* code;
		CASM::allocate_and_prepare_code(sizeof(FuncHandle),(uint8_t*&)code, &trampoline_code, art->allocator(),0);
		new(code) FuncHandle();
		char* code_raw = (char*)code + sizeof(FuncHandle);
		char* code_data = (char*)code + sizeof(FuncHandle) + code_size - 1;


		char* trampoline_jump = (char*)code_data + trampoline_code.labelOffset(data_label) + 8;//24
		char* trampoline_not_compiled_fallback = (char*)code_raw + trampoline_code.labelOffset(not_compiled_code_fallback);
		char* handle_ptr = (char*)code_data + trampoline_code.labelOffset(handle_label) + 8;//16
		char* function_ptr = (char*)code_data + trampoline_code.labelOffset(compile_call_label) + 8;//8



		auto func_ptr = &compile_call;
		if(handle ? handle->env : false)
			*(void**)trampoline_jump = (char*)handle->env;
		else
			*(void**)trampoline_jump = trampoline_not_compiled_fallback;
		*(void**)handle_ptr = code;
		*(void**)function_ptr = reinterpret_cast<void*&>(func_ptr);

		code->trampoline_not_compiled_fallback = (void*)trampoline_not_compiled_fallback;
		code->trampoline_jump = (void**)trampoline_jump;
		code->handle = handle;
		if(handle){
			handle->increase_usage();
			handle->parent = code;
		}
		code->art_ref = new std::shared_ptr<asmjit::JitRuntime>(art);
		return code;
	}
	void FuncHandle::release_func_handle(FuncHandle* handle){
		std::shared_ptr<asmjit::JitRuntime> art = *(std::shared_ptr<asmjit::JitRuntime>*)handle->art_ref;
		handle->~FuncHandle();
		CASM::release_code((uint8_t*)handle, art->allocator());
	}
	FuncHandle::~FuncHandle(){
		lock_guard lock(compile_lock);
		if (handle)
			handle->reduce_usage();
		delete (std::shared_ptr<asmjit::JitRuntime>*)art_ref;
	}
	void FuncHandle::patch(inner_handle* handle) {
		lock_guard lock(compile_lock);
		if(handle){
			if(handle->parent != nullptr)
				throw InvalidArguments("Handle already in use");
				
			if(handle->env != nullptr){
				if(trampoline_jump != nullptr)
					*trampoline_jump = handle->env;
			}else{
				if(trampoline_jump != nullptr)
					*trampoline_jump = trampoline_not_compiled_fallback;
			}
		}else{
			if(trampoline_jump != nullptr)
				*trampoline_jump = trampoline_not_compiled_fallback;
		}
		if(this->handle != nullptr){
			if(!this->handle->is_patchable)
				throw InvalidOperation("Tried patch unpatchable function");
			this->handle->reduce_usage();
		}
		handle->increase_usage();
		this->handle = handle;
		handle->parent = this;
	}
	bool FuncHandle::is_cheap(){
		lock_guard lock(compile_lock);
		return handle? handle->is_cheap : false;
	}
	const std::vector<uint8_t>& FuncHandle::code(){
		lock_guard lock(compile_lock);
		static std::vector<uint8_t> empty;
		return handle ? handle->cross_code : empty;
	}
	ValueItem* FuncHandle::compile_call(ValueItem* arguments, uint32_t arguments_size){
		unique_lock lock(compile_lock);
		if(handle){
			inner_handle::usage_scope scope(handle);
			inner_handle *compile_handle = handle;
			if(handle->_type == inner_handle::FuncType::own){
				if(trampoline_jump != nullptr && handle->env != nullptr){
					if(*trampoline_jump != handle->env){
						*trampoline_jump = handle->env;
						goto not_need_compile;
					}
				}
				lock.unlock();
				compile_handle->compile();
				lock.lock();
				if(trampoline_jump != nullptr)
					*trampoline_jump = compile_handle->env;
			}
		not_need_compile:
			lock.unlock();
			switch (compile_handle->_type) {
			case inner_handle::FuncType::own:
				return compile_handle->env(arguments, arguments_size);
			case inner_handle::FuncType::native_c:
				return compile_handle->dynamic_call_helper(arguments, arguments_size);
			case inner_handle::FuncType::static_native_c:
				return compile_handle->static_call_helper(arguments, arguments_size);
			default:
				throw NotImplementedException();//function not implemented
			}
		}
		throw NotImplementedException();//function not implemented
	}
	void* FuncHandle::get_trampoline_code(){
		unique_lock lock(compile_lock);
		if(handle)
			if(!handle->is_patchable)
				if(handle->_type == inner_handle::FuncType::own)
					return handle->env;
		return trampoline_code;
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
		if(func_ == nullptr)
			throw InvalidFunction("Function is force unloaded");
		return ((Environment)&func_->trampoline_code)(args, arguments_size);
	}
	ValueItem* FuncEnvironment::asyncWrapper(typed_lgr<FuncEnvironment>* self, ValueItem* arguments, uint32_t arguments_size) {
		return FuncEnvironment::async_call(*self, arguments, arguments_size);
	}
	std::string FuncEnvironment::to_string() const{
		if(!func_)
			return "fn(unloaded)@0";
		
		if(func_->handle == nullptr)
			return "fn(unknown)@0";
		void* fn_ptr;
		switch (func_->handle->_type) {
		case FuncHandle::inner_handle::FuncType::own:
			fn_ptr = func_->handle->env;
			break;
		case FuncHandle::inner_handle::FuncType::native_c:
			fn_ptr = func_->handle->frame;
			break;
		case FuncHandle::inner_handle::FuncType::static_native_c:
			fn_ptr = (void*)func_->handle->values[0];
			break;
		default:
			fn_ptr = nullptr;
		}
		{
			unique_lock guard(environments_lock);
			for(auto& it : environments)
				if(it.second.getPtr() == this)
					return "fn(" + it.first + ")@" + string_help::hexstr((ptrdiff_t)fn_ptr);
		}
		if(fn_ptr == nullptr)
			return "fn(unresolved)@unknown";
		return "fn(" + FrameResult::JitResolveFrame(fn_ptr,true).fn_name + ")@" + string_help::hexstr((ptrdiff_t)fn_ptr);
	}
	const std::vector<uint8_t>& FuncEnvironment::get_cross_code(){
		static std::vector<uint8_t> empty;
		return func_?func_->code():empty;
	}

	void FuncEnvironment::fastHotPatch(const std::string& func_name, FuncHandle::inner_handle* new_enviro) {
		unique_lock guard(environments_lock);
		auto& tmp = environments[func_name];
		guard.unlock();
		tmp->patch(new_enviro);
	}
	void FuncEnvironment::fastHotPatch(const patch_list& patches) {
		for(auto& it : patches)
			if(it.second->parent)
				throw InvalidOperation("Can't patch function with bounded handle: " + it.first);
		for(auto& it : patches)
			fastHotPatch(it.first, it.second);
	}
	typed_lgr<FuncEnvironment> FuncEnvironment::environment(const std::string& func_name) {
		return environments[func_name];
	}
	ValueItem* FuncEnvironment::callFunc(const std::string& func_name, ValueItem* arguments, uint32_t arguments_size, bool run_async) {
		auto found = environments.find(func_name);
		if (found != environments.end()) {
			if (run_async)
				return async_call(found->second, arguments, arguments_size);
			else
				return found->second->syncWrapper(arguments, arguments_size);
		}
		throw NotImplementedException();
	}
	void FuncEnvironment::AddNative(Environment function, const std::string& symbol_name, bool can_be_unloaded, bool is_cheap) {
		art::lock_guard guard(environments_lock);
		if (environments.contains(symbol_name))
			throw SymbolException("Fail allocate symbol: \"" + symbol_name + "\" cause them already exists");
		auto symbol = new FuncHandle::inner_handle(function, is_cheap);
		environments[symbol_name] = new FuncEnvironment(symbol, can_be_unloaded);
	}
	bool FuncEnvironment::Exists(const std::string& symbol_name) {
		art::lock_guard guard(environments_lock);
		return environments.contains(symbol_name);
	}
	void FuncEnvironment::Load(typed_lgr<FuncEnvironment> fn, const std::string& symbol_name) {
		art::lock_guard guard(environments_lock);
		auto found = environments.find(symbol_name);
		if (found != environments.end()) {
			if (found->second->func_ != nullptr)
				if(found->second->func_->handle != nullptr)
					throw SymbolException("Fail load symbol: \"" + symbol_name + "\" cause them already exists");
			found->second = fn;
		}
		else
			environments[symbol_name] = fn;
	}
	void FuncEnvironment::Unload(const std::string& func_name) {
		art::lock_guard guard(environments_lock);
		auto found = environments.find(func_name);
		if (found != environments.end()){
			if (!found->second->can_be_unloaded) 
				throw SymbolException("Fail unload symbol: \"" + func_name + "\" cause them can't be unloaded");
			environments.erase(found);
		}
	}
	void FuncEnvironment::ForceUnload(const std::string& func_name) {
		art::lock_guard guard(environments_lock);
		auto found = environments.find(func_name);
		if (found != environments.end())
			environments.erase(found);
	}
	void FuncEnvironment::forceUnload(){
		FuncHandle* handle = func_;
		func_ = nullptr;
		if(handle)
			FuncHandle::release_func_handle(handle);
	}
	void FuncEnvironment::clear_environs(){
		environments.clear();
	}
#pragma endregion
}