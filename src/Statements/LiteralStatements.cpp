#include <omniscript/Statement.h>
#include <omniscript/Statements/LiteralStatements.h>

#include <omniscript/Core.h>
#include <omniscript/utils.h>
#include <omniscript/omniscript_pch.h>
#include <omniscript/Statement.h>
#include <omniscript/Symboltable.h>

// ============================== Literals and casting  ============================== //
std::shared_ptr<Omniscript::Expression> Cast::express(SymbolTableType scope) {
    Omniscript::setPosition(pos.line, pos.col, pos.filePath);
    DEBUG_LOG("");

    if (type && type->isUnresolved()) {
        if (auto unresolved = std::dynamic_pointer_cast<Omniscript::UnresolvedType>(type)) {
            type = scope->getType(unresolved->joinedTypeString);
            rootType = type;
            targetType = type;
            if (!type) {
                console.error("Type '" + unresolved->joinedTypeString + "' does not exist in scope '" + scope->getName() + "'.");
            }
        }
    }

    if (type->isFunction()) {
        type = std::make_shared<Omniscript::PointerType>(type);
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
            console.error("Cannot cast " + value->toString() + " it has no type.");
        } else {
            console.error("There is no value to cast.");
        }
    }

    extendContextOf(value);

    std::shared_ptr<Omniscript::Expression> result;

    // Literal cast (handles constant folding, primitive -> primitive, etc.)
    if (auto literal = std::dynamic_pointer_cast<Literal>(value)) {
        if (!targetType->isNullable()) {
            auto castedStmt = literal->castTo(targetType);
            result = castedStmt->express(scope);
            result->setPosition(getPosition());
            return result;
        }
    }

    auto valueResult = value->express(scope);

    // If casting to a nullable type
    if (targetType->isNullable()) {
        // If the result is already null or nullable, just return as NullableExpression
        if (std::dynamic_pointer_cast<Omniscript::NullableExpression>(valueResult)) {
            result = valueResult;
        }
        // If we're casting a null literal (e.g., NullExpression or NullPointerExpression)
        else if (std::dynamic_pointer_cast<Omniscript::NullExpression>(valueResult) ||
            std::dynamic_pointer_cast<Omniscript::NullPointerExpression>(valueResult)) {
            result = std::make_shared<Omniscript::NullableExpression>();
        }
        // Wrap any expression in a NullableExpression
        else {
            result = std::make_shared<Omniscript::NullableExpression>(valueResult);
        }
    } else {
        // Normal cast expression fallback
        result = std::make_shared<Omniscript::CastExpression>(valueResult, targetType);
    }

    result->setPosition(getPosition());
    return result;
}

std::shared_ptr<Omniscript::Expression> Nullptr::express(SymbolTableType scope) {
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
    auto nullpointerType = type ? type : Omniscript::Type::createPrimitiveType(Omniscript::Kind::Void);

    auto result = Omniscript::make_expression<Omniscript::NullPointerExpression>(type);

    result->setPosition(getPosition());
    return result;
}

std::shared_ptr<Omniscript::Expression> Null::express(SymbolTableType scope) {
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
    std::shared_ptr<Omniscript::Expression> result;
    
    if (type->isNullable()) {
        auto nullable = std::dynamic_pointer_cast<Omniscript::NullableType>(type);
        auto nullableExpr = std::make_shared<Omniscript::NullableExpression>();
        nullableExpr->type = nullable->innerType ? nullable->innerType : Omniscript::Type::createPrimitiveType(Omniscript::Kind::Void);
        nullableExpr->rootType = nullable->innerType ? nullable->innerType : Omniscript::Type::createPrimitiveType(Omniscript::Kind::Void);
        result = nullableExpr;
    } else {
        result = Omniscript::make_expression<Omniscript::NullExpression>(
            type ? type : Omniscript::Type::createPrimitiveType(Omniscript::Kind::Void));
    }

    result->setPosition(getPosition());
    return result;
}

