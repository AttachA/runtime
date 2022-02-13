#pragma once
#include <cstdint>
#include <unordered_map>
enum class Opcode : uint8_t {
	noting,
	set,
	remove,

	sum,
	minus,
	div,
	mul,
	bit_xor,
	bit_or,
	bit_and,
	bit_not,
	log_not,
	compare,
	jump,
	arg_set,
	call,
	ret,
	call_and_ret,
	copy,
	move,
	arr_op,
	debug_break,
	force_debug_break,
	throw_ex,
};
enum class JumpCondition {
	no_condition,
	is_equal,
	is_not_equal,
	is_more,
	is_lower,
	is_lower_or_eq,
	is_more_or_eq
};
struct Command {
	Command(uint8_t ini) {
		code = (Opcode)(ini & 0x1F);
		is_gc_mode = ini & 0x20;
		static_mode = ini & 0x40;
		except_catch = ini & 0x80;
	}
	Command(Opcode op, bool gc_mode = false, bool static_mode = false, bool catch_except = false) {
		code = op;
		is_gc_mode = gc_mode;
		static_mode = static_mode;
		except_catch = catch_except;
	}
	Opcode code : 5;
	uint8_t is_gc_mode : 1;
	uint8_t static_mode : 1;
	uint8_t except_catch : 1;

	uint8_t toCmd() {
		uint8_t res = (uint8_t)code;
		res |= is_gc_mode << 5;
		res |= static_mode << 6;
		res |= except_catch << 7;
		return res;
	}
};

union CallFlags {
	struct {
		uint8_t in_memory : 1;
		uint8_t async_mode : 1;
		uint8_t use_result : 1;
		uint8_t unused : 5;
	};
	uint8_t encoded = 0;
};


struct RFLAGS {
	uint16_t unused_000000 : 1;
	uint16_t nt : 1;
	uint16_t iopl : 1;
	uint16_t overflow : 1;
	uint16_t direction : 1;
	uint16_t ief : 1;
	uint16_t tf : 1;
	uint16_t sign_f : 1;
	uint16_t zero : 1;
	uint16_t unused_000001 : 1;
	uint16_t auxiliary_carry : 1;
	uint16_t unused_000002 : 1;
	uint16_t parity : 1;
	uint16_t unused_000003 : 1;
	uint16_t carry : 1;
};

enum class VType : uint8_t {
	noting,
	i8,
	i16,
	i32,
	i64,
	ui8,
	ui16,
	ui32,
	ui64,
	flo,
	doub,
	uarr,
	string,
	async_res,
	undefined_ptr,
	except_value,//default from except call


	// 
	// class_define
	//	[arr<VType>, arr<func>]
	// 
	// structure {arr<VType>, [values]}
	// class {definer, [values]}
	// morph {definer, [vtable, values]}
	// 
};
union ValueMeta {
	size_t encoded;
	struct {
		VType vtype;
		uint8_t use_gc : 1;
		uint8_t allow_edit : 1;
	} decoded;

	ValueMeta() = default;
	ValueMeta(const ValueMeta& copy) = default;
	ValueMeta(VType ty, bool gc, bool editable) { decoded.vtype = ty; decoded.use_gc = gc; decoded.allow_edit = editable; }
};
struct ArrItem {
	void* val;
	ValueMeta meta;
	ArrItem() {
		val = nullptr;
		meta.encoded = 0;
	}
	ArrItem(ArrItem&& move) noexcept {
		val = move.val;
		meta = move.meta;
		move.val = nullptr;
	}
	ArrItem(void* vall, ValueMeta meta);
	ArrItem(const ArrItem&);
	ArrItem& operator=(const ArrItem& copy);
	ArrItem& operator=(ArrItem&& copy) noexcept;
	~ArrItem();
};

struct FuncRes {
	FuncRes() { value = nullptr; meta = 0; }
	FuncRes(ValueMeta met, void* val) { value = val; meta = *reinterpret_cast<size_t*>(&met); }
	void* value;
	size_t meta;
	~FuncRes();
};


struct ClassValue {
	std::unordered_map<std::string, ArrItem> private_vals;
	std::unordered_map<std::string, ArrItem>  public_vals;

	std::unordered_map<std::string, void*> private_fun;
	std::unordered_map<std::string, void*> private_fun;
};
struct StructValue {
	std::unordered_map<std::string, ArrItem> private_vals;
	std::unordered_map<std::string, ArrItem>  public_vals;
};


struct CompressedClassValue {
	void* class_ptr = nullptr;
	std::unordered_map<std::string, ArrItem> private_vals;
	std::unordered_map<std::string, ArrItem>  public_vals;

	std::unordered_map<std::string, void*> private_fun;
	std::unordered_map<std::string, void*> private_fun;
};