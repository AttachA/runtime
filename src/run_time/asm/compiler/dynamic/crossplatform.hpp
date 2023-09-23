// Copyright Danyil Melnytskyi 2022-Present
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <run_time/asm/compiler/compiler_include.hpp>

namespace art {
    void _inlineReleaseUnused(CASM& a, creg64 reg);

    //only once included in run_time/asm/compiler.cpp
    void Compiler::DynamicCompiler::noting() {
        compiler.a.noting();
    }

    void Compiler::DynamicCompiler::remove(const ValueIndexPos& value_index) {
        CASM& a = compiler.a;
        BuildCall b(a, 1);
        b.lea_valindex({compiler.static_map, compiler.values}, value_index);
        b.finalize(universalRemove);
    }

    void Compiler::DynamicCompiler::copy(const ValueIndexPos& to, const ValueIndexPos& from) {
        CASM& a = compiler.a;
        BuildCall b(a, 2);
        b.lea_valindex({compiler.static_map, compiler.values}, to);
        b.lea_valindex({compiler.static_map, compiler.values}, from);
        b.finalize((ValueItem & (ValueItem::*)(const ValueItem&))(&ValueItem::operator=));
    }

    void Compiler::DynamicCompiler::move(const ValueIndexPos& to, const ValueIndexPos& from) {
        CASM& a = compiler.a;
        BuildCall b(a, 2);
        b.lea_valindex({compiler.static_map, compiler.values}, to);
        b.lea_valindex({compiler.static_map, compiler.values}, from);
        b.finalize((ValueItem & (ValueItem::*)(ValueItem&&))(&ValueItem::operator=));
    }

    void Compiler::DynamicCompiler::sum(const ValueIndexPos& to, const ValueIndexPos& src) {
        CASM& a = compiler.a;
        BuildCall b(a, 2);
        b.lea_valindex({compiler.static_map, compiler.values}, to);
        b.lea_valindex({compiler.static_map, compiler.values}, src);
        b.finalize((ValueItem & (ValueItem::*)(const ValueItem&))(&ValueItem::operator+=));
    }

    void Compiler::DynamicCompiler::sub(const ValueIndexPos& to, const ValueIndexPos& src) {
        CASM& a = compiler.a;
        BuildCall b(a, 2);
        b.lea_valindex({compiler.static_map, compiler.values}, to);
        b.lea_valindex({compiler.static_map, compiler.values}, src);
        b.finalize((ValueItem & (ValueItem::*)(const ValueItem&))(&ValueItem::operator-=));
    }

    void Compiler::DynamicCompiler::mul(const ValueIndexPos& to, const ValueIndexPos& src) {
        CASM& a = compiler.a;
        BuildCall b(a, 2);
        b.lea_valindex({compiler.static_map, compiler.values}, to);
        b.lea_valindex({compiler.static_map, compiler.values}, src);
        b.finalize((ValueItem & (ValueItem::*)(const ValueItem&))(&ValueItem::operator*=));
    }

    void Compiler::DynamicCompiler::div(const ValueIndexPos& to, const ValueIndexPos& src) {
        CASM& a = compiler.a;
        BuildCall b(a, 2);
        b.lea_valindex({compiler.static_map, compiler.values}, to);
        b.lea_valindex({compiler.static_map, compiler.values}, src);
        b.finalize((ValueItem & (ValueItem::*)(const ValueItem&))(&ValueItem::operator/=));
    }

    void Compiler::DynamicCompiler::rest(const ValueIndexPos& to, const ValueIndexPos& src) {
        CASM& a = compiler.a;
        BuildCall b(a, 2);
        b.lea_valindex({compiler.static_map, compiler.values}, to);
        b.lea_valindex({compiler.static_map, compiler.values}, src);
        b.finalize((ValueItem & (ValueItem::*)(const ValueItem&))(&ValueItem::operator%=));
    }

    void Compiler::DynamicCompiler::bit_xor(const ValueIndexPos& to, const ValueIndexPos& src) {
        CASM& a = compiler.a;
        BuildCall b(a, 2);
        b.lea_valindex({compiler.static_map, compiler.values}, to);
        b.lea_valindex({compiler.static_map, compiler.values}, src);
        b.finalize((ValueItem & (ValueItem::*)(const ValueItem&))(&ValueItem::operator^=));
    }

    void Compiler::DynamicCompiler::bit_and(const ValueIndexPos& to, const ValueIndexPos& src) {
        CASM& a = compiler.a;
        BuildCall b(a, 2);
        b.lea_valindex({compiler.static_map, compiler.values}, to);
        b.lea_valindex({compiler.static_map, compiler.values}, src);
        b.finalize((ValueItem & (ValueItem::*)(const ValueItem&))(&ValueItem::operator&=));
    }

    void Compiler::DynamicCompiler::bit_or(const ValueIndexPos& to, const ValueIndexPos& src) {
        CASM& a = compiler.a;
        BuildCall b(a, 2);
        b.lea_valindex({compiler.static_map, compiler.values}, to);
        b.lea_valindex({compiler.static_map, compiler.values}, src);
        b.finalize((ValueItem & (ValueItem::*)(const ValueItem&))(&ValueItem::operator|=));
    }

    void Compiler::DynamicCompiler::bit_not(const ValueIndexPos& to) {
        CASM& a = compiler.a;
        BuildCall b(a, 1);
        b.lea_valindex({compiler.static_map, compiler.values}, to);
        b.finalize((ValueItem & (ValueItem::*)())(&ValueItem::operator!));
    }

    void Compiler::DynamicCompiler::bit_left_shift(const ValueIndexPos& to, const ValueIndexPos& src) {
        CASM& a = compiler.a;
        BuildCall b(a, 2);
        b.lea_valindex({compiler.static_map, compiler.values}, to);
        b.lea_valindex({compiler.static_map, compiler.values}, src);
        b.finalize((ValueItem & (ValueItem::*)(const ValueItem&))(&ValueItem::operator<<=));
    }

    void Compiler::DynamicCompiler::bit_right_shift(const ValueIndexPos& to, const ValueIndexPos& src) {
        CASM& a = compiler.a;
        BuildCall b(a, 2);
        b.lea_valindex({compiler.static_map, compiler.values}, to);
        b.lea_valindex({compiler.static_map, compiler.values}, src);
        b.finalize((ValueItem & (ValueItem::*)(const ValueItem&))(&ValueItem::operator>>=));
    }

    void Compiler::DynamicCompiler::arg_set(const ValueIndexPos& src) {
        BuildCall b(compiler.a, 1);
        b.lea_valindex({compiler.static_map, compiler.values}, src);
        b.finalize(AsArg);
        compiler.a.mov(arg_ptr, resr);
        compiler.a.mov_valindex_meta_size({compiler.static_map, compiler.values}, arg_len_32, src);
    }

    void Compiler::DynamicCompiler::call_self_and_ret() {
        BuildCall b(compiler.a, 2);
        b.addArg(arg_ptr);
        b.addArg(arg_len_32);
        b.finalize(compiler.self_function);
        compiler.a.jmp(compiler.prolog);
    }

    void Compiler::DynamicCompiler::ret(const ValueIndexPos& value) {
        BuildCall b(compiler.a, 1);
        b.lea_valindex({compiler.static_map, compiler.values}, value);
        b.finalize(buildRes);
        compiler.a.jmp(compiler.prolog);
    }

    void Compiler::DynamicCompiler::ret_take(const ValueIndexPos& value) {
        BuildCall b(compiler.a, 1);
        b.lea_valindex({compiler.static_map, compiler.values}, value);
        b.finalize(buildResTake);
        compiler.a.jmp(compiler.prolog);
    }

    void Compiler::DynamicCompiler::ret_noting() {
        compiler.a.xor_(resr, resr);
        compiler.a.jmp(compiler.prolog);
    }

    void Compiler::DynamicCompiler::call(const ValueIndexPos& fn_symbol, CallFlags flags) {
        CASM& a = compiler.a;
        BuildCall b(a, 0);
        if (fn_symbol.pos == ValuePos::in_constants) {
            if (flags.always_dynamic) {
                b.setArguments(5);
                b.addArg(compiler.get_string_constant(fn_symbol));
                b.addArg(arg_ptr);
                b.addArg(arg_len_32);
                b.addArg(flags.async_mode);
                b.finalize(&FuncEnvironment::callFunc);
            } else {
                call_fun_string(
                    a,
                    compiler.get_string_constant(fn_symbol),
                    flags.async_mode,
                    compiler.used_environs);
            }
        } else {
            b.setArguments(2);
            b.lea_valindex({compiler.static_map, compiler.values}, fn_symbol);
            b.addArg(VType::string);
            b.finalize(getSpecificValue);
            b.setArguments(5);
            b.addArg(resr);
            b.addArg(arg_ptr);
            b.addArg(arg_len_32);
            b.addArg(flags.async_mode);
            b.finalize(&FuncEnvironment::callFunc);
        }
        _inlineReleaseUnused(a, resr);
    }

