# X-Phage Tests v4.0.0

add_executable(test_lexer   test_lexer.cpp)
add_executable(test_parser  test_parser.cpp)

target_link_libraries(test_lexer  PRIVATE xphage_lexer  xphage_runtime)
target_link_libraries(test_parser PRIVATE xphage_parse  xphage_lexer  xphage_runtime)

add_test(NAME LexerTests  COMMAND test_lexer)
add_test(NAME ParserTests COMMAND test_parser)

# LLVM backend tests (only meaningful with ENABLE_LLVM=ON)
if(ENABLE_LLVM)
    add_executable(test_llvm_backend test_llvm_backend.cpp)
    target_include_directories(test_llvm_backend PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/compiler/xphage_lexer/include
        ${CMAKE_SOURCE_DIR}/compiler/xphage_parse/include
        ${CMAKE_SOURCE_DIR}/compiler/xphage_codegen_llvm/include
        ${LLVM_INCLUDE_DIRS})
    target_link_libraries(test_llvm_backend PRIVATE
        xphage_lexer xphage_parse xphage_codegen_llvm
        xphage_runtime ${LLVM_LIBS})
    target_compile_definitions(test_llvm_backend PRIVATE ENABLE_LLVM)
    add_test(NAME LLVMBackendTests COMMAND test_llvm_backend)
endif()
