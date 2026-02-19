#include <omniscript/lexer/Lexer.h>
#include <cctype>

namespace Omniscript {

Lexer::Lexer(const std::string& source, const std::string& file_path)
    : m_source(source), m_file_path(file_path) {}

// ---- basic cursor helpers ----
char Lexer::current() const {
    if (m_current_position >= m_source.size()) return '\0';
    return m_source[m_current_position];
}

char Lexer::next(std::size_t lookahead) const {
    std::size_t p = m_current_position + lookahead;
    if (p >= m_source.size()) return '\0';
    return m_source[p];
}

bool Lexer::atEnd() const {
    return m_current_position >= m_source.size();
}

void Lexer::advance() {
    if (atEnd()) return;

    char c = m_source[m_current_position++];
    if (c == '\n') {
        m_line++;
        m_col = 0;
    } else {
        m_col++;
    }
}

bool Lexer::match(char c) {
    if (current() != c) return false;
    advance();
    return true;
}

void Lexer::skipWhitespaceExceptNewline() {
    while (!atEnd()) {
        char c = current();
        if (c == ' ' || c == '\t' || c == '\r') {
            advance();
            continue;
        }
        break; // keep '\n' as a token
    }
}

Token Lexer::makeToken(TokenType type,
                       std::size_t sLine, std::size_t sCol,
                       std::size_t startPos, std::size_t endPos,
                       std::string value) {
    std::string lexeme;
    if (endPos >= startPos && endPos <= m_source.size())
        lexeme = m_source.substr(startPos, endPos - startPos);

    FileSpan span;
    span.start.line = sLine;
    span.start.col = sCol;
    span.start.filePath = m_file_path;
    span.end.line = m_line;
    span.end.col = m_col;
    span.end.filePath = m_file_path;

    if (value.empty()) value = lexeme;
    return Token(type, std::move(lexeme), std::move(value), span);
}

// ---- public API ----
Token Lexer::peekToken(int n) {
    if (n <= 0) n = 1;

    const std::size_t savedPos  = m_current_position;
    const std::size_t savedLine = m_line;
    const std::size_t savedCol  = m_col;

    Token t;
    for (int i = 0; i < n; ++i) {
        t = getNextToken();
        if (t.isEndOfInput()) break;
    }

    m_current_position = savedPos;
    m_line = savedLine;
    m_col = savedCol;
    return t;
}

Token Lexer::getNextToken() {
    skipWhitespaceExceptNewline();

    if (atEnd()) return Token(TokenType::EndOfInput);

    const std::size_t startPos = m_current_position;
    const std::size_t sLine = m_line;
    const std::size_t sCol  = m_col;

    const char c = current();

    // Newline is a real token in your enum
    if (c == '\n') {
        advance();
        return makeToken(TokenType::Newline, sLine, sCol, startPos, m_current_position, "\n");
    }

    // identifiers/keywords
    if (std::isalpha(static_cast<unsigned char>(c)) || c == '_')
        return tokenizeIdentifierOrKeyword();

    // numbers
    if (std::isdigit(static_cast<unsigned char>(c)))
        return tokenizeNumber();

    // strings
    if (c == '"')
        return tokenizeString();

    // operators/punctuators
    return tokenizeOperatorOrPunct();
}

} // namespace Omniscript
