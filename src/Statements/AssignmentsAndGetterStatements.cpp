#include <omniscript/Statements/Statement.h>
#include <omniscript/Statements/AccessStatements.h>
#include <omniscript/Statements/LiteralStatements.h>
#include <omniscript/Statements/FunctionStatement.h>
#include <omniscript/Statements/AssignmentAndGetterStatements.h>

#include <omniscript/Expressions/GetterExpressions.h>
#include <omniscript/Expressions/FunctionExpression.h>
#include <omniscript/Expressions/LiteralExpressions.h>
#include <omniscript/Expressions/AssignmentExpression.h>
#include <omniscript/Expressions/VariableAccessExpression.h>

namespace Omniscript {

std::shared_ptr<Expression> AddressOf::express(SymbolTableType scope) {
    setSpanFromPosition(span.start.line, span.start.col, span.start.filePath);
    std::shared_ptr<Expression> referent = scope->get(name); // non-throwing: missing/overloaded names are handled below

    DEBUG_LOG("Getting the address of '" + name + "'.");
    
    if (!scope->exists(name)) {
        std::string suggestion = "To resolve this:\n"
                               "1. Check spelling of '" + name + "'\n"
                               "2. Verify variable is declared before use\n"
                               "3. Check scope visibility rules";
        console.reportError(
            Console::TYPE_ERROR,
            "Symbol '" + name + "' was not found in the scope",
            suggestion,
            getSpan()
        );
        return nullptr;
    }
    
    if (!referent) {
        auto overloads = scope->getOverloads(name);
        if (overloads.empty()) {
            std::string suggestion = "To resolve this:\n"
                                   "1. Check function '" + name + "' exists\n"
                                   "2. Verify proper function declaration\n"
                                   "3. Check namespace/import requirements";
            console.reportError(
                Console::TYPE_ERROR,
                "No valid referent found for '" + name + "'",
                suggestion,
                getSpan()
            );
            return nullptr;
        }
        
        referent = overloads[0];
        auto mangledName = std::dynamic_pointer_cast<FunctionExpression>(referent)->mangledName;
        DEBUG_LOG("Mangled name '" + mangledName + "'.");

        if (!type) {
            setType(Type::createPointerType(referent->getType()));
            setRootType(Type::createPointerType(referent->getType()));
        }

        auto addrOf = std::make_shared<AddressOfExpression>(mangledName, referent);       
    }

    if (!type) {
        setType(Type::createPointerType(referent->getType()));
        setRootType(Type::createPointerType(referent->getType()));
    }

    auto addrOf = std::make_shared<AddressOfExpression>(name, referent);
    addrOf->setSpan(this->getSpan());
    return addrOf;
}

std::shared_ptr<Expression> ReferenceTo::express(SymbolTableType scope) {
    setSpanFromPosition(span.start.line, span.start.col, span.start.filePath);
    
    if (name.empty() && !referent) {
        console.reportError(
            Console::SYNTAX_ERROR,
            "Invalid reference declaration",
            "References must be initialized with a valid target",
            getSpan()
        );
        return nullptr;
    }
    
    if (referent) {
        DEBUG_LOG("Getting a reference to '" + referent->toString() + "'.");
        auto referentExpr = referent->express(scope);
        if (!referentExpr) {
            std::string suggestion = "To resolve this:\n"
                                   "1. Check referent expression validity\n"
                                   "2. Verify referent is not null\n"
                                   "3. Add debug output for referent";
            console.reportError(
                Console::RUNTIME_ERROR,
                "Error getting referent '" + referent->toString() + "'",
                suggestion,
                referent->getSpan()
            );
            return nullptr;
        }
        // A caller (e.g. member-call `this` on a `Stats?` variable) may have pre-set a pointer to the
        // *wrapped* type. Keep it when the referent is a Nullable<T> and the pre-set pointee is T;
        // otherwise this would be recomputed as `* Stats?` and fail to bind to a `* Stats` parameter.
        std::shared_ptr<Type> refType;
        if (auto nullable = std::dynamic_pointer_cast<NullableType>(referentExpr->getType())) {
            if (nullable->innerType && type && type->isPointer()) {
                auto pointee = type->getBasePointeeType();
                if (pointee && pointee->toString() == nullable->innerType->toString()) {
                    refType = type;
                }
            }
        }
        setType(refType ? refType : Type::createPointerType(referentExpr->getType()));
        setRootType(type);
        auto ref = std::make_shared<ReferenceExpression>(name, referentExpr);
        ref->type = type;
        ref->setSpan(this->getSpan());
        return ref;
    }
    
    DEBUG_LOG("Getting a reference to '" + name + "'.");
    if (!scope->exists(name)) {
        std::string suggestion = "To resolve this:\n"
                               "1. Check spelling of '" + name + "'\n"
                               "2. Verify variable exists in scope\n"
                               "3. Check declaration order";
        console.reportError(
            Console::TYPE_ERROR,
            "Symbol '" + name + "' was not found in the scope",
            suggestion,
            getSpan()
        );
        return nullptr;
    }
    
    auto variable = scope->getValue(name);
    if (!variable) {
        std::string suggestion = "To resolve this:\n"
                               "1. Check variable initialization\n"
                               "2. Verify scope rules\n"
                               "3. For functions, check overloads";
        console.reportError(
            Console::RUNTIME_ERROR,
            "Invalid reference target '" + name + "'",
            suggestion,
            getSpan()
        );
        return nullptr;
    }

    setType(Type::createPointerType(variable->getType()));
    setRootType(type);
    auto ref = std::make_shared<ReferenceExpression>(name, variable);
    ref->type = type;
    ref->setSpan(this->getSpan());
    return ref;
}

std::shared_ptr<Expression> GetVariable::express(SymbolTableType scope) {
    setSpanFromPosition(span.start.line, span.start.col, span.start.filePath);
    DEBUG_LOG("Getting symbol '" + name + "'.");
    DEBUG_LOG(getContextAsString());

    std::shared_ptr<Expression> expr = scope->get(name);
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
                resolvedName = fullCalleeName;
                break;
            }
        }
    }

    if (!expr) {
        auto overloads = scope->getOverloads(name);
        if (!overloads.empty()) {
            expr = overloads[0];
            auto func = std::dynamic_pointer_cast<FunctionExpression>(expr);
            resolvedName = func->mangledName;
        }
    }

    if (!expr) {
        std::string suggestion = "To resolve this:\n"
                               "1. Check spelling of '" + name + "'\n"
                               "2. Verify symbol is declared\n"
                               "3. Check import/namespace requirements";
        console.reportError(
            Console::TYPE_ERROR,
            "Symbol '" + name + "' could not be resolved in scope '" + scope->getName() + "'",
            suggestion,
            getSpan()
        );
        return nullptr;
    }

    DEBUG_LOG("The variable stored in scope is '" + expr->toString() + "'.");

    std::shared_ptr<Type> symbolType = expr->getType();

    // If the stored expression is the assignment node itself, use its declared (qualified) name.
    if (auto va = std::dynamic_pointer_cast<VariableAssignment>(expr)) {
        resolvedName = va->getName();
    }

    if (type) {
        if (!Type::isSameOrCastableTo(symbolType, type)) {
            if (symbolType->isNullable()) {
                if (auto varAccess = std::dynamic_pointer_cast<VariableAccessExpression>(expr)) {
                    if (!varAccess->extractValue) {
                        std::string suggestion = "To resolve this:\n"
                                               "1. Check type requirements\n"
                                               "2. Available conversions:\n"
                                               " - Explicit cast: `(%s)value`\n"
                                               " - Null check: `value ?? defaultValue`";
                        console.reportError(
                            Console::TYPE_ERROR,
                            "Symbol '" + resolvedName + "' is of type '" + symbolType->toString() +
                            "' which cannot be cast to '" + type->toString() + "'",
                            suggestion,
                            getSpan()
                        );
                    }
                }
            }
        } else {
            if (type->isNullable() && !symbolType->isNullable()) {
                auto val = std::make_shared<VariableAccessExpression>(resolvedName, symbolType);
                auto result = std::make_shared<NullableExpression>(val);
                result->setSpan(this->getSpan());
                return result;
            }
            DEBUG_LOG("The casted type is '" + type->toString() + "'.");
        }
    } else {
        setType(symbolType);
    }

    auto varAccess = std::make_shared<VariableAccessExpression>(resolvedName, type);
    varAccess->value = expr;
    varAccess->setSpan(this->getSpan());
    return varAccess;
}

