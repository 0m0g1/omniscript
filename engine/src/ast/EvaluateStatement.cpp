// engine/src/ast/EvaluateStatement.cpp
#include <omniscript/ast/Ast.h>
#include <omniscript/ast/AstStatement.h>
#include <omniscript/semantics/SymbolTable.h>

namespace Omniscript {

bool Program::evaluate(SymbolTable& scope, EvalContext& ctx) {
    bool ok = true;

    // Single-pass is fine for now.
    // If you want recursion + forward calls robustly later, do a predeclare pass.
    for (auto& st : statements) {
        if (!st) continue;
        ok = st->evaluate(scope, ctx) && ok;
    }

    return ok && !ctx.diags.hasErrors();
}

bool BlockStmt::evaluate(SymbolTable& scope, EvalContext& ctx) {
    SymbolTable inner(&scope);
    bool ok = true;
    for (auto& st : statements) {
        if (!st) continue;
        ok = st->evaluate(inner, ctx) && ok;
    }
    return ok;
}

bool ExprStmt::evaluate(SymbolTable& scope, EvalContext& ctx) {
    if (!expr) return true;
    (void)expr->evaluate(scope, ctx);
    return true;
}

static bool isConstFlavor(VarFlavor f) { return f == VarFlavor::Const; }
static bool isLetFlavor(VarFlavor f)   { return f == VarFlavor::Let; }

bool VarDeclStmt::evaluate(SymbolTable& scope, EvalContext& ctx) {
    // 1) evaluate initializer type (if any)
    Type initTy = Type::Unknown();
    if (initializer) initTy = initializer->evaluate(scope, ctx);

    // 2) declared type resolution (starter mapping; replace with real resolver later)
    Type declTy = Type::Unknown();
    if (!declaredType.empty()) {
        const std::string t = declaredType.front().lexeme(); // <-- method
        if (t == "int") declTy = Type::Int();
        else if (t == "float") declTy = Type::Float();
        else if (t == "bool") declTy = Type::Bool();
        else if (t == "char") declTy = Type::Char();
        else if (t == "void") declTy = Type::Void();
        else declTy = Type::Struct(t); // placeholder for named types
    }

    // 3) inference rules
    Type finalTy = declTy.isKnown() ? declTy : initTy;
    if (!finalTy.isKnown()) {
        ctx.diags.error(span, "cannot infer type for variable '" + name.lexeme() + "'");
        finalTy = Type::Unknown();
    }

    // 4) type compatibility check if both exist
    if (declTy.isKnown() && initTy.isKnown() && !sameType(declTy, initTy)) {
        ctx.diags.error(span,
            "type mismatch in variable '" + name.lexeme() + "': declared " +
            declTy.toString() + " but initializer is " + initTy.toString());
    }

    // 5) define symbol
    Symbol sym;
    sym.kind = SymbolKind::Variable;
    sym.name = name.lexeme();
    sym.type = finalTy;
    sym.isMutable = !(isConstFlavor(flavor) || isLetFlavor(flavor));
    sym.nameTok = name;

    return scope.define(sym, ctx.diags);
}

bool IfStmt::evaluate(SymbolTable& scope, EvalContext& ctx) {
    Type cond = condition ? condition->evaluate(scope, ctx) : Type::Unknown();
    if (cond.isKnown() && cond.kind != TypeKind::Bool) {
        ctx.diags.error(span, "if condition must be bool, got " + cond.toString());
    }
    bool ok = true;
    if (thenBranch) ok = thenBranch->evaluate(scope, ctx) && ok;
    if (elseBranch) ok = elseBranch->evaluate(scope, ctx) && ok;
    return ok;
}

bool WhileStmt::evaluate(SymbolTable& scope, EvalContext& ctx) {
    Type cond = condition ? condition->evaluate(scope, ctx) : Type::Unknown();
    if (cond.isKnown() && cond.kind != TypeKind::Bool) {
        ctx.diags.error(span, "while condition must be bool, got " + cond.toString());
    }
    return body ? body->evaluate(scope, ctx) : true;
}

bool ReturnStmt::evaluate(SymbolTable& scope, EvalContext& ctx) {
    (void)scope;

    if (!ctx.inFunction) {
        ctx.diags.error(span, "return statement outside of a function");
        return false;
    }

    Type valTy = Type::Void();
    if (value) valTy = value->evaluate(scope, ctx);

    if (ctx.currentReturn.isKnown() && valTy.isKnown() && !sameType(ctx.currentReturn, valTy)) {
        ctx.diags.error(span,
            "return type mismatch: expected " + ctx.currentReturn.toString() +
            " but got " + valTy.toString());
    }
    return true;
}

static Type resolveSimpleTypeFromTokens(const std::vector<Token>& toks) {
    if (toks.empty()) return Type::Unknown();

    // base is first identifier
    const std::string base = toks.front().lexeme();
    Type ty = Type::Unknown();

    if (base == "void") ty = Type::Void();
    else if (base == "bool") ty = Type::Bool();
    else if (base == "int") ty = Type::Int();
    else if (base == "float") ty = Type::Float();
    else if (base == "char") ty = Type::Char();
    else ty = Type::Struct(base);

    for (std::size_t i = 1; i < toks.size(); ++i) {
        if (toks[i].type() == TokenType::Star) {
            ty = Type::PointerTo(std::move(ty));
        }
    }

    return ty;
}

bool FunctionDeclStmt::evaluate(SymbolTable& scope, EvalContext& ctx) {
    // 1) build function type
    std::vector<Type> paramTypes;
    paramTypes.reserve(params.size());
    bool isVarArg = false;

    for (auto& p : params) {
        if (p.isVarArg) { isVarArg = true; break; }
        const auto& tyToks = p.typeToks.empty() ? p.type : p.typeToks;
        paramTypes.push_back(resolveSimpleTypeFromTokens(tyToks));
    }

    Type retTy = resolveSimpleTypeFromTokens(returnType);
    Type fnTy  = Type::Function(paramTypes, retTy, isVarArg);

    // 2) define function symbol (so recursion works)
    Symbol sym;
    sym.kind = SymbolKind::Function;
    sym.name = name.lexeme();
    sym.type = fnTy;
    sym.isMutable = false;
    sym.nameTok = name;

    bool ok = scope.define(sym, ctx.diags);

    // prototype: no body checks
    if (isPrototype || !body) return ok;

    // 3) evaluate body in new scope with params defined
    SymbolTable fnScope(&scope);

    for (std::size_t i = 0; i < params.size(); ++i) {
        if (params[i].isVarArg) break;

        Symbol ps;
        ps.kind = SymbolKind::Variable;
        ps.name = params[i].name.lexeme();
        ps.type = paramTypes[i];
        ps.isMutable = true;
        ps.nameTok = params[i].name;

        ok = fnScope.define(ps, ctx.diags) && ok;
    }

    // 4) set function context
    const bool prevInFn = ctx.inFunction;
    const Type prevRet  = ctx.currentReturn;
    ctx.inFunction = true;
    ctx.currentReturn = retTy;

    ok = body->evaluate(fnScope, ctx) && ok;

    ctx.inFunction = prevInFn;
    ctx.currentReturn = prevRet;
    return ok;
}

bool StructDeclStmt::evaluate(SymbolTable& scope, EvalContext& ctx) {
    // Define the struct type itself as a symbol (so "Vec2" can be referenced later)
    Symbol s;
    s.kind = SymbolKind::Struct;
    s.name = name.lexeme();
    s.type = Type::Struct(s.name);
    s.isMutable = false;
    s.nameTok = name;

    bool ok = scope.define(s, ctx.diags);

    // Minimal field checks: unique names + default init matches (if present)
    // (Full member typing comes later once you add MemberExpr / struct instance rules.)
    for (std::size_t i = 0; i < fields.size(); ++i) {
        // check duplicate field names
        for (std::size_t j = i + 1; j < fields.size(); ++j) {
            if (fields[i].name.lexeme() == fields[j].name.lexeme()) {
                ctx.diags.error(fields[j].name.span(),
                    "duplicate field '" + fields[j].name.lexeme() + "' in struct '" + s.name + "'");
                ok = false;
            }
        }

        // If you want: resolve field type tokens now
        Type fieldTy = resolveSimpleTypeFromTokens(fields[i].typeToks);

        if (fields[i].defaultValue) {
            Type defTy = fields[i].defaultValue->evaluate(scope, ctx);
            if (fieldTy.isKnown() && defTy.isKnown() && !sameType(fieldTy, defTy)) {
                ctx.diags.error(fields[i].defaultValue->span,
                    "default value type mismatch for field '" + fields[i].name.lexeme() +
                    "': expected " + fieldTy.toString() + " but got " + defTy.toString());
                ok = false;
            }
        }
    }

    return ok;
}

bool ExternStmt::evaluate(SymbolTable& scope, EvalContext& ctx) {
    // After FFI expansion, the resolver injects actual decl statements.
    // Evaluate nested statements in the same scope so they become visible globally.
    bool ok = true;
    for (auto& st : statements) {
        if (!st) continue;
        ok = st->evaluate(scope, ctx) && ok;
    }
    return ok;
}

bool ImportStmt::evaluate(SymbolTable&, EvalContext& ctx) {
    // You can implement module resolution later.
    // For now, just don't crash and optionally warn.
    (void)ctx;
    return true;
}

} // namespace Omniscript