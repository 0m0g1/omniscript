#include <omniscript/Statement.h>
#include <omniscript/Statements/TypeStatements.h>
#include <omniscript/Expressions/TypeExpressions.h>

#include <omniscript/Core.h>
#include <omniscript/utils.h>
#include <omniscript/omniscript_pch.h>
#include <omniscript/Symboltable.h>

std::shared_ptr<Omniscript::Expression> TypeDeclaration::express(SymbolTableType scope) {
    bool isAliasingOtherType = false;
    std::string originalTypeName;
    std::shared_ptr<Omniscript::Type> originalType;

    if (auto storedType = scope->getType(name)) {
        if (!storedType->isInvalid()) {
            console.error("Type '" + name + "' already exists in scope '" + scope->getName() + "' with type '" + storedType->toString() + "'.");
        }
    }

    if (type->isUnresolved()) {
        if (auto unresolved = std::dynamic_pointer_cast<Omniscript::UnresolvedType>(type)) {
            originalType = scope->getType(unresolved->joinedTypeString);
            if (originalType->isInvalid()) {
                console.error("Cannot alias invalid type '" + unresolved->joinedTypeString + "' as '" + name + "'.");
            }
            isAliasingOtherType = true;
            originalTypeName = unresolved->joinedTypeString;
        }
    }

    if (!type) {
        scope->addType(name, Omniscript::Type::createInvalid());
    } else {
        scope->addType(name, type);
    }
    
    auto typeDeclExpr = std::make_shared<Omniscript::TypeDeclarationExpression>(name, type);
    if (isAliasingOtherType) {
        typeDeclExpr->setIsAliasing(originalTypeName);
    }
    typeDeclExpr->setPosition(getPosition());
    return typeDeclExpr;
}