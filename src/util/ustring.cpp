// Copyright Danyil Melnytskyi 2022-Present
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#include <unordered_map>
#include <utf8cpp/utf8.h>

#include <run_time/tasks.hpp>
#include <util/exceptions.hpp>
#include <util/hash.hpp>
#include <util/ustring.hpp>

namespace art {
    namespace constant_pool {
        pool_item* as_pool(const ustring& str) {
            pool_item* item = new pool_item;
            item->from_constant_pool = new char[str.size()];
            memcpy(item->from_constant_pool, str.c_str(), str.size());
            item->hash = str.hash();
            return item;
        }

        pool_item* as_pool(const char* str, size_t size, size_t hash) {
            pool_item* item = new pool_item;
            item->from_constant_pool = new char[size];
            memcpy(item->from_constant_pool, str, size);
            item->hash = hash;
            return item;
        }

        std::unordered_map<size_t, dyn_pool_item*, art::hash<size_t>> dynamic_constant_pool_map;
        TaskMutex dynamic_constant_pool_mutex;

        dyn_pool_item::~dyn_pool_item() {
            if (item.totalLinks() == 2) { // holder + map
                art::lock_guard<TaskMutex> lock(dynamic_constant_pool_mutex);
                dynamic_constant_pool_map.erase(item->hash);
            }
        }

        dyn_pool_item* dynamic_constant_pool(const class ustring& string) {
            art::lock_guard<TaskMutex> lock(dynamic_constant_pool_mutex);
            auto it = dynamic_constant_pool_map.find(string.hash());
            if (it != dynamic_constant_pool_map.end())
                return it->second;
            else {
                dyn_pool_item* item = new dyn_pool_item;
                item->item = as_pool(string);
                return dynamic_constant_pool_map[string.hash()] = item;
            }
        }

        dyn_pool_item* dynamic_constant_pool(const char* string) {
            return dynamic_constant_pool(string, strlen(string));
        }

        dyn_pool_item* dynamic_constant_pool(const char8_t* string) {
            return dynamic_constant_pool((char*)string, strlen((char*)string));
        }

        dyn_pool_item* dynamic_constant_pool(const char16_t* string) {
            return dynamic_constant_pool(string, std::char_traits<char16_t>::length(string));
        }

        dyn_pool_item* dynamic_constant_pool(const char32_t* string) {
            return dynamic_constant_pool(string, std::char_traits<char32_t>::length(string));
        }

        dyn_pool_item* dynamic_constant_pool(const char* string, size_t size) {
            if (!utf8::is_valid(string, string + size))
                throw InvalidEncodingException("String is not valid utf8");

            size_t hash = art::hash<char>()(string, size);
            art::lock_guard<TaskMutex> lock(dynamic_constant_pool_mutex);
            auto it = dynamic_constant_pool_map.find(hash);
            if (it != dynamic_constant_pool_map.end())
                return it->second;
            else {
                dyn_pool_item* item = new dyn_pool_item;
                item->item = as_pool(string, size, hash);
                return dynamic_constant_pool_map[hash] = item;
            }
        }

        dyn_pool_item* dynamic_constant_pool(const char8_t* string, size_t size) {
            return dynamic_constant_pool((char*)string, size);
        }

        dyn_pool_item* dynamic_constant_pool(const char16_t* string, size_t size) {
            std::string utf8_string;
            utf8_string.reserve(size);
            try {
                utf8::utf16to8(string, string + size, std::back_inserter(utf8_string));
            } catch (const utf8::invalid_utf16& e) {
                throw InvalidEncodingException("String is not valid utf16");
            }
            return dynamic_constant_pool((char*)utf8_string.c_str(), utf8_string.size());
        }

        dyn_pool_item* dynamic_constant_pool(const char32_t* string, size_t size) {
            std::string utf8_string;
            utf8_string.reserve(size);
            try {
                utf8::utf32to8(string, string + size, std::back_inserter(utf8_string));
            } catch (const utf8::invalid_code_point& e) {
                throw InvalidEncodingException("String is not valid utf32");
            }
            return dynamic_constant_pool((char*)utf8_string.c_str(), utf8_string.size());
        }

        std::unordered_map<size_t, pool_item*, art::hash<size_t>> constant_pool_map;
        TaskMutex constant_pool_mutex;

