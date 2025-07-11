#include <omniscript/Statement.h>
#include <omniscript/Statements/AccessStatements.h>
#include <omniscript/Statements/LiteralStatements.h>
#include <omniscript/Statements/FunctionStatement.h>
#include <omniscript/Statements/AssignmentAndGetterStatements.h>

#include <omniscript/Expressions/FunctionExpression.h>
#include <omniscript/Expressions/FunctionExpression.h>
#include <omniscript/Expressions/AssignmentExpression.h>
#include <omniscript/Expressions/AssignmentExpression.h>
#include <omniscript/Expressions/VariableAccessExpression.h>


std::shared_ptr<Omniscript::Expression> AddressOf::express(SymbolTableType scope) {
    Omniscript::setPosition(pos.line, pos.col, pos.filePath);
    std::shared_ptr<Omniscript::Expression> referent = scope->getValue(name);

    DEBUG_LOG("Getting the address of '" + name + "'.");
    
    if (!scope->exists(name)) {
        console.error("Symbol '" + name + "' was not found in the scope");
    }
    
    if (!referent) {
        auto overloads = scope->getOverloads(name);
        referent = overloads[0];
        auto mangledName = std::dynamic_pointer_cast<Omniscript::FunctionExpression>(referent)->mangledName;
        DEBUG_LOG("Mangled name '" + mangledName + "'.");

        // if (!Omniscript::Type::isSameOrCastableTo(type))
        if (!type) {
            setType(Omniscript::Type::createPointerType(referent->getType()));
            setRootType(Omniscript::Type::createPointerType(referent->getType()));
        }

        auto addrOf = std::make_shared<Omniscript::AddressOfExpression>(mangledName, referent);
        
    }

    if (!type) {
        setType(Omniscript::Type::createPointerType(referent->getType()));
        setRootType(Omniscript::Type::createPointerType(referent->getType()));
    }

    auto addrOf = std::make_shared<Omniscript::AddressOfExpression>(name, referent);
    addrOf->setPosition(getPosition());
    return addrOf;
}

std::shared_ptr<Omniscript::Expression> ReferenceTo::express(SymbolTableType scope) {
    Omniscript::setPosition(pos.line, pos.col, pos.filePath);
    DEBUG_LOG("Getting a reference to '" + name + "'.");
    // Look up the value in the scope to get the variable
    if (!scope->exists(name)) {
        console.error("Symbol '" + name + "' was not found in the scope");
    }

    auto variable = scope->getValue(name);
    if (variable) {
        setType(Omniscript::Type::createPointerType(variable->getType()));
        setRootType(type);
        auto ref = std::make_shared<Omniscript::ReferenceExpression>(name, variable);
        ref->setPosition(getPosition());
        return ref;
    }

    if (!variable) {
        auto overloads = scope->getOverloads(name);
        auto mangledName = std::dynamic_pointer_cast<Omniscript::FunctionExpression>(overloads[0])->mangledName;
        setType(Omniscript::Type::createPointerType((variable->getType())));
        setRootType(type);
        auto ref = std::make_shared<Omniscript::ReferenceExpression>(mangledName, variable);
        ref->setPosition(getPosition());
        return ref;
    }
    
    // If the variable isn't found, handle the error (e.g., return nullptr)
    console.error("Error: Variable " + name + " not found.\n");
    return nullptr;
}

