find_package(GTest CONFIG REQUIRED)

add_executable(decompiler_tests)
target_sources(decompiler_tests
    PUBLIC
        FILE_SET cxx_modules TYPE CXX_MODULES FILES
            decompiler_tests.cppm
            decompiler_datatests.cppm
            native_marshal_tests.cppm
            native_scalar_tests.cppm
            native_type_tests.cppm
            provider_contract_tests.cppm
            provenance_mapping_tests.cppm
)
target_link_libraries(decompiler_tests PRIVATE
    NewGhidra::DecompilerFrontend
    GTest::gtest_main
)
target_compile_features(decompiler_tests PRIVATE cxx_std_23)

add_test(NAME decompiler_tests COMMAND decompiler_tests)
set_tests_properties(decompiler_tests PROPERTIES
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/.."
)

add_executable(native_paramstore_tests)
target_sources(native_paramstore_tests
    PUBLIC
        FILE_SET cxx_modules TYPE CXX_MODULES FILES native_paramstore_tests.cppm
)
target_link_libraries(native_paramstore_tests PRIVATE
    NewGhidra::Decompiler
    GTest::gtest_main
)
target_compile_definitions(native_paramstore_tests PRIVATE
    NATIVE_PARAMSTORE_DATA_ROOT="${CMAKE_CURRENT_SOURCE_DIR}/data/compiler_specs"
)
target_compile_features(native_paramstore_tests PRIVATE cxx_std_23)

add_test(NAME native_paramstore_tests COMMAND native_paramstore_tests)
set_tests_properties(native_paramstore_tests PROPERTIES
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/.."
)

add_executable(native_circlerange_tests)
target_sources(native_circlerange_tests
    PUBLIC
        FILE_SET cxx_modules TYPE CXX_MODULES FILES native_circlerange_tests.cppm
)
target_link_libraries(native_circlerange_tests PRIVATE
    NewGhidra::Decompiler
    GTest::gtest_main
)
target_compile_features(native_circlerange_tests PRIVATE cxx_std_23)

add_test(NAME native_circlerange_tests COMMAND native_circlerange_tests)
set_tests_properties(native_circlerange_tests PROPERTIES
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/.."
)

add_executable(native_funcproto_tests)
target_sources(native_funcproto_tests
    PUBLIC
        FILE_SET cxx_modules TYPE CXX_MODULES FILES native_funcproto_tests.cppm
)
target_link_libraries(native_funcproto_tests PRIVATE
    NewGhidra::Decompiler
    GTest::gtest_main
)
target_compile_features(native_funcproto_tests PRIVATE cxx_std_23)

add_test(NAME native_funcproto_tests COMMAND native_funcproto_tests)
set_tests_properties(native_funcproto_tests PROPERTIES
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/.."
)

add_executable(decompiler_architecture_tests)
target_sources(decompiler_architecture_tests
    PUBLIC
        FILE_SET cxx_modules TYPE CXX_MODULES FILES decompiler_architecture_tests.cppm
)
target_link_libraries(decompiler_architecture_tests PRIVATE
    NewGhidra::DecompilerFrontend
    GTest::gtest_main
)
target_compile_features(decompiler_architecture_tests PRIVATE cxx_std_23)

add_test(NAME decompiler_architecture_tests COMMAND decompiler_architecture_tests)
set_tests_properties(decompiler_architecture_tests PROPERTIES
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/.."
)


add_executable(metadata_provider_tests)
target_sources(metadata_provider_tests
    PUBLIC
        FILE_SET cxx_modules TYPE CXX_MODULES FILES metadata_provider_tests.cppm
)
target_link_libraries(metadata_provider_tests PRIVATE
    NewGhidra::DecompilerFrontend
    GTest::gtest_main
)
target_compile_features(metadata_provider_tests PRIVATE cxx_std_23)

add_test(NAME metadata_provider_tests COMMAND metadata_provider_tests)
set_tests_properties(metadata_provider_tests PROPERTIES
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/.."
)
