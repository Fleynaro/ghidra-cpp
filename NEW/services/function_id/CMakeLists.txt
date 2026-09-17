find_package(ZLIB REQUIRED)

add_library(function_id STATIC)
set(FUNCTION_ID_MODULES
    src/types.cppm
    src/parse_exception.cppm
    src/storage_helpers.cppm
    src/buffer_file.cppm
    src/hasher.cppm
    src/database.cppm
)
target_sources(function_id
    PUBLIC FILE_SET cxx_modules TYPE CXX_MODULES FILES src/function_id.cppm
    PUBLIC FILE_SET function_id_modules TYPE CXX_MODULES
        BASE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/src" FILES ${FUNCTION_ID_MODULES}
)
add_library(ReCode::FunctionId ALIAS function_id)
target_compile_features(function_id PUBLIC cxx_std_23)
target_link_libraries(function_id PUBLIC ReCode::Core ReCode::SleighRuntime ReCode::Std PRIVATE ZLIB::ZLIB)

add_executable(function_id_cli cli/function_id_cli.cppm)
target_link_libraries(function_id_cli PRIVATE ReCode::FunctionId ReCode::SleighRuntime)
target_compile_features(function_id_cli PRIVATE cxx_std_23)
target_compile_definitions(function_id_cli PRIVATE FUNCTION_ID_DATABASE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}/data")

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

add_library(recode_function_id_service STATIC)
target_sources(recode_function_id_service PUBLIC
    FILE_SET cxx_modules TYPE CXX_MODULES FILES function_id_service.cppm types.cppm database_resource.cppm
)
add_library(ReCode::FunctionIdService ALIAS recode_function_id_service)
target_link_libraries(recode_function_id_service PUBLIC
    ReCode::Core ReCode::RuntimeWorkers function_id
)
target_compile_features(recode_function_id_service PUBLIC cxx_std_23)
