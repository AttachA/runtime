#include "run_time_compiler.hpp"
#include "tasks.hpp"
#include <asmjit/core/cpuinfo.h>
#include "tools.hpp"
using namespace run_time;
namespace constant_calc {
	std::string arch() {
		switch (asmjit::Arch::kHost) {
		default:
		case asmjit::Arch::kUnknown: return "Unknown";
		case asmjit::Arch::kX86: return "X86";
		case asmjit::Arch::kX64: return "X64";
		case asmjit::Arch::kRISCV32: return "RISCv32";
		case asmjit::Arch::kRISCV64: return "RISCv64";
		case asmjit::Arch::kARM: return "ARM";
		case asmjit::Arch::kAArch64: return "AArch64";
		case asmjit::Arch::kThumb: return "ARM_Thumb";
		case asmjit::Arch::kMIPS32_LE: return "MIPS32_LE";
		case asmjit::Arch::kMIPS64_LE: return "MIPS64_LE";
		case asmjit::Arch::kARM_BE: return "ARM_BE";
		case asmjit::Arch::kAArch64_BE: return "AArch64_BE";
		case asmjit::Arch::kThumb_BE: return "ARM_Thumb_BE";
		case asmjit::Arch::kMIPS32_BE: return "MIPS32_BE";
		case asmjit::Arch::kMIPS64_BE: return "MIPS64_BE";
		}
	}
	uint8_t arch_id() {
		switch (asmjit::Arch::kHost) {
		default:
		case asmjit::Arch::kUnknown: return 0;
		case asmjit::Arch::kX86: return 1;
		case asmjit::Arch::kX64: return 2;
		case asmjit::Arch::kRISCV32: return 3;
		case asmjit::Arch::kRISCV64: return 4;
		case asmjit::Arch::kARM: return 5;
		case asmjit::Arch::kAArch64: return 6;
		case asmjit::Arch::kThumb: return 7;
		case asmjit::Arch::kMIPS32_LE: return 8;
		case asmjit::Arch::kMIPS64_LE: return 9;
		case asmjit::Arch::kARM_BE: return 10;
		case asmjit::Arch::kAArch64_BE: return 11;
		case asmjit::Arch::kThumb_BE: return 12;
		case asmjit::Arch::kMIPS32_BE: return 13;
		case asmjit::Arch::kMIPS64_BE: return 14;
		}
	}
}

std::unordered_map<std::string, ValueItem> constants{
	{"arch",constant_calc::arch()},
	{"arch_id",constant_calc::arch_id()}
};

inline ValueItem& getValue(uint16_t val, list_array<ValueItem>& its) {
	if (its.size() <= val)
		its.resize(size_t(val)+1);
	return its[val];
}

