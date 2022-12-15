cmake_minimum_required(VERSION 3.14)
project(AttachA CXX ASM_MASM)
set(CMAKE_CXX_STANDARD 23)
#enable_testing()
#add_subdirectory(tests)


if(MSVC)
  string(REPLACE "/MD" "/MT" CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})
  string(REPLACE "/MD" "/MT" CMAKE_CXX_FLAGS_DEBUG ${CMAKE_CXX_FLAGS_DEBUG})
  string(REPLACE "/MD" "/MT" CMAKE_CXX_FLAGS_RELEASE ${CMAKE_CXX_FLAGS_RELEASE})
  string(REPLACE "/Gv" "/Gr" CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS_RELEASE})
  string(REPLACE "/Gv" "/Gr" CMAKE_CXX_FLAGS_DEBUG ${CMAKE_CXX_FLAGS_DEBUG})
  string(REPLACE "/Gv" "/Gr" CMAKE_CXX_FLAGS_RELEASE ${CMAKE_CXX_FLAGS_RELEASE})
else(MSVC)
  set(Boost_USE_STATIC_LIBS ON)
  set(Boost_USE_STATIC_RUNTIME ON)
endif(MSVC)


FILE(GLOB SRCFILES 
        run_time/dynamic_call_asm.asm 
        run_time/*.cpp
        run_time/library/*.cpp
        run_time/tasks_util/*.cpp
        library/*.cpp
        *.cpp
)
add_executable(AttachA ${SRCFILES})
set_property(TARGET AttachA PROPERTY
  MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

find_package(Boost REQUIRED COMPONENTS context)
find_package(asmjit REQUIRED)
include_directories(${Boost_INCLUDE_DIRS} ${asmjit_INCLUDE_DIRS})

target_link_libraries(AttachA
  PRIVATE
    ${Boost_LIBRARIES}
    asmjit::asmjit)