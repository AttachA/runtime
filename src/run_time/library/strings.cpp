// Copyright Danyil Melnytskyi 2023-Present
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#include <run_time/AttachA_CXX.hpp>
#include <run_time/library/times.hpp>

namespace art {
    namespace strings {
        art::ustring _format(ValueItem* args, uint32_t len);

        namespace standard_operator {
            //d && i - integer
            AttachAFunc(d, 1) {
                if (is_integer(args[0].meta.vtype))
                    return (int64_t)args[0];
                else
                    throw InvalidOperation("operator d can be applied only to integer types");
            }

            //u - unsigned integer
            AttachAFunc(u, 1) {
                if (integer_unsigned(args[0].meta.vtype))
                    return (uint64_t)args[0];
                else
                    throw InvalidOperation("operator i can be applied only to integer types");
            }

            //o - octal
            AttachAFunc(o, 1) {
                if (integer_unsigned(args[0].meta.vtype)) {
                    art::ustring tmp;
                    tmp.reserve(22);
                    tmp += "0o";
                    uint64_t val = (uint64_t)args[0];
                    do {
                        tmp += (char)('0' + (val & 7));
                        val >>= 3;
                    } while (val);
                    return tmp;
                } else
                    throw InvalidOperation("operator o can be applied only to integer types");
            }

            //x - hex
            AttachAFunc(x, 1) {
                if (integer_unsigned(args[0].meta.vtype)) {
                    art::ustring tmp;
                    tmp.set_unsafe_state(true, false);
                    tmp.reserve(22);
                    tmp += "0x";
                    uint64_t val = (uint64_t)args[0];
                    do {
                        uint8_t c = val & 15;
                        tmp += (char)(c < 10 ? '0' + c : 'a' + c - 10);
                        val >>= 4;
                    } while (val);
                    tmp.set_unsafe_state(false, false);
                    return tmp;
                } else
                    throw InvalidOperation("operator x can be applied only to integer types");
            }

            //X - HEX
            AttachAFunc(X, 1) {
                if (integer_unsigned(args[0].meta.vtype)) {
                    art::ustring tmp;
                    tmp.reserve(22);
                    tmp += "0x";
                    uint64_t val = (uint64_t)args[0];
                    do {
                        uint8_t c = val & 15;
                        tmp += (char)(c < 10 ? '0' + c : 'A' + c - 10);
                        val >>= 4;
                    } while (val);
                    tmp.set_unsafe_state(false, false);
                    return tmp;
                } else
                    throw InvalidOperation("operator X can be applied only to integer types");
            }

            //f - float
            AttachAFunc(f, 1) {
                if (args[0].meta.vtype == VType::flo || args[0].meta.vtype == VType::doub)
                    return (double)args[0];
                else
                    throw InvalidOperation("operator f can be applied only to float types");
            }

            //F - float
            AttachAFunc(F, 1) {
                if (args[0].meta.vtype == VType::flo || args[0].meta.vtype == VType::doub) {
                    art::ustring tmp = std::to_string((double)args[0]);
                    for (auto& it : tmp)
                        if (it >= 'a' && it <= 'z')
                            it -= 32;
                    return tmp;
                } else
                    throw InvalidOperation("operator F can be applied only to float types");
            }

            //e - float
            AttachAFunc(e, 1) {
                if (args[0].meta.vtype == VType::flo || args[0].meta.vtype == VType::doub) {
                    auto tmp = (double)args[0];
                    art::ustring res;
                    res.resize(22);
                    auto len = sprintf(res.data(), "%e", tmp);
                    res.resize(len);
                    return res;
                } else
                    throw InvalidOperation("operator e can be applied only to float types");
            }

            //E - float
            AttachAFunc(E, 1) {
                if (args[0].meta.vtype == VType::flo || args[0].meta.vtype == VType::doub) {
                    auto tmp = (double)args[0];
                    art::ustring res;
                    res.resize(22);
                    auto len = sprintf(res.data(), "%E", tmp);
                    res.resize(len);
                    return res;
                } else
                    throw InvalidOperation("operator E can be applied only to float types");
            }

            //g - float
            AttachAFunc(g, 1) {
                if (args[0].meta.vtype == VType::flo || args[0].meta.vtype == VType::doub) {
                    auto tmp = (double)args[0];
                    art::ustring res;
                    res.resize(22);
                    auto len = sprintf(res.data(), "%g", tmp);
                    res.resize(len);
                    return res;
                } else
                    throw InvalidOperation("operator g can be applied only to float types");
            }