        pool_item* make_constant_pool(const ustring& string) {
            size_t hash = string.hash();
            art::lock_guard<TaskMutex> lock(constant_pool_mutex);
            auto it = constant_pool_map.find(hash);
            if (it != constant_pool_map.end())
                return it->second;
            else
                return constant_pool_map[hash] = as_pool(string);
        }

        pool_item* make_constant_pool(const char* string) {
            return make_constant_pool(string, strlen(string));
        }

        pool_item* make_constant_pool(const char8_t* string) {
            return make_constant_pool((char*)string);
        }

        pool_item* make_constant_pool(const char16_t* string) {
            return make_constant_pool(string, std::char_traits<char16_t>::length(string));
        }

        pool_item* make_constant_pool(const char32_t* string) {
            return make_constant_pool(string, std::char_traits<char32_t>::length(string));
        }

        pool_item* make_constant_pool(const char* string, size_t size) {
            if (!utf8::is_valid(string, string + size))
                throw InvalidEncodingException("String is not valid utf8");

            size_t hash = art::hash<char>()(string, size);
            art::lock_guard<TaskMutex> lock(constant_pool_mutex);
            auto it = constant_pool_map.find(hash);
            if (it != constant_pool_map.end())
                return it->second;
            else
                return constant_pool_map[hash] = as_pool(string, size, hash);
        }

        pool_item* make_constant_pool(const char8_t* string, size_t size) {
            return make_constant_pool((char*)string, size);
        }

        pool_item* make_constant_pool(const char16_t* string, size_t size) {
            std::string utf8_string;
            utf8_string.reserve(size);
            try {
                utf8::utf16to8(string, string + size, std::back_inserter(utf8_string));
            } catch (const utf8::invalid_utf16& e) {
                throw InvalidEncodingException("String is not valid utf16");
            }
            return make_constant_pool((char*)utf8_string.c_str(), utf8_string.size());
        }

        pool_item* make_constant_pool(const char32_t* string, size_t size) {
            std::string utf8_string;
            utf8_string.reserve(size);
            try {
                utf8::utf32to8(string, string + size, std::back_inserter(utf8_string));
            } catch (const utf8::invalid_code_point& e) {
                throw InvalidEncodingException("String is not valid utf32");
            }
            return make_constant_pool((char*)utf8_string.c_str(), utf8_string.size());
        }
    };

    void ustring::cleanup_current() {
        switch (flags.type) {
        case ustring::Type::def:
            _data.d.larr.~list_array<char>();
            break;
        case ustring::Type::dyn_constant:
            _data.dynamic_constant_data.~shared_ptr();
            break;
        case ustring::Type::constant:
            _data.constant_data = nullptr;
            break;
        case ustring::Type::short_def:
            _data.short_data[0] = 0;
            break;
        default:
            break;
        }
        flags.type = ustring::Type::short_def;
        _data.short_data[0] = 0;
        flags.has_data = false;
    }

    void ustring::switch_to_type(ustring::Type type) {
        if (flags.type == type)
            return;
        switch (type) {
        case ustring::Type::def: {
            list_array<char> __data;
            size_t _hash = flags.has_hash ? hash() : 0;
            __data.push_back(c_str(), size());
            cleanup_current();
            new (&_data.d.larr) list_array<char>(std::move(__data));
            if (flags.has_hash)
                _data.d.hash = _hash;
            break;
        }
        case ustring::Type::short_def: {
            size_t _size = this->size();
            if (_size >= short_array_size)
                throw InvalidOperation("String is too long for short_def");
            std::unique_ptr<char[]> __data(new char[_size]);
            memcpy(__data.get(), c_str(), _size);
            cleanup_current();
            memcpy(_data.short_data, __data.get(), _size);
            _data.short_data[_size] = 0;
            flags.has_hash = false;
            break;
        }
        case ustring::Type::constant: {
            auto _constant_data = constant_pool::make_constant_pool(*this);
            cleanup_current();
            _data.constant_data = _constant_data;
            flags.has_hash = true;
            break;
        }
        case ustring::Type::dyn_constant: {
            auto _constant_data = constant_pool::dynamic_constant_pool(*this);
            cleanup_current();
            _data.dynamic_constant_data = _constant_data;
            flags.has_hash = true;
            break;
        }
        default:
            throw InvalidType("String type is unrecognized");
        }
        flags.type = type;
    }

