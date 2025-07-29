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
    auto aggregateExpr = std::dynamic_pointer_cast<Omniscript::AggregateExpression>(scope->get(baseTypeName));
    if (aggregateExpr) {
        auto structExpr = std::dynamic_pointer_cast<Omniscript::StructExpression>(scope->get(baseTypeName));
        auto classExpr = std::dynamic_pointer_cast<Omniscript::ClassExpression>(scope->get(baseTypeName));
        auto moduleExpr = std::dynamic_pointer_cast<Omniscript::ModuleExpression>(scope->get(baseTypeName));
        
        if (auto type = scope->get(baseTypeName)) {
            DEBUG_LOG("Accessing a member of type '" + type->toString() + "'.");
        }

        std::shared_ptr<Omniscript::MemberExpression> member;

        if (structExpr) {
            // struct members are always public
        } else if (classExpr) {
            member = classExpr->getMember(memberName);
            
            if (!member) {
                std::string availableMembers;
                for (const auto& m : classExpr->members) {
                    availableMembers += " - " + m->getName() + "\n";
                }
                
                std::string suggestion = "To fix this:\n"
                                       "1. Check spelling of '" + memberName + "'\n"
                                       "2. Available members:\n" + availableMembers +
                                       "3. If inherited, check parent class definitions";
                console.reportError(
                    Omniscript::Console::TYPE_ERROR,
                    "Member '" + memberName + "' not found in class '" + baseTypeName + "'",
                    suggestion,
                    getSpan()
                );
                return;
            }
        } else if (moduleExpr) {
            member = moduleExpr->getMember(memberName);
            
            if (!member) {
                std::string availableMembers;
                for (const auto& m : moduleExpr->members) {
                    availableMembers += " - " + m->getName() + "\n";
                }
                
                std::string suggestion = "To fix this:\n"
                                       "1. Check spelling of '" + memberName + "'\n"
                                       "2. Available members:\n" + availableMembers +
                                       "3. Check if member is exported from module";
                console.reportError(
                    Omniscript::Console::TYPE_ERROR,
                    "Member '" + memberName + "' not found in module '" + baseTypeName + "'",
                    suggestion,
                    getSpan()
                );
                return;
            }
        } else {
            std::string suggestion = "To fix this:\n"
                                   "1. Check if '" + baseTypeName + "' is a class, struct, or module\n"
                                   "2. Verify the type is imported/available\n"
                                   "3. For primitive types, use dot notation only for built-in methods";
            console.reportError(
                Omniscript::Console::TYPE_ERROR,
                "'" + baseTypeName + "' is not a valid aggregate type (class/struct/module)",
                suggestion,
                getSpan()
            );
            return;
        }
    
        if (!structExpr && !member->isPublic() && !containsContext(classExpr->getName())) {
            std::string suggestion = "To fix this:\n"
                                   "1. Make member public if appropriate\n"
                                   "2. Add public getter/setter methods\n"
                                   "3. Move code to same context\n"
                                   "4. Consider friend declarations if needed";
            console.reportError(
                Omniscript::Console::TYPE_ERROR,
                "Cannot access private member '" + memberName + "' of " + 
                (classExpr ? "class" : "module") + " '" + baseTypeName + "'",
                suggestion,
                getSpan()
            );
        }
    }
}

