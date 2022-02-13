#pragma once
#include <stdexcept>
#include <vector>
#include <string>


class infinity_t {
public:
    std::vector<uint64_t> integers;
    uint64_t bits() const {
        uint64_t out = 0;
        int64_t max_elem = integers.size() - 1;
        for (int64_t i = max_elem; i >= 0; --i)
            if (integers[i])
                max_elem = i;
        uint64_t tmp = integers[max_elem];
        out = (integers.size() - 1 - max_elem) * 64;
        while (tmp) {
            tmp >>= 1;
            out++;
        }
        return out;
    }
    std::pair <infinity_t, infinity_t> divmod(const infinity_t& lhs, const infinity_t& rhs) const {
        if (rhs == 0)
            throw std::domain_error("Error: division or modulus by 0");
        else if (rhs == 1)
            return std::pair <infinity_t, infinity_t>(lhs, 1);
        else if (lhs == rhs)
            return std::pair <infinity_t, infinity_t>(1, 0);
        else if ((lhs == 0) || (lhs < rhs))
            return std::pair <infinity_t, infinity_t>(0, lhs);

        std::pair <infinity_t, infinity_t> qr(0, lhs);
        infinity_t copyd = rhs << (lhs.bits() - rhs.bits());
        infinity_t adder = infinity_t(1) << (lhs.bits() - rhs.bits());
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
        return qr;
    }
    void zero_all() {
        size_t tot_bytes = integers.size();
        for (size_t i = 0; i < tot_bytes; i++)
            integers[i] = 0;
    }
public:


