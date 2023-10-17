#include <run_time/AttachA_CXX.hpp>
#include <run_time/library/times.hpp>

namespace art {
    namespace times {
        uint16_t _days_elapsed_from_start(std::chrono::year_month_day& ymd) {
            uint16_t days_elapsed_from_start = 0;
            for (uint8_t i = 0; i < (unsigned int)ymd.month(); i++)
                days_elapsed_from_start += _internal_::days_in_month(i + 1, _internal_::is_leap((uint16_t)(int)ymd.year()));
            days_elapsed_from_start += (uint16_t)(unsigned int)ymd.day();
            return days_elapsed_from_start;
        }

        ValueItem* now(ValueItem*, uint32_t) {
            return new ValueItem(std::chrono::high_resolution_clock::now());
        }

        ValueItem* now_utc(ValueItem*, uint32_t) {
            return new ValueItem(_internal_::as_utc(std::chrono::high_resolution_clock::now()));
        }

        namespace get {

            AttachAFun(year, 1, {
                std::chrono::year_month_day ymd;
                _internal_::extract_date(args[0], ymd);
                return (uint32_t)(int)ymd.year();
            });

            AttachAFun(year_week, 1, {
                std::chrono::year_month_day ymd;
                _internal_::extract_date(args[0], ymd);
                return _days_elapsed_from_start(ymd) / 7;
            });

            AttachAFun(year_day, 1, {
                std::chrono::year_month_day ymd;
                _internal_::extract_date(args[0], ymd);
                return _days_elapsed_from_start(ymd);
            });

            AttachAFun(month, 1, {
                std::chrono::year_month_day ymd;
                _internal_::extract_date(args[0], ymd);
                return (uint8_t)(unsigned int)ymd.month();
            });
            AttachAFun(month_week, 1, {
                std::chrono::year_month_weekday ymw;
                _internal_::extract_date(args[0], ymw);
                return (uint8_t)ymw.weekday().iso_encoding();
            });

            AttachAFun(week_day, 1, {
                std::chrono::year_month_weekday ymw;
                _internal_::extract_date(args[0], ymw);
                return (uint8_t)ymw.weekday().iso_encoding();
            });

            AttachAFun(day, 1, {
                std::chrono::year_month_day ymd;
                _internal_::extract_date(args[0], ymd);
                return (uint8_t)(unsigned int)ymd.day();
            });

            AttachAFun(hour, 1, {
                std::chrono::year_month_day ymd;
                std::chrono::hh_mm_ss<std::chrono::microseconds> hms;
                _internal_::extract_date_time(args[0], ymd, hms);
                return (uint8_t)(unsigned int)hms.hours().count();
            });

            AttachAFun(minute, 1, {
                std::chrono::year_month_day ymd;
                std::chrono::hh_mm_ss<std::chrono::microseconds> hms;
                _internal_::extract_date_time(args[0], ymd, hms);
                return (uint8_t)(unsigned int)hms.minutes().count();
            });

            AttachAFun(second, 1, {
                std::chrono::year_month_day ymd;
                std::chrono::hh_mm_ss<std::chrono::microseconds> hms;
                _internal_::extract_date_time(args[0], ymd, hms);
                return (uint8_t)(unsigned int)hms.seconds().count();
            });

            AttachAFun(millisecond, 1, {
                std::chrono::year_month_day ymd;
                std::chrono::hh_mm_ss<std::chrono::microseconds> hms;
                _internal_::extract_date_time(args[0], ymd, hms);
                return (uint16_t)(unsigned int)hms.subseconds().count() / 1000;
            });

            AttachAFun(microsecond, 1, {
                std::chrono::year_month_day ymd;
                std::chrono::hh_mm_ss<std::chrono::microseconds> hms;
                _internal_::extract_date_time(args[0], ymd, hms);
                return (uint16_t)(unsigned int)hms.subseconds().count();
            });
        }

