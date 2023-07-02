//modified stackoverflow answers
#pragma once
#include <vector>
#include <string>
#include <regex>
namespace string_help {
    inline std::vector<std::string> split(const std::string& s, const std::string& rgx_str = "\\s+") {
        std::vector<std::string> elems;
        std::regex rgx(rgx_str);

        std::sregex_token_iterator iter(s.begin(), s.end(), rgx, -1);
        std::sregex_token_iterator end;

        while (iter != end) {
            elems.push_back(*iter);
            ++iter;
        }
        return elems;
    }
    template <typename T> std::string hexstr(T w, size_t hex_len = sizeof(T) << 1) {
        static const char* digits = "0123456789ABCDEF";
        std::string rc(hex_len, '0');
        if constexpr(std::is_same_v<T,void*> || std::is_same_v<T, const void*>)
            for (size_t i = 0, j = (hex_len - 1) * 4; i < hex_len; ++i, j -= 4)
                rc[i] = digits[(reinterpret_cast<size_t>(w) >> j) & 0x0f];
        else
            for (size_t i = 0, j = (hex_len - 1) * 4; i < hex_len; ++i, j -= 4)
                rc[i] = digits[(w >> j) & 0x0f];
        return rc;
    }
    template <typename T> std::string hexsstr(T w, size_t hex_len = sizeof(T) << 1) {
        std::string rc(hexstr(w, hex_len));
        while(rc[0]=='0')
            rc.erase(rc.begin());
        if(rc.empty())
            return "0";
        return rc;
    }
    //case insensitive string comparison
    inline bool iequals(const std::string& a, const std::string& b) {
        return std::equal(a.begin(), a.end(), b.begin(), b.end(),
            [](char a, char b) {
                return tolower(a) == tolower(b);
            });
    }
    

}