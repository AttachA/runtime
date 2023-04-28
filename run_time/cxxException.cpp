// Copyright Danyil Melnytskyi 2022-2023
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include "cxxException.hpp"
#include <windows.h>
#include <string>
namespace except_abi {
	namespace {
		struct ExThrowInfo {
			uint32_t attributes;
			int32_t clean_up;
			int32_t forward_compat;
			int32_t catchable_type_array;
		};
		struct ExCatchableTypeArray {
			uint32_t catchable_types;
			// It is variable size but we only need the first element of this array
			uint32_t array_of_catchable_types(size_t index) const {
				return reinterpret_cast<uint32_t*>(reinterpret_cast<size_t>(this) + sizeof(uint32_t))[index];
			}
		};
		struct ExCatchableType {
			uint32_t properties; // bit field  IsScalar 1; RefrenceOnly: 2, HasVirtualBases 4; isWinRT: 8, IsStdBadAlloc 16
			int32_t type_info;
			uint32_t non_virtual_adjustment;
			uint32_t offset_to_virtual_base_ptr;
			uint32_t virtual_base_table_index;
			uint32_t size;
			int32_t copy_function;
		};
	}



	CXXExInfo exceptCXXDetails(LPEXCEPTION_RECORD e) {
		CXXExInfo ex;
		if(e->ExceptionCode!=0xe06d7363) {
			ex.native_id = e->ExceptionCode;
			return ex;
		}
		const ExThrowInfo* throwInfo = (const ExThrowInfo*)e->ExceptionInformation[2];
		const ExCatchableTypeArray* cArray = (const ExCatchableTypeArray*)(e->ExceptionInformation[3] + throwInfo->catchable_type_array);
		ex.ex_ptr = (const void*)e->ExceptionInformation[1];
		uint32_t count = cArray->catchable_types;
		while (count--)
		{
			const ExCatchableType* ss = (const ExCatchableType*)(e->ExceptionInformation[3] + cArray->array_of_catchable_types(0));
			CXXExInfo::Tys tys
			{
				(const std::type_info*)(e->ExceptionInformation[3] + ss->type_info),
				(const void*)(e->ExceptionInformation[3] + ss->copy_function),
				(bool)(ss->properties & 16)
			};

			ex.ty_arr.push_back(tys);
		}
		ex.ty_arr.shrink_to_fit();
		ex.cleanup_fn = (const void*)(e->ExceptionInformation[3] + throwInfo->clean_up);
		return ex;
	}
	CXXExInfo exceptCXXDetails(LPEXCEPTION_POINTERS e) {
		return exceptCXXDetails(e->ExceptionRecord);
	}

	static void ex_rethrow(const std::exception_ptr& ex) { std::rethrow_exception(ex); }

	static int getCxxExInfoFromException(CXXExInfo& res, void* e) {
		res = except_abi::exceptCXXDetails((LPEXCEPTION_POINTERS)e);
		return EXCEPTION_EXECUTE_HANDLER;
	}

	static void getCxxExInfoFromException(CXXExInfo& res, const std::exception_ptr& ex) {
		__try { ex_rethrow(ex); }
		__except (except_abi::getCxxExInfoFromException(res, GetExceptionInformation())) {}
	}
}
void getCxxExInfoFromException(CXXExInfo& res, const std::exception_ptr& ex) {
	except_abi::getCxxExInfoFromException(res, ex);
}
void getCxxExInfoFromNative(CXXExInfo& res, void* ex_ptr) {
	res = except_abi::exceptCXXDetails((LPEXCEPTION_POINTERS)ex_ptr);
}
void getCxxExInfoFromNative1(CXXExInfo& res, void* ex_ptr) {
	res = except_abi::exceptCXXDetails((LPEXCEPTION_RECORD)ex_ptr);
}
bool hasClassInEx(CXXExInfo& cxx, const char* class_nam) {
	std::string str = std::string("class ") + class_nam;
	return cxx.ty_arr.contains_one([&str](const CXXExInfo::Tys& ty) { return ty.ty_info->name() == str; });
}
bool isBadAlloc(CXXExInfo& cxx){
	return cxx.ty_arr.contains_one([](const CXXExInfo::Tys& ty) { return ty.is_bad_alloc; });
}