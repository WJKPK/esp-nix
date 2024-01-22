cmake_minimum_required(VERSION 3.15)

include(FetchContent)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
    message(FATAL_ERROR "Do not build in-source. Please remove CMakeCache.txt and the CMakeFiles/ directory. Then build out-of-source.")
endif()

project(tests)

FetchContent_Declare(
  FreeRTOS_Kernel
  # hash: sha256-1cwYHf0fCy58sXjgJC34a8ksm8ul+v/fzFZdbo1x48g=
  GIT_REPOSITORY https://github.com/FreeRTOS/FreeRTOS-Kernel.git
  GIT_TAG 0264280230aa6a828247b5f05bf57e33f1994581)


set( FREERTOS_KERNEL_PATH "tests/freertos" )
set( UNDER_TEST_CODE_PATH "src" )
set( TESTS_CODE_PATH "tests" )

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(freertos_config INTERFACE)
target_include_directories(freertos_config SYSTEM
INTERFACE
  ${FREERTOS_KERNEL_PATH}
)
set( FREERTOS_HEAP "4" CACHE STRING "" FORCE)
set( FREERTOS_PORT "GCC_POSIX" CACHE STRING "" FORCE)
FetchContent_MakeAvailable(FreeRTOS_Kernel)
target_compile_options(freertos_kernel PRIVATE -Og -ggdb -Wall -Werror)

add_executable(tests
               ${TESTS_CODE_PATH}/init.cpp
               ${TESTS_CODE_PATH}/common.cpp
               ${TESTS_CODE_PATH}/schedulerTests.cpp
               ${TESTS_CODE_PATH}/timerTests.cpp
               ${UNDER_TEST_CODE_PATH}/main/utilities/scheduler.c
               ${UNDER_TEST_CODE_PATH}/main/utilities/timer.c
	            )

target_compile_options(tests PRIVATE -Og -ggdb -Wall -Werror)
target_compile_features(tests PRIVATE cxx_std_11)

target_include_directories(tests PRIVATE
  ${UNDER_TEST_CODE_PATH}/main
  ${TESTS_CODE_PATH}/configs
  ${TESTS_CODE_PATH}
  )

target_link_directories(tests PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/build)
target_link_libraries(tests freertos_kernel freertos_config CppUTest CppUTestExt)
