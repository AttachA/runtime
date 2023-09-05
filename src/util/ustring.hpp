// Copyright Danyil Melnytskyi 2022-Present
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef SRC_UTIL_USTRING
#define SRC_UTIL_USTRING
#include <cstdint>
#include <string>

#include <library/list_array.hpp>
#include <util/array.hpp>
#include <util/link_garbage_remover.hpp>
#include <util/platform.hpp>
#include <util/shared_ptr.hpp>

namespace art {
    class ustring;

    namespace constant_pool {
        struct pool_item {
            char* from_constant_pool;
            size_t hash;
            size_t symbols;
            size_t size;
        };

        struct dyn_pool_item {
            typed_lgr<pool_item> item;
            ~dyn_pool_item();
        };

        //TODO implement caching for string operations for dynamic_constant_pool
        dyn_pool_item* dynamic_constant_pool(const ustring& string);
        dyn_pool_item* dynamic_constant_pool(const char* string);
        dyn_pool_item* dynamic_constant_pool(const char8_t* string);
        dyn_pool_item* dynamic_constant_pool(const char16_t* string);
        dyn_pool_item* dynamic_constant_pool(const char32_t* string);
        dyn_pool_item* dynamic_constant_pool(const char* string, size_t size);
        dyn_pool_item* dynamic_constant_pool(const char8_t* string, size_t size);
        dyn_pool_item* dynamic_constant_pool(const char16_t* string, size_t size);
        dyn_pool_item* dynamic_constant_pool(const char32_t* string, size_t size);

        pool_item* make_constant_pool(const ustring& string);
        pool_item* make_constant_pool(const char* string);
        pool_item* make_constant_pool(const char8_t* string);
        pool_item* make_constant_pool(const char16_t* string);
        pool_item* make_constant_pool(const char32_t* string);
        pool_item* make_constant_pool(const char* string, size_t size);
        pool_item* make_constant_pool(const char8_t* string, size_t size);
        pool_item* make_constant_pool(const char16_t* string, size_t size);
        pool_item* make_constant_pool(const char32_t* string, size_t size);
    };

    //ustring internally uses utf8, but dances as utf32
    class ustring {
        static constexpr size_t short_array_size = []() {
            size_t long_arr = sizeof(list_array<char>) + sizeof(size_t) + sizeof(size_t);
            size_t dyn_constant_data = sizeof(std::shared_ptr<constant_pool::dyn_pool_item>);
            return long_arr > dyn_constant_data ? long_arr : dyn_constant_data;
        }();

        union UnifiedData {
            char short_data[short_array_size]{0};

            struct default_ {
                list_array<char> larr;
                size_t hash;
                size_t symbols;

                default_() {}

                ~default_() {}
            } d;

            constant_pool::pool_item* constant_data;
            shared_ptr<constant_pool::dyn_pool_item> dynamic_constant_data;

            UnifiedData() {}

            ~UnifiedData() {}
        } _data;
        enum class type : uint8_t {
            def,
            short_def,
            constant,
            dyn_constant
        };

        struct {
            bool has_hash : 1;   //is hash calculated?
            bool has_length : 1; //if not set, count of bytes in string is unknown
            bool has_data : 1;   //if not set, same as empty string
            bool as_unsafe : 1;  //string not validated, act like raw array, all operations is direct(can even modify constant pool value)(use ony if you know what you do)
            type type : 3;
        } flags;

        void cleanup_current();
        void switch_to_type(type);
        char* direct_ptr();

    public:
        static constexpr size_t npos = -1;
        void set_unsafe_state(bool state, bool throw_on_error);
        ustring();
        ustring(const char* str); //ansi7 | utf8
        ustring(const char* str, size_t size);
        ustring(const char8_t* str); //utf8
        ustring(const char8_t* str, size_t size);
        ustring(const char16_t* str); //utf16
        ustring(const char16_t* str, size_t size);
        ustring(const char32_t* str); //utf32
        ustring(const char32_t* str, size_t size);