    infinity_t() : integers(1) {}
    infinity_t(std::vector<uint64_t> v) : integers(v) {}
    infinity_t(const infinity_t& rhs) {
        integers = rhs.integers;
    }
    infinity_t(infinity_t&& rhs) noexcept {
        integers = std::move(rhs.integers);
    }
    infinity_t& operator=(const infinity_t& rhs) = default;
    infinity_t& operator=(infinity_t&& rhs) noexcept {
        integers = std::move(rhs.integers);
    }
    infinity_t(const long long rhs) { integers.push_back(rhs); }
    infinity_t(const unsigned long long rhs) { integers.push_back(rhs); }
    infinity_t(const unsigned int rhs) { integers.push_back(rhs); }
    infinity_t(const double rhs) { integers.push_back(rhs); }
    infinity_t(const int rhs) { integers.push_back(rhs); }
    infinity_t(const std::string str) {
        *this = str.c_str();
    }
    infinity_t(const char* str) {
        integers.push_back(0);
        infinity_t mult = 10;
        size_t str_len = strlen(str);
        for (size_t i = 0; i < str_len; i++) {
            *this *= mult;
            *this += (int)(str[i] - '0');
        }
    }
    std::string to_ansi_string() const {
        std::string res;
        {
            std::pair <infinity_t, infinity_t> tmp(*this, 0);
            infinity_t dever(10000000000000000000);
            std::string len_tmp;
            while (true) {
                tmp = divmod(tmp.first, dever);
                len_tmp = std::to_string(tmp.second.integers[tmp.second.integers.size() - 1]);
                for (size_t i = len_tmp.size(); i < 19; i++)
                    len_tmp = '0' + len_tmp;
                res = len_tmp + res;
                if (tmp.first == 0)
                    break;
            }
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
        for (auto i : integers) {
             std::string rc(8, '0');
             for (size_t i = 0, j = 28; i < 8; ++i, j -= 4)
                 rc[i] = digits[(i >> j) & 0x0f];
             res += rc;
        }
        while (res[0] == '0')
            res.erase(res.begin());
        if (res.empty())
            res = '0';
        return "0x" + res;
    }




    static infinity_t pow(const infinity_t& A, const infinity_t& N) {
        return infinity_t(A).pow(N);
    }
    infinity_t& pow(const infinity_t& N) {
        infinity_t res = 1;
        infinity_t n = N;
        size_t tot_bits = n.bits();
        for (size_t i = 0; i < tot_bits; i++) {
            if (n.integers[n.integers.size()] & 1)
                res *= *this;
            *this *= *this;
            n >>= 1;
        }
        delete& n;
        return *this = res;
    }
    infinity_t& sqrt() {
        infinity_t a = 1;
        infinity_t b = *this;

        while (b - a > 1) {
            b = *this / a;
            a = (a + b) >> 1;
        }
        return *this = a;
    }

    infinity_t operator&(const infinity_t& rhs) const {
        return infinity_t(*this) &= rhs;
    }
    infinity_t& operator&=(const infinity_t& rhs) {
        if (rhs.integers.size() > integers.size())
            for (int64_t i = rhs.integers.size() - integers.size(); i > 0; --i)
                integers.insert(integers.begin(), 0);
        else if (rhs.integers.size() < integers.size()) {
            int64_t rhs_interator = rhs.integers.size() - 1;
            for (int64_t i = integers.size() - 1; i >= 0; --i)
                integers[i] &= (rhs_interator >= 0 ? rhs.integers[rhs_interator--] : 0);
            return *this;
        }
        size_t tot_bytes = integers.size();
        for (size_t i = 0; i < tot_bytes; i++)
            integers[i] &= rhs.integers[i];
        return *this;
    }
    infinity_t operator|(const infinity_t& rhs) const {
        return infinity_t(*this) |= rhs;
    }
    infinity_t& operator|=(const infinity_t& rhs) {
        if (rhs.integers.size() > integers.size())
            for (int64_t i = rhs.integers.size() - integers.size(); i > 0; --i)
                integers.insert(integers.begin(), 0);
        int64_t rhs_interator = rhs.integers.size() - 1;
        for (int64_t i = integers.size() - 1; i >= 0 && rhs_interator >= 0; --i)
            integers[i] |= rhs.integers[rhs_interator--];

        return *this;
    }
    infinity_t operator^(const infinity_t& rhs) const {
        return infinity_t(*this) ^= rhs;
    }
    infinity_t& operator^=(const infinity_t& rhs) {
        if (rhs.integers.size() > integers.size()) {
            for (int64_t i = rhs.integers.size() - integers.size(); i > 0; --i)
                integers.insert(integers.begin(), 0);
        }
        else if (rhs.integers.size() < integers.size()) {
            int64_t rhs_interator = rhs.integers.size() - 1;
            for (int64_t i = integers.size() - 1; i >= 0; --i)
                integers[i] ^= (rhs_interator >= 0 ? rhs.integers[rhs_interator--] : 0);
            return *this;
        }
        size_t tot_bytes = integers.size();
        for (size_t i = 0; i < tot_bytes; i++)
            integers[i] ^= rhs.integers[i];
        return *this;
    }
    infinity_t operator~() const {
        infinity_t res(*this);
        size_t tot_bytes = integers.size();
        for (size_t i = 0; i < tot_bytes; i++)
            res.integers[i] = ~integers[i];
        return res;
    }





    infinity_t& operator<<=(uint64_t shift) {
        uint64_t bits1 = 0, bits2 = 0;
        uint64_t block_shift = shift / 64;
        uint64_t sub_block_shift = shift % 64;
        for (size_t block_shift_c = 0; block_shift_c < block_shift; block_shift_c++)
            integers.push_back(0);

        if (sub_block_shift) {
            integers.insert(integers.begin(), 0);
            bits1 = bits2 = 0;
            uint64_t anti_shift = 64 - sub_block_shift;
            uint64_t and_op = (uint64_t)-1 << anti_shift;
            for (int64_t i = integers.size() - 1; i >= 0; --i) {
                bits2 = integers[i] & and_op;
                integers[i] <<= sub_block_shift;
                integers[i] |= bits1 >> anti_shift;
                bits1 = bits2;
            }
            if (integers[0] == 0 && (integers.size() - 1))
                integers.erase(integers.begin());
        }
        return *this;
    }
    infinity_t& operator>>=(uint64_t shift) {
        uint64_t bits1 = 0, bits2 = 0;
        uint64_t block_shift = shift / 64;
        uint64_t sub_block_shift = shift % 64;
        for (size_t block_shift_c = 0; block_shift_c < block_shift; block_shift_c++) {
            if (!integers.size())
                break;
            integers.pop_back();
        }
        if (sub_block_shift) {
            bits1 = bits2 = 0;
            uint64_t anti_shift = 64 - sub_block_shift;
            uint64_t and_op = -1 >> anti_shift;
            uint64_t limit = integers.size();
            for (size_t i = 0; i < limit; i++) {
                bits2 = integers[i] & and_op;
                integers[i] >>= sub_block_shift;
                integers[i] |= bits1 << anti_shift;
                bits1 = bits2;
            }
        }
        if (integers.size()) {
            if (integers[0] == 0 && (integers.size() - 1))
                integers.erase(integers.begin());
        }
        else
            integers.push_back(0);
        return *this;
    }



    infinity_t operator<<(uint64_t shift) const {
        return infinity_t(*this) <<= shift;
    }
    infinity_t operator>>(uint64_t shift) const {
        return infinity_t(*this) >>= shift;
    }


    bool operator!() const {
        return !(bool)(*this);
    }
    bool operator&&(const infinity_t& rhs) const {
        return ((bool)*this && rhs);
    }
    bool operator||(const infinity_t& rhs) const {
        return ((bool)*this || rhs);
    }
    bool operator==(const infinity_t& rhs) const {
        return integers == rhs.integers;
    }
    bool operator!=(const infinity_t& rhs) const {
        return integers != rhs.integers;
    }
    bool operator>(const infinity_t& rhs) const {
        if (integers.size() > rhs.integers.size())
            return true;
        if (integers.size() < rhs.integers.size())
            return false;
        size_t tot_bytes = integers.size();
        for (size_t i = 0; i < tot_bytes; i++) {
            if (integers[i] > rhs.integers[i])
                return true;
            else if (integers[i] < rhs.integers[i])
                return false;
        }
        return false;
    }
    bool operator<(const infinity_t& rhs) const {
        if (integers.size() < rhs.integers.size())
            return true;
        if (integers.size() > rhs.integers.size())
            return false;
        size_t tot_bytes = integers.size();
        for (size_t i = 0; i < tot_bytes; i++) {
            if (integers[i] < rhs.integers[i])
                return true;
            else if (integers[i] > rhs.integers[i])
                return false;
        }
        return false;
    }
    bool operator>=(const infinity_t& rhs) const {
        return !(*this < rhs);
    }
    bool operator<=(const infinity_t& rhs) const {
        return !(*this > rhs);
    }


    infinity_t& operator++() {
        return *this += 1;
    }
    infinity_t operator++(int) {
        infinity_t temp(*this);
        *this += 1;
        return temp;
    }
    infinity_t& operator--() {
        return *this -= 1;
    }
    infinity_t operator--(int) {
        infinity_t temp(*this);
        --* this;
        return temp;
    }

    infinity_t operator+(const infinity_t& rhs) const {
        return infinity_t(*this) += rhs;
    }
    infinity_t operator-(const infinity_t& rhs) const {
        return infinity_t(*this) -= rhs;
    }
    infinity_t& operator+=(const infinity_t& rhs) {
        bool bits1 = 0, bits2 = 0;
        for (int64_t i = rhs.integers.size() - integers.size(); i > 0; i--)
            integers.insert(integers.begin(), 0);

        int64_t rhs_interator = rhs.integers.size() - 1;
        for (int64_t i = integers.size() - 1; i >= 0; --i) {
            if (rhs_interator < 0) {
                if (bits2) integers[i]++;
                bits2 = 0;
                break;
            }
            bits1 = ((integers[i] + rhs.integers[rhs_interator] + bits2) < integers[i]);
            integers[i] += rhs.integers[rhs_interator] + bits2;
            bits2 = bits1;
            rhs_interator--;
        }
        if (bits2)
            integers.insert(integers.begin(), 1);
        return *this;
    }
    infinity_t& operator-=(const infinity_t& rhs) {
        if (integers.size() < rhs.integers.size())
            return *this = 0;
        bool bits1 = 0, bits2 = 0;

        int64_t rhs_interator = rhs.integers.size() - 1;
        for (int64_t i = integers.size() - 1; i >= 0; --i) {
            if (rhs_interator < 0) {
                if (bits2)
                    integers[i]--;
                break;
            }

            bits1 = ((integers[i] - rhs.integers[rhs_interator] - bits2) > integers[i]);
            integers[i] -= rhs.integers[rhs_interator] + bits2;
            bits2 = bits1;
            rhs_interator--;
        }

        if (integers[0] == 0 && integers.size())
            integers.erase(integers.begin());
        return *this;
    }


    infinity_t operator*(const infinity_t& rhs) const {
        infinity_t res(0);
        infinity_t multer = rhs;
        size_t control_size = multer.integers.size();
        uint64_t tot_bits = multer.bits();
        for (size_t i = 0; i < tot_bits; i++) {
            if (multer.integers[multer.integers.size() - 1] & 1)
                res += *this << i;
            multer >>= 1;
        }
        return res;
    }
    infinity_t& operator*=(const infinity_t& rhs) {
        return *this = *this * rhs;
    }

    infinity_t operator/(const infinity_t& rhs) const {
        return divmod(*this, rhs).first;
    }
    infinity_t& operator/=(const infinity_t& rhs) {
        return *this = divmod(*this, rhs).first;
    }

    infinity_t operator%(const infinity_t& rhs) const {
        return divmod(*this, rhs).second;
    }
    infinity_t& operator%=(const infinity_t& rhs) {
        *this = divmod(*this, rhs).second;
        return *this;
    }
    explicit operator bool() const {
        for (int v : integers)
            if (v)
                return true;
        return false;
    }
    explicit operator uint8_t() const {
        return integers.size() ? (uint8_t)integers[integers.size() - 1] : 0;
    }
    explicit operator uint16_t() const {
        return integers.size() ? (uint16_t)integers[integers.size() - 1] : 0;
    }
    explicit operator uint32_t() const {
        return integers.size() ? (uint32_t)integers[integers.size() - 1] : 0;
    }
    explicit operator uint64_t() const {
        return integers.size() ? (uint64_t)integers[integers.size() - 1] : 0;
    }
};

class signed_infinity_t {
    signed_infinity_t& switch_my_siqn() {
        unsigned_int = ~unsigned_int;
        unsigned_int += 1;
        is_minus = !is_minus;
        return *this;
    }
    signed_infinity_t switch_my_siqn() const {
        return signed_infinity_t(*this).switch_my_siqn();
    }

