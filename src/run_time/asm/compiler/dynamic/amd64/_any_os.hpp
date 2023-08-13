#include "../../compiler_include.hpp"
namespace art {
	inline void _inlineReleaseUnused(CASM& a, creg64 reg) {
		auto lab = a.newLabel();
		a.test(reg, reg);
		a.jmp_zero(lab);
		BuildCall b(a, 1);
		b.addArg(reg);
		b.finalize(defaultDestructor<ValueItem>);
		a.label_bind(lab);
	}

	void Compiler::DynamicCompiler::jump(uint64_t label_id, JumpCondition condition) {
		auto &label = compiler.resolve_label(label_id);
		switch (condition) {
		default:
		case JumpCondition::no_condition:
			compiler.a.jmp(label);
			break;
		case JumpCondition::is_zero:
			compiler.a.jmp_zero(label);
			break;
		case JumpCondition::is_equal:
			compiler.a.jmp_equal(label);
			break;
		case JumpCondition::is_not_equal:
			compiler.a.jmp_not_equal(label);
			break;
		case JumpCondition::is_unsigned_more:
			compiler.a.jmp_unsigned_more(label);
			break;
		case JumpCondition::is_unsigned_lower:
			compiler.a.jmp_unsigned_lower(label);
			break;
		case JumpCondition::is_unsigned_more_or_eq:
			compiler.a.jmp_unsigned_more_or_eq(label);
			break;
		case JumpCondition::is_unsigned_lower_or_eq:
			compiler.a.jmp_unsigned_lower_or_eq(label);
			break;
		case JumpCondition::is_signed_more:
			compiler.a.jmp_signed_more(label);
			break;
		case JumpCondition::is_signed_lower:
			compiler.a.jmp_signed_lower(label);
			break;
		case JumpCondition::is_signed_more_or_eq:
			compiler.a.jmp_signed_more_or_eq(label);
			break;
		case JumpCondition::is_signed_lower_or_eq:
			compiler.a.jmp_signed_lower_or_eq(label);
			break;
		}
	}

