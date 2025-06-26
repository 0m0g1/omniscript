<<<<<<< HEAD:src/Statements/Literals.cpp
#include <omniscript/Statement.h>
#include <omniscript/Statements/LiteralStatements.h>

#include <omniscript/Core.h>
#include <omniscript/utils.h>
#include <omniscript/omniscript_pch.h>
#include <omniscript/Statement.h>
#include <omniscript/Symboltable.h>
=======
#include <omniscript/engine/Statement.h>
#include <omniscript/engine/Statements/LiteralStatements.h>

#include <omniscript/engine/Core.h>
#include <omniscript/utils.h>
#include <omniscript/omniscript_pch.h>
#include <omniscript/engine/Statement.h>
#include <omniscript/engine/Symboltable.h>
>>>>>>> 7ccebff50dd27e70cffd4d578dcb358f4c9e1613:src/engine/Statements/Literals.cpp

// ============================== Literals and casting  ============================== //
std::shared_ptr<Omniscript::Expression> Cast::express(SymbolTableType scope) {
    Omniscript::setPosition(pos.line, pos.col, pos.filePath);
    DEBUG_LOG("");

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
            console.error("Cannot cast " + value->toString() + " it has no type.");
        } else {
            console.error("There is no value to cast.");
        }
    }

    extendContextOf(value);

    // Literal cast (handles constant folding, primitive -> primitive, etc.)
    if (auto literal = std::dynamic_pointer_cast<Literal>(value)) {
        if (!targetType->isNullable()) {
            auto castedStmt = literal->castTo(targetType);
            return castedStmt->express(scope);
        }
    }

    auto result = value->express(scope);

    // If casting to a nullable type
    if (targetType->isNullable()) {
        // If the result is already null or nullable, just return as NullableExpression
        if (std::dynamic_pointer_cast<Omniscript::NullableExpression>(result)) {
            return result;
        }

        // If we're casting a null literal (e.g., NullExpression or NullPointerExpression)
        if (std::dynamic_pointer_cast<Omniscript::NullExpression>(result) ||
            std::dynamic_pointer_cast<Omniscript::NullPointerExpression>(result)) {
            return std::make_shared<Omniscript::NullableExpression>();
        }

        // Wrap any expression in a NullableExpression
        return std::make_shared<Omniscript::NullableExpression>(result);
    }

    // Normal cast expression fallback
    return std::make_shared<Omniscript::CastExpression>(result, targetType);
}

std::shared_ptr<Omniscript::Expression> Nullptr::express(SymbolTableType scope) {
    Omniscript::setPosition(pos.line, pos.col, pos.filePath);

    auto nullpointerType = type ? type : Omniscript::Type::createPrimitiveType(Omniscript::Kind::Void);

    auto result = Omniscript::make_expression<Omniscript::NullPointerExpression>(type);

    return result;
}

std::shared_ptr<Omniscript::Expression> Null::express(SymbolTableType scope) {
    Omniscript::setPosition(pos.line, pos.col, pos.filePath);
    if (type->isNullable()) {
        auto nullable = std::dynamic_pointer_cast<Omniscript::NullableType>(type);
        auto nullableExpr = std::make_shared<Omniscript::NullableExpression>();
        nullableExpr->type = nullable->innerType ? nullable->innerType : Omniscript::Type::createPrimitiveType(Omniscript::Kind::Void);
        nullableExpr->rootType = nullable->innerType ? nullable->innerType : Omniscript::Type::createPrimitiveType(Omniscript::Kind::Void);
        return nullableExpr;
    }

    return Omniscript::make_expression<Omniscript::NullExpression>(
        type ? type : Omniscript::Type::createPrimitiveType(Omniscript::Kind::Void));
}

