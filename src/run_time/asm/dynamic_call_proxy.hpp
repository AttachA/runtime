// Copyright Danyil Melnytskyi 2022-Present
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef SRC_RUN_TIME_ASM_DYNAMIC_CALL_PROXY
#define SRC_RUN_TIME_ASM_DYNAMIC_CALL_PROXY
#include <run_time/asm/dynamic_call.hpp>
#include <run_time/attacha_abi_structs.hpp>

namespace art {
    namespace __attacha___ {
        void NativeProxy_DynamicToStatic_addValue(DynamicCall::FunctionCall& call, ValueMeta meta, void*& arg);
        ValueItem* NativeProxy_DynamicToStatic(DynamicCall::FunctionCall& call, DynamicCall::FunctionTemplate& nat_templ, ValueItem* arguments, uint32_t arguments_size);
    }
}
#endif /* SRC_RUN_TIME_ASM_DYNAMIC_CALL_PROXY */
