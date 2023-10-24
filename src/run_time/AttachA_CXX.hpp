// Copyright Danyil Melnytskyi 2022-Present
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once
#include <type_traits> //std::forward

#include <configuration/agreement/symbols.hpp>
#include <run_time/asm/dynamic_call_proxy.hpp>
#include <run_time/attacha_abi.hpp>
#include <run_time/func_enviro_builder.hpp>
#include <run_time/util/templates.hpp>

namespace art {
    namespace CXX {
        inline ValueItem aCall(const art::shared_ptr<FuncEnvironment>& func, ValueItem* args, uint32_t len) {
            ValueItem* res = func->syncWrapper(args, len);
            if (res == nullptr)
                return {};
            ValueItem m(std::move(*res));
            delete res;
            return m;
        }

        inline ValueItem aCall(const art::shared_ptr<FuncEnvironment>& func, ValueItem& args) {
            ValueItem copyArgs;
            if (args.meta.vtype == VType::faarr || args.meta.vtype == VType::saarr) {
                if (!args.meta.use_gc)
                    copyArgs = ValueItem(args, as_reference);
                else
                    copyArgs = ValueItem((ValueItem*)args.getSourcePtr(), args.meta.val_len, as_reference);
            } else {
                if (args.meta.vtype != VType::noting)
                    copyArgs = ValueItem(&args, 1, as_reference);
            }
            return aCall(func, (ValueItem*)copyArgs.val, copyArgs.meta.val_len);
        }

        inline ValueItem aCall(Environment func, ValueItem* args, uint32_t len) {
            ValueItem* res = func(args, len);
            if (res == nullptr)
                return {};
            ValueItem m(std::move(*res));
            delete res;
            return m;
        }

        inline ValueItem aCall(Environment func, ValueItem& args) {
            ValueItem copyArgs;
            if (args.meta.vtype == VType::faarr || args.meta.vtype == VType::saarr) {
                if (!args.meta.use_gc)
                    copyArgs = ValueItem(args, as_reference);
                else
                    copyArgs = ValueItem((ValueItem*)args.getSourcePtr(), args.meta.val_len, as_reference);
            } else {
                if (args.meta.vtype != VType::noting)
                    copyArgs = ValueItem(&args, 1, as_reference);
            }
            return aCall(func, (ValueItem*)copyArgs.val, copyArgs.meta.val_len);
        }

        template <class... Types>
        ValueItem cxxCall(art::shared_ptr<FuncEnvironment> func, Types... types) {
            ValueItem args[] = {ABI_IMPL::BVcast(types)...};
            ValueItem* res = func->syncWrapper(args, sizeof...(Types));
            if (res == nullptr)
                return {};
            ValueItem m(std::move(*res));
            delete res;
            return m;
        }

        inline ValueItem cxxCall(art::shared_ptr<FuncEnvironment> func) {
            ValueItem* res = func->syncWrapper(nullptr, 0);
            if (res == nullptr)
                return {};
            ValueItem m(std::move(*res));
            delete res;
            return m;
        }

        template <class... Types>
        ValueItem cxxCall(const art::ustring& fun_name, Types... types) {
            return cxxCall(FuncEnvironment::environment(fun_name), std::forward<Types>(types)...);
        }

        inline ValueItem cxxCall(const art::ustring& fun_name) {
            ValueItem* res = FuncEnvironment::environment(fun_name)->syncWrapper(nullptr, 0);
            if (res == nullptr)
                return {};
            ValueItem m(std::move(*res));
            delete res;
            return m;
        }

        template <class... Types>
        ValueItem cxxCall(Environment func, Types... types) {
            ValueItem args[] = {ABI_IMPL::BVcast(types)...};
            ValueItem* res = func(args, sizeof...(Types));
            if (res == nullptr)
                return {};
            ValueItem m(std::move(*res));
            delete res;
            return m;
        }

        inline ValueItem cxxCall(Environment func) {
            ValueItem* res = func(nullptr, 0);
            if (res == nullptr)
                return {};
            ValueItem m(std::move(*res));
            delete res;
            return m;
        }


        inline void excepted(ValueItem& v, VType type) {
            if (v.meta.vtype != type)
                throw InvalidArguments("Expected " + enum_to_string(type) + " got " + enum_to_string(v.meta.vtype));
        }

        inline void excepted(ValueItem& v, ValueMeta meta) {
            if (v.meta.encoded != meta.encoded) {
                if (v.meta.vtype != meta.vtype)
                    throw InvalidArguments("Expected " + enum_to_string(meta.vtype) + " got " + enum_to_string(v.meta.vtype));
                if (v.meta.allow_edit != meta.allow_edit)
                    throw InvalidArguments("Expected " + art::ustring(meta.allow_edit ? "non const" : "const") + " value got, " + art::ustring(v.meta.allow_edit ? "non const" : "const"));
                if (v.meta.as_ref != meta.as_ref)
                    throw InvalidArguments("Expected " + art::ustring(meta.as_ref ? "reference" : "value") + " got, " + art::ustring(v.meta.as_ref ? "reference" : "value"));
                if (v.meta.use_gc != meta.use_gc)
                    throw InvalidArguments("Expected " + art::ustring(meta.use_gc ? "gc" : "non gc") + " got, " + art::ustring(v.meta.use_gc ? "gc" : "non gc"));
                if (is_raw_array(v.meta.vtype))
                    if (v.meta.val_len != meta.val_len)
                        throw InvalidArguments("Expected array size " + std::to_string(meta.val_len) + " got " + std::to_string(v.meta.val_len));
                throw InvalidArguments("Caught invalid encoded value, expected " + std::to_string(meta.encoded) + " got " + std::to_string(v.meta.encoded));
            }
        }

        inline void excepted_basic_array(ValueItem& v) {
            if (!is_raw_array(v.meta.vtype) && v.meta.vtype != VType::faarr && v.meta.vtype != VType::saarr)
                throw InvalidArguments("Expected basic array got " + enum_to_string(v.meta.vtype));
        }

        inline void excepted_raw_array(ValueItem& v) {
            if (!is_raw_array(v.meta.vtype))
                throw InvalidArguments("Expected raw array got " + enum_to_string(v.meta.vtype));
        }

