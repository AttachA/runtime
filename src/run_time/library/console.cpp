// Copyright Danyil Melnytskyi 2022-Present
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE or copy at
// http://www.boost.org/LICENSE_1_0.txt)
#define _CRT_SECURE_NO_WARNINGS

#ifdef _WIN64
#include <Windows.h>
#endif
#include <cstdio>
#include <string>
#include <utf8cpp/utf8.h>

#include <run_time/AttachA_CXX.hpp>
#include <util/exceptions.hpp>

namespace art {
    namespace console {
        auto _stdin = stdin;
        auto _stdout = stdout;
        auto _stderr = stderr;
#ifdef _WIN64
        bool configureConsole() {
            HANDLE hout = GetStdHandle(-11);
            bool result = true;
            DWORD mode = 0;
            GetConsoleMode(hout, &mode);
            if (!SetConsoleMode(hout, mode | ENABLE_PROCESSED_OUTPUT | ENABLE_VIRTUAL_TERMINAL_PROCESSING))
                result = false;
            if (setvbuf(_stderr, nullptr, _IOFBF, 1024))
                return false;
            if (setvbuf(_stdout, nullptr, _IOFBF, 1024))
                return false;
            if (!SetConsoleOutputCP(65001))
                return false;

            CONSOLE_FONT_INFOEX info;
            memset(&info, 0, sizeof(CONSOLE_FONT_INFOEX));
            info.cbSize = sizeof(CONSOLE_FONT_INFOEX);
            if (GetCurrentConsoleFontEx(hout, false, &info)) {
                info.FontFamily = 0; //FF_DONTCARE (0 << 4) == 0
                info.dwFontSize.X = 0;
                info.dwFontSize.Y = 14;
                info.FontWeight = 400;
                wcscpy_s(info.FaceName, L"Lucida Console");
                if (SetCurrentConsoleFontEx(hout, false, &info))
                    return result;
            }
            return false;
        }

        //enable ANSI escape codes in console
        const bool is_loaded = configureConsole();
#else
        constexpr bool is_loaded = true;
#endif
        enum class RW_CONS : bool {
            R,
            W
        };
        thread_local RW_CONS rw_switcher = RW_CONS::R;

        void was_w_mode() {
            if (rw_switcher == RW_CONS::R)
                fflush(_stdin);
        }

        void was_r_mode() {
            if (rw_switcher == RW_CONS::W) {
                fflush(_stdout);
                fflush(_stderr);
            }
        }

        template <size_t N>
        inline void print(const char (&chars)[N]) {
            if (rw_switcher != RW_CONS::W)
                fflush(_stdin);
            fwrite(chars, 1, N, _stdout);
            fflush(_stdout);
        };

        ValueItem* printLine(ValueItem* args, uint32_t len) {
            was_w_mode();
            if (args == nullptr)
                fwrite("\n", 1, 1, _stdout);
            else {
                while (len--) {
                    auto& it = *args++;
                    art::ustring tmp = (art::ustring)it + '\n';
                    fwrite(tmp.c_str(), 1, tmp.size(), _stdout);
                    fflush(_stdout);
                }
            }
            return nullptr;
        }

        ValueItem* print(ValueItem* args, uint32_t len) {
            if (args != nullptr) {
                was_w_mode();
                while (len--) {
                    auto& it = *args++;
                    art::ustring tmp = (art::ustring)it;
                    fwrite(tmp.c_str(), 1, tmp.size(), _stdout);
                    fflush(_stdout);
                }
            }
            return nullptr;
        }

