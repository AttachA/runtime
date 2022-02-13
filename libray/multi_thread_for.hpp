#pragma once
#include <thread>

namespace {
	template<class Array_t, class Fn>
	void FOR_interate_call(Array_t& arr, size_t start, size_t end, Fn func) {
		for (size_t i = start; i < end; i++)
			func(arr[i], i);
	}
}
template<class Array_t, class Fn>
void for_thread(Array_t& arr, size_t from, size_t to, Fn func ) {
	if (to - from == 0) return;
	size_t left = 0;
	size_t cores_count;
	size_t full;
	if (to - from < 0) {
		size_t swap = from;
		from = to;
		to = swap;
	}

	{
		size_t full_size = to - from;
		cores_count = std::thread::hardware_concurrency();
		full = full_size / cores_count;
		left = full_size % cores_count;
	}
	if (!full) {
		cores_count = left;
		full = 1;
		left = 0;
	}
	std::thread* t = new std::thread[cores_count];
	for (size_t i = 0; i < cores_count; i++)
		t[i] = std::thread(FOR_interate_call<Array_t, Fn>, std::ref(arr), full * i, full * i + full, func);

	for (size_t i = to - left; i < to; i++)
		func(arr[i],i);
	for (size_t i = 0; i < cores_count; ++i) {
		if (t[i].joinable())
			t[i].join();
	}
	delete[] t;
}
template<class Array_t, class Fn>
void for_thread(Array_t& arr, Fn func) {
	size_t to = arr.size();
	if (to == 0) return;
	size_t cores_count = std::thread::hardware_concurrency();
	size_t left = to % cores_count;
	size_t full = to / cores_count;
	if (!full) {
		cores_count = left;
		full = 1;
		left = 0;
	}

	std::thread* t = new std::thread[cores_count];
	for (size_t i = 0; i < cores_count; i++)
		t[i] = std::thread(FOR_interate_call<Array_t, Fn>, std::ref(arr), full * i, full * i + full,func);

	for (size_t i = to - left; i < to; i++)
		func(arr[i],i);

	for (size_t i = 0; i < cores_count; ++i) {
		if (t[i].joinable())
			t[i].join();
	}
	delete[] t;
}
#ifdef _DEBUG
template<class Array_t, class Fn>
void for_debug(Array_t& arr, size_t from, size_t to, Fn func) {
	if (to - from == 0) return;
	if (to - from < 0) {
		size_t swap = from;
		from = to;
		to = swap;
	}
	for (size_t i = from; i < to; i++)
		func(arr[i], i);
}
template<class Array_t, class Fn>
void for_debug(Array_t& arr, Fn func) {
	size_t to = arr.size();
	for (size_t i = 0; i < to; i++)
		func(arr[i], i);
}
#endif