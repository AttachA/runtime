// Copyright Danyil Melnytskyi 2022-2023
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once
#include "../library/list_array.hpp"
#include <typeinfo>
#include <exception>

struct CXXExInfo {
	struct Tys {
		const std::type_info* ty_info;
		const void* copy_fn;
		bool is_bad_alloc;
	};
	list_array<Tys> ty_arr;
	const void* cleanup_fn = nullptr;
	const void* ex_ptr = nullptr;
	uint32_t native_id = 0;
};
void getCxxExInfoFromException(CXXExInfo& res, const std::exception_ptr& ex);
void getCxxExInfoFromNative(CXXExInfo& res, void*);
void getCxxExInfoFromNative1(CXXExInfo& res, void*);
bool hasClassInEx(CXXExInfo& cxx, const char* class_nam);
bool isBadAlloc(CXXExInfo& cxx);