        namespace standard_short_operator {
            //d && i - integer
            AttachAFun(d, 1, {
                if (is_integer(args[0].meta.vtype))
                    return (int64_t)args[0];
                else
                    throw InvalidOperation("operator d can be applied only to integer types");
            });
            //u - unsigned integer
            AttachAFun(u, 1, {
                if (integer_unsigned(args[0].meta.vtype))
                    return (uint64_t)args[0];
                else
                    throw InvalidOperation("operator i can be applied only to integer types");
            });
            //o - octal
            AttachAFun(o, 1, {
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
            });
            //x - hex
            AttachAFun(x, 1, {
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
            });
            //X - HEX
            AttachAFun(X, 1, {
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
            });
            //f - float
            AttachAFun(f, 1, {
                if (args[0].meta.vtype == VType::flo || args[0].meta.vtype == VType::doub)
                    return (double)args[0];
                else
                    throw InvalidOperation("operator f can be applied only to float types");
            });
            //F - float
            AttachAFun(F, 1, {
                if (args[0].meta.vtype == VType::flo || args[0].meta.vtype == VType::doub) {
                    art::ustring tmp = std::to_string((double)args[0]);
                    for (auto& it : tmp)
                        if (it >= 'a' && it <= 'z')
                            it -= 32;
                } else
                    throw InvalidOperation("operator F can be applied only to float types");
            });
            //e - float
            AttachAFun(e, 1, {
                if (args[0].meta.vtype == VType::flo || args[0].meta.vtype == VType::doub) {
                    auto tmp = (double)args[0];
                    art::ustring res;
                    res.resize(22);
                    auto len = sprintf(res.data(), "%e", tmp);
                    res.resize(len);
                    return res;
                } else
                    throw InvalidOperation("operator e can be applied only to float types");
            });
            //E - float
            AttachAFun(E, 1, {
                if (args[0].meta.vtype == VType::flo || args[0].meta.vtype == VType::doub) {
                    auto tmp = (double)args[0];
                    art::ustring res;
                    res.resize(22);
                    auto len = sprintf(res.data(), "%E", tmp);
                    res.resize(len);
                    return res;
                } else
                    throw InvalidOperation("operator E can be applied only to float types");
            });
            //g - float
            AttachAFun(g, 1, {
                if (args[0].meta.vtype == VType::flo || args[0].meta.vtype == VType::doub) {
                    auto tmp = (double)args[0];
                    art::ustring res;
                    res.resize(22);
                    auto len = sprintf(res.data(), "%g", tmp);
                    res.resize(len);
                    return res;
                } else
                    throw InvalidOperation("operator g can be applied only to float types");
            });
            //G - float
            AttachAFun(G, 1, {
                if (args[0].meta.vtype == VType::flo || args[0].meta.vtype == VType::doub) {
                    auto tmp = (double)args[0];
                    art::ustring res;
                    res.resize(22);
                    auto len = sprintf(res.data(), "%G", tmp);
                    res.resize(len);
                    return res;
                } else
                    throw InvalidOperation("operator G can be applied only to float types");
            });
            //a - float
            AttachAFun(a, 1, {
                if (args[0].meta.vtype == VType::flo || args[0].meta.vtype == VType::doub) {
                    auto tmp = (double)args[0];
                    art::ustring res;
                    res.resize(22);
                    auto len = sprintf(res.data(), "%a", tmp);
                    res.resize(len);
                    return res;
                } else
                    throw InvalidOperation("operator a can be applied only to float types");
            });
            //A - float
            AttachAFun(A, 1, {
                if (args[0].meta.vtype == VType::flo || args[0].meta.vtype == VType::doub) {
                    auto tmp = (double)args[0];
                    art::ustring res;
                    res.resize(22);
                    auto len = sprintf(res.data(), "%A", tmp);
                    res.resize(len);
                    return res;
                } else
                    throw InvalidOperation("operator A can be applied only to float types");
            });
            //c - char
            AttachAFun(c, 1, {
                if (args[0].meta.vtype == VType::string) {
                    auto tmp = (art::ustring)args[0];
                    if (tmp.size() == 1)
                        return tmp[0];
                    else
                        throw InvalidOperation("operator c can be applied only to char types");
                } else
                    throw InvalidOperation("operator c can be applied only to char types");
            });
            //s - string
            AttachAFun(s, 1, {
                if (args[0].meta.vtype == VType::string)
                    return (art::ustring)args[0];
                else
                    throw InvalidOperation("operator s can be applied only to string types");
            });
            //p - pointer
            AttachAFun(p, 1, {
                if (args[0].meta.vtype == VType::undefined_ptr)
                    return (void*)args[0];
                else
                    throw InvalidOperation("operator p can be applied only to pointer types");
            });
            //n - pointer
            AttachAFun(n, 1, {
                if (args[0].meta.vtype == VType::undefined_ptr)
                    return (void*)args[0];
                else
                    throw InvalidOperation("operator n can be applied only to pointer types");
            });
            //%
            AttachAFun(percent, 0, {
                return "%";
            });
        }

