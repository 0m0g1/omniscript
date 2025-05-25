#include <omniscript/Core.h>
#include <omniscript/utils.h>
#include <omniscript/engine/Statement.h>
#include <omniscript/engine/Symboltable.h>
#include <omniscript/omniscript_pch.h>
#include <omniscript/utils.h>

// ============================== Accesses  ============================== //

void Access::verifyMemberAccessibility() {

}

std::shared_ptr<Omniscript::Expression> MemberAccess::express(SymbolTableType scope) {
    std::shared_ptr<Omniscript::Expression> baseExpr = nullptr;
    std::string baseTypeName;

    // Evaluate base expression or get type from variable
    if (object) {
        if (auto getter = std::dynamic_pointer_cast<GetVariable>(object)) {
            objectName = getter->getName();
            auto var = scope->get(objectName);
            if (!var) {
                console.error("Variable '" + objectName + "' not found in scope");
                return nullptr;
            }
            baseTypeName = (var->getType()->isPointer()) ? 
                var->getType()->getBasePointeeType()->getName() :
                var->getType()->description();
            object = nullptr;
        } else {
            baseExpr = object->express(scope);
            if (!baseExpr) {
                console.error("Failed to evaluate base expression for member access");
                return nullptr;
            }
            auto baseType = baseExpr->getType();
            baseTypeName = (baseType->isPointer()) ? 
                baseType->getBasePointeeType()->getName() :
                baseType->description();
        }
    } else {
        auto var = scope->get(objectName);
        if (!var) {
            console.error("Variable '" + objectName + "' not found in scope");
            return nullptr;
        }
        baseTypeName = (var->getType()->isPointer()) ? 
            var->getType()->getBasePointeeType()->getName() :
            var->getType()->description();
    }

    validateAccessiblity(baseTypeName, memberName, scope);

    auto userType = std::dynamic_pointer_cast<Omniscript::UserDefinedType>(scope->getType(baseTypeName));
    if (!userType) {
        if (auto type = scope->getType(baseTypeName)) {
            DEBUG_LOG(type->toString());
        } else {
            DEBUG_LOG("No type defined");
        }
        console.error("Type '" + baseTypeName + "' is not a user-defined type.");
        return nullptr;
    }

    // Find the member index
    int memberIndex = -1;
    for (int i = 0; i < userType->paramTypes.size(); ++i) {
        if (userType->paramTypes[i]->getParameterName() == memberName) {
            memberIndex = i;
            setType(userType->paramTypes[i]);
            break;
        }
    }

    if (memberIndex == -1) {
        console.error("Member '" + memberName + "' not found in user-defined type parameters");
        return nullptr;
    }

    std::vector<int> memberIndexPath = { memberIndex };

    // Evaluate assignment expression if present
    std::shared_ptr<Omniscript::Expression> assignmentExpr = nullptr;
    if (assignmentValue) {
        extendContextOf(assignmentValue);
        assignmentExpr = assignmentValue->express(scope);
        if (!assignmentExpr) {
            console.error("Failed to evaluate assignment expression");
            return nullptr;
        }
    }

    // Build final expression
    std::shared_ptr<Omniscript::Expression> baseVarExpr = baseExpr;
    if (!baseVarExpr) {
        auto var = scope->get(objectName);
        baseVarExpr = std::make_shared<Omniscript::VariableAccess>(objectName, var->getType());
    }

    auto result = std::make_shared<Omniscript::MemberAccessExpression>(
        baseVarExpr,
        baseTypeName,
        objectName,
        memberName,
        memberIndex,
        type,
        assignmentExpr
    );
    result->type = type;
    return result;
}

