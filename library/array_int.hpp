#pragma once
#include <stdint.h>
#include <string>
#include <iostream>
#include <vector>




template<uint64_t byte_expand>
class array_uint {
public:
    static const size_t expand_bits = []() { size_t res = 1; for (size_t i = 0; i <= byte_expand; i++) res <<= 2; return res << 6; }();
    static const size_t expand_bytes = []() { size_t res = 1; for (size_t i = 0; i <= byte_expand; i++) res <<= 2; return res; }();
    uint64_t integers[expand_bytes];
//private:
    uint64_t bits() const {
        uint64_t out = 0;
        int64_t max_elem = expand_bytes - 1;
        for (int64_t i = max_elem; i >= 0; --i) 
            if (integers[i])
                max_elem = i;
        uint64_t tmp = integers[max_elem];
        out = (expand_bytes - 1 -max_elem) * 64;
        while (tmp) {
            tmp >>= 1;
            out++;
        }
        return out;
    }
    std::pair <array_uint, array_uint> divmod(const array_uint& lhs, const array_uint& rhs) const {
        if (rhs == 0) {
            throw std::domain_error("Error: division or modulus by 0");
        }
        else if (rhs == 1) {
            return std::pair <array_uint, array_uint>(lhs, 1);
        }
        else if (lhs == rhs) {
            return std::pair <array_uint, array_uint>(1, 0);
        }
        else if ((lhs == 0) || (lhs < rhs)) {
            return std::pair <array_uint, array_uint>(0, lhs);
        }

        std::pair <array_uint, array_uint> qr(0, lhs);
        array_uint& copyd = *new array_uint(rhs << (lhs.bits() - rhs.bits()));
        array_uint& adder = *new array_uint(array_uint(1) << (lhs.bits() - rhs.bits()));
        if (copyd > qr.second) {
            copyd >>= 1;
            adder >>= 1;
        }

        while (qr.second >= rhs) {
            if (qr.second >= copyd) {
                qr.second -= copyd;
                qr.first |= adder;
            }
            copyd >>= 1;
            adder >>= 1;
        }
        delete &copyd;
        delete &adder;
        return qr;
    }
    void zero_all() {
        for (int64_t i = expand_bytes - 1; i >= 0; --i)
            integers[i] = 0;
    }

public:  
    static array_uint get_max() {
        array_uint tmp;
        for (int64_t i = expand_bytes - 1; i >= 0; --i)
            tmp.integers[i] = -1;
        return tmp;
    }
    

    array_uint() {}
    array_uint(const array_uint& rhs) {
        for (int64_t i = expand_bytes - 1; i >= 0; --i)
            integers[i] = rhs.integers[i];
    }
    array_uint(array_uint&& rhs) noexcept {
        for (int64_t i = expand_bytes - 1; i >= 0; --i)
            integers[i] = rhs.integers[i];
    }
    template <uint64_t byt_expand>
    array_uint(const array_uint<byt_expand>& rhs) {
        if constexpr (expand_bits <= array_uint<byt_expand>::expand_bits) {
            for (int64_t i = expand_bytes - 1; i >= 0; --i)
                integers[i] = rhs.integers[i];
        }
        else {
            zero_all();
            for (int64_t i = array_uint<byt_expand>::expand_bytes - 1; i >= 0; --i)
                integers[i] = rhs.integers[i];
        }
    }
    array_uint& operator=(const array_uint& rhs) = default;
    array_uint& operator=(array_uint&& rhs) = default;
    array_uint(const long long rhs) { zero_all(); integers[expand_bytes - 1] = rhs; }
    array_uint(const unsigned long long rhs) { zero_all(); integers[expand_bytes - 1] = rhs; }
    array_uint(const unsigned int rhs) { zero_all(); integers[expand_bytes - 1] = rhs; }
    array_uint(const double rhs) { zero_all(); integers[expand_bytes - 1] = rhs; }
    array_uint(const int rhs) { zero_all(); integers[expand_bytes - 1] = rhs; }
    array_uint(const std::string str) {
        *this = str.c_str();
    }
    array_uint(const char* str) {
        zero_all();
        array_uint* mult = new array_uint(10);
        size_t str_len = strlen(str);
        for (size_t i = 0; i < str_len; i++) {
            *this *= *mult;
            *this += str[i] - '0';
        }
        delete mult;
    }
    std::string to_ansi_string() const {
        std::string res;
        {
            std::pair <array_uint, array_uint>& tmp = *new std::pair <array_uint, array_uint>(*this, 0);
            array_uint& dever = *new array_uint(10000000000000000000);
            std::string len_tmp;
            while (true) {
                tmp = divmod(tmp.first, dever);
                len_tmp = std::to_string(tmp.second.integers[expand_bytes - 1]);
                for (size_t i = len_tmp.size(); i < 19;i++) 
                    len_tmp = '0' + len_tmp;
                res = len_tmp+res;
                if (tmp.first == 0)
                    break;
            }
            delete& dever;
            delete& tmp;
        }
        while (res[0] == '0') {
            res.erase(res.begin());
            if (!res.size()) {
                res = '0';
                break;
            }
        }
        return res;
    }
    std::string to_hex_str() const {
        static const char* digits = "0123456789ABCDEF";
        std::string res;
        std::string rc(8, '0');
        for (auto i : integers){
            for (size_t l = 0, j = 28; l < 8; ++l, j -= 4)
                rc[l] = digits[(i >> j) & 0x0f];
            res += rc;
        }

        while (res[0] == '0')
            res.erase(res.begin());

        if (res.empty())
            res = '0';
        return "0x" + res;
    }




    static array_uint pow(const array_uint& A, const array_uint& N) {
        array_uint res = 1;
        array_uint* a = new array_uint(A);
        array_uint* n = new array_uint(N);
        size_t tot_bits = n->bits();
        uint64_t* proxy_n_end = &n->integers[expand_bytes - 1];
        for (size_t i = 0; i < tot_bits; i++) {
            if (*proxy_n_end & 1)
                res *= *a;
            *a *= *a;
            *n >>= 1;
        }
        delete a;
        delete n;
        return res;
    }
    array_uint& pow(const array_uint& n) {
        return *this = pow(*this,n);
    }
    array_uint& sqrt() {
        array_uint* a = new array_uint(1);
        array_uint* b = new array_uint(*this);

        while (*b - *a > 1) {
            *b = *this / *a;
            *a = (*a + *b) >> 1;
        }
        *this = *a;
        delete a;
        delete b;
        return *this;
    }

    array_uint operator&(const array_uint& rhs) const {
        return array_uint(*this) &= rhs;
    }
    array_uint& operator&=(const array_uint& rhs) {
        for (size_t i = 0; i < expand_bytes; i++)
            integers[i] &= rhs.integers[i];
        return *this;
    }
    array_uint operator|(const array_uint& rhs) const {
        return array_uint(*this) |= rhs;
    }
    array_uint& operator|=(const array_uint& rhs) {
        for (size_t i = 0; i < expand_bytes; i++)
            integers[i] |= rhs.integers[i];
        return *this;
    }
    array_uint operator^(const array_uint& rhs) const {
        return array_uint(*this) ^= rhs;
    }
    array_uint& operator^=(const array_uint& rhs) {
        for (size_t i = 0; i < expand_bytes; i++)
            integers[i] ^= rhs.integers[i];
        return *this;
    }
    array_uint operator~() const {
        array_uint res(*this);
        for (size_t i = 0; i < expand_bytes; i++)
            res.integers[i] = ~integers[i];
        return res;
    }



    array_uint& operator<<=(uint64_t shift) {
        uint64_t bits1 = 0, bits2 = 0;
        uint64_t block_shift = shift / 64;
        uint64_t sub_block_shift = shift % 64;
        for (size_t block_shift_c = 0; block_shift_c < block_shift; block_shift_c++)
            for (int64_t i = expand_bytes - 1; i >= 0; --i) {
                bits1 = integers[i];
                integers[i] = bits2;
                bits2 = bits1;
            }
        if (sub_block_shift) {
            bits1 = bits2 = 0;
            uint64_t anti_shift = 64 - sub_block_shift;
            uint64_t and_op = (uint64_t)-1 << anti_shift;
            for (int64_t i = expand_bytes - 1; i >= 0; --i) {
                bits2 = integers[i] & and_op;
                integers[i] <<= sub_block_shift;
                integers[i] |= bits1 >> anti_shift;
                bits1 = bits2;
            }
        }
        return *this;
    }
    array_uint& operator>>=(uint64_t shift) {
        uint64_t bits1 = 0, bits2 = 0;
        uint64_t block_shift = shift / 64;
        uint64_t sub_block_shift = shift % 64;
        for (size_t block_shift_c = 0; block_shift_c < block_shift; block_shift_c++)
            for (size_t i = 0; i < expand_bytes; i++) {
                bits1 = integers[i];
                integers[i] = bits2;
                bits2 = bits1;
            }
        if (sub_block_shift) {
            bits1 = bits2 = 0;
            uint64_t anti_shift = 64 - sub_block_shift;
            uint64_t and_op = -1 >> anti_shift;
            for (size_t i = 0; i < expand_bytes; i++) {
                bits2 = integers[i] & and_op;
                integers[i] >>= sub_block_shift;
                integers[i] |= bits1 << anti_shift;
                bits1 = bits2;
            }
        }
        return *this;
    }
    array_uint operator<<(uint64_t shift) const {
        return array_uint(*this) <<= shift;
    }
    array_uint operator>>(uint64_t shift) const {
        return array_uint(*this) >>= shift;
    }


    bool operator!() const {
        return !(bool)(*this);
    }
    bool operator&&(const array_uint& rhs) const {
        return ((bool)*this && rhs);
    }
    bool operator||(const array_uint& rhs) const {
        return ((bool)*this || rhs);
    }
    bool operator==(const array_uint& rhs) const {
        for (size_t i = 0; i < expand_bytes; i++)
            if (integers[i] != rhs.integers[i])
                return false;
        return true;
    }
    bool operator!=(const array_uint& rhs) const {
        return !(*this == rhs);
    }
    bool operator>(const array_uint& rhs) const {
        for (size_t i = 0; i < expand_bytes; i++) {
            if (integers[i] > rhs.integers[i])
                return true;
            else if (integers[i] < rhs.integers[i])
                return false;
        }
        return false;
    }
    bool operator<(const array_uint& rhs) const {
        for (size_t i = 0; i < expand_bytes; i++) {
            if (integers[i] < rhs.integers[i])
                return true;
            else if (integers[i] > rhs.integers[i])
                return false;
        }
        return false;
    }
    bool operator>=(const array_uint& rhs) const {
        return !(*this < rhs);
    }
    bool operator<=(const array_uint& rhs) const {
        return !(*this > rhs);
    }


    array_uint& operator++() {
        return *this += 1;
    }
    array_uint operator++(int) {
        array_uint temp(*this);
        *this += 1;
        return temp;
    }
    array_uint& operator--() {
        return *this -= 1;
    }
    array_uint operator--(int) {
        array_uint temp(*this);
        --* this;
        return temp;
    }

    array_uint operator+(const array_uint& rhs) const {
        return array_uint(*this) += rhs;
    }
    array_uint operator-(const array_uint& rhs) const {
        return array_uint(*this) -= rhs;
    }
    array_uint& operator+=(const array_uint& rhs) {
        uint64_t bits1 = 0, bits2 = 0;
        for (int64_t i = expand_bytes - 1; i >= 0; --i) {
            bits1 = ((integers[i] + rhs.integers[i] + bits2) < integers[i]);
            integers[i] += rhs.integers[i] + bits2;
            bits2 = bits1;
        }
        return *this;
    }
    array_uint& operator-=(const array_uint& rhs) {
        uint64_t bits1 = 0, bits2 = 0;
        for (int64_t i = expand_bytes - 1; i >= 0; --i) {
            bits1 = ((integers[i] - rhs.integers[i] - bits2) > integers[i]);
            integers[i] -= rhs.integers[i] + bits2;
            bits2 = bits1;
        }
        return *this;
    }


    array_uint operator*(const array_uint& rhs) const {
        array_uint res(0);
        array_uint& multer = *new array_uint(rhs);
        size_t tot_bits = multer.bits();
        uint64_t& proxy_multer_end = multer.integers[expand_bytes - 1];
        for (size_t i = 0; i < tot_bits; i++) {
            if (proxy_multer_end & 1) {
                res += *this << i;
            }
            multer >>= 1;
        }
        delete& multer;
        return res;
    }
    array_uint& operator*=(const array_uint& rhs) {
        *this = *this * rhs;
        return *this;
    }

