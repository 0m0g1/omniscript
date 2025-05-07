#include <omniscript/Core.h>
#include <omniscript/utils.h>
#include <omniscript/runtime/object.h>
#include <omniscript/engine/parser.h>
#include <omniscript/engine/lexer.h>
#include <omniscript/engine/tokens.h>
#include <omniscript/engine/Statement.h>
#include <omniscript/engine/Symboltable.h>
#include <omniscript/mainthreadrunner.h>
#include <omniscript/omniscript_pch.h>

// built - in objects
// // #include <omniscript/runtime/Function.h>
// #include <omniscript/runtime/Class.h>
// #include <omniscript/runtime/Namespace.h>
// #include <omniscript/runtime/Enum.h>
// #include <omniscript/runtime/Number.h>
// #include <omniscript/runtime/String.h>


// Environment Objects
// #include <omniscript/runtime/graphics/canvas.h>
// // #include <omniscript/runtime/audio/AudioAccess.h>
// #include <omniscript/runtime/Http/Http.h>
// #include <omniscript/runtime/io/console.h>
// #include <omniscript/runtime/io/FileAccess.h>
// #include <omniscript/runtime/Math/Math.h>
// #include <omniscript/runtime/Time/Time.h>
// #include <omniscript/runtime/Json/Json.h>
// #include <omniscript/runtime/Date/Date.h>
// #include <omniscript/runtime/Path/Path.h>
// #include <omniscript/runtime/OS/OS.h>


// Entry point for parsing the program
std::vector<std::shared_ptr<Statement>> Parser::Parse() {
    initializeEnvironment();
    parseProgram(); // Start parsing the program
    return this->statements;
}

void Parser::initializeEnvironment() {
    initializeBuiltInObjects();
    initializeConstants();
    initializeFunctions();
}

void Parser::initializeBuiltInObjects() {
    // // scope.addObject("canvas", std::make_shared<CanvasObject>());
    // // scope.addObject("AudioAccess", std::make_shared<AudioAccess>());
    // scope.addObject("HTTP", std::make_shared<HTTP>());
    // scope.addObject("console", std::make_shared<ConsoleObject>());
    // scope.addObject("FileAccess", std::make_shared<FileAccess>());
    // scope.addObject("Math", std::make_shared<Math>());
    // scope.addObject("Time", std::make_shared<Time>());
    // scope.addObject("JSON", std::make_shared<JSON>());
    // scope.addObject("Date", std::make_shared<Date>());
    // scope.addObject("Path", std::make_shared<Path>());
    // scope.addObject("OS", std::make_shared<OS>());
}

void Parser::initializeConstants() {

}

void Parser::initializeFunctions() {

}


// Parse a complete program
void Parser::parseProgram() {
    DEBUG_LOG();
    DEBUG_LOG("Parsing the script");
    DEBUG_LOG("==================");
    DEBUG_LOG();
    
    while (currentToken.getType() != TokenTypes::EOI) {
        statements.push_back(parseStatement()); // Parse each statement in the program
    }
    
    DEBUG_LOG();
    DEBUG_LOG("Done parsing the script");
    DEBUG_LOG("=======================");
    DEBUG_LOG();
}


// Helper function to consume a token if it matches the expected type
void Parser::eat(TokenTypes expectedType, const std::string& err) {
    // Keep track of your position in the current file
    Omniscript::setPosition(currentToken.getLine(), currentToken.getColumn(), currentToken.getFilePath());
    if (currentToken.getType() == expectedType) {
        previousToken = currentToken;
        currentToken = lexer.getNextToken(); // Move to the next token
    } else {
        std::string errorMessage = "[Parser Error]\nExpected token type: " 
        + getTokenTypeName(expectedType) 
        + " at line: " + std::to_string(currentToken.getLine()) 
        + " column: " + std::to_string(currentToken.getColumn()) 
        + " got token type " + getTokenTypeName(currentToken.getType()) 
        + " instead.\n";

        
        if (err != "") {
            errorMessage += "\n\n" + err;
        }
        console.error(errorMessage);
    }
}

// TODO: Omniscript automatically skips all new lines
void Parser::expectSemicolonOrNewLine() {
    if (currentToken.getType() == TokenTypes::Semicolon) {
        eat(TokenTypes::Semicolon);
    } else {
        eat(TokenTypes::Newline);
    }
}

//Parsing starts at ParseStatement