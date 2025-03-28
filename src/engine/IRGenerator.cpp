#include <omniscript/engine/IRGenerator.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/IR/PassManager.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Passes/StandardInstrumentations.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Linker/Linker.h>
#include <llvm/IR/DataLayout.h>
#include <llvm/Support/Alignment.h>
#include <llvm/ADT/StringMap.h>         // Needed for getHostCPUFeatures

#include <omniscript/debuggingtools/console.h>

IRGenerator::IRGenerator(const std::string& mainModulePath) {
    Context = std::make_unique<llvm::LLVMContext>();
    Module = std::make_unique<llvm::Module>(mainModulePath, *Context);
    Builder = std::make_unique<llvm::IRBuilder<>>(*Context);
    initialize();
}

bool IRGenerator::supportsAVX512() {
    // llvm::StringMap<bool> Features;
    // llvm::sys::getHostCPUFeatures(Features);
    // return Features.lookup("avx512f"); // Check if AVX-512 is supported
    return false;
}

bool IRGenerator::supportsAVX2() {
    // llvm::StringMap<bool> Features;
    // llvm::sys::getHostCPUFeatures(Features);
    // return Features.lookup("avx2"); // Check if AVX2 is supported
    return false;
}

void IRGenerator::initialize() {
    if (!Context) {
        Context = std::make_unique<llvm::LLVMContext>();
    }
    if (!Module) {
        Module = std::make_unique<llvm::Module>("OmniScript", *Context);
    }
    if (!Builder) {
        Builder = std::make_unique<llvm::IRBuilder<>>(*Context);
    }

    // Check if there are any functions in the module
    if (Module->empty()) {
        // No functions exist, create a top-level entry block for global execution
        llvm::FunctionType* funcType = llvm::FunctionType::get(llvm::Type::getVoidTy(*Context), false);
        llvm::Function* topLevelFunc = llvm::Function::Create(funcType, llvm::Function::ExternalLinkage, "__top_level__", Module.get());
        llvm::BasicBlock* entryBlock = llvm::BasicBlock::Create(*Context, "entry", topLevelFunc);
        Builder->SetInsertPoint(entryBlock);
        Builder->CreateRetVoid(); 
    } else {
        // There are existing functions, check if "main" exists
        llvm::Function* function = Module->getFunction("main");
        if (function && function->empty()) {
            // If main exists but is empty, create an entry block
            llvm::BasicBlock* entryBlock = llvm::BasicBlock::Create(*Context, "entry", function);
            Builder->SetInsertPoint(entryBlock);
        } else if (function) {
            // If main already has an entry block, set the insert point there
            Builder->SetInsertPoint(&function->getEntryBlock());
        } else {
            // If other functions exist, set insert point to the first function's entry
            Builder->SetInsertPoint(&Module->begin()->getEntryBlock());
        }
    }
}

void IRGenerator::printIR() {
    Module->print(llvm::outs(), nullptr);
}

void IRGenerator::printErrors() {
    if (llvm::verifyModule(*Module, &llvm::errs())) {
        llvm::errs() << "Module verification for '" << Module->getModuleIdentifier() << "' failed!\n";
    } else {
        llvm::errs() << "No errors found in '" << Module->getModuleIdentifier() << "'.\n";
    }
}

void IRGenerator::printErrors(llvm::Module& module) {
    if (llvm::verifyModule(module, &llvm::errs())) {
        llvm::errs() << "Module verification for '" << module.getModuleIdentifier() << " failed! '\n";
        module.print(llvm::outs(), nullptr);
    } else {
        llvm::errs() << "No errors found in: '" << module.getModuleIdentifier() << "'.\n";
    }
}

void IRGenerator::optimizeModule(int level) {
    console.log("No optimization taking place");
    // console.log("Running module verification before optimization...");

    // // Use VerifierAnalysis for LLVM 15+
    // if (llvm::verifyModule(*Module, &llvm::errs())) {
    //     throw std::runtime_error("Module verification failed before optimization");
    // }

    // llvm::LoopAnalysisManager lam;
    // llvm::FunctionAnalysisManager fam;
    // llvm::CGSCCAnalysisManager cam;
    // llvm::ModuleAnalysisManager mam;
    
    // llvm::PassBuilder pb;
    // pb.registerModuleAnalyses(mam);
    // pb.registerFunctionAnalyses(fam);
    // pb.registerLoopAnalyses(lam);
    // pb.registerCGSCCAnalyses(cam);
    
    // llvm::ModulePassManager mpm = pb.buildPerModuleDefaultPipeline(llvm::OptimizationLevel::O2);

    
    // mpm.run(*Module, mam);
    
    // console.log("Optimized Code:\n");
    // printIR();

    // console.log("Errors in Optimized Code:\n");
    // printErrors();
}