        ustring(const char* begin, const char* end)
            : ustring(begin, end - begin) {}

        ustring(const char8_t* begin, const char8_t* end)
            : ustring(begin, end - begin) {}

        ustring(const char16_t* begin, const char16_t* end)
            : ustring(begin, end - begin) {}

        ustring(const char32_t* begin, const char32_t* end)
            : ustring(begin, end - begin) {}

        ustring(char str)
            : ustring(&str, 1) {}

        ustring(char8_t str)
            : ustring(&str, 1) {}

        ustring(char16_t str)
            : ustring(&str, 1) {}

        ustring(char32_t str)
            : ustring(&str, 1) {}

        ustring(const ustring& str);
        ustring(const std::string& str);
        ustring(const std::u16string& str);
        ustring(const std::u32string& str);
#ifdef PLATFORM_WINDOWS
        ustring(const std::wstring& str);
        ustring(wchar_t* str);
        ustring(wchar_t* str, size_t len);

        ustring(wchar_t* begin, wchar_t* end)
            : ustring(begin, end - begin) {}

        ustring(wchar_t str)
            : ustring(&str, 1) {}
#endif
        ustring(ustring&& str) noexcept;
        ~ustring();

        ustring& operator=(const ustring& str);
        ustring& operator=(ustring&& str) noexcept;

        bool operator==(const ustring& str) const;
        bool operator!=(const ustring& str) const;
        bool operator<(const ustring& str) const;
        bool operator>(const ustring& str) const;
        bool operator<=(const ustring& str) const;
        bool operator>=(const ustring& str) const;
        std::strong_ordering operator<=>(const ustring& str) const;


        ustring& operator+=(const ustring& str);     //insert at end
        ustring operator+(const ustring& str) const; //insert at end

        ustring& operator-=(const ustring& str);     //insert at begin
        ustring operator-(const ustring& str) const; //insert at begin

        const char* c_str() const;
        char* data();
        size_t length() const; //total chars
        size_t size() const;   //used bytes
        size_t hash() const;
        bool empty() const;


        char32_t get(size_t pos) const; //index by utf8 char and return utf32 codepoint
        char get_ansi(size_t pos);      //if pos is utf8 char, throw exception
        char get_codepoint(size_t pos); //direct index

        void set(size_t pos, char32_t c);
        void set_ansi(size_t pos, char c);
        void set_codepoint(size_t pos, char c); //also checks if everything is ok

        array_t<uint32_t> as_array_32(); //returns utf32
        array_t<uint16_t> as_array_16(); //returns utf16
        array_t<uint8_t> as_array_8();   //returns utf8

        operator std::string() const;
        operator std::u8string() const;
        operator std::u16string() const;
        operator std::u32string() const;


        char* begin();
        char* end();
        const char* begin() const;
        const char* end() const;
        const char* cbegin() const;
        const char* cend() const;


        void reserve(size_t);
        void reserve_push_back(size_t);
        void reserve_push_front(size_t);
        void push_back(const ustring& c);
        void push_front(const ustring& c);

        void append(const ustring& c);
        void append(const char* str, size_t len);
        void append(const char8_t* str, size_t len);
        void append(const char16_t* str, size_t len);
        void append(const char32_t* str, size_t len);

        void append(const char* str);
        void append(const char8_t* str);
        void append(const char16_t* str);
        void append(const char32_t* str);

        template <size_t len>
        void append(const char str[len]) {
            append(str, len);
        }

        template <size_t len>
        void append(const char8_t str[len]) {
            append(str, len);
        }

        template <size_t len>
        void append(const char16_t str[len]) {
            append(str, len);
        }

        template <size_t len>
        void append(const char32_t str[len]) {
            append(str, len);
        }

