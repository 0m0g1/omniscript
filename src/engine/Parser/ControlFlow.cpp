#include <omniscript/Core.h>
#include <omniscript/utils.h>
#include <omniscript/engine/Parser.h>
#include <omniscript/engine/tokens.h>
#include <omniscript/engine/Statement.h>
#include <omniscript/engine/Symboltable.h>
#include <omniscript/omniscript_pch.h>

std::shared_ptr<ForLoop> Parser::parseForLoop() {
    Token startToken = currentToken;
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

    auto forLoop = std::make_shared<ForLoop>(initialization, condition, increment, body);
    forLoop->setPosition(startToken);
    return forLoop;
}

std::shared_ptr<ContinueStatement> Parser::parseContinue() {
    Token startToken = currentToken;
    eat(TokenTypes::Continue);
    if (currentToken.getType() != TokenTypes::Semicolon || currentToken.getType() != TokenTypes::Newline) {
        eat(TokenTypes::Semicolon);
    }
    eat(currentToken.getType());
    auto continueStmt = std::make_shared<ContinueStatement>();
    continueStmt->setPosition(startToken);
    return continueStmt;
}

std::shared_ptr<BreakStatement> Parser::parseBreak() {
    Token startToken = currentToken;
    eat(TokenTypes::Break);
    if (currentToken.getType() != TokenTypes::Semicolon || currentToken.getType() != TokenTypes::Newline) {
        eat(TokenTypes::Semicolon);
    }
    eat(currentToken.getType());
    auto breakStmt = std::make_shared<BreakStatement>();
    breakStmt->setPosition(currentToken);
    return breakStmt;
}

std::shared_ptr<Statement> Parser::parseIfStatement() {
    Token startToken = currentToken;
    std::vector<std::shared_ptr<Statement>> conditions;
    std::vector<std::shared_ptr<BlockStatement>> bodies;
    std::shared_ptr<BlockStatement> elseBody = nullptr;

    eat(TokenTypes::If);
    eat(TokenTypes::LeftParen);
    auto condition = parseExpression();
    eat(TokenTypes::RightParen);
    auto body = parseBlock();

    conditions.push_back(condition);
    bodies.push_back(std::dynamic_pointer_cast<BlockStatement>(body));

    while (currentToken.getType() == TokenTypes::Else_if) {
        eat(TokenTypes::Else_if);
        eat(TokenTypes::LeftParen);
        auto elseIfCondition = parseExpression();
        eat(TokenTypes::RightParen);
        auto elseIfBlock = parseBlock();

        conditions.push_back(elseIfCondition);
        bodies.push_back(std::dynamic_pointer_cast<BlockStatement>(elseIfBlock));
    }

    if (currentToken.getType() == TokenTypes::Else) {
        eat(TokenTypes::Else);
        elseBody = std::dynamic_pointer_cast<BlockStatement>(parseBlock());
    }

    auto statement = std::make_shared<IfStatement>(conditions, bodies, elseBody);
    statement->setPosition(startToken);
    DEBUG_LOG("Parsed IfStatement with " + std::to_string(conditions.size()) + " branches");
    return statement;
}

std::shared_ptr<Statement> Parser::parseWhileStatement() {
    Token startToken = currentToken;
    eat(TokenTypes::While);

    eat(TokenTypes::LeftParen);
    auto condition = parseExpression();
    eat(TokenTypes::RightParen);

    auto body = std::dynamic_pointer_cast<BlockStatement>(parseBlock());

    DEBUG_LOG("Parsed while statement");
    auto whileLoop = std::make_shared<WhileStatement>(condition, body);
    whileLoop->setPosition(startToken);
    return whileLoop;
}


std::shared_ptr<ReturnStatement> Parser::parseReturnStatement() {
    Token startToken = currentToken;
    eat(TokenTypes::Return);
    std::shared_ptr<ReturnStatement> result;

    if (currentToken.getType() != TokenTypes::Semicolon) {
        std::shared_ptr<Statement> value = parseExpression();
        result = std::make_shared<ReturnStatement>(value);
    } else {
        result = std::make_shared<ReturnStatement>();
    }

    result->setPosition(startToken);
    return result;
}
