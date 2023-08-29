// Copyright Danyil Melnytskyi 2022-Present
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef SRC_GLOBAL_UTIL_UTIL_ENUM_CLASS
#define SRC_GLOBAL_UTIL_UTIL_ENUM_CLASS
#include <string>

#include <util/macro.hpp>
#define X_DEFINE_ENUM_WITH_STRING_CONVERSIONS_TOSTRING_CASE(data, elem) \
case data::elem:                                                        \
    return ART_STRINGIZE(elem);

#define X_DEFINE_ENUM_WITH_STRING_CONVERSIONS_FOR_EACH(data, elem) \
    if (fn(data::elem))                                            \
        return;

#define X_DEFINE_ENUM_WITH_STRING_CONVERSIONS_FROM_STRING(data, elem) \
    if (lower_str == ART_STRINGIZE(elem)) {                           \
        value = data::elem;                                           \
        return;                                                       \
    }

#define X_DEFINE_ENUM_WITH_STRING_CONVERSIONS_TO_INDEX(data, elem) \
    if (value == data::elem)                                       \
        return i;                                                  \
    i++;

#define X_DEFINE_ENUM_WITH_STRING_CONVERSIONS_TO_SIZE(data, elem) ART_STRINGIZE(elem),

#define basic_enum_class(name, enumerators, aliases)                                          \
    class name {                                                                              \
    public:                                                                                   \
        enum _##name{                                                                         \
            ART_UNROLL_SEQUENCE(enumerators)                                                  \
                ART_UNROLL_SEQUENCE(aliases)};                                                \
        std::string to_string() {                                                             \
            switch (v) {                                                                      \
                ART_JOIN(                                                                     \
                    X_DEFINE_ENUM_WITH_STRING_CONVERSIONS_TOSTRING_CASE,                      \
                    name,                                                                     \
                    ART_UNROLL_SEQUENCE(enumerators)                                          \
                )                                                                             \
            default:                                                                          \
                return '[' + std::to_string((size_t)v) + " invalid " ART_STRINGIZE(name) "]"; \
            }                                                                                 \
        }                                                                                     \
        name(_##name v) : v(v) {}                                                             \
        name(const name&) = default;                                                          \
        name& operator=(const name&) = default;                                               \
        name(name&&) = default;                                                               \
        name& operator=(name&&) = default;                                                    \
        bool operator==(const name& other) const {                                            \
            return v == other.v;                                                              \
        }                                                                                     \
        bool operator!=(const name& other) const {                                            \
            return v != other.v;                                                              \
        }                                                                                     \
                                                                                              \
    private:                                                                                  \
        _##name v;                                                                            \
    };

