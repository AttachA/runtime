// Copyright Danyil Melnytskyi 2022-Present
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef SRC_RUN_TIME_UTIL_MURMUR3_ENCHANTED
#define SRC_RUN_TIME_UTIL_MURMUR3_ENCHANTED
#include <stdint.h>
#include <type_traits>

namespace art {
    struct Murmur3_128 {
        uint64_t h1;
        uint64_t h2;
    };

    void MurmurHash3_x64_128(const void* key, const size_t len, Murmur3_128& out);
    uint64_t MurmurHash3_x64_64(const void* key, const size_t len);
    uint64_t mur_combine(uint64_t h1, uint64_t h2);
    Murmur3_128 mur_combine(Murmur3_128 h1, Murmur3_128 h2);

    namespace __internal {
        //select hash function
        template <typename T, bool is_class, bool is_integer>
        struct hasher {
            size_t operator()(const T& t) const noexcept {
                return MurmurHash3_x64_64(&t, sizeof(T));
            }

            size_t operator()(const T* arr, size_t len) const noexcept {
                return MurmurHash3_x64_64(arr, sizeof(T) * len);
            }
        };

        template <typename T>
        struct hasher<T, true, false> {
            size_t operator()(const T& t) const noexcept {
                return t.hash();
            }

            size_t operator()(const T* arr, size_t len) const noexcept {
                size_t result = 0;
                for (size_t i = 0; i < len; i++)
                    result = mur_combine(result, arr[i].hash());
                return result;
            }
        };

        template <typename T>
        struct hasher<T, true, true> {};

        template <typename T>
        struct hasher<T, false, false> {};

        template <typename T, bool is_class, bool is_integer>
        struct big_hasher {
            Murmur3_128 operator()(const T& t) const noexcept {
                Murmur3_128 result;
                MurmurHash3_x64_128(&t, sizeof(T), result);
                return result;
            }

            Murmur3_128 operator()(const T* arr, size_t len) const noexcept {
                Murmur3_128 result;
                MurmurHash3_x64_128(arr, sizeof(T) * len, result);
                return result;
            }
        };

        template <typename T>
        struct big_hasher<T, true, false> {
            Murmur3_128 operator()(const T& t) const noexcept {
                return t.big_hash();
            }

            Murmur3_128 operator()(const T* arr, size_t len) const noexcept {
                Murmur3_128 result;
                for (size_t i = 0; i < len; i++)
                    result = mur_combine(result, arr[i].big_hash());
                return result;
            }
        };

        template <typename T>
        struct big_hasher<T, true, true> {};

        template <typename T>
        struct big_hasher<T, false, false> {};
    }

    template <typename T>
    struct hash : __internal::hasher<T,
                                     std::is_class_v<T>,
                                     std::is_enum_v<T> || std::is_arithmetic_v<T> || std::is_pointer_v<T>> {
        hash() {}

        hash(const hash&) {}

        hash(hash&&) {}

        hash& operator=(const hash&) {
            return *this;
        }

        hash& operator=(hash&&) {
            return *this;
        }
    };

    template <typename T>
    struct big_hash : __internal::big_hasher<T,
                                             std::is_class_v<T>,
                                             std::is_enum_v<T> || std::is_arithmetic_v<T> || std::is_pointer_v<T>> {
        big_hash() {}

        big_hash(const big_hash&) {}

        big_hash(big_hash&&) {}

        big_hash& operator=(const big_hash&) {
            return *this;
        }

        big_hash& operator=(big_hash&&) {
            return *this;
        }
    };
}


#endif /* SRC_RUN_TIME_UTIL_MURMUR3_ENCHANTED */
