// Copyright Danyil Melnytskyi 2022-Present
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <run_time/AttachA_CXX.hpp>
#include <run_time/asm/CASM.hpp>
#include <run_time/asm/dynamic_call.hpp>
#include <run_time/func_enviro_builder.hpp>
#include <run_time/tasks/util/light_stack.hpp>

namespace art {
    namespace internal {
        namespace memory {
            //returns faarr[faarr[ptr from, ptr to, len, str desk, bool is_fault]...], arg: array/value ptr
            ValueItem* dump(ValueItem* vals, uint32_t len) {
                if (len == 1)
                    return light_stack::dump(vals[0].getSourcePtr());
                throw InvalidArguments("This function requires 1 argument");
            }
        }

        namespace stack {
            //reduce stack size, returns bool, args: shrink threshold(optional)
            ValueItem* shrink(ValueItem* vals, uint32_t len) {
                if (len == 1)
                    return new ValueItem(light_stack::shrink_current((size_t)vals[0]));
                else if (len == 0)
                    return new ValueItem(light_stack::shrink_current());
                throw InvalidArguments("This function requires 0 or 1 argument");
            }

            //grow stack size, returns bool, args: grow count
            ValueItem* prepare(ValueItem* vals, uint32_t len) {
                if (len == 1)
                    return new ValueItem(light_stack::prepare((size_t)vals[0]));
                else if (len == 0)
                    return new ValueItem(light_stack::prepare());
                throw InvalidArguments("This function requires 0 or 1 argument");
            }

            //make sure stack size is enough and increase if too small, returns bool, args: grow count
            ValueItem* reserve(ValueItem* vals, uint32_t len) {
                if (len == 1)
                    return new ValueItem(light_stack::prepare((size_t)vals[0]));
                throw InvalidArguments("This function requires 1 argument");
            }

            //returns faarr[faarr[ptr from, ptr to, str desk, bool is_fault]...], args: none
            ValueItem* dump(ValueItem* vals, uint32_t len) {
                if (len == 0)
                    return light_stack::dump_current();
                throw InvalidArguments("This function requires 0 argument");
            }

            ValueItem* bs_supported(ValueItem* vals, uint32_t len) {
                return new ValueItem(light_stack::is_supported());
            }

            ValueItem* used_size(ValueItem*, uint32_t) {
                return new ValueItem(light_stack::used_size());
            }

            ValueItem* unused_size(ValueItem*, uint32_t) {
                return new ValueItem(light_stack::unused_size());
            }

            ValueItem* allocated_size(ValueItem*, uint32_t) {
                return new ValueItem(light_stack::allocated_size());
            }

            ValueItem* free_size(ValueItem*, uint32_t) {
                return new ValueItem(light_stack::free_size());
            }

            ValueItem* trace(ValueItem* vals, uint32_t len) {
                uint32_t framesToSkip = 0;
                bool include_native = true;
                uint32_t max_frames = 32;
                if (len >= 1)
                    framesToSkip = (uint32_t)vals[0];
                if (len >= 2)
                    include_native = (bool)vals[1];
                if (len >= 3)
                    max_frames = (uint32_t)vals[2];

                auto res = FrameResult::JitCaptureStackTrace(framesToSkip, include_native, max_frames);
                auto res2 = new ValueItem[res.size()];
                for (size_t i = 0; i < res.size(); i++)
                    res2[i] = ValueItem{res[i].file_path, res[i].fn_name, (uint64_t)res[i].line};
                return new ValueItem(res2, ValueMeta(VType::faarr, false, true, res.size()));
            }

            ValueItem* trace_frames(ValueItem* vals, uint32_t len) {
                uint32_t framesToSkip = 0;
                bool include_native = true;
                uint32_t max_frames = 32;
                if (len >= 1)
                    framesToSkip = (uint32_t)vals[0];
                if (len >= 2)
                    include_native = (bool)vals[1];
                if (len >= 3)
                    max_frames = (uint32_t)vals[2];
                auto res = FrameResult::JitCaptureStackChainTrace(framesToSkip, include_native, max_frames);
                auto res2 = new ValueItem[res.size()];
                for (size_t i = 0; i < res.size(); i++)
                    res2[i] = res[i];
                return new ValueItem(res2, ValueMeta(VType::faarr, false, true, res.size()));
            }

            ValueItem* resolve_frame(ValueItem* vals, uint32_t len) {
                StackTraceItem res;
                if (len == 1)
                    res = FrameResult::JitResolveFrame((void*)vals[0]);
                else if (len == 2)
                    res = FrameResult::JitResolveFrame((void*)vals[0], (bool)vals[1]);
                else
                    throw InvalidArguments("This function requires 1 argument and second optional: (rip, include_native)");
                return new ValueItem{res.file_path, res.fn_name, res.line};
            }
        }

        namespace run_time {
            ValueItem* gc_pause(ValueItem*, uint32_t) {
                //lgr not support pause
                return nullptr;
            }

            ValueItem* gc_resume(ValueItem*, uint32_t) {
                //lgr not support resume
                return nullptr;
            }

            //gc can ignore this hint
            ValueItem* gc_hint_collect(ValueItem*, uint32_t) {
                //lgr is determined, not need to hint
                return nullptr;
            }

            namespace native {
                AttachAVirtualTable* define_NativeLib;
                AttachAVirtualTable* define_NativeTemplate;
                AttachAVirtualTable* define_NativeValue;

                namespace constructor {
                    ValueItem* createProxy_NativeValue(ValueItem*, uint32_t) {
                        return new ValueItem(CXX::Interface::constructStructure<typed_lgr<DynamicCall::FunctionTemplate::ValueT>>(define_NativeValue, new DynamicCall::FunctionTemplate::ValueT()), no_copy);
                    }

                    ValueItem* createProxy_NativeTemplate(ValueItem*, uint32_t) {
                        return new ValueItem(CXX::Interface::constructStructure<typed_lgr<DynamicCall::FunctionTemplate>>(define_NativeTemplate, new DynamicCall::FunctionTemplate()), no_copy);
                    }
                    AttachAFun(createProxy_NativeLib, 1, {
                        return ValueItem(CXX::Interface::constructStructure<typed_lgr<NativeLib>>(define_NativeLib, new NativeLib(((art::ustring)args[0]).c_str())), no_copy);
                    })
                }

                AttachAFun(funs_NativeLib_get_function, 3, {
                    auto& class_ = CXX::Interface::getExtractAs<typed_lgr<NativeLib>>(args[0], define_NativeLib);
                    auto fun_name = (art::ustring)args[1];
                    auto& template_ = *CXX::Interface::getExtractAs<typed_lgr<DynamicCall::FunctionTemplate>>(args[2], define_NativeTemplate);
                    return ValueItem(class_->get_func_enviro(fun_name, template_));
                });

                AttachAFun(funs_NativeLib_get_own_function, 2, {
                    auto& class_ = CXX::Interface::getExtractAs<typed_lgr<NativeLib>>(args[0], define_NativeLib);
                    auto fun_name = (art::ustring)args[1];
                    return ValueItem(class_->get_own_enviro(fun_name));
                });