    signed_infinity_t& switch_to_unsiqn() {
        if (is_minus)
            switch_my_siqn();
        return *this;
    }
    signed_infinity_t switch_to_unsiqn() const {
        if (is_minus)
            return signed_infinity_t(*this).switch_my_siqn();
        else return *this;
    }

    signed_infinity_t& switch_to_siqn() {
        if (!is_minus)
            switch_my_siqn();
        return *this;
    }
    signed_infinity_t switch_to_siqn() const {
        if (!is_minus)
            return signed_infinity_t(*this).switch_my_siqn();
        else return *this;
    }

    infinity_t unsigned_int;
    bool is_minus;
public:
    signed_infinity_t() {
        is_minus = 0;
    }
    signed_infinity_t(std::vector<uint64_t> v,bool minus) : unsigned_int(v), is_minus(minus) {}
    signed_infinity_t(const signed_infinity_t& rhs) {
        unsigned_int = rhs.unsigned_int;
    }
    signed_infinity_t(signed_infinity_t&& rhs) noexcept {
        unsigned_int = std::move(rhs.unsigned_int);
    }
    signed_infinity_t(const std::string str) {
        *this = str.c_str();
    }
    signed_infinity_t(const char* str) {
        bool set_minus = 0;
        if (*str++ == '-')
            set_minus = 1;
        else str--;
        unsigned_int = str;
        if (set_minus) {
            switch_to_siqn();
            is_minus = 1;
        }
    }
    template <uint64_t byt_expand>
    signed_infinity_t(const signed_infinity_t& rhs) {
        signed_infinity_t tmp = rhs;
        bool is_minus = tmp.is_minus;
        tmp.is_minus = 0;
        unsigned_int = tmp.unsigned_int;
        is_minus = is_minus;
    }
    template <typename T>
    signed_infinity_t(const T& rhs)
    {
        unsigned_int = (infinity_t)rhs;
    }
    template <typename T>
    signed_infinity_t(const T&& rhs)
    {
        unsigned_int = (infinity_t)rhs;
    }
    signed_infinity_t(const long long rhs) { *this = std::to_string(rhs).c_str(); }
    signed_infinity_t(const int rhs) { *this = std::to_string(rhs).c_str(); }

