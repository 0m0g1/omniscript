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
    virtual ~Expression() = default;  
    
    virtual std::shared_ptr<Expression> clone() const { return nullptr; }
    std::shared_ptr<Type> getType() const { return type; }
    std::shared_ptr<Type> getRootType() const { return rootType; }
    virtual std::string toString() const { return "Expression"; }
    std::string getName() const { return name; }

    std::string name;
    std::shared_ptr<Type> type = Type::createInvalid();  
    std::shared_ptr<Type> rootType = Type::createInvalid();  
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
    
    std::shared_ptr<Expression> clone() const override {
        return std::make_shared<UndefinedExpression>();
    }
};

struct TypeExpression : public Expression {
    std::string name;
    std::shared_ptr<Type> actualType;  

    
    explicit TypeExpression(const std::string& typeName, std::shared_ptr<Type> type)
        : name(typeName), actualType(std::move(type)) {
        this->type = Type::createMetaType();  
    }

    
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

    
    std::shared_ptr<Expression> clone() const override {
        return std::make_shared<TypeExpression>(name, actualType ? actualType->clone() : nullptr);
    }
};

struct CastExpression : public Expression {
    std::shared_ptr<Expression> targetExpr;
    std::shared_ptr<Type> castTargetType;

    CastExpression(std::shared_ptr<Expression> expr, std::shared_ptr<Type> targetType)
        : targetExpr(expr), castTargetType(targetType) {
        type = castTargetType;
    }

    std::string toString() const override {
        return "Cast<" + (castTargetType ? castTargetType->toString() : "unknown") +
               ">(" + (targetExpr ? targetExpr->toString() : "null") + ")";
    }

    std::shared_ptr<Expression> clone() const override {
        return std::make_shared<CastExpression>(
            targetExpr ? targetExpr->clone() : nullptr,
            castTargetType ? castTargetType->clone() : nullptr
        );
    }
};

template <typename T>
struct Primitive : public Expression {
    explicit Primitive(T value) : value(value) {
        type = Type::createPrimitiveType(PrimitiveType::get<T>());

        if constexpr (std::is_same_v<T, std::string> ||
                    std::is_same_v<T, std::u16string> ||
                    std::is_same_v<T, std::u32string>
                ) {
            auto charType = Omniscript::Type::createPrimitiveType(Omniscript::Kind::Char);
            auto stringType = Omniscript::Type::createPointerType(this->type);
            this->type = stringType;
            this->rootType = Omniscript::Type::createPointerType(charType);
        }
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
            return "Primitive: " + std::to_string(value); 
        }
    }

    T getValue() const { return value; }

    
    std::shared_ptr<Expression> clone() const override {
        return std::make_shared<Primitive<T>>(value);
    }

    T value;
};

template <typename T>
class NumericExpression : public Primitive<T> {
public:
    NumericExpression(T value)
        : Primitive<T>(value) {}

    virtual ~NumericExpression() = default;
    
    std::shared_ptr<Expression> clone() const override {
        return std::make_shared<NumericExpression<T>>(this->value);
    }
};

template <typename T>
class Integer : public NumericExpression<T> {
public:
    Integer(T value)
        : NumericExpression<T>(value) {
            this->rootType = std::make_shared<Type>(Kind::Int8);
        }  

    ~Integer() override = default;
    
    std::shared_ptr<Expression> clone() const override {
        return std::make_shared<Integer<T>>(this->value);
    }
};

template <typename T>
class Float : public NumericExpression<T> {
public:
    Float(T value)
        : NumericExpression<T>(value) {
            this->rootType = std::make_shared<Type>(Kind::Half);
        }  

    ~Float() override = default;
        
    std::shared_ptr<Expression> clone() const override {
        return std::make_shared<Float<T>>(this->value);
    }
};

class BigInt : public NumericExpression<std::string> {
public:
    explicit BigInt(std::string value, unsigned bitWidth)
        : NumericExpression<std::string>(value), bitWidth(bitWidth) {
            rootType = Type::createPrimitiveType(Kind::Int8);
        }  

