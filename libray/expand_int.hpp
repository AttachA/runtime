#pragma once
#include <stdint.h>
#include <malloc.h>
#include <utility>
#include <stdexcept>
#include <iostream>

#define IS_BIG_ENDIAN (!(union { uint16_t u16; unsigned char c; }){ .u16 = 1 }.c)


namespace {
    template <typename T> std::string n2hexstr(T w, size_t hex_len = sizeof(T) << 1) {
        static const char* digits = "0123456789ABCDEF";
        std::string rc(hex_len, '0');
        for (size_t i = 0, j = (hex_len - 1) * 4; i < hex_len; ++i, j -= 4)
            rc[i] = digits[(w >> j) & 0x0f];
        return rc;
    }
}


template<uint64_t byte_expand>
class expand_uint;










template<>
class expand_uint<0> {
#ifdef IS_BIG_ENDIAN
    uint64_t LOWER;
    uint64_t UPPER;
#else
    uint64_t UPPER;
    uint64_t LOWER;
#endif
    uint64_t bits() const {
        uint64_t out = 0;
        if (UPPER) {
            out = 64;
            uint64_t up = UPPER;
            while (up) {
                up >>= 1;
                out++;
            }
        }
        else {
            uint64_t low = LOWER;
            while (low) {
                low >>= 1;
                out++;
            }
        }
        return out;
    }
    std::pair <expand_uint, expand_uint> divmod(const expand_uint& lhs, const expand_uint& rhs) const {
        // Save some calculations /////////////////////
        if (rhs == exint_0) {
            throw std::domain_error("Error: division or modulus by 0");
        }
        else if (rhs == exint_1) {
            return std::pair <expand_uint, expand_uint>(lhs, exint_0);
        }
        else if (lhs == rhs) {
            return std::pair <expand_uint, expand_uint>(exint_1, exint_0);
        }
        else if ((lhs == exint_0) || (lhs < rhs)) {
            return std::pair <expand_uint, expand_uint>(exint_0, lhs);
        }

        std::pair <expand_uint, expand_uint> qr(exint_0, exint_0);
        for (uint64_t x = lhs.bits(); x > 0; x--) {
            qr.first <<= 1;
            qr.second <<= 1;

            if ((lhs >> (x - 1U)) & exint_1) {
                ++qr.second;
            }

            if (qr.second >= rhs) {
                qr.second -= rhs;
                ++qr.first;
            }
        }
        return qr;
    }

public:
    expand_uint static get_min() {
        return 0;
    }
    expand_uint static get_max() {
        return expand_uint(-1, -1);
    }


    const uint64_t& upper() const {
        return UPPER;
    }
    const uint64_t& lower() const {
        return LOWER;
    }
    static constexpr uint64_t expand_bits() {
        return 128;
    }
    static constexpr uint64_t expand_bits_childs() {
        return 64;
    }
    static constexpr uint64_t expand_bits_sub_childs() {
        return 32;
    }

    expand_uint() : UPPER(0), LOWER(0) {};
    expand_uint(const expand_uint& rhs) = default;
    expand_uint(expand_uint&& rhs) = default;
    template <uint64_t byt_expand>
    expand_uint(const expand_uint<byt_expand>& rhs) {
        uint64_t* rhs_proxy = ((uint64_t*)&rhs);
        uint64_t* proxy = ((uint64_t*)this);
        if constexpr (expand_bits() <= expand_uint<byt_expand>::expand_bits())
            for (int64_t i = expand_bits() / 64 - 1; i >= 0; --i)
                proxy[i] = rhs_proxy[i];
        else
            for (int64_t i = expand_uint<byt_expand>::expand_bits() / 64 - 1; i >= 0; --i)
                proxy[i] = rhs_proxy[i];
    }

    expand_uint(const long long rhs) : UPPER(0), LOWER(rhs) {}
    expand_uint(const unsigned long long rhs) : UPPER(0), LOWER(rhs) {}
    expand_uint(const unsigned int rhs) : UPPER(0), LOWER(rhs) {}
    expand_uint(const double rhs) : UPPER(0), LOWER(rhs) {}
    expand_uint(const int rhs) : UPPER(0), LOWER(rhs) {}

    template <typename S, typename T>
    expand_uint(const S& upper_rhs, const T& lower_rhs) : UPPER(upper_rhs), LOWER(lower_rhs)
    {}

    expand_uint(const char* str) : UPPER(0), LOWER(0) {
        expand_uint mult(10);
        size_t str_len = strlen(str);
        for (size_t i = 0; i < str_len; i++) {
            *this *= mult;
            *this += str[i] - '0';
        }
    }
    std::string to_ansi_string() const {
        std::string res;
        {
            expand_uint tmp = *this;
            std::pair<expand_uint, expand_uint> temp;
            expand_uint dever(10);
            while (true) {
                res += (uint8_t)(tmp % dever) + '0';
                tmp /= dever;
                if (tmp == exint_0)
                    break;
            }
        }
        std::reverse(res.begin(), res.end());
        return res;
    }
    std::string to_ansi_string(uint8_t base) const {
        if ((base < 2) || (base > 36)) {
            throw std::invalid_argument("Base must be in the range 2-36");
        }
        std::string out = "";
        if (!(*this)) {
            out = "0";
        }
        else {
            std::pair <expand_uint, expand_uint> qr(*this, exint_0);
            do {
                qr = divmod(qr.first, base);
                out = "0123456789abcdefghijklmnopqrstuvwxyz"[(uint8_t)qr.second] + out;
            } while (qr.first);
        }
        return out;
    }

    std::string to_hex_str() const {
        uint64_t* proxy = ((uint64_t*)this);
        std::stringstream stream;
        for (int64_t i = expand_bits() / 64 - 1; i >= 0; --i)
            stream << n2hexstr(proxy[i]);
        return "0x" + stream.str();
    }

    expand_uint& operator=(const expand_uint& rhs) = default;
    expand_uint& operator=(expand_uint&& rhs) = default;



    static const expand_uint exint_0;
    static const expand_uint exint_1;

    expand_uint operator&(const expand_uint& rhs) const {
        return expand_uint(UPPER & rhs.UPPER, LOWER & rhs.LOWER);
    }
    expand_uint& operator&=(const expand_uint& rhs) {
        UPPER &= rhs.UPPER;
        LOWER &= rhs.LOWER;
        return *this;
    }
    expand_uint operator|(const expand_uint& rhs) const {
        return expand_uint(UPPER | rhs.UPPER, LOWER | rhs.LOWER);
    }
    expand_uint& operator|=(const expand_uint& rhs) {
        UPPER |= rhs.UPPER;
        LOWER |= rhs.LOWER;
        return *this;
    }
    expand_uint operator^(const expand_uint& rhs) const {
        return expand_uint(UPPER ^ rhs.UPPER, LOWER ^ rhs.LOWER);
    }
    expand_uint& operator^=(const expand_uint& rhs) {
        UPPER ^= rhs.UPPER;
        LOWER ^= rhs.LOWER;
        return *this;
    }

