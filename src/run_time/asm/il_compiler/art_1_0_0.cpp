#include <run_time/asm/il_compiler/art_1_0_0.hpp>
#include <run_time/util/tools.hpp>

art::CallFlags cast_to_local(art::il_compiler::art_1_0_0::CallFlags flags) {
    art::CallFlags result;
    result.async_mode = flags.async_mode;
    result.use_result = flags.use_result;
    result.always_dynamic = flags.always_dynamic;
    return result;
}
art::OpArrFlags cast_to_local(art::il_compiler::art_1_0_0::OpArrFlags flags) {
    art::OpArrFlags result;
    result.checked = (art::ArrCheckMode)flags.checked;
    result.move_mode = flags.move_mode;
    return result;
}
art::TableJumpFlags cast_to_local(art::il_compiler::art_1_0_0::TableJumpFlags flags) {
    art::TableJumpFlags result;
    result.is_signed = flags.is_signed;
    result.too_large = (art::TableJumpCheckFailAction)flags.too_large;
    result.too_small = (art::TableJumpCheckFailAction)flags.too_small;
    return result;
}
art::JumpCondition cast_to_local(art::il_compiler::art_1_0_0::JumpCondition jump_condition) {
    return (art::JumpCondition)jump_condition;
}


namespace art{
    namespace il_compiler{
        namespace art_1_0_0{
            using namespace reader;
            struct CompilerFabric {
                Command cmd;
                const std::vector<uint8_t>& data;
                size_t data_len;
                size_t i;
                size_t skip_count;

                Compiler &compiler;

                CompilerFabric(
                    const std::vector<uint8_t>& data,
                    size_t data_len,
                    size_t start_from,
                    Compiler& compiler
                ) : data(data), data_len(data_len), i(start_from),skip_count(start_from), compiler(compiler) {}

                void store_constant() {
                    compiler.store_constant(readAny(data, data_len, i));
                }


                
                
#pragma region dynamic opcodes
                void dynamic_noting() {
                    compiler.dynamic().noting();
                }
#pragma region set/remove/move/copy
                void dynamic_create_saarr() {
                    ValueIndexPos value_index = readIndexPos(data, data_len, i);
                    uint32_t len = readData<uint32_t>(data, data_len, i);
                    compiler.dynamic().create_saarr(value_index, len);
                }
                void dynamic_remove() {
                    ValueIndexPos value_index = readIndexPos(data, data_len, i);
                    compiler.dynamic().remove(value_index);
                }
                void dynamic_copy() {
                    ValueIndexPos to = readIndexPos(data, data_len, i);
                    ValueIndexPos from = readIndexPos(data, data_len, i);
                    compiler.dynamic().copy(from, to);
                }
                void dynamic_move() {
                    ValueIndexPos to = readIndexPos(data, data_len, i);
                    ValueIndexPos from = readIndexPos(data, data_len, i);
                    compiler.dynamic().move(from, to);
                }
#pragma endregion
#pragma region dynamic math
                void dynamic_sum() {
                    ValueIndexPos a = readIndexPos(data, data_len, i);
                    ValueIndexPos b = readIndexPos(data, data_len, i);
                    compiler.dynamic().sum(a, b);
                }
                void dynamic_minus() {
                    ValueIndexPos a = readIndexPos(data, data_len, i);
                    ValueIndexPos b = readIndexPos(data, data_len, i);
                    compiler.dynamic().sub(a, b);
                }
                void dynamic_div() {
                    ValueIndexPos a = readIndexPos(data, data_len, i);
                    ValueIndexPos b = readIndexPos(data, data_len, i);
                    compiler.dynamic().div(a, b);
                }
                void dynamic_mul() {
                    ValueIndexPos a = readIndexPos(data, data_len, i);
                    ValueIndexPos b = readIndexPos(data, data_len, i);
                    compiler.dynamic().mul(a, b);
                }
                void dynamic_rest() {
                    ValueIndexPos a = readIndexPos(data, data_len, i);
                    ValueIndexPos b = readIndexPos(data, data_len, i);
                    compiler.dynamic().rest(a, b);
                }
#pragma endregion
#pragma region dynamic bit
                void dynamic_bit_xor() {
                    ValueIndexPos a = readIndexPos(data, data_len, i);
                    ValueIndexPos b = readIndexPos(data, data_len, i);
                    compiler.dynamic().bit_xor(a, b);
                }
                void dynamic_bit_or() {
                    ValueIndexPos a = readIndexPos(data, data_len, i);
                    ValueIndexPos b = readIndexPos(data, data_len, i);
                    compiler.dynamic().bit_or(a, b);
                }
                void dynamic_bit_and() {
                    ValueIndexPos a = readIndexPos(data, data_len, i);
                    ValueIndexPos b = readIndexPos(data, data_len, i);
                    compiler.dynamic().bit_and(a, b);
                }
                void dynamic_bit_not() {
                    ValueIndexPos a = readIndexPos(data, data_len, i);
                    compiler.dynamic().bit_not(a);
                }
                void dynamic_bit_shift_left() {
                    ValueIndexPos a = readIndexPos(data, data_len, i);
                    ValueIndexPos b = readIndexPos(data, data_len, i);
                    compiler.dynamic().bit_left_shift(a, b);
                }
                void dynamic_bit_shift_right() {
                    ValueIndexPos a = readIndexPos(data, data_len, i);
                    ValueIndexPos b = readIndexPos(data, data_len, i);
                    compiler.dynamic().bit_right_shift(a, b);
                }

#pragma endregion
#pragma region dynamic logic
                void dynamic_log_not() {
                    compiler.dynamic().logic_not();
                }
                void dynamic_compare() {
                    ValueIndexPos a = readIndexPos(data, data_len, i);
                    ValueIndexPos b = readIndexPos(data, data_len, i);
                    compiler.dynamic().compare(a, b);
                }
                void dynamic_jump() {
                    uint64_t label = readData<uint64_t>(data, data_len, i);
                    JumpCondition condition = readData<JumpCondition>(data, data_len, i);
                    compiler.dynamic().jump(label, cast_to_local(condition));
                }
#pragma endregion
#pragma region dynamic call
                void dynamic_arg_set() {
                    ValueIndexPos value = readIndexPos(data, data_len, i);
                    compiler.dynamic().arg_set(value);
                }
                void dynamic_call() {
                    CallFlags flags;
                    flags.encoded = readData<uint8_t>(data, data_len, i);
                    ValueIndexPos value_index = readIndexPos(data, data_len, i);
                    if (flags.use_result) {
                        auto result = readIndexPos(data, data_len, i);
                        compiler.dynamic().call(value_index, cast_to_local(flags), result);
                    }
                    else
                        compiler.dynamic().call(value_index, cast_to_local(flags));
                }


                void dynamic_call_self() {
                    CallFlags flags;
                    flags.encoded = readData<uint8_t>(data, data_len, i);
                    if (flags.async_mode)
                        throw InvalidIL("Fail compile async 'call_self', for asynchronous call self use 'call' command");
                    if (flags.use_result) {
                        ValueIndexPos result = readIndexPos(data, data_len, i);
                        compiler.dynamic().call_self(result);
                    }
                    else 
                        compiler.dynamic().call_self();
                }
                void dynamic_call_local() {
                    CallFlags flags;
                    flags.encoded = readData<uint8_t>(data, data_len, i);
                    ValueIndexPos value_index = readIndexPos(data, data_len, i);
                    if (flags.use_result) {
                        auto result = readIndexPos(data, data_len, i);
                        compiler.dynamic().call_local(value_index, cast_to_local(flags), result);
                    }
                    else
                        compiler.dynamic().call_local(value_index, cast_to_local(flags));
                }



                
                void dynamic_call_and_ret() {
                    CallFlags flags;
                    flags.encoded = readData<uint8_t>(data, data_len, i);
                    ValueIndexPos value_index = readIndexPos(data, data_len, i);
                    compiler.dynamic().call_and_ret(value_index, cast_to_local(flags));
                }
                
                void dynamic_call_self_and_ret() {
                    CallFlags flags;
                    flags.encoded = readData<uint8_t>(data, data_len, i);
                    if (flags.async_mode)
                        throw InvalidIL("Fail compile async 'call_self', for asynchronous call self use 'call' command");
                    compiler.dynamic().call_self_and_ret();
                }
                void dynamic_call_local_and_ret() {
                    CallFlags flags;
                    flags.encoded = readData<uint8_t>(data, data_len, i);
                    ValueIndexPos value_index = readIndexPos(data, data_len, i);
                    compiler.dynamic().call_local_and_ret(value_index, cast_to_local(flags));
                }
#pragma endregion
                void dynamic_as() {
                    ValueIndexPos value = readIndexPos(data, data_len, i);
                    VType type = readData<VType>(data, data_len, i);
                    compiler.dynamic().as(value, type);
                }
                void dynamic_is() {
                    ValueIndexPos value = readIndexPos(data, data_len, i);
                    VType type = readData<VType>(data, data_len, i);
                    compiler.dynamic().is(value, type);
                }
                void dynamic_store_bool() {
                    ValueIndexPos value = readIndexPos(data, data_len, i);
                    compiler.dynamic().store_bool(value);
                }
                void dynamic_load_bool() {
                    ValueIndexPos value = readIndexPos(data, data_len, i);
                    compiler.dynamic().load_bool(value);
                }