std::tuple<std::vector<uint8_t>,uint16_t,bool,bool> build(list_array<ValueItem>& its, const std::vector<uint8_t>& code) {
	list_array<ValueItem> local;
	size_t ii = 0;
	std::vector<uint8_t> fn_code;
	list_array<ValueItem>* arg_reg = nullptr;
	size_t code_len = code.size();
	RFLAGS flags;
	uint16_t fn_code_val_count = 0;
	bool fn_code_can_be_unloaded = true;
	while (true) {
		if (code_len <= ii)
			throw CompileTimeException("Reached end of builder function");
		Opcode opcodes = (Opcode)code[ii];
		switch (opcodes) {
		case Opcode::noting:break;
		case Opcode::set:
			uint16_t value_index = readData<uint16_t>(code, code_len, ii);
			switch (readData<VType>(code, code_len, ii)) {
			case VType::i8:
				getValue(value_index, its) = readData<int8_t>(code, code_len, ii);
				break;
			case VType::ui8:
				getValue(value_index, its) = readData<uint8_t>(code, code_len, ii);
				break;
			case VType::i16:
				getValue(value_index, its) = readData<int16_t>(code, code_len, ii);
				break;
			case VType::ui16:
				getValue(value_index, its) = readData<uint16_t>(code, code_len, ii);
				break;
			case VType::i32:
				getValue(value_index, its) = readData<int32_t>(code, code_len, ii);
				break;
			case VType::ui32:
				getValue(value_index, its) = readData<uint32_t>(code, code_len, ii);
				break;
			case VType::flo:
				getValue(value_index, its) = readData<float>(code, code_len, ii);
				break;
			case VType::i64:
				getValue(value_index, its) = readData<int64_t>(code, code_len, ii);
				break;
			case VType::ui64:
				getValue(value_index, its) = readData<uint64_t>(code, code_len, ii);
				break;
			case VType::doub:
				getValue(value_index, its) = readData<double>(code, code_len, ii);
				break;
			case VType::raw_arr_i8:
				getValue(value_index, its) = ValueItem(readRawArray<int8_t>(code, code_len, ii, readLen(code, code_len, ii)), VType::raw_arr_i8, true);
				break;
			case VType::raw_arr_ui8:
				getValue(value_index, its) = ValueItem(readRawArray<uint8_t>(code, code_len, ii, readLen(code, code_len, ii)), VType::raw_arr_ui8, true);
				break;
			case VType::raw_arr_i16:
				getValue(value_index, its) = ValueItem(readRawArray<int16_t>(code, code_len, ii, readLen(code, code_len, ii)), VType::raw_arr_i16, true);
				break;
			case VType::raw_arr_ui16:
				getValue(value_index, its) = ValueItem(readRawArray<uint16_t>(code, code_len, ii, readLen(code, code_len, ii)), VType::raw_arr_ui16, true);
				break;
			case VType::raw_arr_i32:
				getValue(value_index, its) = ValueItem(readRawArray<int32_t>(code, code_len, ii, readLen(code, code_len, ii)), VType::raw_arr_i32, true);
				break;
			case VType::raw_arr_ui32:
				getValue(value_index, its) = ValueItem(readRawArray<uint32_t>(code, code_len, ii, readLen(code, code_len, ii)), VType::raw_arr_ui32, true);
				break;
			case VType::raw_arr_flo: 
				getValue(value_index, its) = ValueItem(readRawArray<float>(code, code_len, ii, readLen(code, code_len, ii)), VType::raw_arr_flo, true);
				break;
			case VType::raw_arr_i64:
				getValue(value_index, its) = ValueItem(readRawArray<int64_t>(code, code_len, ii, readLen(code, code_len, ii)), VType::raw_arr_i64, true);
				break;
			case VType::raw_arr_ui64:
				getValue(value_index, its) = ValueItem(readRawArray<uint64_t>(code, code_len, ii, readLen(code, code_len, ii)), VType::raw_arr_ui64, true);
				break;
			case VType::raw_arr_doub:
				getValue(value_index, its) = ValueItem(readRawArray<double>(code, code_len, ii, readLen(code, code_len, ii)), VType::raw_arr_doub, true);
				break;
			case VType::string:
				getValue(value_index, its) = readString(code, code_len, ii);
				break;
			default:
				break;
			}
			break;
		case Opcode::remove:
			getValue(readData<uint16_t>(code, code_len, ii), its) = VType::noting;
			break;
		case Opcode::sum: {
			auto* v0 = &getValue(readData<uint16_t>(code, code_len, ii), its).val;
			DynSum(v0, &getValue(readData<uint16_t>(code, code_len, ii), its).val);
			break;
		}
		case Opcode::minus: {
			auto* v0 = &getValue(readData<uint16_t>(code, code_len, ii), its).val;
			DynMinus(v0, &getValue(readData<uint16_t>(code, code_len, ii), its).val);
			break;
		}
		case Opcode::div: {
			auto* v0 = &getValue(readData<uint16_t>(code, code_len, ii), its).val;
			DynDiv(v0, &getValue(readData<uint16_t>(code, code_len, ii), its).val);
			break;
		}
		case Opcode::mul: {
			auto* v0 = &getValue(readData<uint16_t>(code, code_len, ii), its).val;
			DynMul(v0, &getValue(readData<uint16_t>(code, code_len, ii), its).val);
			break;
		}
		case Opcode::rest: {
			auto* v0 = &getValue(readData<uint16_t>(code, code_len, ii), its).val;
			DynRest(v0, &getValue(readData<uint16_t>(code, code_len, ii), its).val);
			break;
		}
		case Opcode::bit_xor: {
			auto* v0 = &getValue(readData<uint16_t>(code, code_len, ii), its).val;
			DynBitXor(v0, &getValue(readData<uint16_t>(code, code_len, ii), its).val);
			break;
		}
		case Opcode::bit_or: {
			auto* v0 = &getValue(readData<uint16_t>(code, code_len, ii), its).val;
			DynBitOr(v0, &getValue(readData<uint16_t>(code, code_len, ii), its).val);
			break;
		}
		case Opcode::bit_and: {
			auto* v0 = &getValue(readData<uint16_t>(code, code_len, ii), its).val;
			DynBitAnd(v0, &getValue(readData<uint16_t>(code, code_len, ii), its).val);
			break;
		}
		case Opcode::bit_not:
			DynBitNot(&getValue(readData<uint16_t>(code, code_len, ii), its).val);
			break;
		case Opcode::log_not:
			flags.zero ^= flags.zero;
			flags.carry ^= flags.carry;
			break;
		case Opcode::compare:
			flags = compare(flags, &getValue(readData<uint16_t>(code, code_len, ii), its).val, &getValue(readData<uint16_t>(code, code_len, ii), its).val);
			break;
		case Opcode::jump:
			uint64_t to_jump = readData<uint64_t>(code, code_len, ii);
			switch (readData<JumpCondition>(code, code_len, ii)){
			default:
			case JumpCondition::no_condition: {
				ii = to_jump;
				break;
			}
			case JumpCondition::is_equal: {
				if(flags.zero)
					ii = to_jump;
				break;
			}
			case JumpCondition::is_not_equal: {
				if (!flags.zero)
					ii = to_jump;
				break;
			}
			case JumpCondition::is_more: {
				if (!flags.zero && !flags.carry)
					ii = to_jump;
				break;
			}
			case JumpCondition::is_lower: {
				if (!flags.zero && flags.carry)
					ii = to_jump;
				break;
			}
			case JumpCondition::is_more_or_eq: {
				if (!flags.carry)
					ii = to_jump;
				break;
			}
			case JumpCondition::is_lower_or_eq: {
				if (flags.zero || flags.carry)
					ii = to_jump;
				break;
			}
			}
			break;
		case Opcode::call: {
			CallFlags cflags;
			cflags.encoded = readData<uint8_t>(code, code_len, ii);
			ValueItem* r;

			std::string fnn = cflags.in_memory ?
				(std::string)getValue(readData<uint16_t>(code, code_len, ii), its) :
				readString(code, code_len, ii);

			r = cflags.except_catch ?
				FuncEnviropment::CallFunc_catch(fnn, arg_reg, cflags.async_mode) :
				FuncEnviropment::CallFunc(fnn, arg_reg, cflags.async_mode);

			if (cflags.use_result)
				getValue(readData<uint16_t>(code, code_len, ii), its) = std::move(*r);
			delete r;
			break;
		}
		case Opcode::copy:
			getValue(readData<uint16_t>(code, code_len, ii), its) = getValue(readData<uint16_t>(code, code_len, ii), its);
			break;
		case Opcode::move:
			getValue(readData<uint16_t>(code, code_len, ii), its) = std::move(getValue(readData<uint16_t>(code, code_len, ii), its));
			break;
		case Opcode::arr_op:
			break;
		case Opcode::debug_break:
		case Opcode::force_debug_break:
			__debugbreak();
			break;
		case Opcode::throw_ex:
			bool in_memory = readData<bool>(code, code_len, ii);
			if (in_memory) {
				std::string exname = (std::string)getValue(readData<uint16_t>(code, code_len, ii), its);
				throw AException(exname, (std::string)getValue(readData<uint16_t>(code, code_len, ii), its));
			}
			else {
				std::string exname = readString(code, code_len, ii);
				throw AException(exname, readString(code, code_len, ii));
			}
		case Opcode::as: {
			uint16_t vid = readData<uint16_t>(code, code_len, ii);
			asValue(&getValue(vid, its).val, (readData<VType>(code, code_len, ii)));
			break;
		}
		case Opcode::is:
			break;
		case Opcode::store_bool:
			break;
		case Opcode::load_bool:
			break;
		case Opcode::make_inline_call: {





			break;
		}
			break;
		case Opcode::casm: {
			//TO-DO
			break;
		}
		case Opcode::inline_native: {
			union {
				uint16_t len;
				struct {
					uint8_t heigh;
					uint8_t low;
				};
			};
			len = readData<uint16_t>(code, code_len, ii);
			fn_code.push_back((uint8_t)Opcode::inline_native);
			fn_code.push_back(low);
			fn_code.push_back(heigh);
			for (uint16_t i = 0; i < len; i++)
				fn_code.push_back(readData<uint8_t>(code, code_len, ii));
			break;
		}
		default:
			throw CompileTimeException("Invalid opcode");
		}
	}
}
