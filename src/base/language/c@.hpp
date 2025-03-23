// Copyright Danyil Melnytskyi 2025-Present
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef SRC_BASE_LANGUAGE_C_40
#define SRC_BASE_LANGUAGE_C_40
#include <run_time/library/cxx/language.hpp>

namespace language_parsers {

    class c_async : public art::language::helpers::text_language_handler {
        //TODO store processing results during handle_init()s and compile in handle_init_complete
        //needed for class inheritance support
    public:
        c_async();
        art::patch_list handle_init_complete() override;


        static void init();
    };
}

#endif /* SRC_BASE_LANGUAGE_C_40 */