        namespace set {
            //TODO: optimize
            AttachAFun(year, 2, {
                std::chrono::year_month_day ymd;
                _internal_::extract_date(args[0], ymd);
                int actual = (int)ymd.year();
                int set_year = (int)args[1];
                int year_offset = actual - set_year;
                auto time_point = (std::chrono::high_resolution_clock::time_point)args[0];
                time_point += std::chrono::years(year_offset);
                return time_point;
            });

            AttachAFun(month, 2, {
                std::chrono::year_month_day ymd;
                _internal_::extract_date(args[0], ymd);
                unsigned int actual = (unsigned int)ymd.month();
                int set_month = (int)args[1];
                int month_offset = int(actual) - set_month;
                auto time_point = (std::chrono::high_resolution_clock::time_point)args[0];
                time_point += std::chrono::months(month_offset);
                return time_point;
            });

            AttachAFun(week, 2, {
                std::chrono::year_month_day ymd;
                _internal_::extract_date(args[0], ymd);
                unsigned int actual = (unsigned int)ymd.day() / 7;
                int set_week = (int)args[1];
                int week_offset = int(actual) - set_week;
                auto time_point = (std::chrono::high_resolution_clock::time_point)args[0];
                time_point += std::chrono::weeks(week_offset);
                return time_point;
            });

            AttachAFun(day, 2, {
                std::chrono::year_month_day ymd;
                _internal_::extract_date(args[0], ymd);
                unsigned int actual = (unsigned int)ymd.day();
                int set_day = (int)args[1];
                int day_offset = int(actual) - set_day;
                auto time_point = (std::chrono::high_resolution_clock::time_point)args[0];
                time_point += std::chrono::days(day_offset);
                return time_point;
            });

            AttachAFun(hour, 2, {
                std::chrono::year_month_day ymd;
                std::chrono::hh_mm_ss<std::chrono::microseconds> hms;
                _internal_::extract_date_time(args[0], ymd, hms);
                unsigned int actual = (unsigned int)hms.hours().count();
                int set_hour = (int)args[1];
                int hour_offset = int(actual) - set_hour;
                auto time_point = (std::chrono::high_resolution_clock::time_point)args[0];
                time_point += std::chrono::hours(hour_offset);
                return time_point;
            });

            AttachAFun(minute, 2, {
                std::chrono::year_month_day ymd;
                std::chrono::hh_mm_ss<std::chrono::microseconds> hms;
                _internal_::extract_date_time(args[0], ymd, hms);
                unsigned int actual = (unsigned int)hms.minutes().count();
                int set_minute = (int)args[1];
                int minute_offset = int(actual) - set_minute;
                auto time_point = (std::chrono::high_resolution_clock::time_point)args[0];
                time_point += std::chrono::minutes(minute_offset);
                return time_point;
            });

            AttachAFun(second, 2, {
                std::chrono::year_month_day ymd;
                std::chrono::hh_mm_ss<std::chrono::microseconds> hms;
                _internal_::extract_date_time(args[0], ymd, hms);
                unsigned int actual = (unsigned int)hms.seconds().count();
                int set_second = (int)args[1];
                int second_offset = int(actual) - set_second;
                auto time_point = (std::chrono::high_resolution_clock::time_point)args[0];
                time_point += std::chrono::seconds(second_offset);
                return time_point;
            });

            AttachAFun(millisecond, 2, {
                std::chrono::year_month_day ymd;
                std::chrono::hh_mm_ss<std::chrono::microseconds> hms;
                _internal_::extract_date_time(args[0], ymd, hms);
                unsigned int actual = (unsigned int)hms.subseconds().count() / 1000;
                int set_millisecond = (int)args[1];
                int millisecond_offset = int(actual) - set_millisecond;
                auto time_point = (std::chrono::high_resolution_clock::time_point)args[0];
                time_point += std::chrono::milliseconds(millisecond_offset);
                return time_point;
            });