                void dynamic_throw() {
                    ValueIndexPos name = readIndexPos(data, data_len, i);
                    ValueIndexPos message = readIndexPos(data, data_len, i);
                    compiler.dynamic()._throw(name, message);
                }
                void dynamic_insert_native() {
                    uint32_t len = readData<uint32_t>(data, data_len, i);
                    auto tmp = extractRawArray<uint8_t>(data, data_len, i, len);
                    compiler.dynamic().insert_native(len, tmp);
                }

                void dynamic_call_value_function() {
                    CallFlags flags;
                    flags.encoded = readData<uint8_t>(data, data_len, i);
                    ValueIndexPos function_symbol = readIndexPos(data, data_len, i);
                    ValueIndexPos structure = readIndexPos(data, data_len, i);
                    ClassAccess access = readData<ClassAccess>(data, data_len, i);
                    if (flags.use_result) {
                        auto result = readIndexPos(data, data_len, i);
                        compiler.dynamic().call_value_function(cast_to_local(flags), function_symbol, structure, access, result);
                    }
                    else
                        compiler.dynamic().call_value_function(cast_to_local(flags), function_symbol, structure, access);
                }
                void dynamic_call_value_function_id() {
                    CallFlags flags;
                    flags.encoded = readData<uint8_t>(data, data_len, i);
                    uint64_t function_id = readData<uint64_t>(data, data_len, i);
                    ValueIndexPos structure = readIndexPos(data, data_len, i);
                    if (flags.use_result) {
                        auto result = readIndexPos(data, data_len, i);
                        compiler.dynamic().call_value_function_id(cast_to_local(flags), function_id, structure, result);
                    }
                    else
                        compiler.dynamic().call_value_function_id(cast_to_local(flags), function_id, structure);
                }
                void dynamic_call_value_function_and_ret() {
                    CallFlags flags;
                    flags.encoded = readData<uint8_t>(data, data_len, i);
                    ValueIndexPos function_symbol = readIndexPos(data, data_len, i);
                    ValueIndexPos structure = readIndexPos(data, data_len, i);
                    ClassAccess access = readData<ClassAccess>(data, data_len, i);
                    compiler.dynamic().call_value_function_and_ret(cast_to_local(flags), function_symbol, structure, access);
                }
                void dynamic_call_value_function_id_and_ret() {
                    CallFlags flags;
                    flags.encoded = readData<uint8_t>(data, data_len, i);
                    uint64_t function_id = readData<uint64_t>(data, data_len, i);
                    ValueIndexPos structure = readIndexPos(data, data_len, i);
                    compiler.dynamic().call_value_function_id_and_ret(cast_to_local(flags), function_id, structure);
                }
                void dynamic_static_call_value_function() {
                    CallFlags flags;
                    flags.encoded = readData<uint8_t>(data, data_len, i);
                    ValueIndexPos function_symbol = readIndexPos(data, data_len, i);
                    ValueIndexPos structure = readIndexPos(data, data_len, i);
                    ClassAccess access = readData<ClassAccess>(data, data_len, i);
                    if (flags.use_result) {
                        auto result = readIndexPos(data, data_len, i);
                        compiler.dynamic().static_call_value_function(cast_to_local(flags), function_symbol, structure, access, result);
                    }
                    else
                        compiler.dynamic().static_call_value_function(cast_to_local(flags), function_symbol, structure, access);
                }
                void dynamic_static_call_value_function_id() {
                    CallFlags flags;
                    flags.encoded = readData<uint8_t>(data, data_len, i);
                    uint64_t function_id = readData<uint64_t>(data, data_len, i);
                    ValueIndexPos structure = readIndexPos(data, data_len, i);
                    if (flags.use_result) {
                        auto result = readIndexPos(data, data_len, i);
                        compiler.dynamic().static_call_value_function_id(cast_to_local(flags), function_id, structure, result);
                    }
                    else
                        compiler.dynamic().static_call_value_function_id(cast_to_local(flags), function_id, structure);
                }
                void dynamic_static_call_value_function_and_ret() {
                    CallFlags flags;
                    flags.encoded = readData<uint8_t>(data, data_len, i);
                    ValueIndexPos function_symbol = readIndexPos(data, data_len, i);
                    ValueIndexPos structure = readIndexPos(data, data_len, i);
                    ClassAccess access = readData<ClassAccess>(data, data_len, i);
                    compiler.dynamic().static_call_value_function_and_ret(cast_to_local(flags), function_symbol, structure, access);
                }
                void dynamic_static_call_value_function_id_and_ret() {
                    CallFlags flags;
                    flags.encoded = readData<uint8_t>(data, data_len, i);
                    uint64_t function_id = readData<uint64_t>(data, data_len, i);
                    ValueIndexPos structure = readIndexPos(data, data_len, i);
                    compiler.dynamic().static_call_value_function_id_and_ret(cast_to_local(flags), function_id, structure);
                }

                void dynamic_set_structure_value() {
                    auto value_name = readIndexPos(data, data_len, i);
                    auto access = readData<ClassAccess>(data, data_len, i);
                    auto structure = readIndexPos(data, data_len, i);
                    auto set = readIndexPos(data, data_len, i);
                    compiler.dynamic().set_structure_value(value_name, access, structure, set);
                }
                void dynamic_get_structure_value() {
                    auto value_name = readIndexPos(data, data_len, i);
                    auto access = readData<ClassAccess>(data, data_len, i);
                    auto structure = readIndexPos(data, data_len, i);
                    auto result = readIndexPos(data, data_len, i);
                    compiler.dynamic().get_structure_value(value_name, access, structure, result);
                }
                void dynamic_explicit_await() {
                    ValueIndexPos value = readIndexPos(data, data_len, i);
                    compiler.dynamic().explicit_await(value);
                }
                void dynamic_generator_get() {
                    ValueIndexPos generator = readIndexPos(data, data_len, i);
                    ValueIndexPos result = readIndexPos(data, data_len, i);
                    ValueIndexPos result_index = readIndexPos(data, data_len, i);
                    compiler.dynamic().generator_get(generator, result, result_index);
                }
                void dynamic_yield() {
                    ValueIndexPos value = readIndexPos(data, data_len, i);
                    compiler.dynamic()._yield(value);
                }

                void dynamic_ret() {
                    ValueIndexPos value = readIndexPos(data, data_len, i);
                    compiler.dynamic().ret(value);
                }
                void dynamic_ret_take() {
                    ValueIndexPos value = readIndexPos(data, data_len, i);
                    compiler.dynamic().ret_take(value);
                }
                void dynamic_ret_noting() {
                    compiler.dynamic().ret_noting();
                }

