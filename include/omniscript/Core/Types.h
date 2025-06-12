#pragma once
#ifndef VALUE_H
#define VALUE_H

#include <omniscript/engine/tokens.h>
#include <omniscript/Core.h>
#include <omniscript/omniscript_pch.h>
#include <omniscript/debuggingtools/console.h>
#include <omniscript/Core/Target_config.h>

namespace Omniscript {

inline constexpr int getPointerBitWidth() {
    #if defined(TARGET_32BIT)
        return 32;
    #elif defined(TARGET_64BIT)
        return 64;
    #else
        return sizeof(void*) * 8;  // Works in hosted environments
    #endif
}

// Kind enum
enum class Kind {
    Invalid,
    Undefined,

    // Primitive Types
    Primitive,
    Void,
    Nullptr, Null, Nullable,
    Bool,
    Char, Char16, Char32,
    Size_t,
    Int8, Int16, Int32, Int64, Int128, Int256, Int512, Int1024, BigInt,
    UInt8, UInt16, UInt32, UInt64, UInt128, UInt256, UInt512, UInt1024,
    Half, Float, Double, FP128, X86_FP80, PPC_FP128,

    // Special Types
    Label,
    Token,
    Metadata,

    // Aggregate Types
    Module,
    Class,
    Struct,
    Enum,
    Array,
    Vector,
    
    // Pointer Types
    Pointer,
    Reference,

    // Function Types
    Function,

    // Custom Types
    String,
    Utf8,
    Utf16,
    Utf32,
    
    //ArrayTypes
    FixedArray,       // e.g., [4]i32
    DynamicArray,     // e.g., [i32]
    HeterogeneousArray, // e.g., []
    
    //Other Types
    UserDefined,
    Call,
    Unresolved,
    Generic,
    Block,
};

// Base class for all type representations
class Type {
public:
    std::string parameterName;
    Kind kind = Kind::Invalid;
    std::shared_ptr<Type> returnType;
    std::shared_ptr<Type> elementType;
    std::shared_ptr<Type> pointeeType;
    size_t fixedSize = 0;
    virtual ~Type() = default;

    Type() {}
    Type(Kind kind) : kind(kind) {}

    // ----- Instance methods -----
    bool isUserDefined() const { return kind == Kind::UserDefined; }
    bool isInvalid() const { return kind == Kind::Invalid; }
    bool isVoidLike() const { return kind == Kind::Void || kind == Kind::Undefined;}
    bool isUnresolved() const { return kind == Kind::Unresolved; }
    bool isUndefined() const { return kind == Kind::Undefined; }
    bool isPointer() const { return kind == Kind::Pointer || kind == Kind::Nullptr; }
    bool isNullType() const { return kind == Kind::Null || kind == Kind::Nullptr; }
    bool isNull() const { return kind == Kind::Null; }
    bool isNullable() const { return kind == Kind::Nullable; }
    bool isNullPointer() const { return kind == Kind::Nullptr; }
    bool isReference() const { return kind == Kind::Reference; }
    bool isFunction() const { return kind == Kind::Function; }
    bool isPrimitive() const { return kind == Kind::Primitive; }
    bool isArray() const { return kind == Kind::FixedArray || kind == Kind::DynamicArray || kind == Kind::HeterogeneousArray; }
    bool isFixedArray() const { return kind == Kind::FixedArray; }
    bool isDynamicArray() const { return kind == Kind::DynamicArray; }
    bool isHeterogeneousArray() const { return kind == Kind::HeterogeneousArray; }
    bool isGeneric() const { return kind == Kind::Generic; }
    bool isBlock() const { return kind == Kind::Block; }
    bool isBool() const { return kind == Kind::Bool; }
    bool isStruct() const { return kind == Kind::Struct; }
    bool isClass() const { return kind == Kind::Class; }
    bool isModule() const { return kind == Kind::Module; }

    bool isChar(int bitwidth = -1) const { 
        if (bitwidth == -1) {
            return kind == Kind::Char || 
            kind == Kind::Char16 || 
            kind == Kind::Char32;
        }

        switch (bitwidth) {
            case 8:
                return kind == Kind::Char;
                break;
            case 16:
                return kind == Kind::Char16;
                break;
            case 32:
                return kind == Kind::Char32;
                break;
            default:
                return false;
        }
    }

    bool isNumericLiteral() const {
        return isInteger() || isFloat();
    }
    
