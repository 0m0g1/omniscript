#include <omniscript/engine/Backends/llvm/IRGenerator.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/IR/PassManager.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Passes/StandardInstrumentations.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Linker/Linker.h>
#include <llvm/IR/DataLayout.h>
#include <llvm/Support/Alignment.h>
#include <llvm/ADT/StringMap.h>         // Needed for getHostCPUFeatures
#include <llvm/IR/Constants.h>  // Required for appendToGlobalCtors
#include <llvm/Support/TargetSelect.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/TargetParser/Host.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/IR/DataLayout.h>
#include <llvm/IR/LegacyPassManager.h>
#include <optional>

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
    // Create context, module, builder if not yet created
    if (!Context)
        Context = std::make_unique<llvm::LLVMContext>();

    if (!Module)
        Module = std::make_unique<llvm::Module>("OmniScript", *Context);

    if (!Builder)
        Builder = std::make_unique<llvm::IRBuilder<>>(*Context);

    // Initialize target
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();

    std::string error;
    std::string triple = llvm::sys::getDefaultTargetTriple();

    Module->setTargetTriple(triple);

    const llvm::Target* target = llvm::TargetRegistry::lookupTarget(triple, error);
    if (!target) {
        llvm::errs() << "Target lookup failed: " << error << "\n";
        return;
    }

    llvm::TargetOptions options;
    auto targetMachine = std::unique_ptr<llvm::TargetMachine>(
        target->createTargetMachine(triple, "generic", "", options, std::nullopt)
    );

    Module->setDataLayout(targetMachine->createDataLayout());

    // Optional: Setup main or top-level function
    llvm::FunctionType* funcType = llvm::FunctionType::get(llvm::Type::getVoidTy(*Context), false);
    llvm::Function* topFunc = llvm::Function::Create(funcType, llvm::Function::ExternalLinkage, "__top_level__", Module.get());
    llvm::BasicBlock* entry = llvm::BasicBlock::Create(*Context, "entry", topFunc);
    Builder->SetInsertPoint(entry);
    Builder->CreateRetVoid(); // placeholder

    CurrentModule = Module.get();
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

