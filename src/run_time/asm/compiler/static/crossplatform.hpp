// Copyright Danyil Melnytskyi 2022-Present
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <run_time/asm/compiler/compiler_include.hpp>

namespace art {
    void _inlineReleaseUnused(CASM& a, creg64 reg);
    void Compiler::StaticCompiler::remove(const ValueIndexPos& value_index, ValueMeta value_index_meta) {
        if (needAlloc(value_index_meta)) {
            switch (value_index_meta.vtype) {
            case VType::raw_arr_i8: {
                BuildCall b(compiler.a, 1);
                b.mov_valindex({compiler.static_map, compiler.values}, value_index);
                b.finalize(arrayDestructor<int8_t>);
                break;
            }
            case VType::raw_arr_i16: {
                BuildCall b(compiler.a, 1);
                b.mov_valindex({compiler.static_map, compiler.values}, value_index);
                b.finalize(arrayDestructor<int16_t>);
                break;
            }
            case VType::raw_arr_i32: {
                BuildCall b(compiler.a, 1);
                b.mov_valindex({compiler.static_map, compiler.values}, value_index);
                b.finalize(arrayDestructor<int32_t>);
                break;
            }
            case VType::raw_arr_i64: {
                BuildCall b(compiler.a, 1);
                b.mov_valindex({compiler.static_map, compiler.values}, value_index);
                b.finalize(arrayDestructor<int64_t>);
                break;
            }
            case VType::raw_arr_ui8: {
                BuildCall b(compiler.a, 1);
                b.mov_valindex({compiler.static_map, compiler.values}, value_index);
                b.finalize(arrayDestructor<uint8_t>);
                break;
            }
            case VType::raw_arr_ui16: {
                BuildCall b(compiler.a, 1);
                b.mov_valindex({compiler.static_map, compiler.values}, value_index);
                b.finalize(arrayDestructor<uint16_t>);
                break;
            }
            case VType::raw_arr_ui32: {
                BuildCall b(compiler.a, 1);
                b.mov_valindex({compiler.static_map, compiler.values}, value_index);
                b.finalize(arrayDestructor<uint32_t>);
                break;
            }
            case VType::raw_arr_ui64: {
                BuildCall b(compiler.a, 1);
                b.mov_valindex({compiler.static_map, compiler.values}, value_index);
                b.finalize(arrayDestructor<uint64_t>);
                break;
            }
            case VType::raw_arr_flo: {
                BuildCall b(compiler.a, 1);
                b.mov_valindex({compiler.static_map, compiler.values}, value_index);
                b.finalize(arrayDestructor<float>);
                break;
            }
            case VType::raw_arr_doub: {
                BuildCall b(compiler.a, 1);
                b.mov_valindex({compiler.static_map, compiler.values}, value_index);
                b.finalize(arrayDestructor<double>);
                break;
            }
            case VType::function: {
                BuildCall b(compiler.a, 1);
                b.mov_valindex({compiler.static_map, compiler.values}, value_index);
                b.finalize(defaultDestructor<art::shared_ptr<FuncEnvironment>>);
                break;
            }
            case VType::faarr: {
                BuildCall b(compiler.a, 1);
                b.mov_valindex({compiler.static_map, compiler.values}, value_index);
                b.finalize(arrayDestructor<ValueItem>);
                break;
            }
            case VType::uarr: {
                BuildCall b(compiler.a, 1);
                b.mov_valindex({compiler.static_map, compiler.values}, value_index);
                b.finalize(defaultDestructor<list_array<ValueItem>>);
                break;
            }
            case VType::except_value: {
                BuildCall b(compiler.a, 1);
                b.mov_valindex({compiler.static_map, compiler.values}, value_index);
                b.finalize(defaultDestructor<std::exception_ptr>);
                break;
            }
            case VType::string: {
                BuildCall b(compiler.a, 1);
                b.mov_valindex({compiler.static_map, compiler.values}, value_index);
                b.finalize(defaultDestructor<art::ustring>);
                break;
            }
            case VType::async_res: {
                BuildCall b(compiler.a, 1);
                b.mov_valindex({compiler.static_map, compiler.values}, value_index);
                b.finalize(defaultDestructor<art::typed_lgr<Task>>);
                break;
            }
            case VType::struct_: {
                BuildCall b(compiler.a, 1);
                b.mov_valindex({compiler.static_map, compiler.values}, value_index);
                b.finalize(Structure::destruct);
                break;
            }
            default:
                BuildCall b(compiler.a, 1);
                b.lea_valindex({compiler.static_map, compiler.values}, value_index);
                b.finalize(universalRemove);
            }
        }
        compiler.a.mov_valindex_meta({compiler.static_map, compiler.values}, value_index, 0);
    }
    void Compiler::StaticCompiler::copy(const ValueIndexPos& to, ValueMeta to_meta, const ValueIndexPos& from, ValueMeta from_meta) {
        //TODO: optimize
        compiler.dynamic().copy(to, from);
    }
    void Compiler::StaticCompiler::move(const ValueIndexPos& to, ValueMeta to_meta, const ValueIndexPos& from, ValueMeta from_meta) {
        if (needAlloc(to_meta)) {
            if (to_meta.encoded == from_meta.encoded) {
                if (to_meta.use_gc) {
                    BuildCall b(compiler.a, 2);
                    b.mov_valindex({compiler.static_map, compiler.values}, to);
                    b.mov_valindex({compiler.static_map, compiler.values}, from);
                    b.finalize((lgr & (lgr::*)(lgr&&)) & lgr::operator=);
                } else {
                    switch (to_meta.vtype) {
                    case VType::uarr: {
                        BuildCall b(compiler.a, 2);
                        b.mov_valindex({compiler.static_map, compiler.values}, to);
                        b.mov_valindex({compiler.static_map, compiler.values}, from);
                        b.finalize((list_array<ValueItem> & (list_array<ValueItem>::*)(list_array<ValueItem>&&)) & list_array<ValueItem>::operator=);
                        break;
                    }
                    case VType::string: {
                        BuildCall b(compiler.a, 2);
                        b.mov_valindex({compiler.static_map, compiler.values}, to);
                        b.mov_valindex({compiler.static_map, compiler.values}, from);
                        b.finalize((art::ustring & (art::ustring::*)(art::ustring&&)) & art::ustring::operator=);
                        break;
                    }
                    case VType::async_res: {
                        BuildCall b(compiler.a, 2);
                        b.mov_valindex({compiler.static_map, compiler.values}, to);
                        b.mov_valindex({compiler.static_map, compiler.values}, from);
                        b.finalize((art::typed_lgr<Task> & (art::typed_lgr<Task>::*)(art::typed_lgr<Task>&&)) & art::typed_lgr<Task>::operator=);
                        break;
                    }
                    case VType::except_value: {
                        compiler.a.mov_valindex({compiler.static_map, compiler.values}, resr, from);
                        compiler.a.mov_valindex({compiler.static_map, compiler.values}, to, resr);
                        compiler.a.mov_valindex({compiler.static_map, compiler.values}, from, 0);
                        break;
                    }
                    case VType::struct_: {
                        BuildCall b(compiler.a, 3);
                        b.mov_valindex({compiler.static_map, compiler.values}, to);
                        b.mov_valindex({compiler.static_map, compiler.values}, from);
                        b.addArg(false);
                        b.finalize((void (*)(Structure*, Structure*, bool))Structure::move);
                        break;
                    }
                    case VType::function: {
                        BuildCall b(compiler.a, 2);
                        b.mov_valindex({compiler.static_map, compiler.values}, to);
                        b.mov_valindex({compiler.static_map, compiler.values}, from);
                        b.finalize((art::shared_ptr<FuncEnvironment> & (art::shared_ptr<FuncEnvironment>::*)(art::shared_ptr<FuncEnvironment>&&)) & art::shared_ptr<FuncEnvironment>::operator=);
                        break;
                    }
                    default:
                        compiler.dynamic().move(to, from);
                        break;
                    }
                }
            } else
                compiler.dynamic().move(to, from);
        } else {
            compiler.a.mov_valindex({compiler.static_map, compiler.values}, resr, from);
            compiler.a.mov_valindex({compiler.static_map, compiler.values}, to, resr);
            compiler.a.mov_valindex_meta({compiler.static_map, compiler.values}, resr, from);
            compiler.a.mov_valindex_meta({compiler.static_map, compiler.values}, to, resr);
            compiler.a.mov_valindex({compiler.static_map, compiler.values}, from, 0);
        }
    }
    void Compiler::StaticCompiler::sum(const ValueIndexPos& to, ValueMeta to_meta, const ValueIndexPos& src, ValueMeta src_meta) {
        //TODO: optimize
        compiler.dynamic().sum(to, src);
    }
    void Compiler::StaticCompiler::sub(const ValueIndexPos& to, ValueMeta to_meta, const ValueIndexPos& src, ValueMeta src_meta) {
        //TODO: optimize
        compiler.dynamic().sub(to, src);
    }
    void Compiler::StaticCompiler::mul(const ValueIndexPos& to, ValueMeta to_meta, const ValueIndexPos& src, ValueMeta src_meta) {
        //TODO: optimize
        compiler.dynamic().mul(to, src);
    }
    void Compiler::StaticCompiler::div(const ValueIndexPos& to, ValueMeta to_meta, const ValueIndexPos& src, ValueMeta src_meta) {
        //TODO: optimize
        compiler.dynamic().div(to, src);
    }
    void Compiler::StaticCompiler::rest(const ValueIndexPos& to, ValueMeta to_meta, const ValueIndexPos& src, ValueMeta src_meta) {
        //TODO: optimize
        compiler.dynamic().rest(to, src);
    }