    array_uint operator/(const array_uint& rhs) const {
        return divmod(*this, rhs).first;
    }
    array_uint& operator/=(const array_uint& rhs) {
        return *this = divmod(*this, rhs).first;
    }

    array_uint operator%(const array_uint& rhs) const {
        return divmod(*this, rhs).second;
    }
    array_uint& operator%=(const array_uint& rhs) {
        *this = divmod(*this, rhs).second;
        return *this;
    }
    explicit operator bool() const {
        uint64_t res = 0;
        for (size_t i = 0; i < expand_bytes; i++)
            res |= integers[i];
        return (bool)res;
    }
    explicit operator uint8_t() const {
        return (uint8_t)integers[expand_bytes - 1];
    }
    explicit operator uint16_t() const {
        return (uint16_t)integers[expand_bytes - 1];
    }
    explicit operator uint32_t() const {
        return (uint32_t)integers[expand_bytes - 1];
    }
    explicit operator uint64_t() const {
        return (uint64_t)integers[expand_bytes - 1];
    }
    template <uint64_t byt_expand>
    explicit operator array_uint<byt_expand>() const {
        return array_uint<byt_expand>(*this);
    }
};



template<uint64_t byte_expand>
class array_int {
    array_int& switch_my_siqn() {
        val.unsigned_int = ~val.unsigned_int;
        val.unsigned_int += 1;
        return *this;
    }
    array_int switch_my_siqn() const {
        return array_int(*this).switch_my_siqn();
    }

    array_int& switch_to_unsiqn() {
        if (val.is_minus)
            switch_my_siqn();
        return *this;
    }
    array_int switch_to_unsiqn() const {
        if (val.is_minus)
            return array_int(*this).switch_my_siqn();
        else return *this;
    }

    array_int& switch_to_siqn() {
        if (!val.is_minus)
            switch_my_siqn();
        return *this;
    }
    array_int switch_to_siqn() const {
        if (!val.is_minus)
            return array_int(*this).switch_my_siqn();
        else return *this;
    }
public:
    union for_constructor {
        struct s {
            bool _is_minus : 1;
        public:
            s() {}
            s(bool val) {
                _is_minus = val;
            }
            void operator = (bool val) {
                _is_minus = val;
            }
            operator bool() const {
                return _is_minus;
            }
        } is_minus;

        array_uint<byte_expand> unsigned_int;
        for_constructor() {
        }
    } val;
    static array_int get_min() {
        array_int tmp = 0;
        tmp.val.is_minus = 1;
        return tmp;
    }
    static array_int get_max() {
        array_int tmp;
        tmp.val.unsigned_int = array_uint<byte_expand>::get_max();
        tmp.val.is_minus = 0;
        return tmp;
    }


    array_int() {
        val.unsigned_int = 0;
    }
    array_int(const array_int& rhs) {
        val.unsigned_int = rhs.val.unsigned_int;
    }
    array_int(array_int&& rhs) {
        val.unsigned_int = rhs.val.unsigned_int;
    } 
    array_int(const std::string str) {
        *this = str.c_str();
    }
    array_int(const char* str) {
        bool set_minus = 0;
        if (*str++ == '-')
            set_minus = 1;
        else str--;
        val.unsigned_int = array_uint<byte_expand>(str);
        if (set_minus) {
            switch_to_siqn();
            val.is_minus = 1;
        }
    }
    template <uint64_t byt_expand>
    array_int(const array_int<byt_expand> rhs) {
        array_int<byt_expand> tmp = rhs;
        bool is_minus = tmp.val.is_minus;
        tmp.val.is_minus = 0;
        val.unsigned_int = tmp.val.unsigned_int;
        val.is_minus = is_minus;
    }
    template <typename T>
    array_int(const T& rhs)
    {
        val.unsigned_int = (array_uint<byte_expand>)rhs;
    }
    template <typename T>
    array_int(const T&& rhs)
    {
        val.unsigned_int = (array_uint<byte_expand>)rhs;
    }
    array_int(const long long rhs) { *this = std::to_string(rhs).c_str(); }
    array_int(const int rhs) { *this = std::to_string(rhs).c_str(); }

    std::string to_ansi_string() const {
        if (val.is_minus)
            return
            '-' + array_int(*this).switch_to_unsiqn().
            val.unsigned_int.to_ansi_string();
        else
            return val.unsigned_int.to_ansi_string();
    }
    std::string to_hex_str() const {
        return val.unsigned_int.to_hex_str();
    }

    array_int& operator=(const array_int& rhs) = default;
    array_int& operator=(array_int&& rhs) = default;


    array_int operator&(const array_int& rhs) const {
        return array_int(*this) &= rhs;
    }
    array_int& operator&=(const array_int& rhs) {
        val.unsigned_int &= rhs.val.unsigned_int;
        return *this;
    }
    array_int operator|(const array_int& rhs) const {
        return array_int(*this) |= rhs;
    }
    array_int& operator|=(const array_int& rhs) {
        val.unsigned_int |= rhs.val.unsigned_int;
        return *this;
    }
    array_int operator^(const array_int& rhs) const {
        return array_int(*this) ^= rhs;
    }
    array_int& operator^=(const array_int& rhs) {
        val.unsigned_int ^= rhs.val.unsigned_int;
        return *this;
    }
    array_int operator~() const {
        array_int tmp(*this);
        ~tmp.val.unsigned_int;
        return tmp;
    }


    array_int& operator<<=(uint64_t shift) {
        val.unsigned_int <<= shift;
        return *this;
    }
    array_int operator<<(uint64_t shift) const {
        return array_int(*this) <<= shift;
    }
    array_int& operator>>=(uint64_t shift) {
        val.unsigned_int >>= shift;
        return *this;
    }
    array_int operator>>(uint64_t shift) const {
        return array_int(*this) >>= shift;
    }

    bool operator!() const {
        return !val.unsigned_int;
    }
    bool operator&&(const array_int& rhs) const {
        return ((bool)*this && rhs);
    }
    bool operator||(const array_int& rhs) const {
        return ((bool)*this || rhs);
    }
    bool operator==(const array_int& rhs) const {
        return val.unsigned_int == rhs.val.unsigned_int;
    }
    bool operator!=(const array_int& rhs) const {
        return val.unsigned_int != rhs.val.unsigned_int;
    }
    bool operator>(const array_int& rhs) const {
        if (val.is_minus && !rhs.val.is_minus)
            return false;
        if (!val.is_minus && rhs.val.is_minus)
            return true;
        return array_int(*this).switch_to_unsiqn().val.unsigned_int > rhs.switch_to_unsiqn().val.unsigned_int;
    }
    bool operator<(const array_int& rhs) const {
        if (val.is_minus && !rhs.val.is_minus)
            return true;
        if (!val.is_minus && rhs.val.is_minus)
            return false;
        return array_int(*this).switch_to_unsiqn().val.unsigned_int < rhs.switch_to_unsiqn().val.unsigned_int;
    }
    bool operator>=(const array_int& rhs) const {
        return ((*this > rhs) | (*this == rhs));
    }
    bool operator<=(const array_int& rhs) const {
        return ((*this < rhs) | (*this == rhs));
    }

    array_int& operator++() {
        if (val.is_minus) {
            switch_my_siqn().val.unsigned_int += 1;
            switch_my_siqn();
        }
        else
            val.unsigned_int++;
        return *this;
    }
    array_int operator++(int) {
        array_int temp(*this);
        ++* this;
        return temp;
    }
    array_int& operator--() {
        if (val.is_minus) {
            switch_my_siqn().val.unsigned_int -= 1;
            switch_my_siqn();
        }
        else
            val.unsigned_int--;
        return *this;
    }
    array_int operator--(int) {
        array_int temp(*this);
        --* this;
        return temp;
    }

    array_int operator+(const array_int& rhs) const {
        return array_int(*this) += rhs;
    }
    array_int operator-(const array_int& rhs) const {
        return array_int(*this) -= rhs;
    }

    array_int& operator+=(const array_int& rhs) {
        if (val.is_minus && rhs.val.is_minus) {
            switch_my_siqn().val.unsigned_int += rhs.switch_my_siqn().val.unsigned_int;
            switch_to_siqn();
        }
        else if (val.is_minus) {
            if (switch_my_siqn().val.unsigned_int < rhs.val.unsigned_int) {
                val.unsigned_int = rhs.val.unsigned_int - val.unsigned_int;
                switch_to_unsiqn();
            }
            else {
                val.unsigned_int -= rhs.val.unsigned_int;
                switch_to_siqn();
            }
        }
        else if (rhs.val.is_minus) {
            array_int* temp = new array_int(rhs);
            temp->switch_my_siqn();
            if (val.unsigned_int < temp->val.unsigned_int) {
                val.unsigned_int = temp->val.unsigned_int - val.unsigned_int;
                switch_to_siqn();
            }
            else
                val.unsigned_int -= temp->val.unsigned_int;
            delete temp;
        }
        else
            val.unsigned_int += rhs.val.unsigned_int;
        return *this;
    }
    array_int& operator-=(const array_int& rhs) {
        if (val.is_minus && rhs.val.is_minus) {
            if (*this > rhs) {
                switch_my_siqn().val.unsigned_int -= rhs.switch_my_siqn().val.unsigned_int;
                switch_my_siqn();
            }
            else {
                switch_my_siqn();
                val.unsigned_int = rhs.switch_my_siqn().val.unsigned_int - val.unsigned_int;
            }
        }
        else if (val.is_minus) {
            switch_my_siqn();
            if (this->val.unsigned_int > rhs.val.unsigned_int) {
                val.unsigned_int += rhs.val.unsigned_int;
                switch_to_siqn();
            }
            else 
                val.unsigned_int -= rhs.val.unsigned_int;
        }
        else if (rhs.val.is_minus) {
            array_int* temp = new array_int(rhs);
            temp->switch_my_siqn();
            val.unsigned_int += temp->val.unsigned_int;
            delete temp;
        }
        else {
            if (val.unsigned_int >= rhs.val.unsigned_int)
                val.unsigned_int -= rhs.val.unsigned_int;
            else {
                val.unsigned_int = rhs.val.unsigned_int - val.unsigned_int;
                switch_my_siqn();
            }
        }
        return *this;
    }

    array_int operator*(const array_int& rhs) const {
        return array_int(*this) *= rhs;
    }
    array_int& operator*=(const array_int& rhs) {
        if (val.is_minus && rhs.val.is_minus)
            switch_my_siqn().val.unsigned_int *= rhs.switch_my_siqn().val.unsigned_int;
        else if (val.is_minus || rhs.val.is_minus) {
            switch_to_unsiqn().val.unsigned_int *= rhs.switch_to_unsiqn().val.unsigned_int;
            switch_to_siqn();
        }
        else
            val.unsigned_int *= rhs.val.unsigned_int;
        return *this;
    }


    array_int operator/(const array_int& rhs) const {
        return array_int(*this) /= rhs;
    }
    array_int& operator/=(const array_int& rhs) {
        if (val.is_minus && rhs.val.is_minus)
            switch_my_siqn().val.unsigned_int /= rhs.switch_my_siqn().val.unsigned_int;
        else if (val.is_minus || rhs.val.is_minus) {
            switch_to_unsiqn().val.unsigned_int /= rhs.switch_to_unsiqn().val.unsigned_int;
            switch_my_siqn();
        }
        else
            val.unsigned_int /= rhs.val.unsigned_int;
        return *this;
    }

    array_int operator%(const array_int& rhs) const {
        return array_int(*this) %= rhs;
    }
    array_int& operator%=(const array_int& rhs) {
        if (val.is_minus && rhs.val.is_minus)
            switch_my_siqn().val.unsigned_int %= rhs.switch_my_siqn().val.unsigned_int;
        else if (val.is_minus || rhs.val.is_minus) {
            switch_to_unsiqn().val.unsigned_int %= rhs.switch_to_unsiqn().val.unsigned_int;
            switch_my_siqn();
        }
        else
            val.unsigned_int %= rhs.val.unsigned_int;
        return *this;
    }