std::shared_ptr<Omniscript::Expression> Dereference::express(SymbolTableType scope) {
    Omniscript::setSpanFromPosition(span.start.line, span.start.col, span.start.filePath);
    
    // Pointer expression evaluation
    auto pointerExpr = pointer->express(scope);
    if (!pointerExpr) {
        std::string suggestion = "To resolve this:\n"
                               "1. Check pointer expression validity\n"
                               "2. Verify variable initialization\n"
                               "3. Add debug output before this line";
        console.reportError(
            Omniscript::Console::RUNTIME_ERROR,
            "Invalid pointer expression in dereference operation",
            suggestion,
            getSpan()
        );
        return nullptr;
    }

    // Pointer type verification
    auto pointerType = pointerExpr->getType();
    if (!pointerType->isPointer()) {
        std::string suggestion = Omniscript::Console::formatString(
            "To resolve this:\n"
            "1. Add address-of operator: `&%s`\n"
            "2. Declare as pointer: `%s*`\n"
            "3. Check variable declaration",
            pointer->toString().c_str(),
            pointerType->toString().c_str()
        );
        console.reportError(
            Omniscript::Console::TYPE_ERROR,
            Omniscript::Console::formatString("Cannot dereference non-pointer type '%s'", pointerType->toString().c_str()),
            suggestion,
            getSpan()
        );
        return nullptr;
    }

    // Member access handling
    auto baseType = pointerType->getBasePointeeType();
    setType(baseType);

    auto userType = std::dynamic_pointer_cast<Omniscript::UserDefinedType>(baseType);
    if (!userType) {
        std::string suggestion = "To resolve this:\n"
                               "1. Check type definition\n"
                               "2. For built-in types, use direct access\n"
                               "3. Verify type imports";
        console.reportError(
            Omniscript::Console::TYPE_ERROR,
            Omniscript::Console::formatString("Cannot access members of type '%s'", baseType->toString().c_str()),
            suggestion,
            getSpan()
        );
        return nullptr;
    }

    // Member existence check
    bool memberFound = false;
    for (const auto& param : userType->paramTypes) {
        if (param->getParameterName() == memberName) {
            memberFound = true;
            break;
        }
    }

    if (!memberFound) {
        std::string availableMembers;
        for (const auto& param : userType->paramTypes) {
            availableMembers += " - " + param->getParameterName() + "\n";
        }
        
        std::string suggestion = Omniscript::Console::formatString(
            "To resolve this:\n"
            "1. Check member name spelling\n"
            "2. Available members:\n%s\n"
            "3. Verify inheritance chain",
            availableMembers.c_str()
        );
        
        console.reportError(
            Omniscript::Console::TYPE_ERROR,
            Omniscript::Console::formatString("Member '%s' not found in '%s'", memberName.c_str(), userType->toString().c_str()),
            suggestion,
            getSpan()
        );
        return nullptr;
    }

    // Assignment value handling
    if (assignmentValue) {
        extendContextOf(assignmentValue);
        auto valueExpr = assignmentValue->express(scope);
        if (!valueExpr) {
            std::string suggestion = "To resolve this:\n"
                                   "1. Check right-hand expression\n"
                                   "2. Verify type compatibility\n"
                                   "3. Add debug output for the value";
            console.reportError(
                Omniscript::Console::RUNTIME_ERROR,
                "Invalid assignment value in member access",
                suggestion,
                assignmentValue->getSpan()
            );
            return nullptr;
        }

        if (!Omniscript::Type::isSameOrCastableTo(getType(), valueExpr->getType())) {
            std::string suggestion = Omniscript::Console::formatString(
                "To resolve this:\n"
                "1. Check type requirements\n"
                "2. Available conversions:\n"
                " - Explicit cast: `(%s)value`\n"
                " - Conversion method: `value.to_%s()`\n"
                "3. Verify source type implements required traits",
                getType()->toString().c_str(),
                getType()->toString().c_str()
            );
            
            console.reportError(
                Omniscript::Console::TYPE_ERROR,
                Omniscript::Console::formatString("Cannot assign '%s' to '%s'", 
                    valueExpr->getType()->toString().c_str(), 
                    getType()->toString().c_str()),
                suggestion,
                assignmentValue->getSpan()
            );
            return nullptr;
        }
    }

    // Success case
    auto result = std::make_shared<Omniscript::DereferenceExpression>(
        pointerExpr,
        assignmentValue ? assignmentValue->express(scope) : nullptr,
        getType()
    );

    result->type = type;
    result->setSpan(getSpan());
    return result;
}
