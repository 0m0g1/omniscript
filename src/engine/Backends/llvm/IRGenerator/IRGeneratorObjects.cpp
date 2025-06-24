#include <omniscript/engine/Backends/llvm/IRGenerator.h>

llvm::Value* IRGenerator::createObjectInstance(
    const std::string& typeName,
    const std::string& varName,
    const std::vector<llvm::Value*>& args,
    bool isGlobal
)
{
    DEBUG_LOG("Creating instance of type: " + typeName);
    
    if (llvm::StructType* structType = llvm::StructType::getTypeByName(*Context, typeName)) {
        DEBUG_LOG("Found struct type: " + typeName);
        return createStructInstance(typeName, varName, args);
    }

    console.error("Unknown type: " + typeName);
    return nullptr;
}

void IRGenerator::createStructType(const std::string& name, const std::vector<llvm::Type*>& fieldTypes) {
    if (llvm::StructType::getTypeByName(*Context, name)) {
        DEBUG_LOG("Struct " + name + " already exists. Skipping creation.");
        return;
    }

    llvm::StructType* structType = llvm::StructType::create(*Context, fieldTypes, name);
    if (!structType) {
        console.error("Failed to create struct type: " + name);
        return;
    }
    DEBUG_LOG("Created '" + debugType(structType) + "' struct type");
    activeScope->addType(name, structType);

    DEBUG_LOG("Created struct prototype: " + name);
}

llvm::Value* IRGenerator::createStructInstance(
    const std::string& structName,
    const std::string& varName,
    const std::vector<llvm::Value*>& args,
    bool isGlobal)
{
    llvm::StructType* structType = llvm::StructType::getTypeByName(*Context, structName);
    if (!structType) {
        DEBUG_LOG("Struct type '" + structName + "' does not exist.");
        return nullptr;
    }

    // Ensure the number of arguments matches the number of fields in the struct
    if (args.size() != structType->getNumElements()) {
        console.error("Mismatch between number of fields '" + std::to_string(args.size()) + "' and constructor arguments '" + std::to_string(structType->getNumElements()) + "' for struct: " + structName);
        return nullptr;
    }

    // For initializing fields
    std::vector<llvm::Constant*> constants;
    for (size_t i = 0; i < args.size(); ++i) {
        if (auto* constVal = llvm::dyn_cast<llvm::Constant>(args[i])) {
            constants.push_back(constVal);
        } else {
            // If not constant, still allow the argument to be used in the struct
            constants.push_back(llvm::Constant::getNullValue(args[i]->getType())); // Fallback null value
        }
    }

    llvm::Constant* initializer = llvm::ConstantStruct::get(structType, constants);

    if (isGlobal) {
        // Creating a global variable
        llvm::Module* module = Builder->GetInsertBlock()->getModule();  // Get module from the builder's block
        llvm::GlobalVariable* globalVar = new llvm::GlobalVariable(
            *module,
            structType,
            false,  // isConstant
            llvm::GlobalValue::ExternalLinkage,
            initializer,
            varName
        );
        
        DEBUG_LOG("Created global struct instance: " + varName);
        activeScope->set(varName, globalVar);  // Register in active scope
        return globalVar;

    } else {
        // Creating a local variable (on the stack)
        llvm::Function* currentFunc = Builder->GetInsertBlock()->getParent();
        llvm::BasicBlock* entryBlock = &currentFunc->getEntryBlock();

        llvm::IRBuilder<> entryBuilder(entryBlock);
        if (!entryBlock->empty() && entryBlock->getTerminator()) {
            entryBuilder.SetInsertPoint(entryBlock->getTerminator());
        } else {
            entryBuilder.SetInsertPoint(entryBlock);
        }

        // Create alloca for local variable
        llvm::AllocaInst* localVar = entryBuilder.CreateAlloca(structType, nullptr, varName);

        // Initialize the fields using the current builder
        for (size_t i = 0; i < args.size() && i < structType->getNumElements(); ++i) {
            if (Builder->GetInsertBlock()->getTerminator()) {
                Builder->SetInsertPoint(Builder->GetInsertBlock()->getTerminator());
            }

            llvm::Value* fieldPtr = Builder->CreateStructGEP(structType, localVar, i, varName + "_field" + std::to_string(i));
            Builder->CreateStore(args[i], fieldPtr);
        }

        DEBUG_LOG("Created local struct instance: " + varName);
        activeScope->set(varName, localVar);  // Register struct instance in scope
        return localVar;
    }
}