    void Compiler::StaticCompiler::bit_xor(const ValueIndexPos& to, ValueMeta to_meta, const ValueIndexPos& src, ValueMeta src_meta) {
        //TODO: optimize
        compiler.dynamic().bit_xor(to, src);
    }
    void Compiler::StaticCompiler::bit_and(const ValueIndexPos& to, ValueMeta to_meta, const ValueIndexPos& src, ValueMeta src_meta) {
        //TODO: optimize
        compiler.dynamic().bit_and(to, src);
    }
    void Compiler::StaticCompiler::bit_or(const ValueIndexPos& to, ValueMeta to_meta, const ValueIndexPos& src, ValueMeta src_meta) {
        //TODO: optimize
        compiler.dynamic().bit_or(to, src);
    }
    void Compiler::StaticCompiler::bit_not(const ValueIndexPos& to, ValueMeta to_meta) {
        //TODO: optimize
        compiler.dynamic().bit_not(to);
    }
    void Compiler::StaticCompiler::bit_left_shift(const ValueIndexPos& to, ValueMeta to_meta, const ValueIndexPos& src, ValueMeta src_meta) {
        //TODO: optimize
        compiler.dynamic().bit_left_shift(to, src);
    }
    void Compiler::StaticCompiler::bit_right_shift(const ValueIndexPos& to, ValueMeta to_meta, const ValueIndexPos& src, ValueMeta src_meta) {
        //TODO: optimize
        compiler.dynamic().bit_right_shift(to, src);
    }

    void Compiler::StaticCompiler::compare(const ValueIndexPos& a, ValueMeta a_meta, const ValueIndexPos& b, ValueMeta b_meta) {
        //TODO: optimize
        compiler.dynamic().compare(a, b);
    }

    void Compiler::StaticCompiler::arg_set(const ValueIndexPos& src) {
        compiler.a.mov(arg_ptr, resr);
        compiler.a.mov_valindex_meta_size({compiler.static_map, compiler.values}, arg_len_32, src);
    }
    void Compiler::StaticCompiler::call(const ValueIndexPos& fn_symbol, ValueMeta fn_symbol_meta, CallFlags flags) {
        //TODO: optimize
        compiler.dynamic().call(fn_symbol, flags);
    }
    void Compiler::StaticCompiler::call(const ValueIndexPos& fn_symbol, ValueMeta fn_symbol_meta, CallFlags flags, const ValueIndexPos& res, ValueMeta res_meta) {
        //TODO: optimize
        compiler.dynamic().call(fn_symbol, flags, res);
    }
    void Compiler::StaticCompiler::call_self(const ValueIndexPos& res, ValueMeta res_meta) {
        //TODO: optimize
        compiler.dynamic().call_self(res);
    }
    void Compiler::StaticCompiler::call_local(const ValueIndexPos& fn_symbol, ValueMeta fn_symbol_meta, CallFlags flags) {
        //TODO: optimize
        compiler.dynamic().call_local(fn_symbol, flags);
    }
    void Compiler::StaticCompiler::call_local(const ValueIndexPos& fn_symbol, ValueMeta fn_symbol_meta, CallFlags flags, const ValueIndexPos& res, ValueMeta res_meta) {
        //TODO: optimize
        compiler.dynamic().call_local(fn_symbol, flags, res);
    }

    void Compiler::StaticCompiler::call_and_ret(const ValueIndexPos& fn_symbol, ValueMeta fn_symbol_meta, CallFlags flags) {
        //TODO: optimize
        compiler.dynamic().call_and_ret(fn_symbol, flags);
    }
    void Compiler::StaticCompiler::call_local_and_ret(const ValueIndexPos& fn_symbol, ValueMeta fn_symbol_meta, CallFlags flags) {
        //TODO: optimize
        compiler.dynamic().call_local_and_ret(fn_symbol, flags);
    }

