

enable_testing()
find_package(GTest CONFIG REQUIRED)
FILE(GLOB SRCFILES_TESTS
        *.cpp 
)
add_executable(AttachATests ${SRCFILES_TESTS})
set_property(TARGET AttachATests PROPERTY
  MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

target_link_libraries(AttachATests PRIVATE GTest::gtest GTest::gtest_main)
target_link_libraries(AttachATests PUBLIC AttachA_lib)


include(GoogleTest)
gtest_discover_tests(AttachATests)