llvm::Value* IRGenerator::loadMemberValue(const std::string& objectName, const std::string& memberName) { 
    llvm::Value* objectPtr = getVariable(objectName);
    if (!objectPtr) return nullptr;

    if (objectName.ends_with("_lookup")) {
        std::string enumName = objectName.substr(0, objectName.find("_lookup"));
        return getEnumValue(enumName, memberName);
    }

    llvm::PointerType* pointerType = llvm::dyn_cast<llvm::PointerType>(objectPtr->getType());
    if (!pointerType) return nullptr;

    llvm::Type* elementType = pointerType->getContainedType(0);
    llvm::StructType* structType = llvm::dyn_cast<llvm::StructType>(elementType);
    if (!structType) return nullptr;

    int memberIndex = getStructMemberIndex(structType, memberName);
    if (memberIndex == -1) return nullptr;


    llvm::Value* memberPtr = Builder->CreateStructGEP(structType, objectPtr, memberIndex);

    llvm::Type* loadedType = memberPtr->getType()->getContainedType(0);
    return Builder->CreateLoad(loadedType, memberPtr);
}

llvm::Value* IRGenerator::loadMemberFromStruct(llvm::Value* structPtr, llvm::StructType* structType, const std::string& memberName) {
    int memberIndex = getStructMemberIndex(structType, memberName);
    if (memberIndex == -1) return nullptr;

    llvm::Value* memberPtr = Builder->CreateStructGEP(structType, structPtr, memberIndex);

    llvm::Type* loadedType = memberPtr->getType()->getContainedType(0); 
    return Builder->CreateLoad(loadedType, memberPtr);
}

llvm::Value* IRGenerator::loadMemberFromClass(llvm::Value* classPtr, llvm::StructType* classType, const std::string& memberName) {
    int memberIndex = getStructMemberIndex(classType, memberName);
    if (memberIndex == -1) return nullptr;

    llvm::Value* memberPtr = Builder->CreateStructGEP(classType, classPtr, memberIndex);

    llvm::Type* loadedType = memberPtr->getType()->getContainedType(0); 
    return Builder->CreateLoad(loadedType, memberPtr);
}

int IRGenerator::getStructMemberIndex(llvm::StructType* structType, const std::string& memberName) {
    const llvm::StructLayout* layout = Module->getDataLayout().getStructLayout(structType);
    auto structMembers = structType->elements();

    for (size_t i = 0; i < structMembers.size(); i++) {
        if (memberName == debugType(structMembers[i])) {
            return i;
        }
    }

    return -1;
}

void IRGenerator::setMemberValue(
    llvm::Value* object, 
    const std::string& memberName, 
    llvm::Value* newValue
) {
    return;
}

llvm::Value* IRGenerator::createModuleObject(
    const std::string& moduleName,
    const std::unordered_map<std::string, llvm::Value*>& members
) {
    llvm::LLVMContext& ctx = Builder->getContext();
    llvm::Module* mod = currentModule;

    DEBUG_LOG("Creating module object: " + moduleName);

    std::vector<llvm::Type*> memberTypes;
    std::vector<std::string> memberNames;

    for (const auto& [key, val] : members) {
        memberTypes.push_back(val->getType());
        memberNames.push_back(key);
    }

    if (llvm::StructType::getTypeByName(*Context, moduleName)) {
        console.error("Cannot create module " + moduleName + "as a symbol with the name '" + moduleName + "' already exists in the scope.");
        return nullptr;
    }
    
    createStructType(moduleName, memberTypes);

    llvm::Value* moduleInstance = createObjectInstance(
        moduleName,
        moduleName,
        std::vector<llvm::Value*> {},
        true
    );

    DEBUG_LOG("Module struct instance created: " + moduleName);

    return moduleInstance;
}

llvm::Value* IRGenerator::generateCast(llvm::Value* src, llvm::Type* destType) {
    llvm::Type* srcType = src->getType();

    if (srcType == destType) {
        DEBUG_LOG("No cast needed; source and destination types match.");
        return src;
    }

    if (srcType->isIntegerTy() && destType->isIntegerTy()) {
        unsigned srcBits = srcType->getIntegerBitWidth();
        unsigned destBits = destType->getIntegerBitWidth();
        if (destBits > srcBits) {
            return Builder->CreateZExt(src, destType, "zext");
        } else if (destBits < srcBits) {
            return Builder->CreateTrunc(src, destType, "trunc");
        } else {
            return src;
        }
    }

    if (srcType->isIntegerTy() && destType->isFloatingPointTy()) {
        return Builder->CreateSIToFP(src, destType, "sitofp");
    }

    if (srcType->isFloatingPointTy() && destType->isIntegerTy()) {
        return Builder->CreateFPToSI(src, destType, "fptosi");
    }

    if (srcType->isFloatingPointTy() && destType->isFloatingPointTy()) {
        unsigned srcBits = srcType->getPrimitiveSizeInBits();
        unsigned destBits = destType->getPrimitiveSizeInBits();
        if (destBits > srcBits) {
            return Builder->CreateFPExt(src, destType, "fpext");
        } else {
            return Builder->CreateFPTrunc(src, destType, "fptrunc");
        }
    }

    if (srcType->isPointerTy() && destType->isPointerTy()) {
        return Builder->CreateBitCast(src, destType, "ptrcast");
    }

    console.error("Unsupported cast from '" + debugType(srcType) + "' to '" + debugType(destType) + "'");
    return nullptr;
}
