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
		if (total && in_safe_deph) {
			if (*total > 1) (*total)--;
			else {
				if (ptr)
					if (destructor) destructor(ptr);
				ptr = nullptr;
				delete total;
				total = nullptr;
			}
		}
	}
	void join(std::atomic_size_t* p_total_links) {
		if (total) exit();
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
		exit();
	}
	lgr& operator=(lgr& copy) {
		if (this == &copy)return *this;
		join(copy.total);
		ptr = copy.ptr;
		calc_deph = copy.calc_deph;
		destructor = copy.destructor;
		return *this;
	}
	lgr& operator=(lgr&& mov) noexcept {
		exit();
		ptr = mov.ptr;
		total = mov.total;
		in_safe_deph = mov.in_safe_deph;
		calc_deph = mov.calc_deph;
		destructor = mov.destructor;
		mov.ptr = nullptr;
		mov.total = nullptr;
		join(total);
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
		__lgr_safe_deph.emplace(ptr);
		return calc_deph ? calc_deph(ptr) : true;
	}
};