set(testTargets
    pressure_test
    always_insert
    test_existing_tree
    parse_commands
    gentest
    test_bf
    rb_tree
)

foreach(targetName IN LISTS testTargets)
    add_executable(${targetName} ${targetName}.cpp)
    target_include_directories(${targetName}
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/../include
            ${CMAKE_CURRENT_SOURCE_DIR}/../src
    )
    target_link_libraries(${targetName} PRIVATE bplus_tree)
endforeach()

set(DATA_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../data)

add_test(NAME setup_data_dir
    COMMAND ${CMAKE_COMMAND} -E make_directory ${DATA_DIR}
)

add_test(NAME cleanup_test_data_begin
    COMMAND ${CMAKE_COMMAND} -E rm -f ${DATA_DIR}/test.bin ${DATA_DIR}/_test_for_rb_tree.bin
)
set_tests_properties(cleanup_test_data_begin PROPERTIES DEPENDS setup_data_dir)

add_test(NAME pressure_test COMMAND pressure_test)
set_tests_properties(pressure_test PROPERTIES DEPENDS cleanup_test_data_begin)

add_test(NAME cleanup_after_pressure
    COMMAND ${CMAKE_COMMAND} -E rm -f ${DATA_DIR}/test.bin
)
set_tests_properties(cleanup_after_pressure PROPERTIES DEPENDS pressure_test)

add_test(NAME always_insert COMMAND always_insert)
set_tests_properties(always_insert PROPERTIES DEPENDS cleanup_after_pressure)

add_test(NAME test_existing_tree COMMAND test_existing_tree)
set_tests_properties(test_existing_tree PROPERTIES DEPENDS always_insert)

add_test(NAME cleanup_after_existing_tree
    COMMAND ${CMAKE_COMMAND} -E rm -f ${DATA_DIR}/test.bin
)
set_tests_properties(cleanup_after_existing_tree PROPERTIES DEPENDS test_existing_tree)

add_test(NAME parse_commands COMMAND parse_commands)
set_tests_properties(parse_commands PROPERTIES DEPENDS cleanup_after_existing_tree)

add_test(NAME rb_tree COMMAND rb_tree)
set_tests_properties(rb_tree PROPERTIES DEPENDS parse_commands)

add_test(NAME cleanup_after_rb_tree
    COMMAND ${CMAKE_COMMAND} -E rm -f ${DATA_DIR}/_test_for_rb_tree.bin
)
set_tests_properties(cleanup_after_rb_tree PROPERTIES DEPENDS rb_tree)

set(cmdFile ${CMAKE_CURRENT_BINARY_DIR}/test_cmd.sql)
set(bfOutFile ${CMAKE_CURRENT_BINARY_DIR}/test_result_bf.txt)
set(bplusOutFile ${CMAKE_CURRENT_BINARY_DIR}/test_result_bplus.txt)

add_test(NAME compare_main_with_baseline
    COMMAND ${CMAKE_COMMAND}
        -DgentestExe=$<TARGET_FILE:gentest>
        -DtestBfExe=$<TARGET_FILE:test_bf>
        -DmainExe=$<TARGET_FILE:main>
        -DcmdFile=${cmdFile}
        -DbfOutFile=${bfOutFile}
        -DbplusOutFile=${bplusOutFile}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/run_main_compare.cmake
)
set_tests_properties(compare_main_with_baseline PROPERTIES DEPENDS cleanup_after_rb_tree)

add_test(NAME cleanup_test_data_end
    COMMAND ${CMAKE_COMMAND} -E rm -f ${DATA_DIR}/test.bin ${DATA_DIR}/_test_for_rb_tree.bin
)
set_tests_properties(cleanup_test_data_end PROPERTIES DEPENDS compare_main_with_baseline)