std::shared_ptr<Omniscript::Expression> PointerLiteral::express(SymbolTableType scope) {
    Omniscript::setPosition(pos.line, pos.col, pos.filePath);
    // Handle null pointer case
    if (address == 0) {
        return Omniscript::make_expression<Omniscript::NullPointerExpression>(type->getPointeeType());
    }

    // Create raw pointer expression
    auto pointeeType = type ? type->getPointeeType() : Omniscript::Type::createPrimitiveType(Omniscript::Kind::Void);

    auto ptrType = Omniscript::Type::createPointerType(pointeeType, isConst, isVolatile);

    return Omniscript::make_expression<Omniscript::RawPointerExpression>(
        address,
        ptrType);
}

std::shared_ptr<Literal> PointerLiteral::castTo(std::shared_ptr<Omniscript::Type> targetType) const {
    using Kind = Omniscript::Kind;

    // Handle null pointer case
    if (address == 0) {
        return std::make_shared<Nullptr>(targetType);
    }

    // Get current pointee type (defaults to void if unspecified) 
    auto currentPointeeType = type ? type->getPointeeType() : Omniscript::Type::createPrimitiveType(Kind::Void);

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
        if (Omniscript::Type::isSameOrCastableTo(currentPointeeType, targetPointeeType)) {
            return std::make_shared<PointerLiteral>(
                address,
                targetPointeeType);
        }

        console.error("Invalid pointer cast from " + currentPointeeType->toString() +
                      "* to " + targetPointeeType->toString() + "*");
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
    else if (auto nullable = std::dynamic_pointer_cast<Omniscript::NullableType>(targetType)) {
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

    console.error("Invalid cast from pointer to " + targetType->toString());
    return nullptr;
}

std::shared_ptr<Omniscript::Expression> IntegerLiteral::express(SymbolTableType scope) {
    Omniscript::setPosition(pos.line, pos.col, pos.filePath);
    if (!type) {
        DEBUG_LOG("Creating a 32-bit integer");
        type = Omniscript::Type::createPrimitiveType(Omniscript::Kind::Int32);
        return std::make_shared<Omniscript::Integer<int32_t>>(static_cast<int32_t>(value));
    }

    auto typeToCastFrom = std::make_shared<Omniscript::Type>(Omniscript::Kind::Int8);

    if (!Omniscript::Type::isSameOrCastableTo(typeToCastFrom, type)) {
        console.error("The specified type is '" + type->toString() + "' but '" + std::to_string(value) + "' is an integer.");
    } else {
        if (!type->isInteger()) {
            DEBUG_LOG("Casting integer to '" + type->toString() + "'.");
            if (type->isNullable()) {
                auto nullable = std::dynamic_pointer_cast<Omniscript::NullableType>(type);
                auto clone = this->clone();
                auto typed = std::dynamic_pointer_cast<TypedStatement>(clone);
                typed->setType(nullable->innerType);
                auto cast = std::make_shared<Cast>(clone, type);
                auto castResult = cast->express(scope);
                return castResult;
            }
            return castTo(type)->express(scope);
        }
        DEBUG_LOG("Creating an '" + type->toString() + "' integer");
    }

    // Check for specific bit-widths using the isInteger function with optional bitwidth argument
    if (type->isInteger(8)) {
        DEBUG_LOG("Creating an 8-bit integer");
        return std::make_shared<Omniscript::Integer<int8_t>>(static_cast<int8_t>(value));
    }
    else if (type->isInteger(16)) {
        DEBUG_LOG("Creating a 16-bit integer");
        return std::make_shared<Omniscript::Integer<int16_t>>(static_cast<int16_t>(value));
    }
    else if (type->isInteger(32)) {
        DEBUG_LOG("Creating a 32-bit integer");
        return std::make_shared<Omniscript::Integer<int32_t>>(static_cast<int32_t>(value));
    }
    else if (type->isInteger(64)) {
        DEBUG_LOG("Creating a 64-bit integer");
        return std::make_shared<Omniscript::Integer<int64_t>>(static_cast<int64_t>(value));
    }
    else if (type->isInteger(128)) {
        DEBUG_LOG("Creating a 128-bit integer");
        return std::make_shared<Omniscript::BigInt>(std::to_string(value), 128);
    }
    else if (type->isInteger(256)) {
        DEBUG_LOG("Creating a 256-bit integer");
        return std::make_shared<Omniscript::BigInt>(std::to_string(value), 256);
    }
    else if (type->isInteger(512)) {
        DEBUG_LOG("Creating a 512-bit integer");
        return std::make_shared<Omniscript::BigInt>(std::to_string(value), 512);
    }
    else if (type->isInteger(1024)) {
        DEBUG_LOG("Creating a 1024-bit integer");
        return std::make_shared<Omniscript::BigInt>(std::to_string(value), 1024);
    }

    return nullptr;
}

std::shared_ptr<Literal> IntegerLiteral::castTo(std::shared_ptr<Omniscript::Type> targetType) const {
    if (targetType->isPointer()) {
        return std::make_shared<PointerLiteral>(value, targetType->getPointeeType());
    }

    using Kind = Omniscript::Kind;

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
        console.error("Cannot cast an int to a '" + targetType->toString() + "'.");
        return nullptr;
    }
}