std::string IRGenerator::debugType(llvm::Type* type) {
    std::string str;
    llvm::raw_string_ostream rso(str);
    type->print(rso);
    return rso.str();
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

llvm::Value* IRGenerator::codegen(std::shared_ptr<Omniscript::Expression> value, std::shared_ptr<SymbolTable<std::shared_ptr<Omniscript::Expression>>> scope) {
    llvm::Value* result = codegenPrimitive(value, scope);

    if (result) {
        return result;
    }

    // Handle VariableAssignment
    if (auto varAssign = std::dynamic_pointer_cast<Omniscript::VariableAssignment>(value)) {
        DEBUG_LOG("Assigning variable " + varAssign->variableName + " of type " + varAssign->getType()->kindName());
        llvm::Type* type = resolveLLVMType(varAssign->getType());
        DEBUG_LOG("Variable '" + varAssign->variableName + "' has type '" + debugType(type) + "'.");
        llvm::Value* value = codegen(varAssign->getValue(), scope);
        DEBUG_LOG("Got variable '" + varAssign->variableName + "''s value.");
        return createVariable(
            varAssign->variableName,
            type,
            value, 
            false
        );
    }

    // Handle ReferenceValue
    if (auto refValue = std::dynamic_pointer_cast<Omniscript::ReferenceExpression>(value)) {
        DEBUG_LOG("Creating reference to variable " + refValue->referentName);
        return getReferenceToVariable(refValue->referentName);
    }
    
    if (auto addressOf = std::dynamic_pointer_cast<Omniscript::AddressOfExpression>(value)) {
        DEBUG_LOG("Getting the address of variable " + addressOf->variableName);
        return getAddressOf(addressOf->variableName);
    }

    if (auto null = std::dynamic_pointer_cast<Omniscript::NullExpression>(value)) {
        DEBUG_LOG("Creating a null value");
        return createNullValue();
    }
    
    if (auto block = std::dynamic_pointer_cast<Omniscript::BlockExpression>(value)) {
        DEBUG_LOG("Evaluating a block value");

        for (const auto& expr : block->values) {
            DEBUG_LOG();
            auto result = codegen(expr, scope);
        }

        return nullptr;
    }

    if (auto nullpointer = std::dynamic_pointer_cast<Omniscript::NullPointerExpression>(value)) {
        DEBUG_LOG("Creating a null pointer");
        return createNullPointer();
    }

    if (auto func = std::dynamic_pointer_cast<Omniscript::FunctionExpression>(value)) {
        DEBUG_LOG("Creating an overload for function " + func->name + " with mangled name '" + func->mangledName + "'");
        llvm::Type* returnType = resolveLLVMType(func->getType()->getReturnType());
        return createFunction(func->mangledName, func->body, returnType, func->parameters, scope);
    }

    if (auto ret = std::dynamic_pointer_cast<Omniscript::ReturnExpression>(value)) {
        DEBUG_LOG("Creating a return statement of kind '" + ret->getType()->kindName() + "'.");

        llvm::Type* type = resolveLLVMType(ret->getType());
        llvm::Value* val = codegen(ret->value, scope);
        return createReturn(val, type);
    }

    if (auto unary = std::dynamic_pointer_cast<Omniscript::UnaryExpression>(value)) {
        DEBUG_LOG("Creating a unary expression");
        llvm::Value* operandVal = codegen(unary->operand, scope);
        if (!operandVal) return nullptr;
        return createUnaryExpression(operandVal, unary->op, unary->position);
    }

    if (auto binary = std::dynamic_pointer_cast<Omniscript::BinaryExpression>(value)) {
        DEBUG_LOG("Creating a binary expression");
        llvm::Value* lhs = codegen(binary->left, scope);
        llvm::Value* rhs = codegen(binary->right, scope);
        if (!lhs || !rhs) return nullptr;
        return createBinaryExpression(lhs, binary->op, rhs);
    }

    if (auto ternary = std::dynamic_pointer_cast<Omniscript::TernaryExpression>(value)) {
        DEBUG_LOG("Creating a ternary expression");
        llvm::Value* cond = codegen(ternary->condition, scope);
        llvm::Value* truthy = codegen(ternary->truthy, scope);
        llvm::Value* falsey = codegen(ternary->falsey, scope);
        if (!cond || !truthy || !falsey) return nullptr;
        return createTernaryExpression(cond, truthy, falsey);
    }

    if (auto var = std::dynamic_pointer_cast<Omniscript::VariableAccess>(value)) {
        DEBUG_LOG("Accessing variable: " + var->variableName);
        return getVariable(var->variableName);
    }

    if (auto call = std::dynamic_pointer_cast<Omniscript::CallExpression>(value)) {
        DEBUG_LOG("Calling " + call->calleeName);
        
        std::vector<llvm::Value*> args;
        args.reserve(call->args.size());

        for (const auto& arg : call->args) {
            args.emplace_back(codegen(arg, scope));
        }

        return createCall(call->calleeName, args);
    }

    return nullptr;
}

llvm::Value* IRGenerator::codegenPrimitive(std::shared_ptr<Omniscript::Expression> value, std::shared_ptr<SymbolTable<std::shared_ptr<Omniscript::Expression>>> scope) {

    // Handle 8-bit integer (int8_t)
    if (auto integer8 = std::dynamic_pointer_cast<Omniscript::Integer<int8_t>>(value)) {
        return create8BitInteger(integer8->getValue());
    }
    // Handle 16-bit integer (int16_t)
    else if (auto integer16 = std::dynamic_pointer_cast<Omniscript::Integer<int16_t>>(value)) {
        return create16BitInteger(integer16->getValue());
    }
    // Handle 32-bit integer (int32_t)
    else if (auto integer32 = std::dynamic_pointer_cast<Omniscript::Integer<int32_t>>(value)) {
        return create32BitInteger(integer32->getValue());
    }
    // Handle 64-bit integer (int64_t)
    else if (auto integer64 = std::dynamic_pointer_cast<Omniscript::Integer<int64_t>>(value)) {
        return create64BitInteger(integer64->getValue());

    } else if (auto unsignedInteger8 = std::dynamic_pointer_cast<Omniscript::Integer<uint8_t>>(value)) {
        return createUnsigned8BitInteger(unsignedInteger8->getValue());
    }
    // Handle unsigned 16-bit integer
    else if (auto unsignedInteger16 = std::dynamic_pointer_cast<Omniscript::Integer<uint16_t>>(value)) {
        return createUnsigned16BitInteger(unsignedInteger16->getValue());
    }
    // Handle unsigned 32-bit integer
    else if (auto unsignedInteger32 = std::dynamic_pointer_cast<Omniscript::Integer<uint32_t>>(value)) {
        return createUnsigned32BitInteger(unsignedInteger32->getValue());
    }
    // Handle unsigned 64-bit integer
    else if (auto unsignedInteger64 = std::dynamic_pointer_cast<Omniscript::Integer<uint64_t>>(value)) {
        return createUnsigned64BitInteger(unsignedInteger64->getValue());
    }
    // Handle boolean (bool)
    else if (auto boolean = std::dynamic_pointer_cast<Omniscript::Primitive<bool>>(value)) {
        return createBool(boolean->getValue());
    }
    // Handle half (16-bit floating-point)
    // else if (auto halfPrimitive = std::dynamic_pointer_cast<Omniscript::Float<__half>>(value)) {
    //     return create16BitFloat(halfPrimitive->getValue());
    // }
    // Handle float (32-bit floating-point)
    else if (auto floatPrimitive = std::dynamic_pointer_cast<Omniscript::Float<float>>(value)) {
        return create32BitFloat(floatPrimitive->getValue());
    }
    // Handle double (64-bit floating-point)
    else if (auto doublePrimitive = std::dynamic_pointer_cast<Omniscript::Float<double>>(value)) {
        return create64BitFloat(doublePrimitive->getValue());
    }
    // Handle FP128 (128-bit floating-point)
    else if (auto fp128Primitive = std::dynamic_pointer_cast<Omniscript::Float<__float128>>(value)) {
        return create128BitFloat(fp128Primitive->getValue());
    }
    else if (auto charPrimitive = std::dynamic_pointer_cast<Omniscript::Primitive<char>>(value)) {
        DEBUG_LOG("Creating and int8 char from Primitive<char>");
        return createChar(charPrimitive->getValue());
    }
    // Handle string (std::string)
    else if (auto stringPrimitiveUTF8 = std::dynamic_pointer_cast<Omniscript::Primitive<std::string>>(value)) {
        DEBUG_LOG("Creating UTF-8 string from Primitive<std::string>");
        return createUTF8String(stringPrimitiveUTF8->getValue());
    } else if (auto stringPrimitiveUTF16 = std::dynamic_pointer_cast<Omniscript::Primitive<std::u16string>>(value)) {
        DEBUG_LOG("Creating UTF-16 string from Primitive<std::u16string>");
        return createUTF16String(stringPrimitiveUTF16->getValue());
    } else if (auto stringPrimitiveUTF32 = std::dynamic_pointer_cast<Omniscript::Primitive<std::u32string>>(value)) {
        DEBUG_LOG("Creating UTF-32 string from Primitive<std::u32string>");
        return createUTF32String(stringPrimitiveUTF32->getValue());
    }    
    // Handle BigInt (std::string for now)
    else if (auto bigInt = std::dynamic_pointer_cast<Omniscript::BigInt>(value)) {
        DEBUG_LOG("Creating a Big int");
        return createBigInt(bigInt->getValue(), bigInt->getBitWidth());
    
    } else if (auto arry = std::dynamic_pointer_cast<Omniscript::FixedArrayExpression>(value)) {
        DEBUG_LOG("Creating a fixed array");
        std::vector<llvm::Value*> elems;

        for (const auto elem : arry->elements) {
            DEBUG_LOG("Creating an array element");
            elems.push_back(this->codegen(elem, scope));
        }
        
        llvm::Type* elementType = resolveLLVMType(arry->elementType);

        return createFixedArray(elementType, elems.size(), elems);
    }

    // Return nullptr if no matching type was found
    return nullptr;
}



void IRGenerator::generateModule(
    const std::string& modulePath,
    const std::string& alias,
    const std::vector<std::shared_ptr<Statement>>& statements,
    const std::unordered_map<std::string, std::string>& importedAliases, // Alias -> Original
    bool importAll
) {
    
    DEBUG_LOG("Generating module: " + modulePath);
    
    // if (scope->moduleExists(modulePath)) {
    //     DEBUG_LOG("Module " + modulePath + " is already generated. Skipping.");
    //     return;
    // }

    // 🔹 Store public members in a new module symbol table
    // pushScope();

    // auto newModule = std::make_unique<llvm::Module>(modulePath, *Context);
    // llvm::Module* previousModule = CurrentModule;
    // CurrentModule = newModule.get();

    // std::unordered_map<std::string, llvm::Value*> publicMembers;

    // for (const auto& statement : statements) {
    //     if (auto moduleStatement = std::dynamic_pointer_cast<CreateModule>(statement)) {
    //         DEBUG_LOG("Generating submodule: " + moduleStatement->getName());
    //         generateModule(moduleStatement->getName(), moduleStatement->getName(), moduleStatement->getStatements(), importedAliases, false);

    //     } else if (auto publicStatement = std::dynamic_pointer_cast<PublicMember>(statement)) {
    //         if (auto moduleStatement = std::dynamic_pointer_cast<CreateModule>(publicStatement->getValue())) {
    //             DEBUG_LOG("Generating submodule: " + moduleStatement->getName());
    //             generateModule(moduleStatement->getName(), moduleStatement->getName(), moduleStatement->getStatements(), importedAliases, false);
    //             continue;
    //         }
    //         std::string originalName = publicStatement->getName();
    //         llvm::Value* value = publicStatement->codegen(*this);
    //         if (!value) {
    //             console.warn("Skipping public statement '" + originalName + "' - no valid IR generated.");
    //             continue;
    //         }

    //         if (llvm::GlobalValue* gv = llvm::dyn_cast<llvm::GlobalValue>(value)) {
    //             if (importAll) {
    //                 gv->setLinkage(llvm::GlobalValue::ExternalLinkage);
    //                 // activeScope->set(originalName, gv);
    //             } else {
    //                 auto it = importedAliases.find(originalName);
    //                 if (it != importedAliases.end()) {
    //                     std::string alias = it->second; // Aliased name
    //                     gv->setLinkage(llvm::GlobalValue::ExternalLinkage);
    //                     DEBUG_LOG("Aliased import: " + alias + " -> " + originalName);
    //                 } else {
    //                     gv->setLinkage(llvm::GlobalValue::InternalLinkage);
    //                 }
    //             }
    //         } else {
    //             console.warn("Warning: Public symbol '" + originalName +
    //                          "' is not a global value and cannot have linkage visibility set.");
    //         }
    //     } else if (auto privateStatement = std::dynamic_pointer_cast<PrivateMember>(statement)) {
    //         DEBUG_LOG("Creating private member " + privateStatement->getName());
    //         llvm::Value* value = privateStatement->codegen(*this);
    //         if (importedAliases.find(privateStatement->getName()) != importedAliases.end()) {
    //             console.warn("'" + privateStatement->getName() + "' is not a public member of '" + modulePath + "' and cannot be imported");
    //         }
    //         if (value) {
    //             if (llvm::GlobalValue* gv = llvm::dyn_cast<llvm::GlobalValue>(value)) {
    //                 gv->setLinkage(llvm::GlobalValue::InternalLinkage);
    //             } else {
    //                 console.warn("Warning: Private statement '" + privateStatement->getName() + "' is not a global value and cannot have linkage visibility set to private or public.");
    //             }
    //         }
    //     }
    // }

    // printErrors(*CurrentModule);
    // CurrentModule = previousModule;

    // popScope();
    
    // if (llvm::Linker::linkModules(*Module, std::move(newModule))) {
    //     llvm::errs() << "Error: Linking failed for module " << modulePath << "!\n";
    // }
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
        activeScope->set(name, value);
    }
}


