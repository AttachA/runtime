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
	call_self,
	call_local,
	call_and_ret,
	call_self_and_ret,
	call_local_and_ret,
	ret,
	ret_noting,
	copy,
	move,
	arr_op,
	debug_break,
	force_debug_break,
	throw_ex,
	as,
	is,
	store_bool,//store bool from value for if statements, set false if type is noting, numeric types is zero and containers is empty, if another then true
	load_bool//used if need save equaluty result, set numeric type as 1 or 0
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
		code = (Opcode)(ini & 0x3F);
		is_gc_mode = ini & 0x40;
		static_mode = ini & 0x80;
	}
	Command(Opcode op, bool gc_mode = false, bool static_mode = false) {
		code = op;
		is_gc_mode = gc_mode;
		static_mode = static_mode;
	}
	Opcode code : 6;
	uint8_t is_gc_mode : 1;
	uint8_t static_mode : 1;

	uint8_t toCmd() {
		uint8_t res = (uint8_t)code;
		res |= is_gc_mode << 5;
		res |= is_gc_mode << 6;
		res |= static_mode << 7;
		return res;
	}
};

union CallFlags {
	struct {
		uint8_t in_memory : 1;
		uint8_t async_mode : 1;
		uint8_t use_result : 1;
		uint8_t except_catch : 1;
		uint8_t unused : 4;
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
	raw_arr_i8,
	raw_arr_i16,
	raw_arr_i32,
	raw_arr_i64,
	raw_arr_ui8,
	raw_arr_ui16,
	raw_arr_ui32,
	raw_arr_ui64,
	raw_arr_flo,
	raw_arr_doub,
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
		uint32_t val_len;
	};

	ValueMeta() = default;
	ValueMeta(const ValueMeta& copy) = default;
	ValueMeta(VType ty, bool gc, bool editable, uint32_t length = 0) { vtype = ty; use_gc = gc; allow_edit = editable; val_len = length; }
	ValueMeta(size_t enc) { encoded = enc; }
};

struct ValueItem {
	void* val;
	ValueMeta meta;
	ValueItem() {
		val = nullptr;
		meta.encoded = 0;
	}
	ValueItem(ValueItem&& move) noexcept {
		val = move.val;
		meta = move.meta;
		move.val = nullptr;
	}
	ValueItem(void* vall, ValueMeta meta);
	ValueItem(void* vall, ValueMeta meta, bool no_copy);
	ValueItem(const ValueItem&);
	ValueItem& operator=(const ValueItem& copy);
	ValueItem& operator=(ValueItem&& copy) noexcept;
	~ValueItem();
};

typedef ValueItem* (*Enviropment)(void** enviro, list_array<ValueItem>* args);
typedef ValueItem* (*AttachACXX)(list_array<ValueItem>* arguments);



struct ClassValue {
	std::unordered_map<std::string, ValueItem> private_vals;
	std::unordered_map<std::string, ValueItem>  public_vals;

	std::unordered_map<std::string, class FuncEnviropment*> private_fun;
	std::unordered_map<std::string, class FuncEnviropment*> public_fun;
};
struct StructValue {
	std::unordered_map<std::string, ValueItem> private_vals;
	std::unordered_map<std::string, ValueItem>  public_vals;
};


using ProxyClassGetter = ValueItem(*)(void*);
using ProxyClassSeter = void(*)(void*, ValueItem&);
using ProxyClassDestructor = void(*)(void*);

struct ProxyClassDeclare {
	std::unordered_map<std::string, ProxyClassGetter> value_geter;
	std::unordered_map<std::string, ProxyClassSeter> value_seter;
	std::unordered_map<std::string, class FuncEnviropment*> public_fun;
	ProxyClassDestructor destructor;
};
struct ProxyClassValueLgrItem {
	ProxyClassDeclare* declare_ty = nullptr;
	void* class_ptr = nullptr;
};