#include <omniscript/Backends/LLVM/IRGenerator.h>

namespace Omniscript {
llvm::Value* IRGenerator::handleAccessExpression(
    std::shared_ptr<Omniscript::AccessExpression> expr, 
    SymbolTableType scope
) {
    // First evaluate the base expression recursively
    llvm::Value* baseValue = nullptr;

    // Handle variable access case
    if (auto varAcc = std::dynamic_pointer_cast<Omniscript::VariableAccessExpression>(expr->expr)) {
        baseValue = activeScope->get(varAcc->variableName);
    } 
    // Handle nested member access in arrow access case (like std.Math->pi)
    else if (auto arrowAccess = std::dynamic_pointer_cast<Omniscript::ArrowAccessExpression>(expr)) {
        if (auto memberAccess = std::dynamic_pointer_cast<Omniscript::MemberAccessExpression>(arrowAccess->expr)) {
            // First get the base value for the member access
            llvm::Value* memberBaseValue = nullptr;
            if (auto innerVarAcc = std::dynamic_pointer_cast<Omniscript::VariableAccessExpression>(memberAccess->expr)) {
                DEBUG_LOG("Getting " + innerVarAcc->variableName);
                memberBaseValue = activeScope->get(innerVarAcc->variableName);
            } else {
                memberBaseValue = codegen(memberAccess->expr, scope);
            }
            
            // Process member access with pointer preservation
            baseValue = handleMemberAccess(memberAccess, memberBaseValue, scope, true);
        } else {
            // Regular arrow access case (ptr->member)
            baseValue = codegen(arrowAccess->expr, scope);
        }
    }
    // All other cases
    else {
        baseValue = codegen(expr->expr, scope);
    }

    if (!baseValue) {
        return nullptr;
    }

    // Handle the current access expression
    if (auto memberAccess = std::dynamic_pointer_cast<Omniscript::MemberAccessExpression>(expr)) {
        return handleMemberAccess(memberAccess, baseValue, scope);
    }
    else if (auto arrowAccess = std::dynamic_pointer_cast<Omniscript::ArrowAccessExpression>(expr)) {
        return handleArrowAccess(arrowAccess, baseValue, scope);
    }
    else if (auto derefAccess = std::dynamic_pointer_cast<Omniscript::DereferenceExpression>(expr)) {
        return handleDereference(derefAccess, baseValue, scope);
    }
    else if (auto indexAccess = std::dynamic_pointer_cast<Omniscript::IndexAccessExpression>(expr)) {
        return handleIndexAccess(indexAccess, baseValue, scope);
    }

    console.error("Unknown access expression type");
    return nullptr;
}

llvm::Value* IRGenerator::handleMemberAccess(
    std::shared_ptr<Omniscript::MemberAccessExpression> expr,
    llvm::Value* baseValue,
    SymbolTableType scope,
    bool preservePointer
) {
    llvm::Type* currentType = activeScope->getType(expr->baseType);
    llvm::Value* currentPtr = baseValue;

    int fieldIndex = expr->index;

    if (!currentType->isStructTy()) {
        console.error("Member access requires an aggregate type (struct or class), not a '" + debugType(currentType) + "'.");
        return nullptr;
    }

    auto* structType = llvm::cast<llvm::StructType>(currentType);
    currentPtr = Builder->CreateStructGEP(structType, currentPtr, fieldIndex);
    currentType = structType->getElementType(fieldIndex);
    
    if (expr->isSetter()) {
        llvm::Value* valueToStore = codegen(expr->assignmentValue, scope);
        Builder->CreateStore(valueToStore, currentPtr);
        return valueToStore;
    }

    // // Only load if we're not preserving pointers AND it's not a setter
    // if (!preservePointer) {
    //     return currentPtr;  // Return the pointer if preserving
    // }
    
    if (!preservePointer && !currentType->isStructTy()) {
        return Builder->CreateLoad(currentType, currentPtr);
    }
    return currentPtr;
}

llvm::Value* IRGenerator::handleArrowAccess(
    std::shared_ptr<Omniscript::ArrowAccessExpression> expr,
    llvm::Value* baseValue,
    SymbolTableType scope
) {
    // Validate base value is a pointer
    if (!baseValue->getType()->isPointerTy()) {
        console.error("Arrow access requires pointer type");
        return nullptr;
    }

    // Get the actual pointee type (what the pointer points to)
    llvm::Type* pointeeType = resolveLLVMType(expr->expr->getType()->getPointeeType());
    
    // Handle case where we have a pointer-to-pointer
    if (pointeeType->isPointerTy()) {
        baseValue = Builder->CreateLoad(pointeeType, baseValue);
        pointeeType = resolveLLVMType(expr->expr->getType()->getPointeeType());
    }

    // Validate we're accessing a struct
    if (!pointeeType->isStructTy()) {
        console.error("Arrow access requires pointer to struct type");
        return nullptr;
    }

    auto* structType = llvm::cast<llvm::StructType>(pointeeType);
    int fieldIndex = expr->index;

    // Validate field index
    if (fieldIndex < 0 || fieldIndex >= (int)structType->getNumElements()) {
        console.error("Invalid struct field index");
        return nullptr;
    }

    // Get pointer to the field
    llvm::Value* fieldPtr = Builder->CreateStructGEP(structType, baseValue, fieldIndex);
    llvm::Type* fieldType = structType->getElementType(fieldIndex);

    // Handle setter case
    if (expr->isSetter()) {
        llvm::Value* valueToStore = codegen(expr->assignmentValue, scope);
        // Verify type compatibility
        if (valueToStore->getType() != fieldType) {
            valueToStore = Builder->CreateBitOrPointerCast(valueToStore, fieldType);
        }
        Builder->CreateStore(valueToStore, fieldPtr);
        return valueToStore;
    }

    // Handle getter case
    return Builder->CreateLoad(fieldType, fieldPtr);
}

llvm::Value* IRGenerator::handleDereference(
    std::shared_ptr<Omniscript::DereferenceExpression> expr,
    llvm::Value* baseValue,
    SymbolTableType scope
) {
    llvm::Type* ptrType = baseValue->getType();
    if (!ptrType->isPointerTy()) {
        console.error("Dereference requires pointer type");
        return nullptr;
    }

    llvm::PointerType* pointerType = llvm::dyn_cast<llvm::PointerType>(ptrType);
    if (!pointerType) {
        console.error("Base value is not a pointer.");
        return nullptr;
    }

    llvm::Type* pointeeType = resolveLLVMType(expr->getType());  // Get the type pointed to by the pointer
    llvm::Value* loadedPtr = baseValue;

    if (expr->valueExpr) {
        // This is a dereference assignment (*ptr = value)
        llvm::Value* valueToStore = codegen(expr->valueExpr, scope);
        if (!valueToStore) return nullptr;
        
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
    std::shared_ptr<Omniscript::IndexAccessExpression> expr,
    llvm::Value* baseValue,
    SymbolTableType scope
) {
    // Evaluate the index expression
    llvm::Value* indexValue = codegen(expr->indexExpr, scope);
    if (!indexValue) return nullptr;

    llvm::Type* baseType = baseValue->getType();
    if (!baseType->isPointerTy() && !baseType->isArrayTy()) {
        console.error("Index access requires pointer or array type");
        return nullptr;
    }

    llvm::PointerType* pointerType = llvm::dyn_cast<llvm::PointerType>(baseType);
    if (!pointerType) {
        console.error("Base value is neither pointer nor array.");
        return nullptr;
    }

    // Get pointer to element
    llvm::Value* elementPtr = Builder->CreateGEP(
        resolveLLVMType(expr->getType()),  // The type pointed to by the pointer
        baseValue, 
        indexValue, 
        "index.ptr"
    );

    if (expr->isSetter()) {
        llvm::Value* valueToStore = codegen(expr->assignmentValue, scope);
        if (!valueToStore) return nullptr;
        
        Builder->CreateStore(valueToStore, elementPtr);
        return valueToStore;
    }

    // Load the value
    return Builder->CreateLoad(resolveLLVMType(expr->getType()), elementPtr, "load.index");
}

}