    bool isString(int bitwidth = -1) const {
        if (bitwidth == -1) {
            return kind == Kind::String || 
            kind == Kind::Utf8 || 
            kind == Kind::Utf16 || 
            kind == Kind::Utf32;
        }

        switch (bitwidth) {
            case 8:
                return kind == Kind::String || kind == Kind::Utf8;
                break;
            case 16:
                return kind == Kind::Utf16;
                break;
            case 32:
                return kind == Kind::Utf32;
                break;
            default:
                return false;
        }
    }

    bool isSigned() const {
        switch (kind) {
            case Kind::Int8:
            case Kind::Int16:
            case Kind::Int32:
            case Kind::Int64:
            case Kind::Int128:
            case Kind::Int256:
            case Kind::Int512:
            case Kind::Int1024:
                return true;  // These are signed types
            case Kind::Size_t:
            case Kind::UInt8:
            case Kind::UInt16:
            case Kind::UInt32:
            case Kind::UInt64:
            case Kind::UInt128:
            case Kind::UInt256:
            case Kind::UInt512:
            case Kind::UInt1024:
                return false;  // These are unsigned types
            default:
                return false;  // Other types are neither signed nor unsigned integers
        }
    }

    int getBitWidth() const {
        return getSize();
    }
    
    int getSize() const {
        using enum Kind;
    
        switch (kind) {
            // Fixed-size primitives
            case Bool:         return 1;
            case Char:         return 8;
            case Char16:       return 16;
            case Char32:       return 32;
    
            case Int8:         case UInt8:         return 8;
            case Int16:        case UInt16:        return 16;
            case Int32:        case UInt32:        return 32;
            case Int64:        case UInt64:        return 64;
            case Int128:       case UInt128:       return 128;
            case Int256:       case UInt256:       return 256;
            case Int512:       case UInt512:       return 512;
            case Int1024:      case UInt1024:      return 1024;

            // Size_type
            case Size_t:       return getPointerBitWidth();
    
            // Arbitrary precision
            case BigInt:       return -1; // Size not fixed — may depend on value
    
            // Floats
            case Half:         return 16;
            case Float:        return 32;
            case Double:       return 64;
            case FP128:        return 128;
            case X86_FP80:     return 80;
            case PPC_FP128:    return 128;
    
            // Pointers
            case Pointer:
            case Reference:    return 64; // Assuming 64-bit pointers (could be dynamic)
    
            // Special types with no size
            case Void:
            case Null:
            case Nullptr:
            case Invalid:
            case Unresolved:
                return 0;
    
            // These are not value types
            case Label:
            case Metadata:
            case Token:
            case Call:
            case Block:
            case Function:
                return -1;
    
            // Arrays
            case FixedArray:
            case DynamicArray:
            case HeterogeneousArray:
                return -1; // Size depends on element type and length
    
            // Aggregates
            case Struct:
            case Enum:
            case Vector:
                return -1; // Should be computed based on fields or layout
    
            // Custom / string-like
            case String:
            case Utf8:
            case Utf16:
            case Utf32:
                return -1; // Variable-sized — usually heap-allocated
    
            // Generics
            case Generic:
                return -1; // Needs instantiation first
    
            // Catch-all
            default:
                return -1;
        }
    }
    
    bool isSizeType() const {
        return kind == Kind::Size_t;
    }

    bool isInteger(int bitwidth = -1) const {
        // If no bitwidth is specified, just check if the kind is an integer
        if (bitwidth == -1) {
            return kind == Kind::Int8 || kind == Kind::Int16 || kind == Kind::Int32 ||
                   kind == Kind::Int64 || kind == Kind::Int128 || kind == Kind::Int256 ||
                   kind == Kind::Int512 || kind == Kind::Int1024 ||
                   kind == Kind::Size_t ||
                   kind == Kind::UInt8 || kind == Kind::UInt16 ||
                   kind == Kind::UInt32 || kind == Kind::UInt64 || kind == Kind::UInt128 ||
                   kind == Kind::UInt256 || kind == Kind::UInt512 || kind == Kind::UInt1024 ||
                   kind == Kind::BigInt;
        }

        if (kind == Kind::Size_t) {
            return getPointerBitWidth() == bitwidth;
        }

        // Otherwise, check the bitwidth against the kind of integer
        switch (bitwidth) {
            case 8:
                return kind == Kind::Int8 || kind == Kind::UInt8;
            case 16:
                return kind == Kind::Int16 || kind == Kind::UInt16;
            case 32:
                return kind == Kind::Int32 || kind == Kind::UInt32;
            case 64:
                return kind == Kind::Int64 || kind == Kind::UInt64;
            case 128:
                return kind == Kind::Int128 || kind == Kind::UInt128;
            case 256:
                return kind == Kind::Int256;
            case 512:
                return kind == Kind::Int512;
            case 1024:
                return kind == Kind::Int1024;
            // case 1024:
                // return kind == Kind::BigInt;
            default:
                return false;  // Unsupported bitwidth
        }
    }

