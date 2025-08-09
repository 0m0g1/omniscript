#include <omniscript/Statements/Statement.h>
#include <omniscript/Statements/AccessStatements.h>
#include <omniscript/Statements/CallableStatement.h>
#include <omniscript/Statements/ExpressionStatements.h>
#include <omniscript/Statements/AssignmentAndGetterStatements.h>
#include <omniscript/Core.h>
#include <omniscript/utils.h>
#include <omniscript/Tokens.h>
#include <omniscript/Parser.h>
#include <omniscript/Symboltable.h>
#include <omniscript/omniscript_pch.h>

namespace Omniscript {

std::shared_ptr<Statement> Parser::parseIdentifier() {
    Token startToken = currentToken;
    FileSpan span;
    span.start.line = startToken.getLine();
    span.start.col = startToken.getColumn();
    span.start.filePath = startToken.getFilePath();

    std::string rootIdentifier = currentToken.getValue();
    DEBUG_LOG("The root identifier is '" + rootIdentifier + "'.");
    eat(TokenTypes::Identifier, [&]() {
        std::string suggestion = Console::formatString(
            "To resolve this:\n"
            "1. Provide a valid identifier\n"
            "2. Check identifier syntax\n"
            "3. Expected token: identifier, found '%s'",
            getTokenTypeName(currentToken.getType()).c_str()
        );
        console.reportError(
            Console::SYNTAX_ERROR,
            Console::formatString("Expected identifier, found '%s'", 
                getTokenTypeName(currentToken.getType()).c_str()),
            suggestion,
            span
        );
    });
    
    std::shared_ptr<Statement> expr = std::make_shared<GetVariable>(rootIdentifier);
    expr->setPosition(startToken, currentToken);

    std::vector<std::string> accessContext = { rootIdentifier };
    std::string currentMember = rootIdentifier;
    
    while (true) {
        Token currentStartToken = currentToken;
        
        if (currentToken.getType() == TokenTypes::LeftParen) {
            // Function/method call
            std::vector<std::shared_ptr<Statement>> args = parseArguments();
            if (args.empty() && currentToken.getType() != TokenTypes::RightParen) {
                console.reportError(
                    Console::SYNTAX_ERROR,
                    "Failed to parse function call arguments",
                    "To resolve this:\n1. Verify argument syntax\n2. Check for valid expressions\n3. Ensure proper parentheses",
                    span
                );
                return expr;
            }
            
            // If the current expression is a MemberAccess, mark it as a call and set arguments
            if (auto memberAccess = std::dynamic_pointer_cast<MemberAccess>(expr)) {
                memberAccess->setArguments(args);
                memberAccess->setPosition(currentStartToken, previousToken);
                memberAccess->setSpan(span);
                DEBUG_LOG("Marked MemberAccess as call: " + memberAccess->toString());
                continue;
            }
            
            // For other expressions, create Call with proper context
            auto call = std::make_shared<Call>(expr, currentMember, args);
            call->setPosition(currentStartToken, previousToken);

            
            // Set access context (excluding the current member being called)
            std::vector<std::string> callContext = accessContext;
            if (!callContext.empty()) {
                callContext.pop_back();
            }
            call->setAccessContext(callContext);
            
            std::string contextStr = "empty";
            if (!callContext.empty()) {
                contextStr = "";
                for (size_t i = 0; i < callContext.size(); ++i) {
                    if (i > 0) contextStr += ".";
                    contextStr += callContext[i];
                }
            }
            
            DEBUG_LOG("Created call to '" + currentMember + "' with context: " + contextStr);
            
            expr = call;
            continue;
        }
        
        else if (currentToken.getType() == TokenTypes::LeftBrace) {
            // Constructor call
            std::vector<std::shared_ptr<Statement>> args = parseArguments(
                TokenTypes::LeftBrace, TokenTypes::RightBrace, TokenTypes::Colon);
            if (args.empty() && currentToken.getType() != TokenTypes::RightBrace) {
                console.reportError(
                    Console::SYNTAX_ERROR,
                    "Failed to parse constructor call arguments",
                    "To resolve this:\n1. Verify argument syntax\n2. Check for valid expressions\n3. Ensure proper braces",
                    span
                );
                return expr;
            }
            
            auto constructor = std::make_shared<ObjectConstructorStatement>(expr, currentMember, "", args);
            constructor->setPosition(currentStartToken, previousToken);
            constructor->setSpan(span);
            
            // Set access context for constructor
            std::vector<std::string> constructorContext = accessContext;
            if (!constructorContext.empty()) {
                constructorContext.pop_back();
            }
            
            if (auto ctxAware = std::dynamic_pointer_cast<ContextAwareStatement>(constructor)) {
                ctxAware->setAccessContext(constructorContext);
            }
            
            expr = constructor;
            continue;
        }
        
        else if (currentToken.getType() == TokenTypes::Dot || 
                 currentToken.getType() == TokenTypes::ScopeResolution) {
            // Member access
            TokenTypes op = currentToken.getType();
            eat(currentToken.getType(), [&]() {
                std::string suggestion = Console::formatString(
                    "To resolve this:\n"
                    "1. Use '.' or '::' for member access\n"
                    "2. Check member access syntax\n"
                    "3. Expected token: '.' or '::', found '%s'",
                    getTokenTypeName(currentToken.getType()).c_str()
                );
                console.reportError(
                    Console::SYNTAX_ERROR,
                    Console::formatString("Expected '.' or '::' for member access, found '%s'", 
                        getTokenTypeName(currentToken.getType()).c_str()),
                    suggestion,
                    span
                );
            });
            std::string nextMember = currentToken.getValue();
            eat(TokenTypes::Identifier, [&]() {
                std::string suggestion = Console::formatString(
                    "To resolve this:\n"
                    "1. Provide a valid member name after '.' or '::'\n"
                    "2. Check member access syntax\n"
                    "3. Expected token: identifier, found '%s'",
                    getTokenTypeName(currentToken.getType()).c_str()
                );
                console.reportError(
                    Console::SYNTAX_ERROR,
                    Console::formatString("Expected identifier for member name, found '%s'", 
                        getTokenTypeName(currentToken.getType()).c_str()),
                    suggestion,
                    span
                );
            });
            
            auto memberAccess = std::make_shared<MemberAccess>(expr, nextMember);
            memberAccess->setPosition(currentStartToken, previousToken);
            memberAccess->setSpan(span);
            
            // Update access context
            accessContext.push_back(nextMember);
            currentMember = nextMember;
            
            // Set access context (excluding the current member)
            std::vector<std::string> memberContext = accessContext;
            if (!memberContext.empty()) {
                memberContext.pop_back();
            }
            memberAccess->setAccessContext(memberContext);
            
            std::string contextStr = "empty";
            if (!memberContext.empty()) {
                contextStr = "";
                for (size_t i = 0; i < memberContext.size(); ++i) {
                    if (i > 0) contextStr += ".";
                    contextStr += memberContext[i];
                }
            }
            
            DEBUG_LOG("Created member access to '" + nextMember + "' with context: " + contextStr);
            
            expr = memberAccess;
            continue;
        }
        
        else if (currentToken.getType() == TokenTypes::Arrow) {
            // Arrow access (pointer dereference)
            eat(TokenTypes::Arrow, [&]() {
                std::string suggestion = Console::formatString(
                    "To resolve this:\n"
                    "1. Use '->' for pointer member access\n"
                    "2. Check arrow access syntax\n"
                    "3. Expected token: '->', found '%s'",
                    getTokenTypeName(currentToken.getType()).c_str()
                );
                console.reportError(
                    Console::SYNTAX_ERROR,
                    Console::formatString("Expected '->' for pointer member access, found '%s'", 
                        getTokenTypeName(currentToken.getType()).c_str()),
                    suggestion,
                    span
                );
            });
            std::string nextMember = currentToken.getValue();
            eat(TokenTypes::Identifier, [&]() {
                std::string suggestion = Console::formatString(
                    "To resolve this:\n"
                    "1. Provide a valid member name after '->'\n"
                    "2. Check arrow access syntax\n"
                    "3. Expected token: identifier, found '%s'",
                    getTokenTypeName(currentToken.getType()).c_str()
                );
                console.reportError(
                    Console::SYNTAX_ERROR,
                    Console::formatString("Expected identifier for member name, found '%s'", 
                        getTokenTypeName(currentToken.getType()).c_str()),
                    suggestion,
                    span
                );
            });
            
            auto arrowAccess = std::make_shared<ArrowAccess>(expr, nextMember);
            arrowAccess->setPosition(currentStartToken, previousToken);
            arrowAccess->setSpan(span);
            
            // Update context
            accessContext.push_back(nextMember);
            currentMember = nextMember;
            
            // Set access context
            std::vector<std::string> arrowContext = accessContext;
            if (!arrowContext.empty()) {
                arrowContext.pop_back();
            }
            arrowAccess->setAccessContext(arrowContext);
            
            expr = arrowAccess;
            continue;
        }
        
        else if (currentToken.getType() == TokenTypes::LeftBracket) {
            // Array/index access
            eat(TokenTypes::LeftBracket, [&]() {
                std::string suggestion = Console::formatString(
                    "To resolve this:\n"
                    "1. Start index access with '['\n"
                    "2. Check array access syntax\n"
                    "3. Expected token: '[', found '%s'",
                    getTokenTypeName(currentToken.getType()).c_str()
                );
                console.reportError(
                    Console::SYNTAX_ERROR,
                    Console::formatString("Expected '[' for index access, found '%s'", 
                        getTokenTypeName(currentToken.getType()).c_str()),
                    suggestion,
                    span
                );
            });
            auto index = parseExpression();
            if (!index) {
                console.reportError(
                    Console::SYNTAX_ERROR,
                    "Invalid index expression",
                    "To resolve this:\n1. Provide a valid index expression\n2. Check expression syntax\n3. Ensure valid literals or identifiers",
                    span
                );
                return expr;
            }
            eat(TokenTypes::RightBracket, [&]() {
                std::string suggestion = Console::formatString(
                    "To resolve this:\n"
                    "1. Close index access with ']'\n"
                    "2. Check for matching brackets\n"
                    "3. Expected token: ']', found '%s'",
                    getTokenTypeName(currentToken.getType()).c_str()
                );
                console.reportError(
                    Console::SYNTAX_ERROR,
                    Console::formatString("Expected ']' to close index access, found '%s'", 
                        getTokenTypeName(currentToken.getType()).c_str()),
                    suggestion,
                    span
                );
            });
            expr = std::make_shared<IndexAccess>(expr, index);
            expr->setPosition(currentStartToken, previousToken);
            expr->setSpan(span);
            continue;
        }
        
        else if (isGenericCallOrConstructor()) {
            // Generic function call
            std::vector<std::string> typeParams = parseTypeParametersForCall();
            if (typeParams.empty()) {
                console.reportError(
                    Console::SYNTAX_ERROR,
                    "Invalid generic type parameters",
                    "To resolve this:\n1. Verify generic type syntax\n2. Ensure valid type identifiers\n3. Check type parameter syntax",
                    span
                );
                return expr;
            }
            std::string specializedName = generateSpecializedNameForCall(currentMember, typeParams);
            DEBUG_LOG("Generated Specialized Name: " + specializedName);
            
            std::vector<std::shared_ptr<Statement>> args = parseArguments();
            if (args.empty() && currentToken.getType() != TokenTypes::RightParen) {
                console.reportError(
                    Console::SYNTAX_ERROR,
                    "Failed to parse generic call arguments",
                    "To resolve this:\n1. Verify argument syntax\n2. Check for valid expressions\n3. Ensure proper parentheses",
                    span
                );
                return expr;
            }
            
            auto genericCall = std::make_shared<Call>(expr, specializedName, args);
            genericCall->setPosition(currentStartToken, previousToken);
            genericCall->setSpan(span);
            
            // Set access context
            std::vector<std::string> genericContext = accessContext;
            if (!genericContext.empty()) {
                genericContext.pop_back();
            }
            genericCall->setAccessContext(genericContext);
            
            expr = genericCall;
            continue;
        }
        
        else if (isAssignmentExpression(currentToken.getType())) {
            // Assignment
            expr = parseAssignment(expr);
            if (!expr) {
                console.reportError(
                    Console::SYNTAX_ERROR,
                    "Invalid assignment expression",
                    "To resolve this:\n1. Verify assignment syntax\n2. Check for valid right-hand side expression\n3. Ensure proper operator usage",
                    span
                );
                return expr;
            }
            expr->setPosition(currentStartToken, previousToken);
            expr->setSpan(span);
            break;
        }
        
        else {
            // End of chain
            break;
        }
    }
    
    // Set final access context for the entire expression
    if (auto ctxAware = std::dynamic_pointer_cast<ContextAwareStatement>(expr)) {
        std::vector<std::string> finalContext = accessContext;
        if ((std::dynamic_pointer_cast<Call>(expr) || std::dynamic_pointer_cast<MemberAccess>(expr)) && !finalContext.empty()) {
            finalContext.pop_back();
        }
        ctxAware->setAccessContext(finalContext);
        DEBUG_LOG("Final identifier access context: " + ctxAware->getContextAsString());
    }

    span.end.line = previousToken.getLine();
    span.end.col = previousToken.getColumn();
    span.end.filePath = previousToken.getFilePath();
    expr->setPosition(startToken, currentToken);
    
    return expr;
}

} // namespace Omniscript
