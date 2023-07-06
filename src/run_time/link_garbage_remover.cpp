// Copyright Danyil Melnytskyi 2022-2023
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include "link_garbage_remover.hpp"
namespace art{
	thread_local std::unordered_set<const void*> __lgr_safe_deph;
#if ENABLE_SNAPSHOTS_LGR
#include "CASM.hpp"
	void lgr_join_snapshot(list_array<std::vector<void*>*>* snap_records, std::vector<void*>*& set_current_snap) {
		if (snap_records)
			snap_records->push_back(set_current_snap = FrameResult::JitCaptureStackChainTrace(8));
	}
	void lgr_exit_snapshot(list_array<std::vector<void*>*>* snap_records, std::vector<void*>*& current_snap) {
		if (snap_records)
			snap_records->erase(current_snap);
		if (current_snap) {
			delete current_snap;
			current_snap = nullptr;
		}
	}
	__declspec(noinline) static void lgr_join_snapshot_0(list_array<std::vector<void*>*>*& snap_records, std::vector<void*>*& set_current_snap) {
		snap_records = new list_array<std::vector<void*>*>();
		lgr_join_snapshot(snap_records, set_current_snap);
	}
#define lgr_move_snapshot(snap_records_src,snap_records_dst,current_snap_src,current_snap_dst) snap_records_dst= snap_records_src;current_snap_dst = current_snap_src;current_snap_src = nullptr;snap_records_dst=nullptr;
#define rm_lgr_snap_records if(snap_records) {delete snap_records; snap_records=nullptr; }
#define ini_snap_rec_lgr_arg snap_records = new list_array<std::vector<void*>*>()
#define set_snap_rec_lgr_arg snap_records= snap_record
#define as_snap_rec_lgr_arg ,copy.snap_records

#define can_throw noexcept(false)
#define snap_rec_lgr_arg ,list_array<std::vector<void*>*>* snap_record
#else
#define lgr_move_snapshot(...) 
#define lgr_join_snapshot(...)
#define lgr_exit_snapshot(...)
#define rm_lgr_snap_records
#define set_snap_rec_lgr_arg
#define as_snap_rec_lgr_arg
#define is_snap_rec_lgr_arg

#define snap_records
#define current_snap
#define lgr_join_snapshot_0(...);

#define can_throw noexcept(true)
#define snap_rec_lgr_arg
#endif

	void lgr::exit() {
		lgr_exit_snapshot(snap_records, current_snap);
		if (total == nullptr);
		else if (!in_safe_deph) {
			if (!weak)
				return;
			--(*weak);
			if (!*weak && !*total) {
				delete total;
				delete weak;
				rm_lgr_snap_records;
			}
		}
		else if (*total > 1) 
			--(*total);
		else {
			if (ptr)
				if (destructor) destructor(ptr);
			if (!*weak) {
				delete total;
				delete weak;
				rm_lgr_snap_records;
			}
		}
		ptr = nullptr;
		total = nullptr;
		weak = nullptr;
	}
	void lgr::join(std::atomic_size_t* p_total_links, std::atomic_size_t* tot_weak snap_rec_lgr_arg) {
		set_snap_rec_lgr_arg;
		lgr_join_snapshot(snap_records, current_snap);
		total = p_total_links;
		weak = tot_weak;
		if (p_total_links && (in_safe_deph = calcDeph()))
			++(*p_total_links);
		else if (!in_safe_deph && tot_weak)
			++(*tot_weak);
	}
	lgr::lgr() {}
	lgr::lgr(void* copy, bool(*clc_depth)(void*), void(*destruct)(void*), bool as_weak) : calc_depth(clc_depth), destructor(destruct) {
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
			lgr_join_snapshot_0(snap_records, current_snap);
		}
	}
	lgr::lgr(const lgr& copy) {
		if (this == &copy)return;
		if (total || weak) exit();
		ptr = copy.ptr;
		calc_depth = copy.calc_depth;
		destructor = copy.destructor;
		join(copy.total, copy.weak as_snap_rec_lgr_arg);
	}
	lgr::lgr(lgr&& mov) can_throw {
		lgr_move_snapshot(mov.snap_records, snap_records, mov.current_snap, current_snap);
		lgr_exit_snapshot(snap_records, current_snap);
		lgr_join_snapshot(snap_records, current_snap);

		ptr = mov.ptr;
		total = mov.total;
		weak = mov.weak;
		in_safe_deph = mov.in_safe_deph;
		calc_depth = mov.calc_depth;
		destructor = mov.destructor;
		mov.ptr = nullptr;
		mov.total = nullptr;
		mov.weak = nullptr;
	}
	lgr::~lgr() {
		if (total || !in_safe_deph)
			exit();
	}
	lgr& lgr::operator=(nullptr_t) {
		if (total || weak)
			exit();
		return *this;
	}
	lgr& lgr::operator=(const lgr& copy) {
		if (this == &copy)return *this;
		if (total || weak) exit();
		ptr = copy.ptr;
		calc_depth = copy.calc_depth;
		destructor = copy.destructor;
		join(copy.total, copy.weak as_snap_rec_lgr_arg);
		return *this;
	}
	lgr& lgr::operator=(lgr&& mov) can_throw {
		if (total || weak)
			exit();
		lgr_move_snapshot(mov.snap_records, snap_records, mov.current_snap, current_snap);
		lgr_exit_snapshot(snap_records, current_snap);
		lgr_join_snapshot(snap_records, current_snap);
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
	void*& lgr::operator*() {
		if (total)
			if (!*total)
				ptr = nullptr;
		return ptr;
	}
	void** lgr::operator->() {
		if (total)
			if (!*total)
				ptr = nullptr;
		return &ptr;
	}
	void* lgr::getPtr() {
		if (total)
			if (!*total)
				ptr = nullptr;
		return ptr;
	}
	const void* lgr::getPtr() const {
		if (total)
			if (!*total)
				return nullptr;
		return ptr;
	}
	void(*lgr::getDestructor(void) const)(void*){
		return destructor;
	}
	bool(*lgr::getCalcDepth(void) const)(void*){
		return calc_depth;
	}
	bool lgr::alone(){
		return ptr ?
				total ?
					*total == 1
					: false
				: false;
	}
	void* lgr::try_take_ptr() {
		if(alone()){
			void* res = ptr;
			ptr = nullptr;
			exit();
			return res;
		}
		return nullptr;
	}
	bool lgr::calcDeph() {
		bool res = depth_safety();
		__lgr_safe_deph.clear();
		return res;
	}
	bool lgr::depth_safety() const {
		if (__lgr_safe_deph.contains(ptr))
			return false;
		if (calc_depth) {
			__lgr_safe_deph.emplace(ptr);
			return calc_depth(ptr);
		}
		else
			return true;
	}
	bool lgr::is_deleted() const {
		return !ptr;
	}
	bool lgr::operator==(const lgr& cmp) const {
		return ptr == cmp.ptr;
	}
	bool lgr::operator!=(const lgr& cmp) const {
		return ptr != cmp.ptr;
	}
	lgr::operator bool() const {
		return ptr;
	}
	lgr::operator size_t() const {
		return (size_t)ptr;
	}
	lgr::operator ptrdiff_t() const {
		return (ptrdiff_t)ptr;
	}
}