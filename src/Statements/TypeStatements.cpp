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
            if (!originalType) {
                console.error("Type '" + unresolved->joinedTypeString + "' does not exist in scope '" + scope->getName() + "'.");
            } else if (originalType->isInvalid()) {
                console.error("Cannot alias invalid type '" + unresolved->joinedTypeString + "' as '" + name + "'.");
            }
            isAliasingOtherType = true;
            originalTypeName = unresolved->joinedTypeString;
        }
    }

    std::shared_ptr<Omniscript::Expression> typeDeclExpr;
    if (!type) {
        scope->addType(name, Omniscript::Type::createInvalid());
    } else {
        if (auto funcType = std::dynamic_pointer_cast<Omniscript::FunctionType>(type)) {
            funcType->functionName = name;
        }
        if (type->isFunction()) {
            auto typePointer = std::make_shared<Omniscript::PointerType>(type);
            auto pointerTypeDecl = std::make_shared<Omniscript::TypeDeclarationExpression>(name, typePointer);
            auto typeDecl = std::make_shared<Omniscript::TypeDeclarationExpression>("*" + name, type);
            if (isAliasingOtherType) {
                typeDecl->setIsAliasing(originalTypeName);
                pointerTypeDecl->setIsAliasing(originalTypeName);
            }
            typeDecl->setPosition(getPosition());
            pointerTypeDecl->setPosition(getPosition());
            std::vector<std::shared_ptr<Omniscript::Expression>> declarations = {pointerTypeDecl, typeDecl};
            auto block = std::make_shared<Omniscript::BlockExpression>(declarations);
            block->setPosition(getPosition());
            typeDeclExpr = block;

            scope->addType(name, typePointer);
            scope->addType("*" + name, type);
        } else {
            auto typeDecl = std::make_shared<Omniscript::TypeDeclarationExpression>(name, type);
            if (isAliasingOtherType) {
                typeDecl->setIsAliasing(originalTypeName);
            }
            typeDecl->setPosition(getPosition());
            typeDeclExpr = typeDecl;
            scope->addType(name, type);
        }
    }
    DEBUG_LOG("Declared type: '" + type->toString() + "' in scope as '" + name + "'.");


    return typeDeclExpr;
}