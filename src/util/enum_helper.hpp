// Copyright Danyil Melnytskyi 2022-Present
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once
#ifndef SRC_UTIL_ENUM_HELPER
    #define SRC_UTIL_ENUM_HELPER
    #include <string>

    #include <util/macro.hpp>


    #define ART_SEQ_FROM_VA_ARG(i) (i)
    #define ART_MAP_SEQ_FROM_VA_ARG(...) ART_MAP(ART_SEQ_FROM_VA_ARG, __VA_ARGS__)

    #define X_DEFINE_ENUM_WITH_STRING_CONVERSIONS_TOSTRING_CASE(data, elem) \
    case data::elem:                                                        \
        return ART_STRINGIZE(elem);

    //namespaces here used to disallow use this macro in class/struct body
    #define ENUM(name, ...)                                                                       \
        enum class name {                                                                         \
            __VA_ARGS__                                                                           \
        };                                                                                        \
                                                                                                  \
        namespace {                                                                               \
            inline std::string enum_to_string(name v) {                                           \
                switch (v) {                                                                      \
                    ART_JOIN(                                                                     \
                        X_DEFINE_ENUM_WITH_STRING_CONVERSIONS_TOSTRING_CASE,                      \
                        name,                                                                     \
                        __VA_ARGS__                                                               \
                    )                                                                             \
                default:                                                                          \
                    return '[' + std::to_string((size_t)v) + " invalid " ART_STRINGIZE(name) "]"; \
                }                                                                                 \
            }                                                                                     \
        }


    #define ENUM_t(name, type, ...)                                                             \
        enum class name : type {                                                                \
            __VA_ARGS__                                                                         \
        };                                                                                      \
                                                                                                \
        namespace {                                                                             \
            inline std::string enum_to_string(name v) {                                         \
                switch (v) {                                                                    \
                    ART_JOIN(                                                                   \
                        X_DEFINE_ENUM_WITH_STRING_CONVERSIONS_TOSTRING_CASE,                    \
                        name,                                                                   \
                        __VA_ARGS__                                                             \
                    )                                                                           \
                default:                                                                        \
                    return '[' + std::to_string((type)v) + " invalid " ART_STRINGIZE(name) "]"; \
                }                                                                               \
            }                                                                                   \
        }

    #define ENUM_ta(name, type, aliases, ...)                                                   \
        enum class name : type {                                                                \
            __VA_ARGS__,                                                                        \
            ART_UNROLL_SEQUENCE(aliases)                                                        \
        };                                                                                      \
                                                                                                \
        namespace {                                                                             \
            inline std::string enum_to_string(name v) {                                         \
                switch (v) {                                                                    \
                    ART_JOIN(                                                                   \
                        X_DEFINE_ENUM_WITH_STRING_CONVERSIONS_TOSTRING_CASE,                    \
                        name,                                                                   \
                        __VA_ARGS__                                                             \
                    )                                                                           \
                default:                                                                        \
                    return '[' + std::to_string((type)v) + " invalid " ART_STRINGIZE(name) "]"; \
                }                                                                               \
            }                                                                                   \
        }

#endif /* SRC_UTIL_ENUM_HELPER */