	inline void call_fun_string(CASM& a, std::string* fnn, bool is_async, list_array<art::shared_ptr<FuncEnvironment>>& used_environs) {
		art::shared_ptr<FuncEnvironment> fn = FuncEnvironment::environment(*fnn);
		used_environs.push_back(fn);
		if (is_async) {
			BuildCall b(a, 4);
			b.addArg(fnn);
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


	void Compiler::DynamicCompiler::create_saarr(const ValueIndexPos& value_index, uint32_t length) { CASM& a = compiler.a;
		BuildCall b(a, 1);
		b.lea_valindex({compiler.static_map, compiler.values}, value_index);
		b.finalize(helper_functions::valueDestructDyn);
		a.mov(resr, ValueMeta(VType::saarr, false, true, length).encoded);
		a.mov_valindex_meta({compiler.static_map, compiler.values},value_index, resr);

		a.stackIncrease(length * sizeof(ValueItem));
		b.setArguments(2);
		b.addArg(stack_ptr);
		b.addArg(length*2);
		b.finalize(helper_functions::prepareStack);
		a.mov_valindex({compiler.static_map, compiler.values}, value_index, resr);
	}

	void Compiler::DynamicCompiler::compare(const ValueIndexPos& a_v, const ValueIndexPos& b_v) {  CASM& a = compiler.a;
		a.push_flags();
		a.pop(argr0_16);

		BuildCall b(a, 3);
		b.addArg(argr0_16);
		b.lea_valindex({compiler.static_map, compiler.values}, a_v);
		b.lea_valindex({compiler.static_map, compiler.values}, b_v);
		b.finalize(art::compare);
		a.push(resr_16);
		a.pop_flags();
	}

	void Compiler::DynamicCompiler::logic_not() {
		compiler.a.load_flag8h();
		compiler.a.xor_(resr_8h, RFLAGS::bit::zero & RFLAGS::bit::carry);
		compiler.a.store_flag8h();
	}

	void Compiler::DynamicCompiler::is(const ValueIndexPos &in, VType is_type) {
		compiler.a.mov_valindex_meta({compiler.static_map, compiler.values}, resr, in);
		compiler.a.mov(argr0_8l, is_type);
		compiler.a.cmp(resr_8l, argr0_8l);
	}

	void Compiler::DynamicCompiler::store_bool(const ValueIndexPos &from) {
		BuildCall b(compiler.a, 1);
		b.lea_valindex({compiler.static_map, compiler.values}, from);
		b.finalize(isTrueValue);
		compiler.a.load_flag8h();
		compiler.a.shift_left(resr_8l, RFLAGS::off_left::zero);
		compiler.a.or_(resr_8h, resr_8l);
		compiler.a.store_flag8h();
	}

	void Compiler::DynamicCompiler::load_bool(const ValueIndexPos &to) {
		compiler.a.load_flag8h();
		compiler.a.and_(resr_8h, RFLAGS::bit::zero);
		compiler.a.store_flag8h();
		BuildCall b(compiler.a, 2);
		b.addArg(resr_8h);
		b.lea_valindex({compiler.static_map, compiler.values}, to);
		b.finalize(setBoolValue);
	}

	void Compiler::DynamicCompiler::debug_break() {
		if (compiler.in_debug) compiler.a.int3();
	}

	void Compiler::DynamicCompiler::debug_force_break() {
		compiler.a.int3();
	}

	template<bool direction = true>
	void _dynamic_table_jump_bound_check(CASM& a, Compiler& compiler, TableJumpFlags flags, TableJumpCheckFailAction action, uint64_t table_size, uint64_t index, const char* action_name) {
		static const ValueItem exception_name( "IndexError");
		static const ValueItem exception_description("Index out of range");
		switch (action) {
		case TableJumpCheckFailAction::jump_specified:
			if constexpr (direction) {
				a.cmp(resr, table_size);
				if (flags.is_signed)
					a.jmp_signed_more_or_eq(compiler.resolve_label(index));
				else
					a.jmp_unsigned_more_or_eq(compiler.resolve_label(index));
			}
			else {
				assert(flags.is_signed);
				a.cmp(resr, 0);
				a.jmp_signed_lower(compiler.resolve_label(index));
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
			b.finalize(helper_functions::throwEx);
			a.label_bind(no_exception_label);
			break;
		}
		case TableJumpCheckFailAction::unchecked:
			return;
		default:
			throw InvalidArguments(std::string("Unsupported table jump check fail action for ") + action_name + ": " + enum_to_string(action));
		}
	}

	void Compiler::DynamicCompiler::table_jump(TableJumpFlags flags, uint64_t fail_too_large, uint64_t fail_too_small, const std::vector<uint64_t> &labels, const ValueIndexPos &check_value) {
		std::vector<asmjit::Label> table;

		table.reserve(labels.size());
		for (auto label : labels)
			table.push_back(compiler.resolve_label(label));

		auto table_label = compiler.a.add_table(table);
		BuildCall b(compiler.a, 1);
		b.lea_valindex({compiler.static_map, compiler.values}, check_value);
		if (flags.is_signed)
			b.finalize(&ValueItem::operator int64_t);
		else
			b.finalize(&ValueItem::operator uint64_t);

		_dynamic_table_jump_bound_check<true>(compiler.a, compiler, flags, flags.too_large, labels.size(), fail_too_large, "too_large");
		if (flags.is_signed)
			_dynamic_table_jump_bound_check<false>(compiler.a, compiler, flags, flags.too_small, labels.size(), fail_too_small, "too_small");
		compiler.a.lea(argr0, table_label);
		compiler.a.mov(resr, argr0, resr, 3, 0, 8);
		compiler.a.jmp(resr);
	}

	void Compiler::DynamicCompiler::is_gc(const ValueIndexPos &value) {
		BuildCall b(compiler.a, 0);
		b.mov_valindex({compiler.static_map, compiler.values}, value);
		b.finalize(&ValueItem::is_gc);
		compiler.a.test(resr_8l, resr_8l);
	}
}