                void dynamic_arr_op() {
                    ValueIndexPos arr = readIndexPos(data, data_len, i);
                    auto flags = readData<OpArrFlags>(data, data_len, i);
                    auto arr_op = compiler.dynamic().arr_op(arr, cast_to_local(flags));
                    switch (readData<OpcodeArray>(data, data_len, i)) {
                    case OpcodeArray::set: {
                        ValueIndexPos value = readIndexPos(data, data_len, i);
                        ValueIndexPos index = readIndexPos(data, data_len, i);
                        arr_op.set(index, value);
                        break;
                    }
                    case OpcodeArray::insert: {
                        ValueIndexPos index = readIndexPos(data, data_len, i);
                        ValueIndexPos value = readIndexPos(data, data_len, i);
                        arr_op.insert(index, value);
                        break;
                    }
                    case OpcodeArray::push_end: {
                        ValueIndexPos value = readIndexPos(data, data_len, i);
                        arr_op.push_end(value);
                        break;
                    }
                    case OpcodeArray::push_start: {
                        ValueIndexPos value = readIndexPos(data, data_len, i);
                        arr_op.push_start(value);
                        break;
                    }
                    case OpcodeArray::insert_range: {
                        ValueIndexPos arr1 = readIndexPos(data, data_len, i);
                        ValueIndexPos pos = readIndexPos(data, data_len, i);
                        ValueIndexPos start1 = readIndexPos(data, data_len, i);
                        ValueIndexPos end1 = readIndexPos(data, data_len, i);
                        arr_op.insert_range(arr1, pos, start1, end1);
                        break;
                    }
                    case OpcodeArray::get: {
                        ValueIndexPos index = readIndexPos(data, data_len, i);
                        ValueIndexPos result = readIndexPos(data, data_len, i);
                        arr_op.get(index, result);
                        break;
                    }
                    case OpcodeArray::take: {
                        ValueIndexPos index = readIndexPos(data, data_len, i);
                        ValueIndexPos result = readIndexPos(data, data_len, i);
                        arr_op.take(index, result);
                        break;
                    }
                    case OpcodeArray::take_end: {
                        ValueIndexPos result = readIndexPos(data, data_len, i);
                        arr_op.take_end(result);
                        break;
                    }
                    case OpcodeArray::take_start: {
                        ValueIndexPos result = readIndexPos(data, data_len, i);
                        arr_op.take_start(result);
                        break;
                    }
                    case OpcodeArray::get_range: {
                        ValueIndexPos set_to = readIndexPos(data, data_len, i);
                        ValueIndexPos start = readIndexPos(data, data_len, i);
                        ValueIndexPos end = readIndexPos(data, data_len, i);
                        arr_op.get_range(set_to, start, end);
                        break;
                    }
                    case OpcodeArray::take_range: {
                        ValueIndexPos set_to = readIndexPos(data, data_len, i);
                        ValueIndexPos start = readIndexPos(data, data_len, i);
                        ValueIndexPos end = readIndexPos(data, data_len, i);
                        arr_op.take_range(set_to, start, end);
                        break;
                    }
                    case OpcodeArray::pop_end: {
                        arr_op.pop_end();
                        break;
                    }
                    case OpcodeArray::pop_start: {
                        arr_op.pop_start();
                        break;
                    }
                    case OpcodeArray::remove_item: {
                        ValueIndexPos index = readIndexPos(data, data_len, i);
                        arr_op.remove_item(index);
                        break;
                    }
                    case OpcodeArray::remove_range: {
                        ValueIndexPos start = readIndexPos(data, data_len, i);
                        ValueIndexPos end = readIndexPos(data, data_len, i);
                        arr_op.remove_range(start, end);
                        break;
                    }
                    case OpcodeArray::resize: {
                        ValueIndexPos size = readIndexPos(data, data_len, i);
                        arr_op.resize(size);
                        break;
                    }
                    case OpcodeArray::resize_default: {
                        ValueIndexPos size = readIndexPos(data, data_len, i);
                        ValueIndexPos def = readIndexPos(data, data_len, i);
                        arr_op.resize_default(size, def);
                        break;
                    }
                    case OpcodeArray::reserve_push_end: {
                        ValueIndexPos len = readIndexPos(data, data_len, i);
                        arr_op.reserve_push_end(len);
                        break;
                    }
                    case OpcodeArray::reserve_push_start: {
                        ValueIndexPos len = readIndexPos(data, data_len, i);
                        arr_op.reserve_push_start(len);
                        break;
                    }
                    case OpcodeArray::commit: {
                        arr_op.commit();
                        break;
                    }
                    case OpcodeArray::decommit: {
                        ValueIndexPos blocks = readIndexPos(data, data_len, i);
                        arr_op.decommit(blocks);
                        break;
                    }
                    case OpcodeArray::remove_reserved: {
                        arr_op.remove_reserved();
                        break;
                    }
                    case OpcodeArray::size: {
                        ValueIndexPos result = readIndexPos(data, data_len, i);
                        arr_op.size(result);
                        break;
                    }
                    default:
                        throw InvalidIL("Invalid array operation");
                    }
                }

                void dynamic_debug_break() {
                    compiler.dynamic().debug_break();
                }

                void dynamic_debug_force_break() {
                    compiler.dynamic().debug_force_break();
                }



                void dynamic_handle_begin() {
                    uint64_t handle_id = readData<uint64_t>(data, data_len, i);
                    compiler.dynamic().handle_begin(handle_id);
                }
                void dynamic_handle_catch() {
                    uint64_t handle_id = readData<uint64_t>(data, data_len, i);
                    char command = readData<char>(data, data_len, i);
                    switch (command) {
                    case 0: {
                        uint64_t len = readPackedLen(data, data_len, i);
                        std::vector<art::ustring> to_catch_names;
                        to_catch_names.reserve(len);

                        for (uint64_t i = 0; i < len; i++)
                            to_catch_names.push_back(readString(data, data_len, i));
                        compiler.dynamic().handle_catch_0(handle_id, to_catch_names);
                        break;
                    }
                    case 1:
                        compiler.dynamic().handle_catch_1(handle_id, readData<uint16_t>(data, data_len, i));
                        break;
                    case 2: {
                        uint64_t len = readPackedLen(data, data_len, i);
                        std::vector<uint16_t> to_catch_dynamic_values;
                        to_catch_dynamic_values.reserve(len);
                        for(uint64_t i = 0; i < len; i++)
                            to_catch_dynamic_values.push_back(readData<uint16_t>(data, data_len, i));
                        compiler.dynamic().handle_catch_2(handle_id, to_catch_dynamic_values);
                        break;
                    }
                    case 3: {
                        uint64_t len = readPackedLen(data, data_len, i);
                        if(len == 0)
                            return;
                        std::vector<art::ustring> to_catch_names;
                        std::vector<uint16_t> to_catch_dynamic_values;
                        to_catch_names.reserve(len/2);
                        to_catch_dynamic_values.reserve(len/2);

                        for(uint64_t i = 0; i < len; i++) {
                            bool is_dynamic = readData<bool>(data, data_len, i);
                            if(is_dynamic)
                                to_catch_dynamic_values.push_back(readData<uint16_t>(data, data_len, i));
                            else
                                to_catch_names.push_back(readString(data, data_len, i));
                        }
                        compiler.dynamic().handle_catch_3(handle_id, to_catch_names, to_catch_dynamic_values);
                        break;
                    }
                    case 4: compiler.dynamic().handle_catch_4(handle_id); break;
                    case 5: {
                        if(readData<bool>(data, data_len, i)) {
                            uint64_t local_function = readData<uint64_t>(data, data_len, i);
                            uint16_t enviro_slice_begin = readData<uint16_t>(data, data_len, i);
                            uint16_t enviro_slice_end = readData<uint16_t>(data, data_len, i);
                            compiler.dynamic().handle_catch_5(handle_id,local_function, enviro_slice_begin, enviro_slice_end);
                        }
                        else {
                            art::ustring global_function = readString(data, data_len, i);
                            uint16_t enviro_slice_begin = readData<uint16_t>(data, data_len, i);
                            uint16_t enviro_slice_end = readData<uint16_t>(data, data_len, i);
                            compiler.dynamic().handle_catch_5(handle_id, global_function, enviro_slice_begin, enviro_slice_end);
                        }
                        break;
                    }
                    default:
                        throw InvalidIL("Invalid catch command");
                    }
                }
                void dynamic_handle_finally() {
                    uint64_t handle_id = readData<uint64_t>(data, data_len, i);
                    if(readData<bool>(data, data_len, i)) {
                        uint64_t local_function = readData<uint64_t>(data, data_len, i);
                        uint16_t enviro_slice_begin = readData<uint16_t>(data, data_len, i);
                        uint16_t enviro_slice_end = readData<uint16_t>(data, data_len, i);
                        compiler.dynamic().handle_finally(handle_id,local_function, enviro_slice_begin, enviro_slice_end);
                    }
                    else {
                        art::ustring global_function = readString(data, data_len, i);
                        uint16_t enviro_slice_begin = readData<uint16_t>(data, data_len, i);
                        uint16_t enviro_slice_end = readData<uint16_t>(data, data_len, i);
                        compiler.dynamic().handle_finally(handle_id, global_function, enviro_slice_begin, enviro_slice_end);
                    }
                }
                void dynamic_handle_end() {
                    uint64_t handle_id = readData<uint64_t>(data, data_len, i);
                    compiler.dynamic().handle_end(handle_id);
                }

                void dynamic_value_hold() {
                    uint64_t hold_id = readData<uint64_t>(data, data_len, i);
                    uint16_t value_index = readData<uint16_t>(data, data_len, i);
                    compiler.dynamic().value_hold(hold_id, value_index);
                }
                void dynamic_value_unhold() {
                    uint64_t hold_id = readData<uint64_t>(data, data_len, i);
                    compiler.dynamic().value_unhold(hold_id);
                }