        inline void excepted_array(ValueItem& v) {
            if (!is_raw_array(v.meta.vtype) && v.meta.vtype != VType::uarr)
                throw InvalidArguments("Expected array got " + enum_to_string(v.meta.vtype));
        }

        inline void arguments_range(uint32_t argc, uint32_t min, uint32_t max) {
            if (argc < min || argc > max)
                throw InvalidArguments("Invalid arguments count, expected " + std::to_string(min) + " to " + std::to_string(max) + ", got " + std::to_string(argc));
        }

        inline void arguments_range(uint32_t argc, uint32_t min) {
            if (argc < min)
                throw InvalidArguments("Invalid arguments count, expected " + std::to_string(min) + " or more, got " + std::to_string(argc));
        }

        inline void array_size_min(size_t arr_len, uint32_t min) {
            if (arr_len < min)
                throw InvalidArguments("Excepted array with at least " + std::to_string(min) + " items, but got" + std::to_string(arr_len));
        }

        namespace Interface {
            inline ValueItem makeCall(ClassAccess access, Structure& c, const art::ustring& fun_name) {
                ValueItem arg(&c, as_reference);
                ValueItem* res = c.table_get_dynamic(fun_name, access)(&arg, 1);
                if (res == nullptr)
                    return {};
                ValueItem m(std::move(*res));
                delete res;
                return m;
            }

            inline ValueItem makeCall(ClassAccess access, ValueItem& c, const art::ustring& fun_name) {
                if (c.meta.vtype == VType::struct_) {
                    ValueItem* res = ((Structure&)c).table_get_dynamic(fun_name, access)(&c, 1);
                    if (res == nullptr)
                        return {};
                    ValueItem m(std::move(*res));
                    delete res;
                    return m;
                } else
                    throw InvalidArguments("Invalid type for call");
            }

            template <class... Types>
            ValueItem makeCall(ClassAccess access, Structure& c, const art::ustring& fun_name, const Types&... types) {
                ValueItem args[] = {ValueItem(&c, as_reference), ABI_IMPL::BVcast(types)...};
                ValueItem* res = c.table_get_dynamic(fun_name, access)(args, sizeof...(Types) + 1);
                if (res == nullptr)
                    return {};
                ValueItem m(std::move(*res));
                delete res;
                return m;
            }

            template <class... Types>
            ValueItem makeCall(ClassAccess access, ValueItem& c, const art::ustring& fun_name, const Types&... types) {
                if (c.meta.vtype == VType::struct_) {
                    ValueItem args[] = {ValueItem(c, as_reference), ABI_IMPL::BVcast(types)...};
                    ValueItem* res = ((const Structure&)c).table_get_dynamic(fun_name, access)(args, sizeof...(Types) + 1);
                    if (res == nullptr)
                        return {};
                    ValueItem m(std::move(*res));
                    delete res;
                    return m;
                } else
                    throw InvalidArguments("Invalid type for call");
            }

            inline ValueItem makeCall(ClassAccess access, ValueItem& c, const art::ustring& fun_name, ValueItem* args, uint32_t len) {
                if (c.meta.vtype == VType::struct_) {
                    list_array<ValueItem> args_tmp(args, args + len, len);
                    args_tmp.push_front(ValueItem(c, as_reference));
                    ValueItem* res = ((Structure&)c).table_get_dynamic(fun_name, access)(args_tmp.data(), len + 1);
                    if (res == nullptr)
                        return {};
                    ValueItem m(std::move(*res));
                    delete res;
                    return m;
                } else
                    throw InvalidArguments("Invalid type for call");
            }

            inline ValueItem makeCall(ClassAccess access, const Structure& c, const art::ustring& fun_name) {
                ValueItem arg(&c, as_reference);
                ValueItem* res = c.table_get_dynamic(fun_name, access)(&arg, 1);
                if (res == nullptr)
                    return {};
                ValueItem m(std::move(*res));
                delete res;
                return m;
            }

            inline ValueItem makeCall(ClassAccess access, const ValueItem& c, const art::ustring& fun_name) {
                if (c.meta.vtype == VType::struct_) {
                    ValueItem arg(c, as_reference);
                    ValueItem* res = ((Structure&)c).table_get_dynamic(fun_name, access)(&arg, 1);
                    if (res == nullptr)
                        return {};
                    ValueItem m(std::move(*res));
                    delete res;
                    return m;
                } else
                    throw InvalidArguments("Invalid type for call");
            }

            template <class... Types>
            ValueItem makeCall(ClassAccess access, const Structure& c, const art::ustring& fun_name, const Types&... types) {
                ValueItem args[] = {ValueItem(&c, as_reference), ABI_IMPL::BVcast(types)...};
                ValueItem* res = c.table_get_dynamic(fun_name, access)(args, sizeof...(Types) + 1);
                if (res == nullptr)
                    return {};
                ValueItem m(std::move(*res));
                delete res;
                return m;
            }

            template <class... Types>
            ValueItem makeCall(ClassAccess access, const ValueItem& c, const art::ustring& fun_name, const Types&... types) {
                if (c.meta.vtype == VType::struct_) {
                    ValueItem args[] = {ValueItem(c, as_reference), ABI_IMPL::BVcast(types)...};
                    ValueItem* res = ((const Structure&)c).table_get_dynamic(fun_name, access)(args, sizeof...(Types) + 1);
                    if (res == nullptr)
                        return {};
                    ValueItem m(std::move(*res));
                    delete res;
                    return m;
                } else
                    throw InvalidArguments("Invalid type for call");
            }

            inline ValueItem makeCall(ClassAccess access, const ValueItem& c, const art::ustring& fun_name, ValueItem* args, uint32_t len) {
                if (c.meta.vtype == VType::struct_) {
                    list_array<ValueItem> args_tmp(args, args + len, len);
                    args_tmp.push_front(ValueItem(c, as_reference));
                    ValueItem* res = ((Structure&)c).table_get_dynamic(fun_name, access)(args_tmp.data(), len + 1);
                    if (res == nullptr)
                        return {};
                    ValueItem m(std::move(*res));
                    delete res;
                    return m;
                } else
                    throw InvalidArguments("Invalid type for call");
            }