    std::string to_ansi_string() const {
        if (is_minus)
            return
            '-' + signed_infinity_t(*this).switch_to_unsiqn().
            unsigned_int.to_ansi_string();
        else
            return unsigned_int.to_ansi_string();
    }
    std::string to_hex_str() const {
        return unsigned_int.to_hex_str();
    }

    signed_infinity_t& operator=(const signed_infinity_t& rhs) = default;
    signed_infinity_t& operator=(signed_infinity_t&& rhs) noexcept = default;


    signed_infinity_t operator&(const signed_infinity_t& rhs) const {
        return signed_infinity_t(*this) &= rhs;
    }
    signed_infinity_t& operator&=(const signed_infinity_t& rhs) {
        unsigned_int &= rhs.unsigned_int;
        is_minus &= rhs.is_minus;
        return *this;
    }
    signed_infinity_t operator|(const signed_infinity_t& rhs) const {
        return signed_infinity_t(*this) |= rhs;
    }
    signed_infinity_t& operator|=(const signed_infinity_t& rhs) {
        unsigned_int |= rhs.unsigned_int;
        is_minus |= rhs.is_minus;
        return *this;
    }
    signed_infinity_t operator^(const signed_infinity_t& rhs) const {
        return signed_infinity_t(*this) ^= rhs;
    }
    signed_infinity_t& operator^=(const signed_infinity_t& rhs) {
        unsigned_int ^= rhs.unsigned_int;
        is_minus ^= rhs.is_minus;
        return *this;
    }
    signed_infinity_t operator~() const {
        signed_infinity_t tmp(*this);
        ~tmp.unsigned_int;
        return tmp;
    }