    void Compiler::DynamicCompiler::call(const ValueIndexPos& fn_symbol, CallFlags flags, const ValueIndexPos& res) {
        CASM& a = compiler.a;
        BuildCall b(a, 0);
        if (fn_symbol.pos == ValuePos::in_constants) {
            if (flags.always_dynamic) {
                b.setArguments(5);
                b.addArg(compiler.get_string_constant(fn_symbol));
                b.addArg(arg_ptr);
                b.addArg(arg_len_32);
                b.addArg(flags.async_mode);
                b.finalize(&FuncEnvironment::callFunc);
            } else {
                call_fun_string(
                    a,
                    compiler.get_string_constant(fn_symbol),
                    flags.async_mode,
                    compiler.used_environs);
            }
        } else {
            b.setArguments(2);
            b.lea_valindex({compiler.static_map, compiler.values}, fn_symbol);
            b.addArg(VType::string);
            b.finalize(getSpecificValue);
            b.setArguments(5);
            b.addArg(resr);
            b.addArg(arg_ptr);
            b.addArg(arg_len_32);
            b.addArg(flags.async_mode);
            b.finalize(&FuncEnvironment::callFunc);
        }
        if (flags.use_result) {
            b.setArguments(2);
            b.lea_valindex({compiler.static_map, compiler.values}, res);
            b.addArg(resr);
            b.finalize(getValueItem);
            return;
        } else if (!flags.use_result)
            _inlineReleaseUnused(a, resr);
    }

    void Compiler::DynamicCompiler::call_self() {
        BuildCall b(compiler.a, 2);
        b.addArg(arg_ptr);
        b.addArg(arg_len_32);
        b.finalize(compiler.self_function);
        _inlineReleaseUnused(compiler.a, resr);
    }

    void Compiler::DynamicCompiler::call_self(const ValueIndexPos& res) {
        BuildCall b(compiler.a, 2);
        b.addArg(arg_ptr);
        b.addArg(arg_len_32);
        b.finalize(compiler.self_function);
        b.lea_valindex({compiler.static_map, compiler.values}, res);
        b.addArg(resr);
        b.finalize(getValueItem);
    }

    void Compiler::DynamicCompiler::call_local(const ValueIndexPos& fn_symbol, CallFlags flags) {
        CASM& a = compiler.a;
        if (fn_symbol.pos == ValuePos::in_constants) {
            uint64_t index = (uint64_t)compiler.values[fn_symbol.index + compiler.static_map.size()];
            auto& fn = compiler.build_func->localFn(index);
            if (flags.async_mode) {
                BuildCall b(a, 3);
                b.addArg(&fn);
                b.addArg(arg_ptr);
                b.addArg(arg_len_32);
                b.finalize(&FuncEnvironment::asyncWrapper);
            } else {
                BuildCall b(a, 2);
                b.addArg(arg_ptr);
                b.addArg(arg_len_32);
                b.finalize(fn->get_func_ptr());
            }


        } else {
            BuildCall b(a, 1);
            b.lea_valindex({compiler.static_map, compiler.values}, fn_symbol);
            b.finalize(getSize);
            b.setArguments(5);
            b.addArg(compiler.build_func);
            b.addArg(resr);
            b.addArg(arg_ptr);
            b.addArg(arg_len_32);
            b.addArg(flags.async_mode);
            b.finalize(&FuncHandle::inner_handle::localWrapper);
        }
        _inlineReleaseUnused(a, resr);
    }

    void Compiler::DynamicCompiler::call_local(const ValueIndexPos& fn_symbol, CallFlags flags, const ValueIndexPos& res) {
        CASM& a = compiler.a;
        if (fn_symbol.pos == ValuePos::in_constants) {
            uint64_t index = (uint64_t)compiler.values[fn_symbol.index + compiler.static_map.size()];
            auto& fn = compiler.build_func->localFn(index);
            if (flags.async_mode) {
                BuildCall b(a, 3);
                b.addArg(&fn);
                b.addArg(arg_ptr);
                b.addArg(arg_len_32);
                b.finalize(&FuncEnvironment::asyncWrapper);
            } else {
                BuildCall b(a, 2);
                b.addArg(arg_ptr);
                b.addArg(arg_len_32);
                b.finalize(fn->get_func_ptr());
            }


        } else {
            BuildCall b(a, 1);
            b.lea_valindex({compiler.static_map, compiler.values}, fn_symbol);
            b.finalize(getSize);
            b.setArguments(5);
            b.addArg(compiler.build_func);
            b.addArg(resr);
            b.addArg(arg_ptr);
            b.addArg(arg_len_32);
            b.addArg(flags.async_mode);
            b.finalize(&FuncHandle::inner_handle::localWrapper);
        }
        if (flags.use_result) {
            BuildCall b(a, 2);
            b.lea_valindex({compiler.static_map, compiler.values}, res);
            b.addArg(resr);
            b.finalize(getValueItem);
            return;
        } else
            _inlineReleaseUnused(a, resr);
    }

    void Compiler::DynamicCompiler::call_and_ret(const ValueIndexPos& fn_symbol, CallFlags flags) {
        BuildCall b(compiler.a, 0);
        if (fn_symbol.pos == ValuePos::in_constants) {
            if (flags.always_dynamic) {
                b.setArguments(5);
                b.addArg(compiler.get_string_constant(fn_symbol));
                b.addArg(arg_ptr);
                b.addArg(arg_len_32);
                b.addArg(flags.async_mode);
                b.finalize(&FuncEnvironment::callFunc);
            } else {
                call_fun_string(
                    compiler.a,
                    compiler.get_string_constant(fn_symbol),
                    flags.async_mode,
                    compiler.used_environs);
            }
        } else {
            b.setArguments(2);
            b.lea_valindex({compiler.static_map, compiler.values}, fn_symbol);
            b.addArg(VType::string);
            b.finalize(getSpecificValue);
            b.setArguments(5);
            b.addArg(resr);
            b.addArg(arg_ptr);
            b.addArg(arg_len_32);
            b.addArg(flags.async_mode);
            b.finalize(&FuncEnvironment::callFunc);
        }
        compiler.a.jmp(compiler.prolog);
    }

    void Compiler::DynamicCompiler::call_local_and_ret(const ValueIndexPos& fn_symbol, CallFlags flags) {
        CASM& a = compiler.a;
        if (fn_symbol.pos == ValuePos::in_constants) {
            uint64_t index = (uint64_t)compiler.values[fn_symbol.index + compiler.static_map.size()];
            auto& fn = compiler.build_func->localFn(index);
            if (flags.async_mode) {
                BuildCall b(a, 3);
                b.addArg(&fn);
                b.addArg(arg_ptr);
                b.addArg(arg_len_32);
                b.finalize(&FuncEnvironment::asyncWrapper);
            } else {
                BuildCall b(a, 2);
                b.addArg(arg_ptr);
                b.addArg(arg_len_32);
                b.finalize(fn->get_func_ptr());
            }
        } else {
            BuildCall b(a, 1);
            b.lea_valindex({compiler.static_map, compiler.values}, fn_symbol);
            b.finalize(getSize);
            b.setArguments(5);
            b.addArg(compiler.build_func);
            b.addArg(resr);
            b.addArg(arg_ptr);
            b.addArg(arg_len_32);
            b.addArg(flags.async_mode);
            b.finalize(&FuncHandle::inner_handle::localWrapper);
        }
        a.jmp(compiler.prolog);
    }
    //TODO: add support for gc arrays
    void Compiler::DynamicCompiler::ArrayOperation::set(const ValueIndexPos& index, const ValueIndexPos& value) {
        BuildCall b(compiler.a, 1);
        b.lea_valindex({compiler.static_map, compiler.values}, array);
        b.finalize(AsArr);
        if (index.pos == ValuePos::in_constants) {

            b.setArguments(3);
            b.lea_valindex({compiler.static_map, compiler.values}, value);
            b.mov_valindex({compiler.static_map, compiler.values}, array);
            b.addArg(compiler.get_size_constant(index));
        } else {
            b.lea_valindex({compiler.static_map, compiler.values}, index);
            b.finalize(getSize);

            b.setArguments(3);
            b.lea_valindex({compiler.static_map, compiler.values}, value);
            b.mov_valindex({compiler.static_map, compiler.values}, array);
            b.addArg(resr);
        }
        if (flags.move_mode) {
            switch (flags.checked) {
            case ArrCheckMode::no_check:
                b.finalize(helper_functions::IndexArraySetMoveDynamic<0>);
                break;
            case ArrCheckMode::check:
                b.finalize(helper_functions::IndexArraySetMoveDynamic<1>);
                break;
            case ArrCheckMode::no_throw_check:
                b.finalize(helper_functions::IndexArraySetMoveDynamic<2>);
                break;
            default:
                break;
            }
        } else {
            switch (flags.checked) {
            case ArrCheckMode::no_check:
                b.finalize(helper_functions::IndexArraySetCopyDynamic<0>);
                break;
            case ArrCheckMode::check:
                b.finalize(helper_functions::IndexArraySetCopyDynamic<1>);
                break;
            case ArrCheckMode::no_throw_check:
                b.finalize(helper_functions::IndexArraySetCopyDynamic<2>);
                break;
            default:
                break;
            }
        }
    }