    ~BigInt() override = default;

    unsigned getBitWidth() const { return bitWidth; }

    
    std::shared_ptr<Expression> clone() const override {
        return std::make_shared<BigInt>(this->value, bitWidth);
    }

private:
    unsigned bitWidth;
};

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

    
    std::shared_ptr<Expression> clone() const override {
        return std::make_shared<PointerExpression>(
            pointee ? pointee->clone() : nullptr,
            isConst,
            isVolatile
        );
    }
};

struct RawPointerExpression : public Expression {
    size_t address;
    
    RawPointerExpression(size_t addr, std::shared_ptr<Type> type)
        : address(addr) {
        this->type = type;
        this->rootType = type;
    }

    std::string toString() const override {
        return "RawPointer(" + std::to_string(address) + ")";
    }

    std::shared_ptr<Expression> clone() const override {
        return std::make_shared<RawPointerExpression>(address, type);
    }
};

struct InvalidExpression : public Expression {
    InvalidExpression() {
        type = Type::createInvalid();
        rootType = std::make_shared<Type>(Kind::Invalid);
    }

    std::string toString() const override { return "Invalid"; }
    
    std::shared_ptr<Expression> clone() const override {
        return std::make_shared<InvalidExpression>();
    }
};

struct NullExpression : public Expression {
    bool nullCaseHandled = false;
    bool extractValue = true;
    std::shared_ptr<Type> expectedType;
    NullExpression(std::shared_ptr<Type> expectedType = nullptr) : expectedType(expectedType) {
        type = expectedType;
        rootType = Type::createNullType();
    }
    
    std::string toString() const override { return "Null expects " + type->toString(); }
    
    std::shared_ptr<Expression> clone() const override {
        return std::make_shared<NullExpression>(expectedType ? expectedType : nullptr);
    }
};

struct NullPointerExpression : public Expression {
    bool nullCaseHandled = false;
    bool extractValue = true;
    std::shared_ptr<Type> expectedType;
    NullPointerExpression(std::shared_ptr<Type> expectedType = nullptr) : expectedType(expectedType) {
        type = Type::createPointerType(expectedType);
        rootType = Type::createNullPointerType();
    }

    std::string toString() const override { return "NullPointer expects " + type->toString(); } 
    
    std::shared_ptr<Expression> clone() const override {
        return std::make_shared<NullPointerExpression>(expectedType ? expectedType : nullptr);
    }
};

struct NullableExpression : public Expression {
    bool nullCaseHandled = false;
    bool extractValue = true;
    std::shared_ptr<Expression> inner;

    NullableExpression(std::shared_ptr<Expression> expr = nullptr)
        : inner(std::move(expr)) {
        if (inner) {
            type = type->createNullableType(inner->getType());
            rootType = type;
        } else {
            type = Type::createUndefined();
            rootType = Type::createUndefined();
        }
    }

    std::string toString() const override {
        return inner ? "Nullable(" + inner->toString() + ")" : "Nullable(null)";
    }

    std::shared_ptr<Expression> clone() const override {
        auto clone = std::make_shared<NullableExpression>(inner ? inner->clone() : nullptr);
        clone->nullCaseHandled = nullCaseHandled;
        return clone;
    }

    bool isNull() const { return !inner; }
    std::shared_ptr<Expression> get() const {
        if (!inner || !nullCaseHandled) {
            throw std::runtime_error("Attempted to unwrap a nullable expression without null-checking it.");
        }
        return inner;
    }
};

struct AddressOfExpression : public Expression {
    std::shared_ptr<Expression> referent;  
    std::string variableName;

    explicit AddressOfExpression(const std::string& variableName, std::shared_ptr<Expression> referent = nullptr)
        : variableName(variableName), referent(referent) {
        
        type = Type::createPointerType(this->referent->type); 
        rootType = type;
    }

    
    std::string toString() const override {
        return "AddressOf(" + referent->toString() + ")";
    }
    
