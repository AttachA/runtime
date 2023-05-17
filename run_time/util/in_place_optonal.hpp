// Copyright Danyil Melnytskyi 2022-2023
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#ifndef RUN_TIME_UTIL_IN_PLACE_OPTONAL
#define RUN_TIME_UTIL_IN_PLACE_OPTONAL
#include <stdexcept>
//explicitly in place optional
template<typename T>
class i_p_optional {
    union no_construct {
        char dummy;
        T value;
        no_construct() : dummy() {}
        no_construct(const T& v) : value(v) {}
        no_construct(T&& v) : value(std::move(v)) {}

        ~no_construct() {}
    } v;
    bool contains_value;
public:
    i_p_optional() : contains_value(false) {}
    i_p_optional(const T& value) : v(value), contains_value(true) {}
    i_p_optional(T&& value) : v(std::move(value)), contains_value(true) {}
    i_p_optional(const i_p_optional& other) : contains_value(other.contains_value) {
        if (contains_value) 
            new (&v.value) T(other.value);
    }
    i_p_optional(i_p_optional&& other) : contains_value(other.contains_value) {
        if (contains_value) 
            new (&v.value) T(std::move(other.value));
    }
    ~i_p_optional() {
        if (contains_value) 
            v.value.~T();
    }
    i_p_optional& operator=(const T& value) {
        if (contains_value) 
            v.value = value;
        else {
            new (&v.value) T(value);
            contains_value = true;
        }
    }
    i_p_optional& operator=(T&& value) {
        if (contains_value) 
            v.value = std::move(value);
        else {
            new (&v.value) T(std::move(value));
            contains_value = true;
        }
    }
    i_p_optional& operator=(i_p_optional<T>&& other) {
        if (contains_value && other.contains_value) 
            v.value = std::move(other.v.value);
        else if (contains_value && !other.contains_value){
            v.value.~T();
            contains_value = false;
        }
        else {
            new (&v.value) T(std::move(other.v.value));
            contains_value = true;
        }
    }
    i_p_optional& operator=(const i_p_optional<T>& other) {
        if (contains_value && other.contains_value) 
            v.value = other.v.value;
        else if (contains_value && !other.contains_value){
            v.value.~T();
            contains_value = false;
        }
        else {
            new (&v.value) T(other.v.value);
            contains_value = true;
        }
    }
    T& operator*() {
        if(!contains_value)
            throw std::runtime_error("i_p_optional does not contain value");
        return v.value;
    }
    T* operator&(){
        if(contains_value)
            return &v.value;
        else
            return nullptr;
    }
    bool has_value() const {
        return contains_value;
    }
    template<typename... Args>
    void make_construct(Args... args){
        if(contains_value)
            throw std::runtime_error("i_p_optional already contains value");
        new (&v.value) T(std::forward<Args>(args)...);
        contains_value = true;
    }
};

#endif /* RUN_TIME_UTIL_IN_PLACE_OPTONAL */
