#include <attacha_run_time.hpp>
#include <gtest/gtest.h>

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    int result = RUN_ALL_TESTS();
    art::Task::shutDown();
    return result;
}