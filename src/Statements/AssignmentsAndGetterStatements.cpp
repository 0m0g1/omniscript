#include <omniscript/Statements/Statement.h>
#include <omniscript/Statements/ExpressionStatements.h>
#include <omniscript/Statements/AccessStatements.h>
#include <omniscript/Statements/LiteralStatements.h>
#include <omniscript/Statements/FunctionStatement.h>
#include <omniscript/Statements/AssignmentAndGetterStatements.h>
#include <omniscript/Statements/CallableStatement.h>      // Call (used to build default struct instances)

#include <omniscript/Expressions/GetterExpressions.h>
#include <omniscript/Expressions/EntityExpressions.h>
#include <omniscript/Expressions/ClassExpression.h>
#include <omniscript/Expressions/StructExpression.h>      // StructExpression, InstanceExpression
#include <omniscript/Expressions/CallableExpression.h>    // CallExpression
#include <omniscript/Expressions/FunctionExpression.h>
#include <omniscript/Expressions/LiteralExpressions.h>
#include <omniscript/Expressions/AssignmentExpression.h>
#include <omniscript/Expressions/VariableAccessExpression.h>

namespace Omniscript {

std::string canonicalTypeName(const std::string& name);   // defined elsewhere (see CallStatement.cpp)

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
        addrOf->setSpan(this->getSpan());
        return addrOf;       
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