            AttachAFun(microsecond, 2, {
                std::chrono::year_month_day ymd;
                std::chrono::hh_mm_ss<std::chrono::microseconds> hms;
                _internal_::extract_date_time(args[0], ymd, hms);
                unsigned int actual = (unsigned int)hms.subseconds().count();
                int set_microsecond = (int)args[1];
                int microsecond_offset = int(actual) - set_microsecond;
                auto time_point = (std::chrono::high_resolution_clock::time_point)args[0];
                time_point += std::chrono::microseconds(microsecond_offset);
                return time_point;
            });
        }

        namespace add {
            AttachAFun(year, 2, {
                auto time_point = (std::chrono::high_resolution_clock::time_point)args[0];
                time_point += std::chrono::years((int)args[1]);
                return time_point;
            });

            AttachAFun(month, 2, {
                auto time_point = (std::chrono::high_resolution_clock::time_point)args[0];
                time_point += std::chrono::months((int)args[1]);
                return time_point;
            });

            AttachAFun(week, 2, {
                auto time_point = (std::chrono::high_resolution_clock::time_point)args[0];
                time_point += std::chrono::weeks((int)args[1]);
                return time_point;
            });

            AttachAFun(day, 2, {
                auto time_point = (std::chrono::high_resolution_clock::time_point)args[0];
                time_point += std::chrono::days((int)args[1]);
                return time_point;
            });

            AttachAFun(hour, 2, {
                auto time_point = (std::chrono::high_resolution_clock::time_point)args[0];
                time_point += std::chrono::hours((int)args[1]);
                return time_point;
            });

            AttachAFun(minute, 2, {
                auto time_point = (std::chrono::high_resolution_clock::time_point)args[0];
                time_point += std::chrono::minutes((int)args[1]);
                return time_point;
            });

            AttachAFun(second, 2, {
                auto time_point = (std::chrono::high_resolution_clock::time_point)args[0];
                time_point += std::chrono::seconds((int)args[1]);
                return time_point;
            });

            AttachAFun(millisecond, 2, {
                auto time_point = (std::chrono::high_resolution_clock::time_point)args[0];
                time_point += std::chrono::milliseconds((int)args[1]);
                return time_point;
            });

            AttachAFun(microsecond, 2, {
                auto time_point = (std::chrono::high_resolution_clock::time_point)args[0];
                time_point += std::chrono::microseconds((int)args[1]);
                return time_point;
            });
        }

        namespace subtract {
            AttachAFun(year, 2, {
                auto time_point = (std::chrono::high_resolution_clock::time_point)args[0];
                time_point -= std::chrono::years((int)args[1]);
                return time_point;
            });

            AttachAFun(month, 2, {
                auto time_point = (std::chrono::high_resolution_clock::time_point)args[0];
                time_point -= std::chrono::months((int)args[1]);
                return time_point;
            });

            AttachAFun(week, 2, {
                auto time_point = (std::chrono::high_resolution_clock::time_point)args[0];
                time_point -= std::chrono::weeks((int)args[1]);
                return time_point;
            });

            AttachAFun(day, 2, {
                auto time_point = (std::chrono::high_resolution_clock::time_point)args[0];
                time_point -= std::chrono::days((int)args[1]);
                return time_point;
            });

            AttachAFun(hour, 2, {
                auto time_point = (std::chrono::high_resolution_clock::time_point)args[0];
                time_point -= std::chrono::hours((int)args[1]);
                return time_point;
            });

            AttachAFun(minute, 2, {
                auto time_point = (std::chrono::high_resolution_clock::time_point)args[0];
                time_point -= std::chrono::minutes((int)args[1]);
                return time_point;
            });

            AttachAFun(second, 2, {
                auto time_point = (std::chrono::high_resolution_clock::time_point)args[0];
                time_point -= std::chrono::seconds((int)args[1]);
                return time_point;
            });

            AttachAFun(millisecond, 2, {
                auto time_point = (std::chrono::high_resolution_clock::time_point)args[0];
                time_point -= std::chrono::milliseconds((int)args[1]);
                return time_point;
            });

            AttachAFun(microsecond, 2, {
                auto time_point = (std::chrono::high_resolution_clock::time_point)args[0];
                time_point -= std::chrono::microseconds((int)args[1]);
                return time_point;
            });
        }