            inline ValueItem getValue(const Structure& c, const art::ustring& val_name) {
                return c.dynamic_value_get(val_name, ClassAccess::pub);
            }

            inline ValueItem getValue(const ValueItem& c, const art::ustring& val_name) {
                switch (c.meta.vtype) {
                case VType::struct_:
                    return ((Structure&)c).dynamic_value_get(val_name, ClassAccess::pub);
                default:
                    throw NotImplementedException();
                }
            }

            inline ValueItem getValue(ClassAccess access, const Structure& c, const art::ustring& val_name) {
                return c.dynamic_value_get(val_name, access);
            }

            inline ValueItem getValue(ClassAccess access, const ValueItem& c, const art::ustring& val_name) {
                switch (c.meta.vtype) {
                case VType::struct_:
                    return ((const Structure&)c).dynamic_value_get(val_name, access);
                default:
                    throw NotImplementedException();
                }
            }

            inline void setValue(Structure& c, const art::ustring& val_name, const ValueItem& set) {
                c.dynamic_value_set(val_name, ClassAccess::pub, set);
            }

            inline void setValue(ValueItem& c, const art::ustring& val_name, const ValueItem& set) {
                switch (c.meta.vtype) {
                case VType::struct_:
                    ((Structure&)c).dynamic_value_set(val_name, ClassAccess::pub, set);
                    break;
                default:
                    throw NotImplementedException();
                }
            }

            inline void setValue(ClassAccess access, Structure& c, const art::ustring& val_name, const ValueItem& set) {
                c.dynamic_value_set(val_name, access, set);
            }

            inline void setValue(ClassAccess access, ValueItem& c, const art::ustring& val_name, const ValueItem& set) {
                switch (c.meta.vtype) {
                case VType::struct_:
                    ((Structure&)c).dynamic_value_set(val_name, access, set);
                    break;
                default:
                    throw NotImplementedException();
                }
            }

            inline bool hasImplement(const Structure& c, const art::ustring& fun_name, ClassAccess access) {
                return c.has_method(fun_name, access);
            }

            inline bool hasImplement(const ValueItem& c, const art::ustring& fun_name, ClassAccess access = ClassAccess::pub) {
                switch (c.meta.vtype) {
                case VType::struct_:
                    return ((const Structure&)c).has_method(fun_name, access);
                default:
                    return false;
                }
            }

            inline art::ustring name(const Structure& c) {
                return c.get_name();
            }

            inline art::ustring name(const ValueItem& c) {
                switch (c.meta.vtype) {
                case VType::struct_:
                    return ((Structure&)c).get_name();
                default:
                    return "$ Not class $";
                }
            }

            inline bool is_interface(const Structure& c) {
                return true;
            }

            inline bool is_interface(const ValueItem& c) {
                switch (c.meta.vtype) {
                case VType::struct_:
                    return true;
                default:
                    return false;
                }
            }

            template <class... Methods>
            struct single_method {
                std::tuple<Methods...> methods;
                art::ustring name;

                single_method(const art::ustring& name, Methods... methods)
                    : name(name), methods(std::forward_as_tuple(methods...)) {}
            };

            struct direct_method {
                art::ustring name;
                Environment env;
                ClassAccess access = ClassAccess::pub;
            };

            namespace _createProxyTable_Impl_ {
                template <class Method, size_t i>
                inline std::pair<size_t, size_t> _proceed__find_best_method(ValueItem* args, uint32_t len) {
                    using method_info = templates::function_info<decltype(Method::value)>;
                    if (method_info::arguments_count == len) {
                        size_t score = 0;
                        for (size_t k = 0; k < len; k++) {
                            if (method_info::arguments[k].encoded == args[k].meta.encoded) {
                                score++;
                            }
                        }
                        return {i, score};
                    } else if (method_info::always_perfect)
                        return {i, len};
                    return {0, 0};
                }

                template <size_t index, class Head, class... Methods>
                void _iterate__find_best_method(std::pair<size_t, size_t>& res, ValueItem* args, uint32_t len) {
                    auto [method, score] = _proceed__find_best_method<Head, index>(args, len);
                    if (score == len) {
                        res = {method, score};
                        return;
                    }
                    if constexpr (sizeof...(Methods) > 0)
                        _iterate__find_best_method<index + 1, Methods...>(res, args, len);
                }

                template <class... Methods>
                size_t _find_best_method_index(ValueItem* args, uint32_t len) {
                    std::pair<size_t, size_t> res = {-1, 0};
                    _iterate__find_best_method<0, Methods...>(res, args, len);
                    return res.first;
                }

                template <class Class_, class Method>
                ValueItem* callMethod(ValueItem* args, uint32_t len) {
                    using method_info = templates::function_info<decltype(Method::value)>;
                    if constexpr (method_info::is_static) {
                        if constexpr (method_info::always_perfect)
                            return Method::value(args + 1, len - 1);
                        else {
                            size_t i = 0;
                            DynamicCall::FunctionTemplate template_fun;
                            DynamicCall::buildFn(Method::value, template_fun);
                            auto fnptr = Method::value;
                            DynamicCall::PROC as_proc = reinterpret_cast<DynamicCall::PROC&>(fnptr);
                            DynamicCall::FunctionCall call(as_proc, template_fun, true);
                            return __attacha___::NativeProxy_DynamicToStatic(call, template_fun, args, len);
                        }
                    } else {
                        auto& s = (Structure&)*args;
                        if constexpr (method_info::always_perfect)
                            return (((Class_*)s.self)->*Method::value)(args + 1, len - 1);
                        else {
                            size_t i = 0;
                            DynamicCall::FunctionTemplate template_fun;
                            DynamicCall::buildFn(Method::value, template_fun);
                            auto fnptr = Method::value;
                            DynamicCall::PROC as_proc = reinterpret_cast<DynamicCall::PROC&>(fnptr);
                            DynamicCall::FunctionCall call(as_proc, s.self, template_fun, true);
                            return __attacha___::NativeProxy_DynamicToStatic(call, template_fun, args, len);
                        }
                    }
                }

