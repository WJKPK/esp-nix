cmake_minimum_required(VERSION 3.15)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
    message(FATAL_ERROR "Do not build in-source. Please remove CMakeCache.txt and the CMakeFiles/ directory. Then build out-of-source.")
endif()

project(scheduler_test)

set( FREERTOS_KERNEL_PATH "./freertos" )
set( UNDER_TEST_CODE_PATH "../src" )
set( TESTS_CODE_PATH "." )

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(${FREERTOS_KERNEL_PATH})

add_executable(scheduler_test
               init.cpp
               common.cpp
               schedulerTests.cpp
               ${UNDER_TEST_CODE_PATH}/main/utilities/scheduler.c
	            )

target_compile_options(scheduler_test PRIVATE -Og -ggdb -Wall -Werror)
target_compile_features(scheduler_test PRIVATE cxx_std_11)
target_compile_options(freertos_kernel PRIVATE -Og -ggdb -Wall -Werror)

target_include_directories(scheduler_test PRIVATE
  ${UNDER_TEST_CODE_PATH}/main
  ${TESTS_CODE_PATH}/configs)

target_link_directories(scheduler_test PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/build)
target_link_libraries(scheduler_test freertos_kernel freertos_config CppUTest CppUTestExt)