void IRGenerator::generateModule(
    const std::string& modulePath,
    const std::string& alias,
    const std::vector<std::shared_ptr<Statement>>& statements,
    const std::unordered_map<std::string, std::string>& importedAliases, // Alias -> Original
    bool importAll
) {
    
    DEBUG_LOG("Generating module: " + modulePath);
    
    if (scope->moduleExists(modulePath)) {
        DEBUG_LOG("Module " + modulePath + " is already generated. Skipping.");
        return;
    }

    // 🔹 Store public members in a new module symbol table
    pushScope();

    auto newModule = std::make_unique<llvm::Module>(modulePath, *Context);
    llvm::Module* previousModule = CurrentModule;
    CurrentModule = newModule.get();

    std::unordered_map<std::string, llvm::Value*> publicMembers;

    for (const auto& statement : statements) {
        if (auto moduleStatement = std::dynamic_pointer_cast<CreateModule>(statement)) {
            DEBUG_LOG("Generating submodule: " + moduleStatement->getName());
            generateModule(moduleStatement->getName(), moduleStatement->getName(), moduleStatement->getStatements(), importedAliases, false);

        } else if (auto publicStatement = std::dynamic_pointer_cast<PublicMember>(statement)) {
            if (auto moduleStatement = std::dynamic_pointer_cast<CreateModule>(publicStatement->getValue())) {
                DEBUG_LOG("Generating submodule: " + moduleStatement->getName());
                generateModule(moduleStatement->getName(), moduleStatement->getName(), moduleStatement->getStatements(), importedAliases, false);
                continue;
            }
            std::string originalName = publicStatement->getName();
            llvm::Value* value = publicStatement->codegen(*this);
            if (!value) {
                console.warn("Skipping public statement '" + originalName + "' - no valid IR generated.");
                continue;
            }

            if (llvm::GlobalValue* gv = llvm::dyn_cast<llvm::GlobalValue>(value)) {
                if (importAll) {
                    gv->setLinkage(llvm::GlobalValue::ExternalLinkage);
                    activeScope->set(originalName, gv);
                } else {
                    auto it = importedAliases.find(originalName);
                    if (it != importedAliases.end()) {
                        std::string alias = it->second; // Aliased name
                        gv->setLinkage(llvm::GlobalValue::ExternalLinkage);
                        activeScope->set(alias, gv);
                        DEBUG_LOG("Aliased import: " + alias + " -> " + originalName);
                    } else {
                        gv->setLinkage(llvm::GlobalValue::InternalLinkage);
                    }
                }
            } else {
                console.warn("Warning: Public symbol '" + originalName +
                             "' is not a global value and cannot have linkage visibility set.");
            }
        } else if (auto privateStatement = std::dynamic_pointer_cast<PrivateMember>(statement)) {
            DEBUG_LOG("Creating private member " + privateStatement->getName());
            llvm::Value* value = privateStatement->codegen(*this);
            if (importedAliases.find(privateStatement->getName()) != importedAliases.end()) {
                console.warn("'" + privateStatement->getName() + "' is not a public member of '" + modulePath + "' and cannot be imported");
            }
            if (value) {
                if (llvm::GlobalValue* gv = llvm::dyn_cast<llvm::GlobalValue>(value)) {
                    gv->setLinkage(llvm::GlobalValue::InternalLinkage);
                } else {
                    console.warn("Warning: Private statement '" + privateStatement->getName() + "' is not a global value and cannot have linkage visibility set to private or public.");
                }
            }
        }
    }

    printErrors(*CurrentModule);
    CurrentModule = previousModule;

    scope->addModule(modulePath, activeScope, alias);
    popScope();
    
    if (llvm::Linker::linkModules(*Module, std::move(newModule))) {
        llvm::errs() << "Error: Linking failed for module " << modulePath << "!\n";
    }
}

bool IRGenerator::isLoadedModule(const std::string& modulePath) {
    if (generatedModules.find(modulePath) != generatedModules.end()) {
        return true;
    }
    return false;
}

bool IRGenerator::isLoadedModuleMember(const std::string& modulePath, const std::string& memberName) {
    return true;
}

void IRGenerator::activateModuleMembers(const std::vector<std::string>& members) {
    return;
}

void IRGenerator::linkModules() {

}

void IRGenerator::importModule(const std::string& modulePath, const std::vector<std::string>& members) {
    if (modulePublicSymbols.find(modulePath) == modulePublicSymbols.end()) {
        console.error("Module '" + modulePath + "' not found or has no public symbols.");
        return;
    }

    // Import only public members
    for (const auto& [name, value] : modulePublicSymbols[modulePath]) {
        NamedValues[name] = value;
    }
}


llvm::Type* IRGenerator::resolveLLVMType(std::vector<std::string>& dataTypes) {
    if (dataTypes.empty()) {
        return llvm::Type::getInt32Ty(*Context); // Default to i32
    }

    llvm::LLVMContext& context = *Context;
    int totalPointerDepth = 0;
    int totalReferenceDepth = 0;
    bool isArray = false;
    uint64_t arraySize = 0;
    std::string baseType;
    size_t index = 0;

    // Detect array syntax: ["[", "size", "]", "type"] or dynamic array ["[", "type", "]"]
    if (index < dataTypes.size() && dataTypes[index] == "[") {
        if (index + 1 < dataTypes.size() && std::all_of(dataTypes[index + 1].begin(), dataTypes[index + 1].end(), ::isdigit)) {
            // Fixed-size array
            try {
                arraySize = std::stoull(dataTypes[index + 1]);
                index += 3; // Skip "[", "size", "]"
            } catch (...) {
                std::cerr << "[ERROR] Invalid array size: " << dataTypes[index + 1] << std::endl;
                return nullptr;
            }
        } else {
            // Dynamic array (size 0)
            arraySize = 0;
            index += 3; // Skip "[", "type", "]"
        }
        isArray = true;
    }

    // Count leading references ("&")
    while (index < dataTypes.size() && dataTypes[index] == "&") {
        totalReferenceDepth++;
        index++;
    }

    // Count leading pointers ("*")
    while (index < dataTypes.size() && dataTypes[index] == "*") {
        totalPointerDepth++;
        index++;
    }

    // Check for base type
    if (index >= dataTypes.size()) {
        std::cerr << "[ERROR] No base type found after modifiers!" << std::endl;
        return nullptr;
    }
    baseType = dataTypes[index++];

    // Count trailing pointers ("*")
    while (index < dataTypes.size() && dataTypes[index] == "*") {
        totalPointerDepth++;
        index++;
    }

    llvm::Type* type = nullptr;

    // Handle integer types (i followed by digits)
    if (baseType.size() > 1 && baseType[0] == 'i') {
        std::string numStr = baseType.substr(1);
        if (std::all_of(numStr.begin(), numStr.end(), ::isdigit)) {
            try {
                unsigned bits = std::stoul(numStr);
                if (bits >= 1 && bits <= 8388608) { // LLVM constraint
                    type = llvm::IntegerType::get(context, bits);
                } else {
                    std::cerr << "[ERROR] Invalid integer bit width: " << bits << std::endl;
                    return nullptr;
                }
            } catch (...) {
                std::cerr << "[ERROR] Invalid integer type: " << baseType << std::endl;
                return nullptr;
            }
        }
    }

    // Handle other types if not an integer
    // add support for simd vector types
    if (!type) {
        if (baseType == "char") {
            type = llvm::Type::getInt8Ty(context);
        } else if (baseType == "int") { // Alias for i32
            type = llvm::Type::getInt32Ty(context);
        } else if (baseType == "half" || baseType == "f16") {
            type = llvm::Type::getHalfTy(context);
        } else if (baseType == "float" || baseType == "f32") {
            type = llvm::Type::getFloatTy(context);
        } else if (baseType == "double" || baseType == "f64") {
            type = llvm::Type::getDoubleTy(context);
        } else if (baseType == "fp128") {
            type = llvm::Type::getFP128Ty(context);
        } else if (baseType == "x86_fp80") {
            type = llvm::Type::getX86_FP80Ty(context);
        } else if (baseType == "ppc_fp128") {
            type = llvm::Type::getPPC_FP128Ty(context);
        } else if (baseType == "bool") {
            type = llvm::Type::getInt1Ty(context);
        } else if (baseType == "string" || baseType == "str" || baseType == "utf8") {
            type = llvm::Type::getInt8Ty(context);
            totalPointerDepth++; // +1 pointer depth for strings
        } else if (baseType == "utf16") {
            type = llvm::Type::getInt16Ty(context);
            totalPointerDepth++;
        } else if (baseType == "utf32") {
            type = llvm::Type::getInt32Ty(context);
            totalPointerDepth++;
        } else if (baseType == "void") {
            if (totalPointerDepth > 0) {
                // void* is represented as i8*
                type = llvm::PointerType::get(llvm::Type::getInt8Ty(context), 0);
                totalPointerDepth--;
            } else {
                type = llvm::Type::getVoidTy(context);
            }
        } else {
            std::cerr << "[ERROR] Unknown type: " << baseType << std::endl;
            return nullptr;
        }
    }

    // Apply array type
    if (isArray) {
        type = llvm::ArrayType::get(type, arraySize);
    }

    // Apply references (as non-null pointers)
    if (totalReferenceDepth > 0) {
        type = llvm::PointerType::get(type, 0);
    }

    // Apply pointers
    while (totalPointerDepth > 0) {
        type = llvm::PointerType::get(type, 0);
        totalPointerDepth--;
    }

    return type;
}

