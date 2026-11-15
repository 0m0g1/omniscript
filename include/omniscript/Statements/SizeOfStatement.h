#pragma once
// Place next to the other Statements headers (e.g. include/omniscript/Statements/SizeOfStatement.h).
#include <omniscript/Statements/Statement.h>
#include <omniscript/Statements/AccessStatements.h>
#include <omniscript/Statements/CallableStatement.h>
#include <omniscript/Statements/ExpressionStatements.h>
#include <omniscript/Statements/AssignmentAndGetterStatements.h>
#include <omniscript/Core.h>
#include <omniscript/omniscript_pch.h>
#include <omniscript/Types/Types.h>
#include <omniscript/Expressions/LiteralExpressions.h>

namespace Omniscript {

// `sizeof(T)`: the measured type lives in `measuredType`, NOT in the TypedStatement type. The
// TypedStatement type is overwritten with the declared type of the target (`let c: u64 = sizeof(S)`
// made it UInt64, so every struct measured 8). Generic instantiation substitutes `measuredType`
// explicitly (see GenericInstantiator::substituteInStatement); the size is taken at codegen.
class SizeOfStatement : public TypedStatement, public NamedStatement {
public:
    std::shared_ptr<Type> measuredType;

    std::shared_ptr<Statement> evaluate(SymbolTableType) override { return nullptr; }
    std::shared_ptr<Expression> express(SymbolTableType) override {
        auto e = std::make_shared<SizeOfExpression>(measuredType);
        e->setSpan(getSpan());
        return e;
    }
    std::string toString() const override { return "SizeOf(" + (measuredType ? measuredType->toString() : std::string("?")) + ")"; }
    std::string formatError(const std::string& msg) const override { return "Error in '" + toString() + "'.\n" + msg; }
    std::shared_ptr<Statement> clone() const override {
        auto c = std::make_shared<SizeOfStatement>();
        c->measuredType = measuredType;
        c->setType(getType());
        c->setRootType(getRootType());
        c->setSpan(getSpan());
        return c;
    }
};

} // namespace Omniscript
