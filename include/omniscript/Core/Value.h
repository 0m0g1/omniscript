#pragma once
#ifndef VALUE_H
#define VALUE_H

#include <omniscript/engine/tokens.h>
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
    Utf8,
    Utf16,
    Utf32,
    
    //ArrayTypes
    FixedArray,       // e.g., [4]i32
    DynamicArray,     // e.g., [i32]
    HeterogeneousArray, // e.g., []
    
    //Other Types
    Call,
    Unresolved,
    Generic,
    Block,
};

// Base class for all type representations
class Type {
public:
    Kind kind = Kind::Invalid;
    std::shared_ptr<Type> returnType;
    std::shared_ptr<Type> elementType;
    size_t fixedSize = 0;
    virtual ~Type() = default;

    // ----- Instance methods -----
    bool isChar() const { return kind == Kind::Char; }
    bool isPointer() const { return kind == Kind::Pointer || kind == Kind::Nullptr; }
    bool isNullType() const { return kind == Kind::Null || kind == Kind::Nullptr; }
    bool isNull() const { return kind == Kind::Null; }
    bool isNullPointer() const { return kind == Kind::Nullptr; }
    bool isReference() const { return kind == Kind::Reference; }
    bool isFunction() const { return kind == Kind::Function; }
    bool isPrimitive() const { return kind == Kind::Primitive; }
    bool isArray() const { return kind == Kind::FixedArray || kind == Kind::DynamicArray || kind == Kind::HeterogeneousArray; }
    bool isFixedArray() const { return kind == Kind::FixedArray; }
    bool isDynamicArray() const { return kind == Kind::DynamicArray; }
    bool isHeterogeneousArray() const { return kind == Kind::HeterogeneousArray; }
    bool isUnresolved() const { return kind == Kind::Unresolved || kind == Kind::Generic; }
    bool isGeneric() const { return kind == Kind::Generic || kind == Kind::Unresolved; }
    bool isBlock() const { return kind == Kind::Block; }
    bool isBool() const { return kind == Kind::Bool; }

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

    virtual std::string getName() const { return "type"; };

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
    static std::shared_ptr<Type> createNullType();
    static std::shared_ptr<Type> createNullPointerType();
    static std::shared_ptr<Type> createMetaType();
    static std::shared_ptr<Type> createPointerType(std::shared_ptr<Type> pointee);
    static std::shared_ptr<Type> createReferenceType(std::shared_ptr<Type> referent);
    static std::shared_ptr<Type> createFunctionType(std::shared_ptr<Type> returnType, bool isVarArg = false);
    static std::shared_ptr<Type> createStringType(Kind stringKind = Kind::String);
    static std::shared_ptr<Type> createFixedArrayType(std::shared_ptr<Type> elementType, size_t size);
    static std::shared_ptr<Type> createDynamicArrayType(std::shared_ptr<Type> elementType);
    static std::shared_ptr<Type> createHeterogeneousArrayType();
    static std::shared_ptr<Type> createGenericType(const std::string& typeName);

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

        if constexpr (std::is_same_v<T, std::string>) return Kind::Utf8;
        if constexpr (std::is_same_v<T, std::u16string>) return Kind::Utf16;
        if constexpr (std::is_same_v<T, std::u32string>) return Kind::Utf32;

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
    
    std::shared_ptr<Type> clone() const override {
        return std::make_shared<PointerType>(pointee->clone());
    }    
};

class NullType : public Type {
public:
    NullType() {  // You can use any default 'unknown' type
        kind = Kind::Null;
    }

    std::string pointerDescription() const override {
        return "null";
    }