    char* ustring::direct_ptr() {
        switch (flags.type) {
        case ustring::Type::def:
            return _data.d.larr.data();
        case ustring::Type::short_def:
            return _data.short_data;
        case ustring::Type::constant:
            return _data.constant_data->from_constant_pool;
        case ustring::Type::dyn_constant:
            return _data.dynamic_constant_data->item->from_constant_pool;
        default:
            throw InvalidType("String type is unrecognized");
        }
    }

    void ustring::set_unsafe_state(bool state, bool throw_on_error) {
        if (flags.as_unsafe && !state) {
            if (throw_on_error) {
                if (!utf8::is_valid(c_str(), c_str() + size()))
                    throw InvalidEncodingException("String is not valid utf8");
            } else {
                std::string res;
                utf8::replace_invalid(c_str(), c_str() + size(), std::back_inserter(res));
                *this = ustring(res.c_str(), res.size());
            }
        }
        flags.as_unsafe = state;
    }

    ustring::ustring() {
        flags.type = ustring::Type::short_def;
        flags.has_hash = false;
        flags.has_length = false;
        flags.has_data = false;
        flags.as_unsafe = false;
    }

    ustring::ustring(const char* str)
        : ustring(str, strlen(str)) {}

    ustring::ustring(const char* str, size_t size)
        : ustring() {
        if (size < short_array_size) {
            utf8::replace_invalid(str, str + size, _data.short_data);
            _data.short_data[size] = 0;
            flags.has_data = true;
        } else {
            flags.has_data = true;
            flags.type = ustring::Type::def;
            new (&_data.d.larr) list_array<char>();
            _data.d.larr.reserve_push_back(size);
            utf8::replace_invalid(str, str + size, std::back_inserter(_data.d.larr));
        }
    }

    ustring::ustring(const char8_t* str)
        : ustring((char*)str, strlen((char*)str)) {}

    ustring::ustring(const char8_t* str, size_t size)
        : ustring((char*)str, size) {}

    ustring::ustring(const char16_t* str)
        : ustring(str, std::char_traits<char16_t>::length(str)) {}

    ustring::ustring(const char16_t* str, size_t size)
        : ustring() {
        std::string res;
        utf8::utf16to8(str, str + size, std::back_inserter(res));
        *this = ustring(res.c_str(), res.size());
    }

    ustring::ustring(const char32_t* str)
        : ustring(str, std::char_traits<char32_t>::length(str)) {}

    ustring::ustring(const char32_t* str, size_t size)
        : ustring() {
        std::string res;
        utf8::utf32to8(str, str + size, std::back_inserter(res));
        *this = ustring(res.c_str(), res.size());
    }

    ustring::ustring(char str, size_t count)
        : ustring() {
        set_unsafe_state(true, false);
        if (count < short_array_size) {
            memset(_data.short_data, str, count);
            _data.short_data[count] = 0;
            flags.has_data = true;
        } else {
            flags.has_data = true;
            flags.type = ustring::Type::def;
            new (&_data.d.larr) list_array<char>();
            _data.d.larr.resize(count + 1);
            memset(_data.d.larr.data(), str, count);
            _data.d.larr.data()[count] = 0;
        }
        set_unsafe_state(false, false);
    }

    ustring::ustring(char8_t str, size_t count)
        : ustring((char)str, count) {}

    ustring::ustring(char16_t str, size_t count)
        : ustring() {
        for (size_t i = 0; i < count; i++)
            *this += str;
    }

    ustring::ustring(char32_t str, size_t count)
        : ustring() {
        for (size_t i = 0; i < count; i++)
            *this += str;
    }

    ustring::ustring(const ustring& str)
        : ustring() {
        *this = str;
    }

    ustring::ustring(const std::string& str)
        : ustring(str.c_str(), str.size()) {}

    ustring::ustring(const std::u16string& str)
        : ustring(str.c_str(), str.size()) {}

    ustring::ustring(const std::u32string& str)
        : ustring(str.c_str(), str.size()) {}

    ustring::ustring(ustring&& str) noexcept {
        *this = std::move(str);
    }
#ifdef PLATFORM_WINDOWS
    ustring::ustring(const std::wstring& str)
        : ustring((char16_t*)str.c_str(), str.size()) {}

