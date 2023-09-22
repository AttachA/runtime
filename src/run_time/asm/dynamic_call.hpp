// Copyright Danyil Melnytskyi 2022-Present
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once
#include <library/list_array.hpp>
#include <tuple>

namespace art {
    namespace DynamicCall {
#ifdef _WIN32
        typedef size_t(__stdcall* PROC)();
#else
        typedef size_t (*PROC)();
#endif

        class ArgumentsHolder {
        public:
            struct ArgumentItem {
                void* ptr;
                void (*destructor)(void*);
                size_t value_size : sizeof(size_t) * 4 - 3;
                size_t value_len : sizeof(size_t) * 4;
                size_t need_delete : 1;
                size_t is_8bit : 1;

                explicit ArgumentItem(void* set_ptr = nullptr, bool need_remove = true, size_t set_value_size = 0, size_t set_value_len = 0, bool value_is_8bit = false, void (*destructor)(void*) = nullptr)
                    : destructor(destructor) {
                    ptr = set_ptr;
                    value_size = set_value_size;
                    value_len = set_value_len;
                    if constexpr (sizeof(uint64_t) != sizeof(void*))
                        is_8bit = value_is_8bit;
                    else
                        is_8bit = 0;
                    need_delete = need_remove;
                }

                ArgumentItem(ArgumentItem&& copy) {
                    *this = copy;
                }

                ArgumentItem(const ArgumentItem& copy) {
                    *this = copy;
                }

                ArgumentItem& operator=(const ArgumentItem& copy) {
                    ptr = copy.ptr;
                    value_size = copy.value_size;
                    value_len = copy.value_len;
                    need_delete = copy.need_delete;
                    is_8bit = copy.is_8bit;
                    destructor = copy.destructor;
                    return *this;
                }

                ArgumentItem& operator=(ArgumentItem&& copy) {
                    return operator=(copy);
                }
            };

        private:
            list_array<DynamicCall::ArgumentsHolder::ArgumentItem> arguments;

            template <class T>
            static void arrayDestructor(void* arr) {
                delete[] (T*)arr;
            }

            template <class T>
            static void valueDestructor(void* arr) {
                delete (T*)arr;
            }

        public:
            ArgumentsHolder() {}

            ~ArgumentsHolder() {
                clear();
            }

            template <class T>
            void push_value(const T& value) {
                if constexpr (sizeof(uint64_t) != sizeof(void*) && sizeof(uint64_t) == sizeof(T))
                    arguments.push_back(ArgumentItem(new T(value), true, sizeof(T), true, false, valueDestructor<T>));
                else if constexpr (sizeof(T) > sizeof(void*))
                    arguments.push_back(ArgumentItem(new T(value), true, sizeof(T), false, valueDestructor<T>));
                else
                    arguments.push_back(ArgumentItem(*(void**)&value, false, sizeof(T)));
            }

            template <class T>
            void push_ptr(T* value) {
                if constexpr (std::is_same<T, void>::value)
                    arguments.push_back(ArgumentItem((void*)value, 0, false, 0));
                else
                    arguments.push_back(ArgumentItem((void*)value, 0, false, sizeof(T)));
            }

            //for 32x (in x64 same as push ptr)
            template <class T>
            void push_link(T& value) {
                if constexpr (sizeof(uint64_t) == sizeof(void*))
                    arguments.push_back(ArgumentItem(*(size_t*)(void*)&value, 0, false, sizeof(T)));
                else {
                    void* val = &value;
                    arguments.push_back(ArgumentItem(*(size_t*)(void*)&val, 0, false, sizeof(T)));
                }
            }

            void push_struct(const char* value, size_t siz) {
                char* arg = new char[siz];
                for (size_t i = 0; i < siz; i++)
                    arg[i] = value[i];
                arguments.push_back(ArgumentItem((void*)arg, true, siz, 0, false, arrayDestructor<char>));
            }

            template <class T>
            void push_array(const T* value, size_t siz) {
                static_assert(std::is_destructible<T>::value, "This value will not properly destruct");
                T* arg = new T[siz];
                for (size_t i = 0; i < siz; i++)
                    arg[i] = value[i];
                arguments.push_back(ArgumentItem((void*)arg, true, sizeof(T), siz, false, arrayDestructor<T>));
            }