            //G - float
            AttachAFunc(G, 1) {
                if (args[0].meta.vtype == VType::flo || args[0].meta.vtype == VType::doub) {
                    auto tmp = (double)args[0];
                    art::ustring res;
                    res.resize(22);
                    auto len = sprintf(res.data(), "%G", tmp);
                    res.resize(len);
                    return res;
                } else
                    throw InvalidOperation("operator G can be applied only to float types");
            }

            //a - float
            AttachAFunc(a, 1) {
                if (args[0].meta.vtype == VType::flo || args[0].meta.vtype == VType::doub) {
                    auto tmp = (double)args[0];
                    art::ustring res;
                    res.resize(22);
                    auto len = sprintf(res.data(), "%a", tmp);
                    res.resize(len);
                    return res;
                } else
                    throw InvalidOperation("operator a can be applied only to float types");
            }

            //A - float
            AttachAFunc(A, 1) {
                if (args[0].meta.vtype == VType::flo || args[0].meta.vtype == VType::doub) {
                    auto tmp = (double)args[0];
                    art::ustring res;
                    res.resize(22);
                    auto len = sprintf(res.data(), "%A", tmp);
                    res.resize(len);
                    return res;
                } else
                    throw InvalidOperation("operator A can be applied only to float types");
            }

            //c - char
            AttachAFunc(c, 1) {
                if (args[0].meta.vtype == VType::string) {
                    auto tmp = (art::ustring)args[0];
                    if (tmp.size() == 1)
                        return tmp[0];
                    else
                        throw InvalidOperation("operator c can be applied only to char types");
                } else
                    throw InvalidOperation("operator c can be applied only to char types");
            }

            //s - string
            AttachAFunc(s, 1) {
                if (args[0].meta.vtype == VType::string)
                    return (art::ustring)args[0];
                else
                    throw InvalidOperation("operator s can be applied only to string types");
            }

            //p - pointer
            AttachAFunc(p, 1) {
                if (args[0].meta.vtype == VType::undefined_ptr)
                    return (void*)args[0];
                else
                    throw InvalidOperation("operator p can be applied only to pointer types");
            }

            //n - pointer
            AttachAFunc(n, 1) {
                if (args[0].meta.vtype == VType::undefined_ptr)
                    return (void*)args[0];
                else
                    throw InvalidOperation("operator n can be applied only to pointer types");
            }

            //%
            AttachAFunc(percent, 0) {
                return "%";
            }

            //time
            AttachAFunc(time, 1) {
                if (args[0].meta.vtype == VType::time_point) {
                    std::chrono::year_month_day ymd;
                    std::chrono::hh_mm_ss<std::chrono::microseconds> time;
                    art::times::_internal_::extract_date_time(args[0], ymd, time);
                    art::ustring res;
                    res += (char)('0' + time.hours().count() / 10);
                    res += (char)('0' + time.hours().count() % 10);
                    res += ':';
                    res += (char)('0' + time.minutes().count() / 10);
                    res += (char)('0' + time.minutes().count() % 10);
                    res += ':';
                    res += (char)('0' + time.seconds().count() / 10);
                    res += (char)('0' + time.seconds().count() % 10);
                    return res;
                } else
                    throw InvalidOperation("operator `time` can be applied only to time_point types");
            }

            //date
            AttachAFunc(date, 1) {
                if (args[0].meta.vtype == VType::time_point) {
                    std::chrono::year_month_day ymd;
                    art::times::_internal_::extract_date(args[0], ymd);
                    art::ustring res;
                    res += (char)('0' + (unsigned int)ymd.day() / 10);
                    res += (char)('0' + (unsigned int)ymd.day() % 10);
                    res += '.';
                    res += (char)('0' + (unsigned int)ymd.month() / 10);
                    res += (char)('0' + (unsigned int)ymd.month() % 10);
                    res += '.';
                    res += std::to_string((int)ymd.year());
                    return res;
                } else
                    throw InvalidOperation("operator `date` can be applied only to time_point types");
            }

