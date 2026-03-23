#include "../include/xphage.hpp"
#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <optional> // 🔧 NEW: Added for modern C++ compatibility

#ifdef ENABLE_LLVM

// --- LLVM Core Dependencies ---
#include <llvm/Config/llvm-config.h> // 🔧 NEW: Detects LLVM version automatically
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Verifier.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Type.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetOptions.h"
#include "llvm/IR/LegacyPassManager.h"

// 🔧 FIX 1: LLVM 17+ moved Host.h to TargetParser
#if LLVM_VERSION_MAJOR >= 17
    #include "llvm/TargetParser/Host.h"
    #include "llvm/TargetParser/Triple.h"
#else
    #include "llvm/Support/Host.h"
    #include "llvm/ADT/Triple.h"
#endif

// 🔧 FIX 2: LLVM 18+ moved CodeGenFileType into CodeGen.h
#if LLVM_VERSION_MAJOR >= 18
    #include "llvm/Support/CodeGen.h"
#endif

using namespace llvm;

/**
 * ⚡ X-Phage Titan LLVM Native Compiler v3.5
 * Architecture: Direct Machine Code Generation (AOT)
 * Ownership: AeonCoreX
 */
class XPhageLLVMCompilerImpl {
private:
    std::unique_ptr<LLVMContext> Context;
    std::unique_ptr<Module> TheModule;
    std::unique_ptr<IRBuilder<>> Builder;
    std::unordered_map<std::string, Value*> GlobalMemory;
    Function* MainFunction;
    FunctionCallee PrintfFunc;
    FunctionCallee SystemCallFunc;

public:
    XPhageLLVMCompilerImpl() {
        InitializeAllTargetInfos();
        InitializeAllTargets();
        InitializeAllTargetMCs();
        InitializeAllAsmParsers();
        InitializeAllAsmPrinters();

        Context = std::make_unique<LLVMContext>();
        TheModule = std::make_unique<Module>("X-Phage-Omni-Native", *Context);
        Builder = std::make_unique<IRBuilder<>>(*Context);

        setup_external_functions();
    }

    void setup_external_functions() {
        // 🔧 FIX 3: LLVM 17+ fully adopted Opaque Pointers. getUnqual no longer takes a type.
        #if LLVM_VERSION_MAJOR >= 17
            FunctionType* printfType = FunctionType::get(IntegerType::getInt32Ty(*Context), PointerType::getUnqual(*Context), true);
            FunctionType* sysType = FunctionType::get(IntegerType::getInt32Ty(*Context), PointerType::getUnqual(*Context), false);
        #else
            FunctionType* printfType = FunctionType::get(IntegerType::getInt32Ty(*Context), PointerType::getUnqual(Type::getInt8Ty(*Context)), true);
            FunctionType* sysType = FunctionType::get(IntegerType::getInt32Ty(*Context), PointerType::getUnqual(Type::getInt8Ty(*Context)), false);
        #endif
        
        PrintfFunc = TheModule->getOrInsertFunction("printf", printfType);
        SystemCallFunc = TheModule->getOrInsertFunction("system", sysType);
    }

    void compile_tokens(const std::vector<Token>& tokens, std::string output_filename) {
        std::cout << "\033[1;35m[LLVM-CORE] ⚛ Initiating Native Compilation Matrix...\033[0m\n";

        FunctionType* mainType = FunctionType::get(Builder->getInt32Ty(), false);
        MainFunction = Function::Create(mainType, Function::ExternalLinkage, "main", TheModule.get());
        BasicBlock* entryBlock = BasicBlock::Create(*Context, "entry", MainFunction);
        Builder->SetInsertPoint(entryBlock);

        for (size_t i = 0; i < tokens.size(); ++i) {
            if (tokens[i].type == GLOBAL && i + 3 < tokens.size()) {
                std::string var_name = tokens[i+1].value;
                std::string var_value = tokens[i+3].value;
                
                // 🔧 FIX 4: LLVM 20+ deprecated CreateGlobalStringPtr
                #if LLVM_VERSION_MAJOR >= 20
                    Value* strVal = Builder->CreateGlobalString(var_value, var_name + "_str");
                #else
                    Value* strVal = Builder->CreateGlobalStringPtr(var_value, var_name + "_str");
                #endif
                GlobalMemory[var_name] = strVal;
                i += 3;
            }
            else if (tokens[i].type == BEAM && i + 1 < tokens.size()) {
                std::string target = tokens[i+1].value;
                
                #if LLVM_VERSION_MAJOR >= 20
                    Value* formatStr = Builder->CreateGlobalString("%s\n", "fmt");
                #else
                    Value* formatStr = Builder->CreateGlobalStringPtr("%s\n", "fmt");
                #endif

                Value* targetVal;
                if (GlobalMemory.count(target)) {
                    targetVal = GlobalMemory[target];
                } else {
                    #if LLVM_VERSION_MAJOR >= 20
                        targetVal = Builder->CreateGlobalString(target, target + "_raw");
                    #else
                        targetVal = Builder->CreateGlobalStringPtr(target, target + "_raw");
                    #endif
                }
                Builder->CreateCall(PrintfFunc, {formatStr, targetVal});
                i++;
            }
            else if (tokens[i].type == BYPASS && i + 1 < tokens.size()) {
                #if LLVM_VERSION_MAJOR >= 20
                    Value* sysCmdStr = Builder->CreateGlobalString(tokens[i+1].value, "sys_cmd");
                #else
                    Value* sysCmdStr = Builder->CreateGlobalStringPtr(tokens[i+1].value, "sys_cmd");
                #endif
                
                Builder->CreateCall(SystemCallFunc, {sysCmdStr});
                i++;
            }
        }

        Builder->CreateRet(Builder->getInt32(0));
        if (verifyFunction(*MainFunction, &errs())) {
            std::cerr << "\033[1;31m[LLVM FATAL] ⛔ IR Verification Failed!\033[0m\n";
            return;
        }
        emit_object_file(output_filename);
    }