std::shared_ptr<Omniscript::Expression> FloatLiteral::express(SymbolTableType scope) {
    Omniscript::setPosition(pos.line, pos.col, pos.filePath);
    // Default to 128-bit float if type is not specified
    if (!type) {
        if (isFloat16) {
            type = Omniscript::Type::createPrimitiveType(Omniscript::Kind::Half);
        }
        else if (isFloat32) {
            type = Omniscript::Type::createPrimitiveType(Omniscript::Kind::Float);
        }
        else if (isFloat64) {
            type = Omniscript::Type::createPrimitiveType(Omniscript::Kind::Double);
        }
        else if (isFloat80) {
            type = Omniscript::Type::createPrimitiveType(Omniscript::Kind::X86_FP80);
        }
        else if (isFloat128) {
            type = Omniscript::Type::createPrimitiveType(Omniscript::Kind::FP128);
        } else {
            // Default to 128-bit float if no suffix specified
            DEBUG_LOG("No suffix: defaulting to 128-bit float");
            type = Omniscript::Type::createPrimitiveType(Omniscript::Kind::Double);
        }
    }

    if (!Omniscript::Type::isSameOrCastableTo(rootType, type)) {
        console.error("The specified type is " + type->toString() +
                      " but '" + /* custom __float128 to string needed here */ "' is a float.");
    } else {
        if (!type->isFloat()) {
            DEBUG_LOG("Casting float to '" + type->toString() + "'.");
            if (type->isNullable()) {
                auto nullable = std::dynamic_pointer_cast<Omniscript::NullableType>(type);
                auto clone = this->clone();
                auto typed = std::dynamic_pointer_cast<TypedStatement>(clone);
                typed->setType(nullable->innerType);
                auto cast = std::make_shared<Cast>(clone, type);
                auto castResult = cast->express(scope);
                return castResult;
            }
            return castTo(type)->express(scope);
        }
        DEBUG_LOG("Creating an '" + type->toString() + "' float.");
    }

    // Target-specific 16-bit float handling
    #ifdef __ARM_ARCH
        if (type->isFloat(16)) {
            DEBUG_LOG("Creating a 16-bit float (__fp16 for ARM)");
            return std::make_shared<Omniscript::Float<__fp16>>(static_cast<__fp16>(value));
        }
    #elif defined(__x86_64__) || defined(__i386__) 
        if (type->isFloat(16)) {
            DEBUG_LOG("Creating a 16-bit float (_Float16 for x86)");
            return std::make_shared<Omniscript::Float<_Float16>>(static_cast<_Float16>(value));
        }
    #endif

    if (type->isFloat(32)) {
        DEBUG_LOG("Creating a 32-bit float");
        return std::make_shared<Omniscript::Float<float>>(static_cast<float>(value));
    }

    if (type->isFloat(64)) {
        DEBUG_LOG("Creating a 64-bit float");
        return std::make_shared<Omniscript::Float<double>>(static_cast<double>(value));
    }

    if (type->isFloat(80)) {
        DEBUG_LOG("Creating an 80-bit float (X86_FP80)");
        return std::make_shared<Omniscript::Float<long double>>(static_cast<long double>(value));
    }

    if (type->isFloat(128)) {
        DEBUG_LOG("Creating a 128-bit float (FP128 or PPC_FP128)");
        return std::make_shared<Omniscript::Float<__float128>>(value);
    }

    return nullptr;
}

