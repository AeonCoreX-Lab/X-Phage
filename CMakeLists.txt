# ============================================================
# X-Phage Compiler v4.0.0 — CMakeLists.txt
# Phase 1-3: C++17 Transpiler  |  Phase 4: LLVM Native
# AeonCoreX Lab
# ============================================================
cmake_minimum_required(VERSION 3.18)
project(X-Phage VERSION 4.0.0 LANGUAGES C CXX)

set(CMAKE_C_STANDARD   11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED   ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# ── Compiler flags ────────────────────────────────────────────
if(MSVC)
    add_compile_options(/W4 /WX- /EHsc)
else()
    add_compile_options(
        -Wall -Wextra -Wno-unused-parameter -Wno-unused-function
    )
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_compile_options(-g -O0 -fsanitize=address,undefined)
        add_link_options(-fsanitize=address,undefined)
    else()
        add_compile_options(-O2)
    endif()
endif()

# ── Global includes ───────────────────────────────────────────
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/xprt
    ${CMAKE_SOURCE_DIR}/compiler/xphage_lexer/include
    ${CMAKE_SOURCE_DIR}/compiler/xphage_parse/include
    ${CMAKE_SOURCE_DIR}/compiler/xphage_middle/include
    ${CMAKE_SOURCE_DIR}/compiler/xphage_codegen_transpiler/include
    ${CMAKE_SOURCE_DIR}/compiler/xphage_codegen_llvm/include
    ${CMAKE_SOURCE_DIR}/compiler/xphage_interface/include
    ${CMAKE_SOURCE_DIR}/compiler/xphage_linker/include
)

# ── Platform libs ─────────────────────────────────────────────
if(UNIX AND NOT APPLE)
    set(PLATFORM_LIBS pthread stdc++fs m)
elseif(APPLE)
    set(PLATFORM_LIBS pthread "-framework Foundation" m)
elseif(WIN32)
    set(PLATFORM_LIBS ws2_32)
endif()

# ── xprt runtime ─────────────────────────────────────────────
add_subdirectory(xprt)

# ── Compiler modules ─────────────────────────────────────────
add_library(xphage_runtime STATIC
    src/runtime/core_ops.cpp
    src/runtime/memory.cpp
)
target_include_directories(xphage_runtime PUBLIC ${CMAKE_SOURCE_DIR}/include)

add_library(xphage_lexer STATIC
    compiler/xphage_lexer/src/lexer.cpp)
target_include_directories(xphage_lexer PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/compiler/xphage_lexer/include)

add_library(xphage_parse STATIC
    compiler/xphage_parse/src/parser.cpp)
target_include_directories(xphage_parse PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/compiler/xphage_lexer/include
    ${CMAKE_SOURCE_DIR}/compiler/xphage_parse/include)
target_link_libraries(xphage_parse PRIVATE xphage_lexer)

add_library(xphage_middle STATIC
    compiler/xphage_middle/src/ir_lower.cpp)
target_include_directories(xphage_middle PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/compiler/xphage_middle/include)

add_library(xphage_codegen_transpiler STATIC
    compiler/xphage_codegen_transpiler/src/transpiler.cpp)
target_include_directories(xphage_codegen_transpiler PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/compiler/xphage_codegen_transpiler/include)

add_library(xphage_linker STATIC
    compiler/xphage_linker/src/linker.cpp)
target_include_directories(xphage_linker PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/compiler/xphage_linker/include)
target_link_libraries(xphage_linker PRIVATE xphage_runtime)

# ── LLVM Backend (Phase 4, optional) ─────────────────────────
option(ENABLE_LLVM "Enable LLVM native backend (Phase 4)" OFF)

add_library(xphage_codegen_llvm STATIC
    compiler/xphage_codegen_llvm/src/llvm_backend.cpp)
target_include_directories(xphage_codegen_llvm PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/compiler/xphage_codegen_llvm/include)

if(ENABLE_LLVM)
    find_package(LLVM REQUIRED CONFIG)
    message(STATUS "LLVM ${LLVM_PACKAGE_VERSION} found at ${LLVM_DIR}")
    include_directories(${LLVM_INCLUDE_DIRS})
    add_definitions(${LLVM_DEFINITIONS} -DENABLE_LLVM)

    # Strip LLVM flags that break C++ exception/RTTI
    if(DEFINED LLVM_DEFINITIONS)
        string(REPLACE "-fno-exceptions" "" LLVM_DEFINITIONS "${LLVM_DEFINITIONS}")
        string(REPLACE "-fno-rtti"       "" LLVM_DEFINITIONS "${LLVM_DEFINITIONS}")
    endif()

    llvm_map_components_to_libnames(LLVM_LIBS
        core support irreader passes analysis nativecodegen
        native x86codegen aarch64codegen webassembly
        linker lto)

    target_link_libraries(xphage_codegen_llvm PUBLIC ${LLVM_LIBS})
    target_compile_definitions(xphage_codegen_llvm PUBLIC ENABLE_LLVM)
endif()

# ── Interface (pipeline driver) ───────────────────────────────
add_library(xphage_interface STATIC
    compiler/xphage_interface/src/interface.cpp)
target_include_directories(xphage_interface PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/compiler/xphage_interface/include
    ${CMAKE_SOURCE_DIR}/compiler/xphage_lexer/include
    ${CMAKE_SOURCE_DIR}/compiler/xphage_parse/include
    ${CMAKE_SOURCE_DIR}/compiler/xphage_middle/include
    ${CMAKE_SOURCE_DIR}/compiler/xphage_codegen_transpiler/include
    ${CMAKE_SOURCE_DIR}/compiler/xphage_codegen_llvm/include
    ${CMAKE_SOURCE_DIR}/compiler/xphage_linker/include)
target_link_libraries(xphage_interface PRIVATE
    xphage_lexer xphage_parse xphage_middle
    xphage_codegen_transpiler xphage_codegen_llvm
    xphage_runtime xphage_linker xprt ${PLATFORM_LIBS})

# ── Main binary ───────────────────────────────────────────────
add_executable(xphage compiler/xphage_driver/src/main.cpp)
target_include_directories(xphage PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/compiler/xphage_interface/include
    ${CMAKE_SOURCE_DIR}/compiler/xphage_linker/include)
target_link_libraries(xphage PRIVATE
    xphage_interface xphage_runtime xphage_linker xprt ${PLATFORM_LIBS})

# ── Install ───────────────────────────────────────────────────
install(TARGETS xphage      DESTINATION bin)
install(TARGETS xprt        DESTINATION lib)
install(DIRECTORY library/  DESTINATION share/xphage/library)
install(DIRECTORY include/  DESTINATION include)
install(FILES xprt/xprt.h   DESTINATION include/xprt)

# ── Tests ────────────────────────────────────────────────────
enable_testing()
if(EXISTS "${CMAKE_SOURCE_DIR}/tests/CMakeLists.txt")
    add_subdirectory(tests)
endif()

# ── Summary ──────────────────────────────────────────────────
message(STATUS "")
message(STATUS "  ╔═══════════════════════════════════════╗")
message(STATUS "  ║  X-Phage Compiler v4.0.0              ║")
message(STATUS "  ║  Build  : ${CMAKE_BUILD_TYPE}                     ")
message(STATUS "  ║  LLVM   : ${ENABLE_LLVM}                    ")
message(STATUS "  ║  xprt   : ON (always built)           ║")
message(STATUS "  ╚═══════════════════════════════════════╝")
message(STATUS "")