    std::shared_ptr<Expression> clone() const override {
        return std::make_shared<AddressOfExpression>(
            variableName,
            referent ? referent->clone() : nullptr
        );
    }
};

struct ReferenceExpression : public Expression {
    std::string referentName;
    std::shared_ptr<Expression>* referentPtr = nullptr;  
    std::shared_ptr<Expression> referent = nullptr;     

    
    explicit ReferenceExpression(const std::string& referentName, std::shared_ptr<Expression> referent = nullptr)
        : referentName(referentName), referent(referent) {
        type = Type::createReferenceType(this->referent->type);
    }

    explicit ReferenceExpression(const std::string& name, std::shared_ptr<Expression>* referentPtr)
        : referentName(name), referentPtr(referentPtr) {
        if (referentPtr && *referentPtr) {
            type = Type::createReferenceType((*referentPtr)->type);
        }
    }
    
    std::shared_ptr<Expression> getValue() const {
        if (referent) {
            return referent;  
        }
        return (referentPtr && *referentPtr) ? *referentPtr : nullptr;  
    }

    
    std::string toString() const override {
        if (referent) {
            return "Pointer to(" + referent->toString() + ")";
        }
        return "Reference to(" + (referentPtr && *referentPtr ? (*referentPtr)->toString() : "null") + ")";
    }

    
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

struct ReturnExpression : public Expression {
    std::shared_ptr<Expression> value;
    ReturnExpression(std::shared_ptr<Expression> value, std::shared_ptr<Type> returnType) : value(std::move(value)) {
        type = returnType;
    }
    
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
    Token op;

    BinaryExpression(std::shared_ptr<Expression> lhs, Token op, std::shared_ptr<Expression> rhs, std::shared_ptr<Type> resultType)
        : left(std::move(lhs)), right(std::move(rhs)), op(std::move(op)) {
        this->type = resultType;
    }

    std::string toString() const override {
        
        return "(" + left->toString() + " " + op.getValue() + " " + right->toString() + ")";
        
    }
    
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
    Token op;
    std::shared_ptr<Expression> operand;
    bool isPrefix;

    UnaryExpression(
                    Token op,
                    std::shared_ptr<Expression> operand,
                    std::shared_ptr<Type> resultType,
                    bool isPrefix
                )
        : op(op), operand(std::move(operand)), isPrefix(isPrefix) {
        this->type = resultType;
    }

    std::string toString() const override {
        Token opStr = op;
        if (isPrefix) {
            return  "(" + op.getValue() + (operand ? operand->toString() : "nulloperand") + ")";
        }
        return  "(" + (operand ? operand->toString() : "nulloperand") + op.getValue() + ")";
    }
    