                void dynamic_is_gc() {
                    bool use_result = readData<bool>(data, data_len, i);
                    auto value = readIndexPos(data, data_len, i);
                    if (use_result) {
                        auto result = readIndexPos(data, data_len, i);
                        compiler.dynamic().is_gc(value, result);
                    }
                    else
                        compiler.dynamic().is_gc(value);
                }
                void dynamic_to_gc() {
                    auto value = readIndexPos(data, data_len, i);
                    compiler.dynamic().to_gc(value);
                }
                void dynamic_from_gc() {
                    auto value = readIndexPos(data, data_len, i);
                    compiler.dynamic().from_gc(value);
                }
                void dynamic_localize_gc() {
                    auto value = readIndexPos(data, data_len, i);
                    compiler.dynamic().localize_gc(value);
                }
                void dynamic_table_jump() {
                    TableJumpFlags flags = readData<TableJumpFlags>(data, data_len, i);

                    uint64_t fail_too_large = 0;
                    uint64_t fail_too_small = 0;
                    if (flags.too_large == TableJumpCheckFailAction::jump_specified)
                        fail_too_large = readData<uint64_t>(data, data_len, i);
                    if(flags.too_small == TableJumpCheckFailAction::jump_specified && flags.is_signed)
                        fail_too_small = readData<uint64_t>(data, data_len, i);

                    ValueIndexPos value = readIndexPos(data, data_len, i);
                    uint32_t table_size = readData<uint32_t>(data, data_len, i);

                    std::vector<uint64_t> table;
                    table.reserve(table_size);
                    for (uint32_t j = 0; j < table_size; j++)
                        table.push_back(readData<uint64_t>(data, data_len, i));

                    compiler.dynamic().table_jump(cast_to_local(flags), fail_too_large, fail_too_small, table, value);
                }
                void dynamic_xarray_slice() {
                    ValueIndexPos result_index = readIndexPos(data, data_len, i);
                    ValueIndexPos slice_index = readIndexPos(data, data_len, i);
                    uint8_t slice_flags = readData<uint8_t>(data, data_len, i);

                    uint8_t slice_type = slice_flags & 0x0F;
                    switch(slice_type) {
                        case 1: {
                            compiler.dynamic().xarray_slice(result_index, slice_index);
                            break;
                        }
                        case 2: {
                            auto to = readIndexPos(data, data_len, i);
                            compiler.dynamic().xarray_slice(result_index, slice_index,false, to);
                            break;
                        }
                        case 3: {
                            auto from = readIndexPos(data, data_len, i);
                            compiler.dynamic().xarray_slice(result_index, slice_index, from);
                            break;
                        }
                        case 4: {
                            auto from = readIndexPos(data, data_len, i);
                            auto to = readIndexPos(data, data_len, i);
                            compiler.dynamic().xarray_slice(result_index, slice_index, from, to);
                            break;
                        }
                        default:
                            throw InvalidIL("Invalid opcode, unsupported slice type");
                    }
                }
                void dynamic_get_reference() {
                    ValueIndexPos result = readIndexPos(data, data_len, i);
                    ValueIndexPos value = readIndexPos(data, data_len, i);
                    compiler.dynamic().get_reference(result, value);
                }
                void dynamic_make_as_const() {
                    ValueIndexPos value = readIndexPos(data, data_len, i);
                    compiler.dynamic().make_as_const(value);
                }
                void dynamic_remove_const_protect() {
                    ValueIndexPos value = readIndexPos(data, data_len, i);
                    compiler.dynamic().remove_const_protect(value);
                }
                void dynamic_copy_un_constant() {
                    ValueIndexPos result_index = readIndexPos(data, data_len, i);
                    ValueIndexPos source_index = readIndexPos(data, data_len, i);
                    compiler.dynamic().copy_unconst(result_index, source_index);
                }
                void dynamic_copy_un_reference() {
                    ValueIndexPos result_index = readIndexPos(data, data_len, i);
                    ValueIndexPos source_index = readIndexPos(data, data_len, i);
                    compiler.dynamic().copy_unreference(result_index, source_index);
                }
                void dynamic_move_un_reference() {
                    ValueIndexPos result_index = readIndexPos(data, data_len, i);
                    ValueIndexPos source_index = readIndexPos(data, data_len, i);
                    compiler.dynamic().move_unreference(result_index, source_index);
                }
                void dynamic_remove_qualifiers() {
                    ValueIndexPos value = readIndexPos(data, data_len, i);
                    compiler.dynamic().remove_qualifiers(value);
                }
#pragma endregion
#pragma region static opcodes
#pragma region set/remove/move/copy
                void static_remove() {
                    ValueIndexPos value_index = readIndexPos(data, data_len, i);
                    ValueMeta meta = readData<ValueMeta>(data, data_len, i);
                    compiler.static_().remove(value_index, meta);
                }
                void static_copy() {
                    ValueIndexPos to = readIndexPos(data, data_len, i);
                    ValueMeta to_meta = readData<ValueMeta>(data, data_len, i);
                    ValueIndexPos from = readIndexPos(data, data_len, i);
                    ValueMeta from_meta = readData<ValueMeta>(data, data_len, i);
                    compiler.static_().copy(to, to_meta, from, from_meta);
                }
                void static_move() {
                    ValueIndexPos to = readIndexPos(data, data_len, i);
                    ValueMeta to_meta = readData<ValueMeta>(data, data_len, i);
                    ValueIndexPos from = readIndexPos(data, data_len, i);
                    ValueMeta from_meta = readData<ValueMeta>(data, data_len, i);
                    compiler.static_().move(to, to_meta, from, from_meta);
                }
#pragma endregion
#pragma region static math
                void static_sum() {
                    ValueIndexPos a = readIndexPos(data, data_len, i);
                    ValueMeta a_meta = readData<ValueMeta>(data, data_len, i);
                    ValueIndexPos b = readIndexPos(data, data_len, i);
                    ValueMeta b_meta = readData<ValueMeta>(data, data_len, i);
                    compiler.static_().sum(a, a_meta, b, b_meta);
                }
                void static_minus() {
                    ValueIndexPos a = readIndexPos(data, data_len, i);
                    ValueMeta a_meta = readData<ValueMeta>(data, data_len, i);
                    ValueIndexPos b = readIndexPos(data, data_len, i);
                    ValueMeta b_meta = readData<ValueMeta>(data, data_len, i);
                    compiler.static_().sub(a, a_meta, b, b_meta);
                }
                void static_div() {
                    ValueIndexPos a = readIndexPos(data, data_len, i);
                    ValueMeta a_meta = readData<ValueMeta>(data, data_len, i);
                    ValueIndexPos b = readIndexPos(data, data_len, i);
                    ValueMeta b_meta = readData<ValueMeta>(data, data_len, i);
                    compiler.static_().div(a, a_meta, b, b_meta);
                }
                void static_mul() {
                    ValueIndexPos a = readIndexPos(data, data_len, i);
                    ValueMeta a_meta = readData<ValueMeta>(data, data_len, i);
                    ValueIndexPos b = readIndexPos(data, data_len, i);
                    ValueMeta b_meta = readData<ValueMeta>(data, data_len, i);
                    compiler.static_().mul(a, a_meta, b, b_meta);
                }
                void static_rest() {
                    ValueIndexPos a = readIndexPos(data, data_len, i);
                    ValueMeta a_meta = readData<ValueMeta>(data, data_len, i);
                    ValueIndexPos b = readIndexPos(data, data_len, i);
                    ValueMeta b_meta = readData<ValueMeta>(data, data_len, i);
                    compiler.static_().rest(a, a_meta, b, b_meta);
                }
#pragma endregion
#pragma region static bit
                void static_bit_xor() {
                    ValueIndexPos a = readIndexPos(data, data_len, i);
                    ValueMeta a_meta = readData<ValueMeta>(data, data_len, i);
                    ValueIndexPos b = readIndexPos(data, data_len, i);
                    ValueMeta b_meta = readData<ValueMeta>(data, data_len, i);
                    compiler.static_().bit_xor(a, a_meta, b, b_meta);
                }
                void static_bit_or() {
                    ValueIndexPos a = readIndexPos(data, data_len, i);
                    ValueMeta a_meta = readData<ValueMeta>(data, data_len, i);
                    ValueIndexPos b = readIndexPos(data, data_len, i);
                    ValueMeta b_meta = readData<ValueMeta>(data, data_len, i);
                    compiler.static_().bit_or(a, a_meta, b, b_meta);
                }
                void static_bit_and() {
                    ValueIndexPos a = readIndexPos(data, data_len, i);
                    ValueMeta a_meta = readData<ValueMeta>(data, data_len, i);
                    ValueIndexPos b = readIndexPos(data, data_len, i);
                    ValueMeta b_meta = readData<ValueMeta>(data, data_len, i);
                    compiler.static_().bit_and(a, a_meta, b, b_meta);
                }
                void static_bit_not() {
                    ValueIndexPos a = readIndexPos(data, data_len, i);
                    ValueMeta a_meta = readData<ValueMeta>(data, data_len, i);
                    compiler.static_().bit_not(a, a_meta);
                }
                void static_bit_shift_left() {
                    ValueIndexPos a = readIndexPos(data, data_len, i);
                    ValueMeta a_meta = readData<ValueMeta>(data, data_len, i);
                    ValueIndexPos b = readIndexPos(data, data_len, i);
                    ValueMeta b_meta = readData<ValueMeta>(data, data_len, i);
                    compiler.static_().bit_left_shift(a, a_meta, b, b_meta);
                }
                void static_bit_shift_right() {
                    ValueIndexPos a = readIndexPos(data, data_len, i);
                    ValueMeta a_meta = readData<ValueMeta>(data, data_len, i);
                    ValueIndexPos b = readIndexPos(data, data_len, i);
                    ValueMeta b_meta = readData<ValueMeta>(data, data_len, i);
                    compiler.static_().bit_right_shift(a, a_meta, b, b_meta);
                }

#pragma endregion
#pragma region static logic
                void static_compare() {
                    ValueIndexPos a = readIndexPos(data, data_len, i);
                    ValueMeta a_meta = readData<ValueMeta>(data, data_len, i);
                    ValueIndexPos b = readIndexPos(data, data_len, i);
                    ValueMeta b_meta = readData<ValueMeta>(data, data_len, i);
                    compiler.static_().compare(a, a_meta, b, b_meta);
                }
#pragma endregion
#pragma region static call
                void static_arg_set() {
                    ValueIndexPos value = readIndexPos(data, data_len, i);
                    compiler.static_().arg_set(value);
                }
                void static_call() {
                    CallFlags flags;
                    flags.encoded = readData<uint8_t>(data, data_len, i);
                    ValueIndexPos value_index = readIndexPos(data, data_len, i);
                    ValueMeta value_meta = readData<ValueMeta>(data, data_len, i);
                    if (flags.use_result) {
                        auto result = readIndexPos(data, data_len, i);
                        auto result_meta = readData<ValueMeta>(data, data_len, i);
                        compiler.static_().call(value_index, value_meta, cast_to_local(flags), result, result_meta);
                    }
                    else
                        compiler.static_().call(value_index, value_meta, cast_to_local(flags));
                }


