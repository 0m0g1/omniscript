#include <omniscript/Core.h>
#include <omniscript/utils.h>
#include <omniscript/engine/Statement.h>
#include <omniscript/engine/Symboltable.h>
#include <omniscript/omniscript_pch.h>
#include <omniscript/utils.h>

// ============================== Accesses  ============================== //


void ContextAwareStatement::validateAccessiblity(std::string baseTypeName, std::string memberName, SymbolTableType scope) {
    // Ensure base type is a class
    auto aggregateExpr = std::dynamic_pointer_cast<Omniscript::AggregateExpression>(scope->get(baseTypeName));
    if (aggregateExpr) {
        auto structExpr = std::dynamic_pointer_cast<Omniscript::StructExpression>(scope->get(baseTypeName));
        auto classExpr = std::dynamic_pointer_cast<Omniscript::ClassExpression>(scope->get(baseTypeName));
        auto moduleExpr = std::dynamic_pointer_cast<Omniscript::ModuleExpression>(scope->get(baseTypeName));
        
        if (auto type = scope->get(baseTypeName)) {
            DEBUG_LOG("Accessing a member of type '" + type->toString() + "'.");
        } else {
            DEBUG_LOG("No type defined");
        }

        std::shared_ptr<Omniscript::MemberExpression> member;

        DEBUG_LOG(getContextAsString());
        if (structExpr) {
            // member = structExpr->getMember(memberName);
        } else if (classExpr) {
            member = classExpr->getMember(memberName);
        } else if (moduleExpr) {
            member = moduleExpr->getMember(memberName);
        } else {
            console.error("Type '" + baseTypeName + "' is not an aggregate Type (class, struct, module).");
            return;
        }

        if (!member) {
            console.error("Member '" + memberName + "' not found in type '" + baseTypeName + "'.");
            return;
        }
    
        
        if (!structExpr && !member->isPublic() && member->isPrivate() && !containsContext(classExpr->getName())) {
            if (classExpr) {
                console.error("Cannot access private member '" + memberName + "' of class '" + classExpr->getName() + "'.");
            } else if (moduleExpr) {
                console.error("Cannot access private member '" + memberName + "' of module '" + moduleExpr->getName() + "'.");
            } 
            return;
        }
    }
}

void Access::verifyMemberAccessibility() {

}

