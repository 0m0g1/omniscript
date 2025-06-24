#include <omniscript/engine/Backends/llvm/IRGenerator.h>
#include <omniscript/engine/Backends/LLVM/ExternalFunctionResolvers/CLLVMResolver.h>

#include <llvm/ADT/StringMap.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DataLayout.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/PassManager.h>
#include <llvm/IR/Verifier.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/Support/Alignment.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/TargetParser/Host.h>
#include <llvm/Linker/Linker.h>

IRGenerator::IRGenerator(const Config& configs) {
    this->configs = configs;
    Context = std::make_unique<llvm::LLVMContext>();
    Module = std::make_unique<llvm::Module>(configs.filePath, *Context);
    Builder = std::make_unique<llvm::IRBuilder<>>(*Context);
    initialize();
}

void IRGenerator::setupModuleMetadata() {
    DEBUG_LOG("Setting up the module's metadata");
    // Add module metadata based on configuration
    llvm::LLVMContext& ctx = *Context;
    
    // Add source file information
    if (!configs.filePath.empty()) {
        llvm::Metadata* sourceFile = llvm::MDString::get(ctx, configs.filePath);
        Module->addModuleFlag(llvm::Module::Warning, "source.file", sourceFile);
    }
    
    // Add language standard information
    llvm::Metadata* langStd = llvm::MDString::get(ctx, configs.languageStandard);
    Module->addModuleFlag(llvm::Module::Warning, "language.standard", langStd);
    
    // Add compilation mode
    std::string modeStr = configs.isJITMode() ? "jit" : "aot";
    llvm::Metadata* mode = llvm::MDString::get(ctx, modeStr);
    Module->addModuleFlag(llvm::Module::Warning, "compile.mode", mode);
    
    // Add safety level
    llvm::Metadata* safety = llvm::ConstantAsMetadata::get(
        llvm::ConstantInt::get(llvm::Type::getInt32Ty(ctx), static_cast<int>(configs.runtime.safetyLevel))
    );
    Module->addModuleFlag(llvm::Module::Warning, "safety.level", safety);
    
    // Add PIC flag if enabled
    if (configs.security.enablePositionIndependentCode) {
        llvm::Metadata* pic = llvm::ConstantAsMetadata::get(
            llvm::ConstantInt::get(llvm::Type::getInt1Ty(ctx), 1)
        );
        Module->addModuleFlag(llvm::Module::Error, "PIC Level", pic);
    }
}

void IRGenerator::setupExternalResolvers() {
    DEBUG_LOG("Setting up the external resolvers");
    // Set up external resolvers based on target OS
    auto targetOS = configs.resolveTargetOS();
    
    // Always add C standard library resolver
    addExternalResolver("C", std::make_unique<CStdLibResolver>());
    
    // Add OS-specific resolvers
    // switch (targetOS) {
    //     case TargetOS::Windows:
    //         addExternalResolver("Win32", std::make_unique<Win32Resolver>());
    //         break;
    //     case TargetOS::Linux:
    //     case TargetOS::Ubuntu:
    //     case TargetOS::Debian:
    //         addExternalResolver("POSIX", std::make_unique<PosixResolver>());
    //         addExternalResolver("Linux", std::make_unique<LinuxResolver>());
    //         break;
    //     case TargetOS::macOS:
    //         addExternalResolver("POSIX", std::make_unique<PosixResolver>());
    //         addExternalResolver("Darwin", std::make_unique<DarwinResolver>());
    //         break;
    //     case TargetOS::WebAssembly:
    //         addExternalResolver("WASM", std::make_unique<WasmResolver>());
    //         break;
    //     default:
    //         // Use generic POSIX for unknown Unix-like systems
    //         if (configs.isUnixLikeOS()) {
    //             addExternalResolver("POSIX", std::make_unique<PosixResolver>());
    //         }
    //         break;
    // }
    
    // Add plugin-based resolvers if specified
    for (const auto& plugin : configs.plugins) {
        // This would load and initialize plugin-based resolvers
        // Implementation depends on your plugin system
        // loadPluginResolver(plugin);
    }
}


bool IRGenerator::symbolExistsInStaticLib(const std::string& libPath, const std::string& symbolName) {
    std::string command = "llvm-nm \"" + libPath + "\" 2>&1";

    std::array<char, 512> buffer;
    std::string output;

    FILE* pipe = _popen(command.c_str(), "r");
    if (!pipe) {
        console.error("Failed to run llvm-nm");
    }

    while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
        output += buffer.data();
    }

    _pclose(pipe);

    return output.find(symbolName) != std::string::npos;
}

void IRGenerator::compileAllFunctionBodies(SymbolTableType scope) {
    for (const auto& func : userDefinedFunctions) {
        DEBUG_LOG("Generating body for function: " + func->name + " (mangled: " + func->mangledName + ")");
        auto llvmFunc = currentModule->getFunction(func->mangledName);
        if (!llvmFunc) {
            console.error("Function not found in module during body generation: " + func->mangledName);
            continue;
        }

        generateFunctionBody(
            func->mangledName,
            llvmFunc,
            func->parameters,
            func->body,
            scope
        );
    }
}

void IRGenerator::scheduleGlobalInitialization(
    const std::string& name,
    llvm::GlobalVariable* gVar,
    llvm::Value* initialValue
) {
    // Create initialization in global constructor
    llvm::IRBuilder<> initBuilder(
        Module->getContext()
    );
    auto* initFunc = getOrCreateGlobalInitFunction();
    auto* entry = &initFunc->getEntryBlock();
    
    if (entry->empty()) {
        initBuilder.SetInsertPoint(entry);
    } else {
        initBuilder.SetInsertPoint(
            entry, 
            std::prev(entry->end())
        );
    }

    // Store the initial value
    initBuilder.CreateStore(initialValue, gVar);
}

void IRGenerator::finalizeGlobalInitializers() {
    if (globalInitializers.empty()) return;

    auto* func = getOrCreateGlobalInitFunction();
    auto* entry = &func->getEntryBlock();
    
    // Set insertion point at end of entry block
    Builder->SetInsertPoint(entry, entry->end());

    // Emit all initializers
    for (auto& init : globalInitializers) {
        if (init.value->getType() != init.variable->getValueType()) {
            init.value = Builder->CreateBitCast(init.value, init.variable->getValueType());
        }
        Builder->CreateStore(init.value, init.variable);
    }

    globalInitializers.clear();
}

bool IRGenerator::currentBlockHasTerminator() const {
    return Builder->GetInsertBlock()->getTerminator() != nullptr;
}