    array_int operator+() const {
        return switch_to_unsiqn();
    }
    array_int operator-() const {
        return switch_to_siqn();
    }


    explicit operator bool() const {
        return (bool)val.unsigned_int;
    }
    explicit operator uint8_t() const {
        return (uint8_t)val.unsigned_int;
    }
    explicit operator uint16_t() const {
        return (uint16_t)val.unsigned_int;
    }
    explicit operator uint32_t() const {
        return (uint32_t)val.unsigned_int;
    }
    explicit operator uint64_t() const {
        return (uint64_t)val.unsigned_int;
    }
    template <uint64_t byt_expand>
    explicit operator array_uint<byt_expand>() const {
        return array_uint<byt_expand>(val.unsigned_int);
    }
    template <uint64_t byt_expand>
    explicit operator array_int<byt_expand>() {
        return to_ansi_string().c_str();
    }
    explicit operator int64_t() const {
        return std::stoll(to_ansi_string());;
    }
};


template<uint64_t byte_expand>
class array_real {
    union for_constructor {

        struct s {
            uint64_t unused : 63 - (byte_expand >= 57 ? 63 : (5 + byte_expand));
            uint64_t _dot_pos : (byte_expand >= 57 ? 63 : (5 + byte_expand));
            uint64_t _is_minus : 1;
        public:
            s() {

            }
            s(uint64_t val) {
                _dot_pos = val;
            }
            void operator = (uint64_t val) {
                _dot_pos = val;
            }
            operator uint64_t() const {
                return _dot_pos;
            }
            bool is_minus() const {
                return _is_minus;
            }
            bool modify_minus(bool bol) {
                _is_minus = bol;
            }
        } dot_pos;
        array_int<byte_expand> signed_int;
        for_constructor() {}
    } val;

    static std::vector<std::string> split_dot(std::string value) {
        std::vector<std::string> strPairs;
        size_t pos = 0;
        if ((pos = value.find(".")) != std::string::npos) {
            strPairs.push_back(value.substr(0, pos));
            value.erase(0, pos + 1);
        }
        if (!value.empty())
            strPairs.push_back(value);
        return strPairs;
    }

    uint64_t temp_denormalize_struct() {
        uint64_t tmp = val.dot_pos;
        val.dot_pos = 0;
        return tmp;
    }
    void normalize_struct(uint64_t value) {
        val.dot_pos = value;
    }
    void normalize_dot(uint64_t cur_dot_pos = -1) {
        if (cur_dot_pos == (uint64_t)-1)
            cur_dot_pos = temp_denormalize_struct();

        if (cur_dot_pos == 0)
            return;

        while (val.dot_pos || cur_dot_pos > max_dot_pos) {
            val.signed_int /= 10;
            cur_dot_pos--;
        }
        std::string tmp_this_value = to_ansi_string();
        array_int<byte_expand>* remove_nuls_mult = new array_int<byte_expand>(10);
        array_int<byte_expand>* remove_nuls = new array_int<byte_expand>(1);
        size_t nul_count = 0;
        for (int64_t i = tmp_this_value.length() - 1; i >= 0; i--)
        {
            if (tmp_this_value[i] != '0')
                break;
            else {
                *remove_nuls *= *remove_nuls_mult;
                nul_count++;
            }
        }
        delete remove_nuls_mult;
        uint64_t modify_dot = cur_dot_pos - nul_count;
        if (nul_count)
            val.signed_int /= *remove_nuls;
        delete remove_nuls;
        normalize_struct(modify_dot);
    }
    uint64_t denormalize_dot() {
        if (val.signed_int) {
            uint64_t this_dot_pos = temp_denormalize_struct();
            array_int<byte_expand>& mult_int = *new array_int<byte_expand>(10);
            for (;;) {
                if ((val.signed_int * mult_int) / mult_int == val.signed_int && this_dot_pos != 31) {
                    val.signed_int *= mult_int;
                    this_dot_pos++;
                    mult_int *= 10;
                }
                else
                    break;
            }
            delete &mult_int;
            return this_dot_pos;
        }
        return 0;
    }
    void div(const array_real& rhs) {
        uint64_t this_dot_pos = temp_denormalize_struct();
        array_real tmp = rhs;
        uint64_t rhs_dot_pos = tmp.temp_denormalize_struct();

        this_dot_pos -= rhs_dot_pos;
        while (true) {
            if (!(tmp.val.signed_int % 10) && tmp.val.signed_int) {
                if (this_dot_pos != 31) {
                    this_dot_pos++;
                    tmp.val.signed_int /= 10;
                    continue;
                }
            }
            this_dot_pos += denormalize_dot();
            val.signed_int /= tmp.val.signed_int;
            break;
        }
        normalize_dot();
    }
    void mod(const array_real& rhs) {
        uint64_t this_dot_pos = temp_denormalize_struct();
        array_real tmp = rhs;
        uint64_t rhs_dot_pos = tmp.temp_denormalize_struct();

        this_dot_pos -= rhs_dot_pos;
        while (true) {
            if (!(tmp.val.signed_int % 10) && tmp.val.signed_int) {
                if (this_dot_pos != 31) {
                    this_dot_pos++;
                    tmp.val.signed_int /= 10;
                    continue;
                }
            }
            this_dot_pos += denormalize_dot();
            val.signed_int %= tmp.val.signed_int;
            break;
        }
        normalize_dot();
    }
public:
    static const uint64_t max_dot_pos;
    void mod_dot(uint64_t pos) {
        val.dot_pos = pos;
    }
    static array_real get_min() {
        array_real tmp = 0;
        tmp.val.signed_int = array_int<byte_expand>::get_min();
        tmp.val.dot_pos = 0;
        return tmp;
    }
    static array_real get_max() {
        array_real tmp = 0;
        tmp.val.signed_int = array_int<byte_expand>::get_max();
        tmp.val.dot_pos = 0;
        return tmp;
    }

    array_real() {
        val.signed_int = 0;
    }
    array_real(const array_real& rhs) {
        val.signed_int = rhs.val.signed_int;
    }
    array_real(array_real&& rhs) {
        val.signed_int = rhs.val.signed_int;
    }
    array_real(const char* str) {
        *this = std::string(str);
    }
    array_real(const std::string str) {
        size_t found_pos = str.find('.');
        if (found_pos == std::string::npos)
        {
            val.signed_int = array_int<byte_expand>(str.c_str());
            val.dot_pos = 0;
        }
        else {
            //check str for second dot
            if (str.find('.', found_pos) == std::string::npos)
                throw std::invalid_argument("Real value can contain only one dot");

            std::string tmp = str;
            tmp.erase(tmp.begin() + found_pos);
            val.signed_int = array_int<byte_expand>(tmp.c_str());
            val.dot_pos = (str.length() - found_pos - 1);
        }
        normalize_struct(temp_denormalize_struct());
    }
    template <uint64_t byt_expand>
    array_real(const array_int<byt_expand>& rhs) {
        *this = rhs.to_ansi_string().c_str();
    }
    template <typename T>
    array_real(const T& rhs)
    {
        val.signed_int = (array_int<byte_expand>)rhs;
    }
    template <typename T>
    array_real(const T&& rhs)
    {
        val.signed_int = (array_int<byte_expand>)rhs;
    }
    std::string to_ansi_string() {
        uint64_t tmp = temp_denormalize_struct();
        bool has_minus = val.dot_pos.is_minus();
        if (has_minus)
            val.dot_pos = -1;

        std::string str = val.signed_int.to_ansi_string();
        if (has_minus)
            str.erase(str.begin());
        normalize_struct(tmp);
        if (val.dot_pos) {
            if (str.length() <= val.dot_pos) {
                uint64_t resize_result_len = val.dot_pos - str.length() + 1;
                std::string to_add_zeros;
                while (resize_result_len--)
                    to_add_zeros += '0';
                str = to_add_zeros + str;
            }
            str.insert(str.end() - tmp, '.');
        }
        return (has_minus ? "-" : "") + str;
    }
    std::string to_ansi_string() const {
        return array_real(*this).to_ansi_string();
    }
    std::string to_hex_str() const {
        return val.signed_int.to_hex_str();
    }

    array_real& operator=(const array_real& rhs) = default;
    array_real& operator=(array_real&& rhs) = default;

    array_real operator&(const array_real& rhs) const {
        return array_real(*this) &= rhs;
    }

    array_real& operator&=(const array_real& rhs) {
        val.signed_int &= rhs.val.signed_int;
        return *this;
    }

    array_real operator|(const array_real& rhs) const {
        return array_real(*this) |= rhs;
    }

    array_real& operator|=(const array_real& rhs) {
        val.signed_int |= rhs.val.signed_int;
        return *this;
    }

    array_real operator^(const array_real& rhs) const {
        return array_real(*this) ^= rhs;
    }

    array_real& operator^=(const array_real& rhs) {
        val.signed_int ^= rhs.val.signed_int;
        return *this;
    }

    array_real operator~() const {
        array_real tmp(*this);
        ~tmp.val.signed_int;
        return tmp;
    }



    array_real& operator<<=(uint64_t shift) {
        val.signed_int <<= shift;
        return *this;
    }
    array_real operator<<(uint64_t shift) const {
        return array_real(*this) <<= shift;
    }


    array_real& operator>>=(uint64_t shift) {
        val.signed_int >>= shift;
        return *this;
    }
    array_real operator>>(uint64_t shift) const {
        return array_real(*this) >>= shift;
    }
    bool operator!() const {
        return !val.signed_int;
    }

    bool operator&&(const array_real& rhs) const {
        return ((bool)*this && rhs);
    }

    bool operator||(const array_real& rhs) const {
        return ((bool)*this || rhs);
    }

    bool operator==(const array_real& rhs) const {
        return val.signed_int == rhs.val.signed_int;
    }

    bool operator!=(const array_real& rhs) const {
        return val.signed_int != rhs.val.signed_int;
    }

    bool operator>(const array_real& rhs) const {
        std::vector<std::string> this_parts = split_dot(to_ansi_string());
        std::vector<std::string> rhs_parts = split_dot(rhs.to_ansi_string());
        {
            array_int<byte_expand> temp1(this_parts[0].c_str());
            array_int<byte_expand> temp2(rhs_parts[0].c_str());
            if (temp1 == temp2);
            else return temp1 > temp2;
        }
        if (this_parts.size() == 2 && rhs_parts.size() == 2) {
            array_int<byte_expand> temp1(this_parts[1].c_str());
            array_int<byte_expand> temp2(rhs_parts[1].c_str());
            return temp1 > temp2;
        }
        else
            return this_parts.size() > rhs_parts.size();
    }

    bool operator<(const array_real& rhs) const {
        return !(*this > rhs) && *this != rhs;
    }

    bool operator>=(const array_real& rhs) const {
        return ((*this > rhs) | (*this == rhs));
    }

    bool operator<=(const array_real& rhs) const {
        return !(*this > rhs);
    }


    array_real& operator++() {
        return *this += 1;
    }
    array_real operator++(int) {
        array_real temp(*this);
        *this += 1;
        return temp;
    }
    array_real operator+(const array_real& rhs) const {
        return array_real(*this) += rhs;
    }
    array_real operator-(const array_real& rhs) const {
        return array_real(*this) -= rhs;
    }

    array_real& operator+=(const array_real& rhs) {
        uint64_t this_dot_pos = temp_denormalize_struct();
        array_real tmp = rhs;
        uint64_t rhs_dot_pos = tmp.temp_denormalize_struct();
        if (this_dot_pos == rhs_dot_pos);
        else if (this_dot_pos > rhs_dot_pos) {
            array_int<byte_expand> move_tmp = 1;
            while (this_dot_pos != rhs_dot_pos++)
                move_tmp *= 10;
            tmp.val.signed_int *= move_tmp;
        }
        else {
            array_int<byte_expand> move_tmp = 1;
            while (rhs_dot_pos != this_dot_pos++)
                move_tmp *= 10;
            val.signed_int *= move_tmp;
        }
        val.signed_int += tmp.val.signed_int;
        normalize_struct(this_dot_pos);
        normalize_dot();
        return *this;
    }
    array_real& operator-=(const array_real& rhs) {
        uint64_t this_dot_pos = temp_denormalize_struct();
        array_real tmp = rhs;
        uint64_t rhs_dot_pos = tmp.temp_denormalize_struct();
        if (this_dot_pos == rhs_dot_pos);
        else if (this_dot_pos > rhs_dot_pos) {
            array_int<byte_expand> move_tmp = 1;
            while (this_dot_pos != rhs_dot_pos++)
                move_tmp *= 10;
            tmp.val.signed_int *= move_tmp;
        }
        else {
            array_int<byte_expand> move_tmp = 1;
            while (rhs_dot_pos != this_dot_pos++)
                move_tmp *= 10;
            val.signed_int *= move_tmp;
        }
        val.signed_int -= tmp.val.signed_int;
        normalize_struct(this_dot_pos);
        normalize_dot();
        return *this;
    }

