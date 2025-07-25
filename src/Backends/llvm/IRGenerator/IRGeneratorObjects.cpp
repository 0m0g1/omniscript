#include <omniscript/Backends/llvm/IRGenerator.h>

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
    bool isGlobal) {
    
    llvm::StructType* structType = llvm::StructType::getTypeByName(*Context, structName);
    if (!structType) {
        DEBUG_LOG("Struct type '" + structName + "' does not exist.");
        return nullptr;
    }

    size_t fieldCount = structType->getNumElements();
    if (args.size() != fieldCount) {
        console.error("Mismatch: struct '" + structName + "' expects " +
                      std::to_string(fieldCount) + " fields, but got " +
                      std::to_string(args.size()) + " arguments.");
        return nullptr;
    }

    llvm::Module* module = Builder->GetInsertBlock()->getModule();

    // === Global Instance ===
    if (isGlobal) {
        std::vector<llvm::Constant*> initVals;
        
        for (size_t i = 0; i < args.size(); ++i) {
            llvm::Value* arg = args[i];
            llvm::Type* expectedType = structType->getElementType(i);
            
            // Handle the case where arg might be a pointer to a struct that needs to be loaded
            if (auto* ptrType = llvm::dyn_cast<llvm::PointerType>(arg->getType())) {
                if (auto* globalVar = llvm::dyn_cast<llvm::GlobalVariable>(arg)) {
                    // If it's a global variable, use its initializer directly
                    if (globalVar->hasInitializer()) {
                        auto* initializer = globalVar->getInitializer();
                        if (initializer->getType() == expectedType) {
                            initVals.push_back(initializer);
                            continue;
                        }
                    }
                }
            }
            
            // Try to cast to constant directly
            auto* c = llvm::dyn_cast<llvm::Constant>(arg);
            if (!c) {
                console.error("Non-constant used in global initializer for field " + std::to_string(i));
                return nullptr;
            }
            
            // Verify type compatibility
            if (c->getType() != expectedType) {
                console.error("Type mismatch for field " + std::to_string(i) + 
                             " in struct '" + structName + "'");
                return nullptr;
            }
            
            initVals.push_back(c);
        }

        llvm::Constant* initializer = llvm::ConstantStruct::get(structType, initVals);

        llvm::GlobalVariable* globalVar = new llvm::GlobalVariable(
            *module,
            structType,
            false,
            llvm::GlobalValue::ExternalLinkage,
            initializer,
            varName
        );

        activeScope->set(varName, globalVar);
        return globalVar;
    }

    // === Local Instance ===
    llvm::Function* func = Builder->GetInsertBlock()->getParent();
    llvm::BasicBlock& entryBlock = func->getEntryBlock();

    llvm::IRBuilder<> entryBuilder(&entryBlock, entryBlock.begin());
    llvm::AllocaInst* localVar = entryBuilder.CreateAlloca(structType, nullptr, varName);

    for (size_t i = 0; i < fieldCount; ++i) {
        llvm::Value* fieldPtr = Builder->CreateStructGEP(structType, localVar, i, 
                                                         varName + "_field" + std::to_string(i));
        
        llvm::Value* valueToStore = args[i];
        llvm::Type* expectedType = structType->getElementType(i);
        
        // If the argument is a pointer and we expect a value type, load it first
        if (valueToStore->getType()->isPointerTy() && !expectedType->isPointerTy()) {
            // In modern LLVM with opaque pointers, we need to specify the type to load
            valueToStore = Builder->CreateLoad(expectedType, valueToStore, 
                                              varName + "_load_field" + std::to_string(i));
        }
        
        Builder->CreateStore(valueToStore, fieldPtr);
    }

    DEBUG_LOG("Created local struct instance: " + varName);
    activeScope->set(varName, localVar);
    return localVar;
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
    // Todo:: add a bitcast omniscript expression
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
        if (srcType->getPointerAddressSpace() != destType->getPointerAddressSpace()) {
            console.error("Pointer cast across address spaces is invalid.");
            return nullptr;
        }
        return Builder->CreateBitCast(src, destType, "ptrcast");
    }

    // ✅ NEW: Cast from integer to pointer (e.g., 12 as void*)
    if (srcType->isIntegerTy() && destType->isPointerTy()) {
        return Builder->CreateIntToPtr(src, destType, "inttoptr");
    }

    // ✅ NEW: Cast from pointer to integer (optional reverse)
    if (srcType->isPointerTy() && destType->isIntegerTy()) {
        return Builder->CreatePtrToInt(src, destType, "ptrtoint");
    }

    // Cast from void* (i8*) to function pointer type
    if (srcType->isPointerTy() && destType->isFunctionTy()) {
        llvm::PointerType* funcPtrType = llvm::PointerType::getUnqual(destType);
        return Builder->CreateBitCast(src, funcPtrType, "void_to_func_ptrcast");
    }

    // Cast from function pointer to void*
    if (srcType->isFunctionTy() && destType->isPointerTy()) {
        llvm::PointerType* srcPtrType = llvm::PointerType::getUnqual(srcType);
        return Builder->CreateBitCast(src, destType, "func_to_void_ptrcast");
    }

    console.error("Unsupported cast from '" + debugType(srcType) + "' to '" + debugType(destType) + "'");
    return nullptr;
}
