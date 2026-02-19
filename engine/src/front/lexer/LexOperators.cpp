#include <omniscript/lexer/Lexer.h>

namespace Omniscript {

Token Lexer::tokenizeOperatorOrPunct() {
    const std::size_t startPos = m_current_position;
    const std::size_t sLine = m_line;
    const std::size_t sCol  = m_col;

    const char c = current();

    // multi-char first
    if (c == ':' && next() == ':') { advance(); advance(); return makeToken(TokenType::ScopeResolution, sLine, sCol, startPos, m_current_position); }

    if (c == '?' && next() == '?') { advance(); advance(); return makeToken(TokenType::NullCoalescing, sLine, sCol, startPos, m_current_position); }
    if (c == '?' && next() == '.') { advance(); advance(); return makeToken(TokenType::SafeNavigation, sLine, sCol, startPos, m_current_position); }
    if (c == '=' && next() == '>') { advance(); advance(); return makeToken(TokenType::Arrow, sLine, sCol, startPos, m_current_position); }

    if (c == '+' && next() == '+') { advance(); advance(); return makeToken(TokenType::Increment, sLine, sCol, startPos, m_current_position); }
    if (c == '-' && next() == '-') { advance(); advance(); return makeToken(TokenType::Decrement, sLine, sCol, startPos, m_current_position); }

    if (c == '*' && next() == '*') {
        advance(); advance();
        if (current() == '=') { advance(); return makeToken(TokenType::PowerAssign, sLine, sCol, startPos, m_current_position); }
        return makeToken(TokenType::Power, sLine, sCol, startPos, m_current_position);
    }

    if (c == '/' && next() == '/') { advance(); advance(); return makeToken(TokenType::FloorDivide, sLine, sCol, startPos, m_current_position); }

    if (c == '=' && next() == '=') { advance(); advance(); return makeToken(TokenType::Equals, sLine, sCol, startPos, m_current_position); }
    if (c == '!' && next() == '=') { advance(); advance(); return makeToken(TokenType::NotEquals, sLine, sCol, startPos, m_current_position); }
    if (c == '<' && next() == '=') { advance(); advance(); return makeToken(TokenType::LessEqual, sLine, sCol, startPos, m_current_position); }
    if (c == '>' && next() == '=') { advance(); advance(); return makeToken(TokenType::GreaterEqual, sLine, sCol, startPos, m_current_position); }

    if (c == '<' && next() == '<') {
        advance(); advance();
        if (current() == '=') { advance(); return makeToken(TokenType::ShiftLeftAssign, sLine, sCol, startPos, m_current_position); }
        return makeToken(TokenType::ShiftLeft, sLine, sCol, startPos, m_current_position);
    }

    if (c == '>' && next() == '>') {
        advance(); advance();
        if (current() == '=') { advance(); return makeToken(TokenType::ShiftRightAssign, sLine, sCol, startPos, m_current_position); }
        return makeToken(TokenType::ShiftRight, sLine, sCol, startPos, m_current_position);
    }

    if (c == '+' && next() == '=') { advance(); advance(); return makeToken(TokenType::PlusAssign, sLine, sCol, startPos, m_current_position); }
    if (c == '-' && next() == '=') { advance(); advance(); return makeToken(TokenType::MinusAssign, sLine, sCol, startPos, m_current_position); }
    if (c == '*' && next() == '=') { advance(); advance(); return makeToken(TokenType::StarAssign, sLine, sCol, startPos, m_current_position); }
    if (c == '/' && next() == '=') { advance(); advance(); return makeToken(TokenType::SlashAssign, sLine, sCol, startPos, m_current_position); }
    if (c == '%' && next() == '=') { advance(); advance(); return makeToken(TokenType::PercentAssign, sLine, sCol, startPos, m_current_position); }

    if (c == '&' && next() == '=') { advance(); advance(); return makeToken(TokenType::BitAndAssign, sLine, sCol, startPos, m_current_position); }
    if (c == '|' && next() == '=') { advance(); advance(); return makeToken(TokenType::BitOrAssign, sLine, sCol, startPos, m_current_position); }
    if (c == '^' && next() == '=') { advance(); advance(); return makeToken(TokenType::BitXorAssign, sLine, sCol, startPos, m_current_position); }

    // single-char
    switch (c) {
        case '+': advance(); return makeToken(TokenType::Plus, sLine, sCol, startPos, m_current_position);
        case '-': advance(); return makeToken(TokenType::Minus, sLine, sCol, startPos, m_current_position);
        case '*': advance(); return makeToken(TokenType::Star, sLine, sCol, startPos, m_current_position);
        case '/': advance(); return makeToken(TokenType::Slash, sLine, sCol, startPos, m_current_position);
        case '%': advance(); return makeToken(TokenType::Percent, sLine, sCol, startPos, m_current_position);

        case '=': advance(); return makeToken(TokenType::Assign, sLine, sCol, startPos, m_current_position);

        case '<': advance(); return makeToken(TokenType::Less, sLine, sCol, startPos, m_current_position);
        case '>': advance(); return makeToken(TokenType::Greater, sLine, sCol, startPos, m_current_position);

        case '!': advance(); return makeToken(TokenType::LogicalNot, sLine, sCol, startPos, m_current_position);

        case '&':
            if (next() == '&') { advance(); advance(); return makeToken(TokenType::LogicalAnd, sLine, sCol, startPos, m_current_position); }
            advance(); return makeToken(TokenType::BitAnd, sLine, sCol, startPos, m_current_position);

        case '|':
            if (next() == '|') { advance(); advance(); return makeToken(TokenType::LogicalOr, sLine, sCol, startPos, m_current_position); }
            if (next() == '>') { advance(); advance(); return makeToken(TokenType::Pipe, sLine, sCol, startPos, m_current_position); }
            advance(); return makeToken(TokenType::BitOr, sLine, sCol, startPos, m_current_position);

        case '^': advance(); return makeToken(TokenType::BitXor, sLine, sCol, startPos, m_current_position);
        case '~': advance(); return makeToken(TokenType::BitNot, sLine, sCol, startPos, m_current_position);

        case '(': advance(); return makeToken(TokenType::LeftParen, sLine, sCol, startPos, m_current_position);
        case ')': advance(); return makeToken(TokenType::RightParen, sLine, sCol, startPos, m_current_position);
        case '{': advance(); return makeToken(TokenType::LeftBrace, sLine, sCol, startPos, m_current_position);
        case '}': advance(); return makeToken(TokenType::RightBrace, sLine, sCol, startPos, m_current_position);
        case '[': advance(); return makeToken(TokenType::LeftBracket, sLine, sCol, startPos, m_current_position);
        case ']': advance(); return makeToken(TokenType::RightBracket, sLine, sCol, startPos, m_current_position);

        case ';': advance(); return makeToken(TokenType::Semicolon, sLine, sCol, startPos, m_current_position);
        case ',': advance(); return makeToken(TokenType::Comma, sLine, sCol, startPos, m_current_position);

        case '.':
            if (next() == '.' && next(2) == '.') { advance(); advance(); advance(); return makeToken(TokenType::Ellipsis, sLine, sCol, startPos, m_current_position); }
            advance(); return makeToken(TokenType::Dot, sLine, sCol, startPos, m_current_position);

        case ':': advance(); return makeToken(TokenType::Colon, sLine, sCol, startPos, m_current_position);
        case '?': advance(); return makeToken(TokenType::QuestionMark, sLine, sCol, startPos, m_current_position);

        case '@': advance(); return makeToken(TokenType::At, sLine, sCol, startPos, m_current_position);
        case '#': advance(); return makeToken(TokenType::Hash, sLine, sCol, startPos, m_current_position);
        case '$': advance(); return makeToken(TokenType::Dollar, sLine, sCol, startPos, m_current_position);

        default:
            advance();
            return makeToken(TokenType::Invalid, sLine, sCol, startPos, m_current_position,
                             std::string("Unexpected character: ") + c);
    }
}

} // namespace Omniscript