// llvm::Type* IRGenerator::resolveLLVMType(std::vector<std::string>& dataTypes) {
//     if (dataTypes.empty()) {
//         return llvm::Type::getInt32Ty(*Context);
//     }

//     llvm::LLVMContext& context = *Context;
//     int totalPointerDepth = 0;
//     int totalReferenceDepth = 0;
//     std::string baseType;
//     size_t index = 0;

//     // Parse references (C++-style, treated as non-null pointers)
//     while (index < dataTypes.size() && dataTypes[index] == "&") {
//         totalReferenceDepth++;
//         index++;
//     }

//     // Parse base type
//     if (index >= dataTypes.size()) {
//         std::cerr << "[ERROR] Missing base type!" << std::endl;
//         return nullptr;
//     }
    
//     baseType = dataTypes[index++];

//     // Map base type to LLVM type
//     llvm::Type* type = nullptr;
//     if (baseType == "i8" || baseType == "char") type = llvm::Type::getInt8Ty(context);
//     else if (baseType == "i16") type = llvm::Type::getInt16Ty(context);
//     else if (baseType == "i32" || baseType == "int") type = llvm::Type::getInt32Ty(context);
//     else if (baseType == "i64") type = llvm::Type::getInt64Ty(context);
//     else if (baseType == "f32") type = llvm::Type::getFloatTy(context);
//     else if (baseType == "f64") type = llvm::Type::getDoubleTy(context);
//     else if (baseType == "bool") type = llvm::Type::getInt1Ty(context);
//     else if (baseType == "void") type = llvm::Type::getVoidTy(context);
//     else {
//         std::cerr << "[ERROR] Unknown base type: " << baseType << std::endl;
//         return nullptr;
//     }

//     // Parse trailing pointers ("*" tokens after base type)
//     while (index < dataTypes.size() && dataTypes[index] == "*") {
//         totalPointerDepth++;
//         index++;
//     }

//     // Parse array dimensions (e.g., "[5][10]")
//     while (index < dataTypes.size() && dataTypes[index] == "[") {
//         index++;
//         if (index < dataTypes.size() && std::all_of(dataTypes[index].begin(), dataTypes[index].end(), ::isdigit)) {
//             uint64_t dimSize = std::stoull(dataTypes[index++]);
//             type = llvm::ArrayType::get(type, dimSize);
//         } else {
//             // Dynamic array: treat as pointer
//             type = llvm::PointerType::get(type, 0);
//         }
//         if (index < dataTypes.size() && dataTypes[index] == "]") index++;
//     }

//     // Apply pointers
//     while (totalPointerDepth-- > 0) {
//         type = llvm::PointerType::get(type, 0);
//     }

//     // Apply references (as non-null pointers)
//     while (totalReferenceDepth-- > 0) {
//         type = llvm::PointerType::get(type, 0);
//     }

//     return type;
// }

llvm::Value* IRGenerator::createNullPointer() {
    return llvm::ConstantPointerNull::get(llvm::PointerType::getUnqual(llvm::Type::getInt8Ty(*Context)));
}

llvm::Value* IRGenerator::createNullValue() {
    return llvm::UndefValue::get(llvm::Type::getVoidTy(*Context));
}

// Generate IR for different types
// ============================== Generate IR for Numeric Types ============================== //
// Create an 8-bit integer (i8)
llvm::Value* IRGenerator::create8BitInteger(int value) {
    return llvm::ConstantInt::get(llvm::Type::getInt8Ty(*Context), value, true);
}

// Create a 16-bit integer (i16)
llvm::Value* IRGenerator::create16BitInteger(int value) {
    return llvm::ConstantInt::get(llvm::Type::getInt16Ty(*Context), value, true);
}

// Create a 32-bit integer (i32)
llvm::Value* IRGenerator::create32BitInteger(int value) {
    return llvm::ConstantInt::get(llvm::Type::getInt32Ty(*Context), value, true);
}

// Create a 64-bit integer (i64)
llvm::Value* IRGenerator::create64BitInteger(int value) {
    return llvm::ConstantInt::get(llvm::Type::getInt64Ty(*Context), value, true);
}

// Create a 32-bit floating-point (float)
llvm::Value* IRGenerator::create32BitFloat(float value) {
    return llvm::ConstantFP::get(llvm::Type::getFloatTy(*Context), value);
}

// Create a 64-bit floating-point (double)
llvm::Value* IRGenerator::create64BitFloat(double value) {
    return llvm::ConstantFP::get(llvm::Type::getDoubleTy(*Context), value);
}

// Create an arbitrary-precision integer (BigInt)
llvm::Value* IRGenerator::createBigInt(const std::string& str, unsigned bitSize) {
    // #if SIMD_OPTIMIZATION_LEVEL == 512
    //     return createBigIntAVX512(str, bitSize);
    // #elif SIMD_OPTIMIZATION_LEVEL == 256
    //     return createBigIntAVX2(str, bitSize);
    // #elif SIMD_OPTIMIZATION_LEVEL == 128
    //     return createBigIntAVX(str, bitSize);
    // #else
    //     llvm::APInt bigIntValue(bitSize, str, 10);
    //     return llvm::ConstantInt::get(llvm::Type::getIntNTy(*Context, bitSize), bigIntValue);
    // #endif
    llvm::APInt bigIntValue(bitSize, str, 10);
    return llvm::ConstantInt::get(llvm::Type::getIntNTy(*Context, bitSize), bigIntValue);
}