                template <class Class_, class... Methods>
                ValueItem* selectorMethod(ValueItem* args, uint32_t len, size_t method_index) {
                    bool called = false;
                    ValueItem* res = nullptr;
                    size_t i = 0;
                    ([&]() {
                        if (method_index == i && !called) {
                            res = callMethod<Class_, Methods>(args, len);
                            called = true;
                        }
                    }(),
                     ...);
                    return called ? res : throw InvalidArguments("Fail automaticaly find best method for proxy");
                }

                //will be passed store_value<T> in Methods type
                template <class Class_, class... Methods>
                inline ValueItem* proxyMethod(ValueItem* args, uint32_t len) {
                    if constexpr (sizeof...(Methods) == 0) {
                        return nullptr;
                    } else {
                        size_t best_method = _find_best_method_index<Methods...>(args, len);
                        if (best_method != (size_t)-1)
                            return selectorMethod<Class_, Methods...>(args, len, best_method);
                        else
                            throw InvalidArguments("Fail automaticaly find best method for proxy");
                    }
                }

                template <class Tuple, size_t tuple_len, size_t i = 0>
                //iterate over tuple types
                inline void tuple_to_ValueMeta(list_array<ValueMeta>& res) {
                    if constexpr (tuple_len > 0) {
                        using type = typename std::tuple_element<i, Tuple>::type;
                        res.push_back(Type_as_ValueMeta<type>());
                        tuple_to_ValueMeta<Tuple, tuple_len - 1, i + 1>(res);
                    }
                }

                template <size_t tuple_len, class Tuple, size_t i = 0>
                inline void init_tuple(Tuple& tuple) {
                    if constexpr (tuple_len > 0) {
                        init_value(std::get<i>(tuple));
                        init_tuple<tuple_len - 1, Tuple, i + 1>(tuple);
                    }
                }

                template <class Class_, class... Methods>
                MethodInfo createMethodInfo(const single_method<Methods...>& method) {
                    list_array<
                        std::pair<
                            ValueMeta,
                            list_array<ValueMeta>>>
                        functions_meta = {
                            ([&]() {
                                using method_info = templates::function_info<Methods>;
                                return std::pair<ValueMeta, list_array<ValueMeta>>{
                                    Type_as_ValueMeta<method_info::return_type>(),
                                    ([&]() -> list_array<ValueMeta> {
                                        list_array<ValueMeta> args;
                                        args.reserve_push_back(method_info::arguments_count);
                                        tuple_to_ValueMeta<method_info::arguments_type, method_info::arguments_count>(args);
                                        return args;
                                    }())};
                            }())...};
                    list_array<ValueMeta> return_values = functions_meta.convert<ValueMeta>(
                        [](const std::pair<ValueMeta, list_array<ValueMeta>>& v) { return v.first; }
                    );
                    list_array<list_array<ValueMeta>> arguments = functions_meta.convert<list_array<ValueMeta>>(
                        [](const std::pair<ValueMeta, list_array<ValueMeta>>& v) { return v.second; }
                    );
                    init_tuple<sizeof...(Methods)>(method.methods);
                    return MethodInfo(
                        method.name,
                        proxyMethod<Class_, templates::store_value<Methods>...>,
                        ClassAccess::pub,
                        return_values,
                        arguments,
                        list_array<MethodTag>(),
                        "");
                }

                template <class To_compare, class Head, class... Tail>
                struct are_same : std::bool_constant<
                                      std::conjunction<std::is_same<Head, Tail>...>::value && std::is_same_v<To_compare, Head>> {};

                //destructed in cleanup
                //template <class Class_>
                //typename std::enable_if<std::is_destructible_v<Class_>, ValueItem*>::type
                //_destructor(ValueItem* args, uint32_t) {
                //    auto& s = (Structure&)*args;
                //    ((Class_*)s.self)->~Class_();
                //    return nullptr;
                //}
                template <class Class_>
                Environment destructor = nullptr;
                //[]() {
                //if constexpr (std::is_destructible_v<Class_>)
                //    return _destructor<Class_>;
                //else
                //  return nullptr;
                //}();
#define __is_same_fun(field)                                                             \
    inline bool __is_same_##field(Structure& cmp, Structure& src, Environment env) {     \
        if (cmp.vtable == src.vtable)                                                    \
            return true;                                                                 \
        switch (src.vtable_mode) {                                                       \
        case Structure::VTableMode::AttachAVirtualTable:                                 \
            if (((AttachAVirtualTable*)src.vtable)->field != env)                        \
                return false;                                                            \
            break;                                                                       \
        case Structure::VTableMode::AttachADynamicVirtualTable:                          \
            if (!((AttachADynamicVirtualTable*)src.vtable)->field)                       \
                return false;                                                            \
            if (((AttachADynamicVirtualTable*)src.vtable)->field->get_func_ptr() != env) \
                return false;                                                            \
            break;                                                                       \
        default:                                                                         \
            return false;                                                                \
        }                                                                                \
        return true;                                                                     \
    }
                __is_same_fun(copy);
                __is_same_fun(move);
                __is_same_fun(compare);

                template <class Class_>
                    requires std::copyable<Class_>
                ValueItem* _copy(ValueItem* args, uint32_t) {
                    auto& dest = (Structure&)*args;
                    auto& src = (Structure&)*(args + 1);
                    __is_same_copy(dest, src, _copy<Class_>);
                    bool in_constructor = (bool)args[2];
                    if (in_constructor)
                        new (dest.self) Class_(*(Class_*)src.self);
                    else
                        *(Class_*)dest.self = *(Class_*)src.self;
                    return nullptr;
                }
                template <class Class_>
                Environment copy = []() {
                    if constexpr (std::copyable<Class_>)
                        return _copy<Class_>;
                    else
                        return nullptr;
                }();

                template <class Class_>
                    requires std::movable<Class_>
                ValueItem* _move(ValueItem* args, uint32_t) {
                    auto& dest = (Structure&)*args;
                    auto& src = (Structure&)*(args + 1);
                    __is_same_move(dest, src, _move<Class_>);
                    bool in_constructor = (bool)args[2];
                    if (in_constructor)
                        new (dest.self) Class_(std::move(*(Class_*)src.self));
                    else
                        *(Class_*)dest.self = std::move(*(Class_*)src.self);
                    return nullptr;
                }