                void static_call_self() {
                    CallFlags flags;
                    flags.encoded = readData<uint8_t>(data, data_len, i);
                    if (flags.async_mode)
                        throw InvalidIL("Fail compile async 'call_self', for asynchronous call self use 'call' command");
                    if (!flags.use_result) 
                        throw InvalidIL("Fail compile static 'call_self', for that use dynamic 'call_self' command");
                    ValueIndexPos result = readIndexPos(data, data_len, i);
                    ValueMeta result_meta = readData<ValueMeta>(data, data_len, i);
                    compiler.static_().call_self(result, result_meta);
                }
                void static_call_local() {
                    CallFlags flags;
                    flags.encoded = readData<uint8_t>(data, data_len, i);
                    ValueIndexPos value_index = readIndexPos(data, data_len, i);
                    ValueMeta value_meta = readData<ValueMeta>(data, data_len, i);

                    if (flags.use_result) {
                        auto result = readIndexPos(data, data_len, i);
                        auto result_meta = readData<ValueMeta>(data, data_len, i);
                        compiler.static_().call_local(value_index, value_meta, cast_to_local(flags), result, result_meta);
                    }
                    else
                        compiler.static_().call_local(value_index, value_meta, cast_to_local(flags));
                }



                
                void static_call_and_ret() {
                    CallFlags flags;
                    flags.encoded = readData<uint8_t>(data, data_len, i);
                    ValueIndexPos value_index = readIndexPos(data, data_len, i);
                    ValueMeta value_meta = readData<ValueMeta>(data, data_len, i);
                    compiler.static_().call_and_ret(value_index, value_meta, cast_to_local(flags));
                }
                
                void static_call_local_and_ret() {
                    CallFlags flags;
                    flags.encoded = readData<uint8_t>(data, data_len, i);
                    ValueIndexPos value_index = readIndexPos(data, data_len, i);
                    ValueMeta value_meta = readData<ValueMeta>(data, data_len, i);
                    compiler.static_().call_local_and_ret(value_index, value_meta, cast_to_local(flags));
                }
#pragma endregion
                void static_as() {
                    ValueIndexPos value = readIndexPos(data, data_len, i);
                    ValueMeta value_meta = readData<ValueMeta>(data, data_len, i);
                    VType type = readData<VType>(data, data_len, i);
                    compiler.static_().as(value, value_meta, type);
                }
                void static_is() {
                    ValueIndexPos value = readIndexPos(data, data_len, i);
                    VType type = readData<VType>(data, data_len, i);
                    compiler.static_().is(value, type);
                }
                void static_store_bool() {
                    ValueIndexPos value = readIndexPos(data, data_len, i);
                    ValueMeta value_meta = readData<ValueMeta>(data, data_len, i);
                    compiler.static_().store_bool(value, value_meta);
                }
                void static_load_bool() {
                    ValueIndexPos value = readIndexPos(data, data_len, i);
                    ValueMeta value_meta = readData<ValueMeta>(data, data_len, i);
                    compiler.static_().load_bool(value, value_meta);
                }

                void static_throw() {
                    ValueIndexPos name = readIndexPos(data, data_len, i);
                    ValueMeta name_meta = readData<ValueMeta>(data, data_len, i);
                    ValueIndexPos message = readIndexPos(data, data_len, i);
                    ValueMeta message_meta = readData<ValueMeta>(data, data_len, i);
                    compiler.static_()._throw(name, name_meta, message, message_meta);
                }

                void static_call_value_function() {
                    CallFlags flags;
                    flags.encoded = readData<uint8_t>(data, data_len, i);
                    ValueIndexPos function_symbol = readIndexPos(data, data_len, i);
                    ValueMeta function_symbol_meta = readData<ValueMeta>(data, data_len, i);
                    ValueIndexPos structure = readIndexPos(data, data_len, i);
                    ClassAccess access = readData<ClassAccess>(data, data_len, i);
                    if (flags.use_result) {
                        auto result = readIndexPos(data, data_len, i);
                        auto result_meta = readData<ValueMeta>(data, data_len, i);
                        compiler.static_().call_value_function(cast_to_local(flags), function_symbol, function_symbol_meta, structure, access, result, result_meta);
                    }
                    else
                        compiler.static_().call_value_function(cast_to_local(flags), function_symbol, function_symbol_meta, structure, access);
                }
                void static_call_value_function_id() {
                    CallFlags flags;
                    flags.encoded = readData<uint8_t>(data, data_len, i);
                    uint64_t function_id = readData<uint64_t>(data, data_len, i);
                    ValueIndexPos structure = readIndexPos(data, data_len, i);
                    if (flags.use_result) {
                        auto result = readIndexPos(data, data_len, i);
                        auto result_meta = readData<ValueMeta>(data, data_len, i);
                        compiler.static_().call_value_function_id(cast_to_local(flags), function_id, structure, result, result_meta);
                    }
                    else
                        compiler.static_().call_value_function_id(cast_to_local(flags), function_id, structure);
                }
                void static_call_value_function_and_ret() {
                    CallFlags flags;
                    flags.encoded = readData<uint8_t>(data, data_len, i);
                    ValueIndexPos function_symbol = readIndexPos(data, data_len, i);
                    ValueMeta function_symbol_meta = readData<ValueMeta>(data, data_len, i);
                    ValueIndexPos structure = readIndexPos(data, data_len, i);
                    ClassAccess access = readData<ClassAccess>(data, data_len, i);
                    compiler.static_().call_value_function_and_ret(cast_to_local(flags), function_symbol, function_symbol_meta, structure, access);
                }
                void static_call_value_function_id_and_ret() {
                    CallFlags flags;
                    flags.encoded = readData<uint8_t>(data, data_len, i);
                    uint64_t function_id = readData<uint64_t>(data, data_len, i);
                    ValueIndexPos structure = readIndexPos(data, data_len, i);
                    compiler.static_().call_value_function_id_and_ret(cast_to_local(flags), function_id, structure);
                }
                void static_static_call_value_function() {
                    CallFlags flags;
                    flags.encoded = readData<uint8_t>(data, data_len, i);
                    ValueIndexPos function_symbol = readIndexPos(data, data_len, i);
                    ValueMeta function_symbol_meta = readData<ValueMeta>(data, data_len, i);
                    ValueIndexPos structure = readIndexPos(data, data_len, i);
                    ClassAccess access = readData<ClassAccess>(data, data_len, i);
                    if (flags.use_result) {
                        auto result = readIndexPos(data, data_len, i);
                        auto result_meta = readData<ValueMeta>(data, data_len, i);
                        compiler.static_().static_call_value_function(cast_to_local(flags), function_symbol, function_symbol_meta, structure, access, result, result_meta);
                    }
                    else
                        compiler.static_().static_call_value_function(cast_to_local(flags), function_symbol, function_symbol_meta, structure, access);
                }
                void static_static_call_value_function_id() {
                    CallFlags flags;
                    flags.encoded = readData<uint8_t>(data, data_len, i);
                    uint64_t function_id = readData<uint64_t>(data, data_len, i);
                    ValueIndexPos structure = readIndexPos(data, data_len, i);
                    if (flags.use_result) {
                        auto result = readIndexPos(data, data_len, i);
                        auto result_meta = readData<ValueMeta>(data, data_len, i);
                        compiler.static_().static_call_value_function_id(cast_to_local(flags), function_id, structure, result, result_meta);
                    }
                    else
                        compiler.static_().static_call_value_function_id(cast_to_local(flags), function_id, structure);
                }
                void static_static_call_value_function_and_ret() {
                    CallFlags flags;
                    flags.encoded = readData<uint8_t>(data, data_len, i);
                    ValueIndexPos function_symbol = readIndexPos(data, data_len, i);
                    ValueMeta function_symbol_meta = readData<ValueMeta>(data, data_len, i);
                    ValueIndexPos structure = readIndexPos(data, data_len, i);
                    ClassAccess access = readData<ClassAccess>(data, data_len, i);
                    compiler.static_().static_call_value_function_and_ret(cast_to_local(flags), function_symbol, function_symbol_meta, structure, access);
                }
                void static_static_call_value_function_id_and_ret() {
                    CallFlags flags;
                    flags.encoded = readData<uint8_t>(data, data_len, i);
                    uint64_t function_id = readData<uint64_t>(data, data_len, i);
                    ValueIndexPos structure = readIndexPos(data, data_len, i);
                    compiler.static_().static_call_value_function_id_and_ret(cast_to_local(flags), function_id, structure);
                }

