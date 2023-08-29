// Copyright Danyil Melnytskyi 2022-Present
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef SRC_UTIL_ARRAY
#define SRC_UTIL_ARRAY
#include <cstdint>

namespace art {
    struct ValueItem;

    namespace __ {
        template <typename T, bool as_reference>
        class array {
            T* data;
            uint32_t length;
            friend ValueItem;

        public:
            array()
                : data(nullptr), length(0) {}

            array(T* data, uint32_t length)
                : length(length) {
                if constexpr (as_reference) {
                    this->data = data;
                } else {
                    this->data = new T[length];
                    for (uint32_t i = 0; i < length; i++) {
                        this->data[i] = data[i];
                    }
                }
            }

            array(uint32_t length, T* data)
                : data(data), length(length) {}

            array(const array<T, false>& copy)
                : length(copy.length) {
                if constexpr (as_reference) {
                    data = copy.data;
                } else {
                    data = new T[length];
                    for (uint32_t i = 0; i < length; i++) {
                        data[i] = copy.data[i];
                    }
                }
            }

            array(const array<T, true>& copy)
                : length(copy.length) {
                if constexpr (as_reference) {
                    data = copy.data;
                } else {
                    data = new T[length];
                    for (uint32_t i = 0; i < length; i++) {
                        data[i] = copy.data[i];
                    }
                }
            }

            array(array&& move)
                : data(move.data), length(move.length) {
                move.data = nullptr;
                move.length = 0;
            }

            ~array() {
                if constexpr (!as_reference)
                    if (data)
                        delete[] data;
            }

            T& operator[](uint32_t index) {
                if constexpr (as_reference) {
                    return data[index];
                } else {
                    return data[index];
                }
            }

            const T& operator[](uint32_t index) const {
                if constexpr (as_reference) {
                    return data[index];
                } else {
                    return data[index];
                }
            }

            uint32_t size() const {
                return length;
            }

            T* begin() {
                return data;
            }

            T* end() {
                return data + length;
            }

            const T* begin() const {
                return data;
            }

            const T* end() const {
                return data + length;
            }

            void release() {
                data = nullptr;
                length = 0;
            }
        };
    }

    template <typename T>
    using array_t = __::array<T, false>;
    template <typename T>
    using array_ref_t = __::array<T, true>;
}

#endif /* SRC_UTIL_ARRAY */
