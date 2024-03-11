cmake_minimum_required(VERSION 3.15)

include(makefiles/mocks.cmake)
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


set( FREERTOS_KERNEL_PATH "${CMAKE_CURRENT_SOURCE_DIR}/tests/freertos" )
set( UNDER_TEST_CODE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/src" )
set( TESTS_CODE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/tests" )

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

set ( UNDER_TEST_FILES
      ${UNDER_TEST_CODE_PATH}/main/utilities/scheduler.c
      ${UNDER_TEST_CODE_PATH}/main/utilities/timer.c
      ${UNDER_TEST_CODE_PATH}/main/menu.c
    )

set ( UNDER_TEST_FILES_MOCKED
      ${UNDER_TEST_CODE_PATH}/main/lcd.h
      ${UNDER_TEST_CODE_PATH}/main/encoder.h
    )

set ( UNDER_TEST_HEADERS
      ${TESTS_CODE_PATH}/configs
      ${TESTS_CODE_PATH}
      ${UNDER_TEST_CODE_PATH}/main
    )

generate_mock_files("${UNDER_TEST_FILES_MOCKED}" "${UNDER_TEST_HEADERS}")

set ( TEST_FILES
      ${TESTS_CODE_PATH}/init.cpp
      ${TESTS_CODE_PATH}/common.cpp
      ${TESTS_CODE_PATH}/schedulerTests.cpp
      ${TESTS_CODE_PATH}/timerTests.cpp
      ${TESTS_CODE_PATH}/menuTests.cpp
    )

add_executable( tests
                ${UNDER_TEST_FILES}
                ${TEST_FILES}
                ${STUB_OUTPUT_PATH}
	          )

target_compile_options(tests PRIVATE -Og -ggdb -Wall -Werror)
target_compile_features(tests PRIVATE cxx_std_11)

target_include_directories(tests PRIVATE
  ${UNDER_TEST_HEADERS}
  )

target_link_directories(tests PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/build)
target_link_libraries(tests freertos_kernel stubs freertos_config CppUTest CppUTestExt)
