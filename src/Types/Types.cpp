#include <stdexcept>
#include <string_view>
#include <unordered_map>

#include <omniscript/Console.h>
#include <omniscript/Types/Types.h>
#include <omniscript/Types/BaseType.h>
#include <omniscript/Types/DerivedTypes.h>
#include <omniscript/Types/TraitCapability.h>

namespace Omniscript {

// Map for base type resolution
static const std::unordered_map<std::string_view, Kind> baseTypeMap = {
    {"int", Kind::Int32}, {"i32", Kind::Int32}, {"int32", Kind::Int32},
    {"int8", Kind::Int8}, {"i8", Kind::Int8},
    {"int16", Kind::Int16}, {"i16", Kind::Int16},
    {"int64", Kind::Int64}, {"i64", Kind::Int64},
    {"int128", Kind::Int128}, {"i128", Kind::Int128},
    {"int256", Kind::Int256}, {"i256", Kind::Int256},
    {"int512", Kind::Int512}, {"i512", Kind::Int512},
    {"int1024", Kind::Int1024}, {"i1024", Kind::Int1024},
    {"int1024", Kind::BigInt},
    {"usize", Kind::Size_t}, {"size_t", Kind::Size_t},
    {"uint", Kind::UInt32}, {"u32", Kind::UInt32}, {"uint32", Kind::UInt32},
    {"uint8", Kind::UInt8}, {"u8", Kind::UInt8},
    {"uint16", Kind::UInt16}, {"u16", Kind::UInt16},
    {"uint64", Kind::UInt64}, {"u64", Kind::UInt64},
    {"uint128", Kind::UInt128}, {"u128", Kind::UInt128},
    {"uint256", Kind::UInt256}, {"u256", Kind::UInt256},
    {"uint512", Kind::UInt512}, {"u512", Kind::UInt512},
    {"uint1024", Kind::UInt1024}, {"u1024", Kind::UInt1024},
    {"bool", Kind::Bool},
    {"char", Kind::Char},
    {"char16", Kind::Char16},
    {"char32", Kind::Char32},
    {"void", Kind::Void},
    {"half", Kind::Half}, {"f16", Kind::Half},
    {"float", Kind::Float}, {"f32", Kind::Float},
    {"double", Kind::Double}, {"f64", Kind::Double},
    {"f80", Kind::X86_FP80}, {"x86_fp80", Kind::X86_FP80},
    {"fp128", Kind::FP128}, {"f128", Kind::FP128},
    {"ppc_fp128", Kind::PPC_FP128},
    {"string", Kind::String},
    {"utf8", Kind::Utf8},
    {"utf16", Kind::Utf16},
    {"utf32", Kind::Utf32}
};

// Helper to parse nested tokens within delimiters (e.g., <...>)
std::vector<std::string> parseNestedTokens(const std::vector<std::string>& dataTypes, size_t& index, const std::string_view& startDelim, const std::string_view& endDelim) {
    std::vector<std::string> tokens;
    if (index >= dataTypes.size() || dataTypes[index] != startDelim) {
        return tokens;
    }
    index++; // Skip start delimiter
    int depth = 1;
    while (index < dataTypes.size() && depth > 0) {
        if (dataTypes[index] == startDelim) {
            depth++;
        } else if (dataTypes[index] == endDelim) {
            depth--;
        }
        if (depth > 0) {
            tokens.push_back(dataTypes[index]);
        }
        index++;
    }
    if (depth != 0) {
        TYPE_ERROR("Unmatched delimiter: expected '" + std::string(endDelim) + "' at token " + std::to_string(index));
    }
    return tokens;
}

std::shared_ptr<Type> resolveType(const std::vector<std::string>& dataTypes) {
    if (dataTypes.empty()) {
        TYPE_ERROR("Empty type specification");
        return Type::createInvalid();
    }

    size_t index = 0;
    int totalPointerDepth = 0;
    int totalReferenceDepth = 0;
    int nullableDepth = 0;

    // Parse modifiers
    while (index < dataTypes.size()) {
        if (dataTypes[index] == "&") {
            if (totalReferenceDepth >= 10) { // Arbitrary limit to prevent stack overflow
                TYPE_ERROR("Excessive reference depth at token " + std::to_string(index));
                return Type::createInvalid();
            }
            totalReferenceDepth++;
        } else if (dataTypes[index] == "*") {
            if (totalPointerDepth >= 10) {
                TYPE_ERROR("Excessive pointer depth at token " + std::to_string(index));
                return Type::createInvalid();
            }
            totalPointerDepth++;
        } else if (dataTypes[index] == "?") {
            if (nullableDepth >= 5) {
                TYPE_ERROR("Excessive nullable depth at token " + std::to_string(index));
                return Type::createInvalid();
            }
            nullableDepth++;
        } else {
            break;
        }
        index++;
    }

    if (index >= dataTypes.size()) {
        TYPE_ERROR("No base type found after modifiers at token " + std::to_string(index));
        return Type::createInvalid();
    }

    // Check for function type
    if (dataTypes[index] == "fn") {
        return resolveFunctionType(dataTypes, index);
    }

    std::string baseType = dataTypes[index++];

    // Parse array specifications
    std::vector<uint64_t> arrayStack;
    arrayStack.reserve(4); // Optimize for typical array nesting
    while (index + 2 <= dataTypes.size() && dataTypes[index] == "[") {
        index++;
        if (index >= dataTypes.size()) {
            TYPE_ERROR("Expected array size after '[' at token " + std::to_string(index));
            return Type::createInvalid();
        }

        if (dataTypes[index] == "]") {
            TYPE_ERROR("Array size missing between '[' and ']' at token " + std::to_string(index));
            return Type::createInvalid();
        }

        uint64_t size;
        try {
            size = std::stoull(dataTypes[index]);
            if (size == 0) {
                TYPE_ERROR("Array size must be positive at token " + std::to_string(index));
                return Type::createInvalid();
            }
        } catch (const std::exception& e) {
            TYPE_ERROR("Invalid array size '" + dataTypes[index] + "' at token " + std::to_string(index));
            return Type::createInvalid();
        }
        index++;

        if (index >= dataTypes.size() || dataTypes[index] != "]") {
            TYPE_ERROR("Expected ']' after array size at token " + std::to_string(index));
            return Type::createInvalid();
        }
        index++;
        arrayStack.push_back(size);
    }

    // Parse trailing modifiers
    while (index < dataTypes.size()) {
        if (dataTypes[index] == "&") {
            if (totalReferenceDepth >= 10) {
                TYPE_ERROR("Excessive reference depth at token " + std::to_string(index));
                return Type::createInvalid();
            }
            totalReferenceDepth++;
        } else if (dataTypes[index] == "*") {
            if (totalPointerDepth >= 10) {
                TYPE_ERROR("Excessive pointer depth at token " + std::to_string(index));
                return Type::createInvalid();
            }
            totalPointerDepth++;
        } else if (dataTypes[index] == "?") {
            if (nullableDepth >= 5) {
                TYPE_ERROR("Excessive nullable depth at token " + std::to_string(index));
                return Type::createInvalid();
            }
            nullableDepth++;
        } else {
            std::string seen;
            for (size_t i = 0; i <= index; ++i) {
                if (i > 0) seen += " ";
                seen += dataTypes[i];
            }

            std::string remaining;
            for (size_t i = index; i < dataTypes.size(); ++i) {
                if (i > index) remaining += " ";
                remaining += dataTypes[i];
            }

            TYPE_ERROR(
                "Unexpected token '" + dataTypes[index] + "' after parsing type: '" + seen + "'. "
                "Remaining unparsed: '" + remaining + "'."
            );
            
            return Type::createInvalid();
        }
        index++;
    }

    std::shared_ptr<Type> type;
    std::string_view baseTypeView(baseType);

    // Resolve base type
    auto it = baseTypeMap.find(baseTypeView);
    if (it != baseTypeMap.end()) {
        type = Type::createPrimitiveType(it->second);
        // Attach built-in traits for numeric types
        if (type->isInteger() || type->isFloat()) {
            type->addTrait(BuiltinTraits::Numeric);
            type->addTrait(BuiltinTraits::Addable);
            type->addTrait(BuiltinTraits::Comparable);
            type->addFunctionTrait(BuiltinTraits::Add);
            type->addFunctionTrait(BuiltinTraits::Subtract);
            type->addFunctionTrait(BuiltinTraits::Multiply);
            type->addFunctionTrait(BuiltinTraits::Divide);
            type->addFunctionTrait(BuiltinTraits::Equal);
            type->addFunctionTrait(BuiltinTraits::Compare);
        }
    } else if (baseType == "unique_ptr" || baseType == "shared_ptr" || baseType == "weak_ptr") {
        Kind pointerKind = (baseType == "unique_ptr") ? Kind::UniquePtr :
                          (baseType == "shared_ptr") ? Kind::SharedPtr : Kind::WeakPtr;
        auto pointeeTokens = parseNestedTokens(dataTypes, index, "<", ">");
        if (pointeeTokens.empty() && index < dataTypes.size()) {
            TYPE_ERROR("Expected pointee type for " + baseType + " at token " + std::to_string(index));
            return Type::createInvalid();
        }
        auto pointeeType = resolveType(pointeeTokens);
        if (pointeeType->isInvalid()) {
            TYPE_ERROR("Invalid pointee type for " + baseType + " at token " + std::to_string(index));
            return Type::createInvalid();
        }
        type = Type::createSmartPointerType(pointerKind, pointeeType);
    } else {
        type = Type::createUserDefinedType(baseType);
        // Attach traits for known user-defined types (e.g., Vec2, Sprite)
        if (baseType == "Vec2" || baseType == "Vec3") {
            type->addTrait(BuiltinTraits::Addable);
            type->addFunctionTrait(BuiltinTraits::Add);
        } else if (baseType == "Sprite") {
            type->addTrait(std::make_shared<Trait>("Movable"));
            type->addTrait(std::make_shared<Trait>("Drawable"));
            type->addFunctionTrait(std::make_shared<FunctionTrait>("move", std::dynamic_pointer_cast<FunctionType>(Type::createFunctionType("move", {type}, Type::createPrimitiveType(Kind::Void)))));
            type->addFunctionTrait(std::make_shared<FunctionTrait>("draw", std::dynamic_pointer_cast<FunctionType>(Type::createFunctionType("draw", {type}, Type::createPrimitiveType(Kind::Void)))));
        } else if (baseType == "Scene") {
            type->addTrait(BuiltinTraits::Addable);
            type->addFunctionTrait(std::make_shared<FunctionTrait>("add", std::dynamic_pointer_cast<FunctionType>(Type::createFunctionType("add", {type, Type::createUserDefinedType("Sprite")}, Type::createPrimitiveType(Kind::Void)))));
        }
    }

    if (!type) {
        std::string joined;
        joined.reserve(64);
        for (const auto& t : dataTypes) {
            joined += t;
        }
        TYPE_ERROR("Failed to resolve type: " + joined);
        type = Type::createUnresolved(dataTypes);
    }

    // Apply array modifiers
    for (auto it = arrayStack.rbegin(); it != arrayStack.rend(); ++it) {
        type = Type::createFixedArrayType(type, *it);
    }

    // Apply reference, pointer, and nullable modifiers
    for (int i = 0; i < totalReferenceDepth; ++i) {
        type = Type::createReferenceType(type);
    }
    for (int i = 0; i < totalPointerDepth; ++i) {
        type = Type::createPointerType(type);
    }
    for (int i = 0; i < nullableDepth; ++i) {
        type = Type::createNullableType(type);
    }

    DEBUG_LOG("Resolved type: " + type->toString());
    return type;
}

std::shared_ptr<Type> resolveFunctionType(const std::vector<std::string>& dataTypes, size_t& index) {
    if (index >= dataTypes.size() || dataTypes[index] != "fn") {
        TYPE_ERROR("Expected 'fn' keyword at token " + std::to_string(index));
        return Type::createInvalid();
    }
    index++;

    if (index >= dataTypes.size() || dataTypes[index] != "(") {
        TYPE_ERROR("Expected '(' after 'fn' at token " + std::to_string(index));
        return Type::createInvalid();
    }
    index++;

    std::vector<std::shared_ptr<Type>> paramTypes;
    std::vector<std::string> paramNames;
    paramTypes.reserve(4); // Optimize for typical parameter counts

    while (index < dataTypes.size() && dataTypes[index] != ")") {
        std::vector<std::string> currentParam;
        std::string paramName;

        // Check for named parameter (e.g., "x: int")
        if (index + 1 < dataTypes.size() && dataTypes[index + 1] == ":") {
            paramName = dataTypes[index];
            index += 2;
        }

        // Collect tokens for parameter type
        while (index < dataTypes.size() && dataTypes[index] != "," && dataTypes[index] != ")") {
            currentParam.push_back(dataTypes[index]);
            index++;
        }

        if (!currentParam.empty()) {
            auto paramType = resolveType(currentParam);
            if (paramType->isInvalid()) {
                TYPE_ERROR("Invalid parameter type at token " + std::to_string(index));
                return Type::createInvalid();
            }
            paramTypes.push_back(paramType);
            paramNames.push_back(paramName);
        } else {
            TYPE_ERROR("Empty parameter type at token " + std::to_string(index));
            return Type::createInvalid();
        }

        if (index < dataTypes.size() && dataTypes[index] == ",") {
            index++;
        }
    }

    if (index >= dataTypes.size() || dataTypes[index] != ")") {
        TYPE_ERROR("Expected ')' after function parameters at token " + std::to_string(index));
        return Type::createInvalid();
    }
    index++;

    if (index >= dataTypes.size() || dataTypes[index] != "=>") {
        TYPE_ERROR("Expected '=>' after function parameters at token " + std::to_string(index));
        return Type::createInvalid();
    }
    index++;

    // Parse return type
    std::vector<std::string> returnTypeTokens;
    returnTypeTokens.reserve(dataTypes.size() - index);
    while (index < dataTypes.size()) {
        returnTypeTokens.push_back(dataTypes[index]);
        index++;
    }

    std::shared_ptr<Type> returnType;
    if (returnTypeTokens.empty()) {
        returnType = Type::createPrimitiveType(Kind::Void);
    } else {
        returnType = resolveType(returnTypeTokens);
        if (returnType->isInvalid()) {
            TYPE_ERROR("Invalid return type at token " + std::to_string(index));
            return Type::createInvalid();
        }
    }

    auto fnType = Type::createFunctionType("", paramTypes, returnType, false);
    auto fn = std::dynamic_pointer_cast<FunctionType>(fnType);
    if (fn) {
        for (size_t i = 0; i < paramNames.size(); ++i) {
            fn->setParamName(i, paramNames[i]);
        }
    }

    DEBUG_LOG("Resolved function type: " + fnType->toString());
    return fnType;
}

} // namespace Omniscript