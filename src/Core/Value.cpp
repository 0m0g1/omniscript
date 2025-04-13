#include <omniscript/Core.h>
#include <omniscript/Core/Value.h>
#include <omniscript/omniscript_pch.h>

namespace Omniscript {

std::string Type::kindName() const {
    switch (kind) {
        case Kind::Invalid:     return "Invalid";

        // Primitive Types
        case Kind::Primitive:   return "Primitive";
        case Kind::Void:        return "Void";
        case Kind::Nullptr:     return "Nullptr";
        case Kind::Null:        return "Null";
        case Kind::Bool:        return "Bool";
        case Kind::Char:        return "Char";
        case Kind::Int8:        return "Int8";
        case Kind::Int16:       return "Int16";
        case Kind::Int32:       return "Int32";
        case Kind::Int64:       return "Int64";
        case Kind::Int128:      return "Int128";
        case Kind::Int256:      return "Int256";
        case Kind::Int512:      return "Int512";
        case Kind::Int1024:     return "Int1024";
        case Kind::BigInt:      return "BigInt";

        case Kind::UInt8:       return "UInt8";
        case Kind::UInt16:      return "UInt16";
        case Kind::UInt32:      return "UInt32";
        case Kind::UInt64:      return "UInt64";
        case Kind::UInt128:     return "UInt128";
        case Kind::UInt256:     return "UInt256";
        case Kind::UInt512:     return "UInt512";
        case Kind::UInt1024:    return "UInt1024";

        case Kind::Half:        return "Half";
        case Kind::Float:       return "Float";
        case Kind::Double:      return "Double";
        case Kind::FP128:       return "FP128";
        case Kind::X86_FP80:    return "X86_FP80";
        case Kind::PPC_FP128:   return "PPC_FP128";

        // Special Types
        case Kind::Label:       return "Label";
        case Kind::Token:       return "Token";
        case Kind::Metadata:    return "Metadata";

        // Aggregate Types
        case Kind::Struct:      return "Struct";
        case Kind::Enum:        return "Enum";
        case Kind::Array:       return "Array";
        case Kind::FixedArray:       return "FixedArray";
        case Kind::DynamicArray:       return "DynamicArray";
        case Kind::HeterogeneousArray:    return "HeterogeneousArray";
        case Kind::Vector:      return "Vector";

        // Pointer/Reference Types
        case Kind::Pointer:     return "Pointer";
        case Kind::Reference:   return "Reference";

        // Function Types
        case Kind::Function:    return "Function";

        // Custom Types
        case Kind::String:      return "String";
        case Kind::Utf8:      return "Utf8String";
        case Kind::Utf16:      return "Utf16String";
        case Kind::Utf32:      return "Utf32String";

        case Kind::Generic:    return "Generic";

        default:                return "Unknown";
    }
}

std::shared_ptr<Type> Type::createInvalid() {
    return std::make_shared<Type>();
}

std::shared_ptr<Type> Type::createPrimitiveType(Kind kind) {
    return std::make_shared<PrimitiveType>(kind);
}

std::shared_ptr<Type> Type::createPointerType(std::shared_ptr<Type> pointee) {
    return std::make_shared<PointerType>(std::move(pointee));
}

std::shared_ptr<Type> Type::createNullPointerType() {
    return std::make_shared<Omniscript::NullPointerType>();
}

std::shared_ptr<Type> Type::createNullType() {
    return std::make_shared<Omniscript::NullType>();
}

std::shared_ptr<Type> Type::createReferenceType(std::shared_ptr<Type> referent) {
    return std::make_shared<ReferenceType>(std::move(referent));
}

std::shared_ptr<Type> Type::createFunctionType(std::shared_ptr<Type> returnType, std::vector<std::shared_ptr<Type>> params, bool isVarArg) {
    return std::make_shared<FunctionType>(returnType, std::move(params), isVarArg);
}

std::shared_ptr<Type> Type::createStringType(Kind stringKind) {
    auto t = std::make_shared<Type>();
    t->kind = stringKind;
    return t;
}

std::shared_ptr<Type> Type::createFixedArrayType(std::shared_ptr<Type> elementType, size_t size) {
    auto t = std::make_shared<Type>();
    t->kind = Kind::FixedArray;
    t->elementType = std::move(elementType);
    t->fixedSize = size;
    return t;
}

std::shared_ptr<Type> Type::createDynamicArrayType(std::shared_ptr<Type> elementType) {
    auto t = std::make_shared<Type>();
    t->kind = Kind::DynamicArray;
    t->elementType = std::move(elementType);
    return t;
}

std::shared_ptr<Type> Type::createHeterogeneousArrayType() {
    auto t = std::make_shared<Type>();
    t->kind = Kind::HeterogeneousArray;
    return t;
}

std::shared_ptr<Type> Type::createGenericType(const std::string& typeName) {
    return std::make_shared<GenericType>(typeName);
}

std::shared_ptr<Type> resolveType(const std::vector<std::string>& dataTypes) {
    if (dataTypes.empty()) {
        return Type::createPrimitiveType(Kind::Int32); // Default type
    }

    size_t index = 0;
    int totalPointerDepth = 0;
    int totalReferenceDepth = 0;

    // Stack of arrays (outermost first)
    std::vector<std::optional<uint64_t>> arrayStack;
    // std::vector<Type> arrayTypeStack;
    // Parse nested arrays like [4][3][]
    // [5]char
    if (index < dataTypes.size() && dataTypes[index] == "[") {
        index += 1;
        arrayStack.push_back(std::stoull(dataTypes[index]));
        index += 1;
        if (dataTypes[index] == "]") {
            index += 1;
        }
    }
    // while (index < dataTypes.size() && dataTypes[index] == "[") {
    //     if (index + 2 < dataTypes.size() && dataTypes[index + 2] == "]") {
    //         std::string sizeStr = dataTypes[index + 1];

    //         if (std::all_of(sizeStr.begin(), sizeStr.end(), ::isdigit)) {
    //             arrayStack.push_back(std::stoull(sizeStr));  // Fixed array
    //         } else if (sizeStr.empty()) {
    //             arrayStack.push_back(std::nullopt);          // Dynamic array
    //         } else {
    //             std::cerr << "[ERROR] Invalid array size: '" << sizeStr << "'\n";
    //             return nullptr;
    //         }

    //         index += 3; // Move past [, size, ]
    //     } else {
    //         std::cerr << "[ERROR] Invalid array declaration near index " << index << "\n";
    //         return nullptr;
    //     }
    // }

    // References
    while (index < dataTypes.size() && dataTypes[index] == "&") {
        totalReferenceDepth++;
        index++;
    }

    // Pointers
    while (index < dataTypes.size() && dataTypes[index] == "*") {
        totalPointerDepth++;
        index++;
    }

    // Base type
    if (index >= dataTypes.size()) {
        std::cerr << "[ERROR] No base type found after modifiers!" << std::endl;
        return nullptr;
    }

    std::string baseType = dataTypes[index++];

    // Post-type pointer depth
    while (index < dataTypes.size() && dataTypes[index] == "*") {
        totalPointerDepth++;
        index++;
    }

    std::shared_ptr<Type> type;

    if (!type) {
        DEBUG_LOG("Resolving base type: " + baseType);
    
        // Signed integers
        if (baseType == "int" || baseType == "i32" || baseType == "int32") type = Type::createPrimitiveType(Kind::Int32);
        else if (baseType == "int8" || baseType == "i8") type = Type::createPrimitiveType(Kind::Int8);
        else if (baseType == "int16" || baseType == "i16") type = Type::createPrimitiveType(Kind::Int16);
        else if (baseType == "int64" || baseType == "i64") type = Type::createPrimitiveType(Kind::Int64);
        else if (baseType == "int128" || baseType == "i128") type = Type::createPrimitiveType(Kind::Int128);
        else if (baseType == "int256" || baseType == "i256") type = Type::createPrimitiveType(Kind::Int256);
        else if (baseType == "int512" || baseType == "i512") type = Type::createPrimitiveType(Kind::Int512);
        else if (baseType == "int1024" || baseType == "i1024") type = Type::createPrimitiveType(Kind::Int1024);
        else if (baseType == "BigInt") type = Type::createPrimitiveType(Kind::BigInt);
    
        // Unsigned integers
        else if (baseType == "uint" || baseType == "u32" || baseType == "uint32") type = Type::createPrimitiveType(Kind::UInt32);
        else if (baseType == "uint8" || baseType == "u8") type = Type::createPrimitiveType(Kind::UInt8);
        else if (baseType == "uint16" || baseType == "u16") type = Type::createPrimitiveType(Kind::UInt16);
        else if (baseType == "uint64" || baseType == "u64") type = Type::createPrimitiveType(Kind::UInt64);
        else if (baseType == "uint128" || baseType == "u128") type = Type::createPrimitiveType(Kind::UInt128);
        else if (baseType == "uint256" || baseType == "u256") type = Type::createPrimitiveType(Kind::UInt256);
        else if (baseType == "uint512" || baseType == "u512") type = Type::createPrimitiveType(Kind::UInt512);
        else if (baseType == "uint1024" || baseType == "u1024") type = Type::createPrimitiveType(Kind::UInt1024);
    
        // Other primitives
        else if (baseType == "bool") type = Type::createPrimitiveType(Kind::Bool);
        else if (baseType == "char") type = Type::createPrimitiveType(Kind::Char);
        else if (baseType == "void") type = Type::createPrimitiveType(Kind::Void);
    
        // Floating point
        else if (baseType == "half" || baseType == "f16") type = Type::createPrimitiveType(Kind::Half);
        else if (baseType == "float" || baseType == "f32") type = Type::createPrimitiveType(Kind::Float);
        else if (baseType == "double" || baseType == "f64") type = Type::createPrimitiveType(Kind::Double);
        else if (baseType == "fp128" || baseType == "f128" || baseType == "long_double") type = Type::createPrimitiveType(Kind::FP128);
        else if (baseType == "x86_fp80" || baseType == "x86_80bit" || baseType == "x87_FP80" || baseType == "Intel_FP80") type = Type::createPrimitiveType(Kind::X86_FP80);
        else if (baseType == "ppc_fp128" || baseType == "PPC_double_extended" || baseType == "PPC_F128" || baseType == "PPC_Quad") type = Type::createPrimitiveType(Kind::PPC_FP128);

    
        // Strings / UTF
        else if (baseType == "string" || baseType == "str" || baseType == "utf8") {
            type = Type::createPrimitiveType(Kind::Char);
            totalPointerDepth++;
        }
        else if (baseType == "utf16") {
            type = Type::createPrimitiveType(Kind::Utf16);
            totalPointerDepth++;
        }
        else if (baseType == "utf32") {
            type = Type::createPrimitiveType(Kind::Utf32);
            totalPointerDepth++;
        }

        else {
            if (dataTypes.size() == 1) {
                return Type::createGenericType(baseType);
            }
            DEBUG_LOG("Type: '" + baseType + "' is not supported in omniscript");
            return nullptr;
        }
    }
    

   // Wrap in arrays (from innermost to outermost)
   for (auto it = arrayStack.rbegin(); it != arrayStack.rend(); ++it) {
        if (it->has_value()) {
            type = Type::createFixedArrayType(type, it->value());  // Fixed array
        } else {
            type = Type::createDynamicArrayType(type);             // Dynamic array
        }
    }

    // Wrap in references
    for (int i = 0; i < totalReferenceDepth; ++i) {
        type = Type::createReferenceType(type);
    }

    // Wrap in pointers
    for (int i = 0; i < totalPointerDepth; ++i) {
        type = Type::createPointerType(type);
    }

    return type;
}


} // namespace Omniscript