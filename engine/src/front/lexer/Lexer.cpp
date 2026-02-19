#include <omniscript/lexer/Lexer.h>
#include <cctype>
#include <unordered_map>

namespace Omniscript {

static TokenType keywordType(const std::string& s) {
    // Expand this map as you like.
    static const std::unordered_map<std::string, TokenType> k = {
        {"if", TokenType::If},
        {"elseif", TokenType::ElseIf},
        {"else", TokenType::Else},
        {"while", TokenType::While},
        {"for", TokenType::For},
        {"continue", TokenType::Continue},
        {"break", TokenType::Break},
        {"return", TokenType::Return},
        {"function", TokenType::Function},
        {"let", TokenType::Let},
        {"var", TokenType::Var},
        {"const", TokenType::Const},
        {"new", TokenType::New},
        {"delete", TokenType::Delete},
        {"class", TokenType::Class},
        {"struct", TokenType::Struct},
        {"namespace", TokenType::Namespace},
        {"using", TokenType::Using},
        {"public", TokenType::Public},
        {"private", TokenType::Private},
        {"protected", TokenType::Protected},
        {"override", TokenType::Override},
        {"virtual", TokenType::Virtual},
        {"static", TokenType::Static},
        {"final", TokenType::Final},
        {"true", TokenType::True},
        {"false", TokenType::False},
        {"null", TokenType::Null},
        {"nullptr", TokenType::Nullptr},
        {"enum", TokenType::Enum},
        {"extends", TokenType::Extends},
        {"variant", TokenType::Variant},
        {"any", TokenType::Any},
        {"import", TokenType::Import},
        {"include", TokenType::Include},
        {"from", TokenType::From},
        {"module", TokenType::Module},
        {"extern", TokenType::Extern},
        {"intrinsic", TokenType::Intrinsic},
        {"volatile", TokenType::Volatile},
        {"as", TokenType::As},
        {"type", TokenType::Type},
    };

    auto it = k.find(s);
    return (it == k.end()) ? TokenType::Identifier : it->second;
}

Lexer::Lexer(const std::string& source, const std::string& file_path)
    : m_source(source), m_file_path(file_path) {}

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
        // NOTE: we do NOT skip '\n' because you have TokenType::Newline
        break;
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

    // If caller didn’t provide a value, default to lexeme for non-decoded tokens.
    if (value.empty()) value = lexeme;

    return Token(type, std::move(lexeme), std::move(value), span);
}

Token Lexer::peekToken(int n) {
    if (n <= 0) n = 1;

    // Save *all* state
    const std::size_t savedPos = m_current_position;
    const std::size_t savedLine = m_line;
    const std::size_t savedCol = m_col;

    Token t;
    for (int i = 0; i < n; ++i) {
        t = getNextToken();
        if (t.isEndOfInput()) break;
    }

    // Restore state
    m_current_position = savedPos;
    m_line = savedLine;
    m_col = savedCol;
    return t;
}

Token Lexer::tokenizeIdentifierOrKeyword() {
    const std::size_t startPos = m_current_position;
    const std::size_t sLine = m_line;
    const std::size_t sCol = m_col;

    while (!atEnd()) {
        char c = current();
        if (std::isalnum(static_cast<unsigned char>(c)) || c == '_')
            advance();
        else
            break;
    }

    const std::size_t endPos = m_current_position;
    std::string text = m_source.substr(startPos, endPos - startPos);
    TokenType tt = keywordType(text);
    return makeToken(tt, sLine, sCol, startPos, endPos, text);
}

