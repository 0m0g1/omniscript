#include <omniscript/Core.h>
#include <omniscript/utils.h>
#include <omniscript/Parser.h>
#include <omniscript/Tokens.h>
#include <omniscript/Statements/Statement.h>
#include <omniscript/Symboltable.h>
#include <omniscript/omniscript_pch.h>

namespace Omniscript {

std::shared_ptr<Statement> Parser::parseBlock() {
    Token startToken = currentToken;

    std::vector<std::shared_ptr<Statement>> statements;

    // Every push must be null-guarded. `parseStatement()` returns null for
    // empty statements (stray semicolons, blank lines) and on error; the
    // block statement has no business carrying nulls downstream — codegen
    // and toString() both dereference them without checking.
    auto pushIfValid = [&](std::shared_ptr<Statement> stmt) {
        if (stmt) {
            statements.push_back(std::move(stmt));
        }
    };

    if (currentToken.getType() == TokenTypes::LeftBrace) {

        eat(TokenTypes::LeftBrace);

        while (currentToken.getType() != TokenTypes::RightBrace && currentToken.getType() != TokenTypes::EOI) {
            pushIfValid(parseStatement());
        }

        eat(TokenTypes::RightBrace);

        if (currentToken.getType() == TokenTypes::Semicolon) {
            eat(TokenTypes::Semicolon);
        }

    } else if (currentToken.getType() == TokenTypes::Return) {
        pushIfValid(parseReturnStatement());
    } else {
        pushIfValid(parseStatement());
    }

    auto block = std::make_shared<BlockStatement>(statements);
    block->setPosition(startToken, currentToken);
    return block;
}

} // namespace Omniscript