std::shared_ptr<Literal> FloatLiteral::castTo(std::shared_ptr<Omniscript::Type> targetType) const {
    using Kind = Omniscript::Kind;

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
        console.error("Cannot cast an float to a '" + targetType->toString() + "'.");
        return nullptr;
    }
}

// Arbitrary-precision integer (BigInt)
std::shared_ptr<Omniscript::Expression> BigInt::express(SymbolTableType scope) {
    Omniscript::setPosition(pos.line, pos.col, pos.filePath);
    DEBUG_LOG("Creating a big int " + value);
    unsigned bitWidth = BigInt::determineBitWidth(value);
    return std::make_shared<Omniscript::BigInt>(value, bitWidth);
}

std::shared_ptr<Omniscript::Expression> Invalid::express(SymbolTableType scope) {
    Omniscript::setPosition(pos.line, pos.col, pos.filePath);
    DEBUG_LOG("Creating an invalid");
    return std::make_shared<Omniscript::InvalidExpression>();
}

std::shared_ptr<Omniscript::Expression> BoolLiteral::express(SymbolTableType scope) {
    Omniscript::setPosition(pos.line, pos.col, pos.filePath);
    // DEBUG_LOG("Bool value " + value);
    if (!type) {
        DEBUG_LOG("Creating a bool false");
        type = Omniscript::Type::createPrimitiveType(Omniscript::Kind::Bool);
        return std::make_shared<Omniscript::Primitive<bool>>(value); // Default to double (64-bit)
    }

    auto typeToCastFrom = std::make_shared<Omniscript::Type>(Omniscript::Kind::Bool);

    if (!Omniscript::Type::isSameOrCastableTo(typeToCastFrom, type)) {
        console.error("The specified type is " + type->toString() + " but '" + std::to_string(value) + "' is a bool.");
    } else {
        if (!type->isBool()) {
            DEBUG_LOG("Casting bool to '" + type->toString() + "'.");
            if (type->isNullable()) {
                auto nullable = std::dynamic_pointer_cast<Omniscript::NullableType>(type);
                auto clone = this->clone();
                auto typed = std::dynamic_pointer_cast<TypedStatement>(clone);
                typed->setType(nullable->innerType);
                auto cast = std::make_shared<Cast>(clone, type);
                auto castResult = cast->express(scope);
                return castResult;
            }
            return castTo(type)->express(scope);
        }
        DEBUG_LOG("Creating an '" + type->toString() + "'.");
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
        console.error("Cannot cast a bool to a '" + targetType->toString() + "'.");
        return nullptr;
    }
}

