// Copyright Danyil Melnytskyi 2022-Present
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef SRC_RUN_TIME_UTIL_TEMPLATES
#define SRC_RUN_TIME_UTIL_TEMPLATES
#include <run_time/attacha_abi.hpp>
#include <type_traits> //std::forward

namespace art {
    namespace templates {
        template <typename ReturnTyp, typename... Arguments>
        struct static_function_info {
            using return_type = ReturnTyp;
            inline static ValueMeta arguments[sizeof...(Arguments) ? sizeof...(Arguments) : 1] = {Type_as_ValueMeta<Arguments>()...};
            using arguments_type = std::tuple<Arguments...>;
            constexpr static size_t arguments_count = sizeof...(Arguments);
            constexpr static bool is_static = true;
            constexpr static bool always_perfect =
                sizeof...(Arguments) == 2 && std::is_same_v<ReturnTyp, ValueItem*> && std::is_same_v<std::tuple<Arguments...>, std::tuple<ValueItem*, uint32_t>>;
        };

        template <typename Class_, typename ReturnTyp, typename... Arguments>
        struct method_function_info {
            using return_type = ReturnTyp;
            inline static ValueMeta arguments[sizeof...(Arguments) ? sizeof...(Arguments) : 1] = {Type_as_ValueMeta<Arguments>()...};
            using arguments_type = std::tuple<Arguments...>;
            using class_type = Class_;
            constexpr static size_t arguments_count = sizeof...(Arguments);
            constexpr static bool is_static = false;
            constexpr static bool always_perfect =
                sizeof...(Arguments) == 2 && std::is_same_v<ReturnTyp, ValueItem*> && std::is_same_v<std::tuple<Arguments...>, std::tuple<ValueItem*, uint32_t>>;
        };

        template <typename T>
        struct function_info : public function_info<decltype(&T::operator())> {
            bool is_lambda = true;
        };

        template <typename ReturnTyp, typename... Arguments>
        struct function_info<ReturnTyp(Arguments...)> : static_function_info<ReturnTyp, Arguments...> {};

        template <typename ReturnTyp, typename... Arguments>
        struct function_info<ReturnTyp(Arguments...)&> : static_function_info<ReturnTyp, Arguments...> {};

        template <typename ReturnTyp, typename... Arguments>
        struct function_info<ReturnTyp(Arguments...) &&> : static_function_info<ReturnTyp, Arguments...> {};

        template <typename ReturnTyp, typename... Arguments>
        struct function_info<ReturnTyp(Arguments...) const> : static_function_info<ReturnTyp, Arguments...> {};

        template <typename ReturnTyp, typename... Arguments>
        struct function_info<ReturnTyp(Arguments...) const&> : static_function_info<ReturnTyp, Arguments...> {};

        template <typename ReturnTyp, typename... Arguments>
        struct function_info<ReturnTyp(Arguments...) const&&> : static_function_info<ReturnTyp, Arguments...> {};

        template <typename ReturnTyp, typename... Arguments>
        struct function_info<ReturnTyp(Arguments...) volatile> : static_function_info<ReturnTyp, Arguments...> {};

        template <typename ReturnTyp, typename... Arguments>
        struct function_info<ReturnTyp(Arguments...) volatile&> : static_function_info<ReturnTyp, Arguments...> {};

        template <typename ReturnTyp, typename... Arguments>
        struct function_info<ReturnTyp(Arguments...) volatile&&> : static_function_info<ReturnTyp, Arguments...> {};

        template <typename ReturnTyp, typename... Arguments>
        struct function_info<ReturnTyp(Arguments...) const volatile> : static_function_info<ReturnTyp, Arguments...> {};

        template <typename ReturnTyp, typename... Arguments>
        struct function_info<ReturnTyp(Arguments...) const volatile&> : static_function_info<ReturnTyp, Arguments...> {};

        template <typename ReturnTyp, typename... Arguments>
        struct function_info<ReturnTyp(Arguments...) const volatile&&> : static_function_info<ReturnTyp, Arguments...> {};

