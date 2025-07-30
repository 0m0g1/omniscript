#include <omniscript/Core.h>
#include <omniscript/Types/BaseType.h>
#include <omniscript/Expressions/Expression.h>
#include <omniscript/omniscript_pch.h>
#include <omniscript/Types/BaseType.h>
#include <omniscript/Types/DerivedTypes.h>
#include <omniscript/Types/TraitCapability.h>
#include <omniscript/Console.h>

namespace Omniscript {

std::shared_ptr<Type> resolveType(const std::vector<std::string>& dataTypes) {
    if (dataTypes.empty()) {
        TYPE_ERROR("Empty type specification");
        return Type::createInvalid();
    }

    size_t index = 0;
    int totalPointerDepth = 0;
    int totalReferenceDepth = 0;
    int nullableDepth = 0;

    if (index < dataTypes.size() && dataTypes[index] == "fn") {
        return resolveFunctionType(dataTypes, index);
    }

    while (index < dataTypes.size() && dataTypes[index] == "&") {
        totalReferenceDepth++;
        index++;
    }

    while (index < dataTypes.size() && dataTypes[index] == "*") {
        totalPointerDepth++;
        index++;
    }

    while (index < dataTypes.size() && dataTypes[index] == "?") {
        nullableDepth++;
        index++;
    }

    if (index >= dataTypes.size()) {
        TYPE_ERROR("No base type found after modifiers");
        return Type::createInvalid();
    }

    std::string baseType = dataTypes[index++];

    std::vector<uint64_t> arrayStack;
    while (index + 2 <= dataTypes.size() && dataTypes[index] == "[") {
        index += 1;
        if (index >= dataTypes.size()) {
            TYPE_ERROR("Expected array size after '['");
            return Type::createInvalid();
        }

        uint64_t size;
        if (dataTypes[index] == "]") {
            TYPE_ERROR("An array should have a size between '[' and ']'");
            return Type::createInvalid();
        } else {
            try {
                size = std::stoull(dataTypes[index]);
            } catch (...) {
                TYPE_ERROR("Invalid array size: " + dataTypes[index]);
                return Type::createInvalid();
            }
            index += 1;
        }

        if (index >= dataTypes.size() || dataTypes[index] != "]") {
            TYPE_ERROR("Expected ']' after array size");
            return Type::createInvalid();
        }
        index += 1;

        arrayStack.push_back(size);
    }

    while (index < dataTypes.size() && dataTypes[index] == "*") {
        totalPointerDepth++;
        index++;
    }

    while (index < dataTypes.size() && dataTypes[index] == "&") {
        totalReferenceDepth++;
        index++;
    }

    while (index < dataTypes.size() && dataTypes[index] == "?") {
        nullableDepth++;
        index++;
    }

    std::shared_ptr<Type> type;

    if (!type) {
        DEBUG_LOG("Resolving base type: " + baseType);

        if (baseType == "int" || baseType == "i32" || baseType == "int32") type = Type::createPrimitiveType(Kind::Int32);
        else if (baseType == "int8" || baseType == "i8") type = Type::createPrimitiveType(Kind::Int8);
        else if (baseType == "int16" || baseType == "i16") type = Type::createPrimitiveType(Kind::Int16);
        else if (baseType == "int64" || baseType == "i64") type = Type::createPrimitiveType(Kind::Int64);
        else if (baseType == "int128" || baseType == "i128") type = Type::createPrimitiveType(Kind::Int128);
        else if (baseType == "int256" || baseType == "i256") type = Type::createPrimitiveType(Kind::Int256);
        else if (baseType == "int512" || baseType == "i512") type = Type::createPrimitiveType(Kind::Int512);
        else if (baseType == "int1024" || baseType == "i1024") type = Type::createPrimitiveType(Kind::Int1024);
        else if (baseType == "BigInt") type = Type::createPrimitiveType(Kind::BigInt);
        else if (baseType == "usize" || baseType == "size_t") type = Type::createPrimitiveType(Kind::Size_t);
        else if (baseType == "uint" || baseType == "u32" || baseType == "uint32") type = Type::createPrimitiveType(Kind::UInt32);
        else if (baseType == "uint8" || baseType == "u8") type = Type::createPrimitiveType(Kind::UInt8);
        else if (baseType == "uint16" || baseType == "u16") type = Type::createPrimitiveType(Kind::UInt16);
        else if (baseType == "uint64" || baseType == "u64") type = Type::createPrimitiveType(Kind::UInt64);
        else if (baseType == "uint128" || baseType == "u128") type = Type::createPrimitiveType(Kind::UInt128);
        else if (baseType == "uint256" || baseType == "u256") type = Type::createPrimitiveType(Kind::UInt256);
        else if (baseType == "uint512" || baseType == "u512") type = Type::createPrimitiveType(Kind::UInt512);
        else if (baseType == "uint1024" || baseType == "u1024") type = Type::createPrimitiveType(Kind::UInt1024);
        else if (baseType == "bool") type = Type::createPrimitiveType(Kind::Bool);
        else if (baseType == "char") type = Type::createPrimitiveType(Kind::Char);
        else if (baseType == "char16") type = Type::createPrimitiveType(Kind::Char16);
        else if (baseType == "char32") type = Type::createPrimitiveType(Kind::Char32);
        else if (baseType == "void") type = Type::createPrimitiveType(Kind::Void);
        else if (baseType == "half" || baseType == "f16") type = Type::createPrimitiveType(Kind::Half);
        else if (baseType == "float" || baseType == "f32") type = Type::createPrimitiveType(Kind::Float);
        else if (baseType == "double" || baseType == "f64") type = Type::createPrimitiveType(Kind::Double);
        else if (baseType == "f80" || baseType == "x86_fp80") type = Type::createPrimitiveType(Kind::X86_FP80);
        else if (baseType == "fp128" || baseType == "f128") type = Type::createPrimitiveType(Kind::FP128);
        else if (baseType == "ppc_fp128") type = Type::createPrimitiveType(Kind::PPC_FP128);
        else if (baseType == "string") type = Type::createStringType(Kind::String);
        else if (baseType == "utf8") type = Type::createStringType(Kind::Utf8);
        else if (baseType == "utf16") type = Type::createStringType(Kind::Utf16);
        else if (baseType == "utf32") type = Type::createStringType(Kind::Utf32);
        else if (baseType == "unique_ptr") {
            if (index < dataTypes.size() && dataTypes[index] == "<") {
                std::vector<std::string> pointeeTokens;
                index++;
                while (index < dataTypes.size() && dataTypes[index] != ">") {
                    pointeeTokens.push_back(dataTypes[index]);
                    index++;
                }
                if (index < dataTypes.size()) index++; // Skip '>'
                auto pointeeType = resolveType(pointeeTokens);
                type = Type::createSmartPointerType(Kind::UniquePtr, pointeeType);
            }
        } else if (baseType == "shared_ptr") {
            if (index < dataTypes.size() && dataTypes[index] == "<") {
                std::vector<std::string> pointeeTokens;
                index++;
                while (index < dataTypes.size() && dataTypes[index] != ">") {
                    pointeeTokens.push_back(dataTypes[index]);
                    index++;
                }
                if (index < dataTypes.size()) index++; // Skip '>'
                auto pointeeType = resolveType(pointeeTokens);
                type = Type::createSmartPointerType(Kind::SharedPtr, pointeeType);
            }
        } else if (baseType == "weak_ptr") {
            if (index < dataTypes.size() && dataTypes[index] == "<") {
                std::vector<std::string> pointeeTokens;
                index++;
                while (index < dataTypes.size() && dataTypes[index] != ">") {
                    pointeeTokens.push_back(dataTypes[index]);
                    index++;
                }
                if (index < dataTypes.size()) index++; // Skip '>'
                auto pointeeType = resolveType(pointeeTokens);
                type = Type::createSmartPointerType(Kind::WeakPtr, pointeeType);
            }
        } else {
            type = Type::createUserDefinedType(baseType);
        }
    }

    if (!type) {
        type = Type::createUnresolved(dataTypes);
    }

    for (auto it = arrayStack.rbegin(); it != arrayStack.rend(); ++it) {
        type = Type::createFixedArrayType(type, *it);
    }

    for (int i = 0; i < totalReferenceDepth; ++i) {
        type = Type::createReferenceType(type);
    }

    for (int i = 0; i < totalPointerDepth; ++i) {
        type = Type::createPointerType(type);
    }

    for (int i = 0; i < nullableDepth; ++i) {
        type = Type::createNullableType(type);
    }

    return type;
}

std::shared_ptr<Type> resolveFunctionType(const std::vector<std::string>& dataTypes, size_t& index) {
    if (index >= dataTypes.size() || dataTypes[index] != "fn") {
        TYPE_ERROR("Expected 'fn' keyword for function type");
        return Type::createInvalid();
    }
    index++;

    if (index >= dataTypes.size() || dataTypes[index] != "(") {
        TYPE_ERROR("Expected '(' after 'fn'");
        return Type::createInvalid();
    }
    index++;

    std::vector<std::shared_ptr<Type>> paramTypes;
    std::vector<std::string> paramNames;

    while (index < dataTypes.size() && dataTypes[index] != ")") {
        std::vector<std::string> currentParam;
        std::string paramName;

        if (index + 1 < dataTypes.size() && dataTypes[index + 1] == ":") {
            paramName = dataTypes[index];
            index += 2;
        }

        while (index < dataTypes.size() && dataTypes[index] != "," && dataTypes[index] != ")") {
            currentParam.push_back(dataTypes[index]);
            index++;
        }

        if (!currentParam.empty()) {
            auto paramType = resolveType(currentParam);
            if (paramType) {
                paramTypes.push_back(paramType);
                paramNames.push_back(paramName);
            }
        }

        if (index < dataTypes.size() && dataTypes[index] == ",") {
            index++;
        }
    }

    if (index >= dataTypes.size() || dataTypes[index] != ")") {
        TYPE_ERROR("Expected ')' after function parameters");
        return Type::createInvalid();
    }
    index++;

    if (index >= dataTypes.size() || dataTypes[index] != "=>") {
        TYPE_ERROR("Expected '=>' after function parameters");
        return Type::createInvalid();
    }
    index++;

    std::vector<std::string> returnTypeTokens;
    while (index < dataTypes.size()) {
        returnTypeTokens.push_back(dataTypes[index]);
        index++;
    }

    std::shared_ptr<Type> returnType;
    if (returnTypeTokens.empty()) {
        returnType = Type::createPrimitiveType(Kind::Void);
    } else {
        returnType = resolveType(returnTypeTokens);
    }

    auto fnType = Type::createFunctionType("", paramTypes, returnType, false);
    auto fn = std::dynamic_pointer_cast<FunctionType>(fnType);
    if (fn) {
        for (size_t i = 0; i < paramNames.size(); ++i) {
            fn->setParamName(i, paramNames[i]);
        }
    }
    return fnType;
}

} // namespace Omniscript