    // `&functionName`: functions live in the overload table, not the value table, so getValue()
    // is null for them. Produce a function-pointer reference instead of failing.
    if (!variable) {
        auto overloads = scope->getOverloads(name);
        if (!overloads.empty()) {
            auto funcRef = overloads[0];
            auto funcExpr = std::dynamic_pointer_cast<FunctionExpression>(funcRef);
            const std::string refName = (funcExpr && !funcExpr->mangledName.empty()) ? funcExpr->mangledName : name;
            DEBUG_LOG("Reference to function '" + name + "' resolved as '" + refName + "'.");

            setType(Type::createPointerType(funcRef->getType()));
            setRootType(type);
            auto fref = std::make_shared<ReferenceExpression>(refName, funcRef);
            fref->type = type;
            fref->setSpan(this->getSpan());
            return fref;
        }
    }
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
std::string mangleTypeArgs(const std::vector<std::shared_ptr<Type>>& args);   // Genericinstantiator.cpp

void rebindUserTypeName(const std::shared_ptr<Type>& type, const SymbolTableType& scope) {
    if (!type) return;

    if (auto udt = std::dynamic_pointer_cast<UserDefinedType>(type)) {
        // List<XmlNode*>: rebind the arguments and keep the "<template>_<args>" name in step.
        const std::string oldSuffix = udt->typeArgs.empty() ? std::string() : "_" + mangleTypeArgs(udt->typeArgs);
        for (auto& arg : udt->typeArgs) rebindUserTypeName(arg, scope);
        if (!oldSuffix.empty()) {
            const std::string newSuffix = "_" + mangleTypeArgs(udt->typeArgs);
            if (newSuffix != oldSuffix && udt->name.size() > oldSuffix.size() &&
                udt->name.compare(udt->name.size() - oldSuffix.size(), oldSuffix.size(), oldSuffix) == 0) {
                udt->name = udt->name.substr(0, udt->name.size() - oldSuffix.size()) + newSuffix;
            }
        }
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

// `let x: T;` (a typed declaration with no initializer) of a plain struct T must start from the field
// defaults declared in the struct body, exactly like `let x = T{};` does. Expressing the implicit null
// initializer would give an all-zero value and silently drop every non-zero default (e.g.
// `maxDt: double = 0.04`).
//
// Returns nullptr when this does not apply (not a struct, struct with a constructor, class, unresolved
// name); the caller then keeps the old zero-initialised behaviour.
static std::shared_ptr<Expression> buildDefaultStructInstance(
    const std::shared_ptr<Type>& type, const std::string& variable, SymbolTableType scope) {
    if (!type || type->isPointer() || type->isNullable() || type->isFunction()) return nullptr;

    auto udt = std::dynamic_pointer_cast<UserDefinedType>(type);
    if (!udt) return nullptr;     // primitives, arrays (`Char[32]`), ...
    const std::string typeName = udt->name;

    // Find the symbol the type name denotes, trying the same spellings Call::express falls back to
    // (alias-qualified "Str.String", canonical name, bare name after the last dot).
    std::vector<std::string> candidates{typeName};
    const std::string canon = canonicalTypeName(typeName);
    if (canon != typeName) candidates.push_back(canon);
    const size_t dot = typeName.rfind('.');
    if (dot != std::string::npos) candidates.push_back(typeName.substr(dot + 1));

    bool isStruct = false;
    for (const auto& c : candidates) {
        auto sym = scope->get(c);
        if (std::dynamic_pointer_cast<ClassExpression>(sym)) return nullptr;     // classes: unchanged
        if (std::dynamic_pointer_cast<StructExpression>(sym)) { isStruct = true; break; }
    }
    if (!isStruct) return nullptr;

    // A type with a constructor is initialised by that constructor, not by plain defaults.
    for (const auto& c : candidates) {
        if (!scope->getOverloads(c + ".constructor").empty()) {
            DEBUG_LOG("[Assign] '" + variable + "': '" + c + "' has a constructor, skipping field defaults.");
            return nullptr;
        }
    }

    // Same route as ObjectConstructorStatement::express with no arguments: a Call with no arguments
    // yields a CallExpression whose members carry every field's default value.
    std::vector<std::shared_ptr<Statement>> noArgs;
    auto call = std::dynamic_pointer_cast<CallExpression>(std::make_shared<Call>(typeName, noArgs)->express(scope));
    if (!call) {
        DEBUG_LOG("[Assign] '" + variable + "': could not build default instance of '" + typeName + "'.");
        return nullptr;
    }

    auto instance = std::make_shared<InstanceExpression>(typeName, variable, call->members);
    instance->instanceType = type;
    instance->type = type;
    DEBUG_LOG("[Assign] '" + variable + "' of struct '" + typeName + "' initialised from field defaults (" +
              std::to_string(call->members.size()) + " members).");
    return instance;
}

std::shared_ptr<Expression> AssignVariable::express(SymbolTableType scope) {
    setSpanFromPosition(span.start.line, span.start.col, span.start.filePath);

    // `a += b` where a's type defines `can +=`: call it directly instead of `a = a + b` (which would build a new
    // value and bitwise-assign it over a destructor-owning struct).
    if (isReassign) {
        if (auto compound = std::dynamic_pointer_cast<ASTBinaryExpression>(value)) {
            if (compound->fromCompound) {
                if (auto call = compound->tryCompoundOperator(scope)) return call;
            }
        }
        // `a++;` / `a--;` arrive as a reassignment of `a` from a postfix unary on `a` itself: call `can ++` / `can --`
        // directly. (`x = -a` is NOT this case: the operand must be the variable being assigned.)
        if (auto unary = std::dynamic_pointer_cast<ASTUnaryExpression>(value)) {
            const TokenTypes ut = unary->getOperator().getType();
            auto operandVar = std::dynamic_pointer_cast<GetVariable>(unary->getOperand());
            if ((ut == TokenTypes::Increment || ut == TokenTypes::Decrement) && operandVar && operandVar->getName() == variable) {
                bool handled = false;
                auto call = unary->tryOperator(scope, handled);
                if (handled) return call;
            }
        }
    }
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

    // `let c: Color` where Color is an enum: an enum is an alias of its integer type.
    if (type) {
        if (auto enumExpr = std::dynamic_pointer_cast<EnumExpression>(scope->get(type->getName()))) {
            if (enumExpr->underlyingType) {
                type = enumExpr->underlyingType;
                rootType = type;
            }
        }
    }

    rebindUserTypeName(type, scope);

    DEBUG_LOG("Assigning variable " + variable + (type? " of type " + type->toString() : ""));

    std::shared_ptr<Expression> result;

    // When a typed declaration fails (bad initializer, type mismatch), still declare the variable with
    // its annotated type. Otherwise every later use reports a bogus "Symbol 'x' could not be resolved"
    // and the one real error drowns in follow-on noise. Reassignments are left alone (already declared).
    auto declareAfterFailure = [&]() {
        if (isReassign || !type) return;
        auto placeholder = std::make_shared<NullExpression>(type);
        if (isConstant) {
            scope->setConstant(variable, placeholder);
        } else {
            scope->setVariable(variable, placeholder);
        }
    };

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
            // The parser turns `let x: T;` into `let x: T = <null literal>`, so a plain local struct
            // declaration arrives here rather than in the no-initializer branch below. Build the instance
            // from the field defaults instead of expressing the implicit null (which zero-initialises
            // every field). TODO: test the literal's class instead of its printed name.
            bool implicitStructDefault = false;
            if (value->toString() == "NullLiteralStatement" && !isGlobal && !isExtern && !isReassign) {
                result = buildDefaultStructInstance(type, variable, scope);
                implicitStructDefault = (result != nullptr);
            }
            if (!implicitStructDefault) {
                result = value->express(scope);
            }
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
                declareAfterFailure();
                return nullptr;
            }

            // A function-typed variable holds a function pointer, so an initializer typed
            // `* fn(...)` with the SAME signature (`nullptr`, `&callback`) is compatible with the
            // declared `fn(...)`. A null gets the declared type, like the no-initializer path below.
            bool functionPointerInit = false;
            if (type->isFunction()) {
                auto initType = result->getType();
                if (initType && initType->isPointer() && initType->getPointeeType() &&
                    initType->getPointeeType()->toString() == type->toString()) {
                    functionPointerInit = true;
                    if (std::dynamic_pointer_cast<NullExpression>(result)) {
                        result = std::make_shared<NullExpression>(type);
                    }
                }
            }

            // The initializer's type can carry the same unrenamed module placeholder (`JsonValue*`
            // from a generic's type argument) that the declared type already had rebound to
            // `Json.JsonValue*`; type equality is by name, so rebind it the same way.
            rebindUserTypeName(result->getType(), scope);

            if (!functionPointerInit && !Type::isSameOrCastableTo(result->getType(), type)) {
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
                declareAfterFailure();
                return nullptr;
            }
        } else {
            // No initializer node at all (declarations built by the compiler rather than parsed from
            // `let x: T;`): same rule as above, a plain local struct starts from its field defaults and
            // everything else is zero-initialised.
            if (!isGlobal && !isExtern && !isReassign) {
                result = buildDefaultStructInstance(type, variable, scope);
            }
            if (!result) {
                result = std::make_shared<NullExpression>(type);
            }
        }
    } else if (value) {
        // A reassignment has no declared type, so an untyped literal (`fy = 0.0;`) would default to
        // Double/Int32 and then fail the reassignment check against a Float/narrower variable. Give
        // the literal the variable's existing type first, exactly as the declared-type path above does
        // for `let fy: float = 0.0;`. Pointer, function and nullable variables are left alone because
        // their untyped initializers (`nullptr`, `null`, `&fn`) are already handled elsewhere.
        if (isReassign && scope->exists(variable)) {
            auto prev = scope->get(variable);
            auto prevType = prev ? prev->getType() : nullptr;
            if (prevType && !prevType->isInvalid() && !prevType->isPointer() &&
                !prevType->isFunction() && !prevType->isNullable()) {
                if (auto typedValue = std::dynamic_pointer_cast<TypedStatement>(value)) {
                    if (!typedValue->getType() || typedValue->getType()->isInvalid()) {
                        typedValue->setType(prevType);
                        typedValue->setRootType(prevType);
                    }
                }
            }
        }
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
        // An inferred type (`let v = alloc(k);`) can carry the same unrenamed module placeholder that the
        // declared-type path rebinds (`JsonValue*` vs `Json.JsonValue*`); without this a later `v.s` cannot
        // resolve the pointee as a user-defined type.
        rebindUserTypeName(result->getType(), scope);
        type = result->getType();
    }