// Get Variable
std::shared_ptr<Omniscript::Expression> GetVariable::express(SymbolTableType scope) {
    Omniscript::setPosition(pos.line, pos.col, pos.filePath);
    DEBUG_LOG();
    DEBUG_LOG("Getting symbol '" + name + "'.");
    DEBUG_LOG(getContextAsString());

    std::shared_ptr<Omniscript::Expression> expr = scope->get(name);
    std::string resolvedName = name;

    if (!expr) {
        std::string qualifiedName;
        for (size_t i = 0; i < accessContext.size(); ++i) {
            if (!qualifiedName.empty()) qualifiedName += ".";
            qualifiedName += accessContext[i];

            std::string fullCalleeName = qualifiedName + "." + name;
            expr = scope->get(fullCalleeName);
            DEBUG_LOG("[Call] Attempting overload resolution for '" + fullCalleeName + "'");
            if (expr) {
                DEBUG_LOG("Found overloads for: " + fullCalleeName + "\n");
                resolvedName = fullCalleeName;
                break;
            }
        }
    }

    if (!expr) {
        // Todo: work with function pointers
        auto overloads = scope->getOverloads(name);
        if (!overloads.empty()) {
            expr = overloads[0];
            auto func = std::dynamic_pointer_cast<Omniscript::FunctionExpression>(expr);
            resolvedName = func->mangledName;
        }
    }

    if (!expr) {
        console.error("Symbol '" + name + "' could not be resolved in scope '" + scope->getName() + "'.");
        return nullptr;
    }

    DEBUG_LOG("The variable stored in scope is '" + expr->toString() + "'.");

    std::shared_ptr<Omniscript::Type> symbolType = expr->getType();

    if (type) {
        if (!Omniscript::Type::isSameOrCastableTo(symbolType, type)) {
            if (symbolType->isNullable()) {
                if (auto varAccess = std::dynamic_pointer_cast<Omniscript::VariableAccessExpression>(expr)) {
                    if (!varAccess->extractValue) {
                        console.error("Symbol '" + resolvedName + "' is of type '" + symbolType->toString() +
                                      "', which cannot be casted to '" + type->toString() + "'.");
                    }
                }
            }
        } else {
            if (type->isNullable() && !symbolType->isNullable()) {
                // Wrap the non-nullable expression in a NullableExpression
                auto val = std::make_shared<Omniscript::VariableAccessExpression>(resolvedName, symbolType);
                auto result = std::make_shared<Omniscript::NullableExpression>(val);
                // By default, nullCaseHandled == false here, which is correct
                result->setPosition(getPosition());
                return result;
            }
            DEBUG_LOG("The casted type is '" + type->toString() + "'.");
        }
    } else {
        DEBUG_LOG("The inferred type is '" + symbolType->toString() + "'.");
        setType(symbolType);
    }

    auto varAccess = std::make_shared<Omniscript::VariableAccessExpression>(resolvedName, type);
    varAccess->value = expr;
    // varAccess->isVolatileAccess;
    varAccess->setPosition(getPosition());
    return varAccess;
}

