#include <omniscript/Core.h>
#include <omniscript/utils.h>
#include <omniscript/engine/parser.h>
#include <omniscript/engine/tokens.h>
#include <omniscript/engine/Statement.h>
#include <omniscript/engine/Symboltable.h>
#include <omniscript/omniscript_pch.h>

std::shared_ptr<Statement> Parser::parseIdentifier() {
    // Parse the root identifier
    std::string rootIdentifier = currentToken.getValue();
    eat(TokenTypes::Identifier);

    // Start with base identifier
    std::shared_ptr<Statement> expr = std::make_shared<GetVariable>(rootIdentifier);
    std::vector<std::string> memberPath = {};

    // Loop for dot/arrow/call access
    while (true) {
        if (currentToken.getType() == TokenTypes::LeftParen) {
            // Normal function call
            std::vector<std::shared_ptr<Statement>> args = parseArguments();
            expr = std::make_shared<Call>(expr, memberPath.back(), args);
        }

        // Function call with generics
        else if (isGenericCallOrConstructor()) {
            std::vector<std::string> typeParams = parseTypeParametersForCall();
            std::string specializedName = generateSpecializedNameForCall(memberPath.back(), typeParams);
            DEBUG_LOG("Generated Specialized Name: " + specializedName);
            std::vector<std::shared_ptr<Statement>> args = parseArguments();
            expr = std::make_shared<Call>(expr, specializedName, args);
        }

        // Object constructor
        else if (currentToken.getType() == TokenTypes::LeftBrace) {
            std::vector<std::shared_ptr<Statement>> args = parseArguments(TokenTypes::LeftBrace, TokenTypes::RightBrace, TokenTypes::Colon);
            expr = std::make_shared<ObjectConstructorStatement>(expr, memberPath.back(), "", args);
        }

        // Member access (.)
        else if (currentToken.getType() == TokenTypes::Dot || currentToken.getType() == TokenTypes::ScopeResolution) {
            eat(currentToken.getType()); // Eat dot or scope resolution
            std::string nextMember = currentToken.getValue();
            eat(TokenTypes::Identifier);
            memberPath.push_back(nextMember);
            // expr = std::make_shared<MemberAccess>(rootIdentifier, memberPath);  // Pass full path
            expr = std::make_shared<MemberAccess>(expr, memberPath);  // Pass full path
        }

        // Pointer member access (->)
        else if (currentToken.getType() == TokenTypes::Arrow) {
            eat(TokenTypes::Arrow);
            std::string nextMember = currentToken.getValue();
            eat(TokenTypes::Identifier);
            memberPath.push_back(nextMember);
            expr = std::make_shared<ArrowAccess>(expr, memberPath);  // Pass full path
        }

        // Index access
        else if (currentToken.getType() == TokenTypes::LeftBracket) {
            eat(TokenTypes::LeftBracket);
            auto index = parseExpression();
            eat(TokenTypes::RightBracket);
            expr = std::make_shared<IndexAccess>(expr, index);
        }

        // Assignment
        else if (isAssignmentExpression(currentToken.getType())) {
            return parseAssignment(expr);
        }

        // Done
        else {
            break;
        }
    }

    return expr;
}