    array_real operator*(const array_real& rhs) const {
        return array_real(*this) *= rhs;
    }


    array_real& operator*=(const array_real& rhs) {
        uint64_t this_dot_pos = temp_denormalize_struct();
        array_real tmp = rhs;
        uint64_t rhs_dot_pos = tmp.temp_denormalize_struct();
        this_dot_pos += rhs_dot_pos;

        val.signed_int *= tmp.val.signed_int;

        normalize_struct(this_dot_pos);
        normalize_dot();
        return *this;
    }

    array_real operator/(const array_real& rhs) const {
        return array_real(*this) /= rhs;
    }

    array_real& operator/=(const array_real& rhs) {
        div(rhs);
        return *this;
    }

    array_real operator%(const array_real& rhs) const {
        return array_real(*this) %= rhs;
    }

    array_real& operator%=(const array_real& rhs) {
        mod(rhs);
        return *this;
    }

    explicit operator bool() const {
        return (bool)val.signed_int;
    }
    explicit operator uint8_t() const {
        return (uint8_t)val.signed_int;
    }
    explicit operator uint16_t() const {
        return (uint16_t)val.signed_int;
    }
    explicit operator uint32_t() const {
        return (uint32_t)val.signed_int;
    }
    explicit operator uint64_t() const {
        return (uint64_t)val.signed_int;
    }


    template <uint64_t byt_expand>
    explicit operator array_int<byt_expand>() const {
        array_real tmp = *this;
        array_int<byte_expand> div = 10;
        uint64_t temp = tmp.temp_denormalize_struct();
        for (uint64_t i = 0; i < temp; temp++)
            tmp.val.signed_int /= div;
        return tmp.val.signed_int;
    }
    template <uint64_t byt_expand>
    explicit operator array_uint<byt_expand>() const {
        return (array_uint<byt_expand>)((array_int<byt_expand>)*this);
    }
    template <uint64_t byt_expand>
    explicit operator array_real<byt_expand>() {
        return to_ansi_string().c_str();
    }

    explicit operator double() const {
        array_real tmp = *this;
        array_uint<byte_expand> div = 10;
        uint64_t temp = tmp.temp_denormalize_struct();
        for (uint64_t i = 0; i < temp; i++)
            tmp.val.signed_int /= div;

        double res = (int64_t)tmp.val.signed_int;
        for (uint64_t i = 0; i < temp; i++)
            tmp.val.signed_int *= div;
        tmp = *this - tmp.val.signed_int;
        double res_part = (int64_t)tmp.val.signed_int;;
        for (uint64_t i = 0; i < temp; i++)
            res_part /= 10;
        return res + res_part;
    }

};

template<uint64_t byte_expand>
const uint64_t array_real<byte_expand>::max_dot_pos = []() {
    uint64_t _dot_pos : (byte_expand >= 58 ? 64 : (5 + byte_expand));
    _dot_pos = -1;
    return _dot_pos;
}();


template<uint64_t byte_expand>
class array_unreal {
    union for_constructor {
        struct s {
            uint64_t unused : 64 - (byte_expand >= 57 ? 63 : (5 + byte_expand));
            uint64_t _dot_pos : (byte_expand >= 58 ? 64 : (5 + byte_expand));
        public:
            s() {

            }
            s(uint64_t val) {
                _dot_pos = val;
            }
            void operator = (uint64_t val) {
                _dot_pos = val;
            }
            operator uint64_t() const {
                return _dot_pos;
            }
        } dot_pos;
        array_uint<byte_expand> unsigned_int;
        for_constructor() {}
    } val;
    static std::vector<std::string> split_dot(std::string value) {
        std::vector<std::string> strPairs;
        size_t pos = 0;
        if ((pos = value.find(".")) != std::string::npos) {
            strPairs.push_back(value.substr(0, pos));
            value.erase(0, pos + 1);
        }
        if (!value.empty())
            strPairs.push_back(value);
        return strPairs;
    }
    uint64_t temp_denormalize_struct() {
        uint64_t tmp = val.dot_pos;
        val.dot_pos = 0;
        return tmp;
    }
    void normalize_struct(uint64_t value) {
        val.dot_pos = value;
    }
    void normalize_dot(uint64_t cur_dot_pos = -1) {
        if (cur_dot_pos == (uint64_t)-1)
            cur_dot_pos = temp_denormalize_struct();

        if (cur_dot_pos == 0)
            return;

        array_uint<byte_expand> shrinker(10);
        while (val.dot_pos || cur_dot_pos > max_dot_pos) {
            val.unsigned_int /= shrinker;
            cur_dot_pos--;
        }
        size_t nul_count = 0;
        while(val.unsigned_int % shrinker != 0){
            val.unsigned_int /= shrinker;
            nul_count++;
        }
        normalize_struct(cur_dot_pos - nul_count);
    }

    void denormalize_dot_fixer(array_uint<byte_expand>& src, array_uint<byte_expand>& dst, uint64_t& dot_pos, uint64_t add, uint64_t limit) {
        array_uint<byte_expand>& old = *new array_uint<byte_expand>(src);
        for (;;) {
            if (dot_pos + add <= limit) {
                src *= dst;
                if (old != src / dst) {
                    src = old;
                    break;
                }
                old = src;
                dot_pos += add;
            }
            else break;
        }
        delete &old;
    }

    uint64_t denormalize_dot() {
        if (val.unsigned_int) {


            uint64_t this_dot_pos = temp_denormalize_struct();
            array_uint<byte_expand> move_tmp;
            if (max_dot_pos > 2432) {
                move_tmp = "100000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000";
                denormalize_dot_fixer(val.unsigned_int, move_tmp, this_dot_pos, 2432, max_dot_pos);
            }
            if (max_dot_pos > 1216) {
                move_tmp = "10000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000";
                denormalize_dot_fixer(val.unsigned_int, move_tmp, this_dot_pos, 1216, max_dot_pos);
            }
            if (max_dot_pos > 608) {
                move_tmp = "100000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000";
                denormalize_dot_fixer(val.unsigned_int, move_tmp, this_dot_pos, 608, max_dot_pos);
            }
            if (max_dot_pos > 304) {
                move_tmp = "10000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000";
                denormalize_dot_fixer(val.unsigned_int, move_tmp, this_dot_pos, 304, max_dot_pos);
            }
            if (max_dot_pos > 152) {
                move_tmp = "100000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000";
                denormalize_dot_fixer(val.unsigned_int, move_tmp, this_dot_pos, 152, max_dot_pos);
            }
            if (max_dot_pos > 76) {
                move_tmp = "10000000000000000000000000000000000000000000000000000000000000000000000000000";
                denormalize_dot_fixer(val.unsigned_int, move_tmp, this_dot_pos, 76, max_dot_pos);
            }
            if (max_dot_pos > 38) {
                move_tmp = "100000000000000000000000000000000000000";
                denormalize_dot_fixer(val.unsigned_int, move_tmp, this_dot_pos, 38, max_dot_pos);
            }
            move_tmp = 10000000000000000000;
            denormalize_dot_fixer(val.unsigned_int, move_tmp, this_dot_pos, 19, max_dot_pos);
            move_tmp = 100000;
            denormalize_dot_fixer(val.unsigned_int, move_tmp, this_dot_pos, 5, max_dot_pos);
            move_tmp = 10;
            denormalize_dot_fixer(val.unsigned_int, move_tmp, this_dot_pos,1, max_dot_pos);

            return this_dot_pos;
        }
        return 0;
    }
    void div(const array_unreal& rhs) {
        uint64_t this_dot_pos = temp_denormalize_struct();
        array_unreal tmp = rhs;
        uint64_t rhs_dot_pos = tmp.temp_denormalize_struct();

        this_dot_pos -= rhs_dot_pos;


        array_uint<byte_expand> move_tmp = 10;
        while (true) {
            if (!(tmp.val.unsigned_int % move_tmp) && tmp.val.unsigned_int) {
                if (this_dot_pos <= max_dot_pos) {
                    this_dot_pos++;
                    tmp.val.unsigned_int /= move_tmp;
                    continue;
                }
            }
            this_dot_pos += denormalize_dot();
            val.unsigned_int /= tmp.val.unsigned_int;
            break;
        }
        normalize_dot(this_dot_pos);
    }
    void mod(const array_unreal& rhs) {
        uint64_t this_dot_pos = temp_denormalize_struct();
        array_unreal tmp = rhs;
        uint64_t rhs_dot_pos = tmp.temp_denormalize_struct();

        this_dot_pos -= rhs_dot_pos;


        array_uint<byte_expand> move_tmp = 10;
        while (true) {
            if (val.unsigned_int % tmp.val.unsigned_int) {
                this_dot_pos++;
                if (this_dot_pos != max_dot_pos) {
                    val.unsigned_int = val.unsigned_int * move_tmp;
                    continue;
                }
                this_dot_pos--;
            }
            this_dot_pos += denormalize_dot();
            val.unsigned_int %= tmp.val.unsigned_int;
            break;
        }
        normalize_dot(this_dot_pos);
    }
public:
    static const uint64_t max_dot_pos;
    void mod_dot(uint64_t pos) {
        val.dot_pos = pos;
    }
    static array_unreal get_min() {
        return 0;
    }
    static array_unreal get_max() {
        array_unreal tmp = 0;
        tmp.val.unsigned_int = array_uint<byte_expand>::get_max();
        tmp.val.dot_pos = 0;
        return tmp;
    }

    array_unreal() {
        val.unsigned_int = 0;
    }
    array_unreal(const array_unreal& rhs) {
        val.unsigned_int = rhs.val.unsigned_int;
    }
    array_unreal(array_unreal&& rhs) {
        val.unsigned_int = rhs.val.unsigned_int;
    }
    array_unreal(const char* str) {
        *this = std::string(str);
    }
    array_unreal(const std::string str) {
        size_t found_pos = str.find('.');
        if (found_pos == std::string::npos)
        {
            val.unsigned_int = array_uint<byte_expand>(str.c_str());
            val.dot_pos = 0;
        }
        else {
            //check str for second dot
            if (str.find('.', found_pos) == std::string::npos)
                throw std::invalid_argument("Real value can contain only one dot");

            std::string tmp = str;
            tmp.erase(tmp.begin() + found_pos);
            val.unsigned_int = array_uint<byte_expand>(tmp.c_str());
            val.dot_pos = (str.length() - found_pos - 1);
        }
        normalize_struct(temp_denormalize_struct());
    }
    template <uint64_t byt_expand>
    array_unreal(const array_unreal<byt_expand> rhs) {
        *this = rhs.to_ansi_string().c_str();
    }


    template <typename T>
    array_unreal(const T& rhs)
    {
        val.unsigned_int = (array_uint<byte_expand>)rhs;
    }
    template <typename T>
    array_unreal(const T&& rhs)
    {
        val.unsigned_int = (array_uint<byte_expand>)rhs;
    }
    std::string to_ansi_string() {
        uint64_t tmp = temp_denormalize_struct();
        std::string str = val.unsigned_int.to_ansi_string();
        normalize_struct(tmp);
        if (val.dot_pos) {
            if (str.length() <= val.dot_pos) {
                uint64_t resize_result_len = val.dot_pos - str.length() + 1;
                std::string to_add_zeros;
                while (resize_result_len--)
                    to_add_zeros += '0';
                str = to_add_zeros + str;
            }
            str.insert(str.end() - tmp, '.');
        }
        return str;
    }
    std::string to_ansi_string() const {
        return array_unreal(*this).to_ansi_string();
    }
    std::string to_hex_str() const {
        return val.unsigned_int.to_hex_str();
    }

    array_unreal& operator=(const array_unreal& rhs) = default;
    array_unreal& operator=(array_unreal&& rhs) = default;

