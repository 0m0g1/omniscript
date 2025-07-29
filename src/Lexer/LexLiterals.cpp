#include <omniscript/omniscript_pch.h>
#include <omniscript/Lexer.h>
#include <omniscript/Tokens.h>
#include <omniscript/utils.h>
#include <omniscript/Core.h>

Token Lexer::getNextToken() {
    Omniscript::FileSpan span;
    span.start.line = line;
    span.start.col = column;
    span.start.filePath = sourceFilePath;

    // Skip white spaces and track lines/columns
    while (currentPosition < source.length() && std::isspace(source[currentPosition])) {
        if (source[currentPosition] == '\n') {
            line++;
            column = 1;
            currentPosition++;
            span.end.line = line;
            span.end.col = column;
            span.end.filePath = sourceFilePath;
            return Token(TokenTypes::Newline, "", line, column, sourceFilePath);
        } else {
            column++;
            currentPosition++;
        }
    }

    // Check if we are at the end of the file
    if (currentPosition >= source.length()) {
        span.end.line = line;
        span.end.col = column;
        span.end.filePath = sourceFilePath;
        return Token(TokenTypes::EOI, "", line, column, sourceFilePath);
    }

    char currentChar = source[currentPosition];

    // Handle single-line comments (//)
    if (currentChar == '/' && peek() == '/') {
        currentPosition += 2; // Skip //
        column += 2;
        while (currentPosition < source.length() && source[currentPosition] != '\n') {
            currentPosition++;
            column++;
        }
        span.end.line = line;
        span.end.col = column;
        span.end.filePath = sourceFilePath;
        return getNextToken(); // Recursive call to get the next token
    }

    // Handle multi-line comments (/* */), including nested comments
    if (currentChar == '/' && peek() == '*') {
        int nestingDepth = 1;
        currentPosition += 2; // Skip the initial /*
        column += 2;

        while (nestingDepth > 0 && currentPosition < source.length()) {
            if (source[currentPosition] == '/' && peek() == '*') {
                nestingDepth++;
                currentPosition += 2;
                column += 2;
            } else if (source[currentPosition] == '*' && peek() == '/') {
                nestingDepth--;
                currentPosition += 2;
                column += 2;
            } else {
                if (source[currentPosition] == '\n') {
                    line++;
                    column = 1;
                    if (currentPosition + 1 < source.length() && source[currentPosition] == '\r' && source[currentPosition + 1] == '\n') {
                        currentPosition++;
                    }
                } else {
                    column++;
                }
                currentPosition++;
            }

            if (nestingDepth > 0 && currentPosition >= source.length()) {
                span.end.line = line;
                span.end.col = column;
                span.end.filePath = sourceFilePath;
                console.reportError(
                    Omniscript::Console::SYNTAX_ERROR,
                    "Unclosed multi-line comment",
                    "To resolve this:\n1. Close the comment with '*/'\n2. Check for unterminated comments\n3. Ensure all comments are properly closed",
                    span
                );
                return Token(TokenTypes::Invalid, "", line, column, sourceFilePath);
            }
        }

        span.end.line = line;
        span.end.col = column;
        span.end.filePath = sourceFilePath;
        return getNextToken(); // Recursive call to get the next token
    }

    currentChar = source[currentPosition];

    bool isRawStringPrefix = currentChar == 'r' &&
                            (peek() == '\'' || peek() == '\"' || peek() == '`');

    // Check for identifiers and keywords
    if ((std::isalpha(currentChar) || currentChar == '_') && !isRawStringPrefix) {
        std::string raw_identifier;
        size_t startColumn = column;

        while (currentPosition < source.length() &&
               (std::isalpha(source[currentPosition]) ||
                std::isdigit(source[currentPosition]) ||
                source[currentPosition] == '_') && 
               source[currentPosition] != '.') {
            raw_identifier += source[currentPosition];
            currentPosition++;
            column++;
        }

        std::string identifier = toLowerCaseString(raw_identifier);

        // Special case for "else if"
        if (identifier == "else") {
            if (peek() == 'i' && peek(2) == 'f') {
                currentPosition += 3;
                column += 3;
                span.end.line = line;
                span.end.col = column;
                span.end.filePath = sourceFilePath;
                return Token(TokenTypes::Else_if, "else if", line, startColumn, sourceFilePath);
            }
            span.end.line = line;
            span.end.col = column;
            span.end.filePath = sourceFilePath;
            return Token(TokenTypes::Else, "else", line, startColumn, sourceFilePath);
        }

        if (identifier == "type" && peekToken(1).getType() == TokenTypes::Identifier) {
            span.end.line = line;
            span.end.col = column;
            span.end.filePath = sourceFilePath;
            return Token(TokenTypes::Type, "type", line, startColumn, sourceFilePath);
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
            {"var", TokenTypes::Let},
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

        auto it = keywordMap.find(identifier);
        span.end.line = line;
        span.end.col = column;
        span.end.filePath = sourceFilePath;
        if (it != keywordMap.end()) {
            return Token(it->second, identifier, line, startColumn, sourceFilePath);
        }

        return Token(TokenTypes::Identifier, raw_identifier, line, startColumn, sourceFilePath);
    }

    // Check for string and char literals
    Token stringToken = getStringToken(currentChar);
    if (stringToken.getType() != TokenTypes::Invalid) {
        // Update line and column from stringToken
        line = stringToken.getLine();
        column = stringToken.getColumn();
        return stringToken;
    }

    // Check for number literals
    Token numberLiteral = getNumberLiterals(currentChar);
    if (numberLiteral.getType() != TokenTypes::Invalid) {
        // Update line and column from numberLiteral
        line = numberLiteral.getLine();
        column = numberLiteral.getColumn();
        return numberLiteral;
    }

    // Check for operators
    Token operatorToken = getOperator(currentChar);
    if (operatorToken.getType() != TokenTypes::Invalid) {
        // Update line and column from operatorToken
        line = operatorToken.getLine();
        column = operatorToken.getColumn();
        return operatorToken;
    }

    // Handle unexpected character
    currentPosition++;
    column++;
    span.end.line = line;
    span.end.col = column;
    span.end.filePath = sourceFilePath;
    console.reportError(
        Omniscript::Console::SYNTAX_ERROR,
        Omniscript::Console::formatString("Unexpected character '%c' at line %zu, column %zu",
            currentChar, line, column),
        "To resolve this:\n1. Check for valid tokens\n2. Ensure correct syntax\n3. Remove or correct unexpected characters",
        span
    );
    return Token(TokenTypes::Invalid, std::string(1, currentChar), line, column, sourceFilePath);
}