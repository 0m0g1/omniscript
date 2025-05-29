#pragma once
#ifndef Expression_H
#define Expression_H

#include <omniscript/engine/tokens.h>
#include <omniscript/Core.h>
#include <omniscript/omniscript_pch.h>
#include <omniscript/debuggingtools/console.h>
#include <omniscript/Core/Types.h>

struct MemberModifiers {
    // Access modifiers
    enum class AccessModifier { Public, Protected, Private };
    AccessModifier access = AccessModifier::Public; // Default to private
    bool isInitialized = false;

    // Storage specifiers
    bool isStatic = false;
    bool isExtern = false;
    bool isMutable = false;
    bool isThreadLocal = false;

    // Function-specific modifiers
    bool isVirtual = false;
    bool isOverride = false;
    bool shouldOverride = false;
    bool isFinal = false;
    bool isConst = false;
    bool isVolatile = false;
    bool isNoexcept = false;
    bool isPureVirtual = false; // Implies `= 0`
    bool isExplicit = false;
    bool isInline = false;
    bool isConstexpr = false;

    // Special member function specifiers
    bool isDefault = false; // Implies `= default`
    bool isDeleted = false; // Implies `= delete`

    // Attribute specifiers
    bool isNodiscard = false;
    bool isMaybeUnused = false;
    bool isDeprecated = false;
    bool isLikely = false;
    bool isUnlikely = false;

    // Convenience function to display modifiers as a string
    std::string toString() const {
        std::string result;

        // Access
        switch (access) {
            case AccessModifier::Public: result += "public "; break;
            case AccessModifier::Protected: result += "protected "; break;
            case AccessModifier::Private: result += "private "; break;
        }

        // Storage specifiers
        if (isStatic) result += "static ";
        if (isExtern) result += "extern ";
        if (isMutable) result += "mutable ";
        if (isThreadLocal) result += "thread_local ";

        // Function modifiers
        if (isVirtual) result += "virtual ";
        if (isOverride) result += "is_override ";
        if (shouldOverride) result += "should_override ";
        if (isFinal) result += "final ";
        if (isConst) result += "const ";
        if (isVolatile) result += "volatile ";
        if (isNoexcept) result += "noexcept ";
        if (isPureVirtual) result += "= 0 (pure virtual) ";
        if (isExplicit) result += "explicit ";
        if (isInline) result += "inline ";
        if (isConstexpr) result += "constexpr ";

        // Special member function specifiers
        if (isDefault) result += "= default ";
        if (isDeleted) result += "= delete ";

        // Attributes
        if (isNodiscard) result += "[[nodiscard]] ";
        if (isMaybeUnused) result += "[[maybe_unused]] ";
        if (isDeprecated) result += "[[deprecated]] ";
        if (isLikely) result += "[[likely]] ";
        if (isUnlikely) result += "[[unlikely]] ";

        return result.empty() ? "none" : result;
    }
};

namespace std {
    template <>
    struct hash<MemberModifiers> {
        size_t operator()(const MemberModifiers& modifiers) const {
            size_t result = 0;

            // Hash the access modifier (enum class)
            result ^= static_cast<size_t>(modifiers.access) + 0x9e3779b9 + (result << 6) + (result >> 2);

            // Hash the bool flags for storage specifiers and function-specific modifiers
            result ^= modifiers.isStatic + 0x9e3779b9 + (result << 6) + (result >> 2);
            result ^= modifiers.isExtern + 0x9e3779b9 + (result << 6) + (result >> 2);
            result ^= modifiers.isMutable + 0x9e3779b9 + (result << 6) + (result >> 2);
            result ^= modifiers.isThreadLocal + 0x9e3779b9 + (result << 6) + (result >> 2);
            result ^= modifiers.isVirtual + 0x9e3779b9 + (result << 6) + (result >> 2);
            result ^= modifiers.isOverride + 0x9e3779b9 + (result << 6) + (result >> 2);
            result ^= modifiers.shouldOverride + 0x9e3779b9 + (result << 6) + (result >> 2);
            result ^= modifiers.isFinal + 0x9e3779b9 + (result << 6) + (result >> 2);
            result ^= modifiers.isConst + 0x9e3779b9 + (result << 6) + (result >> 2);
            result ^= modifiers.isVolatile + 0x9e3779b9 + (result << 6) + (result >> 2);
            result ^= modifiers.isNoexcept + 0x9e3779b9 + (result << 6) + (result >> 2);
            result ^= modifiers.isPureVirtual + 0x9e3779b9 + (result << 6) + (result >> 2);
            result ^= modifiers.isExplicit + 0x9e3779b9 + (result << 6) + (result >> 2);
            result ^= modifiers.isInline + 0x9e3779b9 + (result << 6) + (result >> 2);
            result ^= modifiers.isConstexpr + 0x9e3779b9 + (result << 6) + (result >> 2);
            result ^= modifiers.isDefault + 0x9e3779b9 + (result << 6) + (result >> 2);
            result ^= modifiers.isDeleted + 0x9e3779b9 + (result << 6) + (result >> 2);
            result ^= modifiers.isNodiscard + 0x9e3779b9 + (result << 6) + (result >> 2);
            result ^= modifiers.isMaybeUnused + 0x9e3779b9 + (result << 6) + (result >> 2);
            result ^= modifiers.isDeprecated + 0x9e3779b9 + (result << 6) + (result >> 2);
            result ^= modifiers.isLikely + 0x9e3779b9 + (result << 6) + (result >> 2);
            result ^= modifiers.isUnlikely + 0x9e3779b9 + (result << 6) + (result >> 2);

            return result;
        }
    };
}

