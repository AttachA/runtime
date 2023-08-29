// Copyright Danyil Melnytskyi 2022-Present
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef SRC_GLOBAL_UTIL_UTIL_ENUM_CLASS
#define SRC_GLOBAL_UTIL_UTIL_ENUM_CLASS
#include <boost/preprocessor.hpp>
#include <string>
#define X_DEFINE_ENUM_WITH_STRING_CONVERSIONS_TOSTRING_CASE(r, data, elem) \
    case data::elem:                                                       \
        return BOOST_PP_STRINGIZE(elem);

#define X_DEFINE_ENUM_WITH_STRING_CONVERSIONS_FOR_EACH(r, data, elem) \
    if (fn(data::elem))                                               \
        return;

#define X_DEFINE_ENUM_WITH_STRING_CONVERSIONS_FROM_STRING(r, data, elem) \
    if (lower_str == BOOST_PP_STRINGIZE(elem)) {                         \
        value = data::elem;                                              \
        return;                                                          \
    }

#define X_DEFINE_ENUM_WITH_STRING_CONVERSIONS_TO_INDEX(r, data, elem) \
    if (value == data::elem)                                          \
        return i;                                                     \
    i++;


#define basic_enum_class(name, enumerators, aliases)                                               \
    class name {                                                                                   \
    public:                                                                                        \
        enum _##name{                                                                              \
            BOOST_PP_SEQ_ENUM(enumerators)                                                         \
                BOOST_PP_SEQ_ENUM(aliases)};                                                       \
        art::ustring to_string() {                                                                 \
            switch (v) {                                                                           \
                BOOST_PP_SEQ_FOR_EACH(                                                             \
                    X_DEFINE_ENUM_WITH_STRING_CONVERSIONS_TOSTRING_CASE,                           \
                    name,                                                                          \
                    enumerators)                                                                   \
            default:                                                                               \
                return '[' + std::to_string((size_t)v) + " invalid " BOOST_PP_STRINGIZE(name) "]"; \
            }                                                                                      \
        }                                                                                          \
        name(_##name v) : v(v) {}                                                                  \
        name(const name&) = default;                                                               \
        name& operator=(const name&) = default;                                                    \
        name(name&&) = default;                                                                    \
        name& operator=(name&&) = default;                                                         \
        bool operator==(const name& other) const {                                                 \
            return v == other.v;                                                                   \
        }                                                                                          \
        bool operator!=(const name& other) const {                                                 \
            return v != other.v;                                                                   \
        }                                                                                          \
                                                                                                   \
    private:                                                                                       \
        _##name v;                                                                                 \
    };

#define enum_class(name, enumerators, aliases)                                                             \
    namespace __internal {                                                                                 \
        class name {                                                                                       \
        public:                                                                                            \
            enum _##name{                                                                                  \
                BOOST_PP_SEQ_ENUM(enumerators)                                                             \
                    BOOST_PP_SEQ_ENUM(aliases)};                                                           \
            art::ustring to_string() {                                                                     \
                switch (value) {                                                                           \
                    BOOST_PP_SEQ_FOR_EACH(                                                                 \
                        X_DEFINE_ENUM_WITH_STRING_CONVERSIONS_TOSTRING_CASE,                               \
                        _##name,                                                                           \
                        enumerators)                                                                       \
                default:                                                                                   \
                    return '[' + std::to_string((size_t)value) + " invalid " BOOST_PP_STRINGIZE(name) "]"; \
                }                                                                                          \
            }                                                                                              \
            static constexpr size_t count() {                                                              \
                return (size_t)BOOST_PP_SEQ_SIZE(enumerators);                                             \
            }                                                                                              \
            size_t as_type() {                                                                             \
                return (size_t)value;                                                                      \
            }                                                                                              \
            size_t index() {                                                                               \
                size_t i = 0;                                                                              \
                BOOST_PP_SEQ_FOR_EACH(                                                                     \
                    X_DEFINE_ENUM_WITH_STRING_CONVERSIONS_TO_INDEX,                                        \
                    _##name,                                                                               \
                    enumerators)                                                                           \
                return (size_t)-1;                                                                         \
            }                                                                                              \
            template <class _FN>                                                                           \
            static constexpr void for_each(_FN fn) {                                                       \
                BOOST_PP_SEQ_FOR_EACH(                                                                     \
                    X_DEFINE_ENUM_WITH_STRING_CONVERSIONS_FOR_EACH,                                        \
                    _##name,                                                                               \
                    enumerators)                                                                           \
            }                                                                                              \
            static _##name from_index(size_t i) {                                                          \
                size_t j = 0;                                                                              \
                _##name res;                                                                               \
                bool found = false;                                                                        \
                for_each([&](auto v) {                                                                     \
                    if (j == i) {                                                                          \
                        found = true;                                                                      \
                        res = v;                                                                           \
                        return false;                                                                      \
                    }                                                                                      \
                    j++;                                                                                   \
                    return true;                                                                           \
                });                                                                                        \
                if (!found)                                                                                \
                    throw std::runtime_error("invalid index for " BOOST_PP_STRINGIZE(name));               \
                return res;                                                                                \
            }                                                                                              \
            static _##name from_string(const art::ustring& str) {                                          \
                return name(str).value;                                                                    \
            }                                                                                              \
            name(_##name v) : value(v) {}                                                                  \
            name(const art::ustring& str) {                                                                \
                art::ustring lower_str;                                                                    \
                lower_str.reserve(str.size());                                                             \
                for (auto c : str)                                                                         \
                    lower_str.push_back(::tolower(c));                                                     \
                BOOST_PP_SEQ_FOR_EACH(                                                                     \
                    X_DEFINE_ENUM_WITH_STRING_CONVERSIONS_FROM_STRING,                                     \
                    _##name,                                                                               \
                    enumerators)                                                                           \
                throw std::runtime_error("invalid " BOOST_PP_STRINGIZE(name) " string: " + str);           \
            }                                                                                              \
            name(const name&) = default;                                                                   \
            name& operator=(const name&) = default;                                                        \
            name(name&&) = default;                                                                        \
            name& operator=(name&&) = default;                                                             \
            bool operator==(const name& other) const {                                                     \
                return value == other.value;                                                               \
            }                                                                                              \
            bool operator!=(const name& other) const {                                                     \
                return value != other.value;                                                               \
            }                                                                                              \
                                                                                                           \
        protected:                                                                                         \
            _##name value;                                                                                 \
        };                                                                                                 \
    }                                                                                                      \
    class name : public __internal::name


