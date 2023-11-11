// Copyright Danyil Melnytskyi 2022-Present
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <run_time/asm/il_compiler/art_1_0_0.hpp>
#include <run_time/asm/il_compiler/basic.hpp>

namespace art {
    namespace il_compiler {
        basic* map_compiler(const art::ustring& name_version) {
            if (name_version == "art_1.0.0")
                return new art_1_0_0::compiler();
            throw InvalidFunction("Invalid function header, unsupported header name/version: " + name_version);
        }
    }
}