            void clear() {
                for (ArgumentItem& a : arguments) {
                    if (a.need_delete) {
                        a.destructor((void*)a.ptr);
                        a.ptr = nullptr;
                    }
                }
                arguments.clear();
            }

            list_array<ArgumentItem>& GetArguments() {
                return arguments;
            }

            //list_array<ArgumentItem> GetArguments() {
            //	return arguments;
            //}

            auto begin() {
                return arguments.begin();
            }

            auto end() {
                return arguments.end();
            }

            void reserveArguments(size_t reserve) {
                arguments.reserve_push_back(reserve);
            }
        };

        namespace Calls {
            uint64_t call(PROC proc, ArgumentsHolder& ah, bool used_this = false, void* this_pointer = nullptr);
            void callNR(PROC proc, ArgumentsHolder& ah, bool used_this = false, void* this_pointer = nullptr);
            void* callPTR(PROC proc, ArgumentsHolder& ah, bool used_this = false, void* this_pointer = nullptr);
        }

        struct FunctionTemplate {
            struct ValueT {
                enum class PlaceType : size_t {
                    as_ptr,
                    as_value
                };
                enum class ValueType : size_t {
                    integer,
                    signed_integer,
                    floating,
                    pointer,
                    _class
                };
                PlaceType ptype : 1;
                ValueType vtype : 3;
                size_t is_modifiable : 1;
                size_t vsize : sizeof(size_t) * 8 - 5;

                template <class T>
                static ValueT getFromType() {
                    ValueT val;
                    if constexpr (std::is_same_v<T, void>) {
                        val.ptype = PlaceType::as_ptr;
                        val.is_modifiable = false;
                        val.vsize = 0;
                        val.vtype = ValueType::integer;
                    } else {
                        val.ptype = std::is_pointer_v<T> ? PlaceType::as_ptr : PlaceType::as_value;
                        val.is_modifiable = !std::is_const_v<T>;
                        val.vsize = sizeof(std::remove_pointer_t<T>);
                        if constexpr (std::is_unsigned_v<std::remove_pointer_t<T>>)
                            val.vtype = ValueType::integer;
                        else if constexpr (std::is_signed_v<std::remove_pointer_t<T>>)
                            val.vtype = ValueType::signed_integer;
                        else if constexpr (std::is_floating_point_v<std::remove_pointer_t<T>>)
                            val.vtype = ValueType::floating;
                        else if constexpr (std::is_class_v<std::remove_pointer_t<T>>)
                            val.vtype = ValueType::_class;
                        else
                            val.vtype = ValueType::pointer;
                    }
                    return val;
                }

                bool is_void() const {
                    return ptype == PlaceType::as_ptr && vtype == ValueType::integer && !is_modifiable && !vsize;
                }

                ValueT() {}

                ValueT(PlaceType ptype, ValueType vtype, size_t is_modifiable, size_t vsize)
                    : ptype(ptype), vtype(vtype), is_modifiable(is_modifiable), vsize(vsize) {}

                ValueT(const ValueT& other) {
                    *this = other;
                }

                ValueT& operator=(const ValueT& other) {
                    ptype = other.ptype;
                    vtype = other.vtype;
                    is_modifiable = other.is_modifiable;
                    vsize = other.vsize;
                    return *this;
                }
            };

            list_array<ValueT> arguments;
            ValueT result{ValueT::PlaceType::as_ptr, ValueT::ValueType::integer, 0, 0};
            bool is_variadic = false;
        };

        class FunctionCall {
            ArgumentsHolder holder;
            const FunctionTemplate& templ;
            size_t current_argument;
            size_t max_arguments;
            PROC func;
            void* this_pointer;
            bool atc : 1;
            bool used_this : 1;

        public:
            template <class F>
            FunctionCall(F to_call_func, const FunctionTemplate& func_template, bool allow_type_convert = false)
                : templ(func_template) {
                used_this = false;
                this_pointer = nullptr;
                func = (PROC)to_call_func;
                atc = allow_type_convert;
                holder.reserveArguments(max_arguments = templ.arguments.size());
                current_argument = 0;
            }