    if (isReassign) {
        std::shared_ptr<Expression> prevValue = scope->get(variable);
        if (prevValue) { rebindUserTypeName(result->getType(), scope); rebindUserTypeName(prevValue->getType(), scope); }
        // A function-typed variable is stored as a function pointer, so `fn(...)` and `* fn(...)` with the
        // same signature are interchangeable here (e.g. `glFoo = getProc(...) as fn(u32) => void;`).
        auto sameFunctionSignature = [](const std::shared_ptr<Type>& a, const std::shared_ptr<Type>& b) {
            if (!a || !b) return false;
            auto unwrap = [](const std::shared_ptr<Type>& t) {
                return (t->isPointer() && t->getPointeeType()) ? t->getPointeeType() : t;
            };
            auto ua = unwrap(a), ub = unwrap(b);
            return ua && ub && ua->isFunction() && ub->isFunction() && ua->toString() == ub->toString();
        };
        if (prevValue && !sameFunctionSignature(result->getType(), prevValue->getType()) &&
            !Type::isSameOrCastableTo(result->getType(), prevValue->getType())) {
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

    auto assignment = make_expression<VariableAssignment>(variable, result, isGlobal, isReassign);
    // The node above takes its type from the initializer, which drops a declared `T?`
    // (e.g. `let a: Stats? = null;` ended up as plain `Stats`). Keep the declared nullable type.
    if (type && std::dynamic_pointer_cast<NullableType>(type)) {
        assignment->type = type;
    }
    // Same problem for a declared scalar whose initializer has a different (castable) type, e.g.
    // `let PI: float = 3.14159265;` (the literal is a double). The node took the initializer's type,
    // so the module-level global was emitted as `double` while every use site treats it as `float`,
    // giving `fmul float ..., double %PI.val`. Keep the declared type; the IR generator converts the
    // constant initializer to it. Only module-level variables are changed.
    if (isGlobal && type && result && result->getType() && !type->isPointer() && !type->isFunction() &&
        !std::dynamic_pointer_cast<NullableType>(type) &&
        type->toString() != result->getType()->toString()) {
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