            //datetime
            AttachAFunc(datetime, 1) {
                if (args[0].meta.vtype == VType::time_point) {
                    std::chrono::year_month_day ymd;
                    std::chrono::hh_mm_ss<std::chrono::microseconds> time;
                    art::times::_internal_::extract_date_time(args[0], ymd, time);
                    art::ustring res;
                    res += (char)('0' + (unsigned int)ymd.day() / 10);
                    res += (char)('0' + (unsigned int)ymd.day() % 10);
                    res += '.';
                    res += (char)('0' + (unsigned int)ymd.month() / 10);
                    res += (char)('0' + (unsigned int)ymd.month() % 10);
                    res += '.';
                    res += std::to_string((int)ymd.year());
                    res += ' ';
                    res += (char)('0' + time.hours().count() / 10);
                    res += (char)('0' + time.hours().count() % 10);
                    res += ':';
                    res += (char)('0' + time.minutes().count() / 10);
                    res += (char)('0' + time.minutes().count() % 10);
                    res += ':';
                    res += (char)('0' + time.seconds().count() / 10);
                    res += (char)('0' + time.seconds().count() % 10);
                    return res;
                } else
                    throw InvalidOperation("operator `datetime` can be applied only to time_point types");
            }

            //years
            AttachAFunc(years, 1) {
                if (args[0].meta.vtype == VType::time_point) {
                    std::chrono::year_month_day ymd;
                    art::times::_internal_::extract_date(args[0], ymd);
                    return (uint64_t)(int)ymd.year();
                } else
                    throw InvalidOperation("operator `years` can be applied only to time_point types");
            }

            //months
            AttachAFunc(months, 1) {
                if (args[0].meta.vtype == VType::time_point) {
                    std::chrono::year_month_day ymd;
                    art::times::_internal_::extract_date(args[0], ymd);
                    return (uint64_t)(unsigned int)ymd.month();
                } else
                    throw InvalidOperation("operator `months` can be applied only to time_point types");
            }

            //weeks
            AttachAFunc(weeks, 1) {
                if (args[0].meta.vtype == VType::time_point) {
                    std::chrono::year_month_day ymd;
                    art::times::_internal_::extract_date(args[0], ymd);
                    return (uint64_t)(unsigned int)ymd.day() / 7;
                } else
                    throw InvalidOperation("operator `weeks` can be applied only to time_point types");
            }

            //days
            AttachAFunc(days, 1) {
                if (args[0].meta.vtype == VType::time_point) {
                    std::chrono::year_month_day ymd;
                    art::times::_internal_::extract_date(args[0], ymd);
                    return (uint64_t)(unsigned int)ymd.day();
                } else
                    throw InvalidOperation("operator `days` can be applied only to time_point types");
            }

            //hours
            AttachAFunc(hours, 1) {
                if (args[0].meta.vtype == VType::time_point) {
                    std::chrono::year_month_day ymd;
                    std::chrono::hh_mm_ss<std::chrono::microseconds> time;
                    art::times::_internal_::extract_date_time(args[0], ymd, time);
                    return (uint64_t)(unsigned int)time.hours().count();
                } else
                    throw InvalidOperation("operator `hours` can be applied only to time_point types");
            }

            //minutes
            AttachAFunc(minutes, 1) {
                if (args[0].meta.vtype == VType::time_point) {
                    std::chrono::year_month_day ymd;
                    std::chrono::hh_mm_ss<std::chrono::microseconds> time;
                    art::times::_internal_::extract_date_time(args[0], ymd, time);
                    return (uint64_t)(unsigned int)time.minutes().count();
                } else
                    throw InvalidOperation("operator `minutes` can be applied only to time_point types");
            }

            //seconds
            AttachAFunc(seconds, 1) {
                if (args[0].meta.vtype == VType::time_point) {
                    std::chrono::year_month_day ymd;
                    std::chrono::hh_mm_ss<std::chrono::microseconds> time;
                    art::times::_internal_::extract_date_time(args[0], ymd, time);
                    return (uint64_t)time.seconds().count();
                } else
                    throw InvalidOperation("operator `seconds` can be applied only to time_point types");
            }

            //milliseconds
            AttachAFunc(milliseconds, 1) {
                if (args[0].meta.vtype == VType::time_point) {
                    std::chrono::year_month_day ymd;
                    std::chrono::hh_mm_ss<std::chrono::microseconds> time;
                    art::times::_internal_::extract_date_time(args[0], ymd, time);
                    return (uint64_t)time.subseconds().count() / 1000;
                } else
                    throw InvalidOperation("operator `milliseconds` can be applied only to time_point types");
            }

            //microseconds
            AttachAFunc(microseconds, 1) {
                if (args[0].meta.vtype == VType::time_point) {
                    std::chrono::year_month_day ymd;
                    std::chrono::hh_mm_ss<std::chrono::microseconds> time;
                    art::times::_internal_::extract_date_time(args[0], ymd, time);
                    return (uint64_t)time.subseconds().count();
                } else
                    throw InvalidOperation("operator `microseconds` can be applied only to time_point types");
            }

