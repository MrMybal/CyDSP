# Cross-compile static ARM64 archives with installed clang-cl/llvm-lib and SDK headers.
# Compiler/tool paths and INCLUDE are provided by scripts/build_arm64.py.
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR ARM64)
set(CMAKE_CXX_COMPILER_TARGET aarch64-pc-windows-msvc)
# No executable probes or target execution are needed to build an archive.
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

set(CMAKE_C_COMPILER "${CMAKE_CXX_COMPILER}")
set(CMAKE_C_COMPILER_TARGET aarch64-pc-windows-msvc)