        template <typename Class_, typename ReturnTyp, typename... Arguments>
        struct function_info<ReturnTyp (Class_::*)(Arguments...)> : method_function_info<Class_, ReturnTyp, Arguments...> {};

        template <typename Class_, typename ReturnTyp, typename... Arguments>
        struct function_info<ReturnTyp (Class_::*)(Arguments...)&> : method_function_info<Class_, ReturnTyp, Arguments...> {};

        template <typename Class_, typename ReturnTyp, typename... Arguments>
        struct function_info<ReturnTyp (Class_::*)(Arguments...) &&> : method_function_info<Class_, ReturnTyp, Arguments...> {};

        template <typename Class_, typename ReturnTyp, typename... Arguments>
        struct function_info<ReturnTyp (Class_::*)(Arguments...) const> : method_function_info<Class_, ReturnTyp, Arguments...> {};

        template <typename Class_, typename ReturnTyp, typename... Arguments>
        struct function_info<ReturnTyp (Class_::*)(Arguments...) const&> : method_function_info<Class_, ReturnTyp, Arguments...> {};

        template <typename Class_, typename ReturnTyp, typename... Arguments>
        struct function_info<ReturnTyp (Class_::*)(Arguments...) const&&> : method_function_info<Class_, ReturnTyp, Arguments...> {};

        template <typename Class_, typename ReturnTyp, typename... Arguments>
        struct function_info<ReturnTyp (Class_::*)(Arguments...) volatile> : method_function_info<Class_, ReturnTyp, Arguments...> {};

        template <typename Class_, typename ReturnTyp, typename... Arguments>
        struct function_info<ReturnTyp (Class_::*)(Arguments...) volatile&> : method_function_info<Class_, ReturnTyp, Arguments...> {};

        template <typename Class_, typename ReturnTyp, typename... Arguments>
        struct function_info<ReturnTyp (Class_::*)(Arguments...) volatile&&> : method_function_info<Class_, ReturnTyp, Arguments...> {};

        template <typename Class_, typename ReturnTyp, typename... Arguments>
        struct function_info<ReturnTyp (Class_::*)(Arguments...) const volatile> : method_function_info<Class_, ReturnTyp, Arguments...> {};

        template <typename Class_, typename ReturnTyp, typename... Arguments>
        struct function_info<ReturnTyp (Class_::*)(Arguments...) const volatile&> : method_function_info<Class_, ReturnTyp, Arguments...> {};

        template <typename Class_, typename ReturnTyp, typename... Arguments>
        struct function_info<ReturnTyp (Class_::*)(Arguments...) const volatile&&> : method_function_info<Class_, ReturnTyp, Arguments...> {};

        template <typename ReturnTyp>
        struct function_info<ReturnTyp()> : static_function_info<ReturnTyp> {};

        template <typename ReturnTyp>
        struct function_info<ReturnTyp()&> : static_function_info<ReturnTyp> {};

        template <typename ReturnTyp>
        struct function_info<ReturnTyp() &&> : static_function_info<ReturnTyp> {};

        template <typename ReturnTyp>
        struct function_info<ReturnTyp() const> : static_function_info<ReturnTyp> {};

        template <typename ReturnTyp>
        struct function_info<ReturnTyp() const&> : static_function_info<ReturnTyp> {};

        template <typename ReturnTyp>
        struct function_info<ReturnTyp() const&&> : static_function_info<ReturnTyp> {};

        template <typename ReturnTyp>
        struct function_info<ReturnTyp() volatile> : static_function_info<ReturnTyp> {};

        template <typename ReturnTyp>
        struct function_info<ReturnTyp() volatile&> : static_function_info<ReturnTyp> {};

        template <typename ReturnTyp>
        struct function_info<ReturnTyp() volatile&&> : static_function_info<ReturnTyp> {};

        template <typename ReturnTyp>
        struct function_info<ReturnTyp() const volatile> : static_function_info<ReturnTyp> {};