                template <class Class_>
                Environment move = []() {
                    if constexpr (std::movable<Class_>)
                        return _move<Class_>;
                    else
                        return nullptr;
                }();

                template <class Class_>
                    requires std::equality_comparable<Class_> && std::totally_ordered<Class_>
                ValueItem* _compare(ValueItem* args, uint32_t) {
                    auto& dest = (Structure&)*args;
                    auto& src = (Structure&)*(args + 1);
                    __is_same_compare(dest, src, _compare<Class_>);

                    if (*(Class_*)dest.self == *(Class_*)src.self)
                        return nullptr;
                    else if (*(Class_*)dest.self < *(Class_*)src.self)
                        return new ValueItem((int8_t)-1);
                    else
                        return new ValueItem((int8_t)1);
                }

                template <class Class_>
                    requires std::equality_comparable<Class_> && (!std::totally_ordered<Class_>)
                ValueItem* _compare(ValueItem* args, uint32_t) {
                    auto& dest = (Structure&)*args;
                    auto& src = (Structure&)*(args + 1);
                    __is_same_compare(dest, src, _compare<Class_>);

                    if (*(Class_*)dest.self == *(Class_*)src.self)
                        return nullptr;
                    else
                        return new ValueItem((int8_t)-1);
                }

                template <class Class_>
                    requires(!std::equality_comparable<Class_> && !std::totally_ordered<Class_>)
                ValueItem* _compare(ValueItem* args, uint32_t) {
                    auto& dest = (Structure&)*args;
                    auto& src = (Structure&)args[1];
                    __is_same_compare(dest, src, _compare<Class_>);

                    if (dest.has_method(symbols::structures::less_operator, ClassAccess::pub)) {
                        if ((bool)makeCall(ClassAccess::pub, dest, symbols::structures::less_operator, args + 1, 1))
                            return new ValueItem((int8_t)-1);
                        else {
                            if (src.has_method(symbols::structures::greater_operator, ClassAccess::pub)) {
                                if ((bool)makeCall(ClassAccess::pub, dest, symbols::structures::less_operator, args + 1, 1))
                                    return new ValueItem((int8_t)-1);
                                else
                                    return nullptr;
                            } else
                                return args[0].getSourcePtr() == args[1].getSourcePtr() ? nullptr : new ValueItem((int8_t)-1);
                        }
                    } else
                        return args[0].getSourcePtr() == args[1].getSourcePtr() ? nullptr : new ValueItem((int8_t)-1);
                }

                template <class Class_>
                Environment compare = []() {
                    if constexpr (std::equality_comparable<Class_>)
                        return _compare<Class_>;
                    else
                        return nullptr;
                }();

                template <class Class_>
                static art::shared_ptr<FuncEnvironment> ref_destructor() {
                    static art::shared_ptr<FuncEnvironment> ref = _createProxyTable_Impl_::destructor<Class_> ? new FuncEnvironment(_createProxyTable_Impl_::destructor<Class_>, false) : nullptr;
                    return ref;
                }

                template <class Class_>
                static art::shared_ptr<FuncEnvironment> ref_copy() {
                    static art::shared_ptr<FuncEnvironment> ref = _createProxyTable_Impl_::copy<Class_> ? new FuncEnvironment(_createProxyTable_Impl_::copy<Class_>, false) : nullptr;
                    return ref;
                }

                template <class Class_>
                static art::shared_ptr<FuncEnvironment> ref_move() {
                    static art::shared_ptr<FuncEnvironment> ref = _createProxyTable_Impl_::move<Class_> ? new FuncEnvironment(_createProxyTable_Impl_::move<Class_>, false) : nullptr;
                    return ref;
                }

                template <class Class_>
                static art::shared_ptr<FuncEnvironment> ref_compare() {
                    static art::shared_ptr<FuncEnvironment> ref = _createProxyTable_Impl_::compare<Class_> ? new FuncEnvironment(_createProxyTable_Impl_::compare<Class_>, false) : nullptr;
                    return ref;
                }
            }

            template <class Class_, class... Methods>
            MethodInfo make_method(const art::ustring& name, Methods... methods) {
                return _createProxyTable_Impl_::createMethodInfo<Class_>(single_method(name, methods...));
            }

            template <class Class_, class... Methods>
            //std::enable_if Methods is MethodInfos
            typename std::enable_if<
                _createProxyTable_Impl_::are_same<MethodInfo, Methods...>::value,
                AttachADynamicVirtualTable*>::type
            createProxyDTable(Methods... methods) {
                list_array<MethodInfo> proxied;
                list_array<ValueInfo> values;
                proxied.resize(sizeof...(Methods));
                size_t i = 0;
                art::ustring owner_name = typeid(Class_).name();
                ([&]() {
                    proxied[i].owner_name = owner_name;
                    proxied[i++] = methods;
                }(),
                 ...);

                auto res = new AttachADynamicVirtualTable(
                    proxied,
                    values,
                    _createProxyTable_Impl_::ref_destructor<Class_>(),
                    _createProxyTable_Impl_::ref_copy<Class_>(),
                    _createProxyTable_Impl_::ref_move<Class_>(),
                    _createProxyTable_Impl_::ref_compare<Class_>(),
                    sizeof(Class_),
                    false
                );
                res->name = owner_name;
                return res;
            }

            template <class Class_, class... Methods>
            //std::enable_if Methods is MethodInfos
            typename std::enable_if<_createProxyTable_Impl_::are_same<MethodInfo, Methods...>::value, AttachAVirtualTable*>::type
            createProxyTable(Methods... methods) {
                list_array<MethodInfo> proxied;
                list_array<ValueInfo> values;
                proxied.resize(sizeof...(Methods));
                size_t i = 0;
                art::ustring owner_name = typeid(Class_).name();
                ([&]() {
                    proxied[i].owner_name = owner_name;
                    proxied[i++] = methods;
                }(),
                 ...);
                auto res = AttachAVirtualTable::create(
                    proxied,
                    values,
                    _createProxyTable_Impl_::ref_destructor<Class_>(),
                    _createProxyTable_Impl_::ref_copy<Class_>(),
                    _createProxyTable_Impl_::ref_move<Class_>(),
                    _createProxyTable_Impl_::ref_compare<Class_>(),
                    sizeof(Class_),
                    false
                );
                res->setName(owner_name);
                return res;
            }

