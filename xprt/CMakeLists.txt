# ============================================================
# xprt — X-Phage Runtime Static Library
# Compiled as C (not C++) — called from LLVM-generated code
# ============================================================
cmake_minimum_required(VERSION 3.18)

add_library(xprt STATIC xprt.c)
target_include_directories(xprt PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_options(xprt PRIVATE
    $<$<C_COMPILER_ID:GNU,Clang>:-O2 -Wall -Wextra -Wno-unused-parameter>
    $<$<C_COMPILER_ID:MSVC>:/O2 /W3>
)

# Export include path for parent project
set(XPRT_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR} PARENT_SCOPE)
set(XPRT_LIB        xprt                         PARENT_SCOPE)

install(TARGETS xprt DESTINATION lib)
install(FILES   xprt.h DESTINATION include/xprt)
