#pragma once
#include <omniscript/Expression.h>

namespace Omniscript {
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
}