    std::shared_ptr<Expression> clone() const override {
        return std::make_shared<UnaryExpression>(
            op,
            operand ? operand->clone() : nullptr,
            type ? type->clone() : nullptr,
            isPrefix
        );
    }
};

struct BlockExpression : public Expression {
    bool isGlobal = true;
    std::vector<std::shared_ptr<Expression>> values;  

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

    
    std::shared_ptr<Expression> clone() const override {
        std::vector<std::shared_ptr<Expression>> clonedValues;
        for (const auto& val : values) {
            clonedValues.push_back(val ? val->clone() : nullptr);
        }
        return std::make_shared<BlockExpression>(clonedValues);
    }
};

struct AggregateExpression : public virtual Expression {
    ~AggregateExpression() = default;
    std::string toString() const override { return "Aggregate"; }
};

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

struct ModuleExpression : 
public AggregateExpression {
    std::vector<std::shared_ptr<ModuleMemberExpression>> members;

    ModuleExpression(const std::string& moduleName, const std::vector<std::shared_ptr<ModuleMemberExpression>>& members = {})
        : members(members) {
        this->name = moduleName;
        this->type = Type::createInvalid();  
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

    std::vector<std::shared_ptr<MemberExpression>> memberExpressions;  

    InstanceExpression(
        const std::string& baseName,
        const std::string& instanceName,
        const std::vector<std::shared_ptr<MemberExpression>>& memberExpressions = {}
    ) : baseName(baseName),
        instanceName(instanceName),
        memberExpressions(memberExpressions) {

        this->instanceType = std::make_shared<UserDefinedType>(baseName);
        this->type = instanceType; 
    }

    std::string toString() const override {
        return "Instance<" + baseName + "> named " + instanceName;
    }

    
    std::shared_ptr<Expression> getField(const std::string& name) const {
        for (const auto& member : memberExpressions) {
            if (member->getType()->getParameterName() == name) {
                return member;
            }
        }
        return nullptr; 
    }

    
    bool setField(const std::string& name, const std::shared_ptr<Expression>& newValue) {
        for (auto& member : memberExpressions) {
            if (member->getType()->getParameterName() == name) {
                member->value = newValue;  
                return true;
            }
        }
        return false; 
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


struct AccessExpression : public Expression {
    bool isInternal = false;
    std::shared_ptr<Expression> expr;                
    std::string member;                              
    int index = -1;                                   
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

    
    void addEntry(int value, const std::string& valueName, std::shared_ptr<Expression> expression) {
        enumerators[valueName] = value;
        expressionMap[valueName] = expression;  
    }

    
    int get(const std::string& enumeration) const {
        auto it = enumerators.find(enumeration);
        if (it != enumerators.end()) return it->second;
        console.error("Enum '" + enumName + "' does not have an entry " + enumeration);
        return -9999999;
    }

    
    std::string getName(int value) const {
        for (const auto& [name, val] : enumerators)
            if (val == value) return name;
        return "";
    }

    
    std::shared_ptr<Expression> getExpression(const std::string& valueName) const {
        auto it = expressionMap.find(valueName);
        if (it != expressionMap.end()) {
            return it->second;
        }
        console.error("Enum expression for '" + enumName + "' does not have an entry " + valueName);
        return nullptr;
    }

    
    std::shared_ptr<Expression> clone() const override {
        auto copy = std::make_shared<EnumExpression>(enumName, hasLookup, isEnumClass);
        copy->enumerators = enumerators;
        copy->expressionMap = expressionMap;  
        return copy;
    }

    
    std::string toString() const override {
        return (isEnumClass ? "enum class " : "enum ") + enumName;
    }

    
    std::string enumName;
    bool hasLookup = false;
    bool isEnumClass = false;

    
    std::unordered_map<std::string, int> enumerators;  
    std::unordered_map<std::string, std::shared_ptr<Expression>> expressionMap;  
};

template <typename T>
class StringExpression : public Primitive<T> {
public:
    StringExpression(T value)
        : Primitive<T>(value) {}

    virtual ~StringExpression() = default;
    
    std::shared_ptr<Expression> clone() const override {
        return std::make_shared<StringExpression<T>>(this->value);
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
        return "FixedArray<" + (elementType ? elementType->toString() : "unknown") + ">";
    }

    std::string toString() const override {
        std::string result = "[";
        for (size_t i = 0; i < elements.size(); ++i) {
            result += elements[i]->toString();
            if (i < elements.size() - 1) result += ", ";
        }
        return result + "]";
    }
    
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
    
    std::vector<std::shared_ptr<Expression>> conditions;  
    std::vector<std::shared_ptr<Expression>> bodies;      
    std::shared_ptr<Expression> elseBody;                 

    IfExpression(
        std::vector<std::shared_ptr<Expression>> conditions, 
        std::vector<std::shared_ptr<Expression>> bodies,
        std::shared_ptr<Expression> elseBody = nullptr
    ) : conditions(std::move(conditions)),
        bodies(std::move(bodies)),
        elseBody(std::move(elseBody)) {

        this->type = Type::createInvalid(); 
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