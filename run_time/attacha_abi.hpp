#pragma once
#include "../run_time.hpp"
#include "link_garbage_remover.hpp"
#include "attacha_abi_structs.hpp"
#include "cxxException.hpp"


void ignoredAsyncGC();
void ignoredAsync(FuncRes* fres);

bool needAlloc(VType type);











bool calc_safe_deph_arr(void* ptr);

template<class T>
void defaultDestructor(void* a) {
	delete (T*)a;
}
template<class T>
void Allocate(void** a) {
	*a = new T();
}


VType valueType(void** value);



void universalFree(void** value, ValueMeta meta);
void universalRemove(void** value);
void universalAlloc(void** value, ValueMeta meta);

void initEnviropement(void** res, uint32_t vals_count);
void removeEnviropement(void** env, uint16_t vals_count);
void removeArgsEnviropement(list_array<ArrItem>* env);
char* getStrBegin(std::string* str);
void throwInvalidType();

auto gcCall(lgr* gc, list_array<ArrItem>* args, bool async_mode);
FuncRes* getAsyncFuncRes(void* val);
void getFuncRes(void** value, FuncRes* f_res);
FuncRes* buildRes(void** value);


void getAsyncResult(void*& value, ValueMeta& meta);
void* copyValue(void*& val, ValueMeta& meta);
void** copyEnviropement(void** env, uint16_t env_it_count);


void** preSetValue(void** value, ValueMeta set_meta, bool match_gc_dif);
void*& getValue(void*& value, ValueMeta& meta);
void*& getValue(void** value);
void* getSpecificValue(void** value, VType typ);
void** getSpecificValueLink(void** value, VType typ);

void** getValueLink(void** value);

bool is_integer(VType typ);
bool integer_unsigned(VType typ);

//return equal,lower bool result
std::pair<bool, bool> compareValue(VType cmp1, VType cmp2, void* val1, void* val2);
RFLAGS compare(RFLAGS old, void** value_1, void** value_2);

void copyEnviropement(void** env, uint16_t env_it_count, void*** res);





namespace ABI_IMPL {

	template <class T>
	T Vcast(void*& val, ValueMeta meta) {
		switch (meta.decoded.vtype)
		{
		case VType::noting:
			return T();
		case VType::i8:
			return (T)reinterpret_cast<int8_t&>(val);
		case VType::i16:
			return (T)reinterpret_cast<int16_t&>(val);
		case VType::i32:
			return (T)reinterpret_cast<int32_t&>(val);
		case VType::i64:
			return (T)reinterpret_cast<int64_t&>(val);
		case VType::ui8:
			return (T)reinterpret_cast<uint8_t&>(val);
		case VType::ui16:
			return (T)reinterpret_cast<uint16_t&>(val);
		case VType::ui32:
			return (T)reinterpret_cast<uint32_t&>(val);
		case VType::ui64:
			return (T)reinterpret_cast<uint64_t&>(val);
		case VType::flo:
			return (T)reinterpret_cast<float&>(val);
		case VType::doub:
			return (T)reinterpret_cast<double&>(val);
		case VType::uarr:
			throw InvalidCast("Fail cast uarr");
		case VType::string:
			try {
				return (T)std::stol(reinterpret_cast<std::string&>(val));
			}
			catch (const std::exception& ex) {
				throw InvalidCast("Fail cast string to long");
			}
		case VType::undefined_ptr:
			return (T)reinterpret_cast<size_t&>(val);
		default:
			throw InvalidCast("Fail cast undefined type");
		}
	}
	
	std::string Scast(void*& val, ValueMeta meta);
}





void DynSum(void** val0, void** val1);
void DynMinus(void** val0, void** val1);
void DynMul(void** val0, void** val1);
void DynDiv(void** val0, void** val1);


void DynBitXor(void** val0, void** val1);
void DynBitOr(void** val0, void** val1);
void DynBitAnd(void** val0, void** val1);
void DynBitNot(void** val0);

void* AsArg(void** val);
void AsArr(void** val);


namespace exception_abi {
	bool is_except(void** val);
	void ignore_except(void** val);
	void continue_unwind(void** val);
	void call_except_handler(void** val, bool(*func_symbol)(void** val), bool ignore_fault = true);

	struct jump_handle_except {
		std::string type_name;
		ptrdiff_t jump_off;
	};
	ptrdiff_t switch_jump_handle_except(void** val, jump_handle_except* handlers, size_t handlers_c);


	template<class _FN, class ...Args>
	FuncRes* catchCall(_FN func, Args... args) {
		FuncRes* res;
		try {
			return func(args...);
		}
		catch (...) {
			try {
				res = new FuncRes();
			}
			catch (const std::bad_alloc& ex) {
				throw EnviropmentRuinException();
			}
			ValueMeta meta = 0;
			meta.decoded.vtype = VType::except_value;
			meta.decoded.allow_edit = true;
			meta.decoded.use_gc = false;

			try {
				res->value = new std::exception_ptr(std::current_exception());
			}
			catch (const std::bad_alloc& ex) {
				throw EnviropmentRuinException();
			}
			res->meta = meta.encoded;
		}
		restore_stack_fault();
		return res;
	}
}








size_t getSize(void** value);

//TO-DO
void* callNative(void* func_ptr, void** args, size_t args_c);
