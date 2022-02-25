#pragma once
#include <atomic>
#include <unordered_set>

extern thread_local std::unordered_set<const void*> __lgr_safe_deph;

class lgr {
	bool(*calc_deph)(void*);
	void(*destructor)(void*);
	void* ptr;
	std::atomic_size_t* total;
	bool in_safe_deph;

	void exit() {
		if (*total > 1) (*total)--;
		else {
			if (ptr)
				if (destructor) destructor(ptr);
			ptr = nullptr;
			delete total;
			total = nullptr;
		}
	}
	void join(std::atomic_size_t* p_total_links) {
		if (total && in_safe_deph) exit();
		total = p_total_links;
		if (p_total_links && (in_safe_deph = calcDeph()))
			(*p_total_links)++;
	}
public:
	lgr(){
		calc_deph = nullptr;
		destructor = nullptr;
		ptr = nullptr;
		total = nullptr;
		in_safe_deph = false;
	}
	lgr(void* copy, bool(*clc_deph)(void*) = nullptr, void(*destruct)(void*) = nullptr) : calc_deph(clc_deph),destructor(destruct) {
		ptr = copy;
		total = new std::atomic_size_t{ 1 };
		in_safe_deph = calcDeph();
	}
	lgr(lgr&& mov) noexcept {
		*this = std::move(mov);
	}
	~lgr() {
		if (total && in_safe_deph) 
			exit();
	}
	lgr& operator=(const lgr& copy) {
		if (this == &copy)return *this;
		join(copy.total);
		ptr = copy.ptr;
		calc_deph = copy.calc_deph;
		destructor = copy.destructor;
		return *this;
	}
	lgr& operator=(lgr&& mov) noexcept {
		if (total && in_safe_deph) 
			exit();
		ptr = mov.ptr;
		total = mov.total;
		in_safe_deph = mov.in_safe_deph;
		calc_deph = mov.calc_deph;
		destructor = mov.destructor;
		mov.ptr = nullptr;
		mov.total = nullptr;
		return *this;
	}
	void*& operator*() {
		return ptr;
	}
	void** operator->() {
		return &ptr;
	}
	void* getPtr() {
		return ptr;
	}	
	bool calcDeph() {
		bool res = deph_safe();
		__lgr_safe_deph.clear();
		return res;
	}
	bool deph_safe() const {
		if (__lgr_safe_deph.contains(ptr))
			return false;
		if (calc_deph) {
			__lgr_safe_deph.emplace(ptr);
			return calc_deph(ptr);
		}
		else
			return true;
	}
	bool operator==(const lgr& cmp) const {
		return ptr == cmp.ptr;
	}
	bool operator!=(const lgr& cmp) const {
		return ptr != cmp.ptr;
	}
	operator bool() const {
		return ptr;
	}
	operator size_t() const {
		return (size_t)ptr;
	}
	operator ptrdiff_t() const {
		return (ptrdiff_t)ptr;
	}
};

template<class T, bool as_array = false>
class typed_lgr {
	lgr actual_lgr;
	static void destruct(void* v) {
		if constexpr (as_array)
			delete[] (T*)v;
		else 
			delete (T*)v;
	}
public:
	typed_lgr() {}
	typed_lgr(T* capture) : actual_lgr(capture,nullptr, destruct) {}
	typed_lgr(typed_lgr&& mov) noexcept {
		*this = std::move(mov);
	}
	typed_lgr& operator=(const typed_lgr& copy) {
		actual_lgr = copy.actual_lgr;
		return *this;
	}
	typed_lgr& operator=(typed_lgr&& mov) noexcept {
		actual_lgr = std::move(mov.actual_lgr);
		return *this;
	}
	T& operator*() {
		return *(T*)actual_lgr.getPtr();
	}
	T* operator->() {
		return (T*)actual_lgr.getPtr();
	}
	T* getPtr() {
		return (T*)actual_lgr.getPtr();
	}
	bool calcDeph() {
		return actual_lgr.calcDeph();
	}
	bool deph_safe() const {
		return actual_lgr.deph_safe();
	}
	bool operator==(const typed_lgr& cmp) const {
		return actual_lgr != cmp.actual_lgr;
	}
	bool operator!=(const typed_lgr& cmp) const {
		return actual_lgr != cmp.actual_lgr;
	}
	operator bool() const {
		return actual_lgr;
	}
	operator size_t() const {
		return actual_lgr;
	}
	operator ptrdiff_t() const {
		return actual_lgr;
	}
};