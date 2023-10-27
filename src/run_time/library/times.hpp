#ifndef SRC_RUN_TIME_LIBRARY_TIMES
#define SRC_RUN_TIME_LIBRARY_TIMES
#include <chrono>

#include <run_time/attacha_abi_structs.hpp>

namespace art {
    namespace times {
        ValueItem* now(ValueItem*, uint32_t);
        ValueItem* now_utc(ValueItem*, uint32_t);
        ValueItem* to_string(ValueItem*, uint32_t);

        namespace get {
            ValueItem* year(ValueItem*, uint32_t);
            ValueItem* year_week(ValueItem*, uint32_t);
            ValueItem* year_day(ValueItem*, uint32_t);
            ValueItem* month(ValueItem*, uint32_t);
            ValueItem* month_week(ValueItem*, uint32_t);
            ValueItem* week_day(ValueItem*, uint32_t);
            ValueItem* day(ValueItem*, uint32_t);
            ValueItem* hour(ValueItem*, uint32_t);
            ValueItem* minute(ValueItem*, uint32_t);
            ValueItem* second(ValueItem*, uint32_t);
            ValueItem* millisecond(ValueItem*, uint32_t);
            ValueItem* microsecond(ValueItem*, uint32_t);
        }

        namespace set {
            ValueItem* year(ValueItem*, uint32_t);
            ValueItem* month(ValueItem*, uint32_t);
            ValueItem* week(ValueItem*, uint32_t);
            ValueItem* day(ValueItem*, uint32_t);
            ValueItem* hour(ValueItem*, uint32_t);
            ValueItem* minute(ValueItem*, uint32_t);
            ValueItem* second(ValueItem*, uint32_t);
            ValueItem* millisecond(ValueItem*, uint32_t);
            ValueItem* microsecond(ValueItem*, uint32_t);
        }

        namespace add {
            ValueItem* year(ValueItem*, uint32_t);
            ValueItem* month(ValueItem*, uint32_t);
            ValueItem* week(ValueItem*, uint32_t);
            ValueItem* day(ValueItem*, uint32_t);
            ValueItem* hour(ValueItem*, uint32_t);
            ValueItem* minute(ValueItem*, uint32_t);
            ValueItem* second(ValueItem*, uint32_t);
            ValueItem* millisecond(ValueItem*, uint32_t);
            ValueItem* microsecond(ValueItem*, uint32_t);
        }

        namespace subtract {
            ValueItem* year(ValueItem*, uint32_t);
            ValueItem* month(ValueItem*, uint32_t);
            ValueItem* week(ValueItem*, uint32_t);
            ValueItem* day(ValueItem*, uint32_t);
            ValueItem* hour(ValueItem*, uint32_t);
            ValueItem* minute(ValueItem*, uint32_t);
            ValueItem* second(ValueItem*, uint32_t);
            ValueItem* millisecond(ValueItem*, uint32_t);
            ValueItem* microsecond(ValueItem*, uint32_t);
        }

        namespace compare {
            ValueItem* year(ValueItem*, uint32_t);
            ValueItem* year_week(ValueItem*, uint32_t);
            ValueItem* year_day(ValueItem*, uint32_t);
            ValueItem* month(ValueItem*, uint32_t);
            ValueItem* month_week(ValueItem*, uint32_t);
            ValueItem* week_day(ValueItem*, uint32_t);
            ValueItem* day(ValueItem*, uint32_t);
            ValueItem* hour(ValueItem*, uint32_t);
            ValueItem* minute(ValueItem*, uint32_t);
            ValueItem* second(ValueItem*, uint32_t);
            ValueItem* millisecond(ValueItem*, uint32_t);
            ValueItem* microsecond(ValueItem*, uint32_t);
        }

        namespace utils {
            ValueItem* days_in_month(ValueItem*, uint32_t);
            ValueItem* is_leap(ValueItem*, uint32_t);
            ValueItem* days_in_year(ValueItem*, uint32_t);

            //returns in seconds
            ValueItem* current_timezone_offset(ValueItem*, uint32_t);
            ValueItem* current_timezone_name(ValueItem*, uint32_t);

            ValueItem* as_utc(ValueItem*, uint32_t);
            ValueItem* as_local(ValueItem*, uint32_t);
        }

        namespace _internal_ {
            //TODO implement custom calendars

            void extract_date_time(ValueItem& date_time, std::chrono::year_month_day& ymd, std::chrono::hh_mm_ss<std::chrono::microseconds>& hms);
            void extract_date(ValueItem& date_time, std::chrono::year_month_day& ymd);
            void extract_date(ValueItem& date_time, std::chrono::year_month_weekday& ymw);

            uint8_t days_in_month(uint8_t month, bool leap);
            bool is_leap(uint16_t year);
            uint16_t days_in_year(uint16_t year);

            std::chrono::seconds current_timezone_offset();
            art::ustring current_timezone_name();

            std::chrono::high_resolution_clock::time_point as_utc(std::chrono::high_resolution_clock::time_point ymd);
            std::chrono::high_resolution_clock::time_point as_local(std::chrono::high_resolution_clock::time_point ymw);
        }
    }
}
#endif /* SRC_RUN_TIME_LIBRARY_TIMES */