    void Compiler::DynamicCompiler::ArrayOperation::insert(const ValueIndexPos& index, const ValueIndexPos& value) {
        BuildCall b(compiler.a, 1);
        b.lea_valindex({compiler.static_map, compiler.values}, array);
        b.finalize(AsArr);
        if (index.pos == ValuePos::in_constants) {
            b.setArguments(3);
            b.mov_valindex({compiler.static_map, compiler.values}, array);
            b.addArg(compiler.get_size_constant(index));
        } else {
            b.lea_valindex({compiler.static_map, compiler.values}, index);
            b.finalize(getSize);

            b.setArguments(3);
            b.mov_valindex({compiler.static_map, compiler.values}, array);
            b.addArg(resr);
        }
        b.lea_valindex({compiler.static_map, compiler.values}, value);
        if (flags.move_mode)
            b.finalize((void(list_array<ValueItem>::*)(size_t, ValueItem&&)) & list_array<ValueItem>::insert);
        else
            b.finalize((void(list_array<ValueItem>::*)(size_t, const ValueItem&)) & list_array<ValueItem>::insert);
    }

    void Compiler::DynamicCompiler::ArrayOperation::push_end(const ValueIndexPos& value) {
        BuildCall b(compiler.a, 1);
        b.lea_valindex({compiler.static_map, compiler.values}, array);
        b.finalize(AsArr);

        b.setArguments(2);
        b.mov_valindex({compiler.static_map, compiler.values}, array);
        b.lea_valindex({compiler.static_map, compiler.values}, value);
        if (flags.move_mode)
            b.finalize((void(list_array<ValueItem>::*)(ValueItem&&)) & list_array<ValueItem>::push_back);
        else
            b.finalize((void(list_array<ValueItem>::*)(const ValueItem&)) & list_array<ValueItem>::push_back);
    }

    void Compiler::DynamicCompiler::ArrayOperation::push_start(const ValueIndexPos& value) {
        BuildCall b(compiler.a, 1);
        b.lea_valindex({compiler.static_map, compiler.values}, array);
        b.finalize(AsArr);

        b.setArguments(2);
        b.mov_valindex({compiler.static_map, compiler.values}, array);
        b.lea_valindex({compiler.static_map, compiler.values}, value);
        if (flags.move_mode)
            b.finalize((void(list_array<ValueItem>::*)(ValueItem&&)) & list_array<ValueItem>::push_front);
        else
            b.finalize((void(list_array<ValueItem>::*)(const ValueItem&)) & list_array<ValueItem>::push_front);
    }

    void Compiler::DynamicCompiler::ArrayOperation::insert_range(const ValueIndexPos& array2, const ValueIndexPos& index, const ValueIndexPos& from2, const ValueIndexPos& to2) {
        BuildCall b(compiler.a, 1);
        b.lea_valindex({compiler.static_map, compiler.values}, array);
        b.finalize(AsArr);
        b.lea_valindex({compiler.static_map, compiler.values}, array2);
        b.finalize(AsArr);
        if (to2.pos == ValuePos::in_constants && from2.pos == ValuePos::in_constants && index.pos == ValuePos::in_constants) {
            b.setArguments(5);
            b.mov_valindex({compiler.static_map, compiler.values}, array);
            b.addArg(compiler.get_size_constant(index));
            b.mov_valindex({compiler.static_map, compiler.values}, array2);
            b.addArg(compiler.get_size_constant(from2));
            b.addArg(compiler.get_size_constant(to2));
        } else if (to2.pos == ValuePos::in_constants && from2.pos == ValuePos::in_constants) {
            b.lea_valindex({compiler.static_map, compiler.values}, index);
            b.finalize(getSize);

            b.setArguments(5);
            b.mov_valindex({compiler.static_map, compiler.values}, array);
            b.addArg(resr);
            b.mov_valindex({compiler.static_map, compiler.values}, array2);
            b.addArg(compiler.get_size_constant(from2));
            b.addArg(compiler.get_size_constant(to2));
        } else if (to2.pos == ValuePos::in_constants && index.pos == ValuePos::in_constants) {
            b.lea_valindex({compiler.static_map, compiler.values}, from2);
            b.finalize(getSize);

            b.setArguments(5);
            b.mov_valindex({compiler.static_map, compiler.values}, array);
            b.addArg(compiler.get_size_constant(index));
            b.mov_valindex({compiler.static_map, compiler.values}, array2);
            b.addArg(resr);
            b.addArg(compiler.get_size_constant(to2));
        } else if (from2.pos == ValuePos::in_constants && index.pos == ValuePos::in_constants) {
            b.lea_valindex({compiler.static_map, compiler.values}, to2);
            b.finalize(getSize);

            b.setArguments(5);
            b.mov_valindex({compiler.static_map, compiler.values}, array);
            b.addArg(compiler.get_size_constant(index));
            b.mov_valindex({compiler.static_map, compiler.values}, array2);
            b.addArg(compiler.get_size_constant(from2));
            b.addArg(resr);
        } else if (to2.pos == ValuePos::in_constants) {
            b.lea_valindex({compiler.static_map, compiler.values}, from2);
            b.finalize(getSize);
            compiler.a.push(resr);
            compiler.a.push(0);
            b.lea_valindex({compiler.static_map, compiler.values}, index);
            b.finalize(getSize);

            b.setArguments(5);
            b.mov_valindex({compiler.static_map, compiler.values}, array);
            b.addArg(resr);
            b.mov_valindex({compiler.static_map, compiler.values}, array2);
            compiler.a.pop();
            compiler.a.pop(resr);
            b.addArg(resr);
            b.addArg(compiler.get_size_constant(to2));
        } else if (from2.pos == ValuePos::in_constants) {
            b.lea_valindex({compiler.static_map, compiler.values}, to2);
            b.finalize(getSize);
            compiler.a.push(resr);
            compiler.a.push(0);
            b.lea_valindex({compiler.static_map, compiler.values}, index);
            b.finalize(getSize);

            b.setArguments(5);
            b.mov_valindex({compiler.static_map, compiler.values}, array);
            b.addArg(resr); // index
            b.mov_valindex({compiler.static_map, compiler.values}, array2);
            b.addArg(compiler.get_size_constant(from2)); // from
            compiler.a.pop();
            compiler.a.pop(resr);
            b.addArg(resr); // to
        } else if (index.pos == ValuePos::in_constants) {
            b.lea_valindex({compiler.static_map, compiler.values}, to2);
            b.finalize(getSize);
            compiler.a.push(resr);
            compiler.a.push(0);
            b.lea_valindex({compiler.static_map, compiler.values}, from2);
            b.finalize(getSize);

            b.setArguments(5);
            b.mov_valindex({compiler.static_map, compiler.values}, array);
            b.addArg(compiler.get_size_constant(index)); //index
            b.mov_valindex({compiler.static_map, compiler.values}, array2);
            b.addArg(resr); // from
            compiler.a.pop();
            compiler.a.pop(resr);
            b.addArg(resr); // to
        } else {
            b.lea_valindex({compiler.static_map, compiler.values}, to2);
            b.finalize(getSize);
            compiler.a.push(resr);
            compiler.a.push(0);
            b.lea_valindex({compiler.static_map, compiler.values}, from2);
            b.finalize(getSize);
            compiler.a.pop();
            compiler.a.push(resr);
            b.lea_valindex({compiler.static_map, compiler.values}, index);
            b.finalize(getSize);

            b.setArguments(5);
            b.mov_valindex({compiler.static_map, compiler.values}, array);
            b.addArg(resr); // index
            b.mov_valindex({compiler.static_map, compiler.values}, array2);
            compiler.a.pop(resr);
            b.addArg(resr); // from
            compiler.a.pop(resr);
            b.addArg(resr); // to
        }
        b.finalize((void(list_array<ValueItem>::*)(size_t, const list_array<ValueItem>&, size_t, size_t)) & list_array<ValueItem>::insert);
    }

