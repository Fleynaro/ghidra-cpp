# Each analyzer owns this source and test registration boundary.
add_library(analyzer_pdb_msdia STATIC)
target_sources(analyzer_pdb_msdia PUBLIC FILE_SET cxx_modules TYPE CXX_MODULES FILES src/pdb_msdia.cppm)
target_link_libraries(analyzer_pdb_msdia PUBLIC NewGhidra::AnalyzerShared)
target_compile_features(analyzer_pdb_msdia PUBLIC cxx_std_23)

if(WIN32)
    find_path(DIA_INCLUDE_DIR NAMES dia2.h HINTS ENV VSINSTALLDIR ENV ProgramFiles
              PATH_SUFFIXES "DIA SDK/include")
    find_library(DIA_GUIDS_LIBRARY NAMES diaguids HINTS ENV VSINSTALLDIR ENV ProgramFiles
                 PATH_SUFFIXES "DIA SDK/lib/amd64" "DIA SDK/lib")
    if(NOT DIA_INCLUDE_DIR OR NOT DIA_GUIDS_LIBRARY)
        message(FATAL_ERROR "PDB MSDIA requires the Microsoft DIA SDK (dia2.h and diaguids.lib)")
    endif()
    target_include_directories(analyzer_pdb_msdia PRIVATE "${DIA_INCLUDE_DIR}")
    target_link_libraries(analyzer_pdb_msdia PUBLIC ole32 oleaut32 "${DIA_GUIDS_LIBRARY}")
endif()

if(BUILD_TESTING)
    find_package(GTest CONFIG REQUIRED)
    add_executable(analyzer_pdb_msdia_tests)
    target_sources(analyzer_pdb_msdia_tests PRIVATE FILE_SET cxx_modules TYPE CXX_MODULES FILES tests/pdb_msdia_tests.cppm)
    target_link_libraries(analyzer_pdb_msdia_tests PRIVATE analyzer_pdb_msdia GTest::gtest_main)
    target_compile_definitions(analyzer_pdb_msdia_tests PRIVATE ANALYZER_FIXTURE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/..")
    target_compile_features(analyzer_pdb_msdia_tests PRIVATE cxx_std_23)
    add_test(NAME analyzer_pdb_msdia_tests COMMAND analyzer_pdb_msdia_tests)
endif()
