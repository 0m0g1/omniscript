#include <omniscript/omniscript_pch.h>
<<<<<<< HEAD:src/Lexer/Lexer.cpp
#include <omniscript/Lexer.h>
#include <omniscript/Tokens.h>
=======
#include <omniscript/Lexer.h>
#include <omniscript/Tokens.h>
>>>>>>> 7ccebff50dd27e70cffd4d578dcb358f4c9e1613:src/engine/Lexer/Lexer.cpp
#include <omniscript/utils.h>

// Lexer::Lexer(const std::string &source) : source(source) {}

char Lexer::peek(int n) const {
    if (currentPosition < source.length()) {
        return source[currentPosition + n];
    }
    return '\0';
}

Token Lexer::peekToken(int n) {
    size_t currentPositionHolder = currentPosition;
    int count = 0;
    Token nextToken;
    while (count < n) {
        nextToken = getNextToken();
        count++;
    }
    currentPosition = currentPositionHolder;
    return nextToken;
}