std::shared_ptr<Omniscript::Expression> CharacterLiteral::express(SymbolTableType scope) {
    Omniscript::setPosition(pos.line, pos.col, pos.filePath);
    if (!type) {
        DEBUG_LOG("Creating a char literal");
        type = Omniscript::Type::createPrimitiveType(Omniscript::Kind::Char);
        auto utf8 = utf32_to_utf8(std::u32string(1, value));
        return std::make_shared<Omniscript::Primitive<char>>(utf8[0]);
    }

    if (!type->isChar()) {
        if (type->isNullable()) {
            auto nullable = std::dynamic_pointer_cast<Omniscript::NullableType>(type);
            auto clone = this->clone();
            auto typed = std::dynamic_pointer_cast<TypedStatement>(clone);
            typed->setType(nullable->innerType);
            auto cast = std::make_shared<Cast>(clone, type);
            auto castResult = cast->express(scope);
            return castResult;
        }
        console.error("The specified type is " + type->toString() + " but '" + std::to_string(static_cast<uint32_t>(value)) + "' is a char.");
    } else {
        DEBUG_LOG("Creating a '" + type->toString() + "' value.");
    }

    if (type->isChar(8)) {
        DEBUG_LOG("Creating UTF-8 char");
        std::string utf8_value = utf32_to_utf8(std::u32string(1, value));
        return std::make_shared<Omniscript::Primitive<char>>(utf8_value[0]); // assumes single-char utf8
    }
    else if (type->isChar(16)) {
        DEBUG_LOG("Creating UTF-16 char");
        std::u16string utf16_value = utf32_to_utf16(std::u32string(1, value));
        return std::make_shared<Omniscript::Primitive<char16_t>>(utf16_value[0]);
    }
    else if (type->isChar(32)) {
        DEBUG_LOG("Creating UTF-32 char");
        return std::make_shared<Omniscript::Primitive<char32_t>>(value);
    }

    return nullptr;
}

std::shared_ptr<Literal> CharacterLiteral::castTo(std::shared_ptr<Omniscript::Type> targetType) const {
    using Kind = Omniscript::Kind;
    switch (targetType->getKind()) {
    case Kind::Char:
    case Kind::Char16:
    case Kind::Char32:
        return std::make_shared<CharacterLiteral>(value);
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

    case Kind::String:
    case Kind::Utf8:
    case Kind::Utf16:
    case Kind::Utf32: {
        auto utf32 = std::u32string(1, value);
        auto val = std::make_shared<StringLiteral>(utf32);
        val->setType(targetType);
        return val;
    }
    default:
        console.error("Cannot cast a char to a '" + targetType->toString() + "'.");
        return nullptr;
    }
}

std::shared_ptr<Omniscript::Expression> StringLiteral::express(SymbolTableType scope) {
    Omniscript::setPosition(pos.line, pos.col, pos.filePath);
    if (!type) {
        DEBUG_LOG("Creating UTF-8 string");
        type = rootType;
        std::string utf8_value = utf32_to_utf8(value);
        return std::make_shared<Omniscript::StringExpression<std::string>>(utf8_value);
    }
    else if (!type->isPointer()) {
        DEBUG_LOG("Casting char* to '" + type->toString() + "'.");
        if (type->isNullable()) {
            auto nullable = std::dynamic_pointer_cast<Omniscript::NullableType>(type);
            auto clone = this->clone();
            auto typed = std::dynamic_pointer_cast<TypedStatement>(clone);
            typed->setType(nullable->innerType);
            auto cast = std::make_shared<Cast>(clone, type);
            auto castResult = cast->express(scope);
            return castResult;
        }
        console.error("Cannot cast a char* to a " + type->toString());
    }

    std::shared_ptr<Omniscript::Type> pointeeType = type->getPointeeType();

    // auto typeToCastFrom = std::make_shared<Omniscript::Type>(Omniscript::Kind::Utf32);

    if (!Omniscript::Type::isSameOrCastableTo(rootType, type)) {
        console.error("The specified type is " + type->toString() + " but a UTF-8 string was given.");
    } else {
        DEBUG_LOG("Creating a '" + type->toString() + "' string literal.");
    }

    if (pointeeType->isString(8) || pointeeType->isChar(8)) {
        DEBUG_LOG("Creating UTF-8 string");
        type = rootType;
        std::string utf8_value = utf32_to_utf8(value);
        return std::make_shared<Omniscript::StringExpression<std::string>>(utf8_value);
    }
    else if (pointeeType->isString(16) || pointeeType->isChar(16)) {
        DEBUG_LOG("Creating UTF-16 string");
        auto char16Type = Omniscript::Type::createPrimitiveType(Omniscript::Kind::Char16);
        auto string16Type = Omniscript::Type::createPointerType(char16Type);
        type = string16Type;
        std::u16string utf16_value = utf32_to_utf16(value);
        return std::make_shared<Omniscript::StringExpression<std::u16string>>(utf16_value);
    }
    else if (pointeeType->isString(32) || pointeeType->isChar(32)) {
        DEBUG_LOG("Creating UTF-32 string");
        auto char32Type = Omniscript::Type::createPrimitiveType(Omniscript::Kind::Char32);
        auto string32Type = Omniscript::Type::createPointerType(char32Type);
        type = string32Type;
        return std::make_shared<Omniscript::StringExpression<std::u32string>>(value);
    }

    return nullptr;
}

