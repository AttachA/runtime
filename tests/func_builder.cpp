#include "../src/run_time/AttachA_CXX.hpp"
#include <gtest/gtest.h>
using namespace art;


TEST(FuncEviroBuilder, static_test){
    {
        FuncEviroBuilder builder;
        auto noting = builder.create_constant(nullptr);
        auto one = builder.create_constant(1);
        builder.compare(noting, 0_sta);
        builder.jump(JumpCondition::is_not_equal, "not_inited");
        builder.set_constant(0_sta, 0ui32);
        builder.remove_const_protect(0_sta);
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
TEST(FuncEviroBuilder, table_jump){
    {
        FuncEviroBuilder builder;
        builder.table_jump(
            { "0", "1", "2", "3" },
            0_arg,
            true,
            TableJumpCheckFailAction::jump_specified,
            "too_big",
            TableJumpCheckFailAction::jump_specified,
            "too_small"
        );
        builder.bind_pos("0");
        builder.ret();
        builder.bind_pos("1");
        builder.set_constant(0_env, ValueItem(7));
        builder.ret(0_env);
        builder.bind_pos("2");
        builder.set_constant(0_env, ValueItem(6));
        builder.ret(0_env);
        builder.bind_pos("3");
        builder.set_constant(0_env, ValueItem(5));
        builder.ret(0_env);
        builder.bind_pos("too_big");
        builder.set_constant(0_env, ValueItem(10));
        builder.ret(0_env);
        builder.bind_pos("too_small");
        builder.set_constant(0_env, ValueItem(30));
        builder.ret(0_env);
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
TEST(FuncEviroBuilder,table_jump_2){
    {
        FuncEviroBuilder builder;
        builder.table_jump(
            { "0", "1", "2", "3" },
            0_arg,
            true,
            TableJumpCheckFailAction::jump_specified,
            "default",
            TableJumpCheckFailAction::jump_specified,
            "default"
        );
        builder.bind_pos("0");
        builder.ret();
        builder.bind_pos("1");
        builder.set_constant(0_env, ValueItem(7));
        builder.ret(0_env);
        builder.bind_pos("2");
        builder.set_constant(0_env, ValueItem(6));
        builder.ret(0_env);
        builder.bind_pos("3");
        builder.set_constant(0_env, ValueItem(5));
        builder.ret(0_env);
        builder.bind_pos("default");
        builder.set_constant(0_env, ValueItem(10));
        builder.ret(0_env);
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