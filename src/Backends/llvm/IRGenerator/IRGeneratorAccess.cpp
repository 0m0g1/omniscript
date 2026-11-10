// src/Backends/llvm/IRGenerator/IRGeneratorAccess.cpp
#include <omniscript/Backends/LLVM/IRGenerator.h>

namespace Omniscript {
llvm::Value* IRGenerator::handleAccessExpression(
    std::shared_ptr<AccessExpression> expr, 
    SymbolTableType scope
) {
    // First evaluate the base expression recursively
    llvm::Value* baseValue = nullptr;

    // Handle variable access case
    if (auto varAcc = std::dynamic_pointer_cast<VariableAccessExpression>(expr->expr)) {
        baseValue = activeScope->get(varAcc->variableName);
        if (!baseValue) {
            std::string suggestion = Console::formatString(
                "To resolve this:\n"
                "1. Verify variable '%s' is defined in scope '%s'\n"
                "2. Check variable declaration\n"
                "3. Ensure variable is accessible",
                varAcc->variableName.c_str(), scope->getName().c_str()
            );
            console.reportError(
                Console::RUNTIME_ERROR,
                Console::formatString("Variable '%s' not found in scope",
                                 varAcc->variableName.c_str()),
                suggestion,
                varAcc->getSpan()
            );
            return nullptr;
        }
    } 
    // Handle nested member access in arrow access case (like std.Math->pi)
    else if (auto arrowAccess = std::dynamic_pointer_cast<ArrowAccessExpression>(expr)) {
        if (auto memberAccess = std::dynamic_pointer_cast<MemberAccessExpression>(arrowAccess->expr)) {
            // First get the base value for the member access
            llvm::Value* memberBaseValue = nullptr;
            if (auto innerVarAcc = std::dynamic_pointer_cast<VariableAccessExpression>(memberAccess->expr)) {
                DEBUG_LOG("Getting " + innerVarAcc->variableName);
                memberBaseValue = activeScope->get(innerVarAcc->variableName);
                if (!memberBaseValue) {
                    std::string suggestion = Console::formatString(
                        "To resolve this:\n"
                        "1. Verify variable '%s' is defined in scope '%s'\n"
                        "2. Check variable declaration\n"
                        "3. Ensure variable is accessible",
                        innerVarAcc->variableName.c_str(), scope->getName().c_str()
                    );
                    console.reportError(
                        Console::RUNTIME_ERROR,
                        Console::formatString("Variable '%s' not found in scope for member access",
                                         innerVarAcc->variableName.c_str()),
                        suggestion,
                        innerVarAcc->getSpan()
                    );
                    return nullptr;
                }
            } else {
                memberBaseValue = codegen(memberAccess->expr, scope);
                if (!memberBaseValue) {
                    std::string suggestion = "To resolve this:\n"
                                           "1. Verify member access base expression is valid\n"
                                           "2. Check expression syntax and scope\n"
                                           "3. Add debug output for base expression evaluation";
                    console.reportError(
                        Console::RUNTIME_ERROR,
                        "Failed to evaluate base expression for member access",
                        suggestion,
                        memberAccess->expr->getSpan()
                    );
                    return nullptr;
                }
            }
            
            // Process member access with pointer preservation
            baseValue = handleMemberAccess(memberAccess, memberBaseValue, scope, true);
        } else {
            // Regular arrow access case (ptr->member)
            baseValue = codegen(arrowAccess->expr, scope);
            if (!baseValue) {
                std::string suggestion = "To resolve this:\n"
                                       "1. Verify arrow access base expression is valid\n"
                                       "2. Check expression syntax and scope\n"
                                       "3. Add debug output for base expression evaluation";
                console.reportError(
                    Console::RUNTIME_ERROR,
                    "Failed to evaluate base expression for arrow access",
                    suggestion,
                    arrowAccess->expr->getSpan()
                );
                return nullptr;
            }
        }
    }
    // All other cases
    else {
        baseValue = codegen(expr->expr, scope);
        if (!baseValue) {
            std::string suggestion = "To resolve this:\n"
                                   "1. Verify base expression is valid\n"
                                   "2. Check expression syntax and scope\n"
                                   "3. Add debug output for expression evaluation";
            console.reportError(
                Console::RUNTIME_ERROR,
                "Failed to evaluate base expression for access",
                suggestion,
                expr->expr->getSpan()
            );
            return nullptr;
        }
    }

    // Handle the current access expression
    if (auto memberAccess = std::dynamic_pointer_cast<MemberAccessExpression>(expr)) {
        return handleMemberAccess(memberAccess, baseValue, scope);
    }
    else if (auto arrowAccess = std::dynamic_pointer_cast<ArrowAccessExpression>(expr)) {
        return handleArrowAccess(arrowAccess, baseValue, scope);
    }
    else if (auto derefAccess = std::dynamic_pointer_cast<DereferenceExpression>(expr)) {
        return handleDereference(derefAccess, baseValue, scope);
    }
    else if (auto indexAccess = std::dynamic_pointer_cast<IndexAccessExpression>(expr)) {
        return handleIndexAccess(indexAccess, baseValue, scope);
    }

    std::string suggestion = "To resolve this:\n"
                           "1. Verify access expression type is supported\n"
                           "2. Check for correct expression type (Member, Arrow, Dereference, or Index)\n"
                           "3. Ensure proper type casting or expression setup";
    console.reportError(
        Console::RUNTIME_ERROR,
        "Unknown access expression type",
        suggestion,
        expr->getSpan()
    );
    return nullptr;
}

llvm::Value* IRGenerator::handleMemberAccess(
    std::shared_ptr<MemberAccessExpression> expr,
    llvm::Value* baseValue,
    SymbolTableType scope,
    bool preservePointer
) {
    llvm::Type* currentType = activeScope->getType(expr->baseType);
    llvm::Value* currentPtr = baseValue;

    if (!currentType) {
        console.reportError(
            Console::TYPE_ERROR,
            Console::formatString("Unknown type '%s' in member access", expr->baseType.c_str()),
            "Verify the type is declared and visible in this scope",
            expr->getSpan()
        );
        return nullptr;
    }

    if (!currentType->isStructTy()) {
        std::string suggestion = Console::formatString(
            "To resolve this:\n"
            "1. Verify type '%s' is a struct or class\n"
            "2. Check type declaration in scope '%s'\n"
            "3. Ensure member access is valid for this type",
            expr->baseType.c_str(), scope->getName().c_str()
        );
        console.reportError(
            Console::TYPE_ERROR,
            Console::formatString("Member access requires an aggregate type (struct or class), not a '%s'",
                             debugType(currentType).c_str()),
            suggestion,
            expr->getSpan()
        );
        return nullptr;
    }

    auto* structType = llvm::cast<llvm::StructType>(currentType);
    int fieldIndex = expr->index;

    if (fieldIndex < 0 || fieldIndex >= (int)structType->getNumElements()) {
        std::string suggestion = Console::formatString(
            "To resolve this:\n"
            "1. Verify field index %d is valid for struct '%s'\n"
            "2. Check struct definition\n"
            "3. Ensure correct member is accessed",
            fieldIndex, expr->baseType.c_str()
        );
        console.reportError(
            Console::RUNTIME_ERROR,
            Console::formatString("Invalid field index %d for struct '%s'",
                             fieldIndex, expr->baseType.c_str()),
            suggestion,
            expr->getSpan()
        );
        return nullptr;
    }

    currentPtr = Builder->CreateStructGEP(structType, currentPtr, fieldIndex);
    currentType = structType->getElementType(fieldIndex);
    
    if (expr->isSetter()) {
        llvm::Value* valueToStore = codegen(expr->assignmentValue, scope);
        if (!valueToStore) {
            std::string suggestion = "To resolve this:\n"
                                   "1. Verify assignment value expression is valid\n"
                                   "2. Check assignment expression syntax\n"
                                   "3. Add debug output for assignment codegen";
            console.reportError(
                Console::RUNTIME_ERROR,
                "Failed to generate code for member assignment value",
                suggestion,
                expr->assignmentValue->getSpan()
            );
            return nullptr;
        }
        // Struct and array values are passed around as pointers to their storage, so load the
        // value before storing it into an aggregate-typed field (otherwise the pointer itself
        // would be written into the field).
        if ((currentType->isStructTy() || currentType->isArrayTy()) &&
            valueToStore->getType()->isPointerTy()) {
            valueToStore = Builder->CreateLoad(currentType, valueToStore);
        }
        if (valueToStore->getType() != currentType) {
            valueToStore = generateCast(valueToStore, currentType);
            if (!valueToStore) return nullptr;
        }
        Builder->CreateStore(valueToStore, currentPtr);
        return valueToStore;
    }
    
    // Aggregates (structs and arrays) evaluate to their address, like array-to-pointer decay
    // in C. Loading an array member by value broke '&this.array' and 'this.array[i]'.
    if (!preservePointer && !currentType->isStructTy() && !currentType->isArrayTy()) {
        return Builder->CreateLoad(currentType, currentPtr);
    }
    return currentPtr;
}

llvm::Value* IRGenerator::handleArrowAccess(
    std::shared_ptr<ArrowAccessExpression> expr,
    llvm::Value* baseValue,
    SymbolTableType scope
) {
    // Validate base value is a pointer
    if (!baseValue->getType()->isPointerTy()) {
        std::string suggestion = "To resolve this:\n"
                               "1. Ensure base expression for arrow access is a pointer type\n"
                               "2. Check expression type\n"
                               "3. Verify pointer declaration";
        console.reportError(
            Console::TYPE_ERROR,
            "Arrow access requires pointer type",
            suggestion,
            expr->getSpan()
        );
        return nullptr;
    }

    // Get the actual pointee type (what the pointer points to)
    auto pointeeOmni = expr->expr->getType()->getPointeeType();
    llvm::Type* pointeeType = resolveLLVMType(pointeeOmni);
    
    // Handle case where we have a pointer-to-pointer: load the inner pointer, then peel one level
    if (pointeeType->isPointerTy()) {
        baseValue = Builder->CreateLoad(pointeeType, baseValue);
        pointeeOmni = pointeeOmni->getPointeeType();
        pointeeType = resolveLLVMType(pointeeOmni);
    }

    // Validate we're accessing a struct
    if (!pointeeType->isStructTy()) {
        std::string suggestion = "To resolve this:\n"
                               "1. Ensure arrow access points to a struct type\n"
                               "2. Check pointee type definition\n"
                               "3. Verify struct declaration";
        console.reportError(
            Console::TYPE_ERROR,
            "Arrow access requires pointer to struct type",
            suggestion,
            expr->getSpan()
        );
        return nullptr;
    }

    auto* structType = llvm::cast<llvm::StructType>(pointeeType);
    int fieldIndex = expr->index;

    // Validate field index
    if (fieldIndex < 0 || fieldIndex >= (int)structType->getNumElements()) {
        std::string suggestion = Console::formatString(
            "To resolve this:\n"
            "1. Verify field index %d is valid for struct\n"
            "2. Check struct definition\n"
            "3. Ensure correct field is accessed",
            fieldIndex
        );
        console.reportError(
            Console::RUNTIME_ERROR,
            Console::formatString("Invalid struct field index %d", fieldIndex),
            suggestion,
            expr->getSpan()
        );
        return nullptr;
    }

    // Get pointer to the field
    llvm::Value* fieldPtr = Builder->CreateStructGEP(structType, baseValue, fieldIndex);
    llvm::Type* fieldType = structType->getElementType(fieldIndex);

    // Handle setter case
    if (expr->isSetter()) {
        llvm::Value* valueToStore = codegen(expr->assignmentValue, scope);
        if (!valueToStore) {
            std::string suggestion = "To resolve this:\n"
                                   "1. Verify assignment value expression is valid\n"
                                   "2. Check assignment expression syntax\n"
                                   "3. Add debug output for assignment codegen";
            console.reportError(
                Console::RUNTIME_ERROR,
                "Failed to generate code for arrow access assignment value",
                suggestion,
                expr->assignmentValue->getSpan()
            );
            return nullptr;
        }
        // Aggregates are passed as addresses; load before storing into an aggregate field
        if ((fieldType->isStructTy() || fieldType->isArrayTy()) &&
            valueToStore->getType()->isPointerTy()) {
            valueToStore = Builder->CreateLoad(fieldType, valueToStore);
        } else if (valueToStore->getType() != fieldType) {
            valueToStore = generateCast(valueToStore, fieldType);
            if (!valueToStore) return nullptr;
        }
        Builder->CreateStore(valueToStore, fieldPtr);
        return valueToStore;
    }

    // Getter: aggregates evaluate to their address (like array decay), so chained
    // access such as p->inner.x or p->arr[i] keeps working
    if (fieldType->isStructTy() || fieldType->isArrayTy()) {
        return fieldPtr;
    }
    return Builder->CreateLoad(fieldType, fieldPtr);
}

llvm::Value* IRGenerator::handleDereference(
    std::shared_ptr<DereferenceExpression> expr,
    llvm::Value* baseValue,
    SymbolTableType scope
) {
    llvm::Type* ptrType = baseValue->getType();
    if (!ptrType->isPointerTy()) {
        std::string suggestion = "To resolve this:\n"
                               "1. Ensure base expression for dereference is a pointer type\n"
                               "2. Check expression type\n"
                               "3. Verify pointer declaration";
        console.reportError(
            Console::TYPE_ERROR,
            "Dereference requires pointer type",
            suggestion,
            expr->getSpan()
        );
        return nullptr;
    }

    llvm::PointerType* pointerType = llvm::dyn_cast<llvm::PointerType>(ptrType);
    if (!pointerType) {
        std::string suggestion = "To resolve this:\n"
                               "1. Ensure base value is a valid pointer\n"
                               "2. Check pointer type definition\n"
                               "3. Verify pointer initialization";
        console.reportError(
            Console::TYPE_ERROR,
            "Base value is not a pointer",
            suggestion,
            expr->getSpan()
        );
        return nullptr;
    }

    llvm::Type* pointeeType = resolveLLVMType(expr->getType());  // Get the type pointed to by the pointer
    llvm::Value* loadedPtr = baseValue;

    if (expr->valueExpr) {
        // This is a dereference assignment (*ptr = value)
        llvm::Value* valueToStore = codegen(expr->valueExpr, scope);
        if (!valueToStore) {
            std::string suggestion = "To resolve this:\n"
                                   "1. Verify dereference assignment value is valid\n"
                                   "2. Check assignment expression syntax\n"
                                   "3. Add debug output for assignment codegen";
            console.reportError(
                Console::RUNTIME_ERROR,
                "Failed to generate code for dereference assignment value",
                suggestion,
                expr->valueExpr->getSpan()
            );
            return nullptr;
        }
        
        if (valueToStore->getType() != pointeeType) {
            valueToStore = Builder->CreateBitCast(valueToStore, pointeeType, "bitcast.store");
        }
        Builder->CreateStore(valueToStore, loadedPtr);
        return valueToStore;
    }

    // Regular dereference (*ptr)
    return Builder->CreateLoad(pointeeType, loadedPtr, "load.deref");
}

llvm::Value* IRGenerator::handleIndexAccess(
    std::shared_ptr<IndexAccessExpression> expr,
    llvm::Value* baseValue,
    SymbolTableType scope
) {
    // Evaluate the index expression
    llvm::Value* indexValue = codegen(expr->indexExpr, scope);
    if (!indexValue) {
        std::string suggestion = "To resolve this:\n"
                               "1. Verify index expression is valid\n"
                               "2. Check index expression syntax\n"
                               "3. Add debug output for index codegen";
        console.reportError(
            Console::RUNTIME_ERROR,
            "Failed to generate code for index expression",
            suggestion,
            expr->indexExpr->getSpan()
        );
        return nullptr;
    }

    // A local/global variable of pointer type is represented by its storage (alloca / global),
    // so load the pointer it holds before indexing. Array storage is indexed directly.
    if (auto* allocaBase = llvm::dyn_cast<llvm::AllocaInst>(baseValue)) {
        if (allocaBase->getAllocatedType()->isPointerTy()) {
            baseValue = Builder->CreateLoad(allocaBase->getAllocatedType(), allocaBase, "index.base");
        }
    } else if (auto* globalBase = llvm::dyn_cast<llvm::GlobalVariable>(baseValue)) {
        if (globalBase->getValueType()->isPointerTy()) {
            baseValue = Builder->CreateLoad(globalBase->getValueType(), globalBase, "index.base");
        }
    }

    llvm::Type* baseType = baseValue->getType();
    if (!baseType->isPointerTy() && !baseType->isArrayTy()) {
        std::string suggestion = "To resolve this:\n"
                               "1. Ensure base expression for index access is a pointer or array type\n"
                               "2. Check base type definition\n"
                               "3. Verify array or pointer declaration";
        console.reportError(
            Console::TYPE_ERROR,
            "Index access requires pointer or array type",
            suggestion,
            expr->getSpan()
        );
        return nullptr;
    }

    llvm::PointerType* pointerType = llvm::dyn_cast<llvm::PointerType>(baseType);
    if (!pointerType) {
        std::string suggestion = "To resolve this:\n"
                               "1. Ensure base value is a valid pointer or array\n"
                               "2. Check base type declaration\n"
                               "3. Verify array or pointer initialization";
        console.reportError(
            Console::TYPE_ERROR,
            "Base value is neither pointer nor array",
            suggestion,
            expr->getSpan()
        );
        return nullptr;
    }

    // Get pointer to element
    if (!expr->getType()) {
        console.reportError(
            Console::RUNTIME_ERROR,
            "Index expression '" + expr->toString() + "' has no element type",
            "The frontend must set IndexAccessExpression's type to the element type (one pointer level removed).",
            expr->getSpan()
        );
        return nullptr;
    }
    llvm::Type* elemTy = resolveLLVMType(expr->getType());
    bool isAggregate = elemTy->isStructTy() || elemTy->isArrayTy();
    llvm::Value* elementPtr = Builder->CreateGEP(
        elemTy,  // The type pointed to by the pointer
        baseValue, 
        indexValue, 
        "index.ptr"
    );

    if (expr->isSetter()) {
        llvm::Value* valueToStore = codegen(expr->assignmentValue, scope);
        if (!valueToStore) {
            std::string suggestion = "To resolve this:\n"
                                   "1. Verify index assignment value is valid\n"
                                   "2. Check assignment expression syntax\n"
                                   "3. Add debug output for assignment codegen";
            console.reportError(
                Console::RUNTIME_ERROR,
                "Failed to generate code for index assignment value",
                suggestion,
                expr->assignmentValue->getSpan()
            );
            return nullptr;
        }
        
        if (isAggregate && valueToStore->getType()->isPointerTy()) {
            valueToStore = Builder->CreateLoad(elemTy, valueToStore);
        } else if (valueToStore->getType() != elemTy) {
            valueToStore = generateCast(valueToStore, elemTy);
            if (!valueToStore) return nullptr;
        }
        Builder->CreateStore(valueToStore, elementPtr);
        return valueToStore;
    }

    // Aggregates evaluate to their address so arr[i].x GEPs from the element, not a loaded copy
    if (isAggregate) {
        return elementPtr;
    }
    return Builder->CreateLoad(elemTy, elementPtr, "load.index");
}

}