            AttachAFunc(upper, 1) {
                art::ustring str = (art::ustring)args[0];
                for (auto& it : str)
                    if (it >= 'a' && it <= 'z')
                        it -= 32;
                return str;
            }

            AttachAFunc(lower, 1) {
                art::ustring str = (art::ustring)args[0];
                for (auto& it : str)
                    if (it >= 'A' && it <= 'Z')
                        it += 32;
                return str;
            }

            AttachAFunc(reverse, 1) {
                art::ustring str = (art::ustring)args[0];
                std::reverse(str.begin(), str.end());
                return str;
            }

            AttachAFunc(trace, 1) {
                CXX::excepted_array(args[0]);
                CXX::array_size_min(args[0].size(), 1);
                CXX::excepted_array(args[0][0]);
                CXX::array_size_min(args[0][0].size(), 3);

                art::ustring str;
                for (auto& i : args[0])
                    str += (art::ustring)(i[1] + " " + (i[2] == (SIZE_MAX) ? "null" : i[2])) + '\n';
                return str;
            }

            ValueItem* inner_format(ValueItem* args, uint32_t count) {
                return new ValueItem(_format(args, count));
            }
        }

        std::unordered_map<art::ustring, art::shared_ptr<FuncEnvironment>, art::hash<art::ustring>> printf_operators = {
            {"d", new FuncEnvironment(standard_operator::d)},
            {"i", new FuncEnvironment(standard_operator::d)},
            {"u", new FuncEnvironment(standard_operator::u)},
            {"o", new FuncEnvironment(standard_operator::o)},
            {"x", new FuncEnvironment(standard_operator::x)},
            {"X", new FuncEnvironment(standard_operator::X)},
            {"f", new FuncEnvironment(standard_operator::f)},
            {"F", new FuncEnvironment(standard_operator::F)},
            {"e", new FuncEnvironment(standard_operator::e)},
            {"E", new FuncEnvironment(standard_operator::E)},
            {"g", new FuncEnvironment(standard_operator::g)},
            {"G", new FuncEnvironment(standard_operator::G)},
            {"a", new FuncEnvironment(standard_operator::a)},
            {"A", new FuncEnvironment(standard_operator::A)},
            {"c", new FuncEnvironment(standard_operator::c)},
            {"s", new FuncEnvironment(standard_operator::s)},
            {"p", new FuncEnvironment(standard_operator::p)},
            {"n", new FuncEnvironment(standard_operator::n)},
            {"%", new FuncEnvironment(standard_operator::percent)},
            {"time", new FuncEnvironment(standard_operator::time)},
            {"date", new FuncEnvironment(standard_operator::date)},
            {"datetime", new FuncEnvironment(standard_operator::datetime)},
            {"years", new FuncEnvironment(standard_operator::years)},
            {"months", new FuncEnvironment(standard_operator::months)},
            {"weeks", new FuncEnvironment(standard_operator::weeks)},
            {"days", new FuncEnvironment(standard_operator::days)},
            {"hours", new FuncEnvironment(standard_operator::hours)},
            {"minutes", new FuncEnvironment(standard_operator::minutes)},
            {"seconds", new FuncEnvironment(standard_operator::seconds)},
            {"milliseconds", new FuncEnvironment(standard_operator::milliseconds)},
            {"microseconds", new FuncEnvironment(standard_operator::microseconds)},
            {"upper", new FuncEnvironment(standard_operator::upper)},
            {"lower", new FuncEnvironment(standard_operator::lower)},
            {"reverse", new FuncEnvironment(standard_operator::reverse)},
            {"trace", new FuncEnvironment(standard_operator::trace)},
            {"inner_format", new FuncEnvironment(standard_operator::inner_format)}};

        ValueItem* register_format_operator(ValueItem* args, uint32_t len) {
            if (len >= 2) {
                art::ustring _operator = (art::ustring)args[0];
                auto func = args[1].funPtr();
                if (func != nullptr)
                    printf_operators[_operator] = *func;
                else
                    throw InvalidOperation("printf operator must be a function");
            }
            return nullptr;
        }

        art::ustring printf_operator(const art::ustring& _operator, ValueItem* args, uint32_t argc) {
            auto it = printf_operators.find(_operator);
            if (it != printf_operators.end()) {
                auto& func = it->second;
                if (func)
                    return (art::ustring)CXX::aCall(func, args, argc);
            } else
                throw InvalidOperation("printf operator not registered: " + _operator);
            return "";
        }

