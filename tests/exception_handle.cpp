#include <attacha_run_time.hpp>
#include <gtest/gtest.h>


using namespace art;

void exceptionThrower() {
    throw 123;
}

TEST(EXCEPTION, tunnel) {
#ifndef PLATFORM_WINDOWS
    GTEST_SKIP_("Exception unwinding implemented only in windows");
#endif
    FuncEnvironment::AddNative(exceptionThrower, "exceptionThrower");
    {
        FuncEnviroBuilder builder;
        builder.call_and_ret(builder.create_constant("exceptionThrower"));
        builder.O_load_func("exceptionTunnel");
    }
    try {
        CXX::cxxCall("exceptionTunnel");
        FuncEnvironment::Unload("exceptionThrower");
        FuncEnvironment::Unload("exceptionTunnel");
        ASSERT_TRUE(false);
    } catch (int e) {
        FuncEnvironment::Unload("exceptionThrower");
        FuncEnvironment::Unload("exceptionTunnel");
        ASSERT_EQ(e, 123);
    }
}