    ustring::ustring(wchar_t* str)
        : ustring((char16_t*)str) {}

    ustring::ustring(wchar_t* str, size_t len)
        : ustring((char16_t*)str, len) {}
#endif
    ustring::~ustring() {
        cleanup_current();
    }

    ustring& ustring::operator=(const ustring& str) {
        if (&str == this)
            return *this;
        if (flags.type != str.flags.type)
            cleanup_current();
        switch (str.flags.type) {
        case ustring::Type::def:
            _data.d.larr = str._data.d.larr;
            _data.d.hash = str._data.d.hash;
            _data.d.symbols = str._data.d.symbols;
            break;
        case ustring::Type::short_def:
            memcpy(_data.short_data, str._data.short_data, short_array_size);
            break;
        case ustring::Type::constant:
            _data.constant_data = str._data.constant_data;
            break;
        case ustring::Type::dyn_constant:
            _data.dynamic_constant_data = str._data.dynamic_constant_data;
        default:
            assert(false && "String type is unrecognized");
            break;
        }
        bool need_check = !flags.as_unsafe && str.flags.as_unsafe;
        flags = str.flags;
        if (need_check)
            set_unsafe_state(false, false);
        return *this;
    }

    ustring& ustring::operator=(ustring&& str) noexcept {
        if (&str == this)
            return *this;
        if (flags.type != str.flags.type)
            cleanup_current();
        switch (str.flags.type) {
        case ustring::Type::def:
            _data.d.larr = std::move(str._data.d.larr);
            _data.d.hash = str._data.d.hash;
            _data.d.symbols = str._data.d.symbols;
            break;
        case ustring::Type::short_def:
            memcpy(_data.short_data, str._data.short_data, short_array_size);
            memset(str._data.short_data, 0, short_array_size);
            break;
        case ustring::Type::constant:
            _data.constant_data = str._data.constant_data;
            break;
        case ustring::Type::dyn_constant:
            _data.dynamic_constant_data = std::move(str._data.dynamic_constant_data);
        default:
            assert(false && "String type is unrecognized");
            break;
        }
        flags = str.flags;
        str.cleanup_current();
        return *this;
    }

    bool ustring::operator==(const ustring& str) const {
        return hash() == str.hash();
    }

    bool ustring::operator!=(const ustring& str) const {
        return hash() != str.hash();
    }

    bool ustring::operator<(const ustring& str) const {
        return operator<=>(str) == std::strong_ordering::less;
    }

    bool ustring::operator>(const ustring& str) const {
        return operator<=>(str) == std::strong_ordering::greater;
    }

    bool ustring::operator<=(const ustring& str) const {
        return !(*this > str);
    }

    bool ustring::operator>=(const ustring& str) const {
        return !(*this < str);
    }

    std::strong_ordering ustring::operator<=>(const ustring& str) const {
        auto self_size = size();
        auto other_size = str.size();
        auto begin_self = c_str();
        auto end_self = begin_self + self_size;
        auto begin_other = str.c_str();
        auto end_other = begin_other + other_size;
        if (self_size > other_size)
            return std::strong_ordering::greater;
        if (self_size < other_size)
            return std::strong_ordering::less;

        auto result = std::lexicographical_compare_three_way(
            begin_self,
            end_self,
            begin_other,
            end_other
        );
        if (result != std::strong_ordering::equal)
            return result;
        return std::strong_ordering::equal;
    }

    ustring& ustring::operator+=(const ustring& str) {
        size_t old_size = size();
        if (flags.type == ustring::Type::dyn_constant) {
            char* new_data = new char[old_size + str.size()];
            memcpy(new_data, direct_ptr(), old_size);
            memcpy(new_data + old_size, str.c_str(), str.size());
            _data.dynamic_constant_data = constant_pool::dynamic_constant_pool(new_data, old_size + str.size());
            delete[] new_data;
        } else if (flags.type == ustring::Type::short_def) {
            if (old_size + str.size() < short_array_size) {
                memcpy(_data.short_data + old_size, str.c_str(), str.size());
                _data.short_data[old_size + str.size()] = 0;
            } else {
                switch_to_type(ustring::Type::def);
                _data.d.larr.push_back(str.c_str(), str.size());
            }
        } else {
            switch_to_type(ustring::Type::def);
            _data.d.larr.push_back(str.c_str(), str.size());
        }
        if (!str.empty()) {
            flags.has_hash = false;
            flags.has_data = true;
        }
        return *this;
    }