namespace Omniscript {

// ====================================== Expressions ====================================== //
struct Expression {
public:
    virtual ~Expression() = default;  // Polymorphic base
    
    virtual std::shared_ptr<Expression> clone() const { return nullptr; }
    std::shared_ptr<Type> getType() const { return type; }
    std::shared_ptr<Type> getRootType() const { return rootType; }
    virtual std::string toString() const { return "Expression"; }
    std::string getName() const { return name; }

    std::string name;
    std::shared_ptr<Type> type = Type::createInvalid();  // Holds a full Type object now
    std::shared_ptr<Type> rootType = Type::createInvalid();  // Holds a full Type object now
};

template <typename T>
std::shared_ptr<T> make_expression(auto&&... args) {
    return std::make_shared<T>(std::forward<decltype(args)>(args)...);
}

struct UndefinedExpression : public Expression {
    UndefinedExpression() {
        type = Type::createUndefined();
        rootType = Type::createUndefined();
    }

    std::string toString() const override { return "Undefined"; }
    // Undefined
    std::shared_ptr<Expression> clone() const override {
        return std::make_shared<UndefinedExpression>();
    }
};

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
        : NumericExpression<T>(value) {
            this->rootType = std::make_shared<Type>(Kind::Int8);
        }  // Specify type

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
        : NumericExpression<T>(value) {
            this->rootType = std::make_shared<Type>(Kind::Half);
        }  // Specify type

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
        : NumericExpression<std::string>(value), bitWidth(bitWidth) {
            rootType = Type::createPrimitiveType(Kind::Int8);
        }  // Pass value to base class constructor

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
        rootType = std::make_shared<Type>(Kind::Pointer);
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

struct InvalidExpression : public Expression {
    InvalidExpression() {
        type = Type::createInvalid();
        rootType = std::make_shared<Type>(Kind::Invalid);
    }

    std::string toString() const override { return "Invalid"; }
    // NullExpression
    std::shared_ptr<Expression> clone() const override {
        return std::make_shared<InvalidExpression>();
    }
};

struct NullExpression : public Expression {
    std::shared_ptr<Type> expectedType;
    NullExpression(std::shared_ptr<Type> expectedType = nullptr) : expectedType(expectedType) {
        type = expectedType;
        rootType = Type::createNullType();
    }
    
    std::string toString() const override { return "Null expects " + type->toString(); }
    // NullExpression
    std::shared_ptr<Expression> clone() const override {
        return std::make_shared<NullExpression>();
    }
};

struct NullPointerExpression : public Expression {
    std::shared_ptr<Type> expectedType;
    NullPointerExpression(std::shared_ptr<Type> expectedType = nullptr) : expectedType(expectedType) {
        type = Type::createPointerType(expectedType);
        rootType = Type::createNullPointerType();
    }

    std::string toString() const override { return "NullPointer expects " + type->toString(); } 
    // NullPointerExpression
    std::shared_ptr<Expression> clone() const override {
        return std::make_shared<NullPointerExpression>();
    }
};

struct AddressOfExpression : public Expression {
    std::shared_ptr<Expression> referent;  // The variable whose address is being stored
    std::string variableName;