std::shared_ptr<Omniscript::Expression> ArrowAccess::express(SymbolTableType scope) {
    // Evaluate the pointer expression recursively
    auto pointerExpr = pointer->express(scope);
    if (!pointerExpr) {
        console.error("Failed to evaluate pointer expression for arrow access");
        return nullptr;
    }

    auto pointerType = pointerExpr->getType();
    if (!pointerType->isPointer()) {
        console.error("Arrow access requires a pointer type");
        return nullptr;
    }

    auto baseType = pointerType->getBasePointeeType();
    auto userType = std::dynamic_pointer_cast<Omniscript::UserDefinedType>(baseType);
    if (!userType) {
        console.error("Arrow access requires a pointer to user-defined type");
        return nullptr;
    }

    int memberIndex;
    std::shared_ptr<Omniscript::Type> currentType = baseType;

    // Traverse member path
    bool found = false;
    for (int i = 0; i < userType->paramTypes.size(); ++i) {
        if (userType->paramTypes[i]->getParameterName() == memberName) {
            memberIndex = i;
            currentType = userType->paramTypes[i];
            found = true;
            break;
        }
    }

    if (!found) {
        console.error("Member '" + memberName + "' not found in type '" + userType->description() + "'");
        return nullptr;
    }

    // Descend into nested type if needed
    userType = std::dynamic_pointer_cast<Omniscript::UserDefinedType>(pointerType->getBasePointeeType());

    if (!userType) {
        console.error("Cannot traverse non-user-defined member '" + pointerType->getBasePointeeType()->toString() + "'");
        return nullptr;
    }

    if (type) {
        if (Omniscript::isSameOrCastableTo(currentType, type)) {
            setType(type);
        } else {
            console.error("Cannot cast " + currentType->toString() + " to a " + type->toString() + "'.");
        }
    }

    DEBUG_LOG("Current type is '" + currentType ->toString() + "'.");

    // Handle assignment if present
    std::shared_ptr<Omniscript::Expression> assignmentExpr = nullptr;
    if (assignmentValue) {
        extendContextOf(assignmentValue);
        assignmentExpr = assignmentValue->express(scope);
        if (!assignmentExpr) {
            console.error("Failed to evaluate assignment expression");
            return nullptr;
        }
    }

    auto result = std::make_shared<Omniscript::ArrowAccessExpression>(
        pointerExpr,
        memberName,
        memberIndex
    );

    result->type = type;
    return result;
}

std::shared_ptr<Omniscript::Expression> Dereference::express(SymbolTableType scope) {
    // Evaluate the pointer expression recursively
    auto pointerExpr = pointer->express(scope);
    if (!pointerExpr) {
        console.error("Failed to evaluate pointer expression for dereference");
        return nullptr;
    }

    auto pointerType = pointerExpr->getType();
    if (!pointerType->isPointer()) {
        console.error("Dereference requires a pointer type");
        return nullptr;
    }

    auto baseType = pointerType->getBasePointeeType();
    setType(baseType);

    // Handle member access if present
    auto userType = std::dynamic_pointer_cast<Omniscript::UserDefinedType>(baseType);
    if (!userType) {
        console.error("Cannot access members of non-user-defined type");
        return nullptr;
    }

    int memberIndex;
    std::shared_ptr<Omniscript::Type> currentType = baseType;

    // Traverse member path
    bool found = false;
    for (int i = 0; i < userType->paramTypes.size(); ++i) {
        if (userType->paramTypes[i]->getParameterName() == memberName) {
            memberIndex = i;
            currentType = userType->paramTypes[i];
            found = true;
            break;
        }
    }

    if (!found) {
        console.error("Member '" + memberName + "' not found in type '" + userType->description() + "'");
        return nullptr;
    }

    // Descend into nested type if needed
    userType = std::dynamic_pointer_cast<Omniscript::UserDefinedType>(
        currentType->isPointer() ? currentType->getBasePointeeType() : currentType
    );

    if (!userType) {
        console.error("Cannot traverse non-user-defined member '" + memberName + "'");
        return nullptr;
    }

    setType(currentType);

    // Handle assignment if present
    std::shared_ptr<Omniscript::Expression> valueExpr = nullptr;
    if (assignmentValue) {
        extendContextOf(assignmentValue);
        valueExpr = assignmentValue->express(scope);
        if (!valueExpr) {
            console.error("Failed to evaluate assignment expression");
            return nullptr;
        }
    }

    auto result = std::make_shared<Omniscript::DereferenceExpression>(
        pointerExpr,
        valueExpr,
        getType()
    );

    result->type = type;
    return result;
}

std::shared_ptr<Omniscript::Expression> IndexAccess::express(SymbolTableType scope) {
    // Evaluate the container expression recursively
    auto containerExpr = expr->express(scope);
    if (!containerExpr) {
        console.error("Failed to evaluate container expression for index access");
        return nullptr;
    }

    // Evaluate the index expression recursively
    auto indexExprValue = index->express(scope);
    if (!indexExprValue) {
        console.error("Failed to evaluate index expression");
        return nullptr;
    }

    auto containerType = containerExpr->getType();
    if (!containerType->isArray() && !containerType->isPointer()) {
        console.error("Index access requires an array or pointer type");
        return nullptr;
    }

    // Set the result type (for pointer it's the pointee type, for array it's the element type)
    auto resultType = containerType->isPointer() 
        ? containerType->getBasePointeeType() 
        : containerType->getElementType();
    setType(resultType);

    // Handle assignment if present
    std::shared_ptr<Omniscript::Expression> assignmentExpr = nullptr;
    if (assignmentValue) {
        extendContextOf(assignmentValue);
        assignmentExpr = assignmentValue->express(scope);
        if (!assignmentExpr) {
            console.error("Failed to evaluate assignment expression");
            return nullptr;
        }
    }

    auto result = std::make_shared<Omniscript::IndexAccessExpression>(
        containerExpr,
        indexExprValue
    );

    result->type = type;
    return result;
}

