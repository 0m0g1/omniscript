#pragma once

//Includes
// #include <vector>
// #include <string>
#include <omniscript/omniscript_pch.h>
#include <omniscript/tokens.h>

class Lexer {
    public:
        Lexer(const std::string &source = "", const std::string& filePath = "") : source(source), sourceFilePath(filePath) {}
        
        Token getNextToken();
        Token peekToken(int n = 0);
        Token getOperator(char &currentChar);
        Token getCurrentToken() const {return previousToken;}
        Token getStringToken(char &currentChar);
        Token getNumberLiterals(char &currentChar);
        std::string getFilePath() const { return sourceFilePath; }

    private:
        std::string sourceFilePath;
        std::string source;
        size_t currentPosition = 0;
        Token currentToken;
        Token previousToken;
        int line = 1;
        int column = 0;
                
        //Helper functions
        char peek(int n = 1) const; //Look at next character without moving there
};
