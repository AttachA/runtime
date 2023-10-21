#include <attacha_run_time.hpp>
#include <gtest/gtest.h>
using namespace art;

void test_arguments_passing(int one, int two, int three, double five) {
    ASSERT_EQ(one, 1);
    ASSERT_EQ(two, 2);
    ASSERT_EQ(three, 3);
    ASSERT_EQ(five, 4.0);
}

TEST(CXX_BRIDGE, native_call) {
    auto test_native = CXX::MakeNative(test_arguments_passing, false, false);
    CXX::cxxCall(test_native, 1, 2, 3, 4.0);
}

AttachAFunc(test_arguments_passing_, 4) {
    int one = (int)args[0];
    int two = (int)args[1];
    int three = (int)args[2];
    double five = (double)args[3];
    [&]() {
        ASSERT_EQ(one, 1);
        ASSERT_EQ(two, 2);
        ASSERT_EQ(three, 3);
        ASSERT_EQ(five, 4.0);
    }();
    return nullptr;
}

TEST(CXX_BRIDGE, real_call) {
    CXX::cxxCall(test_arguments_passing_, 1, 2, 3, 4.0);
}