                AttachAFun(funs_NativeTemplate_add_argument, 3, {
                    auto& class_ = CXX::Interface::getExtractAs<typed_lgr<DynamicCall::FunctionTemplate>>(args[0], define_NativeTemplate);
                    auto& value = *CXX::Interface::getExtractAs<typed_lgr<DynamicCall::FunctionTemplate::ValueT>>(args[1], define_NativeValue);
                    class_->arguments.push_back(value);
                    return nullptr;
                });

#define funs_setter(name, class, set_typ, extract_typ)                                                 \
    AttachAFun(funs_NativeTemplate_setter_##name, 2, {                                                 \
        auto& class_ = CXX::Interface::getExtractAs<typed_lgr<class>>(args[0], define_NativeTemplate); \
        class_->name = set_typ((extract_typ)args[1]);                                                  \
    });
#define funs_getter(name, class, middle_cast)                                                          \
    AttachAFun(funs_NativeTemplate_getter_##name, 1, {                                                 \
        auto& class_ = CXX::Interface::getExtractAs<typed_lgr<class>>(args[0], define_NativeTemplate); \
        return ValueItem((middle_cast)class_->name);                                                   \
    });

                DynamicCall::FunctionTemplate::ValueT castValueT(uint64_t val) {
                    return *(DynamicCall::FunctionTemplate::ValueT*)&val;
                }

                funs_setter(result, DynamicCall::FunctionTemplate, castValueT, uint64_t);

                AttachAFun(funs_NativeTemplate_getter_result, 1, {
                    auto& class_ = CXX::Interface::getExtractAs<typed_lgr<DynamicCall::FunctionTemplate>>(args[0], define_NativeTemplate);
                    return ValueItem(*(uint64_t*)&class_->result);
                });
                funs_setter(is_variadic, DynamicCall::FunctionTemplate, bool, bool);
                funs_getter(is_variadic, DynamicCall::FunctionTemplate, bool);


                funs_setter(is_modifiable, DynamicCall::FunctionTemplate::ValueT, bool, bool);
                funs_getter(is_modifiable, DynamicCall::FunctionTemplate::ValueT, bool);
                funs_setter(vsize, DynamicCall::FunctionTemplate::ValueT, uint32_t, uint32_t);
                funs_getter(vsize, DynamicCall::FunctionTemplate::ValueT, uint32_t);
                funs_setter(ptype, DynamicCall::FunctionTemplate::ValueT, DynamicCall::FunctionTemplate::ValueT::PlaceType, uint8_t);
                funs_getter(ptype, DynamicCall::FunctionTemplate::ValueT, uint8_t);
                funs_setter(vtype, DynamicCall::FunctionTemplate::ValueT, DynamicCall::FunctionTemplate::ValueT::ValueType, uint8_t);
                funs_getter(vtype, DynamicCall::FunctionTemplate::ValueT, uint8_t);