    explicit AddressOfExpression(const std::string& variableName, std::shared_ptr<Expression> referent = nullptr)
        : variableName(variableName), referent(referent) {
        // We assume AddressOf value is of type Pointer to the referent's type
        type = Type::createPointerType(this->referent->type); 
        rootType = type;
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
    bool isVariadic = false;
    bool isConstant = false;
    std::shared_ptr<Expression> value;

    FunctionInputExpression(const std::string& name, std::shared_ptr<Type> type = nullptr, std::shared_ptr<Expression> value = nullptr, bool isConst = false) :
    value(std::move(value)), isConstant(isConst) {
        this->name = name;
        this->type = std::move(type);
    }
    
    std::string toString() const override { return "(FunctionInput: " + name + ", value: " + value->toString() + ")"; } 

    std::shared_ptr<Expression> clone() const override {
        auto input = std::make_shared<FunctionInputExpression>(
            name,
            type ? type->clone() : nullptr,
            value ? value->clone() : nullptr,
            isConstant
        );
        input->isVariadic = isVariadic;
        return input;
    }
};

struct Callable : public virtual Expression {
    std::string mangledName;
    std::vector<std::shared_ptr<Expression>> parameters;
    bool isVarArg;

    Callable(
            const std::string& name,
            const std::string& mangledName,
            std::vector<std::shared_ptr<Expression>> params = {},
            bool isVarArg = false
        )
        : parameters(std::move(params)), 
            isVarArg(isVarArg) {
        this->name = name;
        this->mangledName = mangledName;
    }

    // Helper method to clone parameters
    std::vector<std::shared_ptr<FunctionInputExpression>> cloneParameters() const {
        std::vector<std::shared_ptr<FunctionInputExpression>> clonedParams;
        for (const auto& parameter : parameters) {
            clonedParams.push_back(std::dynamic_pointer_cast<FunctionInputExpression>(parameter->clone()));
        }
        return clonedParams;
    }

    std::vector<std::shared_ptr<Expression>> getParameters() const {
        return parameters;
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
        return std::make_shared<Callable>(name, mangledName, clonedParams, isVarArg);
    }
};

// FunctionExpression inherits from Callable
struct FunctionExpression : public Callable {
    bool isExtern = false;
    bool isIntrinsic = false;
    std::string externLanguage;
    std::vector<std::shared_ptr<Expression>> body;
    std::shared_ptr<Type> returnType;
    std::vector<std::shared_ptr<Type>> paramTypes;

    FunctionExpression(
                        const std::string& name, 
                        const std::string& mangledName, 
                        std::shared_ptr<Type> returnType,
                        std::vector<std::shared_ptr<Expression>> body = {},
                        std::vector<std::shared_ptr<Expression>> params = {},
                        std::vector<std::shared_ptr<Type>> paramTypes = {},
                        bool isVarArg = false)
        : Callable(name, mangledName, std::move(params), isVarArg),
            body(std::move(body)), paramTypes(paramTypes),
            returnType(returnType) {
        type = Type::createFunctionType(name, paramTypes, returnType, isVarArg);
        returnType = type->getReturnType();
    }