        template <typename ReturnTyp>
        struct function_info<ReturnTyp() const volatile&> : static_function_info<ReturnTyp> {};

        template <typename ReturnTyp>
        struct function_info<ReturnTyp() const volatile&&> : static_function_info<ReturnTyp> {};

        template <typename Class_, typename ReturnTyp>
        struct function_info<ReturnTyp (Class_::*)()> : method_function_info<Class_, ReturnTyp> {};

        template <typename Class_, typename ReturnTyp>
        struct function_info<ReturnTyp (Class_::*)()&> : method_function_info<Class_, ReturnTyp> {};

        template <typename Class_, typename ReturnTyp>
        struct function_info<ReturnTyp (Class_::*)() &&> : method_function_info<Class_, ReturnTyp> {};

        template <typename Class_, typename ReturnTyp>
        struct function_info<ReturnTyp (Class_::*)() const> : method_function_info<Class_, ReturnTyp> {};

        template <typename Class_, typename ReturnTyp>
        struct function_info<ReturnTyp (Class_::*)() const&> : method_function_info<Class_, ReturnTyp> {};

        template <typename Class_, typename ReturnTyp>
        struct function_info<ReturnTyp (Class_::*)() const&&> : method_function_info<Class_, ReturnTyp> {};

        template <typename Class_, typename ReturnTyp>
        struct function_info<ReturnTyp (Class_::*)() volatile> : method_function_info<Class_, ReturnTyp> {};

        template <typename Class_, typename ReturnTyp>
        struct function_info<ReturnTyp (Class_::*)() volatile&> : method_function_info<Class_, ReturnTyp> {};

        template <typename Class_, typename ReturnTyp>
        struct function_info<ReturnTyp (Class_::*)() volatile&&> : method_function_info<Class_, ReturnTyp> {};

        template <typename Class_, typename ReturnTyp>
        struct function_info<ReturnTyp (Class_::*)() const volatile> : method_function_info<Class_, ReturnTyp> {};

        template <typename Class_, typename ReturnTyp>
        struct function_info<ReturnTyp (Class_::*)() const volatile&> : method_function_info<Class_, ReturnTyp> {};

        template <typename Class_, typename ReturnTyp>
        struct function_info<ReturnTyp (Class_::*)() const volatile&&> : method_function_info<Class_, ReturnTyp> {};

        template <typename T>
        class is_lambda {
            typedef char yes[1];
            typedef char no[2];

            template <typename C>
            static yes& test(decltype(&C::operator())*);

            template <typename>
            static no& test(...);

        public:
            constexpr static bool value = sizeof(test<T>(0)) == sizeof(yes);
        };

        template <typename T>
        constexpr bool is_lambda_v = is_lambda<T>::value;

        template <typename T, typename U>
        struct is_simple_lambda_helper : is_simple_lambda_helper<T, decltype(&U::operator())> {};

        template <typename T, typename C, typename R, typename... A>
        struct is_simple_lambda_helper<T, R (C::*)(A...) const> {
            constexpr static bool value = std::is_convertible<T, R (*)(A...)>::value;
            using cast = R (*)(A...);
            using ref = R (C::*)(A...);
        };

        template <typename T>
        struct is_simple_lambda {
            static const constexpr bool value = is_simple_lambda_helper<T, T>::value;
            using cast = is_simple_lambda_helper<T, T>::cast;
        };

        template <typename T>
        constexpr bool is_simple_lambda_v = is_simple_lambda<T>::value;

        template <typename T>
        using fn_lambda = is_simple_lambda<T>::cast;

        template <typename T>
        using ref_lambda = is_simple_lambda<T>::ref;

        template <typename T>
        struct store_value {
            static T value;

            store_value(T v) {
                value == std::move(v);
            };
        };

        template <typename T>
        T store_value<T>::value = nullptr;

        template <class T>
        T init_value(T func) {
            return store_value<decltype(func)>::value = func;
        }
    }
}

#endif /* SRC_RUN_TIME_UTIL_TEMPLATES */
