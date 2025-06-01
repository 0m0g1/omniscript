#include <omniscript/Core.h>
#include <omniscript/utils.h>
#include <omniscript/engine/Statement.h>
#include <omniscript/engine/Symboltable.h>
#include <omniscript/omniscript_pch.h>
#include <omniscript/utils.h>


// ============================== Literals and casting  ============================== //

std::shared_ptr<Omniscript::Expression> Cast::express(SymbolTableType scope) {
    DEBUG_LOG("");
    if (auto typed = std::dynamic_pointer_cast<TypedStatement>(value)) {
        DEBUG_LOG("[Cast] Casting a '" + typed->getRootType()->description() + "' to a '" + targetType->description() + "'.");
    }
    if (auto literal = std::dynamic_pointer_cast<Literal>(value)) {
        auto castedStmt = literal->castTo(targetType);
        return castedStmt->express(scope);
    }

    return nullptr;// Or handle casting explicitly at runtime/codegen
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
        console.error("The specified type is '" + type->description() + "' but '" + std::to_string(value) + "' is an integer.");
    } else {
        if (!type->isInteger()) {
            DEBUG_LOG("Casting integer to '" + type->description() + "'.");
            return castTo(type)->express(scope);
        }
        DEBUG_LOG("Creating an '" + type->description() + "' integer");
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
            return std::make_shared<IntegerLiteral>(value);  // Truncation assumed safe

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
            return nullptr;
    }
}

std::shared_ptr<Omniscript::Expression> FloatLiteral::express(SymbolTableType scope) {
    // Default to 128-bit float if type is not specified
    if (!type) {
        if (isFloat16) {
            type = Omniscript::Type::createPrimitiveType(Omniscript::Kind::Half);
        } else if (isFloat32) {
            type = Omniscript::Type::createPrimitiveType(Omniscript::Kind::Float);
        } else if (isFloat64) {
            type = Omniscript::Type::createPrimitiveType(Omniscript::Kind::Double);
        } else if (isFloat80) {
            type = Omniscript::Type::createPrimitiveType(Omniscript::Kind::X86_FP80);
        } else if (isFloat128) {
            type = Omniscript::Type::createPrimitiveType(Omniscript::Kind::FP128);
        } else {
            // Default to 128-bit float if no suffix specified
            DEBUG_LOG("No suffix: defaulting to 128-bit float");
            type = Omniscript::Type::createPrimitiveType(Omniscript::Kind::Double);
        }
    }

    if (!Omniscript::isSameOrCastableTo(rootType, type))  {
        console.error("The specified type is " + type->description() +
                      " but '" + /* custom __float128 to string needed here */ "' is a float.");
    } else {
        if (!type->isFloat()) {
            DEBUG_LOG("Casting float to '" + type->description() + "'.");
            return castTo(type)->express(scope);
        }
        DEBUG_LOG("Creating an '" + type->description() + "' float.");
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
        console.error("The specified type is " + type->description() + " but '" + std::to_string(value) + "' is a bool.");
    } else {
        if (!type->isBool()) {
            DEBUG_LOG("Casting bool to '" + type->description() + "'.");
            return castTo(type)->express(scope);
        }
        DEBUG_LOG("Creating an '" + type->description() + "'.");
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
            return nullptr;
    }
}

std::shared_ptr<Omniscript::Expression> CharacterLiteral::express(SymbolTableType scope) {
    if (!type) {
        DEBUG_LOG("Creating a char literal");
        type = Omniscript::Type::createPrimitiveType(Omniscript::Kind::Char);
        auto utf8 = utf32_to_utf8(std::u32string(1, value));
        return std::make_shared<Omniscript::Primitive<char>>(utf8[0]);
    }

    if (!type->isChar()) {
        console.error("The specified type is " + type->description() + " but '" + std::to_string(static_cast<uint32_t>(value)) + "' is a char.");
    } else {
        DEBUG_LOG("Creating a '" + type->description() + "' value.");
    }

    if (type->isChar(8)) {
        DEBUG_LOG("Creating UTF-8 char");
        std::string utf8_value = utf32_to_utf8(std::u32string(1, value));
        return std::make_shared<Omniscript::Primitive<char>>(utf8_value[0]);  // assumes single-char utf8
    } else if (type->isChar(16)) {
        DEBUG_LOG("Creating UTF-16 char");
        std::u16string utf16_value = utf32_to_utf16(std::u32string(1, value));
        return std::make_shared<Omniscript::Primitive<char16_t>>(utf16_value[0]);
    } else if (type->isChar(32)) {
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
            return nullptr;
    }
}


std::shared_ptr<Omniscript::Expression> StringLiteral::express(SymbolTableType scope) {
    if (!type) {
        DEBUG_LOG("Creating UTF-8 string");
        type = rootType;
        std::string utf8_value = utf32_to_utf8(value);
        return std::make_shared<Omniscript::StringExpression<std::string>>(utf8_value);
    } else if (!type->isPointer()) {
        console.error("error message");   
    }

    std::shared_ptr<Omniscript::Type> pointeeType = type->getPointeeType();

    
    // auto typeToCastFrom = std::make_shared<Omniscript::Type>(Omniscript::Kind::Utf32);

    if (!Omniscript::isSameOrCastableTo(rootType, type)) {
        console.error("The specified type is " + type->description() + " but a UTF-8 string was given.");
    } else {
        DEBUG_LOG("Creating a '" + type->description() + "' string literal.");
    }

    if (pointeeType->isString(8) || pointeeType->isChar(8)) {
        DEBUG_LOG("Creating UTF-8 string");
        type = rootType;
        std::string utf8_value = utf32_to_utf8(value);
        return std::make_shared<Omniscript::StringExpression<std::string>>(utf8_value);
    } else if (pointeeType->isString(16) || pointeeType->isChar(16)) {
        DEBUG_LOG("Creating UTF-16 string");
        auto char16Type = Omniscript::Type::createPrimitiveType(Omniscript::Kind::Char16);
        auto string16Type = Omniscript::Type::createPointerType(char16Type);
        type = string16Type;
        std::u16string utf16_value = utf32_to_utf16(value);
        return std::make_shared<Omniscript::StringExpression<std::u16string>>(utf16_value);
    } else if (pointeeType->isString(32) || pointeeType->isChar(32)) {
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
            return std::make_shared<StringLiteral>(value);  // Already a UTF-32 string
        case Kind::Char:
        case Kind::Char16:
        case Kind::Char32: {
            if (!value.empty()) {
                auto val = std::make_shared<CharacterLiteral>(value[0]);  // char32_t
                val->setType(targetType);
                return val;
            }
            return nullptr;
        }
        case Kind::Bool:
            return std::make_shared<BoolLiteral>(!value.empty());
        default:
            return nullptr;
    }
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
    
    DEBUG_LOG("[Array] The array has a type '" + type->description() + "' of element types '" + type->elementType->description() + "'.");
    size_t n = 0;
    if (type->isFixedArray()) {
        DEBUG_LOG("[Array] Creating a fixed Array");
        std::vector<std::shared_ptr<Omniscript::Expression>> values;
        std::shared_ptr<Omniscript::Type> expectedElementType = type->elementType;  // Assume you have this method
        DEBUG_LOG("[Array] The expected element kind is '" + expectedElementType->description() + "'.");
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
                    console.error("Array element is of type " + actualType->description() +
                                  " but expected type " + expectedElementType->description());
                }
            }

            DEBUG_LOG("[Array] Value '" + std::to_string(n) + "' is '" + expectedElementType->description() + " " + val->toString() + "'.");
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