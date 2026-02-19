#pragma once
#include <string>
#include <omniscript/tokens/Tokens.h>

namespace Omniscript {

class Lexer {
public:
    Lexer(const std::string& source, const std::string& file_path);
    ~Lexer() = default;

    Token peekToken(int n = 1);
    Token getNextToken();

private:
    // helpers
    char current() const;
    char next(std::size_t lookahead = 1) const;
    bool atEnd() const;

    void advance();                 // advances 1 char, updates line/col
    void skipWhitespaceExceptNewline();
    bool match(char c);

    Token makeToken(TokenType type, std::size_t sLine, std::size_t sCol, std::size_t startPos,
                    std::size_t endPos, std::string value = {});

    Token tokenizeIdentifierOrKeyword();
    Token tokenizeNumber();
    Token tokenizeString();         // "..."

private:
    std::size_t m_current_position = 0;
    std::size_t m_line = 0; // 0-based or 1-based: pick one; I keep 0-based as you had
    std::size_t m_col  = 0;

    std::string m_source;
    std::string m_file_path;
};

} // namespace Omniscript
