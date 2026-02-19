#include <omniscript/Backends/LLVM/IRGenerator.h>
#include <omniscript/Expressions/LiteralExpressions.h>
#include <omniscript/Backends/LLVM/ExternalFunctionResolvers/WindowsAPILLVMResolver.h>

namespace Omniscript {
llvm::Value* IRGenerator::assignVariable(
    std::shared_ptr<VariableAssignment> statement,
    SymbolTableType scope
) {
    std::string name = statement->variableName;
    llvm::Type* type = resolveLLVMType(statement->getType());
    DEBUG_LOG("Variable '" + name + "' has type '" + debugType(type) + "'");

    bool isGlobal = statement->isGlobal;
    bool isConstant = statement->isConstant;
    bool isVolatile = statement->isVolatile;
    llvm::GlobalValue::LinkageTypes linkage = llvm::GlobalValue::InternalLinkage;
    llvm::Module* activeModule = currentModule;

    if (statement->getType()->isFunction()) {
        activeScope->addType("*" + name, resolveLLVMType(statement->getType()));
    }

    // --- Handle external variables ---
    if (statement->isExtern) {
        std::string externName = statement->externName.empty() ? name : statement->externName;
        std::string genericStatic = statement->genericStatic;
        std::string genericDynamic = statement->genericDynamic;
        
        // Resolve platform-specific library paths
        auto targetOS = configs.resolveTargetOS();
        switch (targetOS) {
            case TargetOS::Windows:
                if (!statement->windowsStatic.empty()) genericStatic = statement->windowsStatic;
                if (!statement->windowsDynamic.empty()) genericDynamic = statement->windowsDynamic;
                break;
            case TargetOS::Linux:
            case TargetOS::FreeBSD:
            case TargetOS::Android:
                if (!statement->linuxStatic.empty()) genericStatic = statement->linuxStatic;
                if (!statement->linuxShared.empty()) genericDynamic = statement->linuxShared;
                break;
            case TargetOS::MacOS:
            case TargetOS::iOS:
                if (!statement->macosStatic.empty()) genericStatic = statement->macosStatic;
                if (!statement->macosShared.empty()) genericDynamic = statement->macosShared;
                break;
            default: break;
        }

        // Check symbol existence in libraries
        bool staticExists = false;
        bool dynamicExists = false;
        
        if (!genericStatic.empty() && configs.mode != CompileMode::JIT) {
            if (WindowsAPIResolver::isWindowsSystemLibrary(genericStatic)) {
                staticExists = true; // Trust that Windows system libs have the symbol
            } else if (fileExists(genericStatic)) {
                staticExists = symbolExistsInStaticLib(genericStatic, externName);
                if (!staticExists) {
                    std::string suggestion = Console::formatString(
                        "To resolve this:\n"
                        "1. Verify symbol '%s' exists in static library '%s'\n"
                        "2. Check library documentation for symbol availability\n"
                        "3. Ensure correct library is linked",
                        externName.c_str(), genericStatic.c_str()
                    );
                    console.reportError(
                        Console::RUNTIME_ERROR,
                        Console::formatString("Symbol '%s' not found in static library: '%s'",
                                         externName.c_str(), genericStatic.c_str()),
                        suggestion,
                        statement->getSpan()
                    );
                }
            }
        }

        if (!genericDynamic.empty() && !staticExists) {
            if (fileExists(genericDynamic)) {
                dynamicExists = symbolExistsInDLL(genericDynamic, externName);
                if (!dynamicExists) {
                    std::string suggestion = Console::formatString(
                        "To resolve this:\n"
                        "1. Verify symbol '%s' exists in dynamic library '%s'\n"
                        "2. Check library documentation for symbol availability\n"
                        "3. Ensure correct library is linked",
                        externName.c_str(), genericDynamic.c_str()
                    );
                    console.reportError(
                        Console::RUNTIME_ERROR,
                        Console::formatString("Symbol '%s' not found in dynamic library: '%s'",
                                         externName.c_str(), genericDynamic.c_str()),
                        suggestion,
                        statement->getSpan()
                    );
                }
            }
            std::string error;
            auto dynLib = llvm::sys::DynamicLibrary::getPermanentLibrary(genericDynamic.c_str(), &error);
            if (!dynLib.isValid()) {
                std::string suggestion = Console::formatString(
                    "To resolve this:\n"
                    "1. Verify dynamic library '%s' exists and is accessible\n"
                    "2. Check file permissions\n"
                    "3. Ensure correct library path is specified",
                    genericDynamic.c_str()
                );
                console.reportError(
                    Console::RUNTIME_ERROR,
                    Console::formatString("Failed to load dynamic library '%s': %s",
                                     genericDynamic.c_str(), error.c_str()),
                    suggestion,
                    statement->getSpan()
                );
            }
        }

        if (!staticExists && !dynamicExists) {
            std::string suggestion = Console::formatString(
                "To resolve this:\n"
                "1. Verify external variable '%s' is defined in specified libraries\n"
                "2. Check static library '%s' or dynamic library '%s'\n"
                "3. Ensure correct library paths are provided",
                externName.c_str(), genericStatic.c_str(), genericDynamic.c_str()
            );
            console.reportError(
                Console::RUNTIME_ERROR,
                Console::formatString("External variable '%s' not found in any specified libraries",
                                 externName.c_str()),
                suggestion,
                statement->getSpan()
            );
            return nullptr;
        }

        // Create external global variable
        llvm::GlobalVariable* externVar = new llvm::GlobalVariable(
            *activeModule,
            type,
            isConstant,
            llvm::GlobalValue::ExternalLinkage,
            nullptr,  // No initializer for extern variables
            externName
        );

        // Add to linker dependencies
        LinkDependencies::LibraryInfo info;
        if (staticExists) {
            info.name = genericStatic;
            info.path = genericStatic;
        } else if (dynamicExists) {
            info.name = genericDynamic;
            info.path = genericDynamic;
        }
        linkerDependencies.addRequiredLibrary(info.name, info);

        activeScope->set(name, externVar);
        DEBUG_LOG("Declared external variable '" + name + "' (extern name: '" + externName + "')");
        return externVar;
    }

    // --- If variable exists, reassign value ---
    if (activeScope->exists(name)) {
        llvm::Value* existingVar = activeScope->get(name);
        DEBUG_LOG("Variable '" + name + "' already exists. Reassigning value...");

        if (statement->getValue() && !isConstant) {
            llvm::Value* newValue = codegen(statement->getValue(), scope);
            if (newValue->getType() != type && !llvm::isa<llvm::AllocaInst>(newValue)) {
                newValue = generateCast(newValue, type);
                if (!newValue) {
                    std::string suggestion = Console::formatString(
                        "To resolve this:\n"
                        "1. Verify value type is compatible with variable '%s' type '%s'\n"
                        "2. Check expression type\n"
                        "3. Ensure proper type casting",
                        name.c_str(), debugType(type).c_str()
                    );
                    console.reportError(
                        Console::TYPE_ERROR,
                        Console::formatString("Failed to cast value when reassigning '%s'",
                                         name.c_str()),
                        suggestion,
                        statement->getSpan()
                    );
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
                if (auto arrayLit = std::dynamic_pointer_cast<FixedArrayExpression>(statement->getValue())) {
                    llvm::ArrayType* arrayType = llvm::cast<llvm::ArrayType>(type);
                    std::vector<llvm::Constant*> elements;

                    for (const auto& elem : arrayLit->elements) {
                        auto* val = llvm::dyn_cast<llvm::Constant>(codegen(elem, scope));
                        if (!val) {
                            std::string suggestion = Console::formatString(
                                "To resolve this:\n"
                                "1. Ensure array elements for '%s' are constant expressions\n"
                                "2. Check array initialization syntax\n"
                                "3. Verify element types",
                                name.c_str()
                            );
                            console.reportError(
                                Console::TYPE_ERROR,
                                Console::formatString("Global array '%s' has non-constant initializer element",
                                                 name.c_str()),
                                suggestion,
                                statement->getSpan()
                            );
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
                    gVar->setInitializer(constVal);
                } else {
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
            if (auto arrayLit = std::dynamic_pointer_cast<FixedArrayExpression>(statement->getValue())) {
                llvm::ArrayType* arrayType = llvm::cast<llvm::ArrayType>(type);
                std::vector<llvm::Value*> elements;
                for (const auto& elem : arrayLit->elements) {
                    elements.push_back(this->codegen(elem, scope));
                }
                createFixedArrayInPlace(alloca, arrayType, elements);
            } else {
                llvm::Value* rhs = codegen(statement->getValue(), scope);
                Builder->CreateStore(rhs, alloca, isVolatile);
            }
        } else {
            llvm::Value* initValue = codegen(statement->getValue(), scope);

            if (initValue->getType() != type && !llvm::isa<llvm::AllocaInst>(initValue)) {
                initValue = generateCast(initValue, type);
                if (!initValue) {
                    std::string suggestion = Console::formatString(
                        "To resolve this:\n"
                        "1. Verify initializer type is compatible with variable '%s' type '%s'\n"
                        "2. Check initializer expression\n"
                        "3. Ensure proper type casting",
                        name.c_str(), debugType(type).c_str()
                    );
                    console.reportError(
                        Console::TYPE_ERROR,
                        Console::formatString("Failed to cast initializer for variable '%s'",
                                         name.c_str()),
                        suggestion,
                        statement->getSpan()
                    );
                    return nullptr;
                }
            }
            
            if (initValue->getType() != alloca->getAllocatedType()) {
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
        std::string suggestion = Console::formatString(
            "To resolve this:\n"
            "1. Verify variable '%s' is defined as a local variable\n"
            "2. Check variable declaration\n"
            "3. Ensure variable is in scope",
            name.c_str()
        );
        console.reportError(
            Console::RUNTIME_ERROR,
            Console::formatString("Variable '%s' is not an AllocaInst", name.c_str()),
            suggestion,
            FileSpan() // Default span as no statement context
        );
        return nullptr;
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
    
    std::string suggestion = Console::formatString(
        "To resolve this:\n"
        "1. Verify variable '%s' is defined in scope\n"
        "2. Check for correct variable declaration\n"
        "3. Ensure variable is accessible",
        varname.c_str()
    );
    console.reportError(
        Console::RUNTIME_ERROR,
        Console::formatString("Cannot get reference to: '%s'", varname.c_str()),
        suggestion,
        FileSpan() // Default span as no statement context
    );
    return nullptr;
}

}