set(CMAKE_C_COMPILER zig cc)
set(CMAKE_CXX_COMPILER zig c++)

set(CMAKE_C_COMPILER_TARGET ${TARGET})
set(CMAKE_CXX_COMPILER_TARGET ${TARGET})

# See https://github.com/ziglang/zig/issues/22213

set(CMAKE_C_LINKER_DEPFILE_SUPPORTED OFF)
set(CMAKE_CXX_LINKER_DEPFILE_SUPPORTED OFF)

# See https://github.com/ziglang/zig/issues/25455

set(saucer_unexpected_hack ON)
