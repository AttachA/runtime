#ifndef RUN_TIME_AGREEMENT_SYMBOLS
#define RUN_TIME_AGREEMENT_SYMBOLS





namespace symbols{
    namespace structures{
        constexpr const char* const size = "size";



        constexpr const char* const add_operator = "operator+=";
        constexpr const char* const subtract_operator = "operator-=";
        constexpr const char* const multiply_operator = "operator*=";
        constexpr const char* const divide_operator = "operator/=";
        constexpr const char* const modulo_operator = "operator%=";
        constexpr const char* const bitwise_and_operator = "operator&=";
        constexpr const char* const bitwise_or_operator = "operator|=";
        constexpr const char* const bitwise_xor_operator = "operator^=";
        constexpr const char* const bitwise_shift_left_operator = "operator<<=";
        constexpr const char* const bitwise_shift_right_operator = "operator>>=";
        constexpr const char* const adress_operator = "operator@=";
        constexpr const char* const symbol_operator = "operator$=";
        constexpr const char* const set_non_noting_operator = "operator?=";
        constexpr const char* const orctop_operator = "operator#=";



        constexpr const char* const not_equal_operator = "operator!=";
        constexpr const char* const equal_operator = "operator==";
        constexpr const char* const less_operator = "operator<";
        constexpr const char* const greater_operator = "operator>";
        constexpr const char* const less_or_equal_operator = "operator<=";
        constexpr const char* const greater_or_equal_operator = "operator>=";

        constexpr const char* const increment_operator = "operator++";
        constexpr const char* const decrement_operator = "operator--";

        constexpr const char* const not_operator = "operator!";
        constexpr const char* const bitwise_not_operator = "operator~";

        constexpr const char* const index_operator = "operator[]get";
        constexpr const char* const index_set_operator = "operator[]set";
        namespace _auto {
            constexpr bool enabled = true;
            constexpr const char* const copy_add_operator = "operator+";
            constexpr const char* const copy_subtract_operator = "operator-";
            constexpr const char* const copy_multiply_operator = "operator*";
            constexpr const char* const copy_divide_operator = "operator/";
            constexpr const char* const copy_modulo_operator = "operator%";
            constexpr const char* const copy_bitwise_and_operator = "operator&";
            constexpr const char* const copy_bitwise_or_operator = "operator|";
            constexpr const char* const copy_bitwise_xor_operator = "operator^";
            constexpr const char* const copy_bitwise_shift_left_operator = "operator<<";
            constexpr const char* const copy_bitwise_shift_right_operator = "operator>>";
            constexpr const char* const copy_adress_operator = "operator@";
            constexpr const char* const copy_symbol_operator = "operator$";
            constexpr const char* const copy_set_non_noting_operator = "operator?";
            constexpr const char* const copy_orctop_operator = "operator#";
        }
        namespace iterable{
            constexpr const char* const begin = "begin";
            constexpr const char* const end = "end";
            constexpr const char* const next = "next";
            constexpr const char* const prev = "prev";
            constexpr const char* const get = "get";
            constexpr const char* const set = "set";
        }
        namespace convert{
            constexpr const char* const to_string = "to_string";
            constexpr const char* const to_ui8 = "to_ui8";
            constexpr const char* const to_ui16 = "to_ui16";
            constexpr const char* const to_ui32 = "to_ui32";
            constexpr const char* const to_ui64 = "to_ui64";
            constexpr const char* const to_i8 = "to_i8";
            constexpr const char* const to_i16 = "to_i16";
            constexpr const char* const to_i32 = "to_i32";
            constexpr const char* const to_i64 = "to_i64";
            constexpr const char* const to_float = "to_float";
            constexpr const char* const to_double = "to_double";
            constexpr const char* const to_boolean = "to_boolean";
            constexpr const char* const to_timepoint = "to_timepoint";
            constexpr const char* const to_type_identifier = "to_type_identifier";
            constexpr const char* const to_function = "to_function";
            constexpr const char* const to_map = "to_map";
            constexpr const char* const to_set = "to_set";



            constexpr const char* const to_ui8_arr = "to_ui8[]";
            constexpr const char* const to_ui16_arr = "to_ui16[]";
            constexpr const char* const to_ui32_arr = "to_ui32[]";
            constexpr const char* const to_ui64_arr = "to_ui64[]";
            constexpr const char* const to_i8_arr = "to_i8[]";
            constexpr const char* const to_i16_arr = "to_i16[]";
            constexpr const char* const to_i32_arr = "to_i32[]";
            constexpr const char* const to_i64_arr = "to_i64[]";
            constexpr const char* const to_float_arr = "to_float[]";
            constexpr const char* const to_double_arr = "to_double[]";

            constexpr const char* const to_farr = "to_farr";
            constexpr const char* const to_uarr = "to_uarr";
        }
    }
}


#endif /* RUN_TIME_AGREEMENT_SYMBOLS */
