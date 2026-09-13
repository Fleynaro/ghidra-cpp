target_sources(analyzer_shared
    PUBLIC
        FILE_SET cxx_modules TYPE CXX_MODULES FILES src/analyzer.cppm
    PRIVATE
        src/analyzer.cpp
)