#define enum_class(name, enumerators, aliases)                                                        \
    namespace __internal {                                                                            \
        class name {                                                                                  \
        public:                                                                                       \
            enum _##name{                                                                             \
                ART_UNROLL_SEQUENCE(enumerators)                                                      \
                    ART_UNROLL_SEQUENCE(aliases)};                                                    \
            std::string to_string() {                                                                 \
                switch (value) {                                                                      \
                    ART_JOIN(                                                                         \
                        X_DEFINE_ENUM_WITH_STRING_CONVERSIONS_TOSTRING_CASE,                          \
                        _##name,                                                                      \
                        ART_UNROLL_SEQUENCE(enumerators)                                              \
                    )                                                                                 \
                default:                                                                              \
                    return '[' + std::to_string((size_t)value) + " invalid " ART_STRINGIZE(name) "]"; \
                }                                                                                     \
            }                                                                                         \
            static constexpr size_t count() {                                                         \
                constexpr const char* tt[] = {                                                        \
                    "",                                                                               \
                    ART_JOIN(                                                                         \
                        X_DEFINE_ENUM_WITH_STRING_CONVERSIONS_TO_SIZE,                                \
                        _##name,                                                                      \
                        ART_UNROLL_SEQUENCE(enumerators)                                              \
                    )};                                                                               \
                return sizeof(tt) / sizeof(char*) - 1;                                                \
            }                                                                                         \
            size_t as_type() {                                                                        \
                return (size_t)value;                                                                 \
            }                                                                                         \
            size_t index() {                                                                          \
                size_t i = 0;                                                                         \
                ART_JOIN(                                                                             \
                    X_DEFINE_ENUM_WITH_STRING_CONVERSIONS_TO_INDEX,                                   \
                    _##name,                                                                          \
                    ART_UNROLL_SEQUENCE(enumerators)                                                  \
                )                                                                                     \
                return (size_t)-1;                                                                    \
            }                                                                                         \
            template <class _FN>                                                                      \
            static constexpr void for_each(_FN fn) {                                                  \
                ART_JOIN(                                                                             \
                    X_DEFINE_ENUM_WITH_STRING_CONVERSIONS_FOR_EACH,                                   \
                    _##name,                                                                          \
                    ART_UNROLL_SEQUENCE(enumerators)                                                  \
                )                                                                                     \
            }                                                                                         \
            static constexpr _##name from_index(size_t i) {                                           \
                size_t j = 0;                                                                         \
                _##name res;                                                                          \
                bool found = false;                                                                   \
                for_each([&](auto v) constexpr {                                                      \
                    if (j == i) {                                                                     \
                        found = true;                                                                 \
                        res = v;                                                                      \
                        return false;                                                                 \
                    }                                                                                 \
                    j++;                                                                              \
                    return true;                                                                      \
                });                                                                                   \
                if (!found)                                                                           \
                    throw std::runtime_error("invalid index for " ART_STRINGIZE(name));               \
                return res;                                                                           \
            }                                                                                         \
            static _##name from_string(const std::string& str) {                                      \
                return name(str).value;                                                               \
            }                                                                                         \
            name(_##name v) : value(v) {}                                                             \
            name(const std::string& str) {                                                            \
                std::string lower_str;                                                                \
                lower_str.set_unsafe_state(true, false);                                              \
                lower_str.reserve(str.size());                                                        \
                for (auto c : str)                                                                    \
                    lower_str.push_back((char)::tolower(c));                                          \
                ART_JOIN(                                                                             \
                    X_DEFINE_ENUM_WITH_STRING_CONVERSIONS_FROM_STRING,                                \
                    _##name,                                                                          \
                    ART_UNROLL_SEQUENCE(enumerators)                                                  \
                )                                                                                     \
                lower_str.set_unsafe_state(false, false);                                             \
                throw std::runtime_error("invalid " ART_STRINGIZE(name) " string: " + str);           \
            }                                                                                         \
            name(const name&) = default;                                                              \
            name& operator=(const name&) = default;                                                   \
            name(name&&) = default;                                                                   \
            name& operator=(name&&) = default;                                                        \
            constexpr bool operator==(const name& other) const {                                      \
                return value == other.value;                                                          \
            }                                                                                         \
            constexpr bool operator!=(const name& other) const {                                      \
                return value != other.value;                                                          \
            }                                                                                         \
                                                                                                      \
        protected:                                                                                    \
            _##name value;                                                                            \
        };                                                                                            \
    }                                                                                                 \
    class name : public __internal::name


