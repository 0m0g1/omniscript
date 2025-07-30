#include <omniscript/Lexer.h>
#include <omniscript/Tokens.h>
#include <omniscript/Console.h>
#include <omniscript/utils.h>
#include <cctype>
#include <algorithm>
#include <sstream>
#include <regex>

namespace Omniscript {

const std::unordered_map<std::string, TokenTypes> Lexer::keywordMap_ = {
    {"if", TokenTypes::If}, {"else", TokenTypes::Else}, {"while", TokenTypes::While},
    {"for", TokenTypes::For}, {"continue", TokenTypes::Continue}, {"break", TokenTypes::Break},
    {"return", TokenTypes::Return}, {"function", TokenTypes::Function}, {"fn", TokenTypes::Function},
    {"let", TokenTypes::Let}, {"var", TokenTypes::Let}, {"namespace", TokenTypes::Namespace},
    {"using", TokenTypes::Using}, {"new", TokenTypes::New}, {"delete", TokenTypes::Delete},
    {"struct", TokenTypes::Struct}, {"class", TokenTypes::Class}, {"extends", TokenTypes::Extends},
    {"variant", TokenTypes::Variant}, {"any", TokenTypes::Any}, {"enum", TokenTypes::Enum},
    {"public", TokenTypes::Public}, {"private", TokenTypes::Private}, {"protected", TokenTypes::Protected},
    {"override", TokenTypes::Override}, {"virtual", TokenTypes::Virtual}, {"static", TokenTypes::Static},
    {"final", TokenTypes::Final}, {"const", TokenTypes::Const}, {"true", TokenTypes::True},
    {"false", TokenTypes::False}, {"nullptr", TokenTypes::Nullptr}, {"null", TokenTypes::Null},
    {"xor", TokenTypes::LogicalXor}, {"include", TokenTypes::Include}, {"import", TokenTypes::Import},
    {"from", TokenTypes::From}, {"module", TokenTypes::Module}, {"mod", TokenTypes::Module},
    {"extern", TokenTypes::Extern}, {"intrinsic", TokenTypes::Intrinsic}, {"volatile", TokenTypes::Volatile},
    {"as", TokenTypes::As}, {"type", TokenTypes::Type}
};

const std::unordered_set<std::string> Lexer::reservedWords_ = {
    "if", "else", "while", "for", "function", "fn", "let", "var",
    "const", "true", "false", "null", "nullptr", "return", "break",
    "continue", "struct", "class", "enum", "namespace", "using",
    "public", "private", "protected", "static", "virtual", "override",
    "final", "extern", "intrinsic", "volatile", "new", "delete",
    "this", "super", "extends", "implements", "import", "export",
    "module", "async", "await", "yield", "typeof", "instanceof"
};

// Initialize isIdentifierStartTable_ (A-Z, _, a-z)
const std::array<bool, 256> Lexer::isIdentifierStartTable_ = []() {
    std::array<bool, 256> arr = {};
    for (int i = 65; i <= 90; ++i) arr[i] = true;  // A-Z
    arr[95] = true;                                // _
    for (int i = 97; i <= 122; ++i) arr[i] = true; // a-z
    return arr;
}();

// Initialize isIdentifierContinuationTable_ (0-9, A-Z, _, a-z)
const std::array<bool, 256> Lexer::isIdentifierContinuationTable_ = []() {
    std::array<bool, 256> arr = {};
    for (int i = 48; i <= 57; ++i) arr[i] = true;  // 0-9
    for (int i = 65; i <= 90; ++i) arr[i] = true;  // A-Z
    arr[95] = true;                                // _
    for (int i = 97; i <= 122; ++i) arr[i] = true; // a-z
    return arr;
}();

// Initialize isDigitTable_ (0-9)
const std::array<bool, 256> Lexer::isDigitTable_ = []() {
    std::array<bool, 256> arr = {};
    for (int i = 48; i <= 57; ++i) arr[i] = true;  // 0-9
    return arr;
}();

// Initialize isHexDigitTable_ (0-9, A-F, a-f)
const std::array<bool, 256> Lexer::isHexDigitTable_ = []() {
    std::array<bool, 256> arr = {};
    for (int i = 48; i <= 57; ++i) arr[i] = true;  // 0-9
    for (int i = 65; i <= 70; ++i) arr[i] = true;  // A-F
    for (int i = 97; i <= 102; ++i) arr[i] = true; // a-f
    return arr;
}();

// Initialize isWhitespaceTable_ (tab, newline, carriage return, space)
const std::array<bool, 256> Lexer::isWhitespaceTable_ = []() {
    std::array<bool, 256> arr = {};
    arr[9] = true;   // Tab
    arr[10] = true;  // Newline
    arr[13] = true;  // Carriage return
    arr[32] = true;  // Space
    return arr;
}();

Lexer::Lexer(const std::string& source, const std::string& filePath, const LexerConfig& config)
    : source_(source)
    , sourceFilePath_(filePath)
    , currentPosition_(0)
    , line_(1)
    , column_(1)
    , config_(config)
    , cachePosition_(0)
    , debugMode_(false)
{
    stats_.totalCharacters = source_.length();
}

Token Lexer::getNextToken() {
    if (pendingNewlineToken_.has_value()) {
        Token token = pendingNewlineToken_.value();
        pendingNewlineToken_ = std::nullopt;
        updateStatistics(token);
        if (debugMode_) debugPrintToken(token);
        return token;
    }

    skipWhitespace();

    if (isAtEnd()) {
        Token eoi = TokenFactory::createEOI(line_, column_, sourceFilePath_);
        updateStatistics(eoi);
        if (debugMode_) debugPrintToken(eoi);
        return eoi;
    }

    char currentChar = getCurrentChar();
    size_t startLine = line_;
    size_t startColumn = column_;

    try {
        if (currentChar == '/' && (peek() == '/' || peek() == '*')) {
            Token commentResult = handleComments();
            if (commentResult.getType() != TokenTypes::Invalid) {
                updateStatistics(commentResult);
                if (debugMode_) debugPrintToken(commentResult);
                return commentResult;
            }
        }

        currentChar = getCurrentChar();

        bool isRawStringPrefix = config_.allowRawStrings && isRawStringStart(currentChar);

        if (isIdentifierStart(currentChar) && !isRawStringPrefix) {
            Token token = parseIdentifierOrKeyword();
            updateStatistics(token);
            if (debugMode_) debugPrintToken(token);
            return token;
        }

        if (isQuote(currentChar) || isRawStringPrefix) {
            Token stringToken = getStringToken(currentChar);
            updateStatistics(stringToken);
            if (debugMode_) debugPrintToken(stringToken);
            return stringToken;
        }

        if (isDigit(currentChar) || (currentChar == '.' && isDigit(peek())) || (currentChar == '-' && isDigit(peek()))) {
            Token numberLiteral = getNumberLiterals(currentChar);
            updateStatistics(numberLiteral);
            if (debugMode_) debugPrintToken(numberLiteral);
            return numberLiteral;
        }

        Token operatorToken = getOperator(currentChar);
        updateStatistics(operatorToken);
        if (debugMode_) debugPrintToken(operatorToken);
        return operatorToken;

    } catch (const std::exception& e) {
        console.reportError(Console::ErrorType::INTERNAL_ERROR,
            "Internal lexer error: " + std::string(e.what()),
            "Please report this as a bug",
            FileSpan{startLine, startColumn, line_, column_, sourceFilePath_});
        Token errorToken = TokenFactory::createInvalid("lexer error", startLine, startColumn, sourceFilePath_);
        updateStatistics(errorToken);
        if (debugMode_) debugPrintToken(errorToken);
        return errorToken;
    }
}

void Lexer::skipWhitespace() {
    while (!isAtEnd() && isWhitespace(getCurrentChar())) {
        char c = getCurrentChar();
        if (isNewline(c)) {
            size_t newlineColumn = column_;
            if (c == '\r' && peek() == '\n') {
                advance(2);
            } else {
                advance();
            }
            line_++;
            column_ = 1;
            stats_.totalLines++;
            if (config_.trackNewlines) {
                pendingNewlineToken_ = TokenFactory::createOperator(TokenTypes::Newline, line_ - 1, newlineColumn, sourceFilePath_);
                return;
            }
        } else {
            advance();
        }
    }
}

Token Lexer::handleComments() {
    char currentChar = getCurrentChar();
    char nextChar = peek();
    if (currentChar == '/' && nextChar == '/') {
        return handleSingleLineComment();
    }
    if (currentChar == '/' && nextChar == '*') {
        return handleMultiLineComment();
    }
    return TokenFactory::createInvalid("", line_, column_, sourceFilePath_);
}

Token Lexer::handleSingleLineComment() {
    size_t startLine = line_;
    size_t startColumn = column_;
    advance(2); // Skip //
    stats_.commentCount++;
    while (!isAtEnd() && !isNewline(getCurrentChar())) {
        advance();
    }
    return getNextToken();
}

Token Lexer::handleMultiLineComment() {
    size_t startLine = line_;
    size_t startColumn = column_;
    int nestingDepth = 1;
    advance(2); // Skip /*
    stats_.commentCount++;

    while (nestingDepth > 0 && !isAtEnd()) {
        if (getCurrentChar() == '/' && peek() == '*' && config_.allowNestedComments) {
            if (nestingDepth >= static_cast<int>(config_.maxCommentDepth)) {
                console.reportError(Console::ErrorType::SYNTAX_ERROR,
                    "Comment nesting too deep",
                    "Consider reducing comment nesting or using single-line comments",
                    FileSpan{startLine, startColumn, line_, column_, sourceFilePath_});
                break;
            }
            nestingDepth++;
            advance(2);
        } else if (getCurrentChar() == '*' && peek() == '/') {
            nestingDepth--;
            advance(2);
        } else {
            if (isNewline(getCurrentChar())) {
                line_++;
                column_ = 1;
                stats_.totalLines++;
                if (getCurrentChar() == '\r' && peek() == '\n') {
                    advance(2);
                } else {
                    advance();
                }
            } else {
                advance();
            }
        }
    }

    if (nestingDepth > 0) {
        console.reportError(Console::ErrorType::SYNTAX_ERROR,
            "Unclosed multi-line comment",
            "Close the comment with '*/'",
            FileSpan{startLine, startColumn, line_, column_, sourceFilePath_});
        return TokenFactory::createInvalid("", startLine, startColumn, sourceFilePath_);
    }

    return getNextToken();
}

Token Lexer::handleUnexpectedCharacter(char currentChar) {
    console.reportError(Console::ErrorType::SYNTAX_ERROR,
        "Unexpected character: " + std::string(1, currentChar),
        "Check for valid tokens or syntax",
        FileSpan{line_, column_, line_, column_, sourceFilePath_});
    return TokenFactory::createInvalid(std::string(1, currentChar), line_, column_, sourceFilePath_);
}

char Lexer::getCurrentChar() const {
    return isAtEnd() ? '\0' : source_[currentPosition_];
}

char Lexer::peek(int offset) const {
    size_t pos = currentPosition_ + offset;
    return pos < source_.length() ? source_[pos] : '\0';
}

std::string Lexer::peekString(size_t length) const {
    if (currentPosition_ + length > source_.length()) {
        return source_.substr(currentPosition_);
    }
    return source_.substr(currentPosition_, length);
}

void Lexer::advance(size_t count) {
    for (size_t i = 0; i < count && !isAtEnd(); ++i) {
        if (isNewline(getCurrentChar())) {
            line_++;
            column_ = 1;
            stats_.totalLines++;
        } else {
            column_++;
        }
        currentPosition_++;
    }
}

void Lexer::advanceLine() {
    line_++;
    column_ = 1;
    stats_.totalLines++;
    if (!isAtEnd()) {
        currentPosition_++;
    }
}

void Lexer::updatePositionFromToken(const Token& token) {
    line_ = token.getLine();
    column_ = token.getColumn();
}

bool Lexer::isIdentifierStart(char c) const {
    if (c < 128) {
        return isIdentifierStartTable_[static_cast<unsigned char>(c)];
    }
    return config_.allowUnicodeIdentifiers && isUnicodeIdentifierStart(static_cast<uint32_t>(c));
}

bool Lexer::isIdentifierContinuation(char c) const {
    if (c < 128) {
        return isIdentifierContinuationTable_[static_cast<unsigned char>(c)];
    }
    return config_.allowUnicodeIdentifiers && isUnicodeIdentifierContinuation(static_cast<uint32_t>(c));
}

bool Lexer::isDigit(char c) const {
    return c < 128 && isDigitTable_[static_cast<unsigned char>(c)];
}

bool Lexer::isHexDigit(char c) const {
    return c < 128 && isHexDigitTable_[static_cast<unsigned char>(c)];
}

bool Lexer::isBinaryDigit(char c) const {
    return c == '0' || c == '1';
}

bool Lexer::isOctalDigit(char c) const {
    return c >= '0' && c <= '7';
}

bool Lexer::isWhitespace(char c) const {
    return c < 128 && isWhitespaceTable_[static_cast<unsigned char>(c)];
}

bool Lexer::isNewline(char c) const {
    return c == '\n' || c == '\r';
}

bool Lexer::isQuote(char c) const {
    return c == '"' || c == '\'';
}

bool Lexer::isRawStringStart(char currentChar) const {
    return currentChar == 'R' && (peek() == '"' || peek() == '\'' || peek() == '`');
}

bool Lexer::isUnicodeIdentifierStart(uint32_t codepoint) const {
    return (codepoint >= 'A' && codepoint <= 'Z') || (codepoint >= 'a' && codepoint <= 'z') || codepoint == '_';
}

bool Lexer::isUnicodeIdentifierContinuation(uint32_t codepoint) const {
    return (codepoint >= '0' && codepoint <= '9') ||
           (codepoint >= 'A' && codepoint <= 'Z') ||
           (codepoint >= 'a' && codepoint <= 'z') || codepoint == '_';
}

std::string Lexer::unescapeString(const std::string& str) {
    std::string result;
    for (size_t i = 0; i < str.length(); ++i) {
        if (str[i] == '\\' && i + 1 < str.length()) {
            ++i;
            switch (str[i]) {
                case 'n': result += '\n'; break;
                case 't': result += '\t'; break;
                case 'r': result += '\r'; break;
                case 'b': result += '\b'; break;
                case 'f': result += '\f'; break;
                case 'v': result += '\v'; break;
                case 'a': result += '\a'; break;
                case '\\': result += '\\'; break;
                case '\'': result += '\''; break;
                case '"': result += '"'; break;
                case '?': result += '?'; break;
                default: result += '\\'; result += str[i]; break;
            }
        } else {
            result += str[i];
        }
    }
    return result;
}

std::string Lexer::escapeString(const std::string& str) {
    std::string result;
    for (char c : str) {
        switch (c) {
            case '\n': result += "\\n"; break;
            case '\t': result += "\\t"; break;
            case '\r': result += "\\r"; break;
            case '\b': result += "\\b"; break;
            case '\f': result += "\\f"; break;
            case '\v': result += "\\v"; break;
            case '\a': result += "\\a"; break;
            case '\\': result += "\\\\"; break;
            case '\'': result += "\\'"; break;
            case '"': result += "\\\""; break;
            case '?': result += "\\?"; break;
            default: result += c; break;
        }
    }
    return result;
}

bool Lexer::isValidEscapeSequence(char c) const {
    return c == 'n' || c == 't' || c == 'r' || c == 'b' || c == 'f' ||
           c == 'v' || c == 'a' || c == '\\' || c == '\'' || c == '"' ||
           c == '?' || c == '0' || c == 'x' || c == 'u' || c == 'U';
}

void Lexer::skipToNextStatement() {
    while (!isAtEnd()) {
        char c = getCurrentChar();
        if (c == ';' || isNewline(c)) {
            advance();
            break;
        }
        advance();
    }
}

void Lexer::skipToToken(TokenTypes targetType) {
    while (!isAtEnd()) {
        Token token = peekToken();
        if (token.getType() == targetType || token.getType() == TokenTypes::EOI) {
            break;
        }
        advance();
    }
}

bool Lexer::synchronize() {
    skipToNextStatement();
    return !isAtEnd();
}

void Lexer::skipToMatchingBracket(TokenTypes openBracket) {
    TokenTypes closeBracket;
    switch (openBracket) {
        case TokenTypes::LeftParen: closeBracket = TokenTypes::RightParen; break;
        case TokenTypes::LeftBrace: closeBracket = TokenTypes::RightBrace; break;
        case TokenTypes::LeftBracket: closeBracket = TokenTypes::RightBracket; break;
        default: return;
    }

    int nesting = 1;
    while (!isAtEnd() && nesting > 0) {
        Token token = getNextToken();
        if (token.getType() == openBracket) {
            nesting++;
        } else if (token.getType() == closeBracket) {
            nesting--;
        }
    }
}

bool Lexer::isValidIdentifier(const std::string& identifier) const {
    if (identifier.empty()) return false;
    if (!isIdentifierStart(identifier[0])) return false;
    for (size_t i = 1; i < identifier.length(); ++i) {
        if (!isIdentifierContinuation(identifier[i])) return false;
    }
    return !isReservedKeyword(identifier);
}

bool Lexer::isValidNumber(const std::string& number) const {
    return LexerUtils::isValidNumber(number);
}

bool Lexer::isValidStringLiteral(const std::string& literal) const {
    return literal.length() <= config_.maxStringLength;
}

bool Lexer::isReservedKeyword(const std::string& word) const {
    std::string check = config_.caseSensitiveKeywords ? word : LexerUtils::toLowerCaseString(word);
    return reservedWords_.find(check) != reservedWords_.end();
}

void Lexer::cacheToken(const Token& token) {
    if (tokenCache_.size() <= cachePosition_) {
        tokenCache_.push_back(token);
    } else {
        tokenCache_[cachePosition_] = token;
    }
    cachePosition_++;
}

Token Lexer::getCachedToken(size_t offset) {
    if (offset < tokenCache_.size()) {
        return tokenCache_[offset];
    }
    return TokenFactory::createInvalid("", line_, column_, sourceFilePath_);
}

void Lexer::clearTokenCache() {
    tokenCache_.clear();
    cachePosition_ = 0;
}

void Lexer::debugPrintToken(const Token& token) const {
    console.debug(token.toDebugString());
}

void Lexer::debugPrintPosition() const {
    console.debug("Position: " + sourceFilePath_ + ":" + std::to_string(line_) + ":" + std::to_string(column_));
}

void Lexer::updateStatistics(const Token& token) {
    stats_.totalTokens++;
    stats_.tokenFrequency[token.getType()]++;
    if (token.getType() == TokenTypes::Newline) {
        stats_.totalLines++;
    }
}

bool Lexer::isAtEnd() const {
    return currentPosition_ >= source_.length();
}

double Lexer::getProgress() const {
    if (source_.empty()) return 1.0;
    return static_cast<double>(currentPosition_) / source_.length();
}

void Lexer::reset() {
    currentPosition_ = 0;
    line_ = 1;
    column_ = 1;
    pendingNewlineToken_ = std::nullopt;
    errors_.clear();
    tokenCache_.clear();
    cachePosition_ = 0;
    stats_ = LexerStats();
    stats_.totalCharacters = source_.length();
}

void Lexer::seekTo(size_t position) {
    if (position >= source_.length()) {
        currentPosition_ = source_.length();
        return;
    }
    currentPosition_ = position;
    line_ = 1;
    column_ = 1;
    for (size_t i = 0; i < position; ++i) {
        if (isNewline(source_[i])) {
            line_++;
            column_ = 1;
        } else {
            column_++;
        }
    }
}

void Lexer::seekToLine(size_t line) {
    reset();
    while (line_ < line && !isAtEnd()) {
        if (isNewline(getCurrentChar())) {
            line_++;
            column_ = 1;
            if (getCurrentChar() == '\r' && peek() == '\n') {
                advance(2);
            } else {
                advance();
            }
        } else {
            advance();
        }
    }
}

LexerIterator::LexerIterator(Lexer& lexer, bool atEnd)
    : lexer_(&lexer), atEnd_(atEnd) {
    if (!atEnd_) {
        currentToken_ = lexer_->getNextToken();
    }
}

Token LexerIterator::operator*() {
    return currentToken_;
}

LexerIterator& LexerIterator::operator++() {
    if (!atEnd_) {
        currentToken_ = lexer_->getNextToken();
        if (currentToken_.getType() == TokenTypes::EOI) {
            atEnd_ = true;
        }
    }
    return *this;
}

LexerIterator LexerIterator::operator++(int) {
    LexerIterator tmp = *this;
    ++(*this);
    return tmp;
}

bool LexerIterator::operator==(const LexerIterator& other) const {
    return lexer_ == other.lexer_ && atEnd_ == other.atEnd_;
}

bool LexerIterator::operator!=(const LexerIterator& other) const {
    return !(*this == other);
}

} // namespace Omniscript
