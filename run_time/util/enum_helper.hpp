// Copyright Danyil Melnytskyi 2022-2023
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#pragma once
#ifndef RUN_TIME_UTIL_ENUM_HELPER
#define RUN_TIME_UTIL_ENUM_HELPER
#include <boost/preprocessor/library.hpp>
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

#define ENUM_ta(name,type, enumerators, aliases)                              \
    enum class name : type {                                                  \
        BOOST_PP_SEQ_ENUM(enumerators),                                       \
        BOOST_PP_SEQ_ENUM(aliases)                                            \
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

#endif /* RUN_TIME_UTIL_ENUM_HELPER */
