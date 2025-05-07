#include <omniscript/Core.h>
#include <omniscript/utils.h>
#include <omniscript/engine/Statement.h>
#include <omniscript/engine/Symboltable.h>
#include <omniscript/omniscript_pch.h>
#include <omniscript/utils.h>


// ============================== Entities  ============================== //

std::shared_ptr<Omniscript::Expression> EnumValue::express(SymbolTableType scope) {
    return std::make_shared<IntegerLiteral>(valueIndex)->express(scope);
}

std::shared_ptr<Omniscript::Expression> EnumConstructor::express(SymbolTableType scope) {
    auto expr = std::make_shared<Omniscript::EnumExpression>(name, hasLookup, isEnumClass);
    
    for (const auto& val : values) {
        expr->addEntry(val->getIndex(), val->getName(), val->express(scope));
    }

    return expr;
}