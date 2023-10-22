#include <attacha_run_time.hpp>
#include <gtest/gtest.h>
using namespace art;

const constexpr int C_ONE = 1;
const constexpr int C_TWO = 2;
const constexpr int C_THREE = 3;
const constexpr double C_FOUR = 4.4;


void test_arguments_passing(int one, int two, int three, double five) {
    ASSERT_EQ(one, C_ONE);
    ASSERT_EQ(two, C_TWO);
    ASSERT_EQ(three, C_THREE);
    ASSERT_EQ(five, C_FOUR);
}

TEST(CXX_BRIDGE, native_call) {
    test_arguments_passing(C_ONE, C_TWO, C_THREE, C_FOUR);
}

TEST(CXX_BRIDGE, self_bridge_call) {
    auto test_native = CXX::MakeNative(test_arguments_passing, false, false);
    CXX::cxxCall(test_native, C_ONE, C_TWO, C_THREE, C_FOUR);
}

TEST(CXX_BRIDGE, self_bridge_call_simple_lambda) {
    auto test_native = CXX::MakeNative(
        [](ValueItem* args, uint32_t argc) {
            int one = (int)args[0];
            int two = (int)args[1];
            int three = (int)args[2];
            double five = (double)args[3];
            ASSERT_EQ(one, C_ONE);
            ASSERT_EQ(two, C_TWO);
            ASSERT_EQ(three, C_THREE);
            ASSERT_EQ(five, C_FOUR);
        },
        false,
        false
    );
    CXX::cxxCall(test_native, C_ONE, C_TWO, C_THREE, C_FOUR);
}

TEST(CXX_BRIDGE, self_bridge_call_capturing_lambda) {
    int capture_one = C_ONE;
    int capture_two = C_TWO;
    auto test_native = CXX::MakeNative(
        [capture_one, &capture_two](ValueItem* args, uint32_t argc) {
            int one = (int)args[0];
            int two = (int)args[1];
            int three = (int)args[2];
            double five = (double)args[3];
            ASSERT_EQ(one, C_ONE);
            ASSERT_EQ(two, C_TWO);
            ASSERT_EQ(three, C_THREE);
            ASSERT_EQ(five, C_FOUR);

            ASSERT_EQ(capture_one, C_ONE);
            ASSERT_EQ(capture_two, C_TWO);
        },
        false,
        false
    );
    CXX::cxxCall(test_native, C_ONE, C_TWO, C_THREE, C_FOUR);
}

AttachAFunc(native_test_arguments_passing, 4) {
    int one = (int)args[0];
    int two = (int)args[1];
    int three = (int)args[2];
    double five = (double)args[3];
    [&]() {
        ASSERT_EQ(one, C_ONE);
        ASSERT_EQ(two, C_TWO);
        ASSERT_EQ(three, C_THREE);
        ASSERT_EQ(five, C_FOUR);
    }();
    return nullptr;
}

TEST(CXX_BRIDGE, native_bridge_call) {
    CXX::cxxCall(native_test_arguments_passing, C_ONE, C_TWO, C_THREE, C_FOUR);
}