        art::ustring _format(ValueItem* args, uint32_t len) {
            if (args != nullptr) {
                art::ustring print_string;
                art::ustring parse_string = (art::ustring)args[0];
                uint32_t argument_index = 1;
                bool in_scope = false;
                bool in_scope_index = false;
                bool in_scope_index_final = false;
                bool slash = false;
                bool short_operator = false;
                bool long_operator = false;
                bool range_operator = false;
                bool long_operator_range_passed = false;
                bool long_operator_range_empty = false;
                bool long_operator_range_m_passed = false;
                uint32_t n = 0;
                uint32_t m = 0;
                art::ustring _operator;

                uint32_t auto_tab = 0;


                size_t i = 0;
                for (char ch : parse_string) {
                    i++;
                    // if found [] then get next item from arguments
                    // if found [n] then get n+1 argument from arguments
                    // if found [:] then cast all arguments from 1-len to string and print [1:len]
                    // if found [n:] then cast all arguments from n-len to string and print [n:len]
                    // if found [:n] then cast all arguments from 1-n to string and print [1:n]
                    // if found [n:m] then cast all arguments from n-m to string and print [n:m]
                    // if found \[ then print [
                    // if found \] then print ]
                    // if found \{ then print {
                    // if found \} then print }
                    // if found \\ then print \
                    // if found {[]any symbols} execute operator with next item from arguments and put result to string
                    // if found {[n]any symbols} execute operator with specified argument and put result to string
                    // if found {[:]any symbols} execute operator with all arguments and put result to string
                    // if found {[n:]any symbols} execute operator with specified arguments that starts from n and put result to string
                    // if found {[:n]any symbols} execute operator with all specified arguments up to n and put result to string
                    // if found {[n:m]any symbols} execute operator with specified arguments range and put result to string
                    // if found {any symbols} execute operator and put result to string
                    // if found % then execute short operator and put result to string
                    if (slash) {
                        switch (ch) {
                        case '\\':
                            print_string += '\\';
                            break;
                        case 'n':
                            print_string += '\n';
                            if (auto_tab)
                                print_string += art::ustring('\t', auto_tab);
                            break;
                        case 't':
                            print_string += '\t';
                            break;
                        case 'r':
                            print_string += '\r';
                            break;
                        case 'v':
                            print_string += '\v';
                            break;
                        case 'a':
                            print_string += '\a';
                            break;
                        case 'b':
                            print_string += '\b';
                            break;
                        case 'f':
                            print_string += '\f';
                            break;
                        case '[':
                            print_string += '[';
                        case '{':
                            print_string += '{';
                        case ']':
                            print_string += ']';
                        case '}':
                            print_string += '}';
                            break;
                        case '>':
                            auto_tab++;
                            if (!auto_tab)
                                throw NumericOverflowException();
                            break;
                        case '<':
                            auto_tab--;
                            if (auto_tab == UINT32_MAX)
                                throw NumericUndererflowException();
                            break;
                        default:
                            print_string += '\\';
                            print_string += ch;
                            break;
                        }
                        slash = false;
                        continue;
                    }
                    if (short_operator) {
                        if (argument_index >= len)
                            throw OutOfRange(art::ustring("printf: index out of range, len = ") + std::to_string(len) + art::ustring(", index = ") + std::to_string(argument_index));
                        print_string += printf_operator(ch, args + argument_index++, 1);
                        _operator.clear();
                        short_operator = false;
                        continue;
                    }
                    switch (ch) {
                    case '\n':
                        if (!in_scope) {
                            print_string += '\n';
                            if (auto_tab)
                                print_string += art::ustring('\t', auto_tab);
                        }
                        break;
                    case '\\':
                        slash = true;
                        break;
                    case '[': {
                        in_scope = true;
                        range_operator = true;
                        in_scope_index = false;
                        in_scope_index_final = false;
                        n = 0;
                        m = 0;
                        break;
                    }
                    case '{': {
                        in_scope = true;
                        long_operator = true;
                        in_scope_index = false;
                        in_scope_index_final = false;
                        long_operator_range_passed = false;
                        long_operator_range_empty = false;
                        break;
                    }
                    case '}': {
                        if (argument_index >= len)
                            throw OutOfRange(art::ustring("printf: index out of range, len = ") + std::to_string(len) + art::ustring(", index = ") + std::to_string(argument_index) + ", at: " + std::to_string(i));
                        art::ustring result;
                        if (long_operator_range_passed && !long_operator_range_empty) {
                            uint32_t begin_ = in_scope_index ? n + 1 : 1;
                            uint32_t end_ = long_operator_range_m_passed ? m + 1 : len;
                            if (begin_ > end_) {
                                uint32_t tmp = begin_;
                                begin_ = end_;
                                end_ = tmp;
                            }
                            if (end_ > len)
                                throw OutOfRange(art::ustring("printf: index out of range, len = ") + std::to_string(len) + art::ustring(", index = ") + std::to_string(end_) + ", at: " + std::to_string(i));

                            result = printf_operator(_operator, args + begin_, begin_ - end_);
                        } else
                            result = printf_operator(_operator, args + argument_index++, 1);
                        if (auto_tab) {
                            size_t result_len = result.size();
                            for (size_t i = 0; i < result_len; i++) {
                                if (result[i] == '\n') {
                                    result.insert(i + 1, art::ustring('\t', auto_tab));
                                    i += auto_tab;
                                }
                            }
                        }
                        print_string += result;
                        in_scope = false;
                        long_operator = false;
                        _operator.clear();
                        break;
                    }
                    case ']': {
                        if (!in_scope || !range_operator) {
                            print_string += ']';
                            continue;
                        } else if (in_scope_index || in_scope_index_final) {
                            uint32_t begin_ = in_scope_index ? n + 1 : 1;
                            uint32_t end_ = long_operator_range_m_passed ? m + 1 : len;
                            if (begin_ > end_) {
                                uint32_t tmp = begin_;
                                begin_ = end_;
                                end_ = tmp;
                            }
                            if (end_ > len)
                                throw OutOfRange(art::ustring("printf: index out of range, len = ") + std::to_string(len) + art::ustring(", index = ") + std::to_string(end_) + ", at: " + std::to_string(i));
                            if (!long_operator) {
                                print_string += '[';
                                for (uint32_t i = begin_; i < end_; i++)
                                    print_string += (art::ustring)args[i] + (i == end_ - 1 ? "" : ", ");
                                print_string += ']';
                            }
                            if (!long_operator) {
                                in_scope = false;
                                in_scope_index = false;
                                in_scope_index_final = false;
                            } else {
                                long_operator_range_passed = true;
                                n = 0;
                                m = 0;
                            }
                        } else {
                            if (long_operator) {
                                long_operator_range_passed = true;
                                long_operator_range_empty = true;
                            } else {
                                if (argument_index > len)
                                    throw OutOfRange(art::ustring("printf: index out of range, len = ") + std::to_string(len) + art::ustring(", index = ") + std::to_string(argument_index) + ", at: " + std::to_string(i));
                                print_string += (art::ustring)args[argument_index++];
                            }
                        }
                        range_operator = false;
                        break;
                    }
                    case ':': {
                        if (in_scope && range_operator) {
                            in_scope_index_final = true;
                            long_operator_range_m_passed = true;
                        } else {
                            print_string += ':';
                        }
                        break;
                    }
                    case '0':
                    case '1':
                    case '2':
                    case '3':
                    case '4':
                    case '5':
                    case '6':
                    case '7':
                    case '8':
                    case '9': {
                        if (in_scope && range_operator) {
                            uint32_t old_value = in_scope_index_final ? m : n;
                            if (in_scope_index_final) {
                                m = m * 10 + (ch - '0');
                                if (m < old_value)
                                    throw NumericOverflowException();
                            } else {
                                n = n * 10 + (ch - '0');
                                if (n < old_value)
                                    throw NumericOverflowException();
                                in_scope_index = true;
                            }
                        } else
                            print_string += ch;
                        break;
                    }
                    case '%':
                        if (!in_scope) {
                            short_operator = true;
                            break;
                        }
                    default:
                        if (in_scope) {
                            if (range_operator)
                                throw InvalidOperation("Invalid range operator at: " + std::to_string(i));
                            else if (long_operator)
                                _operator += ch;
                            else {
                                invite_to_debugger("Caught invalid state of _format function, get full stack trace, arguments and send to developer");
                                throw InvalidOperation("Invalid state of _format function, at: " + std::to_string(i));
                            }
                        } else
                            print_string += ch;
                        break;
                    }
                }

                if (short_operator)
                    print_string += '%';
                else if (slash)
                    print_string += '\\';

                //TODO
                //check in_scope and in_scope_*
                return print_string;
            }
            return "";
        }

        ValueItem* format(ValueItem* args, uint32_t len) {
            return new ValueItem(_format(args, len));
        }
    }
}