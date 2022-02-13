#pragma once
#include <stdint.h>
#include <utility>
#include "saloc.hpp"
template<class T>
T* cxxmalloc(size_t siz) {
	T* res = (T*)malloc(siz * sizeof(T));
	if (res == nullptr)
		throw std::bad_alloc();
	if constexpr (std::is_default_constructible<T>::value) new(res) T[siz]();
	return res;
}
template<class T>
auto cxxmalloc(size_t siz, const T& init) -> decltype(std::is_copy_assignable<T>::value) {
	T* res = (T*)malloc(siz * sizeof(T));
	if (res == nullptr)
		throw std::bad_alloc();
	for (size_t i = 0; i < siz; i++)
		res[i] = init;
	return res;
}
template<class T>
void cxxfree(T* val,size_t siz) {
	if constexpr(std::is_destructible<T>::value)
		for (size_t i = 0; i < siz; i++)
			val[i].~T();
	free(val);
}
template<class T>
auto cxxrealloc(T* val,size_t old_size,size_t new_size) {
	if (new_size == old_size)
		return val;
	if (old_size < new_size) {
		T* new_val = (T*)realloc(val, new_size);
		if constexpr (std::is_default_constructible<T>::value)
			for (size_t i = old_size; i < new_size; i++)
				new (&new_val[i])T();
		
		return new_val;
	}
	else {
		if constexpr (std::is_destructible<T>::value) {
			for (size_t i = new_size; i < old_size; i++)
				val[i].~T();
		}
		return (T*)realloc(val, new_size);
	}
}
template<class T>
auto cxxrealloc(T* val, size_t old_size, size_t new_size,const T& init) -> decltype(std::is_copy_assignable<T>::value) {
	if (new_size == old_size)
		return val;
	if (old_size < new_size) {
		T* new_val = (T*)realloc(val, new_size);
		for (size_t i = old_size; i < new_size; i++)
			new_val[i] = init;
		return new_val;
	}
	else {
		if constexpr (std::is_destructible<T>::value)
			for (size_t i = new_size; i < old_size; i++)
				val[i].~T();
		return (T*)realloc(val, new_size);
	}
}



template<class T>
class simple_arr {
	safe_shared_array<T> arr;
public:
	using value_type = T;
	simple_arr() {}
	simple_arr(size_t siz) {
		arr = safe_shared_array<T>(siz);
	}
	simple_arr(simple_arr& link) {
		arr = link.arr;
	}
	simple_arr(const simple_arr& copy) {
		size_t asize = copy.size();
		arr.resize(asize);
		for (size_t i = 0; i < asize; i++) 
			arr[i] = copy.arr[i];
	}
	simple_arr& operator=(const simple_arr& rhs) {
		size_t asize = rhs.size();
		arr.resize(asize);
		for (size_t i = 0; i < asize; i++)
			arr[i] = rhs.arr[i];
	}
	simple_arr& operator=(simple_arr&& rhs) {
		arr = rhs.arr;
	}
	size_t size() const {
		return arr.size();
	}
	void resize(size_t new_size) {
		arr.resize(new_size);
	}
	inline T& operator[](size_t pos) {
		return arr[pos];
	}
	inline const T& operator[](size_t pos) const {
		return arr[pos];
	}
	void push_back(T copy) {
		if (is_external())
			throw std::invalid_argument("This arr is external, cannont get size");
		arr.resize(arr.size() + 1);
		arr[arr.size() - 1] = copy;
	}
	T* begin() {
		return arr;
	}
	T* end() {
		return arr.is_external() ? nullptr : arr + size() - 1;
	}
	const T* begin() const{
		return arr;
	}
	const T* end() const {
		return arr.is_external() ? nullptr : arr + size() - 1;
	}
	void set_external(void* map) {
		arr = (T*)map;
	}
	bool is_external() const {
		return arr.is_external();
	}
};

template<>
class simple_arr<bool> {
	simple_arr<char> chars;
public:
	using value_type = bool;
	class boolean_proxy {
		simple_arr<char> protect;
		char* bool_point;
		char bool_index;
		friend class simple_arr<bool>;
		boolean_proxy(char* pos, size_t index, simple_arr<char> protection) {
			bool_point = pos;
			bool_index = (char)index;
			protect = protection;
		}
	public:
		boolean_proxy(boolean_proxy& proxy) {
			bool_point = proxy.bool_point;
			bool_index = proxy.bool_index;
			protect = proxy.protect;
		}
		boolean_proxy(boolean_proxy&& proxy) noexcept {
			bool_point = proxy.bool_point;
			bool_index = proxy.bool_index;
			protect = proxy.protect;
		}
		operator bool() const {
			return ((*bool_point) >> bool_index) & 1;
		}
		boolean_proxy& operator=(bool set_bol) {
			if (set_bol)
				(*bool_point) |= 1 << bool_index;
			else
				(*bool_point) &= ~(1 << bool_index);
			return *this;
		}
		boolean_proxy& operator++() {
			if (++bool_index == 8) {
				bool_index = 0;
				bool_point++;
			}
			return *this;
		}
		boolean_proxy operator++(int) {
			boolean_proxy tmp = *this;
			++* this;
			return tmp;
		}
		boolean_proxy& operator--() {
			if (bool_index == 0) {
				bool_index = 7;
				bool_point--;
			}
			else bool_index--;
			return *this;
		}
		boolean_proxy operator--(int) {
			boolean_proxy tmp = *this;
			--* this;
			return tmp;
		}
		bool operator==(const boolean_proxy& comparer) const {
			return bool_point == comparer.bool_point && bool_index == comparer.bool_index;
		}
		bool operator!=(const boolean_proxy& comparer) const {
			return !(*this == comparer);
		}
		boolean_proxy& operator*() { return *this; }
		boolean_proxy& operator->() { return *this; }
	};
	simple_arr() {

	}
	boolean_proxy operator[](size_t pos) {
		if (pos)
			return { chars.begin() + pos / 8, pos % 8,chars };
		return { chars.begin(),0,chars };
	}
	inline size_t size() {
		return chars.size() * 8;
	}
	void resize(size_t size) {
		chars.resize(size / 8 + bool(size % 8));
	}
	boolean_proxy begin() {
		return { chars.begin(),0 ,chars };
	}
	boolean_proxy end() {
		return
			chars.size() ?
			boolean_proxy(chars.end(), 7, chars) :
			boolean_proxy(chars.begin(), 0, chars);
	}
	inline void set_external(void* map) {
		chars.set_external(map);
	}
	bool is_external() {
		return chars.is_external();
	}
	char* orgBegin() {
		return chars.begin();
	}
	char* orgEnd() {
		return chars.end();
	}
};

