#include <omniscript/engine/IRGenerator.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/IR/PassManager.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Passes/StandardInstrumentations.h>
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

    // Check if there's already an entry block
    llvm::Function* function = Module->getFunction("main");
    if (!function) {
        // Create function type: void main()
        llvm::FunctionType* funcType = llvm::FunctionType::get(llvm::Type::getVoidTy(*Context), false);
        function = llvm::Function::Create(funcType, llvm::Function::ExternalLinkage, "main", Module.get());
    }

    if (function->empty()) {
        // Create and insert the entry block if it doesn't exist
        llvm::BasicBlock* entryBlock = llvm::BasicBlock::Create(*Context, "entry", function);
        Builder->SetInsertPoint(entryBlock);
    } else {
        // Set insert point to the existing entry block
        Builder->SetInsertPoint(&function->getEntryBlock());
    }
}


void IRGenerator::printIR() {
    Module->print(llvm::errs(), nullptr);
}

void IRGenerator::optimizeModule() {
    console.log("here 1.1");
    llvm::LoopAnalysisManager lam;
    llvm::FunctionAnalysisManager fam;
    llvm::CGSCCAnalysisManager cam;
    llvm::ModuleAnalysisManager mam;
    console.log("here 1.2");
    
    llvm::PassBuilder pb;
    console.log("here 1.3");
    pb.registerModuleAnalyses(mam);
    console.log("here 1.3");
    pb.registerFunctionAnalyses(fam);
    console.log("here 1.4");
    pb.registerLoopAnalyses(lam);
    console.log("here 1.5");
    pb.registerCGSCCAnalyses(cam);
    console.log("here 1.6");
    
    llvm::ModulePassManager mpm = pb.buildPerModuleDefaultPipeline(llvm::OptimizationLevel::O2);
    console.log("here 1.7");
    
    try {
        console.log("here 1.7");
        mpm.run(*Module, mam);
        console.log("here 1.8");
    } catch (const std::exception &e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "Unknown error in optimizeModule!" << std::endl;
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
    
    llvm::AllocaInst* alloca = tempBuilder.CreateAlloca(type, nullptr, name);  // Ensure alloca is in entry block
    Builder->CreateStore(initialValue, alloca);
    
    NamedValues[name] = alloca;
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

