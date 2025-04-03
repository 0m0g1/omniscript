#pragma once
#ifndef VALUE_H
#define VALUE_H

#include <omniscript/Core.h>
#include <omniscript/omniscript_pch.h>

namespace Omniscript {

enum Type {
    Invalid,
    
    // Primitive Types
    Primitive,
    Void,
    Nullptr, Null,
    Bool,
    Char,
    Int8, Int16, Int32, Int64, Int128,
    UInt8, UInt16, UInt32, UInt64, UInt128,
    Half, Float, Double, FP128, X86_FP80, PPC_FP128,
    
    // Special Types
    Label,
    Token,
    Metadata,
    
    // Aggregate Types
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
    WideString
};

// Base class for all types
struct Value {
    Type type = Type::Invalid;  // The type of the value

    virtual ~Value() = default;  // Ensure proper polymorphism
};

// Primitive Types (e.g., Int8, Bool)
struct PrimitiveValue : public Value {
    enum class PrimitiveType {
        Void,
        Bool,
        Char,
        Int8, Int16, Int32, Int64, Int128,
        UInt8, UInt16, UInt32, UInt64, UInt128,
        Half, Float, Double, FP128, X86_FP80, PPC_FP128
    };

    PrimitiveType primitiveType;

    PrimitiveValue(PrimitiveType primitiveType)
        : primitiveType(primitiveType) {
        type = Type::Primitive;  // Set the type to Primitive
    }
};

// Pointer Types
struct PointerValue : public Value {
    std::shared_ptr<Value> pointeeType;  // The type being pointed to
    bool isConst;
    bool isVolatile;

    PointerValue(std::shared_ptr<Value> pointeeType, bool isConst = false, bool isVolatile = false)
        : pointeeType(std::move(pointeeType)), isConst(isConst), isVolatile(isVolatile) {
        type = Type::Pointer;
    }
};

// Reference Types
struct ReferenceValue : public Value {
    std::shared_ptr<Value> referentType;  // The type being referenced

    ReferenceValue(std::shared_ptr<Value> referentType)
        : referentType(std::move(referentType)) {
        type = Type::Reference;
    }
};

// Function Types
struct FunctionValue : public Value {
    Type returnType;
    std::vector<std::shared_ptr<Value>> paramTypes;
    bool isVarArg;

    FunctionValue(Type returnType, std::vector<std::shared_ptr<Value>> paramTypes, bool isVarArg = false)
        : returnType(returnType), paramTypes(std::move(paramTypes)), isVarArg(isVarArg) {
        type = Type::Function;
    }
};

// Aggregate Types (e.g., Struct, Enum, Array)
struct AggregateValue : public Value {
    std::vector<std::shared_ptr<Value>> elements;  // Elements of the aggregate
    std::vector<std::string> elementNames;         // Names of the elements
    uint64_t count;                               // The count of elements

    AggregateValue(std::vector<std::shared_ptr<Value>> elements, std::vector<std::string> elementNames)
        : elements(std::move(elements)), elementNames(std::move(elementNames)), count(elements.size()) {
        type = Type::Struct;  // Can be changed for Enum, Array, or Vector
    }
};

// Custom String and WideString Types
struct StringValue : public Value {
    StringValue() {
        type = Type::String;
    }
};

struct WideStringValue : public Value {
    WideStringValue() {
        type = Type::WideString;
    }
};

}

#endif