    void emit_object_file(std::string output_filename) {
        std::string TripleStr = sys::getDefaultTargetTriple();
        Triple TheTriple(TripleStr);

        // 🔧 FIX 5: LLVM 22+ enforces passing the Triple object instead of a string
        #if LLVM_VERSION_MAJOR >= 22
            TheModule->setTargetTriple(TheTriple);
        #else
            TheModule->setTargetTriple(TripleStr);
        #endif

        std::string Error;
        
        #if LLVM_VERSION_MAJOR >= 22
            auto Target = TargetRegistry::lookupTarget(TheTriple, Error);
        #else
            auto Target = TargetRegistry::lookupTarget(TripleStr, Error);
        #endif

        if (!Target) {
            std::cerr << "[LLVM FATAL] " << Error << "\n";
            return;
        }

        auto CPU = "generic";
        auto Features = "";
        TargetOptions opt;
        
        // 🔧 FIX 6: LLVM 16+ removed llvm::Optional in favor of std::optional
        #if LLVM_VERSION_MAJOR >= 16
            auto RM = std::optional<Reloc::Model>();
        #else
            auto RM = Optional<Reloc::Model>();
        #endif

        #if LLVM_VERSION_MAJOR >= 22
            auto TargetMachine = Target->createTargetMachine(TheTriple, CPU, Features, opt, RM);
        #else
            auto TargetMachine = Target->createTargetMachine(TripleStr, CPU, Features, opt, RM);
        #endif
        
        TheModule->setDataLayout(TargetMachine->createDataLayout());

        std::error_code EC;
        raw_fd_ostream dest(output_filename, EC, sys::fs::OF_None);
        if (EC) {
            std::cerr << "[LLVM FATAL] Could not open file: " << EC.message() << "\n";
            return;
        }

        legacy::PassManager pass;
        
        // 🔧 FIX 7: LLVM 18+ changed CGFT_ObjectFile to CodeGenFileType::ObjectFile
        #if LLVM_VERSION_MAJOR >= 18
            auto FileType = CodeGenFileType::ObjectFile;
        #else
            auto FileType = CGFT_ObjectFile;
        #endif

        if (TargetMachine->addPassesToEmitFile(pass, dest, nullptr, FileType)) {
            std::cerr << "[LLVM FATAL] TargetMachine can't emit a file of this type.\n";
            return;
        }
        pass.run(*TheModule);
        dest.flush();
        std::cout << "\033[1;32m[NATIVE BUILD] 🚀 Generated machine code: " << output_filename << "\033[0m\n";
    }
};

void XPhageLLVMCompiler::compile_tokens(const std::vector<Token>& tokens, std::string output_obj) {
    XPhageLLVMCompilerImpl impl;
    impl.compile_tokens(tokens, output_obj);
}

#else

// --- STUB IMPLEMENTATION (Safe Fallback) ---
void XPhageLLVMCompiler::compile_tokens(const std::vector<Token>& tokens, std::string output_obj) {
    std::cout << "\033[1;31m[ERROR] ⛔ LLVM Backend is disabled in this build.\033[0m\n";
    std::cout << "        Please rebuild with valid LLVM libraries found or use Transpiler Mode.\n";
}

#endif
