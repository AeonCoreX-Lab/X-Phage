cmake_minimum_required(VERSION 3.20)
project(xphage VERSION 3.5.0 LANGUAGES CXX)

# ── Standards ────────────────────────────────────────────────
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ── Options ──────────────────────────────────────────────────
option(XPHAGE_ENABLE_LLVM    "Build with LLVM backend"            ON)
option(XPHAGE_ENABLE_TESTS   "Build test suite (xphage-test)"     ON)
option(XPHAGE_ENABLE_TOOLS   "Build LSP, doc, fmt tools"          ON)
option(XPHAGE_SANITIZE_ASAN  "Enable AddressSanitizer"            OFF)
option(XPHAGE_SANITIZE_UBSAN "Enable UBSanitizer"                 OFF)
option(XPHAGE_LTO            "Enable Link-Time Optimisation"      OFF)

# ── LLVM ─────────────────────────────────────────────────────
if(XPHAGE_ENABLE_LLVM)
    find_package(LLVM CONFIG QUIET)
    if(LLVM_FOUND)
        message(STATUS "LLVM ${LLVM_VERSION} found at ${LLVM_DIR}")
        add_definitions(${LLVM_DEFINITIONS} -DENABLE_LLVM)
        llvm_map_components_to_libnames(LLVM_LIBS
            core support targetparser
            x86codegen x86asmparser x86desc x86info
            aarch64codegen aarch64asmparser aarch64desc aarch64info
            passes irreader mc mcparser asmprinter)
    else()
        message(WARNING "LLVM not found — LLVM backend disabled")
        set(XPHAGE_ENABLE_LLVM OFF)
    endif()
endif()

# ── Sanitizers ───────────────────────────────────────────────
if(XPHAGE_SANITIZE_ASAN)
    add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address)
endif()
if(XPHAGE_SANITIZE_UBSAN)
    add_compile_options(-fsanitize=undefined)
    add_link_options(-fsanitize=undefined)
endif()

# ── LTO ──────────────────────────────────────────────────────
if(XPHAGE_LTO)
    include(CheckIPOSupported)
    check_ipo_supported(RESULT ipo_ok)
    if(ipo_ok)
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
    endif()
endif()

# ── Global include paths ──────────────────────────────────────
set(XPHAGE_INCLUDE_DIRS
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/compiler/xphage_driver/include
    ${CMAKE_SOURCE_DIR}/compiler/xphage_lexer/include
    ${CMAKE_SOURCE_DIR}/compiler/xphage_parse/include
    ${CMAKE_SOURCE_DIR}/compiler/xphage_ast/include
    ${CMAKE_SOURCE_DIR}/compiler/xphage_middle/include
    ${CMAKE_SOURCE_DIR}/compiler/xphage_codegen_llvm/include
    ${CMAKE_SOURCE_DIR}/compiler/xphage_codegen_transpiler/include
    ${CMAKE_SOURCE_DIR}/compiler/xphage_linker/include
    ${CMAKE_SOURCE_DIR}/compiler/xphage_interface/include
)
if(XPHAGE_ENABLE_LLVM)
    list(APPEND XPHAGE_INCLUDE_DIRS ${LLVM_INCLUDE_DIRS})
endif()

include_directories(${XPHAGE_INCLUDE_DIRS})

# ── Compiler pipeline static libraries ───────────────────────

# xphage_lexer
add_library(xphage_lexer STATIC
    compiler/xphage_lexer/src/lexer.cpp)
target_include_directories(xphage_lexer PUBLIC ${XPHAGE_INCLUDE_DIRS})

# xphage_ast
add_library(xphage_ast STATIC
    compiler/xphage_ast/src/ast.cpp)
target_include_directories(xphage_ast PUBLIC ${XPHAGE_INCLUDE_DIRS})
target_link_libraries(xphage_ast PUBLIC xphage_lexer)

# xphage_parse
add_library(xphage_parse STATIC
    compiler/xphage_parse/src/parser.cpp)
target_include_directories(xphage_parse PUBLIC ${XPHAGE_INCLUDE_DIRS})
target_link_libraries(xphage_parse PUBLIC xphage_lexer xphage_ast)

# xphage_middle (IR lowering)
add_library(xphage_middle STATIC
    compiler/xphage_middle/src/ir_lower.cpp)
target_include_directories(xphage_middle PUBLIC ${XPHAGE_INCLUDE_DIRS})
target_link_libraries(xphage_middle PUBLIC xphage_parse)

# xphage_codegen_transpiler
add_library(xphage_codegen_transpiler STATIC
    compiler/xphage_codegen_transpiler/src/transpiler.cpp)
target_include_directories(xphage_codegen_transpiler PUBLIC ${XPHAGE_INCLUDE_DIRS})

