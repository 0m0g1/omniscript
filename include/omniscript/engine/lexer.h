//The lexer class

#ifndef Lexer_H
#define Lexer_H

//Includes
// #include <vector>
// #include <string>
#include <omniscript/omniscript_pch.h>
#include <omniscript/engine/tokens.h>

class Lexer {
    public:
        Lexer(const std::string &source = "") : source(source) {}
        
        Token getNextToken();
        Token peekToken(int n = 0);
        Token getOperator(char &currentChar);
        Token getCurrentToken() const {return previousToken;}
        Token getNumberLiterals(char &currentChar);

    private:
        std::string source;
        size_t currentPosition = 0;
        Token currentToken;
        Token previousToken;
        int line = 1;
        int column = 0;

        //Helper functions
        char peek(int n = 1) const; //Look at next character without moving there
};

#endif