    bool isFloat(int bitWidth = -1) const {
        if (bitWidth == -1) {
            return kind == Kind::Half || kind == Kind::Float ||
                   kind == Kind::Double || kind == Kind::FP128 ||
                   kind == Kind::X86_FP80 || kind == Kind::PPC_FP128;
        }
    
        // Check if the type matches the specified bit width for floats
        if (bitWidth == 16) {
            return kind == Kind::Half;
        } else if (bitWidth == 32) {
            return kind == Kind::Float;
        } else if (bitWidth == 64) {
            return kind == Kind::Double;
        } else if (bitWidth == 128) {
            return kind == Kind::FP128 || kind == Kind::PPC_FP128;  // PPC_FP128 also matches 128 bits
        } else if (bitWidth == 80) {
            return kind == Kind::X86_FP80;
        }
    
        // If needed, you can add further checks for other non-standard float types
    
        return false;
    }    

    virtual std::shared_ptr<Type> getReturnType() const { return nullptr; }

    virtual std::string kindName() const;
    //Todo:: Implement a description/toString method for types
    virtual std::string description() const { return kindName(); }
    virtual std::string toString() const { return description(); }
    Kind getKind() const { return kind; }

    virtual std::string getName() const { return "type"; };
    virtual std::string getParameterName() const { return parameterName; };

    // Access underlying types
    virtual int getReferenceDepth() const { return 0; }
    virtual int getPointerDepth() const { return 0; }
    virtual std::string pointerDescription() const { return ""; }
    virtual std::shared_ptr<Type> getPointeeType() const { return nullptr; }
    virtual std::shared_ptr<Type> getBasePointeeType() const { return nullptr; }
    virtual std::shared_ptr<Type> getReferencedType() const { return nullptr; }
    virtual std::shared_ptr<Type> getBaseReferencedType() const { return nullptr; }
    virtual std::shared_ptr<Type> getElementType() const { return nullptr; }

    // ----- Static factory methods -----
    static std::shared_ptr<Type> createInvalid();
    static std::shared_ptr<Type> createUnresolved(const std::vector<std::string>& types);
    static std::shared_ptr<Type> createUndefined();
    static std::shared_ptr<Type> createPrimitiveType(Kind kind);
    static std::shared_ptr<Type> createNullType(std::shared_ptr<Type> innerType = nullptr);
    static std::shared_ptr<Type> createNullPointerType(std::shared_ptr<Type> innerType = nullptr);
    static std::shared_ptr<Type> createNullableType(std::shared_ptr<Type> innerType = nullptr);
    static std::shared_ptr<Type> createMetaType();
    static std::shared_ptr<Type> createPointerType(
        std::shared_ptr<Type> pointee,
        bool isConst = false,
        bool isVolatile = false
    );
    static std::shared_ptr<Type> createReferenceType(std::shared_ptr<Type> referent);
    static std::shared_ptr<Type> createFunctionType(
        const std::string& name,
        const std::vector<std::shared_ptr<Type>>& paramTypes = {},
        std::shared_ptr<Type> returnType = nullptr,
        bool isVarArg = false
    );
    static std::shared_ptr<Type> createStringType(Kind stringKind = Kind::String);
    static std::shared_ptr<Type> createFixedArrayType(std::shared_ptr<Type> elementType, size_t size);
    static std::shared_ptr<Type> createDynamicArrayType(std::shared_ptr<Type> elementType);
    static std::shared_ptr<Type> createHeterogeneousArrayType();
    static std::shared_ptr<Type> createGenericType(const std::string& typeName);
    static std::shared_ptr<Type> createUserDefinedType(
        const std::string& name,
        Kind kind = Kind::UserDefined,
        const std::vector<std::shared_ptr<Type>>& paramTypes = {},
        const std::vector<std::shared_ptr<Type>>& typeParams = {},
        const std::vector<std::shared_ptr<Type>>& baseTypes = {}
    );
    
    