        void resize(size_t len);
        bool starts_with(const ustring& c) const;
        bool ends_with(const ustring& c) const;
        ustring substr(size_t pos, size_t len) const;
        ustring substr(size_t pos) const;
        void clear();
        void shrink_to_fit();

        size_t find_last_of(const ustring& c) const;
        size_t find_first_of(const ustring& c) const;

        /*[[INTERNAL]]*/ char& operator[](size_t i) {
            return data()[i];
        }

        /*[[INTERNAL]]*/ const char& operator[](size_t i) const {
            return c_str()[i];
        }
    };

    inline ustring operator+(const char* s, const ustring& str) {
        return ustring(s) + str;
    }

    inline ustring operator+(const char8_t* s, const ustring& str) {
        return ustring(s) + str;
    }

    inline ustring operator+(const char16_t* s, const ustring& str) {
        return ustring(s) + str;
    }

    inline ustring operator+(const char32_t* s, const ustring& str) {
        return ustring(s) + str;
    }

    inline ustring operator+(const std::string& s, const ustring& str) {
        return ustring(s) + str;
    }

    inline ustring operator-(const char* s, const ustring& str) {
        return ustring(s) - str;
    }

    inline ustring operator-(const char8_t* s, const ustring& str) {
        return ustring(s) - str;
    }

    inline ustring operator-(const char16_t* s, const ustring& str) {
        return ustring(s) - str;
    }

    inline ustring operator-(const char32_t* s, const ustring& str) {
        return ustring(s) - str;
    }

    inline ustring operator-(const std::string& s, const ustring& str) {
        return ustring(s) - str;
    }

    inline ustring operator+(char s, const ustring& str) {
        return ustring(s) + str;
    }

    inline ustring operator+(char8_t s, const ustring& str) {
        return ustring(s) + str;
    }

    inline ustring operator+(char16_t s, const ustring& str) {
        return ustring(s) + str;
    }

    inline ustring operator+(char32_t s, const ustring& str) {
        return ustring(s) + str;
    }

    inline ustring operator-(char s, const ustring& str) {
        return ustring(s) - str;
    }

    inline ustring operator-(char8_t s, const ustring& str) {
        return ustring(s) - str;
    }

    inline ustring operator-(char16_t s, const ustring& str) {
        return ustring(s) - str;
    }

    inline ustring operator-(char32_t s, const ustring& str) {
        return ustring(s) - str;
    }

    inline bool operator==(const char* s, const ustring& str) {
        return ustring(s) == str;
    }

    inline bool operator==(const char8_t* s, const ustring& str) {
        return ustring(s) == str;
    }

    inline bool operator==(const char16_t* s, const ustring& str) {
        return ustring(s) == str;
    }

    inline bool operator==(const char32_t* s, const ustring& str) {
        return ustring(s) == str;
    }

    inline bool operator==(const std::string& s, const ustring& str) {
        return ustring(s) == str;
    }

    inline bool operator==(char s, const ustring& str) {
        return ustring(s) == str;
    }

    inline bool operator==(char8_t s, const ustring& str) {
        return ustring(s) == str;
    }

    inline bool operator==(char16_t s, const ustring& str) {
        return ustring(s) == str;
    }

    inline bool operator==(char32_t s, const ustring& str) {
        return ustring(s) == str;
    }

    inline bool operator!=(const char* s, const ustring& str) {
        return ustring(s) != str;
    }

    inline bool operator!=(const char8_t* s, const ustring& str) {
        return ustring(s) != str;
    }

    inline bool operator!=(const char16_t* s, const ustring& str) {
        return ustring(s) != str;
    }

    inline bool operator!=(const char32_t* s, const ustring& str) {
        return ustring(s) != str;
    }

    inline bool operator!=(const std::string& s, const ustring& str) {
        return ustring(s) != str;
    }

    inline bool operator!=(char s, const ustring& str) {
        return ustring(s) != str;
    }

    inline bool operator!=(char8_t s, const ustring& str) {
        return ustring(s) != str;
    }

