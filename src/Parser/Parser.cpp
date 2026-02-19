#include <omniscript/Core.h>
#include <omniscript/utils.h>
#include <omniscript/Lexer.h>
#include <omniscript/Parser.h>
#include <omniscript/Tokens.h>
#include <omniscript/Statements/Statement.h>
#include <omniscript/Symboltable.h>
#include <omniscript/omniscript_pch.h>

namespace Omniscript {
// Entry point for parsing the program
std::vector<std::shared_ptr<Statement>> Parser::parse() {
    FileSpan span;
    span.start.line = currentToken.getLine();
    span.start.col = currentToken.getColumn();
    span.start.filePath = currentToken.getFilePath();

    initializeEnvironment();
    parseProgram(); // Start parsing the program

    span.end.line = previousToken.getLine();
    span.end.col = previousToken.getColumn();
    span.end.filePath = previousToken.getFilePath();

    // Set spans on all statements
    // for (auto& stmt : statements) {
    //     if (stmt && !stmt->hasSpan()) {
    //         stmt->setSpan(span);
    //     }
    // }

    return this->statements;
}

void Parser::initializeEnvironment() {
    initializeBuiltInObjects();
    initializeConstants();
    initializeFunctions();
}

void Parser::initializeBuiltInObjects() {
    // Placeholder for future initialization logic
}

void Parser::initializeConstants() {
    // Placeholder for future initialization logic
}

void Parser::initializeFunctions() {
    // Placeholder for future initialization logic
}

void Parser::parseProgram() {
    FileSpan span;
    span.start.line = currentToken.getLine();
    span.start.col = currentToken.getColumn();
    span.start.filePath = currentToken.getFilePath();

    DEBUG_LOG();
    DEBUG_LOG("Parsing the script");
    DEBUG_LOG("==================");
    DEBUG_LOG();
    
    while (currentToken.getType() != TokenTypes::EOI) {
        // Skip empty lines/newlines at the top level
        if (currentToken.getType() == TokenTypes::Newline) {
            eat(TokenTypes::Newline);
            continue;
        }
        
        auto stmt = parseStatement();
        if (!stmt) {
            // Enhanced error reporting with source location
            REPORT_ERROR_WITH_SPAN_AND_SUGGESTION(
                Console::SYNTAX_ERROR,
                "Failed to parse statement",
                "To resolve this:\n1. Verify statement syntax\n2. Check for valid statement types\n3. Ensure proper declaration",
                span
            );
            
            // Try to recover by consuming the current token
            if (currentToken.getType() != TokenTypes::EOI) {
                eat(currentToken.getType());
            }
            continue; // Continue to avoid crashing, but report error
        }
        statements.push_back(stmt);
    }

    span.end.line = previousToken.getLine();
    span.end.col = previousToken.getColumn();
    span.end.filePath = previousToken.getFilePath();
    
    DEBUG_LOG();
    DEBUG_LOG("Done parsing the script");
    DEBUG_LOG("=======================");
    DEBUG_LOG();
}

void Parser::eat(TokenTypes expectedType, const std::string& err) {
    FileSpan span;
    span.start.line = currentToken.getLine();
    span.start.col = currentToken.getColumn();
    span.start.filePath = currentToken.getFilePath();

    if (currentToken.getType() == expectedType) {
        previousToken = currentToken;
        currentToken = lexer.getNextToken();
        span.end.line = previousToken.getLine();
        span.end.col = previousToken.getColumn();
        span.end.filePath = previousToken.getFilePath();
    } else {
        // Enhanced error reporting with automatic source location
        std::string suggestion = Console::formatString(
            "To resolve this:\n1. Ensure the correct token is used\n2. Check syntax around line %d, column %d\n3. %s",
            currentToken.getLine(), currentToken.getColumn(), err.c_str()
        );

        REPORT_ERROR_F_WITH_SUGGESTION(
            Console::SYNTAX_ERROR,
            "Expected token type: %s, found '%s'",
            suggestion,
            getTokenTypeName(expectedType).c_str(),
            getTokenTypeName(currentToken.getType()).c_str()
        );
        
        // Consume the unexpected token to avoid infinite loops
        previousToken = currentToken;
        currentToken = lexer.getNextToken();
    }
}

void Parser::eat(TokenTypes expectedType, const std::function<void()>& errorHandler) {
    FileSpan span;
    span.start.line = currentToken.getLine();
    span.start.col = currentToken.getColumn();
    span.start.filePath = currentToken.getFilePath();

    if (currentToken.getType() == expectedType) {
        previousToken = currentToken;
        currentToken = lexer.getNextToken();
        span.end.line = previousToken.getLine();
        span.end.col = previousToken.getColumn();
        span.end.filePath = previousToken.getFilePath();
    } else {
        errorHandler();
        // Always consume the token to prevent infinite loops
        previousToken = currentToken;
        currentToken = lexer.getNextToken();
    }
}

void Parser::expectSemicolonOrNewLine() {
    FileSpan span;
    span.start.line = currentToken.getLine();
    span.start.col = currentToken.getColumn();
    span.start.filePath = currentToken.getFilePath();

    if (currentToken.getType() == TokenTypes::Semicolon) {
        eat(TokenTypes::Semicolon, [&]() {
            SYNTAX_ERROR_F("Expected ';' to end statement, found '%s'", 
                          getTokenTypeName(currentToken.getType()).c_str());
        });
        
        // Optional newline after semicolon
        if (currentToken.getType() == TokenTypes::Newline) {
            eat(TokenTypes::Newline, [&]() {
                SYNTAX_ERROR_F("Expected newline after semicolon, found '%s'", 
                              getTokenTypeName(currentToken.getType()).c_str());
            });
        }
    } else if (currentToken.getType() == TokenTypes::Newline) {
        eat(TokenTypes::Newline, [&]() {
            SYNTAX_ERROR_F("Expected newline to end statement, found '%s'", 
                          getTokenTypeName(currentToken.getType()).c_str());
        });
    }
    // If neither semicolon nor newline, that's okay - some statements don't require terminators

    span.end.line = previousToken.getLine();
    span.end.col = previousToken.getColumn();
    span.end.filePath = previousToken.getFilePath();
}

} // namespace Omniscript
