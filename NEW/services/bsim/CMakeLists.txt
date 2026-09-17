set(BSIM_MODULES
    bsim_idf.cppm
    bsim_weight_factory.cppm
    bsim_hash_entry.cppm
    bsim_vector_compare.cppm
    bsim_cosine_vector.cppm
    bsim_vector_factory.cppm
    bsim_resource.cppm
    bsim_service.cppm
)

add_library(recode_bsim STATIC)
add_library(ReCode::Bsim ALIAS recode_bsim)
target_sources(recode_bsim PUBLIC
    FILE_SET cxx_modules TYPE CXX_MODULES
    BASE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}"
    FILES ${BSIM_MODULES}
)
target_compile_features(recode_bsim PUBLIC cxx_std_23)
target_compile_definitions(recode_bsim PUBLIC BSIM_RESOURCE_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}/resources")
target_link_libraries(recode_bsim PUBLIC ReCode::Core)

add_library(recode_bsim_decompiler_adapter STATIC)
add_library(ReCode::BsimDecompilerAdapter ALIAS recode_bsim_decompiler_adapter)
target_sources(recode_bsim_decompiler_adapter PUBLIC
    FILE_SET cxx_modules TYPE CXX_MODULES
    BASE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}"
    FILES bsim_decompiler_adapter.cppm
)
target_compile_features(recode_bsim_decompiler_adapter PUBLIC cxx_std_23)
target_link_libraries(recode_bsim_decompiler_adapter PUBLIC ReCode::Bsim ReCode::DecompilerFrontend)
add_dependencies(recode_bsim_decompiler_adapter recode_decompiler_frontend)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