std::shared_ptr<Omniscript::Expression> PointerLiteral::express(SymbolTableType scope) {
    Omniscript::setPosition(pos.line, pos.col, pos.filePath);
    
    std::shared_ptr<Omniscript::Expression> result;
    
    // Handle null pointer case
    if (address == 0) {
        result = Omniscript::make_expression<Omniscript::NullPointerExpression>(type->getPointeeType());
    } else {
        // Create raw pointer expression
        auto pointeeType = type ? type->getPointeeType() : Omniscript::Type::createPrimitiveType(Omniscript::Kind::Void);
        auto ptrType = Omniscript::Type::createPointerType(pointeeType, isConst, isVolatile);
        result = Omniscript::make_expression<Omniscript::RawPointerExpression>(address, ptrType);
    }

    result->setPosition(getPosition());
    return result;
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
    
    std::shared_ptr<Omniscript::Expression> result;
    
    if (!type) {
        DEBUG_LOG("Creating a 32-bit integer");
        type = Omniscript::Type::createPrimitiveType(Omniscript::Kind::Int32);
        result = std::make_shared<Omniscript::Integer<int32_t>>(static_cast<int32_t>(value));
        result->setPosition(getPosition());
        return result;
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
                result = castResult;
                result->setPosition(getPosition());
                return result;
            }
            result = castTo(type)->express(scope);
            result->setPosition(getPosition());
            return result;
        }
        DEBUG_LOG("Creating an '" + type->toString() + "' integer");
    }

    // Check for specific bit-widths using the isInteger function with optional bitwidth argument
    if (type->isInteger(8)) {
        DEBUG_LOG("Creating an 8-bit integer");
        result = std::make_shared<Omniscript::Integer<int8_t>>(static_cast<int8_t>(value));
    }
    else if (type->isInteger(16)) {
        DEBUG_LOG("Creating a 16-bit integer");
        result = std::make_shared<Omniscript::Integer<int16_t>>(static_cast<int16_t>(value));
    }
    else if (type->isInteger(32)) {
        DEBUG_LOG("Creating a 32-bit integer");
        result = std::make_shared<Omniscript::Integer<int32_t>>(static_cast<int32_t>(value));
    }
    else if (type->isInteger(64)) {
        DEBUG_LOG("Creating a 64-bit integer");
        result = std::make_shared<Omniscript::Integer<int64_t>>(static_cast<int64_t>(value));
    }
    else if (type->isInteger(128)) {
        DEBUG_LOG("Creating a 128-bit integer");
        result = std::make_shared<Omniscript::BigInt>(std::to_string(value), 128);
    }
    else if (type->isInteger(256)) {
        DEBUG_LOG("Creating a 256-bit integer");
        result = std::make_shared<Omniscript::BigInt>(std::to_string(value), 256);
    }
    else if (type->isInteger(512)) {
        DEBUG_LOG("Creating a 512-bit integer");
        result = std::make_shared<Omniscript::BigInt>(std::to_string(value), 512);
    }
    else if (type->isInteger(1024)) {
        DEBUG_LOG("Creating a 1024-bit integer");
        result = std::make_shared<Omniscript::BigInt>(std::to_string(value), 1024);
    }

    if (result) {
        result->setPosition(getPosition());
        return result;
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
    
    std::shared_ptr<Omniscript::Expression> result;
    
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
                result = castResult;
                result->setPosition(getPosition());
                return result;
            }
            result = castTo(type)->express(scope);
            result->setPosition(getPosition());
            return result;
        }
        DEBUG_LOG("Creating an '" + type->toString() + "' float.");
    }

    // Target-specific 16-bit float handling
    #ifdef __ARM_ARCH
        if (type->isFloat(16)) {
            DEBUG_LOG("Creating a 16-bit float (__fp16 for ARM)");
            result = std::make_shared<Omniscript::Float<__fp16>>(static_cast<__fp16>(value));
        }
    #elif defined(__x86_64__) || defined(__i386__) 
        if (type->isFloat(16)) {
            DEBUG_LOG("Creating a 16-bit float (_Float16 for x86)");
            result = std::make_shared<Omniscript::Float<_Float16>>(static_cast<_Float16>(value));
        }
    #endif

    if (!result && type->isFloat(32)) {
        DEBUG_LOG("Creating a 32-bit float");
        result = std::make_shared<Omniscript::Float<float>>(static_cast<float>(value));
    }

    if (!result && type->isFloat(64)) {
        DEBUG_LOG("Creating a 64-bit float");
        result = std::make_shared<Omniscript::Float<double>>(static_cast<double>(value));
    }

    if (!result && type->isFloat(80)) {
        DEBUG_LOG("Creating an 80-bit float (X86_FP80)");
        result = std::make_shared<Omniscript::Float<long double>>(static_cast<long double>(value));
    }

    if (!result && type->isFloat(128)) {
        DEBUG_LOG("Creating a 128-bit float (FP128 or PPC_FP128)");
        result = std::make_shared<Omniscript::Float<__float128>>(value);
    }

    if (result) {
        result->setPosition(getPosition());
        return result;
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
    auto result = std::make_shared<Omniscript::BigInt>(value, bitWidth);
    
    result->setPosition(getPosition());
    return result;
}

