#include <omniscript/Core.h>
#include <omniscript/utils.h>
#include <omniscript/engine/parser.h>
#include <omniscript/engine/tokens.h>
#include <omniscript/engine/Statement.h>
#include <omniscript/engine/Symboltable.h>
#include <omniscript/omniscript_pch.h>


//Todo:: Add proper error messages for various exceptions
std::shared_ptr<ForLoop> Parser::parseForLoop() {
    eat(TokenTypes::For);
    eat(TokenTypes::LeftParen);
    std::shared_ptr<Statement> initialization;
    if (currentToken.getType() == TokenTypes::Semicolon) {
        eat(TokenTypes::Semicolon);
    } else {
        DEBUG_LOG("Parsed the for assignment expression");
        initialization = parseAssignment();
        eat(TokenTypes::Semicolon); 
    }
    std::shared_ptr<Statement> condition;
    if (currentToken.getType() != TokenTypes::Semicolon) {
        condition = parseExpression();
        DEBUG_LOG("Parsed the for loop's condition " + condition->toString());
    }
    eat(TokenTypes::Semicolon);
    std::shared_ptr<Statement> increment;
    if (currentToken.getType() != TokenTypes::RightParen) {
        increment = parseExpression();
        DEBUG_LOG("Parsed the for loop's update expression " + increment->toString());
    }
    DEBUG_LOG("Parsed the for loops update expression");
    eat(TokenTypes::RightParen);
    auto body = std::dynamic_pointer_cast<BlockStatement>(parseBlock());
    DEBUG_LOG("Parsed the for loops body");

    return std::make_shared<ForLoop>(initialization, condition, increment, body);
}

std::shared_ptr<ContinueStatement> Parser::parseContinue() {
    eat(TokenTypes::Continue);
    if (currentToken.getType() != TokenTypes::Semicolon || currentToken.getType() != TokenTypes::Newline) {
        eat(TokenTypes::Semicolon);
    }
    eat(currentToken.getType());
    return std::make_shared<ContinueStatement>();
}

std::shared_ptr<BreakStatement> Parser::parseBreak() {
    eat(TokenTypes::Break);
    if (currentToken.getType() != TokenTypes::Semicolon || currentToken.getType() != TokenTypes::Newline) {
        eat(TokenTypes::Semicolon);
    }
    eat(currentToken.getType());
    return std::make_shared<BreakStatement>();
}

// Parse if statements
std::shared_ptr<Statement> Parser::parseIfStatement() {
    std::vector<std::shared_ptr<Statement>> conditions;
    std::vector<std::shared_ptr<BlockStatement>> bodies;
    std::shared_ptr<BlockStatement> elseBody = nullptr;

    // Parse initial 'if'
    eat(TokenTypes::If);
    eat(TokenTypes::LeftParen);
    auto condition = parseExpression();
    eat(TokenTypes::RightParen);
    auto body = parseBlock();

    conditions.push_back(condition);
    bodies.push_back(std::dynamic_pointer_cast<BlockStatement>(body));

    // Parse any number of 'else if' branches
    while (currentToken.getType() == TokenTypes::Else_if) {
        eat(TokenTypes::Else_if);
        eat(TokenTypes::LeftParen);
        auto elseIfCondition = parseExpression();
        eat(TokenTypes::RightParen);
        auto elseIfBlock = parseBlock();

        conditions.push_back(elseIfCondition);
        bodies.push_back(std::dynamic_pointer_cast<BlockStatement>(elseIfBlock));
    }

    // Optional 'else'
    if (currentToken.getType() == TokenTypes::Else) {
        eat(TokenTypes::Else);
        elseBody = std::dynamic_pointer_cast<BlockStatement>(parseBlock());
    }

    auto statement = std::make_shared<IfStatement>(conditions, bodies, elseBody);
    DEBUG_LOG("Parsed IfStatement with " + std::to_string(conditions.size()) + " branches");
    return statement;
}

// Parse while loops
std::shared_ptr<Statement> Parser::parseWhileStatement() {
    eat(TokenTypes::While); // Consume the 'while' keyword
    // Additional logic to parse the condition and body of the while loop would go here
    eat(TokenTypes::LeftParen);
    auto condition = parseExpression();
    eat(TokenTypes::RightParen);

    auto body = std::dynamic_pointer_cast<BlockStatement>(parseBlock());

    DEBUG_LOG("Parsed while statement");
    return std::make_shared<WhileStatement>(condition, body);
}

// Parse return statements
std::shared_ptr<ReturnStatement> Parser::parseReturnStatement() {
    eat(TokenTypes::Return); // Consume the 'return' keyword
    if (currentToken.getType() != TokenTypes::Semicolon) {
        std::shared_ptr<Statement> value = parseExpression();
        return std::make_shared<ReturnStatement>(value);
        // if (auto stmt = std::dynamic_pointer_cast<TypedStatement>(value)) {
        //     return std::make_shared<ReturnStatement>(value);
        // } else {
        //     console.error("Unable to determine the return type");
        // }
    }
    return std::make_shared<ReturnStatement>();
}
