if(NOT DEFINED gentestExe OR NOT DEFINED testBfExe OR NOT DEFINED mainExe)
    message(FATAL_ERROR "gentestExe/testBfExe/mainExe are required")
endif()

if(NOT DEFINED cmdFile OR NOT DEFINED bfOutFile OR NOT DEFINED bplusOutFile)
    message(FATAL_ERROR "cmdFile/bfOutFile/bplusOutFile are required")
endif()

execute_process(
    COMMAND ${gentestExe}
    OUTPUT_FILE ${cmdFile}
    RESULT_VARIABLE gentestResult
)
if(NOT gentestResult EQUAL 0)
    message(FATAL_ERROR "gentest failed with code ${gentestResult}")
endif()

execute_process(
    COMMAND ${testBfExe} ${cmdFile}
    OUTPUT_FILE ${bfOutFile}
    RESULT_VARIABLE testBfResult
)
if(NOT testBfResult EQUAL 0)
    message(FATAL_ERROR "test_bf failed with code ${testBfResult}")
endif()

execute_process(
    COMMAND ${mainExe} ${cmdFile}
    OUTPUT_FILE ${bplusOutFile}
    RESULT_VARIABLE mainResult
)
if(NOT mainResult EQUAL 0)
    message(FATAL_ERROR "main failed with code ${mainResult}")
endif()

file(READ ${bfOutFile} bfOut)
file(READ ${bplusOutFile} bplusOut)

string(REGEX REPLACE "[ \t\r\n]" "" bfCompact "${bfOut}")
string(REGEX REPLACE "[ \t\r\n]" "" bplusCompact "${bplusOut}")

if(NOT bfCompact STREQUAL bplusCompact)
    message(FATAL_ERROR "Output mismatch between baseline and B+ tree engine")
endif()