    signed_infinity_t& operator<<=(uint64_t shift) {
        unsigned_int <<= shift;
        return *this;
    }
    signed_infinity_t operator<<(uint64_t shift) const {
        return signed_infinity_t(*this) <<= shift;
    }
    signed_infinity_t& operator>>=(uint64_t shift) {
        unsigned_int >>= shift;
        return *this;
    }
    signed_infinity_t operator>>(uint64_t shift) const {
        return signed_infinity_t(*this) >>= shift;
    }

    bool operator!() const {
        return !((bool)unsigned_int || is_minus);
    }
    bool operator&&(const signed_infinity_t& rhs) const {
        return ((bool)*this && rhs && is_minus == rhs.is_minus);
    }
    bool operator||(const signed_infinity_t& rhs) const {
        return ((bool)*this || rhs || is_minus == rhs.is_minus);
    }
    bool operator==(const signed_infinity_t& rhs) const {
        return unsigned_int == rhs.unsigned_int && is_minus == rhs.is_minus;
    }
    bool operator!=(const signed_infinity_t& rhs) const {
        return unsigned_int != rhs.unsigned_int || is_minus != rhs.is_minus;
    }
    bool operator>(const signed_infinity_t& rhs) const {
        if (is_minus && !rhs.is_minus)
            return false;
        if (!is_minus && rhs.is_minus)
            return true;
        return signed_infinity_t(*this).switch_to_unsiqn().unsigned_int > rhs.switch_to_unsiqn().unsigned_int;
    }
    bool operator<(const signed_infinity_t& rhs) const {
        if (is_minus && !rhs.is_minus)
            return true;
        if (!is_minus && rhs.is_minus)
            return false;
        return signed_infinity_t(*this).switch_to_unsiqn().unsigned_int < rhs.switch_to_unsiqn().unsigned_int;
    }
    bool operator>=(const signed_infinity_t& rhs) const {
        return ((*this > rhs) || (*this == rhs));
    }
    bool operator<=(const signed_infinity_t& rhs) const {
        return ((*this < rhs) || (*this == rhs));
    }

    signed_infinity_t& operator++() {
        if (is_minus) {
            switch_my_siqn().unsigned_int += 1;
            switch_my_siqn();
        }
        else
            unsigned_int++;
        return *this;
    }
    signed_infinity_t operator++(int) {
        signed_infinity_t temp(*this);
        ++* this;
        return temp;
    }
    signed_infinity_t& operator--() {
        if (is_minus) {
            switch_my_siqn().unsigned_int -= 1;
            switch_my_siqn();
        }
        else
            unsigned_int--;
        return *this;
    }
    signed_infinity_t operator--(int) {
        signed_infinity_t temp(*this);
        --* this;
        return temp;
    }

    signed_infinity_t operator+(const signed_infinity_t& rhs) const {
        return signed_infinity_t(*this) += rhs;
    }
    signed_infinity_t operator-(const signed_infinity_t& rhs) const {
        return signed_infinity_t(*this) -= rhs;
    }

    signed_infinity_t& operator+=(const signed_infinity_t& rhs) {
        if (is_minus && rhs.is_minus) {
            switch_my_siqn().unsigned_int += rhs.switch_my_siqn().unsigned_int;
            switch_to_siqn();
        }
        else if (is_minus) {
            if (switch_my_siqn().unsigned_int < rhs.unsigned_int) {
                unsigned_int = rhs.unsigned_int - unsigned_int;
                switch_to_unsiqn();
            }
            else {
                unsigned_int -= rhs.unsigned_int;
                switch_to_siqn();
            }
        }
        else if (rhs.is_minus) {
            signed_infinity_t temp = rhs;
            temp.switch_my_siqn();
            if (unsigned_int < temp.unsigned_int) {
                unsigned_int = temp.unsigned_int - unsigned_int;
                switch_to_siqn();
            }
            else
                unsigned_int -= temp.unsigned_int;
        }
        else
            unsigned_int += rhs.unsigned_int;
        return *this;
    }
    signed_infinity_t& operator-=(const signed_infinity_t& rhs) {
        if (is_minus && rhs.is_minus) {
            if (*this > rhs) {
                switch_my_siqn().unsigned_int -= rhs.switch_my_siqn().unsigned_int;
                switch_my_siqn();
            }
            else {
                switch_my_siqn();
                unsigned_int = rhs.switch_my_siqn().unsigned_int - unsigned_int;
            }
        }
        else if (is_minus) {
            switch_my_siqn();
            if (this->unsigned_int > rhs.unsigned_int) {
                unsigned_int += rhs.unsigned_int;
                switch_to_siqn();
            }
            else
                unsigned_int -= rhs.unsigned_int;
        }
        else if (rhs.is_minus) {
            signed_infinity_t* temp = new signed_infinity_t(rhs);
            temp->switch_my_siqn();
            unsigned_int += temp->unsigned_int;
            delete temp;
        }
        else {
            if (unsigned_int >= rhs.unsigned_int)
                unsigned_int -= rhs.unsigned_int;
            else {
                unsigned_int = rhs.unsigned_int - unsigned_int;
                switch_my_siqn();
            }
        }
        return *this;
    }