# xphage_codegen_llvm (conditional)
if(XPHAGE_ENABLE_LLVM)
    add_library(xphage_codegen_llvm STATIC
        compiler/xphage_codegen_llvm/src/llvm_compiler.cpp)
    target_include_directories(xphage_codegen_llvm PUBLIC
        ${XPHAGE_INCLUDE_DIRS} ${LLVM_INCLUDE_DIRS})
    target_link_libraries(xphage_codegen_llvm PUBLIC ${LLVM_LIBS})
endif()

# xphage_linker
add_library(xphage_linker STATIC
    compiler/xphage_linker/src/linker.cpp)
target_include_directories(xphage_linker PUBLIC ${XPHAGE_INCLUDE_DIRS})

# ── Runtime static library ────────────────────────────────────
add_library(xphage_runtime STATIC
    src/runtime/core_ops.cpp
    src/runtime/memory.cpp)
target_include_directories(xphage_runtime PUBLIC ${XPHAGE_INCLUDE_DIRS})

# ── Interface library (pipeline driver) ──────────────────────
add_library(xphage_interface STATIC
    compiler/xphage_interface/src/interface.cpp)
target_include_directories(xphage_interface PUBLIC ${XPHAGE_INCLUDE_DIRS})
target_link_libraries(xphage_interface PUBLIC
    xphage_middle
    xphage_lexer
    xphage_parse
    xphage_ast
    xphage_linker
    xphage_codegen_transpiler
    xphage_runtime
    $<$<BOOL:${XPHAGE_ENABLE_LLVM}>:xphage_codegen_llvm>)

# ── xphage binary ────────────────────────────────────────────
add_executable(xphage
    compiler/xphage_driver/src/main.cpp)
target_include_directories(xphage PRIVATE ${XPHAGE_INCLUDE_DIRS})
target_link_libraries(xphage PRIVATE xphage_interface)
if(UNIX)
    target_link_libraries(xphage PRIVATE pthread)
endif()

# ── XPM is a separate repository ─────────────────────────────
# Package management is handled by the standalone XPM tool.
# Install: curl -sL https://raw.githubusercontent.com/AeonCoreX-Lab/XPM/main/scripts/install.sh | bash
# Repo:    https://github.com/AeonCoreX-Lab/XPM

# ── Tools ────────────────────────────────────────────────────
if(XPHAGE_ENABLE_TOOLS)
    # xphage-test
    add_executable(xphage-test
        tools/xphage-test/src/main.cpp)
    target_include_directories(xphage-test PRIVATE ${XPHAGE_INCLUDE_DIRS})
    target_link_libraries(xphage-test PRIVATE xphage_runtime)
    if(UNIX)
        target_link_libraries(xphage-test PRIVATE pthread)
    endif()

    # xphage-fmt
    add_executable(xphage-fmt
        src/tools/xphage-fmt/src/main.cpp)
    target_include_directories(xphage-fmt PRIVATE ${XPHAGE_INCLUDE_DIRS})
    target_link_libraries(xphage-fmt PRIVATE xphage_runtime)

    # xphage-doc
    add_executable(xphage-doc
        tools/xphage-doc/src/main.cpp)
    target_include_directories(xphage-doc PRIVATE ${XPHAGE_INCLUDE_DIRS})
    target_link_libraries(xphage-doc PRIVATE xphage_runtime)

    # xphage-lsp
    add_executable(xphage-lsp
        tools/xphage-lsp/src/main.cpp)
    target_include_directories(xphage-lsp PRIVATE ${XPHAGE_INCLUDE_DIRS})
    target_link_libraries(xphage-lsp PRIVATE
        xphage_lexer xphage_parse xphage_runtime)
endif()

# ── Tests ────────────────────────────────────────────────────
if(XPHAGE_ENABLE_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# ── Install ───────────────────────────────────────────────────
install(TARGETS xphage DESTINATION bin)
if(XPHAGE_ENABLE_TOOLS)
    install(TARGETS xphage-test xphage-fmt xphage-doc xphage-lsp
            DESTINATION bin)
endif()
install(DIRECTORY library/       DESTINATION share/xphage/library)
install(DIRECTORY include/       DESTINATION include)
install(DIRECTORY scripts/       DESTINATION share/xphage/scripts
        USE_SOURCE_PERMISSIONS)

# ── Summary ───────────────────────────────────────────────────
message(STATUS "")
message(STATUS "╔══════════════════════════════════════════╗")
message(STATUS "║  X-Phage ${PROJECT_VERSION} — Build Configuration       ║")
message(STATUS "╠══════════════════════════════════════════╣")
message(STATUS "║  LLVM backend  : ${XPHAGE_ENABLE_LLVM}")
message(STATUS "║  Tools         : ${XPHAGE_ENABLE_TOOLS}")
message(STATUS "║  Tests         : ${XPHAGE_ENABLE_TESTS}")
message(STATUS "║  ASan          : ${XPHAGE_SANITIZE_ASAN}")
message(STATUS "║  UBSan         : ${XPHAGE_SANITIZE_UBSAN}")
message(STATUS "║  LTO           : ${XPHAGE_LTO}")
message(STATUS "╚══════════════════════════════════════════╝")
message(STATUS "")
