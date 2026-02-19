#include <omniscript/Backends/llvm/IRGenerator.h>
#include <omniscript/Backends/LLVM/ExternalFunctionResolvers/CLLVMResolver.h>

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
#include <llvm/Transforms/Utils/Cloning.h>
#include <llvm/Transforms/Utils/ValueMapper.h>

#ifdef _WIN32
    #define popen _popen
    #define pclose _pclose
#else
    #include <dlfcn.h>
    #include <unistd.h>
#endif

namespace Omniscript {
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

bool IRGenerator::symbolExistsInDLL(const std::string& libPath, const std::string& symbolName) {
    namespace fs = std::filesystem;
    
    // Validate file existence and extension
    if (!fs::exists(libPath)) {
        console.error("Library file does not exist: " + libPath);
        return false;
    }

    std::string extension = fs::path(libPath).extension().string();
    bool isDynamicLib = extension == ".dll" || extension == ".so" || extension == ".dylib";
    if (!isDynamicLib) {
        console.error("Not a recognized dynamic library format: " + extension);
        return false;
    }

#ifdef _WIN32
    // Windows: Use LoadLibrary and GetProcAddress
    HMODULE handle = LoadLibraryA(libPath.c_str());
    if (!handle) {
        console.error("Failed to load library: " + libPath + ", error code: " + std::to_string(GetLastError()));
        return false;
    }

    FARPROC symbol = GetProcAddress(handle, symbolName.c_str());
    bool found = (symbol != nullptr);
    
    FreeLibrary(handle);
    if (!found) {
        console.error("Symbol '" + symbolName + "' not found in dynamic library: " + libPath + ", error code: " + std::to_string(GetLastError()));
    }
    return found;
#else
    // Unix-like: Use dlopen/dlsym for SO/DYLIB
    void* handle = dlopen(libPath.c_str(), RTLD_LAZY);
    if (!handle) {
        console.error("Failed to open library: " + std::string(dlerror()));
        return false;
    }

    void* symbol = dlsym(handle, symbolName.c_str());
    bool found = (symbol != nullptr);
    
    dlclose(handle);
    if (!found) {
        console.error("Symbol '" + symbolName + "' not found in dynamic library: " + libPath);
    }
    return found;
#endif
}

bool IRGenerator::symbolExistsInStaticLib(const std::string& libPath, const std::string& symbolName) {
    namespace fs = std::filesystem;
    
    // Validate file existence and extension
    if (!fs::exists(libPath)) {
        console.error("Library file does not exist: " + libPath);
        return false;
    }

    std::string extension = fs::path(libPath).extension().string();
    if (extension != ".a" && extension != ".lib") {
        console.error("Not a recognized static library format: " + extension);
        return false;
    }

#ifdef _WIN32
    std::string command = "llvm-nm --defined-only \"" + libPath + "\" 2>&1";
#else
    std::string command = "nm -g \"" + libPath + "\" 2>&1";
#endif
    
    std::array<char, 512> buffer;
    std::string output;
    
    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) {
        console.error("Failed to run nm command");
        return false;
    }
    
    while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
        output += buffer.data();
    }
    
    pclose(pipe);
    
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
            init.value = generateCast(init.value, init.variable->getValueType());
            if (!init.value) {
                console.error("Failed to cast global initializer value.");
                continue;
            }
        }
        Builder->CreateStore(init.value, init.variable);
    }

    globalInitializers.clear();
}

bool IRGenerator::currentBlockHasTerminator() const {
    return Builder->GetInsertBlock()->getTerminator() != nullptr;
}

std::unique_ptr<llvm::Module> IRGenerator::cloneModule() {
    if (!Module) {
        throw std::runtime_error(formatError("No module to clone"));
    }

    // Create a new module with a cloned name
    std::string cloneName = Module->getName().str() + "_clone";
    auto clonedModule = std::make_unique<llvm::Module>(cloneName, *Context);

    // Copy module-level attributes
    clonedModule->setDataLayout(Module->getDataLayout());
    clonedModule->setTargetTriple(Module->getTargetTriple());
    clonedModule->setModuleInlineAsm(Module->getModuleInlineAsm());

    // Value mapping for resolving references during cloning
    llvm::ValueToValueMapTy valueMap;
    
    // Clone all globals, functions, and aliases
    for (auto& globalVar : Module->globals()) {
        llvm::GlobalVariable* newGlobal = new llvm::GlobalVariable(
            *clonedModule,
            globalVar.getValueType(),
            globalVar.isConstant(),
            globalVar.getLinkage(),
            nullptr, // initializer set later
            globalVar.getName()
        );
        newGlobal->copyAttributesFrom(&globalVar);
        valueMap[&globalVar] = newGlobal;
    }

    // Clone function declarations
    for (auto& func : Module->functions()) {
        llvm::Function* newFunc = llvm::Function::Create(
            func.getFunctionType(),
            func.getLinkage(),
            func.getName(),
            clonedModule.get()
        );
        newFunc->copyAttributesFrom(&func);
        
        // Map parameters
        auto newArgIt = newFunc->arg_begin();
        for (auto& arg : func.args()) {
            newArgIt->setName(arg.getName());
            valueMap[&arg] = &(*newArgIt);
            ++newArgIt;
        }
        valueMap[&func] = newFunc;
    }

    // Clone function bodies and global initializers
    for (auto& func : Module->functions()) {
        if (!func.isDeclaration()) {
            auto* newFunc = llvm::cast<llvm::Function>(valueMap[&func]);
            
            // Clone basic blocks
            for (auto& bb : func) {
                llvm::BasicBlock* newBB = llvm::BasicBlock::Create(
                    *Context, bb.getName(), newFunc);
                valueMap[&bb] = newBB;
            }
            
            // Clone instructions
            for (auto& bb : func) {
                auto* newBB = llvm::cast<llvm::BasicBlock>(valueMap[&bb]);
                for (auto& inst : bb) {
                    llvm::Instruction* newInst = inst.clone();
                    newInst->insertInto(newBB, newBB->end());
                    valueMap[&inst] = newInst;
                }
            }
        }
    }

    // Set global initializers
    for (auto& globalVar : Module->globals()) {
        if (globalVar.hasInitializer()) {
            auto* newGlobal = llvm::cast<llvm::GlobalVariable>(valueMap[&globalVar]);
            newGlobal->setInitializer(llvm::cast<llvm::Constant>(
                llvm::MapValue(globalVar.getInitializer(), valueMap)));
        }
    }

    // Remap all values in the cloned module
    for (auto& func : clonedModule->functions()) {
        if (!func.isDeclaration()) {
            std::vector<llvm::BasicBlock*> blocks;
            for (auto& bb : func) {
                blocks.push_back(&bb);
            }
            llvm::remapInstructionsInBlocks(blocks, valueMap);
        }
    }

    return clonedModule;
}

} // namespace Omniscript
