#ifndef SRC_RUN_TIME_ASM_COMPILER
#define SRC_RUN_TIME_ASM_COMPILER
#include "CASM.hpp"
#include "../attacha_abi_structs.hpp"
#include "FuncEnvironment.hpp"
namespace art{
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

	class Compiler{
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
		list_array<art::shared_ptr<FuncEnvironment>>& used_environs;
		bool in_debug;
		FuncHandle::inner_handle* build_func;

		std::vector<ValueItem*> static_map;
	public:
		Compiler(CASM& a,
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
			list_array<art::shared_ptr<FuncEnvironment>>& used_environs
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

		std::vector<ValueItem*>& get_static_map(){
			return static_map;
		}

		asmjit::Label& resolve_label(uint64_t id) {
			auto label = label_map.find(id);
			if (label == label_map.end())
				throw InvalidFunction("Invalid function header, not found jump position for label: " + std::to_string(id));
			return *label->second;
		}

		void store_constant(ValueItem&& value){
			values.push_back(value);
		}

		void store_constant(const ValueItem& value){
			values.push_back(value);
		}

		void load_current_opcode_label(size_t opcode_offset) {
			auto label = label_bind_map.find(opcode_offset - skip_count);
			if (label != label_bind_map.end())
				a.label_bind(label->second);
		}


		std::string* get_string_constant(const ValueIndexPos& value_index){
			if(value_index.pos != ValuePos::in_constants)
				return nullptr;
			ValueItem& value = values[value_index.index + static_map.size()];
			if(value.meta.vtype == VType::string)
				return (std::string*)value.getSourcePtr();
			else{
				values.push_back((std::string)value);
				return (std::string*)values.back().getSourcePtr();
			}
		}
		uint64_t get_size_constant(const ValueIndexPos& value_index) {
			if(value_index.pos != ValuePos::in_constants)
				return 0;
			return (uint64_t)values[value_index.index + static_map.size()];
		}

		struct DynamicCompiler{
			Compiler& compiler;
			DynamicCompiler(Compiler& compiler) : compiler(compiler) {}
			void noting();
			void create_saarr(const ValueIndexPos& value_index, uint32_t length);
			void remove(const ValueIndexPos& value_index);
			void copy(const ValueIndexPos& to, const ValueIndexPos& from);
			void move(const ValueIndexPos& to, const ValueIndexPos& from);

			void sum(const ValueIndexPos& to, const ValueIndexPos& src);
			void sub(const ValueIndexPos& to, const ValueIndexPos& src);
			void mul(const ValueIndexPos& to, const ValueIndexPos& src);
			void div(const ValueIndexPos& to, const ValueIndexPos& src);
			void rest(const ValueIndexPos& to, const ValueIndexPos& src);

			void bit_xor(const ValueIndexPos& to, const ValueIndexPos& src);
			void bit_and(const ValueIndexPos& to, const ValueIndexPos& src);
			void bit_or(const ValueIndexPos& to, const ValueIndexPos& src);
			void bit_not(const ValueIndexPos& to);
			void bit_left_shift(const ValueIndexPos& to, const ValueIndexPos& src);
			void bit_right_shift(const ValueIndexPos& to, const ValueIndexPos& src);

			void compare(const ValueIndexPos& a, const ValueIndexPos& b);
			void logic_not();
			void jump(uint64_t label, JumpCondition condition);

			void arg_set(const ValueIndexPos& src);
			void call(const ValueIndexPos& fn_symbol, CallFlags flags);
			void call(const ValueIndexPos& fn_symbol, CallFlags flags, const ValueIndexPos& res);
			void call_self();
			void call_self(const ValueIndexPos& res);
			void call_local(const ValueIndexPos& fn_symbol, CallFlags flags);
			void call_local(const ValueIndexPos& fn_symbol, CallFlags flags, const ValueIndexPos& res);

			void call_and_ret(const ValueIndexPos& fn_symbol, CallFlags flags);
			void call_self_and_ret();
			void call_local_and_ret(const ValueIndexPos& fn_symbol, CallFlags flags);

			
			void ret(const ValueIndexPos& value);
			void ret_take(const ValueIndexPos& value);
			void ret_noting();
			struct ArrayOperation{
				Compiler& compiler;
				ValueIndexPos array;
				OpArrFlags flags;
				ArrayOperation(Compiler& compiler, const ValueIndexPos& array, OpArrFlags flags) : compiler(compiler), array(array), flags(flags) {}
				void set(const ValueIndexPos& index, const ValueIndexPos& value);
				void insert(const ValueIndexPos& index, const ValueIndexPos& value);
				void push_end(const ValueIndexPos& value);
				void push_start(const ValueIndexPos& value);
				void insert_range(const ValueIndexPos& array2, const ValueIndexPos& index, const ValueIndexPos& from2, const ValueIndexPos& to2);
				void get(const ValueIndexPos& index, const ValueIndexPos& set);
				void take(const ValueIndexPos& index, const ValueIndexPos& set);
				void take_end(const ValueIndexPos& set);
				void take_start(const ValueIndexPos& set);
				void get_range(const ValueIndexPos& set, const ValueIndexPos& from, const ValueIndexPos& to);
				void take_range(const ValueIndexPos& set, const ValueIndexPos& from, const ValueIndexPos& to);
				void pop_end();
				void pop_start();
				void remove_item(const ValueIndexPos& index);
				void remove_range(const ValueIndexPos& from, const ValueIndexPos& to);
				void resize(const ValueIndexPos& new_size);
				void resize_default(const ValueIndexPos& new_size, const ValueIndexPos& default_value);
				void reserve_push_end(const ValueIndexPos& count);
				void reserve_push_start(const ValueIndexPos& count);
				void commit();
				void decommit(const ValueIndexPos& total_blocks);
				void remove_reserved();
				void size(const ValueIndexPos& set);
			};
			ArrayOperation arr_op(const ValueIndexPos& array, OpArrFlags flags){
				return ArrayOperation(compiler, array, flags);
			}
			void debug_break();
			void debug_force_break();
			void _throw(const ValueIndexPos& name, const ValueIndexPos& message);

			void as(const ValueIndexPos& in, VType as_type);
			void is(const ValueIndexPos& in, VType is_type);
			void store_bool(const ValueIndexPos& from);
			void load_bool(const ValueIndexPos& to);

			void insert_native(uint32_t len, uint8_t* native_code);

			void call_value_function(CallFlags flags, const ValueIndexPos& fn_symbol, const ValueIndexPos& structure_, ClassAccess access);
			void call_value_function(CallFlags flags, const ValueIndexPos& fn_symbol, const ValueIndexPos& structure_, ClassAccess access, const ValueIndexPos& res);
			
			void call_value_function_id(CallFlags flags, uint64_t id, const ValueIndexPos& structure_);
			void call_value_function_id(CallFlags flags, uint64_t id, const ValueIndexPos& structure_, const ValueIndexPos& res);
			

			void call_value_function_and_ret(CallFlags flags, const ValueIndexPos& fn_symbol, const ValueIndexPos& structure_, ClassAccess access);
			void call_value_function_id_and_ret(CallFlags flags, uint64_t id, const ValueIndexPos& structure_);
			
			void static_call_value_function(CallFlags flags, const ValueIndexPos& fn_symbol, const ValueIndexPos& structure_, ClassAccess access);
			void static_call_value_function(CallFlags flags, const ValueIndexPos& fn_symbol, const ValueIndexPos& structure_, ClassAccess access, const ValueIndexPos& res);
			
			void static_call_value_function_id(CallFlags flags, uint64_t id, const ValueIndexPos& structure_);
			void static_call_value_function_id(CallFlags flags, uint64_t id, const ValueIndexPos& structure_, const ValueIndexPos& res);
			

			void static_call_value_function_and_ret(CallFlags flags, const ValueIndexPos& fn_symbol, const ValueIndexPos& structure_, ClassAccess access);
			void static_call_value_function_id_and_ret(CallFlags flags, uint64_t id, const ValueIndexPos& structure_);
			


			void set_structure_value(const ValueIndexPos& value_name, ClassAccess access, const ValueIndexPos& structure_, const ValueIndexPos& value);
			void get_structure_value(const ValueIndexPos& value_name, ClassAccess access, const ValueIndexPos& structure_, const ValueIndexPos& to);

			void explicit_await(const ValueIndexPos& value);
			void generator_get(const ValueIndexPos& generator, const ValueIndexPos& to, const ValueIndexPos& result_index);

			void _yield(const ValueIndexPos& value);

			void handle_begin(uint64_t exception_scope);

			void handle_catch_0(uint64_t exception_scope, const std::vector<std::string>& catch_names);
			void handle_catch_1(uint64_t exception_scope, uint16_t exception_name_env_id);
			void handle_catch_2(uint64_t exception_scope, const std::vector<uint16_t>& exception_name_env_ids);
			void handle_catch_3(uint64_t exception_scope, const std::vector<std::string>& catch_names, const std::vector<uint16_t>& exception_name_env_ids);
			void handle_catch_4(uint64_t exception_scope);
			void handle_catch_5(uint64_t exception_scope, uint64_t local_fun_id, uint16_t enviro_slice_begin, uint16_t enviro_slice_end);
			void handle_catch_5(uint64_t exception_scope, const std::string& function_symbol, uint16_t enviro_slice_begin, uint16_t enviro_slice_end);

			void handle_finally(uint64_t exception_scope, uint64_t local_fun_id, uint16_t enviro_slice_begin, uint16_t enviro_slice_end);
			void handle_finally(uint64_t exception_scope, const std::string& function_symbol, uint16_t enviro_slice_begin, uint16_t enviro_slice_end);

			void handle_end(uint64_t exception_scope);

			void value_hold(uint64_t hold_scope, uint16_t env_id);
			void value_unhold(uint64_t hold_scope);

			void is_gc(const ValueIndexPos& value, const ValueIndexPos& set_bool);
			void is_gc(const ValueIndexPos& value);
			void to_gc(const ValueIndexPos& value);
			void from_gc(const ValueIndexPos& value);
			void localize_gc(const ValueIndexPos& value);

			void table_jump(TableJumpFlags flags, uint64_t too_large_label, uint64_t too_small_label, const std::vector<uint64_t>& labels, const ValueIndexPos& check_value);
			void xarray_slice(const ValueIndexPos& result, const ValueIndexPos& array, const ValueIndexPos& from, const ValueIndexPos& to);
			void xarray_slice(const ValueIndexPos& result, const ValueIndexPos& array, const ValueIndexPos& from);
			void xarray_slice(const ValueIndexPos& result, const ValueIndexPos& array);
			void xarray_slice(const ValueIndexPos& result, const ValueIndexPos& array, bool, const ValueIndexPos& to);

			void get_reference(const ValueIndexPos& result, const ValueIndexPos& value);
			void make_as_const(const ValueIndexPos& value);
			void remove_const_protect(const ValueIndexPos& value);
			void copy_unconst(const ValueIndexPos& set, const ValueIndexPos& from);
			void copy_unreference(const ValueIndexPos& set, const ValueIndexPos& from);
			void move_unreference(const ValueIndexPos& set, const ValueIndexPos& from);
			void remove_qualifiers(const ValueIndexPos& value);
		};

		DynamicCompiler dynamic(){
			return DynamicCompiler(*this);
		}

		struct StaticCompiler{
			Compiler& compiler;

			StaticCompiler(Compiler& compiler) : compiler(compiler) {}
			void remove(const ValueIndexPos& value_index, ValueMeta value_index_meta);
			void copy(const ValueIndexPos& to, ValueMeta to_meta, const ValueIndexPos& from, ValueMeta from_meta);
			void move(const ValueIndexPos& to, ValueMeta to_meta, const ValueIndexPos& from, ValueMeta from_meta);

			void sum(const ValueIndexPos& to, ValueMeta to_meta, const ValueIndexPos& src, ValueMeta src_meta);
			void sub(const ValueIndexPos& to, ValueMeta to_meta, const ValueIndexPos& src, ValueMeta src_meta);
			void mul(const ValueIndexPos& to, ValueMeta to_meta, const ValueIndexPos& src, ValueMeta src_meta);
			void div(const ValueIndexPos& to, ValueMeta to_meta, const ValueIndexPos& src, ValueMeta src_meta);
			void rest(const ValueIndexPos& to, ValueMeta to_meta, const ValueIndexPos& src, ValueMeta src_meta);

			void bit_xor(const ValueIndexPos& to, ValueMeta to_meta, const ValueIndexPos& src, ValueMeta src_meta);
			void bit_and(const ValueIndexPos& to, ValueMeta to_meta, const ValueIndexPos& src, ValueMeta src_meta);
			void bit_or(const ValueIndexPos& to, ValueMeta to_meta, const ValueIndexPos& src, ValueMeta src_meta);
			void bit_not(const ValueIndexPos& to, ValueMeta to_meta);
			void bit_left_shift(const ValueIndexPos& to, ValueMeta to_meta, const ValueIndexPos& src, ValueMeta src_meta);
			void bit_right_shift(const ValueIndexPos& to, ValueMeta to_meta, const ValueIndexPos& src, ValueMeta src_meta);

			void compare(const ValueIndexPos& a, ValueMeta a_meta, const ValueIndexPos& b, ValueMeta b_meta);

			void arg_set(const ValueIndexPos& src);
			void call(const ValueIndexPos& fn_symbol, ValueMeta fn_symbol_meta, CallFlags flags);
			void call(const ValueIndexPos& fn_symbol, ValueMeta fn_symbol_meta, CallFlags flags, const ValueIndexPos& res, ValueMeta res_meta);
			void call_self(const ValueIndexPos& res, ValueMeta res_meta);
			void call_local(const ValueIndexPos& fn_symbol, ValueMeta fn_symbol_meta, CallFlags flags);
			void call_local(const ValueIndexPos& fn_symbol, ValueMeta fn_symbol_meta, CallFlags flags, const ValueIndexPos& res, ValueMeta res_meta);

			void call_and_ret(const ValueIndexPos& fn_symbol, ValueMeta fn_symbol_meta, CallFlags flags);
			void call_local_and_ret(const ValueIndexPos& fn_symbol, ValueMeta fn_symbol_meta, CallFlags flags);

			struct ArrayOperation{
				Compiler& compiler;
				ValueIndexPos array;
				ValueMeta array_meta;
				OpArrFlags flags;
				ArrayOperation(Compiler& compiler, const ValueIndexPos& array, OpArrFlags flags, ValueMeta array_meta) : compiler(compiler), array(array), flags(flags), array_meta(array_meta) {}
				void set(const ValueIndexPos& index, const ValueIndexPos& value);
				void insert(const ValueIndexPos& index, const ValueIndexPos& value);
				void push_end(const ValueIndexPos& value);
				void push_start(const ValueIndexPos& value);
				void insert_range(const ValueIndexPos& array2, ValueMeta array2_meta, const ValueIndexPos& index, const ValueIndexPos& from2, const ValueIndexPos& to2);
				void get(const ValueIndexPos& index, const ValueIndexPos& set);
				void take(const ValueIndexPos& index, const ValueIndexPos& set);
				void take_end(const ValueIndexPos& set);
				void take_start(const ValueIndexPos& set);
				void get_range(const ValueIndexPos& set, const ValueIndexPos& from, const ValueIndexPos& to);
				void take_range(const ValueIndexPos& set, const ValueIndexPos& from, const ValueIndexPos& to);
				void pop_end();
				void pop_start();
				void remove_item(const ValueIndexPos& index);
				void remove_range(const ValueIndexPos& from, const ValueIndexPos& to);
				void resize(const ValueIndexPos& new_size);
				void resize_default(const ValueIndexPos& new_size, const ValueIndexPos& default_value);
				void reserve_push_end(const ValueIndexPos& count);
				void reserve_push_start(const ValueIndexPos& count);
				void commit();
				void decommit(const ValueIndexPos& total_blocks);
				void remove_reserved();
				void size(const ValueIndexPos& set);
			};
			ArrayOperation arr_op(const ValueIndexPos& array, OpArrFlags flags, ValueMeta array_meta);
			void _throw(const ValueIndexPos& name, ValueMeta name_meta, const ValueIndexPos& message, ValueMeta message_meta);

			void as(const ValueIndexPos& in, ValueMeta in_meta, VType as_type);
			void is(const ValueIndexPos& in, VType is_type);
			void store_bool(const ValueIndexPos& from, ValueMeta from_meta);
			void load_bool(const ValueIndexPos& to, ValueMeta to_meta);

			void call_value_function(CallFlags flags, const ValueIndexPos& fn_symbol, ValueMeta fn_symbol_meta, const ValueIndexPos& structure_, ClassAccess access);
			void call_value_function(CallFlags flags, const ValueIndexPos& fn_symbol, ValueMeta fn_symbol_meta, const ValueIndexPos& structure_, ClassAccess access, const ValueIndexPos& res, ValueMeta res_meta);
			
			void call_value_function_id(CallFlags flags, uint64_t id, const ValueIndexPos& structure_);
			void call_value_function_id(CallFlags flags, uint64_t id, const ValueIndexPos& structure_, const ValueIndexPos& res, ValueMeta res_meta);
			

			void call_value_function_and_ret(CallFlags flags, const ValueIndexPos& fn_symbol, ValueMeta fn_symbol_meta, const ValueIndexPos& structure_, ClassAccess access);
			void call_value_function_id_and_ret(CallFlags flags, uint64_t id, const ValueIndexPos& structure_);
			
			void static_call_value_function(CallFlags flags, const ValueIndexPos& fn_symbol, ValueMeta fn_symbol_meta, const ValueIndexPos& structure_, ClassAccess access);
			void static_call_value_function(CallFlags flags, const ValueIndexPos& fn_symbol, ValueMeta fn_symbol_meta, const ValueIndexPos& structure_, ClassAccess access, const ValueIndexPos& res, ValueMeta res_meta);
			
			void static_call_value_function_id(CallFlags flags, uint64_t id, const ValueIndexPos& structure_);
			void static_call_value_function_id(CallFlags flags, uint64_t id, const ValueIndexPos& structure_, const ValueIndexPos& res, ValueMeta res_meta);
			

			void static_call_value_function_and_ret(CallFlags flags, const ValueIndexPos& fn_symbol, ValueMeta fn_symbol_meta, const ValueIndexPos& structure_, ClassAccess access);
			void static_call_value_function_id_and_ret(CallFlags flags, uint64_t id, const ValueIndexPos& structure_);
			


			void set_structure_value(const ValueIndexPos& value_name, ClassAccess access, const ValueIndexPos& structure_, const ValueIndexPos& value, ValueMeta value_meta);
			void get_structure_value(const ValueIndexPos& value_name, ClassAccess access, const ValueIndexPos& structure_, const ValueIndexPos& to, ValueMeta to_meta);

			void explicit_await(const ValueIndexPos& value, ValueMeta value_meta);
			void generator_get(const ValueIndexPos& generator, const ValueIndexPos& to, ValueMeta to_meta, const ValueIndexPos& result_index);


			void is_gc(const ValueIndexPos& value, ValueMeta value_meta, const ValueIndexPos& set_bool, ValueMeta set_bool_meta);
			void is_gc(const ValueIndexPos& value, ValueMeta value_meta);

			void table_jump(TableJumpFlags flags, uint64_t too_large_label, uint64_t too_small_label, const std::vector<uint64_t>& labels, const ValueIndexPos& check_value);
			void xarray_slice(const ValueIndexPos& result, ValueMeta result_meta, const ValueIndexPos& array, ValueMeta array_meta, const ValueIndexPos& , ValueMeta from_meta, const ValueIndexPos& to, ValueMeta to_meta);
			void xarray_slice(const ValueIndexPos& result, ValueMeta result_meta, const ValueIndexPos& array, ValueMeta array_meta, const ValueIndexPos& from, ValueMeta from_meta);
			void xarray_slice(const ValueIndexPos& result, ValueMeta result_meta, const ValueIndexPos& array, ValueMeta array_meta);
			void xarray_slice(const ValueIndexPos& result, ValueMeta result_meta, const ValueIndexPos& array, ValueMeta array_meta, bool, const ValueIndexPos& to, ValueMeta to_meta);

			void copy_unconst(const ValueIndexPos& set, ValueMeta set_meta, const ValueIndexPos& from, ValueMeta from_meta);
			void copy_unreference(const ValueIndexPos& set, ValueMeta set_meta, const ValueIndexPos& from, ValueMeta from_meta);
			void move_unreference(const ValueIndexPos& set, ValueMeta set_meta, const ValueIndexPos& from, ValueMeta from_meta);
		};

		StaticCompiler static_(){
			return StaticCompiler(*this);
		}
	};
}
#endif /* SRC_RUN_TIME_ASM_COMPILER */
