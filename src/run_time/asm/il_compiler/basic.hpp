#ifndef SRC_RUN_TIME_ASM_IL_COMPILER_BASIC
#define SRC_RUN_TIME_ASM_IL_COMPILER_BASIC
#include <vector>
#include "../compiler.hpp"

namespace art{
    namespace il_compiler{
        class basic {
        public:
            virtual void build(
                const std::vector<uint8_t>& data,
                size_t start,
                size_t end_offset,
                Compiler& compiler,
                FuncHandle::inner_handle* func
            ) = 0;

            virtual void decode_header(
                const std::vector<uint8_t>& data,
                size_t& start,
                size_t end_offset,
                CASM& casm_assembler,
                list_array<std::pair<uint64_t, Label>>& jump_list,
                std::vector<art::shared_ptr<FuncEnvironment>>& locals,
                FunctionMetaFlags& flags,
                uint16_t& used_static_values,
                uint16_t& used_enviro_vals, 
                uint32_t& used_arguments, 
                uint64_t& constants_values) = 0;
        };
        basic* map_compiler(std::string name_version);
    }
}
#endif /* SRC_RUN_TIME_ASM_IL_COMPILER_BASIC */