    signed_infinity_t operator*(const signed_infinity_t& rhs) const {
        return signed_infinity_t(*this) *= rhs;
    }
    signed_infinity_t& operator*=(const signed_infinity_t& rhs) {
        if (is_minus && rhs.is_minus)
            switch_my_siqn().unsigned_int *= rhs.switch_my_siqn().unsigned_int;
        else if (is_minus || rhs.is_minus) {
            switch_to_unsiqn().unsigned_int *= rhs.switch_to_unsiqn().unsigned_int;
            switch_to_siqn();
        }
        else
            unsigned_int *= rhs.unsigned_int;
        return *this;
    }


    signed_infinity_t operator/(const signed_infinity_t& rhs) const {
        return signed_infinity_t(*this) /= rhs;
    }
    signed_infinity_t& operator/=(const signed_infinity_t& rhs) {
        if (is_minus && rhs.is_minus)
            switch_my_siqn().unsigned_int /= rhs.switch_my_siqn().unsigned_int;
        else if (is_minus || rhs.is_minus) {
            switch_to_unsiqn().unsigned_int /= rhs.switch_to_unsiqn().unsigned_int;
            switch_my_siqn();
        }
        else
            unsigned_int /= rhs.unsigned_int;
        return *this;
    }

    signed_infinity_t operator%(const signed_infinity_t& rhs) const {
        return signed_infinity_t(*this) %= rhs;
    }
    signed_infinity_t& operator%=(const signed_infinity_t& rhs) {
        if (is_minus && rhs.is_minus)
            switch_my_siqn().unsigned_int %= rhs.switch_my_siqn().unsigned_int;
        else if (is_minus || rhs.is_minus) {
            switch_to_unsiqn().unsigned_int %= rhs.switch_to_unsiqn().unsigned_int;
            switch_my_siqn();
        }
        else
            unsigned_int %= rhs.unsigned_int;
        return *this;
    }


    signed_infinity_t operator+() const {
        return switch_to_unsiqn();
    }
    signed_infinity_t operator-() const {
        return switch_to_siqn();
    }


    explicit operator bool() const {
        return (bool)unsigned_int;
    }
    explicit operator uint8_t() const {
        return (uint8_t)unsigned_int;
    }
    explicit operator uint16_t() const {
        return (uint16_t)unsigned_int;
    }
    explicit operator uint32_t() const {
        return (uint32_t)unsigned_int;
    }
    explicit operator uint64_t() const {
        return (uint64_t)unsigned_int;
    }
    explicit operator int64_t() const {
        return std::stoll(to_ansi_string());;
    }
};

class real_infinity_t {
    size_t dot_pos = 0;
    signed_infinity_t signed_int;


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

