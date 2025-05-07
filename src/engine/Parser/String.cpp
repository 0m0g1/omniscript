#include <omniscript/Core.h>
#include <omniscript/utils.h>
#include <omniscript/engine/parser.h>
#include <omniscript/engine/tokens.h>
#include <omniscript/engine/Statement.h>
#include <omniscript/engine/Symboltable.h>
#include <omniscript/omniscript_pch.h>



std::string Parser::parseStringLiteral() {
    std::string value = currentToken.getValue();
    eat(TokenTypes::StringLiteral);

    while (currentToken.getType() == TokenTypes::Plus) {
        eat(TokenTypes::Plus);
        
        if (currentToken.getType() == TokenTypes::StringLiteral) {
            eat(TokenTypes::StringLiteral);
            value += previousToken.getValue();
        } else if (currentToken.getType() == TokenTypes::IntegerLiteral) {
            value += currentToken.getValue();  // Convert number to string
            eat(TokenTypes::IntegerLiteral);
        } else if (currentToken.getType() == TokenTypes::FloatLiteral) {
            value += currentToken.getValue();  // Convert number to string
            eat(TokenTypes::FloatLiteral);
        }
    }

    // DEBUG_LOG("Parsed string literal");

    return value;
}