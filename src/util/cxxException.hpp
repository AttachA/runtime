// Copyright Danyil Melnytskyi 2022-Present
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once
#ifndef SRC_RUN_TIME_CXXEXCEPTION
#define SRC_RUN_TIME_CXXEXCEPTION
#include <exception>
#include <typeinfo>

#include <library/list_array.hpp>

namespace art {
    struct CXXExInfo {
        struct Tys {
            const std::type_info* ty_info;
            const void* copy_fn;
            bool is_bad_alloc;

            Tys() = default;
            Tys(const Tys&);
            Tys(Tys&&);

            Tys(const std::type_info* ty_info, const void* copy_fn, bool is_bad_alloc)
                : ty_info(ty_info), copy_fn(copy_fn), is_bad_alloc(is_bad_alloc) {}

            Tys& operator=(const Tys&);
            Tys& operator=(Tys&&);
        };

        list_array<Tys> ty_arr;
        const void* cleanup_fn = nullptr;
        const void* ex_ptr = nullptr;
        uint32_t native_id = 0;
        uint64_t ex_data_0 = 0;
        uint64_t ex_data_1 = 0;
        uint64_t ex_data_2 = 0;
        uint64_t ex_data_3 = 0;

        CXXExInfo() = default;
        CXXExInfo(const CXXExInfo&);
        CXXExInfo(CXXExInfo&&);
        CXXExInfo& operator=(const CXXExInfo&);
        CXXExInfo& operator=(CXXExInfo&&);
    };

    void getCxxExInfoFromException(CXXExInfo& res, const std::exception_ptr& ex);
    void getCxxExInfoFromNative(CXXExInfo& res, void*);
    void getCxxExInfoFromNative1(CXXExInfo& res, void*);
    bool hasClassInEx(CXXExInfo& cxx, const char* class_nam);
    bool isBadAlloc(CXXExInfo& cxx);
}
#endif /* SRC_RUN_TIME_CXXEXCEPTION */
