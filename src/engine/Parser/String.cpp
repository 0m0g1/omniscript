#include <omniscript/engine/Statement.h>
#include <omniscript/engine/Statements/LiteralStatements.h>

#include <omniscript/engine/Core.h>
#include <omniscript/utils.h>
#include <omniscript/engine/Parser.h>
#include <omniscript/engine/Tokens.h>
#include <omniscript/omniscript_pch.h>
#include <omniscript/engine/Statement.h>
#include <omniscript/engine/Symboltable.h>


std::u32string Parser::parseStringLiteral() {
    std::u32string value = currentToken.getU32Value();

    eat(TokenTypes::StringLiteral);

    while (currentToken.getType() == TokenTypes::Plus) {
        eat(TokenTypes::Plus);
        
        if (currentToken.getType() == TokenTypes::StringLiteral) {
            eat(TokenTypes::StringLiteral);
            value += utf8_to_utf32(previousToken.getValue());
        } else if (currentToken.getType() == TokenTypes::IntegerLiteral) {
            value += utf8_to_utf32(currentToken.getValue());  // Convert number to string
            eat(TokenTypes::IntegerLiteral);
        } else if (currentToken.getType() == TokenTypes::FloatLiteral) {
            value += utf8_to_utf32(currentToken.getValue());  // Convert number to string
            eat(TokenTypes::FloatLiteral);
        }
    }

    // DEBUG_LOG("Parsed string literal");

    return value;
}

std::shared_ptr<Statement> Parser::parseStringTemplate() {
    Token startToken = currentToken;
    std::u32string value = currentToken.getU32Value();

    eat(TokenTypes::StringLiteral);

    while (currentToken.getType() == TokenTypes::Plus) {
        eat(TokenTypes::Plus);
        
        if (currentToken.getType() == TokenTypes::StringLiteral) {
            eat(TokenTypes::StringLiteral);
            value += utf8_to_utf32(previousToken.getValue());
        } else if (currentToken.getType() == TokenTypes::IntegerLiteral) {
            value += utf8_to_utf32(currentToken.getValue());  // Convert number to string
            eat(TokenTypes::IntegerLiteral);
        } else if (currentToken.getType() == TokenTypes::FloatLiteral) {
            value += utf8_to_utf32(currentToken.getValue());  // Convert number to string
            eat(TokenTypes::FloatLiteral);
        }
    }

    // DEBUG_LOG("Parsed string literal");

    return nullptr;
}