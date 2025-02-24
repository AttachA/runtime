#include <attacha/run_time.hpp>
#include <base/run_time.hpp>
#include <gtest/gtest.h>

int main(int argc, char** argv) {
    art::Task::create_executor();
    art::initRuntime();
    ::testing::InitGoogleTest(&argc, argv);
    int result = RUN_ALL_TESTS();
    art::deinitRuntime();
    return result;
}
