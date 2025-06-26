#include <omniscript/omniscript_pch.h>
#include <omniscript/Lexer.h>
#include <omniscript/Tokens.h>
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