#include <omniscript/Core.h>
#include <omniscript/omniscript_pch.h>
#include <omniscript/Lexer.h>
#include <omniscript/Tokens.h>
#include <omniscript/utils.h>

Token Lexer::getStringToken(char &currentChar) {
    Omniscript::FileSpan span;
    span.start.line = line;
    span.start.col = column;
    span.start.filePath = sourceFilePath;

    bool isRawString = false;
    if (currentChar == 'r') {
        char next = peek();
        if (next == '"' || next == '\'' || next == '`') {
            isRawString = true;
            currentPosition++;
            column++;
            currentChar = next;
        }
    }

    if (currentChar == '\"' || currentChar == '\'' || currentChar == '`') {
        char quoteType = currentChar;
        std::string literalValue;
        size_t startLine = line;
        size_t startColumn = column;
        
        currentPosition++; // Move past the opening quote
        column++;
    
        bool isTemplate = (quoteType == '`') && !isRawString;
        bool isString = (quoteType == '\"');
        bool hasContent = false;

        auto finalizeToken = [&](TokenTypes type) {
            std::u32string u32chars = utf8_to_utf32(literalValue);
            auto tok = Token(type, literalValue, startLine, startColumn, sourceFilePath);
            tok.setU32Value(u32chars);
            span.end.line = line;
            span.end.col = column;
            span.end.filePath = sourceFilePath;
            return tok;
        };

        while (currentPosition < source.length()) {
            char currentChar = source[currentPosition];

            // Check for closing quote (only if not template with ${)
            if (currentChar == quoteType) {
                int backslashCount = 0;
                int i = currentPosition - 1;
                while (i >= 0 && source[i] == '\\') {
                    backslashCount++;
                    i--;
                }

                if (backslashCount % 2 == 0) {
                    currentPosition++;
                    column++;
                    
                    if (quoteType == '\'') {
                        std::u32string u32chars = utf8_to_utf32(literalValue);
                        if (u32chars.size() > 1) {
                            span.end.line = line;
                            span.end.col = column;
                            span.end.filePath = sourceFilePath;
                            console.reportError(
                                Omniscript::Console::SYNTAX_ERROR,
                                "Invalid character literal: too many characters",
                                "To resolve this:\n1. Ensure character literal contains exactly one character\n2. Check single-quote syntax\n3. Use double quotes for strings",
                                span
                            );
                            return finalizeToken(TokenTypes::StringLiteral);
                        }
                        return finalizeToken(TokenTypes::Character);
                    } else if (quoteType == '`') {
                        return finalizeToken(TokenTypes::TemplateTail);
                    } else {
                        return finalizeToken(TokenTypes::StringLiteral);
                    }
                }
            }

            if (isRawString) {
                if (currentChar == quoteType) {
                    currentPosition++;
                    column++;
                    return finalizeToken(isTemplate ? TokenTypes::TemplateTail : 
                                    (quoteType == '\'' ? TokenTypes::Character : TokenTypes::StringLiteral));
                }
                
                if (isTemplate && currentChar == '$' && peek() == '{') {
                    currentPosition += 2;
                    column += 2;
                    return finalizeToken(hasContent ? TokenTypes::TemplateMiddle : TokenTypes::TemplateHead);
                }
                
                literalValue += currentChar;
                hasContent = true;
                currentPosition++;
                column++;
                continue;
            }

            if (isTemplate && currentChar == '$' && peek() == '{') {
                currentPosition += 2;
                column += 2;
                return finalizeToken(hasContent ? TokenTypes::TemplateMiddle : TokenTypes::TemplateHead);
            }

            if (currentChar == '\n' || currentChar == '\r') {                
                if (isTemplate) {
                    literalValue += '\n';
                    line++;
                    column = 1;
                    
                    if (currentChar == '\r' && currentPosition + 1 < source.length() && source[currentPosition + 1] == '\n') {
                        currentPosition++;
                    }
                    currentPosition++;
                    
                    while (currentPosition < source.length()) {
                        char nextChar = source[currentPosition];
                        if (nextChar == ' ' || nextChar == '\t') {
                            currentPosition++;
                            column++;
                        } else {
                            break;
                        }
                    }
                } else {
                    currentPosition++;
                    if (currentChar == '\r' && currentPosition < source.length() && source[currentPosition] == '\n') {
                        currentPosition++;
                    }
                    line++;
                    column = 1;
                    
                    while (currentPosition < source.length()) {
                        char nextChar = source[currentPosition];
                        if (nextChar == ' ' || nextChar == '\t') {
                            currentPosition++;
                            column++;
                        } else {
                            break;
                        }
                    }
                }
                continue;
            }

            if (currentChar == '\\') {
                if (currentPosition + 1 >= source.length()) {
                    span.end.line = line;
                    span.end.col = column;
                    span.end.filePath = sourceFilePath;
                    console.reportError(
                        Omniscript::Console::SYNTAX_ERROR,
                        "Unterminated escape sequence",
                        "To resolve this:\n1. Complete the escape sequence\n2. Check for valid escape characters\n3. Ensure proper string termination",
                        span
                    );
                    return finalizeToken(TokenTypes::Invalid);
                }

                size_t nextPos = currentPosition + 1;
                bool isBackslashAtEndOfLine = false;
                
                while (nextPos < source.length()) {
                    char nextChar = source[nextPos];
                    if (nextChar == ' ' || nextChar == '\t') {
                        nextPos++;
                    } else if (nextChar == '\n' || nextChar == '\r') {
                        isBackslashAtEndOfLine = true;
                        break;
                    } else {
                        break;
                    }
                }

                if (isBackslashAtEndOfLine) {
                    currentPosition = nextPos;
                    if (currentChar == '\r' && currentPosition < source.length() && source[currentPosition] == '\n') {
                        currentPosition++;
                    }
                    line++;
                    column = 1;
                    continue;
                }

                char next = source[currentPosition + 1];
                currentPosition += 2;
                column += 2;

                if (isTemplate && (next == '`' || next == '$' || next == '{')) {
                    literalValue += next;
                    hasContent = true;
                    continue;
                }

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
                    case '\"': literalValue += '\"'; break;
                    case '?': literalValue += '\?'; break;
                    case '0': case '1': case '2': case '3':
                    case '4': case '5': case '6': case '7': {
                        int val = next - '0';
                        for (int i = 0; i < 2 && currentPosition < source.length(); ++i) {
                            char c = source[currentPosition];
                            if (c >= '0' && c <= '7') {
                                val = val * 8 + (c - '0');
                                ++currentPosition;
                                ++column;
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
                        while (currentPosition < source.length()) {
                            char c = source[currentPosition];
                            if (isdigit(c) || (tolower(c) >= 'a' && tolower(c) <= 'f')) {
                                val = val * 16 + (isdigit(c) ? (c - '0') : (tolower(c) - 'a' + 10));
                                ++currentPosition;
                                ++column;
                                ++digits;
                            } else {
                                break;
                            }
                        }
                        if (digits == 0) {
                            span.end.line = line;
                            span.end.col = column;
                            span.end.filePath = sourceFilePath;
                            console.reportError(
                                Omniscript::Console::SYNTAX_ERROR,
                                "Invalid hex escape sequence (\\x)",
                                "To resolve this:\n1. Provide valid hex digits after \\x\n2. Check escape sequence syntax\n3. Ensure at least one hex digit",
                                span
                            );
                            return finalizeToken(TokenTypes::Invalid);
                        }
                        literalValue += static_cast<char>(val);
                        break;
                    }
                    case 'u': case 'U': {
                        int needed = (next == 'u' ? 4 : 8);
                        if (currentPosition + needed > source.length()) {
                            span.end.line = line;
                            span.end.col = column;
                            span.end.filePath = sourceFilePath;
                            console.reportError(
                                Omniscript::Console::SYNTAX_ERROR,
                                "Invalid Unicode escape sequence",
                                "To resolve this:\n1. Provide correct number of hex digits for \\u (4) or \\U (8)\n2. Check Unicode escape syntax\n3. Ensure sufficient characters",
                                span
                            );
                            return finalizeToken(TokenTypes::Invalid);
                        }
                        unsigned int codepoint = 0;
                        for (int i = 0; i < needed; ++i) {
                            char c = source[currentPosition++];
                            ++column;
                            if (isdigit(c) || (tolower(c) >= 'a' && tolower(c) <= 'f')) {
                                codepoint = codepoint * 16 +
                                    (isdigit(c) ? (c - '0') : (tolower(c) - 'a' + 10));
                            } else {
                                span.end.line = line;
                                span.end.col = column;
                                span.end.filePath = sourceFilePath;
                                console.reportError(
                                    Omniscript::Console::SYNTAX_ERROR,
                                    Omniscript::Console::formatString("Invalid Unicode digit in \\%c at position %d", next, i + 1),
                                    "To resolve this:\n1. Use valid hex digits in Unicode escape\n2. Check Unicode escape syntax\n3. Ensure all digits are valid",
                                    span
                                );
                                return finalizeToken(TokenTypes::Invalid);
                            }
                        }
                        if (codepoint <= 0x7F) {
                            literalValue += static_cast<char>(codepoint);
                        } else if (codepoint <= 0x7FF) {
                            literalValue += static_cast<char>(0xC0 | ((codepoint >> 6) & 0x1F));
                            literalValue += static_cast<char>(0x80 | (codepoint & 0x3F));
                        } else if (codepoint <= 0xFFFF) {
                            literalValue += static_cast<char>(0xE0 | ((codepoint >> 12) & 0x0F));
                            literalValue += static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
                            literalValue += static_cast<char>(0x80 | (codepoint & 0x3F));
                        } else {
                            literalValue += static_cast<char>(0xF0 | ((codepoint >> 18) & 0x07));
                            literalValue += static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F));
                            literalValue += static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
                            literalValue += static_cast<char>(0x80 | (codepoint & 0x3F));
                        }
                        break;
                    }
                    default:
                        span.end.line = line;
                        span.end.col = column;
                        span.end.filePath = sourceFilePath;
                        console.reportError(
                            Omniscript::Console::SYNTAX_ERROR,
                            Omniscript::Console::formatString("Unknown escape sequence \\%c", next),
                            "To resolve this:\n1. Use valid escape characters (e.g., \\n, \\t, \\uXXXX)\n2. Check escape sequence syntax\n3. Remove invalid escapes",
                            span
                        );
                        literalValue += '\\';
                        literalValue += next;
                        break;
                }
                hasContent = true;
            } else {
                char c = source[currentPosition];
                literalValue += c;
                hasContent = true;
                currentPosition++;
                if (c == '\n' || c == '\r') {
                    line++;
                    column = 1;
                    if (c == '\r' && currentPosition < source.length() && source[currentPosition] == '\n') {
                        currentPosition++;
                    }
                } else {
                    column++;
                }
            }
        }
        
        span.end.line = line;
        span.end.col = column;
        span.end.filePath = sourceFilePath;
        console.reportError(
            Omniscript::Console::SYNTAX_ERROR,
            Omniscript::Console::formatString("Unterminated %s literal at line %zu", 
                (quoteType == '\'' ? "character" : (quoteType == '`' ? "template" : "string")), startLine),
            "To resolve this:\n1. Close the literal with matching quote\n2. Check for unterminated strings\n3. Ensure proper string termination",
            span
        );
        return finalizeToken(TokenTypes::Invalid);
    }

    span.end.line = line;
    span.end.col = column;
    span.end.filePath = sourceFilePath;
    return Token(TokenTypes::Invalid, "", line, column, sourceFilePath);
}