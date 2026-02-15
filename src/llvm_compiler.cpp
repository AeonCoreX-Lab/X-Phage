#include "../include/xphage.hpp"
#include <iostream>
#include <vector>
#include <string>
#include <system_error>

// --- LLVM Core Dependencies ---
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Verifier.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Constants.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/Host.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetOptions.h"
#include "llvm/IR/LegacyPassManager.h"

using namespace llvm;

/**
 * ⚡ X-Phage Titan LLVM Native Compiler v3.2
 * Architecture: Direct Machine Code Generation (AOT)
 * Ownership: AeonCoreX
 */
class XPhageLLVMCompiler {
private:
    std::unique_ptr<LLVMContext> Context;
    std::unique_ptr<Module> TheModule;
    std::unique_ptr<IRBuilder<>> Builder;
    std::unordered_map<std::string, Value*> GlobalMemory;
    Function* MainFunction;

    // External Runtime Functions (C-Bindings)
    FunctionCallee PrintfFunc;
    FunctionCallee SystemCallFunc;

public:
    XPhageLLVMCompiler() {
        // 1. Initialize LLVM Targets for Cross-Compilation
        InitializeAllTargetInfos();
        InitializeAllTargets();
        InitializeAllTargetMCs();
        InitializeAllAsmParsers();
        InitializeAllAsmPrinters();

        // 2. Setup Core Context & Module
        Context = std::make_unique<LLVMContext>();
        TheModule = std::make_unique<Module>("X-Phage-Omni-Native", *Context);
        Builder = std::make_unique<IRBuilder<>>(*Context);

        setup_external_functions();
    }

    void setup_external_functions() {
        // Setup printf for 'beam' command
        FunctionType* printfType = FunctionType::get(IntegerType::getInt32Ty(*Context), PointerType::getUnqual(Type::getInt8Ty(*Context)), true);
        PrintfFunc = TheModule->getOrInsertFunction("printf", printfType);

        // Setup system() for 'bypass' kernel injection commands
        FunctionType* sysType = FunctionType::get(IntegerType::getInt32Ty(*Context), PointerType::getUnqual(Type::getInt8Ty(*Context)), false);
        SystemCallFunc = TheModule->getOrInsertFunction("system", sysType);
    }

    // --- IR GENERATION CORE ---
    void compile_tokens(const std::vector<Token>& tokens, std::string output_filename) {
        std::cout << "\033[1;35m[LLVM-CORE] ⚛ Initiating Native Compilation Matrix...\033[0m\n";

        // Create int main() entry point for the executable
        FunctionType* mainType = FunctionType::get(Builder->getInt32Ty(), false);
        MainFunction = Function::Create(mainType, Function::ExternalLinkage, "main", TheModule.get());
        BasicBlock* entryBlock = BasicBlock::Create(*Context, "entry", MainFunction);
        Builder->SetInsertPoint(entryBlock);

        // Parse Tokens and generate Native Instructions
        for (size_t i = 0; i < tokens.size(); ++i) {
            
            // 1. GLOBAL VARIABLE DEFINITION
            if (tokens[i].type == GLOBAL && i + 3 < tokens.size()) {
                std::string var_name = tokens[i+1].value;
                std::string var_value = tokens[i+3].value;
                
                // Create native global string constant
                Value* strVal = Builder->CreateGlobalStringPtr(var_value, var_name + "_str");
                GlobalMemory[var_name] = strVal;
                
                std::cout << "  ├─ \033[1;32mAllocated Global Matrix:\033[0m " << var_name << "\n";
                i += 3;
            }
            
            // 2. BEAM (Print to stdout)
            else if (tokens[i].type == BEAM && i + 1 < tokens.size()) {
                std::string target = tokens[i+1].value;
                Value* formatStr = Builder->CreateGlobalStringPtr("%s\n", "fmt");
                Value* targetVal;

                if (GlobalMemory.count(target)) {
                    targetVal = GlobalMemory[target];
                } else {
                    targetVal = Builder->CreateGlobalStringPtr(target, target + "_raw");
                }

                // Inject Printf Call into Native Binary
                Builder->CreateCall(PrintfFunc, {formatStr, targetVal});
                i++;
            }

            // 3. BYPASS (Kernel/System Call execution)
            else if (tokens[i].type == BYPASS && i + 1 < tokens.size()) {
                std::string cmd = tokens[i+1].value;
                
                // Convert bypass command to a direct OS shell/kernel invocation
                Value* sysCmdStr = Builder->CreateGlobalStringPtr(cmd, "sys_cmd");
                Builder->CreateCall(SystemCallFunc, {sysCmdStr});
                
                std::cout << "  ├─ \033[1;31mInjected Native Bypass Hook:\033[0m " << cmd << "\n";
                i++;
            }
        }

        // Return 0 for main function
        Builder->CreateRet(Builder->getInt32(0));

        // Verify the generated IR Code
        if (verifyFunction(*MainFunction, &errs())) {
            std::cerr << "\033[1;31m[LLVM FATAL] ⛔ IR Verification Failed!\033[0m\n";
            return;
        }

        std::cout << "\033[1;36m[LLVM-CORE] ✔ Intermediate Representation (IR) Synthesized.\033[0m\n";
        emit_object_file(output_filename);
    }