        std::unordered_map<art::ustring, art::shared_ptr<FuncEnvironment>, art::hash<art::ustring>> printf_operators = {
            {"d", new FuncEnvironment(standard_short_operator::d)},
            {"i", new FuncEnvironment(standard_short_operator::d)},
            {"u", new FuncEnvironment(standard_short_operator::u)},
            {"o", new FuncEnvironment(standard_short_operator::o)},
            {"x", new FuncEnvironment(standard_short_operator::x)},
            {"X", new FuncEnvironment(standard_short_operator::X)},
            {"f", new FuncEnvironment(standard_short_operator::f)},
            {"F", new FuncEnvironment(standard_short_operator::F)},
            {"e", new FuncEnvironment(standard_short_operator::e)},
            {"E", new FuncEnvironment(standard_short_operator::E)},
            {"g", new FuncEnvironment(standard_short_operator::g)},
            {"G", new FuncEnvironment(standard_short_operator::G)},
            {"a", new FuncEnvironment(standard_short_operator::a)},
            {"A", new FuncEnvironment(standard_short_operator::A)},
            {"c", new FuncEnvironment(standard_short_operator::c)},
            {"s", new FuncEnvironment(standard_short_operator::s)},
            {"p", new FuncEnvironment(standard_short_operator::p)},
            {"n", new FuncEnvironment(standard_short_operator::n)},
            {"%", new FuncEnvironment(standard_short_operator::percent)}};

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
                if (func) {
                    auto result = func->syncWrapper(args, argc);
                    if (result != nullptr) {
                        art::ustring tmp = (art::ustring)*result;
                        delete result;
                        return tmp;
                    }
                }
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
                uint32_t n = 0;
                uint32_t m = 0;
                art::ustring _operator;