llvm::Value* IRGenerator::createBigIntAVX512(const std::string& str, unsigned bitSize) {
    // __m512i bigIntVec = _mm512_setzero_si512(); // Zero-initialize 512-bit register
    return createBigInt(str, 512);  // Use this as placeholder for now
}

llvm::Value* IRGenerator::createBigIntAVX2(const std::string& str, unsigned bitSize) {
    // __m256i bigIntVec = _mm256_setzero_si256(); // Zero-initialize 256-bit register
    return createBigInt(str, 256);
}

llvm::Value* IRGenerator::createBigIntAVX(const std::string& str, unsigned bitSize) {
    __m128i bigIntVec = _mm_setzero_si128(); // Zero-initialize 128-bit register
    return createBigInt(str, 128);
}

// Create an 8-bit character (char)
llvm::Value* IRGenerator::createChar(char value) {
    return llvm::ConstantInt::get(llvm::Type::getInt8Ty(*Context), value, true);
}

// Create a 16-bit character (char16_t)
llvm::Value* IRGenerator::createChar16(char16_t value) {
    return llvm::ConstantInt::get(llvm::Type::getInt16Ty(*Context), value, false);
}

// Create a 32-bit character (char32_t)
llvm::Value* IRGenerator::createChar32(char32_t value) {
    return llvm::ConstantInt::get(llvm::Type::getInt32Ty(*Context), value, false);
}

// Create an 8-bit (UTF-8) string
llvm::Value* IRGenerator::createUTF8String(const std::string& str) {
    return Builder->CreateGlobalString(str, ".utf8str");
}

// Create a 16-bit (UTF-16) string
llvm::Value* IRGenerator::createUTF16String(const std::u16string& str) {
    std::vector<llvm::Constant*> chars;
    for (char16_t c : str) {
        chars.push_back(llvm::ConstantInt::get(llvm::Type::getInt16Ty(*Context), c, false));
    }
    chars.push_back(llvm::ConstantInt::get(llvm::Type::getInt16Ty(*Context), 0)); // Null terminator

    llvm::ArrayType* arrayType = llvm::ArrayType::get(llvm::Type::getInt16Ty(*Context), chars.size());
    llvm::Constant* array = llvm::ConstantArray::get(arrayType, chars);

    auto global = new llvm::GlobalVariable(
        *Module, arrayType, true, llvm::GlobalValue::PrivateLinkage, array, ".utf16str");

    return global;
}

// Create a 32-bit (UTF-32) string
llvm::Value* IRGenerator::createUTF32String(const std::u32string& str) {
    std::vector<llvm::Constant*> chars;
    for (char32_t c : str) {
        chars.push_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(*Context), c, false));
    }
    chars.push_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(*Context), 0)); // Null terminator

    llvm::ArrayType* arrayType = llvm::ArrayType::get(llvm::Type::getInt32Ty(*Context), chars.size());
    llvm::Constant* array = llvm::ConstantArray::get(arrayType, chars);

    auto global = new llvm::GlobalVariable(
        *Module, arrayType, true, llvm::GlobalValue::PrivateLinkage, array, ".utf32str");

    return global;
}

llvm::Value* IRGenerator::createBool(bool value) {
    return llvm::ConstantInt::get(llvm::Type::getInt1Ty(*Context), value ? 1 : 0, false);
}

llvm::Value* IRGenerator::createVariable(
    const std::string& name, 
    llvm::Type* type, 
    llvm::Value* initialValue, 
    bool isGlobal, 
    llvm::BasicBlock* activeBlock 
) {

    llvm::Module* activeModule = CurrentModule; // Get the correct active module

    if (isGlobal) {
        // Create a global variable inside the active module
        llvm::GlobalVariable* gVar = new llvm::GlobalVariable(
            *activeModule,
            type,
            false,  // Not constant
            llvm::GlobalValue::PrivateLinkage,
            llvm::Constant::getNullValue(type), // Default initializer
            name
        );

        // Set the initializer if the value is constant
        if (llvm::Constant* constValue = llvm::dyn_cast<llvm::Constant>(initialValue)) {
            gVar->setInitializer(constValue);
        }

        activeScope->set(name, gVar);
        return gVar;
    } 
    
    // LOCAL VARIABLE CASE:
    llvm::Function* function = nullptr;
    llvm::IRBuilder<> tempBuilder(Builder->getContext());

    if (activeBlock) {
        function = activeBlock->getParent();
        tempBuilder.SetInsertPoint(activeBlock, activeBlock->begin());
    } else {
        function = Builder->GetInsertBlock()->getParent();
        tempBuilder.SetInsertPoint(&function->getEntryBlock(), function->getEntryBlock().begin());
    }
    
    // Place allocation in the entry block
    llvm::AllocaInst* alloca = tempBuilder.CreateAlloca(type, nullptr, name);

    // Ensure stores happen before return
    llvm::BasicBlock* entryBlock = &function->getEntryBlock();
    llvm::Instruction* retInst = nullptr;
    for (llvm::Instruction& I : *entryBlock) {
        if (llvm::isa<llvm::ReturnInst>(&I)) {
            retInst = &I;
            break;
        }
    }

    if (type->isArrayTy()) {
        llvm::ArrayType* arrayType = llvm::dyn_cast<llvm::ArrayType>(type);
        llvm::Type* elementType = arrayType->getElementType();
    
        // Get alignment from DataLayout
        const llvm::DataLayout& dataLayout = activeModule->getDataLayout();
        unsigned elementAlign = dataLayout.getABITypeAlign(elementType).value();
    
        // Set alignment for the entire array allocation
        alloca->setAlignment(llvm::Align(elementAlign));
    
        if (initialValue) {
            // Safely cast to ConstantArray
            if (auto* constArray = llvm::dyn_cast<llvm::ConstantArray>(initialValue)) {
                for (unsigned i = 0; i < arrayType->getNumElements(); ++i) {
                    llvm::Value* index = llvm::ConstantInt::get(
                        llvm::Type::getInt32Ty(activeModule->getContext()), i
                    );
                    // Get pointer to the i-th element
                    llvm::Value* elementPtr = Builder->CreateGEP(
                        arrayType,  // Pointee type of `alloca`
                        alloca,
                        {Builder->getInt32(0), index}
                    );
                    // Store with element-specific alignment
                    llvm::StoreInst* store = Builder->CreateStore(
                        constArray->getAggregateElement(i),
                        elementPtr
                    );
                    store->setAlignment(llvm::Align(elementAlign));
                    if (retInst) store->moveBefore(retInst);
                }
            }
        }
    } else if (type->isIntegerTy()) {
        unsigned bitWidth = type->getIntegerBitWidth();
        unsigned align = 1;

        if (bitWidth >= 1024) align = 128;
        else if (bitWidth >= 512) align = 64;
        else if (bitWidth >= 256) align = 32;
        else if (bitWidth >= 128) align = 16;
        else if (bitWidth >= 64) align = 8;
        else if (bitWidth >= 32) align = 4;
        else if (bitWidth >= 16) align = 2;

        alloca->setAlignment(llvm::Align(llvm::MaybeAlign(align).value_or(llvm::Align(1))));
        if (initialValue) {
            llvm::StoreInst* store = Builder->CreateStore(initialValue, alloca);
            store->setAlignment(llvm::Align(align));
            if (retInst) store->moveBefore(retInst);
        }
    } else if (type->isFloatingPointTy()) {
        // Handle alignment for floating-point types appropriately
        unsigned align = type->isFloatTy() ? 4 : 8; // Assuming f32 = 4-byte alignment, f64 = 8-byte alignment
        alloca->setAlignment(llvm::Align(align));
        if (initialValue) {
            llvm::StoreInst* store = Builder->CreateStore(initialValue, alloca);
            store->setAlignment(llvm::Align(align));
            if (retInst) store->moveBefore(retInst);
        }
    } else if (type->isPointerTy()) {
        if (initialValue) {
            llvm::Type* initType = initialValue->getType();
            if (!initType->isPointerTy()) {
                llvm::errs() << "Error: Attempting to store a non-pointer value into a pointer variable.\n";
            } else if (initType != type) {
                // Ensure the stored pointer matches the expected pointer type
                llvm::Value* castedValue = Builder->CreateBitCast(initialValue, type);
                llvm::StoreInst* store = Builder->CreateStore(castedValue, alloca);
                if (retInst) store->moveBefore(retInst);
            } else {
                llvm::StoreInst* store = Builder->CreateStore(initialValue, alloca);
                if (retInst) store->moveBefore(retInst);
            }
        }
    }

    if (activeScope->has(name)) {
        console.error("Variable " + name + " has already been defined in the current scope");
    }

    activeScope->set(name, alloca);
    return alloca;
}