    expand_uint operator~() const {
        return expand_uint(~UPPER, ~LOWER);
    }
    expand_uint& operator<<=(uint64_t shift) {
        if (shift == expand_bits_childs()) {
            UPPER = LOWER;
            LOWER = 0;
        }
        else if (shift == 0);
        else if (shift < expand_bits_childs()) {
            uint64_t tmp = LOWER;
            UPPER = (UPPER <<= shift) + (LOWER >>= (expand_bits_childs() - shift));
            LOWER = tmp;
            LOWER <<= shift;
        }
        else if ((expand_bits() > shift) && (shift > expand_bits_childs())) {
            UPPER = LOWER <<= (shift - expand_bits_childs());
            LOWER = 0;
        }
        else
            *this = exint_0;

        return *this;
    }
    expand_uint operator<<(uint64_t shift) const {
        return expand_uint(*this) <<= shift;
    }
    expand_uint& operator>>=(uint64_t shift) {
        if (shift < expand_bits_childs()) {
            uint64_t tmp = UPPER;
            LOWER = (UPPER <<= (expand_bits_childs() - shift)) + (LOWER >>= shift);
            UPPER = tmp;
            UPPER >>= shift;
        }
        else if ((expand_bits() > shift) && (shift > expand_bits_childs())) {
            UPPER >>= (shift - expand_bits_childs());
            LOWER = UPPER;
            UPPER = 0;
        }
        else
            *this = exint_0;

        return *this;
    }
    expand_uint operator>>(uint64_t shift) const {
        return expand_uint(*this) >>= shift;
    }

    bool operator!() const {
        return !(bool)(UPPER | LOWER);
    }
    bool operator&&(const expand_uint& rhs) const {
        return ((bool)*this && rhs);
    }
    bool operator||(const expand_uint& rhs) const {
        return ((bool)*this || rhs);
    }
    bool operator==(const expand_uint& rhs) const {
        return ((UPPER == rhs.UPPER) && (LOWER == rhs.LOWER));
    }
    bool operator!=(const expand_uint& rhs) const {
        return ((UPPER != rhs.UPPER) | (LOWER != rhs.LOWER));
    }
    bool operator>(const expand_uint& rhs) const {
        if (UPPER == rhs.UPPER) {
            return (LOWER > rhs.LOWER);
        }
        return (UPPER > rhs.UPPER);
    }
    bool operator<(const expand_uint& rhs) const {
        if (UPPER == rhs.UPPER) {
            return (LOWER < rhs.LOWER);
        }
        return (UPPER < rhs.UPPER);
    }
    bool operator>=(const expand_uint& rhs) const {
        return ((*this > rhs) | (*this == rhs));
    }
    bool operator<=(const expand_uint& rhs) const {
        return ((*this < rhs) | (*this == rhs));
    }

    expand_uint& operator++() {
        return *this += exint_1;
    }
    expand_uint operator++(int) {
        expand_uint temp(*this);
        ++* this;
        return temp;
    }
    expand_uint operator+(const expand_uint& rhs) const {
        return expand_uint(UPPER + rhs.UPPER + ((LOWER + rhs.LOWER) < LOWER), LOWER + rhs.LOWER);
    }
    expand_uint& operator+=(const expand_uint& rhs) {
        UPPER += rhs.UPPER + ((LOWER + rhs.LOWER) < LOWER);
        LOWER += rhs.LOWER;
        return *this;
    }

    expand_uint operator-(const expand_uint& rhs) const {
        return expand_uint(UPPER - rhs.UPPER - ((LOWER - rhs.LOWER) > LOWER), LOWER - rhs.LOWER);
    }
    expand_uint& operator-=(const expand_uint& rhs) {
        UPPER -= rhs.UPPER + ((LOWER - rhs.LOWER) > LOWER);
        LOWER -= rhs.LOWER;
        return *this;
    }

    expand_uint operator*(const expand_uint& rhs) const {
        expand_uint res(0);
        expand_uint multer = rhs;
        char* multer_proxy = (char*)&multer;
        size_t bits = multer.bits();
        for (size_t i = 0; i < bits; i++) {
#ifdef IS_BIG_ENDIAN
                if (multer_proxy[0] & 1)
#else
                if (multer_proxy[expand_bits()/64 - 1] & 0x8000000000000000)
#endif
                res += *this << i;
            multer >>= 1;
        }
        return res;
    }
    expand_uint& operator*=(const expand_uint& rhs) {
        *this = *this * rhs;
        return *this;
    }


    expand_uint operator/(const expand_uint& rhs) const {
        return divmod(*this, rhs).first;
    }
    expand_uint& operator/=(const expand_uint& rhs) {
        *this = *this / rhs;
        return *this;
    }

    expand_uint operator%(const expand_uint& rhs) const {
        return divmod(*this, rhs).second;
    }
    expand_uint& operator%=(const expand_uint& rhs) {
        *this = *this % rhs;
        return *this;
    }
    explicit operator bool() const {
        return (bool)(UPPER | LOWER);
    }
    explicit operator uint8_t() const {
        return (uint8_t)LOWER;
    }
    explicit operator uint16_t() const {
        return (uint16_t)LOWER;
    }
    explicit operator uint32_t() const {
        return (uint32_t)LOWER;
    }
    explicit operator uint64_t() const {
        return (uint64_t)LOWER;
    }

    template <uint64_t byt_expand>
    explicit operator expand_uint<byt_expand>() {
        return to_ansi_string().c_str();
    }
};
const expand_uint<0> expand_uint<0>::exint_0 = 0;
const expand_uint<0> expand_uint<0>::exint_1 = 1;