                void static_set_structure_value() {
                    auto value_name = readIndexPos(data, data_len, i);
                    auto access = readData<ClassAccess>(data, data_len, i);
                    auto structure = readIndexPos(data, data_len, i);
                    auto set = readIndexPos(data, data_len, i);
                    auto set_meta = readData<ValueMeta>(data, data_len, i);
                    compiler.static_().set_structure_value(value_name, access, structure, set, set_meta);
                }
                void static_get_structure_value() {
                    auto value_name = readIndexPos(data, data_len, i);
                    auto access = readData<ClassAccess>(data, data_len, i);
                    auto structure = readIndexPos(data, data_len, i);
                    auto result = readIndexPos(data, data_len, i);
                    auto result_meta = readData<ValueMeta>(data, data_len, i);
                    compiler.static_().get_structure_value(value_name, access, structure, result, result_meta);
                }
                void static_explicit_await() {
                    ValueIndexPos value = readIndexPos(data, data_len, i);
                    ValueMeta value_meta = readData<ValueMeta>(data, data_len, i);
                    compiler.static_().explicit_await(value, value_meta);
                }
                void static_generator_get() {
                    ValueIndexPos generator = readIndexPos(data, data_len, i);
                    ValueIndexPos result = readIndexPos(data, data_len, i);
                    ValueMeta result_meta = readData<ValueMeta>(data, data_len, i);
                    ValueIndexPos result_index = readIndexPos(data, data_len, i);
                    compiler.static_().generator_get(generator, result,result_meta, result_index);
                }

                void static_arr_op() {
                    ValueIndexPos arr = readIndexPos(data, data_len, i);
                    auto flags = readData<OpArrFlags>(data, data_len, i);
                    auto meta = readData<ValueMeta>(data, data_len, i);
                    auto arr_op = compiler.static_().arr_op(arr, cast_to_local(flags), meta);
                    switch (readData<OpcodeArray>(data, data_len, i)) {
                    case OpcodeArray::set: {
                        ValueIndexPos value = readIndexPos(data, data_len, i);
                        ValueIndexPos index = readIndexPos(data, data_len, i);
                        arr_op.set(index, value);
                        break;
                    }
                    case OpcodeArray::insert: {
                        ValueIndexPos index = readIndexPos(data, data_len, i);
                        ValueIndexPos value = readIndexPos(data, data_len, i);
                        arr_op.insert(index, value);
                        break;
                    }
                    case OpcodeArray::push_end: {
                        ValueIndexPos value = readIndexPos(data, data_len, i);
                        arr_op.push_end(value);
                        break;
                    }
                    case OpcodeArray::push_start: {
                        ValueIndexPos value = readIndexPos(data, data_len, i);
                        arr_op.push_start(value);
                        break;
                    }
                    case OpcodeArray::insert_range: {
                        ValueIndexPos arr1 = readIndexPos(data, data_len, i);
                        ValueMeta arr1_meta = readData<ValueMeta>(data, data_len, i);
                        ValueIndexPos pos = readIndexPos(data, data_len, i);
                        ValueIndexPos start1 = readIndexPos(data, data_len, i);
                        ValueIndexPos end1 = readIndexPos(data, data_len, i);
                        arr_op.insert_range(arr1, arr1_meta, pos, start1, end1);
                        break;
                    }
                    case OpcodeArray::get: {
                        ValueIndexPos index = readIndexPos(data, data_len, i);
                        ValueIndexPos result = readIndexPos(data, data_len, i);
                        arr_op.get(index, result);
                        break;
                    }
                    case OpcodeArray::take: {
                        ValueIndexPos index = readIndexPos(data, data_len, i);
                        ValueIndexPos result = readIndexPos(data, data_len, i);
                        arr_op.take(index, result);
                        break;
                    }
                    case OpcodeArray::take_end: {
                        ValueIndexPos result = readIndexPos(data, data_len, i);
                        arr_op.take_end(result);
                        break;
                    }
                    case OpcodeArray::take_start: {
                        ValueIndexPos result = readIndexPos(data, data_len, i);
                        arr_op.take_start(result);
                        break;
                    }
                    case OpcodeArray::get_range: {
                        ValueIndexPos set_to = readIndexPos(data, data_len, i);
                        ValueIndexPos start = readIndexPos(data, data_len, i);
                        ValueIndexPos end = readIndexPos(data, data_len, i);
                        arr_op.get_range(set_to, start, end);
                        break;
                    }
                    case OpcodeArray::take_range: {
                        ValueIndexPos set_to = readIndexPos(data, data_len, i);
                        ValueIndexPos start = readIndexPos(data, data_len, i);
                        ValueIndexPos end = readIndexPos(data, data_len, i);
                        arr_op.take_range(set_to, start, end);
                        break;
                    }
                    case OpcodeArray::pop_end: {
                        arr_op.pop_end();
                        break;
                    }
                    case OpcodeArray::pop_start: {
                        arr_op.pop_start();
                        break;
                    }
                    case OpcodeArray::remove_item: {
                        ValueIndexPos index = readIndexPos(data, data_len, i);
                        arr_op.remove_item(index);
                        break;
                    }
                    case OpcodeArray::remove_range: {
                        ValueIndexPos start = readIndexPos(data, data_len, i);
                        ValueIndexPos end = readIndexPos(data, data_len, i);
                        arr_op.remove_range(start, end);
                        break;
                    }
                    case OpcodeArray::resize: {
                        ValueIndexPos size = readIndexPos(data, data_len, i);
                        arr_op.resize(size);
                        break;
                    }
                    case OpcodeArray::resize_default: {
                        ValueIndexPos size = readIndexPos(data, data_len, i);
                        ValueIndexPos def = readIndexPos(data, data_len, i);
                        arr_op.resize_default(size, def);
                        break;
                    }
                    case OpcodeArray::reserve_push_end: {
                        ValueIndexPos len = readIndexPos(data, data_len, i);
                        arr_op.reserve_push_end(len);
                        break;
                    }
                    case OpcodeArray::reserve_push_start: {
                        ValueIndexPos len = readIndexPos(data, data_len, i);
                        arr_op.reserve_push_start(len);
                        break;
                    }
                    case OpcodeArray::commit: {
                        arr_op.commit();
                        break;
                    }
                    case OpcodeArray::decommit: {
                        ValueIndexPos blocks = readIndexPos(data, data_len, i);
                        arr_op.decommit(blocks);
                        break;
                    }
                    case OpcodeArray::remove_reserved: {
                        arr_op.remove_reserved();
                        break;
                    }
                    case OpcodeArray::size: {
                        ValueIndexPos result = readIndexPos(data, data_len, i);
                        arr_op.size(result);
                        break;
                    }
                    default:
                        throw InvalidIL("Invalid array operation");
                    }
                }


