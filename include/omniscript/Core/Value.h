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
    Int8, Int16, Int32, Int64, Int128, Int256, Int512, Int1024, BigInt,
    UInt8, UInt16, UInt32, UInt64, UInt128, UInt256, UInt512, UInt1024,
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
    bool isPointer() const { return kind == Kind::Pointer || kind == Kind::Nullptr; }
    bool isNull() const { return kind == Kind::Null || kind == Kind::Nullptr; }
    bool isNullPointer() const { return kind == Kind::Nullptr; }
    bool isReference() const { return kind == Kind::Reference; }
    bool isFunction() const { return kind == Kind::Function; }
    bool isPrimitive() const { return kind == Kind::Primitive; }
    bool isArray() const { return kind == Kind::Array; }

    bool isNumericLiteral() const {
        return isInteger() || isFloat();
    }
    
    bool isInteger(int bitwidth = -1) const {
        // If no bitwidth is specified, just check if the kind is an integer
        if (bitwidth == -1) {
            return kind == Kind::Int8 || kind == Kind::Int16 || kind == Kind::Int32 ||
                   kind == Kind::Int64 || kind == Kind::Int128 || kind == Kind::Int256 ||
                   kind == Kind::Int512 || kind == Kind::Int1024 ||
                   kind == Kind::UInt8 || kind == Kind::UInt16 ||
                   kind == Kind::UInt32 || kind == Kind::UInt64 || kind == Kind::UInt128 ||
                   kind == Kind::UInt256 || kind == Kind::UInt512 || kind == Kind::UInt1024 ||
                   kind == Kind::BigInt;
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

    std::string kindName() const;
    Kind getKind() const { return kind; }

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
    static std::shared_ptr<Type> createPrimitiveType(Kind kind);
    static std::shared_ptr<Type> createNullPointerType();
    static std::shared_ptr<Type> createPointerType(std::shared_ptr<Type> pointee);
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
        // if constexpr (std::is_same_v<T, __int256>) return Kind::Int256;
        // if constexpr (std::is_same_v<T, __int512>) return Kind::Int512;
        // if constexpr (std::is_same_v<T, __int1024>) return Kind::Int1024;

        if constexpr (std::is_same_v<T, uint8_t>) return Kind::UInt8;
        if constexpr (std::is_same_v<T, uint16_t>) return Kind::UInt16;
        if constexpr (std::is_same_v<T, uint32_t>) return Kind::UInt32;
        if constexpr (std::is_same_v<T, uint64_t>) return Kind::UInt64;
        if constexpr (std::is_same_v<T, unsigned __int128>) return Kind::UInt128;

        if constexpr (std::is_same_v<T, float>) return Kind::Float;
        if constexpr (std::is_same_v<T, double>) return Kind::Double;
        if constexpr (std::is_same_v<T, __float128>) return Kind::FP128;
        if constexpr (std::is_same_v<T, long double>) return Kind::FP128;

        if constexpr (std::is_same_v<T, std::string>) return Kind::String;
        if constexpr (std::is_same_v<T, std::wstring>) return Kind::WideString;

        return Kind::Invalid;  // Default case if type isn't handled
    }

    Kind primitiveKind;

    explicit PrimitiveType(Kind kind_) : primitiveKind(kind_) { 
        kind = kind_;
    }

    static std::shared_ptr<Type> create(Kind kind) {
        return std::make_shared<PrimitiveType>(kind);
    }
};
    

class PointerType : public Type {
public:
    std::shared_ptr<Type> pointee;

    // Constructor
    PointerType(std::shared_ptr<Type> pointeeType)
        : pointee(pointeeType) {
            kind = Kind::Pointer;
        }

    // Get the pointee type (directly)
    std::shared_ptr<Type> getPointeeType() const {
        return pointee;
    }

    // Method to get the pointer depth recursively
    int getPointerDepth() const override {
        int depth = 0;
        auto currentPointee = pointee;

        while (currentPointee->isPointer()) {
            depth++;
            currentPointee = std::dynamic_pointer_cast<PointerType>(currentPointee)->getPointeeType();
        }

        return depth;
    }

