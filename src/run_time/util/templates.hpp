#ifndef SRC_RUN_TIME_UTIL_TEMPLATES
#define SRC_RUN_TIME_UTIL_TEMPLATES
#include <type_traits>//std::forward
#include "../attacha_abi.hpp"
namespace art{
    namespace templates{	
        template<typename ReturmTyp, typename ...Argumetns>
        struct static_funtion_info {
            using return_type = ReturmTyp;
            inline static ValueMeta arguments[sizeof...(Argumetns)? sizeof...(Argumetns) : 1] = { Type_as_ValueMeta<Argumetns>()... };
            using arguments_type = std::tuple<Argumetns...>;
            constexpr static size_t arguments_count = sizeof...(Argumetns);
            constexpr static bool is_static = true;
            constexpr static bool always_perfect = 
                sizeof...(Argumetns) == 2
                && std::is_same_v<ReturmTyp, ValueItem*>
                && std::is_same_v<std::tuple<Argumetns...>, std::tuple<ValueItem*, uint32_t>>;
        };
        

        template<typename Class_, typename ReturmTyp, typename ...Argumetns>
        struct method_funtion_info {
            using return_type = ReturmTyp;
            inline static ValueMeta arguments[sizeof...(Argumetns)? sizeof...(Argumetns) : 1] = { Type_as_ValueMeta<Argumetns>()... };
            using arguments_type = std::tuple<Argumetns...>;
            using class_type = Class_;
            constexpr static size_t arguments_count = sizeof...(Argumetns);
            constexpr static bool is_static = false;
            constexpr static bool always_perfect = 
                sizeof...(Argumetns) == 2
                && std::is_same_v<ReturmTyp, ValueItem*>
                && std::is_same_v<std::tuple<Argumetns...>, std::tuple<ValueItem*, uint32_t>>;
        };
        

        template<typename> struct funtion_info {};
        template<typename ReturmTyp, typename ...Argumetns>
        struct funtion_info<ReturmTyp(Argumetns...)> : static_funtion_info<ReturmTyp, Argumetns...> {};
        template<typename ReturmTyp, typename ...Argumetns>
        struct funtion_info<ReturmTyp(Argumetns...) &>  : static_funtion_info<ReturmTyp, Argumetns...> {};
        template<typename ReturmTyp, typename ...Argumetns>
        struct funtion_info<ReturmTyp(Argumetns...) &&>  : static_funtion_info<ReturmTyp, Argumetns...> {};
        template<typename ReturmTyp, typename ...Argumetns>
        struct funtion_info<ReturmTyp(Argumetns...) const> : static_funtion_info<ReturmTyp, Argumetns...> {};
        template<typename ReturmTyp, typename ...Argumetns>
        struct funtion_info<ReturmTyp(Argumetns...) const &> : static_funtion_info<ReturmTyp, Argumetns...> {};
        template<typename ReturmTyp, typename ...Argumetns>
        struct funtion_info<ReturmTyp(Argumetns...) const &&> : static_funtion_info<ReturmTyp, Argumetns...> {};
        template<typename ReturmTyp, typename ...Argumetns>
        struct funtion_info<ReturmTyp(Argumetns...) volatile> : static_funtion_info<ReturmTyp, Argumetns...> {};
        template<typename ReturmTyp, typename ...Argumetns>
        struct funtion_info<ReturmTyp(Argumetns...) volatile &> : static_funtion_info<ReturmTyp, Argumetns...> {};
        template<typename ReturmTyp, typename ...Argumetns>
        struct funtion_info<ReturmTyp(Argumetns...) volatile &&> : static_funtion_info<ReturmTyp, Argumetns...> {};
        template<typename ReturmTyp, typename ...Argumetns>
        struct funtion_info<ReturmTyp(Argumetns...) const volatile> : static_funtion_info<ReturmTyp, Argumetns...> {};
        template<typename ReturmTyp, typename ...Argumetns>
        struct funtion_info<ReturmTyp(Argumetns...) const volatile &> : static_funtion_info<ReturmTyp, Argumetns...> {};
        template<typename ReturmTyp, typename ...Argumetns>
        struct funtion_info<ReturmTyp(Argumetns...) const volatile &&> : static_funtion_info<ReturmTyp, Argumetns...> {};



