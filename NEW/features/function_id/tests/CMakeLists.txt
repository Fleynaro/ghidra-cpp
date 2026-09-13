find_package(GTest CONFIG REQUIRED)

set(SLEIGH_X86_TEST_SLA_PATH "${CMAKE_CURRENT_LIST_DIR}/../../sleigh_runtime/test_data/x86-64.sla")
set(SLEIGH_ARM_TEST_SLA_PATH "${CMAKE_CURRENT_LIST_DIR}/../../sleigh_runtime/test_data/ARM8_le.sla")
set(SLEIGH_X86_PARENT_SLA_PATH "${CMAKE_CURRENT_LIST_DIR}/../../../../../../../NEW/features/sleigh_runtime/test_data/x86-64.sla")
if(EXISTS "${SLEIGH_X86_PARENT_SLA_PATH}")
    set(SLEIGH_X86_TEST_SLA_PATH "${SLEIGH_X86_PARENT_SLA_PATH}")
endif()

add_executable(function_id_tests function_id_tests.cpp)
target_link_libraries(function_id_tests PRIVATE NewGhidra::FunctionId GTest::gtest_main)
target_compile_features(function_id_tests PRIVATE cxx_std_23)
target_compile_definitions(function_id_tests PRIVATE
    FUNCTION_ID_TEST_DIR="${CMAKE_CURRENT_LIST_DIR}/../../../../TEST/fid"
    SLEIGH_X86_TEST_SLA="${SLEIGH_X86_TEST_SLA_PATH}"
    SLEIGH_ARM_TEST_SLA="${SLEIGH_ARM_TEST_SLA_PATH}"
)

add_test(NAME function_id_tests COMMAND function_id_tests)