template<uint64_t byte_expand>
class expand_uint {
#ifdef IS_BIG_ENDIAN
    expand_uint<byte_expand - 1> LOWER;
    expand_uint<byte_expand - 1> UPPER;
#else
    expand_uint<byte_expand - 1> UPPER;
    expand_uint<byte_expand - 1> LOWER;
#endif
    uint64_t bits() const {
        uint64_t out = 0;
        if (UPPER) {
            out = expand_bits_childs();
            expand_uint<byte_expand - 1> up = UPPER;
            while (up) {
                up >>= 1;
                out++;
            }
        }
        else {
            expand_uint<byte_expand - 1> low = LOWER;
            while (low) {
                low >>= 1;
                out++;
            }
        }
        return out;
    }
    std::pair <expand_uint, expand_uint> divmod(const expand_uint& lhs, const expand_uint& rhs) const {
        // Save some calculations /////////////////////
        if (rhs == exint_0) {
            throw std::domain_error("Error: division or modulus by 0");
        }
        else if (rhs == exint_1) {
            return std::pair <expand_uint, expand_uint>(lhs, exint_0);
        }
        else if (lhs == rhs) {
            return std::pair <expand_uint, expand_uint>(exint_1, exint_0);
        }
        else if ((lhs == exint_0) || (lhs < rhs)) {
            return std::pair <expand_uint, expand_uint>(exint_0, lhs);
        }

        std::pair <expand_uint, expand_uint> qr(exint_0, lhs);
        std::pair <expand_uint, expand_uint> qrCheck(exint_0, lhs);
        expand_uint copyd = rhs << (lhs.bits() - rhs.bits());
        expand_uint adder = exint_1 << (lhs.bits() - rhs.bits());
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
    template <typename I> static std::string n2hexstr(const I w, size_t hex_len = sizeof(I) << 1) {
        static const char* digits = "0123456789ABCDEF";
        std::string rc(hex_len, '0');
        for (size_t i = 0, j = (hex_len - 1) * 4; i < hex_len; ++i, j -= 4)
            rc[i] = digits[(w >> j) & 0x0f];
        return rc;
    }



public:
    expand_uint static get_min() {
        return exint_0;
    }
    expand_uint static get_max() {
        return expand_uint(expand_uint<byte_expand - 1>::get_max(), expand_uint<byte_expand - 1>::get_max());
    }

    static const expand_uint<byte_expand> exint_0;
    static const expand_uint<byte_expand> exint_1;
    const expand_uint<byte_expand - 1>& upper() const {
        return UPPER;
    }
    const expand_uint<byte_expand - 1>& lower() const {
        return LOWER;
    }
    constexpr static uint64_t expand_bits() {
        return expand_uint<byte_expand - 1>::expand_bits() * 2;
    }
    constexpr static uint64_t expand_bits_childs() {
        return expand_uint<byte_expand - 1>::expand_bits();
    }
    constexpr static uint64_t expand_bits_sub_childs() {
        return expand_uint<byte_expand - 1>::expand_bits_childs();
    }
    expand_uint() : UPPER(0), LOWER(0) {};
    expand_uint(const expand_uint& rhs) = default;
    expand_uint(expand_uint&& rhs) = default;
    expand_uint(const char* str) : UPPER(0), LOWER(0) {
        expand_uint mult(10);
        size_t str_len = strlen(str);
        expand_uint anti_overflow;
        for (size_t i = 0; i < str_len; i++) {
            anti_overflow = *this * mult + str[i] - '0';
            if (anti_overflow < *this) break;
            *this = anti_overflow;
        }
    }
    template <uint64_t byt_expand>
    expand_uint(const expand_uint<byt_expand>& rhs) {
        if constexpr (byt_expand < byte_expand)
            LOWER = rhs;
        else
            *this = rhs.to_ansi_string().c_str();
    }

    expand_uint(const long long rhs) : UPPER(0), LOWER(rhs) {}
    expand_uint(const unsigned long long rhs) : UPPER(0), LOWER(rhs) {}
    expand_uint(const unsigned int rhs) : UPPER(0), LOWER(rhs) {}
    expand_uint(const double rhs) : UPPER(0), LOWER(rhs) {}
    expand_uint(const int rhs) : UPPER(0), LOWER(rhs) {}

    template <typename S, typename T>
    expand_uint(const S& upper_rhs, const T& lower_rhs) : LOWER(lower_rhs), UPPER(upper_rhs)
    {}

    std::string to_ansi_string() {
        std::string res;
        {
            std::pair<expand_uint, expand_uint> temp(*this, exint_0);
            expand_uint dever(10);
            while (true) {
                temp = divmod(temp.first, dever);
                res += (uint8_t)(temp.second) + '0';
                if (temp.first == exint_0)
                    break;
            }
        }
        std::reverse(res.begin(), res.end());
        return res;
    }
    std::string to_ansi_string() const {
        return expand_uint(*this).to_ansi_string();
    }

    std::string to_hex_str() const {
        const uint64_t* proxy = (const uint64_t*)this;
        std::stringstream stream;
        for (int64_t i = expand_bits() / 64 - 1; i >= 0; --i)
            stream << n2hexstr(proxy[i]);
        return "0x" + stream.str();
    }



    expand_uint& operator=(const expand_uint& rhs) = default;
    expand_uint& operator=(expand_uint&& rhs) = default;










    expand_uint operator&(const expand_uint& rhs) const {
        return expand_uint(UPPER & rhs.UPPER, LOWER & rhs.LOWER);
    }
    expand_uint& operator&=(const expand_uint& rhs) {
        UPPER &= rhs.UPPER;
        LOWER &= rhs.LOWER;
        return *this;
    }

    expand_uint operator|(const expand_uint& rhs) const {
        return expand_uint(UPPER | rhs.UPPER, LOWER | rhs.LOWER);
    }
    expand_uint& operator|=(const expand_uint& rhs) {
        UPPER |= rhs.UPPER;
        LOWER |= rhs.LOWER;
        return *this;
    }

    expand_uint operator^(const expand_uint& rhs) const {
        return expand_uint(UPPER ^ rhs.UPPER, LOWER ^ rhs.LOWER);
    }
    expand_uint& operator^=(const expand_uint& rhs) {
        UPPER ^= rhs.UPPER;
        LOWER ^= rhs.LOWER;
        return *this;
    }

    expand_uint operator~() const {
        return expand_uint(~UPPER, ~LOWER);
    }



    expand_uint& operator<<=(uint64_t shift) {
        if (shift >= expand_bits())
            *this = exint_0;

        else if (shift == expand_bits_childs()) {
            UPPER = LOWER;
            LOWER = expand_uint<byte_expand - 1>::exint_0;
        }
        else if (shift == 0);
        else if (shift < expand_bits_childs()) {
            expand_uint<byte_expand - 1> tmp = LOWER;
            UPPER <<= shift;
            UPPER += (tmp >>= (expand_bits_childs() - shift));
            LOWER <<= shift;
        }
        else if ((expand_bits() > shift) && (shift > expand_bits_childs())) {
            UPPER = LOWER <<= (shift - expand_bits_childs());
            LOWER = expand_uint<byte_expand - 1>::exint_0;
        }
        else
            *this = exint_0;

        return *this;
    }
    expand_uint operator<<(uint64_t shift) const {
        return expand_uint(*this) <<= shift;
    }

    expand_uint& operator>>=(uint64_t shift) {
        if (shift >= expand_bits())
            *this = exint_0;
        else if (shift == expand_bits_childs()) {
            LOWER = UPPER;
            UPPER = expand_uint<byte_expand - 1>::exint_0;
        }
        else if (shift == 0);
        else if (shift < expand_bits_childs()) {
            expand_uint<byte_expand - 1> tmp = UPPER;
            LOWER >>= shift;
            LOWER += (tmp <<= (expand_bits_childs() - shift));
            UPPER >>= shift;
        }
        else if ((expand_bits() > shift) && (shift > expand_bits_childs())) {
            UPPER >>= (shift - expand_bits_childs());
            LOWER = UPPER;
            UPPER = expand_uint<byte_expand - 1>::exint_0;
        }
        else
            *this = exint_0;

        return *this;
    }
    expand_uint operator>>(uint64_t shift) const {
        return expand_uint(*this) >>= shift;
    }


    bool operator!() const {
        return !(bool)(UPPER | LOWER);
    }
    bool operator&&(const expand_uint& rhs) const {
        return ((bool)*this && rhs);
    }
    bool operator||(const expand_uint& rhs) const {
        return ((bool)*this || rhs);
    }
    bool operator==(const expand_uint& rhs) const {
        return ((UPPER == rhs.UPPER) && (LOWER == rhs.LOWER));
    }
    bool operator!=(const expand_uint& rhs) const {
        return ((UPPER != rhs.UPPER) | (LOWER != rhs.LOWER));
    }
    bool operator>(const expand_uint& rhs) const {
        if (UPPER == rhs.UPPER) {
            return (LOWER > rhs.LOWER);
        }
        return (UPPER > rhs.UPPER);
    }
    bool operator<(const expand_uint& rhs) const {
        if (UPPER == rhs.UPPER) {
            return (LOWER < rhs.LOWER);
        }
        return (UPPER < rhs.UPPER);
    }
    bool operator>=(const expand_uint& rhs) const {
        return ((*this > rhs) | (*this == rhs));
    }
    bool operator<=(const expand_uint& rhs) const {
        return ((*this < rhs) | (*this == rhs));
    }


    expand_uint& operator++() {
        return *this += exint_1;
    }
    expand_uint operator++(int) {
        expand_uint temp(*this);
        *this += exint_1;
        return temp;
    }
    expand_uint operator+(const expand_uint& rhs) const {
        return expand_uint(*this) += rhs;
    }
    expand_uint operator-(const expand_uint& rhs) const {
        return expand_uint(*this) -= rhs;
    }
    expand_uint& operator+=(const expand_uint& rhs) {
        UPPER += rhs.UPPER;
        UPPER += ((LOWER + rhs.LOWER) < LOWER) ? expand_uint<byte_expand - 1>::exint_1 : expand_uint<byte_expand - 1>::exint_0;
        LOWER += rhs.LOWER;
        return *this;
    }
    expand_uint& operator-=(const expand_uint& rhs) {
        UPPER -= rhs.UPPER;
        UPPER -= ((LOWER - rhs.LOWER) > LOWER) ? expand_uint<byte_expand - 1>::exint_1 : expand_uint<byte_expand - 1>::exint_0;
        LOWER -= rhs.LOWER;
        return *this;
    }


    expand_uint operator*(const expand_uint& rhs) const {
        expand_uint res(0);
        expand_uint multer = rhs;
        char* multer_proxy = (char*)&multer;
        size_t bits = multer.bits();
        for (size_t i = 0; i < bits; i++) {
#ifdef IS_BIG_ENDIAN
            if (multer_proxy[0] & 1)
#else
            if (multer_proxy[expand_bits() / 64 - 1] & 0x8000000000000000)
#endif
                res += *this << i;
            multer >>= 1;
        }
        return res;
    }
    expand_uint& operator*=(const expand_uint& rhs) {
        *this = *this * rhs;
        return *this;
    }

    expand_uint operator/(const expand_uint& rhs) const {
        return divmod(*this, rhs).first;
    }
    expand_uint& operator/=(const expand_uint& rhs) {
        *this = divmod(*this, rhs).first;
        return *this;
    }

    expand_uint operator%(const expand_uint& rhs) const {
        return divmod(*this, rhs).second;
    }
    expand_uint& operator%=(const expand_uint& rhs) {
        *this = divmod(*this, rhs).second;
        return *this;
    }

    explicit operator bool() const {
        return (bool)(UPPER | LOWER);
    }
    explicit operator uint8_t() const {
        return (uint8_t)LOWER;
    }
    explicit operator uint16_t() const {
        return (uint16_t)LOWER;
    }
    explicit operator uint32_t() const {
        return (uint32_t)LOWER;
    }
    explicit operator uint64_t() const {
        return (uint64_t)LOWER;
    }
    template <uint64_t byt_expand>
    explicit operator expand_uint<byt_expand>() {
        return to_ansi_string().c_str();
    }
};
template<uint64_t byte_expand>
const expand_uint<byte_expand> expand_uint<byte_expand>::exint_0 = 0;
template<uint64_t byte_expand>
const expand_uint<byte_expand> expand_uint<byte_expand>::exint_1 = 1;


typedef expand_uint<0> uint128_ext;
typedef expand_uint<1> uint256_ext;
typedef expand_uint<2> uint512_ext;
typedef expand_uint<3> uint1024_ext;
typedef expand_uint<4> uint2048_ext;
typedef expand_uint<5> uint4096_ext;
typedef expand_uint<6> uint8192_ext;
typedef expand_uint<7> uint16384_ext;
typedef expand_uint<8> uint32768_ext;

typedef expand_uint<0> uint16_exb;
typedef expand_uint<1> uint32_exb;
typedef expand_uint<2> uint64_exb;
typedef expand_uint<3> uint128_exb;
typedef expand_uint<4> uint256_exb;
typedef expand_uint<5> uint512_exb;
typedef expand_uint<6> uint1024_exb;
typedef expand_uint<7> uint2048_exb;
typedef expand_uint<8> uint4096_exb;




template<uint64_t byte_expand>
class expand_int {
    expand_int& switch_my_siqn() {
        val.unsigned_int = ~val.unsigned_int;
        val.unsigned_int += expand_uint<byte_expand>::exint_1;
        return *this;
    }
    expand_int switch_my_siqn() const {
        return expand_int(*this).switch_my_siqn();
    }