    virtual std::shared_ptr<Type> clone() const {
        // Fallback clone for base Type (can optionally throw if never meant to be instantiated)
        return std::make_shared<Type>(*this);
    }
};

// --- Derived Types ---

class PrimitiveType : public Type {
public:
    template <typename T>
    static Kind get() {
        if constexpr (std::is_same_v<T, bool>) return Kind::Bool;
        if constexpr (std::is_same_v<T, char>) return Kind::Char;
        if constexpr (std::is_same_v<T, char8_t>) return Kind::Char;
        if constexpr (std::is_same_v<T, char16_t>) return Kind::Char16;
        if constexpr (std::is_same_v<T, char32_t>) return Kind::Char32;
        
        if constexpr (std::is_same_v<T, int8_t>) return Kind::Int8;
        if constexpr (std::is_same_v<T, int16_t>) return Kind::Int16;
        if constexpr (std::is_same_v<T, int32_t>) return Kind::Int32;
        if constexpr (std::is_same_v<T, int64_t>) return Kind::Int64;
        if constexpr (std::is_same_v<T, __int128>) return Kind::Int128;
        // if constexpr (std::is_same_v<T, __int256>) return Kind::Int256;
        // if constexpr (std::is_same_v<T, __int512>) return Kind::Int512;
        // if constexpr (std::is_same_v<T, __int1024>) return Kind::Int1024;

        if constexpr (std::is_same_v<T, size_t>) return Kind::Size_t;
        if constexpr (std::is_same_v<T, uint8_t>) return Kind::UInt8;
        if constexpr (std::is_same_v<T, uint16_t>) return Kind::UInt16;
        if constexpr (std::is_same_v<T, uint32_t>) return Kind::UInt32;
        if constexpr (std::is_same_v<T, uint64_t>) return Kind::UInt64;
        if constexpr (std::is_same_v<T, unsigned __int128>) return Kind::UInt128;

        // Check if the type is a 32-bit float
        #ifdef __ARM_ARCH
            if constexpr (std::is_same_v<T, __fp16>) return Kind::Half;
        #elif defined(__x86_64__) || defined(__i386__)
            if constexpr (std::is_same_v<T, _Float16>) return Kind::Half;
        #endif
        if constexpr (std::is_same_v<T, float>) return Kind::Float;
        if constexpr (std::is_same_v<T, double>) return Kind::Double;
        if constexpr (std::is_same_v<T, __float128>) return Kind::FP128;
        if constexpr (std::is_same_v<T, long double>) return Kind::X86_FP80;

        if constexpr (std::is_same_v<T, std::string>) return Kind::Char;
        if constexpr (std::is_same_v<T, std::u16string>) return Kind::Char16;
        if constexpr (std::is_same_v<T, std::u32string>) return Kind::Char32;

        return Kind::Invalid;  // Default case if type isn't handled
    }

    Kind primitiveKind;

    explicit PrimitiveType(Kind kind_) : primitiveKind(kind_) { 
        kind = kind_;
    }

    static std::shared_ptr<Type> create(Kind kind) {
        return std::make_shared<PrimitiveType>(kind);
    }

    std::shared_ptr<Type> clone() const override {
        return std::make_shared<PrimitiveType>(primitiveKind);
    }    
};


class UnresolvedType : public Type {
public:
    std::vector<std::string> dataTypes;

    UnresolvedType(const std::vector<std::string>& dataTypes)
    : Type(Kind::Unresolved), dataTypes(dataTypes) {}

    // Inheritance check (for multiple inheritance)
    // bool derivesFrom(const std::string& baseName) const {
    //     for (const auto& base : baseTypes) {
    //         if (base->name == baseName || base->derivesFrom(baseName)) {
    //             return true;
    //         }
    //     }
    //     return false;
    // }
};

class UserDefinedType : public Type {
public:
    std::string name;
    std::vector<std::shared_ptr<Type>> paramTypes;
    std::vector<std::shared_ptr<Type>> typeParams;
    std::vector<std::shared_ptr<Type>> baseTypes;

    UserDefinedType(const std::string& name, Kind kind = Kind::UserDefined)
        : Type(kind), name(name) {}

    std::string kindName() const override { return name; }
    std::string getName() const override { return name; }

    // Inheritance check (for multiple inheritance)
    // bool derivesFrom(const std::string& baseName) const {
    //     for (const auto& base : baseTypes) {
    //         if (base->name == baseName || base->derivesFrom(baseName)) {
    //             return true;
    //         }
    //     }
    //     return false;
    // }
};

class PointerType : public Type {
public:
    bool nullCaseHandled = false;
    bool isConst;
    bool isVolatile;