    array_unreal operator&(const array_unreal& rhs) const {
        return array_unreal(*this) &= rhs;
    }

    array_unreal& operator&=(const array_unreal& rhs) {
        val.unsigned_int &= rhs.val.unsigned_int;
        return *this;
    }

    array_unreal operator|(const array_unreal& rhs) const {
        return array_unreal(*this) |= rhs;
    }

    array_unreal& operator|=(const array_unreal& rhs) {
        val.unsigned_int |= rhs.val.unsigned_int;
        return *this;
    }

    array_unreal operator^(const array_unreal& rhs) const {
        return array_unreal(*this) ^= rhs;
    }

    array_unreal& operator^=(const array_unreal& rhs) {
        val.unsigned_int ^= rhs.val.unsigned_int;
        return *this;
    }

    array_unreal operator~() const {
        array_unreal tmp(*this);
        ~tmp.val.unsigned_int;
        return tmp;
    }



    array_unreal& operator<<=(uint64_t shift) {
        val.unsigned_int <<= shift;
        return *this;
    }
    array_unreal operator<<(uint64_t shift) const {
        return array_unreal(*this) <<= shift;
    }


    array_unreal& operator>>=(uint64_t shift) {
        val.unsigned_int >>= shift;
        return *this;
    }
    array_unreal operator>>(uint64_t shift) const {
        return array_unreal(*this) >>= shift;
    }
    bool operator!() const {
        return !val.unsigned_int;
    }

    bool operator&&(const array_unreal& rhs) const {
        return ((bool)*this && rhs);
    }

    bool operator||(const array_unreal& rhs) const {
        return ((bool)*this || rhs);
    }

    bool operator==(const array_unreal& rhs) const {
        return val.unsigned_int == rhs.val.unsigned_int;
    }

    bool operator!=(const array_unreal& rhs) const {
        return val.unsigned_int != rhs.val.unsigned_int;
    }

    bool operator>(const array_unreal& rhs) const {
        std::vector<std::string> this_parts = split_dot(to_ansi_string());
        std::vector<std::string> rhs_parts = split_dot(rhs.to_ansi_string());
        {
            array_uint<byte_expand> temp1(this_parts[0].c_str());
            array_uint<byte_expand> temp2(rhs_parts[0].c_str());
            if (temp1 == temp2);
            else return temp1 > temp2;
        }
        if (this_parts.size() == 2 && rhs_parts.size() == 2) {
            array_uint<byte_expand> temp1(this_parts[1].c_str());
            array_uint<byte_expand> temp2(rhs_parts[1].c_str());
            return temp1 > temp2;
        }
        else
            return this_parts.size() > rhs_parts.size();
    }

    bool operator<(const array_unreal& rhs) const {
        return !(*this > rhs) && *this != rhs;
    }

    bool operator>=(const array_unreal& rhs) const {
        return ((*this > rhs) | (*this == rhs));
    }

    bool operator<=(const array_unreal& rhs) const {
        return !(*this > rhs);
    }


    array_unreal& operator++() {
        return *this += 1;
    }
    array_unreal operator++(int) {
        array_unreal temp(*this);
        *this += 1;
        return temp;
    }
    array_unreal operator+(const array_unreal& rhs) const {
        return array_unreal(*this) += rhs;
    }
    array_unreal operator-(const array_unreal& rhs) const {
        return array_unreal(*this) -= rhs;
    }

    array_unreal& operator+=(const array_unreal& rhs) {
        uint64_t this_dot_pos = temp_denormalize_struct();
        array_unreal tmp = rhs;
        uint64_t rhs_dot_pos = tmp.temp_denormalize_struct();
        if (this_dot_pos == rhs_dot_pos);
        else if (this_dot_pos > rhs_dot_pos) {
            array_uint<byte_expand> move_tmp = 1;
            while (this_dot_pos != rhs_dot_pos++)
                move_tmp *= 10;
            tmp.val.unsigned_int *= move_tmp;
        }
        else {
            array_uint<byte_expand> move_tmp = 1;
            while (rhs_dot_pos != this_dot_pos++)
                move_tmp *= 10;
            val.unsigned_int *= move_tmp;
        }
        val.unsigned_int += tmp.val.unsigned_int;
        normalize_struct(this_dot_pos);
        normalize_dot();
        return *this;
    }
    array_unreal& operator-=(const array_unreal& rhs) {
        uint64_t this_dot_pos = temp_denormalize_struct();
        array_unreal tmp = rhs;
        uint64_t rhs_dot_pos = tmp.temp_denormalize_struct();
        if (this_dot_pos == rhs_dot_pos);
        else if (this_dot_pos > rhs_dot_pos) {
            array_uint<byte_expand> move_tmp = 1;
            while (this_dot_pos != rhs_dot_pos++)
                move_tmp *= 10;
            tmp.val.unsigned_int *= move_tmp;
        }
        else {
            array_uint<byte_expand> move_tmp = 1;
            while (rhs_dot_pos != this_dot_pos++)
                move_tmp *= 10;
            val.unsigned_int *= move_tmp;
        }
        val.unsigned_int -= tmp.val.unsigned_int;
        normalize_struct(this_dot_pos);
        normalize_dot();
        return *this;
    }

    array_unreal operator*(const array_unreal& rhs) const {
        return array_unreal(*this) *= rhs;
    }


    array_unreal& operator*=(const array_unreal& rhs) {
        uint64_t this_dot_pos = temp_denormalize_struct();
        array_unreal tmp = rhs;
        uint64_t rhs_dot_pos = tmp.temp_denormalize_struct();
        this_dot_pos += rhs_dot_pos;

        val.unsigned_int *= tmp.val.unsigned_int;

        normalize_struct(this_dot_pos);
        normalize_dot();
        return *this;
    }

    array_unreal operator/(const array_unreal& rhs) const {
        return array_unreal(*this) /= rhs;
    }

    array_unreal& operator/=(const array_unreal& rhs) {
        div(rhs);
        return *this;
    }

    array_unreal operator%(const array_unreal& rhs) const {
        return array_unreal(*this) %= rhs;
    }

    array_unreal& operator%=(const array_unreal& rhs) {
        mod(rhs);
        return *this;
    }

    explicit operator bool() const {
        return (bool)val.unsigned_int;
    }
    explicit operator uint8_t() const {
        return (uint8_t)(array_uint<byte_expand>)val.unsigned_int;
    }
    explicit operator uint16_t() const {
        return (uint16_t)(array_uint<byte_expand>)val.unsigned_int;
    }
    explicit operator uint32_t() const {
        return (uint32_t)(array_uint<byte_expand>)val.unsigned_int;
    }
    explicit operator uint64_t() const {
        return (uint64_t)(array_uint<byte_expand>)val.unsigned_int;
    }


    template <uint64_t byt_expand>
    explicit operator array_int<byt_expand>() const {
        return (array_int<byt_expand>)((array_uint<byt_expand>)*this);
    }
    template <uint64_t byt_expand>
    explicit operator array_uint<byt_expand>() const {
        array_unreal tmp = *this;
        array_uint<byte_expand> div = 10;
        uint64_t temp = tmp.temp_denormalize_struct();
        for (uint64_t i = 0; i < temp; temp++)
            tmp.val.unsigned_int /= div;
        return tmp.val.unsigned_int;
    }
    template <uint64_t byt_expand>
    explicit operator array_real<byt_expand>() const {
        return to_ansi_string().c_str();
    }
    template <uint64_t byt_expand>
    explicit operator array_unreal<byt_expand>() {
        return to_ansi_string().c_str();
    }

    explicit operator double() const {
        array_unreal tmp = *this;
        array_uint<byte_expand> div = 10;
        uint64_t temp = tmp.temp_denormalize_struct();
        for (uint64_t i = 0; i < temp; i++)
            tmp.val.unsigned_int /= div;

        double res = (uint64_t)tmp.val.unsigned_int;
        for (uint64_t i = 0; i < temp; i++)
            tmp.val.unsigned_int *= div;
        tmp = *this - tmp.val.unsigned_int;

        double res_part = (uint64_t)tmp.val.unsigned_int;;
        for (uint64_t i = 0; i < temp; i++)
            res_part /= 10;
        return res + res_part;
    }
};
template<uint64_t byte_expand>
const uint64_t array_unreal<byte_expand>::max_dot_pos = []() {
    uint64_t _dot_pos : (byte_expand >= 58 ? 64 : (5 + byte_expand));
    _dot_pos = -1;
    return _dot_pos;
}();

class real64_t {
    union for_constructor {
        struct s {
        public:
            uint64_t unused : 58;
            uint64_t _dot_pos : 5;
            uint64_t _is_minus : 1;
            s() {

            }
            s(uint64_t val) {
                _dot_pos = val;
            }
            void operator = (uint64_t val) {
                _dot_pos = val;
            }
            operator uint64_t() const {
                return _dot_pos;
            }
            bool is_minus() const {
                return _is_minus;
            }
            void modify_minus(bool bol) {
                _is_minus = bol;
            }
        } dot_pos;
        int64_t signed_int = 0;
        for_constructor() {}
    } val;


    static std::vector<std::string> split_dot(std::string value) {
        std::vector<std::string> strPairs;
        size_t pos = 0;
        if ((pos = value.find(".")) != std::string::npos) {
            strPairs.push_back(value.substr(0, pos));
            value.erase(0, pos + 1);
        }
        if (!value.empty())
            strPairs.push_back(value);
        return strPairs;
    }

    uint64_t temp_denormalize_struct() {
        uint64_t tmp = val.dot_pos;
        val.dot_pos = 0;
        return tmp;
    }
    void normalize_struct(uint64_t value) {
        val.dot_pos = value;
    }
    void normalize_dot(uint64_t cur_dot_pos = -1) {
        if (cur_dot_pos == (uint64_t)-1)
            cur_dot_pos = temp_denormalize_struct();

        if (cur_dot_pos == 0)
            return;

        while (val.dot_pos || cur_dot_pos > 31) {
            val.signed_int /= 10;
            cur_dot_pos--;
        }
        std::string tmp_this_value = to_ansi_string();
        int64_t remove_nuls_mult(10);
        int64_t remove_nuls(1);
        size_t nul_count = 0;
        for (int64_t i = tmp_this_value.length() - 1; i >= 0; i--)
        {
            if (tmp_this_value[i] != '0')
                break;
            else {
                remove_nuls *= remove_nuls_mult;
                nul_count++;
            }
        }
        uint64_t modify_dot = cur_dot_pos - nul_count;
        if (nul_count)
            val.signed_int /= remove_nuls;
        normalize_struct(modify_dot);
    }

    uint64_t denormalize_dot() {
        if (val.signed_int) {
            uint64_t this_dot_pos = temp_denormalize_struct();
            uint64_t mult_int = 10;
            for (char i = 0; i < 24; i++) {
                if ((val.signed_int * mult_int) / 10 == val.signed_int && this_dot_pos != 31) {
                    val.signed_int *= mult_int;
                    this_dot_pos++;
                    mult_int *= 10;
                }
                else
                    break;
            }
            return this_dot_pos;
        }
        return 0;
    }

    void div(const real64_t& rhs) {
        uint64_t this_dot_pos = temp_denormalize_struct();
        real64_t tmp = rhs;
        uint64_t rhs_dot_pos = tmp.temp_denormalize_struct();
        this_dot_pos -= rhs_dot_pos;
        while (true) {
            if (!(tmp.val.signed_int % 10) && tmp.val.signed_int) {
                if (this_dot_pos != 31) {
                    this_dot_pos++;
                    tmp.val.signed_int /= 10;
                    continue;
                }
            }
            this_dot_pos += denormalize_dot();
            val.signed_int /= tmp.val.signed_int;
            break;
        }
        normalize_dot(this_dot_pos);
    }
    void mod(const real64_t& rhs) {
        uint64_t this_dot_pos = temp_denormalize_struct();
        real64_t tmp = rhs;
        uint64_t rhs_dot_pos = tmp.temp_denormalize_struct();
        this_dot_pos -= rhs_dot_pos;
        while (true) {
            if (!(tmp.val.signed_int % 10) && tmp.val.signed_int) {
                if (this_dot_pos != 31) {
                    this_dot_pos++;
                    tmp.val.signed_int /= 10;
                    continue;
                }
            }
            this_dot_pos += denormalize_dot();
            val.signed_int %= tmp.val.signed_int;
            break;
        }
        normalize_dot(this_dot_pos);
    }
public:
    void mod_dot(uint64_t pos) {
        val.dot_pos = pos;
    }
    static real64_t get_min() {
        real64_t tmp = 0;
        tmp.val.signed_int = 0x8000000000000000;
        tmp.val.dot_pos = 0;
        return tmp;
    }
    static real64_t get_max() {
        real64_t tmp = 0;
        tmp.val.signed_int = 0x7FFFFFFFFFFFFFFF;
        tmp.val.dot_pos = 0;
        return tmp;
    }