        template<typename Class_, typename ReturmTyp, typename ...Argumetns>
        struct funtion_info<ReturmTyp(Class_::*)(Argumetns...)> : method_funtion_info<Class_, ReturmTyp, Argumetns...> {};
        template<typename Class_, typename ReturmTyp, typename ...Argumetns>
        struct funtion_info<ReturmTyp(Class_::*)(Argumetns...) &> : method_funtion_info<Class_, ReturmTyp, Argumetns...> {};
        template<typename Class_, typename ReturmTyp, typename ...Argumetns>
        struct funtion_info<ReturmTyp(Class_::*)(Argumetns...) &&> : method_funtion_info<Class_, ReturmTyp, Argumetns...> {};
        template<typename Class_, typename ReturmTyp, typename ...Argumetns>
        struct funtion_info<ReturmTyp(Class_::*)(Argumetns...) const> : method_funtion_info<Class_, ReturmTyp, Argumetns...> {};
        template<typename Class_, typename ReturmTyp, typename ...Argumetns>
        struct funtion_info<ReturmTyp(Class_::*)(Argumetns...) const &> : method_funtion_info<Class_, ReturmTyp, Argumetns...> {};
        template<typename Class_, typename ReturmTyp, typename ...Argumetns>
        struct funtion_info<ReturmTyp(Class_::*)(Argumetns...) const &&> : method_funtion_info<Class_, ReturmTyp, Argumetns...> {};
        template<typename Class_, typename ReturmTyp, typename ...Argumetns>
        struct funtion_info<ReturmTyp(Class_::*)(Argumetns...) volatile> : method_funtion_info<Class_, ReturmTyp, Argumetns...> {};
        template<typename Class_, typename ReturmTyp, typename ...Argumetns>
        struct funtion_info<ReturmTyp(Class_::*)(Argumetns...) volatile &> : method_funtion_info<Class_, ReturmTyp, Argumetns...> {};
        template<typename Class_, typename ReturmTyp, typename ...Argumetns>
        struct funtion_info<ReturmTyp(Class_::*)(Argumetns...) volatile &&> : method_funtion_info<Class_, ReturmTyp, Argumetns...> {};
        template<typename Class_, typename ReturmTyp, typename ...Argumetns>
        struct funtion_info<ReturmTyp(Class_::*)(Argumetns...) const volatile> : method_funtion_info<Class_, ReturmTyp, Argumetns...> {};
        template<typename Class_, typename ReturmTyp, typename ...Argumetns>
        struct funtion_info<ReturmTyp(Class_::*)(Argumetns...) const volatile &> : method_funtion_info<Class_, ReturmTyp, Argumetns...> {};
        template<typename Class_, typename ReturmTyp, typename ...Argumetns>
        struct funtion_info<ReturmTyp(Class_::*)(Argumetns...) const volatile &&> : method_funtion_info<Class_, ReturmTyp, Argumetns...> {};



        template<typename ReturmTyp>
        struct funtion_info<ReturmTyp()> : static_funtion_info<ReturmTyp> {};
        template<typename ReturmTyp>
        struct funtion_info<ReturmTyp() &>  : static_funtion_info<ReturmTyp> {};
        template<typename ReturmTyp>
        struct funtion_info<ReturmTyp() &&>  : static_funtion_info<ReturmTyp> {};
        template<typename ReturmTyp>
        struct funtion_info<ReturmTyp() const> : static_funtion_info<ReturmTyp> {};
        template<typename ReturmTyp>
        struct funtion_info<ReturmTyp() const &> : static_funtion_info<ReturmTyp> {};
        template<typename ReturmTyp>
        struct funtion_info<ReturmTyp() const &&> : static_funtion_info<ReturmTyp> {};
        template<typename ReturmTyp>
        struct funtion_info<ReturmTyp() volatile> : static_funtion_info<ReturmTyp> {};
        template<typename ReturmTyp>
        struct funtion_info<ReturmTyp() volatile &> : static_funtion_info<ReturmTyp> {};
        template<typename ReturmTyp>
        struct funtion_info<ReturmTyp() volatile &&> : static_funtion_info<ReturmTyp> {};
        template<typename ReturmTyp>
        struct funtion_info<ReturmTyp() const volatile> : static_funtion_info<ReturmTyp> {};
        template<typename ReturmTyp>
        struct funtion_info<ReturmTyp() const volatile &> : static_funtion_info<ReturmTyp> {};
        template<typename ReturmTyp>
        struct funtion_info<ReturmTyp() const volatile &&> : static_funtion_info<ReturmTyp> {};



        template<typename Class_, typename ReturmTyp>
        struct funtion_info<ReturmTyp(Class_::*)()> : method_funtion_info<Class_, ReturmTyp> {};
        template<typename Class_, typename ReturmTyp>
        struct funtion_info<ReturmTyp(Class_::*)() &> : method_funtion_info<Class_, ReturmTyp> {};
        template<typename Class_, typename ReturmTyp>
        struct funtion_info<ReturmTyp(Class_::*)() &&> : method_funtion_info<Class_, ReturmTyp> {};
        template<typename Class_, typename ReturmTyp>
        struct funtion_info<ReturmTyp(Class_::*)() const> : method_funtion_info<Class_, ReturmTyp> {};
        template<typename Class_, typename ReturmTyp>
        struct funtion_info<ReturmTyp(Class_::*)() const &> : method_funtion_info<Class_, ReturmTyp> {};
        template<typename Class_, typename ReturmTyp>
        struct funtion_info<ReturmTyp(Class_::*)() const &&> : method_funtion_info<Class_, ReturmTyp> {};
        template<typename Class_, typename ReturmTyp>
        struct funtion_info<ReturmTyp(Class_::*)() volatile> : method_funtion_info<Class_, ReturmTyp> {};
        template<typename Class_, typename ReturmTyp>
        struct funtion_info<ReturmTyp(Class_::*)() volatile &> : method_funtion_info<Class_, ReturmTyp> {};
        template<typename Class_, typename ReturmTyp>
        struct funtion_info<ReturmTyp(Class_::*)() volatile &&> : method_funtion_info<Class_, ReturmTyp> {};
        template<typename Class_, typename ReturmTyp>
        struct funtion_info<ReturmTyp(Class_::*)() const volatile> : method_funtion_info<Class_, ReturmTyp> {};
        template<typename Class_, typename ReturmTyp>
        struct funtion_info<ReturmTyp(Class_::*)() const volatile &> : method_funtion_info<Class_, ReturmTyp> {};
        template<typename Class_, typename ReturmTyp>
        struct funtion_info<ReturmTyp(Class_::*)() const volatile &&> : method_funtion_info<Class_, ReturmTyp> {};

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
