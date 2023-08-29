// Copyright Danyil Melnytskyi 2022-Present
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef SRC_RUN_TIME_ASM_IL_COMPILER_ART_1_0_0
#define SRC_RUN_TIME_ASM_IL_COMPILER_ART_1_0_0
#include <run_time/asm/il_compiler/basic.hpp>
#include <util/enum_helper.hpp>


namespace art {
    namespace il_compiler {
        namespace art_1_0_0 {
            ENUM_t(
                Opcode,
                uint8_t,
                noting,
                create_saarr,
                remove,
                sum,
                minus,
                div,
                mul,
                rest,
                bit_xor,
                bit_or,
                bit_and,
                bit_not,
                bit_shift_left,
                bit_shift_right,
                log_not,
                compare,
                jump,
                arg_set,
                call,
                call_self,
                call_local,
                call_and_ret,
                call_self_and_ret,
                call_local_and_ret,
                ret,
                ret_take,
                ret_noting,
                copy,
                move,
                arr_op,
                debug_break,
                force_debug_break,
                throw_ex,
                as,
                is,
                store_bool,
                load_bool,
                inline_native,
                call_value_function,
                call_value_function_id,
                call_value_function_and_ret,
                call_value_function_id_and_ret,
                static_call_value_function,
                static_call_value_function_and_ret,
                static_call_value_function_id,
                static_call_value_function_id_and_ret,
                set_structure_value,
                get_structure_value,
                explicit_await,
                generator_get,
                yield,
                handle_begin,
                handle_catch,
                handle_finally,
                handle_end,
                value_hold,
                value_unhold,
                is_gc,
                to_gc,
                localize_gc,
                from_gc,
                table_jump,
                xarray_slice,
                store_constant,
                get_reference,
                make_as_const,
                remove_const_protect,
                copy_un_constant,
                copy_un_reference,
                move_un_reference,
                remove_qualifiers
            );


            ENUM_t(
                OpcodeArray,
                uint8_t,
                set,
                insert,
                push_end,
                push_start,
                insert_range,

                get,
                take,
                take_end,
                take_start,
                get_range,
                take_range,


                pop_end,
                pop_start,
                remove_item,
                remove_range,

                resize,
                resize_default,


                reserve_push_end,
                reserve_push_start,
                commit,
                decommit,
                remove_reserved,
                size
            );

            ENUM_t(
                ArrCheckMode,
                uint8_t,
                no_check,
                check,
                no_throw_check
            );

            ENUM_t(
                TableJumpCheckFailAction,
                uint8_t,
                jump_specified,
                throw_exception,
                unchecked
            );

            union OpArrFlags {
                struct {
                    uint8_t move_mode : 1;
                    ArrCheckMode checked : 2;
                };
                uint8_t raw;
            };

            union TableJumpFlags {
                struct {
                    uint8_t is_signed : 1;
                    TableJumpCheckFailAction too_large : 2;
                    TableJumpCheckFailAction too_small : 2;
                };
                uint8_t raw;
            };

            ENUM_ta(
                JumpCondition,
                uint8_t,
                (is_more = is_unsigned_more)(is_lower = is_unsigned_lower)(is_lower_or_eq = is_unsigned_lower_or_eq)(is_more_or_eq = is_unsigned_more_or_eq),
                is_zero,
                no_condition,
                is_equal,
                is_not_equal,
                is_unsigned_more,
                is_unsigned_lower,
                is_unsigned_lower_or_eq,
                is_unsigned_more_or_eq,
                is_signed_more,
                is_signed_lower,
                is_signed_lower_or_eq,
                is_signed_more_or_eq
            );

            struct Command {
                Command() {
                    code = Opcode::noting;
                    is_gc_mode = false;
                    static_mode = false;
                }
                Command(Opcode op, bool gc_mode = false, bool set_static_mode = false) {
                    code = op;
                    is_gc_mode = gc_mode;
                    static_mode = set_static_mode;
                }
                Opcode code;
                uint8_t is_gc_mode : 1;
                uint8_t static_mode : 1;
            };

            union CallFlags {
                struct {
                    uint8_t always_dynamic : 1; //prevent holding old function reference
                    uint8_t async_mode : 1;
                    uint8_t use_result : 1;
                    uint8_t : 5;
                };
                uint8_t encoded = 0;
            };
            class compiler : public basic {
            public:
                void build(
                    const std::vector<uint8_t>& data,
                    size_t start,
                    size_t end_offset,
                    Compiler& compiler,
                    FuncHandle::inner_handle* func) override;

                void decode_header(
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
                    uint64_t& constants_values) override;
            };
        }
    }
}

#endif /* SRC_RUN_TIME_ASM_IL_COMPILER_ART_1_0_0 */