    // Method to get the base pointee type (the deepest pointee type)
    std::shared_ptr<Type> getBasePointeeType() const override {
        auto currentPointee = pointee;

        while (currentPointee->isPointer()) {
            currentPointee = std::dynamic_pointer_cast<PointerType>(currentPointee)->getPointeeType();
        }

        return currentPointee;
    }

    std::string pointerDescription() const override {
        std::vector<std::string> parts;
        std::shared_ptr<Type> current = pointee;  // start from the first pointee
    
        // Walk through all pointer levels
        while (current->isPointer()) {
            parts.push_back("pointer");
            current = std::dynamic_pointer_cast<PointerType>(current)->getPointeeType();
        }
    
        // Add the base type at the end
        parts.push_back(current->kindName());
    
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
};

class NullPointerType : public Type {
public:
    NullPointerType() {  // You can use any default 'unknown' type
        kind = Kind::Nullptr;
    }

    std::string pointerDescription() const override {
        return "nullptr";
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

        parts.push_back(current->kindName());
        std::reverse(parts.begin(), parts.end());

        std::string desc;
        for (const auto& part : parts) {
            if (!desc.empty()) desc += " ";
            desc += part;
        }

        return desc;
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
            type = Type::createPrimitiveType(Kind::UInt16);
            totalPointerDepth++;
        }
        else if (baseType == "utf32") {
            type = Type::createPrimitiveType(Kind::UInt32);
            totalPointerDepth++;
        }
    
        // Fallback
        else {
            console.error("Type: '" + baseType + "' is supported in omniscript");
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
public:
    virtual ~Value() = default;  // Polymorphic base
    
    std::shared_ptr<Type> getType() const { return type; }
    virtual std::string toString() const { return "Value"; }

    std::shared_ptr<Type> type = Type::createInvalid();  // Holds a full Type object now

};

template <typename T>
std::shared_ptr<T> make_value(auto&&... args) {
    return std::make_shared<T>(std::forward<decltype(args)>(args)...);
}

// Template class for Primitive Types (e.g., Int8, Bool)
template <typename T>
struct Primitive : public Value {
    explicit Primitive(T value) : value(value) {
        type = Type::createPrimitiveType(PrimitiveType::get<T>());
    }

    std::string toString() const override {
        if constexpr (std::is_same_v<T, std::string>) {
            return "Primitive: " + value;
        } else if constexpr (std::is_same_v<T, bool>) {
            return std::string("Primitive: ") + (value ? "true" : "false");
        } else if constexpr (std::is_same_v<T, __float128>) {
            // Custom handling for __float128 type
            char buffer[128];  // Allocate enough space for the representation
            snprintf(buffer, sizeof(buffer), "%.*Lf", 36, (long double)value); // Print with long double format
            return std::string("Primitive: ") + buffer;
        } else if constexpr (std::is_same_v<T, _Float16>) {
            // Custom handling for _Float16 type
            return "Primitive: " + std::to_string(static_cast<float>(value)); // Convert _Float16 to float
        } else {
            return "Primitive: " + std::to_string(value);
        }
    }

    T getValue() const { return value; }

protected:
    T value;
};


// Base class for all numeric values
template <typename T>
class NumericValue : public Primitive<T> {
public:
    NumericValue(T value)
        : Primitive<T>(value) {}

    virtual ~NumericValue() = default;
};

// Specialized Integer template class inheriting from NumericValue
template <typename T>
class Integer : public NumericValue<T> {
public:
    Integer(T value)
        : NumericValue<T>(value) {}  // Specify type

    ~Integer() override = default;
};

// Specialized Float template class inheriting from NumericValue
template <typename T>
class Float : public NumericValue<T> {
public:
    Float(T value)
        : NumericValue<T>(value) {}  // Specify type

    ~Float() override = default;
};

// Specialized BigInt class inheriting from NumericValue for handling large integers
class BigInt : public NumericValue<std::string> {
public:
    explicit BigInt(std::string value, unsigned bitWidth)
        : NumericValue<std::string>(value), bitWidth(bitWidth) {}  // Pass value to base class constructor

