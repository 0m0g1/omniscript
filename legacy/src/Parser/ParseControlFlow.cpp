#include <omniscript/Statements/Statement.h>
#include <omniscript/Statements/ControlFlowStatements.h>

#include <omniscript/Core.h>
#include <omniscript/utils.h>
#include <omniscript/Parser.h>
#include <omniscript/Tokens.h>
#include <omniscript/omniscript_pch.h>
#include <omniscript/Statements/Statement.h>
#include <omniscript/Symboltable.h>

namespace Omniscript {

std::shared_ptr<Statement> Parser::parseForLoop() {
    Token startToken = currentToken;
    FileSpan span;
    span.start.line = startToken.getLine();
    span.start.col = startToken.getColumn();
    span.start.filePath = startToken.getFilePath();

    DEBUG_LOG("Parsing for loop");
    eat(TokenTypes::For, [&]() {
        std::string suggestion = Console::formatString(
            "To resolve this:\n"
            "1. Start for loop with 'for' keyword\n"
            "2. Check for correct syntax\n"
            "3. Expected token: 'for', found '%s'",
            getTokenTypeName(currentToken.getType()).c_str()
        );
        console.reportError(
            Console::SYNTAX_ERROR,
            Console::formatString("Expected 'for' keyword, found '%s'", 
                getTokenTypeName(currentToken.getType()).c_str()),
            suggestion,
            span
        );
    });

    eat(TokenTypes::LeftParen, [&]() {
        std::string suggestion = Console::formatString(
            "To resolve this:\n"
            "1. Use '(' after 'for' keyword\n"
            "2. Check for correct for loop syntax\n"
            "3. Expected token: '(', found '%s'",
            getTokenTypeName(currentToken.getType()).c_str()
        );
        console.reportError(
            Console::SYNTAX_ERROR,
            Console::formatString("Expected '(' for for loop, found '%s'", 
                getTokenTypeName(currentToken.getType()).c_str()),
            suggestion,
            span
        );
    });

    std::shared_ptr<Statement> initialization;
    if (currentToken.getType() == TokenTypes::Semicolon) {
        DEBUG_LOG("No initialization in for loop");
        eat(TokenTypes::Semicolon);
    } else {
        DEBUG_LOG("Parsing for loop initialization");
        initialization = parseAssignment();
        if (!initialization) {
            std::string suggestion = "To resolve this:\n"
                                   "1. Verify initialization syntax\n"
                                   "2. Check for valid variable declaration or assignment\n"
                                   "3. Ensure proper semicolon termination";
            console.reportError(
                Console::SYNTAX_ERROR,
                "Failed to parse for loop initialization",
                suggestion,
                span
            );
            return nullptr;
        }
        DEBUG_LOG("Parsed for loop initialization: " + initialization->toString());
        eat(TokenTypes::Semicolon, [&]() {
            std::string suggestion = Console::formatString(
                "To resolve this:\n"
                "1. Terminate initialization with ';'\n"
                "2. Check for correct for loop syntax\n"
                "3. Expected token: ';', found '%s'",
                getTokenTypeName(currentToken.getType()).c_str()
            );
            console.reportError(
                Console::SYNTAX_ERROR,
                Console::formatString("Expected ';' after for loop initialization, found '%s'", 
                    getTokenTypeName(currentToken.getType()).c_str()),
                suggestion,
                span
            );
        });
    }

    std::shared_ptr<Statement> condition;
    if (currentToken.getType() != TokenTypes::Semicolon) {
        DEBUG_LOG("Parsing for loop condition");
        condition = parseExpression();
        if (!condition) {
            std::string suggestion = "To resolve this:\n"
                                   "1. Verify condition expression syntax\n"
                                   "2. Check for valid boolean expression\n"
                                   "3. Ensure proper expression syntax";
            console.reportError(
                Console::SYNTAX_ERROR,
                "Failed to parse for loop condition",
                suggestion,
                span
            );
            return nullptr;
        }
        DEBUG_LOG("Parsed for loop condition: " + condition->toString());
    } else {
        DEBUG_LOG("No condition in for loop");
    }
    eat(TokenTypes::Semicolon, [&]() {
        std::string suggestion = Console::formatString(
            "To resolve this:\n"
            "1. Terminate condition with ';'\n"
            "2. Check for correct for loop syntax\n"
            "3. Expected token: ';', found '%s'",
            getTokenTypeName(currentToken.getType()).c_str()
        );
        console.reportError(
            Console::SYNTAX_ERROR,
            Console::formatString("Expected ';' after for loop condition, found '%s'", 
                getTokenTypeName(currentToken.getType()).c_str()),
            suggestion,
            span
        );
    });

    std::shared_ptr<Statement> increment;
    if (currentToken.getType() != TokenTypes::RightParen) {
        DEBUG_LOG("Parsing for loop update expression");
        increment = parseExpression();
        if (!increment) {
            std::string suggestion = "To resolve this:\n"
                                   "1. Verify update expression syntax\n"
                                   "2. Check for valid expression\n"
                                   "3. Ensure proper expression syntax";
            console.reportError(
                Console::SYNTAX_ERROR,
                "Failed to parse for loop update expression",
                suggestion,
                span
            );
            return nullptr;
        }
        DEBUG_LOG("Parsed for loop update expression: " + increment->toString());
    } else {
        DEBUG_LOG("No update expression in for loop");
    }
    eat(TokenTypes::RightParen, [&]() {
        std::string suggestion = Console::formatString(
            "To resolve this:\n"
            "1. Close for loop header with ')'\n"
            "2. Check for correct for loop syntax\n"
            "3. Expected token: ')', found '%s'",
            getTokenTypeName(currentToken.getType()).c_str()
        );
        console.reportError(
            Console::SYNTAX_ERROR,
            Console::formatString("Expected ')' after for loop update, found '%s'", 
                getTokenTypeName(currentToken.getType()).c_str()),
            suggestion,
            span
        );
    });

    DEBUG_LOG("Parsing for loop body");
    auto body = std::dynamic_pointer_cast<BlockStatement>(parseBlock());
    if (!body) {
        std::string suggestion = "To resolve this:\n"
                               "1. Verify block syntax\n"
                               "2. Check for valid block structure\n"
                               "3. Ensure block starts with '{' or is a valid statement";
        console.reportError(
            Console::SYNTAX_ERROR,
            "Failed to parse for loop body",
            suggestion,
            span
        );
        return nullptr;
    }
    DEBUG_LOG("Parsed for loop body");

    span.end.line = previousToken.getLine();
    span.end.col = previousToken.getColumn();
    span.end.filePath = previousToken.getFilePath();

    auto forLoop = std::make_shared<ForLoop>(initialization, condition, increment, body);
    forLoop->setPosition(startToken, currentToken);
    forLoop->setSpan(span);
    DEBUG_LOG("Completed parsing for loop");
    return forLoop;
}

std::shared_ptr<Statement> Parser::parseContinue() {
    Token startToken = currentToken;
    FileSpan span;
    span.start.line = startToken.getLine();
    span.start.col = startToken.getColumn();
    span.start.filePath = startToken.getFilePath();

    DEBUG_LOG("Parsing continue statement");
    eat(TokenTypes::Continue, [&]() {
        std::string suggestion = Console::formatString(
            "To resolve this:\n"
            "1. Use 'continue' keyword\n"
            "2. Check for correct syntax\n"
            "3. Expected token: 'continue', found '%s'",
            getTokenTypeName(currentToken.getType()).c_str()
        );
        console.reportError(
            Console::SYNTAX_ERROR,
            Console::formatString("Expected 'continue' keyword, found '%s'", 
                getTokenTypeName(currentToken.getType()).c_str()),
            suggestion,
            span
        );
    });

    if (currentToken.getType() == TokenTypes::Semicolon || currentToken.getType() == TokenTypes::Newline) {
        eat(currentToken.getType());
    } else {
        std::string suggestion = Console::formatString(
            "To resolve this:\n"
            "1. Terminate continue statement with ';' or newline\n"
            "2. Check for correct syntax\n"
            "3. Expected token: ';' or newline, found '%s'",
            getTokenTypeName(currentToken.getType()).c_str()
        );
        console.reportError(
            Console::SYNTAX_ERROR,
            Console::formatString("Expected ';' or newline after continue statement, found '%s'", 
                getTokenTypeName(currentToken.getType()).c_str()),
            suggestion,
            span
        );
        return nullptr;
    }

    span.end.line = previousToken.getLine();
    span.end.col = previousToken.getColumn();
    span.end.filePath = previousToken.getFilePath();

    auto continueStmt = std::make_shared<ContinueStatement>();
    continueStmt->setPosition(startToken, currentToken);
    continueStmt->setSpan(span);
    DEBUG_LOG("Completed parsing continue statement");
    return continueStmt;
}

std::shared_ptr<Statement> Parser::parseBreak() {
    Token startToken = currentToken;
    FileSpan span;
    span.start.line = startToken.getLine();
    span.start.col = startToken.getColumn();
    span.start.filePath = startToken.getFilePath();

    DEBUG_LOG("Parsing break statement");
    eat(TokenTypes::Break, [&]() {
        std::string suggestion = Console::formatString(
            "To resolve this:\n"
            "1. Use 'break' keyword\n"
            "2. Check for correct syntax\n"
            "3. Expected token: 'break', found '%s'",
            getTokenTypeName(currentToken.getType()).c_str()
        );
        console.reportError(
            Console::SYNTAX_ERROR,
            Console::formatString("Expected 'break' keyword, found '%s'", 
                getTokenTypeName(currentToken.getType()).c_str()),
            suggestion,
            span
        );
    });

    if (currentToken.getType() == TokenTypes::Semicolon || currentToken.getType() == TokenTypes::Newline) {
        eat(currentToken.getType());
    } else {
        std::string suggestion = Console::formatString(
            "To resolve this:\n"
            "1. Terminate break statement with ';' or newline\n"
            "2. Check for correct syntax\n"
            "3. Expected token: ';' or newline, found '%s'",
            getTokenTypeName(currentToken.getType()).c_str()
        );
        console.reportError(
            Console::SYNTAX_ERROR,
            Console::formatString("Expected ';' or newline after break statement, found '%s'", 
                getTokenTypeName(currentToken.getType()).c_str()),
            suggestion,
            span
        );
        return nullptr;
    }

    span.end.line = previousToken.getLine();
    span.end.col = previousToken.getColumn();
    span.end.filePath = previousToken.getFilePath();

    auto breakStmt = std::make_shared<BreakStatement>();
    breakStmt->setPosition(startToken, currentToken);
    breakStmt->setSpan(span);
    DEBUG_LOG("Completed parsing break statement");
    return breakStmt;
}

std::shared_ptr<Statement> Parser::parseIfStatement() {
    Token startToken = currentToken;
    FileSpan span;
    span.start.line = startToken.getLine();
    span.start.col = startToken.getColumn();
    span.start.filePath = startToken.getFilePath();

    DEBUG_LOG("Parsing if statement");
    eat(TokenTypes::If, [&]() {
        std::string suggestion = Console::formatString(
            "To resolve this:\n"
            "1. Start if statement with 'if' keyword\n"
            "2. Check for correct syntax\n"
            "3. Expected token: 'if', found '%s'",
            getTokenTypeName(currentToken.getType()).c_str()
        );
        console.reportError(
            Console::SYNTAX_ERROR,
            Console::formatString("Expected 'if' keyword, found '%s'", 
                getTokenTypeName(currentToken.getType()).c_str()),
            suggestion,
            span
        );
    });

    eat(TokenTypes::LeftParen, [&]() {
        std::string suggestion = Console::formatString(
            "To resolve this:\n"
            "1. Use '(' after 'if' keyword\n"
            "2. Check for correct if statement syntax\n"
            "3. Expected token: '(', found '%s'",
            getTokenTypeName(currentToken.getType()).c_str()
        );
        console.reportError(
            Console::SYNTAX_ERROR,
            Console::formatString("Expected '(' for if condition, found '%s'", 
                getTokenTypeName(currentToken.getType()).c_str()),
            suggestion,
            span
        );
    });

    DEBUG_LOG("Parsing if condition");
    auto condition = parseExpression();
    if (!condition) {
        std::string suggestion = "To resolve this:\n"
                               "1. Verify condition expression syntax\n"
                               "2. Check for valid boolean expression\n"
                               "3. Ensure proper expression syntax";
        console.reportError(
            Console::SYNTAX_ERROR,
            "Failed to parse if condition",
            suggestion,
            span
        );
        return nullptr;
    }
    DEBUG_LOG("Parsed if condition: " + condition->toString());

    eat(TokenTypes::RightParen, [&]() {
        std::string suggestion = Console::formatString(
            "To resolve this:\n"
            "1. Close condition with ')'\n"
            "2. Check for correct if statement syntax\n"
            "3. Expected token: ')', found '%s'",
            getTokenTypeName(currentToken.getType()).c_str()
        );
        console.reportError(
            Console::SYNTAX_ERROR,
            Console::formatString("Expected ')' after if condition, found '%s'", 
                getTokenTypeName(currentToken.getType()).c_str()),
            suggestion,
            span
        );
    });

    DEBUG_LOG("Parsing if body");
    auto body = parseBlock();
    if (!body) {
        std::string suggestion = "To resolve this:\n"
                               "1. Verify block syntax\n"
                               "2. Check for valid block structure\n"
                               "3. Ensure block starts with '{' or is a valid statement";
        console.reportError(
            Console::SYNTAX_ERROR,
            "Failed to parse if statement body",
            suggestion,
            span
        );
        return nullptr;
    }
    auto ifBody = std::dynamic_pointer_cast<BlockStatement>(body);
    if (!ifBody) {
        std::string suggestion = "To resolve this:\n"
                               "1. Ensure if body is a valid block\n"
                               "2. Check for proper block syntax\n"
                               "3. Use '{' to start block if multiple statements";
        console.reportError(
            Console::SYNTAX_ERROR,
            "If statement body must be a block statement",
            suggestion,
            span
        );
        return nullptr;
    }
    DEBUG_LOG("Parsed if body");

    std::vector<std::shared_ptr<Statement>> conditions = {condition};
    std::vector<std::shared_ptr<BlockStatement>> bodies = {ifBody};
    std::shared_ptr<BlockStatement> elseBody = nullptr;

    while (currentToken.getType() == TokenTypes::Else_if) {
        DEBUG_LOG("Parsing else if branch");
        eat(TokenTypes::Else_if, [&]() {
            std::string suggestion = Console::formatString(
                "To resolve this:\n"
                "1. Use 'else if' for additional conditions\n"
                "2. Check for correct syntax\n"
                "3. Expected token: 'else if', found '%s'",
                getTokenTypeName(currentToken.getType()).c_str()
            );
            console.reportError(
                Console::SYNTAX_ERROR,
                Console::formatString("Expected 'else if' keyword, found '%s'", 
                    getTokenTypeName(currentToken.getType()).c_str()),
                suggestion,
                span
            );
        });

        eat(TokenTypes::LeftParen, [&]() {
            std::string suggestion = Console::formatString(
                "To resolve this:\n"
                "1. Use '(' after 'else if' keyword\n"
                "2. Check for correct else if syntax\n"
                "3. Expected token: '(', found '%s'",
                getTokenTypeName(currentToken.getType()).c_str()
            );
            console.reportError(
                Console::SYNTAX_ERROR,
                Console::formatString("Expected '(' for else if condition, found '%s'", 
                    getTokenTypeName(currentToken.getType()).c_str()),
                suggestion,
                span
            );
        });

        auto elseIfCondition = parseExpression();
        if (!elseIfCondition) {
            std::string suggestion = "To resolve this:\n"
                                   "1. Verify else if condition syntax\n"
                                   "2. Check for valid boolean expression\n"
                                   "3. Ensure proper expression syntax";
            console.reportError(
                Console::SYNTAX_ERROR,
                "Failed to parse else if condition",
                suggestion,
                span
            );
            return nullptr;
        }
        DEBUG_LOG("Parsed else if condition: " + elseIfCondition->toString());

        eat(TokenTypes::RightParen, [&]() {
            std::string suggestion = Console::formatString(
                "To resolve this:\n"
                "1. Close else if condition with ')'\n"
                "2. Check for correct else if syntax\n"
                "3. Expected token: ')', found '%s'",
                getTokenTypeName(currentToken.getType()).c_str()
            );
            console.reportError(
                Console::SYNTAX_ERROR,
                Console::formatString("Expected ')' after else if condition, found '%s'", 
                    getTokenTypeName(currentToken.getType()).c_str()),
                suggestion,
                span
            );
        });

        auto elseIfBlock = parseBlock();
        if (!elseIfBlock) {
            std::string suggestion = "To resolve this:\n"
                                   "1. Verify else if block syntax\n"
                                   "2. Check for valid block structure\n"
                                   "3. Ensure block starts with '{' or is a valid statement";
            console.reportError(
                Console::SYNTAX_ERROR,
                "Failed to parse else if block",
                suggestion,
                span
            );
            return nullptr;
        }
        auto elseIfBody = std::dynamic_pointer_cast<BlockStatement>(elseIfBlock);
        if (!elseIfBody) {
            std::string suggestion = "To resolve this:\n"
                                   "1. Ensure else if body is a valid block\n"
                                   "2. Check for proper block syntax\n"
                                   "3. Use '{' to start block if multiple statements";
            console.reportError(
                Console::SYNTAX_ERROR,
                "Else if body must be a block statement",
                suggestion,
                span
            );
            return nullptr;
        }
        DEBUG_LOG("Parsed else if body");

        conditions.push_back(elseIfCondition);
        bodies.push_back(elseIfBody);
    }

    if (currentToken.getType() == TokenTypes::Else) {
        DEBUG_LOG("Parsing else branch");
        eat(TokenTypes::Else, [&]() {
            std::string suggestion = Console::formatString(
                "To resolve this:\n"
                "1. Use 'else' for else branch\n"
                "2. Check for correct syntax\n"
                "3. Expected token: 'else', found '%s'",
                getTokenTypeName(currentToken.getType()).c_str()
            );
            console.reportError(
                Console::SYNTAX_ERROR,
                Console::formatString("Expected 'else' keyword, found '%s'", 
                    getTokenTypeName(currentToken.getType()).c_str()),
                suggestion,
                span
            );
        });

        auto elseBlock = parseBlock();
        if (!elseBlock) {
            std::string suggestion = "To resolve this:\n"
                                   "1. Verify else block syntax\n"
                                   "2. Check for valid block structure\n"
                                   "3. Ensure block starts with '{' or is a valid statement";
            console.reportError(
                Console::SYNTAX_ERROR,
                "Failed to parse else block",
                suggestion,
                span
            );
            return nullptr;
        }
        elseBody = std::dynamic_pointer_cast<BlockStatement>(elseBlock);
        if (!elseBody) {
            std::string suggestion = "To resolve this:\n"
                                   "1. Ensure else body is a valid block\n"
                                   "2. Check for proper block syntax\n"
                                   "3. Use '{' to start block if multiple statements";
            console.reportError(
                Console::SYNTAX_ERROR,
                "Else body must be a block statement",
                suggestion,
                span
            );
            return nullptr;
        }
        DEBUG_LOG("Parsed else body");
    }

    span.end.line = previousToken.getLine();
    span.end.col = previousToken.getColumn();
    span.end.filePath = previousToken.getFilePath();

    auto statement = std::make_shared<IfStatement>(conditions, bodies, elseBody);
    statement->setPosition(startToken, currentToken);
    statement->setSpan(span);
    DEBUG_LOG("Completed parsing if statement with " + std::to_string(conditions.size()) + " branches");
    return statement;
}

std::shared_ptr<Statement> Parser::parseWhileStatement() {
    Token startToken = currentToken;
    FileSpan span;
    span.start.line = startToken.getLine();
    span.start.col = startToken.getColumn();
    span.start.filePath = startToken.getFilePath();

    DEBUG_LOG("Parsing while statement");
    eat(TokenTypes::While, [&]() {
        std::string suggestion = Console::formatString(
            "To resolve this:\n"
            "1. Start while loop with 'while' keyword\n"
            "2. Check for correct syntax\n"
            "3. Expected token: 'while', found '%s'",
            getTokenTypeName(currentToken.getType()).c_str()
        );
        console.reportError(
            Console::SYNTAX_ERROR,
            Console::formatString("Expected 'while' keyword, found '%s'", 
                getTokenTypeName(currentToken.getType()).c_str()),
            suggestion,
            span
        );
    });

    eat(TokenTypes::LeftParen, [&]() {
        std::string suggestion = Console::formatString(
            "To resolve this:\n"
            "1. Use '(' after 'while' keyword\n"
            "2. Check for correct while loop syntax\n"
            "3. Expected token: '(', found '%s'",
            getTokenTypeName(currentToken.getType()).c_str()
        );
        console.reportError(
            Console::SYNTAX_ERROR,
            Console::formatString("Expected '(' for while condition, found '%s'", 
                getTokenTypeName(currentToken.getType()).c_str()),
            suggestion,
            span
        );
    });

    DEBUG_LOG("Parsing while condition");
    auto condition = parseExpression();
    if (!condition) {
        std::string suggestion = "To resolve this:\n"
                               "1. Verify condition expression syntax\n"
                               "2. Check for valid boolean expression\n"
                               "3. Ensure proper expression syntax";
        console.reportError(
            Console::SYNTAX_ERROR,
            "Failed to parse while condition",
            suggestion,
            span
        );
        return nullptr;
    }
    DEBUG_LOG("Parsed while condition: " + condition->toString());

    eat(TokenTypes::RightParen, [&]() {
        std::string suggestion = Console::formatString(
            "To resolve this:\n"
            "1. Close while condition with ')'\n"
            "2. Check for correct while loop syntax\n"
            "3. Expected token: ')', found '%s'",
            getTokenTypeName(currentToken.getType()).c_str()
        );
        console.reportError(
            Console::SYNTAX_ERROR,
            Console::formatString("Expected ')' after while condition, found '%s'", 
                getTokenTypeName(currentToken.getType()).c_str()),
            suggestion,
            span
        );
    });

    DEBUG_LOG("Parsing while body");
    auto body = parseBlock();
    if (!body) {
        std::string suggestion = "To resolve this:\n"
                               "1. Verify block syntax\n"
                               "2. Check for valid block structure\n"
                               "3. Ensure block starts with '{' or is a valid statement";
        console.reportError(
            Console::SYNTAX_ERROR,
            "Failed to parse while loop body",
            suggestion,
            span
        );
        return nullptr;
    }
    auto whileBody = std::dynamic_pointer_cast<BlockStatement>(body);
    if (!whileBody) {
        std::string suggestion = "To resolve this:\n"
                               "1. Ensure while body is a valid block\n"
                               "2. Check for proper block syntax\n"
                               "3. Use '{' to start block if multiple statements";
        console.reportError(
            Console::SYNTAX_ERROR,
            "While loop body must be a block statement",
            suggestion,
            span
        );
        return nullptr;
    }
    DEBUG_LOG("Parsed while body");

    span.end.line = previousToken.getLine();
    span.end.col = previousToken.getColumn();
    span.end.filePath = previousToken.getFilePath();

    auto whileLoop = std::make_shared<WhileStatement>(condition, whileBody);
    whileLoop->setPosition(startToken, currentToken);
    whileLoop->setSpan(span);
    DEBUG_LOG("Completed parsing while statement");
    return whileLoop;
}

std::shared_ptr<Statement> Parser::parseReturnStatement() {
    Token startToken = currentToken;
    FileSpan span;
    span.start.line = startToken.getLine();
    span.start.col = startToken.getColumn();
    span.start.filePath = startToken.getFilePath();

    DEBUG_LOG("Parsing return statement");
    eat(TokenTypes::Return, [&]() {
        std::string suggestion = Console::formatString(
            "To resolve this:\n"
            "1. Use 'return' keyword\n"
            "2. Check for correct syntax\n"
            "3. Expected token: 'return', found '%s'",
            getTokenTypeName(currentToken.getType()).c_str()
        );
        console.reportError(
            Console::SYNTAX_ERROR,
            Console::formatString("Expected 'return' keyword, found '%s'", 
                getTokenTypeName(currentToken.getType()).c_str()),
            suggestion,
            span
        );
    });

    std::shared_ptr<ReturnStatement> result;
    if (currentToken.getType() != TokenTypes::Semicolon) {
        DEBUG_LOG("Parsing return value expression");
        std::shared_ptr<Statement> value = parseExpression();
        if (!value) {
            std::string suggestion = "To resolve this:\n"
                                   "1. Verify return expression syntax\n"
                                   "2. Check for valid expression\n"
                                   "3. Ensure proper expression syntax";
            console.reportError(
                Console::SYNTAX_ERROR,
                "Failed to parse return expression",
                suggestion,
                span
            );
            return nullptr;
        }
        result = std::make_shared<ReturnStatement>(value);
        DEBUG_LOG("Parsed return value: " + value->toString());
    } else {
        DEBUG_LOG("No return value (void return)");
        result = std::make_shared<ReturnStatement>();
    }

    if (currentToken.getType() == TokenTypes::Semicolon || currentToken.getType() == TokenTypes::Newline) {
        eat(currentToken.getType());
    } else {
        std::string suggestion = Console::formatString(
            "To resolve this:\n"
            "1. Terminate return statement with ';' or newline\n"
            "2. Check for correct syntax\n"
            "3. Expected token: ';' or newline, found '%s'",
            getTokenTypeName(currentToken.getType()).c_str()
        );
        console.reportError(
            Console::SYNTAX_ERROR,
            Console::formatString("Expected ';' or newline after return statement, found '%s'", 
                getTokenTypeName(currentToken.getType()).c_str()),
            suggestion,
            span
        );
        return nullptr;
    }

    span.end.line = previousToken.getLine();
    span.end.col = previousToken.getColumn();
    span.end.filePath = previousToken.getFilePath();

    result->setPosition(startToken, currentToken);
    result->setSpan(span);
    DEBUG_LOG("Completed parsing return statement");
    return result;
}

} // namespace Omniscript
