#include <omniscript/parser/Parser.h>

namespace Omniscript {

Parser::Parser(Lexer& lexer) : m_lexer(lexer) {}

const Token& Parser::peek(int n) {
    // Your Lexer already supports peekToken(n). We'll rely on it.
    // We'll keep a local cache only for "current token" style.
    if (n == 1) {
        if (!m_hasLookahead) {
            m_lookahead = m_lexer.getNextToken();
            m_hasLookahead = true;
        }
        return m_lookahead;
    }
    // For n>1 just ask lexer directly
    static Token temp;
    temp = m_lexer.peekToken(n);
    return temp;
}

Token Parser::advance() {
    if (!m_hasLookahead) {
        return m_lexer.getNextToken();
    }
    m_hasLookahead = false;
    return std::move(m_lookahead);
}

bool Parser::check(TokenType t) {
    return peek(1).type() == t;
}

bool Parser::match(TokenType t) {
    if (check(t)) { advance(); return true; }
    return false;
}

Token Parser::consume(TokenType t, const char* message) {
    if (check(t)) return advance();
    throw ParseError(message, peek(1));
}

void Parser::consumeStatementTerminator() {
    // Accept ';' or newline as statement terminator (you have TokenType::Newline)
    // Many languages accept optional semicolons; tweak to your taste.
    if (match(TokenType::Semicolon)) return;
    while (match(TokenType::Newline)) {} // allow blank lines
    // If next token is '}' or EOF, allow implicit terminator
    if (check(TokenType::RightBrace) || check(TokenType::EndOfInput)) return;
    // Otherwise require semicolon/newline
    // If you want strict semicolons, remove this flexibility.
    throw ParseError("Expected ';' or newline after statement.", peek(1));
}

std::unique_ptr<Program> Parser::parse() {
    std::vector<StmtPtr> stmts;
    FileSpan startSpan{};

    // Skip initial newlines
    while (match(TokenType::Newline)) {}

    while (!check(TokenType::EndOfInput)) {
        auto st = parseStatement();
        if (!st) break;
        if (stmts.empty()) startSpan = st->span;
        stmts.push_back(std::move(st));
        while (match(TokenType::Newline)) {} // eat extra newlines between statements
    }

    FileSpan progSpan{};
    if (!stmts.empty()) progSpan = mergeSpans(stmts.front()->span, stmts.back()->span);
    return std::make_unique<Program>(std::move(stmts), progSpan);
}

// ----------------- Statements -----------------

StmtPtr Parser::parseStatement() {
    // Skip extra newlines
    while (match(TokenType::Newline)) {}

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
    // We already consumed '{' in parseStatement
    std::vector<StmtPtr> stmts;

    // allow leading newlines
    while (match(TokenType::Newline)) {}

    FileSpan blockStart = peek(1).span(); // not perfect; improved below
    while (!check(TokenType::RightBrace) && !check(TokenType::EndOfInput)) {
        stmts.push_back(parseStatement());
        while (match(TokenType::Newline)) {}
    }

    Token rbrace = consume(TokenType::RightBrace, "Expected '}' to close block.");
    FileSpan span{};
    if (!stmts.empty()) span = mergeSpans(stmts.front()->span, rbrace.span());
    else span = rbrace.span();
    return std::make_unique<BlockStmt>(std::move(stmts), span);
}

StmtPtr Parser::parseIf() {
    // Expect: if (expr) stmt (else stmt)?
    Token ifTok = Token(TokenType::If); // not stored; span from condition/branches

    consume(TokenType::LeftParen, "Expected '(' after 'if'.");
    auto cond = parseExpression();
    Token rp = consume(TokenType::RightParen, "Expected ')' after if condition.");

    auto thenBranch = parseStatement();
    StmtPtr elseBranch = nullptr;

    // else or else-if
    if (match(TokenType::Else)) {
        elseBranch = parseStatement();
    } else if (match(TokenType::ElseIf)) {
        // Represent else-if as else { if (...) ... } or as nested IfStmt directly
        // We'll parse as nested IfStmt for simplicity.
        elseBranch = parseIf(); // parseIf expects 'if' already consumed, but ElseIf consumed.
        // Fix: ElseIf is a keyword; treat it like "if" and parse same shape:
        // We'll do a small workaround:
        // (Alternative: change grammar to tokenize ElseIf as Else + If.)
    }

    // Better: handle ElseIf explicitly:
    // If you keep TokenType::ElseIf, implement it properly instead of the above.
    // For now: simplest is to NOT use ElseIf token; treat it as Else + If in lexer.

    FileSpan s = mergeSpans(cond->span, thenBranch->span);
    if (elseBranch) s = mergeSpans(s, elseBranch->span);
    return std::make_unique<IfStmt>(std::move(cond), std::move(thenBranch), std::move(elseBranch), s);
}

StmtPtr Parser::parseWhile() {
    consume(TokenType::LeftParen, "Expected '(' after 'while'.");
    auto cond = parseExpression();
    consume(TokenType::RightParen, "Expected ')' after while condition.");
    auto body = parseStatement();
    FileSpan s = mergeSpans(cond->span, body->span);
    return std::make_unique<WhileStmt>(std::move(cond), std::move(body), s);
}

StmtPtr Parser::parseReturn() {
    // return expr? ;
    if (check(TokenType::Semicolon) || check(TokenType::Newline) || check(TokenType::RightBrace)) {
        auto stmt = std::make_unique<ReturnStmt>(nullptr, peek(1).span());
        // optional terminator consumption
        if (!check(TokenType::RightBrace)) consumeStatementTerminator();
        return stmt;
    }

    auto value = parseExpression();
    auto s = value->span;
    auto stmt = std::make_unique<ReturnStmt>(std::move(value), s);
    consumeStatementTerminator();
    return stmt;
}

StmtPtr Parser::parseVarDecl(VarFlavor flavor) {
    Token name = consume(TokenType::Identifier, "Expected identifier after let/var/const.");

    ExprPtr init = nullptr;
    if (match(TokenType::Assign)) {
        init = parseExpression();
    }

    FileSpan s = name.span();
    if (init) s = mergeSpans(s, init->span);

    auto stmt = std::make_unique<VarDeclStmt>(flavor, std::move(name), std::move(init), s);
    consumeStatementTerminator();
    return stmt;
}

StmtPtr Parser::parseExprStatement() {
    auto e = parseExpression();
    auto s = e->span;
    auto stmt = std::make_unique<ExprStmt>(std::move(e), s);
    consumeStatementTerminator();
    return stmt;
}

// ----------------- Expressions (Pratt) -----------------

int Parser::precedenceOf(TokenType t) const {
    // Keep this small now; expand later.
    switch (t) {
        case TokenType::Power: return 70;               // **
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
    return t == TokenType::Power || t == TokenType::Assign ||
           t == TokenType::PlusAssign || t == TokenType::MinusAssign ||
           t == TokenType::StarAssign || t == TokenType::SlashAssign ||
           t == TokenType::PercentAssign || t == TokenType::PowerAssign;
}

ExprPtr Parser::parseExpression(int minPrec) {
    auto left = parsePrefix();

    while (true) {
        TokenType tt = peek(1).type();
        int prec = precedenceOf(tt);
        if (prec < minPrec || prec == 0) break;

        Token op = advance();

        int nextMin = prec + (isRightAssociative(op.type()) ? 0 : 1);
        auto right = parseExpression(nextMin);

        FileSpan s = mergeSpans(left->span, right->span);
        left = std::make_unique<BinaryExpr>(std::move(left), std::move(op), std::move(right), s);
    }

    return left;
}

ExprPtr Parser::parsePrefix() {
    // unary operators: ! - + ++ -- ~
    TokenType tt = peek(1).type();
    if (tt == TokenType::LogicalNot || tt == TokenType::Minus || tt == TokenType::Plus || tt == TokenType::BitNot) {
        Token op = advance();
        auto right = parsePrefix();
        FileSpan s = mergeSpans(op.span(), right->span);
        return std::make_unique<UnaryExpr>(std::move(op), std::move(right), s);
    }
    return parsePrimary();
}

ExprPtr Parser::parsePrimary() {
    Token t = advance();

    switch (t.type()) {
        case TokenType::Identifier:
            return std::make_unique<IdentifierExpr>(std::move(t));

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
            Token rp = consume(TokenType::RightParen, "Expected ')' after expression.");
            FileSpan s = mergeSpans(t.span(), rp.span());
            return std::make_unique<GroupExpr>(std::move(inner), s);
        }

        default:
            throw ParseError("Unexpected token in expression.", std::move(t));
    }
}

} // namespace Omniscript