    // Constructor
    PointerType(std::shared_ptr<Type> pointeeType, bool isConst = false, bool isVolatile = false) {
            this->pointeeType = pointeeType;
            kind = Kind::Pointer;
            this->isConst = isConst;
            this->isVolatile = isVolatile;
        }

    // Get the pointee type (directly)
    std::shared_ptr<Type> getPointeeType() const override {
        return pointeeType;
    }

    // Method to get the pointer depth recursively
    int getPointerDepth() const override {
        int depth = 0;
        auto currentPointee = pointeeType;

        while (currentPointee->isPointer()) {
            depth++;
            currentPointee = std::dynamic_pointer_cast<PointerType>(currentPointee)->getPointeeType();
        }

        return depth;
    }

    // Method to get the base pointee type (the deepest pointee type)
    std::shared_ptr<Type> getBasePointeeType() const {
        auto currentPointee = pointeeType;

        while (currentPointee->isPointer()) {
            currentPointee = std::dynamic_pointer_cast<PointerType>(currentPointee)->getPointeeType();
        }

        return currentPointee;
    }

    std::string description() const override { return pointerDescription(); }
    std::string pointerDescription() const override {
        std::vector<std::string> parts;
        std::shared_ptr<Type> current = pointeeType;  // start from the first pointee
    
        // Walk through all pointer levels
        while (current->isPointer()) {
            parts.push_back("pointer");
            current = std::dynamic_pointer_cast<PointerType>(current);
            if (current) {
                current->getPointeeType();
            } else {
                break;
            }
        }
        
        if (current) {
            // Add the base type at the end
            parts.push_back(current->toString());
        }
    
        // Reverse to get natural order (base type first)
        std::reverse(parts.begin(), parts.end());
    
        // Join with spaces
        std::string description;
        for (const auto& part : parts) {
            if (!description.empty()) description += " ";
            description += part;
        }
    
        return description;
    }
    
    std::shared_ptr<Type> clone() const override {
        return std::make_shared<PointerType>(pointeeType->clone());
    }    
};

class NullType : public Type {
public:
    bool nullCaseHandled = false;
    std::shared_ptr<Type> innerType;
    NullType(std::shared_ptr<Type> innerType = nullptr) : innerType(innerType) {  // You can use any default 'unknown' type
        kind = Kind::Null;
    }

    std::string toString() const override {
        return "Null<" + (innerType ? innerType->toString() : "Unknown") + ">";
    }

    std::shared_ptr<Type> clone() const override {
        return std::make_shared<NullType>(innerType ? innerType->clone() : nullptr);
    }  
};

class NullableType : public Type {
public:
    bool nullCaseHandled = false;
    std::shared_ptr<Type> innerType;
    NullableType(std::shared_ptr<Type> innerType = nullptr) : innerType(innerType) {  // You can use any default 'unknown' type
        kind = Kind::Nullable;
    }

    std::string toString() const override {
        return "Nullable<" + (innerType ? innerType->toString() : "Unknown") + ">";
    }

    std::shared_ptr<Type> clone() const override {
        return std::make_shared<NullableType>(innerType ? innerType->clone() : nullptr);
    }  
};

class NullPointerType : public Type {
public:
    bool nullCaseHandled = false;
    bool isConst;
    bool isVolatile;

    // Constructor
    NullPointerType(std::shared_ptr<Type> pointeeType, bool isConst = false, bool isVolatile = false) {
            this->pointeeType = pointeeType;
            kind = Kind::Nullptr;
            this->isConst = isConst;
            this->isVolatile = isVolatile;
        }

    // Get the pointee type (directly)
    std::shared_ptr<Type> getPointeeType() const override {
        return pointeeType;
    }

    // Method to get the pointer depth recursively
    int getPointerDepth() const override {
        int depth = 0;
        auto currentPointee = pointeeType;

        while (currentPointee->isPointer()) {
            depth++;
            currentPointee = std::dynamic_pointer_cast<PointerType>(currentPointee)->getPointeeType();
        }

        return depth;
    }

    // Method to get the base pointee type (the deepest pointee type)
    std::shared_ptr<Type> getBasePointeeType() const {
        auto currentPointee = pointeeType;

        while (currentPointee->isPointer()) {
            currentPointee = std::dynamic_pointer_cast<PointerType>(currentPointee)->getPointeeType();
        }

        return currentPointee;
    }