std::shared_ptr<Omniscript::Expression> AssignVariable::express(SymbolTableType scope) {
    Omniscript::setPosition(pos.line, pos.col, pos.filePath);
    if (type && type->isUnresolved()) {
        if (auto unresolved = std::dynamic_pointer_cast<Omniscript::UnresolvedType>(type)) {
            type = scope->getType(unresolved->joinedTypeString);
            rootType = type;
            if (!type) {
                console.error("Type '" + unresolved->joinedTypeString + "' does not exist in scope '" + scope->getName() + "'.");
            }
        }
    }

    DEBUG_LOG("Assigning variable " + variable + (type? " of type " + type->toString() : ""));

    std::shared_ptr<Omniscript::Expression> result;

    if (isReassign) {
        if (!scope->exists(variable)) {
            console.log("Variable '" + variable + "' was not declared in scope '" + scope->getName() + "'.");
        }
    }

    if (value) {
        extendContextOf(value);
    }

    if (type) {
        if (type->isGeneric()) {
            auto genericVal = scope->get(type->getName());
            if (auto generic = std::dynamic_pointer_cast<Omniscript::TypeExpression>(genericVal)) {
                DEBUG_LOG("The generic type is " + generic->getTypeExpression()->toString());
                type = generic->getTypeExpression()->clone();
            }
        }

        if (type->isFunction()) {
            if (auto func = std::dynamic_pointer_cast<FunctionDeclaration>(value)) {
                func->setName(name);
            }
        }
        
        if (!type->isPointer() && !type->isReference()) {
            if (!value || std::dynamic_pointer_cast<Null>(value)) {
                result = std::make_shared<Omniscript::NullExpression>(type);
            } else if (auto typed = std::dynamic_pointer_cast<TypedStatement>(value)) {
                if (!typed->getType() && !type->isInvalid()) {
                    typed->setType(type);
                    result = value->express(scope);
                    if (!Omniscript::Type::isSameOrCastableTo(result->type, type)) {
                        console.error("Type mismatch when assigning to variable '" + name + "'.\n"
                            "Expected type: '" + type->toString() + "', but got: '" + result->type->toString() + "'.\n"
                            "The type was inferred, but the assigned value is not compatible.");
                    }
                    DEBUG_LOG("The variable's inferred type is '" + type->toString() + "'.");
                } else {
                    result = value->express(scope);
                    if (!Omniscript::Type::isSameOrCastableTo(result->type, type)) {
                        console.error("Type mismatch for variable '" + name + "'.\n"
                            "Expected type: '" + type->toString() + "' (or 'null'), but got: '" + result->getType()->toString() + "'.");
                    }
                }
            }
        } else {
            if (type->isPointer()) {
                if (!value) {
                    result = std::make_shared<Omniscript::NullPointerExpression>(type);
                }  else if (auto typed = std::dynamic_pointer_cast<TypedStatement>(value)) {
                    if (!type->getPointeeType()) {
                        console.error("Variable '" + name + "' is not a pointer");
                    }
                    if (!typed->getType()) {
                        typed->setType(type);
                    }
                    result = typed->express(scope);
                    auto resultType = result->getType();
                    if (!Omniscript::Type::isSameOrCastableTo(resultType, type)) {
                        console.error("The rvalue of '" + variable + "' is not a '" + type->toString() + "', it is a '" + resultType->toString() + "'.");
                    }
                } else {
                    console.error("Pointer '" + variable + "' can only be created from an integer, a reference to an already existing variable, nullptr or a string from a (char*).");
                }
            } else if (type->isReference()) {
                if (auto referenceTo = std::dynamic_pointer_cast<ReferenceTo>(value)) {
                    auto ptr = scope->getPointerToValue(referenceTo->getName());
                    if (!ptr || !*ptr) {
                        DEBUG_LOG("HERE 2.1");
                        console.error("Cannot create reference to undefined variable '" + referenceTo->getName() + "'.");
                    } else {
                        DEBUG_LOG("HERE 2.2");
                        // Get the ultimate base types for comparison
                        auto expectedBaseType = type->getBaseReferencedType();
                        DEBUG_LOG("HERE 2.3");
                        auto actualBaseType = (*ptr)->getType()->getBaseReferencedType();

                        if (!actualBaseType) {
                            actualBaseType = (*ptr)->getType();
                        }

                        DEBUG_LOG("HERE 2.4");
                        
                        DEBUG_LOG(expectedBaseType->toString() + " " + actualBaseType->toString());
                        if (expectedBaseType->getKind() != actualBaseType->getKind()) {
                            DEBUG_LOG("HERE 2.4.1");
                            console.error("Reference '" + variable + "' expects base type '" +
                                expectedBaseType->toString() + "' but got '" +
                                actualBaseType->toString() + "' instead.");
                        }
                        DEBUG_LOG("HERE 2.5");
                        // Check reference depth matches
                        int expectedDepth = type->getReferenceDepth() - 1;
                        DEBUG_LOG("HERE 2.6");
                        int actualDepth = (*ptr)->getType()->getReferenceDepth();
                        DEBUG_LOG("HERE 2.7");
                        
                        if (expectedDepth != actualDepth) {
                            DEBUG_LOG("HERE 2.7.1");
                            console.error("Reference '" + variable + "' expects " + 
                                std::to_string(expectedDepth) + " level(s) of reference but got " +
                                std::to_string(actualDepth) + " level(s) instead.");
                        }
                        DEBUG_LOG("HERE 2.8");
                        result = Omniscript::make_expression<Omniscript::ReferenceExpression>(referenceTo->getName(), ptr);
                        DEBUG_LOG("HERE 2.9");
                    }
                    DEBUG_LOG("HERE 3");
                } else if (auto addressOf = std::dynamic_pointer_cast<AddressOf>(value)) {
                    console.error("Cannot create reference from address-of expression for '" + variable + "'.");
                } else if (auto nullpointer = std::dynamic_pointer_cast<Nullptr>(value)) {
                    console.error("Cannot create reference from nullptr for '" + variable + "'.");
                } else {
                    console.error("Cannot bind reference '" + variable + "' to a non-variable.");
                }
                DEBUG_LOG("HERE 4");
            } 
        }
        DEBUG_LOG("HEREEE");
    } else {
        if (auto typed = std::dynamic_pointer_cast<TypedStatement>(value)) {
            if (!typed->getType()) {
                result = value->express(scope);
                type = typed->getType();
            } else {
                type = typed->getType();
                result = value->express(scope);
            }
            DEBUG_LOG("The infered type is " + result->getType()->toString());
        }
    }

    if (type) {
        if (type->isUnresolved()) {
            if (auto unresolved = std::dynamic_pointer_cast<Omniscript::UnresolvedType>(type)) {
                type = scope->getType(unresolved->joinedTypeString);
                rootType = type;
                result->type = type;
                result->rootType = type;
                if (!type) {
                    console.error("Type '" + unresolved->joinedTypeString + "' does not exist in scope '" + scope->getName() + "'.");
                }
            }
        }
        if (!result) {
            console.error("Variable '" + variable + " has an invalid r value");
        } 
        if (!result->getType()) {
            console.error("The r-value of variable '" + variable + "' is not valid");
        }     
        DEBUG_LOG(
                    "The result is " + variable + " " + result->getType()->toString() + " = " + result->toString()
                );
    } else {
        DEBUG_LOG("No type was deduced for variable '" + variable + "'.\n It had a value and no type or multiple types. Returning its result.");
        result->setPosition(getPosition());
        return result;
    }

    if (isReassign) {
        std::shared_ptr<Omniscript::Expression> prevValue = scope->get(variable);
        if (!Omniscript::Type::isSameOrCastableTo(result->getType(), prevValue->getType())) {
            console.error("'" + variable + "' should be of type '" + prevValue->getType()->toString() + "' not a '" + result->getType()->toString() + "'.");
        }
    }

    if (isConstant) {
        scope->setConstant(variable, result);
    } else {
        scope->setVariable(variable, result);
    }

    auto assignment = Omniscript::make_expression<Omniscript::VariableAssignment>(variable, result, isGlobal, true);
    assignment->isStatic = isStatic;
    assignment->isGlobal = isGlobal;
    assignment->isConstant = isConstant;
    assignment->isVolatile = isVolatile;
    assignment->setPosition(getPosition());
    assignment->isExtern = isExtern;
    assignment->externName = assignment->getName();
    assignment->windowsDynamic  = libraryPaths.windowsDynamic;   // e.g., "lib/foo.dll"
    assignment->windowsStatic   = libraryPaths.windowsStatic;    // e.g., "lib/foo.lib"
    assignment->linuxShared     = libraryPaths.linuxShared;      // e.g., "libfoo.so"
    assignment->linuxStatic     = libraryPaths.linuxStatic;      // e.g., "libfoo.a"
    assignment->macosShared     = libraryPaths.macosShared;      // e.g., "libfoo.dylib"
    assignment->macosStatic     = libraryPaths.macosStatic;      // e.g., "libfoo.a"
    assignment->genericDynamic  = libraryPaths.genericDynamic;   // fallback .so/.dll/.dylib
    assignment->genericStatic   = libraryPaths.genericStatic;    // fallback .a/.lib

    return assignment;
}

