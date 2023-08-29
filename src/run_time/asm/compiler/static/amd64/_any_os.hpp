// Copyright Danyil Melnytskyi 2022-Present
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <run_time/asm/compiler/compiler_include.hpp>

namespace art {
    void Compiler::StaticCompiler::is_gc(const ValueIndexPos& value, ValueMeta value_meta) {
        //TODO: Implement
        compiler.dynamic().is_gc(value);
    }
}