    std::shared_ptr<Type> clone() const override {
        return std::make_shared<NullType>();
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

    std::shared_ptr<Type> clone() const override {
        return std::make_shared<NullPointerType>();
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

    std::shared_ptr<Type> clone() const override {
        return std::make_shared<ReferenceType>(referentType->clone());
    }    
};


class FunctionType : public Type {
public:
    bool isVarArg;

    FunctionType(std::shared_ptr<Type> returnType, bool isVarArg)
        : isVarArg(isVarArg) {
        this->returnType = std::move(returnType);
        kind = Kind::Function;
    }

    std::shared_ptr<Type> getReturnType() const override {
        return returnType; // You may replace this with actual type resolution
    }

    std::shared_ptr<Type> clone() const override {
        std::vector<std::shared_ptr<Type>> clonedParams;
        return std::make_shared<FunctionType>(returnType->clone(), isVarArg);
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


// ====================================== Values ====================================== //
struct Expression {
public:
    virtual ~Expression() = default;  // Polymorphic base
    
    virtual std::shared_ptr<Expression> clone() const { return nullptr; }
    std::shared_ptr<Type> getType() const { return type; }
    virtual std::string toString() const { return "Expression"; }

    std::string name;
    std::shared_ptr<Type> type = Type::createInvalid();  // Holds a full Type object now
};

template <typename T>
std::shared_ptr<T> make_expression(auto&&... args) {
    return std::make_shared<T>(std::forward<decltype(args)>(args)...);
}

struct TypeExpression : public Expression {
    std::string name;
    std::shared_ptr<Type> actualType;  // This holds the real type being wrapped

    // Constructor from existing Type
    explicit TypeExpression(const std::string& typeName, std::shared_ptr<Type> type)
        : name(typeName), actualType(std::move(type)) {
        this->type = Type::createMetaType();  // Meta-type for this Expression
    }

    // Constructor from Kind (for primitive types)
    explicit TypeExpression(Kind kind)
        : actualType(Type::createPrimitiveType(kind)) {
        this->type = Type::createMetaType();
    }

    std::string toString() const override {
        return actualType ? actualType->getName() : "nulltype";
    }

    std::shared_ptr<Type> getTypeExpression() const {
        return actualType;
    }

    bool isSameTypeAs(const std::shared_ptr<TypeExpression>& other) const {
        if (!actualType || !other->actualType) return false;
        return actualType->getKind() == other->actualType->getKind();
    }

    // TypeExpression
    std::shared_ptr<Expression> clone() const override {
        return std::make_shared<TypeExpression>(name, actualType ? actualType->clone() : nullptr);
    }
};

// Template class for Primitive Types (e.g., Int8, Bool)
template <typename T>
struct Primitive : public Expression {
    explicit Primitive(T value) : value(value) {
        type = Type::createPrimitiveType(PrimitiveType::get<T>());
    }

    std::string toString() const override {
        if constexpr (std::is_same_v<T, std::string>) {
            if (PrimitiveType::get<T>() != Kind::Utf8)
                return "Primitive: " + value;
            else
                return "String: \"" + this->getValue() + "\"";
        } else if constexpr (std::is_same_v<T, std::u16string>) {
            return "UTF-16 String";
        } else if constexpr (std::is_same_v<T, std::u32string>) {
            return "UTF-32 String";
        } else if constexpr (std::is_same_v<T, bool>) {
            return std::string("Primitive: ") + (value ? "true" : "false");
        } else if constexpr (std::is_same_v<T, __float128>) {
            char buffer[128];
            snprintf(buffer, sizeof(buffer), "%.*Lf", 36, (long double)value);
            return std::string("Primitive: ") + buffer;
        } else if constexpr (std::is_same_v<T, _Float16>) {
            return "Primitive: (Float16 not yet printable)";
        } else {
            return "Primitive: " + std::to_string(value); // works for int, float, etc.
        }
    }

    T getValue() const { return value; }

    // Primitive (template)
    std::shared_ptr<Expression> clone() const override {
        return std::make_shared<Primitive<T>>(value);
    }

protected:
    T value;
};


// Base class for all numeric values
template <typename T>
class NumericExpression : public Primitive<T> {
public:
    NumericExpression(T value)
        : Primitive<T>(value) {}

    virtual ~NumericExpression() = default;
    // NumericExpression (template)
    std::shared_ptr<Expression> clone() const override {
        return std::make_shared<NumericExpression<T>>(this->value);
    }
};

// Specialized Integer template class inheriting from NumericExpression
template <typename T>
class Integer : public NumericExpression<T> {
public:
    Integer(T value)
        : NumericExpression<T>(value) {}  // Specify type

    ~Integer() override = default;
    // Integer (template)
    std::shared_ptr<Expression> clone() const override {
        return std::make_shared<Integer<T>>(this->value);
    }
};

// Specialized Float template class inheriting from NumericExpression
template <typename T>
class Float : public NumericExpression<T> {
public:
    Float(T value)
        : NumericExpression<T>(value) {}  // Specify type

    ~Float() override = default;
        // Float (template)
    std::shared_ptr<Expression> clone() const override {
        return std::make_shared<Float<T>>(this->value);
    }
};

// Specialized BigInt class inheriting from NumericExpression for handling large integers
class BigInt : public NumericExpression<std::string> {
public:
    explicit BigInt(std::string value, unsigned bitWidth)
        : NumericExpression<std::string>(value), bitWidth(bitWidth) {}  // Pass value to base class constructor

    ~BigInt() override = default;

    unsigned getBitWidth() const { return bitWidth; }

    // BigInt
    std::shared_ptr<Expression> clone() const override {
        return std::make_shared<BigInt>(this->value, bitWidth);
    }

private:
    unsigned bitWidth;
};


// Pointer Types
struct PointerExpression : public Expression {
    std::shared_ptr<Expression> pointee;
    bool isConst;
    bool isVolatile;

    PointerExpression(std::shared_ptr<Expression> pointee, bool isConst = false, bool isVolatile = false)
        : pointee(std::move(pointee)), isConst(isConst), isVolatile(isVolatile) {
        type = Type::createPointerType(this->pointee->type);
    }

    std::string toString() const override { return "Pointer"; } 

    // PointerExpression
    std::shared_ptr<Expression> clone() const override {
        return std::make_shared<PointerExpression>(
            pointee ? pointee->clone() : nullptr,
            isConst,
            isVolatile
        );
    }
};

struct NullExpression : public Expression {
    NullExpression() {
        type = Type::createNullType();
    }

    std::string toString() const override { return "NullPointer"; }
    // NullExpression
    std::shared_ptr<Expression> clone() const override {
        return std::make_shared<NullExpression>();
    }
};

struct NullPointerExpression : public Expression {
    NullPointerExpression() {
        type = Type::createNullPointerType();
    }

    std::string toString() const override { return "NullPointer"; } 
    // NullPointerExpression
    std::shared_ptr<Expression> clone() const override {
        return std::make_shared<NullPointerExpression>();
    }
};

struct AddressOfExpression : public Expression {
    std::shared_ptr<Expression> referent;  // The variable whose address is being stored
    std::string variableName;

    explicit AddressOfExpression(const std::string& variableName, std::shared_ptr<Expression> referent = nullptr)
        : variableName(variableName), referent(std::move(referent)) {
        // We assume AddressOf value is of type Pointer to the referent's type
        type = Type::createPointerType(this->referent->type); 
    }

    // Return a string representation of the AddressOf value
    std::string toString() const override {
        return "AddressOf(" + referent->toString() + ")";
    }
    // AddressOfExpression
    std::shared_ptr<Expression> clone() const override {
        return std::make_shared<AddressOfExpression>(
            variableName,
            referent ? referent->clone() : nullptr
        );
    }
};

// Reference Types
struct ReferenceExpression : public Expression {
    std::string referentName;
    std::shared_ptr<Expression>* referentPtr = nullptr;  // Pointer to a reference
    std::shared_ptr<Expression> referent = nullptr;     // Pointer to a value (for regular pointers)

    // Constructor for pointers (original)
    explicit ReferenceExpression(const std::string& referentName, std::shared_ptr<Expression> referent = nullptr)
        : referentName(referentName), referent(referent) {
        type = Type::createReferenceType(this->referent->type);
    }

    // Constructor for references (using a reference pointer)
    explicit ReferenceExpression(const std::string& name, std::shared_ptr<Expression>* referentPtr)
        : referentName(name), referentPtr(referentPtr) {
        if (referentPtr && *referentPtr) {
            type = Type::createReferenceType((*referentPtr)->type);
        }
    }

    // Getter for value, works for both pointers and references
    std::shared_ptr<Expression> getValue() const {
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

    // ReferenceExpression
    std::shared_ptr<Expression> clone() const override {
        if (referent) {
            return std::make_shared<ReferenceExpression>(
                referentName,
                referent->clone()
            );
        }
        return std::make_shared<ReferenceExpression>(
            referentName,
            referentPtr ? new std::shared_ptr<Expression>(*referentPtr) : nullptr
        );
    }
};


// Function Types
struct ReturnExpression : public Expression {
    std::shared_ptr<Expression> value;
    ReturnExpression(std::shared_ptr<Expression> value, std::shared_ptr<Type> returnType) : value(std::move(value)) {
        type = returnType;
    }
    // ReturnExpression
    std::shared_ptr<Expression> clone() const override {
        return std::make_shared<ReturnExpression>(
            value ? value->clone() : nullptr,
            type ? type->clone() : nullptr
        );
    }
};

struct BinaryExpression : public Expression {
    std::shared_ptr<Expression> left;
    std::shared_ptr<Expression> right;
    TokenTypes op;

    BinaryExpression(std::shared_ptr<Expression> lhs, TokenTypes op, std::shared_ptr<Expression> rhs, std::shared_ptr<Type> resultType)
        : left(std::move(lhs)), right(std::move(rhs)), op(std::move(op)) {
        this->type = resultType;
    }

    std::string toString() const override {
        // return "(" + left->toString() + " " + op + " " + right->toString() + ")";
        return "(" + left->toString() + " op " + right->toString() + ")";
        // return "(bin expr)";
    }
    // BinaryExpression
    std::shared_ptr<Expression> clone() const override {
        return std::make_shared<BinaryExpression>(
            left ? left->clone() : nullptr,
            op,
            right ? right->clone() : nullptr,
            type ? type->clone() : nullptr
        );
    }
};

struct TernaryExpression : public Expression {
    std::shared_ptr<Expression> condition;
    std::shared_ptr<Expression> truthy;
    std::shared_ptr<Expression> falsey;

    TernaryExpression(std::shared_ptr<Expression> cond,
                           std::shared_ptr<Expression> ifTrue,
                           std::shared_ptr<Expression> ifFalse,
                           std::shared_ptr<Type> resultType)
        : condition(std::move(cond)), truthy(std::move(ifTrue)), falsey(std::move(ifFalse)) {
        this->type = resultType;
    }

    std::string toString() const override {
        return "(" + condition->toString() + " ? " + truthy->toString() + " : " + falsey->toString() + ")";
    }

    // TernaryExpression
    std::shared_ptr<Expression> clone() const override {
        return std::make_shared<TernaryExpression>(
            condition ? condition->clone() : nullptr,
            truthy ? truthy->clone() : nullptr,
            falsey ? falsey->clone() : nullptr,
            type ? type->clone() : nullptr
        );
    }
};


struct UnaryExpression : public Expression {
    TokenTypes op;
    std::shared_ptr<Expression> operand;
    bool position;

    UnaryExpression(TokenTypes op,
                         std::shared_ptr<Expression> operand,
                         std::shared_ptr<Type> resultType,
                         bool position)
        : op(op), operand(std::move(operand)), position(position) {
        this->type = resultType;
    }

    std::string toString() const override {
        TokenTypes opStr = op;
        return "(unaryexpr)";
        // return (position)
        //     ? (opStr + operand->toString())
        //     : (operand->toString() + opStr);
    }

    // UnaryExpression
    std::shared_ptr<Expression> clone() const override {
        return std::make_shared<UnaryExpression>(
            op,
            operand ? operand->clone() : nullptr,
            type ? type->clone() : nullptr,
            position
        );
    }
};


//params & args
struct FunctionInputExpression : public Expression {
    bool isConstant = false;
    std::shared_ptr<Expression> value;

    FunctionInputExpression(const std::string& name, std::shared_ptr<Type> type = nullptr, std::shared_ptr<Expression> value = nullptr, bool isConst = false) :
    value(std::move(value)), isConstant(isConst) {
        this->name = name;
        this->type = std::move(type);
    }
    
    std::string toString() const override { return "(FunctionInput: " + name + ", value: " + value->toString() + ")"; } 

    std::shared_ptr<Expression> clone() const override {
        return std::make_shared<FunctionInputExpression>(
            name,
            type ? type->clone() : nullptr,
            value ? value->clone() : nullptr,
            isConstant
        );
    }
};

struct CallExpression : public Expression {
    std::string calleeName;
    std::vector<std::shared_ptr<Expression>> args;

    CallExpression(const std::string& calleeName, const std::vector<std::shared_ptr<Expression>>& args = {}, std::shared_ptr<Type> returnType = nullptr)
    : calleeName(calleeName), args(std::move(args)) {
        type = std::move(returnType);
    }

    // CallExpression
    std::shared_ptr<Expression> clone() const override {
        std::vector<std::shared_ptr<Expression>> clonedArgs;
        for (const auto& arg : args) {
            clonedArgs.push_back(arg ? arg->clone() : nullptr);
        }
        return std::make_shared<CallExpression>(
            calleeName,
            clonedArgs,
            type ? type->clone() : nullptr
        );
    }
};

struct Callable : public Expression {
    std::vector<std::shared_ptr<Expression>> parameters;
    bool isVarArg;

    Callable(const std::string& name, 
            std::vector<std::shared_ptr<Expression>> params = {},
            bool isVarArg = false)
        : parameters(std::move(params)), 
          isVarArg(isVarArg) {
        this->name = name;
    }

    // Helper method to clone parameters
    std::vector<std::shared_ptr<FunctionInputExpression>> cloneParameters() const {
        std::vector<std::shared_ptr<FunctionInputExpression>> clonedParams;
        for (const auto& parameter : parameters) {
            auto param = std::dynamic_pointer_cast<FunctionInputExpression>(parameter);
            clonedParams.push_back(std::make_shared<FunctionInputExpression>(
                param->name,
                param->type,
                param->value ? param->value->clone() : nullptr,
                param->isConstant
            ));
        }
        return clonedParams;
    }

    std::string toString() const override {
        std::string paramsStr;
        for (const auto& param : parameters) {
            if (!paramsStr.empty()) paramsStr += ", ";
            paramsStr += param->toString();
        }
        return "Callable: " + name + "(" + paramsStr + ")";
    }

    // Callable
    std::shared_ptr<Expression> clone() const override {
        std::vector<std::shared_ptr<Expression>> clonedParams;
        for (const auto& param : parameters) {
            clonedParams.push_back(param ? param->clone() : nullptr);
        }
        return std::make_shared<Callable>(name, clonedParams, isVarArg);
    }
};

// FunctionExpression inherits from Callable
struct FunctionExpression : public Callable {
    std::vector<std::shared_ptr<Expression>> body;
    std::shared_ptr<Type> returnType;

    FunctionExpression(const std::string& name, 
                      std::shared_ptr<Type> returnType,
                      std::vector<std::shared_ptr<Expression>> body,
                      std::vector<std::shared_ptr<Expression>> params = {},
                      bool isVarArg = false)
        : Callable(name, std::move(params), isVarArg),
          body(std::move(body)),
          returnType(returnType) {
        type = Type::createFunctionType(returnType, isVarArg);
        returnType = type->getReturnType();
    }

    std::string toString() const override {
        return "Function: " + name + " [Returns: " + (returnType ? returnType->kindName() : "void") + "]";
    }

    std::shared_ptr<Type> getReturnType() {
        return type->getReturnType();
    }

    // FunctionExpression
    std::shared_ptr<Expression> clone() const override {
        std::vector<std::shared_ptr<Expression>> clonedBody;
        for (const auto& expr : body) {
            clonedBody.push_back(expr ? expr->clone() : nullptr);
        }
        
        std::vector<std::shared_ptr<Expression>> clonedParams;
            for (const auto& param : parameters) {
                clonedParams.push_back(param ? param->clone() : nullptr);
            }
            
            return std::make_shared<FunctionExpression>(
                name,
                returnType ? returnType->clone() : nullptr,
                clonedBody,
                clonedParams,
                isVarArg
            );
        }
};

struct BlockExpression : public Expression {
    std::vector<std::shared_ptr<Expression>> values;  // Store multiple values in a vector

    BlockExpression(std::vector<std::shared_ptr<Expression>> values)
        : values(std::move(values)) {
        type = Type::createInvalid();
    }

    std::string toString() const override {
        std::string result = "Block: [ ";
        for (const auto& val : values) {
            result += val ? val->toString() : "null";
            result += " ";
        }
        result += "]";
        return result;
    }

    // BlockExpression
    std::shared_ptr<Expression> clone() const override {
        std::vector<std::shared_ptr<Expression>> clonedValues;
        for (const auto& val : values) {
            clonedValues.push_back(val ? val->clone() : nullptr);
        }
        return std::make_shared<BlockExpression>(clonedValues);
    }
};

// Aggregate Types (e.g., Struct, Enum, Array)
struct AggregateExpression : public Expression {
    std::vector<std::shared_ptr<Expression>> elements;
    std::vector<std::string> elementNames;
    uint64_t count;

    AggregateExpression(std::vector<std::shared_ptr<Expression>> elements, std::vector<std::string> elementNames)
        : elements(std::move(elements)), elementNames(std::move(elementNames)), count(elements.size()) {
        std::vector<std::shared_ptr<Type>> elementTypes;
        for (auto& e : this->elements)
            elementTypes.push_back(e->type);

        // type = Type::createStructType(elementTypes, this->elementNames);
    }

    std::string toString() const override { return "Aggregate"; }

    // AggregateExpression
    std::shared_ptr<Expression> clone() const override {
        std::vector<std::shared_ptr<Expression>> clonedElements;
        for (const auto& elem : elements) {
            clonedElements.push_back(elem ? elem->clone() : nullptr);
        }
        return std::make_shared<AggregateExpression>(
            clonedElements,
            elementNames
        );
    }
};


// Custom String and WideString Types
template <typename T>
class StringExpression : public Primitive<T> {
public:
    StringExpression(T value)
        : Primitive<T>(value) {}

    virtual ~StringExpression() = default;
    // StringExpression (template)
    std::shared_ptr<Expression> clone() const override {
        return std::make_shared<StringExpression<T>>(this->value);
    }
};

struct VariableAssignment : public Expression {
    std::string variableName;
    std::shared_ptr<Expression> assignedValue;

    VariableAssignment(std::string name, std::shared_ptr<Expression> value)
        : variableName(std::move(name)), assignedValue(std::move(value)) {
        type = assignedValue->type;  // Same type as the assigned value
    }

    std::shared_ptr<Expression> getValue() const { return assignedValue; }
    std::string toString() const override {
        return "Assign: " + variableName + " = " + assignedValue->toString();
    }
    // VariableAssignment
    std::shared_ptr<Expression> clone() const override {
        return std::make_shared<VariableAssignment>(
            variableName,
            assignedValue ? assignedValue->clone() : nullptr
        );
    }
};

struct VariableAccess : public Expression {
    std::string variableName;
    std::shared_ptr<Type> type;

    explicit VariableAccess(std::string name, std::shared_ptr<Type> type = nullptr) 
        : variableName(std::move(name)), type(type) {}

    std::string toString() const override {
        return "Variable: " + variableName;
    }
    // VariableAccess
    std::shared_ptr<Expression> clone() const override {
        return std::make_shared<VariableAccess>(variableName, type ? type->clone() : nullptr);
    }
};

struct ArrayExpression : public Expression {
    std::vector<std::shared_ptr<Expression>> elements;

    explicit ArrayExpression(std::shared_ptr<Type> type, std::vector<std::shared_ptr<Expression>> elements)
        : elements(std::move(elements)) {
        this->type = std::move(type);
    }

    std::string toString() const override {
        std::string s = "[";
        for (size_t i = 0; i < elements.size(); ++i) {
            s += elements[i] ? elements[i]->toString() : "null";
            if (i + 1 < elements.size()) s += ", ";
        }
        return s + "]";
    }

    void push(std::shared_ptr<Expression> val) {
        elements.push_back(std::move(val));
    }

    std::shared_ptr<Expression> get(size_t index) const {
        return index < elements.size() ? elements[index] : nullptr;
    }

    const std::vector<std::shared_ptr<Expression>>& getElements() const {
        return elements;
    }

    // ArrayExpression
    std::shared_ptr<Expression> clone() const override {
        std::vector<std::shared_ptr<Expression>> clonedElements;
        for (const auto& elem : elements) {
            clonedElements.push_back(elem ? elem->clone() : nullptr);
        }
        return std::make_shared<ArrayExpression>(
            type ? type->clone() : nullptr,
            clonedElements
        );
    }
};

class FixedArrayExpression : public Expression {
public:
    std::vector<std::shared_ptr<Expression>> elements;
    std::shared_ptr<Type> elementType;

    FixedArrayExpression(std::vector<std::shared_ptr<Expression>> elems, std::shared_ptr<Type> elemType)
        : elements(std::move(elems)), elementType(std::move(elemType)) {
            type = Type::createFixedArrayType(elementType, elements.size());
        }

    std::string typeName() const {
        return "FixedArray<" + (elementType ? elementType->kindName() : "unknown") + ">";
    }

    std::string toString() const override {
        std::string result = "[";
        for (size_t i = 0; i < elements.size(); ++i) {
            result += elements[i]->toString();
            if (i < elements.size() - 1) result += ", ";
        }
        return result + "]";
    }
    // FixedArrayExpression
    std::shared_ptr<Expression> clone() const override {
        std::vector<std::shared_ptr<Expression>> clonedElements;
        for (const auto& elem : elements) {
            clonedElements.push_back(elem ? elem->clone() : nullptr);
        }
        return std::make_shared<FixedArrayExpression>(
            clonedElements,
            elementType ? elementType->clone() : nullptr
        );
    }
};

}

#endif