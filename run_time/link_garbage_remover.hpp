#pragma once
#include <atomic>
#include <unordered_set>
#include "../libray/list_array.hpp"

extern thread_local std::unordered_set<const void*> __lgr_safe_deph;

class lgr {
#if false
	list_array<std::vector<void*>*>* snap_records = nullptr;
	std::vector<void*>* current_snap = nullptr;
#define snap_rec_lgr_arg ,list_array<std::vector<void*>*>* snap_record
#define can_throw 
#else
#define snap_records
#define current_snap
#define snap_rec_lgr_arg
#define can_throw noexcept
#endif
	bool(*calc_depth)(void*);
	void(*destructor)(void*);
	void* ptr;
	std::atomic_size_t* total;
	std::atomic_size_t* weak;
	bool in_safe_deph;

	void exit();
	void join(std::atomic_size_t* p_total_links, std::atomic_size_t* tot_weak snap_rec_lgr_arg);
public:
	lgr();
	lgr(nullptr_t) : lgr(){};
	lgr(void* copy, bool(*clc_depth)(void*) = nullptr, void(*destruct)(void*) = nullptr, bool as_weak = false);
	lgr(const lgr& copy);
	lgr(lgr&& mov) can_throw;
	~lgr();
	lgr& operator=(nullptr_t);
	lgr& operator=(const lgr& copy);
	lgr& operator=(lgr&& mov) can_throw;
	void*& operator*();
	void** operator->();
	void* getPtr();
	bool calcDeph();
	bool depth_safety() const;
	bool is_deleted() const;
	bool operator==(const lgr& cmp) const;
	bool operator!=(const lgr& cmp) const;
	operator bool() const;
	operator size_t() const;
	operator ptrdiff_t() const;
};
#if !_DEBUG
#undef snap_records
#undef current_snap
#undef snap_rec_lgr_arg
#undef can_throw
#endif


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
	typed_lgr(nullptr_t) {};
	typed_lgr(T* capture, bool as_weak = false) : actual_lgr(capture, get_depth_calc(), destruct, as_weak) { }
	typed_lgr(const typed_lgr& mov) noexcept {
		*this = mov;
	}
	typed_lgr(typed_lgr&& mov) noexcept {
		*this = std::move(mov);
	}

	typed_lgr& operator=(nullptr_t) {
		actual_lgr = nullptr;
		return *this;
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