    // --- MACHINE CODE GENERATION ---
    void emit_object_file(std::string output_filename) {
        auto TargetTriple = sys::getDefaultTargetTriple();
        TheModule->setTargetTriple(TargetTriple);

        std::string Error;
        auto Target = TargetRegistry::lookupTarget(TargetTriple, Error);

        if (!Target) {
            std::cerr << "[LLVM FATAL] " << Error << "\n";
            return;
        }

        auto CPU = "generic";
        auto Features = "";
        TargetOptions opt;
        auto RM = Optional<Reloc::Model>();
        
        auto TargetMachine = Target->createTargetMachine(TargetTriple, CPU, Features, opt, RM);
        TheModule->setDataLayout(TargetMachine->createDataLayout());

        std::error_code EC;
        raw_fd_ostream dest(output_filename, EC, sys::fs::OF_None);

        if (EC) {
            std::cerr << "[LLVM FATAL] Could not open file: " << EC.message() << "\n";
            return;
        }

        legacy::PassManager pass;
        auto FileType = CGFT_ObjectFile; // Generate .o object file

        if (TargetMachine->addPassesToEmitFile(pass, dest, nullptr, FileType)) {
            std::cerr << "[LLVM FATAL] TargetMachine can't emit a file of this type.\n";
            return;
        }

        pass.run(*TheModule);
        dest.flush();

        std::cout << "\033[1;32m[NATIVE BUILD] 🚀 Successfully emitted native machine code: \033[1;37m" << output_filename << "\033[0m\n";
    }
};

// --- ENTRY POINT FOR NATIVE COMPILATION ---
void compile_to_native(std::string source_file) {
    std::ifstream file(source_file);
    if (!file.is_open()) {
        std::cerr << "File Error: " << source_file << "\n";
        return;
    }
    std::stringstream buffer;
    buffer << file.rdbuf();

    XPhageLexer lexer;
    std::vector<Token> tokens = lexer.tokenize(buffer.str());

    XPhageLLVMCompiler llvm_compiler;
    
    // Generate output.o object file
    std::string obj_file = "output.o";
    llvm_compiler.compile_tokens(tokens, obj_file);
    
    // Link object file to final executable using clang
    std::cout << "\033[1;33m[LINKER] 🔗 Linking object file to binary executable...\033[0m\n";
    std::string link_cmd = "clang output.o -o output_executable";
    int res = std::system(link_cmd.c_str());
    
    if (res == 0) {
        std::cout << "\033[1;32m[SUCCESS] 💠 Native Compilation Complete. Run ./output_executable \033[0m\n";
    }
}
