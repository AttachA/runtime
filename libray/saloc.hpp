#pragma once
#include <malloc.h>
#include <atomic>
#include <stdexcept>
//safe alocator
// allow crosplatform method get pointer size
// and match nullptr for 'sfree' and 'ssize'
// another same as malloc
template<class T>
T* salloc(size_t size) {
    void* ptr = malloc(size * sizeof(T) + sizeof(size_t));
    if (!ptr)return nullptr;
    *(size_t*)ptr = size;
    return (T*)(((size_t*)ptr) + 1);
}
template<class T>
T* srealloc(T* pointer, size_t size) {
    void* ptr = realloc(((((size_t*)pointer) - 1)), size * sizeof(T) + sizeof(size_t));
    if (!ptr)return nullptr;
    *(size_t*)ptr = size;
    return (T*)(((size_t*)ptr) + 1);
}
template<class T>
void sfree(T*& ptr) {
    if (ptr) {
        T* tmp = (T*)(((size_t*)ptr) - 1);
		ptr = nullptr;
        free(tmp);
    }
}
inline size_t ssize(void* pointer) {
    return pointer ? (*(((size_t*)pointer) - 1)) : 0;
}


template<class T>
class safe_shared_array {
	T* ptr = nullptr;
	std::atomic_size_t* total = nullptr;
	void exit() {
		if (total) {
			if (*total > 1) (*total)--;
			else {
				if (ptr)sfree(ptr);
				ptr = nullptr;
				delete total;
				total = nullptr;
			}
		}
		else
			ptr = nullptr;
	}
	void join(std::atomic_size_t*& p_total_links) {
		if (total) exit();
		if (p_total_links) {
			(*p_total_links)++;
			total = p_total_links;
		}
	}
public:
	safe_shared_array() {}
	safe_shared_array(size_t size) {
		exit();
		ptr = salloc<T>(size);
		total = new std::atomic_size_t{ 1 };
	}
	safe_shared_array(safe_shared_array& copy) {
		*this = copy;
	}
	~safe_shared_array() {
		exit();
	}
	safe_shared_array& operator=(safe_shared_array& copy) {
		if (this == &copy)return *this;
		join(copy.total);
		ptr = copy.ptr;
		return *this;
	}
	safe_shared_array& operator=(T* external) {
		if (ptr == external)return *this;
		exit();
		ptr = external;
		total = nullptr;
		return *this;
	}


	operator T* () {
		return ptr;
	}
	operator const T* () const{
		return ptr;
	}
	void free() {
		exit();
	}
	size_t size() const{
		return total? ssize(ptr) : 0;
	}
	void resize(size_t new_size) {
		if (!total)
			throw std::invalid_argument("fail resize external array");
		if (new_size == 0) free();
		size_t siz = size();
		T* new_arr = srealloc<T>(ptr, new_size);
		if (!new_arr)
			throw std::bad_alloc();
		for (size_t i = siz; i < new_size; i++)
			new_arr[i] = T();
		ptr = new_arr;
	}
	bool is_external() const {
		return !total;
	}
};