Token Lexer::tokenizeNumber() {
    const std::size_t startPos = m_current_position;
    const std::size_t sLine = m_line;
    const std::size_t sCol = m_col;

    bool isFloat = false;

    // Basic decimal; you can extend to 0x/0b/0o + bigint suffix etc.
    while (std::isdigit(static_cast<unsigned char>(current()))) {
        advance();
    }

    if (current() == '.' && std::isdigit(static_cast<unsigned char>(next()))) {
        isFloat = true;
        advance(); // consume '.'
        while (std::isdigit(static_cast<unsigned char>(current()))) {
            advance();
        }
    }

    // Optional exponent: e/E (+/-) digits
    if ((current() == 'e' || current() == 'E')) {
        char la = next();
        if (std::isdigit(static_cast<unsigned char>(la)) || la == '+' || la == '-') {
            isFloat = true;
            advance(); // e/E
            if (current() == '+' || current() == '-') advance();
            while (std::isdigit(static_cast<unsigned char>(current()))) advance();
        }
    }

    const std::size_t endPos = m_current_position;
    std::string lex = m_source.substr(startPos, endPos - startPos);

    return makeToken(isFloat ? TokenType::FloatLiteral : TokenType::IntegerLiteral,
                     sLine, sCol, startPos, endPos, lex);
}

Token Lexer::tokenizeString() {
    const std::size_t startPos = m_current_position;
    const std::size_t sLine = m_line;
    const std::size_t sCol = m_col;

    // opening quote
    advance();

    std::string decoded;
    while (!atEnd()) {
        char c = current();
        if (c == '"') {
            advance(); // closing quote
            const std::size_t endPos = m_current_position;
            return makeToken(TokenType::StringLiteral, sLine, sCol, startPos, endPos, decoded);
        }
        if (c == '\n') {
            // Unterminated string
            const std::size_t endPos = m_current_position;
            return makeToken(TokenType::Error, sLine, sCol, startPos, endPos, "Unterminated string literal");
        }
        if (c == '\\') {
            advance();
            char e = current();
            if (atEnd()) break;
            switch (e) {
                case 'n': decoded.push_back('\n'); break;
                case 't': decoded.push_back('\t'); break;
                case 'r': decoded.push_back('\r'); break;
                case '"': decoded.push_back('"'); break;
                case '\\': decoded.push_back('\\'); break;
                default:
                    // keep unknown escapes literally
                    decoded.push_back(e);
                    break;
            }
            advance();
            continue;
        }
        decoded.push_back(c);
        advance();
    }

    // EOF reached without closing quote
    const std::size_t endPos = m_current_position;
    return makeToken(TokenType::Error, sLine, sCol, startPos, endPos, "Unterminated string literal");
}

