#include <omniscript/Core.h>
#include <omniscript/utils.h>
#include <omniscript/engine/Statement.h>
#include <omniscript/engine/Symboltable.h>
#include <omniscript/omniscript_pch.h>
#include <omniscript/engine/lexer.h>
#include <omniscript/engine/Parser.h>
#include <omniscript/utils.h>
// #include "Statement.h"

// #include <omniscript/runtime/object.h>
// #include <omniscript/runtime/Class.h>
// #include <omniscript/runtime/Namespace.h>
// #include <omniscript/runtime/Enum.h>
// #include <omniscript/runtime/object.h>
// // #include <omniscript/runtime/Function.h>
// #include <omniscript/runtime/Number.h>
// #include <omniscript/runtime/String.h>
// #include <omniscript/runtime/Pointer.h>

void Initializer::initialize() {
    // std::vector<std::shared_ptr<Statement>> types = {};

    // for ()
    // auto type = std::make_shared<Omniscript::TypeExpression>();
}

std::shared_ptr<Omniscript::Expression> Initializer::express(SymbolTableType scope) {
    return nullptr;
}  


std::shared_ptr<Omniscript::Expression> BlockStatement::express(SymbolTableType scope) {
    return std::make_shared<Omniscript::BlockExpression>(expressAsVector(scope));
}

std::vector<std::shared_ptr<Omniscript::Expression>> BlockStatement::expressAsVector(SymbolTableType scope) {
    recursiveUpdate();
    
    std::vector<std::shared_ptr<Omniscript::Expression>> results = {};
    
    // // Generate code for each statement in order
    for (const auto& stmt : statements) {
        // Handle type propagation if needed
        if (auto typed = std::dynamic_pointer_cast<TypedStatement>(stmt)) {
            if (type) {
                typed->setType(type);
            }
        }

        if (auto assignment = std::dynamic_pointer_cast<Assignment>(stmt)) {
            assignment->setGlobalVisibilityTo(false);
        }

        results.push_back(stmt->express(scope));
        
        // // If the current block already has a terminator, stop generating
        // if (generator.currentBlockHasTerminator()) {
        //     break;
        // }
    }
    
    // // Pop the scope we created for this block
    // generator.popScope();
    
    // // Return the last computed value (may be nullptr for statements without values)
    // return lastValue;
    return results;
}

bool BlockStatement::hasSideEffects() {
    return !isCompileTimeEvaluatable();
}


bool BlockStatement::isCompileTimeEvaluatable() {
    for (const auto& stmt : statements) {
        if (!stmt->isCompileTimeEvaluatable()) {
            return false;
        }
    }
    return true;
}

void BlockStatement::recursiveUpdate() {
    resolveGenerics();
    for (auto& stmt : statements) {
        if (auto assign = std::dynamic_pointer_cast<Assignment>(stmt)) {
            if (assign->isStatic) {
                assign->isGlobal = true;
            } else {
                assign->isGlobal = false;
            }
        }
        if (auto assign = std::dynamic_pointer_cast<BlockStatement>(stmt)) {
            recursiveUpdate();
        }
    }
}

std::shared_ptr<Omniscript::Expression> ImportModule::express(SymbolTableType scope) {
    // if (path.empty()) {
    //     console.error("ImportModule::codegen - Module path is empty.");
    // }

    // if (path == "std") {
    //     path = "standard/1/std.os";
    // }

    // std::string sourceCode = readFile(path);
    // if (sourceCode.empty()) {
    //     console.error("ImportModule::codegen - Failed to read module: " + path);
    // }

    // Lexer lexer(sourceCode, path);
    // Parser parser(lexer);

    // // parser.setScopeName(alias.empty() ? moduleName : alias);

    // std::vector<std::shared_ptr<Statement>> statements = parser.Parse();

    // console.log("Importing " + (importAll ? "everything" : joinMapKeys(importedAliases)) + " from " + path + ".");

    // // Ensure module is only loaded once
    // if (!generator.isLoadedModule(path)) {
    //     generator.generateModule(path, alias, statements, importedAliases, importAll);
    // }

    return nullptr; // No direct IR generation
}


std::shared_ptr<Omniscript::Expression> CreateModule::express(SymbolTableType scope) {
    // generator.importModule(name);8
    return nullptr; // Modules themselves don't return a value
}

std::shared_ptr<Omniscript::Expression> PublicMember::express(SymbolTableType scope) {
    if (auto assignment = std::dynamic_pointer_cast<Assignment>(value)) {
        assignment->setGlobalVisibilityTo(true);
    }
    return value->express(scope);
}

std::shared_ptr<Omniscript::Expression> PrivateMember::express(SymbolTableType scope) {
    if (auto assignment = std::dynamic_pointer_cast<Assignment>(value)) {
        assignment->setGlobalVisibilityTo(true);
    }
    return value->express(scope);
}