std::shared_ptr<Omniscript::Expression> Invalid::express(SymbolTableType scope) {
    Omniscript::setPosition(pos.line, pos.col, pos.filePath);
    DEBUG_LOG("Creating an invalid");
    auto result = std::make_shared<Omniscript::InvalidExpression>();
    
    result->setPosition(getPosition());
    return result;
}

std::shared_ptr<Omniscript::Expression> BoolLiteral::express(SymbolTableType scope) {
    Omniscript::setPosition(pos.line, pos.col, pos.filePath);
    
    std::shared_ptr<Omniscript::Expression> result;
    
    // DEBUG_LOG("Bool value " + value);
    if (!type) {
        DEBUG_LOG("Creating a bool false");
        type = Omniscript::Type::createPrimitiveType(Omniscript::Kind::Bool);
        result = std::make_shared<Omniscript::Primitive<bool>>(value); // Default to double (64-bit)
        result->setPosition(getPosition());
        return result;
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
                result = castResult;
                result->setPosition(getPosition());
                return result;
            }
            result = castTo(type)->express(scope);
            result->setPosition(getPosition());
            return result;
        }
        DEBUG_LOG("Creating an '" + type->toString() + "'.");
    }

    result = std::make_shared<Omniscript::Primitive<bool>>(value);
    
    result->setPosition(getPosition());
    return result;
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
    
    std::shared_ptr<Omniscript::Expression> result;
    
    if (!type) {
        DEBUG_LOG("Creating a char literal");
        type = Omniscript::Type::createPrimitiveType(Omniscript::Kind::Char);
        auto utf8 = utf32_to_utf8(std::u32string(1, value));
        result = std::make_shared<Omniscript::Primitive<char>>(utf8[0]);
        result->setPosition(getPosition());
        return result;
    }

    if (!type->isChar()) {
        if (type->isNullable()) {
            auto nullable = std::dynamic_pointer_cast<Omniscript::NullableType>(type);
            auto clone = this->clone();
            auto typed = std::dynamic_pointer_cast<TypedStatement>(clone);
            typed->setType(nullable->innerType);
            auto cast = std::make_shared<Cast>(clone, type);
            auto castResult = cast->express(scope);
            result = castResult;
            result->setPosition(getPosition());
            return result;
        }
        console.error("The specified type is " + type->toString() + " but '" + std::to_string(static_cast<uint32_t>(value)) + "' is a char.");
    } else {
        DEBUG_LOG("Creating a '" + type->toString() + "' value.");
    }

    if (type->isChar(8)) {
        DEBUG_LOG("Creating UTF-8 char");
        std::string utf8_value = utf32_to_utf8(std::u32string(1, value));
        result = std::make_shared<Omniscript::Primitive<char>>(utf8_value[0]); // assumes single-char utf8
    }
    else if (type->isChar(16)) {
        DEBUG_LOG("Creating UTF-16 char");
        std::u16string utf16_value = utf32_to_utf16(std::u32string(1, value));
        result = std::make_shared<Omniscript::Primitive<char16_t>>(utf16_value[0]);
    }
    else if (type->isChar(32)) {
        DEBUG_LOG("Creating UTF-32 char");
        result = std::make_shared<Omniscript::Primitive<char32_t>>(value);
    }

    if (result) {
        result->setPosition(getPosition());
        return result;
    }

    return nullptr;
}

std::shared_ptr<Literal> CharacterLiteral::castTo(std::shared_ptr<Omniscript::Type> targetType) const {
    using Kind = Omniscript::Kind;
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
        console.error("Cannot cast a char to a '" + targetType->toString() + "'.");
        return nullptr;
    }
    
    if (result) {
        result->setPosition(getPosition());
    }
    return result;
}