    ustring ustring::operator+(const ustring& str) const {
        return ustring(*this) += str;
    }

    ustring& ustring::operator-=(const ustring& str) {
        if (flags.type == ustring::Type::dyn_constant) {
            char* new_data = new char[size() + str.size()];
            memcpy(new_data, str.c_str(), str.size());
            memcpy(new_data + str.size(), direct_ptr(), size());
            _data.dynamic_constant_data = constant_pool::dynamic_constant_pool(new_data, size() + str.size());
            delete[] new_data;
        } else if (flags.type == ustring::Type::short_def) {
            size_t old_size = size();
            if (old_size + str.size() < short_array_size) {
                memmove(_data.short_data + str.size(), _data.short_data, old_size);
                memcpy(_data.short_data, str.c_str(), str.size());
                _data.short_data[old_size + str.size()] = 0;
            } else {
                switch_to_type(ustring::Type::def);
                _data.d.larr.push_front(str.c_str(), str.size());
            }
        } else {
            switch_to_type(ustring::Type::def);
            _data.d.larr.push_front(str.c_str(), str.size());
        }
        if (!str.empty()) {
            flags.has_hash = false;
            flags.has_data = true;
        }
        return *this;
    }

    ustring ustring::operator-(const ustring& str) const {
        return ustring(str) += *this;
    }

    const char* ustring::c_str() const {
        return const_cast<ustring*>(this)->direct_ptr();
    }

    char* ustring::data() {
        if (flags.type == ustring::Type::dyn_constant || flags.type == ustring::Type::constant)
            if (!flags.as_unsafe)
                switch_to_type(ustring::Type::def);
        return const_cast<ustring*>(this)->direct_ptr();
    }

    size_t ustring::length() const {
        switch (flags.type) {
        case ustring::Type::def: {
            if (flags.has_length)
                return _data.d.symbols;
            else {
                ptrdiff_t symbols_ = utf8::distance(c_str(), c_str() + size());
                const_cast<size_t&>(_data.d.symbols) = (size_t)symbols_;
                const_cast<decltype(flags)&>(flags).has_length = true;
                return _data.d.symbols;
            }
        }
        case ustring::Type::short_def: {
            ptrdiff_t symbols_ = utf8::distance(c_str(), c_str() + size());
            return (size_t)symbols_;
        }
        case ustring::Type::constant:
            return _data.constant_data->symbols;
        case ustring::Type::dyn_constant:
            return _data.dynamic_constant_data->item->symbols;
        default:
            throw InvalidType("String type is unrecognized");
        }
    }

    size_t ustring::size() const {
        switch (flags.type) {
        case ustring::Type::def:
            return _data.d.larr.size();
        case ustring::Type::short_def:
            return strlen(_data.short_data);
        case ustring::Type::constant:
            return _data.constant_data->size;
        case ustring::Type::dyn_constant:
            return _data.dynamic_constant_data->item->size;
        default:
            throw InvalidType("String type is unrecognized");
        }
    }

    size_t ustring::hash() const {
        switch (flags.type) {
        case ustring::Type::def:
            return flags.has_hash ? _data.d.hash : const_cast<size_t&>(_data.d.hash) = art::hash<char>()(c_str(), _data.d.larr.size());
        case ustring::Type::short_def:
            return art::hash<char>()(_data.short_data, strlen(_data.short_data));
        case ustring::Type::constant:
            return _data.constant_data->hash;
        case ustring::Type::dyn_constant:
            return _data.dynamic_constant_data->item->hash;
        default:
            throw InvalidType("String type is unrecognized");
        }
    }

    size_t ustring::hash(uint32_t seed) const {
        switch (flags.type) {
        case ustring::Type::def:
            return art::hash<char>().seed_hash_array(c_str(), _data.d.larr.size(), seed);
        case ustring::Type::short_def:
            return art::hash<char>().seed_hash_array(_data.short_data, strlen(_data.short_data), seed);
        case ustring::Type::constant:
            return art::hash<char>().seed_hash_array(_data.constant_data->from_constant_pool, _data.constant_data->size, seed);
        case ustring::Type::dyn_constant:
            return art::hash<char>().seed_hash_array(_data.dynamic_constant_data->item->from_constant_pool, _data.dynamic_constant_data->item->size, seed);
        default:
            throw InvalidType("String type is unrecognized");
        }
    }

