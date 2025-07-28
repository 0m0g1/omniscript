#include <omniscript/Statement.h>
#include <omniscript/Statements/AccessStatements.h>
#include <omniscript/Statements/CallableStatement.h>
#include <omniscript/Statements/AssignmentAndGetterStatements.h>

#include <omniscript/Expressions/ClassExpression.h>
#include <omniscript/Expressions/StructExpression.h>
#include <omniscript/Expressions/AccessExpressions.h>
#include <omniscript/Expressions/AggregateExpressions.h>
#include <omniscript/Expressions/AssignmentExpression.h>
#include <omniscript/Expressions/VariableAccessExpression.h>

#include <omniscript/Core.h>
#include <omniscript/utils.h>
#include <omniscript/omniscript_pch.h>
#include <omniscript/Symboltable.h>

void ContextAwareStatement::validateAccessiblity(std::string baseTypeName, std::string memberName, SymbolTableType scope) {
    // Ensure base type is a class
    auto aggregateExpr = std::dynamic_pointer_cast<Omniscript::AggregateExpression>(scope->get(baseTypeName));
    if (aggregateExpr) {
        auto structExpr = std::dynamic_pointer_cast<Omniscript::StructExpression>(scope->get(baseTypeName));
        auto classExpr = std::dynamic_pointer_cast<Omniscript::ClassExpression>(scope->get(baseTypeName));
        auto moduleExpr = std::dynamic_pointer_cast<Omniscript::ModuleExpression>(scope->get(baseTypeName));
        
        if (auto type = scope->get(baseTypeName)) {
            DEBUG_LOG("Accessing a member of type '" + type->toString() + "'.");
        } else {
            DEBUG_LOG("No type defined");
        }

        std::shared_ptr<Omniscript::MemberExpression> member;

        DEBUG_LOG(getContextAsString());
        if (structExpr) {
            // member = structExpr->getMember(memberName);
        } else if (classExpr) {
            member = classExpr->getMember(memberName);
        } else if (moduleExpr) {
            member = moduleExpr->getMember(memberName);
        } else {
            console.error("Type '" + baseTypeName + "' is not an aggregate Type (class, struct, module).");
            return;
        }

        if (!member) {
            console.error("Member '" + memberName + "' not found in type '" + baseTypeName + "'.");
            return;
        }
    
        if (!structExpr && !member->isPublic() && member->isPrivate() && !containsContext(classExpr->getName())) {
            if (classExpr) {
                console.error("Cannot access private member '" + memberName + "' of class '" + classExpr->getName() + "'.");
            } else if (moduleExpr) {
                console.error("Cannot access private member '" + memberName + "' of module '" + moduleExpr->getName() + "'.");
            } 
            return;
        }
    }
}

void Access::verifyMemberAccessibility() {

}

std::shared_ptr<Omniscript::Expression> Dereference::express(SymbolTableType scope) {
    Omniscript::setSpanFromPosition(span.start.line, span.start.col, span.start.filePath);
    // Evaluate the pointer expression recursively
    auto pointerExpr = pointer->express(scope);
    if (!pointerExpr) {
        console.error("Failed to evaluate pointer expression for dereference");
        return nullptr;
    }

    auto pointerType = pointerExpr->getType();
    if (!pointerType->isPointer()) {
        console.error("Dereference requires a pointer type");
        return nullptr;
    }

    auto baseType = pointerType->getBasePointeeType();
    setType(baseType);

    // Handle member access if present
    auto userType = std::dynamic_pointer_cast<Omniscript::UserDefinedType>(baseType);
    if (!userType) {
        console.error("Cannot access members of non-user-defined type");
        return nullptr;
    }

    int memberIndex;
    std::shared_ptr<Omniscript::Type> currentType = baseType;

    // Traverse member path
    bool found = false;
    for (int i = 0; i < userType->paramTypes.size(); ++i) {
        if (userType->paramTypes[i]->getParameterName() == memberName) {
            memberIndex = i;
            currentType = userType->paramTypes[i];
            found = true;
            break;
        }
    }

    if (!found) {
        console.error("Member '" + memberName + "' not found in type '" + userType->toString() + "'");
        return nullptr;
    }

    // Descend into nested type if needed
    userType = std::dynamic_pointer_cast<Omniscript::UserDefinedType>(
        currentType->isPointer() ? currentType->getBasePointeeType() : currentType
    );

    if (!userType) {
        console.error("Cannot traverse non-user-defined member '" + memberName + "'");
        return nullptr;
    }

    setType(currentType);

    // Handle assignment if present
    std::shared_ptr<Omniscript::Expression> valueExpr = nullptr;
    if (assignmentValue) {
        extendContextOf(assignmentValue);
        valueExpr = assignmentValue->express(scope);
        if (!valueExpr) {
            console.error("Failed to evaluate assignment expression");
            return nullptr;
        }
    }

    auto result = std::make_shared<Omniscript::DereferenceExpression>(
        pointerExpr,
        valueExpr,
        getType()
    );

    result->type = type;
    result->setSpan(getSpan());
    return result;
}