                void init() {
                    static bool is_init = false;
                    if (is_init)
                        return;
                    define_NativeLib = CXX::Interface::createTable<typed_lgr<NativeLib>>(
                        "native_lib",
                        CXX::Interface::direct_method("get_function", funs_NativeLib_get_function),
                        CXX::Interface::direct_method("get_own_function", funs_NativeLib_get_own_function)
                    );
                    define_NativeTemplate = CXX::Interface::createTable<typed_lgr<DynamicCall::FunctionTemplate>>(
                        "native_template",
                        CXX::Interface::direct_method("add_argument", funs_NativeTemplate_add_argument),
                        CXX::Interface::direct_method("set_return_type", funs_NativeTemplate_setter_result),
                        CXX::Interface::direct_method("get_return_type", funs_NativeTemplate_getter_result),
                        CXX::Interface::direct_method("set_is_variadic", funs_NativeTemplate_setter_is_variadic),
                        CXX::Interface::direct_method("get_is_variadic", funs_NativeTemplate_getter_is_variadic)
                    );
                    define_NativeValue = CXX::Interface::createTable<typed_lgr<DynamicCall::FunctionTemplate::ValueT>>(
                        "native_value",
                        CXX::Interface::direct_method("set_is_modifiable", funs_NativeTemplate_setter_is_modifiable),
                        CXX::Interface::direct_method("get_is_modifiable", funs_NativeTemplate_getter_is_modifiable),
                        CXX::Interface::direct_method("set_vsize", funs_NativeTemplate_setter_vsize),
                        CXX::Interface::direct_method("get_vsize", funs_NativeTemplate_getter_vsize),
                        CXX::Interface::direct_method("set_place_type", funs_NativeTemplate_setter_ptype),
                        CXX::Interface::direct_method("get_place_type", funs_NativeTemplate_getter_ptype),
                        CXX::Interface::direct_method("set_value_type", funs_NativeTemplate_setter_vtype),
                        CXX::Interface::direct_method("get_value_type", funs_NativeTemplate_getter_vtype)
                    );
                    CXX::Interface::typeVTable<typed_lgr<NativeLib>>() = define_NativeLib;
                    CXX::Interface::typeVTable<typed_lgr<DynamicCall::FunctionTemplate>>() = define_NativeTemplate;
                    CXX::Interface::typeVTable<typed_lgr<DynamicCall::FunctionTemplate::ValueT>>() = define_NativeValue;
                    is_init = true;
                }
            }
        }

        namespace constructor {
            AttachAVirtualTable* define_FuncBuilder;
            AttachAVirtualTable* define_ValueIndexPos;
            AttachAVirtualTable* define_FuncEnviroBuilder_line_info;
            AttachAVirtualTable* define_UniversalCompiler;

            ValueItem makeVIP(ValueIndexPos vpos) {
                return ValueItem(CXX::Interface::constructStructure<ValueIndexPos>(define_ValueIndexPos, vpos));
            }

            ValueItem makeLineInfo(FuncEnviroBuilder_line_info lf) {
                return ValueItem(CXX::Interface::constructStructure<FuncEnviroBuilder_line_info>(define_FuncEnviroBuilder_line_info, lf));
            }

            ValueIndexPos getVIP(ValueItem vip) {
                return CXX::Interface::getExtractAs<ValueIndexPos>(vip, define_ValueIndexPos);
            }

            FuncEnviroBuilder_line_info getLineInfo(ValueItem lf) {
                return CXX::Interface::getExtractAs<FuncEnviroBuilder_line_info>(lf, define_FuncEnviroBuilder_line_info);
            }

            AttachAFun(funs_FuncBuilder_create_constant, 2, {
                return makeVIP(CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->create_constant(args[1]));
            });
            AttachAFun(funs_FuncBuilder_set_stack_any_array, 3, {
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->set_stack_any_array(getVIP(args[1]), (uint32_t)args[2]);
            });
            AttachAFun(funs_FuncBuilder_remove, 2, {
                if (len > 2)
                    CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->remove(getVIP(args[1]), (ValueMeta)args[2]);
                else
                    CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->remove(getVIP(args[1]));
            });
            AttachAFun(funs_FuncBuilder_sum, 3, {
                if (len > 4)
                    CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->sum(getVIP(args[1]), getVIP(args[2]), (ValueMeta)args[3], (ValueMeta)args[4]);
                else
                    CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->sum(getVIP(args[1]), getVIP(args[2]));
            });
            AttachAFun(funs_FuncBuilder_minus, 3, {
                if (len > 4)
                    CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->minus(getVIP(args[1]), getVIP(args[2]), (ValueMeta)args[3], (ValueMeta)args[4]);
                else
                    CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->minus(getVIP(args[1]), getVIP(args[2]));
            });
            AttachAFun(funs_FuncBuilder_div, 3, {
                if (len > 4)
                    CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->div(getVIP(args[1]), getVIP(args[2]), (ValueMeta)args[3], (ValueMeta)args[4]);
                else
                    CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->div(getVIP(args[1]), getVIP(args[2]));
            });
            AttachAFun(funs_FuncBuilder_mul, 3, {
                if (len > 4)
                    CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->mul(getVIP(args[1]), getVIP(args[2]), (ValueMeta)args[3], (ValueMeta)args[4]);
                else
                    CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->mul(getVIP(args[1]), getVIP(args[2]));
            });
            AttachAFun(funs_FuncBuilder_rest, 3, {
                if (len > 4)
                    CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->rest(getVIP(args[1]), getVIP(args[2]), (ValueMeta)args[3], (ValueMeta)args[4]);
                else
                    CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->rest(getVIP(args[1]), getVIP(args[2]));
            });
            AttachAFun(funs_FuncBuilder_bit_xor, 3, {
                if (len > 4)
                    CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->bit_xor(getVIP(args[1]), getVIP(args[2]), (ValueMeta)args[3], (ValueMeta)args[4]);
                else
                    CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->bit_xor(getVIP(args[1]), getVIP(args[2]));
            });
            AttachAFun(funs_FuncBuilder_bit_or, 3, {
                if (len > 4)
                    CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->bit_or(getVIP(args[1]), getVIP(args[2]), (ValueMeta)args[3], (ValueMeta)args[4]);
                else
                    CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->bit_or(getVIP(args[1]), getVIP(args[2]));
            });
            AttachAFun(funs_FuncBuilder_bit_and, 3, {
                if (len > 4)
                    CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->bit_and(getVIP(args[1]), getVIP(args[2]), (ValueMeta)args[3], (ValueMeta)args[4]);
                else
                    CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->bit_and(getVIP(args[1]), getVIP(args[2]));
            });
            AttachAFun(funs_FuncBuilder_bit_not, 2, {
                if (len > 2)
                    CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->bit_not(getVIP(args[1]), (ValueMeta)args[2]);
                else
                    CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->bit_not(getVIP(args[1]));
            });
            AttachAFun(funs_FuncBuilder_log_not, 1, {
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->log_not();
            });
            AttachAFun(funs_FuncBuilder_compare, 3, {
                if (len > 4)
                    CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->compare(getVIP(args[1]), getVIP(args[2]), (ValueMeta)args[3], (ValueMeta)args[4]);
                else
                    CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->compare(getVIP(args[1]), getVIP(args[2]));
            });
            AttachAFun(funs_FuncBuilder_jump, 2, {
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->jump((JumpCondition)(uint8_t)args[1], (art::ustring)args[2]);
            });
            AttachAFun(funs_FuncBuilder_arg_set, 2, {
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->arg_set(getVIP(args[1]));
            });
            AttachAFun(funs_FuncBuilder_call, 2, {
                bool is_async = false;
                if (len > 2) {
                    if (args[2].meta.vtype == VType::struct_) {
                        if (len > 3)
                            is_async = (bool)args[3];
                        CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->call(getVIP(args[1]), getVIP(args[2]), is_async);
                        return nullptr;
                    }
                    is_async = (bool)args[2];
                }
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->call(getVIP(args[1]), is_async);
            });
            AttachAFun(funs_FuncBuilder_call_self, 1, {
                bool is_async = false;
                if (len > 1) {
                    if (args[1].meta.vtype == VType::struct_) {
                        if (len > 2)
                            is_async = (bool)args[2];
                        CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->call_self(getVIP(args[1]), is_async);
                        return nullptr;
                    }
                    is_async = (bool)args[1];
                }
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->call_self(is_async);
            });
            AttachAFun(funs_FuncBuilder_add_local_fn, 1, {
                return CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->add_local_fn((art::shared_ptr<FuncEnvironment>)args[1]);
            });
            AttachAFun(funs_FuncBuilder_call_local, 2, {
                bool is_async = false;
                if (len > 2) {
                    if (args[2].meta.vtype == VType::struct_) {
                        if (len > 3)
                            is_async = (bool)args[3];
                        CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->call_local(getVIP(args[1]), getVIP(args[2]), is_async);
                        return nullptr;
                    }
                    is_async = (bool)args[2];
                }
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->call_local(getVIP(args[1]), is_async);
            });
            AttachAFun(funs_FuncBuilder_call_and_ret, 2, {
                bool is_async = len > 2 ? (bool)args[2] : false;
                bool fn_mem_only_str = len > 3 ? (bool)args[3] : false;
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->call_and_ret(getVIP(args[1]), is_async, fn_mem_only_str);
            });
            AttachAFun(funs_FuncBuilder_call_self_and_ret, 1, {
                bool is_async = len > 1 ? (bool)args[1] : false;
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->call_self_and_ret(is_async);
            });
            AttachAFun(funs_FuncBuilder_call_local_and_ret, 2, {
                bool is_async = len > 2 ? (bool)args[2] : false;
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->call_local_and_ret(getVIP(args[1]), is_async);
            });
            AttachAFun(funs_FuncBuilder_ret, 1, {
                if (len == 1)
                    CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->ret();
                else
                    CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->ret(getVIP(args[1]));
            });
            AttachAFun(funs_FuncBuilder_ret_take, 2, {
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->ret_take(getVIP(args[1]));
            });
            AttachAFun(funs_FuncBuilder_copy, 3, {
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->copy(getVIP(args[1]), getVIP(args[2]));
            });
            AttachAFun(funs_FuncBuilder_move, 3, {
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->move(getVIP(args[1]), getVIP(args[2]));
            });
            AttachAFun(funs_FuncBuilder_debug_break, 1, {
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->debug_break();
            });
            AttachAFun(funs_FuncBuilder_force_debug_break, 1, {
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->force_debug_break();
            });
            AttachAFun(funs_FuncBuilder_throw_ex, 3, {
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->throw_ex(getVIP(args[1]), getVIP(args[2]));
            });
            AttachAFun(funs_FuncBuilder_as, 3, {
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->as(getVIP(args[1]), ((ValueMeta)args[2]).vtype);
            });
            AttachAFun(funs_FuncBuilder_is, 3, {
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->is(getVIP(args[1]), ((ValueMeta)args[2]).vtype);
            });
            AttachAFun(funs_FuncBuilder_is_gc, 2, {
                if (len == 2)
                    CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->is_gc(getVIP(args[1]));
                else
                    CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->is_gc(getVIP(args[1]), getVIP(args[2]));
            });
            AttachAFun(funs_FuncBuilder_store_bool, 2, {
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->store_bool(getVIP(args[1]));
            });
            AttachAFun(funs_FuncBuilder_load_bool, 2, {
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->load_bool(getVIP(args[1]));
            });

            //internal
            AttachAFun(funs_FuncBuilder_inline_native_opcode, 2, {
                if (allow_intern_access == false)
                    throw NotImplementedException(); //act like we don't have this function
                if (args[1].meta.vtype != VType::raw_arr_i8 && args[1].meta.vtype != VType::raw_arr_ui8)
                    CXX::excepted(args[1], VType::raw_arr_ui8);
                else
                    CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->inline_native_opcode((uint8_t*)args[1].getSourcePtr(), args[1].meta.val_len);
            });

            AttachAFun(funs_FuncBuilder_bind_pos, 2, {
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->bind_pos((art::ustring)args[1]);
            });

            AttachAFun(funs_FuncBuilder_static_arr_set, 5, {
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->static_arr(getVIP(args[1]), (ValueMeta)args[2]).set(getVIP(args[3]), getVIP(args[4]), len > 5 ? (bool)args[5] : true, len > 6 ? (ArrCheckMode)(uint8_t)args[6] : ArrCheckMode::no_check);
            });
            AttachAFun(funs_FuncBuilder_static_arr_insert, 5, {
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->static_arr(getVIP(args[1]), (ValueMeta)args[2]).insert(getVIP(args[3]), getVIP(args[4]), len > 5 ? (bool)args[5] : true);
            });
            AttachAFun(funs_FuncBuilder_static_arr_push_end, 4, {
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->static_arr(getVIP(args[1]), (ValueMeta)args[2]).push_end(getVIP(args[3]), len > 4 ? (bool)args[4] : true);
            });
            AttachAFun(funs_FuncBuilder_static_arr_push_start, 4, {
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->static_arr(getVIP(args[1]), (ValueMeta)args[2]).push_start(getVIP(args[3]), len > 4 ? (bool)args[4] : true);
            });
            AttachAFun(funs_FuncBuilder_static_arr_insert_range, 7, {
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->static_arr(getVIP(args[1]), (ValueMeta)args[2]).insert_range(getVIP(args[3]), getVIP(args[4]), getVIP(args[5]), getVIP(args[6]), len > 7 ? (bool)args[7] : true);
            });
            AttachAFun(funs_FuncBuilder_static_arr_get, 5, {
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->static_arr(getVIP(args[1]), (ValueMeta)args[2]).get(getVIP(args[3]), getVIP(args[4]), len > 5 ? (bool)args[5] : true);
            });
            AttachAFun(funs_FuncBuilder_static_arr_take, 5, {
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->static_arr(getVIP(args[1]), (ValueMeta)args[2]).take(getVIP(args[3]), getVIP(args[4]), len > 5 ? (bool)args[5] : true);
            });
            AttachAFun(funs_FuncBuilder_static_arr_take_end, 4, {
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->static_arr(getVIP(args[1]), (ValueMeta)args[2]).take_end(getVIP(args[3]), len > 4 ? (bool)args[4] : true);
            });
            AttachAFun(funs_FuncBuilder_static_arr_take_start, 4, {
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->static_arr(getVIP(args[1]), (ValueMeta)args[2]).take_start(getVIP(args[3]), len > 4 ? (bool)args[4] : true);
            });
            AttachAFun(funs_FuncBuilder_static_arr_get_range, 6, {
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->static_arr(getVIP(args[1]), (ValueMeta)args[2]).get_range(getVIP(args[3]), getVIP(args[4]), getVIP(args[5]), len > 6 ? (bool)args[6] : true);
            });
            AttachAFun(funs_FuncBuilder_static_arr_take_range, 6, {
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->static_arr(getVIP(args[1]), (ValueMeta)args[2]).take_range(getVIP(args[3]), getVIP(args[4]), getVIP(args[5]), len > 6 ? (bool)args[6] : true);
            });
            AttachAFun(funs_FuncBuilder_static_arr_pop_end, 3, {
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->static_arr(getVIP(args[1]), (ValueMeta)args[2]).pop_end();
            });
            AttachAFun(funs_FuncBuilder_static_arr_pop_start, 3, {
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->static_arr(getVIP(args[1]), (ValueMeta)args[2]).pop_start();
            });
            AttachAFun(funs_FuncBuilder_static_arr_remove_item, 4, {
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->static_arr(getVIP(args[1]), (ValueMeta)args[2]).remove_item(getVIP(args[3]));
            });
            AttachAFun(funs_FuncBuilder_static_arr_remove_range, 5, {
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->static_arr(getVIP(args[1]), (ValueMeta)args[2]).remove_range(getVIP(args[3]), getVIP(args[4]));
            });
            AttachAFun(funs_FuncBuilder_static_arr_resize, 4, {
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->static_arr(getVIP(args[1]), (ValueMeta)args[2]).resize(getVIP(args[3]));
            });
            AttachAFun(funs_FuncBuilder_static_arr_resize_default, 5, {
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->static_arr(getVIP(args[1]), (ValueMeta)args[2]).resize_default(getVIP(args[3]), getVIP(args[4]));
            });
            AttachAFun(funs_FuncBuilder_static_arr_reserve_push_end, 4, {
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->static_arr(getVIP(args[1]), (ValueMeta)args[2]).reserve_push_end(getVIP(args[3]));
            });
            AttachAFun(funs_FuncBuilder_static_arr_reserve_push_start, 4, {
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->static_arr(getVIP(args[1]), (ValueMeta)args[2]).reserve_push_start(getVIP(args[3]));
            });
            AttachAFun(funs_FuncBuilder_static_arr_commit, 3, {
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->static_arr(getVIP(args[1]), (ValueMeta)args[2]).commit();
            });
            AttachAFun(funs_FuncBuilder_static_arr_decommit, 4, {
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->static_arr(getVIP(args[1]), (ValueMeta)args[2]).decommit(getVIP(args[3]));
            });
            AttachAFun(funs_FuncBuilder_static_arr_remove_reserved, 3, {
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->static_arr(getVIP(args[1]), (ValueMeta)args[2]).remove_reserved();
            });
            AttachAFun(funs_FuncBuilder_static_arr_size, 4, {
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->static_arr(getVIP(args[1]), (ValueMeta)args[2]).size(getVIP(args[3]));
            });


            AttachAFun(funs_FuncBuilder_arr_set, 4, {
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->arr(getVIP(args[1])).set(getVIP(args[2]), getVIP(args[3]), len > 4 ? (bool)args[4] : true, len > 5 ? (ArrCheckMode)(uint8_t)args[5] : ArrCheckMode::no_check);
            });
            AttachAFun(funs_FuncBuilder_arr_insert, 4, {
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->arr(getVIP(args[1])).insert(getVIP(args[2]), getVIP(args[3]), len > 4 ? (bool)args[4] : true);
            });
            AttachAFun(funs_FuncBuilder_arr_push_end, 3, {
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->arr(getVIP(args[1])).push_end(getVIP(args[2]), len > 3 ? (bool)args[3] : true);
            });
            AttachAFun(funs_FuncBuilder_arr_push_start, 3, {
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->arr(getVIP(args[1])).push_start(getVIP(args[2]), len > 3 ? (bool)args[3] : true);
            });
            AttachAFun(funs_FuncBuilder_arr_insert_range, 6, {
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->arr(getVIP(args[1])).insert_range(getVIP(args[2]), getVIP(args[3]), getVIP(args[4]), getVIP(args[5]), len > 6 ? (bool)args[6] : true);
            });
            AttachAFun(funs_FuncBuilder_arr_get, 4, {
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->arr(getVIP(args[1])).get(getVIP(args[2]), getVIP(args[3]), len > 4 ? (bool)args[4] : true, len > 5 ? (ArrCheckMode)(uint8_t)args[5] : ArrCheckMode::no_check);
            });
            AttachAFun(funs_FuncBuilder_arr_take, 4, {
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->arr(getVIP(args[1])).take(getVIP(args[2]), getVIP(args[3]), len > 4 ? (bool)args[4] : true);
            });
            AttachAFun(funs_FuncBuilder_arr_take_end, 3, {
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->arr(getVIP(args[1])).take_end(getVIP(args[2]), len > 3 ? (bool)args[3] : true);
            });
            AttachAFun(funs_FuncBuilder_arr_take_start, 3, {
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->arr(getVIP(args[1])).take_start(getVIP(args[2]), len > 3 ? (bool)args[3] : true);
            });
            AttachAFun(funs_FuncBuilder_arr_get_range, 5, {
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->arr(getVIP(args[1])).get_range(getVIP(args[2]), getVIP(args[3]), getVIP(args[4]), len > 5 ? (bool)args[5] : true);
            });
            AttachAFun(funs_FuncBuilder_arr_take_range, 5, {
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->arr(getVIP(args[1])).take_range(getVIP(args[2]), getVIP(args[3]), getVIP(args[4]), len > 5 ? (bool)args[5] : true);
            });
            AttachAFun(funs_FuncBuilder_arr_pop_end, 2, {
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->arr(getVIP(args[1])).pop_end();
            });
            AttachAFun(funs_FuncBuilder_arr_pop_start, 2, {
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->arr(getVIP(args[1])).pop_start();
            });
            AttachAFun(funs_FuncBuilder_arr_remove_item, 3, {
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->arr(getVIP(args[1])).remove_item(getVIP(args[2]));
            });
            AttachAFun(funs_FuncBuilder_arr_remove_range, 4, {
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->arr(getVIP(args[1])).remove_range(getVIP(args[2]), getVIP(args[3]));
            });
            AttachAFun(funs_FuncBuilder_arr_resize, 3, {
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->arr(getVIP(args[1])).resize(getVIP(args[2]));
            });
            AttachAFun(funs_FuncBuilder_arr_resize_default, 4, {
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->arr(getVIP(args[1])).resize_default(getVIP(args[2]), getVIP(args[3]));
            });
            AttachAFun(funs_FuncBuilder_arr_reserve_push_end, 3, {
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->arr(getVIP(args[1])).reserve_push_end(getVIP(args[2]));
            });
            AttachAFun(funs_FuncBuilder_arr_reserve_push_start, 3, {
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->arr(getVIP(args[1])).reserve_push_start(getVIP(args[2]));
            });
            AttachAFun(funs_FuncBuilder_arr_commit, 2, {
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->arr(getVIP(args[1])).commit();
            });
            AttachAFun(funs_FuncBuilder_arr_decommit, 3, {
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->arr(getVIP(args[1])).decommit(getVIP(args[2]));
            });
            AttachAFun(funs_FuncBuilder_arr_remove_reserved, 2, {
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->arr(getVIP(args[1])).remove_reserved();
            });
            AttachAFun(funs_FuncBuilder_arr_size, 3, {
                CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder)->arr(getVIP(args[1])).size(getVIP(args[2]));
            });

            AttachAFun(funs_FuncBuilder_call_value_interface, 4, {
                auto& builder = CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder);
                ClassAccess access = (ClassAccess)(uint8_t)args[1];
                ValueIndexPos class_val = getVIP(args[2]);
                auto vpos = getVIP(args[3]);
                if (len > 4) {
                    if (args[4].meta.vtype == VType::boolean)
                        builder->call_value_interface(access, class_val, vpos, (bool)args[4]);
                    else
                        builder->call_value_interface(access, class_val, vpos, getVIP(args[4]), len > 5 ? (bool)args[5] : false);
                } else
                    builder->call_value_interface(access, class_val, vpos);
            });
            AttachAFun(funs_FuncBuilder_call_value_interface_id, 3, {
                auto& builder = CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder);
                ValueIndexPos class_val = getVIP(args[1]);
                uint64_t class_fun_id = (uint64_t)args[2];
                if (len > 3) {
                    if (args[3].meta.vtype == VType::boolean)
                        builder->call_value_interface_id(class_val, class_fun_id, (bool)args[3]);
                    else
                        builder->call_value_interface_id(class_val, class_fun_id, getVIP(args[3]), len > 4 ? (bool)args[4] : false);
                } else
                    builder->call_value_interface_id(class_val, class_fun_id);
            });
            AttachAFun(funs_FuncBuilder_call_value_interface_and_ret, 4, {
                auto& builder = CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder);
                ClassAccess access = (ClassAccess)(uint8_t)args[1];
                ValueIndexPos class_val = getVIP(args[2]);
                builder->call_value_interface_and_ret(access, class_val, getVIP(args[3]), len > 4 ? (bool)args[4] : false);
            });
            AttachAFun(funs_FuncBuilder_call_value_interface_id_and_ret, 3, {
                auto& builder = CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder);
                ValueIndexPos class_val = getVIP(args[1]);
                uint64_t class_fun_id = (uint64_t)args[2];
                builder->call_value_interface_id_and_ret(class_val, class_fun_id, len > 3 ? (bool)args[3] : false);
            });
            AttachAFun(funs_FuncBuilder_static_call_value_interface, 4, {
                auto& builder = CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder);
                ClassAccess access = (ClassAccess)(uint8_t)args[1];
                ValueIndexPos class_val = getVIP(args[2]);
                auto vpos = getVIP(args[3]);
                if (len > 4) {
                    if (args[4].meta.vtype == VType::boolean)
                        builder->static_call_value_interface(access, class_val, vpos, (bool)args[4]);
                    else
                        builder->static_call_value_interface(access, class_val, vpos, getVIP(args[4]), len > 5 ? (bool)args[5] : false);
                } else
                    builder->static_call_value_interface(access, class_val, vpos);
            });
            AttachAFun(funs_FuncBuilder_static_call_value_interface_id, 4, {
                auto& builder = CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder);
                ValueIndexPos class_val = getVIP(args[1]);
                uint64_t fun_id = (uint64_t)args[2];

                if (len == 2)
                    builder->static_call_value_interface_id(class_val, fun_id);
                else {
                    if (args[3].meta.vtype == VType::boolean)
                        builder->static_call_value_interface_id(class_val, fun_id, (bool)args[3]);
                    else
                        builder->static_call_value_interface_id(class_val, fun_id, getVIP(args[3]), len > 4 ? (bool)args[4] : false);
                }
            });
            AttachAFun(funs_FuncBuilder_static_call_value_interface_and_ret, 4, {
                auto& builder = CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder);
                ClassAccess access = (ClassAccess)(uint8_t)args[1];
                ValueIndexPos class_val = getVIP(args[2]);
                builder->static_call_value_interface_and_ret(access, class_val, getVIP(args[3]), len > 4 ? (bool)args[4] : false);
            });
            AttachAFun(funs_FuncBuilder_static_call_value_interface_id_and_ret, 3, {
                auto& builder = CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder);
                ValueIndexPos class_val = getVIP(args[1]);
                uint64_t fun_id = (uint64_t)args[2];
                builder->static_call_value_interface_id_and_ret(class_val, fun_id, len > 3 ? (bool)args[3] : false);
            });
            AttachAFun(funs_FuncBuilder_get_interface_value, 5, {
                auto& builder = CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder);
                ClassAccess access = (ClassAccess)(uint8_t)args[1];
                ValueIndexPos class_val = getVIP(args[2]);
                builder->get_interface_value(access, class_val, getVIP(args[3]), getVIP(args[4]));
            });
            AttachAFun(funs_FuncBuilder_set_interface_value, 5, {
                auto& builder = CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder);
                ClassAccess access = (ClassAccess)(uint8_t)args[1];
                ValueIndexPos class_val = getVIP(args[2]);
                builder->set_interface_value(access, class_val, getVIP(args[3]), getVIP(args[4]));
            });
            AttachAFun(funs_FuncBuilder_explicit_await, 2, {
                auto& builder = CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder);
                builder->explicit_await(getVIP(args[1]));
            });
            AttachAFun(funs_FuncBuilder_to_gc, 2, {
                auto& builder = CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder);
                builder->to_gc(getVIP(args[1]));
            });
            AttachAFun(funs_FuncBuilder_localize_gc, 2, {
                auto& builder = CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder);
                builder->localize_gc(getVIP(args[1]));
            });
            AttachAFun(funs_FuncBuilder_from_gc, 2, {
                auto& builder = CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder);
                builder->from_gc(getVIP(args[1]));
            });
            AttachAFun(funs_FuncBuilder_xarray_slice, 3, {
                auto& builder = CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder);
                ValueIndexPos result = getVIP(args[1]);
                ValueIndexPos val = getVIP(args[2]);
                bool first_arg_variant = false;
                bool second_arg_variant = false;
                if (len > 3) {
                    if (args[3].meta.vtype == VType::noting)
                        first_arg_variant = false;
                    else
                        first_arg_variant = true;
                }
                if (len > 4) {
                    if (args[4].meta.vtype == VType::noting)
                        second_arg_variant = false;
                    else
                        second_arg_variant = true;
                }

                if (first_arg_variant) {
                    if (second_arg_variant)
                        builder->xarray_slice(result, val);
                    else
                        builder->xarray_slice(result, val, false, getVIP(args[4]));
                } else {
                    builder->xarray_slice(result, val, getVIP(args[3]));
                    break;
                    builder->xarray_slice(result, val, getVIP(args[3]), getVIP(args[4]));
                    break;
                }
            });
            AttachAFun(funs_FuncBuilder_table_jump, 2, {
                auto& builder = CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder);
                std::vector<art::ustring> table;
                switch (args[1].meta.vtype) {
                case VType::uarr:
                    for (ValueItem& it : (list_array<ValueItem>&)args[1])
                        table.push_back((art::ustring)it);
                    break;
                case VType::string:
                    table.push_back((art::ustring)args[1]);
                    break;
                case VType::saarr:
                case VType::faarr: {
                    ValueItem* it = (ValueItem*)args[1].getSourcePtr();
                    uint32_t len = args[1].meta.val_len;
                    for (uint32_t i = 0; i < len; i++)
                        table.push_back((art::ustring)it[i]);
                    break;
                }
                default:
                    CXX::excepted(args[1], VType::uarr);
                    break;
                }
                ValueIndexPos check_val = getVIP(args[2]);
                bool is_signed = len > 3 ? (bool)args[3] : false;
                TableJumpCheckFailAction too_large = len > 4 ? (TableJumpCheckFailAction)(uint8_t)args[4] : TableJumpCheckFailAction::throw_exception;
                art::ustring too_large_label = len > 5 ? (art::ustring)args[5] : "";
                TableJumpCheckFailAction too_small = len > 6 ? (TableJumpCheckFailAction)(uint8_t)args[6] : TableJumpCheckFailAction::throw_exception;
                art::ustring too_small_label = len > 7 ? (art::ustring)args[7] : "";
                builder->table_jump(table, check_val, is_signed, too_large, too_large_label, too_small, too_small_label);
            });


            AttachAFun(funs_FuncBuilder_O_flag_can_be_unloaded, 2, {
                auto& builder = CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder);
                builder->O_flag_can_be_unloaded((bool)args[1]);
            });
            AttachAFun(funs_FuncBuilder_O_flag_is_translated, 2, {
                auto& builder = CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder);
                builder->O_flag_is_translated((bool)args[1]);
            });
            AttachAFun(funs_FuncBuilder_O_flag_is_cheap, 2, {
                auto& builder = CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder);
                builder->O_flag_is_cheap((bool)args[1]);
            });
            AttachAFun(funs_FuncBuilder_O_flag_used_vec128, 2, {
                auto& builder = CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder);
                builder->O_flag_used_vec128((uint8_t)args[1]);
            });
            AttachAFun(funs_FuncBuilder_O_flag_is_patchable, 2, {
                auto& builder = CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder);
                builder->O_flag_is_patchable((bool)args[1]);
            });
            AttachAFun(funs_FuncBuilder_O_line_info_begin, 1, {
                auto& builder = CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder);
                return makeLineInfo(builder->O_line_info_begin());
            });
            AttachAFun(funs_FuncBuilder_O_line_info_end, 2, {
                auto& builder = CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder);
                builder->O_line_info_end(getLineInfo(args[1]));
            });
            AttachAFun(funs_FuncBuilder_O_prepare_func, 1, {
                auto& builder = CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder);
                return builder->O_prepare_func();
            });
            AttachAFun(funs_FuncBuilder_O_build_func, 1, {
                auto& builder = CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder);
                auto result = builder->O_build_func();
                if (result.size() == (uint32_t)result.size())
                    return ValueItem(result.data(), (uint32_t)result.size());
                else {
                    list_array<ValueItem> map_result;
                    map_result.reserve_push_back(result.size());
                    for (uint8_t it : result)
                        map_result.push_back(ValueItem(it));
                    return map_result;
                }
            });
            AttachAFun(funs_FuncBuilder_O_load_func, 2, {
                auto& builder = CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder);
                builder->O_load_func((art::ustring)args[1]);
            });
            AttachAFun(funs_FuncBuilder_O_patch_func, 2, {
                auto& builder = CXX::Interface::getExtractAs<typed_lgr<FuncEnviroBuilder>>(args[0], define_FuncBuilder);
                builder->O_patch_func((art::ustring)args[1]);
            });


            AttachAFun(funs_FuncEnviroBuilder_line_info_set_line, 2, {
                auto& line_info = CXX::Interface::getExtractAs<FuncEnviroBuilder_line_info>(args[0], define_FuncEnviroBuilder_line_info);
                line_info.line = (uint64_t)args[1];
            });
            AttachAFun(funs_FuncEnviroBuilder_line_info_set_column, 2, {
                auto& line_info = CXX::Interface::getExtractAs<FuncEnviroBuilder_line_info>(args[0], define_FuncEnviroBuilder_line_info);
                line_info.column = (uint64_t)args[1];
            });
            AttachAFun(funs_FuncEnviroBuilder_line_info_get_line, 1, {
                auto& line_info = CXX::Interface::getExtractAs<FuncEnviroBuilder_line_info>(args[0], define_FuncEnviroBuilder_line_info);
                return line_info.line;
            });
            AttachAFun(funs_FuncEnviroBuilder_line_info_get_column, 1, {
                auto& line_info = CXX::Interface::getExtractAs<FuncEnviroBuilder_line_info>(args[0], define_FuncEnviroBuilder_line_info);
                return line_info.column;
            });

            AttachAFun(funs_ValueIndexPos_set_index, 2, {
                auto& vpos = CXX::Interface::getExtractAs<ValueIndexPos>(args[0], define_ValueIndexPos);
                vpos.index = (uint16_t)args[1];
            });
            AttachAFun(funs_ValueIndexPos_set_pos, 2, {
                auto& vpos = CXX::Interface::getExtractAs<ValueIndexPos>(args[0], define_ValueIndexPos);
                vpos.pos = (ValuePos)(uint8_t)args[1];
            });
            AttachAFun(funs_ValueIndexPos_get_index, 1, {
                auto& vpos = CXX::Interface::getExtractAs<ValueIndexPos>(args[0], define_ValueIndexPos);
                return vpos.index;
            });
            AttachAFun(funs_ValueIndexPos_get_pos, 1, {
                auto& vpos = CXX::Interface::getExtractAs<ValueIndexPos>(args[0], define_ValueIndexPos);
                return (uint8_t)vpos.pos;
            });


            void init() {
                define_ValueIndexPos = CXX::Interface::createTable<ValueIndexPos>(
                    "index_pos",
                    CXX::Interface::direct_method("set_index", funs_ValueIndexPos_set_index),
                    CXX::Interface::direct_method("set_pos", funs_ValueIndexPos_set_pos),
                    CXX::Interface::direct_method("get_index", funs_ValueIndexPos_get_index),
                    CXX::Interface::direct_method("get_pos", funs_ValueIndexPos_get_pos)
                );
                CXX::Interface::typeVTable<ValueIndexPos>() = define_ValueIndexPos;
                define_FuncEnviroBuilder_line_info = CXX::Interface::createTable<FuncEnviroBuilder_line_info>(
                    "line_info",
                    CXX::Interface::direct_method("set_line", funs_FuncEnviroBuilder_line_info_set_line),
                    CXX::Interface::direct_method("set_column", funs_FuncEnviroBuilder_line_info_set_column),
                    CXX::Interface::direct_method("get_line", funs_FuncEnviroBuilder_line_info_get_line),
                    CXX::Interface::direct_method("get_column", funs_FuncEnviroBuilder_line_info_get_column)
                );
                CXX::Interface::typeVTable<FuncEnviroBuilder_line_info>() = define_FuncEnviroBuilder_line_info;
                define_FuncBuilder = CXX::Interface::createTable<typed_lgr<FuncEnviroBuilder>>(
                    "func_builder",
                    CXX::Interface::direct_method("create_constant", funs_FuncBuilder_create_constant),
                    CXX::Interface::direct_method("set_stack_any_array", funs_FuncBuilder_set_stack_any_array),
                    CXX::Interface::direct_method("remove", funs_FuncBuilder_remove),
                    CXX::Interface::direct_method("sum", funs_FuncBuilder_sum),
                    CXX::Interface::direct_method("minus", funs_FuncBuilder_minus),
                    CXX::Interface::direct_method("div", funs_FuncBuilder_div),
                    CXX::Interface::direct_method("mul", funs_FuncBuilder_mul),
                    CXX::Interface::direct_method("rest", funs_FuncBuilder_rest),
                    CXX::Interface::direct_method("bit_xor", funs_FuncBuilder_bit_xor),
                    CXX::Interface::direct_method("bit_or", funs_FuncBuilder_bit_or),
                    CXX::Interface::direct_method("bit_and", funs_FuncBuilder_bit_and),
                    CXX::Interface::direct_method("bit_not", funs_FuncBuilder_bit_not),
                    CXX::Interface::direct_method("log_not", funs_FuncBuilder_log_not),
                    CXX::Interface::direct_method("compare", funs_FuncBuilder_compare),
                    CXX::Interface::direct_method("jump", funs_FuncBuilder_jump),
                    CXX::Interface::direct_method("arg_set", funs_FuncBuilder_arg_set),
                    CXX::Interface::direct_method("call", funs_FuncBuilder_call),
                    CXX::Interface::direct_method("call_self", funs_FuncBuilder_call_self),
                    CXX::Interface::direct_method("add_local_fn", funs_FuncBuilder_add_local_fn),
                    CXX::Interface::direct_method("call_local", funs_FuncBuilder_call_local),
                    CXX::Interface::direct_method("call_and_ret", funs_FuncBuilder_call_and_ret),
                    CXX::Interface::direct_method("call_self_and_ret", funs_FuncBuilder_call_self_and_ret),
                    CXX::Interface::direct_method("call_local_and_ret", funs_FuncBuilder_call_local_and_ret),
                    CXX::Interface::direct_method("ret", funs_FuncBuilder_ret),
                    CXX::Interface::direct_method("ret_take", funs_FuncBuilder_ret_take),
                    CXX::Interface::direct_method("copy", funs_FuncBuilder_copy),
                    CXX::Interface::direct_method("move", funs_FuncBuilder_move),
                    CXX::Interface::direct_method("debug_break", funs_FuncBuilder_debug_break),
                    CXX::Interface::direct_method("force_debug_break", funs_FuncBuilder_force_debug_break),
                    CXX::Interface::direct_method("throw_ex", funs_FuncBuilder_throw_ex),
                    CXX::Interface::direct_method("as", funs_FuncBuilder_as),
                    CXX::Interface::direct_method("is", funs_FuncBuilder_is),
                    CXX::Interface::direct_method("is_gc", funs_FuncBuilder_is_gc),
                    CXX::Interface::direct_method("store_bool", funs_FuncBuilder_store_bool),
                    CXX::Interface::direct_method("load_bool", funs_FuncBuilder_load_bool),
                    CXX::Interface::direct_method("inline_native_opcode", funs_FuncBuilder_inline_native_opcode, ClassAccess::intern),
                    CXX::Interface::direct_method("bind_pos", funs_FuncBuilder_bind_pos),

                    CXX::Interface::direct_method("arr_set", funs_FuncBuilder_arr_set),
                    CXX::Interface::direct_method("arr_insert", funs_FuncBuilder_arr_insert),
                    CXX::Interface::direct_method("arr_push_end", funs_FuncBuilder_arr_push_end),
                    CXX::Interface::direct_method("arr_push_start", funs_FuncBuilder_arr_push_start),
                    CXX::Interface::direct_method("arr_insert_range", funs_FuncBuilder_arr_insert_range),
                    CXX::Interface::direct_method("arr_get", funs_FuncBuilder_arr_get),
                    CXX::Interface::direct_method("arr_take", funs_FuncBuilder_arr_take),
                    CXX::Interface::direct_method("arr_take_end", funs_FuncBuilder_arr_take_end),
                    CXX::Interface::direct_method("arr_take_start", funs_FuncBuilder_arr_take_start),
                    CXX::Interface::direct_method("arr_get_range", funs_FuncBuilder_arr_get_range),
                    CXX::Interface::direct_method("arr_take_range", funs_FuncBuilder_arr_take_range),
                    CXX::Interface::direct_method("arr_pop_end", funs_FuncBuilder_arr_pop_end),
                    CXX::Interface::direct_method("arr_pop_start", funs_FuncBuilder_arr_pop_start),
                    CXX::Interface::direct_method("arr_remove_item", funs_FuncBuilder_arr_remove_item),
                    CXX::Interface::direct_method("arr_remove_range", funs_FuncBuilder_arr_remove_range),
                    CXX::Interface::direct_method("arr_resize", funs_FuncBuilder_arr_resize),
                    CXX::Interface::direct_method("arr_resize_default", funs_FuncBuilder_arr_resize_default),
                    CXX::Interface::direct_method("arr_reserve_push_end", funs_FuncBuilder_arr_reserve_push_end),
                    CXX::Interface::direct_method("arr_reserve_push_start", funs_FuncBuilder_arr_reserve_push_start),
                    CXX::Interface::direct_method("arr_commit", funs_FuncBuilder_arr_commit),
                    CXX::Interface::direct_method("arr_decommit", funs_FuncBuilder_arr_decommit),
                    CXX::Interface::direct_method("arr_remove_reserved", funs_FuncBuilder_arr_remove_reserved),
                    CXX::Interface::direct_method("arr_size", funs_FuncBuilder_arr_size),

                    CXX::Interface::direct_method("static_arr_set", funs_FuncBuilder_static_arr_set),
                    CXX::Interface::direct_method("static_arr_insert", funs_FuncBuilder_static_arr_insert),
                    CXX::Interface::direct_method("static_arr_push_end", funs_FuncBuilder_static_arr_push_end),
                    CXX::Interface::direct_method("static_arr_push_start", funs_FuncBuilder_static_arr_push_start),
                    CXX::Interface::direct_method("static_arr_insert_range", funs_FuncBuilder_static_arr_insert_range),
                    CXX::Interface::direct_method("static_arr_get", funs_FuncBuilder_static_arr_get),
                    CXX::Interface::direct_method("static_arr_take", funs_FuncBuilder_static_arr_take),
                    CXX::Interface::direct_method("static_arr_take_end", funs_FuncBuilder_static_arr_take_end),
                    CXX::Interface::direct_method("static_arr_take_start", funs_FuncBuilder_static_arr_take_start),
                    CXX::Interface::direct_method("static_arr_get_range", funs_FuncBuilder_static_arr_get_range),
                    CXX::Interface::direct_method("static_arr_take_range", funs_FuncBuilder_static_arr_take_range),
                    CXX::Interface::direct_method("static_arr_pop_end", funs_FuncBuilder_static_arr_pop_end),
                    CXX::Interface::direct_method("static_arr_pop_start", funs_FuncBuilder_static_arr_pop_start),
                    CXX::Interface::direct_method("static_arr_remove_item", funs_FuncBuilder_static_arr_remove_item),
                    CXX::Interface::direct_method("static_arr_remove_range", funs_FuncBuilder_static_arr_remove_range),
                    CXX::Interface::direct_method("static_arr_resize", funs_FuncBuilder_static_arr_resize),
                    CXX::Interface::direct_method("static_arr_resize_default", funs_FuncBuilder_static_arr_resize_default),
                    CXX::Interface::direct_method("static_arr_reserve_push_end", funs_FuncBuilder_static_arr_reserve_push_end),
                    CXX::Interface::direct_method("static_arr_reserve_push_start", funs_FuncBuilder_static_arr_reserve_push_start),
                    CXX::Interface::direct_method("static_arr_commit", funs_FuncBuilder_static_arr_commit),
                    CXX::Interface::direct_method("static_arr_decommit", funs_FuncBuilder_static_arr_decommit),
                    CXX::Interface::direct_method("static_arr_remove_reserved", funs_FuncBuilder_static_arr_remove_reserved),
                    CXX::Interface::direct_method("static_arr_size", funs_FuncBuilder_static_arr_size),

                    CXX::Interface::direct_method("call_value_interface", funs_FuncBuilder_call_value_interface),
                    CXX::Interface::direct_method("call_value_interface_id", funs_FuncBuilder_call_value_interface_id),
                    CXX::Interface::direct_method("call_value_interface_and_ret", funs_FuncBuilder_call_value_interface_and_ret),
                    CXX::Interface::direct_method("call_value_interface_id_and_ret", funs_FuncBuilder_call_value_interface_id_and_ret),
                    CXX::Interface::direct_method("static_call_value_interface", funs_FuncBuilder_static_call_value_interface),
                    CXX::Interface::direct_method("static_call_value_interface_id", funs_FuncBuilder_static_call_value_interface_id),
                    CXX::Interface::direct_method("static_call_value_interface_and_ret", funs_FuncBuilder_static_call_value_interface_and_ret),
                    CXX::Interface::direct_method("static_call_value_interface_id_and_ret", funs_FuncBuilder_static_call_value_interface_id_and_ret),
                    CXX::Interface::direct_method("get_interface_value", funs_FuncBuilder_get_interface_value),
                    CXX::Interface::direct_method("set_interface_value", funs_FuncBuilder_set_interface_value),
                    CXX::Interface::direct_method("explicit_await", funs_FuncBuilder_explicit_await),
                    CXX::Interface::direct_method("to_gc", funs_FuncBuilder_to_gc),
                    CXX::Interface::direct_method("localize_gc", funs_FuncBuilder_localize_gc),
                    CXX::Interface::direct_method("from_gc", funs_FuncBuilder_from_gc),
                    CXX::Interface::direct_method("xarray_slice", funs_FuncBuilder_xarray_slice),
                    CXX::Interface::direct_method("table_jump", funs_FuncBuilder_table_jump),
                    CXX::Interface::direct_method("O_flag_can_be_unloaded", funs_FuncBuilder_O_flag_can_be_unloaded),
                    CXX::Interface::direct_method("O_flag_is_translated", funs_FuncBuilder_O_flag_is_translated),
                    CXX::Interface::direct_method("O_flag_is_cheap", funs_FuncBuilder_O_flag_is_cheap),
                    CXX::Interface::direct_method("O_flag_used_vec128", funs_FuncBuilder_O_flag_used_vec128),
                    CXX::Interface::direct_method("O_flag_is_patchable", funs_FuncBuilder_O_flag_is_patchable),
                    CXX::Interface::direct_method("O_line_info_begin", funs_FuncBuilder_O_line_info_begin),
                    CXX::Interface::direct_method("O_line_info_end", funs_FuncBuilder_O_line_info_end),
                    CXX::Interface::direct_method("O_prepare_func", funs_FuncBuilder_O_prepare_func),
                    CXX::Interface::direct_method("O_build_func", funs_FuncBuilder_O_build_func),
                    CXX::Interface::direct_method("O_load_func", funs_FuncBuilder_O_load_func),
                    CXX::Interface::direct_method("O_patch_func", funs_FuncBuilder_O_patch_func)
                );
                CXX::Interface::typeVTable<typed_lgr<FuncEnviroBuilder>>() = define_FuncBuilder;
            }

            ValueItem* createProxy_function_builder(ValueItem* args, uint32_t len) {
                bool strict_mode = len >= 1 ? (bool)args[0] : true;
                bool use_dynamic_values = len >= 2 ? (bool)args[1] : true;
                return new ValueItem(CXX::Interface::constructStructure<typed_lgr<FuncEnviroBuilder>>(define_FuncBuilder, new FuncEnviroBuilder(strict_mode, use_dynamic_values)));
            }

            ValueItem* createProxy_index_pos(ValueItem* args, uint32_t len) {
                uint64_t index = len >= 1 ? (uint64_t)args[0] : 0;
                ValuePos pos = len >= 2 ? (ValuePos)(uint8_t)args[1] : ValuePos::in_enviro;
                return new ValueItem(CXX::Interface::constructStructure<ValueIndexPos>(define_ValueIndexPos, index, pos));
            }

            ValueItem* createProxy_line_info(ValueItem* args, uint32_t len) {
                uint64_t line = len >= 1 ? (uint64_t)args[0] : 0;
                uint64_t column = len >= 2 ? (uint64_t)args[1] : 0;
                return new ValueItem(CXX::Interface::constructStructure<FuncEnviroBuilder_line_info>(define_FuncEnviroBuilder_line_info, line, column));
            }
        }

        void init() {
            constructor::init();
        }
    }
}