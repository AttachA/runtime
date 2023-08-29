// Copyright Danyil Melnytskyi 2022-Present
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef SRC_RUN_TIME_ASM_IL_HEADER_DECODER
#define SRC_RUN_TIME_ASM_IL_HEADER_DECODER
#include <run_time/asm/CASM.hpp>
#include <run_time/asm/il_compiler/basic.hpp>

namespace art {
    namespace il_header {
        struct decoded {
            il_compiler::basic* compiler = nullptr;
            list_array<std::pair<uint64_t, Label>> jump_list;
            std::vector<art::shared_ptr<FuncEnvironment>> locals;
            FunctionMetaFlags flags;
            uint16_t used_static_values;
            uint16_t used_enviro_vals;
            uint32_t used_arguments;
            uint64_t constants_values;

            size_t decode(
                const art::ustring& header_compiler_name_version,
                CASM& casm_assembler, //for labels
                const std::vector<uint8_t>& data,
                size_t start,
                size_t end_offset);
            size_t decode(
                CASM& casm_assembler, //for labels
                const std::vector<uint8_t>& data,
                size_t start,
                size_t end_offset);

            ~decoded();
        };
    }
}
#endif /* SRC_RUN_TIME_ASM_IL_HEADER_DECODER */