    expand_int& switch_to_unsiqn() {
        if (val.is_minus)
            switch_my_siqn();
        return *this;
    }
    expand_int switch_to_unsiqn() const {
        if (val.is_minus)
            return expand_int(*this).switch_my_siqn();
        else return *this;
    }

    expand_int& switch_to_siqn() {
        if (!val.is_minus)
            switch_my_siqn();
        return *this;
    }
    expand_int switch_to_siqn() const {
        if (!val.is_minus)
            return expand_int(*this).switch_my_siqn();
        else return *this;
    }

    union for_constructor {
        struct s {
#ifdef IS_BIG_ENDIAN
            uint64_t unused_[(expand_uint<byte_expand>::expand_bits()) / 64 - 1];
            uint64_t unused : 63;
            uint64_t _is_minus : 1;
#else
            bool _is_minus : 1;
#endif
        public:
            s() {

            }
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

        expand_uint<byte_expand> unsigned_int;
        for_constructor() {
        }
    } val;
public:
    static expand_int get_min() {
        expand_int tmp = 0;
        tmp.val.is_minus = 1;
        return tmp;
    }
    static expand_int get_max() {
        expand_int tmp;
        tmp.val.unsigned_int = expand_uint<byte_expand>::get_max();
        tmp.val.is_minus = 0;
        return tmp;
    }


    expand_int() {
        val.unsigned_int = 0;
    }
    expand_int(const expand_int& rhs) = default;
    expand_int(expand_int&& rhs) = default;
    expand_int(const char* str) {
        bool set_minus = 0;
        if (*str++ == '-')
            set_minus = 1;
        else str--;
        val.unsigned_int = expand_uint<byte_expand>(str);
        if (set_minus) {
            switch_to_siqn();
            val.is_minus = 1;
        }
    }
    template <uint64_t byt_expand>
    expand_int(const expand_int<byt_expand> rhs) {
        if constexpr (byt_expand < byte_expand) {
            bool is_minus = rhs.val.is_minus;
            rhs.val.is_minus = 0;
            val.unsigned_int = rhs.val.unsigned_int;
            val.is_minus = is_minus;
        }
        else
            *this = rhs.to_ansi_string().c_str();
    }
    template <typename T>
    expand_int(const T& rhs)
    {
        val.unsigned_int = (expand_uint<byte_expand>)rhs;
    }
    template <typename T>
    expand_int(const T&& rhs)
    {
        val.unsigned_int = (expand_uint<byte_expand>)rhs;
    }
    expand_int(const long long rhs) { *this = std::to_string(rhs).c_str(); }
    expand_int(const int rhs) { *this = std::to_string(rhs).c_str(); }

    std::string to_ansi_string() {
        if (val.is_minus)
            return
            '-' + expand_int(*this).switch_to_unsiqn().
            val.unsigned_int.to_ansi_string();
        else
            return val.unsigned_int.to_ansi_string();
    }
    std::string to_hex_str() const {
        return val.unsigned_int.to_ansi_string();
    }

    expand_int& operator=(const expand_int& rhs) = default;
    expand_int& operator=(expand_int&& rhs) = default;


    expand_int operator&(const expand_int& rhs) const {
        return expand_int(*this) &= rhs;
    }
    expand_int& operator&=(const expand_int& rhs) {
        val.unsigned_int &= rhs.val.unsigned_int;
        return *this;
    }
    expand_int operator|(const expand_int& rhs) const {
        return expand_int(*this) |= rhs;
    }
    expand_int& operator|=(const expand_int& rhs) {
        val.unsigned_int |= rhs.val.unsigned_int;
        return *this;
    }
    expand_int operator^(const expand_int& rhs) const {
        return expand_int(*this) ^= rhs;
    }
    expand_int& operator^=(const expand_int& rhs) {
        val.unsigned_int ^= rhs.val.unsigned_int;
        return *this;
    }
    expand_int operator~() const {
        expand_int tmp(*this);
        ~tmp.val.unsigned_int;
        return tmp;
    }


