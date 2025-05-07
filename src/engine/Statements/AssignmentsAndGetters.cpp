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
    setType(Omniscript::Type::createPointerType(referent->getType()));
    return std::make_shared<Omniscript::AddressOfExpression>(name, referent);
}

std::shared_ptr<Omniscript::Expression> ReferenceTo::express(SymbolTableType scope) {
    // Look up the value in the scope to get the variable
    auto variable = scope->getValue(name);
    if (variable) {
        return std::make_shared<Omniscript::ReferenceExpression>(name, variable);
    }
    
    // If the variable isn't found, handle the error (e.g., return nullptr)
    console.error("Error: Variable " + name + " not found.\n");
    return nullptr;
}

// Get Variable
std::shared_ptr<Omniscript::Expression> GetVariable::express(SymbolTableType scope) {
    if (!type) {
        setType(scope->get(name)->getType());
    }
    return std::make_shared<Omniscript::VariableAccess>(name, type);
}

// Get Dynamic Variable
GetDynamicVariable::GetDynamicVariable(const std::string &variable) : variable(variable) {}

std::shared_ptr<Omniscript::Expression> GetDynamicVariable::express(SymbolTableType scope) {
    // return generator.getDynamicVariable(variable);
    return nullptr;
}

void Assignment::setGlobalVisibilityTo(bool state) {
    isGlobal = state;
}

std::shared_ptr<Omniscript::Expression> AssignVariable::express(SymbolTableType scope) {
    DEBUG_LOG("Assigning variable " + variable);

    std::shared_ptr<Omniscript::Expression> result;

    if (isReassign) {
        if (!scope->exists(variable)) {
            console.log("Variable '" + variable + "' was not declared in scope '" + scope->getName() + "'.");
        }
    }

    if (type) {
        if (type->isGeneric()) {
            auto genericVal = scope->get(type->getName());
            if (auto generic = std::dynamic_pointer_cast<Omniscript::TypeExpression>(genericVal)) {
                DEBUG_LOG("The generic type is " + generic->getTypeExpression()->kindName());
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
                result = std::make_shared<Omniscript::NullPointerExpression>(type);
            } else if (auto typed = std::dynamic_pointer_cast<TypedStatement>(value)) {
                if (!typed->getType()) {
                    typed->setType(type);
                    result = value->express(scope);
                } else {
                    result = value->express(scope);
                    if (type->getKind() != result->getType()->getKind() && !result->getType()->isNull()) {
                        console.error("The variable '" + variable + "' expects type '" + type->kindName() + "' or 'null' "+ 
                        " but got '" + result->getType()->kindName() + "' instead.");
                    }
                }
            }
        } else {
            if (type->isPointer()) {
                if (!value) {
                    result = std::make_shared<Omniscript::NullPointerExpression>(type);
                } else if (auto nullpointer = std::dynamic_pointer_cast<Nullptr>(value)) {
                    result = nullpointer->express(scope);
                } else if (auto addressOf = std::dynamic_pointer_cast<AddressOf>(value)) {
                    result = addressOf->express(scope);
                    if (auto ptr = std::dynamic_pointer_cast<Omniscript::PointerExpression>(result)) {
                        console.info("Pointer '" + variable + "' should point to a '" + type->getPointeeType()->kindName() + "' and is pointing to a '" +
                        ptr->getType()->getPointeeType()->kindName() + "'.");
                        if (ptr->getType()->getPointeeType()->getKind() != type->getPointeeType()->getKind()) {
                            console.error("Pointer '" + variable + "' should point to a '" + type->getPointeeType()->kindName() + "' but is pointing to a '" +
                            ptr->getType()->getPointeeType()->kindName() + "' instead.");
                        }
                    } else if (auto addr = std::dynamic_pointer_cast<Omniscript::AddressOfExpression>(result)) {
                        if (addr->getType()->getBasePointeeType()->getKind() != type->getBasePointeeType()->getKind()) {
                            console.error("Pointer '" + variable + "' should point to a '" + type->pointerDescription() + "' but is pointing to a '" +
                            addr->getType()->pointerDescription() + "' instead.");
                        }
                    } else {
                        console.error("Pointer '" + variable + "' is pointing to an invalid pointer type '" + result->toString() + "'.");
                    }
                } else if (auto referenceTo = std::dynamic_pointer_cast<ReferenceTo>(value)) {
                    result = referenceTo->express(scope);
                    
                    if (result->getType()->getKind() != type->getPointeeType()->getKind()) {
                        console.error("Pointer '" + variable + "' should point to a '" + type->kindName() + "' but is pointing to a '" +
                        type->getPointeeType()->kindName() + "' instead.");
                    }
                } else if (auto string = std::dynamic_pointer_cast<StringLiteral>(value)) {
                    if (!type->getPointeeType()->isChar() && !type->getPointeeType()->isString()) {
                        console.error("A string's can be character pointer (let " + variable + " : char* = \"foo bar\";) or 'utf8', 'utf16; or 'utFloat' not a '" + type->pointerDescription() + "'.");
                    }
                    if (auto typed = std::dynamic_pointer_cast<TypedStatement>(value)) {
                        if (!type->getPointeeType()->isChar()) {
                            typed->setType(type->getPointeeType());
                        }
                    }
                    result = string->express(scope);
                    
                    // if (result->getType()->getKind() != type->getPointeeType()->getKind()) {
                    //     console.error("Pointer '" + variable + "' should point to a '" + type->kindName() + "' but is pointing to a '" +
                    //     type->getPointeeType()->kindName() + "' instead.");
                    // }
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
                        
                        DEBUG_LOG(expectedBaseType->kindName() + " " + actualBaseType->kindName());
                        if (expectedBaseType->getKind() != actualBaseType->getKind()) {
                            DEBUG_LOG("HERE 2.4.1");
                            console.error("Reference '" + variable + "' expects base type '" +
                                expectedBaseType->kindName() + "' but got '" +
                                actualBaseType->kindName() + "' instead.");
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
            DEBUG_LOG("The infered type is " + result->getType()->kindName());
        }
    }

    if (type) {     
        DEBUG_LOG(
                    "The result is " + variable + " " + 
                    (result->getType()->elementType ? result->getType()->elementType->kindName() + " " + result->getType()->kindName() : result->getType()->kindName())
                    + " = " + result->toString()
                );
    } else {
        DEBUG_LOG("No type was deduced for variable '" + variable + "'. It had a value and no type or multiple types. Returning its result.");
        return result;
    }

    if (isReassign) {
        std::shared_ptr<Omniscript::Expression> prevValue = scope->get(variable);
        if (!Omniscript::isSameOrCastableTo(result->getType(), prevValue->getType())) {
            console.error("'" + variable + "' should be of type " + prevValue->getType()->kindName() + "' not a '" + result->getType()->kindName() + "'.");
        }
    }

    scope->setVariable(variable, result);

    return Omniscript::make_expression<Omniscript::VariableAssignment>(variable, result, isGlobal, true);
}    


// Constant Assignment
std::shared_ptr<Omniscript::Expression> createConstant::express(SymbolTableType scope) {
    // return generator.createConstant(variable, type, value->express(scope));
    return nullptr;
}

// Dynamic Assignment
createDynamicVariable::createDynamicVariable(const std::string &variable, std::shared_ptr<Statement> value)
    : variable(variable), value(value) {}

std::shared_ptr<Omniscript::Expression> createDynamicVariable::express(SymbolTableType scope) {
    // return generator.assignDynamicVariable(variable, value->express(scope));
    return nullptr;
}