    std::string toString() const override {
        return "Function: " + name + " [Returns: " + (returnType ? returnType->description() : "void") + "]";
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
                mangledName,
                returnType ? returnType->clone() : nullptr,
                clonedBody,
                clonedParams,
                paramTypes,
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
struct AggregateExpression : public virtual Expression {
    ~AggregateExpression() = default;
    std::string toString() const override { return "Aggregate"; }
};

struct StructExpression : 
public Callable,
public AggregateExpression {
    std::string structName;
    std::vector<std::string> elementNames;

    StructExpression(
        const std::string& structName,
        const std::string& mangledName,
        const std::vector<std::shared_ptr<Expression>>& fields = {},
        const std::vector<std::string>& fieldNames = {},
        bool isVarArg = false
    )
        : Callable(structName, mangledName, fields, isVarArg),
          structName(structName),
          elementNames(fieldNames)
    {
        // Define type as struct
        std::vector<std::shared_ptr<Type>> fieldTypes;
        for (auto& f : parameters)
            fieldTypes.push_back(f->type);

        type = std::make_shared<UserDefinedType>(name);
    }

    std::string toString() const override {
        return "Struct : " + structName;
    }

    std::shared_ptr<Expression> clone() const override {
        std::vector<std::shared_ptr<Expression>> clonedFields;
        for (const auto& field : parameters)
            clonedFields.push_back(field ? field->clone() : nullptr);

        return std::make_shared<StructExpression>(
            structName,
            mangledName,
            clonedFields,
            elementNames,
            isVarArg
        );
    }
};

// MemberExpression.h
struct MemberExpression : public Expression {
public:
    std::shared_ptr<Expression> value;
    MemberModifiers modifiers;

    MemberExpression(
        const std::string& name,
        std::shared_ptr<Type> type,
        std::shared_ptr<Expression> value,
        const MemberModifiers& mods = {}
    ) {
        this->name = name;
        this->type = type;
        this->value = value;
        this->modifiers = mods;
    }

    // --- Modifier queries ---
    bool isPublic() const        { return modifiers.access == MemberModifiers::AccessModifier::Public; }
    bool isPrivate() const       { return modifiers.access == MemberModifiers::AccessModifier::Private; }
    bool isProtected() const     { return modifiers.access == MemberModifiers::AccessModifier::Protected; }

    bool isStatic() const        { return modifiers.isStatic; }
    bool isConst() const         { return modifiers.isConst; }
    bool isVirtual() const       { return modifiers.isVirtual; }
    bool isOverride() const      { return modifiers.shouldOverride; }
    bool isFinal() const         { return modifiers.isFinal; }
    bool isConstexpr() const     { return modifiers.isConstexpr; }
    bool isInline() const        { return modifiers.isInline; }
    bool isNoexcept() const      { return modifiers.isNoexcept; }
    bool isPureVirtual() const   { return modifiers.isPureVirtual; }
    bool isExplicit() const      { return modifiers.isExplicit; }
    bool isDeleted() const       { return modifiers.isDeleted; }
    bool isDefault() const       { return modifiers.isDefault; }
    bool isMutable() const       { return modifiers.isMutable; }
    bool isThreadLocal() const   { return modifiers.isThreadLocal; }
    bool isExtern() const        { return modifiers.isExtern; }

    MemberModifiers::AccessModifier getAccess() const {
        return modifiers.access;
    }

    std::string getAccessString() const {
        switch (modifiers.access) {
            case MemberModifiers::AccessModifier::Public: return "public";
            case MemberModifiers::AccessModifier::Protected: return "protected";
            case MemberModifiers::AccessModifier::Private: return "private";
        }
        return "unknown";
    }

    std::string toString() const override {
        return modifiers.toString() + (type ? type->toString() : "unknown") + " " + name + ";";
    }
};

// ClassMemberExpression.h
struct ClassMemberExpression : public MemberExpression {
    ClassMemberExpression(
        const std::string& name,
        std::shared_ptr<Expression> value,
        const MemberModifiers& modifiers = {}
    )
        : MemberExpression(name, value ? value->getType() : Type::createInvalid(), value, modifiers) {
        if (this->value) {
            this->rootType = this->value->getRootType();
        }
    }

    std::shared_ptr<Expression> clone() const override {
        return std::make_shared<ClassMemberExpression>(
            name,
            value ? value->clone() : nullptr,
            modifiers
        );
    }

    std::string toString() const override {
        return "ClassMember(" + name + "): " + modifiers.toString();
    }
};

// ModuleMemberExpression.h
struct ModuleMemberExpression : public MemberExpression {
public:
    std::shared_ptr<Expression> value;

    ModuleMemberExpression(
        const std::string& name,
        std::shared_ptr<Expression> value,
        const MemberModifiers& modifiers = {}
    )
        : MemberExpression(name, value ? value->getType() : Type::createInvalid(), value, modifiers) {
        if (this->value) {
            this->rootType = this->value->getRootType();
        }
    }

    std::shared_ptr<Expression> clone() const override {
        return std::make_shared<ModuleMemberExpression>(
            name,
            value ? value->clone() : nullptr,
            modifiers
        );
    }

    std::string toString() const override {
        return "ModuleMember(" + name + "): " + modifiers.toString();
    }
};


struct ClassExpression : 
public Callable,
public AggregateExpression {
    std::shared_ptr<StructExpression> structExpr;
    std::vector<std::shared_ptr<FunctionExpression>> constructors;
    std::shared_ptr<FunctionExpression> destructor;
    std::vector<std::shared_ptr<ClassMemberExpression>> members;

    ClassExpression(
        const std::string& name,
        std::shared_ptr<StructExpression> structExpr,
        std::vector<std::shared_ptr<FunctionExpression>> constructors = {},
        std::shared_ptr<FunctionExpression> destructor = nullptr,
        std::vector<std::shared_ptr<ClassMemberExpression>> members = {}
    )
        : Callable(name, name, {}, false),  // Dummy mangled name for now
          structExpr(std::move(structExpr)),
          constructors(std::move(constructors)),
          destructor(std::move(destructor)),
          members(std::move(members))
    {
        type = this->structExpr->getType(); // Inherit struct type
    }

    std::string toString() const override {
        std::string memberStr;
        for (const auto& member : members) {
            memberStr += "\n  " + member->toString();
        }

        return "Class: " + structExpr->structName +
               " [Constructors: " + std::to_string(constructors.size()) +
               ", Destructor: " + (destructor ? "yes" : "none") + "]" +
               (members.empty() ? "" : "\nMembers:" + memberStr);
    }

    std::shared_ptr<FunctionExpression> resolveConstructor(const std::vector<std::shared_ptr<Expression>>& args) const {
        for (const auto& ctor : constructors) {
            if (ctor->getParameters().size() == args.size()) {
                return ctor;
            }
        }
        return nullptr;
    }

    std::shared_ptr<Expression> clone() const override {
        auto clonedStruct = std::dynamic_pointer_cast<StructExpression>(structExpr->clone());

        std::vector<std::shared_ptr<FunctionExpression>> clonedCtors;
        for (const auto& ctor : constructors)
            clonedCtors.push_back(std::dynamic_pointer_cast<FunctionExpression>(ctor->clone()));

        auto clonedDtor = destructor ? std::dynamic_pointer_cast<FunctionExpression>(destructor->clone()) : nullptr;

        std::vector<std::shared_ptr<ClassMemberExpression>> clonedMembers;
        for (const auto& member : members)
            clonedMembers.push_back(std::dynamic_pointer_cast<ClassMemberExpression>(member->clone()));

        return std::make_shared<ClassExpression>(
            name, clonedStruct, clonedCtors, clonedDtor, clonedMembers
        );
    }

    std::shared_ptr<ClassMemberExpression> getMember(const std::string& name) {
        for (const auto& member : members) {
            if (member->getName() == name) {
                return member;
            }
        }
        console.error("Member '" + name + "' not found in class '" + this->getName() + "'.");
        return nullptr;
    }

    std::string serializeMembers() const {
        std::string result = "[\n";
        for (const auto& member : members) {
            result += "  { name: \"" + member->getName() + "\", ";
            result += "type: \"" + member->getType()->toString() + "\", ";
            result += "modifiers: \"" + member->getAccessString() + "\" },\n";
        }
        result += "]";
        return result;
    }
};


// Represents a complete module
struct ModuleExpression : 
public AggregateExpression {
    std::vector<std::shared_ptr<ModuleMemberExpression>> members;

    ModuleExpression(const std::string& moduleName, const std::vector<std::shared_ptr<ModuleMemberExpression>>& members = {})
        : members(members) {
        this->name = moduleName;
        this->type = Type::createInvalid();  // Modules may have special types or be treated as namespaces
    }

    std::string toString() const override {
        std::string result = "Module " + name + " {\n";
        for (const auto& member : members) {
            result += "  " + member->toString() + "\n";
        }
        result += "}";
        return result;
    }

    std::shared_ptr<Expression> clone() const override {
        std::vector<std::shared_ptr<ModuleMemberExpression>> clonedMembers;
        for (const auto& member : members) {
            clonedMembers.push_back(std::dynamic_pointer_cast<ModuleMemberExpression>(member->clone()));
        }
        return std::make_shared<ModuleExpression>(name, clonedMembers);
    }

    std::shared_ptr<ModuleMemberExpression> getMember(const std::string& name) {
        for (const auto& member : members) {
            if (member->getName() == name) {
                return member;
            }
        }
        console.error("Member '" + name + "' not found in class '" + this->getName() + "'.");
        return nullptr;
    }

};

struct InstanceExpression : public Expression {
public:
    std::string baseName;
    std::string instanceName;
    std::shared_ptr<Type> instanceType;

    std::vector<std::shared_ptr<MemberExpression>> memberExpressions;  // Unified vector for all members

    InstanceExpression(
        const std::string& baseName,
        const std::string& instanceName,
        const std::vector<std::shared_ptr<MemberExpression>>& memberExpressions = {}
    ) : baseName(baseName),
        instanceName(instanceName),
        memberExpressions(memberExpressions) {

        this->instanceType = std::make_shared<UserDefinedType>(baseName);
        this->type = instanceType; // inherited from Expression
    }

    std::string toString() const override {
        return "Instance<" + baseName + "> named " + instanceName;
    }

    // Retrieves a member by name from the unified list of members
    std::shared_ptr<Expression> getField(const std::string& name) const {
        for (const auto& member : memberExpressions) {
            if (member->getType()->getParameterName() == name) {
                return member;
            }
        }
        return nullptr; // Not found
    }

    // Sets or replaces a member value in the unified list of members
    bool setField(const std::string& name, const std::shared_ptr<Expression>& newValue) {
        for (auto& member : memberExpressions) {
            if (member->getType()->getParameterName() == name) {
                member->value = newValue;  // This line should work now
                return true;
            }
        }
        return false; // Not found
    }

    std::shared_ptr<Expression> clone() const override {
        std::vector<std::shared_ptr<MemberExpression>> clonedMembers;
        for (const auto& member : memberExpressions) {
            clonedMembers.push_back(std::dynamic_pointer_cast<MemberExpression>(member->clone()));
        }

        return std::make_shared<InstanceExpression>(
            baseName,
            instanceName,
            clonedMembers
        );
    }
};

struct CallExpression : public Expression {
    std::string calleeName;
    std::string instanceName;
    std::vector<std::shared_ptr<Expression>> args;
    std::vector<std::shared_ptr<MemberExpression>> members;

    bool isGlobal;

    CallExpression(const std::string& calleeName, const std::vector<std::shared_ptr<Expression>>& args = {}, std::shared_ptr<Type> returnType = nullptr)
    : calleeName(calleeName), args(std::move(args)) {
        type = std::move(returnType);
    }

    CallExpression(const std::string& objectName,
        const std::string& instanceName,
        const std::vector<std::shared_ptr<Expression>>& args = {},
        std::shared_ptr<Type> returnType = nullptr,
        bool isGlobal = true
    )
    : calleeName(objectName), instanceName(instanceName), args(std::move(args)), isGlobal(isGlobal) {
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
            instanceName,
            clonedArgs,
            type ? type->clone() : nullptr
        );
    }
    std::string toString() const override {
        if (instanceName.empty()) {
            return "Call: " + calleeName;
        }
        return "Call create instance '" + instanceName + "' of object '" + calleeName + "'.";
    }
};

// Base class for all access expressions
struct AccessExpression : public Expression {
    bool isInternal = false;
    std::shared_ptr<Expression> expr;                // expression that represents the parent object
    std::string member;                              // single member name
    int index = -1;                                   // single index (e.g., for array fields)
    std::shared_ptr<Expression> assignmentValue;

