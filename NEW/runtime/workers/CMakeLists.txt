set(NEW_GHIDRA_RUNTIME_WORKER_MODULES
    worker_pool.cppm
    workers.cppm
    task.cppm
    cancellation.cppm
    progress.cppm
)

add_library(new_ghidra_runtime_workers STATIC)
target_sources(new_ghidra_runtime_workers
    PUBLIC
        FILE_SET cxx_modules TYPE CXX_MODULES
        BASE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}"
        FILES ${NEW_GHIDRA_RUNTIME_WORKER_MODULES}
)
add_library(NewGhidra::RuntimeWorkers ALIAS new_ghidra_runtime_workers)
target_link_libraries(new_ghidra_runtime_workers PUBLIC NewGhidra::Core)
target_compile_features(new_ghidra_runtime_workers PUBLIC cxx_std_23)