            FunctionCall(PROC to_call_func, void* this_ptr, const FunctionTemplate& func_template, bool allow_type_convert = false)
                : templ(func_template) {
                used_this = true;
                this_pointer = this_ptr;
                func = (PROC)to_call_func;
                atc = allow_type_convert;
                holder.reserveArguments(max_arguments = templ.arguments.size());
                current_argument = 0;
            }

            template <class T>
            void AddValueArgument(const T& value) {
                FunctionTemplate::ValueT tmp;
                if (max_arguments <= current_argument) {
                    if (templ.is_variadic)
                        tmp = FunctionTemplate::ValueT::getFromType<void>();
                    else
                        throw std::out_of_range("Out of arguments range");
                } else
                    tmp = templ.arguments[current_argument];
                if (tmp.ptype == FunctionTemplate::ValueT::PlaceType::as_value) {
                    if (tmp.vsize == sizeof(T)) {
                        holder.push_value(value);
                        current_argument++;
                        return;
                    } else if (atc) {
                        switch (tmp.vsize) {
                        case 1:
                            holder.push_value(*(uint8_t*)&value);
                            current_argument++;
                            break;
                        case 2:
                            holder.push_value(*(uint16_t*)&value);
                            current_argument++;
                            break;
                        case 4:
                            holder.push_value(*(uint32_t*)&value);
                            current_argument++;
                            break;
                        case 8:
                            holder.push_value(*(uint64_t*)&value);
                            current_argument++;
                            break;
                        default:
                            throw std::logic_error("Fail push invalid size value, and fail convert it");
                        }
                        return;
                    }
                } else {
                    holder.push_value((size_t)value);
                    current_argument++;
                    return;
                }
                throw std::logic_error("Fail push invalid type argument or value size mismatch(for avoid this enable allow_type_convert)");
            }

            template <class T>
            void AddLinkArgument(T& value) {
                if (max_arguments <= current_argument) {
                    if (templ.is_variadic) {
                        holder.push_ptr(value);
                        current_argument++;
                        return;
                    } else
                        throw std::out_of_range("Out of arguments range");
                }
                auto& tmp = templ.arguments[current_argument];
                if (tmp.ptype == FunctionTemplate::ValueT::PlaceType::as_ptr) {
                    holder.push_link(value);
                    current_argument++;
                } else
                    throw std::logic_error("Fail push invalid type argument");
            }

            template <class T>
            void AddPtrArgument(T* value) {
                if (max_arguments <= current_argument) {
                    if (templ.is_variadic) {
                        holder.push_ptr(value);
                        current_argument++;
                        return;
                    } else
                        throw std::out_of_range("Out of arguments range");
                }
                auto& tmp = templ.arguments[current_argument];
                if (tmp.ptype == FunctionTemplate::ValueT::PlaceType::as_ptr) {
                    holder.push_ptr(value);
                    current_argument++;
                } else
                    throw std::logic_error("Fail push invalid type argument");
            }

            template <class T>
            void AddArray(const T* arr, size_t arr_len) {
                if (max_arguments <= current_argument) {
                    if (templ.is_variadic) {
                        holder.push_array(arr, arr_len);
                        current_argument++;
                        return;
                    } else
                        throw std::out_of_range("Out of arguments range");
                }
                if (max_arguments <= current_argument)
                    throw std::out_of_range("Out of arguments range");
                auto& tmp = templ.arguments[current_argument];
                if (tmp.ptype == FunctionTemplate::ValueT::PlaceType::as_ptr) {
                    holder.push_array(arr, arr_len);
                    current_argument++;
                } else
                    throw std::logic_error("Fail push invalid type argument");
            }

            void* Call() {
                if (max_arguments > current_argument || (!templ.is_variadic && max_arguments < current_argument))
                    throw std::logic_error("Invalid arguments count");
                return (void*)Calls::call(func, holder, used_this, this_pointer);
            }

            template <class T>
            T CallValue() {
                if (templ.result.vsize == sizeof(T))
                    return (T)Calls::call(func, holder, used_this, this_pointer);
                throw std::logic_error("Fail invalid result size");
            }