    bool isInternalAccess() const {
        return isInternal;
    }

    bool isSetter() const {
        return assignmentValue != nullptr;
    }

    virtual std::string toString() const override = 0;
    virtual std::shared_ptr<Expression> clone() const = 0;
};

struct MemberAccessExpression : public AccessExpression {
public:
    std::string baseType;
    std::string instanceName;

    MemberAccessExpression(
        std::shared_ptr<Expression> parentExpr,
        const std::string& baseType,
        const std::string& instanceName,
        const std::string& member,
        int index,
        std::shared_ptr<Type> memberType,
        std::shared_ptr<Expression> assignmentValue = nullptr
    ) : baseType(baseType), instanceName(instanceName)
    {
        expr = parentExpr;
        this->member = member;
        this->index = index;
        this->type = memberType;
        this->assignmentValue = assignmentValue;
    }

    std::string toString() const override {
        std::string indexStr = (index >= 0) ? "[" + std::to_string(index) + "]" : "";
        if (isSetter()) {
            return expr->toString() + "." + member + indexStr + " = " + assignmentValue->toString();
        } else {
            return expr->toString() + "." + member + indexStr;
        }
    }

    std::shared_ptr<Expression> clone() const override {
        return std::make_shared<MemberAccessExpression>(
            expr->clone(),
            baseType,
            instanceName,
            member,
            index,
            type,
            assignmentValue ? assignmentValue->clone() : nullptr
        );
    }
};

struct ArrowAccessExpression : public AccessExpression {
public:
    ArrowAccessExpression(
        std::shared_ptr<Expression> parentExpr,
        const std::string& member,
        int index = -1
    ) {
        expr = parentExpr;
        this->member = member;
        this->index = index;
    }

