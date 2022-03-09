#pragma once
#include <atomic>
#include <unordered_set>

extern thread_local std::unordered_set<const void*> __lgr_safe_deph;

class lgr {
	bool(*calc_depth)(void*);
	void(*destructor)(void*);
	void* ptr;
	std::atomic_size_t* total;
	std::atomic_size_t* weak;
	bool in_safe_deph;

	void exit() {
		if (total == nullptr);
		else if (!in_safe_deph) {
			--(*weak);
			if (!*weak && !*total) {
				delete total;
				delete weak;
			}
		}
		else if (*total > 1) {
			--(*total);
			return;
		}
		else {
			if (ptr)
				if (destructor) destructor(ptr);
			if (!*weak) {
				delete total;
				delete weak;
			}
		}
		ptr = nullptr;
		total = nullptr;
		weak = nullptr;
	}
	void join(std::atomic_size_t* p_total_links, std::atomic_size_t* tot_weak) {
		total = p_total_links;
		weak = tot_weak;
		if (p_total_links && (in_safe_deph = calcDeph()))
			++(*p_total_links);
		else if (!in_safe_deph && tot_weak)
			++(*tot_weak);
	}
public:
	lgr() {
		calc_depth = nullptr;
		destructor = nullptr;
		ptr = nullptr;
		total = nullptr;
		weak = nullptr;
		in_safe_deph = false;
	}
	lgr(void* copy, bool(*clc_depth)(void*) = nullptr, void(*destruct)(void*) = nullptr, bool as_weak = false) : calc_depth(clc_depth), destructor(destruct) {
		if (copy) {
			ptr = copy;
			total = new std::atomic_size_t{ 1 };
			weak = new std::atomic_size_t{ 0 };
			if (!as_weak)
				in_safe_deph = calcDeph();
			else
				in_safe_deph = false;
			if (!in_safe_deph) {
				++(*weak);
				--(*total);
			}
		}
		else {
			ptr = nullptr;
			total = nullptr;
			weak = nullptr;
			in_safe_deph = false;
		}
	}
	lgr(const lgr& mov) {
		*this = mov;
	}
	lgr(lgr&& mov) noexcept {
		*this = std::move(mov);
	}
	~lgr() {
		if (total || in_safe_deph)
			exit();
	}
	lgr& operator=(const lgr& copy) {
		if (this == &copy)return *this;
		if (total && weak) exit();
		ptr = copy.ptr;
		calc_depth = copy.calc_depth;
		destructor = copy.destructor;
		join(copy.total, copy.weak);
		return *this;
	}
	lgr& operator=(lgr&& mov) noexcept {
		if (total && in_safe_deph)
			exit();
		ptr = mov.ptr;
		total = mov.total;
		weak = mov.weak;
		in_safe_deph = mov.in_safe_deph;
		calc_depth = mov.calc_depth;
		destructor = mov.destructor;
		mov.ptr = nullptr;
		mov.total = nullptr;
		mov.weak = nullptr;
		return *this;
	}
	void*& operator*() {
		if (total)
			if (!*total)
				ptr = nullptr;
		return ptr;
	}
	void** operator->() {
		if (total)
			if (!*total)
				ptr = nullptr;
		return &ptr;
	}
	void* getPtr() {
		if (total)
			if (!*total)
				ptr = nullptr;
		return ptr;
	}
	bool calcDeph() {
		bool res = depth_safety();
		__lgr_safe_deph.clear();
		return res;
	}
	bool depth_safety() const {
		if (__lgr_safe_deph.contains(ptr))
			return false;
		if (calc_depth) {
			__lgr_safe_deph.emplace(ptr);
			return calc_depth(ptr);
		}
		else
			return true;
	}
	bool is_deleted() const {
		if (in_safe_deph)
			return total;
		else if (total)
			return *total;
		else 
			return false;
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



template<typename, typename T>
struct has_depth_safety {
	static_assert(std::integral_constant<T, false>::value, "Second template parameter needs to be of function type.");
};

// specialization that does the checking
template<class C, class Ret, class... Args>
struct has_depth_safety<C, Ret(Args...)> {
private:
	template<typename T>
	static constexpr auto check(T*) -> typename std::is_same<decltype(std::declval<T>().depth_safety(std::declval<Args>()...)), Ret>::type { return true; }

	template<typename>
	static constexpr std::false_type check(...) { return false; }


public:
	typedef decltype(check<C>(nullptr)) type;
	static constexpr bool value = type::value;
};




template<class T, bool as_array = false>
class typed_lgr {
	lgr actual_lgr;

	template<typename = std::enable_if_t<has_depth_safety<T, bool()>::value>>
	static bool depth_calc(void* v) {
		return ((T*)v)->depth_safety();
	}
public:
	using depth_t = bool(*)(void*);
	static constexpr depth_t get_depth_calc() {
		if constexpr (has_depth_safety<T, bool()>::value)
			return &depth_calc;
		else
			return nullptr;
	}
	static void destruct(void* v) {
		if constexpr (as_array)
			delete[](T*)v;
		else
			delete (T*)v;
	}
	typed_lgr() {}
	typed_lgr(T* capture, bool as_weak = false) : actual_lgr(capture, get_depth_calc(), destruct, as_weak) { }
	typed_lgr(const typed_lgr& mov) noexcept {
		*this = mov;
	}
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
	bool depth_safety() const {
		return actual_lgr.depth_safety();
	}
	bool is_deleted() const {
		return actual_lgr.is_deleted();
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