#define enum_class_t(name, type, enumerators, aliases)                                                     \
    namespace __internal {                                                                                 \
        class name {                                                                                       \
        public:                                                                                            \
            enum _##name : type{                                                                           \
                BOOST_PP_SEQ_ENUM(enumerators)                                                             \
                    BOOST_PP_SEQ_ENUM(aliases)};                                                           \
            art::ustring to_string() {                                                                     \
                switch (value) {                                                                           \
                    BOOST_PP_SEQ_FOR_EACH(                                                                 \
                        X_DEFINE_ENUM_WITH_STRING_CONVERSIONS_TOSTRING_CASE,                               \
                        _##name,                                                                           \
                        enumerators)                                                                       \
                default:                                                                                   \
                    return '[' + std::to_string((size_t)value) + " invalid " BOOST_PP_STRINGIZE(name) "]"; \
                }                                                                                          \
            }                                                                                              \
            static constexpr type count() {                                                                \
                return (type)BOOST_PP_SEQ_SIZE(enumerators);                                               \
            }                                                                                              \
            type as_type() {                                                                               \
                return (type)value;                                                                        \
            }                                                                                              \
            size_t index() {                                                                               \
                size_t i = 0;                                                                              \
                BOOST_PP_SEQ_FOR_EACH(                                                                     \
                    X_DEFINE_ENUM_WITH_STRING_CONVERSIONS_TO_INDEX,                                        \
                    _##name,                                                                               \
                    enumerators)                                                                           \
                return (size_t)-1;                                                                         \
            }                                                                                              \
            template <class _FN>                                                                           \
            static constexpr void for_each(_FN fn) {                                                       \
                BOOST_PP_SEQ_FOR_EACH(                                                                     \
                    X_DEFINE_ENUM_WITH_STRING_CONVERSIONS_FOR_EACH,                                        \
                    _##name,                                                                               \
                    enumerators)                                                                           \
            }                                                                                              \
            static _##name from_index(size_t i) {                                                          \
                size_t j = 0;                                                                              \
                _##name res;                                                                               \
                bool found = false;                                                                        \
                for_each([&](auto v) {                                                                     \
                    if (j == i) {                                                                          \
                        found = true;                                                                      \
                        res = v;                                                                           \
                        return false;                                                                      \
                    }                                                                                      \
                    j++;                                                                                   \
                    return true;                                                                           \
                });                                                                                        \
                if (!found)                                                                                \
                    throw std::runtime_error("invalid index for " BOOST_PP_STRINGIZE(name));               \
                return res;                                                                                \
            }                                                                                              \
            static _##name from_string(const art::ustring& str) {                                          \
                return name(str).value;                                                                    \
            }                                                                                              \
            name(_##name v) : value(v) {}                                                                  \
            name(const art::ustring& str) {                                                                \
                art::ustring lower_str;                                                                    \
                lower_str.reserve(str.size());                                                             \
                for (auto c : str)                                                                         \
                    lower_str.push_back(::tolower(c));                                                     \
                BOOST_PP_SEQ_FOR_EACH(                                                                     \
                    X_DEFINE_ENUM_WITH_STRING_CONVERSIONS_FROM_STRING,                                     \
                    _##name,                                                                               \
                    enumerators)                                                                           \
                throw std::runtime_error("invalid " BOOST_PP_STRINGIZE(name) " string: " + str);           \
            }                                                                                              \
            name(const name&) = default;                                                                   \
            name& operator=(const name&) = default;                                                        \
            name(name&&) = default;                                                                        \
            name& operator=(name&&) = default;                                                             \
            bool operator==(const name& other) const {                                                     \
                return value == other.value;                                                               \
            }                                                                                              \
            bool operator!=(const name& other) const {                                                     \
                return value != other.value;                                                               \
            }                                                                                              \
                                                                                                           \
        protected:                                                                                         \
            _##name value;                                                                                 \
        };                                                                                                 \
    }                                                                                                      \
    class name : public __internal::name

#endif /* SRC_GLOBAL_UTIL_UTIL_ENUM_CLASS */
