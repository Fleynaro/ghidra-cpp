find_package(ZLIB REQUIRED)

file(TO_CMAKE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/specifications" SLEIGH_RUNTIME_SPECIFICATION_DIR)
file(GLOB SLEIGH_RUNTIME_MODULES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cppm")

add_library(sleigh_runtime STATIC)
target_sources(sleigh_runtime
    PUBLIC FILE_SET cxx_modules TYPE CXX_MODULES FILES sleigh_runtime.cppm
    PRIVATE sleigh_runtime_adapter.cppm
    PRIVATE FILE_SET runtime_modules TYPE CXX_MODULES
        BASE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/src" FILES ${SLEIGH_RUNTIME_MODULES}
)
add_library(ReCode::SleighRuntime ALIAS sleigh_runtime)
target_compile_features(sleigh_runtime PUBLIC cxx_std_23)
target_compile_definitions(sleigh_runtime PUBLIC SLEIGH_RUNTIME_SPECIFICATION_DIR="${SLEIGH_RUNTIME_SPECIFICATION_DIR}")
target_link_libraries(sleigh_runtime PUBLIC ReCode::Core ReCode::Std PRIVATE ZLIB::ZLIB)

add_executable(sleigh_runtime_decode cli/sleigh_runtime_decode.cppm)
target_link_libraries(sleigh_runtime_decode PRIVATE ReCode::SleighRuntime)
target_compile_features(sleigh_runtime_decode PRIVATE cxx_std_23)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

add_library(recode_sleigh_service STATIC)
target_sources(recode_sleigh_service PUBLIC
    FILE_SET cxx_modules TYPE CXX_MODULES FILES sleigh_service.cppm decoder_resource.cppm
)
add_library(ReCode::Sleigh ALIAS recode_sleigh_service)
target_link_libraries(recode_sleigh_service PUBLIC
    ReCode::Core ReCode::RuntimeWorkers ReCode::TranslationEngine ReCode::SleighRuntime
)
target_compile_features(recode_sleigh_service PUBLIC cxx_std_23)