                for (char ch : parse_string) {
                    // if found [] then get next item from arguments
                    // if found [n] then get n+1 argument from arguments
                    // if found [:] then cast all arguments from 1-len to string and print [1:len]
                    // if found [n:] then cast all arguments from n-len to string and print [n:len]
                    // if found [:n] then cast all arguments from 1-n to string and print [1:n]
                    // if found [n:m] then cast all arguments from n-m to string and print [n:m]
                    // if found \[ then print [
                    // if found \] then print ]
                    // if found \\ then print \
					// if found [any symbols] execute operator and put result to string
                    // if found % then execute short operator and put result to string
                    if (slash) {
                        switch (ch) {
                        case 'n':
                            print_string += '\n';
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
                        default:
                            print_string += ch;
                            break;
                        }
                        print_string += ch;
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
                    if (!_operator.empty()) {
                        if (ch != ']') {
                            _operator += ch;
                            continue;
                        } else {
                            if (argument_index >= len)
                                throw OutOfRange(art::ustring("printf: index out of range, len = ") + std::to_string(len) + art::ustring(", index = ") + std::to_string(argument_index));
                            print_string += printf_operator(_operator, args + argument_index++, 1);
                            _operator.clear();
                            continue;
                        }
                    }
                    switch (ch) {
                    case '\\':
                        slash = true;
                        break;
                    case '[': {
                        in_scope = true;
                        in_scope_index = false;
                        in_scope_index_final = false;
                        break;
                    }
                    case ']': {
                        if (!in_scope) {
                            print_string += ']';
                            continue;
                        }
                        if (in_scope_index || in_scope_index_final) {
                            uint32_t begin_ = in_scope_index ? n + 1 : 1;
                            uint32_t end_ = in_scope_index_final ? m + 1 : len;
                            if (begin_ > end_) {
                                uint32_t tmp = begin_;
                                begin_ = end_;
                                end_ = tmp;
                            }
                            if (end_ > len)
                                throw OutOfRange(art::ustring("printf: index out of range, len = ") + std::to_string(len) + art::ustring(", index = ") + std::to_string(end_));
                            print_string += '[';
                            for (uint32_t i = begin_; i < end_; i++)
                                print_string += (art::ustring)args[i] + (i == end_ - 1 ? "" : ", ");
                            print_string += ']';
                            n = 0;
                            m = 0;
                        } else {
                            if (argument_index > len)
                                throw OutOfRange(art::ustring("printf: index out of range, len = ") + std::to_string(len) + art::ustring(", index = ") + std::to_string(argument_index));
                            print_string += (art::ustring)args[argument_index++];
                        }
                        break;
                    }
                    case ':': {
                        if (in_scope) {
                            if (in_scope_index) {
                                in_scope_index_final = true;
                            } else {
                                in_scope_index = true;
                            }
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
                        if (in_scope) {
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
                    case '%': {
                        if (in_scope)
                            throw InvalidOperation("In scope short operators not allowed");
                        short_operator = true;
                        break;
                    }
                    default:
                        if (in_scope)
                            _operator += ch;
                        else
                            print_string += ch;
                        break;
                    }
                }
                //TODO
                //check short_operator
                //check slash
                //check in_scope and in_scope_*
                return print_string;
            }
            return "";
        }

        ValueItem* format(ValueItem* args, uint32_t len) {
            return new ValueItem(_format(args, len));
        }

        ValueItem* printf(ValueItem* args, uint32_t len) {
            auto res = _format(args, len);
            was_w_mode();
            fwrite(res.c_str(), 1, res.size(), _stdout);
            fflush(_stdout);
            return nullptr;
        }

        void resetModifiers() {
            print("\033[0m");
        }

        void boldText() {
            print("\033[1m");
        }

        void italicText() {
            print("\033[3m");
        }

        void underlineText() {
            print("\033[4m");
        }

        void slowBlink() {
            print("\033[5m");
        }

        void rapidBlink() {
            print("\033[6m");
        }

        void invertColors() {
            print("\033[7m");
        }

        void notBoldText() {
            print("\033[22m");
        }

        void notUnderlinedText() {
            print("\033[24m");
        }

        void hideBlinkText() {
            print("\033[25m");
        }

        void resetTextColor() {
            print("\033[39m");
        }

        void resetBgColor() {
            print("\033[49m");
        }

        void setTextColor(uint8_t r, uint8_t g, uint8_t b) {
            ::printf("\033[38;2;%d;%d;%dm", r, g, b);
        }

        void setBgColor(uint8_t r, uint8_t g, uint8_t b) {
            ::printf("\033[48;2;%d;%d;%dm", r, g, b);
        }

        void setPos(uint16_t row, uint16_t col) {
            ::printf("\033[%d;%dH", row + 1, col + 1);
        }

        void saveCurPos() {
            print("\033[s");
        }

        void loadCurPos() {
            print("\033[u");
        }

        void setLine(uint32_t y) {
            if (y == 0)
                print("\033[999999D");
            else
                ::printf("\033[999999D\033[%dC", y);
        }

        void showCursor() {
            print("\033[?25h");
        }

        void hideCursor() {
            print("\033[?25l");
        }

        ValueItem* readWord(ValueItem* args, uint32_t len) {
            was_r_mode();
            art::ustring str;
            str.set_unsafe_state(true, false);
            char c;
            while ((c = getchar()) != EOF) {
                switch (c) {
                case '\n':
                case '\b':
                case '\t':
                case '\r':
                case ' ':
                    goto end;
                default:
                    str += c;
                }
            }
        end:
            str.set_unsafe_state(false, false);
            return new ValueItem(std::move(str));
        }

        ValueItem* readLine(ValueItem* args, uint32_t len) {
            was_r_mode();
            art::ustring str;
            str.set_unsafe_state(true, false);
            char c;
            bool do_continue = true;
            while (do_continue) {
                switch (c = getchar()) {
                case EOF:
                case '\n':
                    do_continue = false;
                }
                str += c;
            }
            str.set_unsafe_state(false, false);
            return new ValueItem(std::move(str));
        }

        ValueItem* readInput(ValueItem* args, uint32_t len) {
            was_r_mode();
            art::ustring str;
            str.set_unsafe_state(true, false);
            char c;
            while ((c = getchar()) != EOF)
                str += c;
            str.set_unsafe_state(false, false);
            return new ValueItem(std::move(str));
        }

        ValueItem* readValue(ValueItem* args, uint32_t len) {
            was_r_mode();
            art::ustring str;
            str.set_unsafe_state(true, false);
            char c;
            while ((c = getchar()) != EOF) {
                switch (c) {
                case '\n':
                case '\b':
                case '\t':
                case '\r':
                case ' ':
                    break;
                default:
                    str += c;
                }
            }
            str.set_unsafe_state(false, false);
            return new ValueItem(ABI_IMPL::SBcast(str));
        }

        ValueItem* readInt(ValueItem* args, uint32_t len) {
            was_r_mode();
            bool first = true;
            bool minus = false;
            uint64_t old_num = 0;
            uint64_t num = 0;
            char c;
            while ((c = getchar()) != EOF) {
                if (first) {
                    if (c == '-')
                        minus = true;
                    else if (c != '+')
                        throw InvalidInput("Invalid input, expected a number");

                    first = false;
                    continue;
                }
                if (c >= '0' && c <= '9') {
                    num *= 10;
                    num += c - '0';
                    if (num < old_num)
                        throw InvalidInput("Invalid input, too large number");
                    old_num = num;
                }
            }

            ValueItem res;
            if (minus) {
                if (num == (int8_t)num)
                    res = ValueItem(-(int8_t)num);
                else if (num == (int16_t)num)
                    res = ValueItem(-(int16_t)num);
                else if (num == (int32_t)num)
                    res = ValueItem(-(int32_t)num);
                else if (num == (int64_t)num)
                    res = ValueItem(-(int64_t)num);
                else
                    throw InvalidInput("Invalid input, too large number");
            } else {
                if (num == (uint8_t)num)
                    res = ValueItem((uint8_t)num);
                else if (num == (uint16_t)num)
                    res = ValueItem((uint16_t)num);
                else if (num == (uint32_t)num)
                    res = ValueItem((uint32_t)num);
                else if (num == (uint64_t)num)
                    res = ValueItem((uint64_t)num);
            }
            return new ValueItem(res);
        }
    }
}