    std::string toString() const override {
        std::string indexStr = (index >= 0) ? "[" + std::to_string(index) + "]" : "";
        return expr->toString() + "->" + member + indexStr;
    }

    std::shared_ptr<Expression> clone() const override {
        return std::make_shared<ArrowAccessExpression>(
            expr->clone(),
            member,
            index
        );
    }
};

struct DereferenceExpression : public AccessExpression {
public:
    std::shared_ptr<Expression> valueExpr;
    std::shared_ptr<Type> type;

    DereferenceExpression(
        std::shared_ptr<Expression> pointerExpr,
        std::shared_ptr<Expression> val,
        std::shared_ptr<Type> resultType,
        const std::string& member = "",
        int index = -1
    ) : valueExpr(val), type(resultType)
    {
        expr = pointerExpr;
        this->member = member;
        this->index = index;
    }

    std::string toString() const override {
        std::string suffix;
        if (!member.empty()) {
            suffix = "." + member;
            if (index >= 0) suffix += "[" + std::to_string(index) + "]";
        }

        if (valueExpr) {
            return "*(" + expr->toString() + ")" + suffix + " = " + valueExpr->toString();
        } else {
            return "*(" + expr->toString() + ")" + suffix;
        }
    }

    std::shared_ptr<Expression> clone() const override {
        return std::make_shared<DereferenceExpression>(
            expr->clone(),
            valueExpr ? valueExpr->clone() : nullptr,
            type,
            member,
            index
        );
    }
};

struct IndexAccessExpression : public AccessExpression {
public:
    std::shared_ptr<Expression> indexExpr;