    void Compiler::DynamicCompiler::ArrayOperation::get(const ValueIndexPos& index, const ValueIndexPos& set) {
        BuildCall b(compiler.a, 1);
        b.lea_valindex({compiler.static_map, compiler.values}, array);
        b.finalize(AsArr);

        if (index.pos == ValuePos::in_constants) {
            b.setArguments(3);
            b.lea_valindex({compiler.static_map, compiler.values}, set);
            b.lea_valindex({compiler.static_map, compiler.values}, array);
            b.addArg(compiler.get_size_constant(index));
        } else {
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
                b.finalize(helper_functions::IndexArrayMoveDynamic<0>);
                break;
            case ArrCheckMode::check:
                b.finalize(helper_functions::IndexArrayMoveDynamic<1>);
                break;
            case ArrCheckMode::no_throw_check:
                b.finalize(helper_functions::IndexArrayMoveDynamic<2>);
                break;
            default:
                break;
            }
        } else {
            switch (flags.checked) {
            case ArrCheckMode::no_check:
                b.finalize(helper_functions::IndexArrayCopyDynamic<0>);
                break;
            case ArrCheckMode::check:
                b.finalize(helper_functions::IndexArrayCopyDynamic<1>);
                break;
            case ArrCheckMode::no_throw_check:
                b.finalize(helper_functions::IndexArrayCopyDynamic<2>);
                break;
            default:
                break;
            }
        }
    }

    void Compiler::DynamicCompiler::ArrayOperation::take(const ValueIndexPos& index, const ValueIndexPos& set) {
        BuildCall b(compiler.a, 1);
        b.lea_valindex({compiler.static_map, compiler.values}, array);
        b.finalize(AsArr);
        if (index.pos == ValuePos::in_constants) {
            b.setArguments(3);
            b.addArg(compiler.get_size_constant(index));
            b.mov_valindex({compiler.static_map, compiler.values}, array);
            b.lea_valindex({compiler.static_map, compiler.values}, set);
        } else {
            b.lea_valindex({compiler.static_map, compiler.values}, index);
            b.finalize(getSize);
            b.setArguments(3);
            b.addArg(resr);
            b.mov_valindex({compiler.static_map, compiler.values}, array);
            b.lea_valindex({compiler.static_map, compiler.values}, set);
        }
        b.finalize(helper_functions::take);
    }

    void Compiler::DynamicCompiler::ArrayOperation::take_end(const ValueIndexPos& set) {
        BuildCall b(compiler.a, 1);
        b.lea_valindex({compiler.static_map, compiler.values}, array);
        b.finalize(AsArr);

        b.setArguments(2);
        b.mov_valindex({compiler.static_map, compiler.values}, array);
        b.lea_valindex({compiler.static_map, compiler.values}, set);
        b.finalize(helper_functions::takeEnd);
    }

    void Compiler::DynamicCompiler::ArrayOperation::take_start(const ValueIndexPos& set) {
        BuildCall b(compiler.a, 1);
        b.lea_valindex({compiler.static_map, compiler.values}, array);
        b.finalize(AsArr);

        b.setArguments(2);
        b.mov_valindex({compiler.static_map, compiler.values}, array);
        b.lea_valindex({compiler.static_map, compiler.values}, set);
        b.finalize(helper_functions::takeStart);
    }

    void Compiler::DynamicCompiler::ArrayOperation::get_range(const ValueIndexPos& set, const ValueIndexPos& from, const ValueIndexPos& to) {
        BuildCall b(compiler.a, 1);
        b.lea_valindex({compiler.static_map, compiler.values}, array);
        b.finalize(AsArr);

        if (from.pos == ValuePos::in_constants && to.pos == ValuePos::in_constants) {
            b.setArguments(4);
            b.mov_valindex({compiler.static_map, compiler.values}, array);
            b.lea_valindex({compiler.static_map, compiler.values}, set);
            b.addArg(compiler.get_size_constant(from));
            b.addArg(compiler.get_size_constant(to));
        } else if (from.pos == ValuePos::in_constants) {
            b.lea_valindex({compiler.static_map, compiler.values}, to);
            b.finalize(getSize);

            b.setArguments(4);
            b.mov_valindex({compiler.static_map, compiler.values}, array);
            b.lea_valindex({compiler.static_map, compiler.values}, set);
            b.addArg(compiler.get_size_constant(from));
            b.addArg(resr);
        } else if (to.pos == ValuePos::in_constants) {
            b.lea_valindex({compiler.static_map, compiler.values}, from);
            b.finalize(getSize);

            b.setArguments(4);
            b.mov_valindex({compiler.static_map, compiler.values}, array);
            b.lea_valindex({compiler.static_map, compiler.values}, set);
            b.addArg(resr);
            b.addArg(compiler.get_size_constant(to));
        } else {
            b.lea_valindex({compiler.static_map, compiler.values}, from);
            b.finalize(getSize);
            compiler.a.push(resr);
            compiler.a.push(0);
            b.lea_valindex({compiler.static_map, compiler.values}, to);
            b.finalize(getSize);

            b.setArguments(4);
            b.mov_valindex({compiler.static_map, compiler.values}, array);
            b.lea_valindex({compiler.static_map, compiler.values}, set);
            b.addArg(resr);
            compiler.a.pop();
            compiler.a.pop(resr);
            b.addArg(resr);
        }
        b.finalize(helper_functions::getRange);
    }

    void Compiler::DynamicCompiler::ArrayOperation::take_range(const ValueIndexPos& set, const ValueIndexPos& from, const ValueIndexPos& to) {
        BuildCall b(compiler.a, 1);
        b.lea_valindex({compiler.static_map, compiler.values}, array);
        b.finalize(AsArr);

        if (from.pos == ValuePos::in_constants && to.pos == ValuePos::in_constants) {
            b.setArguments(4);
            b.mov_valindex({compiler.static_map, compiler.values}, array);
            b.lea_valindex({compiler.static_map, compiler.values}, set);
            b.addArg(compiler.get_size_constant(from));
            b.addArg(compiler.get_size_constant(to));
        } else if (from.pos == ValuePos::in_constants) {
            b.lea_valindex({compiler.static_map, compiler.values}, to);
            b.finalize(getSize);

            b.setArguments(4);
            b.mov_valindex({compiler.static_map, compiler.values}, array);
            b.lea_valindex({compiler.static_map, compiler.values}, set);
            b.addArg(compiler.get_size_constant(from));
            b.addArg(resr);
        } else if (to.pos == ValuePos::in_constants) {
            b.lea_valindex({compiler.static_map, compiler.values}, from);
            b.finalize(getSize);

            b.setArguments(4);
            b.mov_valindex({compiler.static_map, compiler.values}, array);
            b.lea_valindex({compiler.static_map, compiler.values}, set);
            b.addArg(resr);
            b.addArg(compiler.get_size_constant(to));
        } else {
            b.lea_valindex({compiler.static_map, compiler.values}, from);
            b.finalize(getSize);
            compiler.a.push(resr);
            compiler.a.push(0);
            b.lea_valindex({compiler.static_map, compiler.values}, to);
            b.finalize(getSize);

            b.setArguments(4);
            b.mov_valindex({compiler.static_map, compiler.values}, array);
            b.lea_valindex({compiler.static_map, compiler.values}, set);
            b.addArg(resr);
            compiler.a.pop();
            compiler.a.pop(resr);
            b.addArg(resr);
        }
        b.finalize(helper_functions::takeRange);
    }

    void Compiler::DynamicCompiler::ArrayOperation::pop_end() {
        BuildCall b(compiler.a, 1);
        b.lea_valindex({compiler.static_map, compiler.values}, array);
        b.finalize(AsArr);
        b.mov_valindex({compiler.static_map, compiler.values}, array);
        b.finalize(&list_array<ValueItem>::pop_back);
    }

    void Compiler::DynamicCompiler::ArrayOperation::pop_start() {
        BuildCall b(compiler.a, 1);
        b.lea_valindex({compiler.static_map, compiler.values}, array);
        b.finalize(AsArr);
        b.mov_valindex({compiler.static_map, compiler.values}, array);
        b.finalize(&list_array<ValueItem>::pop_front);
    }

    void Compiler::DynamicCompiler::ArrayOperation::remove_item(const ValueIndexPos& index) {
        BuildCall b(compiler.a, 1);
        b.lea_valindex({compiler.static_map, compiler.values}, array);
        b.finalize(AsArr);

        if (index.pos == ValuePos::in_constants) {
            b.setArguments(2);
            b.mov_valindex({compiler.static_map, compiler.values}, array);
            b.addArg(compiler.get_size_constant(index));
        } else {
            b.lea_valindex({compiler.static_map, compiler.values}, index);
            b.finalize(getSize);
            b.setArguments(2);
            b.mov_valindex({compiler.static_map, compiler.values}, array);
            b.addArg(resr);
        }
        b.finalize((void(list_array<ValueItem>::*)(size_t)) & list_array<ValueItem>::remove);
    }

    void Compiler::DynamicCompiler::ArrayOperation::remove_range(const ValueIndexPos& from, const ValueIndexPos& to) {
        BuildCall b(compiler.a, 1);
        b.lea_valindex({compiler.static_map, compiler.values}, array);
        b.finalize(AsArr);

        if (from.pos == ValuePos::in_constants || to.pos == ValuePos::in_constants) {
            b.mov_valindex({compiler.static_map, compiler.values}, array);
            b.addArg(compiler.get_size_constant(from));
            b.addArg(compiler.get_size_constant(to));
        } else if (from.pos == ValuePos::in_constants) {
            b.lea_valindex({compiler.static_map, compiler.values}, from);
            b.finalize(getSize);

            b.setArguments(2);
            b.mov_valindex({compiler.static_map, compiler.values}, array);
            b.addArg(resr);
            b.addArg(compiler.get_size_constant(to));
        } else if (to.pos == ValuePos::in_constants) {
            b.lea_valindex({compiler.static_map, compiler.values}, to);
            b.finalize(getSize);

            b.setArguments(2);
            b.mov_valindex({compiler.static_map, compiler.values}, array);
            b.addArg(compiler.get_size_constant(from));
            b.addArg(resr);
        } else {
            b.lea_valindex({compiler.static_map, compiler.values}, to);
            b.finalize(getSize);
            compiler.a.push(resr);
            compiler.a.push(0);
            b.lea_valindex({compiler.static_map, compiler.values}, from);
            b.finalize(getSize);

            b.setArguments(2);
            b.mov_valindex({compiler.static_map, compiler.values}, array);
            b.addArg(resr);
            compiler.a.pop();
            compiler.a.pop(resr);
            b.addArg(resr);
        }
        b.finalize((void(list_array<ValueItem>::*)(size_t, size_t)) & list_array<ValueItem>::remove);
    }

    void Compiler::DynamicCompiler::ArrayOperation::resize(const ValueIndexPos& new_size) {
        BuildCall b(compiler.a, 1);
        b.lea_valindex({compiler.static_map, compiler.values}, array);
        b.finalize(AsArr);

        if (new_size.pos == ValuePos::in_constants) {
            b.setArguments(2);
            b.mov_valindex({compiler.static_map, compiler.values}, array);
            b.addArg(compiler.get_size_constant(new_size));
        } else {
            b.lea_valindex({compiler.static_map, compiler.values}, new_size);
            b.finalize(getSize);

            b.setArguments(2);
            b.mov_valindex({compiler.static_map, compiler.values}, array);
            b.addArg(resr);
        }
        b.finalize((void(list_array<ValueItem>::*)(size_t)) & list_array<ValueItem>::resize);
    }

    void Compiler::DynamicCompiler::ArrayOperation::resize_default(const ValueIndexPos& new_size, const ValueIndexPos& default_value) {
        BuildCall b(compiler.a, 1);
        b.lea_valindex({compiler.static_map, compiler.values}, array);
        b.finalize(AsArr);

        if (new_size.pos == ValuePos::in_constants) {
            b.setArguments(3);
            b.mov_valindex({compiler.static_map, compiler.values}, array);
            b.addArg(compiler.get_size_constant(new_size));
            b.lea_valindex({compiler.static_map, compiler.values}, default_value);
        } else {
            b.lea_valindex({compiler.static_map, compiler.values}, new_size);
            b.finalize(getSize);

            b.setArguments(3);
            b.mov_valindex({compiler.static_map, compiler.values}, array);
            b.addArg(resr);
            b.lea_valindex({compiler.static_map, compiler.values}, default_value);
        }
        b.finalize((void(list_array<ValueItem>::*)(size_t, const ValueItem&)) & list_array<ValueItem>::resize);
    }

    void Compiler::DynamicCompiler::ArrayOperation::reserve_push_end(const ValueIndexPos& count) {
        BuildCall b(compiler.a, 1);
        b.lea_valindex({compiler.static_map, compiler.values}, array);
        b.finalize(AsArr);

        if (count.pos == ValuePos::in_constants) {
            b.setArguments(2);
            b.mov_valindex({compiler.static_map, compiler.values}, array);
            b.addArg(compiler.get_size_constant(count));
        } else {
            b.lea_valindex({compiler.static_map, compiler.values}, count);
            b.finalize(getSize);

            b.setArguments(2);
            b.mov_valindex({compiler.static_map, compiler.values}, array);
            b.addArg(resr);
        }
        b.finalize(&list_array<ValueItem>::reserve_push_back);
    }

    void Compiler::DynamicCompiler::ArrayOperation::reserve_push_start(const ValueIndexPos& count) {
        BuildCall b(compiler.a, 1);
        b.lea_valindex({compiler.static_map, compiler.values}, array);
        b.finalize(AsArr);

        if (count.pos == ValuePos::in_constants) {
            b.setArguments(2);
            b.mov_valindex({compiler.static_map, compiler.values}, array);
            b.addArg(compiler.get_size_constant(count));
        } else {
            b.lea_valindex({compiler.static_map, compiler.values}, count);
            b.finalize(getSize);

            b.setArguments(2);
            b.mov_valindex({compiler.static_map, compiler.values}, array);
            b.addArg(resr);
        }
        b.finalize(&list_array<ValueItem>::reserve_push_front);
    }

    void Compiler::DynamicCompiler::ArrayOperation::commit() {
        BuildCall b(compiler.a, 1);
        b.lea_valindex({compiler.static_map, compiler.values}, array);
        b.finalize(AsArr);
        b.mov_valindex({compiler.static_map, compiler.values}, array);
        b.finalize(&list_array<ValueItem>::commit);
    }

    void Compiler::DynamicCompiler::ArrayOperation::decommit(const ValueIndexPos& total_blocks) {
        BuildCall b(compiler.a, 1);
        b.lea_valindex({compiler.static_map, compiler.values}, array);
        b.finalize(AsArr);

        if (total_blocks.pos == ValuePos::in_constants) {
            b.setArguments(2);
            b.mov_valindex({compiler.static_map, compiler.values}, array);
            b.addArg(compiler.get_size_constant(total_blocks));
        } else {
            b.lea_valindex({compiler.static_map, compiler.values}, total_blocks);
            b.finalize(getSize);

            b.setArguments(2);
            b.mov_valindex({compiler.static_map, compiler.values}, array);
            b.addArg(resr);
        }
        b.finalize(&list_array<ValueItem>::decommit);
    }

    void Compiler::DynamicCompiler::ArrayOperation::remove_reserved() {
        BuildCall b(compiler.a, 1);
        b.lea_valindex({compiler.static_map, compiler.values}, array);
        b.finalize(AsArr);
        b.mov_valindex({compiler.static_map, compiler.values}, array);
        b.finalize(&list_array<ValueItem>::shrink_to_fit);
    }

    void Compiler::DynamicCompiler::ArrayOperation::size(const ValueIndexPos& set) {
        BuildCall b(compiler.a, 1);
        b.lea_valindex({compiler.static_map, compiler.values}, array);
        b.finalize(AsArr);
        b.mov_valindex({compiler.static_map, compiler.values}, array);
        b.finalize(&list_array<ValueItem>::size);
        b.setArguments(2);
        b.lea_valindex({compiler.static_map, compiler.values}, set);
        b.addArg(resr);
        b.finalize(helper_functions::setSize);
    }

    void Compiler::DynamicCompiler::_throw(const ValueIndexPos& name, const ValueIndexPos& message) {
        BuildCall b(compiler.a, 2);
        b.lea_valindex({compiler.static_map, compiler.values}, name);
        b.lea_valindex({compiler.static_map, compiler.values}, message);
        b.finalize(helper_functions::throwEx);
    }

    void Compiler::DynamicCompiler::as(const ValueIndexPos& in, VType as_type) {
        BuildCall b(compiler.a, 2);
        b.lea_valindex({compiler.static_map, compiler.values}, in);
        b.addArg(as_type);
        b.finalize(asValue);
    }

    void Compiler::DynamicCompiler::insert_native(uint32_t len, uint8_t* native_code) {
        compiler.a.insertNative(native_code, len);
    }

    template <bool async_call>
    ValueItem* _valueItemDynamicCall(const art::ustring& name, ValueItem* class_ptr, ClassAccess access, ValueItem* args, uint32_t len) {
        switch (class_ptr->meta.vtype) {
        case VType::struct_:
            if constexpr (async_call)
                return FuncEnvironment::async_call(((Structure&)*class_ptr).get_method_dynamic(name, access), args, len);
            else
                return ((Structure&)*class_ptr).table_get_dynamic(name, access)(args, len);
        default:
            throw NotImplementedException();
        }
    }
    template <bool async_call>
    ValueItem* _valueItemDynamicCall(uint64_t id, ValueItem* class_ptr, ValueItem* args, uint32_t len) {
        switch (class_ptr->meta.vtype) {
        case VType::struct_:
            if constexpr (async_call)
                return FuncEnvironment::async_call(((Structure&)*class_ptr).get_method(id), args, len);
            else
                return ((Structure&)*class_ptr).table_get(id)(args, len);
        default:
            throw NotImplementedException();
        }
    }
    template <bool async_mode>
    ValueItem* valueItemDynamicCall(const art::ustring& name, ValueItem* class_ptr, ValueItem* args, uint32_t len, ClassAccess access) {
        if (!class_ptr)
            throw NullPointerException();
        class_ptr->getAsync();
        list_array<ValueItem> args_tmp;
        args_tmp.reserve_push_back(len + 1);
        args_tmp.push_back(ValueItem(*class_ptr, as_reference));
        for (uint32_t i = 0; i < len; i++)
            args_tmp.push_back(ValueItem(args[i], as_reference));
        return _valueItemDynamicCall<async_mode>(name, class_ptr, access, args_tmp.data(), len + 1);
    }

    template <bool async_mode>
    ValueItem* valueItemDynamicCallId(uint64_t id, ValueItem* class_ptr, ValueItem* args, uint32_t len) {
        if (!class_ptr)
            throw NullPointerException();
        class_ptr->getAsync();
        list_array<ValueItem> args_tmp;
        args_tmp.reserve_push_back(len + 1);
        args_tmp.push_back(ValueItem(*class_ptr, as_reference));
        for (uint32_t i = 0; i < len; i++)
            args_tmp.push_back(ValueItem(args[i], as_reference));

        return _valueItemDynamicCall<async_mode>(id, class_ptr, args_tmp.data(), len + 1);
    }

    template <bool async_mode>
    void* staticValueItemDynamicCall(const art::ustring& name, ValueItem* class_ptr, ValueItem* args, uint32_t len, ClassAccess access) {
        if (!class_ptr)
            throw NullPointerException();
        class_ptr->getAsync();
        return _valueItemDynamicCall<async_mode>(name, class_ptr, access, args, len);
    }

    template <bool async_mode>
    void* staticValueItemDynamicCallId(uint64_t id, ValueItem* class_ptr, ValueItem* args, uint32_t len) {
        if (!class_ptr)
            throw NullPointerException();
        class_ptr->getAsync();
        return _valueItemDynamicCall<async_mode>(id, class_ptr, args, len);
    }

    void Compiler::DynamicCompiler::call_value_function(CallFlags flags, const ValueIndexPos& fn_symbol, const ValueIndexPos& structure_, ClassAccess access) {
        BuildCall b(compiler.a, 5);
        if (fn_symbol.pos == ValuePos::in_constants) {
            b.addArg(compiler.get_string_constant(fn_symbol));
        } else {
            b.setArguments(2);
            b.lea_valindex({compiler.static_map, compiler.values}, fn_symbol);
            b.addArg(VType::string);
            b.finalize(getSpecificValue);
            b.setArguments(5);
            b.addArg(resr);
        }

        b.lea_valindex({compiler.static_map, compiler.values}, structure_);
        b.addArg(arg_ptr);
        b.addArg(arg_len_32);
        b.addArg((uint8_t)access);
        if (flags.async_mode)
            b.finalize(valueItemDynamicCall<true>);
        else
            b.finalize(valueItemDynamicCall<false>);
        _inlineReleaseUnused(compiler.a, resr);
    }

    void Compiler::DynamicCompiler::call_value_function(CallFlags flags, const ValueIndexPos& fn_symbol, const ValueIndexPos& structure_, ClassAccess access, const ValueIndexPos& res) {
        BuildCall b(compiler.a, 5);
        if (fn_symbol.pos == ValuePos::in_constants) {
            b.addArg(compiler.get_string_constant(fn_symbol));
        } else {
            b.setArguments(2);
            b.lea_valindex({compiler.static_map, compiler.values}, fn_symbol);
            b.addArg(VType::string);
            b.finalize(getSpecificValue);
            b.setArguments(5);
            b.addArg(resr);
        }

        b.lea_valindex({compiler.static_map, compiler.values}, structure_);
        b.addArg(arg_ptr);
        b.addArg(arg_len_32);
        b.addArg((uint8_t)access);
        if (flags.async_mode)
            b.finalize(valueItemDynamicCall<true>);
        else
            b.finalize(valueItemDynamicCall<false>);

        b.setArguments(2);
        b.lea_valindex({compiler.static_map, compiler.values}, res);
        b.addArg(resr);
        b.finalize(getValueItem);
    }

    void Compiler::DynamicCompiler::call_value_function_id(CallFlags flags, uint64_t id, const ValueIndexPos& structure_) {
        BuildCall b(compiler.a, 4);
        b.addArg(id);
        b.lea_valindex({compiler.static_map, compiler.values}, structure_);
        b.addArg(arg_ptr);
        b.addArg(arg_len_32);
        if (flags.async_mode)
            b.finalize(valueItemDynamicCallId<true>);
        else
            b.finalize(valueItemDynamicCallId<false>);
        _inlineReleaseUnused(compiler.a, resr);
    }

    void Compiler::DynamicCompiler::call_value_function_id(CallFlags flags, uint64_t id, const ValueIndexPos& structure_, const ValueIndexPos& res) {
        BuildCall b(compiler.a, 4);
        b.addArg(id);
        b.lea_valindex({compiler.static_map, compiler.values}, structure_);
        b.addArg(arg_ptr);
        b.addArg(arg_len_32);
        if (flags.async_mode)
            b.finalize(valueItemDynamicCallId<true>);
        else
            b.finalize(valueItemDynamicCallId<false>);

        b.setArguments(2);
        b.lea_valindex({compiler.static_map, compiler.values}, res);
        b.addArg(resr);
        b.finalize(getValueItem);
    }

    void Compiler::DynamicCompiler::call_value_function_and_ret(CallFlags flags, const ValueIndexPos& fn_symbol, const ValueIndexPos& structure_, ClassAccess access) {
        BuildCall b(compiler.a, 5);
        if (fn_symbol.pos == ValuePos::in_constants) {
            b.addArg(compiler.get_string_constant(fn_symbol));
        } else {
            b.setArguments(2);
            b.lea_valindex({compiler.static_map, compiler.values}, fn_symbol);
            b.addArg(VType::string);
            b.finalize(getSpecificValue);
            b.setArguments(5);
            b.addArg(resr);
        }
        b.lea_valindex({compiler.static_map, compiler.values}, structure_);
        b.addArg(arg_ptr);
        b.addArg(arg_len_32);
        b.addArg((uint8_t)access);
        if (flags.async_mode)
            b.finalize(valueItemDynamicCall<true>);
        else
            b.finalize(valueItemDynamicCall<false>);
        compiler.a.jmp(compiler.prolog);
    }

    void Compiler::DynamicCompiler::call_value_function_id_and_ret(CallFlags flags, uint64_t id, const ValueIndexPos& structure_) {
        BuildCall b(compiler.a, 4);
        b.addArg(id);
        b.lea_valindex({compiler.static_map, compiler.values}, structure_);
        b.addArg(arg_ptr);
        b.addArg(arg_len_32);
        if (flags.async_mode)
            b.finalize(valueItemDynamicCallId<true>);
        else
            b.finalize(valueItemDynamicCallId<false>);
        compiler.a.jmp(compiler.prolog);
    }

    void Compiler::DynamicCompiler::static_call_value_function(CallFlags flags, const ValueIndexPos& fn_symbol, const ValueIndexPos& structure_, ClassAccess access) {
        BuildCall b(compiler.a, 5);
        if (fn_symbol.pos == ValuePos::in_constants) {
            b.addArg(compiler.get_string_constant(fn_symbol));
        } else {
            b.setArguments(2);
            b.lea_valindex({compiler.static_map, compiler.values}, fn_symbol);
            b.addArg(VType::string);
            b.finalize(getSpecificValue);
            b.setArguments(5);
            b.addArg(resr);
        }

        b.lea_valindex({compiler.static_map, compiler.values}, structure_);
        b.addArg(arg_ptr);
        b.addArg(arg_len_32);
        b.addArg((uint8_t)access);
        if (flags.async_mode)
            b.finalize(staticValueItemDynamicCall<true>);
        else
            b.finalize(staticValueItemDynamicCall<false>);
        _inlineReleaseUnused(compiler.a, resr);
    }

    void Compiler::DynamicCompiler::static_call_value_function(CallFlags flags, const ValueIndexPos& fn_symbol, const ValueIndexPos& structure_, ClassAccess access, const ValueIndexPos& res) {
        BuildCall b(compiler.a, 5);
        if (fn_symbol.pos == ValuePos::in_constants) {
            b.addArg(compiler.get_string_constant(fn_symbol));
        } else {
            b.setArguments(2);
            b.lea_valindex({compiler.static_map, compiler.values}, fn_symbol);
            b.addArg(VType::string);
            b.finalize(getSpecificValue);
            b.setArguments(5);
            b.addArg(resr);
        }

        b.lea_valindex({compiler.static_map, compiler.values}, structure_);
        b.addArg(arg_ptr);
        b.addArg(arg_len_32);
        b.addArg((uint8_t)access);
        if (flags.async_mode)
            b.finalize(staticValueItemDynamicCall<true>);
        else
            b.finalize(staticValueItemDynamicCall<false>);

        b.setArguments(2);
        b.lea_valindex({compiler.static_map, compiler.values}, res);
        b.addArg(resr);
        b.finalize(getValueItem);
    }

    void Compiler::DynamicCompiler::static_call_value_function_id(CallFlags flags, uint64_t id, const ValueIndexPos& structure_) {
        BuildCall b(compiler.a, 4);
        b.addArg(id);
        b.lea_valindex({compiler.static_map, compiler.values}, structure_);
        b.addArg(arg_ptr);
        b.addArg(arg_len_32);
        if (flags.async_mode)
            b.finalize(staticValueItemDynamicCallId<true>);
        else
            b.finalize(staticValueItemDynamicCallId<false>);
        _inlineReleaseUnused(compiler.a, resr);
    }

    void Compiler::DynamicCompiler::static_call_value_function_id(CallFlags flags, uint64_t id, const ValueIndexPos& structure_, const ValueIndexPos& res) {
        BuildCall b(compiler.a, 4);
        b.addArg(id);
        b.lea_valindex({compiler.static_map, compiler.values}, structure_);
        b.addArg(arg_ptr);
        b.addArg(arg_len_32);
        if (flags.async_mode)
            b.finalize(staticValueItemDynamicCallId<true>);
        else
            b.finalize(staticValueItemDynamicCallId<false>);

        b.setArguments(2);
        b.lea_valindex({compiler.static_map, compiler.values}, res);
        b.addArg(resr);
        b.finalize(getValueItem);
    }

    void Compiler::DynamicCompiler::static_call_value_function_and_ret(CallFlags flags, const ValueIndexPos& fn_symbol, const ValueIndexPos& structure_, ClassAccess access) {
        BuildCall b(compiler.a, 5);
        if (fn_symbol.pos == ValuePos::in_constants) {
            b.addArg(compiler.get_string_constant(fn_symbol));
        } else {
            b.setArguments(2);
            b.lea_valindex({compiler.static_map, compiler.values}, fn_symbol);
            b.addArg(VType::string);
            b.finalize(getSpecificValue);
            b.setArguments(5);
            b.addArg(resr);
        }
        b.lea_valindex({compiler.static_map, compiler.values}, structure_);
        b.addArg(arg_ptr);
        b.addArg(arg_len_32);
        b.addArg((uint8_t)access);
        if (flags.async_mode)
            b.finalize(staticValueItemDynamicCall<true>);
        else
            b.finalize(staticValueItemDynamicCall<false>);
        compiler.a.jmp(compiler.prolog);
    }

    void Compiler::DynamicCompiler::static_call_value_function_id_and_ret(CallFlags flags, uint64_t id, const ValueIndexPos& structure_) {
        BuildCall b(compiler.a, 4);
        b.addArg(id);
        b.lea_valindex({compiler.static_map, compiler.values}, structure_);
        b.addArg(arg_ptr);
        b.addArg(arg_len_32);
        if (flags.async_mode)
            b.finalize(staticValueItemDynamicCallId<true>);
        else
            b.finalize(staticValueItemDynamicCallId<false>);
        compiler.a.jmp(compiler.prolog);
    }

    void Compiler::DynamicCompiler::set_structure_value(const ValueIndexPos& value_name, ClassAccess access, const ValueIndexPos& structure_, const ValueIndexPos& value) {
        BuildCall b(compiler.a, 0);
        if (value_name.pos == ValuePos::in_constants) {
            b.addArg(access);
            b.lea_valindex({compiler.static_map, compiler.values}, structure_);
            b.addArg(compiler.get_string_constant(value_name));
        } else {
            b.lea_valindex({compiler.static_map, compiler.values}, value_name);
            b.addArg(VType::string);
            b.finalize(getSpecificValue);
            b.addArg(access);
            b.lea_valindex({compiler.static_map, compiler.values}, structure_);
            b.addArg(resr);
        }
        b.lea_valindex({compiler.static_map, compiler.values}, value);
        b.finalize((void (*)(ClassAccess, ValueItem&, const art::ustring&, const ValueItem&))art::CXX::Interface::setValue);
    }

    void Compiler::DynamicCompiler::get_structure_value(const ValueIndexPos& value_name, ClassAccess access, const ValueIndexPos& structure_, const ValueIndexPos& to) {
        BuildCall b(compiler.a, 0);
        if (value_name.pos == ValuePos::in_constants) {
            b.addArg(access);
            b.lea_valindex({compiler.static_map, compiler.values}, structure_);
            b.addArg(compiler.get_string_constant(value_name));
        } else {
            b.lea_valindex({compiler.static_map, compiler.values}, value_name);
            b.addArg(VType::string);
            b.finalize(getSpecificValue);
            b.addArg(access);
            b.lea_valindex({compiler.static_map, compiler.values}, structure_);
            b.addArg(resr);
        }
        b.lea_valindex({compiler.static_map, compiler.values}, to);
        b.finalize(helper_functions::getInterfaceValue);
    }

    void Compiler::DynamicCompiler::explicit_await(const ValueIndexPos& value) {
        BuildCall b(compiler.a, 1);
        b.lea_valindex({compiler.static_map, compiler.values}, value);
        b.finalize(&ValueItem::getAsync);
    }

    void Compiler::DynamicCompiler::generator_get(const ValueIndexPos& generator, const ValueIndexPos& to, const ValueIndexPos& result_index) {
        BuildCall b(compiler.a, 3);
        if (generator.pos == ValuePos::in_constants) {
            b.lea_valindex({compiler.static_map, compiler.values}, generator);
            b.lea_valindex({compiler.static_map, compiler.values}, to);
            b.addArg(compiler.get_size_constant(result_index));
        } else {
            b.setArguments(1);
            b.lea_valindex({compiler.static_map, compiler.values}, result_index);
            b.finalize(getSize);
            b.setArguments(3);
            b.lea_valindex({compiler.static_map, compiler.values}, generator);
            b.lea_valindex({compiler.static_map, compiler.values}, to);
            b.addArg(resr);
        }

        b.finalize(&ValueItem::getGeneratorResult);
    }

    void Compiler::DynamicCompiler::_yield(const ValueIndexPos& value) {
        BuildCall b(compiler.a, 2);
        b.lea_valindex({compiler.static_map, compiler.values}, value);
        b.finalize(Task::result);
    }

    void Compiler::DynamicCompiler::handle_begin(uint64_t exception_scope) {
        compiler.scope_map.mapHandle(exception_scope);
    }

    void Compiler::DynamicCompiler::handle_catch_0(uint64_t exception_scope, const std::vector<art::ustring>& catch_names) {
        size_t handle = compiler.scope_map.try_mapHandle(exception_scope);
        std::vector<uint8_t> handler_data;
        handler_data.reserve(40);
        handler_data.push_back(0);
        builder::write(handler_data, (uint64_t)catch_names.size());
        for (art::ustring name : catch_names) {
            builder::write(handler_data, name.size());
            handler_data.insert(handler_data.end(), name.begin(), name.end());
        }
        compiler.scope.setExceptionHandle(handle, exception::_attacha_filter, handler_data.data(), handler_data.size());
    }

    void Compiler::DynamicCompiler::handle_catch_1(uint64_t exception_scope, uint16_t exception_name_env_id) {
        size_t handle = compiler.scope_map.try_mapHandle(exception_scope);
        std::vector<uint8_t> handler_data;
        handler_data.reserve(40);
        handler_data.push_back(1);
        builder::write(handler_data, exception_name_env_id);
        compiler.scope.setExceptionHandle(handle, exception::_attacha_filter, handler_data.data(), handler_data.size());
    }

    void Compiler::DynamicCompiler::handle_catch_2(uint64_t exception_scope, const std::vector<uint16_t>& exception_name_env_ids) {
        size_t handle = compiler.scope_map.try_mapHandle(exception_scope);
        std::vector<uint8_t> handler_data;
        handler_data.reserve(40);
        handler_data.push_back(2);
        builder::write(handler_data, (uint64_t)exception_name_env_ids.size());
        for (uint16_t id : exception_name_env_ids)
            builder::write(handler_data, id);
        compiler.scope.setExceptionHandle(handle, exception::_attacha_filter, handler_data.data(), handler_data.size());
    }

    void Compiler::DynamicCompiler::handle_catch_3(uint64_t exception_scope, const std::vector<art::ustring>& catch_names, const std::vector<uint16_t>& exception_name_env_ids) {
        size_t handle = compiler.scope_map.try_mapHandle(exception_scope);
        std::vector<uint8_t> handler_data;
        handler_data.reserve(40);
        handler_data.push_back(3);
        builder::write(handler_data, (uint64_t)(catch_names.size() + exception_name_env_ids.size()));
        for (art::ustring name : catch_names) {
            handler_data.push_back(0);
            builder::write(handler_data, name.size());
            handler_data.insert(handler_data.end(), name.begin(), name.end());
        }
        for (uint16_t id : exception_name_env_ids) {
            handler_data.push_back(1);
            builder::write(handler_data, id);
        }
        compiler.scope.setExceptionHandle(handle, exception::_attacha_filter, handler_data.data(), handler_data.size());
    }

    void Compiler::DynamicCompiler::handle_catch_4(uint64_t exception_scope) {
        size_t handle = compiler.scope_map.try_mapHandle(exception_scope);
        std::vector<uint8_t> handler_data;
        handler_data.push_back(4);
        compiler.scope.setExceptionHandle(handle, exception::_attacha_filter, handler_data.data(), handler_data.size());
    }

    void Compiler::DynamicCompiler::handle_catch_5(uint64_t exception_scope, uint64_t local_fun_id, uint16_t enviro_slice_begin, uint16_t enviro_slice_end) {
        size_t handle = compiler.scope_map.try_mapHandle(exception_scope);
        std::vector<uint8_t> handler_data;
        handler_data.reserve(40);
        handler_data.push_back(5);
        handler_data.push_back(0);
        builder::write(handler_data, compiler.build_func->localFn(local_fun_id)->get_func_ptr());
        builder::write(handler_data, enviro_slice_begin);
        builder::write(handler_data, enviro_slice_end);
        compiler.scope.setExceptionHandle(handle, exception::_attacha_filter, handler_data.data(), handler_data.size());
    }

    void Compiler::DynamicCompiler::handle_catch_5(uint64_t exception_scope, const art::ustring& function_symbol, uint16_t enviro_slice_begin, uint16_t enviro_slice_end) {
        size_t handle = compiler.scope_map.try_mapHandle(exception_scope);
        std::vector<uint8_t> handler_data;
        handler_data.reserve(40);
        handler_data.push_back(5);
        handler_data.push_back(1);

        builder::write(handler_data, FuncEnvironment::environment(function_symbol)->get_func_ptr());
        builder::write(handler_data, enviro_slice_begin);
        builder::write(handler_data, enviro_slice_end);
        compiler.scope.setExceptionHandle(handle, exception::_attacha_filter, handler_data.data(), handler_data.size());
    }

    void Compiler::DynamicCompiler::handle_finally(uint64_t exception_scope, uint64_t local_fun_id, uint16_t enviro_slice_begin, uint16_t enviro_slice_end) {
        size_t handle = compiler.scope_map.try_mapHandle(exception_scope);
        if (handle == -1)
            throw InvalidArguments("Undefined handle");
        std::vector<uint8_t> handler_data;
        handler_data.reserve(sizeof(Environment) + sizeof(uint16_t) * 2 + 1);
        handler_data.push_back(0);
        builder::write(handler_data, compiler.build_func->localFn(local_fun_id)->get_func_ptr());
        builder::write(handler_data, enviro_slice_begin);
        builder::write(handler_data, enviro_slice_end);
        compiler.scope.setExceptionFinal(handle, exception::_attacha_finally, handler_data.data(), handler_data.size());
    }

    void Compiler::DynamicCompiler::handle_finally(uint64_t exception_scope, const art::ustring& function_symbol, uint16_t enviro_slice_begin, uint16_t enviro_slice_end) {
        size_t handle = compiler.scope_map.try_mapHandle(exception_scope);
        if (handle == -1)
            throw InvalidArguments("Undefined handle");
        std::vector<uint8_t> handler_data;
        handler_data.reserve(sizeof(Environment) + sizeof(uint16_t) * 2 + 1);
        handler_data.push_back(0);

        builder::write(handler_data, FuncEnvironment::environment(function_symbol)->get_func_ptr());
        builder::write(handler_data, enviro_slice_begin);
        builder::write(handler_data, enviro_slice_end);
        compiler.scope.setExceptionFinal(handle, exception::_attacha_finally, handler_data.data(), handler_data.size());
    }


    void Compiler::DynamicCompiler::handle_end(uint64_t exception_scope) {
        bool removed = compiler.scope_map.unmapHandle(exception_scope);
        if (!removed)
            throw InvalidArguments("Undefined handle");
    }

    void Compiler::DynamicCompiler::value_hold(uint64_t hold_scope, uint16_t env_id) {
        compiler.scope_map.mapValueHold(hold_scope, universalRemove, env_id);
    }

    void Compiler::DynamicCompiler::value_unhold(uint64_t hold_scope) {
        if (!compiler.scope_map.unmapValueHold(hold_scope))
            throw InvalidArguments("Undefined hold");
    }

    void Compiler::DynamicCompiler::is_gc(const ValueIndexPos& value, const ValueIndexPos& set_bool) {
        BuildCall b(compiler.a, 0);
        b.mov_valindex({compiler.static_map, compiler.values}, value);
        b.finalize(helper_functions::ValueItem_is_gc_proxy);
        b.lea_valindex({compiler.static_map, compiler.values}, set_bool);
        b.addArg(resr);
        b.finalize(getValueItem);
    }

    void Compiler::DynamicCompiler::to_gc(const ValueIndexPos& value) {
        BuildCall b(compiler.a, 1);
        b.mov_valindex({compiler.static_map, compiler.values}, value);
        b.finalize(&ValueItem::make_gc);
    }

    void Compiler::DynamicCompiler::from_gc(const ValueIndexPos& value) {
        BuildCall b(compiler.a, 1);
        b.mov_valindex({compiler.static_map, compiler.values}, value);
        b.finalize(&ValueItem::ungc);
    }

    void Compiler::DynamicCompiler::localize_gc(const ValueIndexPos& value) {
        BuildCall b(compiler.a, 1);
        b.mov_valindex({compiler.static_map, compiler.values}, value);
        b.finalize(&ValueItem::localize_gc);
    }

    void Compiler::DynamicCompiler::xarray_slice(const ValueIndexPos& result, const ValueIndexPos& array, const ValueIndexPos& from, const ValueIndexPos& to) {
        BuildCall b(compiler.a, 4);
        b.lea_valindex({compiler.static_map, compiler.values}, result);
        b.lea_valindex({compiler.static_map, compiler.values}, array);
        b.lea_valindex({compiler.static_map, compiler.values}, from);
        b.lea_valindex({compiler.static_map, compiler.values}, to);
        b.finalize(helper_functions::ValueItem_xmake_slice11);
    }

    void Compiler::DynamicCompiler::xarray_slice(const ValueIndexPos& result, const ValueIndexPos& array, const ValueIndexPos& from) {
        BuildCall b(compiler.a, 4);
        b.lea_valindex({compiler.static_map, compiler.values}, result);
        b.lea_valindex({compiler.static_map, compiler.values}, array);
        b.lea_valindex({compiler.static_map, compiler.values}, from);
        b.addArg(0);
        b.finalize(helper_functions::ValueItem_xmake_slice10);
    }

    void Compiler::DynamicCompiler::xarray_slice(const ValueIndexPos& result, const ValueIndexPos& array) {
        BuildCall b(compiler.a, 4);
        b.lea_valindex({compiler.static_map, compiler.values}, result);
        b.lea_valindex({compiler.static_map, compiler.values}, array);
        b.addArg(0);
        b.addArg(0);
        b.finalize(helper_functions::ValueItem_xmake_slice00);
    }

    void Compiler::DynamicCompiler::xarray_slice(const ValueIndexPos& result, const ValueIndexPos& array, bool, const ValueIndexPos& to) {
        BuildCall b(compiler.a, 4);
        b.lea_valindex({compiler.static_map, compiler.values}, result);
        b.lea_valindex({compiler.static_map, compiler.values}, array);
        b.addArg(0);
        b.lea_valindex({compiler.static_map, compiler.values}, to);
        b.finalize(helper_functions::ValueItem_xmake_slice01);
    }

    void Compiler::DynamicCompiler::get_reference(const ValueIndexPos& result, const ValueIndexPos& value) {
        BuildCall b(compiler.a, 2);
        b.lea_valindex({compiler.static_map, compiler.values}, result);
        b.lea_valindex({compiler.static_map, compiler.values}, value);
        b.finalize(helper_functions::ValueItem_make_ref);
    }

    void Compiler::DynamicCompiler::make_as_const(const ValueIndexPos& value) {
        compiler.a.mov_valindex_meta({compiler.static_map, compiler.values}, resr, value);
        compiler.a.and_(resr, ~ValueMeta(VType::noting, false, true, 0, false).encoded);
        compiler.a.mov_valindex_meta({compiler.static_map, compiler.values}, value, resr, argr0);
    }

    void Compiler::DynamicCompiler::remove_const_protect(const ValueIndexPos& value) {
        compiler.a.mov_valindex_meta({compiler.static_map, compiler.values}, resr, value);
        compiler.a.or_(resr, ValueMeta(VType::noting, false, true, 0, false).encoded);
        compiler.a.mov_valindex_meta({compiler.static_map, compiler.values}, value, resr, argr0);
    }

    void Compiler::DynamicCompiler::copy_unconst(const ValueIndexPos& set, const ValueIndexPos& from) {
        BuildCall b(compiler.a, 2);
        b.lea_valindex({compiler.static_map, compiler.values}, set);
        b.lea_valindex({compiler.static_map, compiler.values}, from);
        b.finalize((ValueItem & (ValueItem::*)(const ValueItem&)) & ValueItem::operator=);
        remove_const_protect(set);
    }

    void Compiler::DynamicCompiler::copy_unreference(const ValueIndexPos& set, const ValueIndexPos& from) {
        BuildCall b(compiler.a, 2);
        b.lea_valindex({compiler.static_map, compiler.values}, set);
        b.lea_valindex({compiler.static_map, compiler.values}, from);
        b.finalize(helper_functions::ValueItem_copy_unref);
    }

    void Compiler::DynamicCompiler::move_unreference(const ValueIndexPos& set, const ValueIndexPos& from) {
        BuildCall b(compiler.a, 2);
        b.lea_valindex({compiler.static_map, compiler.values}, set);
        b.lea_valindex({compiler.static_map, compiler.values}, from);
        b.finalize(helper_functions::ValueItem_move_unref);
    }

    void Compiler::DynamicCompiler::remove_qualifiers(const ValueIndexPos& value) {
        from_gc(value);
        remove_const_protect(value);
    }
}