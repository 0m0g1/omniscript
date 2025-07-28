#include <omniscript/Statements/AccessStatements.h>
#include <omniscript/Statements/CallableStatement.h>
#include <omniscript/Statements/ExpressionStatements.h>
#include <omniscript/Statements/AssignmentAndGetterStatements.h>

#include <omniscript/Core.h>
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
    std::vector<std::string> accessContext = { rootIdentifier };
    std::string currentMember = rootIdentifier;
    
    while (true) {
        Token startToken = currentToken;
        expr->setPosition(startToken, previousToken);
        
        if (currentToken.getType() == TokenTypes::LeftParen) {
            // Function/method call
            std::vector<std::shared_ptr<Statement>> args = parseArguments();
            
            // If the current expression is a MemberAccess, mark it as a call and set arguments
            if (auto memberAccess = std::dynamic_pointer_cast<MemberAccess>(expr)) {
                memberAccess->setArguments(args); // This sets both arguments and isCall = true
                
                DEBUG_LOG("Marked MemberAccess as call: " + memberAccess->toString());
                continue; // Don't create a separate Call object
            }
            
            // For other expressions, create Call with proper context
            auto call = std::make_shared<Call>(expr, currentMember, args);
            
            // Set access context (excluding the current member being called)
            std::vector<std::string> callContext = accessContext;
            if (!callContext.empty()) {
                callContext.pop_back(); // Remove the method name from context
            }
            call->setAccessContext(callContext);
            
            // Create context string for debug logging
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
            
            auto constructor = std::make_shared<ObjectConstructorStatement>(expr, currentMember, "", args);
            
            // Set access context for constructor if it supports it
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
            eat(currentToken.getType());
            std::string nextMember = currentToken.getValue();
            eat(TokenTypes::Identifier);
            
            // Create member access
            auto memberAccess = std::make_shared<MemberAccess>(expr, nextMember);
            
            // Update access context
            accessContext.push_back(nextMember);
            currentMember = nextMember;
            
            // Set access context (excluding the current member)
            std::vector<std::string> memberContext = accessContext;
            if (!memberContext.empty()) {
                memberContext.pop_back();
            }
            memberAccess->setAccessContext(memberContext);
            
            // Create context string for debug logging
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
            eat(TokenTypes::Arrow);
            std::string nextMember = currentToken.getValue();
            eat(TokenTypes::Identifier);
            
            auto arrowAccess = std::make_shared<ArrowAccess>(expr, nextMember);
            
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
            eat(TokenTypes::LeftBracket);
            auto index = parseExpression();
            eat(TokenTypes::RightBracket);
            expr = std::make_shared<IndexAccess>(expr, index);
            continue;
        }
        
        else if (isGenericCallOrConstructor()) {
            // Generic function call
            std::vector<std::string> typeParams = parseTypeParametersForCall();
            std::string specializedName = generateSpecializedNameForCall(currentMember, typeParams);
            DEBUG_LOG("Generated Specialized Name: " + specializedName);
            
            std::vector<std::shared_ptr<Statement>> args = parseArguments();
            
            auto genericCall = std::make_shared<Call>(expr, specializedName, args);
            
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
            break;
        }
        
        else {
            // End of chain
            break;
        }
    }
    
    // Set final access context for the entire expression
    if (auto ctxAware = std::dynamic_pointer_cast<ContextAwareStatement>(expr)) {
        // For the final expression, we want the full context except the last element
        // if it's a Call (since the last element is the method being called)
        std::vector<std::string> finalContext = accessContext;
        
        if (std::dynamic_pointer_cast<Call>(expr) && !finalContext.empty()) {
            finalContext.pop_back(); // Remove method name from context
        } else if (std::dynamic_pointer_cast<MemberAccess>(expr) && !finalContext.empty()) {
            finalContext.pop_back(); // Remove member name from context
        }
        
        ctxAware->setAccessContext(finalContext);
        DEBUG_LOG("Final identifier access context: " + ctxAware->getContextAsString());
    }
    
    return expr;
}