    IndexAccessExpression(
        std::shared_ptr<Expression> containerExpr,
        std::shared_ptr<Expression> indexExpr
    ) : indexExpr(indexExpr) {
        expr = containerExpr;
    }

    std::string toString() const override {
        return expr->toString() + "[" + indexExpr->toString() + "]";
    }

    std::shared_ptr<Expression> clone() const override {
        return std::make_shared<IndexAccessExpression>(
            expr->clone(),
            indexExpr->clone()
        );
    }
};

struct EnumExpression : public Expression {
    EnumExpression(const std::string& enumName, bool hasLookup = false, bool isEnumClass = false)
        : enumName(enumName), hasLookup(hasLookup), isEnumClass(isEnumClass) {
        name = enumName;
    }

    // Add entry for int -> expression mapping
    void addEntry(int value, const std::string& valueName, std::shared_ptr<Expression> expression) {
        enumerators[valueName] = value;
        expressionMap[valueName] = expression;  // Store the corresponding expression
    }

    // Retrieve the integer value associated with the enum name
    int get(const std::string& enumeration) const {
        auto it = enumerators.find(enumeration);
        if (it != enumerators.end()) return it->second;
        console.error("Enum '" + enumName + "' does not have an entry " + enumeration);
        return -9999999;
    }

    // Retrieve the string name for a given integer value
    std::string getName(int value) const {
        for (const auto& [name, val] : enumerators)
            if (val == value) return name;
        return "";
    }

    // Retrieve the expression associated with a given name
    std::shared_ptr<Expression> getExpression(const std::string& valueName) const {
        auto it = expressionMap.find(valueName);
        if (it != expressionMap.end()) {
            return it->second;
        }
        console.error("Enum expression for '" + enumName + "' does not have an entry " + valueName);
        return nullptr;
    }

    // Clone this enum expression
    std::shared_ptr<Expression> clone() const override {
        auto copy = std::make_shared<EnumExpression>(enumName, hasLookup, isEnumClass);
        copy->enumerators = enumerators;
        copy->expressionMap = expressionMap;  // Also clone the expression map
        return copy;
    }

    // Convert the enum to a string representation
    std::string toString() const override {
        return (isEnumClass ? "enum class " : "enum ") + enumName;
    }

    // Member variables
    std::string enumName;
    bool hasLookup = false;
    bool isEnumClass = false;

    // Maps for the enum entries
    std::unordered_map<std::string, int> enumerators;  // value name -> value
    std::unordered_map<std::string, std::shared_ptr<Expression>> expressionMap;  // value name -> expression
};


// Custom String and WideString Types
template <typename T>
class StringExpression : public Primitive<T> {
public:
    StringExpression(T value)
        : Primitive<T>(value) {
            auto charType = Omniscript::Type::createPrimitiveType(Omniscript::Kind::Char);
            auto stringType = Omniscript::Type::createPointerType(charType);
            this->rootType = stringType;
        }

    virtual ~StringExpression() = default;
    // StringExpression (template)
    std::shared_ptr<Expression> clone() const override {
        return std::make_shared<StringExpression<T>>(this->value);
    }
};

struct VariableAssignment : public Expression {
    bool isConstant;
    bool isGlobal;
    bool isReassignment;
    std::string variableName;
    std::shared_ptr<Expression> assignedValue;

