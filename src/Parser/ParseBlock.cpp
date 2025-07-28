#include <omniscript/Core.h>
#include <omniscript/utils.h>
#include <omniscript/Parser.h>
#include <omniscript/Tokens.h>
#include <omniscript/Statement.h>
#include <omniscript/Symboltable.h>
#include <omniscript/omniscript_pch.h>

std::shared_ptr<Statement> Parser::parseBlock() {
    Token startToken = currentToken;

    std::vector<std::shared_ptr<Statement>> statements;

    if (currentToken.getType() == TokenTypes::LeftBrace) {

        eat(TokenTypes::LeftBrace);

        while (currentToken.getType() != TokenTypes::RightBrace && currentToken.getType() != TokenTypes::EOI) {
            statements.push_back(parseStatement());
        }

        eat(TokenTypes::RightBrace);

        if (currentToken.getType() == TokenTypes::Semicolon) {
            eat(TokenTypes::Semicolon);
        }

    } else if (currentToken.getType() == TokenTypes::Return) {

        statements.push_back(parseReturnStatement());

        if (currentToken.getType() != TokenTypes::Semicolon || currentToken.getType() != TokenTypes::Newline) {
            eat(TokenTypes::Semicolon);    
        } else {
            eat(currentToken.getType());
        }

    } else {
        statements.push_back(parseStatement());
    }

    auto block = std::make_shared<BlockStatement>(statements);
    block->setPosition(startToken, previousToken);
    return block;
}
