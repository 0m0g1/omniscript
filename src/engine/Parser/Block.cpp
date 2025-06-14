#include <omniscript/Core.h>
#include <omniscript/utils.h>
#include <omniscript/engine/parser.h>
#include <omniscript/engine/tokens.h>
#include <omniscript/engine/Statement.h>
#include <omniscript/engine/Symboltable.h>
#include <omniscript/omniscript_pch.h>


std::shared_ptr<Statement> Parser::parseBlock() {
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

    return std::make_shared<BlockStatement>(statements);
}
