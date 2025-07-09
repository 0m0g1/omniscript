
#include <omniscript/omniscript_pch.h>
#include <omniscript/Lexer.h>
#include <omniscript/Tokens.h>
#include <omniscript/utils.h>

Token Lexer::getStringToken(char &currentChar) {
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

    if (currentChar == '\"' || currentChar == '\'' || currentChar == '`') { // Both single and double quotes
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
            return tok;
        };

        while (currentPosition < source.length()) {
            char currentChar = source[currentPosition];

            // Check for closing quote (only if not template with ${)
            if (currentChar == quoteType) {
                // Count preceding backslashes to determine if escaped
                int backslashCount = 0;
                int i = currentPosition - 1;
                while (i >= 0 && source[i] == '\\') {
                    backslashCount++;
                    i--;
                }

                // If even number of backslashes, quote is not escaped
                if (backslashCount % 2 == 0) {
                    currentPosition++; // Skip closing quote
                    column++;
                    
                    if (quoteType == '\'') {
                        // Character literal validation
                        std::u32string u32chars = utf8_to_utf32(literalValue);
                        if (u32chars.size() > 1) {
                            // throw std::runtime_error("Invalid character literal at line " + std::to_string(line));
                            return finalizeToken(TokenTypes::StringLiteral);
                        }
                        return finalizeToken(TokenTypes::Character);
                    }
                    else if (quoteType == '`') {
                        return finalizeToken(TokenTypes::TemplateTail);
                    }
                    else {
                        return finalizeToken(TokenTypes::StringLiteral);
                    }
                }
            }

            // Special handling for raw strings
            if (isRawString) {
                // Raw strings ignore escape sequences
                if (currentChar == quoteType) {
                    currentPosition++;
                    column++;
                    return finalizeToken(isTemplate ? TokenTypes::TemplateTail : 
                                    (quoteType == '\'' ? TokenTypes::Character : TokenTypes::StringLiteral));
                }
                
                // Still need to handle template expressions in raw templates
                if (isTemplate && currentChar == '$' && peek() == '{') {
                    currentPosition += 2;
                    column += 2;
                    return finalizeToken(hasContent ? TokenTypes::TemplateMiddle : TokenTypes::TemplateHead);
                }
                
                // Process character normally (no escape handling)
                literalValue += currentChar;
                hasContent = true;
                currentPosition++;
                continue;
            }

            // Check for embedded expressions in templates
            if (isTemplate && source[currentPosition] == '$' && (currentPosition + 1) < source.length() && source[currentPosition + 1] == '{') {
                currentPosition += 2;
                column += 2;
                return finalizeToken(hasContent ? TokenTypes::TemplateMiddle : TokenTypes::TemplateHead);
            }

           if (currentChar == '\n' || currentChar == '\r') {                
                if (isTemplate) {
                    // For backtick strings: KEEP the newline but skip leading whitespace
                    literalValue += '\n'; // Normalize to LF
                    line++;
                    column = 1;
                    
                    // Skip CRLF if present
                    if (currentChar == '\r' && currentPosition + 1 < source.length() && source[currentPosition + 1] == '\n') {
                        currentPosition++;
                    }
                    currentPosition++; // Skip the newline
                    
                    // Skip leading spaces/tabs after the newline
                    while (currentPosition < source.length()) {
                        char nextChar = source[currentPosition];
                        if (nextChar == ' ' || nextChar == '\t') {
                            currentPosition++;
                            column++;
                        } else {
                            break; // Stop at first non-whitespace character
                        }
                    }
                } else {
                    // For normal strings: REMOVE the newline and all following whitespace
                    currentPosition++;
                    if (currentChar == '\r' && currentPosition < source.length() && source[currentPosition] == '\n') {
                        currentPosition++; // Skip LF in CRLF
                    }
                    line++;
                    column = 1;
                    
                    // Skip all whitespace after newline
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
                continue; // Skip adding newline/whitespace to `literalValue` (except for template newlines)
            }

            // Handle escape sequences
            if (source[currentPosition] == '\\') {
                if (currentPosition + 1 >= source.length()) {
                    throw std::runtime_error(
                        "Unterminated escape sequence at line " + std::to_string(line));
                }

                {
                    // Check if this is the last character before a newline
                    size_t nextPos = currentPosition + 1;
                    bool isBackslashAtEndOfLine = false;
                    
                    // Skip whitespace after the backslash
                    while (nextPos < source.length()) {
                        char nextChar = source[nextPos];
                        if (nextChar == ' ' || nextChar == '\t') {
                            nextPos++; // Skip spaces/tabs
                        }
                        else if (nextChar == '\n' || nextChar == '\r') {
                            isBackslashAtEndOfLine = true;
                            break;
                        }
                        else {
                            break; // Not a newline, treat as normal escape
                        }
                    }

                    if (isBackslashAtEndOfLine) {
                    //     // Literal backslash (no escape behavior)
                    //     literalValue += '\\';
                        currentPosition = nextPos; // Skip to newline
                        continue;
                    }
                }

                char next = source[currentPosition+1];
                currentPosition += 2;
                column += 2;

                // Add template-specific escapes
                if (isTemplate && (next == '`' || next == '$' || next == '{')) {
                    literalValue += next;
                    hasContent = true;
                    continue;
                }

                switch (next) {
                // Simple single‐character escapes
                case 'n':  literalValue += '\n'; break;
                case 't':  literalValue += '\t'; break;
                case 'r':  literalValue += '\r'; break;
                case 'b':  literalValue += '\b'; break;
                case 'f':  literalValue += '\f'; break;
                case 'v':  literalValue += '\v'; break;
                case 'a':  literalValue += '\a'; break;
                case '\\': literalValue += '\\'; break;
                case '\'': literalValue += '\''; break;
                case '\"': literalValue += '\"'; break;
                case '?':  literalValue += '\?'; break;

                // Null / octal: up to three octal digits [0–7]
                case '0': case '1': case '2': case '3':
                case '4': case '5': case '6': case '7': {
                    int val = next - '0';
                    // Read up to 3 octal digits total (first one is already read)
                    // two more octal digits
                    for (int i = 0; i < 2 && currentPosition < source.length(); ++i) {
                    char c = source[currentPosition];
                    if (c >= '0' && c <= '7') {
                        val = val * 8 + (c - '0');
                        ++currentPosition; ++column;
                    } else break;
                    }
                    literalValue += static_cast<char>(val);
                    break;
                }

                // Hex: \xhh… (any number of hex digits, but typically up to 2)
                case 'x': {
                    int val = 0;
                    int digits = 0;
                    while (currentPosition < source.length()) {
                    char c = source[currentPosition];
                    if (isdigit(c) || (tolower(c) >= 'a' && tolower(c) <= 'f')) {
                        val = val * 16 + (isdigit(c) ? (c - '0') : (tolower(c) - 'a' + 10));
                        ++currentPosition; ++column; ++digits;
                    } else break;
                    }
                    if (digits == 0)
                    throw std::runtime_error(
                        "Invalid hex escape (\\x) at line " + std::to_string(line));
                    literalValue += static_cast<char>(val);
                    break;
                }

                // Unicode: \uNNNN (4 hex digits), \UNNNNNNNN (8 hex digits)
                case 'u': case 'U': {
                    int needed = (next == 'u' ? 4 : 8);
                    if (currentPosition + needed > source.length())
                        throw std::runtime_error("Invalid Unicode escape at line " + std::to_string(line));
                    unsigned int codepoint = 0;
                    for (int i = 0; i < needed; ++i) {
                        char c = source[currentPosition++];
                        ++column;
                        if (isdigit(c) || (tolower(c) >= 'a' && tolower(c) <= 'f')) {
                            codepoint = codepoint * 16 +
                            (isdigit(c) ? (c - '0') : (tolower(c) - 'a' + 10));
                        } else {
                            throw std::runtime_error("Invalid Unicode digit in \\u/\\U at line " + std::to_string(line));
                        }
                    }
                    // Now encode codepoint as UTF‑8:
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
                    break;
                    // console.error(
                    // "Unknown escape sequence \\" + std::string(1,next)
                    // + " at line " + std::to_string(line));
                }
            } else {
                char c = source[currentPosition];
                literalValue += c;
                hasContent = true;
                currentPosition++;

                // Update line/column for newlines
                if (c == '\n' || c == '\r') {
                    line++;
                    column = 1;
                    // Handle CRLF as single newline
                    if (c == '\r' && currentPosition < source.length() && source[currentPosition] == '\n') {
                        currentPosition++;
                    }
                } else {
                    column++;
                }
            }
        }
        
        // Check for unterminated literal
        if (currentPosition >= source.length()) {
            std::string typeName;
            if (quoteType == '\'') typeName = "character";
            else if (quoteType == '\"') typeName = "string";
            else typeName = "template";
            throw std::runtime_error("Unterminated " + typeName + " literal at line " + std::to_string(startLine));
        }
    }

    return Token(TokenTypes::Invalid);
} 