    //TODO: add support for gc arrays
    void Compiler::StaticCompiler::ArrayOperation::set(const ValueIndexPos& index, const ValueIndexPos& value) {
        if (array_meta.allow_edit == false)
            throw InvalidOperation("Cannot edit constant value");
        if (!is_raw_array(array_meta.vtype) && array_meta.vtype != VType::uarr) {
            BuildCall b(compiler.a, 1);
            b.lea_valindex({compiler.static_map, compiler.values}, array);
            b.finalize(AsArr);
            array_meta.vtype = VType::uarr;
        }
        BuildCall b(compiler.a, 3);
        if (index.pos == ValuePos::in_constants) {
            b.lea_valindex({compiler.static_map, compiler.values}, value);
            b.lea_valindex({compiler.static_map, compiler.values}, array);
            b.addArg(compiler.get_size_constant(index));
        } else {
            b.setArguments(1);
            b.lea_valindex({compiler.static_map, compiler.values}, index);
            b.finalize(getSize);
            b.setArguments(3);
            b.lea_valindex({compiler.static_map, compiler.values}, value);
            b.lea_valindex({compiler.static_map, compiler.values}, array);
            b.addArg(resr);
        }
        if (flags.move_mode) {
            switch (flags.checked) {
            case ArrCheckMode::no_check:
                helper_functions::inlineIndexArraySetMoveStatic<0>(b, array_meta.vtype);
                break;
            case ArrCheckMode::check:
                helper_functions::inlineIndexArraySetMoveStatic<1>(b, array_meta.vtype);
                break;
            case ArrCheckMode::no_throw_check:
                helper_functions::inlineIndexArraySetMoveStatic<2>(b, array_meta.vtype);
                break;
            default:
                break;
            }
        } else {
            switch (flags.checked) {
            case ArrCheckMode::no_check:
                helper_functions::inlineIndexArraySetCopyStatic<0>(b, array_meta.vtype);
                break;
            case ArrCheckMode::check:
                helper_functions::inlineIndexArraySetCopyStatic<1>(b, array_meta.vtype);
                break;
            case ArrCheckMode::no_throw_check:
                helper_functions::inlineIndexArraySetCopyStatic<2>(b, array_meta.vtype);
                break;
            default:
                break;
            }
        }
    }
    void Compiler::StaticCompiler::ArrayOperation::insert(const ValueIndexPos& index, const ValueIndexPos& value) {
        if (array_meta.allow_edit == false)
            throw InvalidOperation("Cannot edit constant value");
        if (array_meta.vtype != VType::uarr) {
            BuildCall b(compiler.a, 1);
            b.lea_valindex({compiler.static_map, compiler.values}, array);
            b.finalize(AsArr);
        }
        BuildCall b(compiler.a, 3);
        if (index.pos == ValuePos::in_constants) {
            b.mov_valindex({compiler.static_map, compiler.values}, array);
            b.addArg(compiler.get_size_constant(index));
            b.lea_valindex({compiler.static_map, compiler.values}, value);
        } else {
            b.setArguments(1);
            b.lea_valindex({compiler.static_map, compiler.values}, index);
            b.finalize(getSize);

            b.setArguments(3);
            b.mov_valindex({compiler.static_map, compiler.values}, array);
            b.addArg(resr);
            b.lea_valindex({compiler.static_map, compiler.values}, value);
        }
        if (flags.move_mode)
            b.finalize((void(list_array<ValueItem>::*)(size_t, const ValueItem&)) & list_array<ValueItem>::insert);
        else
            b.finalize((void(list_array<ValueItem>::*)(size_t, ValueItem&&)) & list_array<ValueItem>::insert);
    }
    void Compiler::StaticCompiler::ArrayOperation::push_end(const ValueIndexPos& value) {
        if (array_meta.allow_edit == false)
            throw InvalidOperation("Cannot edit constant value");
        if (array_meta.vtype != VType::uarr) {
            BuildCall b(compiler.a, 1);
            b.lea_valindex({compiler.static_map, compiler.values}, array);
            b.finalize(AsArr);
        }
        BuildCall b(compiler.a, 2);
        b.mov_valindex({compiler.static_map, compiler.values}, array);
        b.lea_valindex({compiler.static_map, compiler.values}, value);
        if (flags.move_mode)
            b.finalize((void(list_array<ValueItem>::*)(ValueItem&&)) & list_array<ValueItem>::push_back);
        else
            b.finalize((void(list_array<ValueItem>::*)(const ValueItem&)) & list_array<ValueItem>::push_back);
    }
    void Compiler::StaticCompiler::ArrayOperation::push_start(const ValueIndexPos& value) {
        if (array_meta.allow_edit == false)
            throw InvalidOperation("Cannot edit constant value");
        if (array_meta.vtype != VType::uarr) {
            BuildCall b(compiler.a, 1);
            b.lea_valindex({compiler.static_map, compiler.values}, array);
            b.finalize(AsArr);
        }
        BuildCall b(compiler.a, 2);
        b.mov_valindex({compiler.static_map, compiler.values}, array);
        b.lea_valindex({compiler.static_map, compiler.values}, value);
        if (flags.move_mode)
            b.finalize((void(list_array<ValueItem>::*)(ValueItem&&)) & list_array<ValueItem>::push_front);
        else
            b.finalize((void(list_array<ValueItem>::*)(const ValueItem&)) & list_array<ValueItem>::push_front);
    }
    void Compiler::StaticCompiler::ArrayOperation::insert_range(const ValueIndexPos& array2, ValueMeta array2_meta, const ValueIndexPos& index, const ValueIndexPos& from2, const ValueIndexPos& to2) {
        BuildCall b(compiler.a, 1);
        if (array_meta.allow_edit == false)
            throw InvalidOperation("Cannot edit constant value");
        if (array_meta.vtype != VType::uarr) {
            BuildCall b(compiler.a, 1);
            b.lea_valindex({compiler.static_map, compiler.values}, array);
            b.finalize(AsArr);
        }
        if (array2_meta.vtype != VType::uarr) {
            if (array2_meta.allow_edit == false)
                throw InvalidOperation("Cannot insert range from non-editable value");
            b.lea_valindex({compiler.static_map, compiler.values}, array2);
            b.finalize(AsArr);
        }
        if (index.pos == ValuePos::in_constants && from2.pos == ValuePos::in_constants && to2.pos == ValuePos::in_constants) {
            b.setArguments(5);
            b.mov_valindex({compiler.static_map, compiler.values}, array);
            b.addArg(compiler.get_size_constant(index));
            b.mov_valindex({compiler.static_map, compiler.values}, array2);
            b.addArg(compiler.get_size_constant(from2));
            b.addArg(compiler.get_size_constant(to2));
        } else if (index.pos == ValuePos::in_constants && from2.pos == ValuePos::in_constants) {
            b.lea_valindex({compiler.static_map, compiler.values}, to2);
            b.finalize(getSize);

            b.setArguments(5);
            b.mov_valindex({compiler.static_map, compiler.values}, array);
            b.addArg(compiler.get_size_constant(index));
            b.mov_valindex({compiler.static_map, compiler.values}, array2);
            b.addArg(compiler.get_size_constant(from2));
            b.addArg(resr);
        } else if (index.pos == ValuePos::in_constants && to2.pos == ValuePos::in_constants) {
            b.lea_valindex({compiler.static_map, compiler.values}, from2);
            b.finalize(getSize);

            b.setArguments(5);
            b.mov_valindex({compiler.static_map, compiler.values}, array);
            b.addArg(compiler.get_size_constant(index));
            b.mov_valindex({compiler.static_map, compiler.values}, array2);
            b.addArg(resr);
            b.addArg(compiler.get_size_constant(to2));
        } else if (from2.pos == ValuePos::in_constants && to2.pos == ValuePos::in_constants) {
            b.lea_valindex({compiler.static_map, compiler.values}, index);
            b.finalize(getSize);

            b.setArguments(5);
            b.mov_valindex({compiler.static_map, compiler.values}, array);
            b.addArg(resr);
            b.mov_valindex({compiler.static_map, compiler.values}, array2);
            b.addArg(compiler.get_size_constant(from2));
            b.addArg(compiler.get_size_constant(to2));
        } else if (index.pos == ValuePos::in_constants) {
            b.lea_valindex({compiler.static_map, compiler.values}, to2);
            b.finalize(getSize);
            compiler.a.push(resr);
            compiler.a.push(0); //align

            b.lea_valindex({compiler.static_map, compiler.values}, from2);
            b.finalize(getSize);
            compiler.a.pop(); //align

            b.setArguments(5);
            b.mov_valindex({compiler.static_map, compiler.values}, array);
            b.addArg(compiler.get_size_constant(index));
            b.mov_valindex({compiler.static_map, compiler.values}, array2);
            b.addArg(resr);
            compiler.a.pop(resr);
            b.addArg(resr);
        } else if (from2.pos == ValuePos::in_constants) {
            b.lea_valindex({compiler.static_map, compiler.values}, to2);
            b.finalize(getSize);
            compiler.a.push(resr);
            compiler.a.push(0); //align

            b.lea_valindex({compiler.static_map, compiler.values}, index);
            b.finalize(getSize);
            compiler.a.pop(); //align

            b.setArguments(5);
            b.mov_valindex({compiler.static_map, compiler.values}, array);
            b.addArg(resr);
            b.mov_valindex({compiler.static_map, compiler.values}, array2);
            b.addArg(compiler.get_size_constant(from2));
            compiler.a.pop(resr);
            b.addArg(resr);
        } else if (to2.pos == ValuePos::in_constants) {
            b.lea_valindex({compiler.static_map, compiler.values}, from2);
            b.finalize(getSize);
            compiler.a.push(resr);
            compiler.a.push(0); //align

            b.lea_valindex({compiler.static_map, compiler.values}, index);
            b.finalize(getSize);
            compiler.a.pop(); //align

            b.setArguments(5);
            b.mov_valindex({compiler.static_map, compiler.values}, array);
            b.addArg(resr);
            b.mov_valindex({compiler.static_map, compiler.values}, array2);
            compiler.a.pop(resr);
            b.addArg(resr);
            b.addArg(compiler.get_size_constant(to2));
        } else {
            b.lea_valindex({compiler.static_map, compiler.values}, to2);
            b.finalize(getSize);
            compiler.a.push(resr);
            compiler.a.push(0); //align

            b.lea_valindex({compiler.static_map, compiler.values}, from2);
            b.finalize(getSize);
            compiler.a.pop(); //align
            compiler.a.push(resr);

            b.lea_valindex({compiler.static_map, compiler.values}, index);
            b.finalize(getSize);


            b.setArguments(5);
            b.mov_valindex({compiler.static_map, compiler.values}, array);
            b.addArg(resr); //index
            b.mov_valindex({compiler.static_map, compiler.values}, array2);
            compiler.a.pop(resr);
            b.addArg(resr); //from
            compiler.a.pop(resr);
            b.addArg(resr); //to
        }
        b.finalize((void(list_array<ValueItem>::*)(size_t, const list_array<ValueItem>&, size_t, size_t)) & list_array<ValueItem>::insert);
    }
    void Compiler::StaticCompiler::ArrayOperation::get(const ValueIndexPos& index, const ValueIndexPos& set) {
        if (!is_raw_array(array_meta.vtype) && array_meta.vtype != VType::uarr) {
            BuildCall b(compiler.a, 1);
            b.lea_valindex({compiler.static_map, compiler.values}, array);
            b.finalize(AsArr);
            array_meta.vtype = VType::uarr;
        }
        BuildCall b(compiler.a, 3);
        if (index.pos == ValuePos::in_constants) {
            b.lea_valindex({compiler.static_map, compiler.values}, set);
            b.lea_valindex({compiler.static_map, compiler.values}, array);
            b.addArg(compiler.get_size_constant(index));
        } else {
            b.setArguments(1);
            b.lea_valindex({compiler.static_map, compiler.values}, index);
            b.finalize(getSize);

            b.setArguments(3);
            b.lea_valindex({compiler.static_map, compiler.values}, set);
            b.lea_valindex({compiler.static_map, compiler.values}, array);
            b.addArg(resr);
        }
        if (flags.move_mode) {
            switch (flags.checked) {
            case ArrCheckMode::no_check:
                helper_functions::inlineIndexArrayMoveStatic<0>(b, array_meta.vtype);
                break;
            case ArrCheckMode::check:
                helper_functions::inlineIndexArrayMoveStatic<1>(b, array_meta.vtype);
                break;
            case ArrCheckMode::no_throw_check:
                helper_functions::inlineIndexArrayMoveStatic<2>(b, array_meta.vtype);
                break;
            default:
                break;
            }
        } else {
            switch (flags.checked) {
            case ArrCheckMode::no_check:
                helper_functions::inlineIndexArrayCopyStatic<0>(b, array_meta.vtype);
                break;
            case ArrCheckMode::check:
                helper_functions::inlineIndexArrayCopyStatic<1>(b, array_meta.vtype);
                break;
            case ArrCheckMode::no_throw_check:
                helper_functions::inlineIndexArrayCopyStatic<2>(b, array_meta.vtype);
                break;
            default:
                break;
            }
        }
    }
    void Compiler::StaticCompiler::ArrayOperation::take(const ValueIndexPos& index, const ValueIndexPos& set) {
        if (array_meta.allow_edit == false)
            throw InvalidOperation("Cannot edit constant value");
        if (array_meta.vtype != VType::uarr) {
            BuildCall b(compiler.a, 1);
            b.lea_valindex({compiler.static_map, compiler.values}, array);
            b.finalize(AsArr);
            array_meta.vtype = VType::uarr;
        }
        BuildCall b(compiler.a, 3);
        if (index.pos == ValuePos::in_constants) {
            b.lea_valindex({compiler.static_map, compiler.values}, set);
            b.mov_valindex({compiler.static_map, compiler.values}, array);
            b.addArg(compiler.get_size_constant(index));
        } else {
            b.setArguments(1);
            b.lea_valindex({compiler.static_map, compiler.values}, index);
            b.finalize(getSize);

            b.setArguments(3);
            b.lea_valindex({compiler.static_map, compiler.values}, set);
            b.mov_valindex({compiler.static_map, compiler.values}, array);
            b.addArg(resr);
        }
        b.finalize(helper_functions::take);
    }
    void Compiler::StaticCompiler::ArrayOperation::take_end(const ValueIndexPos& set) {
        if (array_meta.allow_edit == false)
            throw InvalidOperation("Cannot edit constant value");
        if (array_meta.vtype != VType::uarr) {
            BuildCall b(compiler.a, 1);
            b.lea_valindex({compiler.static_map, compiler.values}, array);
            b.finalize(AsArr);
            array_meta.vtype = VType::uarr;
        }
        BuildCall b(compiler.a, 2);
        b.mov_valindex({compiler.static_map, compiler.values}, array);
        b.lea_valindex({compiler.static_map, compiler.values}, set);
        b.finalize(helper_functions::takeEnd);
    }
    void Compiler::StaticCompiler::ArrayOperation::take_start(const ValueIndexPos& set) {
        if (array_meta.allow_edit == false)
            throw InvalidOperation("Cannot edit constant value");
        if (array_meta.vtype != VType::uarr) {
            BuildCall b(compiler.a, 1);
            b.lea_valindex({compiler.static_map, compiler.values}, array);
            b.finalize(AsArr);
            array_meta.vtype = VType::uarr;
        }
        BuildCall b(compiler.a, 2);
        b.mov_valindex({compiler.static_map, compiler.values}, array);
        b.lea_valindex({compiler.static_map, compiler.values}, set);
        b.finalize(helper_functions::takeStart);
    }
    void Compiler::StaticCompiler::ArrayOperation::get_range(const ValueIndexPos& set, const ValueIndexPos& from, const ValueIndexPos& to) {
        if (array_meta.vtype != VType::uarr) {
            BuildCall b(compiler.a, 1);
            b.lea_valindex({compiler.static_map, compiler.values}, array);
            b.finalize(AsArr);
            array_meta.vtype = VType::uarr;
        }

        BuildCall b(compiler.a, 4);
        if (from.pos == ValuePos::in_constants && to.pos == ValuePos::in_constants) {
            b.mov_valindex({compiler.static_map, compiler.values}, array);
            b.lea_valindex({compiler.static_map, compiler.values}, set);
            b.addArg(compiler.get_size_constant(from));
            b.addArg(compiler.get_size_constant(to));
        } else if (from.pos == ValuePos::in_constants) {
            b.setArguments(1);
            b.lea_valindex({compiler.static_map, compiler.values}, to);
            b.finalize(getSize);
            b.setArguments(4);
            b.mov_valindex({compiler.static_map, compiler.values}, array);
            b.lea_valindex({compiler.static_map, compiler.values}, set);
            b.addArg(compiler.get_size_constant(from));
            b.addArg(resr);
        } else if (to.pos == ValuePos::in_constants) {
            b.setArguments(1);
            b.lea_valindex({compiler.static_map, compiler.values}, from);
            b.finalize(getSize);
            b.setArguments(4);
            b.mov_valindex({compiler.static_map, compiler.values}, array);
            b.lea_valindex({compiler.static_map, compiler.values}, set);
            b.addArg(resr);
            b.addArg(compiler.get_size_constant(to));
        } else {
            b.setArguments(1);
            b.lea_valindex({compiler.static_map, compiler.values}, to);
            b.finalize(getSize);
            compiler.a.push(resr);
            compiler.a.push(0);
            b.lea_valindex({compiler.static_map, compiler.values}, from);
            b.finalize(getSize);
            compiler.a.pop();
            b.setArguments(4);
            b.mov_valindex({compiler.static_map, compiler.values}, array);
            b.lea_valindex({compiler.static_map, compiler.values}, set);
            b.addArg(resr);
            compiler.a.pop(resr);
            b.addArg(resr);
        }
        b.finalize(helper_functions::getRange);
    }
    void Compiler::StaticCompiler::ArrayOperation::take_range(const ValueIndexPos& set, const ValueIndexPos& from, const ValueIndexPos& to) {
        if (array_meta.allow_edit == false)
            throw InvalidOperation("Cannot edit constant value");
        if (array_meta.vtype != VType::uarr) {
            BuildCall b(compiler.a, 1);
            b.lea_valindex({compiler.static_map, compiler.values}, array);
            b.finalize(AsArr);
            array_meta.vtype = VType::uarr;
        }

        BuildCall b(compiler.a, 4);
        if (from.pos == ValuePos::in_constants && to.pos == ValuePos::in_constants) {
            b.mov_valindex({compiler.static_map, compiler.values}, array);
            b.lea_valindex({compiler.static_map, compiler.values}, set);
            b.addArg(compiler.get_size_constant(from));
            b.addArg(compiler.get_size_constant(to));
        } else if (from.pos == ValuePos::in_constants) {
            b.setArguments(1);
            b.lea_valindex({compiler.static_map, compiler.values}, to);
            b.finalize(getSize);
            b.setArguments(4);
            b.mov_valindex({compiler.static_map, compiler.values}, array);
            b.lea_valindex({compiler.static_map, compiler.values}, set);
            b.addArg(compiler.get_size_constant(from));
            b.addArg(resr);
        } else if (to.pos == ValuePos::in_constants) {
            b.setArguments(1);
            b.lea_valindex({compiler.static_map, compiler.values}, from);
            b.finalize(getSize);
            b.setArguments(4);
            b.mov_valindex({compiler.static_map, compiler.values}, array);
            b.lea_valindex({compiler.static_map, compiler.values}, set);
            b.addArg(resr);
            b.addArg(compiler.get_size_constant(to));
        } else {
            b.setArguments(1);
            b.lea_valindex({compiler.static_map, compiler.values}, to);
            b.finalize(getSize);
            compiler.a.push(resr);
            compiler.a.push(0);
            b.lea_valindex({compiler.static_map, compiler.values}, from);
            b.finalize(getSize);
            compiler.a.pop();
            b.setArguments(4);
            b.mov_valindex({compiler.static_map, compiler.values}, array);
            b.lea_valindex({compiler.static_map, compiler.values}, set);
            b.addArg(resr);
            compiler.a.pop(resr);
            b.addArg(resr);
        }
        b.finalize(helper_functions::takeRange);
    }
    void Compiler::StaticCompiler::ArrayOperation::pop_end() {
        if (array_meta.allow_edit == false)
            throw InvalidOperation("Cannot edit constant value");
        if (array_meta.vtype != VType::uarr) {
            BuildCall b(compiler.a, 1);
            b.lea_valindex({compiler.static_map, compiler.values}, array);
            b.finalize(AsArr);
            array_meta.vtype = VType::uarr;
        }
        BuildCall b(compiler.a, 2);
        b.mov_valindex({compiler.static_map, compiler.values}, array);
        b.finalize(&list_array<ValueItem>::pop_back);
    }
    void Compiler::StaticCompiler::ArrayOperation::pop_start() {
        if (array_meta.allow_edit == false)
            throw InvalidOperation("Cannot edit constant value");
        if (array_meta.vtype != VType::uarr) {
            BuildCall b(compiler.a, 1);
            b.lea_valindex({compiler.static_map, compiler.values}, array);
            b.finalize(AsArr);
            array_meta.vtype = VType::uarr;
        }
        BuildCall b(compiler.a, 2);
        b.mov_valindex({compiler.static_map, compiler.values}, array);
        b.finalize(&list_array<ValueItem>::pop_front);
    }
    void Compiler::StaticCompiler::ArrayOperation::remove_item(const ValueIndexPos& index) {
        if (array_meta.allow_edit == false)
            throw InvalidOperation("Cannot edit constant value");
        if (array_meta.vtype != VType::uarr) {
            BuildCall b(compiler.a, 1);
            b.lea_valindex({compiler.static_map, compiler.values}, array);
            b.finalize(AsArr);
            array_meta.vtype = VType::uarr;
        }
        BuildCall b(compiler.a, 2);
        if (index.pos == ValuePos::in_constants) {
            b.mov_valindex({compiler.static_map, compiler.values}, array);
            b.addArg(compiler.get_size_constant(index));
        } else {
            b.setArguments(1);
            b.lea_valindex({compiler.static_map, compiler.values}, index);
            b.finalize(getSize);

            b.setArguments(2);
            b.mov_valindex({compiler.static_map, compiler.values}, array);
            b.addArg(resr);
        }
        b.finalize((void(list_array<ValueItem>::*)(size_t)) & list_array<ValueItem>::remove);
    }
    void Compiler::StaticCompiler::ArrayOperation::remove_range(const ValueIndexPos& from, const ValueIndexPos& to) {
        if (array_meta.allow_edit == false)
            throw InvalidOperation("Cannot edit constant value");
        if (array_meta.vtype != VType::uarr) {
            BuildCall b(compiler.a, 1);
            b.lea_valindex({compiler.static_map, compiler.values}, array);
            b.finalize(AsArr);
            array_meta.vtype = VType::uarr;
        }
        BuildCall b(compiler.a, 3);
        if (from.pos == ValuePos::in_constants && to.pos == ValuePos::in_constants) {
            b.mov_valindex({compiler.static_map, compiler.values}, array);
            b.addArg(compiler.get_size_constant(from));
            b.addArg(compiler.get_size_constant(to));
        } else if (from.pos == ValuePos::in_constants) {
            b.setArguments(1);
            b.lea_valindex({compiler.static_map, compiler.values}, from);
            b.finalize(getSize);

            b.setArguments(3);
            b.mov_valindex({compiler.static_map, compiler.values}, array);
            b.addArg(compiler.get_size_constant(from));
            b.addArg(resr);
        } else if (to.pos == ValuePos::in_constants) {
            b.setArguments(1);
            b.lea_valindex({compiler.static_map, compiler.values}, to);
            b.finalize(getSize);

            b.setArguments(3);
            b.mov_valindex({compiler.static_map, compiler.values}, array);
            b.addArg(resr);
            b.addArg(compiler.get_size_constant(to));
        } else {
            b.setArguments(1);
            b.lea_valindex({compiler.static_map, compiler.values}, from);
            b.finalize(getSize);
            compiler.a.push(resr);
            compiler.a.push(0);
            b.lea_valindex({compiler.static_map, compiler.values}, to);
            b.finalize(getSize);

            b.setArguments(3);
            b.mov_valindex({compiler.static_map, compiler.values}, array);
            b.addArg(resr);
            compiler.a.pop();
            compiler.a.pop(argr2);
            b.addArg(argr2);
        }
        b.finalize((void(list_array<ValueItem>::*)(size_t, size_t)) & list_array<ValueItem>::remove);
    }
    void Compiler::StaticCompiler::ArrayOperation::resize(const ValueIndexPos& new_size) {
        if (array_meta.allow_edit == false)
            throw InvalidOperation("Cannot edit constant value");
        if (array_meta.vtype != VType::uarr) {
            BuildCall b(compiler.a, 1);
            b.lea_valindex({compiler.static_map, compiler.values}, array);
            b.finalize(AsArr);
            array_meta.vtype = VType::uarr;
        }
        BuildCall b(compiler.a, 2);
        if (new_size.pos == ValuePos::in_constants) {
            b.mov_valindex({compiler.static_map, compiler.values}, array);
            b.addArg(compiler.get_size_constant(new_size));
        } else {
            b.setArguments(1);
            b.lea_valindex({compiler.static_map, compiler.values}, new_size);
            b.finalize(getSize);

            b.setArguments(2);
            b.mov_valindex({compiler.static_map, compiler.values}, array);
            b.addArg(resr);
        }
        b.finalize((void(list_array<ValueItem>::*)(size_t)) & list_array<ValueItem>::resize);
    }
    void Compiler::StaticCompiler::ArrayOperation::resize_default(const ValueIndexPos& new_size, const ValueIndexPos& default_value) {
        if (array_meta.allow_edit == false)
            throw InvalidOperation("Cannot edit constant value");
        if (array_meta.vtype != VType::uarr) {
            BuildCall b(compiler.a, 1);
            b.lea_valindex({compiler.static_map, compiler.values}, array);
            b.finalize(AsArr);
            array_meta.vtype = VType::uarr;
        }
        BuildCall b(compiler.a, 3);
        if (new_size.pos == ValuePos::in_constants) {
            b.mov_valindex({compiler.static_map, compiler.values}, array);
            b.addArg(compiler.get_size_constant(new_size));
            b.lea_valindex({compiler.static_map, compiler.values}, default_value);
        } else {
            b.setArguments(1);
            b.lea_valindex({compiler.static_map, compiler.values}, new_size);
            b.finalize(getSize);

            b.setArguments(3);
            b.mov_valindex({compiler.static_map, compiler.values}, array);
            b.addArg(resr);
            b.lea_valindex({compiler.static_map, compiler.values}, default_value);
        }
        b.finalize((void(list_array<ValueItem>::*)(size_t, const ValueItem&)) & list_array<ValueItem>::resize);
    }
    void Compiler::StaticCompiler::ArrayOperation::reserve_push_end(const ValueIndexPos& count) {
        if (array_meta.allow_edit == false)
            throw InvalidOperation("Cannot edit constant value");
        if (array_meta.vtype != VType::uarr) {
            BuildCall b(compiler.a, 1);
            b.lea_valindex({compiler.static_map, compiler.values}, array);
            b.finalize(AsArr);
            array_meta.vtype = VType::uarr;
        }
        BuildCall b(compiler.a, 2);
        if (count.pos == ValuePos::in_constants) {
            b.mov_valindex({compiler.static_map, compiler.values}, array);
            b.addArg(compiler.get_size_constant(count));
        } else {
            b.setArguments(1);
            b.lea_valindex({compiler.static_map, compiler.values}, count);
            b.finalize(getSize);

            b.setArguments(2);
            b.mov_valindex({compiler.static_map, compiler.values}, array);
            b.addArg(resr);
        }
        b.finalize((void(list_array<ValueItem>::*)(size_t)) & list_array<ValueItem>::reserve_push_back);
    }
    void Compiler::StaticCompiler::ArrayOperation::reserve_push_start(const ValueIndexPos& count) {
        if (array_meta.allow_edit == false)
            throw InvalidOperation("Cannot edit constant value");
        if (array_meta.vtype != VType::uarr) {
            BuildCall b(compiler.a, 1);
            b.lea_valindex({compiler.static_map, compiler.values}, array);
            b.finalize(AsArr);
            array_meta.vtype = VType::uarr;
        }
        BuildCall b(compiler.a, 2);
        if (count.pos == ValuePos::in_constants) {
            b.mov_valindex({compiler.static_map, compiler.values}, array);
            b.addArg(compiler.get_size_constant(count));
        } else {
            b.setArguments(1);
            b.lea_valindex({compiler.static_map, compiler.values}, count);
            b.finalize(getSize);

            b.setArguments(2);
            b.mov_valindex({compiler.static_map, compiler.values}, array);
            b.addArg(resr);
        }
        b.finalize((void(list_array<ValueItem>::*)(size_t)) & list_array<ValueItem>::reserve_push_front);
    }

