#pragma once
#include <stdexcept>
#include <string>
#include <vector>

#include <omniscript/lexer/Lexer.h>
#include <omniscript/ast/Ast.h>

namespace Omniscript {

struct ParseError : std::runtime_error {
    Token token;
    explicit ParseError(const std::string& msg, Token t)
        : std::runtime_error(msg), token(std::move(t)) {}
};

class Parser {
public:
    explicit Parser(Lexer& lexer);

    // Entry point
    std::unique_ptr<Program> parse();

private:
    // ----- token helpers -----
    const Token& peek(int n = 1);
    Token advance();
    bool check(TokenType t);
    bool match(TokenType t);
    Token consume(TokenType t, const char* message);

    // ----- statements -----
    StmtPtr parseStatement();
    StmtPtr parseBlock();                 // { ... }
    StmtPtr parseIf();
    StmtPtr parseWhile();
    StmtPtr parseReturn();
    StmtPtr parseVarDecl(VarFlavor flavor);
    StmtPtr parseExprStatement();

    // ----- expressions (Pratt) -----
    ExprPtr parseExpression(int minPrec = 0);
    ExprPtr parsePrefix();
    ExprPtr parsePrimary();

    int precedenceOf(TokenType t) const;
    bool isRightAssociative(TokenType t) const;

    // optional: newline/semicolon handling
    void consumeStatementTerminator();

private:
    Lexer& m_lexer;
    Token m_lookahead;     // current token
    bool m_hasLookahead = false;
};

} // namespace Omniscript
