#include <omniscript/engine/Statements/EntityStatements.h>
#include <omniscript/engine/Statements/LiteralStatements.h>

#include <omniscript/Core.h>
#include <omniscript/utils.h>
#include <omniscript/omniscript_pch.h>
#include <omniscript/engine/Statement.h>
#include <omniscript/engine/Symboltable.h>


std::shared_ptr<Omniscript::Expression> EnumValue::express(SymbolTableType scope) {
    Omniscript::setPosition(pos.line, pos.col, pos.filePath);
    return std::make_shared<IntegerLiteral>(valueIndex)->express(scope);
}

std::shared_ptr<Omniscript::Expression> EnumConstructor::express(SymbolTableType scope) {
    Omniscript::setPosition(pos.line, pos.col, pos.filePath);
    auto expr = std::make_shared<Omniscript::EnumExpression>(name, hasLookup, isEnumClass);
    
    for (const auto& val : values) {
        expr->addEntry(val->getIndex(), val->getName(), val->express(scope));
    }

    return expr;
}