    expand_int& operator<<=(uint64_t shift) {
        val.unsigned_int <<= shift;
        return *this;
    }
    expand_int operator<<(uint64_t shift) const {
        return expand_int(*this) <<= shift;
    }
    expand_int& operator>>=(uint64_t shift) {
        val.unsigned_int >>= shift;
        return *this;
    }
    expand_int operator>>(uint64_t shift) const {
        return expand_int(*this) >>= shift;
    }

    bool operator!() const {
        return !val.unsigned_int;
    }
    bool operator&&(const expand_int& rhs) const {
        return ((bool)*this && rhs);
    }
    bool operator||(const expand_int& rhs) const {
        return ((bool)*this || rhs);
    }
    bool operator==(const expand_int& rhs) const {
        return val.unsigned_int == rhs.val.unsigned_int;
    }
    bool operator!=(const expand_int& rhs) const {
        return val.unsigned_int != rhs.val.unsigned_int;
    }
    bool operator>(const expand_int& rhs) const {
        if (val.is_minus && !rhs.val.is_minus)
            return false;
        if (!val.is_minus && rhs.val.is_minus)
            return true;
        return expand_int(*this).switch_to_unsiqn().val.unsigned_int > rhs.switch_to_unsiqn().val.unsigned_int;
    }
    bool operator<(const expand_int& rhs) const {
        if (val.is_minus && !rhs.val.is_minus)
            return true;
        if (!val.is_minus && rhs.val.is_minus)
            return false;
        return expand_int(*this).switch_to_unsiqn().val.unsigned_int < rhs.switch_to_unsiqn().val.unsigned_int;
    }
    bool operator>=(const expand_int& rhs) const {
        return ((*this > rhs) | (*this == rhs));
    }
    bool operator<=(const expand_int& rhs) const {
        return ((*this < rhs) | (*this == rhs));
    }

    expand_int& operator++() {
        if (val.is_minus) {
            switch_my_siqn().val.unsigned_int += expand_uint<byte_expand>::exint_1;
            switch_my_siqn();
        }
        else
            val.unsigned_int++;
        return *this;
    }
    expand_int operator++(int) {
        expand_int temp(*this);
        ++* this;
        return temp;
    }
    expand_int& operator--() {
        if (val.is_minus) {
            switch_my_siqn().val.unsigned_int -= expand_uint<byte_expand>::exint_1;
            switch_my_siqn();
        }
        else
            val.unsigned_int--;
        return *this;
    }
    expand_int operator--(int) {
        expand_int temp(*this);
        --* this;
        return temp;
    }

    expand_int operator+(const expand_int& rhs) const {
        return expand_int(*this) += rhs;
    }
    expand_int operator-(const expand_int& rhs) const {
        return expand_int(*this) -= rhs;
    }

