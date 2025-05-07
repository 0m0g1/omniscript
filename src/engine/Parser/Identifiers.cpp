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
    
    // Start with the base identifier as the initial statement
    std::shared_ptr<Statement> expr = std::make_shared<GetVariable>(rootIdentifier);
    std::string member = rootIdentifier;

    // Loop to handle member accesses and function calls
    while (true) {
        if (currentToken.getType() == TokenTypes::LeftParen || currentToken.getType() == TokenTypes::LessThan) {
            // Handle function calls with generics
            if (isGenericCallOrConstructor()) {
                std::vector<std::string> typeParams = parseTypeParametersForCall();
                std::string specializedName = generateSpecializedNameForCall(member, typeParams);
                DEBUG_LOG("Generated Specialized Name: " + specializedName);
                
                std::vector<std::shared_ptr<Statement>> args = parseArguments();
                expr = std::make_shared<Call>(expr, specializedName, args);  // Use the specialized name
            } else {
                // Normal function call
                std::vector<std::shared_ptr<Statement>> args = parseArguments();
                expr = std::make_shared<Call>(expr, member, args);
            }
        }
        
        // Object constructor
        else if (currentToken.getType() == TokenTypes::LeftBrace) {
            std::vector<std::shared_ptr<Statement>> args = parseArguments(TokenTypes::LeftBrace, TokenTypes::RightBrace, TokenTypes::Colon);
            expr = std::make_shared<ObjectConstructorStatement>(expr, member, "", args);
        }
        
        // Member access (.)
        else if (currentToken.getType() == TokenTypes::Dot || currentToken.getType() == TokenTypes::ScopeResolution) {
            eat(currentToken.getType()); // Consume the dot or scope resolution operator
            member = currentToken.getValue();
            eat(TokenTypes::Identifier);  // Eat the member name
            DEBUG_LOG(expr->toString());
            expr = std::make_shared<MemberAccess>(expr, member);  // Add a member access
        }

        // Pointer member access (->)
        else if (currentToken.getType() == TokenTypes::Minus && lexer.peekToken(1).getType() == TokenTypes::GreaterThan) {
            eat(TokenTypes::Minus);
            eat(TokenTypes::GreaterThan);
            member = currentToken.getValue();
            eat(TokenTypes::Identifier);  // Eat the member name
            expr = std::make_shared<ArrowAccess>(expr, member);  // Add a pointer member access
        }

        // Index access ([])
        else if (currentToken.getType() == TokenTypes::LeftBracket) {
            eat(TokenTypes::LeftBracket);
            auto index = parseExpression();
            eat(TokenTypes::RightBracket);
            expr = std::make_shared<IndexAccess>(expr, index);  // Add an index access
        }

        // Assignment handling
        else if (isAssignmentExpression(currentToken.getType())) {
            return parseAssignment(expr);
        }

        // Break if no more valid tokens
        else {
            break;
        }
    }

    return expr;
}
