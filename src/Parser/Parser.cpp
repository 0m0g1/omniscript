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

}

void Parser::initializeConstants() {

}

void Parser::initializeFunctions() {

}

void Parser::parseProgram() {
    DEBUG_LOG();
    DEBUG_LOG("Parsing the script");
    DEBUG_LOG("==================");
    DEBUG_LOG();
    
    while (currentToken.getType() != TokenTypes::EOI) {
        statements.push_back(parseStatement());
    }
    
    DEBUG_LOG();
    DEBUG_LOG("Done parsing the script");
    DEBUG_LOG("=======================");
    DEBUG_LOG();
}

void Parser::eat(TokenTypes expectedType, const std::string& err) {
    Omniscript::setSpanFromPosition(currentToken.getLine(), currentToken.getColumn(), currentToken.getFilePath());
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
        if (currentToken.getType() == TokenTypes::Newline) {
            eat(TokenTypes::Newline);
        }
    } else {
        eat(TokenTypes::Newline);
    }
}

//Parsing starts at ParseStatement