            template <class Class_, class... DirectMethod>
            typename std::enable_if<_createProxyTable_Impl_::are_same<direct_method, DirectMethod...>::value, AttachADynamicVirtualTable*>::type
            createDTable(DirectMethod... methods) {
                list_array<MethodInfo> proxied;
                list_array<ValueInfo> values;
                proxied.resize(sizeof...(DirectMethod));
                size_t i = 0;
                art::ustring owner_name = typeid(Class_).name();
                ([&]() {
                    proxied[i].owner_name = owner_name;
                    proxied[i].name = methods.name;
                    proxied[i].access = methods.access;
                    proxied[i++].ref = new FuncEnvironment(methods.env, false);
                }(),
                 ...);

                auto res = new AttachADynamicVirtualTable(
                    proxied,
                    values,
                    _createProxyTable_Impl_::ref_destructor<Class_>(),
                    _createProxyTable_Impl_::ref_copy<Class_>(),
                    _createProxyTable_Impl_::ref_move<Class_>(),
                    _createProxyTable_Impl_::ref_compare<Class_>(),
                    sizeof(Class_),
                    false
                );
                res->name = owner_name;
                return res;
            }

            template <class Class_, class... DirectMethod>
            typename std::enable_if<_createProxyTable_Impl_::are_same<direct_method, DirectMethod...>::value, AttachAVirtualTable*>::type
            createTable(DirectMethod... methods) {
                list_array<MethodInfo> proxied;
                list_array<ValueInfo> values;
                proxied.resize(sizeof...(DirectMethod));
                size_t i = 0;
                art::ustring owner_name = typeid(Class_).name();
                ([&]() {
                    proxied[i].owner_name = owner_name;
                    proxied[i].name = methods.name;
                    proxied[i].access = methods.access;
                    proxied[i++].ref = new FuncEnvironment(methods.env, false);
                }(),
                 ...);
                auto res = AttachAVirtualTable::create(
                    proxied,
                    values,
                    _createProxyTable_Impl_::ref_destructor<Class_>(),
                    _createProxyTable_Impl_::ref_copy<Class_>(),
                    _createProxyTable_Impl_::ref_move<Class_>(),
                    _createProxyTable_Impl_::ref_compare<Class_>(),
                    sizeof(Class_),
                    false
                );
                res->setName(owner_name);
                return res;
            }

            template <class Class_, class... Methods>
            typename std::enable_if<
                _createProxyTable_Impl_::are_same<MethodInfo, Methods...>::value,
                AttachADynamicVirtualTable*>::type
            createProxyDTable(const art::ustring& owner_name, Methods... methods) {
                list_array<MethodInfo> proxied;
                list_array<ValueInfo> values;
                proxied.resize(sizeof...(Methods));
                size_t i = 0;
                ([&]() {
                    proxied[i].owner_name = owner_name;
                    proxied[i++] = methods;
                }(),
                 ...);
                auto res = new AttachADynamicVirtualTable(
                    proxied,
                    values,
                    _createProxyTable_Impl_::ref_destructor<Class_>(),
                    _createProxyTable_Impl_::ref_copy<Class_>(),
                    _createProxyTable_Impl_::ref_move<Class_>(),
                    _createProxyTable_Impl_::ref_compare<Class_>(),
                    sizeof(Class_),
                    false
                );
                res->name = owner_name;
                return res;
            }

            template <class Class_, class... Methods>
            typename std::enable_if<_createProxyTable_Impl_::are_same<MethodInfo, Methods...>::value, AttachAVirtualTable*>::type
            createProxyTable(const art::ustring& owner_name, Methods... methods) {
                list_array<MethodInfo> proxied;
                list_array<ValueInfo> values;
                proxied.resize(sizeof...(Methods));
                size_t i = 0;
                ([&]() {
                    proxied[i].owner_name = owner_name;
                    proxied[i++] = methods;
                }(),
                 ...);
                auto res = AttachAVirtualTable::create(
                    proxied,
                    values,
                    _createProxyTable_Impl_::ref_destructor<Class_>(),
                    _createProxyTable_Impl_::ref_copy<Class_>(),
                    _createProxyTable_Impl_::ref_move<Class_>(),
                    _createProxyTable_Impl_::ref_compare<Class_>(),
                    sizeof(Class_),
                    false
                );
                res->setName(owner_name);
                return res;
            }

            template <class Class_, class... DirectMethod>
            typename std::enable_if<_createProxyTable_Impl_::are_same<direct_method, DirectMethod...>::value, AttachADynamicVirtualTable*>::type
            createDTable(const art::ustring& owner_name, DirectMethod... methods) {
                list_array<MethodInfo> proxied;
                list_array<ValueInfo> values;
                proxied.resize(sizeof...(DirectMethod));
                size_t i = 0;
                ([&]() {
                    proxied[i].owner_name = owner_name;
                    proxied[i].name = methods.name;
                    proxied[i].access = methods.access;
                    proxied[i++].ref = new FuncEnvironment(methods.env, false);
                }(),
                 ...);

                auto res = new AttachADynamicVirtualTable(
                    proxied,
                    values,
                    _createProxyTable_Impl_::ref_destructor<Class_>(),
                    _createProxyTable_Impl_::ref_copy<Class_>(),
                    _createProxyTable_Impl_::ref_move<Class_>(),
                    _createProxyTable_Impl_::ref_compare<Class_>(),
                    sizeof(Class_),
                    false
                );
                res->name = owner_name;
                return res;
            }

