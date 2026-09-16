add_library(new_ghidra_translation_engine STATIC)
target_sources(new_ghidra_translation_engine PUBLIC
    FILE_SET cxx_modules TYPE CXX_MODULES FILES
        native/native_translation.cppm
        native/address.cppm
        native/space.cppm
        native/types.cppm
        native/translate.cppm
        native/loadimage.cppm
        native/globalcontext.cppm
        native/context.cppm
        native/marshal.cppm
        native/opcodes.cppm
        native/pcoderaw.cppm
        native/error.cppm
)
add_library(NewGhidra::TranslationEngine ALIAS new_ghidra_translation_engine)
target_link_libraries(new_ghidra_translation_engine PUBLIC NewGhidra::Core)
target_compile_features(new_ghidra_translation_engine PUBLIC cxx_std_23)
