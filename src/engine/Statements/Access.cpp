#include <omniscript/Core.h>
#include <omniscript/utils.h>
#include <omniscript/engine/Statement.h>
#include <omniscript/engine/Symboltable.h>
#include <omniscript/omniscript_pch.h>
#include <omniscript/utils.h>

// ============================== Accesses  ============================== //

std::shared_ptr<Omniscript::Expression> MemberAccess::express(SymbolTableType scope) {
    std::vector<int> memberIndexPath;
    
    // Evaluate the base expression recursively
    std::shared_ptr<Omniscript::Expression> baseExpr = nullptr;
    std::string baseTypeName;
    
    if (object) {
        if (auto getter = std::dynamic_pointer_cast<GetVariable>(object)) {
            objectName = getter->getName();

             // Handle direct variable access case
            auto var = scope->get(objectName);
            if (!var) {
                console.error("Variable '" + objectName + "' not found in scope");
                return nullptr;
            }
            baseTypeName = (var->getType()->isPointer()) ? 
                        var->getType()->getBasePointeeType()->getName() :
                        var->getType()->description();
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
        // Handle direct variable access case
        auto var = scope->get(objectName);
        if (!var) {
            console.error("Variable '" + objectName + "' not found in scope");
            return nullptr;
        }
        baseTypeName = (var->getType()->isPointer()) ? 
                      var->getType()->getBasePointeeType()->getName() :
                      var->getType()->description();
    }

    DEBUG_LOG("Base type name is '" + baseTypeName + "'.");
    std::shared_ptr<Omniscript::Type> currentType = scope->getType(baseTypeName);
    
    if (!currentType) {
        console.error("Could not find base type '" + baseTypeName + "'.");
        return nullptr;
    }

    auto userType = std::dynamic_pointer_cast<Omniscript::UserDefinedType>(currentType);
    if (!userType) {
        console.error("Type '" + baseTypeName + "' is not a user-defined type.");
        return nullptr;
    }

    // Traverse property path and build index path
    for (const std::string& memberName : memberPath) {
        bool found = false;
        for (int i = 0; i < userType->paramTypes.size(); ++i) {
            if (userType->paramTypes[i]->getParameterName() == memberName) {
                memberIndexPath.push_back(i);
                currentType = userType->paramTypes[i];
                found = true;
                break;
            }
        }

        if (!found) {
            console.error("Member '" + memberName + "' not found in type '" + userType->description() + "'.");
            return nullptr;
        }

        // Descend into nested type
        userType = std::dynamic_pointer_cast<Omniscript::UserDefinedType>(
            currentType->isPointer() ? currentType->getBasePointeeType() : currentType
        );

        if (!userType && memberName != memberPath.back()) {
            console.error("Cannot traverse non-user-defined member '" + memberName + "'.");
            return nullptr;
        }
    }

    setType(currentType);

    // Handle assignment if present
    std::shared_ptr<Omniscript::Expression> assignmentExpr = nullptr;
    if (assignmentValue) {
        assignmentExpr = assignmentValue->express(scope);
        if (!assignmentExpr) {
            console.error("Failed to evaluate assignment expression");
            return nullptr;
        }
    }

    // If we have a base expression (from nested access), use it
    if (baseExpr) {
        auto result = std::make_shared<Omniscript::MemberAccessExpression>(
            baseExpr,
            baseTypeName,
            objectName,
            memberPath,
            memberIndexPath,
            currentType,
            assignmentExpr
        );

        result->type = type;
        return result;
    }
    
    // Otherwise create a variable reference as the base
    auto varExpr = std::make_shared<Omniscript::VariableAccess>(objectName, scope->get(objectName)->getType());
    auto result = std::make_shared<Omniscript::MemberAccessExpression>(
        varExpr,
        baseTypeName,
        objectName,
        memberPath,
        memberIndexPath,
        currentType,
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

    std::vector<int> memberIndexPath;
    std::shared_ptr<Omniscript::Type> currentType = baseType;

    // Traverse member path
    for (const std::string& memberName : memberPath) {
        bool found = false;
        for (int i = 0; i < userType->paramTypes.size(); ++i) {
            if (userType->paramTypes[i]->getParameterName() == memberName) {
                memberIndexPath.push_back(i);
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
        if (memberName != memberPath.back()) {
            userType = std::dynamic_pointer_cast<Omniscript::UserDefinedType>(
                currentType->isPointer() ? currentType->getBasePointeeType() : currentType
            );
            if (!userType) {
                console.error("Cannot traverse non-user-defined member '" + memberName + "'");
                return nullptr;
            }
        }
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
        assignmentExpr = assignmentValue->express(scope);
        if (!assignmentExpr) {
            console.error("Failed to evaluate assignment expression");
            return nullptr;
        }
    }

    auto result = std::make_shared<Omniscript::ArrowAccessExpression>(
        pointerExpr,
        memberPath,
        memberIndexPath
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
    if (!memberPath.empty()) {
        auto userType = std::dynamic_pointer_cast<Omniscript::UserDefinedType>(baseType);
        if (!userType) {
            console.error("Cannot access members of non-user-defined type");
            return nullptr;
        }

        std::vector<int> memberIndexPath;
        std::shared_ptr<Omniscript::Type> currentType = baseType;

        // Traverse member path
        for (const std::string& memberName : memberPath) {
            bool found = false;
            for (int i = 0; i < userType->paramTypes.size(); ++i) {
                if (userType->paramTypes[i]->getParameterName() == memberName) {
                    memberIndexPath.push_back(i);
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
            if (memberName != memberPath.back()) {
                userType = std::dynamic_pointer_cast<Omniscript::UserDefinedType>(
                    currentType->isPointer() ? currentType->getBasePointeeType() : currentType
                );
                if (!userType) {
                    console.error("Cannot traverse non-user-defined member '" + memberName + "'");
                    return nullptr;
                }
            }
        }

        setType(currentType);
    }

    // Handle assignment if present
    std::shared_ptr<Omniscript::Expression> valueExpr = nullptr;
    if (assignmentValue) {
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

