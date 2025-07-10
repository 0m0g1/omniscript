#include <omniscript/Backends/LLVM/IRGenerator.h>

llvm::Value* IRGenerator::assignVariable(
    std::shared_ptr<Omniscript::VariableAssignment> statement,
    SymbolTableType scope
) {
    //Todo:: To debug string method for all expressions
    // DEBUG_LOG(statement->toDebugString());
    std::string name = statement->variableName;
    llvm::Type* type = resolveLLVMType(statement->getType());
    DEBUG_LOG("Variable '" + name + "' has type '" + debugType(type) + "'.");

    bool isGlobal = statement->isGlobal;
    bool isConstant = statement->isConstant;
    bool isVolatile = statement->isVolatile;
    llvm::GlobalValue::LinkageTypes linkage = llvm::GlobalValue::InternalLinkage;
    llvm::Module* activeModule = currentModule;

    DEBUG_LOG("Creating variable: " + name + (isGlobal ? " (global)" : " (local)") + (isConstant ? " [const]" : ""));

    // --- If variable exists, reassign value ---
    if (activeScope->exists(name)) {
        llvm::Value* existingVar = activeScope->get(name);
        DEBUG_LOG("Variable '" + name + "' already exists. Reassigning value...");

        if (statement->getValue() && !isConstant) {
            llvm::Value* newValue = codegen(statement->getValue(), scope);
            if (newValue->getType() != type && !llvm::isa<llvm::AllocaInst>(newValue)) {
                newValue = generateCast(newValue, type);
                if (!newValue) {
                    console.error("Failed to cast value when reassigning '" + name + "'");
                    return nullptr;
                }
            }
            llvm::StoreInst* store = Builder->CreateStore(newValue, existingVar, isVolatile);
            store->setAlignment(llvm::Align(4));
        }

        return existingVar;
    }

    // --- GLOBAL VARIABLE HANDLING ---
    if (isGlobal) {
        llvm::Constant* initVal = llvm::Constant::getNullValue(type);

        llvm::GlobalVariable* gVar = new llvm::GlobalVariable(
            *activeModule,
            type,
            isConstant,
            linkage,
            initVal,
            name
        );

        // If value is specified and not a constant, add runtime initializer
        if (statement->getValue()) {
            if (type->isArrayTy()) {
                if (auto arrayLit = std::dynamic_pointer_cast<Omniscript::FixedArrayExpression>(statement->getValue())) {
                    llvm::ArrayType* arrayType = llvm::cast<llvm::ArrayType>(type);
                    std::vector<llvm::Constant*> elements;

                    for (const auto& elem : arrayLit->elements) {
                        auto* val = llvm::dyn_cast<llvm::Constant>(codegen(elem, scope));
                        if (!val) {
                            console.error("Global array '" + name + "' has non-constant initializer element.");
                            return nullptr;
                        }
                        elements.push_back(val);
                    }

                    llvm::Constant* arrayInit = llvm::ConstantArray::get(arrayType, elements);
                    gVar->setInitializer(arrayInit);
                    activeScope->set(name, gVar);
                    return gVar;
                }
            } else {
                auto val = codegen(statement->getValue(), scope);
                if (auto* constVal = llvm::dyn_cast<llvm::Constant>(val)) {
                    // Constant value — compile-time initializer
                    gVar->setInitializer(constVal);
                } else {
                    // Runtime value — must insert store into main or init block
                    llvm::Function* initFunc = getOrCreateGlobalInitFunction();
                    llvm::IRBuilder<>::InsertPoint savedIP = Builder->saveIP();
        
                    llvm::BasicBlock& entryBlock = initFunc->getEntryBlock();
                    Builder->SetInsertPoint(&entryBlock, entryBlock.begin());
                    Builder->CreateStore(val, gVar);
        
                    Builder->restoreIP(savedIP);
                }
            }
        }

        activeScope->set(name, gVar);

        return gVar;
    }

    // --- LOCAL VARIABLE HANDLING ---
    llvm::Function* function = Builder->GetInsertBlock()->getParent();
    llvm::BasicBlock* entryBlock = &function->getEntryBlock();

    llvm::IRBuilder<>::InsertPoint savedIP = Builder->saveIP();
    Builder->SetInsertPoint(entryBlock, entryBlock->begin());
    llvm::AllocaInst* alloca = Builder->CreateAlloca(type, nullptr, name);
    Builder->restoreIP(savedIP);

    // Set alignment
    unsigned align = type->isDoubleTy() ? 8 : 4;
    if (type->isIntegerTy()) {
        align = (type->getIntegerBitWidth() >= 64) ? 8 : 4;
    }
    alloca->setAlignment(llvm::Align(align));

    // --- Handle initialization ---
    if (statement->getValue()) {
        if (type->isArrayTy()) {
            if (auto arrayLit = std::dynamic_pointer_cast<Omniscript::FixedArrayExpression>(statement->getValue())) {
                llvm::ArrayType* arrayType = llvm::cast<llvm::ArrayType>(type);

                std::vector<llvm::Value*> elements;
                for (const auto& elem : arrayLit->elements) {
                    elements.push_back(this->codegen(elem, scope));
                }

                createFixedArrayInPlace(alloca, arrayType, elements);  // ✅ write directly into local
            } else {
                llvm::Value* rhs = codegen(statement->getValue(), scope);
                Builder->CreateStore(rhs, alloca, isVolatile);
            }
        } else {
            llvm::Value* initValue = codegen(statement->getValue(), scope);

            if (initValue->getType() != type && !llvm::isa<llvm::AllocaInst>(initValue)) {
                initValue = generateCast(initValue, type);
                if (!initValue) {
                    console.error("Failed to cast initializer for variable '" + name + "'");
                    return nullptr;
                }
            }
            
            // if (auto* globalString = llvm::dyn_cast<llvm::GlobalVariable>(initValue)) {
            //     globalString->setName(name);
            // }
            // else
            if (initValue->getType() != alloca->getAllocatedType()) {
                // Normal type mismatch handling
                initValue = generateCast(initValue, alloca->getAllocatedType());
            }
            
            llvm::StoreInst* store = Builder->CreateStore(initValue, alloca, isVolatile);
            store->setAlignment(llvm::Align(align));
        }
    } else {
        llvm::Value* zeroInit = llvm::Constant::getNullValue(type);
        Builder->CreateStore(zeroInit, alloca);
    }

    activeScope->set(name, alloca);
    DEBUG_LOG("Local variable '" + name + "' allocated and initialized");
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