    ~BigInt() override = default;

    unsigned getBitWidth() const { return bitWidth; }

private:
    unsigned bitWidth;
};


// Pointer Types
struct PointerValue : public Value {
    std::shared_ptr<Value> pointee;
    bool isConst;
    bool isVolatile;

    PointerValue(std::shared_ptr<Value> pointee, bool isConst = false, bool isVolatile = false)
        : pointee(std::move(pointee)), isConst(isConst), isVolatile(isVolatile) {
        type = Type::createPointerType(this->pointee->type);
    }

    std::string toString() const override { return "Pointer"; } 
};

struct NullPointerValue : public Value {
    NullPointerValue() {
        type = Type::createNullPointerType();
    }

    std::string toString() const override { return "NullPointer"; } 
};

struct AddressOfValue : public Value {
    std::shared_ptr<Value> referent;  // The variable whose address is being stored
    std::string variableName;

    explicit AddressOfValue(const std::string& variableName, std::shared_ptr<Value> referent = nullptr)
        : variableName(variableName), referent(std::move(referent)) {
        // We assume AddressOf value is of type Pointer to the referent's type
        type = Type::createPointerType(this->referent->type); 
    }

    // Return a string representation of the AddressOf value
    std::string toString() const override {
        return "AddressOf(" + referent->toString() + ")";
    }
};

// Reference Types
struct ReferenceValue : public Value {
    std::string referentName;
    std::shared_ptr<Value>* referentPtr = nullptr;  // Pointer to a reference
    std::shared_ptr<Value> referent = nullptr;     // Pointer to a value (for regular pointers)

    // Constructor for pointers (original)
    explicit ReferenceValue(const std::string& referentName, std::shared_ptr<Value> referent = nullptr)
        : referentName(referentName), referent(referent) {
        type = Type::createReferenceType(this->referent->type);
    }

    // Constructor for references (using a reference pointer)
    explicit ReferenceValue(const std::string& name, std::shared_ptr<Value>* referentPtr)
        : referentName(name), referentPtr(referentPtr) {
        if (referentPtr && *referentPtr) {
            type = Type::createReferenceType((*referentPtr)->type);
        }
    }

    // Getter for value, works for both pointers and references
    std::shared_ptr<Value> getValue() const {
        if (referent) {
            return referent;  // Regular pointer, just return the referent
        }
        return (referentPtr && *referentPtr) ? *referentPtr : nullptr;  // Dereference reference pointer
    }

    // String representation for both pointers and references
    std::string toString() const override {
        if (referent) {
            return "Pointer to(" + referent->toString() + ")";
        }
        return "Reference to(" + (referentPtr && *referentPtr ? (*referentPtr)->toString() : "null") + ")";
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

    std::string toString() const override { return "Function"; } 
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

    std::string toString() const override { return "Aggregate"; } 
};


// Custom String and WideString Types
struct StringValue : public Value {
    StringValue() {
        type = Type::createStringType();
    }
    std::string toString() const override { return "String"; } 
};

struct WideStringValue : public Value {
    WideStringValue() {
        type = Type::createWideStringType();
    }
    std::string toString() const override { return "WideString"; } 
};

struct VariableAssignment : public Value {
    std::string variableName;
    std::shared_ptr<Value> assignedValue;

    VariableAssignment(std::string name, std::shared_ptr<Value> value)
        : variableName(std::move(name)), assignedValue(std::move(value)) {
        type = assignedValue->type;  // Same type as the assigned value
    }

    std::shared_ptr<Value> getValue() const { return assignedValue; }
    std::string toString() const override {
        return "Assign: " + variableName + " = " + assignedValue->toString();
    }
};

struct VariableAccess : public Value {
    std::string variableName;

    explicit VariableAccess(std::string name)
        : variableName(std::move(name)) {}

    std::string toString() const override {
        return "Variable: " + variableName;
    }
};


}

#endif