llvm::Type* IRGenerator::resolveLLVMType(std::shared_ptr<Omniscript::Type> type) {
    if (!type) {
        std::cerr << "[ERROR] Type is null!" << std::endl;
        return nullptr;
    }

    DEBUG_LOG("Resolving a '" + type->kindName() + "'.");
    llvm::LLVMContext& context = *Context;

    // If the type is an array, resolve the base type first.
    if (type->isArray()) {
        DEBUG_LOG("The array is of size '" + std::to_string(type->fixedSize) + "' and holds type " + type->elementType->kindName() + "'.");
        auto elementType = resolveLLVMType(type->elementType);
        uint64_t arraySize = type->fixedSize;
        return llvm::ArrayType::get(elementType, arraySize);
    }

    // If the type is a pointer, resolve the base type and add pointer depth.
    if (type->isPointer()) {
        if (auto pointer = std::dynamic_pointer_cast<Omniscript::PointerType>(type)) {
            int pointerDepth;
            llvm::Type* pointeeType;
            pointeeType = resolveLLVMType(pointer->getBasePointeeType());
            pointerDepth = pointer->getPointerDepth();
            return llvm::PointerType::get(pointeeType, pointerDepth);
        }

        if (auto nullpointer = std::dynamic_pointer_cast<Omniscript::NullPointerType>(type)) {
            return llvm::PointerType::getUnqual(llvm::Type::getInt8Ty(context));
        }
        
    }

    // If the type is a reference, treat it as a pointer.
    if (type->isReference()) {
        auto referencedType = resolveLLVMType(type->getReferencedType());
        return llvm::PointerType::get(referencedType, 0);
    }

    // Resolve the base type kind
    llvm::Type* llvmType = nullptr;

    switch (type->getKind()) {
        case Omniscript::Kind::Int8:
            DEBUG_LOG("Generating LLVM type: Int8");
            llvmType = llvm::Type::getInt8Ty(context);
            break;
        case Omniscript::Kind::Int16:
            DEBUG_LOG("Generating LLVM type: Int16");
            llvmType = llvm::Type::getInt16Ty(context);
            break;
        case Omniscript::Kind::Int32:
            DEBUG_LOG("Generating LLVM type: Int32");
            llvmType = llvm::Type::getInt32Ty(context);
            break;
        case Omniscript::Kind::Int64:
            DEBUG_LOG("Generating LLVM type: Int64");
            llvmType = llvm::Type::getInt64Ty(context);
            break;
        case Omniscript::Kind::Int128:
            DEBUG_LOG("Generating LLVM type: Int128");
            llvmType = llvm::IntegerType::get(context, 128);
            break;
        case Omniscript::Kind::Int256:
            DEBUG_LOG("Generating LLVM type: Int256");
            llvmType = llvm::IntegerType::get(context, 256);
            break;
        case Omniscript::Kind::Int512:
            DEBUG_LOG("Generating LLVM type: Int512");
            llvmType = llvm::IntegerType::get(context, 512);
            break;
        case Omniscript::Kind::Int1024:
            DEBUG_LOG("Generating LLVM type: Int1024");
            llvmType = llvm::IntegerType::get(context, 1024);
            break;
        case Omniscript::Kind::BigInt:
            DEBUG_LOG("Generating LLVM type: BigInt (treated as Int1024)");
            llvmType = llvm::IntegerType::get(context, 1024);
            break;
        case Omniscript::Kind::UInt8:
            DEBUG_LOG("Generating LLVM type: UInt8");
            llvmType = llvm::Type::getInt8Ty(context);
            break;
        case Omniscript::Kind::UInt16:
            DEBUG_LOG("Generating LLVM type: UInt16");
            llvmType = llvm::Type::getInt16Ty(context);
            break;
        case Omniscript::Kind::UInt32:
            DEBUG_LOG("Generating LLVM type: UInt32");
            llvmType = llvm::Type::getInt32Ty(context);
            break;
        case Omniscript::Kind::UInt64:
            DEBUG_LOG("Generating LLVM type: UInt64");
            llvmType = llvm::Type::getInt64Ty(context);
            break;
        case Omniscript::Kind::UInt128:
            DEBUG_LOG("Generating LLVM type: UInt128");
            llvmType = llvm::IntegerType::get(context, 128);
            break;
        case Omniscript::Kind::UInt256:
            DEBUG_LOG("Generating LLVM type: UInt256");
            llvmType = llvm::IntegerType::get(context, 256);
            break;
        case Omniscript::Kind::UInt512:
            DEBUG_LOG("Generating LLVM type: UInt512");
            llvmType = llvm::IntegerType::get(context, 512);
            break;
        case Omniscript::Kind::UInt1024:
            DEBUG_LOG("Generating LLVM type: UInt1024");
            llvmType = llvm::IntegerType::get(context, 1024);
            break;
        case Omniscript::Kind::Half:
            DEBUG_LOG("Generating LLVM type: Half");
            llvmType = llvm::Type::getHalfTy(context);
            break;
        case Omniscript::Kind::Float:
            DEBUG_LOG("Generating LLVM type: Float");
            llvmType = llvm::Type::getFloatTy(context);
            break;
        case Omniscript::Kind::Double:
            DEBUG_LOG("Generating LLVM type: Double");
            llvmType = llvm::Type::getDoubleTy(context);
            break;
        case Omniscript::Kind::FP128:
            DEBUG_LOG("Generating LLVM type: FP128");
            llvmType = llvm::Type::getFP128Ty(context);
            break;
        case Omniscript::Kind::X86_FP80:
            DEBUG_LOG("Generating LLVM type: X86_FP80");
            llvmType = llvm::Type::getX86_FP80Ty(context);
            break;
        case Omniscript::Kind::PPC_FP128:
            DEBUG_LOG("Generating LLVM type: PPC_FP128");
            llvmType = llvm::Type::getPPC_FP128Ty(context);
            break;
        case Omniscript::Kind::Char:
            DEBUG_LOG("Generating LLVM type: Char (as Int8)");
            llvmType = llvm::Type::getInt8Ty(context);
            break;
        case Omniscript::Kind::Bool:
            DEBUG_LOG("Generating LLVM type: Bool (as Int1)");
            llvmType = llvm::Type::getInt1Ty(context);
            break;
        case Omniscript::Kind::Void:
            DEBUG_LOG("Generating LLVM type: Void");
            llvmType = llvm::Type::getVoidTy(context);
            break;
        case Omniscript::Kind::Null:
            DEBUG_LOG("Generating LLVM type: Null (as pointer to i8)");
            llvmType = llvm::PointerType::getUnqual(llvm::Type::getInt8Ty(context));
            break;
        case Omniscript::Kind::Utf8:
            DEBUG_LOG("Generating LLVM type: Utf8 (i8*)");
            llvmType = llvm::PointerType::get(llvm::Type::getInt8Ty(context), 0);
            break;
        case Omniscript::Kind::Utf16:
            DEBUG_LOG("Generating LLVM type: Utf16 (i16*)");
            llvmType = llvm::PointerType::get(llvm::Type::getInt16Ty(context), 0);
            break;
        case Omniscript::Kind::Utf32:
            DEBUG_LOG("Generating LLVM type: Utf32 (i32*)");
            llvmType = llvm::PointerType::get(llvm::Type::getInt32Ty(context), 0);
            break;
        default:
            std::cerr << "[ERROR] Unknown type: " << type->kindName() << std::endl;
            return nullptr;
    }
    

    return llvmType;
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
llvm::Value* IRGenerator::create8BitInteger(int8_t value) {
    return llvm::ConstantInt::get(llvm::Type::getInt8Ty(*Context), value, true);
}

// Create a 16-bit integer (i16)
llvm::Value* IRGenerator::create16BitInteger(int16_t value) {
    return llvm::ConstantInt::get(llvm::Type::getInt16Ty(*Context), value, true);
}

// Create a 32-bit integer (i32)
llvm::Value* IRGenerator::create32BitInteger(int32_t value) {
    return llvm::ConstantInt::get(llvm::Type::getInt32Ty(*Context), value, true);
}

// Create a 64-bit integer (i64)
llvm::Value* IRGenerator::create64BitInteger(int64_t value) {
    return llvm::ConstantInt::get(llvm::Type::getInt64Ty(*Context), value, true);
}

// Create a 16-bit floating-point (half)
// llvm::Value* IRGenerator::create16BitFloat(__half value) {
//     return llvm::ConstantFP::get(llvm::Type::getHalfTy(*Context), static_cast<float>(value));  // Half precision needs to be converted to float
// }

// Create a 32-bit floating-point (float)
llvm::Value* IRGenerator::create32BitFloat(float value) {
    return llvm::ConstantFP::get(llvm::Type::getFloatTy(*Context), value);
}

// Create a 64-bit floating-point (double)
llvm::Value* IRGenerator::create64BitFloat(double value) {
    return llvm::ConstantFP::get(llvm::Type::getDoubleTy(*Context), value);
}

// Create a 128-bit floating-point (FP128)
llvm::Value* IRGenerator::create128BitFloat(__float128 value) {
    return llvm::ConstantFP::get(llvm::Type::getFP128Ty(*Context), static_cast<double>(value));  // FP128 is typically represented as a double in LLVM
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

llvm::Value* IRGenerator::createUnsigned8BitInteger(uint8_t value) {
    return llvm::ConstantInt::get(llvm::Type::getInt8Ty(*Context), value, false);  // Unsigned 8-bit integer
}

llvm::Value* IRGenerator::createUnsigned16BitInteger(uint16_t value) {
    return llvm::ConstantInt::get(llvm::Type::getInt16Ty(*Context), value, false);  // Unsigned 16-bit integer
}

llvm::Value* IRGenerator::createUnsigned32BitInteger(uint32_t value) {
    return llvm::ConstantInt::get(llvm::Type::getInt32Ty(*Context), value, false);  // Unsigned 32-bit integer
}

llvm::Value* IRGenerator::createUnsigned64BitInteger(uint64_t value) {
    return llvm::ConstantInt::get(llvm::Type::getInt64Ty(*Context), value, false);  // Unsigned 64-bit integer
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

llvm::Function* IRGenerator::getOrCreateGlobalInitFunction() {
    const char* initName = "__startup__";
    
    // First check if function already exists
    if (auto* existing = Module->getFunction(initName)) {
        return existing;
    }

    // Create function type (void -> void)
    auto* funcType = llvm::FunctionType::get(
        Builder->getVoidTy(), 
        false
    );

    // Create the function
    auto* func = llvm::Function::Create(
        funcType,
        llvm::Function::InternalLinkage,
        initName,
        Module.get()
    );

    // Create entry block
    auto* entry = llvm::BasicBlock::Create(
        Module->getContext(), 
        "entry", 
        func
    );

    Builder->SetInsertPoint(entry);
    Builder->CreateRetVoid(); // Ensure the function has a return

    // Get or create `llvm.global_ctors`
    llvm::GlobalVariable* globalCtors = Module->getNamedGlobal("llvm.global_ctors");
    
    llvm::StructType* ctorStructType = llvm::StructType::get(
        Builder->getInt32Ty(), // Priority
        func->getType(),       // Function pointer
        llvm::PointerType::getUnqual(Module->getContext()) // Data
    );

    llvm::Constant* ctorEntry = llvm::ConstantStruct::get(
        ctorStructType,
        {
            llvm::ConstantInt::get(Builder->getInt32Ty(), 0), // Priority = 0
            func,                                            // Function pointer
            llvm::Constant::getNullValue(
                llvm::PointerType::getUnqual(Module->getContext())
            ) // Data (nullptr)
        }
    );

    // If `llvm.global_ctors` exists, append the new function
    if (globalCtors) {
        auto* arrayType = llvm::dyn_cast<llvm::ArrayType>(globalCtors->getValueType());
        size_t existingSize = arrayType->getNumElements();
        
        std::vector<llvm::Constant*> ctorEntries;

        auto* existingInit = llvm::dyn_cast<llvm::ConstantArray>(globalCtors->getInitializer());
        for (size_t i = 0; i < existingSize; ++i) {
            ctorEntries.push_back(existingInit->getOperand(i));
        }

        // Add new constructor
        ctorEntries.push_back(ctorEntry);

        auto* newArrayType = llvm::ArrayType::get(ctorStructType, ctorEntries.size());
        auto* newInit = llvm::ConstantArray::get(newArrayType, ctorEntries);

        // Replace global variable with updated initializer
        globalCtors->setInitializer(newInit);
    } else {
        // If `llvm.global_ctors` doesn't exist, create it
        auto* arrayType = llvm::ArrayType::get(ctorStructType, 1);
        auto* globalCtorVar = new llvm::GlobalVariable(
            *Module,
            arrayType,
            false,
            llvm::GlobalValue::AppendingLinkage,
            llvm::ConstantArray::get(arrayType, {ctorEntry}),
            "llvm.global_ctors"
        );

        globalCtorVar->setAlignment(llvm::Align(8));
    }

    return func;
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

llvm::Value* IRGenerator::createVariable( 
    const std::string& name, 
    llvm::Type* type, 
    llvm::Value* initialValue, 
    bool isGlobal, 
    llvm::BasicBlock* activeBlock 
) {
    llvm::Module* activeModule = CurrentModule;

    DEBUG_LOG("Creating variable: " + name + (isGlobal ? " (global)" : " (local)"));

    if (isGlobal) {
        llvm::GlobalVariable* gVar = new llvm::GlobalVariable(
            *activeModule,
            type,
            false,
            llvm::GlobalValue::ExternalLinkage,
            initialValue ? llvm::dyn_cast<llvm::Constant>(initialValue) 
                         : llvm::Constant::getNullValue(type),
            name
        );

        DEBUG_LOG("Global variable '" + name + "' created with type: " + debugType(type));

        if (!gVar->hasInitializer() && initialValue) {
            console.warn("Warning: Global variable '" + name + 
                         "' requires a constant initializer but received non-constant.");
        }

        return gVar;
    }

    // Local variable case
    llvm::Function* function = nullptr;
    llvm::IRBuilder<> tempBuilder(Builder->getContext());

    if (activeBlock) {
        function = activeBlock->getParent();
        tempBuilder.SetInsertPoint(activeBlock, activeBlock->begin());
    } else {
        function = Builder->GetInsertBlock()->getParent();
        tempBuilder.SetInsertPoint(&function->getEntryBlock(), function->getEntryBlock().begin());
    }

    llvm::AllocaInst* alloca = tempBuilder.CreateAlloca(type, nullptr, name);
    DEBUG_LOG("Local variable '" + name + "' allocated in function: " + function->getName().str());

    llvm::BasicBlock* entryBlock = &function->getEntryBlock();
    llvm::Instruction* retInst = nullptr;
    for (llvm::Instruction& I : *entryBlock) {
        if (llvm::isa<llvm::ReturnInst>(&I)) {
            retInst = &I;
            break;
        }
    }
    
    if (type->isArrayTy()) {
        DEBUG_LOG("Here");
        llvm::ArrayType* arrayType = llvm::dyn_cast<llvm::ArrayType>(type);
        DEBUG_LOG("Here 1");
        llvm::Type* elementType = arrayType->getElementType();
        DEBUG_LOG("Here 2");
        
        const llvm::DataLayout& dataLayout = activeModule->getDataLayout();
        DEBUG_LOG("Here 3");
        DEBUG_LOG("Element type of array: " + debugType(elementType));

        if (activeModule->getDataLayout().isDefault()) {
            DEBUG_LOG("Warning: DataLayout not initialized in activeModule!");
        }
        
        llvm::MaybeAlign maybeAlign = dataLayout.getABITypeAlign(elementType);
        unsigned elementAlign = 0;

        if (!maybeAlign) {
            DEBUG_LOG("Failed to get ABI alignment for element type of array '" + name + "'");
            elementAlign = 1; // Fallback
        } else {
            elementAlign = maybeAlign->value();
        }
        DEBUG_LOG("Here 4");

        alloca->setAlignment(llvm::Align(elementAlign));
        DEBUG_LOG("Set alignment for array '" + name + "' to " + std::to_string(elementAlign));

        if (initialValue) {
            if (auto* constArray = llvm::dyn_cast<llvm::ConstantArray>(initialValue)) {
                for (unsigned i = 0; i < arrayType->getNumElements(); ++i) {
                    llvm::Value* index = llvm::ConstantInt::get(
                        llvm::Type::getInt32Ty(activeModule->getContext()), i);
                    llvm::Value* elementPtr = Builder->CreateGEP(
                        arrayType, alloca, {Builder->getInt32(0), index});
                    llvm::StoreInst* store = Builder->CreateStore(
                        constArray->getAggregateElement(i), elementPtr);
                    store->setAlignment(llvm::Align(elementAlign));
                    if (retInst) store->moveBefore(*retInst->getParent(), retInst->getIterator());
                    DEBUG_LOG("Initialized array element " + std::to_string(i) + " of '" + name + "'");
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

        alloca->setAlignment(llvm::Align(align));
        DEBUG_LOG("Integer '" + name + "' alignment set to " + std::to_string(align));

        if (initialValue) {
            llvm::StoreInst* store = Builder->CreateStore(initialValue, alloca);
            store->setAlignment(llvm::Align(align));
            if (retInst) store->moveBefore(*retInst->getParent(), retInst->getIterator());
            DEBUG_LOG("Stored initial value for integer '" + name + "'");
        }
    } else if (type->isFloatingPointTy()) {
        unsigned align = 1;
        if (type->isHalfTy()) align = 2;
        else if (type->isFloatTy()) align = 4;
        else if (type->isDoubleTy()) align = 8;
        else if (type->isFP128Ty() || type->isX86_FP80Ty() || type->isPPC_FP128Ty()) align = 16;

        alloca->setAlignment(llvm::Align(align));
        DEBUG_LOG("Floating-point '" + name + "' alignment set to " + std::to_string(align));

        if (initialValue) {
            llvm::StoreInst* store = Builder->CreateStore(initialValue, alloca);
            store->setAlignment(llvm::Align(align));
            if (retInst) store->moveBefore(*retInst->getParent(), retInst->getIterator());
            DEBUG_LOG("Stored initial value for floating-point '" + name + "'");
        }
    } else if (type->isPointerTy()) {
        if (initialValue) {
            llvm::Type* initType = initialValue->getType();
            if (!initType->isPointerTy()) {
                llvm::errs() << "Error: Attempting to store a non-pointer value into a pointer variable.\n";
            } else {
                llvm::PointerType* targetPtrType = llvm::cast<llvm::PointerType>(type);
                llvm::PointerType* sourcePtrType = llvm::cast<llvm::PointerType>(initType);

                llvm::Value* castedValue = initialValue;
                std::string castType;

                if (sourcePtrType->getAddressSpace() != targetPtrType->getAddressSpace()) {
                    castedValue = Builder->CreateAddrSpaceCast(initialValue, targetPtrType, "addrspace.cast");
                    castType = "addrspacecast";
                } else if (initType != type) {
                    castedValue = Builder->CreateBitCast(initialValue, type, "bit.cast");
                    castType = "bitcast";
                }

                if (retInst) {
                    if (auto* castInst = llvm::dyn_cast<llvm::Instruction>(castedValue)) {
                        castInst->moveBefore(*retInst->getParent(), retInst->getIterator());
                    }
                }

                llvm::StoreInst* store = Builder->CreateStore(castedValue, alloca);
                if (retInst) store->moveBefore(*retInst->getParent(), retInst->getIterator());
                DEBUG_LOG("Pointer '" + name + "' initialized with " + castType);
            }
        }
    }

    activeScope->set(name, alloca);
    DEBUG_LOG("Variable '" + name + "' registered in active scope");
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
    return activeScope->get(varname);
}


llvm::Value* IRGenerator::getReferenceToVariable(const std::string& varname) {
    // if (!activeScope->has(varname)) {
    //     throw std::runtime_error("Cannot get reference to: " + varname);
    // }
    
    // // Return the pointer/alloca directly
    // return activeScope->get(varname);
    return nullptr;
}

llvm::Value* IRGenerator::getVariable(const std::string& name) {
    // // Check current scope
    // if (activeScope->exists(name)) {
    llvm::Value* val = activeScope->get(name);

    if (llvm::AllocaInst* alloca = llvm::dyn_cast<llvm::AllocaInst>(val)) {
//         // Generate the load operation (no need to change insertion point)
        return Builder->CreateLoad(alloca->getAllocatedType(), alloca, name + ".val");
    }

    return val;
    // }

    // throw std::runtime_error("Unknown variable name: " + name);
    return nullptr;
}


llvm::Value* IRGenerator::createConstant(const std::string& name, llvm::Type* type, llvm::Value* value) {
    activeScope->setConstant(name, value);
    return value;
}

llvm::Value* IRGenerator::createDynamicVariable(const std::string& name, llvm::Value* value) {
    llvm::IRBuilder<> builder(Builder->GetInsertBlock());
    llvm::AllocaInst* alloca = builder.CreateAlloca(value->getType(), nullptr, name);
    builder.CreateStore(value, alloca);
    activeScope->set(name, alloca);
    return alloca;
}

llvm::Value* IRGenerator::reassign(const std::string& name, llvm::Value* newValue) {
    Builder->CreateStore(newValue, activeScope->get(name));
    return newValue;
}


llvm::Value* IRGenerator::createDynamicConstant(const std::string& name, llvm::Value* value) {
    activeScope->setConstant(name, value);
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
    llvm::AllocaInst* alloca = llvm::dyn_cast<llvm::AllocaInst>(activeScope->get(name));
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

    activeScope->set(name, alloca);
    return alloca;
}

llvm::Value* IRGenerator::createFixedArray(
    llvm::Type* elementType,
    size_t size,
    const std::vector<llvm::Value*>& elements
) {

    DEBUG_LOG("Creating fixed array of size " + std::to_string(size));
    DEBUG_LOG("Element LLVM type: " + debugType(elementType));

    // Set insert point before terminator if present
    llvm::BasicBlock* currentBlock = Builder->GetInsertBlock();
    if (currentBlock && !currentBlock->empty() && currentBlock->back().isTerminator()) {
        DEBUG_LOG("Found terminator in current block, inserting before it");
        auto termIt = currentBlock->getTerminator()->getIterator();
        Builder->SetInsertPoint(currentBlock, termIt);
    }

    const llvm::DataLayout& dataLayout = CurrentModule != nullptr 
        ? CurrentModule->getDataLayout() 
        : Module->getDataLayout();

    unsigned elementAlign = dataLayout.getABITypeAlign(elementType).value();
    DEBUG_LOG("Element alignment: " + std::to_string(elementAlign));

    llvm::ArrayType* arrayType = llvm::ArrayType::get(elementType, size);
    DEBUG_LOG("Array LLVM type: " + debugType(arrayType));

    llvm::Value* arrayAlloc = Builder->CreateAlloca(arrayType, nullptr, "static_array");
    DEBUG_LOG("Array allocation created");

    llvm::cast<llvm::AllocaInst>(arrayAlloc)->setAlignment(llvm::Align(elementAlign));
    DEBUG_LOG("Set alignment of allocation");

    for (size_t i = 0; i < elements.size(); ++i) {
        DEBUG_LOG("Inserting element at index " + std::to_string(i));

        llvm::Value* element = elements[i];

        if (element->getType() != elementType) {
            DEBUG_LOG("Type mismatch: expected " + debugType(elementType) +
                      ", got " + debugType(element->getType()));
            element = Builder->CreateBitCast(element, elementType);
            DEBUG_LOG("Bitcasted element to match element type");
        }

        llvm::Value* elementPtr = Builder->CreateGEP(
            arrayType,
            arrayAlloc,
            {Builder->getInt32(0), Builder->getInt32(i)}
        );
        DEBUG_LOG("Computed GEP for index " + std::to_string(i));

        llvm::StoreInst* store = Builder->CreateStore(element, elementPtr);
        store->setAlignment(llvm::Align(elementAlign));
        DEBUG_LOG("Stored element at index " + std::to_string(i));
    }

    DEBUG_LOG("Finished creating fixed array");
    return arrayAlloc;
}

llvm::Function* IRGenerator::createFunction(
    const std::string& name,
    std::vector<std::shared_ptr<Omniscript::Expression>>& body,
    llvm::Type* returnType,
    std::vector<std::shared_ptr<Omniscript::Expression>>& params,
    std::shared_ptr<SymbolTable<std::shared_ptr<Omniscript::Expression>>> scope
) {
    DEBUG_LOG("Creating function: " + name + " with parameter size " + std::to_string(params.size()));
    
    // Create function type
    std::vector<llvm::Type*> paramTypes;
    for (auto& param : params) {
        auto type = param->getType();
        auto llvmType = resolveLLVMType(type);
        paramTypes.push_back(llvmType);
        
        DEBUG_LOG("Resolved parameter type: " + type->kindName() + " to LLVM type: " + debugType(llvmType));
    }

    DEBUG_LOG("Resolved return type LLVM: " + debugType(returnType));
    
    llvm::FunctionType* funcType = llvm::FunctionType::get(
        returnType,
        paramTypes,
        false
    );

    // Create function
    llvm::Function* function = llvm::Function::Create(
        funcType,
        llvm::Function::ExternalLinkage,
        name,
        CurrentModule
    );

    // Set parameter names
    unsigned idx = 0;
    for (auto& arg : function->args()) {
        auto& param = params[idx];
        arg.setName(param->name);

        DEBUG_LOG("Setting function argument: " + param->name + " of kind: " + param->getType()->kindName());
        idx++;
    }

    // Store function in scope
    activeScope->set(name, function);
    DEBUG_LOG("Stored function: " + name + " in scope");

    // Generate function body
    generateFunctionBody(function, body, scope);

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

void IRGenerator::generateFunctionBody( 
    llvm::Function* function,
    std::vector<std::shared_ptr<Omniscript::Expression>>& body,
    std::shared_ptr<SymbolTable<std::shared_ptr<Omniscript::Expression>>> scope
) {
    DEBUG_LOG("Generating body for function: " + function->getName().str());
    DEBUG_LOG("Function return type: " + debugType(function->getReturnType()));

    pushActiveBlock();

    // Create entry block
    llvm::BasicBlock* entry = llvm::BasicBlock::Create(*Context, "entry", function);
    Builder->SetInsertPoint(entry);
    DEBUG_LOG("Created entry block for function: " + function->getName().str());
    
    // Create a new scope for function parameters + body
    pushScope();
    DEBUG_LOG("Pushed new scope for function");

    // Create allocas for parameters in the entry block
    for (auto& arg : function->args()) {
        std::string argName = arg.getName().str();
        DEBUG_LOG("Allocating parameter: " + argName + " with type: " + debugType(arg.getType()));
        llvm::AllocaInst* alloca = createEntryBlockAlloca(function, arg.getType(), argName);
        Builder->CreateStore(&arg, alloca);
        activeScope->set(argName, alloca);
        DEBUG_LOG("Stored parameter in scope: " + argName);
    }

    // Generate function body
    llvm::Value* retVal = nullptr;
    for (const auto& expr : body) {
        DEBUG_LOG("Generating code for body expression of kind: " + expr->getType()->kindName());
        retVal = codegen(expr, scope);
        if (retVal) {
            DEBUG_LOG("Body expression result type: " + debugType(retVal->getType()));
        }
    }

    // Handle implicit return if needed
    if (!currentBlockHasTerminator()) {
        if (function->getReturnType()->isVoidTy()) {
            DEBUG_LOG("Creating void return for function: " + function->getName().str());
            Builder->CreateRetVoid();
        } else if (retVal) {
            DEBUG_LOG("Creating return with value type: " + debugType(retVal->getType()));
            DEBUG_LOG("Function expects return type: " + debugType(function->getReturnType()));

            // Ensure return value matches function type
            if (retVal->getType() != function->getReturnType()) {
                DEBUG_LOG("Return type mismatch: attempting cast");
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
                DEBUG_LOG("Cast successful. New return type: " + debugType(retVal->getType()));
            }
            Builder->CreateRet(retVal);
            DEBUG_LOG("Created return instruction for function: " + function->getName().str());
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
    DEBUG_LOG("Popped function scope");
    
    popActiveBlock();
    DEBUG_LOG("Popped active block");

    // Verify the function for consistency
    if (llvm::verifyFunction(*function, &llvm::errs())) {
        console.error("Function verification failed: " + function->getName().str());
        function->eraseFromParent();
    } else {
        DEBUG_LOG("Function verified successfully: " + function->getName().str());
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
        throw std::runtime_error(
                                "Return type mismatch: expected " + 
                                 debugType(expectedReturnType) + 
                                 ", got " + 
                                 debugType(returnValue->getType())
                                );
    }

    // Create the return instruction
    return Builder->CreateRet(returnValue);
}


bool IRGenerator::currentBlockHasTerminator() const {
    return Builder->GetInsertBlock()->getTerminator() != nullptr;
}

llvm::Value* IRGenerator::castValue(llvm::Value* val, llvm::Type* targetType) {
    return val;
    // if (val->getType() == targetType) return val;

    // if (val->getType()->isIntegerTy() && targetType->isIntegerTy()) {
    //     return Builder->CreateIntCast(val, targetType, /*isSigned=*/true);
    // }

    // if (val->getType()->isFloatingPointTy() && targetType->isFloatingPointTy()) {
    //     return Builder->CreateFPCast(val, targetType);
    // }

    // DEBUG_LOG("Unsupported cast from " + debugType(val->getType()) + " to " + debugType(targetType));
    // return nullptr;
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

llvm::Value* IRGenerator::createObjectInstance(
    const std::string& typeName,
    const std::string& varName,
    const std::vector<llvm::Value*>& args)
{
    DEBUG_LOG("Creating instance of type: " + typeName);
    
    // 1. Check for value types first
    // if (typeName == "int32") {
    //     return createPrimitiveInstance(
    //         Builder.getInt32Ty(), 
    //         varName,
    //         args.empty() ? nullptr : args[0]);
    // }
    // else if (typeName == "float") {
    //     return createPrimitiveInstance(
    //         Builder.getFloatTy(),
    //         varName,
    //         args.empty() ? nullptr : args[0]);
    // }
    // ... other primitives ...

    // 2. Check for struct types
    if (llvm::StructType::getTypeByName(*Context, typeName)) {
        return createStructInstance(typeName, varName, args);
    }

    // 3. Check for class types (might be in a different namespace)
    // if (/* class exists check */) {
    //     return createClassInstance(typeName, varName, args);
    // }

    console.error("Unknown type: " + typeName);
    return nullptr;
}

void IRGenerator::createStructType(const ConstructStructPrototype& structProto) {
    // 🔹 Check if the struct type already exists
    if (llvm::StructType::getTypeByName(*Context, structProto.getName())) {
        DEBUG_LOG("Struct " + structProto.getName() + " already exists. Skipping creation.");
        return;
    }

    // 🔹 Define struct type
    llvm::StructType* structType = llvm::StructType::create(*Context, structProto.getName());
    
    // 🔹 Collect field types
    std::vector<llvm::Type*> fieldTypes;
    for (const auto& field : structProto.getBody()) {
        if (auto varDecl = std::dynamic_pointer_cast<TypedStatement>(field)) {
            // fieldTypes.push_back(varDecl->getType());
        } else {
            console.warn("Skipping non-variable declaration in struct body");
        }
    }
    
    // 🔹 Set struct element types
    structType->setBody(fieldTypes);
    
    // 🔹 Store struct in symbol table
    // activeScope->set(structProto.getName(), structType);
    DEBUG_LOG("Created struct " + structProto.getName());
}

llvm::Value* IRGenerator::createStructInstance(
    const std::string& structName,
    const std::string& varName,
    const std::vector<llvm::Value*>& args)
{
    llvm::StructType* structType = llvm::StructType::getTypeByName(*Context, structName);
    assert(structType && "Struct type should exist");

    // Get current function and entry block
    llvm::Function* currentFunc = Builder->GetInsertBlock()->getParent();
    llvm::BasicBlock* entryBlock = &currentFunc->getEntryBlock();

    // Create a temporary builder for the entry block if needed
    llvm::IRBuilder<> entryBuilder(entryBlock);
    if (!entryBlock->empty() && entryBlock->getTerminator()) {
        // If entry block already has a terminator, insert before it
        entryBuilder.SetInsertPoint(entryBlock->getTerminator());
    } else {
        // Otherwise insert at end of entry block
        entryBuilder.SetInsertPoint(entryBlock);
    }

    // Create allocation
    llvm::AllocaInst* instance = entryBuilder.CreateAlloca(structType, nullptr, varName);

    // Initialize fields using current builder (not entry builder)
    for (size_t i = 0; i < args.size() && i < structType->getNumElements(); ++i) {
        // Check if current builder is pointing at a terminator
        if (Builder->GetInsertBlock()->getTerminator()) {
            // If so, move insertion point before terminator
            Builder->SetInsertPoint(Builder->GetInsertBlock()->getTerminator());
        }

        llvm::Value* fieldPtr = Builder->CreateStructGEP(
            structType, instance, i, varName + "_field" + std::to_string(i));
        Builder->CreateStore(args[i], fieldPtr);
    }
    // Assuming you have a way to add variables to the scope
    // activeScope->set(varName, instance);
    return instance;
}

// llvm::Value* IRGenerator::getMember(llvm::Value* object, const std::string& memberName) {
//     if (!object) {
//         console.error("Object cannot be null");
//     }

//     llvm::Type* objType = object->getType();
    
//     // Handle pointer types (structs and class instances are usually pointers)
//     if (objType->isPointerTy()) {
//         objType = objType->getPointerElementType();
//     }

//     if (auto* structType = llvm::dyn_cast<llvm::StructType>(objType)) {
//         // 🔹 Lookup field index from struct metadata
//         auto it = structDefinitions.find(structType->getName().str());
//         if (it == structDefinitions.end()) {
//             console.error("Unknown struct: " + structType->getName().str());
//             return nullptr;
//         }

//         const auto& fieldMap = it->second; // Map of field names to indices
//         auto fieldIt = fieldMap.find(memberName);
//         if (fieldIt == fieldMap.end()) {
//             console.error("Struct '" + structType->getName().str() + "' has no member '" + memberName + "'");
//             return nullptr;
//         }

//         unsigned fieldIndex = fieldIt->second;

//         // 🔹 Get pointer to field
//         llvm::Value* fieldPtr = Builder->CreateStructGEP(structType, object, fieldIndex, memberName + "_ptr");

//         // 🔹 Load field value
//         return Builder->CreateLoad(structType->getElementType(fieldIndex), fieldPtr, memberName);
//     }
    
//     // 🔹 Handle class types (assuming they are mapped similarly to structs)
//     if (auto* classType = llvm::dyn_cast<llvm::StructType>(objType)) {
//         auto it = classDefinitions.find(classType->getName().str());
//         if (it == classDefinitions.end()) {
//             console.error("Unknown class: " + classType->getName().str());
//             return nullptr;
//         }

//         const auto& methodMap = it->second;
//         auto fieldIt = methodMap.find(memberName);
//         if (fieldIt == methodMap.end()) {
//             console.error("Class '" + classType->getName().str() + "' has no member '" + memberName + "'");
//             return nullptr;
//         }

//         unsigned fieldIndex = fieldIt->second;
//         llvm::Value* fieldPtr = Builder->CreateStructGEP(classType, object, fieldIndex, memberName + "_ptr");
//         return Builder->CreateLoad(classType->getElementType(fieldIndex), fieldPtr, memberName);
//     }

//     console.error("Cannot access member '" + memberName + "' of non-struct/class type.");
//     return nullptr;
// }

// Getter: Load a member variable from an object
llvm::Value* IRGenerator::loadMemberValue(const std::string& objectName, const std::string& memberName) { 
    // Retrieve the object pointer
    llvm::Value* objectPtr = getVariable(objectName);
    if (!objectPtr) return nullptr; // Variable not found

    // If this is an enum lookup table, use `getEnumValue` instead
    if (objectName.ends_with("_lookup")) {
        std::string enumName = objectName.substr(0, objectName.find("_lookup"));
        return getEnumValue(enumName, memberName);
    }

    // Ensure it's a pointer
    llvm::PointerType* pointerType = llvm::dyn_cast<llvm::PointerType>(objectPtr->getType());
    if (!pointerType) return nullptr; // Not a pointer

    // Get the struct/class type
    llvm::Type* elementType = pointerType->getContainedType(0);
    llvm::StructType* structType = llvm::dyn_cast<llvm::StructType>(elementType);
    if (!structType) return nullptr; // Not a struct/class

    // Get the index of the member
    int memberIndex = getStructMemberIndex(structType, memberName);
    if (memberIndex == -1) return nullptr; // Member not found

    // Generate GEP
    llvm::Value* memberPtr = Builder->CreateStructGEP(structType, objectPtr, memberIndex);

    // Load the value
    llvm::Type* loadedType = memberPtr->getType()->getContainedType(0);
    return Builder->CreateLoad(loadedType, memberPtr);
}


llvm::Value* IRGenerator::loadMemberFromStruct(llvm::Value* structPtr, llvm::StructType* structType, const std::string& memberName) {
    // Get the index of the member
    int memberIndex = getStructMemberIndex(structType, memberName);
    if (memberIndex == -1) return nullptr;

    // Generate a GEP instruction
    llvm::Value* memberPtr = Builder->CreateStructGEP(structType, structPtr, memberIndex);

    // Load the member value
    llvm::Type* loadedType = memberPtr->getType()->getContainedType(0);  // Get the type of the element being pointed to
    return Builder->CreateLoad(loadedType, memberPtr);  // Create the load with the correct type
}

llvm::Value* IRGenerator::loadMemberFromClass(llvm::Value* classPtr, llvm::StructType* classType, const std::string& memberName) {
    // Get the index of the member in the class
    int memberIndex = getStructMemberIndex(classType, memberName);
    if (memberIndex == -1) return nullptr;

    // Generate GEP instruction to access member
    llvm::Value* memberPtr = Builder->CreateStructGEP(classType, classPtr, memberIndex);

    // Load the member value
    llvm::Type* loadedType = memberPtr->getType()->getContainedType(0);  // Get the type of the element being pointed to
    return Builder->CreateLoad(loadedType, memberPtr);  // Create the load with the correct type
}


int IRGenerator::getStructMemberIndex(llvm::StructType* structType, const std::string& memberName) {
    const llvm::StructLayout* layout = Module->getDataLayout().getStructLayout(structType);
    auto structMembers = structType->elements();

    // Check if the member name matches any known struct/class member
    for (size_t i = 0; i < structMembers.size(); i++) {
        if (memberName == typeToString(structMembers[i])) {
            return i;
        }
    }

    return -1; // Not found
}


// Setter: Store a new value in a member variable
void IRGenerator::setMemberValue(
    llvm::Value* object, 
    const std::string& memberName, 
    llvm::Value* newValue
) {
    return;
}

void IRGenerator::createEnum(
    const std::string& enumName,
    const std::vector<std::string>& valueNames,
    const std::vector<int>& valueIndices)
{
    assert(valueNames.size() == valueIndices.size() && "Enum names and values must match");

    llvm::Type* intType = llvm::Type::getInt32Ty(*Context);

    for (size_t i = 0; i < valueNames.size(); ++i) {
        llvm::Constant* intValue = llvm::ConstantInt::get(intType, valueIndices[i]);

        // Store the enum value in the symbol table
        // activeScope->set(enumName + "::" + valueNames[i], intValue);
    }
}


void IRGenerator::createEnumWithLookup(
    const std::string& enumName,
    const std::vector<std::string>& valueNames,
    const std::vector<int>& valueIndices)
{
    assert(valueNames.size() == valueIndices.size() && "Enum names and values must match");

    llvm::Type* intType = llvm::Type::getInt32Ty(*Context);
    llvm::Type* charPtrType = llvm::PointerType::get(llvm::Type::getInt8Ty(*Context), 0);

    // Create an array of {int, string} pairs
    std::vector<llvm::Constant*> enumEntries;
    for (size_t i = 0; i < valueNames.size(); ++i) {
        llvm::Constant* intValue = llvm::ConstantInt::get(intType, valueIndices[i]);
        llvm::Constant* strValue = Builder->CreateGlobalString(valueNames[i]);

        llvm::StructType* pairType = llvm::StructType::get(*Context, {intType, charPtrType});
        enumEntries.push_back(llvm::ConstantStruct::get(pairType, {intValue, strValue}));
    }

    llvm::ArrayType* arrayType = llvm::ArrayType::get(enumEntries[0]->getType(), enumEntries.size());
    llvm::Constant* enumArray = llvm::ConstantArray::get(arrayType, enumEntries);

    llvm::GlobalVariable* globalEnumTable = new llvm::GlobalVariable(
        *Module, arrayType, true, llvm::GlobalValue::ExternalLinkage, enumArray, enumName + "_lookup");

    // Store the lookup table in the symbol table
    // activeScope->set(enumName + "_table", globalEnumTable);

    // Create lookup function: const char* getEnumName(int value)
    llvm::FunctionType* lookupFuncType = llvm::FunctionType::get(charPtrType, {intType}, false);
    llvm::Function* lookupFunction = llvm::Function::Create(
        lookupFuncType, 
        llvm::Function::ExternalLinkage, 
        "get" + enumName + "Name", 
        *Module  // Dereference the unique_ptr to get a reference
    );
    
    // Create function body
    llvm::BasicBlock* entry = llvm::BasicBlock::Create(*Context, "entry", lookupFunction);
    Builder->SetInsertPoint(entry);

    llvm::Argument* valueArg = lookupFunction->getArg(0);
    llvm::Value* tablePtr = Builder->CreateBitCast(globalEnumTable, llvm::PointerType::get(arrayType, 0));

    llvm::BasicBlock* loopBody = llvm::BasicBlock::Create(*Context, "loop", lookupFunction);
    llvm::BasicBlock* notFound = llvm::BasicBlock::Create(*Context, "not_found", lookupFunction);
    llvm::BasicBlock* exitBlock = llvm::BasicBlock::Create(*Context, "exit", lookupFunction);

    llvm::PHINode* result = Builder->CreatePHI(charPtrType, 2, "result");

    // Loop over the table
    Builder->CreateBr(loopBody);
    Builder->SetInsertPoint(loopBody);
    
    llvm::PHINode* index = Builder->CreatePHI(intType, 2, "index");
    index->addIncoming(llvm::ConstantInt::get(intType, 0), entry);

    // Get the struct type from the table entry
    llvm::Type* structType = enumEntries[0]->getType();  // Use the first enum entry to get the struct type
    llvm::Type* fieldType = structType->getStructElementType(0);  // Get the type of the first field of the struct

    // Now create the GEP (Get Element Pointer) for the first element (int)
    llvm::Value* entryPtr = Builder->CreateGEP(structType, tablePtr, {index});  // GEP to access the current enum entry
    llvm::Value* entryInt = Builder->CreateStructGEP(structType, entryPtr, 0);  // Access the int field
    llvm::Value* entryStr = Builder->CreateStructGEP(structType, entryPtr, 1);  // Access the string field

    llvm::Value* cmp = Builder->CreateICmpEQ(entryInt, valueArg);
    Builder->CreateCondBr(cmp, exitBlock, notFound);

    Builder->SetInsertPoint(notFound);
    llvm::Value* nextIndex = Builder->CreateAdd(index, llvm::ConstantInt::get(intType, 1));
    llvm::Value* endCond = Builder->CreateICmpEQ(nextIndex, llvm::ConstantInt::get(intType, valueNames.size()));
    Builder->CreateCondBr(endCond, exitBlock, loopBody);

    index->addIncoming(nextIndex, notFound);
    result->addIncoming(entryStr, loopBody);
    result->addIncoming(Builder->CreateGlobalString("UNKNOWN"), notFound);

    Builder->SetInsertPoint(exitBlock);
    Builder->CreateRet(result);
}


llvm::Value* IRGenerator::getEnumValue(const std::string& enumName, const std::string& memberName) {
    // Construct lookup variable name
    // std::string lookupTableName = enumName + "_lookup";

    // // Retrieve the enum lookup table from the symbol table
    // llvm::GlobalVariable* lookupTable = dynamic_cast<llvm::GlobalVariable*>(activeScope->get(lookupTableName));
    // if (!lookupTable) {
    //     console.error("Enum '" + enumName + "' not found.");
    //     return nullptr;
    // }

    // // Get the array type and its length
    // llvm::ArrayType* arrayType = llvm::dyn_cast<llvm::ArrayType>(lookupTable->getValueType());
    // if (!arrayType) {
    //     console.error("Invalid enum lookup table format for '" + enumName + "'.");
    //     return nullptr;
    // }

    // size_t numEntries = arrayType->getNumElements();
    // llvm::Type* structType = arrayType->getElementType(); // Struct { i32, i8* }

    // // Iterate through the lookup table to find the matching member
    // for (size_t i = 0; i < numEntries; ++i) {
    //     // Get pointer to entry i
    //     llvm::Value* entryPtr = Builder->CreateConstGEP2_32(arrayType, lookupTable, 0, i);

    //     // Extract the member name (char*)
    //     llvm::Value* namePtr = Builder->CreateStructGEP(structType, entryPtr, 1);
    //     llvm::Value* nameValue = Builder->CreateLoad(namePtr->getType()->getPointerElementType(), namePtr);

    //     // Compare with the requested memberName
    //     llvm::Value* cmp = Builder->CreateCall(getStrcmpFunction(), {nameValue, Builder->CreateGlobalStringPtr(memberName)});
    //     llvm::Value* isMatch = Builder->CreateICmpEQ(cmp, llvm::ConstantInt::get(llvm::Type::getInt32Ty(*Context), 0));

    //     // If matched, return the integer value
    //     llvm::Value* intPtr = Builder->CreateStructGEP(structType, entryPtr, 0);
    //     llvm::Value* intValue = Builder->CreateLoad(intPtr->getType()->getPointerElementType(), intPtr);

    //     llvm::BasicBlock* returnBlock = llvm::BasicBlock::Create(*Context, "return_enum", Builder->GetInsertBlock()->getParent());
    //     llvm::BasicBlock* continueBlock = llvm::BasicBlock::Create(*Context, "continue_enum", Builder->GetInsertBlock()->getParent());

    //     Builder->CreateCondBr(isMatch, returnBlock, continueBlock);

    //     // Set insert point for return
    //     Builder->SetInsertPoint(returnBlock);
    //     Builder->CreateRet(intValue);

    //     // Set insert point for continue
    //     Builder->SetInsertPoint(continueBlock);
    // }

    // // If no match found, return an error value (-1)
    // return Builder->CreateRet(llvm::ConstantInt::get(llvm::Type::getInt32Ty(*Context), -1));
    return nullptr;
}