    inline bool operator!=(char16_t s, const ustring& str) {
        return ustring(s) != str;
    }

    inline bool operator!=(char32_t s, const ustring& str) {
        return ustring(s) != str;
    }

    inline bool operator<(const char* s, const ustring& str) {
        return ustring(s) < str;
    }

    inline bool operator<(const char8_t* s, const ustring& str) {
        return ustring(s) < str;
    }

    inline bool operator<(const char16_t* s, const ustring& str) {
        return ustring(s) < str;
    }

    inline bool operator<(const char32_t* s, const ustring& str) {
        return ustring(s) < str;
    }

    inline bool operator<(const std::string& s, const ustring& str) {
        return ustring(s) < str;
    }

    inline bool operator<(char s, const ustring& str) {
        return ustring(s) < str;
    }

    inline bool operator<(char8_t s, const ustring& str) {
        return ustring(s) < str;
    }

    inline bool operator<(char16_t s, const ustring& str) {
        return ustring(s) < str;
    }

    inline bool operator<(char32_t s, const ustring& str) {
        return ustring(s) < str;
    }

    inline bool operator>(const char* s, const ustring& str) {
        return ustring(s) > str;
    }

    inline bool operator>(const char8_t* s, const ustring& str) {
        return ustring(s) > str;
    }

    inline bool operator>(const char16_t* s, const ustring& str) {
        return ustring(s) > str;
    }

    inline bool operator>(const char32_t* s, const ustring& str) {
        return ustring(s) > str;
    }

    inline bool operator>(const std::string& s, const ustring& str) {
        return ustring(s) > str;
    }

    inline bool operator>(char s, const ustring& str) {
        return ustring(s) > str;
    }

    inline bool operator>(char8_t s, const ustring& str) {
        return ustring(s) > str;
    }

    inline bool operator>(char16_t s, const ustring& str) {
        return ustring(s) > str;
    }

    inline bool operator>(char32_t s, const ustring& str) {
        return ustring(s) > str;
    }

    inline bool operator<=(const char* s, const ustring& str) {
        return ustring(s) <= str;
    }

    inline bool operator<=(const char8_t* s, const ustring& str) {
        return ustring(s) <= str;
    }

    inline bool operator<=(const char16_t* s, const ustring& str) {
        return ustring(s) <= str;
    }

    inline bool operator<=(const char32_t* s, const ustring& str) {
        return ustring(s) <= str;
    }

    inline bool operator<=(const std::string& s, const ustring& str) {
        return ustring(s) <= str;
    }

    inline bool operator<=(char s, const ustring& str) {
        return ustring(s) <= str;
    }

    inline bool operator<=(char8_t s, const ustring& str) {
        return ustring(s) <= str;
    }

    inline bool operator<=(char16_t s, const ustring& str) {
        return ustring(s) <= str;
    }

    inline bool operator<=(char32_t s, const ustring& str) {
        return ustring(s) <= str;
    }

    inline bool operator>=(const char* s, const ustring& str) {
        return ustring(s) >= str;
    }

    inline bool operator>=(const char8_t* s, const ustring& str) {
        return ustring(s) >= str;
    }

    inline bool operator>=(const char16_t* s, const ustring& str) {
        return ustring(s) >= str;
    }

    inline bool operator>=(const char32_t* s, const ustring& str) {
        return ustring(s) >= str;
    }

    inline bool operator>=(const std::string& s, const ustring& str) {
        return ustring(s) >= str;
    }

    inline bool operator>=(char s, const ustring& str) {
        return ustring(s) >= str;
    }

    inline bool operator>=(char8_t s, const ustring& str) {
        return ustring(s) >= str;
    }

    inline bool operator>=(char16_t s, const ustring& str) {
        return ustring(s) >= str;
    }

    inline bool operator>=(char32_t s, const ustring& str) {
        return ustring(s) >= str;
    }
}
#endif /* SRC_UTIL_USTRING */
