#include <omniscript/Statements/Statement.h>
#include <omniscript/Statements/LiteralStatements.h>

#include <omniscript/Core.h>
#include <omniscript/utils.h>
#include <omniscript/Symboltable.h>
#include <omniscript/omniscript_pch.h>
#include <omniscript/Types/DerivedTypes.h>
#include <omniscript/Statements/Statement.h>

#include <omniscript/Expressions/LiteralExpressions.h>
#include <omniscript/Expressions/CastExpression.h>

namespace Omniscript {
    
std::shared_ptr<Expression> ASTCast::express(SymbolTableType scope) {
    setSpanFromPosition(span.start.line, span.start.col, span.start.filePath);
    DEBUG_LOG("");

    if (type && type->isUnresolved()) {
        if (auto unresolved = std::dynamic_pointer_cast<UnresolvedType>(type)) {
            type = scope->getType(unresolved->joinedTypeString);
            rootType = type;
            targetType = type;
            if (!type) {
                std::string suggestion = Console::formatString(
                    "To resolve this:\n"
                    "1. Verify type '%s' is defined in the current scope\n"
                    "2. Check for correct namespace imports\n"
                    "3. Ensure type is declared before use",
                    unresolved->joinedTypeString.c_str()
                );
                console.reportError(
                    Console::TYPE_ERROR,
                    Console::formatString("Type '%s' does not exist in scope '%s'",
                                     unresolved->joinedTypeString.c_str(), scope->getName().c_str()),
                    suggestion,
                    getSpan()
                );
                return nullptr;
            }
        }
    }

    if (type->isFunction()) {
        type = std::make_shared<PointerType>(type);
    }

    if (auto typed = std::dynamic_pointer_cast<TypedStatement>(value)) {
        if (typed->getRootType()) {
            DEBUG_LOG("[Cast] Casting '" + value->toString() + "' a '" + typed->getRootType()->toString() + "' to a '" + targetType->toString() + "'.");
        } else if (typed->getType()) {
            DEBUG_LOG("[Cast] Casting '" + value->toString() + "' a '" + typed->getType()->toString() + "' to a '" + targetType->toString() + "'.");
        } else {
            DEBUG_LOG("[Cast] Casting a '" + value->toString() + "' to a '" + targetType->toString() + "'.");
        }
    } else {
        if (value) {
            std::string suggestion = "To resolve this:\n"
                                   "1. Ensure the value has a defined type\n"
                                   "2. Check for proper variable declaration\n"
                                   "3. Verify type annotations are correct";
            console.reportError(
                Console::TYPE_ERROR,
                "Cannot cast " + value->toString() + " it has no type",
                suggestion,
                getSpan()
            );
        } else {
            std::string suggestion = "To resolve this:\n"
                                   "1. Provide a valid expression to cast\n"
                                   "2. Check for null or undefined values\n"
                                   "3. Add debug output to trace value source";
            console.reportError(
                Console::RUNTIME_ERROR,
                "There is no value to cast",
                suggestion,
                getSpan()
            );
        }
        return nullptr;
    }

    extendContextOf(value);

    std::shared_ptr<Expression> result;

    // Literal cast (handles constant folding, primitive -> primitive, etc.)
    if (auto literal = std::dynamic_pointer_cast<Literal>(value)) {
        if (!targetType->isNullable()) {
            auto castedStmt = literal->castTo(targetType);
            result = castedStmt->express(scope);
            result->setSpan(this->getSpan());
            return result;
        }
    }

    auto valueResult = value->express(scope);

    // If casting to a nullable type
    if (targetType->isNullable()) {
        // If the result is already null or nullable, just return as NullableExpression
        if (std::dynamic_pointer_cast<NullableExpression>(valueResult)) {
            result = valueResult;
        }
        // If we're casting a null literal (e.g., NullExpression or NullPointerExpression)
        else if (std::dynamic_pointer_cast<NullExpression>(valueResult) ||
                 std::dynamic_pointer_cast<NullPointerExpression>(valueResult)) {
            result = std::make_shared<NullableExpression>();
        }
        // Wrap any expression in a NullableExpression
        else {
            result = std::make_shared<NullableExpression>(valueResult);
        }
    } else {
        // Normal cast expression fallback
        result = std::make_shared<CastExpression>(valueResult, targetType);
    }

    result->setSpan(this->getSpan());
    return result;
}

std::shared_ptr<Expression> Nullptr::express(SymbolTableType scope) {
    setSpanFromPosition(span.start.line, span.start.col, span.start.filePath);
    if (type && type->isUnresolved()) {
        if (auto unresolved = std::dynamic_pointer_cast<UnresolvedType>(type)) {
            type = scope->getType(unresolved->joinedTypeString);
            rootType = type;
            if (!type) {
                std::string suggestion = Console::formatString(
                    "To resolve this:\n"
                    "1. Verify type '%s' is defined in the current scope\n"
                    "2. Check for correct namespace imports\n"
                    "3. Ensure type is declared before use",
                    unresolved->joinedTypeString.c_str()
                );
                console.reportError(
                    Console::TYPE_ERROR,
                    Console::formatString("Type '%s' does not exist in scope '%s'",
                                     unresolved->joinedTypeString.c_str(), scope->getName().c_str()),
                    suggestion,
                    getSpan()
                );
                return nullptr;
            }
        }
    }
    auto nullpointerType = type ? type : Type::createPrimitiveType(Kind::Void);

    auto result = make_expression<NullPointerExpression>(type);

    result->setSpan(this->getSpan());
    return result;
}

std::shared_ptr<Expression> Null::express(SymbolTableType scope) {
    setSpanFromPosition(span.start.line, span.start.col, span.start.filePath);
    if (type && type->isUnresolved()) {
        if (auto unresolved = std::dynamic_pointer_cast<UnresolvedType>(type)) {
            type = scope->getType(unresolved->joinedTypeString);
            rootType = type;
            if (!type) {
                std::string suggestion = Console::formatString(
                    "To resolve this:\n"
                    "1. Verify type '%s' is defined in the current scope\n"
                    "2. Check for correct namespace imports\n"
                    "3. Ensure type is declared before use",
                    unresolved->joinedTypeString.c_str()
                );
                console.reportError(
                    Console::TYPE_ERROR,
                    Console::formatString("Type '%s' does not exist in scope '%s'",
                                     unresolved->joinedTypeString.c_str(), scope->getName().c_str()),
                    suggestion,
                    getSpan()
                );
                return nullptr;
            }
        }
    }
    std::shared_ptr<Expression> result;

    if (type->isNullable()) {
        auto nullable = std::dynamic_pointer_cast<NullableType>(type);
        auto nullableExpr = std::make_shared<NullableExpression>();
        nullableExpr->type = nullable->innerType ? nullable->innerType : Type::createPrimitiveType(Kind::Void);
        nullableExpr->rootType = nullable->innerType ? nullable->innerType : Type::createPrimitiveType(Kind::Void);
        result = nullableExpr;
    } else {
        result = make_expression<NullExpression>(
            type ? type : Type::createPrimitiveType(Kind::Void));
    }

    result->setSpan(this->getSpan());
    return result;
}

std::shared_ptr<Expression> PointerLiteral::express(SymbolTableType scope) {
    setSpanFromPosition(span.start.line, span.start.col, span.start.filePath);

    std::shared_ptr<Expression> result;

    // Handle null pointer case
    if (address == 0) {
        result = make_expression<NullPointerExpression>(type->getPointeeType());
    } else {
        // Create raw pointer expression
        auto pointeeType = type ? type->getPointeeType() : Type::createPrimitiveType(Kind::Void);
        auto ptrType = Type::createPointerType(pointeeType, isConst, isVolatile);
        result = make_expression<RawPointerExpression>(address, ptrType);
    }

    result->setSpan(this->getSpan());
    return result;
}

std::shared_ptr<Literal> PointerLiteral::castTo(std::shared_ptr<Type> targetType) const {
    using Kind = Kind;

    // Handle null pointer case
    if (address == 0) {
        return std::make_shared<Nullptr>(targetType);
    }

    // Get current pointee type (defaults to void if unspecified)
    auto currentPointeeType = type ? type->getPointeeType() : Type::createPrimitiveType(Kind::Void);

    // Case 1: Casting to another pointer type
    if (targetType->isPointer()) {
        auto targetPointeeType = targetType->getPointeeType();

        // void* -> T* is always allowed
        if (currentPointeeType->isVoidLike()) {
            return std::make_shared<PointerLiteral>(
                address,
                targetPointeeType);
        }

        // T* -> void* is always allowed
        if (targetPointeeType->isVoidLike()) {
            return std::make_shared<PointerLiteral>(
                address,
                targetPointeeType);
        }

        // Check for compatible pointee types
        if (Type::isSameOrCastableTo(currentPointeeType, targetPointeeType)) {
            return std::make_shared<PointerLiteral>(
                address,
                targetPointeeType);
        }

        std::string suggestion = Console::formatString(
            "To resolve this:\n"
            "1. Verify compatibility between '%s' and '%s'\n"
            "2. Check for valid type conversions\n"
            "3. Consider using an intermediate cast",
            currentPointeeType->toString().c_str(),
            targetPointeeType->toString().c_str()
        );
        console.reportError(
            Console::TYPE_ERROR,
            Console::formatString("Invalid pointer cast from %s* to %s*",
                             currentPointeeType->toString().c_str(), targetPointeeType->toString().c_str()),
            suggestion,
            getSpan()
        );
        return nullptr;
    }
    // Case 2: Casting to integer (address as numeric value)
    else if (targetType->isInteger()) {
        return std::make_shared<IntegerLiteral>(static_cast<int64_t>(address));
    }
    // Case 3: Casting to boolean (null check)
    else if (targetType->isBool()) {
        return std::make_shared<BoolLiteral>(address != 0);
    }
    // Case 4: Casting to nullable pointer type
    else if (auto nullable = std::dynamic_pointer_cast<NullableType>(targetType)) {
        if (nullable->innerType->isPointer()) {
            if (address == 0) {
                return std::make_shared<Nullptr>(targetType);
            }

            auto nonNullable = std::make_shared<PointerLiteral>(
                address,
                targetType->getPointeeType());

            // Wrap in nullable container if needed
            return nonNullable;
        }
    }

    std::string suggestion = Console::formatString(
        "To resolve this:\n"
        "1. Verify target type '%s' is valid for pointer casting\n"
        "2. Check for supported cast operations\n"
        "3. Ensure correct type hierarchy",
        targetType->toString().c_str()
    );
    console.reportError(
        Console::TYPE_ERROR,
        Console::formatString("Invalid cast from pointer to %s",
                         targetType->toString().c_str()),
        suggestion,
        getSpan()
    );
    return nullptr;
}

std::shared_ptr<Expression> IntegerLiteral::express(SymbolTableType scope) {
    setSpanFromPosition(span.start.line, span.start.col, span.start.filePath);

    std::shared_ptr<Expression> result;

    if (!type) {
        DEBUG_LOG("Creating a 32-bit integer");
        type = Type::createPrimitiveType(Kind::Int32);
        result = std::make_shared<Integer<int32_t>>(static_cast<int32_t>(value));
        result->setSpan(this->getSpan());
        return result;
    }

    auto typeToCastFrom = std::make_shared<Type>(Kind::Int8);

    if (!Type::isSameOrCastableTo(typeToCastFrom, type)) {
        std::string suggestion = Console::formatString(
            "To resolve this:\n"
            "1. Ensure type '%s' is compatible with integer values\n"
            "2. Check for valid integer type casts\n"
            "3. Verify type annotations",
            type->toString().c_str()
        );
        console.reportError(
            Console::TYPE_ERROR,
            Console::formatString("The specified type is '%s' but '%s' is an integer",
                             type->toString().c_str(), std::to_string(value).c_str()),
            suggestion,
            getSpan()
        );
        return nullptr;
    } else {
        if (!type->isInteger()) {
            DEBUG_LOG("Casting integer to '" + type->toString() + "'.");
            if (type->isNullable()) {
                auto nullable = std::dynamic_pointer_cast<NullableType>(type);
                auto clone = this->clone();
                auto typed = std::dynamic_pointer_cast<TypedStatement>(clone);
                typed->setType(nullable->innerType);
                auto cast = std::make_shared<ASTCast>(clone, type);
                auto castResult = cast->express(scope);
                result = castResult;
                result->setSpan(this->getSpan());
                return result;
            }
            result = castTo(type)->express(scope);
            result->setSpan(this->getSpan());
            return result;
        }
        DEBUG_LOG("Creating an '" + type->toString() + "' integer");
    }

    // Check for specific bit-widths using the isInteger function with optional bitwidth argument
    if (type->isInteger(8)) {
        DEBUG_LOG("Creating an 8-bit integer");
        result = std::make_shared<Integer<int8_t>>(static_cast<int8_t>(value));
    }
    else if (type->isInteger(16)) {
        DEBUG_LOG("Creating a 16-bit integer");
        result = std::make_shared<Integer<int16_t>>(static_cast<int16_t>(value));
    }
    else if (type->isInteger(32)) {
        DEBUG_LOG("Creating a 32-bit integer");
        result = std::make_shared<Integer<int32_t>>(static_cast<int32_t>(value));
    }
    else if (type->isInteger(64)) {
        DEBUG_LOG("Creating a 64-bit integer");
        result = std::make_shared<Integer<int64_t>>(static_cast<int64_t>(value));
    }
    else if (type->isInteger(128)) {
        DEBUG_LOG("Creating a 128-bit integer");
        result = std::make_shared<BigInt>(std::to_string(value), 128);
    }
    else if (type->isInteger(256)) {
        DEBUG_LOG("Creating a 256-bit integer");
        result = std::make_shared<BigInt>(std::to_string(value), 256);
    }
    else if (type->isInteger(512)) {
        DEBUG_LOG("Creating a 512-bit integer");
        result = std::make_shared<BigInt>(std::to_string(value), 512);
    }
    else if (type->isInteger(1024)) {
        DEBUG_LOG("Creating a 1024-bit integer");
        result = std::make_shared<BigInt>(std::to_string(value), 1024);
    }

    if (result) {
        result->setSpan(this->getSpan());
        return result;
    }

    return nullptr;
}

std::shared_ptr<Literal> IntegerLiteral::castTo(std::shared_ptr<Type> targetType) const {
    if (targetType->isPointer()) {
        return std::make_shared<PointerLiteral>(value, targetType->getPointeeType());
    }

    using Kind = Kind;

    switch (targetType->getKind()) {
    case Kind::Int8:
    case Kind::Int16:
    case Kind::Int32:
    case Kind::Int64: {
        auto val = std::make_shared<IntegerLiteral>(static_cast<int64_t>(value));
        val->setType(targetType);
        return val;
    }

    case Kind::Half: {
        auto lit = std::make_shared<FloatLiteral>(static_cast<_Float16>(value));
        lit->isFloat16 = true;
        return lit;
    }
    case Kind::Float: {
        auto lit = std::make_shared<FloatLiteral>(static_cast<float>(value));
        lit->isFloat32 = true;
        return lit;
    }
    case Kind::Double: {
        auto lit = std::make_shared<FloatLiteral>(static_cast<double>(value));
        lit->isFloat64 = true;
        return lit;
    }
    case Kind::FP128:
    case Kind::PPC_FP128: {
        auto lit = std::make_shared<FloatLiteral>(static_cast<__float128>(value));
        lit->isFloat128 = true;
        return lit;
    }
    case Kind::X86_FP80: {
        auto lit = std::make_shared<FloatLiteral>(static_cast<long double>(value));
        lit->isFloat80 = true;
        return lit;
    }

    case Kind::Bool:
        return std::make_shared<BoolLiteral>(value != 0);

    case Kind::Char:
    case Kind::Char16:
    case Kind::Char32:
        return std::make_shared<CharacterLiteral>(static_cast<char32_t>(value));

    default:
        std::string suggestion = Console::formatString(
            "To resolve this:\n"
            "1. Verify target type '%s' is valid for integer casting\n"
            "2. Check for supported cast operations\n"
            "3. Ensure correct type hierarchy",
            targetType->toString().c_str()
        );
        console.reportError(
            Console::TYPE_ERROR,
            Console::formatString("Cannot cast an int to a '%s'",
                             targetType->toString().c_str()),
            suggestion,
            getSpan()
        );
        return nullptr;
    }
}

std::shared_ptr<Expression> FloatLiteral::express(SymbolTableType scope) {
    setSpanFromPosition(span.start.line, span.start.col, span.start.filePath);

    std::shared_ptr<Expression> result;

    // Default to 128-bit float if type is not specified
    if (!type) {
        if (isFloat16) {
            type = Type::createPrimitiveType(Kind::Half);
        }
        else if (isFloat32) {
            type = Type::createPrimitiveType(Kind::Float);
        }
        else if (isFloat64) {
            type = Type::createPrimitiveType(Kind::Double);
        }
        else if (isFloat80) {
            type = Type::createPrimitiveType(Kind::X86_FP80);
        }
        else if (isFloat128) {
            type = Type::createPrimitiveType(Kind::FP128);
        } else {
            // Default to 128-bit float if no suffix specified
            DEBUG_LOG("No suffix: defaulting to 128-bit float");
            type = Type::createPrimitiveType(Kind::Double);
        }
    }

    if (!Type::isSameOrCastableTo(rootType, type)) {
        std::string suggestion = Console::formatString(
            "To resolve this:\n"
            "1. Ensure type '%s' is compatible with float values\n"
            "2. Check for valid float type casts\n"
            "3. Verify type annotations",
            type->toString().c_str()
        );
        console.reportError(
            Console::TYPE_ERROR,
            Console::formatString("The specified type is '%s' but value is a float",
                             type->toString().c_str()),
            suggestion,
            getSpan()
        );
        return nullptr;
    } else {
        if (!type->isFloat()) {
            DEBUG_LOG("Casting float to '" + type->toString() + "'.");
            if (type->isNullable()) {
                auto nullable = std::dynamic_pointer_cast<NullableType>(type);
                auto clone = this->clone();
                auto typed = std::dynamic_pointer_cast<TypedStatement>(clone);
                typed->setType(nullable->innerType);
                auto cast = std::make_shared<ASTCast>(clone, type);
                auto castResult = cast->express(scope);
                result = castResult;
                result->setSpan(this->getSpan());
                return result;
            }
            result = castTo(type)->express(scope);
            result->setSpan(this->getSpan());
            return result;
        }
        DEBUG_LOG("Creating an '" + type->toString() + "' float.");
    }

    // Target-specific 16-bit float handling
    #ifdef __ARM_ARCH
        if (type->isFloat(16)) {
            DEBUG_LOG("Creating a 16-bit float (__fp16 for ARM)");
            result = std::make_shared<Float<__fp16>>(static_cast<__fp16>(value));
        }
    #elif defined(__x86_64__) || defined(__i386__)
        if (type->isFloat(16)) {
            DEBUG_LOG("Creating a 16-bit float (_Float16 for x86)");
            result = std::make_shared<Float<_Float16>>(static_cast<_Float16>(value));
        }
    #endif

    if (!result && type->isFloat(32)) {
        DEBUG_LOG("Creating a 32-bit float");
        result = std::make_shared<Float<float>>(static_cast<float>(value));
    }

    if (!result && type->isFloat(64)) {
        DEBUG_LOG("Creating a 64-bit float");
        result = std::make_shared<Float<double>>(static_cast<double>(value));
    }

    if (!result && type->isFloat(80)) {
        DEBUG_LOG("Creating an 80-bit float (X86_FP80)");
        result = std::make_shared<Float<long double>>(static_cast<long double>(value));
    }

    if (!result && type->isFloat(128)) {
        DEBUG_LOG("Creating a 128-bit float (FP128 or PPC_FP128)");
        result = std::make_shared<Float<__float128>>(value);
    }

    if (result) {
        result->setSpan(this->getSpan());
        return result;
    }

    return nullptr;
}

std::shared_ptr<Literal> FloatLiteral::castTo(std::shared_ptr<Type> targetType) const {
    using Kind = Kind;

    switch (targetType->getKind()) {
    case Kind::Half: {
        auto lit = std::make_shared<FloatLiteral>(static_cast<_Float16>(value));
        lit->isFloat16 = true;
        return lit;
    }
    case Kind::Float: {
        auto lit = std::make_shared<FloatLiteral>(static_cast<float>(value));
        lit->isFloat32 = true;
        return lit;
    }
    case Kind::Double: {
        auto lit = std::make_shared<FloatLiteral>(static_cast<double>(value));
        lit->isFloat64 = true;
        return lit;
    }
    case Kind::FP128:
    case Kind::PPC_FP128: {
        auto lit = std::make_shared<FloatLiteral>(static_cast<__float128>(value));
        lit->isFloat128 = true;
        return lit;
    }
    case Kind::X86_FP80: {
        auto lit = std::make_shared<FloatLiteral>(static_cast<long double>(value));
        lit->isFloat80 = true;
        return lit;
    }

    case Kind::Int8:
    case Kind::Int16:
    case Kind::Int32:
    case Kind::Int64: {
        auto val = std::make_shared<IntegerLiteral>(static_cast<int64_t>(value));
        val->setType(targetType);
        return val;
    }

    case Kind::Char:
    case Kind::Char16:
    case Kind::Char32: {
        auto val = static_cast<char32_t>(static_cast<int64_t>(value));
        return std::make_shared<CharacterLiteral>(val);
    }

    case Kind::Bool:
        return std::make_shared<BoolLiteral>(value != 0.0);

    default:
        std::string suggestion = Console::formatString(
            "To resolve this:\n"
            "1. Verify target type '%s' is valid for float casting\n"
            "2. Check for supported cast operations\n"
            "3. Ensure correct type hierarchy",
            targetType->toString().c_str()
        );
        console.reportError(
            Console::TYPE_ERROR,
            Console::formatString("Cannot cast a float to a '%s'",
                             targetType->toString().c_str()),
            suggestion,
            getSpan()
        );
        return nullptr;
    }
}

// Arbitrary-precision integer (BigInt)
std::shared_ptr<Expression> ASTBigInt::express(SymbolTableType scope) {
    setSpanFromPosition(span.start.line, span.start.col, span.start.filePath);
    DEBUG_LOG("Creating a big int " + value);
    unsigned bitWidth = ASTBigInt::determineBitWidth(value);
    auto result = std::make_shared<BigInt>(value, bitWidth);

    result->setSpan(this->getSpan());
    return result;
}

std::shared_ptr<Expression> Invalid::express(SymbolTableType scope) {
    setSpanFromPosition(span.start.line, span.start.col, span.start.filePath);
    DEBUG_LOG("Creating an invalid");
    auto result = std::make_shared<InvalidExpression>();

    result->setSpan(this->getSpan());
    return result;
}

std::shared_ptr<Expression> BoolLiteral::express(SymbolTableType scope) {
    setSpanFromPosition(span.start.line, span.start.col, span.start.filePath);

    std::shared_ptr<Expression> result;

    if (!type) {
        DEBUG_LOG("Creating a bool false");
        type = Type::createPrimitiveType(Kind::Bool);
        result = std::make_shared<Primitive<bool>>(value);
        result->setSpan(this->getSpan());
        return result;
    }

    auto typeToCastFrom = std::make_shared<Type>(Kind::Bool);

    if (!Type::isSameOrCastableTo(typeToCastFrom, type)) {
        std::string suggestion = Console::formatString(
            "To resolve this:\n"
            "1. Ensure type '%s' is compatible with boolean values\n"
            "2. Check for valid boolean type casts\n"
            "3. Verify type annotations",
            type->toString().c_str()
        );
        console.reportError(
            Console::TYPE_ERROR,
            Console::formatString("The specified type is '%s' but '%s' is a bool",
                             type->toString().c_str(), std::to_string(value).c_str()),
            suggestion,
            getSpan()
        );
        return nullptr;
    } else {
        if (!type->isBool()) {
            DEBUG_LOG("Casting bool to '" + type->toString() + "'.");
            if (type->isNullable()) {
                auto nullable = std::dynamic_pointer_cast<NullableType>(type);
                auto clone = this->clone();
                auto typed = std::dynamic_pointer_cast<TypedStatement>(clone);
                typed->setType(nullable->innerType);
                auto cast = std::make_shared<ASTCast>(clone, type);
                auto castResult = cast->express(scope);
                result = castResult;
                result->setSpan(this->getSpan());
                return result;
            }
            result = castTo(type)->express(scope);
            result->setSpan(this->getSpan());
            return result;
        }
        DEBUG_LOG("Creating an '" + type->toString() + "'.");
    }

    result = std::make_shared<Primitive<bool>>(value);

    result->setSpan(this->getSpan());
    return result;
}

std::shared_ptr<Literal> BoolLiteral::castTo(std::shared_ptr<Type> targetType) const {
    using Kind = Kind;

    switch (targetType->getKind()) {
    case Kind::Bool:
        return std::make_shared<BoolLiteral>(value);
    case Kind::Int8:
    case Kind::Int16:
    case Kind::Int32:
    case Kind::Int64:
        return std::make_shared<IntegerLiteral>(value ? 1 : 0);
    case Kind::Half: {
        auto lit = std::make_shared<FloatLiteral>(static_cast<_Float16>(value ? 1.0f : 0.0f));
        lit->isFloat16 = true;
        return lit;
    }
    case Kind::Float: {
        auto lit = std::make_shared<FloatLiteral>(static_cast<float>(value ? 1.0f : 0.0f));
        lit->isFloat32 = true;
        return lit;
    }
    case Kind::Double: {
        auto lit = std::make_shared<FloatLiteral>(static_cast<double>(value ? 1.0 : 0.0));
        lit->isFloat64 = true;
        return lit;
    }
    case Kind::FP128:
    case Kind::PPC_FP128: {
        auto lit = std::make_shared<FloatLiteral>(static_cast<__float128>(value ? 1.0 : 0.0));
        lit->isFloat128 = true;
        return lit;
    }
    case Kind::X86_FP80: {
        auto lit = std::make_shared<FloatLiteral>(static_cast<long double>(value ? 1.0L : 0.0L));
        lit->isFloat80 = true;
        return lit;
    }
    default:
        std::string suggestion = Console::formatString(
            "To resolve this:\n"
            "1. Verify target type '%s' is valid for boolean casting\n"
            "2. Check for supported cast operations\n"
            "3. Ensure correct type hierarchy",
            targetType->toString().c_str()
        );
        console.reportError(
            Console::TYPE_ERROR,
            Console::formatString("Cannot cast a bool to a '%s'",
                             targetType->toString().c_str()),
            suggestion,
            getSpan()
        );
        return nullptr;
    }
}

std::shared_ptr<Expression> CharacterLiteral::express(SymbolTableType scope) {
    setSpanFromPosition(span.start.line, span.start.col, span.start.filePath);

    std::shared_ptr<Expression> result;

    if (!type) {
        DEBUG_LOG("Creating a char literal");
        type = Type::createPrimitiveType(Kind::Char);
        auto utf8 = utf32_to_utf8(std::u32string(1, value));
        result = std::make_shared<Primitive<char>>(utf8[0]);
        result->setSpan(this->getSpan());
        return result;
    }

    if (!type->isChar()) {
        if (type->isNullable()) {
            auto nullable = std::dynamic_pointer_cast<NullableType>(type);
            auto clone = this->clone();
            auto typed = std::dynamic_pointer_cast<TypedStatement>(clone);
            typed->setType(nullable->innerType);
            auto cast = std::make_shared<ASTCast>(clone, type);
            auto castResult = cast->express(scope);
            result = castResult;
            result->setSpan(this->getSpan());
            return result;
        }
        std::string suggestion = Console::formatString(
            "To resolve this:\n"
            "1. Ensure type '%s' is compatible with character values\n"
            "2. Check for valid character type casts\n"
            "3. Verify type annotations",
            type->toString().c_str()
        );
        console.reportError(
            Console::TYPE_ERROR,
            Console::formatString("The specified type is '%s' but '%s' is a char",
                             type->toString().c_str(), std::to_string(static_cast<uint32_t>(value)).c_str()),
            suggestion,
            getSpan()
        );
        return nullptr;
    } else {
        DEBUG_LOG("Creating a '" + type->toString() + "' value.");
    }

    if (type->isChar(8)) {
        DEBUG_LOG("Creating UTF-8 char");
        std::string utf8_value = utf32_to_utf8(std::u32string(1, value));
        result = std::make_shared<Primitive<char>>(utf8_value[0]);
    }
    else if (type->isChar(16)) {
        DEBUG_LOG("Creating UTF-16 char");
        std::u16string utf16_value = utf32_to_utf16(std::u32string(1, value));
        result = std::make_shared<Primitive<char16_t>>(utf16_value[0]);
    }
    else if (type->isChar(32)) {
        DEBUG_LOG("Creating UTF-32 char");
        result = std::make_shared<Primitive<char32_t>>(value);
    }

    if (result) {
        result->setSpan(this->getSpan());
        return result;
    }

    return nullptr;
}

std::shared_ptr<Literal> CharacterLiteral::castTo(std::shared_ptr<Type> targetType) const {
    using Kind = Kind;
    std::shared_ptr<Literal> result = nullptr;

    switch (targetType->getKind()) {
    case Kind::Char:
    case Kind::Char16:
    case Kind::Char32:
        result = std::make_shared<CharacterLiteral>(value);
        break;
    case Kind::Int8:
    case Kind::Int16:
    case Kind::Int32:
    case Kind::Int64: {
        auto val = std::make_shared<IntegerLiteral>(static_cast<int64_t>(value));
        val->setType(targetType);
        result = val;
        break;
    }
    case Kind::Half: {
        auto lit = std::make_shared<FloatLiteral>(static_cast<_Float16>(value));
        lit->isFloat16 = true;
        result = lit;
        break;
    }
    case Kind::Float: {
        auto lit = std::make_shared<FloatLiteral>(static_cast<float>(value));
        lit->isFloat32 = true;
        result = lit;
        break;
    }
    case Kind::Double: {
        auto lit = std::make_shared<FloatLiteral>(static_cast<double>(value));
        lit->isFloat64 = true;
        result = lit;
        break;
    }
    case Kind::FP128:
    case Kind::PPC_FP128: {
        auto lit = std::make_shared<FloatLiteral>(static_cast<__float128>(value));
        lit->isFloat128 = true;
        result = lit;
        break;
    }
    case Kind::X86_FP80: {
        auto lit = std::make_shared<FloatLiteral>(static_cast<long double>(value));
        lit->isFloat80 = true;
        result = lit;
        break;
    }
    case Kind::Bool:
        result = std::make_shared<BoolLiteral>(value != 0);
        break;
    case Kind::String:
    case Kind::Utf8:
    case Kind::Utf16:
    case Kind::Utf32: {
        auto utf32 = std::u32string(1, value);
        auto val = std::make_shared<StringLiteral>(utf32);
        val->setType(targetType);
        result = val;
        break;
    }
    default:
        std::string suggestion = Console::formatString(
            "To resolve this:\n"
            "1. Verify target type '%s' is valid for character casting\n"
            "2. Check for supported cast operations\n"
            "3. Ensure correct type hierarchy",
            targetType->toString().c_str()
        );
        console.reportError(
            Console::TYPE_ERROR,
            Console::formatString("Cannot cast a char to a '%s'",
                             targetType->toString().c_str()),
            suggestion,
            getSpan()
        );
        return nullptr;
    }

    if (result) {
        result->setSpan(this->getSpan());
    }
    return result;
}

std::shared_ptr<Expression> StringLiteral::express(SymbolTableType scope) {
    setSpanFromPosition(span.start.line, span.start.col, span.start.filePath);
    std::shared_ptr<Expression> result = nullptr;

    if (!type) {
        DEBUG_LOG("Creating UTF-8 string");
        type = rootType;
        std::string utf8_value = utf32_to_utf8(value);
        result = std::make_shared<StringExpression<std::string>>(utf8_value);
    }
    else if (!type->isPointer()) {
        DEBUG_LOG("Casting char* to '" + type->toString() + "'.");
        if (type->isNullable()) {
            auto nullable = std::dynamic_pointer_cast<NullableType>(type);
            auto clone = this->clone();
            auto typed = std::dynamic_pointer_cast<TypedStatement>(clone);
            typed->setType(nullable->innerType);
            auto cast = std::make_shared<ASTCast>(clone, type);
            result = cast->express(scope);
            return result;
        }
        std::string suggestion = Console::formatString(
            "To resolve this:\n"
            "1. Ensure type '%s' is a pointer type for string literals\n"
            "2. Check for valid string type casts\n"
            "3. Verify type annotations",
            type->toString().c_str()
        );
        console.reportError(
            Console::TYPE_ERROR,
            Console::formatString("Cannot cast a char* to a %s",
                             type->toString().c_str()),
            suggestion,
            getSpan()
        );
        return nullptr;
    }
    else {
        std::shared_ptr<Type> pointeeType = type->getPointeeType();

        if (!Type::isSameOrCastableTo(rootType, type)) {
            std::string suggestion = Console::formatString(
                "To resolve this:\n"
                "1. Ensure type '%s' is compatible with string literals\n"
                "2. Check for valid string type casts\n"
                "3. Verify type annotations",
                type->toString().c_str()
            );
            console.reportError(
                Console::TYPE_ERROR,
                Console::formatString("The specified type is '%s' but a UTF-8 string was given",
                                 type->toString().c_str()),
                suggestion,
                getSpan()
            );
            return nullptr;
        } else {
            DEBUG_LOG("Creating a '" + type->toString() + "' string literal.");
        }

        if (pointeeType->isString(8) || pointeeType->isChar(8)) {
            DEBUG_LOG("Creating UTF-8 string");
            type = rootType;
            std::string utf8_value = utf32_to_utf8(value);
            result = std::make_shared<StringExpression<std::string>>(utf8_value);
        }
        else if (pointeeType->isString(16) || pointeeType->isChar(16)) {
            DEBUG_LOG("Creating UTF-16 string");
            auto char16Type = Type::createPrimitiveType(Kind::Char16);
            auto string16Type = Type::createPointerType(char16Type);
            type = string16Type;
            std::u16string utf16_value = utf32_to_utf16(value);
            result = std::make_shared<StringExpression<std::u16string>>(utf16_value);
        }
        else if (pointeeType->isString(32) || pointeeType->isChar(32)) {
            DEBUG_LOG("Creating UTF-32 string");
            auto char32Type = Type::createPrimitiveType(Kind::Char32);
            auto string32Type = Type::createPointerType(char32Type);
            type = string32Type;
            result = std::make_shared<StringExpression<std::u32string>>(value);
        }
    }

    if (result) {
        result->setSpan(this->getSpan());
    }
    return result;
}

std::shared_ptr<Literal> StringLiteral::castTo(std::shared_ptr<Type> targetType) const {
    using Kind = Kind;
    std::shared_ptr<Literal> result = nullptr;

    switch (targetType->getKind()) {
    case Kind::String:
    case Kind::Utf8:
    case Kind::Utf16:
    case Kind::Utf32:
        result = std::make_shared<StringLiteral>(value);
        break;
    case Kind::Char:
    case Kind::Char16:
    case Kind::Char32: {
        if (!value.empty()) {
            auto val = std::make_shared<CharacterLiteral>(value[0]);
            val->setType(targetType);
            result = val;
        } else {
            std::string suggestion = "To resolve this:\n"
                                   "1. Ensure the string is not empty\n"
                                   "2. Check for valid character extraction\n"
                                   "3. Verify string initialization";
            console.reportError(
                Console::RUNTIME_ERROR,
                "Cannot cast an empty string to a character type",
                suggestion,
                getSpan()
            );
        }
        break;
    }
    case Kind::Bool:
        result = std::make_shared<BoolLiteral>(!value.empty());
        break;
    default:
        std::string suggestion = Console::formatString(
            "To resolve this:\n"
            "1. Verify target type '%s' is valid for string casting\n"
            "2. Check for supported cast operations\n"
            "3. Ensure correct type hierarchy",
            targetType->toString().c_str()
        );
        console.reportError(
            Console::TYPE_ERROR,
            Console::formatString("Cannot cast a char* to a '%s'",
                             targetType->toString().c_str()),
            suggestion,
            getSpan()
        );
        return nullptr;
    }

    if (result) {
        result->setSpan(this->getSpan());
    }
    return result;
}

std::shared_ptr<Expression> Array::express(SymbolTableType scope) {
    setSpanFromPosition(span.start.line, span.start.col, span.start.filePath);
    DEBUG_LOG("[Array] Creating an array");
    std::shared_ptr<Expression> result = nullptr;

    if (!type) {
        DEBUG_LOG("[Array] The array has no explicit type. Inferring...");

        std::vector<std::shared_ptr<Expression>> values;
        std::vector<std::shared_ptr<Type>> seenTypes;

        for (const auto& expr : initialValues) {
            auto val = expr->express(scope);
            if (!val) continue;
            values.push_back(val);
            seenTypes.push_back(val->getType());
        }

        if (values.empty()) {
            std::string suggestion = "To resolve this:\n"
                                   "1. Ensure the array initializer contains valid expressions\n"
                                   "2. Check for proper expression definitions\n"
                                   "3. Add debug output to trace initializer values";
            console.reportError(
                Console::TYPE_ERROR,
                "Cannot infer array type from empty initializer",
                suggestion,
                getSpan()
            );
            return nullptr;
        }

        // Find best common type
        std::shared_ptr<Type> bestType = seenTypes[0];
        for (size_t i = 1; i < seenTypes.size(); ++i) {
            auto& currentType = seenTypes[i];
            if (Type::isSame(bestType, currentType)) {
                continue;
            }
            if (Type::isSameOrCastableTo(currentType, bestType)) {
                continue;
            } else if (Type::isSameOrCastableTo(bestType, currentType)) {
                bestType = currentType;
            } else {
                std::string suggestion = Console::formatString(
                    "To resolve this:\n"
                    "1. Ensure types '%s' and '%s' are compatible\n"
                    "2. Check for valid type conversions\n"
                    "3. Consider explicit type casting",
                    bestType->toString().c_str(), currentType->toString().c_str()
                );
                console.reportError(
                    Console::TYPE_ERROR,
                    Console::formatString("Cannot infer a common array type between '%s' and '%s'",
                                     bestType->toString().c_str(), currentType->toString().c_str()),
                    suggestion,
                    getSpan()
                );
                return nullptr;
            }
        }

        // Cast all mismatches
        std::vector<std::shared_ptr<Expression>> castedValues;
        for (const auto& val : values) {
            auto valType = val->getType();
            if (Type::isSame(valType, bestType)) {
                castedValues.push_back(val);
            } else if (Type::isSameOrCastableTo(valType, bestType)) {
                castedValues.push_back(std::make_shared<CastExpression>(val, bestType));
            } else {
                std::string suggestion = Console::formatString(
                    "To resolve this:\n"
                    "1. Ensure element type '%s' can be cast to '%s'\n"
                    "2. Check for valid type conversions\n"
                    "3. Verify array element types",
                    valType->toString().c_str(), bestType->toString().c_str()
                );
                console.reportError(
                    Console::TYPE_ERROR,
                    Console::formatString("Cannot cast array element of type '%s' to inferred type '%s'",
                                     valType->toString().c_str(), bestType->toString().c_str()),
                    suggestion,
                    getSpan()
                );
                return nullptr;
            }
        }

        auto arrayType = Type::createFixedArrayType(bestType, castedValues.size());
        setType(arrayType);
        setRootType(arrayType);

        result = std::make_shared<FixedArrayExpression>(castedValues, bestType);
    }
    else if (type->isArray()) {
        DEBUG_LOG("[Array] The array has a declared type: '" + type->toString() + "'");
        DEBUG_LOG("[Array] Creating a fixed array with declared element type");

        std::vector<std::shared_ptr<Expression>> values;
        auto expectedElementType = type->elementType;
        size_t n = 0;

        for (const auto& expr : initialValues) {
            std::shared_ptr<Expression> val;

            if (auto typed = std::dynamic_pointer_cast<TypedStatement>(expr)) {
                if (!typed->getType()) typed->setType(expectedElementType);
            }

            val = expr->express(scope);
            if (!val) continue;

            auto actualType = val->getType();
            if (Type::isSame(actualType, expectedElementType)) {
                values.push_back(val);
            } else if (Type::isSameOrCastableTo(actualType, expectedElementType)) {
                values.push_back(std::make_shared<CastExpression>(val, expectedElementType));
            } else {
                std::string suggestion = Console::formatString(
                    "To resolve this:\n"
                    "1. Ensure element type '%s' matches or is castable to '%s'\n"
                    "2. Check for valid type conversions\n"
                    "3. Verify array element types",
                    actualType->toString().c_str(), expectedElementType->toString().c_str()
                );
                console.reportError(
                    Console::TYPE_ERROR,
                    Console::formatString("Element %s has type '%s' but expected '%s'",
                                     std::to_string(n).c_str(), actualType->toString().c_str(),
                                     expectedElementType->toString().c_str()),
                    suggestion,
                    getSpan()
                );
                return nullptr;
            }

            n++;
        }

        if (n != type->fixedSize) {
            std::string suggestion = Console::formatString(
                "To resolve this:\n"
                "1. Ensure array has exactly %s elements\n"
                "2. Check array initializer syntax\n"
                "3. Verify array size declaration",
                std::to_string(type->fixedSize).c_str()
            );
            console.reportError(
                Console::TYPE_ERROR,
                Console::formatString("Array expects '%s' elements or less but got '%s' instead",
                                 std::to_string(type->fixedSize).c_str(), std::to_string(n).c_str()),
                suggestion,
                getSpan()
            );
            return nullptr;
        }

        result = std::make_shared<FixedArrayExpression>(values, expectedElementType);
    }
    else if (type->isDynamicArray()) {
        DEBUG_LOG("[Array] Creating a dynamic Array");
        // TODO: Implement
        return nullptr;
    }
    else if (type->isHeterogeneousArray()) {
        DEBUG_LOG("[Array] Creating a heterogeneous Array");
        // TODO: Implement
        return nullptr;
    }
    else {
        std::string suggestion = Console::formatString(
            "To resolve this:\n"
            "1. Ensure type '%s' is a valid array type\n"
            "2. Check for correct array type declaration\n"
            "3. Verify type compatibility",
            type->toString().c_str()
        );
        console.reportError(
            Console::TYPE_ERROR,
            Console::formatString("The array has an invalid declared type: '%s'",
                             type->toString().c_str()),
            suggestion,
            getSpan()
        );
        return nullptr;
    }

    if (result) {
        result->setSpan(this->getSpan());
    }
    return result;
}

std::shared_ptr<Literal> Array::castTo(std::shared_ptr<Type> targetType) const {
    std::string suggestion = Console::formatString(
        "To resolve this:\n"
        "1. Verify target type '%s' is valid for array casting\n"
        "2. Check for supported array cast operations\n"
        "3. Ensure correct type hierarchy",
        targetType->toString().c_str()
    );
    console.reportError(
        Console::TYPE_ERROR,
        Console::formatString("Cannot cast array to type '%s'",
                         targetType->toString().c_str()),
        suggestion,
        getSpan()
    );
    return nullptr;
}

} //namespace Omniscript
