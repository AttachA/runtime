// Copyright Danyil Melnytskyi 2022-Present
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef SRC_RUN_TIME_ASM_EXCEPTION
#define SRC_RUN_TIME_ASM_EXCEPTION
#include <run_time/attacha_abi_structs.hpp>

namespace art {
    struct CXXExInfo;

    namespace exception {
        void* __get_internal_handler();
        ValueItem* get_current_exception_name();
        ValueItem* get_current_exception_name();
        ValueItem* get_current_exception_description();
        ValueItem* get_current_exception_full_description();
        ValueItem* has_current_exception_inner_exception();
        void unpack_current_exception();
        void current_exception_catched();
        CXXExInfo take_current_exception();
        CXXExInfo& peek_current_exception();
        void load_current_exception(CXXExInfo& cxx);
        bool try_catch_all(CXXExInfo& cxx);
        bool has_exception();
        list_array<art::ustring> map_native_exception_names(CXXExInfo& cxx);

        bool _attacha_filter(CXXExInfo& info, void** continue_from, void* data, size_t size, void* enviro, uint8_t* image_base);
        void _attacha_finally(void* data, size_t size, void* enviro);
    }
}

#endif /* SRC_RUN_TIME_ASM_EXCEPTION */