                void static_is_gc() {
                    bool use_result = readData<bool>(data, data_len, i);
                    auto value = readIndexPos(data, data_len, i);
                    auto meta = readData<ValueMeta>(data, data_len, i);
                    if (use_result) {
                        auto result = readIndexPos(data, data_len, i);
                        auto result_meta = readData<ValueMeta>(data, data_len, i);
                        compiler.static_().is_gc(value, meta, result, result_meta);
                    }
                    else
                        compiler.static_().is_gc(value, meta);
                }
                void static_table_jump() {
                    TableJumpFlags flags = readData<TableJumpFlags>(data, data_len, i);

                    uint64_t fail_too_large = 0;
                    uint64_t fail_too_small = 0;
                    if (flags.too_large == TableJumpCheckFailAction::jump_specified)
                        fail_too_large = readData<uint64_t>(data, data_len, i);
                    if(flags.too_small == TableJumpCheckFailAction::jump_specified && flags.is_signed)
                        fail_too_small = readData<uint64_t>(data, data_len, i);

                    ValueIndexPos value = readIndexPos(data, data_len, i);
                    uint32_t table_size = readData<uint32_t>(data, data_len, i);

                    std::vector<uint64_t> table;
                    table.reserve(table_size);
                    for (uint32_t j = 0; j < table_size; j++)
                        table.push_back(readData<uint64_t>(data, data_len, i));

                    compiler.static_().table_jump(cast_to_local(flags), fail_too_large, fail_too_small, table, value);
                }
                void static_xarray_slice() {
                    ValueIndexPos result_index = readIndexPos(data, data_len, i);
                    ValueMeta result_meta = readData<ValueMeta>(data, data_len, i);
                    ValueIndexPos slice_index = readIndexPos(data, data_len, i);
                    ValueMeta slice_meta = readData<ValueMeta>(data, data_len, i);
                    uint8_t slice_flags = readData<uint8_t>(data, data_len, i);

                    uint8_t slice_type = slice_flags & 0x0F;
                    switch(slice_type) {
                        case 1: {
                            compiler.static_().xarray_slice(result_index, result_meta, slice_index, slice_meta);
                            break;
                        }
                        case 2: {
                            auto to = readIndexPos(data, data_len, i);
                            auto to_meta = readData<ValueMeta>(data, data_len, i);
                            compiler.static_().xarray_slice(result_index, result_meta, slice_index, slice_meta,false, to, to_meta);
                            break;
                        }
                        case 3: {
                            auto from = readIndexPos(data, data_len, i);
                            auto from_meta = readData<ValueMeta>(data, data_len, i);
                            compiler.static_().xarray_slice(result_index, result_meta, slice_index, slice_meta, from, from_meta);
                            break;
                        }
                        case 4: {
                            auto from = readIndexPos(data, data_len, i);
                            auto from_meta = readData<ValueMeta>(data, data_len, i);
                            auto to = readIndexPos(data, data_len, i);
                            auto to_meta = readData<ValueMeta>(data, data_len, i);
                            compiler.static_().xarray_slice(result_index, result_meta, slice_index, slice_meta, from, from_meta, to, to_meta);
                            break;
                        }
                        default:
                            throw InvalidIL("Invalid opcode, unsupported slice type");
                    }
                }
                void static_copy_un_constant() {
                    ValueIndexPos result_index = readIndexPos(data, data_len, i);
                    ValueMeta result_meta = readData<ValueMeta>(data, data_len, i);
                    ValueIndexPos source_index = readIndexPos(data, data_len, i);
                    ValueMeta source_meta = readData<ValueMeta>(data, data_len, i);
                    compiler.static_().copy_unconst(result_index, result_meta, source_index, source_meta);
                }
                void static_copy_un_reference() {
                    ValueIndexPos result_index = readIndexPos(data, data_len, i);
                    ValueMeta result_meta = readData<ValueMeta>(data, data_len, i);
                    ValueIndexPos source_index = readIndexPos(data, data_len, i);
                    ValueMeta source_meta = readData<ValueMeta>(data, data_len, i);
                    compiler.static_().copy_unreference(result_index, result_meta, source_index, source_meta);
                }
                void static_move_un_reference() {
                    ValueIndexPos result_index = readIndexPos(data, data_len, i);
                    ValueMeta result_meta = readData<ValueMeta>(data, data_len, i);
                    ValueIndexPos source_index = readIndexPos(data, data_len, i);
                    ValueMeta source_meta = readData<ValueMeta>(data, data_len, i);
                    compiler.static_().move_unreference(result_index, result_meta, source_index, source_meta);
                }
#pragma endregion
#pragma endregion




                void undefined() {
                    throw InvalidIL("Invalid opcode, undefined opcode");
                }

                void static_build() {
                    switch (cmd.code) {
                        case Opcode::remove: static_remove(); break;
                        case Opcode::sum: static_sum(); break;
                        case Opcode::minus: static_minus(); break;
                        case Opcode::div: static_div(); break;
                        case Opcode::rest: static_rest(); break;
                        case Opcode::mul: static_mul(); break;
                        case Opcode::bit_xor: static_bit_xor(); break;
                        case Opcode::bit_or: static_bit_or(); break;
                        case Opcode::bit_and: static_bit_and(); break;
                        case Opcode::bit_not: static_bit_not(); break;
                        case Opcode::bit_shift_left: static_bit_shift_left(); break;
                        case Opcode::bit_shift_right: static_bit_shift_right(); break;
                        case Opcode::compare: static_compare(); break;
                        case Opcode::arg_set: static_arg_set(); break;
                        case Opcode::call: static_call(); break;
                        case Opcode::call_self: static_call_self(); break;
                        case Opcode::call_local: static_call_local(); break;
                        case Opcode::call_and_ret: static_call_and_ret(); break;
                        case Opcode::call_local_and_ret: static_call_local_and_ret(); break;
                        case Opcode::copy: static_copy(); break;
                        case Opcode::move: static_move(); break;
                        case Opcode::arr_op: static_arr_op(); break;
                        case Opcode::throw_ex: static_throw(); break;
                        case Opcode::as: static_as(); break;
                        case Opcode::is: static_is(); break;
                        case Opcode::store_bool: static_store_bool(); break;
                        case Opcode::load_bool: static_load_bool(); break;
                        case Opcode::call_value_function: static_call_value_function(); break;
                        case Opcode::call_value_function_id: static_call_value_function_id(); break;
                        case Opcode::call_value_function_and_ret: static_call_value_function_and_ret(); break;
                        case Opcode::call_value_function_id_and_ret: static_call_value_function_id_and_ret(); break;
                        case Opcode::static_call_value_function: static_static_call_value_function(); break;
                        case Opcode::static_call_value_function_id: static_static_call_value_function_id(); break;
                        case Opcode::static_call_value_function_and_ret: static_static_call_value_function_and_ret(); break;
                        case Opcode::static_call_value_function_id_and_ret: static_static_call_value_function_id_and_ret(); break;
                        case Opcode::set_structure_value: static_set_structure_value(); break;
                        case Opcode::get_structure_value: static_get_structure_value(); break;
                        case Opcode::explicit_await: static_explicit_await(); break;
                        case Opcode::generator_get: static_generator_get(); break;
                        case Opcode::is_gc: static_is_gc(); break;
                        case Opcode::table_jump: static_table_jump(); break;
                        case Opcode::xarray_slice: static_xarray_slice(); break;
                        case Opcode::store_constant: store_constant(); break;
                        case Opcode::copy_un_constant: static_copy_un_constant(); break;
                        case Opcode::copy_un_reference:static_copy_un_reference(); break;
                        case Opcode::move_un_reference: static_move_un_reference(); break;

                        case Opcode::call_self_and_ret:
                        case Opcode::ret: 
                        case Opcode::ret_take:
                        case Opcode::yield: 
                        case Opcode::jump:
                        case Opcode::ret_noting:
                        case Opcode::inline_native:
                        case Opcode::to_gc:
                        case Opcode::localize_gc:
                        case Opcode::from_gc:
                        case Opcode::get_reference:
                        case Opcode::make_as_const:
                        case Opcode::remove_const_protect:
                        case Opcode::remove_qualifiers:
                        case Opcode::log_not:
                        case Opcode::noting: 
                        case Opcode::create_saarr:
                        case Opcode::debug_break:
                        case Opcode::force_debug_break:
                        case Opcode::handle_begin: 
                        case Opcode::handle_catch: 
                        case Opcode::handle_finally: 
                        case Opcode::handle_end: 
                        case Opcode::value_hold: 
                        case Opcode::value_unhold: throw InvalidIL("Opcode \"" + enum_to_string(cmd.code) + "\" is not supported in static mode"); break;
                        default: undefined(); break;
                    }
                }
                

