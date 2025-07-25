#include <omniscript/Statements/EntityStatements.h>
#include <omniscript/Statements/LiteralStatements.h>

#include <omniscript/Core.h>
#include <omniscript/utils.h>
#include <omniscript/omniscript_pch.h>
#include <omniscript/Statement.h>
#include <omniscript/Symboltable.h>

#include <omniscript/Expressions/EntityExpressions.h>

std::shared_ptr<Omniscript::Expression> EnumValue::express(SymbolTableType scope) {
    Omniscript::setPosition(pos.line, pos.col, pos.filePath);
    auto integer = std::make_shared<IntegerLiteral>(valueIndex)->express(scope);
    integer->setPosition(getPosition());
    return integer;
}

std::shared_ptr<Omniscript::Expression> EnumConstructor::express(SymbolTableType scope) {
    Omniscript::setPosition(pos.line, pos.col, pos.filePath);
    auto expr = std::make_shared<Omniscript::EnumExpression>(name, hasLookup, isEnumClass);
    
    for (const auto& val : values) {
        expr->addEntry(val->getIndex(), val->getName(), val->express(scope));
    }

    expr->setPosition(getPosition());
    return expr;
}