    void Compiler::StaticCompiler::ArrayOperation::commit() {
        if (array_meta.vtype == VType::uarr) {
            if (array_meta.allow_edit == false)
                throw InvalidOperation("Cannot edit constant value");
            BuildCall b(compiler.a, 1);
            b.lea_valindex({compiler.static_map, compiler.values}, array);
            b.finalize(&list_array<ValueItem>::commit);
        }
    }
    void Compiler::StaticCompiler::ArrayOperation::decommit(const ValueIndexPos& total_blocks) {
        if (array_meta.allow_edit == false)
            throw InvalidOperation("Cannot edit constant value");
        if (array_meta.vtype != VType::uarr) {
            BuildCall b(compiler.a, 1);
            b.lea_valindex({compiler.static_map, compiler.values}, array);
            b.finalize(AsArr);
            array_meta.vtype = VType::uarr;
        }
        BuildCall b(compiler.a, 2);
        if (total_blocks.pos == ValuePos::in_constants) {
            b.mov_valindex({compiler.static_map, compiler.values}, array);
            b.addArg(compiler.get_size_constant(total_blocks));
        } else {
            b.setArguments(1);
            b.lea_valindex({compiler.static_map, compiler.values}, total_blocks);
            b.finalize(getSize);

            b.setArguments(2);
            b.mov_valindex({compiler.static_map, compiler.values}, array);
            b.addArg(resr);
        }
        b.finalize((void(list_array<ValueItem>::*)(size_t)) & list_array<ValueItem>::decommit);
    }
    void Compiler::StaticCompiler::ArrayOperation::remove_reserved() {
        if (array_meta.vtype == VType::uarr) {
            if (array_meta.allow_edit == false)
                throw InvalidOperation("Cannot edit constant value");
            BuildCall b(compiler.a, 1);
            b.lea_valindex({compiler.static_map, compiler.values}, array);
            b.finalize(&list_array<ValueItem>::shrink_to_fit);
        }
    }
    void Compiler::StaticCompiler::ArrayOperation::size(const ValueIndexPos& set) {
        if (is_raw_array(array_meta.vtype)) {
            BuildCall b(compiler.a, 2);
            compiler.a.mov_valindex_meta_size({compiler.static_map, compiler.values}, argr1_32, array);
            b.lea_valindex({compiler.static_map, compiler.values}, set);
            b.addArg(argr1_32);
            b.finalize(helper_functions::setSize);
        } else if (array_meta.vtype == VType::uarr) {
            BuildCall b(compiler.a, 1);
            b.lea_valindex({compiler.static_map, compiler.values}, array);
            b.finalize(&list_array<ValueItem>::size);

            b.setArguments(2);
            b.lea_valindex({compiler.static_map, compiler.values}, set);
            b.addArg(resr);
            b.finalize(helper_functions::setSize);
        } else {
            if (array_meta.allow_edit == false)
                throw InvalidOperation("Cannot get size of non-array value");
            BuildCall b(compiler.a, 1);
            b.lea_valindex({compiler.static_map, compiler.values}, array);
            b.finalize(AsArr);
            b.setArguments(2);
            b.lea_valindex({compiler.static_map, compiler.values}, set);
            b.addArg(1);
            b.finalize(helper_functions::setSize);
        }
    }

