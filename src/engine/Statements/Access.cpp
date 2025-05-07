#include <omniscript/Core.h>
#include <omniscript/utils.h>
#include <omniscript/engine/Statement.h>
#include <omniscript/engine/Symboltable.h>
#include <omniscript/omniscript_pch.h>
#include <omniscript/utils.h>

// ============================== Accesses  ============================== //

std::shared_ptr<Omniscript::Expression> MemberAccess::express(SymbolTableType scope) {
    auto named = std::dynamic_pointer_cast<NamedStatement>(expr);

    if (!named) {
        console.error("The object holding the member being accessed should be named");
        return nullptr;
    }

    objectName = named->getName();
    DEBUG_LOG("The object name is '" + objectName + "' of type " + scope->get(objectName)->getType()->getName());

    std::string baseTypeName = (scope->get(objectName)->getType()->isPointer()) ? 
                                scope->get(objectName)->getType()->getBasePointeeType()->getName() :
                                scope->get(objectName)->getType()->kindName();
    std::shared_ptr<Omniscript::Type> baseType = scope->getType(baseTypeName);

    if (!baseType) {
        console.error("Could not find base type '" + baseTypeName + "'.");
        return nullptr;
    }

    std::shared_ptr<Omniscript::Type> currentType = baseType;
    auto userType = std::dynamic_pointer_cast<Omniscript::UserDefinedType>(currentType);

    if (!userType) {
        console.error("Type '" + currentType->kindName() + "' is not a struct or does not have members.");
        return nullptr;
    }

    bool found = false;
    for (const auto& field : userType->paramTypes) {
        DEBUG_LOG("Field parameter name is " + field->getParameterName());
        if (field->getParameterName() == member) {
            currentType = field;
            found = true;
            break;
        }
    }

    if (!found) {
        console.error("Member '" + member + "' not found in type '" + userType->kindName() + "'.");
        return nullptr;
    }

    setType(currentType);

    if (auto typed = std::dynamic_pointer_cast<TypedStatement>(assignmentValue)) {
        if (!typed->getType()) {
            typed->setType(currentType);
        }
    }

    DEBUG_LOG("The member being accessed is '" + expr->toString() + "'.");
    auto obj = expr->express(scope);

    if (assignmentValue) {
        DEBUG_LOG("Setting '" + this->toString() + "' to '" + assignmentValue->toString() + "'.");
        auto result = assignmentValue->express(scope);

        if (auto instance = std::dynamic_pointer_cast<Omniscript::InstanceExpression>(scope->get(objectName))) {
            auto parentInstance = instance;
            auto subInstance = parentInstance->getField(member);

            if (!subInstance) {
                console.error("Member '" + member + "' not found on instance of '" + parentInstance->getType()->kindName() + "'.");
                return nullptr;
            }

            auto currentInstance = std::dynamic_pointer_cast<Omniscript::InstanceExpression>(subInstance);

            parentInstance->setField(member, result);

            // ✅ Always return the working constructor
            return std::make_shared<Omniscript::MemberAccessExpression>(baseTypeName, objectName, obj, member, currentType, result);

        } else if (auto typeExpr = scope->get(objectName)) {
            auto type = typeExpr->getType();
            if (auto userDefined = std::dynamic_pointer_cast<Omniscript::UserDefinedType>(type->isPointer() ? type->getBasePointeeType() : type)) {
                std::shared_ptr<Omniscript::Type> currentType = userDefined;
                std::string lastMember;
                bool foundAll = true;

                for (const auto& member : propertyPath) {
                    lastMember = member;
                    bool found = false;

                    for (const auto& field : userDefined->paramTypes) {
                        if (field->getParameterName() == member) {
                            currentType = field;
                            found = true;
                            break;
                        }
                    }

                    if (!found) {
                        console.error("Member '" + member + "' not found in type '" + userDefined->kindName() + "'.");
                        foundAll = false;
                        break;
                    }

                    userDefined = std::dynamic_pointer_cast<Omniscript::UserDefinedType>(currentType->isPointer() ? currentType->getBasePointeeType() : currentType);
                    if (!userDefined && member != propertyPath.back()) {
                        console.error("Cannot traverse into non-user-defined member '" + member + "'.");
                        foundAll = false;
                        break;
                    }
                }

                if (foundAll) {
                    // ✅ Use the same working constructor
                    return std::make_shared<Omniscript::MemberAccessExpression>(
                        baseTypeName,
                        objectName,
                        obj,
                        member,
                        currentType,
                        result
                    );
                }

                return nullptr;
            }

            console.error("Type of '" + objectName + "' is not a user-defined type.");
            return nullptr;
        }

        console.error("Object '" + objectName + "' is not an instance.");
        return nullptr;

    } else {
        DEBUG_LOG("Getting '" + this->toString() + "'.");
        // ✅ Use consistent constructor (getter form)
        return std::make_shared<Omniscript::MemberAccessExpression>(
            baseTypeName,
            objectName,
            obj,
            member,
            currentType
        );
    }
}

std::shared_ptr<Omniscript::Expression> ArrowAccess::express(SymbolTableType scope) {
    return nullptr;
}

std::shared_ptr<Omniscript::Expression> IndexAccess::express(SymbolTableType scope) {
    return nullptr;
}