llvm::GlobalVariable* IRGenerator::createGlobalVariable(
    const std::string& name, 
    llvm::Type* type, 
    llvm::Value* initialValue, 
    llvm::GlobalValue::LinkageTypes linkage
) {
    llvm::Module* activeModule = CurrentModule; // Ensure it's created in the current module

    // Create a global variable inside the active module
    llvm::GlobalVariable* gVar = new llvm::GlobalVariable(
        *activeModule, 
        type,
        false,  // Not constant
        linkage, // Use the specified linkage type (default is InternalLinkage)
        llvm::Constant::getNullValue(type), // Default initializer
        name
    );

    // Set the initializer if the value is a constant
    if (llvm::Constant* constValue = llvm::dyn_cast<llvm::Constant>(initialValue)) {
        gVar->setInitializer(constValue);
    }

    return gVar;
}

llvm::Value* IRGenerator::getAddressOf(const std::string& varname) {
    if (!activeScope->has(varname)) {
        throw std::runtime_error("Undefined variable: " + varname);
    }
    
    // Return the pointer/alloca directly
    return activeScope->get(varname);
}

llvm::Value* IRGenerator::getReferenceToVariable(const std::string& varname) {
    if (!activeScope->has(varname)) {
        throw std::runtime_error("Cannot get reference to: " + varname);
    }
    
    // Return the pointer/alloca directly
    return activeScope->get(varname);
}

llvm::Value* IRGenerator::getVariable(const std::string& name) {
    // Check current scope
    if (activeScope->exists(name)) {
        llvm::Value* val = activeScope->get(name);

        if (llvm::AllocaInst* alloca = llvm::dyn_cast<llvm::AllocaInst>(val)) {
            // Generate the load operation (no need to change insertion point)
            return Builder->CreateLoad(alloca->getAllocatedType(), alloca, name + ".val");
        }

        return val;
    }

    throw std::runtime_error("Unknown variable name: " + name);
}


llvm::Value* IRGenerator::createConstant(const std::string& name, llvm::Type* type, llvm::Value* value) {
    NamedValues[name] = value;
    return value;
}

llvm::Value* IRGenerator::createDynamicVariable(const std::string& name, llvm::Value* value) {
    llvm::IRBuilder<> builder(Builder->GetInsertBlock());
    llvm::AllocaInst* alloca = builder.CreateAlloca(value->getType(), nullptr, name);
    builder.CreateStore(value, alloca);
    NamedValues[name] = alloca;
    return alloca;
}

llvm::Value* IRGenerator::reassign(const std::string& name, llvm::Value* newValue) {
    if (NamedValues.find(name) == NamedValues.end()) {
        throw std::runtime_error("Variable not found: " + name);
    }
    Builder->CreateStore(newValue, NamedValues[name]);
    return newValue;
}


llvm::Value* IRGenerator::createDynamicConstant(const std::string& name, llvm::Value* value) {
    NamedValues[name] = value;
    return value;
}

llvm::Value* IRGenerator::assignDynamicVariable(const std::string& name, llvm::Value* newValue) {
    auto it = runtimeVariables.find(name);
    if (it != runtimeVariables.end()) {
        delete it->second; // Free old value
    }

    if (llvm::ConstantInt* intVal = llvm::dyn_cast<llvm::ConstantInt>(newValue)) {
        runtimeVariables[name] = new DynamicValue(intVal->getSExtValue());
    } else if (llvm::ConstantFP* floatVal = llvm::dyn_cast<llvm::ConstantFP>(newValue)) {
        runtimeVariables[name] = new DynamicValue(floatVal->getValueAPF().convertToDouble());
    } else {
        // Assume it's a string (you need proper string handling)
        runtimeVariables[name] = new DynamicValue("string_value_placeholder");
    }

    return newValue;
}


