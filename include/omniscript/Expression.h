#pragma once
#ifndef Expression_H
#define Expression_H

#include <omniscript/tokens.h>
#include <omniscript/Core.h>
#include <omniscript/omniscript_pch.h>
#include <omniscript/debuggingtools/console.h>
#include <omniscript/Types.h>

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
    
    inline void setPosition(Token startToken) {
        pos.line = startToken.getLine();
        pos.col = startToken.getColumn();
        pos.fileName = startToken.getFilePath();
        pos.filePath = startToken.getFilePath();
    }

    inline void setPosition(int line, int column, const std::string& file, const std::string& path) {
        pos.line = line;
        pos.col = column;
        pos.fileName = file;
        pos.filePath = path;
    }

    inline void setPosition(const Omniscript::filePosition& position) {
        pos = position;
    }

    inline Omniscript::filePosition getPosition() const {
        return pos;
    }

    protected:
        Omniscript::filePosition pos;
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
        : name(typeName), actualType(type) {
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

}

#endif