    void normalize_dot() {
        if (dot_pos == 0)
            return;
        std::string tmp_this_value = to_ansi_string();
        real_infinity_t remove_nuls_mult(10);
        real_infinity_t remove_nuls(1);
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
        if (nul_count)
            div(remove_nuls, false);
    }
public:
    static const size_t max_dot_pos;
    void div(const real_infinity_t& rhs, infinity_t limit_range = 100, bool do_normalize_dot = true) {
        dot_pos -= rhs.dot_pos;


        signed_infinity_t move_tmp = 10;
        real_infinity_t shift_tmp;
        shift_tmp.dot_pos = -1;
        infinity_t range_limiter = 0;
        while (true) {
            if (signed_int % rhs.signed_int) {
                dot_pos++;
                if (dot_pos != shift_tmp.dot_pos) {
                    signed_int = signed_int * move_tmp;
                    if (range_limiter++ < limit_range)
                        continue;
                }
                dot_pos--;
            }
            signed_int /= rhs.signed_int;
            break;
        }
        if (do_normalize_dot)
            normalize_dot();
    }
    void mod(const real_infinity_t& rhs, infinity_t limit_range = 100, bool do_normalize_dot = true) {
        dot_pos -= rhs.dot_pos;


        signed_infinity_t move_tmp = 10;
        real_infinity_t shift_tmp;
        shift_tmp.dot_pos = -1;
        infinity_t range_limiter = 0;
        while (true) {
            if (signed_int % rhs.signed_int) {
                dot_pos++;
                if (dot_pos != shift_tmp.dot_pos) {
                    signed_int = signed_int * move_tmp;
                    if (range_limiter++ < limit_range)
                        continue;
                }
                dot_pos--;
            }
            signed_int %= rhs.signed_int;
            break;
        }
        if (do_normalize_dot)
            normalize_dot();
    }
    real_infinity_t() {
        signed_int = 0;
    }
    real_infinity_t(const real_infinity_t& rhs) {
        signed_int = rhs.signed_int;
        dot_pos = rhs.dot_pos;
    }
    real_infinity_t(real_infinity_t&& rhs) noexcept {
        signed_int = std::move(rhs.signed_int);
        dot_pos = rhs.dot_pos;
    }
    real_infinity_t(const char* str) {
        *this = std::string(str);
    }
    real_infinity_t(const std::string str) {
        size_t found_pos = str.find('.');
        if (found_pos == std::string::npos)
        {
            signed_int = str.c_str();
            dot_pos = 0;
        }
        else {
            //check str for second dot
            if (str.find('.', found_pos) == std::string::npos)
                throw std::invalid_argument("Real value can contain only one dot");

            std::string tmp = str;
            tmp.erase(tmp.begin() + found_pos);
            signed_int = tmp.c_str();
            dot_pos = (str.length() - found_pos - 1);
        }
    }
    real_infinity_t(const infinity_t& rhs) {
        *this = rhs.to_ansi_string().c_str();
    }
    template <typename T>
    real_infinity_t(const T& rhs)
    {
        signed_int = rhs;
    }
    template <typename T>
    real_infinity_t(const T&& rhs)
    {
        signed_int = rhs;
    }
    std::string to_ansi_string() {
        bool has_minus;
        std::string str = signed_int.to_ansi_string();
        if (has_minus = str[0] == '-')
            str.erase(str.begin());

        if (dot_pos) {
            if ((infinity_t)str.length() <= dot_pos) {
                infinity_t resize_result_len = dot_pos - str.length() + 1;
                std::string to_add_zeros;
                while (resize_result_len--)
                    to_add_zeros += '0';
                str = to_add_zeros + str;
            }
            str.insert(str.end() - (uint64_t)dot_pos, '.');
        }
        return (has_minus ? "-" : "") + str;
    }
    std::string to_ansi_string() const {
        return real_infinity_t(*this).to_ansi_string();
    }
    std::string to_hex_str() const {
        return signed_int.to_hex_str();
    }

    real_infinity_t& operator=(const real_infinity_t& rhs) = default;
    real_infinity_t& operator=(real_infinity_t&& rhs) noexcept = default;
    real_infinity_t& operator=(const char* str) {
        return operator=(real_infinity_t(str));
    }

    real_infinity_t operator&(const real_infinity_t& rhs) const {
        return real_infinity_t(*this) &= rhs;
    }

    real_infinity_t& operator&=(const real_infinity_t& rhs) {
        signed_int &= rhs.signed_int;
        return *this;
    }

    real_infinity_t operator|(const real_infinity_t& rhs) const {
        return real_infinity_t(*this) |= rhs;
    }

    real_infinity_t& operator|=(const real_infinity_t& rhs) {
        signed_int |= rhs.signed_int;
        return *this;
    }

    real_infinity_t operator^(const real_infinity_t& rhs) const {
        return real_infinity_t(*this) ^= rhs;
    }

    real_infinity_t& operator^=(const real_infinity_t& rhs) {
        signed_int ^= rhs.signed_int;
        return *this;
    }

    real_infinity_t operator~() const {
        real_infinity_t tmp(*this);
        ~tmp.signed_int;
        return tmp;
    }



    real_infinity_t& operator<<=(uint64_t shift) {
        signed_int <<= shift;
        return *this;
    }
    real_infinity_t operator<<(uint64_t shift) const {
        return real_infinity_t(*this) <<= shift;
    }


    real_infinity_t& operator>>=(uint64_t shift) {
        signed_int >>= shift;
        return *this;
    }
    real_infinity_t operator>>(uint64_t shift) const {
        return real_infinity_t(*this) >>= shift;
    }
    bool operator!() const {
        return !signed_int;
    }

    bool operator&&(const real_infinity_t& rhs) const {
        return ((bool)*this && rhs);
    }

    bool operator||(const real_infinity_t& rhs) const {
        return ((bool)*this || rhs);
    }

    bool operator==(const real_infinity_t& rhs) const {
        return signed_int == rhs.signed_int;
    }

    bool operator!=(const real_infinity_t& rhs) const {
        return signed_int != rhs.signed_int;
    }

