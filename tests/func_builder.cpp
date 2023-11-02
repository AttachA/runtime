#include <attacha/run_time.hpp>

#include <gtest/gtest.h>

using namespace art;

TEST(FuncEnviroBuilder, static_test) {
    {
        FuncEnviroBuilder builder;
        auto noting = builder.create_constant(nullptr);
        auto one = builder.create_constant(1);
        auto num = builder.create_constant((uint32_t)0);
        builder.compare(noting, 0_sta);
        builder.jump(JumpCondition::is_not_equal, "not_inited");
        builder.copy_un_constant(0_sta, num);
        builder.bind_pos("not_inited");
        builder.sum(0_sta, one);
        builder.ret(0_sta);
        builder.O_load_func("static_test");
    }
    EXPECT_EQ(CXX::cxxCall("static_test"), 1);
    EXPECT_EQ(CXX::cxxCall("static_test"), 2);
    EXPECT_EQ(CXX::cxxCall("static_test"), 3);
    FuncEnvironment::Unload("static_test");
}

TEST(FuncEnviroBuilder, table_jump) {
    {
        FuncEnviroBuilder builder;
        builder.table_jump({"0", "1", "2", "3"},
                           0_arg,
                           true,
                           TableJumpCheckFailAction::jump_specified,
                           "too_big",
                           TableJumpCheckFailAction::jump_specified,
                           "too_small");
        auto num7 = builder.create_constant(7);
        auto num6 = builder.create_constant(6);
        auto num5 = builder.create_constant(5);
        auto num10 = builder.create_constant(10);
        auto num30 = builder.create_constant(30);
        builder.bind_pos("0");
        builder.ret();
        builder.bind_pos("1");
        builder.ret(num7);
        builder.bind_pos("2");
        builder.ret(num6);
        builder.bind_pos("3");
        builder.ret(num5);
        builder.bind_pos("too_big");
        builder.ret(num10);
        builder.bind_pos("too_small");
        builder.ret(num30);
        builder.O_load_func("table_jump_test");
    }
    EXPECT_EQ(CXX::cxxCall("table_jump_test", 0), nullptr);
    EXPECT_EQ(CXX::cxxCall("table_jump_test", 1), 7);
    EXPECT_EQ(CXX::cxxCall("table_jump_test", 2), 6);
    EXPECT_EQ(CXX::cxxCall("table_jump_test", 3), 5);
    EXPECT_EQ(CXX::cxxCall("table_jump_test", 4), 10);
    EXPECT_EQ(CXX::cxxCall("table_jump_test", 50), 10);
    EXPECT_EQ(CXX::cxxCall("table_jump_test", -1), 30);
    EXPECT_EQ(CXX::cxxCall("table_jump_test", -100), 30);
    FuncEnvironment::Unload("table_jump_test");
}

TEST(FuncEnviroBuilder, table_jump_2) {
    {
        FuncEnviroBuilder builder;
        builder.table_jump({"0", "1", "2", "3"},
                           0_arg,
                           true,
                           TableJumpCheckFailAction::jump_specified,
                           "default",
                           TableJumpCheckFailAction::jump_specified,
                           "default");
        auto num7 = builder.create_constant(7);
        auto num6 = builder.create_constant(6);
        auto num5 = builder.create_constant(5);
        auto num10 = builder.create_constant(10);
        builder.bind_pos("0");
        builder.ret();
        builder.bind_pos("1");
        builder.ret(num7);
        builder.bind_pos("2");
        builder.ret(num6);
        builder.bind_pos("3");
        builder.ret(num5);
        builder.bind_pos("default");
        builder.ret(num10);
        builder.O_load_func("table_jump_test");
    }
    EXPECT_EQ(CXX::cxxCall("table_jump_test", 0), nullptr);
    EXPECT_EQ(CXX::cxxCall("table_jump_test", 1), 7);
    EXPECT_EQ(CXX::cxxCall("table_jump_test", 2), 6);
    EXPECT_EQ(CXX::cxxCall("table_jump_test", 3), 5);
    EXPECT_EQ(CXX::cxxCall("table_jump_test", 4), 10);
    EXPECT_EQ(CXX::cxxCall("table_jump_test", 50), 10);
    EXPECT_EQ(CXX::cxxCall("table_jump_test", -1), 10);
    EXPECT_EQ(CXX::cxxCall("table_jump_test", -100), 10);
    FuncEnvironment::Unload("table_jump_test");
}
