#include <omniscript/omniscript_pch.h>
<<<<<<< HEAD:src/Lexer/LexNumbers.cpp
#include <omniscript/Lexer.h>
#include <omniscript/Tokens.h>
=======
#include <omniscript/engine/Lexer.h>
#include <omniscript/engine/Tokens.h>
>>>>>>> 7ccebff50dd27e70cffd4d578dcb358f4c9e1613:src/engine/Lexer/LexNumbers.cpp
#include <omniscript/utils.h>

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
        if (currentPosition < source.length() && 
            (
                source[currentPosition] == 'f' || source[currentPosition] == 'F' ||
                source[currentPosition] == 'd' || source[currentPosition] == 'D' ||
                source[currentPosition] == 'l' || source[currentPosition] == 'L'
            )
            ) {
            numberValue += source[currentPosition];
            currentPosition++;
            column++;
            return Token(TokenTypes::FloatLiteral, numberValue, line, column, sourceFilePath); // 32-bit float
        }

        return Token(isFloat ? TokenTypes::FloatLiteral : tokenType, numberValue, line, column, sourceFilePath);
    }
    return Token(TokenTypes::Invalid);
}