            template <class T>
            T* CallPtr() {
                if (templ.result.ptype == FunctionTemplate::ValueT::PlaceType::as_ptr)
                    return (T*)Calls::call(func, holder, used_this, this_pointer);
                throw std::logic_error("Fail invalid result type");
            }

            FunctionTemplate::ValueT ToAddArgument() const {
                if (current_argument >= max_arguments)
                    return FunctionTemplate::ValueT(FunctionTemplate::ValueT::PlaceType::as_ptr, FunctionTemplate::ValueT::ValueType::integer, 0, 0);
                return templ.arguments[current_argument];
            }

            bool is_variadic() const {
                return templ.is_variadic;
            }
        };

        template <size_t i, class T, class... Args>
        struct ArgumentsBuild {
            ArgumentsBuild(DynamicCall::FunctionTemplate& templ)
                : t(templ) {
                templ.arguments.push_front(DynamicCall::FunctionTemplate::ValueT::getFromType<T>());
            }

            ArgumentsBuild<i - 1, Args...> t;
        };

        template <class T, class... Args>
        struct ArgumentsBuild<1, T, Args...> {
            ArgumentsBuild(DynamicCall::FunctionTemplate& templ) {
                templ.arguments.push_front(DynamicCall::FunctionTemplate::ValueT::getFromType<T>());
            }
        };

        template <class Ret, class... Args>
        struct StartBuildTemplate {
            ArgumentsBuild<sizeof...(Args), Args...> res;

            StartBuildTemplate(Ret (*function)(Args...), DynamicCall::FunctionTemplate& templ)
                : res(templ) {
                templ.result = DynamicCall::FunctionTemplate::ValueT::getFromType<Ret>();
            }

            StartBuildTemplate(DynamicCall::FunctionTemplate& templ)
                : res(templ) {
                templ.result = DynamicCall::FunctionTemplate::ValueT::getFromType<Ret>();
            }
        };

        template <class Class_, class Ret, class... Args>
        struct StartBuildType : StartBuildTemplate<Ret, Args...> {
            StartBuildType(DynamicCall::FunctionTemplate& templ)
                : StartBuildTemplate<Ret, Args...>(templ) {}
        };

        template <class Class_, class Ret, class... Args>
        void buildFn(Ret (Class_::*)(Args...), DynamicCall::FunctionTemplate& templ) {
            StartBuildTemplate<Ret, Args...> unused(templ);
        }

        template <class Class_, class Ret, class... Args>
        void buildFn(Ret (Class_::*)(Args...) const, DynamicCall::FunctionTemplate& templ) {
            StartBuildTemplate<Ret, Args...> unused(templ);
        }

        template <class Class_, class Ret, class... Args>
        void buildFn(Ret (Class_::*)(Args...) volatile, DynamicCall::FunctionTemplate& templ) {
            StartBuildTemplate<Ret, Args...> unused(templ);
        }

        template <class Class_, class Ret, class... Args>
        void buildFn(Ret (Class_::*)(Args...) const volatile, DynamicCall::FunctionTemplate& templ) {
            StartBuildTemplate<Ret, Args...> unused(templ);
        }

        template <class Class_, class Ret, class... Args>
        void buildFn(Ret (Class_::*)(Args...) &, DynamicCall::FunctionTemplate& templ) {
            StartBuildTemplate<Ret, Args...> unused(templ);
        }

        template <class Class_, class Ret, class... Args>
        void buildFn(Ret (Class_::*)(Args...) const&, DynamicCall::FunctionTemplate& templ) {
            StartBuildTemplate<Ret, Args...> unused(templ);
        }

        template <class Class_, class Ret, class... Args>
        void buildFn(Ret (Class_::*)(Args...) volatile&, DynamicCall::FunctionTemplate& templ) {
            StartBuildTemplate<Ret, Args...> unused(templ);
        }

        template <class Class_, class Ret, class... Args>
        void buildFn(Ret (Class_::*)(Args...) const volatile&, DynamicCall::FunctionTemplate& templ) {
            StartBuildTemplate<Ret, Args...> unused(templ);
        }

        template <class Class_, class Ret, class... Args>
        void buildFn(Ret (Class_::*)(Args...) &&, DynamicCall::FunctionTemplate& templ) {
            StartBuildTemplate<Ret, Args...> unused(templ);
        }

