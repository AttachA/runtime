#include "../src/run_time/AttachA_CXX.hpp"
#include <gtest/gtest.h>
using namespace art;


void exceptionThrower(){
    throw 123;
}

TEST(EXCEPTION, tunel){
    FuncEnvironment::AddNative(exceptionThrower, "exceptionThrower");
    {
        FuncEnviroBuilder builder;
        builder.call_and_ret(builder.create_constant("exceptionThrower"));
        builder.O_load_func("exceptionTunel");
    }
    try{
        CXX::cxxCall("exceptionTunel");
        ASSERT_TRUE(false);
    }catch(int e){
        ASSERT_EQ(e, 123);
    }
}