    real64_t() {
        val.signed_int = 0;
    }
    real64_t(const real64_t& rhs) {
        val.signed_int = rhs.val.signed_int;
    }
    real64_t(real64_t&& rhs) noexcept {
        val.signed_int = rhs.val.signed_int;
    }
    real64_t(const char* str) {
        *this = std::string(str);
    }
    real64_t(const std::string str) {
        size_t found_pos = str.find('.');
        if (found_pos == std::string::npos)
        {
            val.signed_int = std::stoull(str.c_str());
            val.dot_pos = 0;
        }
        else {
            //check str for second dot
            if (str.find('.', found_pos) == std::string::npos)
                throw std::invalid_argument("Real value can contain only one dot");

            std::string tmp = str;
            tmp.erase(tmp.begin() + found_pos);
            val.signed_int = std::stoull(tmp.c_str());
            val.dot_pos = (str.length() - found_pos - 1);
        }
        normalize_struct(temp_denormalize_struct());
    }
    template <uint64_t byt_expand>
    real64_t(const array_int<byt_expand>& rhs) {
        *this = rhs.to_ansi_string().c_str();
    }
    template <typename T>
    real64_t(const T& rhs)
    {
        val.signed_int = (int64_t)rhs;
    }
    template <typename T>
    real64_t(const T&& rhs)
    {
        val.signed_int = (int64_t)rhs;
    }
    std::string to_ansi_string() {
        uint64_t tmp = temp_denormalize_struct();
        bool has_minus = val.dot_pos.is_minus();
        if (has_minus)
            val.dot_pos = -1;

        std::string str = std::to_string(val.signed_int);
        if (has_minus)
            str.erase(str.begin());
        normalize_struct(tmp);
        if (val.dot_pos) {
            if (str.length() <= val.dot_pos) {
                uint64_t resize_result_len = val.dot_pos - str.length() + 1;
                std::string to_add_zeros;
                while (resize_result_len--)
                    to_add_zeros += '0';
                str = to_add_zeros + str;
            }
            str.insert(str.end() - tmp, '.');
        }
        return (has_minus ? "-" : "") + str;
    }
    std::string to_ansi_string() const {
        return real64_t(*this).to_ansi_string();
    }

    real64_t& operator=(const real64_t& rhs) = default;
    real64_t& operator=(real64_t&& rhs) = default;

    real64_t operator&(const real64_t& rhs) const {
        return real64_t(*this) &= rhs;
    }

    real64_t& operator&=(const real64_t& rhs) {
        val.signed_int &= rhs.val.signed_int;
        return *this;
    }

    real64_t operator|(const real64_t& rhs) const {
        return real64_t(*this) |= rhs;
    }

    real64_t& operator|=(const real64_t& rhs) {
        val.signed_int |= rhs.val.signed_int;
        return *this;
    }

    real64_t operator^(const real64_t& rhs) const {
        return real64_t(*this) ^= rhs;
    }

    real64_t& operator^=(const real64_t& rhs) {
        val.signed_int ^= rhs.val.signed_int;
        return *this;
    }

    real64_t operator~() const {
        real64_t tmp(*this);
        tmp.val.signed_int = ~tmp.val.signed_int;
        return tmp;
    }



    real64_t& operator<<=(uint64_t shift) {
        val.signed_int <<= shift;
        return *this;
    }
    real64_t operator<<(uint64_t shift) const {
        return real64_t(*this) <<= shift;
    }


    real64_t& operator>>=(uint64_t shift) {
        val.signed_int >>= shift;
        return *this;
    }
    real64_t operator>>(uint64_t shift) const {
        return real64_t(*this) >>= shift;
    }
    bool operator!() const {
        return !val.signed_int;
    }

    bool operator&&(const real64_t& rhs) const {
        return ((bool)*this && rhs);
    }

    bool operator||(const real64_t& rhs) const {
        return ((bool)*this || rhs);
    }

    bool operator==(const real64_t& rhs) const {
        return val.signed_int == rhs.val.signed_int;
    }

    bool operator!=(const real64_t& rhs) const {
        return val.signed_int != rhs.val.signed_int;
    }

    bool operator>(const real64_t& rhs) const {
        std::vector<std::string> this_parts = split_dot(to_ansi_string());
        std::vector<std::string> rhs_parts = split_dot(rhs.to_ansi_string());
        {
            uint64_t temp1 = std::stoull(this_parts[0].c_str());
            uint64_t temp2 = std::stoull(rhs_parts[0].c_str());
            if (temp1 == temp2);
            else return temp1 > temp2;
        }
        if (this_parts.size() == 2 && rhs_parts.size() == 2) {
            uint64_t temp1 = std::stoull(this_parts[1].c_str());
            uint64_t temp2 = std::stoull(rhs_parts[1].c_str());
            return temp1 > temp2;
        }
        else
            return this_parts.size() > rhs_parts.size();
    }

    bool operator<(const real64_t& rhs) const {
        return !(*this > rhs) && *this != rhs;
    }

    bool operator>=(const real64_t& rhs) const {
        return ((*this > rhs) || (*this == rhs));
    }

    bool operator<=(const real64_t& rhs) const {
        return !(*this > rhs);
    }


    real64_t& operator++() {
        return *this += 1;
    }
    real64_t operator++(int) {
        real64_t temp(*this);
        *this += 1;
        return temp;
    }
    real64_t operator+(const real64_t& rhs) const {
        return real64_t(*this) += rhs;
    }
    real64_t operator-(const real64_t& rhs) const {
        return real64_t(*this) -= rhs;
    }

    real64_t& operator+=(const real64_t& rhs) {
        uint64_t this_dot_pos = temp_denormalize_struct();
        real64_t tmp = rhs;
        uint64_t rhs_dot_pos = tmp.temp_denormalize_struct();
        if (this_dot_pos == rhs_dot_pos);
        else if (this_dot_pos > rhs_dot_pos) {
            uint64_t move_tmp = 1;
            while (this_dot_pos != rhs_dot_pos++)
                move_tmp *= 10;
            tmp.val.signed_int *= move_tmp;
        }
        else {
            uint64_t move_tmp = 1;
            while (rhs_dot_pos != this_dot_pos++)
                move_tmp *= 10;
            val.signed_int *= move_tmp;
        }
        val.signed_int += tmp.val.signed_int;
        normalize_struct(this_dot_pos);
        normalize_dot();
        return *this;
    }
    real64_t& operator-=(const real64_t& rhs) {
        uint64_t this_dot_pos = temp_denormalize_struct();
        real64_t tmp = rhs;
        uint64_t rhs_dot_pos = tmp.temp_denormalize_struct();
        if (this_dot_pos == rhs_dot_pos);
        else if (this_dot_pos > rhs_dot_pos) {
            uint64_t move_tmp = 1;
            while (this_dot_pos != rhs_dot_pos++)
                move_tmp *= 10;
            tmp.val.signed_int *= move_tmp;
        }
        else {
            uint64_t move_tmp = 1;
            while (rhs_dot_pos != this_dot_pos++)
                move_tmp *= 10;
            val.signed_int *= move_tmp;
        }
        val.signed_int -= tmp.val.signed_int;
        normalize_struct(this_dot_pos);
        normalize_dot();
        return *this;
    }

    real64_t operator*(const real64_t& rhs) const {
        return real64_t(*this) *= rhs;
    }


    real64_t& operator*=(const real64_t& rhs) {
        uint64_t this_dot_pos = temp_denormalize_struct();
        real64_t tmp = rhs;
        uint64_t rhs_dot_pos = tmp.temp_denormalize_struct();
        this_dot_pos += rhs_dot_pos;

        val.signed_int *= tmp.val.signed_int;

        normalize_struct(this_dot_pos);
        normalize_dot();
        return *this;
    }

    real64_t operator/(const real64_t& rhs) const {
        return real64_t(*this) /= rhs;
    }

    real64_t& operator/=(const real64_t& rhs) {
        div(rhs);
        return *this;
    }

    real64_t operator%(const real64_t& rhs) const {
        return real64_t(*this) %= rhs;
    }

    real64_t& operator%=(const real64_t& rhs) {
        mod(rhs);
        return *this;
    }

    explicit operator bool() const {
        return (bool)val.signed_int;
    }
    explicit operator uint8_t() const {
        return (uint8_t)val.signed_int;
    }
    explicit operator uint16_t() const {
        return (uint16_t)val.signed_int;
    }
    explicit operator uint32_t() const {
        return (uint32_t)val.signed_int;
    }
    explicit operator uint64_t() const {
        real64_t tmp = *this;
        tmp.temp_denormalize_struct();
        return tmp.val.signed_int;
    }


    explicit operator int64_t() const {
        real64_t tmp = *this;
        tmp.temp_denormalize_struct();
        return tmp.val.signed_int;
    }
    template <uint64_t byt_expand>
    explicit operator array_real<byt_expand>() {
        return to_ansi_string().c_str();
    }

    explicit operator double() const {
        real64_t tmp = *this;
        uint64_t div = 10;
        uint64_t temp = tmp.temp_denormalize_struct();
        for (uint64_t i = 0; i < temp; temp++)
            tmp.val.signed_int /= div;

        double res = (int64_t)tmp.val.signed_int;
        for (uint64_t i = 0; i < temp && i < 15; temp++)
            tmp.val.signed_int *= div;
        tmp = *this - tmp.val.signed_int;
        double res_part = (int64_t)tmp.val.signed_int;;
        for (uint64_t i = 0; i < temp && i<15; temp++)
            res_part /= 10;
        return res + res_part;
    }

};

class unreal64_t {
    union for_constructor {
        struct s {
            uint64_t unused : 59;
            uint64_t _dot_pos : 5;
        public:
            s() {

            }
            s(uint64_t val) {
                _dot_pos = val;
            }
            void operator = (uint64_t val) {
                _dot_pos = val;
            }
            operator uint64_t() const {
                return _dot_pos;
            }
        } dot_pos;
        uint64_t unsigned_int;
        for_constructor() {}
    } val;
    static std::vector<uint64_t> split_dot(std::string value) {
        std::vector<uint64_t> strPairs;
        size_t pos = 0;
        if ((pos = value.find(".")) != std::string::npos) {
            strPairs.push_back(std::stoull(value.substr(0, pos)));
            value.erase(0, pos + 1);
        }
        if (!value.empty())
            strPairs.push_back(std::stoull(value));
        return strPairs;
    }

