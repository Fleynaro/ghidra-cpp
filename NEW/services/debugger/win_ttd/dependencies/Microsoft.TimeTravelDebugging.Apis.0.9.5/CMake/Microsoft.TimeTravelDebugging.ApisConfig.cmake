cmake_minimum_required (VERSION 3.16)

# FUTURE: Include TargetInfo.cmake in the Apis kit and make use of it to simplify the logic below

# TODO: Standardize this better.
if(MSVC AND DEFINED $ENV{Platform})
    set(TargetPlatform "$ENV{Platform}")
elseif(CMAKE_CXX_COMPILER_ARCHITECTURE_ID)
    set(TargetPlatform "${CMAKE_CXX_COMPILER_ARCHITECTURE_ID}")
else()
    set(TargetPlatform "${CMAKE_SYSTEM_PROCESSOR}")
endif()

if("${TargetPlatform}" STREQUAL "AMD64")
    set(TargetPlatform "x64")
elseif("${TargetPlatform}" STREQUAL "X86")
    set(TargetPlatform "x86")
elseif("${TargetPlatform}" STREQUAL "ARM64")
    set(TargetPlatform "arm64")
endif()

if(NOT TARGET Microsoft.TimeTravelDebugging.Apis::Includes)
    if("${CoreTtd.SDK_ROOT}" STREQUAL "")
        add_library(Microsoft.TimeTravelDebugging.Apis::Includes        INTERFACE IMPORTED)
        add_library(Microsoft.TimeTravelDebugging.Apis::TTDReplay       STATIC    IMPORTED)
        add_library(Microsoft.TimeTravelDebugging.Apis::TTDLiveRecorder STATIC    IMPORTED)

        set(InstallRoot "${CMAKE_CURRENT_LIST_DIR}/../sdk")
        set(LibBasePath "${InstallRoot}/lib/${TargetPlatform}")

        target_include_directories(Microsoft.TimeTravelDebugging.Apis::Includes INTERFACE "${InstallRoot}/include")

        set_target_properties(Microsoft.TimeTravelDebugging.Apis::TTDReplay       PROPERTIES IMPORTED_LOCATION "${LibBasePath}/TTDReplay.lib")
        set_target_properties(Microsoft.TimeTravelDebugging.Apis::TTDLiveRecorder PROPERTIES IMPORTED_LOCATION "${LibBasePath}/TTDLiveRecorder.lib")

        target_link_libraries(Microsoft.TimeTravelDebugging.Apis::TTDReplay       INTERFACE Microsoft.TimeTravelDebugging.Apis::Includes)
        target_link_libraries(Microsoft.TimeTravelDebugging.Apis::TTDLiveRecorder INTERFACE Microsoft.TimeTravelDebugging.Apis::Includes)
    else()
        # This allows use of this package where it's built.
        add_library(Microsoft.TimeTravelDebugging.Apis::Includes        ALIAS SDK)
        add_library(Microsoft.TimeTravelDebugging.Apis::TTDReplay       ALIAS TTDReplay_ImportLib)
        add_library(Microsoft.TimeTravelDebugging.Apis::TTDLiveRecorder ALIAS TTDLiveRecorder_ImportLib)
    endif()
endif()
