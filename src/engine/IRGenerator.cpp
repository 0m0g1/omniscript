#include <omniscript/engine/IRGenerator.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/IR/PassManager.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Passes/StandardInstrumentations.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Linker/Linker.h>
#include <llvm/IR/DataLayout.h>
#include <llvm/Support/Alignment.h>
#include <omniscript/debuggingtools/console.h>

IRGenerator::IRGenerator(const std::string& mainModulePath) {
    Context = std::make_unique<llvm::LLVMContext>();
    Module = std::make_unique<llvm::Module>(mainModulePath, *Context);
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
        llvm::errs() << "Module verification failed!\n";
    } else {
        llvm::errs() << "No errors found.\n";
    }
}

void IRGenerator::printErrors(llvm::Module& module) {
    if (llvm::verifyModule(module, &llvm::errs())) {
        llvm::errs() << "Module verification failed!\n";
    } else {
        llvm::errs() << "No errors found.\n";
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

void IRGenerator::generateModule(const std::string& modulePath, const std::vector<std::shared_ptr<Statement>>& statements) {
    console.debug("Generating module " + modulePath);
    if (generatedModules.find(modulePath) != generatedModules.end()) {
        return;
    }
    
    auto newModule = std::make_unique<llvm::Module>(modulePath, *Context);
    llvm::Module* previousModule = CurrentModule;
    CurrentModule = newModule.get();
    
    std::unordered_map<std::string, llvm::Value*> publicMembers;
    
    for (const auto& statement : statements) {
        if (auto moduleStatement = std::dynamic_pointer_cast<CreateModule>(statement)) {
            console.debug("Generating a submodule");
            generateModule(moduleStatement->getName(), moduleStatement->getStatements());
        } else if (auto publicStatement = std::dynamic_pointer_cast<PublicMember>(statement)) {
            console.debug("Creating public member " + publicStatement->getName());
            llvm::Value* value = publicStatement->codegen(*this);
            if (value) {
                if (llvm::GlobalValue* gv = llvm::dyn_cast<llvm::GlobalValue>(value)) {
                    gv->setLinkage(llvm::GlobalValue::ExternalLinkage);  // Ensure it's visible externally
                    publicMembers[publicStatement->getName()] = gv;
                } else {
                    console.warn("Warning: Public statement '" + publicStatement->getName() + "' is not a global value and cannot have linkage visibility set to private or public.");
                }
            }
        } else if (auto privateStatement = std::dynamic_pointer_cast<PrivateMember>(statement)) {
            console.debug("Creating private member " + privateStatement->getName());
            llvm::Value* value = privateStatement->codegen(*this);
            if (value) {
                if (llvm::GlobalValue* gv = llvm::dyn_cast<llvm::GlobalValue>(value)) {
                    gv->setLinkage(llvm::GlobalValue::InternalLinkage);
                } else {
                    console.warn("Warning: Private statement '" + privateStatement->getName() + "' is not a global value and cannot have linkage visibility set to private or public.");
                }
            }
        } else {
            console.debug("Creating an unnamed instruction inside " + modulePath);
            llvm::Value* value = statement->codegen(*this);
            if (llvm::GlobalValue* gv = llvm::dyn_cast<llvm::GlobalValue>(value)) {
                gv->setLinkage(llvm::GlobalValue::InternalLinkage);
            }            
        }
    }
    
    printErrors(*CurrentModule);

    CurrentModule = previousModule;
    modulePublicSymbols[modulePath] = std::move(publicMembers);
    
    if (llvm::Linker::linkModules(*Module, std::move(newModule))) {
        llvm::errs() << "Error: Linking failed!\n";
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
    } else if (dataTypes[0] == "i128") {
        return llvm::IntegerType::get(context, 128);
    } else if (dataTypes[0] == "i256") {
        return llvm::IntegerType::get(context, 256);
    } else if (dataTypes[0] == "i1024") {
        return llvm::IntegerType::get(context, 1024);
    } else if (dataTypes[0] == "f32") {
        return llvm::Type::getFloatTy(context);
    } else if (dataTypes[0] == "f64") {
        return llvm::Type::getDoubleTy(context);
    }

    std::cerr << "Unknown type: " << dataTypes[0] << std::endl;
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
llvm::Value* IRGenerator::create128BitBigInt(const std::string& str) {
    llvm::APInt bigIntValue(128, str, 10); // 1024-bit integer from string (base 10)
    return llvm::ConstantInt::get(*Context, bigIntValue);
}
llvm::Value* IRGenerator::create256BitBigInt(const std::string& str) {
    llvm::APInt bigIntValue(256, str, 10); // 1024-bit integer from string (base 10)
    return llvm::ConstantInt::get(*Context, bigIntValue);
}
llvm::Value* IRGenerator::create1024BitBigInt(const std::string& str) {
    llvm::APInt bigIntValue(1024, str, 10); // 1024-bit integer from string (base 10)
    return llvm::ConstantInt::get(*Context, bigIntValue);
}


llvm::Value* IRGenerator::createVariable(
    const std::string& name, llvm::Type* type, llvm::Value* initialValue, bool isGlobal) {

    llvm::Module* activeModule = CurrentModule; // Get the correct active module

    if (isGlobal) {
        // Create a global variable inside the active module
        llvm::GlobalVariable* gVar = new llvm::GlobalVariable(
            *activeModule,
            type,
            false,  // Not constant
            llvm::GlobalValue::ExternalLinkage,
            llvm::Constant::getNullValue(type), // Default initializer
            name
        );

        // Set the initializer if the value is constant
        if (llvm::Constant* constValue = llvm::dyn_cast<llvm::Constant>(initialValue)) {
            gVar->setInitializer(constValue);
        }

        return gVar;
    } 
    
    // LOCAL VARIABLE CASE:
    llvm::Function* function = Builder->GetInsertBlock()->getParent();  
    llvm::IRBuilder<> tempBuilder(&function->getEntryBlock(), function->getEntryBlock().begin());

    // Place allocation in the entry block
    llvm::AllocaInst* alloca = tempBuilder.CreateAlloca(type, nullptr, name);

    // 🔹 **Set Correct Alignment for Big Integers**
    unsigned bitWidth = type->getIntegerBitWidth();
    unsigned align = 1;

    if (bitWidth >= 1024) align = 128;
    else if (bitWidth >= 256) align = 32;
    else if (bitWidth >= 128) align = 16;
    else if (bitWidth >= 64) align = 8;
    else if (bitWidth >= 32) align = 4;
    else if (bitWidth >= 16) align = 2;

    alloca->setAlignment(llvm::Align(llvm::MaybeAlign(align).value_or(llvm::Align(1))));

    NamedValues[name] = alloca;

    // Ensure stores happen before the return statement
    llvm::BasicBlock* insertBlock = Builder->GetInsertBlock();
    if (llvm::ReturnInst* retInst = llvm::dyn_cast<llvm::ReturnInst>(insertBlock->getTerminator())) {
        Builder->SetInsertPoint(retInst);  // Insert before return
    }

    // 🔹 **Check if Initial Value Exists Before Storing**
    if (initialValue) {
        Builder->CreateStore(initialValue, alloca)->setAlignment(llvm::Align(align));
    }

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