        template <class Class_, class Ret, class... Args>
        void buildFn(Ret (Class_::*)(Args...) const&&, DynamicCall::FunctionTemplate& templ) {
            StartBuildTemplate<Ret, Args...> unused(templ);
        }

        template <class Class_, class Ret, class... Args>
        void buildFn(Ret (Class_::*)(Args...) volatile&&, DynamicCall::FunctionTemplate& templ) {
            StartBuildTemplate<Ret, Args...> unused(templ);
        }

        template <class Class_, class Ret, class... Args>
        void buildFn(Ret (Class_::*)(Args...) const volatile&&, DynamicCall::FunctionTemplate& templ) {
            StartBuildTemplate<Ret, Args...> unused(templ);
        }

        template <class Ret, class... Args>
        void buildFn(Ret (*)(Args...), DynamicCall::FunctionTemplate& templ) {
            StartBuildTemplate<Ret, Args...> unused(templ);
        }

        template <class Class_, class Ret>
        void buildFn(Ret (Class_::*)(), DynamicCall::FunctionTemplate& templ) {
            templ.result = DynamicCall::FunctionTemplate::ValueT::getFromType<Ret>();
        }

        template <class Class_, class Ret>
        void buildFn(Ret (Class_::*)() const, DynamicCall::FunctionTemplate& templ) {
            templ.result = DynamicCall::FunctionTemplate::ValueT::getFromType<Ret>();
        }

        template <class Class_, class Ret>
        void buildFn(Ret (Class_::*)() volatile, DynamicCall::FunctionTemplate& templ) {
            templ.result = DynamicCall::FunctionTemplate::ValueT::getFromType<Ret>();
        }

        template <class Class_, class Ret>
        void buildFn(Ret (Class_::*)() const volatile, DynamicCall::FunctionTemplate& templ) {
            templ.result = DynamicCall::FunctionTemplate::ValueT::getFromType<Ret>();
        }

        template <class Class_, class Ret>
        void buildFn(Ret (Class_::*)() &, DynamicCall::FunctionTemplate& templ) {
            templ.result = DynamicCall::FunctionTemplate::ValueT::getFromType<Ret>();
        }

        template <class Class_, class Ret>
        void buildFn(Ret (Class_::*)() const&, DynamicCall::FunctionTemplate& templ) {
            templ.result = DynamicCall::FunctionTemplate::ValueT::getFromType<Ret>();
        }

        template <class Class_, class Ret>
        void buildFn(Ret (Class_::*)() volatile&, DynamicCall::FunctionTemplate& templ) {
            templ.result = DynamicCall::FunctionTemplate::ValueT::getFromType<Ret>();
        }

        template <class Class_, class Ret>
        void buildFn(Ret (Class_::*)() const volatile&, DynamicCall::FunctionTemplate& templ) {
            templ.result = DynamicCall::FunctionTemplate::ValueT::getFromType<Ret>();
        }

        template <class Class_, class Ret>
        void buildFn(Ret (Class_::*)() &&, DynamicCall::FunctionTemplate& templ) {
            templ.result = DynamicCall::FunctionTemplate::ValueT::getFromType<Ret>();
        }

        template <class Class_, class Ret>
        void buildFn(Ret (Class_::*)() const&&, DynamicCall::FunctionTemplate& templ) {
            templ.result = DynamicCall::FunctionTemplate::ValueT::getFromType<Ret>();
        }

        template <class Class_, class Ret>
        void buildFn(Ret (Class_::*)() volatile&&, DynamicCall::FunctionTemplate& templ) {
            templ.result = DynamicCall::FunctionTemplate::ValueT::getFromType<Ret>();
        }

        template <class Class_, class Ret>
        void buildFn(Ret (Class_::*)() const volatile&&, DynamicCall::FunctionTemplate& templ) {
            templ.result = DynamicCall::FunctionTemplate::ValueT::getFromType<Ret>();
        }

        template <class Ret>
        void buildFn(Ret (*)(), DynamicCall::FunctionTemplate& templ) {
            templ.result = DynamicCall::FunctionTemplate::ValueT::getFromType<Ret>();
        }

        extern "C" [[noreturn]] void justJump(void* point);
    }
}