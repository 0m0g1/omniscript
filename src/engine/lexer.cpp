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
        } else if (identifier == "function") {
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
        } else if (identifier == "as") {
            return Token(TokenTypes::As, "", line, column, sourceFilePath);
        }

        // Check for function calls
        // if (currentPosition < source.length() && source[currentPosition] == '(') {
        //     currentPosition++;
        //     column++;
        //     return Token(TokenTypes::FunctionCall, identifier, line, column, sourceFilePath);
        // }

        // Otherwise treat it as an identifier token
        return Token(TokenTypes::Identifier, raw_identifier, line, column, sourceFilePath);
    }

    // Check for string and char literals
    if (currentChar == '\"' || currentChar == '\'') { // Both single and double quotes
        char quoteType = currentChar;
        std::string literalValue;
        currentPosition++; // Move past the opening quote
        column++;
        
        bool isString = (quoteType == '\"');
        
        while (currentPosition < source.length() && 
            (source[currentPosition] != quoteType || 
            (source[currentPosition] == quoteType && source[currentPosition-1] == '\\'))) {
            
            // Handle escape sequences
            if (source[currentPosition] == '\\') {
                if (currentPosition + 1 >= source.length()) {
                    throw std::runtime_error("Unterminated escape sequence at line " + std::to_string(line));
                }
                
                // Translate escape sequence
                switch (source[currentPosition+1]) {
                    case 'n': literalValue += '\n'; break;
                    case 't': literalValue += '\t'; break;
                    case 'r': literalValue += '\r'; break;
                    case '\\': literalValue += '\\'; break;
                    case '\"': literalValue += '\"'; break;
                    case '\'': literalValue += '\''; break;
                    default: 
                        throw std::runtime_error("Invalid escape sequence at line " + std::to_string(line));
                }
                currentPosition += 2;
                column += 2;
            } else {
                literalValue += source[currentPosition];
                currentPosition++;
                column++;
            }
        }
        
        // Check for closing quote
        if (currentPosition >= source.length()) {
            throw std::runtime_error("Unterminated " + std::string(isString ? "string" : "character") + 
                                " literal at line " + std::to_string(line));
        }
        
        currentPosition++; // Skip closing quote
        column++;
        
        // Convert literalValue (UTF-8) to UTF-32
        std::u32string u32chars = utf8_to_utf32(literalValue);
        if (!isString) {
            // Character literal - must be exactly one character (after escape processing)

            if (u32chars.size() != 1) {
                throw std::runtime_error("Invalid character literal at line " + std::to_string(line));
            }

            auto tok = Token(TokenTypes::Character, literalValue, line, column, sourceFilePath);
            tok.setU32Value(u32chars);
            return tok;
        } else {
            // String literal
            auto tok = Token(TokenTypes::StringLiteral, literalValue, line, column, sourceFilePath);
            tok.setU32Value(u32chars);
            return tok;
        }
    }

    Token numberLiteral = getNumberLiterals(currentChar);
    if (numberLiteral.getType() != TokenTypes::Invalid) {
        return numberLiteral;
    }

    return getOperator(currentChar);
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