    bool ustring::empty() const {
        if (!flags.has_data)
            return true;
        bool has_data;
        switch (flags.type) {
        case ustring::Type::def:
            has_data = _data.d.larr.empty();
            break;
        case ustring::Type::short_def:
            has_data = _data.short_data[0] != 0;
            break;
        case ustring::Type::constant:
            has_data = _data.constant_data->from_constant_pool[0] != 0;
            break;
        case ustring::Type::dyn_constant:
            has_data = _data.dynamic_constant_data->item->from_constant_pool[0] != 0;
            break;
        default:
            throw InvalidType("String type is unrecognized");
        }
        return !(const_cast<decltype(flags)&>(flags).has_data = has_data);
    }

    char32_t ustring::get(size_t pos) const {
        const char* begin = c_str();
        const char* end = c_str() + size();

        for (size_t i = 0; i < pos; i++) {
            if (begin == end)
                throw OutOfRange("String symbol index out of range");
            utf8::next(begin, end);
        }
        if (begin == end)
            throw OutOfRange("String symbol index out of range");
        return (char32_t)utf8::next(begin, end);
    }

    char ustring::get_ansi(size_t pos) {
        const char* begin = c_str();
        const char* end = c_str() + size();

        for (size_t i = 0; i < pos; i++) {
            if (begin == end)
                throw OutOfRange("String symbol index out of range");
            utf8::next(begin, end);
        }
        if (begin == end)
            throw OutOfRange("String symbol index out of range");
        uint32_t res = utf8::next(begin, end);
        if (res > 0x7F)
            throw InvalidEncodingException("Symbol is not ansi7");
        return (char)res;
    }

    char ustring::get_codepoint(size_t pos) {
        if (pos >= size())
            throw OutOfRange("String codepoint index out of range");
        return c_str()[pos];
    }

    void ustring::set(size_t pos, char32_t c) {
        char* begin = direct_ptr();
        char* end = direct_ptr() + size();

        for (size_t i = 0; i < pos; i++) {
            if (begin == end)
                throw OutOfRange("String symbol index out of range");
            utf8::next(begin, end);
        }
        if (begin == end)
            throw OutOfRange("String symbol index out of range");

        char* old_begin = begin;
        utf8::next(begin, end);
        size_t remove_points = begin - old_begin;
        char buff[9]{0};
        utf8::utf32to8(&c, &c + 1, buff);
        size_t insert_points = strlen(buff);
        if (insert_points == remove_points)
            memcpy(begin, buff, insert_points);
        else {
            if (flags.type != ustring::Type::dyn_constant) {
                switch_to_type(ustring::Type::def);
                _data.d.larr.remove(begin - direct_ptr(), remove_points);
                _data.d.larr.insert(begin - direct_ptr(), buff, insert_points);
                flags.has_hash = false;
            } else {
                char* new_data = new char[size() - remove_points + insert_points];
                memcpy(new_data, direct_ptr(), begin - direct_ptr());
                memcpy(new_data + (begin - direct_ptr()), buff, insert_points);
                memcpy(new_data + (begin - direct_ptr()) + insert_points, begin, size() - (begin - direct_ptr()));
                _data.dynamic_constant_data = constant_pool::dynamic_constant_pool(new_data, size() - remove_points + insert_points);
                delete[] new_data;
            }
        }
    }

    void ustring::set_ansi(size_t pos, char c) {
        char* begin = direct_ptr();
        char* end = direct_ptr() + size();

        for (size_t i = 0; i < pos; i++) {
            if (begin == end)
                throw OutOfRange("String symbol index out of range");
            utf8::next(begin, end);
        }
        if (begin == end)
            throw OutOfRange("String symbol index out of range");

        char* old_begin = begin;
        utf8::next(begin, end);
        size_t remove_points = begin - old_begin;
        if (1 == remove_points)
            *begin = c;
        else {
            if (flags.type != ustring::Type::dyn_constant) {
                switch_to_type(ustring::Type::def);
                _data.d.larr.remove(begin - direct_ptr(), remove_points);
                _data.d.larr.insert(begin - direct_ptr(), &c, 1);
                flags.has_hash = false;
            } else {
                char* new_data = new char[size() - remove_points + c];
                memcpy(new_data, direct_ptr(), begin - direct_ptr());
                memcpy(new_data + (begin - direct_ptr()), &c, 1);
                memcpy(new_data + (begin - direct_ptr()) + 1, begin, size() - (begin - direct_ptr()));
                _data.dynamic_constant_data = constant_pool::dynamic_constant_pool(new_data, size() - remove_points + 1);
                delete[] new_data;
            }
        }
    }

