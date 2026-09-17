# Each analyzer owns this source and test registration boundary.
add_library(analyzer_pdb_msdia STATIC)
target_sources(analyzer_pdb_msdia PUBLIC FILE_SET cxx_modules TYPE CXX_MODULES FILES src/pdb_msdia.cppm)
target_link_libraries(analyzer_pdb_msdia PUBLIC ReCode::AnalyzerShared)
target_compile_features(analyzer_pdb_msdia PUBLIC cxx_std_23)

if(WIN32)
    # CMake Tools does not run VsDevCmd.bat before configure, so VSINSTALLDIR
    # is normally absent even though the selected Visual Studio generator has
    # already resolved the compiler installation.
    set(DIA_SDK_ROOT_HINTS)
    if(CMAKE_GENERATOR_INSTANCE)
        list(APPEND DIA_SDK_ROOT_HINTS
            "${CMAKE_GENERATOR_INSTANCE}/DIA SDK"
        )
    elseif(CMAKE_VS_INSTALLATION_PATH)
        list(APPEND DIA_SDK_ROOT_HINTS
            "${CMAKE_VS_INSTALLATION_PATH}/DIA SDK"
        )
    endif()
    if(DEFINED ENV{VSINSTALLDIR})
        list(APPEND DIA_SDK_ROOT_HINTS "$ENV{VSINSTALLDIR}/DIA SDK")
    endif()

    # Keep the discovery usable with Ninja and command-line CMake, where the
    # generator does not expose CMAKE_VS_INSTALLATION_PATH.
    set(DIA_VSWHERE_HINTS)
    set(DIA_PROGRAM_FILES "$ENV{ProgramFiles}")
    if(DIA_PROGRAM_FILES)
        get_filename_component(
            DIA_PROGRAM_FILES_X86
            "${DIA_PROGRAM_FILES}/../Program Files (x86)"
            ABSOLUTE
        )
        list(APPEND DIA_VSWHERE_HINTS
            "${DIA_PROGRAM_FILES_X86}/Microsoft Visual Studio/Installer"
        )
        list(APPEND DIA_VSWHERE_HINTS
            "${DIA_PROGRAM_FILES}/Microsoft Visual Studio/Installer"
        )
    endif()
    find_program(DIA_VSWHERE_EXECUTABLE NAMES vswhere HINTS ${DIA_VSWHERE_HINTS})
    if(DIA_VSWHERE_EXECUTABLE)
        execute_process(
            COMMAND "${DIA_VSWHERE_EXECUTABLE}"
                -latest
                -products *
                -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64
                -property installationPath
            OUTPUT_VARIABLE DIA_VS_INSTALLATION_PATH
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET
            RESULT_VARIABLE DIA_VSWHERE_RESULT
        )
        if(DIA_VSWHERE_RESULT EQUAL 0 AND DIA_VS_INSTALLATION_PATH)
            list(APPEND DIA_SDK_ROOT_HINTS
                "${DIA_VS_INSTALLATION_PATH}/DIA SDK"
            )
        endif()
    endif()

    find_path(DIA_INCLUDE_DIR NAMES dia2.h HINTS ${DIA_SDK_ROOT_HINTS}
              PATH_SUFFIXES include)
    find_library(DIA_GUIDS_LIBRARY NAMES diaguids HINTS ${DIA_SDK_ROOT_HINTS}
                 PATH_SUFFIXES lib/amd64 lib)
    if(NOT DIA_INCLUDE_DIR OR NOT DIA_GUIDS_LIBRARY)
        message(FATAL_ERROR
            "PDB MSDIA requires the Microsoft DIA SDK (dia2.h and diaguids.lib). "
            "Install the Visual Studio C++ workload or set VSINSTALLDIR to its installation directory."
        )
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
