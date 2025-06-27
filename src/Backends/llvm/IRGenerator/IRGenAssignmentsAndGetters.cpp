#include <omniscript/Backends/LLVM/IRGenerator.h>

llvm::Value* IRGenerator::assignVariable(
    std::shared_ptr<Omniscript::VariableAssignment> statement,
    SymbolTableType scope
) {
    DEBUG_LOG(statement->toDebugString());
    std::string name = statement->variableName;
    llvm::Type* type = resolveLLVMType(statement->getType());
    DEBUG_LOG("Variable '" + name + "' has type '" + debugType(type) + "'.");
    llvm::Value* value = codegen(statement->getValue(), scope);
    DEBUG_LOG("Got variable '" + statement->variableName + "''s value.");
    llvm::Value* initialValue = value;
    bool isGlobal = statement->isGlobal;
    bool isConstant = statement->isConstant;
    llvm::GlobalValue::LinkageTypes linkage = llvm::GlobalValue::InternalLinkage;
    llvm::Module* activeModule = currentModule;
    bool isVolatile = statement->isVolatile;

    DEBUG_LOG("Creating variable: " + name + (isGlobal ? " (global)" : " (local)") + (isConstant ? " [const]" : ""));

    if (activeScope->exists(name)) {
        llvm::Value* existingVar = activeScope->get(name);
        DEBUG_LOG("Variable '" + name + "' already exists. Reassigning value...");

        if (initialValue && !isConstant) {
            llvm::Type* valueType = type;
            if (initialValue->getType() != valueType) {
                initialValue = generateCast(initialValue, valueType);
                if (!initialValue) {
                    console.error("Failed to cast value when reassigning '" + name + "'");
                    return nullptr;
                }
            }
            llvm::StoreInst* store = Builder->CreateStore(initialValue, existingVar, isVolatile);
            store->setAlignment(llvm::Align(4));
        }
        
        return existingVar;
    }

    // --- Global variable ---
    if (isGlobal) {
        llvm::Constant* constInit = llvm::dyn_cast<llvm::Constant>(initialValue);
        llvm::GlobalVariable* gVar = new llvm::GlobalVariable(
            *activeModule,
            type,
            isConstant,
            linkage,
            constInit ? constInit : llvm::Constant::getNullValue(type),
            name
        );

        activeScope->set(name, gVar);
        DEBUG_LOG("Global variable '" + name + "' created with type: " + debugType(type));

        if (initialValue && !constInit && !isConstant) {
            DEBUG_LOG("Global variable '" + name + "' initialized with non-constant value; adding runtime store.");
            Builder->CreateStore(initialValue, gVar, isVolatile);
        }

        return gVar;
    }

    // --- Local variable ---
    llvm::Function* function = (Builder->GetInsertBlock()->getParent());
    llvm::BasicBlock* entryBlock = &function->getEntryBlock();

    // Save current insertion point
    llvm::IRBuilder<>::InsertPoint savedIP = Builder->saveIP();

    // Move builder to the beginning of the entry block
    Builder->SetInsertPoint(entryBlock, entryBlock->begin());

    // Create the alloca
    llvm::AllocaInst* alloca = Builder->CreateAlloca(type, nullptr, name);

    // Restore builder to original insertion point
    Builder->restoreIP(savedIP);

    // Set alignment
    unsigned align = 4;
    if (type->isIntegerTy()) {
        unsigned bits = type->getIntegerBitWidth();
        align = (bits >= 64) ? 8 : (bits >= 32) ? 4 : 2;
    } else if (type->isFloatingPointTy()) {
        align = type->isDoubleTy() ? 8 : 4;
    }
    alloca->setAlignment(llvm::Align(align));

    // Store initializer
    if (initialValue) {
        if (initialValue->getType() != type) {
            initialValue = generateCast(initialValue, type);
            if (!initialValue) {
                console.error("Failed to cast initializer for variable '" + name + "'");
                return nullptr;
            }
        }
        llvm::StoreInst* store = Builder->CreateStore(initialValue, alloca, isVolatile);
        store->setAlignment(llvm::Align(align));
    }

    // Register in scope
    if (isConstant) {
        activeScope->setConstant(name, alloca);
    } else {
        activeScope->set(name, alloca);
    }

    DEBUG_LOG("Local variable '" + name + "' allocated and initialized" + (isConstant ? " (const)" : ""));
    return alloca;
}

llvm::Value* IRGenerator::getVariable(const std::string& name, bool extractValue) {
    llvm::Value* val = activeScope->get(name);

    llvm::Value* loaded;
    if (llvm::AllocaInst* alloca = llvm::dyn_cast<llvm::AllocaInst>(val)) {
        loaded = Builder->CreateLoad(alloca->getAllocatedType(), alloca, name + ".val");
    } else if (llvm::GlobalVariable* gvar = llvm::dyn_cast<llvm::GlobalVariable>(val)) {
        loaded = Builder->CreateLoad(gvar->getValueType(), gvar, name + ".val");
    } else {
        loaded = val;
    }

    if (extractValue && isNullableStruct(loaded->getType())) {
        return Builder->CreateExtractValue(loaded, 1);
    }

    return loaded;
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

llvm::Value* IRGenerator::getAddressOf(const std::string& varname) {
    return activeScope->get(varname);
}

llvm::Value* IRGenerator::getReferenceToVariable(const std::string& varname) {
    if (activeScope->exists(varname)) {
        return activeScope->get(varname);
    }
    
    // // Return the pointer/alloca directly
    console.error("Cannot get reference to: " + varname);
    return nullptr;
}