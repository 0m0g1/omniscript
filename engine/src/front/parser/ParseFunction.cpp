#include <omniscript/parser/Parser.h>

namespace Omniscript {

// Parse: fn name ( params ) ( => type )? ( block | terminator )
StmtPtr Parser::parseFunctionDeclaration() {
    // Precondition: 'fn' already consumed by match(TokenType::Function) in parseStatement()/extern.
    // If you ever call this directly, uncomment:
    // eat(TokenType::Function, "Expected 'fn'.");

    if (!check(TokenType::Identifier))
        throw ParseError("Expected function name after 'fn'.", current());
    Token nameTok = advance(); // consume identifier

    eat(TokenType::LeftParen, "Expected '(' after function name.");
    auto params = parseParameters();
    const Token rpTok = current(); // capture before consuming
    eat(TokenType::RightParen, "Expected ')' after parameters.");

    // Return type (optional): =>
    std::vector<Token> returnType;
    if (match(TokenType::Arrow)) {
        returnType = parseType();
    }
    // If no Arrow, treat as void (empty vector or a dedicated void token elsewhere)

    // Body or prototype terminator
    if (check(TokenType::LeftBrace)) {
        match(TokenType::LeftBrace); // consume '{'
        auto body = parseBlock();    // parseBlock() expects '{' already consumed

        FileSpan full = mergeSpans(nameTok.span(), body->span);
        return std::make_unique<FunctionDeclStmt>(
            std::move(nameTok),
            std::move(params),
            std::move(returnType),
            std::move(body),
            false,
            full
        );
    }

    // Prototype form: ends in ; or newline (or implicit before '}' / EOF)
    eatStatementTerminator();

    FileSpan full = mergeSpans(nameTok.span(), rpTok.span());
    return std::make_unique<FunctionDeclStmt>(
        std::move(nameTok),
        std::move(params),
        std::move(returnType),
        nullptr,
        true,
        full
    );
}

// params: (name : type) ( , name : type )*
std::vector<ParamDecl> Parser::parseParameters() {
    std::vector<ParamDecl> out;

    // empty params: ()
    if (check(TokenType::RightParen)) return out;

    bool seenVarArgs = false;

    while (true) {
        // ✅ varargs: ...
        if (match(TokenType::Ellipsis)) {
            if (seenVarArgs)
                throw ParseError("Duplicate '...' in parameter list.", previous());

            // '...' must be last (allow trailing comma only if you want; usually disallow)
            seenVarArgs = true;

            Token dots = previous();
            FileSpan ps = dots.span();

            ParamDecl vd;
            vd.isVarArg = true;
            vd.span = ps;
            out.push_back(std::move(vd));

            // After '...' we must see ')'
            if (!check(TokenType::RightParen))
                throw ParseError("Expected ')' after '...'.", current());

            break; // done
        }

        // normal param: name : type
        if (!check(TokenType::Identifier))
            throw ParseError("Expected parameter name or '...'.", current());
        Token paramName = advance();

        eat(TokenType::Colon, "Expected ':' after parameter name.");

        auto typeToks = parseType();
        if (typeToks.empty())
            throw ParseError("Expected type name after ':'.", current());

        // (Optional) ✅ support typed rest param: int...
        // e.g. fn f(xs: int...) => void
        bool typedVarArgs = false;
        Token dotsTok;
        if (match(TokenType::Ellipsis)) {
            typedVarArgs = true;
            dotsTok = previous();
        }

        FileSpan ps = mergeSpans(paramName.span(), typedVarArgs ? dotsTok.span() : typeToks.back().span());

        ParamDecl p;
        p.name = std::move(paramName);
        p.typeToks = std::move(typeToks);
        p.span = ps;
        p.isVarArg = typedVarArgs;
        out.push_back(std::move(p));

        if (typedVarArgs) {
            // typed varargs must be last
            if (!check(TokenType::RightParen))
                throw ParseError("Typed varargs parameter must be the last parameter.", current());
            break;
        }

        if (!match(TokenType::Comma)) break;

        // allow trailing comma before ')'
        if (check(TokenType::RightParen)) break;

        // if we've already seen '...' then there should not be anything else
        if (seenVarArgs)
            throw ParseError("No parameters allowed after '...'.", current());
    }

    return out;
}

// Minimal type parser:
// TypeName := Identifier ( '*' )*
std::vector<Token> Parser::parseType() {
    std::vector<Token> tks;

    if (!check(TokenType::Identifier))
        throw ParseError("Expected type name.", current());
    tks.push_back(advance()); // consume base type identifier

    while (match(TokenType::Star)) {
        // Keep the actual consumed '*' token (no synthetic tokens needed).
        tks.push_back(previous());
    }

    return tks;
}

} // namespace Omniscript