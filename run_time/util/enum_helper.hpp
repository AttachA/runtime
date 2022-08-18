#pragma once
#include <boost/preprocessor.hpp>
#include <string>
#define X_DEFINE_ENUM_WITH_STRING_CONVERSIONS_TOSTRING_CASE(r, data, elem)    \
    case data::elem : return BOOST_PP_STRINGIZE(elem);

//namespaces here used to disallow use this macro in class/struct body
#define ENUM(name, enumerators)                \
    enum class name {                                                               \
        BOOST_PP_SEQ_ENUM(enumerators)                                        \
    };                                                                        \
                                                                              \
    namespace{inline std::string enum_to_string(name v)                                 \
    {                                                                         \
        switch (v)                                                            \
        {                                                                     \
            BOOST_PP_SEQ_FOR_EACH(                                            \
                X_DEFINE_ENUM_WITH_STRING_CONVERSIONS_TOSTRING_CASE,          \
                name,                                                         \
                enumerators                                                   \
            )                                                                 \
            default: return '[' + std::to_string((size_t)v) + " invalid " BOOST_PP_STRINGIZE(name) "]";\
        }                                                                     \
    }}

#define ENUM_t(name,type, enumerators)                \
    enum class name : type {                                                        \
        BOOST_PP_SEQ_ENUM(enumerators)                                        \
    };                                                                        \
                                                                              \
    namespace{inline std::string enum_to_string(name v)                       \
    {                                                                         \
        switch (v)                                                            \
        {                                                                     \
            BOOST_PP_SEQ_FOR_EACH(                                            \
                X_DEFINE_ENUM_WITH_STRING_CONVERSIONS_TOSTRING_CASE,          \
                name,                                                         \
                enumerators                                                   \
            )                                                                 \
            default: return '[' + std::to_string((type)v) + " invalid " BOOST_PP_STRINGIZE(name) "]";\
        }                                                                     \
    }}
