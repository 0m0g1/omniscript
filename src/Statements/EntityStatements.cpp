#include <omniscript/Statements/EntityStatements.h>
#include <omniscript/Statements/LiteralStatements.h>

#include <omniscript/Core.h>
#include <omniscript/utils.h>
#include <omniscript/omniscript_pch.h>
#include <omniscript/Statements/Statement.h>
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

    // Same integer type the enumerators were expressed with (int32).
    expr->underlyingType = std::make_shared<IntegerLiteral>(0)->express(scope)->getType();
    expr->type = expr->underlyingType;

    // Register like a struct does, so `Color` resolves as a symbol and `let c: Color` as a type.
    scope->addType(name, expr->underlyingType);
    scope->set(name, expr);

    expr->setSpan(this->getSpan());
    return expr;
}

} // namespace Omniscript
