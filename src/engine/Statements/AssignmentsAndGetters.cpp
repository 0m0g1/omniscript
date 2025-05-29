#include <omniscript/Core.h>
#include <omniscript/utils.h>
#include <omniscript/engine/Statement.h>
#include <omniscript/engine/Symboltable.h>
#include <omniscript/omniscript_pch.h>
#include <omniscript/utils.h>


// ======================= Assignments and Variable Getters ======================= //
// ============================== Getters  ============================== //
std::shared_ptr<Omniscript::Expression> AddressOf::express(SymbolTableType scope) {
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

        // if (!Omniscript::isSameOrCastableTo(type))
        if (!type) {
            setType(Omniscript::Type::createPointerType(referent->getType()));
            setRootType(Omniscript::Type::createPointerType(referent->getType()));
        }

        return std::make_shared<Omniscript::AddressOfExpression>(mangledName, referent);
    }

    if (!type) {
        setType(Omniscript::Type::createPointerType(referent->getType()));
        setRootType(Omniscript::Type::createPointerType(referent->getType()));
    }

    return std::make_shared<Omniscript::AddressOfExpression>(name, referent);
}

std::shared_ptr<Omniscript::Expression> ReferenceTo::express(SymbolTableType scope) {
    DEBUG_LOG("Getting a reference to '" + name + "'.");
    // Look up the value in the scope to get the variable
    if (!scope->exists(name)) {
        console.error("Symbol '" + name + "' was not found in the scope");
    }

    auto variable = scope->getValue(name);
    if (variable) {
        setType(Omniscript::Type::createPointerType(variable->getType()));
        return std::make_shared<Omniscript::ReferenceExpression>(name, variable);
    }

    if (!variable) {
        auto overloads = scope->getOverloads(name);
        auto mangledName = std::dynamic_pointer_cast<Omniscript::FunctionExpression>(overloads[0])->mangledName;
        setType(Omniscript::Type::createPointerType(variable->getType()));
        return std::make_shared<Omniscript::ReferenceExpression>(mangledName, variable);
    }
    
    // If the variable isn't found, handle the error (e.g., return nullptr)
    console.error("Error: Variable " + name + " not found.\n");
    return nullptr;
}

// Get Variable
std::shared_ptr<Omniscript::Expression> GetVariable::express(SymbolTableType scope) {
    DEBUG_LOG();
    DEBUG_LOG("Getting symbol '" + name + "'.");
    if (!scope->exists(name)) {
        console.error("Symbol '" + name + "' does not exist in scope '" + scope->getName() + "'.");
    }

    std::shared_ptr<Omniscript::Type> symbolType = scope->get(name)->getType();

    if (type) {
        if (!Omniscript::isSameOrCastableTo(symbolType, type)) {
            console.error("Symbol '" + name + "' is of type '" + symbolType->toString() + "' it cannot be casted to a '" + type->toString() + "'.");
        }
        DEBUG_LOG("The casted type is '" + type->toString() + "'.");
    } else {
        DEBUG_LOG("The infered type is '" + symbolType->toString() + "'.");
        setType(symbolType);
    }

    return std::make_shared<Omniscript::VariableAccess>(name, type);
}

// Get Dynamic Variable
GetDynamicVariable::GetDynamicVariable(const std::string &variable) : variable(variable) {}

std::shared_ptr<Omniscript::Expression> GetDynamicVariable::express(SymbolTableType scope) {
    // return generator.getDynamicVariable(variable);
    return nullptr;
}

std::shared_ptr<Omniscript::Expression> AssignVariable::express(SymbolTableType scope) {
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
                DEBUG_LOG("The generic type is " + generic->getTypeExpression()->description());
                type = generic->getTypeExpression()->clone();
            }
        }

        if (type->isFunction()) {
            if (auto func = std::dynamic_pointer_cast<FunctionDeclaration>(value)) {
                func->setName(name);
            }
        }
        
        if (!type->isPointer() && !type->isReference()) {
            if (!value) {
                result = std::make_shared<Omniscript::NullExpression>(type);
            } else if (auto typed = std::dynamic_pointer_cast<TypedStatement>(value)) {
                if (!typed->getType() && !type->isInvalid()) {
                    typed->setType(type);
                    result = value->express(scope);
                    DEBUG_LOG("The variables set type is '" + type->toString() + "'.");
                } else {
                    result = value->express(scope);
                    if (type->getKind() != result->getType()->getKind() && !result->getType()->isNull()) {
                        console.error("The variable '" + variable + "' expects type '" + type->description() + "' or 'null' "+ 
                        " but got '" + result->getType()->description() + "' instead.");
                    }
                }
            }
        } else {
            if (type->isPointer()) {
                if (!value) {
                    result = std::make_shared<Omniscript::NullPointerExpression>(type);
                }  else if (auto typed = std::dynamic_pointer_cast<TypedStatement>(value)) {
                    result = typed->express(scope);
                    auto resultType = typed->getType();
                    if (!Omniscript::isSameOrCastableTo(resultType, type)) {
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
                        
                        DEBUG_LOG(expectedBaseType->description() + " " + actualBaseType->description());
                        if (expectedBaseType->getKind() != actualBaseType->getKind()) {
                            DEBUG_LOG("HERE 2.4.1");
                            console.error("Reference '" + variable + "' expects base type '" +
                                expectedBaseType->description() + "' but got '" +
                                actualBaseType->description() + "' instead.");
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
            DEBUG_LOG("The infered type is " + result->getType()->description());
        }
    }

    if (type) {     
        DEBUG_LOG(
                    "The result is " + variable + " " + result->getType()->toString() + " = " + result->toString()
                );
    } else {
        DEBUG_LOG("No type was deduced for variable '" + variable + "'.\n It had a value and no type or multiple types. Returning its result.");
        return result;
    }

    if (isReassign) {
        std::shared_ptr<Omniscript::Expression> prevValue = scope->get(variable);
        if (!Omniscript::isSameOrCastableTo(result->getType(), prevValue->getType())) {
            console.error("'" + variable + "' should be of type " + prevValue->getType()->description() + "' not a '" + result->getType()->description() + "'.");
        }
    }

    if (isConstant) {
        scope->setConstant(variable, result);
    } else {
        scope->setVariable(variable, result);
    }

    auto assignment = Omniscript::make_expression<Omniscript::VariableAssignment>(variable, result, isGlobal, true);
    assignment->isGlobal = isGlobal;
    assignment->isConstant = isConstant;
    return assignment;
}

// Dynamic Assignment
createDynamicVariable::createDynamicVariable(const std::string &variable, std::shared_ptr<Statement> value)
    : variable(variable), value(value) {}

std::shared_ptr<Omniscript::Expression> createDynamicVariable::express(SymbolTableType scope) {
    // return generator.assignDynamicVariable(variable, value->express(scope));
    return nullptr;
}

