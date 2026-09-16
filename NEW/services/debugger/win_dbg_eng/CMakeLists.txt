add_library(new_ghidra_win_dbg_eng STATIC)
target_sources(new_ghidra_win_dbg_eng PUBLIC
    FILE_SET cxx_modules TYPE CXX_MODULES FILES win_dbg_eng.cppm
)
add_library(NewGhidra::WinDbgEng ALIAS new_ghidra_win_dbg_eng)
target_link_libraries(new_ghidra_win_dbg_eng PUBLIC NewGhidra::Core NewGhidra::Std)
target_compile_features(new_ghidra_win_dbg_eng PUBLIC cxx_std_23)

if(WIN32)
    # DbgEng is shipped with the Windows debugging tools/SDK, not vcpkg.
    # The import is intentionally private to this backend target.
    set(DBGENG_PROGRAM_FILES "$ENV{ProgramFiles}")
    get_filename_component(DBGENG_PROGRAM_FILES_X86
        "${DBGENG_PROGRAM_FILES}/../Program Files (x86)" ABSOLUTE)
    file(GLOB DBGENG_SDK_VERSION_DIRS LIST_DIRECTORIES true
        "${DBGENG_PROGRAM_FILES_X86}/Windows Kits/10/Include/*")
    list(SORT DBGENG_SDK_VERSION_DIRS COMPARE NATURAL ORDER DESCENDING)
    list(GET DBGENG_SDK_VERSION_DIRS 0 DBGENG_SDK_VERSION_DIR)
    get_filename_component(DBGENG_SDK_VERSION "${DBGENG_SDK_VERSION_DIR}" NAME)
    set(DBGENG_SDK_LIB_VERSION_DIR
        "${DBGENG_PROGRAM_FILES_X86}/Windows Kits/10/Lib/${DBGENG_SDK_VERSION}")
    get_filename_component(DBGENG_MSVC_BIN "${CMAKE_CXX_COMPILER}" DIRECTORY)
    get_filename_component(DBGENG_MSVC_HOST "${DBGENG_MSVC_BIN}" DIRECTORY)
    get_filename_component(DBGENG_MSVC_BIN_ROOT "${DBGENG_MSVC_HOST}" DIRECTORY)
    get_filename_component(DBGENG_MSVC_VERSION "${DBGENG_MSVC_BIN_ROOT}" DIRECTORY)
    find_path(DBGENG_INCLUDE_DIR NAMES dbgeng.h
        HINTS
            "${DBGENG_PROGRAM_FILES_X86}/Windows Kits/10/Debuggers"
            "${DBGENG_PROGRAM_FILES_X86}/Windows Kits/10/Include"
        PATH_SUFFIXES inc um
    )
    find_library(DBGENG_LIBRARY NAMES dbgeng DbgEng
        HINTS "${DBGENG_PROGRAM_FILES_X86}/Windows Kits/10/Debuggers"
        PATH_SUFFIXES lib/x64 lib
    )
    if(NOT DBGENG_INCLUDE_DIR OR NOT DBGENG_LIBRARY)
        message(FATAL_ERROR "WinDbgEng requires dbgeng.h and dbgeng.lib from the Windows debugging tools/SDK")
    endif()
    target_include_directories(new_ghidra_win_dbg_eng PRIVATE
        "${DBGENG_INCLUDE_DIR}"
        "${DBGENG_SDK_VERSION_DIR}/um"
        "${DBGENG_SDK_VERSION_DIR}/shared"
        "${DBGENG_SDK_VERSION_DIR}/ucrt"
        "${DBGENG_MSVC_VERSION}/include"
    )
    target_link_libraries(new_ghidra_win_dbg_eng PRIVATE "${DBGENG_LIBRARY}")
endif()

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