    Compiler::StaticCompiler::ArrayOperation Compiler::StaticCompiler::arr_op(const ValueIndexPos& array, OpArrFlags flags, ValueMeta array_meta) {
        return ArrayOperation(compiler, array, flags, array_meta);
    }

    void Compiler::StaticCompiler::_throw(const ValueIndexPos& name, ValueMeta name_meta, const ValueIndexPos& message, ValueMeta message_meta) {
        //TODO: optimize
        compiler.dynamic()._throw(name, message);
    }

    void Compiler::StaticCompiler::as(const ValueIndexPos& in, ValueMeta in_meta, VType as_type) {
        //TODO: optimize
        compiler.dynamic().as(in, as_type);
    }
    void Compiler::StaticCompiler::is(const ValueIndexPos& in, VType is_type) {
        //TODO: optimize
        compiler.dynamic().is(in, is_type);
    }
    void Compiler::StaticCompiler::store_bool(const ValueIndexPos& from, ValueMeta from_meta) {
        //TODO: optimize
        compiler.dynamic().store_bool(from);
    }
    void Compiler::StaticCompiler::load_bool(const ValueIndexPos& to, ValueMeta to_meta) {
        //TODO: optimize
        compiler.dynamic().load_bool(to);
    }

    void Compiler::StaticCompiler::call_value_function(CallFlags flags, const ValueIndexPos& fn_symbol, ValueMeta fn_symbol_meta, const ValueIndexPos& structure_, ClassAccess access) {
        //TODO: optimize
        compiler.dynamic().call_value_function(flags, fn_symbol, structure_, access);
    }
    void Compiler::StaticCompiler::call_value_function(CallFlags flags, const ValueIndexPos& fn_symbol, ValueMeta fn_symbol_meta, const ValueIndexPos& structure_, ClassAccess access, const ValueIndexPos& res, ValueMeta res_meta) {
        //TODO: optimize
        compiler.dynamic().call_value_function(flags, fn_symbol, structure_, access, res);
    }

