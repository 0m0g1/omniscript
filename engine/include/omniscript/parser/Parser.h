// ==============================
// //engine/include/omniscript/parser/Parser.h  (REWRITTEN)
// ==============================
#pragma once

#include <stdexcept>
#include <string>
#include <vector>
#include <memory>

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
    // ----- token helpers (NO peeking here) -----
    const Token& current() const { return m_current_token; }
    const Token& previous() const { return m_previous_token; }

    Token advance();                         // consumes current -> previous, loads next into current
    bool check(TokenType t) const;           // checks current token type
    bool match(TokenType t);                 // if check(t) { advance(); return true; }
    void eat(TokenType t, const char* message); // consume if matches, else throw

    // Optional: statement separator handling
    void eatStatementTerminator();           // ';' or newline or implicit before '}' / EOF

    // ----- statements -----
    StmtPtr parseStatement();
    StmtPtr parseExtern();

    // extern-only helpers
    StmtPtr parseImport();                   // parses: import fn/let/const IDENT [as IDENT] OR import *
    bool isExternDeclStart() const;          // fn / let / const / import
    void skipExternSeparators();             // semicolons/newlines inside extern block

    StmtPtr parseFunctionDeclaration();
    std::vector<ParamDecl> parseParameters();
    std::vector<Token> parseType();      // minimal
    StmtPtr parseBlock();                    // { ... }
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

private:
    Lexer& m_lexer;

    // Parser-owned stream state:
    // - m_current_token is the token at the parser cursor
    // - m_previous_token is the last token consumed by advance()
    Token m_current_token;
    Token m_previous_token;
};

} // namespace Omniscript