// A struct/class declared in a module gets renamed ("String" -> "Str.String") after the parser has
// already created UserDefinedType placeholders named "String" for local declarations inside method
// bodies. Type equality is by name, so `let r: String; r.append(x)` fails to bind `this`.
// Re-point such placeholders at the name the scope now knows the type by (in place, like
// renameTypeInPlace in ModuleAndImportStatements.cpp).
static void rebindUserTypeName(const std::shared_ptr<Type>& type, const SymbolTableType& scope) {
    if (!type) return;

    if (auto udt = std::dynamic_pointer_cast<UserDefinedType>(type)) {
        auto resolved = scope->getType(udt->name);
        if (resolved && resolved != type) {
            const std::string full = resolved->getName();
            if (!full.empty() && full != udt->name) {
                udt->name = full;
            }
        }
        return;
    }
    if (type->isPointer()) {
        rebindUserTypeName(type->getPointeeType(), scope);
        return;
    }
    if (type->isReference()) {
        rebindUserTypeName(type->getReferencedType(), scope);
        return;
    }
    if (type->isNullable()) {
        if (auto nullable = std::dynamic_pointer_cast<NullableType>(type)) {
            rebindUserTypeName(nullable->innerType, scope);
        }
        return;
    }
}

std::shared_ptr<Expression> AssignVariable::express(SymbolTableType scope) {
    setSpanFromPosition(span.start.line, span.start.col, span.start.filePath);
    if (type && type->isUnresolved()) {
        if (auto unresolved = std::dynamic_pointer_cast<UnresolvedType>(type)) {
            type = scope->getType(unresolved->joinedTypeString);
            rootType = type;
            if (!type) {
                std::string suggestion = "To resolve this:\n"
                                       "1. Check type spelling\n"
                                       "2. Verify type is imported\n"
                                       "3. Check namespace requirements";
                console.reportError(
                    Console::TYPE_ERROR,
                    "Type '" + unresolved->joinedTypeString + "' does not exist in scope '" + scope->getName() + "'",
                    suggestion,
                    getSpan()
                );
                return nullptr;
            }
        }
    }

    rebindUserTypeName(type, scope);

    DEBUG_LOG("Assigning variable " + variable + (type? " of type " + type->toString() : ""));

    std::shared_ptr<Expression> result;

    if (isReassign && !scope->exists(variable)) {
        std::string suggestion = "To resolve this:\n"
                               "1. Check variable spelling\n"
                               "2. Declare variable before reassignment\n"
                               "3. Verify scope rules";
        console.reportError(
            Console::TYPE_ERROR,
            "Variable '" + variable + "' was not declared in scope '" + scope->getName() + "'",
            suggestion,
            getSpan()
        );
    }

    if (value) {
        extendContextOf(value);
    }

    if (type) {
        if (type->isGeneric()) {
            auto genericVal = scope->get(type->getName());
            if (auto generic = std::dynamic_pointer_cast<TypeExpression>(genericVal)) {
                type = generic->getTypeExpression()->clone();
            }
        }

        if (value) {
            // Propagate the declared type onto the value BEFORE expressing it.
            // Without this, `let i: u64 = 1;` produces `IntegerLiteral(1)` with type
            // == nullptr, and `IntegerLiteral::express` defaults to Int32 — so any
            // literal > 2^31 silently truncates into a 4-byte value in an 8-byte slot.
            if (type) {
                if (auto typedValue = std::dynamic_pointer_cast<TypedStatement>(value)) {
                    if (!typedValue->getType() || typedValue->getType()->isInvalid()) {
                        typedValue->setType(type);
                        typedValue->setRootType(type);
                    }
                }
            }
            result = value->express(scope);
            if (!result) {
                std::string suggestion = "To resolve this:\n"
                                       "1. Check right-hand expression\n"
                                       "2. Verify expression validity\n"
                                       "3. Add debug output";
                console.reportError(
                    Console::RUNTIME_ERROR,
                    "Invalid assignment value for '" + variable + "'",
                    suggestion,
                    value->getSpan()
                );
                return nullptr;
            }

            if (!Type::isSameOrCastableTo(result->getType(), type)) {
                std::string suggestion = "To resolve this:\n"
                                       "1. Check type requirements\n"
                                       "2. Available conversions:\n"
                                       " - Explicit cast: `(%s)value`\n"
                                       " - Conversion method: `value.to_%s()`";
                console.reportError(
                    Console::TYPE_ERROR,
                    "Type mismatch for variable '" + variable + "'\n"
                    "Expected: '" + type->toString() + "'\n"
                    "Got: '" + result->getType()->toString() + "'",
                    suggestion,
                    getSpan()
                );
                return nullptr;
            }
        } else {
            result = std::make_shared<NullExpression>(type);
        }
    } else if (value) {
        result = value->express(scope);
        if (!result) {
            console.reportError(
                Console::RUNTIME_ERROR,
                "Invalid inferred type for '" + variable + "'",
                "The type could not be inferred from the right-hand expression",
                value->getSpan()
            );
            return nullptr;
        }
        type = result->getType();
    }

    if (isReassign) {
        std::shared_ptr<Expression> prevValue = scope->get(variable);
        if (prevValue && !Type::isSameOrCastableTo(result->getType(), prevValue->getType())) {
            std::string suggestion = "To resolve this:\n"
                                   "1. Check type consistency\n"
                                   "2. Verify assignment compatibility\n"
                                   "3. Consider explicit cast if appropriate";
            console.reportError(
                Console::TYPE_ERROR,
                "Reassignment type mismatch for '" + variable + "'\n"
                "Original: '" + prevValue->getType()->toString() + "'\n"
                "New: '" + result->getType()->toString() + "'",
                suggestion,
                getSpan()
            );
            return nullptr;
        }
    }

    if (isConstant) {
        scope->setConstant(variable, result);
    } else {
        scope->setVariable(variable, result);
    }

    auto assignment = make_expression<VariableAssignment>(variable, result, isGlobal, true);
    // The node above takes its type from the initializer, which drops a declared `T?`
    // (e.g. `let a: Stats? = null;` ended up as plain `Stats`). Keep the declared nullable type.
    if (type && std::dynamic_pointer_cast<NullableType>(type)) {
        assignment->type = type;
    }
    assignment->isStatic = isStatic;
    assignment->isGlobal = isGlobal;
    assignment->isConstant = isConstant;
    assignment->isVolatile = isVolatile;
    assignment->setSpan(this->getSpan());
    assignment->isExtern = isExtern;
    assignment->externName = assignment->getName();
    assignment->windowsDynamic  = libraryPaths.windowsDynamic;
    assignment->windowsStatic   = libraryPaths.windowsStatic;
    assignment->linuxShared     = libraryPaths.linuxShared;
    assignment->linuxStatic     = libraryPaths.linuxStatic;
    assignment->macosShared     = libraryPaths.macosShared;
    assignment->macosStatic     = libraryPaths.macosStatic;
    assignment->genericDynamic  = libraryPaths.genericDynamic;
    assignment->genericStatic   = libraryPaths.genericStatic;

    return assignment;
}

} // namespace Omniscript