        namespace compare {
            AttachAFun(year, 2, {
                CXX::excepted(args[0], VType::time_point);
                if (is_integer(args[1].meta.vtype)) {
                    std::chrono::year_month_day ymd;
                    _internal_::extract_date(args[0], ymd);
                    return (uint8_t)(int)std::clamp((int)ymd.year() - (int)args[1], -1, 1);
                } else if (args[1].meta.vtype == VType::time_point) {
                    std::chrono::year_month_day ymd1;
                    std::chrono::year_month_day ymd2;
                    _internal_::extract_date(args[0], ymd1);
                    _internal_::extract_date(args[1], ymd2);
                    return (uint8_t)(int)std::clamp((int)ymd1.year() - (int)ymd2.year(), -1, 1);
                } else
                    CXX::excepted(args[0], VType::i32);
            });

            AttachAFun(year_week, 2, {
                CXX::excepted(args[0], VType::time_point);
                if (is_integer(args[1].meta.vtype)) {
                    std::chrono::year_month_day ymd;
                    _internal_::extract_date(args[0], ymd);
                    return (uint8_t)(int)std::clamp((int)(_days_elapsed_from_start(ymd) / 7) - (int)args[1], -1, 1);
                } else if (args[1].meta.vtype == VType::time_point) {
                    std::chrono::year_month_day ymd1;
                    std::chrono::year_month_day ymd2;
                    _internal_::extract_date(args[0], ymd1);
                    _internal_::extract_date(args[1], ymd2);
                    return (uint8_t)(int)std::clamp((int)(_days_elapsed_from_start(ymd1) / 7) - (int)(_days_elapsed_from_start(ymd2) / 7), -1, 1);
                } else
                    CXX::excepted(args[0], VType::i32);
            });

            AttachAFun(year_day, 2, {
                CXX::excepted(args[0], VType::time_point);
                if (is_integer(args[1].meta.vtype)) {
                    std::chrono::year_month_day ymd;
                    _internal_::extract_date(args[0], ymd);
                    return (uint8_t)(int)std::clamp((int)_days_elapsed_from_start(ymd) - (int)args[1], -1, 1);
                } else if (args[1].meta.vtype == VType::time_point) {
                    std::chrono::year_month_day ymd1;
                    std::chrono::year_month_day ymd2;
                    _internal_::extract_date(args[0], ymd1);
                    _internal_::extract_date(args[1], ymd2);
                    return (uint8_t)(int)std::clamp((int)_days_elapsed_from_start(ymd1) - (int)_days_elapsed_from_start(ymd2), -1, 1);
                } else
                    CXX::excepted(args[0], VType::i32);
            });

            AttachAFun(month, 2, {
                CXX::excepted(args[0], VType::time_point);
                if (is_integer(args[1].meta.vtype)) {
                    std::chrono::year_month_day ymd;
                    _internal_::extract_date(args[0], ymd);
                    return (uint8_t)(int)std::clamp((int)(unsigned int)ymd.month() - (int)args[1], -1, 1);
                } else if (args[1].meta.vtype == VType::time_point) {
                    std::chrono::year_month_day ymd1;
                    std::chrono::year_month_day ymd2;
                    _internal_::extract_date(args[0], ymd1);
                    _internal_::extract_date(args[1], ymd2);
                    return (uint8_t)(int)std::clamp((int)(unsigned int)ymd1.month() - (int)(unsigned int)ymd2.month(), -1, 1);
                } else
                    CXX::excepted(args[0], VType::i32);
            });