    void Compiler::StaticCompiler::call_value_function_id(CallFlags flags, uint64_t id, const ValueIndexPos& structure_) {
        //TODO: optimize
        compiler.dynamic().call_value_function_id(flags, id, structure_);
    }
    void Compiler::StaticCompiler::call_value_function_id(CallFlags flags, uint64_t id, const ValueIndexPos& structure_, const ValueIndexPos& res, ValueMeta res_meta) {
        //TODO: optimize
        compiler.dynamic().call_value_function_id(flags, id, structure_, res);
    }


    void Compiler::StaticCompiler::call_value_function_and_ret(CallFlags flags, const ValueIndexPos& fn_symbol, ValueMeta fn_symbol_meta, const ValueIndexPos& structure_, ClassAccess access) {
        //TODO: optimize
        compiler.dynamic().call_value_function_and_ret(flags, fn_symbol, structure_, access);
    }
    void Compiler::StaticCompiler::call_value_function_id_and_ret(CallFlags flags, uint64_t id, const ValueIndexPos& structure_) {
        //TODO: optimize
        compiler.dynamic().call_value_function_id_and_ret(flags, id, structure_);
    }

    void Compiler::StaticCompiler::static_call_value_function(CallFlags flags, const ValueIndexPos& fn_symbol, ValueMeta fn_symbol_meta, const ValueIndexPos& structure_, ClassAccess access) {
        //TODO: optimize
        compiler.dynamic().static_call_value_function(flags, fn_symbol, structure_, access);
    }
    void Compiler::StaticCompiler::static_call_value_function(CallFlags flags, const ValueIndexPos& fn_symbol, ValueMeta fn_symbol_meta, const ValueIndexPos& structure_, ClassAccess access, const ValueIndexPos& res, ValueMeta res_meta) {
        //TODO: optimize
        compiler.dynamic().static_call_value_function(flags, fn_symbol, structure_, access, res);
    }

