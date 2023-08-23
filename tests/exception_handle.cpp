#include <attacha_run_time.hpp>
#include <gtest/gtest.h>
using namespace art;


void exceptionThrower(){
    throw 123;
}

TEST(EXCEPTION, tunnel){
    FuncEnvironment::AddNative(exceptionThrower, "exceptionThrower");
    {
        FuncEnviroBuilder builder;
        builder.call_and_ret(builder.create_constant("exceptionThrower"));
        builder.O_load_func("exceptionTunnel");
    }
    try{
        CXX::cxxCall("exceptionTunnel");
        ASSERT_TRUE(false);
    }catch(int e){
        ASSERT_EQ(e, 123);
    }
    FuncEnvironment::Unload("exceptionThrower");
    FuncEnvironment::Unload("exceptionTunnel");
}