#pragma once
#ifndef VALUE_H
#define VALUE_H

#include <omniscript/Core.h>
#include <omniscript/omniscript_pch.h>
#include <omniscript/debuggingtools/console.h>

namespace Omniscript {

// Kind enum
enum class Kind {
    Invalid,

    // Primitive Types
    Primitive,
    Void,
    Nullptr, Null,
    Bool,
    Char,
    Int8, Int16, Int32, Int64, Int128, BigInt,
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

// Base class for all type representations
class Type {
public:
    Kind kind = Kind::Invalid;
    virtual ~Type() = default;

    // ----- Instance methods -----
    bool isPointer() const { return kind == Kind::Pointer; }
    bool isReference() const { return kind == Kind::Reference; }
    bool isFunction() const { return kind == Kind::Function; }
    bool isPrimitive() const { return kind == Kind::Primitive; }

    bool isNumericLiteral() const {
        return isInteger() || isFloat();
    }
    
    bool isInteger(int bitwidth = -1) const {
        // If no bitwidth is specified, just check if the kind is an integer
        if (bitwidth == -1) {
            return kind == Kind::Int8 || kind == Kind::Int16 || kind == Kind::Int32 ||
                   kind == Kind::Int64 || kind == Kind::Int128 ||
                   kind == Kind::UInt8 || kind == Kind::UInt16 ||
                   kind == Kind::UInt32 || kind == Kind::UInt64 || kind == Kind::UInt128;
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
            default:
                return false;  // Unsupported bitwidth
        }
    }

    bool is8BitInteger() const {
        return kind == Kind::Int8 || kind == Kind::UInt8;
    }

    bool is16BitInteger() const {
        return kind == Kind::Int16 || kind == Kind::UInt16;
    }

    bool is32BitInteger() const {
        return kind == Kind::Int32 || kind == Kind::UInt32;
    }

    bool is64BitInteger() const {
        return kind == Kind::Int64 || kind == Kind::UInt64;
    }

    bool isFloatingPoint() const {
        return kind == Kind::Half || kind == Kind::Float ||
               kind == Kind::Double || kind == Kind::FP128 ||
               kind == Kind::X86_FP80 || kind == Kind::PPC_FP128;
    }

    bool isFloat(int bitWidth = -1) const {
        if (bitWidth == -1) {
            return kind == Kind::Half || kind == Kind::Float ||
               kind == Kind::Double || kind == Kind::FP128 ||
               kind == Kind::X86_FP80 || kind == Kind::PPC_FP128;
        }
        
        // Check if the type matches the specified bit width for floats
        if (bitWidth == 32) {
            return kind == Kind::Float;
        } else if (bitWidth == 64) {
            return kind == Kind::Double;
        } else if (bitWidth == 128) {
            return kind == Kind::FP128;
        }
        // Add more checks if necessary for other float types like X86_FP80 or PPC_FP128
        return false;
    }


    virtual std::shared_ptr<Type> getReturnType() const { return nullptr; }

    std::string kindName() const {
        return std::to_string(static_cast<int>(kind)); // Replace with actual string conversion if needed
    }

    Kind getKind() const { return kind; }

    // Access underlying types
    virtual std::shared_ptr<Type> getPointeeType() const { return nullptr; }
    virtual std::shared_ptr<Type> getReferencedType() const { return nullptr; }
    virtual std::shared_ptr<Type> getElementType() const { return nullptr; }

    // ----- Static factory methods -----
    static std::shared_ptr<Type> createInvalid();
    static std::shared_ptr<Type> createPrimitiveType(Kind kind);
    static std::shared_ptr<Type> createPointerType(std::shared_ptr<Type> pointee, bool isConst = false, bool isVolatile = false);
    static std::shared_ptr<Type> createReferenceType(std::shared_ptr<Type> referent);
    static std::shared_ptr<Type> createFunctionType(Kind returnKind, std::vector<std::shared_ptr<Type>> params, bool isVarArg = false);
    static std::shared_ptr<Type> createStringType();
    static std::shared_ptr<Type> createWideStringType();
};

// --- Derived Types ---

class PrimitiveType : public Type {
public:
    template <typename T>
    static Kind get() {
        if constexpr (std::is_same_v<T, bool>) return Kind::Bool;
        if constexpr (std::is_same_v<T, char>) return Kind::Char;
        
        if constexpr (std::is_same_v<T, int8_t>) return Kind::Int8;
        if constexpr (std::is_same_v<T, int16_t>) return Kind::Int16;
        if constexpr (std::is_same_v<T, int32_t>) return Kind::Int32;
        if constexpr (std::is_same_v<T, int64_t>) return Kind::Int64;
        if constexpr (std::is_same_v<T, __int128>) return Kind::Int128;

        if constexpr (std::is_same_v<T, uint8_t>) return Kind::UInt8;
        if constexpr (std::is_same_v<T, uint16_t>) return Kind::UInt16;
        if constexpr (std::is_same_v<T, uint32_t>) return Kind::UInt32;
        if constexpr (std::is_same_v<T, uint64_t>) return Kind::UInt64;
        if constexpr (std::is_same_v<T, unsigned __int128>) return Kind::UInt128;

        if constexpr (std::is_same_v<T, float>) return Kind::Float;
        if constexpr (std::is_same_v<T, double>) return Kind::Double;
        if constexpr (std::is_same_v<T, long double>) return Kind::FP128;

        if constexpr (std::is_same_v<T, std::string>) return Kind::String;
        if constexpr (std::is_same_v<T, std::wstring>) return Kind::WideString;

        return Kind::Invalid;  // Default case if type isn't handled
    }

