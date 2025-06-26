#include <omniscript/Statements/AccessStatements.h>
#include <omniscript/Statements/CallableStatement.h>
#include <omniscript/Statements/ExpressionStatements.h>
#include <omniscript/Statements/AssignmentAndGetterStatements.h>

#include <omniscript/Core/Core.h>
#include <omniscript/utils.h>
#include <omniscript/Tokens.h>
#include <omniscript/Parser.h>
#include <omniscript/omniscript_pch.h>
#include <omniscript/Statement.h>
#include <omniscript/Symboltable.h>

std::shared_ptr<Statement> Parser::parseIdentifier() {
    std::string rootIdentifier = currentToken.getValue();
    DEBUG_LOG("The root identifier is '" + rootIdentifier + "'.");
    eat(TokenTypes::Identifier);

    std::shared_ptr<Statement> expr = std::make_shared<GetVariable>(rootIdentifier);
    std::vector<std::string> memberPath = {};
    std::vector<std::string> accessContext = { rootIdentifier };
    std::string member = rootIdentifier;

    while (true) {
        Token startToken = currentToken;
        expr->setPosition(startToken);
        if (currentToken.getType() == TokenTypes::LeftParen) {
            std::vector<std::shared_ptr<Statement>> args = parseArguments();
            if (memberPath.empty()) {
                expr = std::make_shared<Call>(expr, member, args);
            } else {
                expr = std::make_shared<Call>(expr, memberPath.back(), args);
            }
            continue;
        }

        else if (currentToken.getType() == TokenTypes::LeftBrace) {
            std::vector<std::shared_ptr<Statement>> args = parseArguments(TokenTypes::LeftBrace, TokenTypes::RightBrace, TokenTypes::Colon);
            expr = std::make_shared<ObjectConstructorStatement>(expr, memberPath.back(), "", args);
            continue;
        }

        else if (currentToken.getType() == TokenTypes::Dot || currentToken.getType() == TokenTypes::ScopeResolution) {
            eat(currentToken.getType()); // Eat dot or scope resolution
            std::string nextMember = currentToken.getValue();
            member = nextMember;
            eat(TokenTypes::Identifier);
            memberPath.push_back(nextMember);
            accessContext.push_back(nextMember);
            expr = std::make_shared<MemberAccess>(expr, member);  // Pass full path
            continue;
        }

        else if (currentToken.getType() == TokenTypes::Arrow) {
            eat(TokenTypes::Arrow);
            std::string nextMember = currentToken.getValue();
            member = nextMember;
            eat(TokenTypes::Identifier);
            memberPath.push_back(nextMember);
            expr = std::make_shared<ArrowAccess>(expr, member);
            continue;
        }

        else if (currentToken.getType() == TokenTypes::LeftBracket) {
            eat(TokenTypes::LeftBracket);
            auto index = parseExpression();
            eat(TokenTypes::RightBracket);
            expr = std::make_shared<IndexAccess>(expr, index);
            continue;
        }

        else if (isGenericCallOrConstructor()) {
            std::vector<std::string> typeParams = parseTypeParametersForCall();
            std::string specializedName = generateSpecializedNameForCall(memberPath.back(), typeParams);
            DEBUG_LOG("Generated Specialized Name: " + specializedName);
            std::vector<std::shared_ptr<Statement>> args = parseArguments();
            expr = std::make_shared<Call>(expr, specializedName, args);
            continue;
        }

        else if (isAssignmentExpression(currentToken.getType())) {
            expr = parseAssignment(expr);
            break;
        }

        else {
            break;
        }
    }

    if (auto ctxAware = std::dynamic_pointer_cast<ContextAwareStatement>(expr)) {
        if (!accessContext.empty()) {
            accessContext.pop_back();
        }
        ctxAware->setAccessContext(accessContext);
        DEBUG_LOG("Identifier access context\n" + ctxAware->getContextAsString());
    }
    
    return expr;
}
