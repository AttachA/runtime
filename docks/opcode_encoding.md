(opcode:struct Command => .code:enum Opcode)

    [noting: 0]{}
    [set: 1]{
        [struct Command => .static_mode:bool] true
        {
            (ValueMeta)
            [struct Command => .is_gc_mode:bool] // defines if need compare ValueMeta::use_gc
        }
        (value_id:uint16_t)// determines the value to be set
        [value:struct ValueItem]// determines the value constant to be set
        {
            (value.meta:ValueMeta)
            [value.meta.vtype:VType enum]
                [VType::noting]{}
                [VType::ui8]
                [VType::i8]
                    (ui8)
                [VType::ui16]
                [VType::i16]
                    (ui16)
                [VType::ui32]
                [VType::i32]
                [VType::flo]
                    (ui32)
                [VType::ui64]
                [VType::i64]
                [VType::undefined_ptr]
                [VType::time_point]
                [VType::type_identifier]
                [VType::doub]
                    (ui64)
                [VType::raw_arr_ui8]
                [VType::raw_arr_i8]
                    (size:uint32_t)
                    (raw_arr_ui8:array<uint8_t>)
                [VType::raw_arr_ui16]
                [VType::raw_arr_i16]
                    (size:uint32_t)
                    (raw_arr_ui16:array<uint16_t>)
                [VType::raw_arr_ui32]
                [VType::raw_arr_i32]
                [VType::raw_arr_flo]
                    (size:uint32_t)
                    (raw_arr_ui32:array<uint32_t>)
                [VType::raw_arr_ui64]
                [VType::raw_arr_i64]
                [VType::raw_arr_doub]
                    (size:uint32_t)
                    (raw_arr_ui64:array<uint64_t>)
                [VType::uarr]
                    (size:uint64_t)
                    (uarr:array<struct ValueItem>)
                [VType::string]
                    (size:uint32_t)
                    (string:array<uint8_t>)
                [VType::faarr]
                    (size:uint32_t)
                    (faarr:array<struct ValueItem>)
                [default]{{NOT IMPLEMENTED}}
        }
    }
    [set_saarr: 2]
    {
        (value_id:uint16_t)// determines the value to be set
        (length:uint32_t)// determines the length of the stack array
    }
    [remove: 3]{
        [struct Command => .static_mode:bool] true
        {
            (ValueMeta)// determines the type value to be removed
        }
        (value_id:uint16_t)// determines the value to be removed
    }
    [sum: 4]{
        (to_add_id:uint16_t)// determines the value to be set
        (value_id:uint16_t)// determines the value to be used in add operation
    }
    [minus: 5]{
        (to_sub_id:uint16_t)// determines the value to be set
        (value_id:uint16_t)// determines the value to be used in substract operation
    }
    [div: 6]{
        (to_div_id:uint16_t)// determines the value to be set
        (value_id:uint16_t)// determines the value to be used in divide operation
    }
    [mul: 7]{
        (to_mul_id:uint16_t)// determines the value to be set
        (value_id:uint16_t)// determines the value to be used in multiply operation
    }
    [rest: 8]{
        (to_rest_id:uint16_t)// determines the value to be set rest
        (value_id:uint16_t)// determines the value to be used in division with rest operation
    }
    [bit_xor: 9]{
        (to_xor_id:uint16_t)// determines the value to be set
        (value_id:uint16_t)// determines the value to be used in xor operation
    }
    [bit_or: 10]{
        (to_or_id:uint16_t)// determines the value to be set
        (value_id:uint16_t)// determines the value to be used in or operation
    }
    [bit_and: 11] {
        (to_and_id:uint16_t)// determines the value to be set
        (value_id:uint16_t)// determines the value to be used in and operation
    }
    [bit_not: 12]{
        (to_not_id:uint16_t)// determines the value to be set
    }
    [bit_shift_left: 13]{
        (to_shift_id:uint16_t)// determines the value to be set
        (value_id:uint16_t)// determines the value to be used in shift operation
    }
    [bit_shift_right: 14]{
        (to_shift_id:uint16_t)// determines the value to be set
        (value_id:uint16_t)// determines the value to be used in shift operation
    }
    [log_not: 15]{}
    [compare: 16]{
        (to_compare_id:uint16_t)// determines the value to be compared
        (value_id:uint16_t)// determines the value to be used in compare operation
    }
    [jump: 17]{
        (jump_id:uint64_t)// determines the jump id
        (jump_condition:enum JumpCondition)
            [no_condition:0]
            [is_equal:1]
            [is_not_equal:2]
            [is_more:3]
            [is_lower:4]
            [is_lower_or_eq:5]
            [is_more_or_eq:6]
    }
    [arg_set: 18]{
        (value_id:uint16_t)// determines the value to be set as argument, can also convert to farr VType
    }
    [call: 19]{
        (call_flags:struct CallFlags){
            .in_memory:bool
            .async_mode:bool
            .use_result:bool
        }
        [call_flags.in_memory] true{
            (function_name:uint16_t)// determines the call name by value id
        }
        [call_flags.in_memory] false{
            (uint32_t: name_size)
            (name:array<uint8_t>)
        }
        [call_flags.use_result] true{
            (result_id:uint16_t)// determines the value to be set as result
        }
    }
    [call_self: 20]{
        (call_flags:struct CallFlags){
            .async_mode:bool
            .use_result:bool
        }
        [call_flags.use_result] true{
            (result_id:uint16_t)// determines the value to be set as result
        }
    }
    [call_local: 21]{
        (call_flags:struct CallFlags){
            .async_mode:bool
            .use_result:bool
        }
        (function_id:uint16_t)// determines to be called function by local id
        [call_flags.use_result] true{
            (result_id:uint16_t)// determines the value to be set as result
        }
    }
    [call_and_ret: 22]{
        (call_flags:struct CallFlags){
            .in_memory:bool
            .async_mode:bool
        }
        [call_flags.in_memory] true{
            (function_name:uint16_t)// determines the call name by value id
        }
        [call_flags.in_memory] false{
            (uint32_t: name_size)
            (name:array<uint8_t>)
        }
    }
    [call_self_and_ret: 23]{
        (call_flags:struct CallFlags){
            .async_mode:bool
        }
    }
    [call_local_and_ret: 24]{
        (call_flags:struct CallFlags){
            .async_mode:bool
        }
        (function_id:uint16_t)// determines to be called function by local id
    }
    [ret: 25]{
        (value_id:uint16_t)// determines the value to be returned
    }
    [ret_noting: 26]{}
    [copy: 27]{
        (to_copy_id:uint16_t)// determines the value to be set
        (value_id:uint16_t)// determines the value to be copied
    }
    [move: 28]{
        (to_move_id:uint16_t)// determines the value to be set
        (value_id:uint16_t)// determines the value to be moved
    }
    [arr_op: 29]{
        (array_id:uint16_t)// determines the array to be operated
        (flags: struct OpArrFlags){
            .move_mode:bool
            .by_val_mode:bool
            .checked:bool
        }
        (opcode: enum OpcodeArray)
        [struct Command => .static_mode:bool] true{
            (type:VType)// determines the array type
        }
        [enum OpcodeArray]{
            [set]{
                (from_id:uint16_t)// determines the value to be set
                [flags.by_val_mode] true{
                    (to_id:uint16_t)// determines the value that contains index to set
                }
                [flags.by_val_mode] false{
                    (to:uint64_t)// determines the index to be set
                }
            }
            [insert]{
                (from_id:uint16_t)// determines the value to be inserted
                [flags.by_val_mode] true{
                    (to_id:uint16_t)// determines the value that contains index to insert
                }
                [flags.by_val_mode] false{
                    (to:uint64_t)// determines the index to be inserted
                }
            }
            [push_end]{
                (from_id:uint16_t)// determines the value to be pushed
            }
            [push_start]{
                (from_id:uint16_t)// determines the value to be pushed
            }
            [insert_range]{
                (from_id:uint16_t)// determines the array to be inserted
                [flags.by_val_mode] true{
                    (to_id:uint16_t)// determines the value that contains index to insert
                    (from_start_id:uint16_t)// determines the value that contains index to start insert
                    (from_end_id:uint16_t)// determines the value that contains index to end insert
                }
                [flags.by_val_mode] false{
                    (to:uint64_t)// determines the index to be inserted
                    (from_start:uint64_t)// determines the index to start insert
                    (from_end:uint64_t)// determines the index to end insert
                }
            }
            [get]{
                (result_id:uint16_t)// determines the value to be set as result
                [flags.by_val_mode] true{
                    (from_id:uint16_t)// determines the value that contains index to get
                }
                [flags.by_val_mode] false{
                    (from:uint64_t)// determines the index to be get
                }
            }
            [take]{
                (result_id:uint16_t)// determines the value to be set as result
                [flags.by_val_mode] true{
                    (from_id:uint16_t)// determines the value that contains index to take
                }
                [flags.by_val_mode] false{
                    (from:uint64_t)// determines the index to be take
                }
            }
            [take_end]{
                (result_id:uint16_t)// determines the value to be set as result
            }
            [take_start]{
                (result_id:uint16_t)// determines the value to be set as result
            }
            [get_range]{
                (result_id:uint16_t)// determines the value to be set as result
                [flags.by_val_mode] true{
                    (start_id:uint16_t)// determines the value that contains index to start get
                    (end_id:uint16_t)// determines the value that contains index to end get
                }
                [flags.by_val_mode] false{
                    (start:uint64_t)// determines the index to start get
                    (end:uint64_t)// determines the index to end get
                }
            }
            [take_range]{
                (result_id:uint16_t)// determines the value to be set as result
                [flags.by_val_mode] true{
                    (start_id:uint16_t)// determines the value that contains index to start take
                    (end_id:uint16_t)// determines the value that contains index to end take
                }
                [flags.by_val_mode] false{
                    (start:uint64_t)// determines the index to start take
                    (end:uint64_t)// determines the index to end take
                }
            }
            [pop_end]{}
            [pop_start]{}
            [remove_item]{
                [flags.by_val_mode] true{
                    (from_id:uint16_t)// determines the value that contains index to remove
                }
                [flags.by_val_mode] false{
                    (from:uint64_t)// determines the index to be remove
                }
            }
            [remove_range]{
                [flags.by_val_mode] true{
                    (start_id:uint16_t)// determines the value that contains index to start remove
                    (end_id:uint16_t)// determines the value that contains index to end remove
                }
                [flags.by_val_mode] false{
                    (start:uint64_t)// determines the index to start remove
                    (end:uint64_t)// determines the index to end remove
                }
            }
            [resize]{
                [flags.by_val_mode] true{
                    (size_id:uint16_t)// determines the value that contains size to resize
                }
                [flags.by_val_mode] false{
                    (size:uint64_t)// determines the size to resize
                }
            }
            [resize_default]{
                [flags.by_val_mode] true{
                    (size_id:uint16_t)// determines the value that contains size to resize
                }
                [flags.by_val_mode] false{
                    (size:uint64_t)// determines the size to resize
                }
                (default_id:uint16_t)// determines the value to be used as default
            }
            [reserve_push_end]{
                [flags.by_val_mode] true{
                    (size_id:uint16_t)// determines the value that contains size to reserve
                }
                [flags.by_val_mode] false{
                    (size:uint64_t)// determines the size to reserve
                }
            }
            [reserve_push_start]{
                [flags.by_val_mode] true{
                    (size_id:uint16_t)// determines the value that contains size to reserve
                }
                [flags.by_val_mode] false{
                    (size:uint64_t)// determines the size to reserve
                }
            }
            [commit]{
                [flags.by_val_mode] true{
                    (size_id:uint16_t)// determines the value that contains size to commit
                }
                [flags.by_val_mode] false{
                    (size:uint64_t)// determines the size to commit
                }
            }
            [decommit]{
                [flags.by_val_mode] true{
                    (size_id:uint16_t)// determines the value that contains size to decommit
                }
                [flags.by_val_mode] false{
                    (size:uint64_t)// determines the size to decommit
                }
            }
            [remove_reserved]{}
            [size]{
                (result_id:uint16_t)// determines the value to be set as result
            }
        }
        [flags.move_mode] true{
            (move_id:uint16_t)// determines the value to be moved
        }
        [flags.move_mode] false{
            (copy_id:uint16_t)// determines the value to be copied
        }
        [flags.by_val_mode] true{
            (value_id:uint16_t)// determines the value to be used in operation
        }
        [flags.by_val_mode] false{
            (value_id:uint16_t)// determines the value to be used in operation
        }
    }
    [debug_break: 30]{}
    [force_debug_break: 31]{}
    [throw_ex: 32]{
        (in_value:bool)
        [in_value] true{
            (name_id:uint16_t)// determines the value that contains name of exception
            (message_id:uint16_t)// determines the value that contains message of exception
        }
        [in_value] false{
            (name_length:uint32_t)// determines the length of name of exception
            (name:array<uint8_t>)// determines the name of exception
            (message_length:uint32_t)// determines the length of message of exception
            (message:array<uint8_t>)// determines the message of exception
        }
    }
    [as: 33]{
        (result_id:uint16_t)// determines the value to be set as result
        (type_id:ValueItem)// determines the type to be casted
    }
    [is: 34]{
        (result_id:uint16_t)// determines the value to be set as result
        (type_id:ValueItem)// determines the type to be checked
    }
    [store_bool: 35]{
        (value_id:uint16_t)// determines the value to be stored as bool
    }
    [load_bool: 36]{
        (result_id:uint16_t)// determines the value to be set as result
    }
    [inline_native: 37]{
        (raw_asm_lenght:uint32_t)// determines the length of raw assembly
        (raw_asm:array<uint8_t>)// determines the raw assembly to be inlined
    }
    [call_value_function: 38]{
        (flags:struct CallFlags){
            .in_memory:bool
            .async_mode:bool
            .use_result:bool
        }
        [flags.in_memory] true{
            (function_id:uint16_t)// determines the value that contains name of function to be called
        }
        [flags.in_memory] false{
            (function_length:uint32_t)// determines the length of name of function to be called
            (function:array<uint8_t>)// determines the name of function to be called
        }
        (value_id:uint16_t)// determines the value to be used as this
        (access:enum ClassAccess){
            [pub]
            [priv]
            [prot]
            [intern]
        }
        [flags.use_result] true{
            (result_id:uint16_t)// determines the value to be set as result
        }
    }
    [call_value_function_and_ret: 39]{
        (flags:struct CallFlags){
            .in_memory:bool
            .async_mode:bool
        }
        [flags.in_memory] true{
            (function_id:uint16_t)// determines the value that contains name of function to be called
        }
        [flags.in_memory] false{
            (function_length:uint32_t)// determines the length of name of function to be called
            (function:array<uint8_t>)// determines the name of function to be called
        }
        (value_id:uint16_t)// determines the value to be used as this
        (access:enum ClassAccess){
            [pub]
            [priv]
            [prot]
            [intern]
        }
    }
    [static_call_value_function: 40]{
        (flags:struct CallFlags){
            .in_memory:bool
            .async_mode:bool
            .use_result:bool
        }
        [flags.in_memory] true{
            (function_id:uint16_t)// determines the value that contains name of function to be called
        }
        [flags.in_memory] false{
            (function_length:uint32_t)// determines the length of name of function to be called
            (function:array<uint8_t>)// determines the name of function to be called
        }
        (value_id:uint16_t)// determines the value to be used to get function
        (access:enum ClassAccess){
            [pub]
            [priv]
            [prot]
            [intern]
        }
        [flags.use_result] true{
            (result_id:uint16_t)// determines the value to be set as result
        }
    }
    [static_call_value_function_and_ret: 41]{
        (flags:struct CallFlags){
            .in_memory:bool
            .async_mode:bool
        }
        [flags.in_memory] true{
            (function_id:uint16_t)// determines the value that contains name of function to be called
        }
        [flags.in_memory] false{
            (function_length:uint32_t)// determines the length of name of function to be called
            (function:array<uint8_t>)// determines the name of function to be called
        }
        (value_id:uint16_t)// determines the value to be used to get function
        (access:enum ClassAccess){
            [pub]
            [priv]
            [prot]
            [intern]
        }
    }
    [set_structure_value: 42]{
        (is_not_in_memory:bool)
        [is_not_in_memory] true{
            (value_name_id:uint16_t)// determines the value that contains name of value to be set
        }
        [is_not_in_memory] false{
            (value_name_length:uint32_t)// determines the length of name of value to be set
            (value_name:array<uint8_t>)// determines the name of value to be set
        }
        (access:enum ClassAccess){
            [pub]
            [priv]
            [prot]
            [intern]
        }
        (structure_id:uint16_t)// determines the value that contains structure with value to be set
        (value_id:uint16_t)// determines the value to be used as value to be set
    }
    [get_structure_value: 43]{
        (is_not_in_memory:bool)
        [is_not_in_memory] true{
            (value_name_id:uint16_t)// determines the value that contains name of value to be get
        }
        [is_not_in_memory] false{
            (value_name_length:uint32_t)// determines the length of name of value to be get
            (value_name:array<uint8_t>)// determines the name of value to be get
        }
        (access:enum ClassAccess){
            [pub]
            [priv]
            [prot]
            [intern]
        }
        (structure_id:uint16_t)// determines the value that contains structure with value to be get
        (result_id:uint16_t)// determines the value to be set as result
    }
    [explicit_await: 44]{
        (value_id:uint16_t)// determines the value to be awaited
    }
    [async_get: 45]{
        //[TODO]
    }
    [generator_next: 46]{
        //[TODO]

    }
    [yield: 47]{
        //[TODO]
    }
    [handle_begin: 48]{
        (handle_id:uint64_t)// determines the handle id
    }
    [handle_catch: 49]{
        (handle_id:uint64_t)// determines the handle id
        //[TODO]
    }
    [handle_finally: 50]{
        //[TODO]

    }
    [handle_convert: 51]{
        //[TODO]
    }
    [value_hold: 52]{
        //[TODO]
    }
    [value_unhold: 53]{
        //[TODO]
    }
    [is_gc: 54]{
        (use_result:bool)
        (value_id:uint16_t)// determines the value to be checked
        [use_result] true{
            (result_id:uint16_t)// determines the value to be set as result
        }
        [use_result] false // result stored in flags
    }
    [to_gc: 55]{
        (value_id:uint16_t)// determines the value to be converted
    }
    [localize_gc: 56]{
        (value_id:uint16_t)// determines the value to be localized
    }
    [from_gc: 57]{
        (value_id:uint16_t)// determines the value to be converted
    }
    [table_jump: 58]{
        (flags: struct TableJumpFlags){
            [.is_signed:bool]
            [.too_large: enum TableJumpCheckFailAction]{
                [jump_specified]
	            [throw_exception]
	            [unchecked]
            }
            [.too_small: enum TableJumpCheckFailAction]{
                [jump_specified]
	            [throw_exception]
	            [unchecked]
            }
        }
        [flags.too_large == TableJumpCheckFailAction::jump_specified]{
            (too_large_label:uint64_t)
        }
        [flags.too_small == TableJumpCheckFailAction::jump_specified && flags.is_signed == true]{
            (too_small_label:uint64_t)
        }
        (value_id:uint16_t)// determines the value to be used as index
        (table_length:uint32_t)// determines the length of table
        (table:array<uint64_t>)// determines the table of jumps
    }
    [xarray_slice: 59]{
        (result_id:uint16_t)// determines the value to be set as result
        (value_id:uint16_t)// determines the value to be sliced
        (flags:uint8_t){
            [slice_type = flags & 0x0F]
            [slece_offset_used = flags >> 4]
            {
                [slice_type = 0]{
                    [slece_offset_used & 1] true{
                        (slice_offset:uint32_t)// determines the offset of slice
                    }
                    [slece_offset_used & 2] true{
                        (slice_end:uint32_t)// determines the end of slice
                    }
                }
                [slice_type = 1]{
                    [slece_offset_used & 1] true{
                        (slice_offset:uint64_t)// determines the offset of slice
                    }
                    [slece_offset_used & 2] true{
                        (slice_end:uint16_t)// determines the value that contains end of slice
                    }
                }
                [slice_type = 2]{
                    [slece_offset_used & 1] true{
                        (slice_offset:uint16_t)// determines the value that contains offset of slice
                    }
                    [slece_offset_used & 2] true{
                        (slice_end:uint32_t)// determines the end of slice
                    }
                }
                [slice_type = 3]{
                    [slece_offset_used & 1] true{
                        (slice_offset:uint16_t)// determines the value that contains offset of slice
                    }
                    [slece_offset_used & 2] true{
                        (slice_end:uint16_t)// determines the value that contains end of slice
                    }
                }
            }
        }
    }