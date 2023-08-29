#ifndef SRC_UTIL_MACRO
#define SRC_UTIL_MACRO
#
#define ART_STRINGIZE_delay(arg) #arg
#define ART_STRINGIZE(arg) ART_STRINGIZE_delay(arg)
#
#define ART_PARENS ()
#define ART_EVAL0(...) __VA_ARGS__
#define ART_EVAL1(...) ART_EVAL0(ART_EVAL0(ART_EVAL0(__VA_ARGS__)))
#define ART_EVAL2(...) ART_EVAL1(ART_EVAL1(ART_EVAL1(__VA_ARGS__)))
#define ART_EVAL3(...) ART_EVAL2(ART_EVAL2(ART_EVAL2(__VA_ARGS__)))
#define ART_EVAL4(...) ART_EVAL3(ART_EVAL3(ART_EVAL3(__VA_ARGS__)))
#define ART_EVAL(...) ART_EVAL4(ART_EVAL4(ART_EVAL4(__VA_ARGS__)))
#
#define ART_MAP(macro, ...) __VA_OPT__(ART_EVAL(ART_MAP_HELPER(macro, __VA_ARGS__)))
#define ART_MAP_HELPER(macro, a1, ...) \
    macro(a1)                          \
        __VA_OPT__(                    \
            ART_MAP_AGAIN              \
                ART_PARENS /**/ (      \
                    macro,             \
                    __VA_ARGS__        \
                )                      \
        )
#
#define ART_MAP_AGAIN() ART_MAP_HELPER
#
#
#define ART_JOIN(macro, arg1, ...) __VA_OPT__(ART_EVAL(ART_JOIN_HELPER(macro, arg1, __VA_ARGS__)))
#define ART_JOIN_HELPER(macro, arg1, a1, ...) \
    macro(arg1, a1)                           \
        __VA_OPT__(                           \
            ART_JOIN_AGAIN                    \
                ART_PARENS /**/ (             \
                    macro,                    \
                    arg1,                     \
                    __VA_ARGS__               \
                )                             \
        )
#
#define ART_JOIN_AGAIN() ART_JOIN_HELPER
#
#define ART_UNROLL_SEQUENCE(seq) ART_UNROLL_SEQUENCE_END(ART_UNROLL_SEQUENCE_A seq)
#define ART_UNROLL_SEQUENCE_BODY(x) x,
#define ART_UNROLL_SEQUENCE_A(x) ART_UNROLL_SEQUENCE_BODY(x) ART_UNROLL_SEQUENCE_B
#define ART_UNROLL_SEQUENCE_B(x) ART_UNROLL_SEQUENCE_BODY(x) ART_UNROLL_SEQUENCE_A
#define ART_UNROLL_SEQUENCE_A_END
#define ART_UNROLL_SEQUENCE_B_END
#define ART_UNROLL_SEQUENCE_END(...) ART_UNROLL_SEQUENCE_END_(__VA_ARGS__)
#define ART_UNROLL_SEQUENCE_END_(...) __VA_ARGS__##_END
#
#endif /* SRC_UTIL_MACRO */