Token Lexer::getNextToken() {
    skipWhitespaceExceptNewline();

    if (atEnd()) {
        return Token(TokenType::EndOfInput);
    }

    const std::size_t startPos = m_current_position;
    const std::size_t sLine = m_line;
    const std::size_t sCol = m_col;

    char c = current();

    // Newline token
    if (c == '\n') {
        advance();
        return makeToken(TokenType::Newline, sLine, sCol, startPos, m_current_position, "\n");
    }

    // Identifier / keyword
    if (std::isalpha(static_cast<unsigned char>(c)) || c == '_') {
        return tokenizeIdentifierOrKeyword();
    }

    // Number
    if (std::isdigit(static_cast<unsigned char>(c))) {
        return tokenizeNumber();
    }

    // String
    if (c == '"') {
        return tokenizeString();
    }

    // Operators / punctuation (handle multi-char first)
    // ::
    if (c == ':' && next() == ':') { advance(); advance(); return makeToken(TokenType::ScopeResolution, sLine, sCol, startPos, m_current_position); }

    // ??
    if (c == '?' && next() == '?') { advance(); advance(); return makeToken(TokenType::NullCoalescing, sLine, sCol, startPos, m_current_position); }
    // ?.
    if (c == '?' && next() == '.') { advance(); advance(); return makeToken(TokenType::SafeNavigation, sLine, sCol, startPos, m_current_position); }
    // =>
    if (c == '=' && next() == '>') { advance(); advance(); return makeToken(TokenType::Arrow, sLine, sCol, startPos, m_current_position); }

    // ++ / --
    if (c == '+' && next() == '+') { advance(); advance(); return makeToken(TokenType::Increment, sLine, sCol, startPos, m_current_position); }
    if (c == '-' && next() == '-') { advance(); advance(); return makeToken(TokenType::Decrement, sLine, sCol, startPos, m_current_position); }

    // ** / **=
    if (c == '*' && next() == '*') {
        advance(); advance();
        if (current() == '=') { advance(); return makeToken(TokenType::PowerAssign, sLine, sCol, startPos, m_current_position); }
        return makeToken(TokenType::Power, sLine, sCol, startPos, m_current_position);
    }

    // // (floor divide)
    if (c == '/' && next() == '/') { advance(); advance(); return makeToken(TokenType::FloorDivide, sLine, sCol, startPos, m_current_position); }

    // == != <= >=
    if (c == '=' && next() == '=') { advance(); advance(); return makeToken(TokenType::Equals, sLine, sCol, startPos, m_current_position); }
    if (c == '!' && next() == '=') { advance(); advance(); return makeToken(TokenType::NotEquals, sLine, sCol, startPos, m_current_position); }
    if (c == '<' && next() == '=') { advance(); advance(); return makeToken(TokenType::LessEqual, sLine, sCol, startPos, m_current_position); }
    if (c == '>' && next() == '=') { advance(); advance(); return makeToken(TokenType::GreaterEqual, sLine, sCol, startPos, m_current_position); }

    // <<, >>, <<=, >>=
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

    // += -= *= /= %= &= |= ^=
    if (c == '+' && next() == '=') { advance(); advance(); return makeToken(TokenType::PlusAssign, sLine, sCol, startPos, m_current_position); }
    if (c == '-' && next() == '=') { advance(); advance(); return makeToken(TokenType::MinusAssign, sLine, sCol, startPos, m_current_position); }
    if (c == '*' && next() == '=') { advance(); advance(); return makeToken(TokenType::StarAssign, sLine, sCol, startPos, m_current_position); }
    if (c == '/' && next() == '=') { advance(); advance(); return makeToken(TokenType::SlashAssign, sLine, sCol, startPos, m_current_position); }
    if (c == '%' && next() == '=') { advance(); advance(); return makeToken(TokenType::PercentAssign, sLine, sCol, startPos, m_current_position); }
    if (c == '&' && next() == '=') { advance(); advance(); return makeToken(TokenType::BitAndAssign, sLine, sCol, startPos, m_current_position); }
    if (c == '|' && next() == '=') { advance(); advance(); return makeToken(TokenType::BitOrAssign, sLine, sCol, startPos, m_current_position); }
    if (c == '^' && next() == '=') { advance(); advance(); return makeToken(TokenType::BitXorAssign, sLine, sCol, startPos, m_current_position); }

    // Single-char tokens
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
            // Ellipsis ...
            if (next() == '.' && next(2) == '.') {
                advance(); advance(); advance();
                return makeToken(TokenType::Ellipsis, sLine, sCol, startPos, m_current_position);
            }
            advance(); return makeToken(TokenType::Dot, sLine, sCol, startPos, m_current_position);

        case ':': advance(); return makeToken(TokenType::Colon, sLine, sCol, startPos, m_current_position);
        case '?': advance(); return makeToken(TokenType::QuestionMark, sLine, sCol, startPos, m_current_position);

        case '@': advance(); return makeToken(TokenType::At, sLine, sCol, startPos, m_current_position);
        case '#': advance(); return makeToken(TokenType::Hash, sLine, sCol, startPos, m_current_position);
        case '$': advance(); return makeToken(TokenType::Dollar, sLine, sCol, startPos, m_current_position);

        default:
            // Unknown character
            advance();
            return makeToken(TokenType::Invalid, sLine, sCol, startPos, m_current_position,
                             std::string("Unexpected character: ") + c);
    }
}

} // namespace Omniscript
