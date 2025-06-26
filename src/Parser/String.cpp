<<<<<<< HEAD:src/Parser/String.cpp
#include <omniscript/Statement.h>
#include <omniscript/Statements/LiteralStatements.h>

#include <omniscript/Core.h>
#include <omniscript/utils.h>
#include <omniscript/Parser.h>
#include <omniscript/Tokens.h>
#include <omniscript/omniscript_pch.h>
#include <omniscript/Statement.h>
#include <omniscript/Symboltable.h>
=======
#include <omniscript/Statement.h>
#include <omniscript/Statements/LiteralStatements.h>

#include <omniscript/Core.h>
#include <omniscript/utils.h>
#include <omniscript/Parser.h>
#include <omniscript/Tokens.h>
#include <omniscript/omniscript_pch.h>
#include <omniscript/Statement.h>
#include <omniscript/Symboltable.h>
>>>>>>> 7ccebff50dd27e70cffd4d578dcb358f4c9e1613:src/engine/Parser/String.cpp


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