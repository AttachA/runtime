#include <attacha/run_time.hpp>
#include <gtest/gtest.h>
#include <util/threading.hpp>
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
            ASSERT_EQ(argc, 4u);
            ASSERT_EQ(args[0].meta.vtype, VType::i32);
            ASSERT_EQ(args[1].meta.vtype, VType::i32);
            ASSERT_EQ(args[2].meta.vtype, VType::i32);
            ASSERT_EQ(args[3].meta.vtype, VType::doub);

            int one = (int)args[0];
            int two = (int)args[1];
            int three = (int)args[2];
            double five = (double)args[3];
            ASSERT_EQ(one, C_ONE);
            ASSERT_EQ(two, C_TWO);
            ASSERT_EQ(three, C_THREE);
            ASSERT_EQ(five, C_FOUR);

            ASSERT_EQ(capture_one, C_ONE);
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
        ASSERT_EQ(len, 4u);
        ASSERT_EQ(args[0].meta.vtype, VType::i32);
        ASSERT_EQ(args[1].meta.vtype, VType::i32);
        ASSERT_EQ(args[2].meta.vtype, VType::i32);
        ASSERT_EQ(args[3].meta.vtype, VType::doub);


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

struct SAMPLE_STRUCT {
    int a;
    int b;
    int c;
    double d;
};

void test_struct_passing(SAMPLE_STRUCT s) {
    ASSERT_EQ(s.a, C_ONE);
    ASSERT_EQ(s.b, C_TWO);
    ASSERT_EQ(s.c, C_THREE);
    ASSERT_EQ(s.d, C_FOUR);
}

TEST(CXX_BRIDGE, native_call_struct) {
    CXX::Interface::typeVTable<SAMPLE_STRUCT>() = CXX::Interface::createTable<SAMPLE_STRUCT>("SAMPLE_STRUCT");
    SAMPLE_STRUCT s;
    s.a = C_ONE;
    s.b = C_TWO;
    s.c = C_THREE;
    s.d = C_FOUR;
    CXX::cxxCall(CXX::MakeNative(test_struct_passing), s);
    CXX::Interface::typeVTable<SAMPLE_STRUCT>().unregister();
}

TEST(CXX_BRIDGE, _art_wait) {
    auto current_id = art::this_thread::get_id();
    ASSERT_FALSE(Task::is_task());
    art_wait {
        ASSERT_TRUE(Task::is_task());
        ASSERT_NE(current_id, art::this_thread::get_id());
    };
}

TEST(CXX_BRIDGE, _art_async_await) {
    auto current_id = art::this_thread::get_id();
    ASSERT_FALSE(Task::is_task());
    bool complete = false;
    auto task = art_async {
        ASSERT_TRUE(Task::is_task());
        ASSERT_NE(current_id, art::this_thread::get_id());
        Task::sleep(1);
        complete = true;
    };
    ASSERT_FALSE(complete);
    art_await task;
    ASSERT_TRUE(complete);
}