    expand_int& operator+=(const expand_int& rhs) {
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
            val.unsigned_int -= rhs.switch_my_siqn().val.unsigned_int;
        }
        else
            val.unsigned_int += rhs.val.unsigned_int;
        return *this;
    }
    expand_int& operator-=(const expand_int& rhs) {
        if (val.is_minus && rhs.val.is_minus) {
            switch_my_siqn().val.unsigned_int -= rhs.switch_my_siqn().val.unsigned_int;
            switch_my_siqn();
        }
        else if (val.is_minus) {
            switch_my_siqn().val.unsigned_int -= rhs.val.unsigned_int;
            if (val.unsigned_int)
                switch_to_unsiqn();
        }
        else if (rhs.val.is_minus) {
            if (switch_my_siqn().val.unsigned_int < rhs.val.unsigned_int) {
                val.unsigned_int = rhs.val.unsigned_int - val.unsigned_int;
                switch_to_siqn();
            }
            else
                val.unsigned_int -= rhs.val.unsigned_int;
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

    expand_int operator*(const expand_int& rhs) const {
        return expand_int(*this) *= rhs;
    }
    expand_int& operator*=(const expand_int& rhs) {
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


    expand_int operator/(const expand_int& rhs) const {
        return expand_int(*this) /= rhs;
    }
    expand_int& operator/=(const expand_int& rhs) {
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

    expand_int operator%(const expand_int& rhs) const {
        return expand_int(*this) %= rhs;
    }
    expand_int& operator%=(const expand_int& rhs) {
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


    expand_int operator+() const {
        return switch_to_unsiqn();
    }
    expand_int operator-() const {
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
    explicit operator expand_uint<byt_expand>() const {
        return expand_uint<byt_expand>(val.unsigned_int);
    }
    template <uint64_t byt_expand>
    explicit operator expand_int<byt_expand>() {
        return to_ansi_string().c_str();
    }
};


typedef expand_int<0> int128_ext;
typedef expand_int<1> int256_ext;
typedef expand_int<2> int512_ext;
typedef expand_int<3> int1024_ext;
typedef expand_int<4> int2048_ext;
typedef expand_int<5> int4096_ext;
typedef expand_int<6> int8192_ext;
typedef expand_int<7> int16384_ext;
typedef expand_int<8> int32768_ext;

typedef expand_int<0> int16_exb;
typedef expand_int<1> int32_exb;
typedef expand_int<2> int64_exb;
typedef expand_int<3> int128_exb;
typedef expand_int<4> int256_exb;
typedef expand_int<5> int512_exb;
typedef expand_int<6> int1024_exb;
typedef expand_int<7> int2048_exb;
typedef expand_int<8> int4096_exb;








template<uint64_t byte_expand>
class expand_real {
    union for_constructor {

        struct s {
#ifdef IS_BIG_ENDIAN
            uint64_t unused_[(expand_uint<byte_expand>::expand_bits()) / 64 - 1];
            uint64_t unused : 63 - (byte_expand >= 57 ? 63 : (5 + byte_expand));
            uint64_t _dot_pos : (byte_expand >= 57 ? 63 : (5 + byte_expand));
            uint64_t _is_minus : 1;
#else
            uint64_t _is_minus : 1;
            uint64_t _dot_pos : (byte_expand >= 63 ? 64 : (5 + byte_expand));
#endif
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
        expand_int<byte_expand> signed_int;
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
    void normalize_dot() {
        if (val.dot_pos == 0)
            return;
        std::string tmp_this_value = to_ansi_string();
        expand_real remove_nuls_mult(10);
        expand_real remove_nuls(1);
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

    void div(const expand_real& rhs, bool do_normalize_dot = true) {
        uint64_t this_dot_pos = temp_denormalize_struct();
        expand_real tmp = rhs;
        uint64_t rhs_dot_pos = tmp.temp_denormalize_struct();

        this_dot_pos -= rhs_dot_pos;


        expand_int<byte_expand> move_tmp = expand_int < byte_expand>(10);
        expand_real shift_tmp;
        shift_tmp.val.dot_pos = -1;
        while (true) {
            if (val.signed_int % tmp.val.signed_int) {
                this_dot_pos++;
                if (this_dot_pos != shift_tmp.val.dot_pos) {
                    val.signed_int = val.signed_int * move_tmp;
                    continue;
                }
                this_dot_pos--;
            }
            val.signed_int /= tmp.val.signed_int;
            break;
        }
        normalize_struct(this_dot_pos);
        if (do_normalize_dot)
            normalize_dot();
    }
    void mod(const expand_real& rhs, bool do_normalize_dot = true) {
        uint64_t this_dot_pos = temp_denormalize_struct();
        expand_real tmp = rhs;
        uint64_t rhs_dot_pos = tmp.temp_denormalize_struct();

        this_dot_pos -= rhs_dot_pos;


        expand_int<byte_expand> move_tmp = 10;
        expand_real shift_tmp;
        shift_tmp.val.dot_pos = -1;
        while (true) {
            if (val.signed_int % tmp.val.signed_int) {
                this_dot_pos++;
                if (this_dot_pos != shift_tmp.val.dot_pos) {
                    val.signed_int = val.signed_int * move_tmp;
                    continue;
                }
                this_dot_pos--;
            }
            val.signed_int %= tmp.val.signed_int;
            break;
        }
        normalize_struct(this_dot_pos);
        if (do_normalize_dot)
            normalize_dot();
    }
public:
    static expand_real get_min() {
        expand_real tmp = 0;
        tmp.val.signed_int = expand_int<byte_expand>::get_min();
        tmp.val.dot_pos = 0;
        return tmp;
    }
    static expand_real get_max() {
        expand_real tmp = 0;
        tmp.val.signed_int = expand_int<byte_expand>::get_max();
        tmp.val.dot_pos = 0;
        return tmp;
    }

    expand_real() {
        val.signed_int = 0;
    }
    expand_real(const expand_real& rhs) = default;
    expand_real(expand_real&& rhs) = default;
    expand_real(const char* str) {
        *this = std::string(str);
    }
    expand_real(const std::string str) {
        size_t found_pos = str.find('.');
        if (found_pos == std::string::npos)
        {
            val.signed_int = expand_int<byte_expand>(str.c_str());
            val.dot_pos = 0;
        }
        else {
            //check str for second dot
            if (str.find('.', found_pos) == std::string::npos)
                throw std::invalid_argument("Real value can contain only one dot");

            std::string tmp = str;
            tmp.erase(tmp.begin() + found_pos);
            val.signed_int = expand_int<byte_expand>(tmp.c_str());
            val.dot_pos = (str.length() - found_pos - 1);
        }
        normalize_struct(temp_denormalize_struct());
    }
    template <uint64_t byt_expand>
    expand_real(const expand_int<byt_expand>& rhs) {
        *this = rhs.to_ansi_string().c_str();
    }
    template <typename T>
    expand_real(const T& rhs)
    {
        val.signed_int = (expand_int<byte_expand>)rhs;
    }
    template <typename T>
    expand_real(const T&& rhs)
    {
        val.signed_int = (expand_int<byte_expand>)rhs;
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
        return expand_real(*this).to_ansi_string();
    }
    std::string to_hex_str() const {
        return val.signed_int.to_hex_str();
    }

    expand_real& operator=(const expand_real& rhs) = default;
    expand_real& operator=(expand_real&& rhs) = default;

    expand_real operator&(const expand_real& rhs) const {
        return expand_real(*this) &= rhs;
    }

    expand_real& operator&=(const expand_real& rhs) {
        val.signed_int &= rhs.val.signed_int;
        return *this;
    }

    expand_real operator|(const expand_real& rhs) const {
        return expand_real(*this) |= rhs;
    }

    expand_real& operator|=(const expand_real& rhs) {
        val.signed_int |= rhs.val.signed_int;
        return *this;
    }

    expand_real operator^(const expand_real& rhs) const {
        return expand_real(*this) ^= rhs;
    }

    expand_real& operator^=(const expand_real& rhs) {
        val.signed_int ^= rhs.val.signed_int;
        return *this;
    }

    expand_real operator~() const {
        expand_real tmp(*this);
        ~tmp.val.signed_int;
        return tmp;
    }



    expand_real& operator<<=(uint64_t shift) {
        val.signed_int <<= shift;
        return *this;
    }
    expand_real operator<<(uint64_t shift) const {
        return expand_real(*this) <<= shift;
    }


    expand_real& operator>>=(uint64_t shift) {
        val.signed_int >>= shift;
        return *this;
    }
    expand_real operator>>(uint64_t shift) const {
        return expand_real(*this) >>= shift;
    }
    bool operator!() const {
        return !val.signed_int;
    }

    bool operator&&(const expand_real& rhs) const {
        return ((bool)*this && rhs);
    }

    bool operator||(const expand_real& rhs) const {
        return ((bool)*this || rhs);
    }

    bool operator==(const expand_real& rhs) const {
        return val.signed_int == rhs.val.signed_int;
    }

    bool operator!=(const expand_real& rhs) const {
        return val.signed_int != rhs.val.signed_int;
    }

    bool operator>(const expand_real& rhs) const {
        std::vector<std::string> this_parts = split_dot(to_ansi_string());
        std::vector<std::string> rhs_parts = split_dot(rhs.to_ansi_string());
        {
            expand_int<byte_expand> temp1(this_parts[0].c_str());
            expand_int<byte_expand> temp2(rhs_parts[0].c_str());
            if (temp1 == temp2);
            else return temp1 > temp2;
        }
        if (this_parts.size() == 2 && rhs_parts.size() == 2) {
            expand_int<byte_expand> temp1(this_parts[1].c_str());
            expand_int<byte_expand> temp2(rhs_parts[1].c_str());
            return temp1 > temp2;
        }
        else
            return this_parts.size() > rhs_parts.size();
    }

    bool operator<(const expand_real& rhs) const {
        return !(*this > rhs) && *this != rhs;
    }

    bool operator>=(const expand_real& rhs) const {
        return ((*this > rhs) | (*this == rhs));
    }

    bool operator<=(const expand_real& rhs) const {
        return !(*this > rhs);
    }


    expand_real& operator++() {
        return *this += 1;
    }
    expand_real operator++(int) {
        expand_real temp(*this);
        *this += 1;
        return temp;
    }
    expand_real operator+(const expand_real& rhs) const {
        return expand_real(*this) += rhs;
    }
    expand_real operator-(const expand_real& rhs) const {
        return expand_real(*this) -= rhs;
    }

    expand_real& operator+=(const expand_real& rhs) {
        uint64_t this_dot_pos = temp_denormalize_struct();
        expand_real tmp = rhs;
        uint64_t rhs_dot_pos = tmp.temp_denormalize_struct();
        if (this_dot_pos == rhs_dot_pos);
        else if (this_dot_pos > rhs_dot_pos) {
            expand_int<byte_expand> move_tmp = 1;
            while (this_dot_pos != rhs_dot_pos++)
                move_tmp *= 10;
            tmp.val.signed_int *= move_tmp;
        }
        else {
            expand_int<byte_expand> move_tmp = 1;
            while (rhs_dot_pos != this_dot_pos++)
                move_tmp *= 10;
            val.signed_int *= move_tmp;
        }
        val.signed_int += tmp.val.signed_int;
        normalize_struct(this_dot_pos);
        normalize_dot();
        return *this;
    }
    expand_real& operator-=(const expand_real& rhs) {
        uint64_t this_dot_pos = temp_denormalize_struct();
        expand_real tmp = rhs;
        uint64_t rhs_dot_pos = tmp.temp_denormalize_struct();
        if (this_dot_pos == rhs_dot_pos);
        else if (this_dot_pos > rhs_dot_pos) {
            expand_int<byte_expand> move_tmp = 1;
            while (this_dot_pos != rhs_dot_pos++)
                move_tmp *= 10;
            tmp.val.signed_int *= move_tmp;
        }
        else {
            expand_int<byte_expand> move_tmp = 1;
            while (rhs_dot_pos != this_dot_pos++)
                move_tmp *= 10;
            val.signed_int *= move_tmp;
        }
        val.signed_int -= tmp.val.signed_int;
        normalize_struct(this_dot_pos);
        normalize_dot();
        return *this;
    }

    expand_real operator*(const expand_real& rhs) const {
        return expand_real(*this) *= rhs;
    }


    expand_real& operator*=(const expand_real& rhs) {
        uint64_t this_dot_pos = temp_denormalize_struct();
        expand_real tmp = rhs;
        uint64_t rhs_dot_pos = tmp.temp_denormalize_struct();
        this_dot_pos += rhs_dot_pos;

        val.signed_int *= tmp.val.signed_int;

        normalize_struct(this_dot_pos);
        normalize_dot();
        return *this;
    }

    expand_real operator/(const expand_real& rhs) const {
        return expand_real(*this) /= rhs;
    }

    expand_real& operator/=(const expand_real& rhs) {
        div(rhs);
        return *this;
    }

    expand_real operator%(const expand_real& rhs) const {
        return expand_real(*this) %= rhs;
    }

    expand_real& operator%=(const expand_real& rhs) {
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
    explicit operator expand_int<byt_expand>() const {
        return val.signed_int;
    }
    template <uint64_t byt_expand>
    explicit operator expand_uint<byt_expand>() const {
        return expand_uint<byt_expand>(val.signed_int);
    }
    template <uint64_t byt_expand>
    explicit operator expand_real<byt_expand>() {
        return to_ansi_string().c_str();
    }
};



typedef expand_real<0> real128_ext;
typedef expand_real<1> real256_ext;
typedef expand_real<2> real512_ext;
typedef expand_real<3> real1024_ext;
typedef expand_real<4> real2048_ext;
typedef expand_real<5> real4096_ext;
typedef expand_real<6> real8192_ext;
typedef expand_real<7> real16384_ext;
typedef expand_real<8> real32768_ext;

typedef expand_real<0> real16_exb;
typedef expand_real<1> real32_exb;
typedef expand_real<2> real64_exb;
typedef expand_real<3> real128_exb;
typedef expand_real<4> real256_exb;
typedef expand_real<5> real512_exb;
typedef expand_real<6> real1024_exb;
typedef expand_real<7> real2048_exb;
typedef expand_real<8> real4096_exb;


template<uint64_t byte_expand>
class expand_unreal {
    union for_constructor {

        struct s {
#ifdef IS_BIG_ENDIAN
            uint64_t unused_[(expand_uint<byte_expand>::expand_bits()) / 64 - 1];
            uint64_t unused : 64 - (byte_expand >= 58 ? 64 : (5 + byte_expand));
            uint64_t _dot_pos : (byte_expand >= 58 ? 64 : (5 + byte_expand));
#else
            uint64_t _dot_pos : (byte_expand >= 58 ? 64 : (5 + byte_expand));
#endif
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
        expand_uint<byte_expand> unsigned_int;
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
    void normalize_dot() {
        if (val.dot_pos == 0)
            return;
        std::string tmp_this_value = to_ansi_string();
        expand_unreal remove_nuls_mult(10);
        expand_unreal remove_nuls(1);
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

    void div(const expand_unreal& rhs, bool do_normalize_dot = true) {
        uint64_t this_dot_pos = temp_denormalize_struct();
        expand_unreal tmp = rhs;
        uint64_t rhs_dot_pos = tmp.temp_denormalize_struct();

        this_dot_pos -= rhs_dot_pos;


        expand_uint<byte_expand> move_tmp = 10;
        expand_unreal shift_tmp;
        shift_tmp.val.dot_pos = -1;
        while (true) {
            if (val.unsigned_int % tmp.val.unsigned_int) {
                this_dot_pos++;
                if (this_dot_pos != shift_tmp.val.dot_pos) {
                    val.unsigned_int = val.unsigned_int * move_tmp;
                    continue;
                }
                this_dot_pos--;
            }
            val.unsigned_int /= tmp.val.unsigned_int;
            break;
        }
        normalize_struct(this_dot_pos);
        if (do_normalize_dot)
            normalize_dot();
    }
    void mod(const expand_unreal& rhs, bool do_normalize_dot = true) {
        uint64_t this_dot_pos = temp_denormalize_struct();
        expand_unreal tmp = rhs;
        uint64_t rhs_dot_pos = tmp.temp_denormalize_struct();

        this_dot_pos -= rhs_dot_pos;


        expand_uint<byte_expand> move_tmp = 10;
        expand_unreal shift_tmp;
        shift_tmp.val.dot_pos = -1;
        while (true) {
            if (val.unsigned_int % tmp.val.unsigned_int) {
                this_dot_pos++;
                if (this_dot_pos != shift_tmp.val.dot_pos) {
                    val.unsigned_int = val.unsigned_int * move_tmp;
                    continue;
                }
                this_dot_pos--;
            }
            val.unsigned_int %= tmp.val.unsigned_int;
            break;
        }
        normalize_struct(this_dot_pos);
        if (do_normalize_dot)
            normalize_dot();
    }
public:
    static expand_unreal get_min() {
        expand_unreal tmp = 0;
        tmp.val.unsigned_int = expand_uint<byte_expand>::get_min();
        tmp.val.dot_pos = 0;
        return tmp;
    }
    static expand_unreal get_max() {
        expand_unreal tmp = 0;
        tmp.val.unsigned_int = expand_uint<byte_expand>::get_max();
        tmp.val.dot_pos = 0;
        return tmp;
    }

    expand_unreal() {
        val.unsigned_int = 0;
    }
    expand_unreal(const expand_unreal& rhs) = default;
    expand_unreal(expand_unreal&& rhs) = default;
    expand_unreal(const char* str) {
        *this = std::string(str);
    }
    expand_unreal(const std::string str) {
        size_t found_pos = str.find('.');
        if (found_pos == std::string::npos)
        {
            val.unsigned_int = expand_uint<byte_expand>(str.c_str());
            val.dot_pos = 0;
        }
        else {
            //check str for second dot
            if (str.find('.', found_pos) == std::string::npos)
                throw std::invalid_argument("Real value can contain only one dot");

            std::string tmp = str;
            tmp.erase(tmp.begin() + found_pos);
            val.unsigned_int = expand_uint<byte_expand>(tmp.c_str());
            val.dot_pos = (str.length() - found_pos - 1);
        }
        normalize_struct(temp_denormalize_struct());
    }
    template <uint64_t byt_expand>
    expand_unreal(const expand_unreal<byt_expand> rhs) {
        *this = rhs.to_ansi_string().c_str();
    }


    template <typename T>
    expand_unreal(const T& rhs)
    {
        val.unsigned_int = (expand_uint<byte_expand>)rhs;
    }
    template <typename T>
    expand_unreal(const T&& rhs)
    {
        val.unsigned_int = (expand_uint<byte_expand>)rhs;
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
        return expand_unreal(*this).to_ansi_string();
    }
    std::string to_hex_str() const {
        return val.unsigned_int.to_hex_str();
    }

    expand_unreal& operator=(const expand_unreal& rhs) = default;
    expand_unreal& operator=(expand_unreal&& rhs) = default;

    expand_unreal operator&(const expand_unreal& rhs) const {
        return expand_unreal(*this) &= rhs;
    }

    expand_unreal& operator&=(const expand_unreal& rhs) {
        val.unsigned_int &= rhs.val.unsigned_int;
        return *this;
    }

    expand_unreal operator|(const expand_unreal& rhs) const {
        return expand_unreal(*this) |= rhs;
    }

    expand_unreal& operator|=(const expand_unreal& rhs) {
        val.signed_int |= rhs.val.signed_int;
        return *this;
    }

    expand_unreal operator^(const expand_unreal& rhs) const {
        return expand_real(*this) ^= rhs;
    }

    expand_unreal& operator^=(const expand_unreal& rhs) {
        val.unsigned_int ^= rhs.val.unsigned_int;
        return *this;
    }

    expand_unreal operator~() const {
        expand_unreal tmp(*this);
        ~tmp.val.signed_int;
        return tmp;
    }



    expand_unreal& operator<<=(uint64_t shift) {
        val.unsigned_int <<= shift;
        return *this;
    }
    expand_unreal operator<<(uint64_t shift) const {
        return expand_unreal(*this) <<= shift;
    }


    expand_unreal& operator>>=(uint64_t shift) {
        val.unsigned_int >>= shift;
        return *this;
    }
    expand_unreal operator>>(uint64_t shift) const {
        return expand_unreal(*this) >>= shift;
    }
    bool operator!() const {
        return !val.unsigned_int;
    }

    bool operator&&(const expand_unreal& rhs) const {
        return ((bool)*this && rhs);
    }

    bool operator||(const expand_unreal& rhs) const {
        return ((bool)*this || rhs);
    }

    bool operator==(const expand_unreal& rhs) const {
        return val.unsigned_int == rhs.val.unsigned_int;
    }

    bool operator!=(const expand_unreal& rhs) const {
        return val.unsigned_int != rhs.val.unsigned_int;
    }

    bool operator>(const expand_unreal& rhs) const {
        std::vector<std::string> this_parts = split_dot(to_ansi_string());
        std::vector<std::string> rhs_parts = split_dot(rhs.to_ansi_string());
        {
            expand_uint<byte_expand> temp1(this_parts[0].c_str());
            expand_uint<byte_expand> temp2(rhs_parts[0].c_str());
            if (temp1 == temp2);
            else return temp1 > temp2;
        }
        if (this_parts.size() == 2 && rhs_parts.size() == 2) {
            expand_uint<byte_expand> temp1(this_parts[1].c_str());
            expand_uint<byte_expand> temp2(rhs_parts[1].c_str());
            return temp1 > temp2;
        }
        else
            return this_parts.size() > rhs_parts.size();
    }

    bool operator<(const expand_unreal& rhs) const {
        return !(*this > rhs) && *this != rhs;
    }

    bool operator>=(const expand_unreal& rhs) const {
        return ((*this > rhs) | (*this == rhs));
    }

    bool operator<=(const expand_unreal& rhs) const {
        return !(*this > rhs);
    }


    expand_unreal& operator++() {
        return *this += 1;
    }
    expand_unreal operator++(int) {
        expand_unreal temp(*this);
        *this += 1;
        return temp;
    }
    expand_unreal operator+(const expand_unreal& rhs) const {
        return expand_real(*this) += rhs;
    }
    expand_unreal operator-(const expand_unreal& rhs) const {
        return expand_real(*this) -= rhs;
    }

    expand_unreal& operator+=(const expand_unreal& rhs) {
        uint64_t this_dot_pos = temp_denormalize_struct();
        expand_unreal tmp = rhs;
        uint64_t rhs_dot_pos = tmp.temp_denormalize_struct();
        if (this_dot_pos == rhs_dot_pos);
        else if (this_dot_pos > rhs_dot_pos) {
            expand_uint<byte_expand> move_tmp = 1;
            while (this_dot_pos != rhs_dot_pos++)
                move_tmp *= 10;
            tmp.val.unsigned_int *= move_tmp;
        }
        else {
            expand_uint<byte_expand> move_tmp = 1;
            while (rhs_dot_pos != this_dot_pos++)
                move_tmp *= 10;
            val.unsigned_int *= move_tmp;
        }
        val.unsigned_int += tmp.val.unsigned_int;
        normalize_struct(this_dot_pos);
        normalize_dot();
        return *this;
    }
    expand_unreal& operator-=(const expand_unreal& rhs) {
        uint64_t this_dot_pos = temp_denormalize_struct();
        expand_unreal tmp = rhs;
        uint64_t rhs_dot_pos = tmp.temp_denormalize_struct();
        if (this_dot_pos == rhs_dot_pos);
        else if (this_dot_pos > rhs_dot_pos) {
            expand_uint<byte_expand> move_tmp = 1;
            while (this_dot_pos != rhs_dot_pos++)
                move_tmp *= 10;
            tmp.val.unsigned_int *= move_tmp;
        }
        else {
            expand_uint<byte_expand> move_tmp = 1;
            while (rhs_dot_pos != this_dot_pos++)
                move_tmp *= 10;
            val.unsigned_int *= move_tmp;
        }
        val.unsigned_int -= tmp.val.unsigned_int;
        normalize_struct(this_dot_pos);
        normalize_dot();
        return *this;
    }

    expand_unreal operator*(const expand_unreal& rhs) const {
        return expand_unreal(*this) *= rhs;
    }


    expand_unreal& operator*=(const expand_unreal& rhs) {
        uint64_t this_dot_pos = temp_denormalize_struct();
        expand_unreal tmp = rhs;
        uint64_t rhs_dot_pos = tmp.temp_denormalize_struct();
        this_dot_pos += rhs_dot_pos;

        val.unsigned_int *= tmp.val.unsigned_int;

        normalize_struct(this_dot_pos);
        normalize_dot();
        return *this;
    }

    expand_unreal operator/(const expand_unreal& rhs) const {
        return expand_unreal(*this) /= rhs;
    }

    expand_unreal& operator/=(const expand_unreal& rhs) {
        div(rhs);
        return *this;
    }

    expand_unreal operator%(const expand_unreal& rhs) const {
        return expand_unreal(*this) %= rhs;
    }

    expand_unreal& operator%=(const expand_unreal& rhs) {
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
    explicit operator expand_int<byt_expand>() const {
        return (expand_int<byt_expand>)expand_uint<byt_expand>(val.unsigned_int);
    }
    template <uint64_t byt_expand>
    explicit operator expand_uint<byt_expand>() const {
        return val.unsigned_int;
    }
    template <uint64_t byt_expand>
    explicit operator expand_real<byt_expand>() const {
        return to_ansi_string().c_str();
    }
    template <uint64_t byt_expand>
    explicit operator expand_unreal<byt_expand>() {
        return to_ansi_string().c_str();
    }
};



typedef expand_unreal<0> unreal128_ext;
typedef expand_unreal<1> unreal256_ext;
typedef expand_unreal<2> unreal512_ext;
typedef expand_unreal<3> unreal1024_ext;
typedef expand_unreal<4> unreal2048_ext;
typedef expand_unreal<5> unreal4096_ext;
typedef expand_unreal<6> unreal8192_ext;
typedef expand_unreal<7> unreal16384_ext;
typedef expand_unreal<8> unreal32768_ext;

typedef expand_unreal<0> unreal16_exb;
typedef expand_unreal<1> unreal32_exb;
typedef expand_unreal<2> unreal64_exb;
typedef expand_unreal<3> unreal128_exb;
typedef expand_unreal<4> unreal256_exb;
typedef expand_unreal<5> unreal512_exb;
typedef expand_unreal<6> unreal1024_exb;
typedef expand_unreal<7> unreal2048_exb;
typedef expand_unreal<8> unreal4096_exb;