    void ustring::set_codepoint(size_t pos, char c) {
        if (size() <= pos)
            throw OutOfRange("String codepoint index out of range");
        direct_ptr()[pos] = c;
        if (!flags.as_unsafe) {
            flags.as_unsafe = true;
            set_unsafe_state(false, true);
        }
    }

    array_t<uint32_t> ustring::as_array_32() {
        list_array<uint32_t> res;
        utf8::utf8to32(c_str(), c_str() + size(), std::back_inserter(res));
        size_t len;
        uint32_t* arr = res.take_raw(len);
        return array_t<uint32_t>(len, arr);
    }

    array_t<uint16_t> ustring::as_array_16() {
        list_array<uint16_t> res;
        utf8::utf8to16(c_str(), c_str() + size(), std::back_inserter(res));
        size_t len;
        uint16_t* arr = res.take_raw(len);
        return array_t<uint16_t>(len, arr);
    }

    array_t<uint8_t> ustring::as_array_8() {
        return array_t<uint8_t>((uint8_t*)c_str(), size());
    }

    ustring::operator std::string() const {
        return std::string(c_str(), size());
    }

    ustring::operator std::u8string() const {
        return std::u8string((char8_t*)c_str(), size());
    }

    ustring::operator std::u16string() const {
        std::u16string res;
        utf8::utf8to16(c_str(), c_str() + size(), std::back_inserter(res));
        return res;
    }

    ustring::operator std::u32string() const {
        std::u32string res;
        utf8::utf8to32(c_str(), c_str() + size(), std::back_inserter(res));
        return res;
    }

    char* ustring::begin() {
        return data();
    }

    char* ustring::end() {
        return data() + size();
    }

    const char* ustring::begin() const {
        return c_str();
    }

    const char* ustring::end() const {
        return c_str() + size();
    }

    const char* ustring::cbegin() const {
        return c_str();
    }

    const char* ustring::cend() const {
        return c_str() + size();
    }

    void ustring::reserve(size_t siz) {
        switch_to_type(ustring::Type::def);
        _data.d.larr.reserve_push_back(siz);
    }

    void ustring::reserve_push_back(size_t siz) {
        switch_to_type(ustring::Type::def);
        _data.d.larr.reserve_push_back(siz);
    }

    void ustring::reserve_push_front(size_t siz) {
        switch_to_type(ustring::Type::def);
        _data.d.larr.reserve_push_front(siz);
    }

    void ustring::push_back(const ustring& c) {
        operator+=(c);
    }

    void ustring::push_front(const ustring& c) {
        operator-=(c);
    }

    void ustring::append(const ustring& c) {
        switch_to_type(ustring::Type::def);
        operator+=(c);
    }

    void ustring::append(const char* str, size_t len) {
        switch_to_type(ustring::Type::def);
        if (!flags.as_unsafe)
            if (!utf8::is_valid(str, str + len))
                throw InvalidEncodingException("String is not valid utf8");
        _data.d.larr.push_back(str, len);
        if (len) {
            flags.has_data = true;
            flags.has_hash = false;
        }
    }

    void ustring::append(const char8_t* str, size_t len) {
        switch_to_type(ustring::Type::def);
        if (!flags.as_unsafe)
            if (!utf8::is_valid(str, str + len))
                throw InvalidEncodingException("String is not valid utf8");
        _data.d.larr.push_back((const char*)str, len);
        if (len)
            flags.has_data = true;
    }

    void ustring::append(const char16_t* str, size_t len) {
        switch_to_type(ustring::Type::def);
        try {
            utf8::utf16to8(str, str + len, std::back_inserter(_data.d.larr));
            if (len) {
                flags.has_data = true;
                flags.has_hash = false;
            }
        } catch (const utf8::invalid_utf16& ex) {
            throw InvalidEncodingException("String is not valid utf16, fail cast to utf8");
        }
    }

