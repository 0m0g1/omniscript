// engine/src/ast/EvaluateExpression.cpp
#include <omniscript/ast/AstExpression.h>
#include <omniscript/semantics/SymbolTable.h>

namespace Omniscript {

static bool isNum(const Type& t) {
    return t.kind == TypeKind::Int || t.kind == TypeKind::Float;
}

static std::string joinPath(const IdentifierPath& p) {
    std::string s;
    for (std::size_t i = 0; i < p.parts.size(); ++i) {
        if (i) s += "::";
        s += p.parts[i].lexeme();
    }
    return s;
}

Type GroupExpr::evaluate(SymbolTable& scope, EvalContext& ctx) {
    if (!inner) return Type::Unknown();
    return inner->evaluate(scope, ctx);
}

Type LiteralExpr::evaluate(SymbolTable&, EvalContext&) {
    switch (literal.type()) {
        case TokenType::True:
        case TokenType::False:
            return Type::Bool();

        case TokenType::IntegerLiteral:
        case TokenType::BigIntLiteral:
        case TokenType::BinaryLiteral:
        case TokenType::OctalLiteral:
        case TokenType::HexLiteral:
            return Type::Int();

        case TokenType::FloatLiteral:
            return Type::Float();

        case TokenType::CharacterLiteral:
            return Type::Char();

        case TokenType::StringLiteral:
        case TokenType::TemplateLiteral:
        case TokenType::TemplateHead:
        case TokenType::TemplateMiddle:
        case TokenType::TemplateTail:
            // Omniscript: string literal is a C-string => char*
            return Type::PointerTo(Type::Char());

        case TokenType::Null:
        case TokenType::Nullptr:
            // You can later add a Null type that is compatible with any pointer.
            return Type::Unknown();

        default:
            return Type::Unknown();
    }
}

Type UnaryExpr::evaluate(SymbolTable& scope, EvalContext& ctx) {
    Type R = right ? right->evaluate(scope, ctx) : Type::Unknown();
    if (!R.isKnown()) return Type::Unknown();

    const TokenType opType = op.type();

    if (opType == TokenType::LogicalNot) {
        if (R.kind != TypeKind::Bool) {
            ctx.diags.error(span, "operator '" + op.lexeme() + "' expects bool, got " + R.toString());
            return Type::Unknown();
        }
        return Type::Bool();
    }

    if (opType == TokenType::Plus || opType == TokenType::Minus) {
        if (!isNum(R)) {
            ctx.diags.error(span, "operator '" + op.lexeme() + "' expects numeric operand, got " + R.toString());
            return Type::Unknown();
        }
        return R;
    }

    if (opType == TokenType::BitNot) {
        if (R.kind != TypeKind::Int) {
            ctx.diags.error(span, "operator '" + op.lexeme() + "' expects int operand, got " + R.toString());
            return Type::Unknown();
        }
        return Type::Int();
    }

    if (opType == TokenType::Increment || opType == TokenType::Decrement) {
        if (!isNum(R)) {
            ctx.diags.error(span, "operator '" + op.lexeme() + "' expects numeric operand, got " + R.toString());
            return Type::Unknown();
        }
        return R;
    }

    ctx.diags.error(span, "unsupported unary operator '" + op.lexeme() + "'");
    return Type::Unknown();
}

Type BinaryExpr::evaluate(SymbolTable& scope, EvalContext& ctx) {
    Type L = left  ? left->evaluate(scope, ctx)  : Type::Unknown();
    Type R = right ? right->evaluate(scope, ctx) : Type::Unknown();

    if (!L.isKnown() || !R.isKnown()) return Type::Unknown();

    const TokenType opType = op.type();

    if (opType == TokenType::Plus || opType == TokenType::Minus ||
        opType == TokenType::Star || opType == TokenType::Slash ||
        opType == TokenType::Percent || opType == TokenType::Power) {

        if (isNum(L) && isNum(R)) {
            if (L.kind == TypeKind::Float || R.kind == TypeKind::Float) return Type::Float();
            return Type::Int();
        }

        ctx.diags.error(span,
            "invalid operands for '" + op.lexeme() + "': " + L.toString() + " and " + R.toString());
        return Type::Unknown();
    }

    if (opType == TokenType::Equals || opType == TokenType::NotEquals) {
        if (!sameType(L, R)) {
            ctx.diags.error(span,
                "cannot compare different types with '" + op.lexeme() + "': " + L.toString() + " and " + R.toString());
        }
        return Type::Bool();
    }

    if (opType == TokenType::Less || opType == TokenType::LessEqual ||
        opType == TokenType::Greater || opType == TokenType::GreaterEqual) {

        if (!isNum(L) || !isNum(R)) {
            ctx.diags.error(span,
                "relational operator '" + op.lexeme() + "' requires numeric operands, got " + L.toString() + " and " + R.toString());
        }
        return Type::Bool();
    }

    if (opType == TokenType::LogicalAnd || opType == TokenType::LogicalOr || opType == TokenType::LogicalXor) {
        if (L.kind != TypeKind::Bool || R.kind != TypeKind::Bool) {
            ctx.diags.error(span,
                "logical operator '" + op.lexeme() + "' requires bool operands, got " + L.toString() + " and " + R.toString());
        }
        return Type::Bool();
    }

    if (opType == TokenType::BitAnd || opType == TokenType::BitOr || opType == TokenType::BitXor ||
        opType == TokenType::ShiftLeft || opType == TokenType::ShiftRight) {
        if (L.kind != TypeKind::Int || R.kind != TypeKind::Int) {
            ctx.diags.error(span,
                "bitwise operator '" + op.lexeme() + "' requires int operands, got " + L.toString() + " and " + R.toString());
            return Type::Unknown();
        }
        return Type::Int();
    }

    ctx.diags.error(span, "unsupported binary operator '" + op.lexeme() + "'");
    return Type::Unknown();
}

Type IdentifierExpr::evaluate(SymbolTable& scope, EvalContext& ctx) {
    const std::string full = joinPath(name);

    if (name.parts.size() != 1) {
        ctx.diags.error(span, "qualified identifier not supported yet in evaluator: '" + full + "'");
        return Type::Unknown();
    }

    const std::string id = name.parts[0].lexeme();
    if (Symbol* sym = scope.lookup(id)) return sym->type;

    ctx.diags.error(span, "undefined identifier '" + id + "'");
    return Type::Unknown();
}

Type CallExpr::evaluate(SymbolTable& scope, EvalContext& ctx) {
    const std::string full = joinPath(callee);

    if (callee.parts.size() != 1) {
        ctx.diags.error(span, "qualified call target not supported yet in evaluator: '" + full + "'");
        return Type::Unknown();
    }

    const std::string fnName = callee.parts[0].lexeme();
    Symbol* sym = scope.lookup(fnName);
    if (!sym) {
        ctx.diags.error(span, "call to undefined function '" + fnName + "'");
        return Type::Unknown();
    }

    if (sym->type.kind != TypeKind::Function) {
        ctx.diags.error(span, "'" + fnName + "' is not callable (type is " + sym->type.toString() + ")");
        return Type::Unknown();
    }

    std::vector<Type> argTypes;
    argTypes.reserve(args.size());
    for (auto& a : args) argTypes.push_back(a ? a->evaluate(scope, ctx) : Type::Unknown());

    const Type& fnTy = sym->type;

    if (!fnTy.isVarArg) {
        if (argTypes.size() != fnTy.params.size()) {
            ctx.diags.error(span,
                "function '" + fnName + "' expects " + std::to_string(fnTy.params.size()) +
                " args but got " + std::to_string(argTypes.size()));
            return fnTy.returnTypeOrUnknown();
        }
    } else {
        if (argTypes.size() < fnTy.params.size()) {
            ctx.diags.error(span,
                "function '" + fnName + "' expects at least " + std::to_string(fnTy.params.size()) +
                " args but got " + std::to_string(argTypes.size()));
            return fnTy.returnTypeOrUnknown();
        }
    }

    for (std::size_t i = 0; i < fnTy.params.size() && i < argTypes.size(); ++i) {
        if (fnTy.params[i].isKnown() && argTypes[i].isKnown() && !sameType(fnTy.params[i], argTypes[i])) {
            ctx.diags.error(span,
                "arg " + std::to_string(i + 1) + " of '" + fnName + "' expected " +
                fnTy.params[i].toString() + " but got " + argTypes[i].toString());
        }
    }

    return fnTy.returnTypeOrUnknown();
}

} // namespace Omniscript