std::shared_ptr<Literal> StringLiteral::castTo(std::shared_ptr<Omniscript::Type> targetType) const {
    using Kind = Omniscript::Kind;
    switch (targetType->getKind()) {
    case Kind::String:
    case Kind::Utf8:
    case Kind::Utf16:
    case Kind::Utf32:
        return std::make_shared<StringLiteral>(value); // Already a UTF-32 string
    case Kind::Char:
    case Kind::Char16:
    case Kind::Char32: {
        if (!value.empty()) {
            auto val = std::make_shared<CharacterLiteral>(value[0]); // char32_t
            val->setType(targetType);
            return val;
        }
        return nullptr;
    }
    case Kind::Bool:
        return std::make_shared<BoolLiteral>(!value.empty());
    default:
        console.error("Cannot cast a char* to a '" + targetType->toString() + "'.");
        return nullptr;
    }
}

std::shared_ptr<Omniscript::Expression> Array::express(SymbolTableType scope) {
    Omniscript::setPosition(pos.line, pos.col, pos.filePath);
    DEBUG_LOG("[Array] Creating an array");

    if (!type) {
        DEBUG_LOG("[Array] The array has no explicit type. Inferring...");

        std::vector<std::shared_ptr<Omniscript::Expression>> values;
        std::vector<std::shared_ptr<Omniscript::Type>> seenTypes;

        for (const auto& expr : initialValues) {
            auto val = expr->express(scope);
            if (!val) continue;
            values.push_back(val);
            seenTypes.push_back(val->getType());
        }

        if (values.empty()) {
            console.error("Cannot infer array type from empty initializer.");
            return nullptr;
        }

        // Find best common type
        std::shared_ptr<Omniscript::Type> bestType = seenTypes[0];
        for (size_t i = 1; i < seenTypes.size(); ++i) {
            auto& currentType = seenTypes[i];
            if (Omniscript::Type::isSame(bestType, currentType)) {
                continue;
            }
            if (Omniscript::Type::isSameOrCastableTo(currentType, bestType)) {
                // current can be casted to best -> OK
                continue;
            } else if (Omniscript::Type::isSameOrCastableTo(bestType, currentType)) {
                // new type is better
                bestType = currentType;
            } else {
                console.error("Cannot infer a common array type between '" +
                              bestType->toString() + "' and '" + currentType->toString() + "'");
                return nullptr;
            }
        }

        // Cast all mismatches
        std::vector<std::shared_ptr<Omniscript::Expression>> castedValues;
        for (const auto& val : values) {
            auto valType = val->getType();
            if (Omniscript::Type::isSame(valType, bestType)) {
                castedValues.push_back(val);
            } else if (Omniscript::Type::isSameOrCastableTo(valType, bestType)) {
                castedValues.push_back(std::make_shared<Omniscript::CastExpression>(val, bestType));
            } else {
                console.error("Cannot cast array element of type '" + valType->toString() +
                              "' to inferred type '" + bestType->toString() + "'");
                return nullptr;
            }
        }

        auto arrayType = Omniscript::Type::createFixedArrayType(bestType, castedValues.size());
        setType(arrayType);
        setRootType(arrayType);

        return std::make_shared<Omniscript::FixedArrayExpression>(castedValues, bestType);
    }

    // === If type is specified (not inferred) ===
    DEBUG_LOG("[Array] The array has a declared type: '" + type->toString() + "'");

    if (type->isArray()) {
        DEBUG_LOG("[Array] Creating a fixed array with declared element type");

        std::vector<std::shared_ptr<Omniscript::Expression>> values;
        auto expectedElementType = type->elementType;
        size_t n = 0;

        for (const auto& expr : initialValues) {
            std::shared_ptr<Omniscript::Expression> val;

            if (auto typed = std::dynamic_pointer_cast<TypedStatement>(expr)) {
                if (!typed->getType()) typed->setType(expectedElementType);
            }

            val = expr->express(scope);
            if (!val) continue;

            auto actualType = val->getType();
            if (Omniscript::Type::isSame(actualType, expectedElementType)) {
                values.push_back(val);
            } else if (Omniscript::Type::isSameOrCastableTo(actualType, expectedElementType)) {
                values.push_back(std::make_shared<Omniscript::CastExpression>(val, expectedElementType));
            } else {
                console.error("Element " + std::to_string(n) +
                              " has type " + actualType->toString() +
                              " but expected " + expectedElementType->toString());
                return nullptr;
            }

            n++;
        }

        return std::make_shared<Omniscript::FixedArrayExpression>(values, expectedElementType);
    }

    if (type->isDynamicArray()) {
        DEBUG_LOG("[Array] Creating a dynamic Array");
        // TODO: Implement
        return nullptr;
    }

    if (type->isHeterogeneousArray()) {
        DEBUG_LOG("[Array] Creating a heterogeneous Array");
        // TODO: Implement
        return nullptr;
    }

    return nullptr;
}

