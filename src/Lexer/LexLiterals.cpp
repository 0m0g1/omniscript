#include <omniscript/omniscript_pch.h>
#include <omniscript/Lexer.h>
#include <omniscript/Tokens.h>
#include <omniscript/utils.h>

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
        // Special cases first
        if (identifier == "else") {
            if (peek() == 'i' && peek(2) == 'f') {
                currentPosition += 3; // Skip "i" and "f"
                column += 3;
                return Token(TokenTypes::Else_if, "else if", line, column, sourceFilePath);
            }
            return Token(TokenTypes::Else, "else", line, column, sourceFilePath);
        }

        if (identifier == "type" && peekToken(1).getType() == TokenTypes::Identifier) {
            return Token(TokenTypes::Type, "type", line, column, sourceFilePath);
        }

        // Main keyword map
        static const std::unordered_map<std::string, TokenTypes> keywordMap = {
            {"if", TokenTypes::If},
            {"while", TokenTypes::While},
            {"for", TokenTypes::For},
            {"continue", TokenTypes::Continue},
            {"break", TokenTypes::Break},
            {"return", TokenTypes::Return},
            {"function", TokenTypes::Function},
            {"fn", TokenTypes::Function},
            {"let", TokenTypes::Let},
            {"var", TokenTypes::Let}, // Assuming var = let in your language
            {"namespace", TokenTypes::Namespace},
            {"using", TokenTypes::Using},
            {"new", TokenTypes::New},
            {"delete", TokenTypes::Delete},
            {"struct", TokenTypes::Struct},
            {"class", TokenTypes::Class},
            {"extends", TokenTypes::Extends},
            {"variant", TokenTypes::Variant},
            {"any", TokenTypes::Any},
            {"enum", TokenTypes::Enum},
            {"public", TokenTypes::Public},
            {"private", TokenTypes::Private},
            {"override", TokenTypes::Override},
            {"virtual", TokenTypes::Virtual},
            {"static", TokenTypes::Static},
            {"final", TokenTypes::Final},
            {"const", TokenTypes::Const},
            {"true", TokenTypes::True},
            {"false", TokenTypes::False},
            {"nullptr", TokenTypes::Nullptr},
            {"null", TokenTypes::Null},
            {"xor", TokenTypes::LogicalXor},
            {"include", TokenTypes::Include},
            {"import", TokenTypes::Import},
            {"from", TokenTypes::From},
            {"module", TokenTypes::Module},
            {"mod", TokenTypes::Module},
            {"extern", TokenTypes::Extern},
            {"intrinsic", TokenTypes::Intrinsic},
            {"volatile", TokenTypes::Volatile},
            {"as", TokenTypes::As}
        };

        // Lookup
        auto it = keywordMap.find(identifier);
        if (it != keywordMap.end()) {
            return Token(it->second, identifier, line, column, sourceFilePath);
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