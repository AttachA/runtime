// Copyright Danyil Melnytskyi 2022-2023
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef RUN_TIME_LIBRARY_BYTES
#define RUN_TIME_LIBRARY_BYTES
#include "../attacha_abi_structs.hpp"
#include "stdint.h"
namespace bytes{
    enum class Endian : uint8_t {
        little,
        big,
        native =
#if 'AABB' == 0x41414242
        little
#else
        big
#endif
    };
    inline void swap_bytes(void* value_ptr, size_t len) {
		char* tmp = new char[len];
		char* prox = (char*)value_ptr;
		int j = 0;
		for (int64_t i = len - 1; i >= 0; i--)
			tmp[i] = prox[j++];
		for (size_t i = 0; i < len; i++)
			prox[i] = prox[i];
		delete[]tmp;
	}
    template<class T>
    inline constexpr void swap_bytes(T& val) {
        char tmp[sizeof(T)];
        char* prox = (char*)&val;
        int j = 0;
        for (int64_t i = sizeof(T) - 1; i >= 0; i--)
            tmp[i] = prox[j++];
        for (size_t i = 0; i < sizeof(T); i++)
            prox[i] = tmp[i];
    }

	template<class T>
    inline constexpr void swap_bytes(T* val, size_t len) {
		for (size_t i = 0; i < len; i++)
				swap_bytes<T>(val[i]);
    }


	inline void convert_endian(Endian value_endian, void* value_ptr, size_t len) {
		if (Endian::native != value_endian)
			swap_bytes(value_ptr, len);
	}
	template<Endian value_endian>
	inline void convert_endian(void* value_ptr, size_t len) {
		if constexpr (Endian::native != value_endian)
			swap_bytes(value_ptr, len);
	}

	template<class T>
	inline constexpr T convert_endian(Endian value_endian, T val) {
		if (Endian::native != value_endian)
			swap_bytes<T>(val);
		return val;
	}
	template<Endian value_endian, class T>
	inline constexpr T convert_endian(T val) {
		if constexpr (Endian::native != value_endian)
			swap_bytes<T>(val);
		return val;
	}
	template<class T>
	inline constexpr T* convert_endian_arr(Endian value_endian, T* val, size_t size) {
		if (Endian::native != value_endian)
			for (size_t i = 0; i < size; i++)
				swap_bytes<T>(val[i]);
		return val;
	}

	template<Endian value_endian, class T>
	inline constexpr T* convert_endian_arr(T* val, size_t size) {
		if constexpr (Endian::native != value_endian)
			for (size_t i = 0; i < size; i++)
				swap_bytes<T>(val[i]);
		return val;
	}

	

	void convert_endian(Endian value_endian, ValueItem& value);

	ValueItem* current_endian(ValueItem* args, uint32_t len);
	ValueItem* convert_endian(ValueItem* args, uint32_t len);
	ValueItem* swap_bytes(ValueItem* args, uint32_t len);
	ValueItem* from_bytes(ValueItem* args, uint32_t len);
	ValueItem* to_bytes(ValueItem* args, uint32_t len);
}

#endif /* RUN_TIME_LIBRARY_BYTES */
