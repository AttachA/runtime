cmake_minimum_required(VERSION 3.20)
project(AttachA CXX)
set(CMAKE_CXX_STANDARD 20)
set(Boost_NO_WARN_NEW_VERSIONS ON)
#enable_testing()
#add_subdirectory(tests)

set(list_array_INCLUDE_DIR "src/library/list_array")

if(MSVC)
  enable_language(ASM_MASM)
  string(REPLACE "/MD" "/MT" CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})
  string(REPLACE "/MD" "/MT" CMAKE_CXX_FLAGS_DEBUG ${CMAKE_CXX_FLAGS_DEBUG})
  string(REPLACE "/MD" "/MT" CMAKE_CXX_FLAGS_RELEASE ${CMAKE_CXX_FLAGS_RELEASE})
  string(REPLACE "/Gv" "/Gr" CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS_RELEASE})
  string(REPLACE "/Gv" "/Gr" CMAKE_CXX_FLAGS_DEBUG ${CMAKE_CXX_FLAGS_DEBUG})
  string(REPLACE "/Gv" "/Gr" CMAKE_CXX_FLAGS_RELEASE ${CMAKE_CXX_FLAGS_RELEASE})
else(MSVC)
  add_compile_options(-fpermissive)
  add_compile_options(-m64)
  add_compile_options(-g)
  set(Boost_USE_STATIC_LIBS ON)
  set(Boost_USE_STATIC_RUNTIME ON)
endif(MSVC)
set(ENABLE_BOOST_PREPROCESSOR ON)

FILE(GLOB SRCFILES 
        src/run_time/asm/dynamic_call_asm.asm 
        src/run_time/*.cpp
        src/run_time/asm/*.cpp
        src/run_time/asm/il_compiler/*.cpp
        src/run_time/cxx_library/*.cpp
        src/run_time/library/*.cpp
        src/run_time/tasks_util/*.cpp
        src/library/*.cpp
        src/*.cpp 
)
FILE(GLOB LIB_SRCFILES 
        src/run_time/asm/dynamic_call_asm.asm 
        src/run_time/*.cpp
        src/run_time/asm/*.cpp
        src/run_time/asm/il_compiler/*.cpp
        src/run_time/cxx_library/*.cpp
        src/run_time/library/*.cpp
        src/run_time/tasks_util/*.cpp
        src/library/*.cpp
        src/run_time.cpp 
)


find_package(Boost REQUIRED COMPONENTS context)
find_package(asmjit REQUIRED)
find_package(utf8cpp CONFIG REQUIRED)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  find_package(PkgConfig)
  pkg_check_modules(liburing REQUIRED IMPORTED_TARGET GLOBAL liburing>=2.0)
  pkg_check_modules(libunwind REQUIRED IMPORTED_TARGET GLOBAL libunwind)
  set (liburing_LIBRARIES PkgConfig::liburing PkgConfig::libunwind)
  include_directories(${Boost_INCLUDE_DIRS} ${asmjit_INCLUDE_DIRS} ${liburing_INCLUDE_DIRS} ${libunwind_INCLUDE_DIRS} ${list_array_INCLUDE_DIR})
else()
  set (liburing_LIBRARIES "")
  include_directories(${Boost_INCLUDE_DIRS} ${asmjit_INCLUDE_DIRS} ${list_array_INCLUDE_DIR})
endif()


add_executable(AttachA ${SRCFILES})
set_property(TARGET AttachA PROPERTY
  MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
target_link_libraries(AttachA
  PRIVATE
    ${Boost_LIBRARIES}
    asmjit::asmjit
    utf8::cpp
    ${liburing_LIBRARIES}
  )

  
add_library(AttachA_lib STATIC ${LIB_SRCFILES})
set_property(TARGET AttachA_lib PROPERTY
  MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
target_link_libraries(AttachA_lib
  PRIVATE
    ${Boost_LIBRARIES}
    asmjit::asmjit
    utf8::cpp
    ${liburing_LIBRARIES}
  )
  


IF(MSVC)
  SET(CMAKE_CXX_FLAGS "/EHsc")
ENDIF(MSVC)

include(CTest)
if(BUILD_TESTING)
  add_subdirectory(tests)
endif()