#include <omniscript/engine/IRGenerator.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/IR/PassManager.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Passes/StandardInstrumentations.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Linker/Linker.h>
#include <omniscript/debuggingtools/console.h>

IRGenerator::IRGenerator() {
    Context = std::make_unique<llvm::LLVMContext>();
    Module = std::make_unique<llvm::Module>("OmniScript", *Context);
    Builder = std::make_unique<llvm::IRBuilder<>>(*Context);
    initialize();
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
        llvm::Function* topLevelFunc = llvm::Function::Create(funcType, llvm::Function::InternalLinkage, "__top_level__", Module.get());
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
    Module->print(llvm::errs(), nullptr);
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

void IRGenerator::generateModule(const std::string& moduleName, const std::vector<std::shared_ptr<Statement>>& statements) {
    // Check if the module already exists
    if (generatedModules.find(moduleName) != generatedModules.end()) {
        console.warn("Module '" + moduleName + "' is already defined.");
        return;
    }

    // Create a new LLVM module
    auto newModule = std::make_unique<llvm::Module>(moduleName, *Context);

    // Backup the current module and switch context
    llvm::Module* previousModule = CurrentModule;
    CurrentModule = newModule.get();

    // Public symbol table for the module
    std::unordered_map<std::string, llvm::Value*> publicMembers;

    for (const auto& statement : statements) {
        if (auto moduleStatement = std::dynamic_pointer_cast<CreateModule>(statement)) {
            // Nested module found - process it separately
            generateModule(moduleStatement->getName(), moduleStatement->getStatements());
        } 
        else if (auto publicStatement = std::dynamic_pointer_cast<PublicMember>(statement)) {
            llvm::Value* value = publicStatement->codegen(*this);
            if (value) publicMembers[publicStatement->getName()] = value;
        } 
        else {
            // Private or global statement - just generate IR
            statement->codegen(*this);
        }
    }

    // Restore previous module
    CurrentModule = previousModule;

    // Store the module with its public members
    generatedModules[moduleName] = std::move(newModule);
    modulePublicSymbols[moduleName] = std::move(publicMembers);

    if (llvm::Linker::linkModules(*Module, std::move(newModule))) {
        llvm::errs() << "Error: Linking failed!\n";
    }
}


void IRGenerator::importModule(const std::string& moduleName) {
    if (modulePublicSymbols.find(moduleName) == modulePublicSymbols.end()) {
        console.error("Module '" + moduleName + "' not found or has no public symbols.");
        return;
    }

    // Import only public members
    for (const auto& [name, value] : modulePublicSymbols[moduleName]) {
        NamedValues[name] = value;
    }
}


llvm::Type* IRGenerator::resolveLLVMType(const std::vector<std::string>& dataTypes) {
    if (dataTypes.empty()) {
        return nullptr; // No type annotation provided
    }

    llvm::LLVMContext& context = *Context; // Dereferencing the unique_ptr

    if (dataTypes[0] == "i8") {
        return llvm::Type::getInt8Ty(context);
    } else if (dataTypes[0] == "i16") {
        return llvm::Type::getInt16Ty(context);
    } else if (dataTypes[0] == "i32") {
        return llvm::Type::getInt32Ty(context);
    } else if (dataTypes[0] == "i64") {
        return llvm::Type::getInt64Ty(context);
    } else if (dataTypes[0] == "f32") {
        return llvm::Type::getFloatTy(context);
    } else if (dataTypes[0] == "f64") {
        return llvm::Type::getDoubleTy(context);
    }

    // console.error("Unknown type: " + dataTypes[0]);
    return nullptr;
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
llvm::Value* IRGenerator::createBigInt(const std::string& str) {
    llvm::APInt bigIntValue(1024, str, 10); // 1024-bit integer from string (base 10)
    return llvm::ConstantInt::get(*Context, bigIntValue);
}


llvm::Value* IRGenerator::createVariable(const std::string& name, llvm::Type* type, llvm::Value* initialValue) {
    llvm::Function* function = Builder->GetInsertBlock()->getParent();  // Get current function
    llvm::IRBuilder<> tempBuilder(&function->getEntryBlock(), function->getEntryBlock().begin());
    
    // Place allocation in the entry block
    llvm::AllocaInst* alloca = tempBuilder.CreateAlloca(type, nullptr, name);
    NamedValues[name] = alloca;

    // ✅ Ensure the store is inserted *before* the return statement
    llvm::BasicBlock* insertBlock = Builder->GetInsertBlock();
    if (llvm::ReturnInst* retInst = llvm::dyn_cast<llvm::ReturnInst>(insertBlock->getTerminator())) {
        Builder->SetInsertPoint(retInst);  // Insert before the return
    }

    Builder->CreateStore(initialValue, alloca);
    return alloca;
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

llvm::Value* IRGenerator::getVariable(const std::string& name) {
    if (NamedValues.find(name) == NamedValues.end()) {
        throw std::runtime_error("Variable not found: " + name);
    }

    llvm::AllocaInst* alloca = llvm::dyn_cast<llvm::AllocaInst>(NamedValues[name]);
    if (!alloca) {
        throw std::runtime_error("Invalid variable type: " + name);
    }

    return Builder->CreateLoad(alloca->getAllocatedType(), alloca, name);
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

