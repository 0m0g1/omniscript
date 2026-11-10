// src/Backends/llvm/IRGenerator/IRGeneratorCodegen.cpp
#include <functional>
#include <omniscript/Expressions/Expressions.h>
#include <omniscript/Backends/LLVM/IRGenerator.h>
#include <omniscript/Expressions/CastExpression.h>
#include <omniscript/Expressions/BlockExpression.h>
#include <omniscript/Expressions/TypeExpressions.h>
#include <omniscript/Expressions/GetterExpressions.h>
#include <omniscript/Expressions/EntityExpressions.h>
#include <omniscript/Expressions/AccessExpressions.h>
#include <omniscript/Expressions/CallableExpression.h>
#include <omniscript/Expressions/LiteralExpressions.h>
#include <omniscript/Expressions/AssignmentExpression.h>
#include <omniscript/Expressions/ControlFlowExpressions.h>

namespace Omniscript {
// Nullable<T> is lowered as an *unnamed* literal struct { i1 hasValue, T value } (see resolveLLVMType).
// Checked structurally here so this does not depend on isNullableStruct()'s exact rules.
static bool looksNullable(llvm::Type* t) {
    auto* st = llvm::dyn_cast<llvm::StructType>(t);
    return st && !st->hasName() && st->getNumElements() == 2 && st->getElementType(0)->isIntegerTy(1);
}

llvm::Value* IRGenerator::codegen(std::shared_ptr<Expression> value, SymbolTableType scope) {
    DEBUG_LOG();
    if (!value) {
        console.reportError(Console::INTERNAL_ERROR, "There is no value to be codegened.", FileSpan{});
    }
    
    if (!scope) {
        console.reportError(Console::INTERNAL_ERROR, "There is no scope for codegen to perform its operations in.", FileSpan{});
    }
    
    setSpan(value->getSpan());

    DEBUG_LOG("Calling codegen on scope '" + scope->getName() + "' for '" + value->toString() + "'.");
    
    llvm::Value* result = codegenPrimitive(value, scope);

    if (result) {
        return result;
    }

    // Handle VariableAssignment
    if (auto varAssign = std::dynamic_pointer_cast<VariableAssignment>(value)) {
        DEBUG_LOG("Assigning variable " + varAssign->variableName + " of type " + varAssign->getType()->toString());
        return assignVariable(varAssign, scope);
    }

    if (auto castExpr = std::dynamic_pointer_cast<CastExpression>(value)) {
        DEBUG_LOG("Generating cast from type " + castExpr->targetExpr->getType()->toString() + " to " + castExpr->type->toString());

        llvm::Value* src = codegen(castExpr->targetExpr, scope);
        if (!src) return nullptr;

        llvm::Type* destType = resolveLLVMType(castExpr->type);
        return generateCast(src, destType);
    }

    if (auto nullable = std::dynamic_pointer_cast<NullableExpression>(value)) {
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
    // Address of a variable's storage. For a Nullable<T> variable ({ i1 hasValue, T value }) the
    // address handed out is that of the wrapped T value (field 1), so `r.method()` on a `Stats?`
    // passes a real Stats* as `this` instead of a pointer to the nullable wrapper.
    auto refToVariableValue = [&](const std::string& varName) -> llvm::Value* {
        llvm::Value* ref = getReferenceToVariable(varName);
        if (!ref) return nullptr;
        llvm::Type* storage = nullptr;
        if (auto* alloca = llvm::dyn_cast<llvm::AllocaInst>(ref)) storage = alloca->getAllocatedType();
        else if (auto* global = llvm::dyn_cast<llvm::GlobalVariable>(ref)) storage = global->getValueType();
        if (storage && looksNullable(storage)) {
            DEBUG_LOG("Reference to nullable variable '" + varName + "': using the address of its value field");
            return Builder->CreateStructGEP(storage, ref, 1, varName + ".value");
        }
        return ref;
    };

    if (auto refValue = std::dynamic_pointer_cast<ReferenceExpression>(value)) {
        // Reference to an lvalue expression with no variable name (e.g. 'this.position'):
        // compute the address of the expression itself instead of looking up a name.
        if (refValue->referentName.empty() && refValue->referent) {
            DEBUG_LOG("Creating reference to expression " + refValue->referent->toString());

            std::function<llvm::Value*(const std::shared_ptr<Expression>&)> addressOf;
            addressOf = [&](const std::shared_ptr<Expression>& e) -> llvm::Value* {
                if (auto var = std::dynamic_pointer_cast<VariableAccessExpression>(e)) {
                    // The scope holds the variable's storage (alloca / global / direct argument)
                    return refToVariableValue(var->variableName);
                }

                if (auto member = std::dynamic_pointer_cast<MemberAccessExpression>(e)) {
                    if (member->index < 0 || !member->expr) {
                        console.reportError(Console::SEMANTIC_ERROR, "Cannot take the address of member '" + member->member + "': unresolved member index", FileSpan{});
                        return nullptr;
                    }

                    auto baseOmniType = member->expr->getType();
                    if (!baseOmniType) {
                        console.reportError(Console::TYPE_ERROR, "Cannot take the address of member '" + member->member + "': base has no type", FileSpan{});
                        return nullptr;
                    }

                    llvm::Value* baseAddr = addressOf(member->expr);
                    if (!baseAddr) return nullptr;

                    std::shared_ptr<Type> structOmniType = baseOmniType;
                    if (baseOmniType->isPointer()) {
                        structOmniType = baseOmniType->getPointeeType();
                        // A pointer-typed base: 'baseAddr' is either the pointer itself (a direct
                        // function argument such as 'this') or the storage holding the pointer.
                        if (!llvm::isa<llvm::Argument>(baseAddr)) {
                            baseAddr = Builder->CreateLoad(
                                llvm::PointerType::get(*Context, 0), baseAddr, "ref.base");
                        }
                    }

                    auto* structType = llvm::dyn_cast_or_null<llvm::StructType>(resolveLLVMType(structOmniType));
                    if (!structType) {
                        console.reportError(Console::TYPE_ERROR, "Cannot take the address of member '" + member->member + "': base is not a struct", FileSpan{});
                        return nullptr;
                    }
                    if ((unsigned)member->index >= structType->getNumElements()) {
                        console.reportError(Console::SEMANTIC_ERROR, "Member index " + std::to_string(member->index) + " out of range for '" +
                                      member->member + "'", FileSpan{});
                        return nullptr;
                    }
                    return Builder->CreateStructGEP(structType, baseAddr, (unsigned)member->index,
                                                    "ref." + member->member);
                }

                if (auto idx = std::dynamic_pointer_cast<IndexAccessExpression>(e)) {
                    // &base[i]: GEP to the element, no load
                    if (!idx->expr || !idx->indexExpr || !idx->getType()) {
                        console.reportError(Console::SEMANTIC_ERROR, "Cannot take the address of an incomplete index expression", FileSpan{});
                        return nullptr;
                    }
                    llvm::Value* baseVal = codegen(idx->expr, scope);
                    llvm::Value* indexVal = codegen(idx->indexExpr, scope);
                    if (!baseVal || !indexVal) return nullptr;
                    llvm::Type* elemTy = resolveLLVMType(idx->getType());
                    return Builder->CreateGEP(elemTy, baseVal, indexVal, "ref.index");
                }

                console.reportError(Console::SEMANTIC_ERROR, "Cannot take a reference to expression: " + e->toString(), FileSpan{});
                return nullptr;
            };

            return addressOf(refValue->referent);
        }

        DEBUG_LOG("Creating reference to variable " + refValue->referentName);
        return refToVariableValue(refValue->referentName);
    }
    
    if (auto addressOf = std::dynamic_pointer_cast<AddressOfExpression>(value)) {
        DEBUG_LOG("Getting the address of variable " + addressOf->variableName);
        return getAddressOf(addressOf->variableName);
    }

    if (auto null = std::dynamic_pointer_cast<NullExpression>(value)) {
        DEBUG_LOG("Creating a null value");
        auto innerType = resolveLLVMType(null->getType());
        return createNullValue(innerType);
    }

    else if (auto rawPtr = std::dynamic_pointer_cast<RawPointerExpression>(value)) {
        DEBUG_LOG("Creating raw pointer from address: " + std::to_string(rawPtr->address));
        
        // Get the LLVM type for the pointee
        llvm::Type* pointeeType = resolveLLVMType(rawPtr->getType()->getPointeeType());
        
        // Handle null pointer case
        if (rawPtr->address == 0) {
            return llvm::ConstantPointerNull::get(
                llvm::PointerType::get(*Context, 0)
            );
        }
        
        // Create the raw pointer
        return createRawPointer(rawPtr->address, pointeeType);
    }
    
    if (auto block = std::dynamic_pointer_cast<BlockExpression>(value)) {
        DEBUG_LOG("Evaluating a block value — First pass (registration)");

        for (const auto& expr : block->values) {
            if (auto structExpr = std::dynamic_pointer_cast<StructExpression>(expr)) {
                codegen(expr, scope);
            } else if (auto func = std::dynamic_pointer_cast<FunctionExpression>(expr)) {
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
            if (auto structExpr = std::dynamic_pointer_cast<StructExpression>(value)) {
                continue;
            }

            if (std::dynamic_pointer_cast<FunctionExpression>(expr)) {
                continue;
            }

            if (auto ret = std::dynamic_pointer_cast<ReturnExpression>(expr)) {
                if (ret->getType()) {
                    if (!ret->getType()->isVoidLike()) {
                        return codegen(expr, scope);
                    }
                } else {
                    console.reportError(Console::TYPE_ERROR, "The return type has no type", FileSpan{});
                }
            } 

            if (auto varAssign = std::dynamic_pointer_cast<VariableAssignment>(expr)) {
                if (!varAssign->isStatic) {
                    varAssign->isGlobal = block->isGlobal;
                }
            }

            codegen(expr, scope);
        }

        return nullptr;
    }

    if (auto nullpointer = std::dynamic_pointer_cast<NullPointerExpression>(value)) {
        DEBUG_LOG("Creating a null pointer of type " + nullpointer->getType()->toString());
        DEBUG_LOG("Creating a null pointer of root type " + nullpointer->getRootType()->toString());
        auto pointeeType = resolveLLVMType(nullpointer->getRootType()->getPointeeType());
        return createNullPointer(pointeeType);
    }

    if (auto func = std::dynamic_pointer_cast<FunctionExpression>(value)) {
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

    if (auto ret = std::dynamic_pointer_cast<ReturnExpression>(value)) {
        DEBUG_LOG("Creating a return statement of kind '" + ret->getType()->toString() + "'.");

        llvm::Type* type = resolveLLVMType(ret->getType());
        if (!ret->value) {
            return createReturn(nullptr, nullptr);
        }
        llvm::Value* val = codegen(ret->value, scope);
        return createReturn(val, type);
    }

    if (auto unary = std::dynamic_pointer_cast<UnaryExpression>(value)) {
        DEBUG_LOG("Creating a unary expression");
        llvm::Value* operandVal = codegen(unary->operand, scope);
        if (!operandVal) {
            console.reportError(Console::SEMANTIC_ERROR, "The operand value is invalid", FileSpan{});
        }
        return createUnaryExpression(operandVal, unary->op.getType(), unary->isPrefix);
    }

    if (auto binary = std::dynamic_pointer_cast<BinaryExpression>(value)) {
        DEBUG_LOG("Creating a binary expression: " + binary->left->toString() + " " + binary->op.getValue() + " " + binary->right->toString());
        llvm::Value* lhs = codegen(binary->left, scope);
        llvm::Value* rhs = codegen(binary->right, scope);
        if (!lhs) {
            console.reportError(Console::SEMANTIC_ERROR, "The lhs value is invalid", FileSpan{});
            return nullptr;
        }
        if (!rhs) {
            console.reportError(Console::SEMANTIC_ERROR, "The rhs value is invalid", FileSpan{});
            return nullptr;
        }
        DEBUG_LOG("lhs type: '" + debugType(lhs->getType()) + "' rhs type: '" + debugType(rhs->getType()) + "'.");

        // `nullable == null` / `nullable != null` / `null == nullable`: decide this from the AST, where a
        // null literal is unambiguous, rather than guessing from the LLVM value. Nullable layout is
        // { i1 hasValue, T value } (the null literal above is built with flag = false).
        if (binary->op.getType() == TokenTypes::Equals || binary->op.getType() == TokenTypes::NotEquals) {
            auto isNullAst = [](const std::shared_ptr<Expression>& e) {
                auto n = std::dynamic_pointer_cast<NullableExpression>(e);
                return n && n->isNull();
            };
            bool leftIsNull = isNullAst(binary->left);
            bool rightIsNull = isNullAst(binary->right);
            DEBUG_LOG(std::string("null-compare check: leftIsNull=") + (leftIsNull ? "1" : "0") +
                      " rightIsNull=" + (rightIsNull ? "1" : "0") +
                      " lhsNullable=" + (looksNullable(lhs->getType()) ? "1" : "0") +
                      " rhsNullable=" + (looksNullable(rhs->getType()) ? "1" : "0"));
            if (leftIsNull != rightIsNull) {
                llvm::Value* nullableVal = rightIsNull ? lhs : rhs;
                if (looksNullable(nullableVal->getType())) {
                    llvm::Value* hasValue = Builder->CreateExtractValue(nullableVal, {0}, "hasValue");
                    return binary->op.getType() == TokenTypes::Equals
                        ? Builder->CreateNot(hasValue, "isnull")
                        : hasValue;
                }
            }
        }

        return createBinaryExpression(lhs, binary->op.getType(), rhs);
    }

    if (auto ternary = std::dynamic_pointer_cast<TernaryExpression>(value)) {
        DEBUG_LOG("Creating a ternary expression");
        llvm::Value* cond = codegen(ternary->condition, scope);
        llvm::Value* truthy = codegen(ternary->truthy, scope);
        llvm::Value* falsey = codegen(ternary->falsey, scope);
        if (!cond || !truthy || !falsey) return nullptr;
        return createTernaryExpression(cond, truthy, falsey);
    }

    if (auto var = std::dynamic_pointer_cast<VariableAccessExpression>(value)) {
        DEBUG_LOG("Accessing variable: " + var->variableName);
        
        if (var->extractValue) {
            DEBUG_LOG("Extracting the value of a nullable");
        }

        return getVariable(var->variableName, false);
    }

    if (auto instance = std::dynamic_pointer_cast<InstanceExpression>(value)) {
        DEBUG_LOG("Creating an instance of '" + instance->baseName + "'.");

        // Check if we already generated this instance
        if (!instance->instanceName.empty()) {
            // Try to get existing LLVM value if it exists
            auto existing = activeScope->lookup(instance->instanceName);
            if (existing.found() && existing.value) {
                return existing.value;
            }
        }
        
        std::vector<llvm::Value*> args;
        args.reserve(instance->members.size()); // Pre-reserve to avoid reallocations

        for (const auto& arg : instance->members) {
            DEBUG_LOG(arg->toString());

            if (auto member = std::dynamic_pointer_cast<MemberExpression>(arg)) {
                if (auto value = codegen(member->value, scope)) {
                    args.push_back(value);
                }
            } else {
                console.reportError(Console::TYPE_ERROR, formatError("Failed to generate code for constructor argument: " + arg->toString()), FileSpan{});
            }
            
        }

        DEBUG_LOG("Creating an object instance");
        auto result = createObjectInstance(instance->baseName, instance->instanceName, args, false);
        
        // Store the result if it has a name
        if (!instance->instanceName.empty()) {
            activeScope->setConstant(instance->instanceName, result);
        }
        
        return result;
    }

    if (auto call = std::dynamic_pointer_cast<CallExpression>(value)) {
        DEBUG_LOG("Calling " + call->calleeName);
        
        std::vector<llvm::Value*> args;
        args.reserve(call->args.size()); // Pre-reserve to avoid reallocations

        for (const auto& arg : call->args) {
            DEBUG_LOG(arg->toString());

            if (auto arr = std::dynamic_pointer_cast<ArrayExpression>(arg); arr && arr->isVariadicArray) {
                // If variadic, reserve space ahead (optional perf tweak)
                args.reserve(args.size() + arr->elements.size());

                for (const auto& element : arr->elements) {
                    if (auto value = codegen(element, scope)) {
                        args.push_back(value);
                    } else {
                        console.reportError(Console::SEMANTIC_ERROR, formatError("Failed to generate code for variadic argument element."), FileSpan{});
                    }
                }
            } else {
                if (auto value = codegen(arg, scope)) {
                    args.push_back(value);
                } else {
                    console.reportError(Console::SEMANTIC_ERROR, formatError("Failed to generate code for argument: " + arg->toString()), FileSpan{});
                }
            }
        }


        if (call->instanceName.empty()) {
            DEBUG_LOG("Creating a normal call for " + call->calleeName);
            return createCall(call->calleeName, args, call->functionTypeName);
        }
        DEBUG_LOG("Creating an object instance");
        return createObjectInstance(call->calleeName, call->instanceName, args, call->isGlobal);
    }

    if (auto structExpr = std::dynamic_pointer_cast<StructExpression>(value)) {
        std::vector<llvm::Type*> fieldTypes;
        fieldTypes.reserve(structExpr->parameters.size());
        
        int methodsCount = structExpr->parameters.size();

        for (const auto& field : structExpr->parameters) {
            if (auto input = std::dynamic_pointer_cast<FunctionInputExpression>(field)) {
                llvm::Type* llvmFieldType = resolveLLVMType(input->getType());
                if (!llvmFieldType) {
                    console.reportError(Console::TYPE_ERROR, "Failed to generate type for field '" + input->name + "' in struct '" + structExpr->name + "'.", FileSpan{});
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
                if (auto method = std::dynamic_pointer_cast<FunctionExpression>(field)) {
                    llvm::Type* returnType = resolveLLVMType(method->returnType);
                    llvm::Function* methd = registerFunction(method->mangledName, returnType, method->parameters, scope);
                    methods.emplace_back(methd);
                }
            }
            
            int methodIndex = 0;
            for (const auto& field : structExpr->parameters) {
                if (auto method = std::dynamic_pointer_cast<FunctionExpression>(field)) {
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

    if (auto classExpr = std::dynamic_pointer_cast<ClassExpression>(value)) {
        return codegen(classExpr->structExpr, scope);
    }

    if (auto ifExpr = std::dynamic_pointer_cast<IfExpression>(value)) {
        DEBUG_LOG("Creating an if expression");
        return createIfStatement(ifExpr->conditions, ifExpr->bodies, ifExpr->elseBody, scope);
    }

    if (auto enumExpr = std::dynamic_pointer_cast<EnumExpression>(value)) {
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
    
    if (auto forExpr = std::dynamic_pointer_cast<ForLoopExpression>(value)) {
        DEBUG_LOG("Processing ForLoopExpression");
        return createForLoop(forExpr, scope);
    }

    if (auto whileExpr = std::dynamic_pointer_cast<WhileLoopExpression>(value)) {
        DEBUG_LOG("Processing WhileLoopExpression");
        return createWhileLoop(whileExpr, scope);
    }

    // Handle access expressions recursively
    if (auto accessExpr = std::dynamic_pointer_cast<AccessExpression>(value)) {
        return handleAccessExpression(accessExpr, scope);
    }

    if (auto moduleExpr = std::dynamic_pointer_cast<ModuleExpression>(value)) {
        DEBUG_LOG("Processing ModuleExpression: " + moduleExpr->name);
    
        // Create a new scope for the module
        auto moduleScope = std::make_shared<SymbolTable<std::shared_ptr<Expression>, std::shared_ptr<Type>>>(scope);
    
        // Store generated IR values for module members
        std::unordered_map<std::string, llvm::Value*> memberIRValues;
    
        for (const auto& member : moduleExpr->members) {
            std::string memberName = member->name;
            DEBUG_LOG("Generating IR for module member: " + memberName);
    
            llvm::Value* memberValue = codegen(member->value, moduleScope);
            if (!memberValue) {
                console.reportError(Console::SEMANTIC_ERROR, "Failed to generate IR for module member: " + memberName, FileSpan{});
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
    
    if (auto typeDecl = std::dynamic_pointer_cast<TypeDeclarationExpression>(value)) {
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

}