#define enum_class_t(name, type, enumerators, aliases)                                                \
    namespace __internal {                                                                            \
        class name {                                                                                  \
        public:                                                                                       \
            enum _##name : type{                                                                      \
                ART_UNROLL_SEQUENCE(enumerators)                                                      \
                    ART_UNROLL_SEQUENCE(aliases)};                                                    \
            std::string to_string() {                                                                 \
                switch (value) {                                                                      \
                    ART_JOIN(                                                                         \
                        X_DEFINE_ENUM_WITH_STRING_CONVERSIONS_TOSTRING_CASE,                          \
                        _##name,                                                                      \
                        ART_UNROLL_SEQUENCE(enumerators)                                              \
                    )                                                                                 \
                default:                                                                              \
                    return '[' + std::to_string((size_t)value) + " invalid " ART_STRINGIZE(name) "]"; \
                }                                                                                     \
            }                                                                                         \
            static constexpr type count() {                                                           \
                constexpr const char* tt[] = {                                                        \
                    "",                                                                               \
                    ART_JOIN(                                                                         \
                        X_DEFINE_ENUM_WITH_STRING_CONVERSIONS_TO_SIZE,                                \
                        _##name,                                                                      \
                        ART_UNROLL_SEQUENCE(enumerators)                                              \
                    )};                                                                               \
                return sizeof(tt) / sizeof(char*) - 1;                                                \
            }                                                                                         \
            type as_type() {                                                                          \
                return (type)value;                                                                   \
            }                                                                                         \
            size_t index() {                                                                          \
                size_t i = 0;                                                                         \
                ART_JOIN(                                                                             \
                    X_DEFINE_ENUM_WITH_STRING_CONVERSIONS_TO_INDEX,                                   \
                    _##name,                                                                          \
                    ART_UNROLL_SEQUENCE(enumerators)                                                  \
                )                                                                                     \
                return (size_t)-1;                                                                    \
            }                                                                                         \
            template <class _FN>                                                                      \
            static constexpr void for_each(_FN fn) {                                                  \
                ART_JOIN(                                                                             \
                    X_DEFINE_ENUM_WITH_STRING_CONVERSIONS_FOR_EACH,                                   \
                    _##name,                                                                          \
                    ART_UNROLL_SEQUENCE(enumerators)                                                  \
                )                                                                                     \
            }                                                                                         \
            static constexpr _##name from_index(size_t i) {                                           \
                size_t j = 0;                                                                         \
                _##name res;                                                                          \
                bool found = false;                                                                   \
                for_each([&](auto v) constexpr {                                                      \
                    if (j == i) {                                                                     \
                        found = true;                                                                 \
                        res = v;                                                                      \
                        return false;                                                                 \
                    }                                                                                 \
                    j++;                                                                              \
                    return true;                                                                      \
                });                                                                                   \
                if (!found)                                                                           \
                    throw std::runtime_error("invalid index for " ART_STRINGIZE(name));               \
                return res;                                                                           \
            }                                                                                         \
            static _##name from_string(const std::string& str) {                                      \
                return name(str).value;                                                               \
            }                                                                                         \
            name(_##name v) : value(v) {}                                                             \
            name(const std::string& str) {                                                            \
                std::string lower_str;                                                                \
                lower_str.set_unsafe_state(true, false);                                              \
                lower_str.reserve(str.size());                                                        \
                for (auto c : str)                                                                    \
                    lower_str.push_back((char)::tolower(c));                                          \
                ART_JOIN(                                                                             \
                    X_DEFINE_ENUM_WITH_STRING_CONVERSIONS_FROM_STRING,                                \
                    _##name,                                                                          \
                    ART_UNROLL_SEQUENCE(enumerators)                                                  \
                )                                                                                     \
                lower_str.set_unsafe_state(false, false);                                             \
                throw std::runtime_error("invalid " ART_STRINGIZE(name) " string: " + str);           \
            }                                                                                         \
            name(const name&) = default;                                                              \
            name& operator=(const name&) = default;                                                   \
            name(name&&) = default;                                                                   \
            name& operator=(name&&) = default;                                                        \
            bool operator==(const name& other) const {                                                \
                return value == other.value;                                                          \
            }                                                                                         \
            bool operator!=(const name& other) const {                                                \
                return value != other.value;                                                          \
            }                                                                                         \
                                                                                                      \
        protected:                                                                                    \
            _##name value;                                                                            \
        };                                                                                            \
    }                                                                                                 \
    class name : public __internal::name

#endif /* SRC_GLOBAL_UTIL_UTIL_ENUM_CLASS */
