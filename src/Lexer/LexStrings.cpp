#include <omniscript/Lexer.h>
#include <omniscript/Tokens.h>
#include <omniscript/Console.h>
#include <cctype>

namespace Omniscript {

Token Lexer::getStringToken(char currentChar) {
    size_t startLine = line_;
    size_t startColumn = column_;
    bool isRawString = config_.allowRawStrings && isRawStringStart(currentChar);

    if (isRawString) {
        return parseRawString();
    }
    if (config_.allowTemplateStrings && currentChar == '`') {
        return parseTemplateString();
    }
    if (currentChar == '\'' || currentChar == '"') {
        if (currentChar == '\'' && peek() != '\'') {
            return parseCharacterLiteral();
        }
        return parseStringLiteral();
    }
    console.reportError(Console::ErrorType::SYNTAX_ERROR,
        "Invalid string literal start: " + std::string(1, currentChar),
        FileSpan{{startLine, startColumn}, {line_, column_}, sourceFilePath_});
    return TokenFactory::createInvalid("", line_, column_, sourceFilePath_);
}

Token Lexer::parseStringLiteral() {
    size_t startLine = line_;
    size_t startColumn = column_;
    char quoteType = getCurrentChar();
    advance(); // Skip opening quote
    std::string literalValue;
    bool hasContent = false;

    auto finalizeToken = [&](TokenTypes type) {
        std::u32string u32chars = LexerUtils::utf8ToUtf32(literalValue);
        Token tok = TokenFactory::createString(literalValue, startLine, startColumn, sourceFilePath_);
        if (type == TokenTypes::Invalid) {
            tok = TokenFactory::createInvalid(literalValue, startLine, startColumn, sourceFilePath_);
        }
        tok.setU32Value(u32chars);
        stats_.literalCount++;
        return tok;
    };

    while (!isAtEnd()) {
        char current = getCurrentChar();
        if (literalValue.length() >= config_.maxStringLength) {
            console.reportError(Console::ErrorType::SYNTAX_ERROR,
                "String literal too long",
                "Maximum string length is " + std::to_string(config_.maxStringLength),
                FileSpan{{startLine, startColumn}, {line_, column_}, sourceFilePath_});
            return finalizeToken(TokenTypes::Invalid);
        }

        if (current == quoteType) {
            int backslashCount = 0;
            size_t i = currentPosition_ - 1;
            while (i > 0 && source_[i] == '\\') {
                backslashCount++;
                i--;
            }
            if (backslashCount % 2 == 0) {
                advance();
                return finalizeToken(TokenTypes::StringLiteral);
            }
        }

        if (isNewline(current)) {
            console.reportError(Console::ErrorType::SYNTAX_ERROR,
                "Unterminated string literal",
                "Close the string with a matching quote",
                FileSpan{{startLine, startColumn}, {line_, column_}, sourceFilePath_});
            return finalizeToken(TokenTypes::Invalid);
        }

        if (current == '\\') {
            advance();
            if (isAtEnd()) {
                console.reportError(Console::ErrorType::SYNTAX_ERROR,
                    "Unterminated escape sequence",
                    FileSpan{{startLine, startColumn}, {line_, column_}, sourceFilePath_});
                return finalizeToken(TokenTypes::Invalid);
            }
            char next = getCurrentChar();
            advance();
            switch (next) {
                case 'n': literalValue += '\n'; break;
                case 't': literalValue += '\t'; break;
                case 'r': literalValue += '\r'; break;
                case 'b': literalValue += '\b'; break;
                case 'f': literalValue += '\f'; break;
                case 'v': literalValue += '\v'; break;
                case 'a': literalValue += '\a'; break;
                case '\\': literalValue += '\\'; break;
                case '\'': literalValue += '\''; break;
                case '"': literalValue += '"'; break;
                case '?': literalValue += '?'; break;
                case '0': case '1': case '2': case '3':
                case '4': case '5': case '6': case '7': {
                    int val = next - '0';
                    for (int i = 0; i < 2 && !isAtEnd(); ++i) {
                        char c = getCurrentChar();
                        if (c >= '0' && c <= '7') {
                            val = val * 8 + (c - '0');
                            advance();
                        } else {
                            break;
                        }
                    }
                    literalValue += static_cast<char>(val);
                    break;
                }
                case 'x': {
                    int val = 0;
                    int digits = 0;
                    while (!isAtEnd()) {
                        char c = getCurrentChar();
                        if (std::isdigit(c) || (std::tolower(c) >= 'a' && std::tolower(c) <= 'f')) {
                            val = val * 16 + (std::isdigit(c) ? (c - '0') : (std::tolower(c) - 'a' + 10));
                            advance();
                            digits++;
                        } else {
                            break;
                        }
                    }
                    if (digits == 0) {
                        console.reportError(Console::ErrorType::SYNTAX_ERROR,
                            "Invalid hex escape sequence",
                            "Provide valid hex digits after \\x",
                            FileSpan{{startLine, startColumn}, {line_, column_}, sourceFilePath_});
                        return finalizeToken(TokenTypes::Invalid);
                    }
                    literalValue += static_cast<char>(val);
                    break;
                }
                case 'u': case 'U': {
                    uint32_t codepoint = parseUnicodeEscape();
                    if (codepoint == 0xFFFFFFFF) {
                        return finalizeToken(TokenTypes::Invalid);
                    }
                    literalValue += utf32ToUtf8(codepoint);
                    break;
                }
                default:
                    console.reportError(Console::ErrorType::SYNTAX_ERROR,
                        "Unknown escape sequence: \\" + std::string(1, next),
                        "Use valid escape characters (e.g., \\n, \\t, \\uXXXX)",
                        FileSpan{{startLine, startColumn}, {line_, column_}, sourceFilePath_});
                    literalValue += '\\';
                    literalValue += next;
                    break;
            }
            hasContent = true;
        } else {
            literalValue += current;
            hasContent = true;
            advance();
        }
    }

    console.reportError(Console::ErrorType::SYNTAX_ERROR,
        "Unterminated string literal",
        "Close the string with a matching quote",
        FileSpan{{startLine, startColumn}, {line_, column_}, sourceFilePath_});
    return finalizeToken(TokenTypes::Invalid);
}

Token Lexer::parseCharacterLiteral() {
    size_t startLine = line_;
    size_t startColumn = column_;
    advance(); // Skip '
    std::string literalValue;

    auto finalizeToken = [&](TokenTypes type) {
        std::u32string u32chars = LexerUtils::utf8ToUtf32(literalValue);
        Token tok = type == TokenTypes::Character ?
            Token(TokenTypes::Character, u32chars.empty() ? '\0' : u32chars[0], startLine, startColumn, sourceFilePath_) :
            TokenFactory::createInvalid(literalValue, startLine, startColumn, sourceFilePath_);
        tok.setU32Value(u32chars);
        stats_.literalCount++;
        return tok;
    };

    if (isAtEnd()) {
        console.reportError(Console::ErrorType::SYNTAX_ERROR,
            "Unterminated character literal",
            "Close the character literal with a single quote",
            FileSpan{{startLine, startColumn}, {line_, column_}, sourceFilePath_});
        return finalizeToken(TokenTypes::Invalid);
    }

    char current = getCurrentChar();
    if (current == '\\') {
        advance();
        if (isAtEnd()) {
            console.reportError(Console::ErrorType::SYNTAX_ERROR,
                "Unterminated escape sequence",
                FileSpan{{startLine, startColumn}, {line_, column_}, sourceFilePath_});
            return finalizeToken(TokenTypes::Invalid);
        }
        char next = getCurrentChar();
        advance();
        switch (next) {
            case 'n': literalValue += '\n'; break;
            case 't': literalValue += '\t'; break;
            case 'r': literalValue += '\r'; break;
            case 'b': literalValue += '\b'; break;
            case 'f': literalValue += '\f'; break;
            case 'v': literalValue += '\v'; break;
            case 'a': literalValue += '\a'; break;
            case '\\': literalValue += '\\'; break;
            case '\'': literalValue += '\''; break;
            case '"': literalValue += '"'; break;
            case '?': literalValue += '?'; break;
            case '0': case '1': case '2': case '3':
            case '4': case '5': case '6': case '7': {
                int val = next - '0';
                for (int i = 0; i < 2 && !isAtEnd(); ++i) {
                    char c = getCurrentChar();
                    if (c >= '0' && c <= '7') {
                        val = val * 8 + (c - '0');
                        advance();
                    } else {
                        break;
                    }
                }
                literalValue += static_cast<char>(val);
                break;
            }
            case 'x': {
                int val = 0;
                int digits = 0;
                while (!isAtEnd()) {
                    char c = getCurrentChar();
                    if (std::isdigit(c) || (std::tolower(c) >= 'a' && std::tolower(c) <= 'f')) {
                        val = val * 16 + (std::isdigit(c) ? (c - '0') : (std::tolower(c) - 'a' + 10));
                        advance();
                        digits++;
                    } else {
                        break;
                    }
                }
                if (digits == 0) {
                    console.reportError(Console::ErrorType::SYNTAX_ERROR,
                        "Invalid hex escape sequence",
                        "Provide valid hex digits after \\x",
                        FileSpan{{startLine, startColumn}, {line_, column_}, sourceFilePath_});
                    return finalizeToken(TokenTypes::Invalid);
                }
                literalValue += static_cast<char>(val);
                break;
            }
            case 'u': case 'U': {
                uint32_t codepoint = parseUnicodeEscape();
                if (codepoint == 0xFFFFFFFF) {
                    return finalizeToken(TokenTypes::Invalid);
                }
                literalValue += utf32ToUtf8(codepoint);
                break;
            }
            default:
                console.reportError(Console::ErrorType::SYNTAX_ERROR,
                    "Unknown escape sequence: \\" + std::string(1, next),
                    "Use valid escape characters",
                    FileSpan{{startLine, startColumn}, {line_, column_}, sourceFilePath_});
                literalValue += '\\';
                literalValue += next;
                break;
        }
    } else {
        literalValue += current;
        advance();
    }

    if (isAtEnd() || getCurrentChar() != '\'') {
        console.reportError(Console::ErrorType::SYNTAX_ERROR,
            "Invalid character literal",
            "Character literals must contain exactly one character and end with a single quote",
            FileSpan{{startLine, startColumn}, {line_, column_}, sourceFilePath_});
        return finalizeToken(TokenTypes::Invalid);
    }

    advance(); // Skip closing '
    std::u32string u32chars = LexerUtils::utf8ToUtf32(literalValue);
    if (u32chars.size() != 1) {
        console.reportError(Console::ErrorType::SYNTAX_ERROR,
            "Invalid character literal: too many characters",
            "Character literals must contain exactly one character",
            FileSpan{{startLine, startColumn}, {line_, column_}, sourceFilePath_});
        return finalizeToken(TokenTypes::Invalid);
    }
    return finalizeToken(TokenTypes::Character);
}

Token Lexer::parseRawString() {
    size_t startLine = line_;
    size_t startColumn = column_;
    advance(2); // Skip R"
    std::string delimiter;
    while (!isAtEnd() && getCurrentChar() != '(') {
        delimiter += getCurrentChar();
        advance();
    }

    if (isAtEnd()) {
        console.reportError(Console::ErrorType::SYNTAX_ERROR,
            "Incomplete raw string literal",
            FileSpan{{startLine, startColumn}, {line_, column_}, sourceFilePath_});
        return TokenFactory::createInvalid("", startLine, startColumn, sourceFilePath_);
    }

    advance(); // Skip (
    std::string value;
    while (!isAtEnd()) {
        if (value.length() >= config_.maxStringLength) {
            console.reportError(Console::ErrorType::SYNTAX_ERROR,
                "Raw string literal too long",
                "Maximum string length is " + std::to_string(config_.maxStringLength),
                FileSpan{{startLine, startColumn}, {line_, column_}, sourceFilePath_});
            return TokenFactory::createInvalid(value, startLine, startColumn, sourceFilePath_);
        }

        if (getCurrentChar() == ')' && peekString(delimiter.length() + 1) == ")" + delimiter + "\"") {
            advance(delimiter.length() + 2);
            stats_.literalCount++;
            return TokenFactory::createString(value, startLine, startColumn, sourceFilePath_);
        }

        value += getCurrentChar();
        advance();
    }

    console.reportError(Console::ErrorType::SYNTAX_ERROR,
        "Unterminated raw string literal",
        FileSpan{{startLine, startColumn}, {line_, column_}, sourceFilePath_});
    return TokenFactory::createInvalid(value, startLine, startColumn, sourceFilePath_);
}

Token Lexer::parseTemplateString() {
    size_t startLine = line_;
    size_t startColumn = column_;
    advance(); // Skip `
    std::string value;
    bool hasContent = false;

    auto finalizeToken = [&](TokenTypes type) {
        std::u32string u32chars = LexerUtils::utf8ToUtf32(value);
        Token tok = Token(type, value, startLine, startColumn, sourceFilePath_);
        tok.setU32Value(u32chars);
        stats_.literalCount++;
        return tok;
    };

    while (!isAtEnd()) {
        if (value.length() >= config_.maxStringLength) {
            console.reportError(Console::ErrorType::SYNTAX_ERROR,
                "Template string too long",
                "Maximum string length is " + std::to_string(config_.maxStringLength),
                FileSpan{{startLine, startColumn}, {line_, column_}, sourceFilePath_});
            return finalizeToken(TokenTypes::Invalid);
        }

        if (getCurrentChar() == '`') {
            advance();
            return finalizeToken(TokenTypes::TemplateTail);
        }

        if (getCurrentChar() == '$' && peek() == '{') {
            advance(2);
            return finalizeToken(hasContent ? TokenTypes::TemplateMiddle : TokenTypes::TemplateHead);
        }

        if (isNewline(getCurrentChar())) {
            value += '\n';
            line_++;
            column_ = 1;
            stats_.totalLines++;
            if (getCurrentChar() == '\r' && peek() == '\n') {
                advance(2);
            } else {
                advance();
            }
            continue;
        }

        value += getCurrentChar();
        hasContent = true;
        advance();
    }

    console.reportError(Console::ErrorType::SYNTAX_ERROR,
        "Unterminated template string",
        "Close the template string with a backtick",
        FileSpan{{startLine, startColumn}, {line_, column_}, sourceFilePath_});
    return finalizeToken(TokenTypes::Invalid);
}

uint32_t Lexer::parseUnicodeEscape() {
    size_t startLine = line_;
    size_t startColumn = column_;
    char type = getCurrentChar(); // u or U
    advance();
    int needed = (type == 'u' ? 4 : 8);
    uint32_t codepoint = 0;

    if (currentPosition_ + needed > source_.length()) {
        console.reportError(Console::ErrorType::SYNTAX_ERROR,
            "Invalid Unicode escape sequence",
            "Provide correct number of hex digits for \\u (4) or \\U (8)",
            FileSpan{{startLine, startColumn}, {line_, column_}, sourceFilePath_});
        return 0xFFFFFFFF;
    }

    for (int i = 0; i < needed; ++i) {
        char c = getCurrentChar();
        if (std::isdigit(c) || (std::tolower(c) >= 'a' && std::tolower(c) <= 'f')) {
            codepoint = codepoint * 16 + (std::isdigit(c) ? (c - '0') : (std::tolower(c) - 'a' + 10));
            advance();
        } else {
            console.reportError(Console::ErrorType::SYNTAX_ERROR,
                "Invalid Unicode digit in \\" + std::string(1, type),
                "Use valid hex digits in Unicode escape",
                FileSpan{{startLine, startColumn}, {line_, column_}, sourceFilePath_});
            return 0xFFFFFFFF;
        }
    }
    return codepoint;
}

std::string Lexer::utf32ToUtf8(uint32_t codepoint) {
    std::string result;
    if (codepoint <= 0x7F) {
        result += static_cast<char>(codepoint);
    } else if (codepoint <= 0x7FF) {
        result += static_cast<char>(0xC0 | ((codepoint >> 6) & 0x1F));
        result += static_cast<char>(0x80 | (codepoint & 0x3F));
    } else if (codepoint <= 0xFFFF) {
        result += static_cast<char>(0xE0 | ((codepoint >> 12) & 0x0F));
        result += static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
        result += static_cast<char>(0x80 | (codepoint & 0x3F));
    } else {
        result += static_cast<char>(0xF0 | ((codepoint >> 18) & 0x07));
        result += static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F));
        result += static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
        result += static_cast<char>(0x80 | (codepoint & 0x3F));
    }
    return result;
}

} // namespace Omniscript