            AttachAFun(month_week, 2, {
                CXX::excepted(args[0], VType::time_point);
                if (is_integer(args[1].meta.vtype)) {
                    std::chrono::year_month_weekday ymw;
                    _internal_::extract_date(args[0], ymw);
                    return (uint8_t)(int)std::clamp((int)(uint8_t)ymw.weekday().iso_encoding() - (int)args[1], -1, 1);
                } else if (args[1].meta.vtype == VType::time_point) {
                    std::chrono::year_month_weekday ymw1;
                    std::chrono::year_month_weekday ymw2;
                    _internal_::extract_date(args[0], ymw1);
                    _internal_::extract_date(args[1], ymw2);
                    return (uint8_t)(int)std::clamp((int)(uint8_t)ymw1.weekday().iso_encoding() - (int)(uint8_t)ymw2.weekday().iso_encoding(), -1, 1);
                } else
                    CXX::excepted(args[0], VType::i32);
            });

            AttachAFun(week, 2, {
                std::chrono::year_month_day ymd;
                _internal_::extract_date(args[0], ymd);
                return (uint8_t)(int)std::clamp<int>((unsigned int)ymd.day() / 7 - (int)args[1], -1, 1);
            });

            AttachAFun(week_day, 2, {
                std::chrono::year_month_weekday ymw;
                _internal_::extract_date(args[0], ymw);
                return (uint8_t)ymw.weekday().iso_encoding();
            });

            AttachAFun(day, 2, {
                std::chrono::year_month_day ymd;
                _internal_::extract_date(args[0], ymd);
                return (uint8_t)(int)std::clamp<int>((unsigned int)ymd.day() - (int)args[1], -1, 1);
            });

            AttachAFun(hour, 2, {
                std::chrono::year_month_day ymd;
                std::chrono::hh_mm_ss<std::chrono::microseconds> hms;
                _internal_::extract_date_time(args[0], ymd, hms);
                return (uint8_t)(int)std::clamp<int>((unsigned int)hms.hours().count() - (int)args[1], -1, 1);
            });

            AttachAFun(minute, 2, {
                std::chrono::year_month_day ymd;
                std::chrono::hh_mm_ss<std::chrono::microseconds> hms;
                _internal_::extract_date_time(args[0], ymd, hms);
                return (uint8_t)(int)std::clamp<int>((unsigned int)hms.minutes().count() - (int)args[1], -1, 1);
            });

            AttachAFun(second, 2, {
                std::chrono::year_month_day ymd;
                std::chrono::hh_mm_ss<std::chrono::microseconds> hms;
                _internal_::extract_date_time(args[0], ymd, hms);
                return (uint8_t)(int)std::clamp<int>((unsigned int)hms.seconds().count() - (int)args[1], -1, 1);
            });

            AttachAFun(millisecond, 2, {
                std::chrono::year_month_day ymd;
                std::chrono::hh_mm_ss<std::chrono::microseconds> hms;
                _internal_::extract_date_time(args[0], ymd, hms);
                return (uint16_t)(int)std::clamp<int>((unsigned int)hms.subseconds().count() / 1000 - (int)args[1], -1, 1);
            });

            AttachAFun(microsecond, 2, {
                std::chrono::year_month_day ymd;
                std::chrono::hh_mm_ss<std::chrono::microseconds> hms;
                _internal_::extract_date_time(args[0], ymd, hms);
                return (uint16_t)(int)std::clamp<int>((unsigned int)hms.subseconds().count() - (int)args[1], -1, 1);
            });
        }

        AttachAFun(to_string, 1, {
            std::chrono::year_month_day ymd;
            std::chrono::hh_mm_ss<std::chrono::microseconds> hms;
            _internal_::extract_date_time(args[0], ymd, hms);
            art::ustring str =
                std::to_string((int)ymd.year()) +
                "-" +
                std::to_string((unsigned int)ymd.month()) +
                "-" +
                std::to_string((unsigned int)ymd.day()) +
                " " +
                std::to_string((int)hms.hours().count()) +
                ":" +
                std::to_string((int)hms.minutes().count()) +
                ":" +
                std::to_string((int)hms.seconds().count()) +
                "." +
                std::to_string((int)hms.subseconds().count());
            return str;
        });

        namespace utils {
            AttachAFun(days_in_month, 2, {
                return _internal_::days_in_month((uint8_t)args[0], _internal_::is_leap((uint16_t)args[1]));
            });