            template <class Class_, class... DirectMethod>
            typename std::enable_if<_createProxyTable_Impl_::are_same<direct_method, DirectMethod...>::value, AttachAVirtualTable*>::type
            createTable(const art::ustring& owner_name, DirectMethod... methods) {
                list_array<MethodInfo> proxied;
                list_array<ValueInfo> values;
                proxied.resize(sizeof...(DirectMethod));
                size_t i = 0;
                ([&]() {
                    proxied[i].owner_name = owner_name;
                    proxied[i].name = methods.name;
                    proxied[i].access = methods.access;
                    proxied[i++].ref = new FuncEnvironment(methods.env, false);
                }(),
                 ...);

                auto res = AttachAVirtualTable::create(
                    proxied,
                    values,
                    _createProxyTable_Impl_::ref_destructor<Class_>(),
                    _createProxyTable_Impl_::ref_copy<Class_>(),
                    _createProxyTable_Impl_::ref_move<Class_>(),
                    _createProxyTable_Impl_::ref_compare<Class_>(),
                    sizeof(Class_),
                    false
                );
                res->setName(owner_name);
                return res;
            }

            template <class Class_>
            AttachADynamicVirtualTable* createProxyDTable(const art::ustring& owner_name) {
                list_array<MethodInfo> proxied;
                list_array<ValueInfo> values;
                auto res = new AttachADynamicVirtualTable(
                    proxied,
                    values,
                    _createProxyTable_Impl_::ref_destructor<Class_>(),
                    _createProxyTable_Impl_::ref_copy<Class_>(),
                    _createProxyTable_Impl_::ref_move<Class_>(),
                    _createProxyTable_Impl_::ref_compare<Class_>(),
                    sizeof(Class_),
                    false
                );
                res->name = owner_name;
                return res;
            }

            template <class Class_>
            AttachAVirtualTable* createProxyTable(const art::ustring& owner_name) {
                list_array<MethodInfo> proxied;
                list_array<ValueInfo> values;
                auto res = AttachAVirtualTable::create(
                    proxied,
                    values,
                    _createProxyTable_Impl_::ref_destructor<Class_>(),
                    _createProxyTable_Impl_::ref_copy<Class_>(),
                    _createProxyTable_Impl_::ref_move<Class_>(),
                    _createProxyTable_Impl_::ref_compare<Class_>(),
                    sizeof(Class_),
                    false
                );
                res->setName(owner_name);
                return res;
            }

            template <class Class_>
            AttachADynamicVirtualTable* createDTable(const art::ustring& owner_name) {
                list_array<MethodInfo> proxied;
                list_array<ValueInfo> values;
                auto res = new AttachADynamicVirtualTable(
                    proxied,
                    values,
                    _createProxyTable_Impl_::ref_destructor<Class_>(),
                    _createProxyTable_Impl_::ref_copy<Class_>(),
                    _createProxyTable_Impl_::ref_move<Class_>(),
                    _createProxyTable_Impl_::ref_compare<Class_>(),
                    sizeof(Class_),
                    false
                );
                res->name = owner_name;
                return res;
            }

            template <class Class_>
            AttachAVirtualTable* createTable(const art::ustring& owner_name) {
                list_array<MethodInfo> proxied;
                list_array<ValueInfo> values;
                auto res = AttachAVirtualTable::create(
                    proxied,
                    values,
                    _createProxyTable_Impl_::ref_destructor<Class_>(),
                    _createProxyTable_Impl_::ref_copy<Class_>(),
                    _createProxyTable_Impl_::ref_move<Class_>(),
                    _createProxyTable_Impl_::ref_compare<Class_>(),
                    sizeof(Class_),
                    false
                );
                res->setName(owner_name);
                return res;
            }

            template <class Class_, class... Args>
            Structure* constructStructure(AttachAVirtualTable* table, Args&&... args) {
                return new Structure(new Class_(std::forward<Args>(args)...), table, Structure::VTableMode::AttachAVirtualTable, defaultDestructor<Class_>);
            }

            template <class Class_, class... Args>
            Structure* constructStructure(AttachADynamicVirtualTable* table, Args&&... args) {
                return new Structure(new Class_(std::forward<Args>(args)...), table, Structure::VTableMode::AttachADynamicVirtualTable, defaultDestructor<Class_>);
            }
        }

        template <class>
        struct Proxy {};

        namespace _internal_ {
            template <class T>
            struct proxy_lambda_extract_args {};

            template <class ReturnTyp, class... Arguments>
            struct proxy_lambda_extract_args<ReturnTyp (*)(Arguments...)> {
                static std::tuple<Arguments...> apply(ValueItem* args, uint32_t len) {
                    arguments_range(len, sizeof...(Arguments));
                    size_t arg_i = 0;
                    return {ABI_IMPL::Vcast<Arguments>(args[arg_i++])...};
                }

                using rt_type = ReturnTyp;
            };

            template <class ReturnTyp>
            struct proxy_lambda_extract_args<ReturnTyp (*)(ValueItem*, uint32_t)> {
                static std::tuple<ValueItem*, uint32_t> apply(ValueItem* args, uint32_t len) {
                    return {args, len};
                }

                using rt_type = ReturnTyp;
            };

            template <class RealLambda, class Lambda_fn_point>
            struct ProxyLambdaHelper {
                static ValueItem* proxy(RealLambda& fn, ValueItem* args, uint32_t len) {
                    using lambda_info = proxy_lambda_extract_args<Lambda_fn_point>;
                    auto tuple = lambda_info::apply(args, len);

                    if constexpr (std::is_same_v<lambda_info::rt_type, void>) {
                        std::apply(fn, tuple);
                        return nullptr;
                    } else if constexpr (std::is_same_v<lambda_info::rt_type, ValueItem*>)
                        return std::apply(fn, tuple);
                    else
                        return new ValueItem(std::apply(fn, tuple));
                }
            };

            template <class T>
            struct ProxyLambda {
                static ValueItem* proxy(T& fn, ValueItem* args, uint32_t len) {
                    if constexpr (templates::is_simple_lambda_v<T>)
                        return Proxy<templates::fn_lambda<T>>::proxy((templates::fn_lambda<T>)fn, args, len);
                    else {
                        return _internal_::ProxyLambdaHelper<T, templates::fn_lambda<T>>::proxy(fn, args, len);
                    }
                }

                static ValueItem* abstract_Proxy(void* fn, ValueItem* args, uint32_t len) {
                    return proxy(*(T*)fn, args, len);
                }
            };

