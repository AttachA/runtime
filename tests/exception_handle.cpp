#include <attacha/internal_run_time.hpp>
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

namespace throw_test {
    bool ex_catch_destructed = false;
    int filters_touch_count = 0;
    int filters_join_count = 0;
    int thrower_destruct_count = 0;
    bool catched_ = false;

    class ExCatchTest : public std::exception {
    public:
        ExCatchTest() {
        }

        ~ExCatchTest() {
            ex_catch_destructed = true;
        }
    };

    void throw_test() {
        filters_join_count++;

        struct destruction_test {
            ~destruction_test() {
                thrower_destruct_count++;
            }
        } test;

        ExCatchTest value;
        throw value;
    }

    bool catch_test() {
        filters_touch_count++;
        if (art::exception::has_exception()) {
            filters_join_count++;
            return art::exception::map_native_exception_names(art::exception::lookup_meta())
                .contains("class throw_test::ExCatchTest");
        }
        return false;
    }

    void catched() {
        catched_ = true;
    }

    void test_except() {
        FuncEnvironment::AddNative(throw_test, "throw_test", true, false);
        FuncEnvironment::AddNative(catch_test, "catch_test", true, false);
        FuncEnvironment::AddNative(catched, "catched", true, false);
        FuncEnviroBuilder build;
        auto _throw_test = build.create_constant("Inner Call");
        auto _catch_test = build.create_constant("catch_test");
        auto _catched = build.create_constant("catched");
        build.except().handle_begin(0);
        build.call_and_ret(_throw_test);
        build.except().handle_catch_filter(0, _catch_test);
        build.except().handle_end(0);
        build.call_and_ret(_catched);


        FuncEnviroBuilder build_2;
        auto _throw_test_2 = build_2.create_constant("throw_test");
        build_2.copy(0_env, _throw_test_2);
        build_2.call_and_ret(0_env);
        build_2.O_load_func("Inner Call");


        ASSERT_NO_THROW(
            try {
                CXX::cxxCall(build.O_prepare_func());
            } catch (...) {
                FuncEnvironment::Unload("Inner Call");
                FuncEnvironment::Unload("throw_test");
                FuncEnvironment::Unload("catch_test");
                FuncEnvironment::Unload("catched");
                throw;
            }
        );
        FuncEnvironment::Unload("Inner Call");
        FuncEnvironment::Unload("throw_test");
        FuncEnvironment::Unload("catch_test");
        FuncEnvironment::Unload("catched");
        ASSERT_TRUE(ex_catch_destructed);
        ASSERT_EQ(filters_join_count, filters_join_count);
        ASSERT_EQ(thrower_destruct_count, 1);
        ASSERT_TRUE(catched_);
        ex_catch_destructed = false;
        filters_touch_count = 0;
        filters_join_count = 0;
        thrower_destruct_count = 0;
        catched_ = false;
    }
}

TEST(EXCEPTION, throw_test_normal) {
#ifndef PLATFORM_WINDOWS
    GTEST_SKIP_("Exception unwinding implemented only in windows");
#endif
    throw_test::test_except();
}

TEST(EXCEPTION, throw_test_task) {
#ifndef PLATFORM_WINDOWS
    GTEST_SKIP_("Exception unwinding implemented only in windows");
#endif
    throw_test::test_except();
}