    void ustring::append(const char32_t* str, size_t len) {
        switch_to_type(ustring::Type::def);
        try {
            utf8::utf32to8(str, str + len, std::back_inserter(_data.d.larr));
            if (len) {
                flags.has_data = true;
                flags.has_hash = false;
            }
        } catch (const utf8::invalid_code_point& ex) {
            throw InvalidEncodingException("String is not valid utf32, fail cast to utf8");
        }
    }

    void ustring::append(const char* str) {
        append(str, strlen(str));
    }

    void ustring::append(const char8_t* str) {
        append(str, std::char_traits<char8_t>::length(str));
    }

    void ustring::append(const char16_t* str) {
        append(str, std::char_traits<char16_t>::length(str));
    }

    void ustring::append(const char32_t* str) {
        append(str, std::char_traits<char32_t>::length(str));
    }

    void ustring::insert(size_t pos, const ustring& c) {
        switch_to_type(ustring::Type::def);
        _data.d.larr.insert(pos, c.c_str(), c.size());
        flags.has_hash = false;
        flags.has_data = true;
    }

    void ustring::resize(size_t len) {
        switch (flags.type) {
        case ustring::Type::short_def: {
            if (len <= short_array_size) {
                size_t old_len = size();
                if(old_len < len)
                    memset(_data.short_data + old_len, ' ', len - old_len);
                _data.short_data[len - 1] = 0;
                break;
            }
            [[fallthrough]];
        }
        default:
            switch_to_type(ustring::Type::def);
            [[fallthrough]];
        case ustring::Type::def:
            _data.d.larr.resize(len);
            flags.has_hash = false;
            break;
        }
        flags.has_data = (bool)len;
    }

    bool ustring::starts_with(const ustring& c) const {
        size_t self_siz = size();
        size_t c_siz = c.size();
        if (self_siz < c_siz)
            return false;
        const char* self_ = begin();
        const char* c_ = c.begin();
        while (c_siz) {
            if (*self_ != *c_)
                return false;
            c_siz--;
            self_++;
            c_++;
        }
        return true;
    }

    bool ustring::ends_with(const ustring& c) const {
        size_t self_siz = size();
        size_t c_siz = c.size();
        if (self_siz < c_siz)
            return false;
        const char* self_ = end();
        const char* c_ = c.end();
        self_--;
        c_--;
        while (c_siz) {
            if (*self_ != *c_)
                return false;
            c_siz--;
            self_--;
            c_--;
        }
        return true;
    }

    ustring ustring::substr(size_t pos, size_t len) const {
        size_t self_siz = size();
        if (pos >= self_siz)
            throw OutOfRange("String begin index out of range");
        if (pos + len > self_siz)
            throw OutOfRange("String end index out of range");
        return ustring(c_str() + pos, len);
    }

    ustring ustring::substr(size_t pos) const {
        size_t self_siz = size();
        if (pos >= self_siz)
            throw OutOfRange("String begin index out of range");
        return ustring(c_str() + pos, self_siz - pos);
    }

    void ustring::clear() {
        cleanup_current();
    }

    void ustring::shrink_to_fit() {
        if (flags.type == ustring::Type::def)
            _data.d.larr.shrink_to_fit();
    }

    size_t ustring::find_last_of(const ustring& c) const {
        size_t self_siz = size();
        size_t c_siz = c.size();
        if (self_siz < c_siz)
            return npos;
        const char* self_begin = begin();
        const char* self_end = end();

        const char* c_begin = c.begin();
        const char* c_end = c.end();

        const char* self_ = self_end - c_siz;
        while (self_ >= self_begin) {
            if (std::equal(c_begin, c_end, self_))
                return self_ - self_begin;
            self_--;
        }
        return npos;
    }

    size_t ustring::find_first_of(const ustring& c) const {
        size_t self_siz = size();
        size_t c_siz = c.size();
        if (self_siz < c_siz)
            return npos;
        const char* self_begin = begin();
        const char* self_end = end();

        const char* c_begin = c.begin();
        const char* c_end = c.end();

        const char* self_ = self_begin;
        while (self_ <= self_end - c_siz) {
            if (std::equal(c_begin, c_end, self_))
                return self_ - self_begin;
            self_++;
        }
        return npos;
    }
}