llvm::Value* IRGenerator::getDynamicVariable(const std::string& name) {
    llvm::AllocaInst* alloca = llvm::dyn_cast<llvm::AllocaInst>(NamedValues[name]);
    if (!alloca) {
        throw std::runtime_error("Variable is not an AllocaInst: " + name);
    }
    return Builder->CreateLoad(alloca->getAllocatedType(), alloca, name);
}

llvm::Value* IRGenerator::generateOpaqueDynamicVariable(const std::string& name, llvm::Value* value) {
    llvm::Type* int8PtrType = llvm::PointerType::get(llvm::Type::getInt8Ty(*Context), 0);
    llvm::Value* castedValue = Builder->CreateBitCast(value, int8PtrType);

    llvm::AllocaInst* alloca = Builder->CreateAlloca(int8PtrType, nullptr, name);
    Builder->CreateStore(castedValue, alloca);

    NamedValues[name] = alloca;
    return alloca;
}

llvm::Value* IRGenerator::createStaticFixedArray(
    llvm::Type* elementType, 
    size_t size, 
    const std::vector<llvm::Value*>& elements) {

    // ========== CRITICAL FIX: Handle insertion point ==========
    llvm::BasicBlock* currentBlock = Builder->GetInsertBlock();
    if (currentBlock && !currentBlock->empty() && currentBlock->back().isTerminator()) {
        // Insert BEFORE the terminator if one exists
        Builder->SetInsertPoint(&currentBlock->back());
    }

    const llvm::DataLayout& dataLayout = CurrentModule != nullptr ? CurrentModule->getDataLayout() : Module->getDataLayout();
    unsigned elementAlign = dataLayout.getABITypeAlign(elementType).value();

    llvm::ArrayType* arrayType = llvm::ArrayType::get(elementType, size);
    llvm::Value* arrayAlloc = Builder->CreateAlloca(arrayType, nullptr, "static_array");
    
    // Set alignment for allocation
    llvm::cast<llvm::AllocaInst>(arrayAlloc)->setAlignment(llvm::Align(elementAlign));

    for (size_t i = 0; i < elements.size(); ++i) {
        llvm::Value* element = elements[i];
        
        // Type check/cast
        if (element->getType() != elementType) {
            element = Builder->CreateBitCast(element, elementType);
        }

        // Get element pointer
        llvm::Value* elementPtr = Builder->CreateGEP(
            arrayType,
            arrayAlloc,
            {Builder->getInt32(0), Builder->getInt32(i)}
        );

        // Create store with alignment
        llvm::StoreInst* store = Builder->CreateStore(element, elementPtr);
        store->setAlignment(llvm::Align(elementAlign));

        // ========== SECONDARY FIX: Ensure stores are before terminators ==========
        if (currentBlock && !currentBlock->empty() && currentBlock->back().isTerminator()) {
            store->moveBefore(&currentBlock->back());
        }
    }

    return arrayAlloc;
}

llvm::Function* IRGenerator::createFunction(const FunctionDeclaration& funcDecl) {
    // Create function type
    std::vector<llvm::Type*> paramTypes;
    for (auto& param : funcDecl.parameters) {
        if (auto typed = std::dynamic_pointer_cast<TypedStatement>(param)) {
            paramTypes.push_back(typed->getType());
        }
    }
    
    llvm::FunctionType* funcType = llvm::FunctionType::get(
        funcDecl.getType(),
        paramTypes,
        false
    );
    
    // Create function
    llvm::Function* function = llvm::Function::Create(
        funcType,
        llvm::Function::ExternalLinkage,
        funcDecl.name,
        *Module
    );
    
    // Set parameter names
    unsigned idx = 0;
    for (auto& arg : function->args()) {
        if (auto stmt = std::dynamic_pointer_cast<NamedStatement>(funcDecl.parameters[idx++])) {
            arg.setName(stmt->getName());
        }
    }
    activeScope->set(funcDecl.getName(), function);
    return function;
}

// When processing a function call:
llvm::Value* IRGenerator::createCall(
    const std::string& callee, 
    std::vector<llvm::Value*>& args, 
    llvm::BasicBlock* activeBlock
) {
    // 1. Look up function
    llvm::Function* func = Module->getFunction(callee);
    if (!func) {
        console.error("Function '" + callee + "' not found");
        return nullptr;
    }

    // 2. Verify argument count
    if (func->arg_size() != args.size()) {
        console.error("Argument count mismatch for '" + callee + "'");
        return nullptr;
    }

    // 3. Type checking and casting
    for (size_t i = 0; i < args.size(); ++i) {
        llvm::Type* expected = func->getFunctionType()->getParamType(i);
        if (args[i]->getType() != expected) {
            llvm::Value* castedArg = castValue(args[i], expected);
            if (!castedArg) {
                console.error("Type mismatch for argument " + std::to_string(i) + 
                            " in call to '" + callee + "'");
                return nullptr;
            }
            args[i] = castedArg;
        }
    }

    // 4. Determine insertion point safely
    llvm::BasicBlock* insertBlock = activeBlock ? activeBlock : Builder->GetInsertBlock();
    if (!insertBlock) {
        console.error("No valid insert block found for function call");
        return nullptr;
    }

    // 5. Find safe insertion point (BEFORE any terminator)
    if (insertBlock->getTerminator()) {
        // If block already has a terminator, insert right before it
        Builder->SetInsertPoint(insertBlock->getTerminator());
    } else {
        // Otherwise insert at end of block
        Builder->SetInsertPoint(insertBlock);
    }

    // 6. Generate call
    llvm::Value* callInst = Builder->CreateCall(func, args);

    // 7. Ensure we didn't accidentally insert after terminator
    // #ifdef LLVM_DEBUG
    // if (insertBlock->getTerminator() && 
    //     callInst->getNextNode() != insertBlock->getTerminator()) {
    //     llvm::errs() << "WARNING: Call instruction inserted after terminator!\n";
    // }
    // #endif

    return callInst;
}

