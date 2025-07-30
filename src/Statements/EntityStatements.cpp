#include <omniscript/Statements/EntityStatements.h>
#include <omniscript/Statements/LiteralStatements.h>

#include <omniscript/Core.h>
#include <omniscript/utils.h>
#include <omniscript/omniscript_pch.h>
#include <omniscript/Statement.h>
#include <omniscript/Symboltable.h>

#include <omniscript/Expressions/EntityExpressions.h>

namespace Omniscript {

std::shared_ptr<Expression> EnumValue::express(SymbolTableType scope) {
    setSpanFromPosition(span.start.line, span.start.col, span.start.filePath);
    auto integer = std::make_shared<IntegerLiteral>(valueIndex)->express(scope);
    integer->setSpan(this->getSpan());
    return integer;
}

std::shared_ptr<Expression> EnumConstructor::express(SymbolTableType scope) {
    setSpanFromPosition(span.start.line, span.start.col, span.start.filePath);
    auto expr = std::make_shared<EnumExpression>(name, hasLookup, isEnumClass);
    
    for (const auto& val : values) {
        expr->addEntry(val->getIndex(), val->getName(), val->express(scope));
    }

    expr->setSpan(this->getSpan());
    return expr;
}

} // namespace Omniscript