                void dynamic_build() {
                    switch (cmd.code) {
                        case Opcode::noting: dynamic_noting(); break;
                        case Opcode::create_saarr: dynamic_create_saarr(); break;
                        case Opcode::remove: dynamic_remove(); break;
                        case Opcode::sum: dynamic_sum(); break;
                        case Opcode::minus: dynamic_minus(); break;
                        case Opcode::div: dynamic_div(); break;
                        case Opcode::rest: dynamic_rest(); break;
                        case Opcode::mul: dynamic_mul(); break;
                        case Opcode::bit_xor: dynamic_bit_xor(); break;
                        case Opcode::bit_or: dynamic_bit_or(); break;
                        case Opcode::bit_and: dynamic_bit_and(); break;
                        case Opcode::bit_not: dynamic_bit_not(); break;
                        case Opcode::bit_shift_left: dynamic_bit_shift_left(); break;
                        case Opcode::bit_shift_right: dynamic_bit_shift_right(); break;
                        case Opcode::log_not: dynamic_log_not(); break;
                        case Opcode::compare: dynamic_compare(); break;
                        case Opcode::jump: dynamic_jump(); break;
                        case Opcode::arg_set: dynamic_arg_set(); break;
                        case Opcode::call: dynamic_call(); break;
                        case Opcode::call_self: dynamic_call_self(); break;
                        case Opcode::call_local: dynamic_call_local(); break;
                        case Opcode::call_and_ret: dynamic_call_and_ret(); break;
                        case Opcode::call_self_and_ret: dynamic_call_self_and_ret(); break;
                        case Opcode::call_local_and_ret: dynamic_call_local_and_ret(); break;
                        case Opcode::ret: dynamic_ret(); break;
                        case Opcode::ret_take: dynamic_ret_take(); break;
                        case Opcode::ret_noting: dynamic_ret_noting(); break;
                        case Opcode::copy: dynamic_copy(); break;
                        case Opcode::move: dynamic_move(); break;
                        case Opcode::arr_op: dynamic_arr_op(); break;
                        case Opcode::debug_break: dynamic_debug_break(); break;
                        case Opcode::force_debug_break: dynamic_debug_force_break(); break;
                        case Opcode::throw_ex: dynamic_throw(); break;
                        case Opcode::as: dynamic_as(); break;
                        case Opcode::is: dynamic_is(); break;
                        case Opcode::store_bool: dynamic_store_bool(); break;
                        case Opcode::load_bool: dynamic_load_bool(); break;
                        case Opcode::inline_native: dynamic_insert_native(); break;
                        case Opcode::call_value_function: dynamic_call_value_function(); break;
                        case Opcode::call_value_function_id: dynamic_call_value_function_id(); break;
                        case Opcode::call_value_function_and_ret: dynamic_call_value_function_and_ret(); break;
                        case Opcode::call_value_function_id_and_ret: dynamic_call_value_function_id_and_ret(); break;
                        case Opcode::static_call_value_function: dynamic_static_call_value_function(); break;
                        case Opcode::static_call_value_function_id: dynamic_static_call_value_function_id(); break;
                        case Opcode::static_call_value_function_and_ret: dynamic_static_call_value_function_and_ret(); break;
                        case Opcode::static_call_value_function_id_and_ret: dynamic_static_call_value_function_id_and_ret(); break;
                        case Opcode::set_structure_value: dynamic_set_structure_value(); break;
                        case Opcode::get_structure_value: dynamic_get_structure_value(); break;
                        case Opcode::explicit_await: dynamic_explicit_await(); break;
                        case Opcode::generator_get: dynamic_generator_get(); break;
                        case Opcode::yield: dynamic_yield(); break;
                        case Opcode::handle_begin: dynamic_handle_begin(); break;
                        case Opcode::handle_catch: dynamic_handle_catch(); break;
                        case Opcode::handle_finally: dynamic_handle_finally(); break;
                        case Opcode::handle_end: dynamic_handle_end(); break;
                        case Opcode::value_hold: dynamic_value_hold(); break;
                        case Opcode::value_unhold: dynamic_value_unhold(); break;
                        case Opcode::is_gc: dynamic_is_gc(); break;
                        case Opcode::to_gc: dynamic_to_gc(); break;
                        case Opcode::localize_gc: dynamic_localize_gc(); break;
                        case Opcode::from_gc: dynamic_from_gc(); break;
                        case Opcode::table_jump: dynamic_table_jump(); break;
                        case Opcode::xarray_slice: dynamic_xarray_slice(); break;
                        case Opcode::store_constant: store_constant(); break;
                        case Opcode::get_reference: dynamic_get_reference(); break;
                        case Opcode::make_as_const: dynamic_make_as_const(); break;
                        case Opcode::remove_const_protect: dynamic_remove_const_protect(); break;
                        case Opcode::copy_un_constant: dynamic_copy_un_constant(); break;
                        case Opcode::copy_un_reference:dynamic_copy_un_reference(); break;
                        case Opcode::move_un_reference: dynamic_move_un_reference(); break;
                        case Opcode::remove_qualifiers: dynamic_remove_qualifiers(); break;
                        default: undefined(); break;
                    }
                }

                void build() {
                    for (; i < data_len; ) {
                        compiler.load_current_opcode_label(i);
                        cmd = readData<Command>(data, data_len, i);
                        !cmd.static_mode ? dynamic_build() : static_build();
                    }
                }
            };

            void compiler::build(
                    const std::vector<uint8_t>& data,
                    size_t start,
                    size_t end_offset,
                    Compiler& compiler,
                    FuncHandle::inner_handle* func
            ) {
                CompilerFabric fabric(data, end_offset, start, compiler);
                fabric.build();
            }
            
            
            list_array<std::pair<uint64_t, Label>> prepareJumpList(CASM& a, const std::vector<uint8_t>& data, size_t data_len, size_t& to_be_skiped) {
                if (uint8_t size = data[to_be_skiped++]) {
                    uint64_t labels = 0;
                    switch (size) {
                    case 8:
                        labels = data[to_be_skiped++];
                        labels <<= 8;
                        labels |= data[to_be_skiped++];
                        labels <<= 8;
                        labels |= data[to_be_skiped++];
                        labels <<= 8;
                        labels |= data[to_be_skiped++];
                        labels <<= 8;
                        [[fallthrough]];
                    case 4:
                        labels |= data[to_be_skiped++];
                        labels <<= 8;
                        labels |= data[to_be_skiped++];
                        labels <<= 8;
                        [[fallthrough]];
                    case 2:
                        labels |= data[to_be_skiped++];
                        labels <<= 8;
                        [[fallthrough]];
                    case 1:
                        labels |= data[to_be_skiped++];
                        break;
                    default:
                        throw InvalidFunction("Invalid function header, unsupported label size: " + std::to_string(size) + " bytes, supported: 1,2,4,8");
                    }
                    list_array<std::pair<uint64_t, Label>> res;
                    res.resize(labels);
                    for (uint64_t i = 0; i < labels; i++)
                        res[i] = { reader::readData<uint64_t>(data,data_len,to_be_skiped), a.newLabel() };
                    return res;
                }
                return {};
            }

            void compiler::decode_header(
                const std::vector<uint8_t>& data,
                size_t& to_be_skiped,
                size_t data_len,
                CASM& casm_assembler,
                list_array<std::pair<uint64_t, Label>>& jump_list,
                std::vector<art::shared_ptr<FuncEnvironment>>& locals,
                FunctionMetaFlags& flags,
                uint16_t& used_static_values,
                uint16_t& used_enviro_vals, 
                uint32_t& used_arguments, 
                uint64_t& constants_values) {

                flags = readData<FunctionMetaFlags>(data, data_len, to_be_skiped);
                if(flags.length != data_len)
                    throw InvalidFunction("Invalid function header, invalid function length");

                if(flags.used_static)
                    used_static_values = readData<uint16_t>(data, data_len, to_be_skiped);
                if(flags.used_enviro_vals)
                    used_enviro_vals = readData<uint16_t>(data, data_len, to_be_skiped);
                if(flags.used_arguments)
                    used_arguments = readData<uint32_t>(data, data_len, to_be_skiped);
                constants_values = readPackedLen(data, data_len, to_be_skiped);
                locals.clear();
                if(flags.has_local_functions) {
                    uint64_t locals_count = readPackedLen(data, data_len, to_be_skiped);
                    locals.resize(locals_count);
                    for (uint64_t i = 0; i < locals_count; i++) {
                        uint64_t local_fn_len = readPackedLen(data, data_len, to_be_skiped);
                        uint8_t* local_fn = extractRawArray<uint8_t>(data, data_len, to_be_skiped, local_fn_len);
                        std::vector<uint8_t> local_fn_data(local_fn, local_fn + local_fn_len);
                        locals[i] = new FuncEnvironment(local_fn_data);
                    }
                }
                jump_list = prepareJumpList(casm_assembler, data, data_len, to_be_skiped);
            }
        }
    }
}