void IRGenerator::generateFunctionBody(llvm::Function* function, 
                                       const FunctionDeclaration& funcDecl) {
    pushActiveBlock();

    // Create entry block
    llvm::BasicBlock* entry = llvm::BasicBlock::Create(*Context, "entry", function);
    Builder->SetInsertPoint(entry);
    
    // Create a new scope for function parameters + body
    pushScope();
    
    // Create allocas for parameters in the entry block
    for (auto& arg : function->args()) {
        std::string argName = arg.getName().str(); // Convert StringRef to std::string
        llvm::AllocaInst* alloca = createEntryBlockAlloca(function, arg.getType(), argName);
        Builder->CreateStore(&arg, alloca);
        activeScope->set(argName, alloca);
    }
    
    // Generate function body
    llvm::Value* retVal = funcDecl.body->codegen(*this);
    
    // Handle implicit return if needed
    if (!currentBlockHasTerminator()) {
        if (function->getReturnType()->isVoidTy()) {
            Builder->CreateRetVoid();
        } else if (retVal) {
            // Ensure return value matches function type
            if (retVal->getType() != function->getReturnType()) {
                llvm::Value* castedRet = castValue(retVal, function->getReturnType());
                if (!castedRet) {
                    console.error("Failed to cast return value in function: " + function->getName().str());
                    Builder->CreateUnreachable();
                    popScope();
                    popActiveBlock();
                    function->eraseFromParent();
                    return;
                }
                retVal = castedRet;
            }
            Builder->CreateRet(retVal);
        } else {
            // Error: Non-void function missing return
            console.error("Non-void function missing return: " + function->getName().str());
            Builder->CreateUnreachable();
            popScope();
            popActiveBlock();
            function->eraseFromParent();
            return;
        }
    }
    
    popScope();  // Parameters + function body scope
    popActiveBlock();

    // Verify the function for consistency
    if (llvm::verifyFunction(*function, &llvm::errs())) {
        console.error("Function verification failed: " + function->getName().str());
        function->eraseFromParent();
    }
}


llvm::AllocaInst* IRGenerator::createEntryBlockAlloca(llvm::Function* function,llvm::Type* type, const std::string& name) {
    llvm::IRBuilder<> tmpBuilder(&function->getEntryBlock(),
    function->getEntryBlock().begin());
    return tmpBuilder.CreateAlloca(type, nullptr, name);
}

llvm::Value* IRGenerator::createReturn(llvm::Value* returnValue, llvm::Type* expectedReturnType) {
    // Get current function and verify we're in a function context
    llvm::Function* currentFunction = Builder->GetInsertBlock()->getParent();
    if (!currentFunction) {
        throw std::runtime_error("Return statement outside function");
    }

    // Handle void returns
    if (currentFunction->getReturnType()->isVoidTy()) {
        if (returnValue) {
            throw std::runtime_error("Void function cannot return a value");
        }
        return Builder->CreateRetVoid();
    }

    // Handle value returns
    if (!returnValue) {
        throw std::runtime_error("Non-void function must return a value");
    }

    // Ensure type compatibility
    if (returnValue->getType() != expectedReturnType) {
        throw std::runtime_error("Return type mismatch: expected " + 
                                 std::to_string(expectedReturnType->getTypeID()) + 
                                 ", got " + 
                                 std::to_string(returnValue->getType()->getTypeID()));
    }

    // Create the return instruction
    return Builder->CreateRet(returnValue);
}


bool IRGenerator::currentBlockHasTerminator() const {
    return Builder->GetInsertBlock()->getTerminator() != nullptr;
}

llvm::Value* IRGenerator::castValue(llvm::Value* val, llvm::Type* targetType) {
    if (val->getType() == targetType) return val;
    
    // Handle common cases (e.g., int extensions)
    // if (targetType->isIntegerTy() && val->getType()->isIntegerTy()) {
    //     bool isSigned = /* your type system's signedness */;
    //     return Builder->CreateIntCast(val, targetType, isSigned);
    // }
    // Add more casts as needed...
    
    return nullptr; // No valid cast
}

std::string IRGenerator::typeToString(llvm::Type* type) {
    std::string typeStr;
    llvm::raw_string_ostream rso(typeStr);
    type->print(rso);
    return rso.str();
}

llvm::Value* IRGenerator::createUnaryExpression(llvm::Value* operand, TokenTypes op, bool isPostfix) {
    if (!operand) return nullptr;

    // Handle numeric operations (both integer and floating point)
    auto handleNumericUnary = [&](auto createOp) -> llvm::Value* {
        if (operand->getType()->isIntegerTy()) {
            return createOp(operand, "unarytmp");
        } else if (operand->getType()->isFloatingPointTy()) {
            return createOp(operand, "funarytmp");
        }
        return nullptr;
    };

    switch (op) {
        case TokenTypes::Plus:
            return operand; // +x is just x

        case TokenTypes::Minus:
            return handleNumericUnary([&](auto val, auto name) {
                return val->getType()->isIntegerTy() 
                    ? Builder->CreateNeg(val, name)
                    : Builder->CreateFNeg(val, name);
            });

        case TokenTypes::LogicalNot:
            return Builder->CreateNot(Builder->CreateICmpNE(
                operand, 
                llvm::ConstantInt::get(operand->getType(), 0),
                "booltmp"
            ), "nottmp");

        case TokenTypes::Tilde:
            return Builder->CreateNot(operand, "bwnottmp");

        case TokenTypes::Increment:
        case TokenTypes::Decrement: {
            llvm::Value* one = llvm::ConstantInt::get(operand->getType(), 1);
            llvm::Value* delta = (op == TokenTypes::Increment) ? one : Builder->CreateNeg(one, "deltatmp");
            llvm::Value* newVal = Builder->CreateAdd(operand, delta, "incdec");
            
            // For variables, store the new value
            if (auto* load = llvm::dyn_cast<llvm::LoadInst>(operand)) {
                Builder->CreateStore(newVal, load->getPointerOperand());
            }
            
            return isPostfix ? operand : newVal;
        }

        default:
            throw std::runtime_error("Unknown unary operator");
    }
}

