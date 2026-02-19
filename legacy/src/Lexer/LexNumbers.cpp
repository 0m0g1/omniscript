#include <omniscript/Lexer.h>
#include <omniscript/Tokens.h>
#include <omniscript/Console.h>
#include <stdexcept>

namespace Omniscript {

Token Lexer::getNumberLiterals(char currentChar) {
    size_t startLine = line_;
    size_t startColumn = column_;
    std::string numberValue;
    bool isFloat = false;
    bool hasDecimalPoint = false;
    bool hasExponent = false;
    bool isBigInt = false;
    TokenTypes tokenType = TokenTypes::IntegerLiteral;

    if (currentChar == '-') {
        numberValue += currentChar;
        advance();
    }

    if (currentChar == '0' && !isAtEnd()) {
        char next = peek();
        if (config_.allowBinaryLiterals && (next == 'b' || next == 'B')) {
            return parseBinaryLiteral();
        }
        if (config_.allowOctalLiterals && (next == 'o' || next == 'O')) {
            return parseOctalLiteral();
        }
        if (config_.allowHexLiterals && (next == 'x' || next == 'X')) {
            return parseHexLiteral();
        }
    }

    if (config_.allowBigInts && peekString(2) == "0n") {
        return parseBigIntLiteral();
    }

    if (currentChar == '.' && isDigit(peek())) {
        isFloat = true;
        hasDecimalPoint = true;
        numberValue = "0";
        numberValue += '.';
        advance();
    }

    while (!isAtEnd() && isDigit(getCurrentChar())) {
        numberValue += getCurrentChar();
        advance();
    }

    if (!isAtEnd() && getCurrentChar() == 'n' && config_.allowBigInts) {
        isBigInt = true;
        numberValue += 'n';
        advance();
        stats_.literalCount++;
        return TokenFactory::createString(numberValue, startLine, startColumn, sourceFilePath_);
    }

    if (!isAtEnd() && getCurrentChar() == '.' && !hasDecimalPoint && !std::isalpha(peek()) && peek() != '_') {
        hasDecimalPoint = true;
        isFloat = true;
        numberValue += '.';
        advance();
        if (!isAtEnd() && isDigit(getCurrentChar())) {
            while (!isAtEnd() && isDigit(getCurrentChar())) {
                numberValue += getCurrentChar();
                advance();
            }
        } else {
            numberValue += '0';
        }
    }

    if (isFloat && !isAtEnd() && (getCurrentChar() == 'e' || getCurrentChar() == 'E')) {
        hasExponent = true;
        numberValue += getCurrentChar();
        advance();
        if (!isAtEnd() && (getCurrentChar() == '+' || getCurrentChar() == '-')) {
            numberValue += getCurrentChar();
            advance();
        }
        if (!isAtEnd() && isDigit(getCurrentChar())) {
            while (!isAtEnd() && isDigit(getCurrentChar())) {
                numberValue += getCurrentChar();
                advance();
            }
        } else {
            console.reportError(Console::ErrorType::SYNTAX_ERROR,
                "Invalid exponent format",
                FileSpan{startLine, startColumn, line_, column_, sourceFilePath_});
            return TokenFactory::createInvalid(numberValue, startLine, startColumn, sourceFilePath_);
        }
    }

    if (!isAtEnd() && (getCurrentChar() == 'f' || getCurrentChar() == 'F' ||
                       getCurrentChar() == 'd' || getCurrentChar() == 'D' ||
                       getCurrentChar() == 'l' || getCurrentChar() == 'L')) {
        numberValue += getCurrentChar();
        advance();
        isFloat = true;
    }

    if (!isValidNumber(numberValue)) {
        console.reportError(Console::ErrorType::SYNTAX_ERROR,
            "Invalid number literal: " + numberValue,
            FileSpan{startLine, startColumn, line_, column_, sourceFilePath_});
        return TokenFactory::createInvalid(numberValue, startLine, startColumn, sourceFilePath_);
    }

    try {
        if (isFloat) {
            double value = LexerUtils::parseFloat(numberValue);
            stats_.literalCount++;
            return TokenFactory::createFloat(value, startLine, startColumn, sourceFilePath_);
        } else {
            int64_t value = LexerUtils::parseInteger(numberValue, 10);
            stats_.literalCount++;
            return TokenFactory::createInteger(value, startLine, startColumn, sourceFilePath_);
        }
    } catch (const std::exception& e) {
        console.reportError(Console::ErrorType::SYNTAX_ERROR,
            "Number literal out of range: " + numberValue,
            FileSpan{startLine, startColumn, line_, column_, sourceFilePath_});
        return TokenFactory::createInvalid(numberValue, startLine, startColumn, sourceFilePath_);
    }
}

Token Lexer::parseBinaryLiteral() {
    size_t startLine = line_;
    size_t startColumn = column_;
    advance(2); // Skip 0b
    std::string numberValue;

    while (!isAtEnd() && isBinaryDigit(getCurrentChar())) {
        numberValue += getCurrentChar();
        advance();
    }

    if (numberValue.empty()) {
        console.reportError(Console::ErrorType::SYNTAX_ERROR,
            "Empty binary literal",
            FileSpan{startLine, startColumn, line_, column_, sourceFilePath_});
        return TokenFactory::createInvalid("0b", startLine, startColumn, sourceFilePath_);
    }

    try {
        int64_t value = LexerUtils::parseInteger(numberValue, 2);
        stats_.literalCount++;
        return Token(TokenTypes::BinaryLiteral, value, startLine, startColumn, sourceFilePath_);
    } catch (const std::exception& e) {
        console.reportError(Console::ErrorType::SYNTAX_ERROR,
            "Invalid binary literal: 0b" + numberValue,
            FileSpan{startLine, startColumn, line_, column_, sourceFilePath_});
        return TokenFactory::createInvalid("0b" + numberValue, startLine, startColumn, sourceFilePath_);
    }
}

Token Lexer::parseOctalLiteral() {
    size_t startLine = line_;
    size_t startColumn = column_;
    advance(2); // Skip 0o
    std::string numberValue;

    while (!isAtEnd() && isOctalDigit(getCurrentChar())) {
        numberValue += getCurrentChar();
        advance();
    }

    if (numberValue.empty()) {
        console.reportError(Console::ErrorType::SYNTAX_ERROR,
            "Empty octal literal",
            FileSpan{startLine, startColumn, line_, column_, sourceFilePath_});
        return TokenFactory::createInvalid("0o", startLine, startColumn, sourceFilePath_);
    }

    try {
        int64_t value = LexerUtils::parseInteger(numberValue, 8);
        stats_.literalCount++;
        return Token(TokenTypes::OctalLiteral, value, startLine, startColumn, sourceFilePath_);
    } catch (const std::exception& e) {
        console.reportError(Console::ErrorType::SYNTAX_ERROR,
            "Invalid octal literal: 0o" + numberValue,
            FileSpan{startLine, startColumn, line_, column_, sourceFilePath_});
        return TokenFactory::createInvalid("0o" + numberValue, startLine, startColumn, sourceFilePath_);
    }
}

Token Lexer::parseHexLiteral() {
    size_t startLine = line_;
    size_t startColumn = column_;
    advance(2); // Skip 0x
    std::string numberValue;

    while (!isAtEnd() && isHexDigit(getCurrentChar())) {
        numberValue += getCurrentChar();
        advance();
    }

    if (numberValue.empty()) {
        console.reportError(Console::ErrorType::SYNTAX_ERROR,
            "Empty hex literal",
            FileSpan{startLine, startColumn, line_, column_, sourceFilePath_});
        return TokenFactory::createInvalid("0x", startLine, startColumn, sourceFilePath_);
    }

    try {
        int64_t value = LexerUtils::parseInteger(numberValue, 16);
        stats_.literalCount++;
        return Token(TokenTypes::HexLiteral, value, startLine, startColumn, sourceFilePath_);
    } catch (const std::exception& e) {
        console.reportError(Console::ErrorType::SYNTAX_ERROR,
            "Invalid hex literal: 0x" + numberValue,
            FileSpan{startLine, startColumn, line_, column_, sourceFilePath_});
        return TokenFactory::createInvalid("0x" + numberValue, startLine, startColumn, sourceFilePath_);
    }
}

Token Lexer::parseBigIntLiteral() {
    size_t startLine = line_;
    size_t startColumn = column_;
    advance(2); // Skip 0n
    std::string numberValue;

    while (!isAtEnd() && isDigit(getCurrentChar())) {
        numberValue += getCurrentChar();
        advance();
    }

    if (numberValue.empty()) {
        console.reportError(Console::ErrorType::SYNTAX_ERROR,
            "Empty BigInt literal",
            FileSpan{startLine, startColumn, line_, column_, sourceFilePath_});
        return TokenFactory::createInvalid("0n", startLine, startColumn, sourceFilePath_);
    }

    stats_.literalCount++;
    return TokenFactory::createString(numberValue + "n", startLine, startColumn, sourceFilePath_);
}

} // namespace Omniscript
