#include <omniscript/lexer/Lexer.h>

namespace Omniscript {

Token Lexer::tokenizeString() {
    const std::size_t startPos = m_current_position;
    const std::size_t sLine = m_line;
    const std::size_t sCol  = m_col;

    // consume opening quote
    advance();

    std::string decoded;
    while (!atEnd()) {
        const char c = current();

        if (c == '"') {
            advance(); // closing quote
            return makeToken(TokenType::StringLiteral, sLine, sCol,
                             startPos, m_current_position, decoded);
        }

        if (c == '\n') {
            // unterminated string
            return makeToken(TokenType::Error, sLine, sCol,
                             startPos, m_current_position,
                             "Unterminated string literal");
        }

        if (c == '\\') {
            advance();
            if (atEnd()) break;
            const char e = current();
            switch (e) {
                case 'n': decoded.push_back('\n'); break;
                case 't': decoded.push_back('\t'); break;
                case 'r': decoded.push_back('\r'); break;
                case '"': decoded.push_back('"'); break;
                case '\\': decoded.push_back('\\'); break;
                default: decoded.push_back(e); break;
            }
            advance();
            continue;
        }

        decoded.push_back(c);
        advance();
    }

    return makeToken(TokenType::Error, sLine, sCol,
                     startPos, m_current_position,
                     "Unterminated string literal");
}

} // namespace Omniscript