    uint64_t temp_denormalize_struct() {
        uint64_t tmp = val.dot_pos;
        val.dot_pos = 0;
        return tmp;
    }
    void normalize_struct(uint64_t value) {
        val.dot_pos = value;
    }
    void normalize_dot(uint64_t cur_dot_pos = -1) {
        if (cur_dot_pos == (uint64_t)-1)
            cur_dot_pos = temp_denormalize_struct();

        if (cur_dot_pos == 0)
            return;

        while (val.dot_pos || cur_dot_pos > 31) {
            val.unsigned_int /= 10;
            cur_dot_pos--;
        }
        std::string tmp_this_value = to_ansi_string();
        unreal64_t remove_nuls_mult(10);
        unreal64_t remove_nuls(1);
        size_t nul_count = 0;
        for (int64_t i = tmp_this_value.length() - 1; i >= 0; i--)
        {
            if (tmp_this_value[i] != '0')
                break;
            else {
                remove_nuls *= remove_nuls_mult;
                nul_count++;
            }
        }
        uint64_t modify_dot = temp_denormalize_struct() - nul_count;
        if (nul_count)
            div(remove_nuls, false);
        normalize_struct(modify_dot);
    }
    uint64_t denormalize_dot() {
        if (val.unsigned_int) {
            uint64_t this_dot_pos = temp_denormalize_struct();
            uint64_t mult_int = 10;
            for (char i = 0; i < 24; i++) {
                if ((val.unsigned_int * mult_int) / 10 == val.unsigned_int && this_dot_pos != 31) {
                    val.unsigned_int *= mult_int;
                    this_dot_pos++;
                    mult_int *= 10;
                }
                else
                    break;
            }
            return this_dot_pos;
        }
        return 0;
    }
    void div(const unreal64_t& rhs, bool do_normalize_dot = true) {
        uint64_t this_dot_pos = temp_denormalize_struct();
        unreal64_t tmp = rhs;
        uint64_t rhs_dot_pos = tmp.temp_denormalize_struct();

        this_dot_pos -= rhs_dot_pos;
        while (true) {
            if (!(tmp.val.unsigned_int % 10) && tmp.val.unsigned_int) {
                if (this_dot_pos != 31) {
                    this_dot_pos++;
                    tmp.val.unsigned_int /= 10;
                    continue;
                }
            }
            this_dot_pos += denormalize_dot();
            val.unsigned_int /= tmp.val.unsigned_int;
            break;
        }
        normalize_dot(this_dot_pos);
    }
    void mod(const unreal64_t& rhs, bool do_normalize_dot = true) {
        uint64_t this_dot_pos = temp_denormalize_struct();
        unreal64_t tmp = rhs;
        uint64_t rhs_dot_pos = tmp.temp_denormalize_struct();

        this_dot_pos -= rhs_dot_pos;


        unreal64_t shift_tmp;
        shift_tmp.val.dot_pos = -1;
        while (true) {
            if (val.unsigned_int % tmp.val.unsigned_int) {
                this_dot_pos++;
                if (this_dot_pos != shift_tmp.val.dot_pos) {
                    val.unsigned_int *= 10;
                    continue;
                }
                this_dot_pos--;
            }
            this_dot_pos += denormalize_dot();
            val.unsigned_int %= tmp.val.unsigned_int;
            break;
        }
        normalize_dot(this_dot_pos);
    }
public:
    void mod_dot(uint64_t pos) {
        val.dot_pos = pos;
    }
    static unreal64_t get_min() {
        unreal64_t tmp = 0;
        tmp.val.unsigned_int = 0;
        tmp.val.dot_pos = 0;
        return tmp;
    }
    static unreal64_t get_max() {
        unreal64_t tmp = 0;
        tmp.val.unsigned_int = -1;
        tmp.val.dot_pos = 0;
        return tmp;
    }

    unreal64_t() {
        val.unsigned_int = 0;
    }
    unreal64_t(const unreal64_t& rhs) {
        val.unsigned_int = rhs.val.unsigned_int;
    }
    unreal64_t(unreal64_t&& rhs) noexcept {
        val.unsigned_int = rhs.val.unsigned_int;
    }
    unreal64_t(const char* str) {
        *this = std::string(str);
    }
    unreal64_t(const std::string str) {
        size_t found_pos = str.find('.');
        if (found_pos == std::string::npos)
        {
            val.unsigned_int = std::stoull(str.c_str());
            val.dot_pos = 0;
        }
        else {
            //check str for second dot
            if (str.find('.', found_pos) == std::string::npos)
                throw std::invalid_argument("Real value can contain only one dot");

            std::string tmp = str;
            tmp.erase(tmp.begin() + found_pos);
            val.unsigned_int = std::stoull(tmp.c_str());
            val.dot_pos = (str.length() - found_pos - 1);
        }
        normalize_struct(temp_denormalize_struct());
    }

    //1 004 006 004 001
    template <typename T>
    unreal64_t(const T& rhs) {
        val.unsigned_int = (uint64_t)rhs;
    }
    template <typename T>
    unreal64_t(const T&& rhs) {
        val.unsigned_int = (uint64_t)rhs;
    }
    std::string to_ansi_string() {
        uint64_t tmp = temp_denormalize_struct();
        std::string str = std::to_string(val.unsigned_int);
        normalize_struct(tmp);
        if (val.dot_pos) {
            if (str.length() <= val.dot_pos) {
                uint64_t resize_result_len = val.dot_pos - str.length() + 1;
                std::string to_add_zeros;
                while (resize_result_len--)
                    to_add_zeros += '0';
                str = to_add_zeros + str;
            }
            str.insert(str.end() - tmp, '.');
        }
        return str;
    }
    std::string to_ansi_string() const {
        return unreal64_t(*this).to_ansi_string();
    }
    std::string to_hex_str() const {
        static const char* digits = "0123456789ABCDEF";
        std::string rc(8, '0');
        for (size_t i = 0, j = 28; i < 8; ++i, j -= 4)
            rc[i] = digits[(val.unsigned_int >> j) & 0x0f];
        return rc;
    }

    unreal64_t& operator=(const unreal64_t& rhs) = default;
    unreal64_t& operator=(unreal64_t&& rhs) = default;

    unreal64_t operator&(const unreal64_t& rhs) const {
        return unreal64_t(*this) &= rhs;
    }

    unreal64_t& operator&=(const unreal64_t& rhs) {
        val.unsigned_int &= rhs.val.unsigned_int;
        return *this;
    }

    unreal64_t operator|(const unreal64_t& rhs) const {
        return unreal64_t(*this) |= rhs;
    }

    unreal64_t& operator|=(const unreal64_t& rhs) {
        val.unsigned_int |= rhs.val.unsigned_int;
        return *this;
    }

    unreal64_t operator^(const unreal64_t& rhs) const {
        return unreal64_t(*this) ^= rhs;
    }

    unreal64_t& operator^=(const unreal64_t& rhs) {
        val.unsigned_int ^= rhs.val.unsigned_int;
        return *this;
    }

    unreal64_t operator~() const {
        unreal64_t tmp(*this);
        tmp.val.unsigned_int = ~tmp.val.unsigned_int;
        return tmp;
    }



    unreal64_t& operator<<=(uint64_t shift) {
        val.unsigned_int <<= shift;
        return *this;
    }
    unreal64_t operator<<(uint64_t shift) const {
        return unreal64_t(*this) <<= shift;
    }


    unreal64_t& operator>>=(uint64_t shift) {
        val.unsigned_int >>= shift;
        return *this;
    }
    unreal64_t operator>>(uint64_t shift) const {
        return unreal64_t(*this) >>= shift;
    }
    bool operator!() const {
        return !val.unsigned_int;
    }

    bool operator&&(const unreal64_t& rhs) const {
        return ((bool)*this && rhs);
    }

    bool operator||(const unreal64_t& rhs) const {
        return ((bool)*this || rhs);
    }

    bool operator==(const unreal64_t& rhs) const {
        return val.unsigned_int == rhs.val.unsigned_int;
    }

    bool operator!=(const unreal64_t& rhs) const {
        return val.unsigned_int != rhs.val.unsigned_int;
    }

    bool operator>(const unreal64_t& rhs) const {
        std::vector<uint64_t> this_parts = split_dot(to_ansi_string());
        std::vector<uint64_t> rhs_parts = split_dot(rhs.to_ansi_string());
        {
            if (this_parts[0] == rhs_parts[0]);
            else return this_parts[0] > rhs_parts[0];
        }
        if (this_parts.size() == 2 && rhs_parts.size() == 2) {
            return this_parts[1] > rhs_parts[1];
        }
        else
            return this_parts.size() > rhs_parts.size();
    }

    bool operator<(const unreal64_t& rhs) const {
        return !(*this > rhs) && *this != rhs;
    }

    bool operator>=(const unreal64_t& rhs) const {
        return ((*this > rhs) || (*this == rhs));
    }

    bool operator<=(const unreal64_t& rhs) const {
        return !(*this > rhs);
    }


    unreal64_t& operator++() {
        return *this += 1;
    }
    unreal64_t operator++(int) {
        unreal64_t temp(*this);
        *this += 1;
        return temp;
    }
    unreal64_t operator+(const unreal64_t& rhs) const {
        return unreal64_t(*this) += rhs;
    }
    unreal64_t operator-(const unreal64_t& rhs) const {
        return unreal64_t(*this) -= rhs;
    }

    unreal64_t& operator+=(const unreal64_t& rhs) {
        uint64_t this_dot_pos = temp_denormalize_struct();
        unreal64_t tmp = rhs;
        uint64_t rhs_dot_pos = tmp.temp_denormalize_struct();
        if (this_dot_pos == rhs_dot_pos);
        else if (this_dot_pos > rhs_dot_pos) {
            uint64_t move_tmp = 1;
            while (this_dot_pos != rhs_dot_pos++)
                move_tmp *= 10;
            tmp.val.unsigned_int *= move_tmp;
        }
        else {
            uint64_t move_tmp = 1;
            while (rhs_dot_pos != this_dot_pos++)
                move_tmp *= 10;
            val.unsigned_int *= move_tmp;
        }
        val.unsigned_int += tmp.val.unsigned_int;
        normalize_struct(this_dot_pos);
        normalize_dot();
        return *this;
    }
    unreal64_t& operator-=(const unreal64_t& rhs) {
        uint64_t this_dot_pos = temp_denormalize_struct();
        unreal64_t tmp = rhs;
        uint64_t rhs_dot_pos = tmp.temp_denormalize_struct();
        if (this_dot_pos == rhs_dot_pos);
        else if (this_dot_pos > rhs_dot_pos) {
            uint64_t move_tmp = 1;
            while (this_dot_pos != rhs_dot_pos++)
                move_tmp *= 10;
            tmp.val.unsigned_int *= move_tmp;
        }
        else {
            uint64_t move_tmp = 1;
            while (rhs_dot_pos != this_dot_pos++)
                move_tmp *= 10;
            val.unsigned_int *= move_tmp;
        }
        val.unsigned_int -= tmp.val.unsigned_int;
        normalize_struct(this_dot_pos);
        normalize_dot();
        return *this;
    }

    unreal64_t operator*(const unreal64_t& rhs) const {
        return unreal64_t(*this) *= rhs;
    }


    unreal64_t& operator*=(const unreal64_t& rhs) {
        uint64_t this_dot_pos = temp_denormalize_struct();
        unreal64_t tmp = rhs;
        uint64_t rhs_dot_pos = tmp.temp_denormalize_struct();
        this_dot_pos += rhs_dot_pos;

        val.unsigned_int *= tmp.val.unsigned_int;

        normalize_struct(this_dot_pos);
        normalize_dot();
        return *this;
    }

    unreal64_t operator/(const unreal64_t& rhs) const {
        return unreal64_t(*this) /= rhs;
    }

    unreal64_t& operator/=(const unreal64_t& rhs) {
        div(rhs);
        return *this;
    }

    unreal64_t operator%(const unreal64_t& rhs) const {
        return unreal64_t(*this) %= rhs;
    }

    unreal64_t& operator%=(const unreal64_t& rhs) {
        mod(rhs);
        return *this;
    }

    explicit operator bool() const {
        return (bool)val.unsigned_int;
    }
    explicit operator uint8_t() const {
        return (uint64_t)val.unsigned_int;
    }
    explicit operator uint16_t() const {
        return (uint64_t)val.unsigned_int;
    }
    explicit operator uint32_t() const {
        return (uint64_t)val.unsigned_int;
    }

    explicit operator int64_t() const {
        return (uint64_t)*this;
    }
    explicit operator uint64_t() const {
        unreal64_t tmp = *this;
        uint64_t div = 10;
        uint64_t temp = tmp.temp_denormalize_struct();
        for (uint64_t i = 0; i < temp; temp++)
            tmp.val.unsigned_int /= div;
        return tmp.val.unsigned_int;
    }

    explicit operator double() const {
        unreal64_t tmp = *this;
        uint64_t temp = tmp.temp_denormalize_struct();
        for (uint64_t i = 0; i < temp; temp++)
            tmp.val.unsigned_int /= 10;

        double res = (double)(uint64_t)tmp.val.unsigned_int;
        for (uint64_t i = 0; i < temp && i < 15; temp++)
            tmp.val.unsigned_int *= 10;
        tmp = *this - tmp.val.unsigned_int;

        double res_part = (double)(uint64_t)tmp.val.unsigned_int;;
        for (uint64_t i = 0; i < temp && i<15; temp++)
            res_part /= 10;
        return res + res_part;
    }
};




