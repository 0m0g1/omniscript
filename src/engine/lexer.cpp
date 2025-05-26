//Lexer functionality

// #include <cstring>
// #include <cctype> //for std::isdigit std::isspace etc std::toLower
// #include <string>
// #include <iostream>

#include <omniscript/omniscript_pch.h>
#include <omniscript/engine/lexer.h>
#include <omniscript/engine/tokens.h>
#include <omniscript/utils.h>

// Lexer::Lexer(const std::string &source) : source(source) {}

char Lexer::peek(int n) const {
    if (currentPosition < source.length()) {
        return source[currentPosition + n];
    }
    return '\0';
}

Token Lexer::peekToken(int n) {
    size_t currentPositionHolder = currentPosition;
    int count = 0;
    Token nextToken;
    while (count < n) {
        nextToken = getNextToken();
        count++;
    }
    currentPosition = currentPositionHolder;
    return nextToken;
}

Token Lexer::getNextToken() {
    // Skip white spaces and track lines/columns
    while (currentPosition < source.length() && std::isspace(source[currentPosition])) {
        if (source[currentPosition] == '\n') {
            line++;
            column = 1; // Reset column for new line
            // Check if there were any non-space characters before the newline
            if (column > 1) {
                currentPosition++;
                return Token(TokenTypes::Newline, "", line, column, sourceFilePath);
            }
        } else {
            column++; // Increment column for every space
        }
        currentPosition++;
    }

    // Check if we are at the end of the file
    if (currentPosition >= source.length()) {
        return Token(TokenTypes::EOI, "", line, column, sourceFilePath);
    }

    char currentChar = source[currentPosition];

     // Handle single-line comments (//)
    if (currentChar == '/' && peek() == '/') {
        while (currentPosition < source.length() && source[currentPosition] != '\n') {
            currentPosition++;  // Skip the characters in the comment
            column++;
        }
        return getNextToken();  // Continue to the next token after skipping the comment
    }

    // Handle multi-line comments (/* */), including nested comments
    if (currentChar == '/' && peek() == '*') {
        int nestingDepth = 1;
        currentPosition += 2; // Skip the initial /*

        while (nestingDepth > 0 && currentPosition < source.length()) {
            if (source[currentPosition] == '/' && peek() == '*') {
                nestingDepth++;
                currentPosition += 2;
            } else if (source[currentPosition] == '*' && peek() == '/') {
                nestingDepth--;
                currentPosition += 2;
            } else {
                if (source[currentPosition] == '\n') {
                    line++;
                    column = 1;
                } else {
                    column++;
                }
                currentPosition++;
            }

            // Error if end of file reached without closing the comment
            if (nestingDepth > 0 && currentPosition >= source.length()) {
                throw std::runtime_error("Error: Unclosed multi-line comment at line " + std::to_string(line));
            }
        }
        
        return getNextToken();  // Continue to the next token after skipping the comment
    }

    currentChar = source[currentPosition];

    // Check for identifiers and keywords
    if (std::isalpha(currentChar) || currentChar == '_') { // If the current character is an alphabetical [A-Z][a-z]
        std::string raw_identifier; // Identifier as given in the input could be in any case

        // Check if an identifier's character is a letter, number, underscore and it is not a fullstop
        // Full stops will be the beginning of a method call or a reference to a property
        while (currentPosition < source.length() &&
                (std::isalpha(source[currentPosition]) ||
                std::isdigit(source[currentPosition]) ||
                source[currentPosition] == '_') && (source[currentPosition] != '.')) {
            raw_identifier += source[currentPosition];
            currentPosition++;
            column++; // Increment column for each character in identifier
        }

        std::string identifier = toLowerCaseString(raw_identifier); // Convert to lowercase

        // Return the corresponding token for keywords
        if (identifier == "if") {
            return Token(TokenTypes::If, "", line, column, sourceFilePath);
        } else if (identifier == "else") {
            if (peek() == 'i' && peek(2) == 'f') {
                currentPosition += 3; // Skip "i" and "f"
                column += 3; // Increment column for "if"
                // std::cout << "The current value is '" << source[currentPosition] << "'" << std::endl;
                return Token(TokenTypes::Else_if, "", line, column, sourceFilePath);
            }
            return Token(TokenTypes::Else, "", line, column, sourceFilePath);
        } else if (identifier == "while") {
            return Token(TokenTypes::While, "", line, column, sourceFilePath);
        } else if (identifier == "for") {
            return Token(TokenTypes::For, "", line, column, sourceFilePath);
        } else if (identifier == "continue") {
            return Token(TokenTypes::Continue, "", line, column, sourceFilePath);
        } else if (identifier == "break") {
            return Token(TokenTypes::Break, "", line, column, sourceFilePath);
        } else if (identifier == "return") {
            return Token(TokenTypes::Return, "", line, column, sourceFilePath);
        } else if (identifier == "function" || identifier == "fn") {
            return Token(TokenTypes::Function, "", line, column, sourceFilePath);
        } else if (identifier == "let" || identifier == "var") {
            return Token(TokenTypes::Let, "", line, column, sourceFilePath);
        } else if (identifier == "var") {
            return Token(TokenTypes::Var, "", line, column, sourceFilePath);
        } else if (identifier == "namespace") {
            return Token(TokenTypes::Namespace, "", line, column, sourceFilePath);
        } else if (identifier == "using") {
            return Token(TokenTypes::Using, "", line, column, sourceFilePath);
        } else if (identifier == "new") {
            return Token(TokenTypes::New, "", line, column, sourceFilePath);
        } else if (identifier == "delete") {
            return Token(TokenTypes::New, "", line, column, sourceFilePath);
        } else if (identifier == "struct") {
            return Token(TokenTypes::Struct, "", line, column, sourceFilePath);
        } else if (identifier == "class") {
            return Token(TokenTypes::Class, "", line, column, sourceFilePath);
        } else if (identifier == "extends") {
            return Token(TokenTypes::Extends, "", line, column, sourceFilePath);
        }  else if (identifier == "variant") {
            return Token(TokenTypes::Variant, "", line, column, sourceFilePath);
        }  else if (identifier == "any") {
            return Token(TokenTypes::Any, "", line, column, sourceFilePath);
        } else if (identifier == "enum") {
            return Token(TokenTypes::Enum, "", line, column, sourceFilePath);
        } else if (identifier == "public") {
            return Token(TokenTypes::Public, "", line, column, sourceFilePath);
        } else if (identifier == "private") {
            return Token(TokenTypes::Private, "", line, column, sourceFilePath);
        } else if (identifier == "override") {
            return Token(TokenTypes::Override, "", line, column, sourceFilePath);
        } else if (identifier == "virtual") {
            return Token(TokenTypes::Virtual, "", line, column, sourceFilePath);
        } else if (identifier == "static") {
            return Token(TokenTypes::Static, "", line, column, sourceFilePath);
        } else if (identifier == "final") {
            return Token(TokenTypes::Final, "", line, column, sourceFilePath);
        } else if (identifier == "const") {
            return Token(TokenTypes::Const, "", line, column, sourceFilePath);
        } else if (identifier == "true") {
            return Token(TokenTypes::True, "", line, column, sourceFilePath);
        } else if (identifier == "false") {
            return Token(TokenTypes::False, "", line, column, sourceFilePath);
        } else if (identifier == "nullptr") {
            return Token(TokenTypes::Nullptr, "", line, column, sourceFilePath);
        } else if (identifier == "null" || identifier == "nullptr") {
            return Token(TokenTypes::Null, "", line, column, sourceFilePath);
        } else if (identifier == "xor") {
            return Token(TokenTypes::LogicalXor, "", line, column, sourceFilePath);
        }  else if (identifier == "include") {
            return Token(TokenTypes::Include, "", line, column, sourceFilePath);
        } else if (identifier == "import") {
            return Token(TokenTypes::Import, "", line, column, sourceFilePath);
        } else if (identifier == "from") {
            return Token(TokenTypes::From, "", line, column, sourceFilePath);
        } else if (identifier == "module" || identifier == "mod") {
            return Token(TokenTypes::Module, "", line, column, sourceFilePath);
        } else if (identifier == "extern") {
            return Token(TokenTypes::Extern, "", line, column, sourceFilePath);
        } else if (identifier == "intrinsic") {
            return Token(TokenTypes::Intrinsic, "", line, column, sourceFilePath);
        } else if (identifier == "as") {
            return Token(TokenTypes::As, "", line, column, sourceFilePath);
        }

        // Otherwise treat it as an identifier token
        return Token(TokenTypes::Identifier, raw_identifier, line, column, sourceFilePath);
    }

    // Check for string and char literals
    Token stringToken = getStringToken(currentChar);
    if (stringToken.getType() != TokenTypes::Invalid) {
        return stringToken;
    }

    Token numberLiteral = getNumberLiterals(currentChar);
    if (numberLiteral.getType() != TokenTypes::Invalid) {
        return numberLiteral;
    }

    return getOperator(currentChar);
}

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
    
        bool isTemplate = (quoteType == '`');
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
                        if (u32chars.size() != 1) {
                            throw std::runtime_error("Invalid character literal at line " + std::to_string(line));
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
                continue;
            }

            // Check for embedded expressions in templates
            if (isTemplate && source[currentPosition] == '$' && (currentPosition + 1) < source.length() && source[currentPosition + 1] == '{') {
                currentPosition += 2;
                column += 2;
                return finalizeToken(hasContent ? TokenTypes::TemplateMiddle : TokenTypes::TemplateHead);
            }

            // Handle escape sequences
            if (source[currentPosition] == '\\') {
                if (currentPosition + 1 >= source.length()) {
                    throw std::runtime_error(
                        "Unterminated escape sequence at line " + std::to_string(line));
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
                    throw std::runtime_error(
                    "Unknown escape sequence \\" + std::string(1,next)
                    + " at line " + std::to_string(line));
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

Token Lexer::getNumberLiterals(char &currentChar) {
    if (std::isdigit(currentChar) || (currentChar == '.' && std::isdigit(peek())) ||
        (currentChar == '0' && (peek() == 'x' || peek() == 'o' || peek() == 'b'))) {

        std::string numberValue;
        bool isFloat = false;
        bool hasDecimalPoint = false;
        bool hasExponent = false;
        bool isBigInt = false;
        TokenTypes tokenType = TokenTypes::IntegerLiteral;

        // Handle hex, octal, and binary literals
        if (currentChar == '0' && (peek() == 'x' || peek() == 'o' || peek() == 'b')) {
            char baseIndicator = peek();
            numberValue += currentChar;
            numberValue += baseIndicator;
            currentPosition += 2;
            column += 2;

            if (baseIndicator == 'x') {
                while (currentPosition < source.length() && std::isxdigit(source[currentPosition])) {
                    numberValue += source[currentPosition];
                    currentPosition++;
                    column++;
                }
                tokenType = TokenTypes::HexLiteral;
            } else if (baseIndicator == 'o') {
                while (currentPosition < source.length() && source[currentPosition] >= '0' && source[currentPosition] <= '7') {
                    numberValue += source[currentPosition];
                    currentPosition++;
                    column++;
                }
                tokenType = TokenTypes::OctalLiteral;
            } else if (baseIndicator == 'b') {
                while (currentPosition < source.length() && (source[currentPosition] == '0' || source[currentPosition] == '1')) {
                    numberValue += source[currentPosition];
                    currentPosition++;
                    column++;
                }
                tokenType = TokenTypes::BinaryLiteral;
            }
        } else {
            // Handle normal numbers (integers & floats)
            if (currentChar == '.') { // ".5" should be "0.5"
                isFloat = true;
                hasDecimalPoint = true;
                numberValue = "0";
                numberValue += '.';
                currentPosition++;
                column++;
            }

            while (currentPosition < source.length() && std::isdigit(source[currentPosition])) {
                numberValue += source[currentPosition];
                currentPosition++;
                column++;
            }

            // Check if the number is a BigInt (must be an integer, no decimals)
            if (source[currentPosition] == 'n') {
                isBigInt = true;
                numberValue += 'n';
                currentPosition++;
                column++;
                return Token(TokenTypes::BigInt, numberValue, line, column, sourceFilePath);
            }

            // Prevent accidental method calls (e.g., "123.toString()")
            if (source[currentPosition] == '.' && (std::isalpha(peek()) || peek() == '_')) {
                return Token(TokenTypes::IntegerLiteral, numberValue, line, column, sourceFilePath);
            }

            // Handle decimal point and floating-point numbers
            if (currentPosition < source.length() && source[currentPosition] == '.' && !hasDecimalPoint) {
                hasDecimalPoint = true;
                isFloat = true;
                numberValue += '.';
                currentPosition++;
                column++;

                if (currentPosition < source.length() && std::isdigit(source[currentPosition])) {
                    while (currentPosition < source.length() && std::isdigit(source[currentPosition])) {
                        numberValue += source[currentPosition];
                        currentPosition++;
                        column++;
                    }
                } else {
                    numberValue += '0'; // Ensure valid float format
                }
            }

            // Handle scientific notation (e.g., "1.23e4", "5e-6")
            if (isFloat && currentPosition < source.length() && (source[currentPosition] == 'e' || source[currentPosition] == 'E')) {
                hasExponent = true;
                numberValue += source[currentPosition];
                currentPosition++;
                column++;

                if (currentPosition < source.length() && (source[currentPosition] == '+' || source[currentPosition] == '-')) {
                    numberValue += source[currentPosition];
                    currentPosition++;
                    column++;
                }

                if (currentPosition < source.length() && std::isdigit(source[currentPosition])) {
                    while (currentPosition < source.length() && std::isdigit(source[currentPosition])) {
                        numberValue += source[currentPosition];
                        currentPosition++;
                        column++;
                    }
                } else {
                    std::cerr << "Error: Invalid exponent format at line " << line << ", column " << column << std::endl;
                    return Token(TokenTypes::Invalid, "", line, column, sourceFilePath);
                }
            }
        }

        // Handle suffixes (e.g., 'f' for float, otherwise default to double)
        if (currentPosition < source.length() && source[currentPosition] == 'f') {
            numberValue += source[currentPosition];
            currentPosition++;
            column++;
            return Token(TokenTypes::FloatLiteral, numberValue, line, column, sourceFilePath); // 32-bit float
        }

        return Token(isFloat ? TokenTypes::FloatLiteral : tokenType, numberValue, line, column, sourceFilePath);
    }
    return Token(TokenTypes::Invalid);
}

Token Lexer::getOperator(char &currentChar) {
    // Operator Tokens
    switch (currentChar) {
        case '+':
            if (peek() == '+') {
                currentPosition += 2;
                column += 2;
                return Token(TokenTypes::Increment, "", line, column, sourceFilePath);
            } else if (peek() == '=') {
                currentPosition += 2;
                column += 2;
                return Token(TokenTypes::PlusAssign, "", line, column, sourceFilePath);
            }
            currentPosition++;
            column++;
            return Token(TokenTypes::Plus, "", line, column, sourceFilePath);
        case '-':
            if (peek() == '>') {
                currentPosition += 2;
                column += 2;
                return Token(TokenTypes::Arrow, "", line, column, sourceFilePath);
            }
            if (peek() == '-') {
                if (peek(2) == '>') {
                    currentPosition += 3;
                    column += 3;
                    return Token(TokenTypes::Arrow, "", line, column, sourceFilePath);
                }
                currentPosition += 2;
                column += 2;
                return Token(TokenTypes::Decrement, "", line, column, sourceFilePath);
            }
            if (peek() == '=') {
                currentPosition += 2;
                column += 2;
                return Token(TokenTypes::MinusAssign, "", line, column, sourceFilePath);
            }
            currentPosition++;
            column++;
            return Token(TokenTypes::Minus, "", line, column, sourceFilePath);
        case '/':
            if (peek() == '/') {
                
            } else if (peek() == '=') {
                currentPosition += 2;
                column += 2;
                return Token(TokenTypes::DivideAssign, "", line, column, sourceFilePath);
            }
            currentPosition++;
            column++;
            return Token(TokenTypes::Divide, "", line, column, sourceFilePath);
        case '*':
            if (peek() == '=') {
                currentPosition += 2;
                column += 2;
                return Token(TokenTypes::MultiplyAssign, "", line, column, sourceFilePath);
            }
            currentPosition++;
            column++;
            return Token(TokenTypes::Multiply, "", line, column, sourceFilePath);
        case '%':
            currentPosition++;
            column++;
            return Token(TokenTypes::Modulo, "", line, column, sourceFilePath);
        case '&':
            if (peek() == '&') {
                currentPosition += 2;
                column += 2;
                return Token(TokenTypes::LogicalAnd, "", line, column, sourceFilePath);
            } else if (peek() == '=') {
                currentPosition += 2;
                column += 2;
                return Token(TokenTypes::BitwiseAndAssign, "", line, column, sourceFilePath);
            }
            currentPosition++;
            column++;
            return Token(TokenTypes::BitwiseAnd, "", line, column, sourceFilePath);
        case '|':
            if (peek() == '|') {
                currentPosition += 2;
                column += 2;
                return Token(TokenTypes::LogicalOr, "", line, column, sourceFilePath);
            } else if (peek() == '=') {
                currentPosition += 2;
                column += 2;
                return Token(TokenTypes::BitwiseOrAssign, "", line, column, sourceFilePath);
            }
            currentPosition++;
            column++;
            return Token(TokenTypes::BitwiseOr, "", line, column, sourceFilePath);
        case '^':
            if (peek() == '=') {
                currentPosition += 2;
                column += 2;
                return Token(TokenTypes::BitwiseXorAssign, "", line, column, sourceFilePath);
            }
            currentPosition++;
            column++;
            return Token(TokenTypes::BitwiseXor, "", line, column, sourceFilePath);
        case '~':
            currentPosition++;
            column++;
            return Token(TokenTypes::Tilde, "", line, column, sourceFilePath);
        case '=':
            if (peek() == '=') {
                currentPosition += 2;
                column += 2;
                return Token(TokenTypes::Equals, "", line, column, sourceFilePath);
            } else if (peek() == '>') {
                currentPosition += 2;
                column += 2;
                return Token(TokenTypes::Arrow, "", line, column, sourceFilePath);
            }
            currentPosition++;
            column++;
            return Token(TokenTypes::Assign, "", line, column, sourceFilePath);
        case '(':
            currentPosition++;
            column++;
            return Token(TokenTypes::LeftParen, "", line, column, sourceFilePath);
        case ')':
            currentPosition++;
            column++;
            return Token(TokenTypes::RightParen, "", line, column, sourceFilePath);
        case '{':
            currentPosition++;
            column++;
            return Token(TokenTypes::LeftBrace, "", line, column, sourceFilePath);
        case '}':
            currentPosition++;
            column++;
            return Token(TokenTypes::RightBrace, "", line, column, sourceFilePath);
        case '[':
            currentPosition++;
            column++;
            return Token(TokenTypes::LeftBracket, "", line, column, sourceFilePath);
        case ']':
            currentPosition++;
            column++;
            return Token(TokenTypes::RightBracket, "", line, column, sourceFilePath);
        case ';':
            currentPosition++;
            column++;
            return Token(TokenTypes::Semicolon, "", line, column, sourceFilePath);
        case ',':
            currentPosition++;
            column++;
            return Token(TokenTypes::Comma, "", line, column, sourceFilePath);
        case '.':
            currentPosition++;
            column++;
            return Token(TokenTypes::Dot, "", line, column, sourceFilePath);
        case ':':
            if (peek() == ':') {
                currentPosition += 2;
                column += 2;
                return Token(TokenTypes::ScopeResolution, "", line, column, sourceFilePath);
            }
            currentPosition++;
            column++;
            return Token(TokenTypes::Colon, "", line, column, sourceFilePath);
        case '?':
            currentPosition++;
            column++;
            return Token(TokenTypes::QuestionMark, "", line, column, sourceFilePath);
        case '!':
            if (peek() == '=') {
                currentPosition += 2;
                return Token(TokenTypes::NotEquals);
            }
        case '<':
            if (peek() == '<') {
                if (peek(1) == '=') {
                    currentPosition += 3;
                    column += 3;
                    return Token(TokenTypes::ShiftLeftAssign, "", line, column, sourceFilePath);
                }
                currentPosition += 2;
                column += 2;
                return Token(TokenTypes::ShiftLeft, "", line, column, sourceFilePath);
            } else if (peek() == '=') {
                currentPosition += 2;
                return Token(TokenTypes::LessEqual);
            } 
            currentPosition++;
            return Token(TokenTypes::LessThan);
        case '>':
            if (peek() == '>') {
                if (peek(1) == '=') {
                    currentPosition += 3;
                    column += 3;
                    return Token(TokenTypes::ShiftRightAssign, "", line, column, sourceFilePath);
                }
                currentPosition += 2;
                column += 2;
                return Token(TokenTypes::ShiftRight, "", line, column, sourceFilePath);
            } else if (peek() == '=') {
                currentPosition += 2;
                return Token(TokenTypes::GreaterEqual);
            }
            currentPosition++;
            return Token(TokenTypes::GreaterThan);
        case '\n':
            currentPosition++;
            line++;
            column = 1;
            return getNextToken();
        // Add other cases for additional operators or punctuation as needed

        // If there is no token for the defined character
        default:
            std::cerr << "Error: Unrecognized character '" << currentChar << "' at line " << line << ", column " << column << std::endl;
            currentPosition++;
            column++;
            return Token(TokenTypes::Invalid, "", line, column, sourceFilePath);
    }
}