std::shared_ptr<Omniscript::Expression> StringLiteral::express(SymbolTableType scope) {
    Omniscript::setPosition(pos.line, pos.col, pos.filePath);
    std::shared_ptr<Omniscript::Expression> result = nullptr;
    
    if (!type) {
        DEBUG_LOG("Creating UTF-8 string");
        type = rootType;
        std::string utf8_value = utf32_to_utf8(value);
        result = std::make_shared<Omniscript::StringExpression<std::string>>(utf8_value);
    }
    else if (!type->isPointer()) {
        DEBUG_LOG("Casting char* to '" + type->toString() + "'.");
        if (type->isNullable()) {
            auto nullable = std::dynamic_pointer_cast<Omniscript::NullableType>(type);
            auto clone = this->clone();
            auto typed = std::dynamic_pointer_cast<TypedStatement>(clone);
            typed->setType(nullable->innerType);
            auto cast = std::make_shared<Cast>(clone, type);
            result = cast->express(scope);
            // Note: cast->express() will handle its own position setting
            return result;
        }
        console.error("Cannot cast a char* to a " + type->toString());
        return nullptr;
    }
    else {
        std::shared_ptr<Omniscript::Type> pointeeType = type->getPointeeType();

        if (!Omniscript::Type::isSameOrCastableTo(rootType, type)) {
            console.error("The specified type is " + type->toString() + " but a UTF-8 string was given.");
            return nullptr;
        } else {
            DEBUG_LOG("Creating a '" + type->toString() + "' string literal.");
        }

        if (pointeeType->isString(8) || pointeeType->isChar(8)) {
            DEBUG_LOG("Creating UTF-8 string");
            type = rootType;
            std::string utf8_value = utf32_to_utf8(value);
            result = std::make_shared<Omniscript::StringExpression<std::string>>(utf8_value);
        }
        else if (pointeeType->isString(16) || pointeeType->isChar(16)) {
            DEBUG_LOG("Creating UTF-16 string");
            auto char16Type = Omniscript::Type::createPrimitiveType(Omniscript::Kind::Char16);
            auto string16Type = Omniscript::Type::createPointerType(char16Type);
            type = string16Type;
            std::u16string utf16_value = utf32_to_utf16(value);
            result = std::make_shared<Omniscript::StringExpression<std::u16string>>(utf16_value);
        }
        else if (pointeeType->isString(32) || pointeeType->isChar(32)) {
            DEBUG_LOG("Creating UTF-32 string");
            auto char32Type = Omniscript::Type::createPrimitiveType(Omniscript::Kind::Char32);
            auto string32Type = Omniscript::Type::createPointerType(char32Type);
            type = string32Type;
            result = std::make_shared<Omniscript::StringExpression<std::u32string>>(value);
        }
    }
    
    if (result) {
        result->setPosition(getPosition());
    }
    return result;
}

std::shared_ptr<Literal> StringLiteral::castTo(std::shared_ptr<Omniscript::Type> targetType) const {
    using Kind = Omniscript::Kind;
    std::shared_ptr<Literal> result = nullptr;
    
    switch (targetType->getKind()) {
    case Kind::String:
    case Kind::Utf8:
    case Kind::Utf16:
    case Kind::Utf32:
        result = std::make_shared<StringLiteral>(value); // Already a UTF-32 string
        break;
    case Kind::Char:
    case Kind::Char16:
    case Kind::Char32: {
        if (!value.empty()) {
            auto val = std::make_shared<CharacterLiteral>(value[0]); // char32_t
            val->setType(targetType);
            result = val;
        }
        break;
    }
    case Kind::Bool:
        result = std::make_shared<BoolLiteral>(!value.empty());
        break;
    default:
        console.error("Cannot cast a char* to a '" + targetType->toString() + "'.");
        return nullptr;
    }
    
    if (result) {
        result->setPosition(getPosition());
    }
    return result;
}

std::shared_ptr<Omniscript::Expression> Array::express(SymbolTableType scope) {
    Omniscript::setPosition(pos.line, pos.col, pos.filePath);
    DEBUG_LOG("[Array] Creating an array");
    std::shared_ptr<Omniscript::Expression> result = nullptr;

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

        result = std::make_shared<Omniscript::FixedArrayExpression>(castedValues, bestType);
    }
    else if (type->isArray()) {
        DEBUG_LOG("[Array] The array has a declared type: '" + type->toString() + "'");
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

        if (n > type->fixedSize) {
            console.error("Array expects '" + std::to_string(type->fixedSize) + "' elements or less but got '" + std::to_string(n) + "' instead.");
        }

        result = std::make_shared<Omniscript::FixedArrayExpression>(values, expectedElementType);
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
        DEBUG_LOG("[Array] The array has a declared type: '" + type->toString() + "'");
        return nullptr;
    }
    
    if (result) {
        result->setPosition(getPosition());
    }
    return result;
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