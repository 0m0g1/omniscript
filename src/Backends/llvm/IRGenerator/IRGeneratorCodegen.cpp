#include <omniscript/Backends/LLVM/IRGenerator.h>
#include <omniscript/Expressions/TypeExpressions.h>

llvm::Value* IRGenerator::codegen(std::shared_ptr<Omniscript::Expression> value, SymbolTableType scope) {
    Omniscript::setPosition(value->getPosition());
    DEBUG_LOG();
    if (!value) {
        console.error("There is no value to be codegened.");
    }
    if (!scope) {
        console.error("There is no scope for codegen to perform its operations in.");
    }

    DEBUG_LOG("Calling codegen on scope '" + scope->getName() + "' for '" + value->toString() + "'.");
    
    llvm::Value* result = codegenPrimitive(value, scope);

    if (result) {
        return result;
    }

    // Handle VariableAssignment
    if (auto varAssign = std::dynamic_pointer_cast<Omniscript::VariableAssignment>(value)) {
        DEBUG_LOG("Assigning variable " + varAssign->variableName + " of type " + varAssign->getType()->toString());
        return assignVariable(varAssign, scope);
    }

    if (auto castExpr = std::dynamic_pointer_cast<Omniscript::CastExpression>(value)) {
        DEBUG_LOG("Generating cast from type " + castExpr->targetExpr->getType()->toString() + " to " + castExpr->type->toString());

        llvm::Value* src = codegen(castExpr->targetExpr, scope);
        if (!src) return nullptr;

        llvm::Type* destType = resolveLLVMType(castExpr->type);
        return generateCast(src, destType);
    }

    if (auto nullable = std::dynamic_pointer_cast<Omniscript::NullableExpression>(value)) {
        DEBUG_LOG("Generating nullable expression");

        if (nullable->isNull()) {
            DEBUG_LOG("Nullable expression has null value");
            // Return a 'null' representation
            // Assuming you use `{ i1, T }` style struct, set `is_null = true` and `value = undef`
            llvm::Type* innerType = resolveLLVMType(nullable->getType()); // Get full nullable type
            llvm::StructType* nullableType = llvm::StructType::get(*Context, {
                llvm::Type::getInt1Ty(*Context), // is_null
                innerType
            });

            llvm::Value* undefVal = llvm::UndefValue::get(innerType);
            llvm::Value* isNull = llvm::ConstantInt::getFalse(*Context);

            llvm::Value* result = llvm::UndefValue::get(nullableType);
            result = Builder->CreateInsertValue(result, isNull, {0});
            result = Builder->CreateInsertValue(result, undefVal, {1});
            return result;
        }

        // If value is not null
        // Generate the value
        llvm::Value* innerValue = codegen(nullable->inner, scope);

        llvm::Type* innerType = innerValue->getType();
        llvm::StructType* nullableType = llvm::StructType::get(*Context, {
            llvm::Type::getInt1Ty(*Context), // is_null
            innerType
        });

        llvm::Value* isNull = llvm::ConstantInt::getTrue(*Context);

        llvm::Value* result = llvm::UndefValue::get(nullableType);
        result = Builder->CreateInsertValue(result, isNull, {0});
        result = Builder->CreateInsertValue(result, innerValue, {1});

        return result;
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

    else if (auto rawPtr = std::dynamic_pointer_cast<Omniscript::RawPointerExpression>(value)) {
        DEBUG_LOG("Creating raw pointer from address: " + std::to_string(rawPtr->address));
        
        // Get the LLVM type for the pointee
        llvm::Type* pointeeType = resolveLLVMType(rawPtr->getType()->getPointeeType());
        
        // Handle null pointer case
        if (rawPtr->address == 0) {
            return llvm::ConstantPointerNull::get(
                llvm::PointerType::get(pointeeType, 0)
            );
        }
        
        // Create the raw pointer
        return createRawPointer(rawPtr->address, pointeeType);
    }
    
    if (auto block = std::dynamic_pointer_cast<Omniscript::BlockExpression>(value)) {
        DEBUG_LOG("Evaluating a block value — First pass (registration)");

        for (const auto& expr : block->values) {
            if (auto func = std::dynamic_pointer_cast<Omniscript::FunctionExpression>(expr)) {
                DEBUG_LOG("Processing function declaration: " + func->name + " (mangled: " + func->mangledName + ")");
                llvm::Type* returnType = resolveLLVMType(func->returnType);

                if (func->isExtern) {
                    createExternFunction(func, scope);
                } else if (func->isIntrinsic) {
                    std::string nameWithoutPrefix = func->intrinsicName;
                    const std::string prefix = "intrinsic_";
                    if (nameWithoutPrefix.rfind(prefix, 0) == 0)
                        nameWithoutPrefix = nameWithoutPrefix.substr(prefix.size());

                    createIntrinsicFunction(
                        func->mangledName,
                        "llvm." + nameWithoutPrefix,
                        returnType
                    );
                } else {
                    registerFunction(
                        func->mangledName,
                        returnType,
                        func->parameters,
                        scope,
                        func->isVarArg
                    );
                    userDefinedFunctions.push_back(func);
                }
            }
        }

        for (const auto& expr : block->values) {
            if (std::dynamic_pointer_cast<Omniscript::FunctionExpression>(expr)) {
                continue;
            }

            if (auto ret = std::dynamic_pointer_cast<Omniscript::ReturnExpression>(expr)) {
                if (ret->getType()) {
                    if (!ret->getType()->isVoidLike()) {
                        return codegen(expr, scope);
                    }
                } else {
                    console.error("The return type has no type");
                }
            } 

            if (auto varAssign = std::dynamic_pointer_cast<Omniscript::VariableAssignment>(expr)) {
                if (!varAssign->isStatic) {
                    varAssign->isGlobal = block->isGlobal;
                }
            }

            codegen(expr, scope);
        }

        return nullptr;
    }

    if (auto nullpointer = std::dynamic_pointer_cast<Omniscript::NullPointerExpression>(value)) {
        DEBUG_LOG("Creating a null pointer of type " + nullpointer->getType()->toString());
        DEBUG_LOG("Creating a null pointer of root type " + nullpointer->getRootType()->toString());
        auto pointeeType = resolveLLVMType(nullpointer->getRootType()->getPointeeType());
        return createNullPointer(pointeeType);
    }

    if (auto func = std::dynamic_pointer_cast<Omniscript::FunctionExpression>(value)) {
        DEBUG_LOG("Creating an overload for function " + func->name + " with mangled name '" + func->mangledName + "'");
        llvm::Type* returnType = resolveLLVMType(func->returnType);
        if (func->isExtern) {
            return createExternFunction(func, scope);
        } else if (func->isIntrinsic) {
            std::string nameWithoutPrefix = func->intrinsicName;
            const std::string prefix = "intrinsic_";

            // Strip 'intrinsic_' prefix if present
            if (nameWithoutPrefix.rfind(prefix, 0) == 0) {
                nameWithoutPrefix = nameWithoutPrefix.substr(prefix.size());
            }

            return createIntrinsicFunction(
                func->mangledName,
                "llvm." + nameWithoutPrefix,
                returnType
            );
        }

        if (configs.mode == CompileMode::JIT) {
            if (func->mangledName == "main") {
                func->mangledName = "__main";
            }
        }

        registerFunction(
            func->mangledName,
            returnType,
            func->parameters,
            scope,
            func->isVarArg
        );
        userDefinedFunctions.push_back(func);
    }

    if (auto ret = std::dynamic_pointer_cast<Omniscript::ReturnExpression>(value)) {
        DEBUG_LOG("Creating a return statement of kind '" + ret->getType()->toString() + "'.");

        llvm::Type* type = resolveLLVMType(ret->getType());
        if (!ret->value) {
            return createReturn(nullptr, nullptr);
        }
        llvm::Value* val = codegen(ret->value, scope);
        return createReturn(val, type);
    }

    if (auto unary = std::dynamic_pointer_cast<Omniscript::UnaryExpression>(value)) {
        DEBUG_LOG("Creating a unary expression");
        llvm::Value* operandVal = codegen(unary->operand, scope);
        if (!operandVal) {
            console.error("The operand value is invalid");
        }
        return createUnaryExpression(operandVal, unary->op.getType(), unary->isPrefix);
    }

    if (auto binary = std::dynamic_pointer_cast<Omniscript::BinaryExpression>(value)) {
        DEBUG_LOG("Creating a binary expression: " + binary->left->toString() + " " + binary->op.getValue() + " " + binary->right->toString());
        llvm::Value* lhs = codegen(binary->left, scope);
        llvm::Value* rhs = codegen(binary->right, scope);
        DEBUG_LOG("lhs type: '" + debugType(lhs->getType()) + "' rhs type: '" + debugType(rhs->getType()) + "'.");
        if (!lhs) {
            console.error("The lhs value is invalid");
        }
        if (!rhs) {
            console.error("The rhs value is invalid");
        }
        return createBinaryExpression(lhs, binary->op.getType(), rhs);
    }

    if (auto ternary = std::dynamic_pointer_cast<Omniscript::TernaryExpression>(value)) {
        DEBUG_LOG("Creating a ternary expression");
        llvm::Value* cond = codegen(ternary->condition, scope);
        llvm::Value* truthy = codegen(ternary->truthy, scope);
        llvm::Value* falsey = codegen(ternary->falsey, scope);
        if (!cond || !truthy || !falsey) return nullptr;
        return createTernaryExpression(cond, truthy, falsey);
    }

    if (auto var = std::dynamic_pointer_cast<Omniscript::VariableAccessExpression>(value)) {
        DEBUG_LOG("Accessing variable: " + var->variableName);
        if (var->extractValue) {
            DEBUG_LOG("Extracting the value of a nullable");
        }
        // return getVariable(var->variableName, var->extractValue);
        return getVariable(var->variableName, false);
    }

    if (auto call = std::dynamic_pointer_cast<Omniscript::CallExpression>(value)) {
        DEBUG_LOG("Calling " + call->calleeName);
        
        std::vector<llvm::Value*> args;
        args.reserve(call->args.size()); // Pre-reserve to avoid reallocations

        for (const auto& arg : call->args) {
            DEBUG_LOG(arg->toString());

            if (auto arr = std::dynamic_pointer_cast<Omniscript::ArrayExpression>(arg); arr && arr->isVariadicArray) {
                // If variadic, reserve space ahead (optional perf tweak)
                args.reserve(args.size() + arr->elements.size());

                for (const auto& element : arr->elements) {
                    if (auto value = codegen(element, scope)) {
                        args.push_back(value);
                    } else {
                        console.error(formatError("Failed to generate code for variadic argument element."));
                    }
                }
            } else {
                if (auto value = codegen(arg, scope)) {
                    args.push_back(value);
                } else {
                    console.error(formatError("Failed to generate code for argument: " + arg->toString()));
                }
            }
        }


        if (call->instanceName.empty()) {
            DEBUG_LOG("Creating a normal call for " + call->calleeName);
            return createCall(call->calleeName, args);
        }
        DEBUG_LOG("Creating an object instance");
        return createObjectInstance(call->calleeName, call->instanceName, args, call->isGlobal);
    }

    if (auto structExpr = std::dynamic_pointer_cast<Omniscript::StructExpression>(value)) {
        std::vector<llvm::Type*> fieldTypes;
        fieldTypes.reserve(structExpr->parameters.size());
        
        int methodsCount = structExpr->parameters.size();

        for (const auto& field : structExpr->parameters) {
            if (auto input = std::dynamic_pointer_cast<Omniscript::FunctionInputExpression>(field)) {
                llvm::Type* llvmFieldType = resolveLLVMType(input->getType());
                if (!llvmFieldType) {
                    console.error("Failed to generate type for field '" + input->name + "' in struct '" + structExpr->name + "'.");
                    return nullptr;
                }
        
                fieldTypes.push_back(llvmFieldType);
                methodsCount--;
            }
        }
        
        // Create the LLVM struct type (opaque or packed depending on your system)
        createStructType(structExpr->name, fieldTypes);

        if (methodsCount > 0) {
            std::vector<llvm::Function*> methods;
    
            methods.reserve(methodsCount);
    
            for (const auto& field : structExpr->parameters) {
                if (auto method = std::dynamic_pointer_cast<Omniscript::FunctionExpression>(field)) {
                    llvm::Type* returnType = resolveLLVMType(method->returnType);
                    llvm::Function* methd = registerFunction(method->mangledName, returnType, method->parameters, scope);
                    methods.emplace_back(methd);
                }
            }
            
            int methodIndex = 0;
            for (const auto& field : structExpr->parameters) {
                if (auto method = std::dynamic_pointer_cast<Omniscript::FunctionExpression>(field)) {
                    generateFunctionBody(
                        method->mangledName,
                        methods[methodIndex],
                        method->parameters,
                        method->body,
                        scope
                    );
                    methodIndex++;
                }
        }
        }

        return nullptr;
    }

    if (auto classExpr = std::dynamic_pointer_cast<Omniscript::ClassExpression>(value)) {
        return codegen(classExpr->structExpr, scope);
    }

    if (auto ifExpr = std::dynamic_pointer_cast<Omniscript::IfExpression>(value)) {
        DEBUG_LOG("Creating an if expression");
        return createIfStatement(ifExpr->conditions, ifExpr->bodies, ifExpr->elseBody, scope);
    }

    if (auto enumExpr = std::dynamic_pointer_cast<Omniscript::EnumExpression>(value)) {
        DEBUG_LOG("Processing EnumExpression for enum '" + enumExpr->enumName + "'");
    
        std::vector<std::string> names;
        std::vector<llvm::Value*> values;
    
        // Reserve space to avoid repeated allocations
        names.reserve(enumExpr->expressionMap.size());
        values.reserve(enumExpr->expressionMap.size());
    
        for (const auto& [name, val] : enumExpr->expressionMap) {
            llvm::Value* enumValue = codegen(val, scope); // Generate LLVM IR for each enum entry
    
            if (enumValue) {
                names.emplace_back(name);
                values.emplace_back(enumValue);
                DEBUG_LOG("Enum '" + enumExpr->enumName + "' has enumerator '" + name + "' with value " + val->toString());
            } else {
                DEBUG_LOG("Failed to generate IR for enumerator '" + name + "'");
            }
        }
    
        // Call the appropriate method depending on flags
        if (enumExpr->isEnumClass && enumExpr->hasLookup) {
            return createEnumClassWithLookup(names, values, enumExpr->enumName, /*isGlobal=*/true);
        } else if (enumExpr->isEnumClass) {
            return createEnumClass(names, values, enumExpr->enumName, /*isGlobal=*/true);
        } else if (enumExpr->hasLookup) {
            return createEnumWithLookup(names, values, enumExpr->enumName, /*isGlobal=*/true);
        } else {
            return createEnum(names, values, enumExpr->enumName, /*isGlobal=*/true);
        }
    }
    
    if (auto forExpr = std::dynamic_pointer_cast<Omniscript::ForLoopExpression>(value)) {
        DEBUG_LOG("Processing ForLoopExpression");
        return createForLoop(forExpr, scope);
    }

    if (auto whileExpr = std::dynamic_pointer_cast<Omniscript::WhileLoopExpression>(value)) {
        DEBUG_LOG("Processing WhileLoopExpression");
        return createWhileLoop(whileExpr, scope);
    }

    // Handle access expressions recursively
    if (auto accessExpr = std::dynamic_pointer_cast<Omniscript::AccessExpression>(value)) {
        return handleAccessExpression(accessExpr, scope);
    }

    if (auto moduleExpr = std::dynamic_pointer_cast<Omniscript::ModuleExpression>(value)) {
        DEBUG_LOG("Processing ModuleExpression: " + moduleExpr->name);
    
        // Create a new scope for the module
        auto moduleScope = std::make_shared<SymbolTable<std::shared_ptr<Omniscript::Expression>, std::shared_ptr<Omniscript::Type>>>(scope);
    
        // Store generated IR values for module members
        std::unordered_map<std::string, llvm::Value*> memberIRValues;
    
        for (const auto& member : moduleExpr->members) {
            std::string memberName = member->name;
            DEBUG_LOG("Generating IR for module member: " + memberName);
    
            llvm::Value* memberValue = codegen(member->value, moduleScope);
            if (!memberValue) {
                console.error("Failed to generate IR for module member: " + memberName);
                continue;
            }
    
            // Save the member value and type
            memberIRValues[memberName] = memberValue;
            // moduleScope->define(memberName, memberExpr->getType());
        }
    
        // Generate the actual module object
        llvm::Value* moduleInstance = createModuleObject(
            moduleExpr->name,
            memberIRValues
        );
    
        return moduleInstance;
    } 
    
    if (auto typeDecl = std::dynamic_pointer_cast<Omniscript::TypeDeclarationExpression>(value)) {
        if (!typeDecl->isAliasing()) {
            llvm::Type* resolvedType = resolveLLVMType(typeDecl->getType());
            activeScope->addType(typeDecl->typeName, resolvedType);
        } else {
            llvm::Type* originalType = activeScope->getType(typeDecl->originalTypeName);
            activeScope->addType(typeDecl->typeName, originalType);
        }
    }

    return nullptr;
}