    Kind primitiveKind;

    explicit PrimitiveType(Kind kind) : primitiveKind(kind) { 
        kind = Kind::Primitive;
    }

    static std::shared_ptr<Type> create(Kind kind) {
        return std::make_shared<PrimitiveType>(kind);
    }
};
    

class PointerType : public Type {
public:
    std::shared_ptr<Type> pointeeType;
    bool isConst;
    bool isVolatile;

    PointerType(std::shared_ptr<Type> pointeeType, bool isConst, bool isVolatile)
        : pointeeType(std::move(pointeeType)), isConst(isConst), isVolatile(isVolatile) {
        kind = Kind::Pointer;
    }
};

class ReferenceType : public Type {
public:
    std::shared_ptr<Type> referentType;

    explicit ReferenceType(std::shared_ptr<Type> referentType)
        : referentType(std::move(referentType)) {
        kind = Kind::Reference;
    }
};

class FunctionType : public Type {
public:
    Kind returnKind;
    std::vector<std::shared_ptr<Type>> paramTypes;
    bool isVarArg;

    FunctionType(Kind returnKind, std::vector<std::shared_ptr<Type>> paramTypes, bool isVarArg)
        : returnKind(returnKind), paramTypes(std::move(paramTypes)), isVarArg(isVarArg) {
        kind = Kind::Function;
    }