std::shared_ptr<Literal> Array::castTo(std::shared_ptr<Omniscript::Type> targetType) const {
    // using Kind = Omniscript::Kind;

    // if (!targetType) {
    //     console.error("Target type for array cast is null.");
    //     return nullptr;
    // }

    // std::vector<std::shared_ptr<Statement>> values;

    // // --- Handle string conversions ---
    // if (targetType->isString()) {
    //     std::ostringstream stream;

    //     for (size_t i = 0; i < intialValues.size(); ++i) {

    //     }

    //     std::string result = stream.str();
    //     std::u32string resultUtf32(result.begin(), result.end());

    //     auto strLiteral = std::make_shared<StringLiteral>(resultUtf32);
    //     strLiteral->setType(targetType);
    //     return strLiteral;
    // }

    // // --- Handle casting to another fixed array ---
    // if (targetType->isFixedArray()) {
    //     auto targetElemType = targetType->elementType;

    //     if (targetType->getFixedSize() != values.size()) {
    //         console.error("Cannot cast fixed array: size mismatch.");
    //         return nullptr;
    //     }

    //     std::vector<std::shared_ptr<Omniscript::Expression>> castedValues;
    //     for (const auto& val : values) {
    //         if (!val) continue;

    //         auto casted = val->castTo(targetElemType);
    //         if (!casted) {
    //             console.error("Failed to cast array element to target type " + targetElemType->toString());
    //             return nullptr;
    //         }

    //         castedValues.push_back(casted);
    //     }

    //     auto newArray = std::make_shared<FixedArrayExpression>(castedValues, targetElemType);
    //     newArray->setType(targetType);
    // return newArray;
    //     return nullptr;
    // }

    console.error("Cannot cast array to type '" + targetType->toString() + "'.");
    return nullptr;
}