    void Compiler::StaticCompiler::static_call_value_function_id(CallFlags flags, uint64_t id, const ValueIndexPos& structure_) {
        //TODO: optimize
        compiler.dynamic().static_call_value_function_id(flags, id, structure_);
    }
    void Compiler::StaticCompiler::static_call_value_function_id(CallFlags flags, uint64_t id, const ValueIndexPos& structure_, const ValueIndexPos& res, ValueMeta res_meta) {
        //TODO: optimize
        compiler.dynamic().static_call_value_function_id(flags, id, structure_, res);
    }


    void Compiler::StaticCompiler::static_call_value_function_and_ret(CallFlags flags, const ValueIndexPos& fn_symbol, ValueMeta fn_symbol_meta, const ValueIndexPos& structure_, ClassAccess access) {
        //TODO: optimize
        compiler.dynamic().static_call_value_function_and_ret(flags, fn_symbol, structure_, access);
    }
    void Compiler::StaticCompiler::static_call_value_function_id_and_ret(CallFlags flags, uint64_t id, const ValueIndexPos& structure_) {
        //TODO: optimize
        compiler.dynamic().static_call_value_function_id_and_ret(flags, id, structure_);
    }


    void Compiler::StaticCompiler::set_structure_value(const ValueIndexPos& value_name, ClassAccess access, const ValueIndexPos& structure_, const ValueIndexPos& value, ValueMeta value_meta) {
        //TODO: optimize
        compiler.dynamic().set_structure_value(value_name, access, structure_, value);
    }
    void Compiler::StaticCompiler::get_structure_value(const ValueIndexPos& value_name, ClassAccess access, const ValueIndexPos& structure_, const ValueIndexPos& to, ValueMeta to_meta) {
        //TODO: optimize
        compiler.dynamic().get_structure_value(value_name, access, structure_, to);
    }