    std::shared_ptr<Type> getReturnType() const override {
        return std::make_shared<PrimitiveType>(returnKind); // You may replace this with actual type resolution
    }
};


inline std::shared_ptr<Type> resolveType(std::vector<std::string>& dataTypes) {
    if (dataTypes.empty()) {
        return Type::createPrimitiveType(Kind::Int32); // Default to i32
    }

    size_t index = 0;
    int totalPointerDepth = 0;
    int totalReferenceDepth = 0;
    bool isArray = false;
    uint64_t arraySize = 0;
    std::string baseType;

    // Array detection
    if (dataTypes[index] == "[") {
        if (index + 2 < dataTypes.size() && std::all_of(dataTypes[index + 1].begin(), dataTypes[index + 1].end(), ::isdigit)) {
            arraySize = std::stoull(dataTypes[index + 1]);
            index += 3; // Skip "[", "size", "]"
        } else {
            arraySize = 0; // Dynamic array
            index += 3;
        }
        isArray = true;
    }

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

    if (index >= dataTypes.size()) {
        std::cerr << "[ERROR] No base type found after modifiers!" << std::endl;
        return nullptr;
    }

    baseType = dataTypes[index++];

    // Post-type pointer depth
    while (index < dataTypes.size() && dataTypes[index] == "*") {
        totalPointerDepth++;
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
        // else if (baseType == "int16" || baseType == "i16") type = Type::createPrimitiveType(Kind::Int256);
        // else if (baseType == "int16" || baseType == "i16") type = Type::createPrimitiveType(Kind::Int512);
        // else if (baseType == "int16" || baseType == "i16") type = Type::createPrimitiveType(Kind::Int1024;
        else if (baseType == "bool") type = Type::createPrimitiveType(Kind::Bool);
        else if (baseType == "char") type = Type::createPrimitiveType(Kind::Char);
        else if (baseType == "void") type = Type::createPrimitiveType(Kind::Void);
        else if (baseType == "half" || baseType == "f16") type = Type::createPrimitiveType(Kind::Half);
        else if (baseType == "float" || baseType == "f32") type = Type::createPrimitiveType(Kind::Float);
        else if (baseType == "double" || baseType == "f64") type = Type::createPrimitiveType(Kind::Double);
        else if (baseType == "fp128") type = Type::createPrimitiveType(Kind::FP128);
        else if (baseType == "x86_fp80") type = Type::createPrimitiveType(Kind::X86_FP80);
        else if (baseType == "ppc_fp128") type = Type::createPrimitiveType(Kind::PPC_FP128);
        else if (baseType == "string" || baseType == "str" || baseType == "utf8") {
            type = Type::createPrimitiveType(Kind::Char);
            totalPointerDepth++;
        }
        else if (baseType == "utf16") {
            type = Type::createPrimitiveType(Kind::UInt16);
            totalPointerDepth++;
        }
        else if (baseType == "utf32") {
            type = Type::createPrimitiveType(Kind::UInt32);
            totalPointerDepth++;
        }
        else {
            std::cerr << "[ERROR] Unknown type: " << baseType << std::endl;
            return nullptr;
        }
    }

    // Wrap in array if needed
    if (isArray) {
        // type = std::make_shared<ArrayType>(type, arraySize);
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


// ====================================== Values ====================================== //
struct Value {
    std::shared_ptr<Type> type = Type::createInvalid();  // Holds a full Type object now

    virtual ~Value() = default;  // Polymorphic base
};

template <typename T>
std::shared_ptr<T> make_value(auto&&... args) {
    return std::make_shared<T>(std::forward<decltype(args)>(args)...);
}

// Primitive Types (e.g., Int8, Bool)
struct PrimitiveValue : public Value {
    explicit PrimitiveValue(Kind primitiveKind) {
        type = Type::createPrimitiveType(primitiveKind);
    }
};

// Base class for all numeric values
template <typename T>
class NumericValue : public PrimitiveValue {
public:
    NumericValue(T value)
        : PrimitiveValue(PrimitiveType::get<T>()), value(value) {}  // Auto-detect type

    virtual ~NumericValue() = default;

    T getValue() const { return value; }

protected:
    T value;
};


// Specialized IntegerValue template class inheriting from NumericValue
template <typename T>
class Integer : public NumericValue<T> {
public:
    Integer(T value)
        : NumericValue<T>(value) {}  // Specify type

    ~Integer() override = default;
};

// Specialized FloatValue template class inheriting from NumericValue
template <typename T>
class Float : public NumericValue<T> {
public:
    Float(T value)
        : NumericValue<T>(value) {}  // Specify type

    ~Float() override = default;
};

// Specialized BigIntValue for handling large integers
class BigIntValue : public NumericValue<std::string> {
public:
    explicit BigIntValue(const std::string& value)
        : NumericValue<std::string>(value) {}  // Explicitly pass type

    ~BigIntValue() override = default;
};


// Specialized BigIntValue for handling large integers
class BigInt : public NumericValue<std::string> {
public:
    explicit BigInt(const std::string& value)
        : NumericValue<std::string>(value) {}  // Explicitly pass type

    ~BigInt() override = default;
};



// Pointer Types
struct PointerValue : public Value {
    std::shared_ptr<Value> pointee;
    bool isConst;
    bool isVolatile;

    PointerValue(std::shared_ptr<Value> pointee, bool isConst = false, bool isVolatile = false)
        : pointee(std::move(pointee)), isConst(isConst), isVolatile(isVolatile) {
        type = Type::createPointerType(this->pointee->type, isConst, isVolatile);
    }
};


// Reference Types
struct ReferenceValue : public Value {
    std::shared_ptr<Value> referent;

    explicit ReferenceValue(std::shared_ptr<Value> referent)
        : referent(std::move(referent)) {
        type = Type::createReferenceType(this->referent->type);
    }
};


// Function Types
struct FunctionValue : public Value {
    std::vector<std::shared_ptr<Value>> args;
    std::shared_ptr<Value> returnValue;
    bool isVarArg;

    FunctionValue(std::shared_ptr<Value> returnValue, std::vector<std::shared_ptr<Value>> args, bool isVarArg = false)
        : returnValue(std::move(returnValue)), args(std::move(args)), isVarArg(isVarArg) {
        std::vector<std::shared_ptr<Type>> paramTypes;
        for (auto& arg : this->args)
            paramTypes.push_back(arg->type);

        type = Type::createFunctionType(returnValue->type->getKind(), std::move(paramTypes), isVarArg);
    }
};


// Aggregate Types (e.g., Struct, Enum, Array)
struct AggregateValue : public Value {
    std::vector<std::shared_ptr<Value>> elements;
    std::vector<std::string> elementNames;
    uint64_t count;

    AggregateValue(std::vector<std::shared_ptr<Value>> elements, std::vector<std::string> elementNames)
        : elements(std::move(elements)), elementNames(std::move(elementNames)), count(elements.size()) {
        std::vector<std::shared_ptr<Type>> elementTypes;
        for (auto& e : this->elements)
            elementTypes.push_back(e->type);

        // type = Type::createStructType(elementTypes, this->elementNames);
    }
};


// Custom String and WideString Types
struct StringValue : public Value {
    StringValue() {
        type = Type::createStringType();
    }
};

struct WideStringValue : public Value {
    WideStringValue() {
        type = Type::createWideStringType();
    }
};


}

#endif