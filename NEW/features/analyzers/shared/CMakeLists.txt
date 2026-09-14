target_sources(analyzer_shared
    PUBLIC
        FILE_SET cxx_modules TYPE CXX_MODULES FILES
            src/analyzer.cppm
            src/analyzer_types.cppm
            src/analyzer_cancellation_token.cppm
            src/analyzer_base.cppm
            src/analyzer_context.cppm
            src/analyzer_registry.cppm
            src/analyzer_manager.cppm
)

add_subdirectory(test_support)
add_subdirectory(tests)