    VariableAssignment(std::string name, std::shared_ptr<Expression> value, bool isGlobal = false, bool isReassignment = false)
        : variableName(std::move(name)), assignedValue(std::move(value)), isGlobal(isGlobal), isReassignment(isReassignment) {
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

    explicit VariableAccess(std::string name, std::shared_ptr<Type> type = nullptr) 
        : variableName(std::move(name)) {
            this->type = type ? type : this->type;
        }

    std::string toString() const override {
        return "Variable: " + variableName;
    }
    // VariableAccess
    std::shared_ptr<Expression> clone() const override {
        return std::make_shared<VariableAccess>(variableName, type ? type->clone() : nullptr);
    }
};

struct ArrayExpression : public Expression {
    bool isVariadicArray = false;
    std::vector<std::shared_ptr<Expression>> elements;

    explicit ArrayExpression(std::shared_ptr<Type> type, std::vector<std::shared_ptr<Expression>> elements = {}, bool isVariadic = false)
        : elements(std::move(elements)), isVariadicArray(isVariadic) {
        this->type = Type::createFixedArrayType(type, elements.size());
        this->rootType = this->type;
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
        return "FixedArray<" + (elementType ? elementType->description() : "unknown") + ">";
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

struct IfExpression : public Expression {
    // Branches for if, else if, and an optional else part
    std::vector<std::shared_ptr<Expression>> conditions;  // conditions of if/else if
    std::vector<std::shared_ptr<Expression>> bodies;      // corresponding bodies (blocks)
    std::shared_ptr<Expression> elseBody;                 // optional else body

    IfExpression(
        std::vector<std::shared_ptr<Expression>> conditions, 
        std::vector<std::shared_ptr<Expression>> bodies,
        std::shared_ptr<Expression> elseBody = nullptr
    ) : conditions(std::move(conditions)),
        bodies(std::move(bodies)),
        elseBody(std::move(elseBody)) {

        this->type = Type::createInvalid(); // Default type for now
    }

    std::string toString() const override {
        std::string result = "IfExpression with " + std::to_string(conditions.size()) + " branches";

        for (size_t i = 0; i < conditions.size(); ++i) {
            result += "\n  if (" + conditions[i]->toString() + ") " + bodies[i]->toString();
        }

        if (elseBody) {
            result += "\n  else " + elseBody->toString();
        }

        return result;
    }

    std::shared_ptr<Expression> clone() const override {
        std::vector<std::shared_ptr<Expression>> clonedConditions;
        for (const auto& cond : conditions) {
            clonedConditions.push_back(cond->clone());
        }

        std::vector<std::shared_ptr<Expression>> clonedBodies;
        for (const auto& body : bodies) {
            clonedBodies.push_back(body->clone());
        }

        std::shared_ptr<Expression> clonedElseBody = nullptr;
        if (elseBody) {
            clonedElseBody = elseBody->clone();
        }

        return std::make_shared<IfExpression>(clonedConditions, clonedBodies, clonedElseBody);
    }
};

struct ForLoopExpression : public Expression {
    std::shared_ptr<Expression> initializer;
    std::shared_ptr<Expression> condition;
    std::shared_ptr<Expression> increment;
    std::shared_ptr<Expression> body;

    ForLoopExpression(
        std::shared_ptr<Expression> initializer,
        std::shared_ptr<Expression> condition,
        std::shared_ptr<Expression> increment,
        std::shared_ptr<Expression> body
    ) : initializer(std::move(initializer)),
        condition(std::move(condition)),
        increment(std::move(increment)),
        body(std::move(body)) {}

    std::string toString() const override {
        return "ForLoop(init: " + (initializer ? initializer->toString() : "null") +
               ", cond: " + (condition ? condition->toString() : "null") +
               ", inc: " + (increment ? increment->toString() : "null") +
               ", body: " + (body ? body->toString() : "null") + ")";
    }

    std::shared_ptr<Expression> clone() const override {
        return std::make_shared<ForLoopExpression>(
            initializer ? initializer->clone() : nullptr,
            condition ? condition->clone() : nullptr,
            increment ? increment->clone() : nullptr,
            body ? body->clone() : nullptr
        );
    }
};

struct WhileLoopExpression : public Expression {
    std::shared_ptr<Expression> condition;
    std::shared_ptr<Expression> body;

    WhileLoopExpression(
        std::shared_ptr<Expression> condition,
        std::shared_ptr<Expression> body
    ) : condition(std::move(condition)), body(std::move(body)) {}

    std::string toString() const override {
        return "WhileLoop(cond: " + (condition ? condition->toString() : "null") +
               ", body: " + (body ? body->toString() : "null") + ")";
    }

    std::shared_ptr<Expression> clone() const override {
        return std::make_shared<WhileLoopExpression>(
            condition ? condition->clone() : nullptr,
            body ? body->clone() : nullptr
        );
    }
};

}

#endif