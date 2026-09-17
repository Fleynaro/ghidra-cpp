set(RECODE_RUNTIME_WORKER_MODULES
    worker_pool.cppm
    workers.cppm
    task.cppm
    cancellation.cppm
    progress.cppm
)

add_library(recode_runtime_workers STATIC)
target_sources(recode_runtime_workers
    PUBLIC
        FILE_SET cxx_modules TYPE CXX_MODULES
        BASE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}"
        FILES ${RECODE_RUNTIME_WORKER_MODULES}
)
add_library(ReCode::RuntimeWorkers ALIAS recode_runtime_workers)
target_link_libraries(recode_runtime_workers PUBLIC ReCode::Core)
target_compile_features(recode_runtime_workers PUBLIC cxx_std_23)