            AttachAFun(is_leap, 1, {
                return _internal_::is_leap((uint16_t)args[0]);
            });

            AttachAFun(days_in_year, 1, {
                return _internal_::days_in_year((uint16_t)args[0]);
            });

            AttachAFun(current_timezone_offset, 0, {
                return _internal_::current_timezone_offset().count();
            });

            AttachAFun(current_timezone_name, 0, {
                return _internal_::current_timezone_name();
            });

            AttachAFun(as_utc, 1, {
                auto time_point = (std::chrono::high_resolution_clock::time_point)args[0];
                return _internal_::as_utc(time_point);
            });

            AttachAFun(as_local, 1, {
                auto time_point = (std::chrono::high_resolution_clock::time_point)args[0];
                return _internal_::as_local(time_point);
            });
        }

        namespace _internal_ {
            void extract_date_time(ValueItem& date_time, std::chrono::year_month_day& ymd, std::chrono::hh_mm_ss<std::chrono::microseconds>& hms) {
                auto time_point = (std::chrono::high_resolution_clock::time_point)date_time;
                auto system = std::chrono::system_clock::time_point(std::chrono::duration_cast<std::chrono::system_clock::duration>(time_point.time_since_epoch()));
                auto days_point = std::chrono::floor<std::chrono::days>(system);
                ymd = std::chrono::year_month_day{days_point};
                hms = std::chrono::hh_mm_ss{std::chrono::floor<std::chrono::microseconds>(system - days_point)};
            }

            void extract_date(ValueItem& date_time, std::chrono::year_month_day& ymd) {
                auto time_point = (std::chrono::high_resolution_clock::time_point)date_time;
                auto system = std::chrono::system_clock::time_point(std::chrono::duration_cast<std::chrono::system_clock::duration>(time_point.time_since_epoch()));
                auto days_point = std::chrono::floor<std::chrono::days>(system);
                ymd = std::chrono::year_month_day{days_point};
            }

            void extract_date(ValueItem& date_time, std::chrono::year_month_weekday& ymw) {
                auto time_point = (std::chrono::high_resolution_clock::time_point)date_time;
                auto system = std::chrono::system_clock::time_point(std::chrono::duration_cast<std::chrono::system_clock::duration>(time_point.time_since_epoch()));
                auto days_point = std::chrono::floor<std::chrono::days>(system);
                ymw = std::chrono::year_month_weekday{days_point};
            }

            uint8_t days_in_month(uint8_t month, bool leap) {
                if (month == 0 || month > 12)
                    throw InvalidArguments("Invalid month, excepted 1 to 12, but got: " + std::to_string(month));
                uint8_t days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
                if (leap && month == 2)
                    return 29;
                else
                    return days[month - 1];
            }

            unsigned day_of_week(unsigned year, unsigned month, unsigned day) {
                unsigned a, y, m;
                a = (14 - month) / 12;
                y = year - a;
                m = month + (12 * a) - 2;
                // Gregorian:
                return (day + y + (y / 4) - (y / 100) + (y / 400) + ((31 * m) / 12)) % 7;
            }

            bool is_leap(uint16_t year) {
                return (year % 4 == 0 && year % 100 != 0) || year % 400 == 0;
            }

            uint16_t days_in_year(uint16_t year) {
                return is_leap(year) ? 366 : 365;
            }

            std::chrono::seconds current_timezone_offset() {
                std::chrono::sys_time time = std::chrono::floor<std::chrono::seconds>(std::chrono::system_clock::now());
                std::chrono::seconds off = std::chrono::current_zone()->get_info(time).offset;
                return off;
            }

            art::ustring current_timezone_name() {
                return (std::string)std::chrono::current_zone()->name();
            }

            std::chrono::high_resolution_clock::time_point as_utc(std::chrono::high_resolution_clock::time_point ymd) {
                return ymd - current_timezone_offset();
            }

            std::chrono::high_resolution_clock::time_point as_local(std::chrono::high_resolution_clock::time_point ymw) {
                return ymw + current_timezone_offset();
            }
        }
    }
}