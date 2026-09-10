module;

#include <stdio.h>

export module hello;

export namespace hello {

[[nodiscard]] constexpr bool is_supported_language(const char* language) {
    return language != nullptr && language[0] != '\0';
}

consteval bool compile_time_feature_check() {
    return is_supported_language("C++23");
}

static_assert(compile_time_feature_check());

[[nodiscard]] inline const char* build_message(const char* language) {
    return is_supported_language(language) ? "Hello from C++23 modules" : "Invalid language";
}

inline int run_demo() {
    return puts(build_message("C++23")) == 0 ? 0 : 1;
}

} // namespace hello
