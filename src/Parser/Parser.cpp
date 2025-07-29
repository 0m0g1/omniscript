#include <omniscript/Core.h>
#include <omniscript/utils.h>
#include <omniscript/Lexer.h>
#include <omniscript/Parser.h>
#include <omniscript/Tokens.h>
#include <omniscript/Statement.h>
#include <omniscript/Symboltable.h>
#include <omniscript/omniscript_pch.h>

// Entry point for parsing the program
std::vector<std::shared_ptr<Statement>> Parser::parse() {
    Omniscript::FileSpan span;
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
    Omniscript::FileSpan span;
    span.start.line = currentToken.getLine();
    span.start.col = currentToken.getColumn();
    span.start.filePath = currentToken.getFilePath();

    DEBUG_LOG();
    DEBUG_LOG("Parsing the script");
    DEBUG_LOG("==================");
    DEBUG_LOG();
    
    while (currentToken.getType() != TokenTypes::EOI) {
        auto stmt = parseStatement();
        if (!stmt) {
            console.reportError(
                Omniscript::Console::SYNTAX_ERROR,
                "Failed to parse statement",
                "To resolve this:\n1. Verify statement syntax\n2. Check for valid statement types\n3. Ensure proper declaration",
                span
            );
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
    Omniscript::FileSpan span;
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
        std::string errorMessage = Omniscript::Console::formatString(
            "Expected token type: %s, found '%s'",
            getTokenTypeName(expectedType).c_str(),
            getTokenTypeName(currentToken.getType()).c_str()
        );
        std::string suggestion = "To resolve this:\n1. Ensure the correct token is used\n2. Check syntax around line " +
            std::to_string(currentToken.getLine()) + ", column " + std::to_string(currentToken.getColumn()) + "\n3. " + err;

        console.reportError(
            Omniscript::Console::SYNTAX_ERROR,
            errorMessage,
            suggestion,
            span
        );
        eat(currentToken.getType());
    }
}

void Parser::eat(TokenTypes expectedType, const std::function<void()>& errorHandler) {
    Omniscript::FileSpan span;
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
        eat(currentToken.getType());
    }
}

void Parser::expectSemicolonOrNewLine() {
    Omniscript::FileSpan span;
    span.start.line = currentToken.getLine();
    span.start.col = currentToken.getColumn();
    span.start.filePath = currentToken.getFilePath();

    if (currentToken.getType() == TokenTypes::Semicolon) {
        eat(TokenTypes::Semicolon, [&]() {
            std::string suggestion = Omniscript::Console::formatString(
                "To resolve this:\n"
                "1. End statement with ';'\n"
                "2. Check for proper termination\n"
                "3. Expected token: ';', found '%s'",
                getTokenTypeName(currentToken.getType()).c_str()
            );
            console.reportError(
                Omniscript::Console::SYNTAX_ERROR,
                Omniscript::Console::formatString("Expected ';' to end statement, found '%s'", 
                    getTokenTypeName(currentToken.getType()).c_str()),
                suggestion,
                span
            );
        });
        if (currentToken.getType() == TokenTypes::Newline) {
            eat(TokenTypes::Newline, [&]() {
                std::string suggestion = Omniscript::Console::formatString(
                    "To resolve this:\n"
                    "1. Ensure newline follows semicolon if present\n"
                    "2. Check statement termination\n"
                    "3. Expected token: newline, found '%s'",
                    getTokenTypeName(currentToken.getType()).c_str()
                );
                console.reportError(
                    Omniscript::Console::SYNTAX_ERROR,
                    Omniscript::Console::formatString("Expected newline after semicolon, found '%s'", 
                        getTokenTypeName(currentToken.getType()).c_str()),
                    suggestion,
                    span
                );
            });
        }
    } else {
        eat(TokenTypes::Newline, [&]() {
            std::string suggestion = Omniscript::Console::formatString(
                "To resolve this:\n"
                "1. End statement with newline or ';'\n"
                "2. Check statement termination\n"
                "3. Expected token: newline, found '%s'",
                getTokenTypeName(currentToken.getType()).c_str()
            );
            console.reportError(
                Omniscript::Console::SYNTAX_ERROR,
                Omniscript::Console::formatString("Expected newline to end statement, found '%s'", 
                    getTokenTypeName(currentToken.getType()).c_str()),
                suggestion,
                span
            );
        });
    }

    span.end.line = previousToken.getLine();
    span.end.col = previousToken.getColumn();
    span.end.filePath = previousToken.getFilePath();
}