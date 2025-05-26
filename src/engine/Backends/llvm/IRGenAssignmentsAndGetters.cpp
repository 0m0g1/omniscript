#include <omniscript/engine/Backends/LLVM/IRGenerator.h>

llvm::Value* IRGenerator::assignVariable(
    const std::string& name,
    llvm::Type* type,
    llvm::Value* initialValue,
    bool isGlobal,
    bool isConstant,
    llvm::BasicBlock* activeBlock,
    llvm::GlobalValue::LinkageTypes linkage
) {
    llvm::Module* activeModule = CurrentModule;
    DEBUG_LOG("Creating variable: " + name + (isGlobal ? " (global)" : " (local)") + (isConstant ? " [const]" : ""));

    if (activeScope->exists(name)) {
        llvm::Value* existingVar = activeScope->get(name);
        DEBUG_LOG("Variable '" + name + "' already exists. Reassigning value...");

        if (initialValue && !isConstant) {
            llvm::Type* valueType = type;
            if (initialValue->getType() != valueType) {
                initialValue = Builder->CreateBitCast(initialValue, valueType, "bitcast");
            }

            llvm::StoreInst* store = Builder->CreateStore(initialValue, existingVar);
            store->setAlignment(llvm::Align(4)); // Adjust based on type if needed
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

        // If not constant and initializer isn't a constant, emit store in init func
        if (initialValue && !constInit && !isConstant) {
            console.warn("Global variable '" + name + "' initialized with non-constant value; adding runtime store.");

            llvm::Function* initFunc = getOrCreateGlobalInitFunction();
            llvm::IRBuilder<> tempBuilder(&initFunc->getEntryBlock(), initFunc->getEntryBlock().end());
            tempBuilder.CreateStore(initialValue, gVar);
        }

        return gVar;
    }

    // --- Local variable ---
    llvm::Function* function = (activeBlock ? activeBlock->getParent() : Builder->GetInsertBlock()->getParent());
    llvm::IRBuilder<> tempBuilder(Builder->getContext());
    tempBuilder.SetInsertPoint(activeBlock ? activeBlock : &function->getEntryBlock(), (activeBlock ? activeBlock->begin() : function->getEntryBlock().begin()));

    llvm::AllocaInst* alloca = tempBuilder.CreateAlloca(type, nullptr, name);

    // Apply alignment (based on type as before)
    unsigned align = 4;
    if (type->isIntegerTy()) {
        unsigned bits = type->getIntegerBitWidth();
        align = (bits >= 64) ? 8 : (bits >= 32) ? 4 : 2;
    } else if (type->isFloatingPointTy()) {
        align = type->isDoubleTy() ? 8 : 4;
    }
    alloca->setAlignment(llvm::Align(align));

    // Store initializer if present
    if (initialValue) {
        llvm::StoreInst* store = Builder->CreateStore(initialValue, alloca);
        store->setAlignment(llvm::Align(align));

        // Move before return if one exists
        llvm::BasicBlock* entryBlock = &function->getEntryBlock();
        for (llvm::Instruction& I : *entryBlock) {
            if (llvm::isa<llvm::ReturnInst>(&I)) {
                store->moveBefore(I.getIterator());
                break;
            }
        }
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


llvm::Value* IRGenerator::getVariable(const std::string& name) {
    llvm::Value* val = activeScope->get(name);

    if (llvm::AllocaInst* alloca = llvm::dyn_cast<llvm::AllocaInst>(val)) {
        return Builder->CreateLoad(alloca->getAllocatedType(), alloca, name + ".val");
    } else if (llvm::GlobalVariable* gvar = llvm::dyn_cast<llvm::GlobalVariable>(val)) {
        return Builder->CreateLoad(gvar->getValueType(), gvar, name + ".val");
    }

    return val;
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