    std::string description() const override { return pointerDescription(); }
    std::string pointerDescription() const override {
        std::vector<std::string> parts;
        std::shared_ptr<Type> current = pointeeType;  // start from the first pointee
    
        // Walk through all pointer levels
        while (current->isPointer()) {
            parts.push_back("pointer");
            current = std::dynamic_pointer_cast<PointerType>(current);
            if (current) {
                current->getPointeeType();
            } else {
                break;
            }
        }
    
        // Add the base type at the end
        if (current) {
            // Add the base type at the end
            parts.push_back(current->toString());
        }
    
        // Reverse to get natural order (base type first)
        std::reverse(parts.begin(), parts.end());
    
        // Join with spaces
        std::string description;
        for (const auto& part : parts) {
            if (!description.empty()) description += " ";
            description += part;
        }
    
        return description;
    }
    
    std::shared_ptr<Type> clone() const override {
        return std::make_shared<NullPointerType>(pointeeType->clone());
    }    
};

class ReferenceType : public Type {
public:
    std::shared_ptr<Type> referentType;

    explicit ReferenceType(std::shared_ptr<Type> referentType)
        : referentType(referentType) {
        kind = Kind::Reference;
    }

    std::shared_ptr<Type> getReferencedType() const override { 
        return referentType; 
    }

    // Get depth of reference levels (e.g., &&var is depth 2)
    int getReferenceDepth() const override {
        int depth = 1;  // start from 1 since this is already a reference
        auto current = referentType;

        while (current->isReference()) {
            depth++;
            current = std::dynamic_pointer_cast<ReferenceType>(current)->getReferencedType();
        }

        return depth;
    }

    // Get the ultimate base type (i.e., the non-reference type at the bottom)
    std::shared_ptr<Type> getBaseReferencedType() const override {
        auto current = referentType;

        while (current->isReference()) {
            current = std::dynamic_pointer_cast<ReferenceType>(current)->getReferencedType();
        }

        return current;
    }

    std::string referenceDescription() const {
        std::vector<std::string> parts;
        std::shared_ptr<Type> current = referentType;

        while (current->isReference()) {
            parts.push_back("reference");
            current = std::dynamic_pointer_cast<ReferenceType>(current)->getReferencedType();
        }

        parts.push_back(current->toString());
        std::reverse(parts.begin(), parts.end());

        std::string desc;
        for (const auto& part : parts) {
            if (!desc.empty()) desc += " ";
            desc += part;
        }

        return desc;
    }

    std::shared_ptr<Type> clone() const override {
        return std::make_shared<ReferenceType>(referentType->clone());
    }    
};


class FunctionType : public Type {
public:
    std::string functionName;
    std::vector<std::shared_ptr<Type>> parameterTypes;
    std::shared_ptr<Type> returnType;
    bool isVarArg = false;

    FunctionType(
        const std::string& name,
        std::shared_ptr<Type> returnType,
        const std::vector<std::shared_ptr<Type>>& params,
        bool isVarArg = false
    ) {
        kind = Kind::Function;
        functionName = name;
        this->returnType = returnType;
        parameterTypes = params;
        this->isVarArg = isVarArg;
    }

    size_t getArity() const {
        return parameterTypes.size();
    }

    std::shared_ptr<Type> getParamType(size_t index) const {
        if (index < parameterTypes.size()) {
            return parameterTypes[index];
        }
        return nullptr;
    }

    std::shared_ptr<Type> getReturnType() const override {
        return returnType;
    }

    std::string toString() const {
        std::string result = functionName + "(";
        for (size_t i = 0; i < parameterTypes.size(); ++i) {
            result += parameterTypes[i]->parameterName;
            if (i < parameterTypes.size() - 1)
                result += ", ";
        }
        if (isVarArg) {
            if (!parameterTypes.empty()) result += ", ";
            result += "...";
        }
        result += ") -> ";
        result += returnType ? returnType->parameterName : "void";
        return result;
    }
};


class GenericType : public Type {
public:
    std::string name;

    GenericType(const std::string& name_)
        : name(name_) {
        kind = Kind::Generic;
    }

    std::string getName() const override {
        return name;
    }

    std::shared_ptr<Type> clone() const override {
        return std::make_shared<GenericType>(name);
    }    
};


std::shared_ptr<Type> resolveType(const std::vector<std::string>& dataTypes);
bool isSameOrCastableTo(const std::shared_ptr<Type>& from, const std::shared_ptr<Type>& to);

}

#endif