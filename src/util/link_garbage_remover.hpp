// Copyright Danyil Melnytskyi 2022-Present
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#pragma once
#include <atomic>
#include <unordered_set>

#include <library/list_array.hpp>

namespace art {
#define ENABLE_SNAPSHOTS_LGR false
    extern thread_local std::unordered_set<const void*> __lgr_safe_depth;

    class lgr {
#if ENABLE_SNAPSHOTS_LGR
        list_array<std::vector<void*>*>* snap_records = nullptr;
        std::vector<void*>* current_snap = nullptr;
#define snap_rec_lgr_arg , list_array<std::vector<void*>*>* snap_record
#define can_throw noexcept(false)
#else
#define snap_records
#define current_snap
#define snap_rec_lgr_arg
#define can_throw noexcept(true)
#endif
        bool (*calc_depth)(void*) = nullptr;
        void (*destructor)(void*) = nullptr;
        void* ptr = nullptr;
        std::atomic_size_t* total = nullptr;
        std::atomic_size_t* weak = nullptr;
        bool in_safe_depth = false;

        void exit();
        void join(std::atomic_size_t* p_total_links, std::atomic_size_t* tot_weak snap_rec_lgr_arg);

    public:
        lgr();
        lgr(std::nullptr_t)
            : lgr(){};
        lgr(void* copy, bool (*clc_depth)(void*) = nullptr, void (*destruct)(void*) = nullptr, bool as_weak = false);
        lgr(const lgr& copy);
        lgr(lgr&& mov) can_throw;
        ~lgr();
        lgr& operator=(std::nullptr_t);
        lgr& operator=(const lgr& copy);
        lgr& operator=(lgr&& mov) can_throw;
        void*& operator*();
        const void* const& operator*() const;
        void** operator->();
        const void* const* operator->() const;
        void* getPtr();
        const void* getPtr() const;
        void (*getDestructor(void) const)(void*);
        bool (*getCalcDepth(void) const)(void*);
        bool alone();
        void* try_take_ptr();
        bool calcDepth();
        bool depth_safety() const;
        bool is_deleted() const;

        lgr take() {
            return lgr(std::move(*this));
        }

        size_t totalLinks() const {
            return total ? total->load() : 0;
        }

        bool operator==(const lgr& cmp) const;
        bool operator!=(const lgr& cmp) const;
        operator bool() const;
        operator size_t() const;
        operator ptrdiff_t() const;

#if !ENABLE_SNAPSHOTS_LGR
#undef snap_records
#undef current_snap
#endif
#undef snap_rec_lgr_arg
#undef can_throw
    };

    template <typename, typename T>
    struct has_depth_safety {
        static_assert(std::integral_constant<T, false>::value, "Second template parameter needs to be of function type.");
    };

    // specialization that does the checking
    template <class C, class Ret, class... Args>
    struct has_depth_safety<C, Ret(Args...)> {
    private:
        template <typename T>
        static constexpr auto check(T*) -> typename std::is_same<decltype(std::declval<T>().depth_safety(std::declval<Args>()...)), Ret>::type {
            return true;
        }

        template <typename>
        static constexpr std::false_type check(...) {
            return std::false_type();
        }


    public:
        typedef decltype(check<C>(nullptr)) type;
        static constexpr bool value = type::value;
    };

    template <bool, class T>
    class gcc_bug_enable_if_t {
    };

    template <class T>
    class gcc_bug_enable_if_t<true, T> {
        using type = T;
    };

    template <class T, bool as_array = false>
    class typed_lgr {
        lgr actual_lgr;

        template <gcc_bug_enable_if_t<
                      has_depth_safety<T, bool()>::value,
                      int> = 0>
        static bool depth_calc(void* v) {
            return ((T*)v)->depth_safety();
        }

    public:
        using depth_t = bool (*)(void*);

        static constexpr depth_t get_depth_calc() {
            if constexpr (has_depth_safety<T, bool()>::value)
                return &depth_calc;
            else
                return nullptr;
        }

        static void destruct(void* v) {
            if constexpr (as_array)
                delete[] (T*)v;
            else
                delete (T*)v;
        }

        typed_lgr() = default;

        typed_lgr(std::nullptr_t){};

        typed_lgr(T* capture, bool as_weak = false)
            : actual_lgr(capture, get_depth_calc(), destruct, as_weak) {}

        typed_lgr(const typed_lgr& mov) noexcept {
            *this = mov;
        }

        typed_lgr(typed_lgr&& mov) noexcept {
            *this = std::move(mov);
        }

        typed_lgr& operator=(std::nullptr_t) {
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

        const T& operator*() const {
            return *(T*)actual_lgr.getPtr();
        }

        const T* operator->() const {
            return (T*)actual_lgr.getPtr();
        }

        T* getPtr() {
            return (T*)actual_lgr.getPtr();
        }

        bool calcDepth() {
            return actual_lgr.calcDepth();
        }

        bool depth_safety() const {
            return actual_lgr.depth_safety();
        }

        bool is_deleted() const {
            return actual_lgr.is_deleted();
        }

        typed_lgr take() {
            return typed_lgr(std::move(*this));
        }

        lgr actual() {
            return actual_lgr;
        }

        size_t totalLinks() const {
            return actual_lgr.totalLinks();
        }

        bool operator==(const typed_lgr& cmp) const {
            return actual_lgr == cmp.actual_lgr;
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
}