llvm::Value* IRGenerator::createBinaryExpression(llvm::Value* left, TokenTypes op, llvm::Value* right) {
    if (!left || !right) return nullptr;

    // Type promotion helper
    auto promoteTypes = [&]() {
        if (left->getType() == right->getType()) return;

        // Integer promotion
        if (left->getType()->isIntegerTy() && right->getType()->isIntegerTy()) {
            unsigned maxBits = std::max(
                left->getType()->getIntegerBitWidth(),
                right->getType()->getIntegerBitWidth()
            );
            llvm::Type* targetType = llvm::Type::getIntNTy(*Context, maxBits);
            left = Builder->CreateIntCast(left, targetType, true, "casttmp");
            right = Builder->CreateIntCast(right, targetType, true, "casttmp");
        }
        // TODO: Add floating point promotion if needed
    };

    promoteTypes();

    switch (op) {
        // Arithmetic
        case TokenTypes::Plus:
            return left->getType()->isFloatingPointTy()
                ? Builder->CreateFAdd(left, right, "faddtmp")
                : Builder->CreateAdd(left, right, "addtmp");
            
        case TokenTypes::Minus:
            return left->getType()->isFloatingPointTy()
                ? Builder->CreateFSub(left, right, "fsubtmp")
                : Builder->CreateSub(left, right, "subtmp");
            
        case TokenTypes::Multiply:
            return left->getType()->isFloatingPointTy()
                ? Builder->CreateFMul(left, right, "fmultmp")
                : Builder->CreateMul(left, right, "multmp");
            
        case TokenTypes::Divide:
            return left->getType()->isFloatingPointTy()
                ? Builder->CreateFDiv(left, right, "fdivtmp")
                : Builder->CreateSDiv(left, right, "divtmp");
            
        case TokenTypes::Modulo:
            return left->getType()->isFloatingPointTy()
                ? Builder->CreateFRem(left, right, "fmodtmp")
                : Builder->CreateSRem(left, right, "modtmp");
            
        // Comparisons
        case TokenTypes::Equals:
            return left->getType()->isFloatingPointTy()
                ? Builder->CreateFCmpOEQ(left, right, "feqtmp")
                : Builder->CreateICmpEQ(left, right, "eqtmp");
            
        case TokenTypes::NotEquals:
            return left->getType()->isFloatingPointTy()
                ? Builder->CreateFCmpONE(left, right, "fnetmp")
                : Builder->CreateICmpNE(left, right, "netmp");
            
        case TokenTypes::LessThan:
            return left->getType()->isFloatingPointTy()
                ? Builder->CreateFCmpOLT(left, right, "flttmp")
                : Builder->CreateICmpSLT(left, right, "lttmp");
            
        case TokenTypes::LessEqual:
            return left->getType()->isFloatingPointTy()
                ? Builder->CreateFCmpOLE(left, right, "fletmp")
                : Builder->CreateICmpSLE(left, right, "letmp");
            
        case TokenTypes::GreaterThan:
            return left->getType()->isFloatingPointTy()
                ? Builder->CreateFCmpOGT(left, right, "fgttmp")
                : Builder->CreateICmpSGT(left, right, "gttmp");
            
        case TokenTypes::GreaterEqual:
            return left->getType()->isFloatingPointTy()
                ? Builder->CreateFCmpOGE(left, right, "fgetmp")
                : Builder->CreateICmpSGE(left, right, "getmp");
            
        // Bitwise
        case TokenTypes::BitwiseAnd:
            return Builder->CreateAnd(left, right, "andtmp");
            
        case TokenTypes::BitwiseOr:
            return Builder->CreateOr(left, right, "ortmp");
            
        case TokenTypes::BitwiseXor:
            return Builder->CreateXor(left, right, "xortmp");
            
        case TokenTypes::ShiftLeft:
            return Builder->CreateShl(left, right, "shltmp");
            
        // case TokenTypes::ShiftRight: {
        //     // For shift right, we need to know if it's arithmetic (signed) or logical (unsigned)
        //     if (left->getType()->isIntegerTy()) {
        //         // Check if the value being shifted is signed by looking at its uses
        //         bool isSigned = false;
        //         if (llvm::Instruction* inst = llvm::dyn_cast<llvm::Instruction>(left)) {
        //             if (inst->getOpcode() == llvm::Instruction::SExt) {
        //                 isSigned = true;
        //             }
        //             // You might need to add more cases here depending on your IR
        //         }
        //         return isSigned ? builder.CreateAShr(left, right, "ashrtmp")
        //                         : builder.CreateLShr(left, right, "lshrtmp");
        //     }
        //     return nullptr; // Shift right only valid for integer types
        // }
        case TokenTypes::ShiftRight:
            return left->getType()->isIntegerTy()
                ? Builder->CreateAShr(left, right, "shrtmp")
                : nullptr;
        default:
            throw std::runtime_error("Unknown binary operator");
    }
}

llvm::Value* IRGenerator::createTernaryExpression(llvm::Value* cond, llvm::Value* truthy, llvm::Value* falsey) {
    if (!cond || !truthy || !falsey) return nullptr;

    // Convert condition to bool if needed
    if (!cond->getType()->isIntegerTy(1)) {
        cond = Builder->CreateICmpNE(
            cond, 
            llvm::ConstantInt::get(cond->getType(), 0),
            "booltmp"
        );
    }

    llvm::Function* fn = Builder->GetInsertBlock()->getParent();
    
    // Create blocks for the true/false/merge cases
    llvm::BasicBlock* trueBB = llvm::BasicBlock::Create(*Context, "true", fn);
    llvm::BasicBlock* falseBB = llvm::BasicBlock::Create(*Context, "false", fn);
    llvm::BasicBlock* mergeBB = llvm::BasicBlock::Create(*Context, "merge", fn);

    Builder->CreateCondBr(cond, trueBB, falseBB);

    // Emit true value
    Builder->SetInsertPoint(trueBB);
    Builder->CreateBr(mergeBB);
    trueBB = Builder->GetInsertBlock();

    // Emit false value
    Builder->SetInsertPoint(falseBB);
    Builder->CreateBr(mergeBB);
    falseBB = Builder->GetInsertBlock();

    // Create PHI node
    Builder->SetInsertPoint(mergeBB);
    llvm::PHINode* phi = Builder->CreatePHI(truthy->getType(), 2, "ternarytmp");
    phi->addIncoming(truthy, trueBB);
    phi->addIncoming(falsey, falseBB);

    return phi;
}

// llvm::StructType* IRGenerator::createStructType(
//     const std::string& name,
//     const std::vector<std::pair<std::string, llvm::Type*>>& fields
// ) {
//     // Check if struct already exists
//     if (llvm::StructType* existing = llvm::StructType::getTypeByName(*Context, name)) {
//         DEBUG_LOG("Struct " + name + " already exists.");
//         return existing;
//     }

//     std::vector<llvm::Type*> fieldTypes;
//     for (const auto& field : fields) {
//         fieldTypes.push_back(field.second);
//     }

//     llvm::StructType* structType = llvm::StructType::create(*Context, fieldTypes, name);
//     DEBUG_LOG("Created struct: " + name);
//     return structType;
// }

// llvm::Value* IRGenerator::createStructInstance(
//     llvm::StructType* structType,
//     const std::string& varName
// ) {
//     llvm::IRBuilder<> builder(*Context);
//     llvm::AllocaInst* instance = builder.CreateAlloca(structType, nullptr, varName);
//     DEBUG_LOG("Allocated struct instance: " + varName);
//     return instance;
// }
