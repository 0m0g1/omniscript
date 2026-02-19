#include <omniscript/lexer/Lexer.h>
#include <cctype>

namespace Omniscript {

Token Lexer::tokenizeNumber() {
    const std::size_t startPos = m_current_position;
    const std::size_t sLine = m_line;
    const std::size_t sCol  = m_col;

    bool isFloat = false;

    while (std::isdigit(static_cast<unsigned char>(current())))
        advance();

    // fractional part
    if (current() == '.' && std::isdigit(static_cast<unsigned char>(next()))) {
        isFloat = true;
        advance(); // '.'
        while (std::isdigit(static_cast<unsigned char>(current())))
            advance();
    }

    // exponent
    if (current() == 'e' || current() == 'E') {
        const char la = next();
        if (std::isdigit(static_cast<unsigned char>(la)) || la == '+' || la == '-') {
            isFloat = true;
            advance(); // e/E
            if (current() == '+' || current() == '-') advance();
            while (std::isdigit(static_cast<unsigned char>(current())))
                advance();
        }
    }

    const std::size_t endPos = m_current_position;
    const std::string lex = m_source.substr(startPos, endPos - startPos);

    return makeToken(isFloat ? TokenType::FloatLiteral : TokenType::IntegerLiteral,
                     sLine, sCol, startPos, endPos, lex);
}

} // namespace Omniscript