std::shared_ptr<Omniscript::Expression> MemberAccess::express(SymbolTableType scope) {
    Omniscript::setPosition(pos.line, pos.col, pos.filePath);
    std::shared_ptr<Omniscript::Expression> baseExpr = nullptr;
    std::string baseTypeName;
    std::string resolvedObjectName = objectName;

    std::string prefix = accessContext[0];
    for (size_t i = 0; i < accessContext.size() - 1; ++i) {
        std::string baseName = accessContext[i];
        std::string memberName = accessContext[i + 1];

        DEBUG_LOG("Validating access for '" + memberName + "' in '" + baseName + "'.");
        validateAccessiblity(baseName, memberName, scope);
        prefix += "." + memberName;
    }

    // Evaluate base expression or get type from variable
    DEBUG_LOG("The object is " + object->toString());
    if (object) {
        if (auto getter = std::dynamic_pointer_cast<GetVariable>(object)) {
            objectName = getter->getName();
            std::shared_ptr<Omniscript::Expression> expr = scope->get(objectName);
            resolvedObjectName = objectName;

            if (!expr) {
                std::string qualifiedName;
                for (size_t i = 0; i < accessContext.size(); ++i) {
                    if (!qualifiedName.empty()) qualifiedName += ".";
                    qualifiedName += accessContext[i];

                    std::string fullName = qualifiedName + "." + objectName;
                    expr = scope->get(fullName);
                    DEBUG_LOG("[MemberAccess] Trying contextual name: " + fullName);
                    if (expr) {
                        resolvedObjectName = fullName;
                        break;
                    }
                }
            }

            if (!expr) {
                console.error("Variable '" + objectName + "' not found in current or contextual scope");
                return nullptr;
            }

            auto type = expr->getType();
            baseTypeName = (type->isPointer()) ? 
                type->getBasePointeeType()->getName() :
                type->toString();
            object = nullptr;

        } else if (auto memberAcc = std::dynamic_pointer_cast<MemberAccess>(object)) {
            std::shared_ptr<Omniscript::Expression> expr; 
            resolvedObjectName = objectName;
            std::string qualifiedName;
            for (size_t i = 0; i < accessContext.size(); ++i) {
                if (!qualifiedName.empty()) qualifiedName += ".";
                qualifiedName += accessContext[i];

                std::string fullName = qualifiedName + "." + memberName;
                DEBUG_LOG("[MemberAccess] Trying contextual name: " + fullName);
            }

            qualifiedName += "." + memberName;
            
            expr = scope->get(qualifiedName);

            if (expr) {
                type = expr->getType();
                DEBUG_LOG("'" + qualifiedName + "' has type " + type->toString() + "'.");
                
                if (!assignmentValue) {
                    return std::make_shared<Omniscript::VariableAccess>(qualifiedName, type);
                }
    
                std::shared_ptr<Omniscript::Expression> assignmentExpr = nullptr;
                extendContextOf(assignmentValue);
                assignmentExpr = assignmentValue->express(scope);
                if (!assignmentExpr) {
                    console.error("Failed to evaluate assignment expression");
                    return nullptr;
                }
                return std::make_shared<Omniscript::VariableAssignment>(qualifiedName, assignmentExpr);
            }
            
        } else {
            baseExpr = object->express(scope);
            if (!baseExpr) {
                console.error("Failed to evaluate base expression for member access");
                return nullptr;
            }
            auto baseType = baseExpr->getType();
            baseTypeName = (baseType->isPointer()) ?
                baseType->getBasePointeeType()->getName() :
                baseType->toString();
        }
    } else {
        std::shared_ptr<Omniscript::Expression> expr = scope->get(objectName);
        resolvedObjectName = objectName;

        if (!expr) {
            std::string qualifiedName;
            for (size_t i = 0; i < accessContext.size(); ++i) {
                if (!qualifiedName.empty()) qualifiedName += ".";
                qualifiedName += accessContext[i];

                std::string fullName = qualifiedName + "." + objectName;
                expr = scope->get(fullName);
                DEBUG_LOG("[MemberAccess] Trying contextual name: " + fullName);
                if (expr) {
                    resolvedObjectName = fullName;
                    break;
                }
            }
        }

        if (!expr) {
            console.error("Variable '" + objectName + "' not found in current or contextual scope");
            return nullptr;
        }

        auto type = expr->getType();
        baseTypeName = (type->isPointer()) ?
            type->getBasePointeeType()->getName() :
            type->toString();
    }

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
        auto var = scope->get(resolvedObjectName);
        baseVarExpr = std::make_shared<Omniscript::VariableAccess>(resolvedObjectName, var->getType());
    }

    auto result = std::make_shared<Omniscript::MemberAccessExpression>(
        baseVarExpr,
        baseTypeName,
        resolvedObjectName,
        memberName,
        memberIndex,
        type,
        assignmentExpr
    );
    result->type = type;
    return result;
}

std::shared_ptr<Omniscript::Expression> ArrowAccess::express(SymbolTableType scope) {
    Omniscript::setPosition(pos.line, pos.col, pos.filePath);
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
        console.error("Member '" + memberName + "' not found in type '" + userType->toString() + "'");
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
    Omniscript::setPosition(pos.line, pos.col, pos.filePath);
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
        console.error("Member '" + memberName + "' not found in type '" + userType->toString() + "'");
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
    Omniscript::setPosition(pos.line, pos.col, pos.filePath);
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
        console.error("Index access requires an array or pointer type not '" + containerType->toString() + "'.");
        return nullptr;
    }

    // Set the result type (for pointer it's the pointee type, for array it's the element type)
    auto resultType = containerType->isPointer() 
        ? containerType->getBasePointeeType() 
        : containerType->elementType;
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

