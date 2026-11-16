#pragma once
#include <omniscript/Statements/Statement.h>
#include <omniscript/Statements/CallableStatement.h>
#include <omniscript/Statements/StructConstructorStatement.h>
#include <cctype>
#include <omniscript/Types/Types.h>
#include <omniscript/Symboltable.h>
#include <omniscript/Core.h>

namespace Omniscript {

// `{ field: value, ... }` / `{ v1, v2 }` with no type written. The struct type is the expected type,
// pushed in by the surrounding construct before express() runs (a typed `let`, a field initialiser,
// a parameter). Once the type is known this is exactly `Type { ... }`, so it is expressed through
// ObjectConstructorStatement, which applies the field initialisers.
class ObjectLiteralStatement : public TypedStatement, public NamedStatement {
public:
    // ArgumentStatement (named field) or a plain value (positional, declaration order).
    std::vector<std::shared_ptr<Statement>> fields;

    explicit ObjectLiteralStatement(std::vector<std::shared_ptr<Statement>> f = {}) : fields(std::move(f)) {}

    std::shared_ptr<Statement> evaluate(SymbolTableType) override { return nullptr; }

    std::shared_ptr<Expression> express(SymbolTableType scope) override {
        if (!type || type->isInvalid()) {
            type = inferAnonymousStruct(scope);
            if (!type) return nullptr;
        }
        if (type->isPointer()) {
            console.reportError(
                Console::TYPE_ERROR,
                "A '{ ... }' literal builds a struct value, but the target type is '" + type->toString() + "'.",
                "To resolve this:\n1. Assign it to a struct-typed target\n2. Or take its address after building it",
                getSpan());
            return nullptr;
        }

        // `Type { f: v }` applies field values through the struct's callable (named/positional
        // arguments). ObjectConstructorStatement ignores them when the type has no constructor.
        auto args = fields;
        auto call = std::make_shared<Call>(type->getName(), args);
        call->setSpan(getSpan());
        auto result = call->express(scope);
        if (result && result->getType()) {
            setType(result->getType());
            setRootType(result->getType());
        }
        return result;
    }

    std::string toString() const override {
        return "ObjectLiteral(" + (type ? type->toString() : std::string("?")) + ")";
    }
    std::string formatError(const std::string& msg) const override {
        return "Error in '" + toString() + "'.\n" + msg;
    }
    std::shared_ptr<Statement> clone() const override {
        std::vector<std::shared_ptr<Statement>> copied;
        copied.reserve(fields.size());
        for (const auto& f : fields) copied.push_back(f ? f->clone() : nullptr);
        auto c = std::make_shared<ObjectLiteralStatement>(copied);
        c->setType(getType());
        c->setRootType(getRootType());
        c->setSpan(getSpan());
        return c;
    }
private:
    static std::string sanitize(const std::string& in) {
        std::string out;
        for (char c : in) out += std::isalnum(static_cast<unsigned char>(c)) ? c : '_';
        return out;
    }

    // No expected type: build `struct __anon_<shape> { name: T, ... }` from the field values
    // (registered once per shape, so identical shapes are the same type) and return its type.
    std::shared_ptr<Type> inferAnonymousStruct(SymbolTableType scope) {
        std::vector<std::pair<std::string, std::shared_ptr<Type>>> shape;
        for (const auto& f : fields) {
            auto arg = std::dynamic_pointer_cast<ArgumentStatement>(f);
            if (!arg || !arg->value) {
                console.reportError(
                    Console::TYPE_ERROR,
                    "Cannot infer the type of this '{ ... }' literal.",
                    "Positional fields have no names to infer a type from. To resolve this:\n"
                    "1. Use named fields: `{ width: 800, height: 600 }`\n"
                    "2. Or name the type: `let c = Config { ... };`",
                    getSpan());
                return nullptr;
            }
            // Express a clone: the real value is expressed again when the struct is built.
            auto v = arg->value->clone()->express(scope);
            if (!v || !v->getType() || v->getType()->isInvalid()) {
                console.reportError(
                    Console::TYPE_ERROR,
                    "Cannot infer the type of field '" + arg->name + "' in this '{ ... }' literal.",
                    "Annotate the target instead: `let c: Config = { ... };`",
                    arg->value->getSpan());
                return nullptr;
            }
            shape.emplace_back(arg->name, v->getType());
        }

        std::string name = "__anon";
        for (const auto& [n, t] : shape) name += "_" + n + "_" + sanitize(t->toString());

        if (!scope->getType(name)) {
            std::vector<std::shared_ptr<Statement>> body;
            for (const auto& [n, t] : shape) {
                auto field = std::make_shared<ParameterStatement>(n);
                field->setType(t->clone());   // ConstructStructPrototype writes parameterName onto it
                field->setRootType(field->getType());
                field->setSpan(getSpan());
                body.push_back(field);
            }
            ConstructStructPrototype proto(name, body);
            proto.setSpan(getSpan());
            // Registers the type and stores the StructExpression in `scope` under `name`;
            // codegen finds it there (IRGenerator::codegen, findStructDecl).
            if (!proto.express(scope)) return nullptr;
        }
        return scope->getType(name);
    }
};

} // namespace Omniscript