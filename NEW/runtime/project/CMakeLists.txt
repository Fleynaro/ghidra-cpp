add_library(recode_runtime_project STATIC)
target_sources(recode_runtime_project PUBLIC
    FILE_SET cxx_modules TYPE CXX_MODULES
    BASE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/../api" "${CMAKE_CURRENT_SOURCE_DIR}/../dispatcher"
    FILES
        project_config.cppm
        project_state.cppm
        project_session.cppm
        project_manager.cppm
        runtime_core.cppm
        ../api/project_facade.cppm
        ../api/api.cppm
        ../dispatcher/command_dispatcher.cppm
        ../dispatcher/dispatcher.cppm
        ../dispatcher/handlers.cppm
        project.cppm
)
add_library(ReCode::RuntimeProject ALIAS recode_runtime_project)
target_link_libraries(recode_runtime_project PUBLIC
    ReCode::Runtime
    ReCode::PeLoaderService
    ReCode::Sleigh
    ReCode::DecompilerService
    ReCode::FunctionIdService
    ReCode::AnalyzerServices
    ReCode::Analyzer
)
target_compile_features(recode_runtime_project PUBLIC cxx_std_23)
target_compile_definitions(recode_runtime_project PUBLIC
    SLEIGH_RUNTIME_SPECIFICATION_DIR="${CMAKE_SOURCE_DIR}/services/sleigh/specifications"
)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
