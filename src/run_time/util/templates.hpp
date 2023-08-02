#ifndef SRC_RUN_TIME_UTIL_TEMPLATES
#define SRC_RUN_TIME_UTIL_TEMPLATES
#include <type_traits>//std::forward
#include "../attacha_abi.hpp"
namespace art{
    namespace templates{	
        template<typename ReturnTyp, typename ...Arguments>
        struct static_function_info {
            using return_type = ReturnTyp;
            inline static ValueMeta arguments[sizeof...(Arguments)? sizeof...(Arguments) : 1] = { Type_as_ValueMeta<Arguments>()... };
            using arguments_type = std::tuple<Arguments...>;
            constexpr static size_t arguments_count = sizeof...(Arguments);
            constexpr static bool is_static = true;
            constexpr static bool always_perfect = 
                sizeof...(Arguments) == 2
                && std::is_same_v<ReturnTyp, ValueItem*>
                && std::is_same_v<std::tuple<Arguments...>, std::tuple<ValueItem*, uint32_t>>;
        };
        

        template<typename Class_, typename ReturnTyp, typename ...Arguments>
        struct method_function_info {
            using return_type = ReturnTyp;
            inline static ValueMeta arguments[sizeof...(Arguments)? sizeof...(Arguments) : 1] = { Type_as_ValueMeta<Arguments>()... };
            using arguments_type = std::tuple<Arguments...>;
            using class_type = Class_;
            constexpr static size_t arguments_count = sizeof...(Arguments);
            constexpr static bool is_static = false;
            constexpr static bool always_perfect = 
                sizeof...(Arguments) == 2
                && std::is_same_v<ReturnTyp, ValueItem*>
                && std::is_same_v<std::tuple<Arguments...>, std::tuple<ValueItem*, uint32_t>>;
        };
        

        template<typename> struct function_info {};
        template<typename ReturnTyp, typename ...Arguments>
        struct function_info<ReturnTyp(Arguments...)> : static_function_info<ReturnTyp, Arguments...> {};
        template<typename ReturnTyp, typename ...Arguments>
        struct function_info<ReturnTyp(Arguments...) &>  : static_function_info<ReturnTyp, Arguments...> {};
        template<typename ReturnTyp, typename ...Arguments>
        struct function_info<ReturnTyp(Arguments...) &&>  : static_function_info<ReturnTyp, Arguments...> {};
        template<typename ReturnTyp, typename ...Arguments>
        struct function_info<ReturnTyp(Arguments...) const> : static_function_info<ReturnTyp, Arguments...> {};
        template<typename ReturnTyp, typename ...Arguments>
        struct function_info<ReturnTyp(Arguments...) const &> : static_function_info<ReturnTyp, Arguments...> {};
        template<typename ReturnTyp, typename ...Arguments>
        struct function_info<ReturnTyp(Arguments...) const &&> : static_function_info<ReturnTyp, Arguments...> {};
        template<typename ReturnTyp, typename ...Arguments>
        struct function_info<ReturnTyp(Arguments...) volatile> : static_function_info<ReturnTyp, Arguments...> {};
        template<typename ReturnTyp, typename ...Arguments>
        struct function_info<ReturnTyp(Arguments...) volatile &> : static_function_info<ReturnTyp, Arguments...> {};
        template<typename ReturnTyp, typename ...Arguments>
        struct function_info<ReturnTyp(Arguments...) volatile &&> : static_function_info<ReturnTyp, Arguments...> {};
        template<typename ReturnTyp, typename ...Arguments>
        struct function_info<ReturnTyp(Arguments...) const volatile> : static_function_info<ReturnTyp, Arguments...> {};
        template<typename ReturnTyp, typename ...Arguments>
        struct function_info<ReturnTyp(Arguments...) const volatile &> : static_function_info<ReturnTyp, Arguments...> {};
        template<typename ReturnTyp, typename ...Arguments>
        struct function_info<ReturnTyp(Arguments...) const volatile &&> : static_function_info<ReturnTyp, Arguments...> {};