std::shared_ptr<Omniscript::Expression> AddressOf::express(SymbolTableType scope) {
    std::shared_ptr<Omniscript::Expression> referent = scope->getValue(name);
    setType(referent->getType());
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

std::shared_ptr<Omniscript::Expression> Nullptr::express(SymbolTableType scope) {
    if (!type) {
        type = Omniscript::Type::createNullPointerType();
    }
    return Omniscript::make_expression<Omniscript::NullPointerExpression>(type);
}

std::shared_ptr<Omniscript::Expression> Null::express(SymbolTableType scope) {
    if (!type) {
        type = Omniscript::Type::createNullType();
    }
    return Omniscript::make_expression<Omniscript::NullExpression>(type);
}

std::shared_ptr<Omniscript::Expression> IntegerLiteral::express(SymbolTableType scope) {
    if (!type) {
        DEBUG_LOG("Creating a 32-bit integer");
        type = Omniscript::Type::createPrimitiveType(Omniscript::Kind::Int32);
        return std::make_shared<Omniscript::Integer<int32_t>>(static_cast<int32_t>(value));
    }

    auto typeToCastFrom = std::make_shared<Omniscript::Type>(Omniscript::Kind::Int8);

    if (!Omniscript::isSameOrCastableTo(typeToCastFrom, type)) {
        console.error("The specified type is '" + type->kindName() + "' but '" + std::to_string(value) + "' is an integer.");
    } else {
        if (!type->isInteger()) {
            DEBUG_LOG("Casting integer to '" + type->kindName() + "'.");
            return castTo(type)->express(scope);
        }
        DEBUG_LOG("Creating an '" + type->kindName() + "' integer");
    }
    
    // Check for specific bit-widths using the isInteger function with optional bitwidth argument
    if (type->isInteger(8)) {
        DEBUG_LOG("Creating an 8-bit integer");
        return std::make_shared<Omniscript::Integer<int8_t>>(static_cast<int8_t>(value));
    } else if (type->isInteger(16)) {
        DEBUG_LOG("Creating a 16-bit integer");
        return std::make_shared<Omniscript::Integer<int16_t>>(static_cast<int16_t>(value));
    } else if (type->isInteger(32)) {
        DEBUG_LOG("Creating a 32-bit integer");
        return std::make_shared<Omniscript::Integer<int32_t>>(static_cast<int32_t>(value));
    } else if (type->isInteger(64)) {
        DEBUG_LOG("Creating a 64-bit integer");
        return std::make_shared<Omniscript::Integer<int64_t>>(static_cast<int64_t>(value));
    } else if (type->isInteger(128)) {
        DEBUG_LOG("Creating a 128-bit integer");
        return std::make_shared<Omniscript::BigInt>(std::to_string(value), 128);
    } else if (type->isInteger(256)) {
        DEBUG_LOG("Creating a 256-bit integer");
        return std::make_shared<Omniscript::BigInt>(std::to_string(value), 256);
    } else if (type->isInteger(512)) {
        DEBUG_LOG("Creating a 512-bit integer");
        return std::make_shared<Omniscript::BigInt>(std::to_string(value), 512);
    } else if (type->isInteger(1024)) {
        DEBUG_LOG("Creating a 1024-bit integer");
        return std::make_shared<Omniscript::BigInt>(std::to_string(value), 1024);
    }

    return nullptr;
}

std::shared_ptr<Literal> IntegerLiteral::castTo(std::shared_ptr<Omniscript::Type> targetType) const {
    using Kind = Omniscript::Kind;

    switch (targetType->getKind()) {
        case Kind::Int8:
        case Kind::Int16:
        case Kind::Int32:
        case Kind::Int64:
            return std::make_shared<IntegerLiteral>(value);  // Safe truncation assumed
        case Kind::Float:
        case Kind::Double:
        case Kind::Half: {
            auto val = std::make_shared<FloatLiteral>(static_cast<double>(value));
            val->setType(type);
            return val;
        }
        case Kind::Bool:
            return std::make_shared<BoolLiteral>(value != 0);
        case Kind::Char:
            return std::make_shared<CharacterLiteral>(static_cast<char>(value));
        default:
            return nullptr;
    }
}

std::shared_ptr<Omniscript::Expression> FloatLiteral::express(SymbolTableType scope) {
    // Default to 64-bit float if type is null or unknown
    if (!type) {
        DEBUG_LOG("Creating a 64-bit float");
        type = Omniscript::Type::createPrimitiveType(Omniscript::Kind::Double);
        return std::make_shared<Omniscript::Float<double>>(static_cast<double>(value));  // Default to double (64-bit)
    }

    auto typeToCastFrom = std::make_shared<Omniscript::Type>(Omniscript::Kind::Half);

    if (!Omniscript::isSameOrCastableTo(typeToCastFrom, type))  {
        console.error("The specified type is " + type->kindName() + " but '" + std::to_string(value) + "' is a float.");
    } else {
        if (!type->isFloat()) {
            DEBUG_LOG("Casting float to '" + type->kindName() + "'.");
            return castTo(type)->express(scope);
        }
        DEBUG_LOG("Creating an '" + type->kindName() + "' float.");
    }


    // Check if the type is a 32-bit float
    #ifdef __ARM_ARCH
        // ARM platforms using __fp16
        if (type->isFloat(16)) {
            DEBUG_LOG("Creating a 16-bit float (__fp16 for ARM)");
            return std::make_shared<Omniscript::Float<__fp16>>(static_cast<__fp16>(value));  // ARM __fp16 type
        }
    #elif defined(__x86_64__) || defined(__i386__)
        // x86 platforms using _Float16
        if (type->isFloat(16)) {
            DEBUG_LOG("Creating a 16-bit float (_Float16 for x86)");
            return std::make_shared<Omniscript::Float<_Float16>>(static_cast<_Float16>(value));  // x86 _Float16 type
        }
    #endif
    if (type->isFloat(32)) {
        DEBUG_LOG("Creating a 32-bit float");
        return std::make_shared<Omniscript::Float<float>>(static_cast<float>(value));  // 32-bit float
    }

    // Check if the type is a 64-bit float
    if (type->isFloat(64)) {
        DEBUG_LOG("Creating a 64-bit float");
        return std::make_shared<Omniscript::Float<double>>(static_cast<double>(value));  // 64-bit double
    }

    // Check if the type is FP128 (128-bit floating point)
    if (type->isFloat(128)) {
        DEBUG_LOG("Creating a 128-bit float (FP128)");
        return std::make_shared<Omniscript::Float<__float128>>(static_cast<__float128>(value));  // FP128 (128-bit)
    }

    // Check if the type is X86_FP80 (80-bit floating point)
    if (type->isFloat(80)) {
        DEBUG_LOG("Creating an 80-bit float (X86_FP80)");
        return std::make_shared<Omniscript::Float<long double>>(static_cast<long double>(value));  // X86_FP80 (80-bit)
    }

    // Check if the type is PPC_FP128 (128-bit floating point)
    if (type->isFloat(128)) {
        DEBUG_LOG("Creating a 128-bit float (PPC_FP128)");
        return std::make_shared<Omniscript::Float<__float128>>(static_cast<__float128>(value));  // PPC_FP128 (128-bit)
    }

    // If necessary, handle other custom floating-point types (e.g., Half, etc.)
    return nullptr;  // If no valid type matches, return nullptr
}

std::shared_ptr<Literal> FloatLiteral::castTo(std::shared_ptr<Omniscript::Type> targetType) const {
    using Kind = Omniscript::Kind;
    switch (targetType->getKind()) {
        case Kind::Float:
        case Kind::Double:
        case Kind::Half: {
            auto val = std::make_shared<FloatLiteral>(value);
            val->setType(type);
            return val;
        }
        case Kind::Int8:
        case Kind::Int16:
        case Kind::Int32:
        case Kind::Int64: {
            auto val = std::make_shared<IntegerLiteral>(static_cast<int64_t>(value));
            val->setType(type);
            return val;
        }
        case Kind::Bool:
            return std::make_shared<BoolLiteral>(value != 0.0);
        default:
            return nullptr;
    }
}


// Arbitrary-precision integer (BigInt)
std::shared_ptr<Omniscript::Expression> BigInt::express(SymbolTableType scope) {
    DEBUG_LOG("Creating a big int " + value);
    unsigned bitWidth = BigInt::determineBitWidth(value);
    return std::make_shared<Omniscript::BigInt>(value, bitWidth);
}

std::shared_ptr<Omniscript::Expression> Invalid::express(SymbolTableType scope) {
    DEBUG_LOG("Creating an invalid");
    return std::make_shared<Omniscript::InvalidExpression>();
}

std::shared_ptr<Omniscript::Expression> BoolLiteral::express(SymbolTableType scope) {
    // DEBUG_LOG("Bool value " + value);
    if (!type) {
        DEBUG_LOG("Creating a bool false");
        type = Omniscript::Type::createPrimitiveType(Omniscript::Kind::Bool);
        return std::make_shared<Omniscript::Primitive<bool>>(value);  // Default to double (64-bit)
    }

    auto typeToCastFrom = std::make_shared<Omniscript::Type>(Omniscript::Kind::Bool);

    if (!Omniscript::isSameOrCastableTo(typeToCastFrom, type))  {
        console.error("The specified type is " + type->kindName() + " but '" + std::to_string(value) + "' is a bool.");
    } else {
        if (!type->isBool()) {
            DEBUG_LOG("Casting bool to '" + type->kindName() + "'.");
            return castTo(type)->express(scope);
        }
        DEBUG_LOG("Creating an '" + type->kindName() + "'.");
    }

    return std::make_shared<Omniscript::Primitive<bool>>(value);
}

std::shared_ptr<Literal> BoolLiteral::castTo(std::shared_ptr<Omniscript::Type> targetType) const {
    using Kind = Omniscript::Kind;

    switch (targetType->getKind()) {
        case Kind::Bool:
            return std::make_shared<BoolLiteral>(value);
        case Kind::Int8:
        case Kind::Int16:
        case Kind::Int32:
        case Kind::Int64:
            return std::make_shared<IntegerLiteral>(value ? 1 : 0);
        case Kind::Float:
        case Kind::Double:
        case Kind::Half:
            return std::make_shared<FloatLiteral>(value ? 1.0 : 0.0);
        default:
            return nullptr;
    }
}


std::shared_ptr<Omniscript::Expression> CharacterLiteral::express(SymbolTableType scope) {
    if (!type) {
        DEBUG_LOG("Creating a char literal");
        type = Omniscript::Type::createPrimitiveType(Omniscript::Kind::Char);
        return std::make_shared<Omniscript::Primitive<char>>(value);  // Default to double (64-bit)
    }

    if (!type->isChar()) {
        console.error("The specified type is " + type->kindName() + " but '" + std::to_string(value) + "' is a char.");
    } else {
        DEBUG_LOG("Creating a '" + type->kindName() + value + "'.");
    }

    return std::make_shared<Omniscript::Primitive<char>>(value);
}

std::shared_ptr<Omniscript::Expression> StringLiteral::express(SymbolTableType scope) {
    if (!type) {
        DEBUG_LOG("Creating a string value");
        type = Omniscript::Type::createPrimitiveType(Omniscript::Kind::Utf8);
        return std::make_shared<Omniscript::StringExpression<std::string>>(value); 
    }

    auto typeToCastFrom = std::make_shared<Omniscript::Type>(Omniscript::Kind::Utf8);

    if (!Omniscript::isSameOrCastableTo(typeToCastFrom, type))  {
        console.error("The specified type is " + type->kindName() + " but '" + value + "' is a string.");
    } else {
        DEBUG_LOG("Creating a '" + type->kindName() + value + "'.");
    }

    if (type->isString(8)) {
        DEBUG_LOG("Creating UTF-8 string");
        return std::make_shared<Omniscript::StringExpression<std::string>>(value);
    } else if (type->isString(16)) {
        DEBUG_LOG("Creating UTF-16 string");
        std::u16string utf16_value(value.begin(), value.end());
        return std::make_shared<Omniscript::StringExpression<std::u16string>>(utf16_value);
    } else if (type->isString(32)) {
        DEBUG_LOG("Creating UTF-32 string");
        std::u32string utFloat_value(value.begin(), value.end());
        return std::make_shared<Omniscript::StringExpression<std::u32string>>(utFloat_value);
    }
    
    return nullptr;
}


// ======================= Assignments and Variable Getters ======================= //
// Assignment
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

// Get Variable
std::shared_ptr<Omniscript::Expression> GetVariable::express(SymbolTableType scope) {
    return std::make_shared<Omniscript::VariableAccess>(name, type);
}

// Get Dynamic Variable
GetDynamicVariable::GetDynamicVariable(const std::string &variable) : variable(variable) {}

std::shared_ptr<Omniscript::Expression> GetDynamicVariable::express(SymbolTableType scope) {
    // return generator.getDynamicVariable(variable);
    return nullptr;
}

std::shared_ptr<Omniscript::Expression> BreakStatement::express(SymbolTableType scope) {
    return nullptr;
}

std::shared_ptr<Omniscript::Expression> ContinueStatement::express(SymbolTableType scope) {
    return nullptr;
}

// std::shared_ptr<Omniscript::Expression> ObjectConstructorStatement::express(SymbolTableType scope) {
//     return nullptr;
// }

std::shared_ptr<Omniscript::Expression> ForLoop::express(SymbolTableType scope) {
    auto localScope = scope->createChildScope("forloop");
    DEBUG_LOG("Creating a for loop expression");
    std::shared_ptr<Omniscript::Expression> initializationExpr;
    if (initialization) {
        if (auto assign = std::dynamic_pointer_cast<Assignment>(initialization)) {
            assign->isGlobal = false;
        }
        initializationExpr = initialization->express(localScope);
    }
    DEBUG_LOG("Created its initialization expression");
    std::shared_ptr<Omniscript::Expression> conditionExpr = condition? condition->express(localScope) : nullptr;
    DEBUG_LOG("Created its condition expression");
    std::shared_ptr<Omniscript::Expression> increamentExpr = increment? increment->express(localScope) : nullptr;
    DEBUG_LOG("Created its update expression");
    std::shared_ptr<Omniscript::Expression> bodyExpr = body->express(localScope);
    DEBUG_LOG("Created its body");

    return std::make_shared<Omniscript::ForLoopExpression>(initializationExpr, conditionExpr, increamentExpr, bodyExpr);
}

std::shared_ptr<Omniscript::Expression> GetProperty::express(SymbolTableType scope) {
    return nullptr;
}

std::shared_ptr<Omniscript::Expression> CallMethod::express(SymbolTableType scope) {
    return nullptr;
}

std::shared_ptr<Omniscript::Expression> WhileStatement::express(SymbolTableType scope) {
   auto localScope = scope->createChildScope("whileloop");
    DEBUG_LOG("Creating a while loop expression");

    std::shared_ptr<Omniscript::Expression> conditionExpr = condition ? condition->express(localScope) : nullptr;
    DEBUG_LOG("Created its condition expression");

    std::shared_ptr<Omniscript::Expression> bodyExpr = body ? body->express(localScope) : nullptr;
    DEBUG_LOG("Created its body expression");

    return std::make_shared<Omniscript::WhileLoopExpression>(conditionExpr, bodyExpr);
}


std::shared_ptr<Omniscript::Expression> TernaryExpression::express(SymbolTableType scope) {
    // Assign types
    if (auto stmt = std::dynamic_pointer_cast<TypedStatement>(truthy)) {
        stmt->setType(type);
    }

    if (auto stmt = std::dynamic_pointer_cast<TypedStatement>(falsey)) {
        stmt->setType(type);
    }

    if (auto stmt = std::dynamic_pointer_cast<TypedStatement>(condition)) {
        std::vector<std::string> typeStr = {"bool"};
        stmt->setType(Omniscript::resolveType(typeStr));
    }

    // Evaluate condition, then branches
    std::shared_ptr<Omniscript::Expression> condValue = condition->express(scope);
    if (!condValue) return nullptr;

    std::shared_ptr<Omniscript::Expression> trueValue = truthy->express(scope);
    if (!trueValue) return nullptr;

    std::shared_ptr<Omniscript::Expression> falseValue = falsey->express(scope);
    if (!falseValue) return nullptr;

    return std::make_shared<Omniscript::TernaryExpression>(
        condValue, trueValue, falseValue, type
    );
}


std::shared_ptr<Omniscript::Expression> BinaryExpression::express(SymbolTableType scope) {
    DEBUG_LOG();
    // Set the expected result type for child expressions
    std::shared_ptr<Omniscript::Expression> leftValue;
    std::shared_ptr<Omniscript::Expression> rightValue;
     
    if (auto stmt = std::dynamic_pointer_cast<TypedStatement>(left)) {
        if (!type) {
            leftValue = left->express(scope);
            setType(stmt->getType());
        } else {
            stmt->setType(type);
        }
        DEBUG_LOG("Set the type for the left expression");
    }
    if (auto stmt = std::dynamic_pointer_cast<TypedStatement>(right)) {
        if (!type) {
            rightValue = right->express(scope);
            setType(stmt->getType());
        } else {
            stmt->setType(type);
        }
        DEBUG_LOG("Set the type for the right expression");
    }
    
    DEBUG_LOG("Creating a binary expression of kind '" + type->kindName() + "'.");

    // Evaluate left and right operands
    if (!leftValue) return nullptr;
    DEBUG_LOG("The left value is " + leftValue->toString());
    
    if (!rightValue) return nullptr;
    DEBUG_LOG("The right value is " + rightValue->toString());

    // Wrap both evaluated values into a BinaryExpressionValue
    return std::make_shared<Omniscript::BinaryExpression>(leftValue, op, rightValue, type);
}

bool BinaryExpression::hasSideEffects() {
    return !isCompileTimeEvaluatable();
}

bool BinaryExpression::isCompileTimeEvaluatable() {
    if (left->isCompileTimeEvaluatable() && right->isCompileTimeEvaluatable()) {
        return true;
    }
    return false;
}

std::shared_ptr<Omniscript::Expression> UnaryExpression::express(SymbolTableType scope) {
    DEBUG_LOG("Creating a unary expression");
    // Set the expected type on the operand
    std::shared_ptr<Omniscript::Expression> operandValue;

    if (auto stmt = std::dynamic_pointer_cast<TypedStatement>(operand)) {
        if (type) {
            stmt->setType(type);
            operand->express(scope);
        } else {
            operandValue = operand->express(scope);
            setType(stmt->getType());
        }
    }

    // Evaluate the operand
    if (!operandValue) return nullptr;

    bool isPrefix = position == Position::Prefix;

    return std::make_shared<Omniscript::UnaryExpression>(op, operandValue, type, isPrefix);
}

std::shared_ptr<Statement> IfStatement::evaluate(SymbolTableType scope) {
    // Iterate through the conditions and check which one is true
    for (size_t i = 0; i < conditions.size(); ++i) {
        // Assuming the condition evaluates to a boolean
        auto result = conditions[i]->evaluate(scope); // Evaluate condition

        // If the condition is true, execute the corresponding body
        // if (result->isTruthy(scope)) {
        //     return bodies[i]; // Return the body if condition is true
        // }
    }

    // If no condition matches, return the elseBody if present
    if (elseBody) {
        return elseBody;
    }

    return nullptr; // If no condition is met and no elseBody exists, return nullptr
}

std::shared_ptr<Omniscript::Expression> IfStatement::express(SymbolTableType scope) {
    std::vector<std::shared_ptr<Omniscript::Expression>> exprConditions;
    std::vector<std::shared_ptr<Omniscript::Expression>> exprBranches;

    for (size_t i = 0; i < conditions.size(); ++i) {
        exprConditions.push_back(conditions[i]->express(scope)); // already Expression pointers
        exprBranches.push_back(bodies[i]->express(scope)); // convert BlockStatement to Expression
    }

    std::shared_ptr<Omniscript::Expression> elseExpr = nullptr;
    if (elseBody) {
        elseExpr = elseBody->express(scope); // convert BlockStatement to Expression
    }

    return std::make_shared<Omniscript::IfExpression>(
        exprConditions,
        exprBranches,
        elseExpr
    );
}

// TODO: Add the name of the object being called
std::shared_ptr<Omniscript::Expression> Call::express(SymbolTableType scope) {
    DEBUG_LOG("[Call] Evaluating call to '" + callee + "'");
    
    std::string originalCallee = callee;
    DEBUG_LOG("[Call] Looking up callee '" + originalCallee + "' in scope");
    std::shared_ptr<Omniscript::Expression> called;

    // Attempt overload resolution
    DEBUG_LOG("[Call] Attempting overload resolution for '" + originalCallee + "'");
    auto overloads = scope->getOverloads(originalCallee);
    if (!overloads.empty()) {
        DEBUG_LOG("[Call] Found " + std::to_string(overloads.size()) + " overload candidates");

        for (auto& overload : overloads) {
            auto funcExpr = std::dynamic_pointer_cast<Omniscript::FunctionExpression>(overload);
            if (!funcExpr) {
                DEBUG_LOG("[Call] Skipping non-function overload.");
                continue;
            }
        
            DEBUG_LOG("[Call] Checking if the overload '" + funcExpr->mangledName + "' (" + funcExpr->name + ") is the required overload.");
        
            auto paramList = funcExpr->getParameters();
            DEBUG_LOG("[Call] Function '" + funcExpr->mangledName + "' expects " + std::to_string(paramList.size()) + " parameters.");
        
            std::vector<std::shared_ptr<Omniscript::FunctionInputExpression>> evaluatedArgs;
            for (const auto& arg : args) {
                auto result = arg->express(scope);
                if (auto argStatement = std::dynamic_pointer_cast<ArgumentStatement>(arg)) {
                    DEBUG_LOG("[Call] Evaluated a named argument '" + argStatement->getName() + "'.");
                    auto inputExpr = std::make_shared<Omniscript::FunctionInputExpression>(argStatement->getName(), result->getType(), result);
                    evaluatedArgs.push_back(inputExpr);
                    DEBUG_LOG("[Call] Evaluated argument: " + std::string(inputExpr ? "OK" : "null (treated as undefined)"));
                } else {
                    DEBUG_LOG("[Call] Evaluated an unamed argument");
                    auto inputExpr = std::make_shared<Omniscript::FunctionInputExpression>("", result->getType(), result);
                    evaluatedArgs.push_back(inputExpr);
                    DEBUG_LOG("[Call] Evaluated argument: " + std::string(inputExpr ? "OK" : "null (treated as undefined)"));
                }
            }
        
            std::vector<std::shared_ptr<Omniscript::FunctionInputExpression>> inputParams;
            for (const auto& param : paramList) {
                auto casted = std::dynamic_pointer_cast<Omniscript::FunctionInputExpression>(param);
                if (!casted) {
                    console.error(formatError("Failed to cast parameter to FunctionInputExpression."));
                    continue; // or return nullptr / throw, depending on your error handling
                }
                inputParams.push_back(casted);
                DEBUG_LOG("[Call] Parameter '" + casted->name + "' of type '" + casted->getType()->kindName() + "'");
            }
        
            if (matchArgumentsToParameters(evaluatedArgs, inputParams, scope)) {
                DEBUG_LOG("[Call] ✅ Matched overload: using mangled name '" + funcExpr->mangledName + "'");
                callee = funcExpr->mangledName;
                called = funcExpr;
                DEBUG_LOG("[Call] Called is now " + called->toString());
                break;
            } else {
                DEBUG_LOG("[Call] ❌ Overload '" + funcExpr->mangledName + "' did not match.");
            }
        }
    } else {
        called = scope->get(originalCallee);
    }

    if (!called) {
        DEBUG_LOG("[Call] ERROR: Callee '" + originalCallee + "' not found in scope");
        console.error(formatError("Callable '" + originalCallee + "' not found in scope " + scope->getName()));
        return nullptr;
    }

    DEBUG_LOG("[Call] Found callee '" + callee + "' of type '" + 
              (called->getType() ? called->getType()->kindName() : "null") + "'");

    // Extract parameter list and return type
    std::vector<std::shared_ptr<Omniscript::FunctionInputExpression>> parameters;
    if (auto callable = std::dynamic_pointer_cast<Omniscript::Callable>(called)) {
        DEBUG_LOG("[Call] Callee is callable, cloning parameters");
        parameters = callable->cloneParameters();
        type = callable->getType();
        if (type->isFunction()) {
            type = type->getReturnType();
        }
        DEBUG_LOG("[Call] Cloned " + std::to_string(parameters.size()) + " parameters");
    } else {
        DEBUG_LOG("[Call] ERROR: Callee is not callable");
        console.error(formatError("'" + callee + "' is not callable; it is of kind '" + 
                      (called->getType() ? called->getType()->kindName() : "null") + "'."));
        return nullptr;
    }

    // Create a local scope
    DEBUG_LOG("[Call] Creating local scope for call to '" + callee + "'");
    auto localScope = scope->createChildScope("call_" + callee);
    DEBUG_LOG("[Call] Created local scope with " + std::to_string(parameters.size()) + " parameters");

    std::unordered_set<std::string> providedParams;
    size_t positionalArgIndex = 0;

    DEBUG_LOG("[Call] Processing " + std::to_string(args.size()) + " arguments");

    // First pass: named arguments
    for (const auto& arg : args) {
        if (auto namedArg = std::dynamic_pointer_cast<ArgumentStatement>(arg)) {
            const std::string& paramName = namedArg->getName();
            DEBUG_LOG("[Call] Processing named argument '" + paramName + "'");

            bool found = false;
            for (const auto& param : parameters) {
                if (param->name == paramName) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                DEBUG_LOG("[Call] ERROR: Unknown parameter '" + paramName + "'");
                console.error(formatError("Unknown parameter '" + paramName + "' for callable '" + callee + "'"));
                continue;
            }

            auto evaluated = namedArg->value ? namedArg->value->express(scope) : nullptr;
            localScope->set(paramName, evaluated);
            providedParams.insert(paramName);
            DEBUG_LOG("[Call] Set named parameter '" + paramName + "' in local scope");

            if (auto typed = std::dynamic_pointer_cast<TypedStatement>(namedArg->value)) {
                typed->setType(type);
                DEBUG_LOG("[Call] Set type for named argument '" + paramName + "'");
            }
        }
    }

    // Second pass: positional arguments and defaults
    for (const auto& param : parameters) {
        const std::string& paramName = param->name;

        if (providedParams.count(paramName)) {
            continue;
        }

        if (positionalArgIndex < args.size()) {
            auto arg = args[positionalArgIndex++];

            if (std::dynamic_pointer_cast<ArgumentStatement>(arg)) {
                DEBUG_LOG("[Call] ERROR: Positional argument after named argument");
                console.error(formatError("Positional argument after named argument is not allowed."));
                continue;
            }

            if (auto typed = std::dynamic_pointer_cast<TypedStatement>(arg)) {
                if (Omniscript::isSameOrCastableTo(typed->getRootType(), param->getType())) {
                    if (!typed->getType()) {
                        typed->setType(param->getType());
                    }
                } else {
                    console.error(formatError("Cannot bind argument of type '" + typed->getRootType()->kindName() +
                                  "' to parameter '" + paramName + "'; expected '" + param->getType()->kindName() + "'"));
                }
            }

            auto value = arg->express(scope);
            if (!value || value->getType()->isInvalid()) {
                console.error(formatError("Invalid argument for parameter '" + paramName + "'"));
            }

            localScope->set(paramName, value);
            DEBUG_LOG("[Call] Set positional argument for '" + paramName + "'");

        } else if (param->value) {
            DEBUG_LOG("[Call] Using default value for parameter '" + paramName + "'");
            localScope->set(paramName, param->value);
        } else {
            DEBUG_LOG("[Call] ERROR: Missing required parameter '" + paramName + "'");
            console.error(formatError("Missing required argument for parameter '" + paramName + "'"));
        }
    }

    // Check for extra args (varargs or error)
    if (positionalArgIndex < args.size()) {
        if (auto func = std::dynamic_pointer_cast<Omniscript::Callable>(called)) {
            if (!func->isVarArg) {
                DEBUG_LOG("[Call] ERROR: Too many arguments provided");
                console.error(formatError("Too many arguments provided to '" + callee + "'"));
                return nullptr;
            }
        }
    }

    DEBUG_LOG("[Call] Preparing arguments for CallExpression");
    std::vector<std::shared_ptr<Omniscript::Expression>> finalArgs;
    for (const auto& param : parameters) {
        auto val = localScope->get(param->name);
        val->name = param->name;
        finalArgs.push_back(val);
    }

    if (instanceName.empty()) {
        DEBUG_LOG("[Call] Returning CallExpression for '" + callee + "' with " + std::to_string(finalArgs.size()) + " args");
        return std::make_shared<Omniscript::CallExpression>(callee, finalArgs, type);
    }

    return std::make_shared<Omniscript::CallExpression>(callee, instanceName, finalArgs);
}

bool Call::matchArgumentsToParameters(
    const std::vector<std::shared_ptr<Omniscript::FunctionInputExpression>>& args,
    const std::vector<std::shared_ptr<Omniscript::FunctionInputExpression>>& params,
    SymbolTableType scope
) {
    DEBUG_LOG("[Call] Starting argument-to-parameter matching");

    std::unordered_set<std::string> matchedNames;
    std::unordered_map<std::string, std::shared_ptr<Omniscript::FunctionInputExpression>> namedArgs;
    std::vector<std::shared_ptr<Omniscript::FunctionInputExpression>> positionalArgs;

    // Separate named and positional args
    for (const auto& arg : args) {
        if (!arg->name.empty()) {
            DEBUG_LOG("[Call] Found named arg: " + arg->name);
            namedArgs[arg->name] = arg;
        } else {
            DEBUG_LOG("[Call] Found positional arg at index: " + std::to_string(positionalArgs.size()) + " of kind '" + arg->value->getRootType()->kindName() + "'.");
            positionalArgs.push_back(arg);
        }
    }

    size_t positionalIndex = 0;

    for (const auto& param : params) {
        const std::string& paramName = param->name;
        std::shared_ptr<Omniscript::FunctionInputExpression> matchingArg;

        DEBUG_LOG("[Call] Matching parameter: " + paramName);

        if (namedArgs.count(paramName)) {
            matchingArg = namedArgs[paramName];
            matchedNames.insert(paramName);
            DEBUG_LOG("[Call] Matched named argument: " + paramName);
        } else if (positionalIndex < positionalArgs.size()) {
            matchingArg = positionalArgs[positionalIndex++];
            DEBUG_LOG("[Call] Matched positional argument to parameter '" + paramName + "'");
        } else if (param->value) {
            DEBUG_LOG("[Call] No argument provided for '" + paramName + "', using default value");
            continue;
        } else {
            DEBUG_LOG("[Call] Missing required argument for parameter: " + paramName);
            return false;
        }

        if (!Omniscript::isSameOrCastableTo(matchingArg->value->getRootType(), param->getType())) {
            DEBUG_LOG("[Call] Type mismatch for parameter: " + paramName);
            return false;
        }
    }

    for (const auto& [name, _] : namedArgs) {
        if (matchedNames.count(name) == 0) {
            DEBUG_LOG("[Call] Unused named argument: " + name);
            return false;
        }
    }

    if (positionalIndex < positionalArgs.size()) {
        DEBUG_LOG("[Call] Too many positional arguments: expected " + std::to_string(positionalIndex) +
                  ", but got " + std::to_string(positionalArgs.size()));
        return false;
    }

    DEBUG_LOG("[Call] All arguments matched successfully");
    return true;
}

std::shared_ptr<Omniscript::Expression> ReturnStatement::express(SymbolTableType scope) {
    DEBUG_LOG("[Return] Creating a return value of kind '" + type->kindName() + "'.");
    std::shared_ptr<Omniscript::Expression> result = nullptr;
    if (returnValue) {
        if (auto stmt = std::dynamic_pointer_cast<TypedStatement>(returnValue)) {
            stmt->setType(type);
        }
        result = returnValue->express(scope);

        DEBUG_LOG("[Return] The result of the return value is '" + result->toString() + "' of kind '" + result->getType()->kindName() + "'.");
    } else {
        DEBUG_LOG("[Return] The result of the return value is 'void'.");
    }
    
    return std::make_shared<Omniscript::ReturnExpression>(result, type);
}

std::shared_ptr<Statement> ReturnStatement::evaluate(SymbolTableType scope) {
    DEBUG_LOG("[Evaluate Return Statement]");
    std::shared_ptr<Statement> result = returnValue->evaluate(scope);

    if (auto typed = std::dynamic_pointer_cast<TypedStatement>(result)) {
        return std::make_shared<ReturnStatement>(result, typed->getType());
    }
    return std::make_shared<ReturnStatement>(result);
}

bool ReturnStatement::hasSideEffects() {
    return !isCompileTimeEvaluatable();
}

bool ReturnStatement::isCompileTimeEvaluatable() {
    if (returnValue->isCompileTimeEvaluatable()) {
        return true;
    }
    return false;
}


std::shared_ptr<Omniscript::Expression> Array::express(SymbolTableType scope) {
    DEBUG_LOG("[Array] Creating an array");
    if (!type) {
        DEBUG_LOG("[Array] The array has no type");
        std::vector<std::shared_ptr<Omniscript::Expression>> values;
        std::shared_ptr<Omniscript::Type> inferredType = nullptr;
        bool allSameType = true;
        
        for (const auto& expr : initialValues) {
            auto val = expr->express(scope);
            if (!val) continue;
    
            auto valType = val->getType();
            if (!inferredType) {
                inferredType = valType; // Infer from first element
            } else if (valType->getKind() != inferredType->getKind()) {
                allSameType = false;
            }
            
            values.push_back(val);
        }
        
        if (!allSameType) {
            // Future: return DynamicArrayValue(values)
            DEBUG_LOG("[Array] Creating a Heterogeneous Dynamic Array");
            return nullptr;
        }
        setType(inferredType);
        DEBUG_LOG("[Array] Creating a fixed Array");
        return std::make_shared<Omniscript::FixedArrayExpression>(values, inferredType);
    }
    
    DEBUG_LOG("[Array] The array has a type '" + type->kindName() + "' of element types '" + type->elementType->kindName() + "'.");
    size_t n = 0;
    if (type->isFixedArray()) {
        DEBUG_LOG("[Array] Creating a fixed Array");
        std::vector<std::shared_ptr<Omniscript::Expression>> values;
        std::shared_ptr<Omniscript::Type> expectedElementType = type->elementType;  // Assume you have this method
        DEBUG_LOG("[Array] The expected element kind is '" + expectedElementType->kindName() + "'.");
        for (const auto& expr : initialValues) {
            std::shared_ptr<Omniscript::Expression> val;

            if (auto typed = std::dynamic_pointer_cast<TypedStatement>(expr)) {
                if (!typed->getType()) {
                    typed->setType(type->elementType);
                }
            }
            
            val = expr->express(scope);

            if (!val) continue;
            std::shared_ptr<Omniscript::Type> actualType = val->getType();

            bool isMatch = false;
            
            // Check for pointer type compatibility
            if (expectedElementType->isPointer() && actualType->isPointer()) {
                isMatch =
                    expectedElementType->getPointerDepth() == actualType->getPointerDepth() &&
                    expectedElementType->getBasePointeeType()->getKind() == actualType->getBasePointeeType()->getKind();
            }
            // Check for reference type compatibility
            else if (expectedElementType->isReference() && actualType->isReference()) {
                isMatch =
                    expectedElementType->getReferenceDepth() == actualType->getReferenceDepth() &&
                    expectedElementType->getBaseReferencedType()->getKind() == actualType->getBaseReferencedType()->getKind();
            }
            // Fallback to simple kind match
            else {
                isMatch = actualType->getKind() == expectedElementType->getKind();
            }


            if (!isMatch) {
                if (type->isPointer()) {
                    console.error("Array element is of type " + actualType->pointerDescription() +
                                  " but expected type " + expectedElementType->pointerDescription());
                } else {
                    console.error("Array element is of type " + actualType->kindName() +
                                  " but expected type " + expectedElementType->kindName());
                }
            }

            DEBUG_LOG("[Array] Value '" + std::to_string(n) + "' is '" + expectedElementType->kindName() + " " + val->toString() + "'.");
            values.push_back(val);
            n++;
        }
        
        return std::make_shared<Omniscript::FixedArrayExpression>(values, expectedElementType);
    }
    
    if (type->isDynamicArray()) {
        DEBUG_LOG("[Array] Creating a dynamic Array");
        // Dynamic arrays logic here
        return nullptr;
    }
    
    if (type->isHeterogeneousArray()) {
        DEBUG_LOG("[Array] Creating a heterogeneous dynamic Array");
        // Heterogeneous arrays logic here
        return nullptr;
    }

    return nullptr;
}

std::shared_ptr<Omniscript::Expression> FunctionDeclaration::express(SymbolTableType scope) {
    DEBUG_LOG();
    DEBUG_LOG("[Function] Constructing a function " + name + " prototype the return Type is '" + type->kindName() + "'.");

    DEBUG_LOG("[Function] Creating a local scope for the function");
    localScope = scope->createChildScope(name);

    if (name == "main") {
        name = "__main";
    }

    std::vector<std::shared_ptr<Omniscript::TypeExpression>> genericTypes = createTypeExpressionListFromBoundGenerics();
    for (const auto& genericType : genericTypes) {
        localScope->setConstant(genericType->name, genericType);
    }

    DEBUG_LOG("[Function] Setting the function's return type");
    if (!type) {
        std::vector<std::string> retType = {"void"};
        type = Omniscript::resolveType(retType);
        returnType = type;
    } else if (type->isGeneric()) {
        type = resolveGeneric(type->getName());
        returnType = type;
    }

    DEBUG_LOG("[Function] Setting the function's body's return type to " + type->kindName());
    if (auto typed = std::dynamic_pointer_cast<TypedStatement>(body)) {
        typed->setType(returnType);
    }

    setReturnTypes();

    DEBUG_LOG("[Function] Extracting argument values for function type construction");
    std::vector<std::shared_ptr<Omniscript::Expression>> argValues;
    bool isVarArg = false;

    for (const auto& param : parameters) {
        DEBUG_LOG("[Function] The parameter is '" + param->toString() + "'.");
        if (auto typed = std::dynamic_pointer_cast<TypedStatement>(param)) {
            auto paramType = typed->getType();
            DEBUG_LOG("[Function] Parameter has type '" + paramType->kindName() + "'.");

            if (paramType->isGeneric()) {
                typed->setType(std::move(resolveGeneric(paramType->getName())));
            }
        }
        auto result = param->express(localScope);
        argValues.push_back(result);
        DEBUG_LOG("[Function] Parameter '" + result->name + "' has type " + result->getType()->kindName());
    }

    DEBUG_LOG("[Function] Passing generic type bindings from function to body block");
    if (auto holder = std::dynamic_pointer_cast<GenericHolder>(body)) {
        holder->inheritGenericsFrom(*this);
    }

    // auto bod = body->resolveExpressions(localScope);

    std::vector<std::shared_ptr<Omniscript::Expression>> functionBody = body->expressAsVector(localScope);
    // for (auto& stmt : body->statements) {
    //     functionBody.push_back(stmt->express(localScope));
    // }

    std::string mangledName = (name == "__main" ? "__main" : generateMangledName());

    DEBUG_LOG("[Function] Creating FunctionValue");
    auto functionVal = std::make_shared<Omniscript::FunctionExpression>(name, mangledName, returnType, functionBody, argValues, isVarArg);

    DEBUG_LOG("[Function] Storing overloaded function in scope '" + scope->getName() + "' under base name: " + name + " (mangled as: " + mangledName + ")");
    scope->addOverloadable(name, functionVal);

    return functionVal;
}

std::string FunctionDeclaration::generateMangledName() const {
    std::string mangled = name + "(";
    for (size_t i = 0; i < parameters.size(); ++i) {
        if (auto typed = std::dynamic_pointer_cast<TypedStatement>(parameters[i])) {
            auto paramType = typed->getType();
            mangled += paramType ? paramType->kindName() : "unknown";
        } else {
            mangled += "any";
        }
        if (i < parameters.size() - 1) mangled += ",";
    }
    mangled += ")";
    return mangled;
}

void FunctionDeclaration::setReturnTypes() {
    DEBUG_LOG("[Function] Setting the function's return type to " + returnType->kindName());
    std::shared_ptr<Omniscript::Type> funcReturnType = getType();

    for (const auto& stmt : body->statements) {
        setReturnTypesInStatement(stmt, funcReturnType);
    }
}

void FunctionDeclaration::setReturnTypesInStatement(
    const std::shared_ptr<Statement>& stmt, 
    std::shared_ptr<Omniscript::Type> returnType
) {
    if (auto retStmt = std::dynamic_pointer_cast<ReturnStatement>(stmt)) {
        retStmt->setType(returnType);
        return;
    }

    if (auto block = std::dynamic_pointer_cast<BlockStatement>(stmt)) {
        for (const auto& subStmt : block->statements) {
            setReturnTypesInStatement(subStmt, returnType);
        }
    }
    // else if (auto ifStmt = std::dynamic_pointer_cast<IfStatement>(stmt)) {
    //     if (ifStmt->thenBranch)
    //         setReturnTypesInStatement(ifStmt->thenBranch, returnType);
    //     if (ifStmt->elseBranch)
    //         setReturnTypesInStatement(ifStmt->elseBranch, returnType);
    // }
    // else if (auto whileStmt = std::dynamic_pointer_cast<WhileStatement>(stmt)) {
    //     if (whileStmt->body)
    //         setReturnTypesInStatement(whileStmt->body, returnType);
    // }
    // else if (auto forStmt = std::dynamic_pointer_cast<ForStatement>(stmt)) {
    //     if (forStmt->body)
    //         setReturnTypesInStatement(forStmt->body, returnType);
    // }
}


std::shared_ptr<Omniscript::Expression> ParameterStatement::express(SymbolTableType scope) {
    DEBUG_LOG("[Parameter] Creating parameter " + name + " of kind " + type->kindName());
    if (auto stmt = std::dynamic_pointer_cast<TypedStatement>(defaultValue)) {
        stmt->setType(type);
    }

    std::shared_ptr<Omniscript::Expression> result;
    
    if (defaultValue) {
        result = defaultValue->express(scope);
    } else {
        if (type->isPointer()) {
            result = std::make_shared<Omniscript::NullPointerExpression>(type);
        } else {
            result = std::make_shared<Omniscript::NullExpression>(type);
        }
    }
    DEBUG_LOG("[Parameter] Created value for parameter " + name + " of kind " + result->getType()->kindName());
    return std::make_shared<Omniscript::FunctionInputExpression>(name, type, result, isConstant);
}

std::shared_ptr<Omniscript::Expression> ArgumentStatement::express(SymbolTableType scope) {
    DEBUG_LOG("[Argument] Creating argument " + name);
    if (auto stmt = std::dynamic_pointer_cast<TypedStatement>(value)) {
        stmt->setType(type);
    }
    std::shared_ptr<Omniscript::Expression> result = value->express(scope);
    DEBUG_LOG("[Argument] The value for argument '" + name + "' is " + result->toString());
    return std::make_shared<Omniscript::FunctionInputExpression>(name, type, result);
}

std::shared_ptr<Statement> ParameterStatement::getDefaultValue() {
    // if (auto stmt = std::dynamic_pointer_cast<TypedStatement>(defaultValue)) {
    //     stmt->setType(type);
    // }
    // return defaultValue;
    return nullptr;
}

std::shared_ptr<Omniscript::Expression> ConstructStructPrototype::express(SymbolTableType scope) {
    DEBUG_LOG("[ConstructStructPrototype] Constructing a struct expression");

    std::vector<std::shared_ptr<Omniscript::Expression>> fields;
    std::vector<std::shared_ptr<Omniscript::Type>> fieldTypes;
    std::vector<std::string> fieldNames;

    for (const auto& field : body) {
        if (auto paramDecl = std::dynamic_pointer_cast<ParameterStatement>(field)) {
            std::string fieldName = paramDecl->getName();
            fieldNames.push_back(fieldName);

            std::shared_ptr<Omniscript::Expression> fieldExpr = paramDecl->express(scope);

            fields.push_back(fieldExpr);
            fieldExpr->getType()->parameterName = fieldName;
            fieldTypes.push_back(fieldExpr->getType());
            DEBUG_LOG("Parameter '" + fieldName + "' has type " + fieldExpr->getType()->kindName());
        } else {
            DEBUG_LOG("Skipping non-variable declaration in struct body");
        }
    }

    auto structType = Omniscript::Type::createUserDefinedType(name, Omniscript::Kind::Struct, fieldTypes);
    scope->addType(name, structType);
    
    for (const auto& field : body) {
        if (auto methodStmt = std::dynamic_pointer_cast<FunctionDeclaration>(field)) {
            auto thisParam = std::make_shared<ParameterStatement>("this");
            thisParam->setType(scope->getType(name));
            methodStmt->parameters.insert(methodStmt->parameters.begin(), std::dynamic_pointer_cast<Statement>(thisParam));
            std::shared_ptr<Omniscript::Expression> method = methodStmt->express(scope);
            fields.push_back(method);
        } else {
            DEBUG_LOG("Skipping non-method declaration in struct body");
        }
    }
    
    // 🧱 Construct the StructExpression as a Callable
    auto structExpr = std::make_shared<Omniscript::StructExpression>(
        getName(),
        getName(),
        fields,
        fieldNames,
        /* isVarArg */ false
    );

    scope->set(getName(), structExpr);

    return structExpr;
}
 

std::shared_ptr<Omniscript::Expression> ObjectConstructorStatement::express(SymbolTableType scope) {
    DEBUG_LOG("Constructing object: " + objectType + " " + instanceName);

    // std::vector<std::shared_ptr<Omniscript::Expression>> argValues;
    // for (const auto& arg : constructorArgs) {
    //     argValues.push_back(arg->express(scope));
    // }

    if (scope->getType(objectType)) {
        type = std::make_shared<Omniscript::UserDefinedType>(objectType);
        auto constructorCall = std::make_shared<Call>(objectType, instanceName, constructorArgs);
        auto call = std::dynamic_pointer_cast<Omniscript::CallExpression>(constructorCall->express(scope));

        auto instance = std::make_shared<Omniscript::InstanceExpression>(
            objectType,
            instanceName,
            call->args
        );

        instance->instanceType = scope->getType(objectType);
        instance->type = scope->getType(objectType);
        scope->set(instanceName, instance);
        return call;
    } else {
        console.error("Object type was not found in the scope");
    }

    // // 5. Return the allocated instance
    // return std::make_shared<Omniscript::CallExpression>(callee, finalArgs, type);
    return nullptr;
    // return std::make_shared<Omniscript::CallExpression>(objectType, instanceName, argValues); //, type);
}

std::shared_ptr<Omniscript::Expression> MemberAccess::express(SymbolTableType scope) {
    // Get base type name from the object
    DEBUG_LOG("The object name is '" + objectName + "' of type " + scope->get(objectName)->getType()->getName());
    std::string baseTypeName = scope->get(objectName)->getType()->kindName();
    std::shared_ptr<Omniscript::Type> baseType = scope->getType(baseTypeName);

    if (!baseType) {
        console.error("Could not find base type '" + baseTypeName + "'.");
        return nullptr;
    }

    std::shared_ptr<Omniscript::Type> currentType = baseType;

    for (const auto& member : propertyPath) {
        auto userType = std::dynamic_pointer_cast<Omniscript::UserDefinedType>(currentType);
        if (!userType) {
            console.error("Type '" + currentType->kindName() + "' is not a struct or does not have members.");
            return nullptr;
        }

        bool found = false;
        for (const auto& field : userType->paramTypes) {
            DEBUG_LOG("Field parameter name is " + field->getParameterName());
            if (field->getParameterName() == member) {
                currentType = field; // Move deeper into the chain
                found = true;
                break;
            }
        }

        if (!found) {
            console.error("Member '" + member + "' not found in type '" + userType->kindName() + "'.");
            return nullptr;
        }
    }

    // At this point, `currentType` is the final member's type
    // You can now use it as `memberType` if needed
    std::shared_ptr<Omniscript::Type> memberType = currentType;
    DEBUG_LOG("Member '" + propertyPath[propertyPath.size() - 1] + "' has type " + currentType->kindName());
    setType(currentType);

    if (assignmentValue) {
        DEBUG_LOG("Setting '" + this->toString() + "' to '" + assignmentValue->toString() + "'.");
        auto result = assignmentValue->express(scope);
    
        if (auto instance = std::dynamic_pointer_cast<Omniscript::InstanceExpression>(scope->get(objectName))) {
            // Traverse into the instance fields
            std::shared_ptr<Omniscript::InstanceExpression> currentInstance = instance;
            std::shared_ptr<Omniscript::InstanceExpression> parentInstance = nullptr;
            std::string lastMember;
    
            for (const auto& member : propertyPath) {
                parentInstance = currentInstance;
                lastMember = member;
    
                auto subInstance = currentInstance->getField(member);
                if (!subInstance) {
                    console.error("Member '" + member + "' not found on instance of '" + currentInstance->getType()->kindName() + "'.");
                    return nullptr;
                }
    
                currentInstance = std::dynamic_pointer_cast<Omniscript::InstanceExpression>(subInstance);
                if (!currentInstance) {
                    break; // Might be at the leaf now
                }
            }
    
            // Actually set the value now
            if (parentInstance) {
                parentInstance->setField(lastMember, result);
            } else {
                console.error("Unable to set field: no valid parent instance found.");
                return nullptr;
            }
    
            return std::make_shared<Omniscript::MemberAccessExpression>(baseTypeName, objectName, propertyPath, currentType, result);
        } else {
            console.error("Object '" + objectName + "' is not an instance.");
            return nullptr;
        }
    } else {
        DEBUG_LOG("Getting '" + this->toString() + "'.");
        return std::make_shared<Omniscript::MemberAccessExpression>(baseTypeName, objectName, propertyPath, currentType);
    }
}


std::shared_ptr<Omniscript::Expression> EnumValue::express(SymbolTableType scope) {
    return std::make_shared<IntegerLiteral>(valueIndex)->express(scope);
}

std::shared_ptr<Omniscript::Expression> EnumConstructor::express(SymbolTableType scope) {
    auto expr = std::make_shared<Omniscript::EnumExpression>(name, hasLookup, isEnumClass);
    
    for (const auto& val : values) {
        expr->addEntry(val->getIndex(), val->getName(), val->express(scope));
    }

    return expr;
}



// // Helper function to extract values from statements
// std::optional<SymbolTable::ValueType> Expression::evaluate(const SymbolTable::ValueType& object, SymbolTable &scope) {
    //     // Variable to store the result
    //     SymbolTable::ValueType result;

    //     // Check if the object holds a shared pointer to a Statement
//     if (auto statement = std::get_if<std::shared_ptr<Statement>>(&object)) {
//         std::shared_ptr<Statement> stmt = *statement;
//         Omniscript::setPosition(stmt->getPosition());

//         // Process different types of statements
//         if (auto valueStatement = std::dynamic_pointer_cast<Value>(stmt)) {
//             result = valueStatement->express(scope);
//         } else if (auto varStatement = std::dynamic_pointer_cast<Variable>(stmt)) {
//             result = varStatement->express(scope);
//         } else if (auto constAssign = std::dynamic_pointer_cast<ConstantAssignment>(stmt)) {
//             constAssign->execute(scope);
//             result = SymbolTable::ValueType{};
//         } else if (auto constAssign = std::dynamic_pointer_cast<GenericAssignment>(stmt)) {
//             constAssign->execute(scope);
//             result = SymbolTable::ValueType{};
//         } else if (auto block = std::dynamic_pointer_cast<BlockStatement>(stmt)) {
//             block->execute(scope);
//             result = SymbolTable::ValueType{};
//         } else if (auto assign = std::dynamic_pointer_cast<Assignment>(stmt)) {
//             assign->execute(scope);
//             result = SymbolTable::ValueType{};
//         } else if (auto constructor = std::dynamic_pointer_cast<ObjectConstructorStatement>(stmt)) {
//             result = constructor->express(scope);
//         } else if (auto destructor = std::dynamic_pointer_cast<ObjectDestructorStatement>(stmt)) {
//             destructor->execute(scope);
//             result = SymbolTable::ValueType{};
//         } else if (auto functionCall = std::dynamic_pointer_cast<FunctionCallStatement>(stmt)) {
//             result = functionCall->express(scope);
//         } else if (auto returnStatement = std::dynamic_pointer_cast<ReturnStatement>(stmt)) {
//             result = returnStatement->express(scope);
//         } else if (auto ifStatement = std::dynamic_pointer_cast<IfStatement>(stmt)) {
//             result = ifStatement->express(scope);
//         } else if (auto whileLoopStatement = std::dynamic_pointer_cast<WhileStatement>(stmt)) {
//             result = whileLoopStatement->express(scope);
//         } else if (auto forLoopStatement = std::dynamic_pointer_cast<ForLoop>(stmt)) {
//             result = forLoopStatement->express(scope);
//         } else if (auto unaryExpr = std::dynamic_pointer_cast<UnaryExpression>(stmt)) {
//             result = unaryExpr->express(scope).value();
//         } else if (auto binaryExpr = std::dynamic_pointer_cast<BinaryExpression>(stmt)) {
//             auto evalResult = binaryExpr->express(scope);
//             if (evalResult.has_value()) {
//                 DEBUG_LOG("The Expression's result is " + valueToString(evalResult.value()));
//                 result = evalResult.value();
//             } else {
//                 std::cerr << "Error: Binary expression returned an empty optional!" << std::endl;
//                 return std::nullopt;
//             }
//         } else if (auto ternaryExpression = std::dynamic_pointer_cast<TenaryExpression>(stmt)) {
//             result = ternaryExpression->express(scope);
//         } else if (auto methodCall = std::dynamic_pointer_cast<CallMethod>(stmt)) {
//             result = methodCall->express(scope);
//         } else if (auto propertyCall = std::dynamic_pointer_cast<GetProperty>(stmt)) {
//             result = propertyCall->express(scope);
//         } else {
//             // Error handling for unknown statement types
//             std::cerr << "Error: Unknown statement type encountered." << std::endl;
//             return std::nullopt;
//         }
//     }  else {
//         return object;
//         // console.error("Error: Unsupported type encountered " + valueToString(object) + ".");
//     }

//     // Ensure the result does not contain a shared_ptr<Statement>
//     if (std::holds_alternative<std::shared_ptr<Statement>>(result)) {
//         auto stmt = std::get<std::shared_ptr<Statement>>(result);
//         return Expression::evaluate(stmt, scope).value();
//     }

//     return result;
// }

// SymbolTable::ValueType Value::evaluate(SymbolTable &scope) {

//     if (auto object = std::get_if<std::shared_ptr<Statement>>(&value)) {
//         return Expression::evaluate(object, scope).value();
//     }
//     return value;
// }

// void Assignment::execute(SymbolTable &scope) {
//     // Check if value is a shared_ptr to a Statement
//     if (value.has_value() && std::holds_alternative<std::shared_ptr<Statement>>(*value)) {
//         auto statementPtr = std::get<std::shared_ptr<Statement>>(*value);
//         auto result = Expression::evaluate(statementPtr, scope);
//         scope->set(variable, result);
//     } else {
//         scope->set(variable, value);
//     }

//     if (value.has_value()) { // Check if value is present
//         DEBUG_LOG("Assigned variable '" + variable + "' with value " + valueToString(value.value()) + " in scope " + scope->name);
//     } else {
//         DEBUG_LOG("Assigned variable " + variable + " with no value (nullopt) in scope " + scope->name);
//     }
// }

// void ConstantAssignment::execute(SymbolTable &scope) {
//     tempValue = value;
//     // Check if value is a shared_ptr to a Statement
//     if (value.has_value() && std::holds_alternative<std::shared_ptr<Statement>>(*value)) {
//         auto statementPtr = std::get<std::shared_ptr<Statement>>(*value);
//         value = Expression::evaluate(statementPtr, scope).value();
//     }
//     scope->setConstant(variable, value);
//     if (value.has_value()) { // Check if value is present
//         DEBUG_LOG("Assigned constant '" + variable + "' with value " + valueToString(value.value()));
//     } else {
//         DEBUG_LOG("Assigned constant " + variable + " with no value (nullopt)");
//     }
// }

// void GenericAssignment::execute(SymbolTable &scope) {
//     tempValue = value;
//     // Check if value is a shared_ptr to a Statement
//     if (value.has_value() && std::holds_alternative<std::shared_ptr<Statement>>(*value)) {
//         auto statementPtr = std::get<std::shared_ptr<Statement>>(*value);
//         value = Expression::evaluate(statementPtr, scope).value();
//     }

//     auto func = std::get<std::shared_ptr<Function>>(value.value());
//     scope->addGenericFunction(variable, func);
//     if (value.has_value()) { // Check if value is present
//         DEBUG_LOG("Assigned generic function constant '" + variable + "' with value " + valueToString(value.value()));
//     } else {
//         DEBUG_LOG("Assigned generic function constant " + variable + " with no value (nullopt)");
//     }
// }

// SymbolTable::ValueType Variable::evaluate(SymbolTable &scope) {
//     if (std::holds_alternative<std::string>(variable)) {
//         auto varName = std::get<std::string>(variable);
//         auto variableValue = scope->get(varName);

//         if (variableValue.has_value()) {
//             return variableValue.value();
//         }
//     }
//     // TODO::
//     // else {}

//     return SymbolTable::ValueType{}; // Safe after the check
// }

// SymbolTable::ValueType ReturnStatement::evaluate(SymbolTable &scope) {
//     // Retrieve the return value based on the type in variableReturnValues
//     SymbolTable::ValueType result;

//     // Check if there's a return value
//     if (returnValue.has_value()) {
//         result = returnValue.value();
//         // If the return value is a shared pointer to a statement (like Variable)
//         if (auto statementPtr = std::get_if<std::shared_ptr<Statement>>(&*returnValue)) {
//             DEBUG_LOG("Returning value: " + valueToString(returnValue.value()));
//             result = Expression::evaluate(*statementPtr, scope).value();  // Use helper to evaluate expressions
//         }
//     } else {
//         DEBUG_LOG("Returning no value (void)");  // Handle the void case
//         return SymbolTable::ValueType{};
//     }
    
//     return result;
// }


// SymbolTable::ValueType FunctionCallStatement::evaluate(SymbolTable &scope) {
//     showDebugSection("Evaluating a function call");

//     std::shared_ptr<Function> functionPtr = nullptr;
    
//     if (specializedName != "") {
//         console.log("here 1");
//         std::shared_ptr<Function> tempFunctionPtr = scope->getFunction(specializedName);
//         if (tempFunctionPtr) {
//             console.log("here 2");
//             functionPtr = tempFunctionPtr;
//         } else {
//             console.log("here 3");
//             std::string modifiedBaseName = baseName;
//             for (const auto type : types) {
//                 modifiedBaseName += "_any";
//             }
//             console.log(modifiedBaseName);
//             std::shared_ptr<Function> baseGenericFunction = scope->getGenericFunction(modifiedBaseName);
//             if (baseGenericFunction) {
//                 auto genericFunction = baseGenericFunction->clone();
//                 baseGenericFunction->name = specializedName;
//                 scope->addFunction(specializedName, genericFunction);
//                 functionPtr = genericFunction;
//             }
//         }
//     } 
//     // Check if `func` is already a function
//     else if (auto tempFunc = std::get_if<std::shared_ptr<Function>>(&func)) {
//         functionPtr = *tempFunc;
//     }
//     // If `func` is a string, treat it as a function name and look it up
//     else if (auto funcName = std::get_if<std::string>(&func)) {
//         functionPtr = scope->getFunction(*funcName);
//     }
//     // If `func` is an expression, evaluate it and check if it resolves to a function
//     else {
//         auto evaluatedFunc = Expression::evaluate(func, scope);
//         if (evaluatedFunc) {
//             if (auto tempFunc = std::get_if<std::shared_ptr<Function>>(&evaluatedFunc.value())) {
//                 functionPtr = *tempFunc;
//             }
//         }
//     }
    
//     console.log("here 4");
    
//     // Ensure function exists before calling
//     if (!functionPtr) {
//         console.error("Function call failed: function not found or invalid.");
//     }
    
//     // Evaluate function arguments
//     console.log("here 5");
//     std::vector<SymbolTable::ValueType> evaluatedArgs;
//     for (const auto& arg : args) {
//         evaluatedArgs.push_back(Expression::evaluate(arg, scope).value());
//     }
//     console.log("here 6");

//     // Call function and return result
//     return functionPtr->express(scope, evaluatedArgs);
// }

// void BlockStatement::execute(SymbolTable &scope) {

//     for (const auto& statement : statements) {
//         Expression::evaluate(statement, scope);
//     }
// }

// SymbolTable::ValueType IfStatement::evaluate(SymbolTable &scope) {
//     showDebugSection("Executing if statement");
//     SymbolTable localScope;
//     localscope->name = "An if statement's scope";
//     localscope->setParent(&scope);

//     if (conditionIsMet(scope)) {
//         for (const auto &stmnt : body) {
//             auto result = Expression::evaluate(stmnt, localScope);

//             if (auto returnStatement = std::dynamic_pointer_cast<ReturnStatement>(stmnt)) {
//                 return result.value(); // Propagate the return value
//             }

//             if (result != SymbolTable::ValueType{}) {
//                 return result.value();
//             }
//         }
//         return SymbolTable::ValueType{};
//     }

//     for (const auto &branch : branches) {
//         // Execute the appropriate branch based on the condition result
//         if (branch->conditionIsMet(scope)) {
//             for (const auto &stmnt : branch->body) {
//                 auto result = Expression::evaluate(stmnt, localScope);

//                 if (auto returnStatement = std::dynamic_pointer_cast<ReturnStatement>(stmnt)) {
//                     return result.value(); // Propagate the return value
//                 }

//                 if (result != SymbolTable::ValueType{}) {
//                     return result.value();
//                 }
//             }
//             return SymbolTable::ValueType{};
//         }
//     }

//     for (const auto &stmnt : falseBranch) {
//         auto result = Expression::evaluate(stmnt, localScope);

//         if (auto returnStatement = std::dynamic_pointer_cast<ReturnStatement>(stmnt)) {
//             return result.value(); // Propagate the return value
//         }

//         if (result != SymbolTable::ValueType{}) {
//             return result.value();
//         }
//     }

//     return SymbolTable::ValueType{};
// }

// SymbolTable::ValueType ForLoop::express(SymbolTableType scope) {
//     SymbolTable localScope;
//     localscope->name = "a for loop's scope";
//     localscope->setParent(&scope);
//     showDebugSection("Executing a for loop");
//     // Execute initialization
//     Expression::evaluate(initialization, localScope);
//     // Loop while condition evaluates to true
//     while (std::get<bool>(Expression::evaluate(condition, localScope).value())) {
//         // Execute body of the loop
//         for (auto& stmnt : body) {
//             // Check if the statement is a ReturnStatement
//             auto result = Expression::evaluate(stmnt, localScope);
//             if (auto returnStatement = std::dynamic_pointer_cast<ReturnStatement>(stmnt)) {
//                 return result.value(); // Propagate the return value
//             }
//             // Check for 'break' statement
//             if (auto breakStmt = std::dynamic_pointer_cast<BreakStatement>(stmnt)) {
//                 return SymbolTable::ValueType{};  // Exit the loop and function
//             }
//             // Check for 'continue' statement
//             if (auto continueStmt = std::dynamic_pointer_cast<ContinueStatement>(stmnt)) {
//                 Expression::evaluate(increment, localScope); // Skip the current iteration and continue the loop
//                 continue;
//             }
//             // If a statement evaluates to a return value, exit the loop
//             if (result != SymbolTable::ValueType{}) {
//                 return result.value();
//             }
//         }
//         // Execute increment expression after each iteration
//         Expression::evaluate(increment, localScope);
//     }
//     // Return default value if no explicit return was encountered
//     return SymbolTable::ValueType{};
// }

// SymbolTable::ValueType WhileStatement::evaluate(SymbolTable &scope) {
//     // Use the Expression::evaluate() method to evaluate the condition
//     showDebugSection("Executing a while loop");
//     SymbolTable localScope;
//     localscope->name = "a while loop's scope";
//     localscope->setParent(&scope);

//     DEBUG_LOG("Checking if the condition of the while loop was met");
//     // Continue executing the loop as long as the condition evaluates to true
//     while (std::get<bool>(Expression::evaluate(condition, scope).value())) {
//         DEBUG_LOG("executing the statements in the while loop");
//         for (auto &stmnt : body) {
//             auto result = Expression::evaluate(stmnt, localScope);

//             if (auto returnStatement = std::dynamic_pointer_cast<ReturnStatement>(stmnt)) {
//                 return result.value(); // Propagate the return value
//             }

//             if (result != SymbolTable::ValueType{}) {
//                 return result.value();
//             }
//         }
//     }
//     DEBUG_LOG("done executing the statements in the while loop");
//     return SymbolTable::ValueType{};
// }

// //If statements
// // Checks if the conditional in an if statement is met
// bool IfStatement::conditionIsMet(SymbolTable &scope) {
//     // Evaluate the condition, assuming `BinaryExpression` returns a boolean-like result
//     // auto conditionExpr = std::dynamic_pointer_cast<BinaryExpression>(condition);
//     auto conditionResult = Expression::evaluate(condition, scope); //conditionExpr->express(scope);

//     // Check if the condition has a value and convert it to bool
//     bool result = conditionResult.has_value() && std::holds_alternative<bool>(conditionResult.value()) 
//     ? std::get<bool>(conditionResult.value())
//     : false; // default to false if no value or invalid type

//     return result;
// }

// template <typename T>
// bool compare_symbol_variant(const SymbolTable::ValueType& variant, const T& rightElement) {
//     if constexpr (std::is_same_v<std::decay_t<T>, std::string>) {
//         if (auto* str = std::get_if<std::string>(&variant)) {
//             return *str == rightElement;
//         }
//     } else if constexpr (std::is_same_v<std::decay_t<T>, bool>) {
//         if (auto* b = std::get_if<bool>(&variant)) {
//             return *b == rightElement;
//         }
//     } else if constexpr (std::is_same_v<std::decay_t<T>, std::shared_ptr<Array>>) {
//         if (auto* arr = std::get_if<std::shared_ptr<Array>>(&variant)) {
//             return *arr == rightElement;
//         }
//     } else if constexpr (std::is_same_v<std::decay_t<T>, std::shared_ptr<Statement>>) {
//         if (auto* stmt = std::get_if<std::shared_ptr<Statement>>(&variant)) {
//             return *stmt == rightElement;
//         }
//     } else if constexpr (std::is_same_v<std::decay_t<T>, std::vector<std::shared_ptr<Statement>>>) {
//         if (auto* stmtVec = std::get_if<std::vector<std::shared_ptr<Statement>>>(&variant)) {
//             return *stmtVec == rightElement;
//         }
//     } else if constexpr (std::is_same_v<std::decay_t<T>, std::shared_ptr<Object>>) {
//         if (auto* obj = std::get_if<std::shared_ptr<Object>>(&variant)) {
//             return *obj == rightElement;
//         }
//     } else if constexpr (std::is_same_v<std::decay_t<T>, std::vector<std::shared_ptr<Object>>>) {
//         if (auto* objVec = std::get_if<std::vector<std::shared_ptr<Object>>>(&variant)) {
//             return *objVec == rightElement;
//         }
//     } else if constexpr (std::is_same_v<std::decay_t<T>, int>) {
//         if (auto* num = std::get_if<int>(&variant)) {
//             return *num == rightElement;
//         }
//     } else if constexpr (std::is_same_v<std::decay_t<T>, unsigned int>) {
//         if (auto* num = std::get_if<unsigned int>(&variant)) {
//             return *num == rightElement;
//         }
//     } else if constexpr (std::is_same_v<std::decay_t<T>, long>) {
//         if (auto* num = std::get_if<long>(&variant)) {
//             return *num == rightElement;
//         }
//     } else if constexpr (std::is_same_v<std::decay_t<T>, unsigned long>) {
//         if (auto* num = std::get_if<unsigned long>(&variant)) {
//             return *num == rightElement;
//         }
//     } else if constexpr (std::is_same_v<std::decay_t<T>, long long>) {
//         if (auto* num = std::get_if<long long>(&variant)) {
//             return *num == rightElement;
//         }
//     } else if constexpr (std::is_same_v<std::decay_t<T>, unsigned long long>) {
//         if (auto* num = std::get_if<unsigned long long>(&variant)) {
//             return *num == rightElement;
//         }
//     } else if constexpr (std::is_same_v<std::decay_t<T>, float>) {
//         if (auto* num = std::get_if<float>(&variant)) {
//             return *num == rightElement;
//         }
//     } else if constexpr (std::is_same_v<std::decay_t<T>, double>) {
//         if (auto* num = std::get_if<double>(&variant)) {
//             return *num == rightElement;
//         }
//     } else if constexpr (std::is_same_v<std::decay_t<T>, long double>) {
//         if (auto* num = std::get_if<long double>(&variant)) {
//             return *num == rightElement;
//         }
//     } else if constexpr (std::is_same_v<std::decay_t<T>, void*>) {
//         if (auto* ptr = std::get_if<void*>(&variant)) {
//             return *ptr == rightElement;
//         }
//     }

//     // If types don't match, return false
//     return false;
// }

// template <typename T>
// bool compare_element(const SymbolTable::ValueType& element, const T& rightElement) {
//     return compare_symbol_variant(element, rightElement);
// }

// std::optional<SymbolTable::ValueType> UnaryExpression::evaluate(SymbolTable &scope) {
//     return SymbolTable::ValueType{};
// }

// // Binary Expression methods
// std::optional<SymbolTable::ValueType> BinaryExpression::evaluate(SymbolTable &scope) {
//     // DEBUG_LOG("The left is " + valueToString(isTruthy(left)) + " " + getOperatorString(op) + " " + valueToString(isTruthy(right)));
//     if (!isTruthy(left) && isTruthy(right)) { // If there is no left statement, treat it as unary (e.g., unary minus)
//         auto operandValue = Expression::evaluate(right, scope);  

//         // Check if the operand is present
//         if (!operandValue.has_value()) {
//             console.error("Unary operand missing value.");
//         }

//         // Handle unary minus (negation) -1
//         if (op == TokenTypes::Minus) {
//             if (std::holds_alternative<int>(operandValue.value())) {
//                 return -std::get<int>(operandValue.value());  // Apply unary minus
//             } else if (std::holds_alternative<float>(operandValue.value())) {
//                 return -std::get<float>(operandValue.value());  // Apply unary minus
//             } else {
//                 console.error("Unsupported operand type for unary minus.");
//             }
//         } else if (std::holds_alternative<float>(operandValue.value())) { // Handle if the operand is a float
//             return -std::get<float>(operandValue.value());  // Apply unary minus
//         } else {
//             console.error("Unsupported operand type for unary minus.");
//         }

//     // If there is no right statement, treat it as unary (e.g., increment or decrement)
//     } else if (isTruthy(left) && (op == TokenTypes::Increment || op == TokenTypes::Decrement) && !isTruthy(right)) {
//         auto leftValue = Expression::evaluate(left, scope);
//         if (!leftValue.has_value()) {
//             console.error("Unary expression operand missing value.");
//         }

//         // Apply the unary operator (either increment or decrement)
//         switch (op) {
//             case TokenTypes::Increment: {
//                 if (std::holds_alternative<int>(leftValue.value())) {
//                     int value = std::get<int>(leftValue.value());
//                     return value + 1;  // Increment operation
//                 }
//                 break;
//             }
//             case TokenTypes::Decrement: {
//                 if (std::holds_alternative<int>(leftValue.value())) {
//                     int value = std::get<int>(leftValue.value());
//                     return value - 1;  // Decrement operation
//                 }
//                 break;
//             }
//             default:
//                 console.error("Unsupported unary operator." + getTokenTypeName(op));
//         }
//     // Handle booleans
//     } else if (std::holds_alternative<bool>(left) && op == TokenTypes::Null && !isTruthy(right)) { 
//         if (std::get<bool>(left) == true) { // Extract the bool value and compare
//             return true;
//         }
//         return false;
//     }
//     else if (isTruthy(left) && op == TokenTypes::Null && !isTruthy(right)) {
//         auto result = Expression::evaluate(left, scope).value();
//         return result;
//     } else {
//         // Handle binary operations 
//         // Retrieve values from evaluated statements
//         auto leftValueOpt = Expression::evaluate(left, scope);
//         auto rightValueOpt = Expression::evaluate(right, scope);

//         // Ensure both values are present
//         if (!leftValueOpt || !rightValueOpt) {
//             console.error("Binary expression operands are missing values.");
//         }

//         auto leftValue = leftValueOpt.value();
//         auto rightValue = rightValueOpt.value();

//         // Debug output for the entire statement
//         DEBUG_LOG("The statement is '" + valueToString(leftValue) + " " + getOperatorString(op) + " " + valueToString(rightValue) + "'");

//         // Handle numeric operations using std::visit
//         auto handleNumericOperation = [&](auto&& left, auto&& right) -> SymbolTable::ValueType {
//             using LeftType = std::decay_t<decltype(left)>;
//             using RightType = std::decay_t<decltype(right)>;

//             // Ensure both are numeric
//             if constexpr (std::is_arithmetic_v<LeftType> && std::is_arithmetic_v<RightType>) {
//                 using CommonType = std::common_type_t<LeftType, RightType>;
//                 CommonType leftNum = static_cast<CommonType>(left);
//                 CommonType rightNum = static_cast<CommonType>(right);

//                 switch (op) {
//                     case TokenTypes::Plus: 
//                         return leftNum + rightNum;
//                     case TokenTypes::Minus: 
//                         return leftNum - rightNum;
//                     case TokenTypes::Multiply: 
//                         return leftNum * rightNum;
//                     case TokenTypes::Divide: 
//                         if (rightNum != 0) 
//                             return leftNum / rightNum;
//                         else 
//                             console.error("Division by zero.");
//                     case TokenTypes::Modulo: 
//                         if constexpr (std::is_integral_v<CommonType>)
//                             return static_cast<CommonType>(std::fmod(leftNum, rightNum));
//                         else
//                             console.error("Modulo not supported for non-integer types.");
//                     case TokenTypes::Equals: 
//                         return leftNum == rightNum;
//                     case TokenTypes::NotEquals: 
//                         return leftNum != rightNum;
//                     case TokenTypes::LessThan: 
//                         return leftNum < rightNum;
//                     case TokenTypes::GreaterThan: 
//                         return leftNum > rightNum;
//                     case TokenTypes::LessEqual: 
//                         return leftNum <= rightNum;
//                     case TokenTypes::GreaterEqual: 
//                         return leftNum >= rightNum;
                    
//                     case TokenTypes::BitwiseXor:
//                     case TokenTypes::BitwiseAnd:
//                     case TokenTypes::BitwiseOr:
//                     case TokenTypes::ShiftLeft:
//                     case TokenTypes::ShiftRight: {
//                         if constexpr (!std::is_integral_v<CommonType>)
//                             console.error("Bitwise operations require integer operands.");
                        
//                         int64_t leftInt = static_cast<int64_t>(leftNum);
//                         int64_t rightInt = static_cast<int64_t>(rightNum);

//                         switch (op) {
//                             case TokenTypes::BitwiseXor: return leftInt ^ rightInt;
//                             case TokenTypes::BitwiseAnd: return leftInt & rightInt;
//                             case TokenTypes::BitwiseOr:  return leftInt | rightInt;
//                             case TokenTypes::ShiftLeft:  return leftInt << rightInt;
//                             case TokenTypes::ShiftRight: return leftInt >> rightInt;
//                             default: break;
//                         }
//                     }
//                     default:
//                         console.error("Unsupported operator for numeric types.");
//                 }
//             } else {
//                 console.error("Invalid types for binary operation.");
//             }
//         };

//         // Use std::visit for numeric handling
//         if (std::holds_alternative<int>(leftValue) || std::holds_alternative<float>(leftValue) ||
//             std::holds_alternative<double>(leftValue) || std::holds_alternative<long>(leftValue) ||
//             std::holds_alternative<unsigned int>(leftValue) || std::holds_alternative<long long>(leftValue) ||
//             std::holds_alternative<unsigned long long>(leftValue)) {
//             return std::visit(handleNumericOperation, leftValue, rightValue);
//         }

//         if (std::holds_alternative<bool>(leftValue) && std::holds_alternative<bool>(rightValue)) {
//             bool leftBool = std::get<bool>(leftValue);
//             bool rightBool = std::get<bool>(rightValue);

//             switch (op) {
//                 case TokenTypes::LogicalAnd:
//                     return leftBool && rightBool;
//                 case TokenTypes::LogicalOr:
//                     return leftBool || rightBool;
//                 case TokenTypes::LogicalXor:
//                     return (leftBool || rightBool) && !(leftBool && rightBool);
//                 case TokenTypes::Equals:
//                     return leftBool == rightBool;
//                 case TokenTypes::NotEquals:
//                     return leftBool != rightBool;
//                 default:
//                     console.error("Unsupported operator for booleans.");
//             }
//         // String and string
//         } else if (std::holds_alternative<std::string>(leftValue) && std::holds_alternative<std::string>(rightValue)) {
//             const std::string& leftString = std::get<std::string>(leftValue);
//             const std::string& rightString = std::get<std::string>(rightValue);

//             // The resulting string of any operation
//             std::string result = leftString;
//             switch (op) {
//                 case TokenTypes::Plus:
//                     return leftString + rightString; // Concatenation
//                 case TokenTypes::Minus:
//                     if (rightString.empty()) return leftString; // Nothing to remove
//                     size_t pos;
//                     // Find and erase all occurrences of `toRemove` in `result`
//                     while ((pos = result.find(rightString)) != std::string::npos) {
//                         result.erase(pos, rightString.length());
//                     }
                    
//                     return result;
//                 case TokenTypes::Equals:
//                     return leftString == rightString;
//                 case TokenTypes::NotEquals:
//                     return leftString != rightString;
//                 case TokenTypes::LessThan:
//                     return leftString < rightString;
//                 case TokenTypes::GreaterThan:
//                     return leftString > rightString;
//                 case TokenTypes::LessEqual:
//                     return leftString <= rightString;
//                 case TokenTypes::GreaterEqual:
//                     return leftString >= rightString;
//                 default:
//                     console.error("Unsupported operator for strings.");
//             }

//         // string and number
//         } else if ((std::holds_alternative<std::string>(leftValue) && std::holds_alternative<int>(rightValue)) ||
//                     (std::holds_alternative<int>(leftValue) && std::holds_alternative<std::string>(rightValue))) {

//             // Initialize variables before the switch statement
//             std::string str = std::holds_alternative<std::string>(leftValue) ? std::get<std::string>(leftValue) : std::get<std::string>(rightValue);
//             int n = std::holds_alternative<int>(leftValue) ? std::get<int>(leftValue) : std::get<int>(rightValue);

//             // Initialize other variables
//             std::string result;
//             bool isNegative = n < 0;
//             if (isNegative) {
//                 // Reverse the string if multiplier is negative
//                 std::reverse(str.begin(), str.end());
//                 n = -n; // Make the multiplier positive for repetition
//             }

//             // Now perform the switch statement
//             switch (op) {
//                 case TokenTypes::Multiply:
//                     // Add full repetitions
//                     for (int i = 0; i < n; ++i) result += str;
//                     return result;

//                 case TokenTypes::Plus:
//                     // Handle string + integer or integer + string
//                     if (std::holds_alternative<std::string>(leftValue)) {
//                         // (string + integer) -> Append integer to string
//                         std::string numAsString = std::to_string(n);
//                         return str + numAsString;
//                     } else {
//                         // (integer + string) -> Prepend integer to string
//                         std::string numAsString = std::to_string(n);
//                         return numAsString + str;
//                     }
//                 case TokenTypes::Equals:
//                     return false;

//                 default:
//                     console.error("Unsupported operator for string and integer.");
//             }
//         } else if ((std::holds_alternative<std::string>(leftValue) && (std::holds_alternative<float>(rightValue) || std::holds_alternative<double>(rightValue))) ||
//                     (std::holds_alternative<float>(leftValue) && std::holds_alternative<std::string>(rightValue)) ||
//                     (std::holds_alternative<double>(leftValue) && std::holds_alternative<std::string>(rightValue))) {

//             // Initialize variables before the switch statement
//             std::string str = std::holds_alternative<std::string>(leftValue) ? std::get<std::string>(leftValue) : std::get<std::string>(rightValue);
//             double n = std::holds_alternative<float>(leftValue) ? std::get<float>(leftValue) :
//                     std::holds_alternative<double>(leftValue) ? std::get<double>(leftValue) : 
//                     std::holds_alternative<float>(rightValue) ? std::get<float>(rightValue) : 
//                     std::get<double>(rightValue);

//             // Initialize other variables
//             std::string result;
//             bool isNegative = n < 0;
//             double fractionalPart = isNegative ? -(n - static_cast<int>(n)) : n - static_cast<int>(n);  // Fractional part for partial repetition
//             int fullRepetitions = static_cast<int>(n);  // Full repetitions from the integer part

//             // Initialize partialLength before the switch statement
//             int partialLength = static_cast<int>(fractionalPart * str.size());

//             // // If negative, reverse the string and make the multiplier positive
//             // if (isNegative) {
//             //     // Reverse the string if multiplier is negative
//             //     std::reverse(str.begin(), str.end());
//             //     n = -n; // Make the multiplier positive for repetition
//             // }

//             // Now perform the switch statement
//             switch (op) {
//                 case TokenTypes::Multiply:
//                     if (n == 0) return std::string(""); // Handle zero multiplier

//                     // Add full repetitions
//                     for (int i = 0; i < fullRepetitions; ++i) {
//                         result += str;
//                     }

//                     // Add fractional repetition (take the portion of the string based on fractional part)
//                     result += str.substr(0, partialLength);

//                     // If the multiplier was negative, reverse the result string back
//                     if (isNegative) {
//                         std::reverse(result.begin(), result.end());
//                     }

//                     return result;

//                 case TokenTypes::Plus:
//                     // Handle string + float/double or float/double + string
//                     if (std::holds_alternative<std::string>(leftValue)) {
//                         // (string + float/double) -> Append float/double to string
//                         std::string numAsString = std::to_string(n);
//                         return str + numAsString;
//                     } else {
//                         // (float/double + string) -> Prepend float/double to string
//                         std::string numAsString = std::to_string(n);
//                         return numAsString + str;
//                     }

//                 default:
//                     console.error("Unsupported operator for string and float/double.");
//             }
//         } else if (auto leftArray = std::dynamic_pointer_cast<Array>(std::get<std::shared_ptr<Object>>(leftValue));
//             auto rightArray = std::dynamic_pointer_cast<Array>(std::get<std::shared_ptr<Object>>(rightValue))) {

//             switch (op) {
//                 case TokenTypes::Plus: {
//                     // Concatenate the arrays
//                     auto result = std::make_shared<Array>(leftArray->elements);
//                     result->elements.insert(result->elements.end(), rightArray->elements.begin(), rightArray->elements.end());
//                     return result;
//                 }
//                 case TokenTypes::Equals:
//                     return leftArray->elements == rightArray->elements; // Compare the elements directly
//                 case TokenTypes::NotEquals:
//                     return leftArray->elements != rightArray->elements; // Inequality
//                 default:
//                     console.error("Unsupported operator for Array.");
//             }
        
//         // Array (op) Value
//         } else if (std::holds_alternative<std::shared_ptr<Object>>(leftValue) &&
//                 isTruthy(rightValue)) {

//             auto leftObject = std::get<std::shared_ptr<Object>>(leftValue);
//             auto leftArray = std::dynamic_pointer_cast<Array>(leftObject);

//             auto rightElement = rightValue;

//             switch (op) {
//                 case TokenTypes::Plus: {
//                     // Append the rightValue to the array
//                     leftArray->elements.push_back(rightElement);
//                     return leftArray;
//                 }
//                 case TokenTypes::Minus: {
//                     // Remove all occurrences of rightValue from the array
//                    leftArray->elements.erase(
//                                 std::remove_if(
//                                     leftArray->elements.begin(),
//                                     leftArray->elements.end(),
//                                     [&rightElement](const auto& element) {
//                                         return compare_element(element, rightElement);
//                                     }),
//                                 leftArray->elements.end()
//                             );
//                     return leftArray;
//                 }
//                 default:
//                     console.error(
//                         "Unsupported operator between Array and Object. Supported Operations are '+' and '-'");
//             }
//         }

//         // Value (op) Array
//         else if (isTruthy(leftValue) &&
//         std::holds_alternative<std::shared_ptr<Array>>(rightValue)) {

//         auto leftElement = leftValue;
//         auto rightArray = std::get<std::shared_ptr<Array>>(rightValue);

//             switch (op) {
//                 case TokenTypes::Plus: {
//                     // Prepend the leftValue to the array
//                     rightArray->elements.insert(rightArray->elements.begin(), leftElement);
//                     return rightArray;
//                 }
//                 default:
//                     console.error("Unsupported operator for Object + Array.");
//             }
//         } else {
//             console.error("Unsupported operand types " + valueToString(leftValue) + " " + getOperatorString(op) + " " +
//             valueToString(rightValue) + " in binary expression.");
//         }

//     } 

//     return std::nullopt;
// }

// //Evaluate a Tenary expression
// SymbolTable::ValueType TenaryExpression::express(SymbolTableType scope) {
//     auto conditionResult = Expression::evaluate(condition, scope);
//     // Check if the condition has a value and convert it to bool
//     bool result = conditionResult.has_value() && std::holds_alternative<bool>(conditionResult.value()) 
//                     ? std::get<bool>(conditionResult.value())
//                     : false; // default to false if no value or invalid type
//     if (result) {
//         return Expression::evaluate(truthy, scope).value();
//     }
//     return Expression::evaluate(falsey, scope).value();
// }

// SymbolTable::ValueType CallMethod::evaluate(SymbolTable &scope) {
//     showDebugSection("Calling the method '" + methodName + "' on object " + valueToString(object));

//     // Lambda to convert a SymbolTable::ValueType to a shared_ptr<Object>
//     auto resolveObjectValue = [&](const SymbolTable::ValueType &val) -> std::shared_ptr<Object> {
//         if (auto objPtr = std::get_if<std::shared_ptr<Object>>(&val)) {
//             return *objPtr;
//         }
//         return primitiveToObject(val);
//     };

//     std::shared_ptr<Object> baseObject;

//     // === 1. Try to resolve the target object from the 'object' variant ===
//     if (std::holds_alternative<std::shared_ptr<Statement>>(object)) {
//         auto stmtPtr = std::get<std::shared_ptr<Statement>>(object);

//         // If it's a Value statement, try to extract the underlying string name.
//         if (auto valueStmt = std::dynamic_pointer_cast<Value>(stmtPtr)) {
//             if (auto objectName = std::get_if<std::string>(&valueStmt->value)) {
//                 // Replace object with the string name.
//                 object = *objectName;
//             }
//         }
//         // If it's a Variable, evaluate it.
//         else if (auto varStmt = std::dynamic_pointer_cast<Variable>(stmtPtr)) {
//             auto objectValue = Expression::evaluate(varStmt, scope).value();
//             baseObject = resolveObjectValue(objectValue);
            
//         } else if (auto getMemberStmt = std::dynamic_pointer_cast<GetProperty>(stmtPtr)) {
//             auto objectMember = Expression::evaluate(getMemberStmt, scope).value();
//             baseObject = resolveObjectValue(objectMember);
//         }
//     }
//     // If object holds a string name, try resolving it from the SymbolTable.
//     else if (std::holds_alternative<std::string>(object)) {
//         std::string objectName = std::get<std::string>(object);
//         auto lookupResult = scope->get(objectName);
//         if (lookupResult.has_value()) {
//             auto objectValue = lookupResult.value();
//             DEBUG_LOG("Got object '" + valueToString(objectValue) + "'");
//             baseObject = resolveObjectValue(objectValue);
//         }
//     }
//     // If object is still a Statement (another branch), evaluate it.
//     else if (std::holds_alternative<std::shared_ptr<Statement>>(object)) {
//         auto stmt = std::get<std::shared_ptr<Statement>>(object);
//         auto objectValue = Expression::evaluate(stmt, scope).value();
//         baseObject = resolveObjectValue(objectValue);
//     }

//     // === 2. Fallback: Evaluate 'object' directly if baseObject is not yet resolved ===
//     if (!baseObject) {
//         auto objectValue = Expression::evaluate(object, scope).value();
//         baseObject = resolveObjectValue(objectValue);
//     }

//     // === 3. Evaluate method arguments ===
//     std::vector<SymbolTable::ValueType> evaluatedArgs;
//     DEBUG_LOG("Evaluating the args");
//     DEBUG_LOG("The args are " + valueToString(arguments));
//     int argPosition = 0;
//     for (const auto &arg : arguments) {
//         DEBUG_LOG("Arg " + std::to_string(argPosition) + " is a " + valueToString(arg));
//         auto value = Expression::evaluate(arg, scope).value();
//         DEBUG_LOG(valueToString(arg) + " = " + valueToString(value));
//         evaluatedArgs.push_back(value);
//         argPosition++;
//     }

//     SymbolTable::ValueType result;

//     // === 4. Dispatch to the proper method (class method vs. object method) ===
//     if (auto instance = std::dynamic_pointer_cast<ClassInstance>(baseObject)) {
//         // Retrieve the class method (with any modifiers)
//         auto [methodVariant, modifiers] = instance->getClassInstanceMethod(methodName);
//         if (std::holds_alternative<std::nullptr_t>(methodVariant)) {
//             // Method not found on class instance: return a null value.
//             return nullptr;
//         }
//         // If the method is a C++ lambda wrapper:
//         if (auto func = std::get_if<std::function<SymbolTable::ValueType(const ArgumentDefinition&)>>(&methodVariant)) {
//             result = (*func)(evaluatedArgs);
//         }
//         // Otherwise, if it's a Function statement:
//         else if (auto funcStmt = std::get_if<std::shared_ptr<Function>>(&methodVariant)) {
//             evaluatedArgs.push_back(std::make_shared<ConstantAssignment>("this", instance));
//             (*funcStmt)->express(scope, evaluatedArgs);
//         }
//     } else {
//         // Handle regular objects:
//         DEBUG_LOG("Calling method '" + methodName + "' on object '" + baseObject->getName() + "'");
//         auto methodVariant = baseObject->getMethod(methodName);
//         if (std::holds_alternative<std::nullptr_t>(methodVariant)) {
//             console.error("Method '" + methodName + "' not found on object '" + baseObject->getName() + "'.");
//         }
//         if (auto func = std::get_if<std::function<SymbolTable::ValueType(const ArgumentDefinition&)>>(&methodVariant)) {
//             result = (*func)(evaluatedArgs);
//         } else if (auto funcStmt = std::get_if<std::shared_ptr<Function>>(&methodVariant)) {
//             result = (*funcStmt)->express(scope, evaluatedArgs);
//         }
//     }

//     return result;
// }



// SymbolTable::ValueType GetProperty::express(SymbolTableType scope) {
//     DEBUG_LOG("Evaluating a property call");
//     std::shared_ptr<Object> baseObject;

//     // 1. Check if the object is a string identifier.
//     if (auto objectName = std::get_if<std::string>(&object)) {
//         auto result = scope->get(*objectName); // Retrieve from the symbol table
//         if (result.has_value()) { // Check if the value exists
//             auto objectValue = result.value();
//             DEBUG_LOG("Found object '" + valueToString(objectValue) + "'");

//             // If it's already an object, use it
//             if (auto objPtr = std::get_if<std::shared_ptr<Object>>(&objectValue)) {
//                 baseObject = *objPtr;
//             } else {
//                 // Convert primitives to objects if needed
//                 baseObject = primitiveToObject(objectValue);
//             }
//         }
//     }

//     // 2. Evaluate the object if not resolved via SymbolTable.
//     if (!baseObject) {
//         auto objectValue = Expression::evaluate(object, scope).value();
//         baseObject = primitiveToObject(objectValue);
//     }

//     // 3. Handle property retrieval.
//     SymbolTable::ValueType result;
//     if (auto instance = std::dynamic_pointer_cast<ClassInstance>(baseObject)) {
//         DEBUG_LOG("Accessing property '" + propertyName + "' on class instance");

//         // Retrieve property from the class instance
//         auto [pvalue, mods] = instance->getClassInstanceProperty(propertyName);
//         if (!std::holds_alternative<std::nullptr_t>(pvalue)) {
//            result = pvalue;
//         } else {
//             auto [mvalue, mods] = instance->getClassInstanceMethod(propertyName);
//             if (!std::holds_alternative<std::nullptr_t>(mvalue)) {
//                 result = "\033[3mf\033[0m() => {}";
//             } else {
//                 result = nullptr;
//             }
//         }

//         if (std::get_if<std::nullptr_t>(&result)) {
//             console.error("Property '" + propertyName + "' not found on class instance.");
//         }
//     } else {
//         DEBUG_LOG("Accessing property '" + propertyName + "' on object '" + baseObject->getName() + "'");

//         // Retrieve property from the regular object
//         result = baseObject->getProperty(propertyName);

//         if (std::get_if<std::nullptr_t>(&result)) {
//             console.error("Property '" + propertyName + "' not found on object '" + baseObject->getName() + "'.");
//         }
//     }

//     DEBUG_LOG("Retrieved property value: " + valueToString(result));
//     return result;
// }


// SymbolTable::ValueType ObjectConstructorStatement::evaluate(SymbolTable &scope) {
//     // Evaluate constructor arguments
//     showDebugSection("Constructing an object");
//     std::vector<SymbolTable::ValueType> evaluatedArgs;
//     DEBUG_LOG("Parsing the arguments");
//     for (const auto& arg : constructorArgs) {
//         auto result = Expression::evaluate(arg, scope);
//         if (result.has_value()) {
//             evaluatedArgs.push_back(result.value());
//         } else {
//             console.error("Argument could not be evaluated");
//         }
//     }

//     // Handle an Array (which should be an Object)
//     if (auto arry = std::get_if<std::shared_ptr<Array>>(&obj)) {
//         DEBUG_LOG("Constructing an array");
//         std::vector<SymbolTable::ValueType> evaluatedElements;
//         for (auto &elem : (*arry)->elements) {
//             auto evalResult = Expression::evaluate(elem, scope);
//             if (evalResult.has_value()) {
//                 evaluatedElements.push_back(evalResult.value());
//             } else {
//                 console.error("Failed to evaluate array element.");
//             }
//         }
//         auto newArray = std::make_shared<Array>(evaluatedElements);
//         return newArray; // Ensure it's returned as an Object
//     } 

//     // Handle an Object
//     if (auto object = std::get_if<std::shared_ptr<Object>>(&obj)) {
//         DEBUG_LOG("Constructing an object");
//         if (*object) { // Ensure the shared pointer is valid

//             // Also check if the Object is an EnumObject
//             if (auto enumObj = std::dynamic_pointer_cast<Enum>(*object)) {
//                 DEBUG_LOG("Constructing an enum");
//                 for (const auto& [key, value] : enumObj->properties) {
//                     auto result = Expression::evaluate(value, scope);
//                     if (result.has_value()) {
//                         enumObj->properties[key] = result.value();
//                     } else {
//                         console.error("Failed to evaluate enum value: " + key);
//                     }
//                 }
//             }  else {
//                 DEBUG_LOG("Constructing another object type");
//                 for (const auto& [key, value] : (*object)->properties) {
//                     if (std::holds_alternative<std::shared_ptr<Statement>>(value)) {
//                         auto result = Expression::evaluate(value, scope);
//                         if (result.has_value()) {
//                             (*object)->setProperty(key, result.value());
//                         } else {
//                             console.error("Failed to evaluate property: " + key);
//                         }
//                     } else {
//                         (*object)->setProperty(key, value);
//                     }
//                 }
//             }
//             return *object; // Ensure we return the processed object
//         } else {
//             console.error("Null Object pointer during construction");
//         }
//     }


//     // Handle a Namespace: Evaluate the Namespace body
//     if (auto namespaceObj = std::get_if<std::shared_ptr<Namespace>>(&obj)) {
//         DEBUG_LOG("Constructing a namespace");
//         if (*namespaceObj) {
//             DEBUG_LOG("Evaluating Namespace body");
//             // Evaluate the body of the Namespace in the provided scope
//             (*namespaceObj)->express(scope);
//             return *namespaceObj; // Return the shared_ptr to the Namespace
//         } else {
//             console.error("Null Namespace pointer during construction");
//         }
//     }


//     // Handle a Statement
//     if (auto statement = std::get_if<std::shared_ptr<Statement>>(&obj)) {
//         DEBUG_LOG("Evaluating a Statement for construction");
//         auto result = Expression::evaluate(*statement, scope);
//         if (result.has_value()) {
//             // Ensure result is an Object
//             if (auto objPtr = std::get_if<std::shared_ptr<Object>>(&result.value())) {
//                 if (objPtr && *objPtr) { // Ensure valid shared pointer
//                     if (auto classType = std::dynamic_pointer_cast<Class>(*objPtr)) {
//                         DEBUG_LOG("Constructing a ClassInstance");
//                         auto instance = std::make_shared<ClassInstance>(classType);
//                         int constructorPos = 0;
//                         for (const auto& constructor : instance->constructors) {
//                             if (constructorPos == 0) {
//                                 std::vector<SymbolTable::ValueType> convertedArgs;
//                                 for (const auto& arg : constructorArgs) {
//                                     convertedArgs.push_back(arg); // Convert each Statement to a ValueType
//                                 }

//                                 constructor->addParameter("this", std::make_shared<ConstantAssignment>("this", instance));
//                                 constructor->express(scope, convertedArgs);
//                             }
//                         }
//                         return std::static_pointer_cast<Object>(instance); // Ensure consistent return type
//                     } else {
//                         console.error("Resulting Object is not a Class");
//                     }
//                 } else {
//                     console.error("Null Object pointer in evaluation");
//                 }
//             } else {
//                 console.error("Evaluation result is not an Object");
//             }
//         } else {
//             console.error("Failed to evaluate Statement");
//         }
//     }

//     console.error("Invalid object type for construction");
//     return nullptr; // Return nullptr explicitly instead of returning `obj` directly
// }



// void ObjectDestructorStatement::execute(SymbolTable &scope) {
//     // Retrieve the object from the scope
//     auto objectValue = scope->get(variableName);
//     if (!objectValue) {
//         console.error("Variable " + variableName + " not found");
//     }

//     // Attempt to extract a shared_ptr<Object> from the variant
//     auto objectPtr = std::get_if<std::shared_ptr<Object>>(&*objectValue);
//     if (!objectPtr || !*objectPtr) {
//         console.error(variableName + " is not an object");
//     }

//     auto object = *objectPtr;

//     // Call the destructor method if defined
//     // if (object->hasMethod("~destructor")) {
//     //     object->callMethod("~destructor", {});
//     // }

//     // Remove the object from the scope
//     scope->unset(variableName);
// }
