#include <omniscript/Statement.h>
#include <omniscript/Statements/TypeStatements.h>
#include <omniscript/Expressions/TypeExpressions.h>

#include <omniscript/Core.h>
#include <omniscript/utils.h>
#include <omniscript/omniscript_pch.h>
#include <omniscript/Symboltable.h>

#include <omniscript/Expressions/BlockExpression.h>

namespace Omniscript {

std::shared_ptr<Expression> TypeDeclaration::express(SymbolTableType scope) {
    bool isAliasingOtherType = false;
    std::string originalTypeName;
    std::shared_ptr<Type> originalType;

    if (auto storedType = scope->getType(name)) {
        if (!storedType->isInvalid()) {
            std::string suggestion = Console::formatString(
                "To resolve this:\n"
                "1. Use a different type name instead of '%s'\n"
                "2. Check for duplicate type declarations\n"
                "3. Verify scope hierarchy",
                name.c_str()
            );
            console.reportError(
                Console::TYPE_ERROR,
                Console::formatString("Type '%s' already exists in scope '%s' with type '%s'",
                                 name.c_str(), scope->getName().c_str(), storedType->toString().c_str()),
                suggestion,
                getSpan()
            );
            return nullptr;
        }
    }

    if (type->isUnresolved()) {
        if (auto unresolved = std::dynamic_pointer_cast<UnresolvedType>(type)) {
            originalType = scope->getType(unresolved->joinedTypeString);
            if (!originalType) {
                std::string suggestion = Console::formatString(
                    "To resolve this:\n"
                    "1. Verify type '%s' is defined in scope '%s'\n"
                    "2. Check for correct namespace imports\n"
                    "3. Ensure type is declared before use",
                    unresolved->joinedTypeString.c_str(), scope->getName().c_str()
                );
                console.reportError(
                    Console::TYPE_ERROR,
                    Console::formatString("Type '%s' does not exist in scope '%s'",
                                     unresolved->joinedTypeString.c_str(), scope->getName().c_str()),
                    suggestion,
                    getSpan()
                );
                return nullptr;
            } else if (originalType->isInvalid()) {
                std::string suggestion = Console::formatString(
                    "To resolve this:\n"
                    "1. Ensure type '%s' is valid before aliasing\n"
                    "2. Check original type declaration\n"
                    "3. Verify type initialization",
                    unresolved->joinedTypeString.c_str()
                );
                console.reportError(
                    Console::TYPE_ERROR,
                    Console::formatString("Cannot alias invalid type '%s' as '%s'",
                                     unresolved->joinedTypeString.c_str(), name.c_str()),
                    suggestion,
                    getSpan()
                );
                return nullptr;
            }
            isAliasingOtherType = true;
            originalTypeName = unresolved->joinedTypeString;
        }
    }

    std::shared_ptr<Expression> typeDeclExpr;
    if (!type) {
        scope->addType(name, Type::createInvalid());
    } else {
        if (auto funcType = std::dynamic_pointer_cast<FunctionType>(type)) {
            funcType->functionName = name;
        }
        if (type->isFunction()) {
            auto typePointer = std::make_shared<PointerType>(type);
            auto pointerTypeDecl = std::make_shared<TypeDeclarationExpression>(name, typePointer);
            auto typeDecl = std::make_shared<TypeDeclarationExpression>("*" + name, type);
            if (isAliasingOtherType) {
                typeDecl->setIsAliasing(originalTypeName);
                pointerTypeDecl->setIsAliasing(originalTypeName);
            }
            typeDecl->setSpan(this->getSpan());
            pointerTypeDecl->setSpan(this->getSpan());
            std::vector<std::shared_ptr<Expression>> declarations = {pointerTypeDecl, typeDecl};
            auto block = std::make_shared<BlockExpression>(declarations);
            block->setSpan(this->getSpan());
            typeDeclExpr = block;

            scope->addType(name, typePointer);
            scope->addType("*" + name, type);
        } else {
            auto typeDecl = std::make_shared<TypeDeclarationExpression>(name, type);
            if (isAliasingOtherType) {
                typeDecl->setIsAliasing(originalTypeName);
            }
            typeDecl->setSpan(this->getSpan());
            typeDeclExpr = typeDecl;
            scope->addType(name, type);
        }
    }
    DEBUG_LOG("Declared type: '" + type->toString() + "' in scope as '" + name + "'.");

    return typeDeclExpr;
}

} // namespace Omniscript
