function(generate_mock_file_name HEADER_FILE MOCK_FILE)
    # Get the base name of the header file (without path)
    get_filename_component(BASE_NAME ${HEADER_FILE} NAME_WE)

    # Append "_mock.cpp" to the base name
    set(${MOCK_FILE} "${BASE_NAME}_mock.cpp" PARENT_SCOPE)
endfunction()

function(generate_mock_files FILES_TO_MOCK UNDER_TEST_HEADERS)
    set(STUB_OUTPUT_PATH "${CMAKE_BINARY_DIR}")

    set(GENERATED_FILES_LIST)
    set(INCLUDE_DIRS)

    foreach(HEADER_DIR ${UNDER_TEST_HEADERS})
         list(APPEND INCLUDE_DIRS -I "${HEADER_DIR}")
    endforeach()
    message("${INCLUDE_DIRS}")

    foreach(INPUT_FILE ${FILES_TO_MOCK})
        # Generate stub file using CppUMockGen
        generate_mock_file_name(${INPUT_FILE} MOCK_FILE)
        add_custom_command( OUTPUT "${STUB_OUTPUT_PATH}/${MOCK_FILE}"
            COMMAND CppUMockGen -i "${INPUT_FILE}"
            -m "${STUB_OUTPUT_PATH}/${MOCK_FILE}"
                -I "$ENV{RISCV_INCLUDE_PATH}/lib/gcc/riscv32-esp-elf/12.2.0/include/"
                ${INCLUDE_DIRS}
        )
        list(APPEND GENERATED_FILES_LIST "${STUB_OUTPUT_PATH}/${MOCK_FILE}")
    endforeach()

    add_library(stubs "${GENERATED_FILES_LIST}")
    target_include_directories(stubs PRIVATE
      ${UNDER_TEST_HEADERS}
    )
endfunction()

