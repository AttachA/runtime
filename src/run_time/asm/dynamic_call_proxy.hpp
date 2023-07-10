#ifndef SRC_RUN_TIME_ASM_DYNAMIC_CALL_PROXY
#define SRC_RUN_TIME_ASM_DYNAMIC_CALL_PROXY
#include "dynamic_call.hpp"
#include "../attacha_abi_structs.hpp"
namespace art{
    namespace __attacha___ {
        void NativeProxy_DynamicToStatic_addValue(DynamicCall::FunctionCall& call, ValueMeta meta, void*& arg);
        ValueItem* NativeProxy_DynamicToStatic(DynamicCall::FunctionCall& call, DynamicCall::FunctionTemplate& nat_templ, ValueItem* arguments, uint32_t arguments_size);
    }
}
#endif /* SRC_RUN_TIME_ASM_DYNAMIC_CALL_PROXY */