    void Compiler::StaticCompiler::explicit_await(const ValueIndexPos& value, ValueMeta value_meta) {
        //TODO: optimize
        compiler.dynamic().explicit_await(value);
    }
    void Compiler::StaticCompiler::generator_get(const ValueIndexPos& generator, const ValueIndexPos& to, ValueMeta to_meta, const ValueIndexPos& result_index) {
        //TODO: optimize
        compiler.dynamic().generator_get(generator, to, result_index);
    }


    void Compiler::StaticCompiler::is_gc(const ValueIndexPos& value, ValueMeta value_meta, const ValueIndexPos& set_bool, ValueMeta set_bool_meta) {
        //TODO: optimize
        compiler.dynamic().is_gc(value, set_bool);
    }

    void Compiler::StaticCompiler::table_jump(TableJumpFlags flags, uint64_t too_large_label, uint64_t too_small_label, const std::vector<uint64_t>& labels, const ValueIndexPos& check_value) {
        //TODO: optimize
        compiler.dynamic().table_jump(flags, too_large_label, too_small_label, labels, check_value);
    }
    void Compiler::StaticCompiler::xarray_slice(const ValueIndexPos& result, ValueMeta result_meta, const ValueIndexPos& array, ValueMeta array_meta, const ValueIndexPos& from, ValueMeta from_meta, const ValueIndexPos& to, ValueMeta to_meta) {
        //TODO: optimize
        compiler.dynamic().xarray_slice(result, array, from, to);
    }
    void Compiler::StaticCompiler::xarray_slice(const ValueIndexPos& result, ValueMeta result_meta, const ValueIndexPos& array, ValueMeta array_meta, const ValueIndexPos& from, ValueMeta from_meta) {
        //TODO: optimize
        compiler.dynamic().xarray_slice(result, array, from);
    }
    void Compiler::StaticCompiler::xarray_slice(const ValueIndexPos& result, ValueMeta result_meta, const ValueIndexPos& array, ValueMeta array_meta) {
        //TODO: optimize
        compiler.dynamic().xarray_slice(result, array);
    }
    void Compiler::StaticCompiler::xarray_slice(const ValueIndexPos& result, ValueMeta result_meta, const ValueIndexPos& array, ValueMeta array_meta, bool, const ValueIndexPos& to, ValueMeta to_meta) {
        //TODO: optimize
        compiler.dynamic().xarray_slice(result, array, false, to);
    }

    void Compiler::StaticCompiler::copy_unconst(const ValueIndexPos& set, ValueMeta set_meta, const ValueIndexPos& from, ValueMeta from_meta) {
        //TODO: optimize
        compiler.dynamic().copy_unconst(set, from);
    }
    void Compiler::StaticCompiler::copy_unreference(const ValueIndexPos& set, ValueMeta set_meta, const ValueIndexPos& from, ValueMeta from_meta) {
        //TODO: optimize
        compiler.dynamic().copy_unreference(set, from);
    }
    void Compiler::StaticCompiler::move_unreference(const ValueIndexPos& set, ValueMeta set_meta, const ValueIndexPos& from, ValueMeta from_meta) {
        //TODO: optimize
        compiler.dynamic().move_unreference(set, from);
    }
}