            template <class T>
            void cleanup(void* ptr) {
                delete (T*)ptr;
            }
        }


        template <class ReturnTyp, class... Arguments>
        struct Proxy<ReturnTyp (*)(Arguments...)> {
            typedef ReturnTyp (*Excepted)(Arguments...);

            static std::tuple<Arguments...> extract_args(ValueItem* args, uint32_t len) {
                arguments_range(len, sizeof...(Arguments));
                size_t arg_i = 0;
                if constexpr (std::is_same_v<std::tuple<ValueItem*, uint32_t>, std::tuple<Arguments...>>)
                    return {args, len};
                else
                    return {ABI_IMPL::Vcast<Arguments>(args[arg_i++])...};
            }

            static ValueItem* proxy(Excepted fn, ValueItem* args, uint32_t len) {
                std::tuple<Arguments...> tuple = extract_args(args, len);

                if constexpr (std::is_same_v<ReturnTyp, void>) {
                    std::apply(fn, tuple);
                    return nullptr;
                } else if constexpr (std::is_same_v<ReturnTyp, ValueItem*>)
                    return std::apply(fn, tuple);
                else
                    return new ValueItem(std::apply(fn, tuple));
            }

            static ValueItem* abstract_Proxy(void* fn, ValueItem* args, uint32_t len) {
                Excepted cls_fn;
                memcpy(&cls_fn, &fn, sizeof(Excepted));
                return proxy(cls_fn, args, len);
            }
        };

        template <class Class_, class ReturnTyp, class... Arguments>
        struct Proxy<ReturnTyp (Class_::*)(Arguments...)> {
            typedef ReturnTyp (Class_::*Excepted)(Arguments...);

            static std::tuple<Arguments...> extract_args(ValueItem* args, uint32_t len) {
                arguments_range(len, sizeof...(Arguments) + 1);
                size_t arg_i = 1;
                if (std::is_same_v<std::tuple<Class_, ValueItem*, uint32_t>, std::tuple<Class_, Arguments...>>)
                    return {std::ref(Interface::getAs<Class_>(args[0])), args, len};
                else
                    return {std::ref(Interface::getAs<Class_>(args[0])), ABI_IMPL::Vcast<Arguments>(args[arg_i++])...};
            }

            static ValueItem* proxy(Excepted fn, ValueItem* args, uint32_t len) {
                std::tuple<Class_, Arguments...> tuple = extract_args();

                if constexpr (std::is_same_v<ReturnTyp, void>) {
                    std::apply(fn, tuple);
                    return nullptr;
                } else if constexpr (std::is_same_v<ReturnTyp, ValueItem*>)
                    return std::apply(fn, tuple);
                else
                    return new ValueItem(std::apply(fn, tuple));
            }

            static ValueItem* abstract_Proxy(void* fn, ValueItem* args, uint32_t len) {
                Excepted cls_fn;
                memcpy(&cls_fn, &fn, sizeof(Excepted));
                return proxy(cls_fn, args, len);
            }
        };

        template <class _FN>
        inline art::shared_ptr<FuncEnvironment> MakeNative(_FN env, bool can_be_unloaded = true, bool is_cheap = true) {
            if constexpr (std::is_same_v<_FN, Environment>)
                return new FuncEnvironment(env, can_be_unloaded, is_cheap);
            else if constexpr (templates::is_lambda_v<_FN>) {
                if constexpr (templates::is_simple_lambda_v<_FN>) {
                    if constexpr (std::is_same_v<templates::fn_lambda<_FN>, Environment>)
                        return new FuncEnvironment((templates::fn_lambda<_FN>)env, can_be_unloaded, is_cheap);
                    else
                        return new FuncEnvironment((templates::fn_lambda<_FN>)env, nullptr, CXX::Proxy<templates::fn_lambda<_FN>>::abstract_Proxy, can_be_unloaded, is_cheap);
                } else
                    return new FuncEnvironment(new _FN(env), _internal_::cleanup<_FN>, CXX::_internal_::ProxyLambda<_FN>::abstract_Proxy, can_be_unloaded, is_cheap);
            } else
                return new FuncEnvironment((void*)env, nullptr, CXX::Proxy<_FN>::abstract_Proxy, can_be_unloaded, is_cheap);
        }

        namespace _internal_ {
            struct await_lambda {
                template <class T>
                list_array<ValueItem> operator=(T&& fn) {
                    art::typed_lgr async_ref(new Task(MakeNative(fn, true, false), {}));
                    return Task::await_results(async_ref);
                }
            };

            extern await_lambda _await_lambda;
        }
    }

    template <class _FN>
    inline void FuncEnvironment::AddNative(_FN env, const art::ustring& func_name, bool can_be_unloaded, bool is_cheap) {
        FuncEnvironment::Load(CXX::MakeNative(env, can_be_unloaded, is_cheap), func_name);
    }


#define AttachAFun(name, min_args, content)             \
    ValueItem* name(ValueItem* args, uint32_t len) {    \
        if (len >= min_args)                            \
            return new ValueItem([&]() -> ValueItem {   \
                do                                      \
                    content while (false);              \
                return nullptr;                         \
            }());                                       \
        else                                            \
            ::art::CXX::arguments_range(len, min_args); \
        return nullptr;                                 \
    }
#define AttachAManagedFun(name, min_args, content)      \
    ValueItem* name(ValueItem* args, uint32_t len) {    \
        if (len >= min_args) {                          \
            do                                          \
                content while (false);                  \
        } else                                          \
            ::art::CXX::arguments_range(len, min_args); \
        return nullptr;                                 \
    }
#define AttachAFunc(name, min_args)                               \
    ValueItem __art_native_##name(ValueItem* args, uint32_t len); \
    ValueItem* name(ValueItem* args, uint32_t len) {              \
        if (len >= min_args)                                      \
            return new ValueItem(__art_native_##name(args, len)); \
        else                                                      \
            ::art::CXX::arguments_range(len, min_args);           \
        return nullptr;                                           \
    }                                                             \
    ValueItem __art_native_##name(ValueItem* args, uint32_t len)

#define art_wait ::art::CXX::_internal_::_await_lambda = [&]()
}

#include <run_time/AttachA_CXX_struct.hpp>