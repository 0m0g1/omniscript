#include <iostream>
#include <omniscript/parser/Parser.h>

namespace Omniscript {

Parser::Parser(Lexer& lexer)
    : m_lexer(lexer),
      m_current_token(m_lexer.getNextToken()),
      m_previous_token(Token(TokenType::EndOfInput)) // sentinel; adjust if you have a better "invalid" token
{}

// ----------------- token helpers (NO parser peeking) -----------------

Token Parser::advance() {
    std::cout << m_current_token.toString() << std::endl;
    m_previous_token = std::move(m_current_token);
    m_current_token  = m_lexer.getNextToken();
    return m_previous_token; // return the token we just consumed
}

bool Parser::check(TokenType t) const {
    return m_current_token.type() == t;
}

bool Parser::match(TokenType t) {
    if (!check(t)) return false;
    advance();
    return true;
}

void Parser::eat(TokenType t, const char* message) {
    if (check(t)) {
        advance();
        return;
    }
    throw ParseError(message, m_current_token);
}

// optional: newline/semicolon handling
void Parser::eatStatementTerminator() {
    // 1) Accept optional semicolons (one or many)
    if (match(TokenType::Semicolon)) {
        while (match(TokenType::Semicolon)) {}
        while (match(TokenType::Newline)) {}
        return;
    }

    // 2) If your lexer emits Newline tokens, accept them too
    if (match(TokenType::Newline)) {
        while (match(TokenType::Newline)) {}
        return;
    }

    // 3) Allow implicit terminator before '}' or EOF
    if (check(TokenType::RightBrace) || check(TokenType::EndOfInput)) return;

    // 4) Otherwise error
    throw ParseError("Expected ';' or newline after statement.", m_current_token);
}

// ----------------- top-level -----------------

std::unique_ptr<Program> Parser::parse() {
    std::vector<StmtPtr> stmts;

    // Skip initial newlines
    while (match(TokenType::Newline)) {}

    while (!check(TokenType::EndOfInput)) {
        auto st = parseStatement();
        if (!st) break;
        stmts.push_back(std::move(st));

        // eat extra newlines between statements
        while (match(TokenType::Newline)) {}
    }

    FileSpan progSpan{};
    if (!stmts.empty()) progSpan = mergeSpans(stmts.front()->span, stmts.back()->span);
    return std::make_unique<Program>(std::move(stmts), progSpan);
}

// ----------------- Statements -----------------

StmtPtr Parser::parseStatement() {
    // Skip extra newlines
    while (match(TokenType::Newline)) {}

    if (match(TokenType::Extern)) {
        return parseExtern();
    }

    if (match(TokenType::Function)) {
        return parseFunctionDeclaration();
    }

    if (match(TokenType::LeftBrace)) {
        return parseBlock();
    }

    if (match(TokenType::If)) {
        return parseIf();
    }

    if (match(TokenType::While)) {
        return parseWhile();
    }

    if (match(TokenType::Return)) {
        return parseReturn();
    }

    if (match(TokenType::Let)) {
        return parseVarDecl(VarFlavor::Let);
    }
    if (match(TokenType::Var)) {
        return parseVarDecl(VarFlavor::Var);
    }
    if (match(TokenType::Const)) {
        return parseVarDecl(VarFlavor::Const);
    }

    return parseExprStatement();
}

StmtPtr Parser::parseBlock() {
    // '{' already consumed by parseStatement()
    std::vector<StmtPtr> stmts;

    // allow leading newlines
    while (match(TokenType::Newline)) {}

    while (!check(TokenType::RightBrace) && !check(TokenType::EndOfInput)) {
        stmts.push_back(parseStatement());
        while (match(TokenType::Newline)) {}
    }

    // capture right brace for span
    const Token rbrace = m_current_token;
    eat(TokenType::RightBrace, "Expected '}' to close block.");

    FileSpan span{};
    if (!stmts.empty()) span = mergeSpans(stmts.front()->span, rbrace.span());
    else span = rbrace.span();

    return std::make_unique<BlockStmt>(std::move(stmts), span);
}

StmtPtr Parser::parseIf() {
    // Expect: if (expr) stmt (else stmt)?
    eat(TokenType::LeftParen, "Expected '(' after 'if'.");

    auto cond = parseExpression();

    eat(TokenType::RightParen, "Expected ')' after if condition.");

    auto thenBranch = parseStatement();
    StmtPtr elseBranch = nullptr;

    // else or else-if
    if (match(TokenType::Else)) {
        elseBranch = parseStatement();
    } else if (match(TokenType::ElseIf)) {
        // NOTE: This only works if ElseIf is treated like "if" (i.e., grammar shape matches).
        // Many languages tokenize it as Else + If to avoid special-casing.
        elseBranch = parseIf();
    }

    FileSpan s = mergeSpans(cond->span, thenBranch->span);
    if (elseBranch) s = mergeSpans(s, elseBranch->span);

    return std::make_unique<IfStmt>(std::move(cond), std::move(thenBranch), std::move(elseBranch), s);
}

StmtPtr Parser::parseWhile() {
    eat(TokenType::LeftParen, "Expected '(' after 'while'.");
    auto cond = parseExpression();
    eat(TokenType::RightParen, "Expected ')' after while condition.");

    auto body = parseStatement();
    FileSpan s = mergeSpans(cond->span, body->span);
    return std::make_unique<WhileStmt>(std::move(cond), std::move(body), s);
}

StmtPtr Parser::parseReturn() {
    // return expr? ;
    if (check(TokenType::Semicolon) || check(TokenType::Newline) || check(TokenType::RightBrace)) {
        auto stmt = std::make_unique<ReturnStmt>(nullptr, m_current_token.span());
        if (!check(TokenType::RightBrace)) eatStatementTerminator();
        return stmt;
    }

    auto value = parseExpression();
    auto s = value->span;

    auto stmt = std::make_unique<ReturnStmt>(std::move(value), s);
    eatStatementTerminator();
    return stmt;
}

StmtPtr Parser::parseVarDecl(VarFlavor flavor) {
    // require identifier
    if (!check(TokenType::Identifier))
        throw ParseError("Expected identifier after let/var/const.", m_current_token);

    Token name = advance(); // consume identifier

    ExprPtr init = nullptr;
    if (match(TokenType::Assign)) {
        init = parseExpression();
    }

    FileSpan s = name.span();
    if (init) s = mergeSpans(s, init->span);

    auto stmt = std::make_unique<VarDeclStmt>(flavor, std::move(name), std::move(init), s);
    eatStatementTerminator();
    return stmt;
}

StmtPtr Parser::parseExprStatement() {
    auto e = parseExpression();
    auto s = e->span;

    auto stmt = std::make_unique<ExprStmt>(std::move(e), s);
    eatStatementTerminator();
    return stmt;
}

// ----------------- Expressions (Pratt) -----------------

int Parser::precedenceOf(TokenType t) const {
    switch (t) {
        case TokenType::Power: return 70; // **

        case TokenType::Star:
        case TokenType::Slash:
        case TokenType::Percent: return 60;

        case TokenType::Plus:
        case TokenType::Minus: return 50;

        case TokenType::Less:
        case TokenType::LessEqual:
        case TokenType::Greater:
        case TokenType::GreaterEqual: return 40;

        case TokenType::Equals:
        case TokenType::NotEquals: return 35;

        case TokenType::LogicalAnd: return 20;
        case TokenType::LogicalOr:  return 10;

        case TokenType::Assign:
        case TokenType::PlusAssign:
        case TokenType::MinusAssign:
        case TokenType::StarAssign:
        case TokenType::SlashAssign:
        case TokenType::PercentAssign:
        case TokenType::PowerAssign: return 5;

        default: return 0;
    }
}

bool Parser::isRightAssociative(TokenType t) const {
    return t == TokenType::Power ||
           t == TokenType::Assign ||
           t == TokenType::PlusAssign || t == TokenType::MinusAssign ||
           t == TokenType::StarAssign || t == TokenType::SlashAssign ||
           t == TokenType::PercentAssign || t == TokenType::PowerAssign;
}

ExprPtr Parser::parseExpression(int minPrec) {
    auto left = parsePrefix();

    while (true) {
        // Look at the *current* token as the infix operator
        const TokenType tt = m_current_token.type();
        const int prec = precedenceOf(tt);
        if (prec < minPrec || prec == 0) break;

        Token op = advance(); // consume operator token

        const int nextMin = prec + (isRightAssociative(op.type()) ? 0 : 1);
        auto right = parseExpression(nextMin);

        FileSpan s = mergeSpans(left->span, right->span);
        left = std::make_unique<BinaryExpr>(std::move(left), std::move(op), std::move(right), s);
    }

    return left;
}

ExprPtr Parser::parsePrefix() {
    // unary operators: ! - + ~
    const TokenType tt = m_current_token.type();
    if (tt == TokenType::LogicalNot || tt == TokenType::Minus ||
        tt == TokenType::Plus || tt == TokenType::BitNot) {

        Token op = advance();
        auto right = parsePrefix();
        FileSpan s = mergeSpans(op.span(), right->span);
        return std::make_unique<UnaryExpr>(std::move(op), std::move(right), s);
    }

    return parsePrimary();
}

ExprPtr Parser::parsePrimary() {
    // ---- identifier path / call ----
    if (check(TokenType::Identifier)) {
        IdentifierPath path = parseIdentifiers("Expected identifier.");

        // Function call?  foo(...) or ns::foo(...)
        if (match(TokenType::LeftParen)) {
            auto args = parseArguments();
            Token rp = m_current_token;
            eat(TokenType::RightParen, "Expected ')' after call arguments.");

            FileSpan s = mergeSpans(path.span, rp.span());
            return std::make_unique<CallExpr>(std::move(path), std::move(args), s);
        }

        // Plain identifier expression
        return std::make_unique<IdentifierExpr>(std::move(path));
    }

    // ---- everything else: consume one token and switch ----
    Token t = advance();

    switch (t.type()) {
        case TokenType::IntegerLiteral:
        case TokenType::FloatLiteral:
        case TokenType::StringLiteral:
        case TokenType::CharacterLiteral:
        case TokenType::True:
        case TokenType::False:
        case TokenType::Null:
        case TokenType::Nullptr:
            return std::make_unique<LiteralExpr>(std::move(t));

        case TokenType::LeftParen: {
            auto inner = parseExpression();
            Token rp = m_current_token;
            eat(TokenType::RightParen, "Expected ')' after expression.");

            FileSpan s = mergeSpans(t.span(), rp.span());
            return std::make_unique<GroupExpr>(std::move(inner), s);
        }

        default:
            throw ParseError("Unexpected token in expression.", std::move(t));
    }
}

} // namespace Omniscript