    bool operator>(const real_infinity_t& rhs) const {
        std::vector<std::string> this_parts = split_dot(to_ansi_string());
        std::vector<std::string> rhs_parts = split_dot(rhs.to_ansi_string());
        {
            signed_infinity_t temp1(this_parts[0].c_str());
            signed_infinity_t temp2(rhs_parts[0].c_str());
            if (temp1 == temp2);
            else return temp1 > temp2;
        }
        if (this_parts.size() == 2 && rhs_parts.size() == 2) {
            signed_infinity_t temp1(this_parts[1].c_str());
            signed_infinity_t temp2(rhs_parts[1].c_str());
            return temp1 > temp2;
        }
        else
            return this_parts.size() > rhs_parts.size();
    }

    bool operator<(const real_infinity_t& rhs) const {
        return !(*this > rhs) && *this != rhs;
    }

    bool operator>=(const real_infinity_t& rhs) const {
        return ((*this > rhs) ? true : (*this == rhs));
    }

    bool operator<=(const real_infinity_t& rhs) const {
        return !(*this > rhs);
    }


    real_infinity_t& operator++() {
        return *this += 1;
    }
    real_infinity_t operator++(int) {
        real_infinity_t temp(*this);
        *this += 1;
        return temp;
    }
    real_infinity_t operator+(const real_infinity_t& rhs) const {
        return real_infinity_t(*this) += rhs;
    }
    real_infinity_t operator-(const real_infinity_t& rhs) const {
        return real_infinity_t(*this) -= rhs;
    }

    real_infinity_t& operator+=(const real_infinity_t& rhs) {
        real_infinity_t tmp = rhs;
        if (dot_pos == rhs.dot_pos);
        else if (dot_pos > rhs.dot_pos) {
            signed_infinity_t move_tmp = 1;
            while (dot_pos != tmp.dot_pos++)
                move_tmp *= 10;
            tmp.signed_int *= move_tmp;
        }
        else {
            signed_infinity_t move_tmp = 1;
            while (tmp.dot_pos != dot_pos++)
                move_tmp *= 10;
            signed_int *= move_tmp;
        }
        signed_int += tmp.signed_int;
        normalize_dot();
        return *this;
    }
    real_infinity_t& operator-=(const real_infinity_t& rhs) {
        real_infinity_t tmp = rhs;
        if (dot_pos == tmp.dot_pos);
        else if (dot_pos > tmp.dot_pos) {
            signed_infinity_t move_tmp = 1;
            while (dot_pos != tmp.dot_pos++)
                move_tmp *= 10;
            tmp.signed_int *= move_tmp;
        }
        else {
            signed_infinity_t move_tmp = 1;
            while (tmp.dot_pos != dot_pos++)
                move_tmp *= 10;
            signed_int *= move_tmp;
        }
        signed_int -= tmp.signed_int;
        normalize_dot();
        return *this;
    }

    real_infinity_t operator*(const real_infinity_t& rhs) const {
        return real_infinity_t(*this) *= rhs;
    }


    real_infinity_t& operator*=(const real_infinity_t& rhs) {
        dot_pos += rhs.dot_pos;
        signed_int *= rhs.signed_int;
        normalize_dot();
        return *this;
    }

    real_infinity_t operator/(const real_infinity_t& rhs) const {
        return real_infinity_t(*this) /= rhs;
    }

    real_infinity_t& operator/=(const real_infinity_t& rhs) {
        div(rhs);
        return *this;
    }

    real_infinity_t operator%(const real_infinity_t& rhs) const {
        return real_infinity_t(*this) %= rhs;
    }

    real_infinity_t& operator%=(const real_infinity_t& rhs) {
        mod(rhs);
        return *this;
    }

    explicit operator bool() const {
        return (bool)signed_int;
    }
    explicit operator uint8_t() const {
        return (uint8_t)signed_int;
    }
    explicit operator uint16_t() const {
        return (uint16_t)signed_int;
    }
    explicit operator uint32_t() const {
        return (uint32_t)signed_int;
    }
    explicit operator uint64_t() const {
        return (uint64_t)signed_int;
    }
    explicit operator double() const {
        real_infinity_t tmp = *this;
        signed_infinity_t div = 10;
        for (infinity_t i = 0; i < tmp.dot_pos; i++)
            tmp.signed_int /= div;

        double res = (int64_t)tmp.signed_int;
        for (infinity_t i = 0; i < tmp.dot_pos; i++)
            tmp.signed_int *= div;
        tmp = *this - tmp.signed_int;
        double res_part = (int64_t)tmp.signed_int;;
        for (infinity_t i = 0; i < tmp.dot_pos; i++)
            res_part /= 10;
        return res + res_part;
    }
};
const size_t real_infinity_t::max_dot_pos = -1;
