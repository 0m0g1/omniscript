#pragma once
#include <omniscript/Expression.h>

namespace Omniscript {
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
        int index = -1,
        std::shared_ptr<Type> memberType = nullptr,
        std::shared_ptr<Expression> assignmentValue = nullptr
    ) {
        expr = parentExpr;
        this->member = member;
        this->index = index;
        this->type = memberType;
        this->assignmentValue = assignmentValue;
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
        int index = -1,
        std::shared_ptr<Type> memberType = nullptr,
        std::shared_ptr<Expression> assignmentValue = nullptr        
    ) : valueExpr(val), type(resultType)
    {
        expr = pointerExpr;
        this->member = member;
        this->index = index;
        this->type = memberType;
        this->assignmentValue = assignmentValue;    
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
        std::shared_ptr<Expression> indexExpr,
        std::shared_ptr<Type> memberType = nullptr,
        std::shared_ptr<Expression> assignmentValue = nullptr
    ) : indexExpr(indexExpr) {
        expr = containerExpr;
        this->type = memberType;
        this->assignmentValue = assignmentValue;
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
}