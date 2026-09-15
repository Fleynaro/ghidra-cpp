# Boost.Iostreams Test

[`test.cpp`](test.cpp) writes and reads a real file with Boost.Iostreams file source/sink devices and maps it with `mapped_file_source`. The FID verification accepts direct matches whose applied demangled name is a method such as `init` but whose original FID record identifies `mapped_file_source`.
