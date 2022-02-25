#pragma once
#include "../libray/list_array.hpp"
#include <typeinfo>
#include <exception>

struct CXXExInfo {
	struct Tys {
		const std::type_info* ty_info;
		const void* copy_fn;
	};
	list_array<Tys> ty_arr;
	const void* cleanup_fn = nullptr;
	const void* ex_ptr = nullptr;
};
void getCxxExInfoFromException(CXXExInfo& res, const std::exception_ptr& ex);
void getCxxExInfoFromNative(CXXExInfo& res, void*);
bool hasClassInEx(CXXExInfo& cxx, const char* class_nam);