#include <omniscript/Lexer.h>
#include <omniscript/Tokens.h>
#include <omniscript/Console.h>
#include <stdexcept>
#include <cctype>
#include <string>

namespace Omniscript {

namespace {

// Parses digits (no prefix) in the given base as an unsigned 64-bit value and
// reinterprets it as int64_t, so 0xFFFFFFFFFFFFFFFF is accepted (as -1).
// Throws std::exception on empty input, bad digits or overflow past 64 bits.
int64_t parseRadixUnsigned(const std::string& digits, int base) {
    size_t pos = 0;
    unsigned long long value = std::stoull(digits, &pos, base);
    if (pos != digits.size()) {
        throw std::invalid_argument("trailing characters in numeric literal");
    }
    return static_cast<int64_t>(value);
}

bool isIntegerSuffix(const std::string& s) {
    return s.empty() || s == "u" || s == "l" || s == "ul" || s == "lu" || s == "ll" || s == "ull";
}

bool isFloatSuffix(const std::string& s) {
    return s == "f" || s == "d";
}

} // namespace

Token Lexer::getNumberLiterals(char currentChar) {
    size_t startLine = line_;
    size_t startColumn = column_;

    // The caller only dispatches here on a digit or on '.' followed by a digit,
    // so a leading '-' is never part of the literal (n-1 lexes as n, -, 1).

    // Prefixed literals: 0b..., 0o..., 0x...
    if (currentChar == '0') {
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
        // "0n" is BigInt zero
        if (config_.allowBigInts && next == 'n' && !isIdentifierContinuation(peek(2))) {
            return parseBigIntLiteral();
        }
    }

    std::string numberValue;
    bool isFloat = false;

    if (currentChar == '.') {
        // .5 -> 0.5
        isFloat = true;
        numberValue = "0.";
        advance();
        while (!isAtEnd() && isDigit(getCurrentChar())) {
            numberValue += getCurrentChar();
            advance();
        }
    } else {
        while (!isAtEnd() && isDigit(getCurrentChar())) {
            numberValue += getCurrentChar();
            advance();
        }

        // BigInt: 123n (not 123name)
        if (!isAtEnd() && config_.allowBigInts && getCurrentChar() == 'n' &&
            !isIdentifierContinuation(peek())) {
            advance();
            stats_.literalCount++;
            return Token(TokenTypes::BigInt, numberValue, startLine, startColumn, sourceFilePath_);
        }

        // Fractional part: 1.5 and 1. (but not 1..5, 1.foo or 1._x)
        if (!isAtEnd() && getCurrentChar() == '.') {
            char after = peek();
            bool digitFollows = isDigit(after);
            bool plainTrailingDot = after != '.' && !isIdentifierStart(after);
            if (digitFollows || plainTrailingDot) {
                isFloat = true;
                numberValue += '.';
                advance();
                if (digitFollows) {
                    while (!isAtEnd() && isDigit(getCurrentChar())) {
                        numberValue += getCurrentChar();
                        advance();
                    }
                } else {
                    numberValue += '0';
                }
            }
        }
    }

    // Exponent: 1e5, 1.5E-3. Only taken when a digit really follows,
    // so "1else" is not misread.
    if (!isAtEnd() && (getCurrentChar() == 'e' || getCurrentChar() == 'E')) {
        char n1 = peek();
        bool signedExp = (n1 == '+' || n1 == '-') && isDigit(peek(2));
        if (isDigit(n1) || signedExp) {
            isFloat = true;
            numberValue += getCurrentChar();
            advance();
            if (signedExp) {
                numberValue += getCurrentChar();
                advance();
            }
            while (!isAtEnd() && isDigit(getCurrentChar())) {
                numberValue += getCurrentChar();
                advance();
            }
        }
    }

    // Suffix: take the whole identifier-like run so garbage such as "12abc"
    // is reported once instead of lexing as 12 followed by abc.
    std::string suffix;
    while (!isAtEnd() && isIdentifierContinuation(getCurrentChar())) {
        char c = getCurrentChar();
        suffix += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        advance();
    }

    if (!suffix.empty()) {
        bool floatSuffix = isFloatSuffix(suffix);
        bool ok = isFloat ? (floatSuffix || suffix == "l") : (floatSuffix || isIntegerSuffix(suffix));
        if (!ok) {
            console.reportError(Console::ErrorType::SYNTAX_ERROR,
                "Invalid suffix '" + suffix + "' on number literal: " + numberValue,
                FileSpan{startLine, startColumn, line_, column_, sourceFilePath_});
            return TokenFactory::createInvalid(numberValue + suffix, startLine, startColumn, sourceFilePath_);
        }
        if (floatSuffix) {
            isFloat = true;
        }
    }

    try {
        if (isFloat) {
            double value = LexerUtils::parseFloat(numberValue);
            stats_.literalCount++;
            return TokenFactory::createFloat(value, startLine, startColumn, sourceFilePath_);
        }
        // Use the same unsigned parser the prefixed literals use. `stoll` would
        // reject anything above INT64_MAX, but a u64 language must accept the
        // full uint64 range (e.g. FNV offset basis 14695981039346656037).
        // parseRadixUnsigned wraps the u64 into int64_t via two's complement;
        // the type checker reinterprets back to u64 when the target type is unsigned.
        int64_t value = parseRadixUnsigned(numberValue, 10);
        stats_.literalCount++;
        return TokenFactory::createInteger(value, startLine, startColumn, sourceFilePath_);
    } catch (const std::exception&) {
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
        int64_t value = parseRadixUnsigned(numberValue, 2);
        stats_.literalCount++;
        return Token(TokenTypes::BinaryLiteral, value, startLine, startColumn, sourceFilePath_);
    } catch (const std::exception&) {
        console.reportError(Console::ErrorType::SYNTAX_ERROR,
            "Binary literal out of range: 0b" + numberValue,
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
        int64_t value = parseRadixUnsigned(numberValue, 8);
        stats_.literalCount++;
        return Token(TokenTypes::OctalLiteral, value, startLine, startColumn, sourceFilePath_);
    } catch (const std::exception&) {
        console.reportError(Console::ErrorType::SYNTAX_ERROR,
            "Octal literal out of range: 0o" + numberValue,
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
        int64_t value = parseRadixUnsigned(numberValue, 16);
        stats_.literalCount++;
        return Token(TokenTypes::HexLiteral, value, startLine, startColumn, sourceFilePath_);
    } catch (const std::exception&) {
        console.reportError(Console::ErrorType::SYNTAX_ERROR,
            "Hex literal out of range: 0x" + numberValue,
            FileSpan{startLine, startColumn, line_, column_, sourceFilePath_});
        return TokenFactory::createInvalid("0x" + numberValue, startLine, startColumn, sourceFilePath_);
    }
}

// Called for the literal "0n" (BigInt zero). Other BigInts (123n) are handled
// in getNumberLiterals.
Token Lexer::parseBigIntLiteral() {
    size_t startLine = line_;
    size_t startColumn = column_;
    advance(2); // Skip 0n
    stats_.literalCount++;
    return Token(TokenTypes::BigInt, std::string("0"), startLine, startColumn, sourceFilePath_);
}

} // namespace Omniscript