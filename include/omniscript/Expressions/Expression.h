#pragma once

#include <omniscript/Tokens.h>
#include <omniscript/Core.h>
#include <omniscript/omniscript_pch.h>
#include <omniscript/Types/BaseType.h>
#include <omniscript/MemberModifiers.h>

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
    
    inline void setStartPosition(const filePosition& pos) {
        span.start = pos;
    }

    inline void setEndPosition(const filePosition& pos) {
        span.end = pos;
    }

    inline void setSpan(const FileSpan& s) {
        span = s;
    }

    inline const FileSpan& getSpan() const {
        return span;
    }

    protected:
        FileSpan span;
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
