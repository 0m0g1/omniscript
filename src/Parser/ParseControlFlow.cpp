#include <omniscript/Statements/Statement.h>
#include <omniscript/Statements/ControlFlowStatements.h>
#include <omniscript/Statements/LiteralStatements.h>
#include <omniscript/Statements/ExpressionStatements.h>
#include <omniscript/Statements/AssignmentAndGetterStatements.h>

#include <omniscript/Core.h>
#include <omniscript/utils.h>
#include <omniscript/Parser.h>
#include <omniscript/Tokens.h>
#include <omniscript/omniscript_pch.h>
#include <omniscript/Statements/Statement.h>
#include <omniscript/Symboltable.h>

namespace Omniscript {

namespace {
// Inside a loop body, `continue` belongs to that loop again, even if the loop sits inside a switch case.
struct SwitchNestingReset {
    int& slot; int saved;
    explicit SwitchNestingReset(int& s) : slot(s), saved(s) { slot = 0; }
    ~SwitchNestingReset() { slot = saved; }
};
int nextDesugarId() { static int id = 0; return ++id; }
} // namespace

std::shared_ptr<Statement> Parser::parseForLoop() {
    SwitchNestingReset loopScope(switchNesting_);
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
    if (switchNesting_ > 0) {
        FileSpan cspan;
        cspan.start.line = currentToken.getLine();
        cspan.start.col = currentToken.getColumn();
        cspan.start.filePath = currentToken.getFilePath();
        cspan.end = cspan.start;
        cspan.end.col += 8;
        console.reportError(
            Console::SYNTAX_ERROR,
            "'continue' directly inside a switch case is not supported",
            "To resolve this:\n"
            "1. Restructure so the continue is outside the switch\n"
            "2. Or wrap the switch in an if/else chain",
            cspan
        );
    }
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
    SwitchNestingReset loopScope(switchNesting_);
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


// do { body } while (cond);
//
// Lowered (no new IR) to
//     { let __doN = true; while (__doN || cond) { __doN = false; { body } } }
// so `continue` in the body jumps to the condition check, where __doN is already false and the real
// condition decides, which is exactly do/while behaviour.
std::shared_ptr<Statement> Parser::parseDoWhile() {
    Token startToken = currentToken;
    FileSpan span;
    span.start.line = startToken.getLine();
    span.start.col = startToken.getColumn();
    span.start.filePath = startToken.getFilePath();

    DEBUG_LOG("Parsing do/while statement");
    eat(TokenTypes::Identifier);   // 'do'

    SwitchNestingReset loopScope(switchNesting_);
    auto body = std::dynamic_pointer_cast<BlockStatement>(parseBlock());
    if (!body) {
        console.reportError(Console::SYNTAX_ERROR, "Failed to parse do/while body",
            "To resolve this:\n1. Start the body with '{'\n2. Check for balanced braces", span);
        return nullptr;
    }

    while (currentToken.getType() == TokenTypes::Newline) eat(TokenTypes::Newline);

    eat(TokenTypes::While, [&]() {
        console.reportError(Console::SYNTAX_ERROR,
            Console::formatString("Expected 'while' after do-block, found '%s'",
                getTokenTypeName(currentToken.getType()).c_str()),
            "To resolve this:\n1. Write: do { ... } while (condition);", span);
    });
    eat(TokenTypes::LeftParen, [&]() {
        console.reportError(Console::SYNTAX_ERROR,
            Console::formatString("Expected '(' after 'while', found '%s'",
                getTokenTypeName(currentToken.getType()).c_str()),
            "To resolve this:\n1. Write: do { ... } while (condition);", span);
    });
    auto condition = parseExpression();
    if (!condition) {
        console.reportError(Console::SYNTAX_ERROR, "Failed to parse do/while condition",
            "To resolve this:\n1. Provide a boolean expression", span);
        return nullptr;
    }
    eat(TokenTypes::RightParen, [&]() {
        console.reportError(Console::SYNTAX_ERROR,
            Console::formatString("Expected ')' after do/while condition, found '%s'",
                getTokenTypeName(currentToken.getType()).c_str()),
            "To resolve this:\n1. Close the condition with ')'", span);
    });

    const std::string flag = "__do" + std::to_string(nextDesugarId());

    auto declare = std::make_shared<AssignVariable>(flag, nullptr, std::make_shared<BoolLiteral>(true));
    auto reset   = std::make_shared<AssignVariable>(flag, nullptr, std::make_shared<BoolLiteral>(false), true);
    auto loopCond = std::make_shared<ASTBinaryExpression>(
        std::make_shared<GetVariable>(flag), TokenTypes::LogicalOr, condition);

    std::vector<std::shared_ptr<Statement>> loopBodyStmts = { reset, body };
    auto loopBody = std::make_shared<BlockStatement>(loopBodyStmts);

    span.end.line = previousToken.getLine();
    span.end.col = previousToken.getColumn();
    span.end.filePath = previousToken.getFilePath();

    auto loop = std::make_shared<WhileStatement>(loopCond, loopBody);
    loop->setPosition(startToken, currentToken);
    loop->setSpan(span);

    std::vector<std::shared_ptr<Statement>> outerStmts = { declare, loop };
    auto outer = std::make_shared<BlockStatement>(outerStmts);
    outer->setPosition(startToken, currentToken);
    outer->setSpan(span);
    DEBUG_LOG("Completed parsing do/while statement");
    return outer;
}

// switch (x) { case 1: ...; break; case 2: case 3: ...; default: ...; }
//
// Lowered (no new IR) to
//     { let __swN = x;
//       while (true) { if (__swN == 1) {...} else if (__swN == 2 || __swN == 3) {...} else {...}  break; } }
// The one-pass while makes `break` inside a case leave only the switch. Cases do not fall through, except
// that consecutive empty cases (`case 2: case 3: body`) share the next body. `continue` directly inside a
// case is rejected (see parseContinue) because it would target the one-pass loop.
std::shared_ptr<Statement> Parser::parseSwitch() {
    Token startToken = currentToken;
    FileSpan span;
    span.start.line = startToken.getLine();
    span.start.col = startToken.getColumn();
    span.start.filePath = startToken.getFilePath();

    DEBUG_LOG("Parsing switch statement");
    eat(TokenTypes::Identifier);   // 'switch'
    eat(TokenTypes::LeftParen, [&]() {
        console.reportError(Console::SYNTAX_ERROR,
            Console::formatString("Expected '(' after 'switch', found '%s'",
                getTokenTypeName(currentToken.getType()).c_str()),
            "To resolve this:\n1. Write: switch (value) { case 1: ... }", span);
    });
    auto discriminant = parseExpression();
    if (!discriminant) {
        console.reportError(Console::SYNTAX_ERROR, "Failed to parse switch expression",
            "To resolve this:\n1. Provide the value to switch on", span);
        return nullptr;
    }
    eat(TokenTypes::RightParen, [&]() {
        console.reportError(Console::SYNTAX_ERROR,
            Console::formatString("Expected ')' after switch expression, found '%s'",
                getTokenTypeName(currentToken.getType()).c_str()),
            "To resolve this:\n1. Close the expression with ')'", span);
    });
    while (currentToken.getType() == TokenTypes::Newline) eat(TokenTypes::Newline);
    eat(TokenTypes::LeftBrace, [&]() {
        console.reportError(Console::SYNTAX_ERROR,
            Console::formatString("Expected '{' to open the switch body, found '%s'",
                getTokenTypeName(currentToken.getType()).c_str()),
            "To resolve this:\n1. Write: switch (value) { case 1: ... }", span);
    });

    const std::string tmp = "__sw" + std::to_string(nextDesugarId());
    auto isLabel = [&](const char* word) {
        return currentToken.getType() == TokenTypes::Identifier && currentToken.getValue() == word;
    };
    auto skipBlank = [&]() {
        while (currentToken.getType() == TokenTypes::Newline || currentToken.getType() == TokenTypes::Semicolon)
            eat(currentToken.getType());
    };
    auto parseCaseBody = [&]() {
        std::vector<std::shared_ptr<Statement>> stmts;
        ++switchNesting_;
        skipBlank();
        while (currentToken.getType() != TokenTypes::RightBrace &&
               currentToken.getType() != TokenTypes::EOI &&
               !isLabel("case") && !isLabel("default")) {
            auto st = parseStatement();
            if (st) stmts.push_back(st);
            skipBlank();
        }
        --switchNesting_;
        return stmts;
    };

    std::vector<std::shared_ptr<Statement>> conditions;
    std::vector<std::shared_ptr<BlockStatement>> bodies;
    std::shared_ptr<BlockStatement> defaultBody;
    std::shared_ptr<Statement> carried;   // condition(s) of preceding empty cases

    skipBlank();
    while (currentToken.getType() != TokenTypes::RightBrace && currentToken.getType() != TokenTypes::EOI) {
        if (isLabel("case")) {
            eat(TokenTypes::Identifier);
            auto value = parseExpression();
            if (!value) {
                console.reportError(Console::SYNTAX_ERROR, "Failed to parse case value",
                    "To resolve this:\n1. Write: case 1:", span);
                return nullptr;
            }
            eat(TokenTypes::Colon, [&]() {
                console.reportError(Console::SYNTAX_ERROR,
                    Console::formatString("Expected ':' after case value, found '%s'",
                        getTokenTypeName(currentToken.getType()).c_str()),
                    "To resolve this:\n1. Write: case 1:", span);
            });
            std::shared_ptr<Statement> cond = std::make_shared<ASTBinaryExpression>(
                std::make_shared<GetVariable>(tmp), TokenTypes::Equals, value);
            if (carried) {
                cond = std::make_shared<ASTBinaryExpression>(carried, TokenTypes::LogicalOr, cond);
                carried = nullptr;
            }
            auto stmts = parseCaseBody();
            if (stmts.empty()) {
                carried = cond;            // `case 2:` with no body shares the next case's body
            } else {
                conditions.push_back(cond);
                bodies.push_back(std::make_shared<BlockStatement>(stmts));
            }
        } else if (isLabel("default")) {
            eat(TokenTypes::Identifier);
            eat(TokenTypes::Colon, [&]() {
                console.reportError(Console::SYNTAX_ERROR,
                    Console::formatString("Expected ':' after 'default', found '%s'",
                        getTokenTypeName(currentToken.getType()).c_str()),
                    "To resolve this:\n1. Write: default:", span);
            });
            defaultBody = std::make_shared<BlockStatement>(parseCaseBody());
        } else {
            console.reportError(Console::SYNTAX_ERROR,
                Console::formatString("Expected 'case' or 'default' inside switch, found '%s'",
                    getTokenTypeName(currentToken.getType()).c_str()),
                "To resolve this:\n1. Every statement in a switch belongs to a case or default", span);
            return nullptr;
        }
        skipBlank();
    }
    eat(TokenTypes::RightBrace, [&]() {
        console.reportError(Console::SYNTAX_ERROR, "Expected '}' to close the switch",
            "To resolve this:\n1. Check for balanced braces", span);
    });

    span.end.line = previousToken.getLine();
    span.end.col = previousToken.getColumn();
    span.end.filePath = previousToken.getFilePath();

    std::vector<std::shared_ptr<Statement>> passStmts;
    if (!conditions.empty()) {
        passStmts.push_back(std::make_shared<IfStatement>(conditions, bodies, defaultBody));
    } else if (defaultBody) {
        passStmts.push_back(defaultBody);
    }
    passStmts.push_back(std::make_shared<BreakStatement>());

    auto onePass = std::make_shared<WhileStatement>(
        std::make_shared<BoolLiteral>(true), std::make_shared<BlockStatement>(passStmts));
    onePass->setPosition(startToken, currentToken);
    onePass->setSpan(span);

    auto declare = std::make_shared<AssignVariable>(tmp, nullptr, discriminant);
    std::vector<std::shared_ptr<Statement>> outerStmts = { declare, onePass };
    auto outer = std::make_shared<BlockStatement>(outerStmts);
    outer->setPosition(startToken, currentToken);
    outer->setSpan(span);
    DEBUG_LOG("Completed parsing switch statement");
    return outer;
}

} // namespace Omniscript