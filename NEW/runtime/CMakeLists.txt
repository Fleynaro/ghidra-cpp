find_package(unofficial-sqlite3 CONFIG REQUIRED)

add_subdirectory(workers)

set(RECODE_RUNTIME_MODULES
    event_bus/event_bus.cppm
    event_store/event_store.cppm
    event_store/event_codec.cppm
    event_store/append_only_log.cppm
    event_store/replay.cppm
    storage/sqlite_connection.cppm
    storage/sqlite_projection_store.cppm
    projections/software_model_projection.cppm
    projections/analysis_projection.cppm
    projections/diagnostics_projection.cppm
    projections/projection_coordinator.cppm
    projections/projections.cppm
    analysis/analyzer_registry.cppm
    analysis/trigger_coalescer.cppm
    analysis/analysis_scheduler.cppm
    analysis/analysis.cppm
    analysis/analysis_snapshot.cppm
    analysis/trigger_index.cppm
    analysis/analysis_run.cppm
    resources/resource_manager.cppm
    resources/resource_lease.cppm
    resources/sla_cache.cppm
    resources/compiler_spec_cache.cppm
    resources/fid_cache.cppm
    runtime.cppm
    dispatcher/command_contract.cppm
)

add_library(recode_runtime STATIC)
target_sources(recode_runtime
    PUBLIC
        FILE_SET cxx_modules TYPE CXX_MODULES
        BASE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}"
        FILES ${RECODE_RUNTIME_MODULES}
)
add_library(ReCode::Runtime ALIAS recode_runtime)
target_link_libraries(recode_runtime PUBLIC ReCode::Core ReCode::RuntimeWorkers unofficial::sqlite3::sqlite3)
target_compile_features(recode_runtime PUBLIC cxx_std_23)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