        template<typename Class_, typename ReturnTyp, typename ...Arguments>
        struct function_info<ReturnTyp(Class_::*)(Arguments...)> : method_function_info<Class_, ReturnTyp, Arguments...> {};
        template<typename Class_, typename ReturnTyp, typename ...Arguments>
        struct function_info<ReturnTyp(Class_::*)(Arguments...) &> : method_function_info<Class_, ReturnTyp, Arguments...> {};
        template<typename Class_, typename ReturnTyp, typename ...Arguments>
        struct function_info<ReturnTyp(Class_::*)(Arguments...) &&> : method_function_info<Class_, ReturnTyp, Arguments...> {};
        template<typename Class_, typename ReturnTyp, typename ...Arguments>
        struct function_info<ReturnTyp(Class_::*)(Arguments...) const> : method_function_info<Class_, ReturnTyp, Arguments...> {};
        template<typename Class_, typename ReturnTyp, typename ...Arguments>
        struct function_info<ReturnTyp(Class_::*)(Arguments...) const &> : method_function_info<Class_, ReturnTyp, Arguments...> {};
        template<typename Class_, typename ReturnTyp, typename ...Arguments>
        struct function_info<ReturnTyp(Class_::*)(Arguments...) const &&> : method_function_info<Class_, ReturnTyp, Arguments...> {};
        template<typename Class_, typename ReturnTyp, typename ...Arguments>
        struct function_info<ReturnTyp(Class_::*)(Arguments...) volatile> : method_function_info<Class_, ReturnTyp, Arguments...> {};
        template<typename Class_, typename ReturnTyp, typename ...Arguments>
        struct function_info<ReturnTyp(Class_::*)(Arguments...) volatile &> : method_function_info<Class_, ReturnTyp, Arguments...> {};
        template<typename Class_, typename ReturnTyp, typename ...Arguments>
        struct function_info<ReturnTyp(Class_::*)(Arguments...) volatile &&> : method_function_info<Class_, ReturnTyp, Arguments...> {};
        template<typename Class_, typename ReturnTyp, typename ...Arguments>
        struct function_info<ReturnTyp(Class_::*)(Arguments...) const volatile> : method_function_info<Class_, ReturnTyp, Arguments...> {};
        template<typename Class_, typename ReturnTyp, typename ...Arguments>
        struct function_info<ReturnTyp(Class_::*)(Arguments...) const volatile &> : method_function_info<Class_, ReturnTyp, Arguments...> {};
        template<typename Class_, typename ReturnTyp, typename ...Arguments>
        struct function_info<ReturnTyp(Class_::*)(Arguments...) const volatile &&> : method_function_info<Class_, ReturnTyp, Arguments...> {};



        template<typename ReturnTyp>
        struct function_info<ReturnTyp()> : static_function_info<ReturnTyp> {};
        template<typename ReturnTyp>
        struct function_info<ReturnTyp() &>  : static_function_info<ReturnTyp> {};
        template<typename ReturnTyp>
        struct function_info<ReturnTyp() &&>  : static_function_info<ReturnTyp> {};
        template<typename ReturnTyp>
        struct function_info<ReturnTyp() const> : static_function_info<ReturnTyp> {};
        template<typename ReturnTyp>
        struct function_info<ReturnTyp() const &> : static_function_info<ReturnTyp> {};
        template<typename ReturnTyp>
        struct function_info<ReturnTyp() const &&> : static_function_info<ReturnTyp> {};
        template<typename ReturnTyp>
        struct function_info<ReturnTyp() volatile> : static_function_info<ReturnTyp> {};
        template<typename ReturnTyp>
        struct function_info<ReturnTyp() volatile &> : static_function_info<ReturnTyp> {};
        template<typename ReturnTyp>
        struct function_info<ReturnTyp() volatile &&> : static_function_info<ReturnTyp> {};
        template<typename ReturnTyp>
        struct function_info<ReturnTyp() const volatile> : static_function_info<ReturnTyp> {};
        template<typename ReturnTyp>
        struct function_info<ReturnTyp() const volatile &> : static_function_info<ReturnTyp> {};
        template<typename ReturnTyp>
        struct function_info<ReturnTyp() const volatile &&> : static_function_info<ReturnTyp> {};



        template<typename Class_, typename ReturnTyp>
        struct function_info<ReturnTyp(Class_::*)()> : method_function_info<Class_, ReturnTyp> {};
        template<typename Class_, typename ReturnTyp>
        struct function_info<ReturnTyp(Class_::*)() &> : method_function_info<Class_, ReturnTyp> {};
        template<typename Class_, typename ReturnTyp>
        struct function_info<ReturnTyp(Class_::*)() &&> : method_function_info<Class_, ReturnTyp> {};
        template<typename Class_, typename ReturnTyp>
        struct function_info<ReturnTyp(Class_::*)() const> : method_function_info<Class_, ReturnTyp> {};
        template<typename Class_, typename ReturnTyp>
        struct function_info<ReturnTyp(Class_::*)() const &> : method_function_info<Class_, ReturnTyp> {};
        template<typename Class_, typename ReturnTyp>
        struct function_info<ReturnTyp(Class_::*)() const &&> : method_function_info<Class_, ReturnTyp> {};
        template<typename Class_, typename ReturnTyp>
        struct function_info<ReturnTyp(Class_::*)() volatile> : method_function_info<Class_, ReturnTyp> {};
        template<typename Class_, typename ReturnTyp>
        struct function_info<ReturnTyp(Class_::*)() volatile &> : method_function_info<Class_, ReturnTyp> {};
        template<typename Class_, typename ReturnTyp>
        struct function_info<ReturnTyp(Class_::*)() volatile &&> : method_function_info<Class_, ReturnTyp> {};
        template<typename Class_, typename ReturnTyp>
        struct function_info<ReturnTyp(Class_::*)() const volatile> : method_function_info<Class_, ReturnTyp> {};
        template<typename Class_, typename ReturnTyp>
        struct function_info<ReturnTyp(Class_::*)() const volatile &> : method_function_info<Class_, ReturnTyp> {};
        template<typename Class_, typename ReturnTyp>
        struct function_info<ReturnTyp(Class_::*)() const volatile &&> : method_function_info<Class_, ReturnTyp> {};

        template<typename T>
        struct store_value {
            static T value;
            store_value(T v) : value(v) {};
        };
        template<typename T>
        T store_value<T>::value = nullptr;

		template<class T>
		T init_value(T func){
			return store_value<decltype(func)>::value = func;
		}
    }
}

#endif /* SRC_RUN_TIME_UTIL_TEMPLATES */
