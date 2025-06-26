<<<<<<< HEAD:src/Statements/Entities.cpp
#include <omniscript/Statements/EntityStatements.h>
#include <omniscript/Statements/LiteralStatements.h>

#include <omniscript/Core.h>
#include <omniscript/utils.h>
#include <omniscript/omniscript_pch.h>
#include <omniscript/Statement.h>
#include <omniscript/Symboltable.h>
=======
#include <omniscript/engine/Statements/EntityStatements.h>
#include <omniscript/engine/Statements/LiteralStatements.h>

#include <omniscript/engine/Core.h>
#include <omniscript/utils.h>
#include <omniscript/omniscript_pch.h>
#include <omniscript/engine/Statement.h>
#include <omniscript/engine/Symboltable.h>
>>>>>>> 7ccebff50dd27e70cffd4d578dcb358f4c9e1613:src/engine/Statements/Entities.cpp


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