template<class REAL>
REAL get_pi_const(uint16_t numbers) {
    const char* tmp = "3141592653589793238462643383279502884197169399375105820974944592307816406286208998628034825342117067982148086513282306647093844609550582231725359408128481117450284102701938521105559644622948954930381964428810975665933446128475648233786783165271201909145648566923460348610454326648213393607260249141273724587006606315588174881520920962829254091715364367892590360011330530548820466521384146951941511609433057270365759591953092186117381932611793105118548074462379962749567351885752724891227938183011949129833673362440656643086021394946395224737190702179860943702770539217176293176752384674818467669405132000568127145263560827785771342757789609173637178721468440901224953430146549585371050792279689258923542019956112129021960864034418159813629774771309960518707211349999998372978049951059731732816096318595024459455346908302642522308253344685035261931188171010003137838752886587533208381420617177669147303598253490428755468731159562863882353787593751957781857780532171226806613001927876611195909216420198938095257201065485863278865936153381827968230301952035301852968995773622599413891249721775283479131515574857242454150695950829533116861727855889075098381754637464939319255060400927701671139009848824012858361603563707660104710181942955596198946767837449448255379774726847104047534646208046684259069491293313677028989152104752162056966024058038150193511253382430035587640247496473263914199272604269922796782354781636009341721641219924586315030286182974555706749838505494588586926995690927210797509302955321165344987202755960236480665499119881834797753566369807426542527862551818417574672890977772793800081647060016145249192173217214772350141441973568548161361157352552133475741849468438523323907394143334547762416862518983569485562099219222184272550254256887671790494601653466804988627232791786085784383827967976681454100953883786360950680064225125205117392984896084128488626945604241965285022210661186306744278622039194945047123713786960956364371917287467764657573962413890865832645995813390478027590099465764078951269468398352595709825822620522489407726719478268482601476990902640136394437455305068203496252451749399651431429809190659250937221696461515709858387410597885959772975498930161753928468138268683868942774155991855925245953959431049972524680845987273644695848653836736222626099124608051243884390451244136549762780797715691435997700129616089441694868555848406353422072225828488648158456028506016842739452267467678895252138522549954666727823986456596116354886230577456498035593634568174324112515076069479451096596094025228879710893145669136867228748940560101503308617928680920874760917824938589009714909675985261365549781893129784821682998948722658804857564014270477555132379641451523746234364542858444795265867821051141354735739523113427166102135969536231442952484937187110145765403590279934403742007310578539062198387447808478489683321445713868751943506430218453191048481005370614680674919278191197939952061419663428754440643745123718192179998391015919561814675142691239748940907186494231961567945208095146550225231603881930142093762137855956638937787083039069792077346722182562599661501421503068038447734549202605414665925201497442850732518666002132434088190710486331734649651453905796268561005508106658796998163574736384052571459102897064140110971206280439039759515677157700420337869936007230558763176359421873125147120532928191826186125867321579198414848829164470609575270695722091756711672291098169091528017350671274858322287183520935396572512108357915136988209144421006751033467110314126711136990865851639831501970165151168517143765761835155650884909989859982387345528331635507647918535893226185489632132933089857064204675259070915481416549859461637180270981994309924488957571282890592323326097299712084433573265489382391193259746366730583604142813883032038249037589852437441702913276561809377344403070746921120191302033038019762110110044929321516084244485963766983895228684783123552658213144957685726243344189303968642624341077322697802807318915441101044682325271620105265227211166039666557309254711055785376346682065310989652691862056476931257058635662018558100729360659876486117910453348850346113657686753249441668039626579787718556084552965412665408530614344431858676975145661406800700237877659134401712749470420562230538994561314071127000407854733269939081454664645880797270826683063432858785698305235808933065757406795457163775254202114955761581400250126228594130216471550979259230990796547376125517656751357517829666454779174501129961489030463994713296210734043751895735961458901938971311179042978285647503203198691514028708085990480109412147221317947647772622414254854540332157185306142288137585043063321751829798662237172159160771669254748738986654949450114654062843366393790039769265672146385306736096571209180763832716641627488880078692560290228472104031721186082041900042296617119637792133757511495950156604963186294726547364252308177036751590673502350728354056704038674351362222477158915049530984448933309634087807693259939780541934144737744184263129860809988868741326047215695162396586457302163159819319516735381297416772947867242292465436680098067692823828068996400482435403701416314965897940924323789690706977942236250822168895738379862300159377647165122893578601588161755782973523344604281512627203734314653197777416031990665541876397929334419521541341899485444734567383162499341913181480927777103863877343177207545654532207770921201905166096280490926360197598828161332316663652861932668633606273567630354477628035045077723554710585954870279081435624014517180624643626794561275318134078330336254232783944975382437205835311477119926063813346776879695970309833913077109870408591337464144282277263465947047458784778720192771528073176790770715721344473060570073349243693113835049316312840425121925651798069411352801314701304781643788518529092854520116583934196562134914341595625865865570552690496520985803385072242648293972858478316305777756068887644624824685792603953527734803048029005876075825104747091643961362676044925627420420832085661190625454337213153595845068772460290161876679524061634252257719542916299193064553779914037340432875262888963995879475729174642635745525407909145135711136941091193932519107602082520261879853188770584297259167781314969900901921169717372784768472686084900337702424291651300500516832336435038951702989392233451722013812806965011784408745196012122859937162313017114448464090389064495444006198690754851602632750529834918740786680881833851022833450850486082503930213321971551843063545500766828294930413776552793975175461395398468339363830474611996653858153842056853386218672523340283087112328278921250771262946322956398989893582116745627010218356462201349671518819097303811980049734072396103685406643193950979019069963955245300545058068550195673022921913933918568034490398205955100226353536192041994745538593810234395544959778377902374216172711172364343543947822181852862408514006660443325888569867054315470696574745855033232334210730154594051655379068662733379958511562578432298827372319898757141595781119635833005940873068121602876496286744604774649159950549737425626901049037781986835938146574126804925648798556145372347867330390468838343634655379498641927056387293174872332083760112302991136793862708943879936201629515413371424892830722012690147546684765357616477379467520049075715552781965362132392640616013635815590742202020318727760527721900556148425551879253034351398442532234157623361064250639049750086562710953591946589751413103482276930624743536325691607815478181152843667957061108615331504452127473924544945423682886061340841486377670096120715124914043027253860764823634143346235189757664521641376796903149501910857598442391986291642193994907236234646844117394032659184044378051333894525742399508296591228508555821572503107125701266830240292952522011872676756220415420516184163484756516999811614101002996078386909291603028840026910414079288621507842451670908700069928212066041837180653556725253256753286129104248776182582976515795984703562226293486003415872298053498965022629174878820273420922224533985626476691490556284250391275771028402799806636582548892648802545661017296702664076559042909945681506526530537182941270336931378517860904070866711496558343434769338578171138645587367812301458768712660348913909562009939361031029161615288138437909904231747336394804575931493140529763475748119356709110137751721008031559024853090669203767192203322909433467685142214477379393751703443661991040337511173547191855046449026365512816228824462575916333039107225383742182140883508657391771509682887478265699599574490661758344137522397096834080053559849175417381883999446974867626551658276584835884531427756879002909517028352971634456212964043523117600665101241200659755851276178583829204197484423608007193045761893234922927965019875187212726750798125547095890455635792122103334669749923563025494780249011419521238281530911407907386025152274299581807247162591668545133312394804947079119153267343028244186041426363954800044800267049624820179289647669758318327131425170296923488962766844032326092752496035799646925650493681836090032380929345958897069536534940603402166544375589004563288225054525564056448246515187547119621844396582533754388569094113031509526179378002974120766514793942590298969594699556576121865619673378623625612521632086286922210327488921865436480229678070576561514463204692790682120738837781423356282360896320806822246801224826117718589638140918390367367222088832151375560037279839400415297002878307667094447456013455641725437090697939612257142989467154357846878861444581231459357198492252847160504922124247014121478057345510500801908699603302763478708108175450119307141223390866393833952942578690507643100638351983438934159613185434754649556978103829309716465143840700707360411237359984345225161050702705623526601276484830840761183013052793205427462865403603674532865105706587488225698157936789766974220575059683440869735020141020672358502007245225632651341055924019027421624843914035998953539459094407046912091409387001264560016237428802109276457931065792295524988727584610126483699989225695968815920560010165525637567856672279661988578279484885583439751874454551296563443480396642055798293680435220277098429423253302257634180703947699415979159453006975214829336655566156787364005366656416547321704390352132954352916941459904160875320186837937023488868947915107163785290234529244077365949563051007421087142613497459561513849871375704710178795731042296906667021449863746459528082436944578977233004876476524133907592043401963403911473202338071509522201068256342747164602433544005152126693249341967397704159568375355516673027390074972973635496453328886984406119649616277344951827369558822075735517665158985519098666539354948106887320685990754079234240230092590070173196036225475647894064754834664776041146323390565134330684495397907090302346046147096169688688501408347040546074295869913829668246818571031887906528703665083243197440477185567893482308943106828702722809736248093996270607472645539925399442808113736943388729406307926159599546262462970706259484556903471197299640908941805953439325123623550813494900436427852713831591256898929519642728757394691427253436694153236100453730488198551706594121735246258954873016760029886592578662856124966552353382942878542534048308330701653722856355915253478445981831341129001999205981352205117336585640782648494276441137639386692480311836445369858917544264739988228462184490087776977631279572267265556259628254276531830013407092233436577916012809317940171859859993384923549564005709955856113498025249906698423301735035804408116855265311709957089942732870925848789443646005041089226691783525870785951298344172953519537885534573742608590290817651557803905946408735061232261120093731080485485263572282576820341605048466277504500312620080079980492548534694146977516493270950493463938243222718851597405470214828971117779237612257887347718819682546298126868581705074027255026332904497627789442362167411918626943965067151577958675648239939176042601763387045499017614364120469218237076488783419689686118155815873606293860381017121585527266830082383404656475880405138080163363887421637140643549556186896411228214075330265510042410489678352858829024367090488711819090949453314421828766181031007354770549815968077200947469613436092861484941785017180779306810854690009445899527942439813921350558642219648349151263901280383200109773868066287792397180146134324457264009737425700735921003154150893679300816998053652027600727749674584002836240534603726341655425902760183484030681138185510597970566400750942608788573579603732451414678670368809880609716425849759513806930944940151542222194329130217391253835591503100333032511174915696917450271494331515588540392216409722910112903552181576282328318234254832611191280092825256190205263016391147724733148573910777587442538761174657867116941477642144111126358355387136101102326798775641024682403226483464176636980663785768134920453022408197278564719839630878154322116691224641591177673225326433568614618654522268126887268445968442416107854016768142080885028005414361314623082102594173756238994207571362751674573189189456283525704413354375857534269869947254703165661399199968262824727064133622217892390317608542894373393561889165125042440400895271983787386480584726895462438823437517885201439560057104811949884239060613695734231559079670346149143447886360410318235073650277859089757827273130504889398900992391350337325085598265586708924261242947367019390772713070686917092646254842324074855036608013604668951184009366860954632500214585293095000090715105823626729326453738210493872499669933942468551648326113414611068026744663733437534076429402668297386522093570162638464852851490362932019919968828517183953669134522244470804592396602817156551565666111359823112250628905854914509715755390024393153519090210711945730024388017661503527086260253788179751947806101371500448991721002220133501310601639154158957803711779277522597874289191791552241718958536168059474123419339842021874564925644346239253195313510331147639491199507285843065836193536932969928983791494193940608572486396883690326556436421664425760791471086998431573374964883529276932822076294728238153740996154559879825989109371712621828302584811238901196822142945766758071865380650648702613389282299497257453033283896381843944770779402284359883410035838542389735424395647555684095224844554139239410001620769363684677641301781965937997155746854194633489374843912974239143365936041003523437770658886778113949861647874714079326385873862473288964564359877466763847946650407411182565837887845485814896296127399841344272608606187245545236064315371011274680977870446409475828034876975894832824123929296058294";
    REAL res = 0;
    REAL& multer = *new REAL(10);
    int16_t i = 0;
    uint64_t max_dot_pos = REAL::max_dot_pos;
    for (; i < numbers && i < 14508 && i < max_dot_pos; i++) {
        res *= multer;
        res += int(tmp[i]-'0');
    }
    delete& multer;
    res.mod_dot(i? i-1 : 0);
    return res;
}