#include <omniscript/omniscript_pch.h>
#include <omniscript/Lexer.h>
#include <omniscript/Tokens.h>
#include <omniscript/utils.h>

Token Lexer::getOperator(char &currentChar) {
    // Operator Tokens
    switch (currentChar) {
        case '+':
            if (peek() == '+') {
                currentPosition += 2;
                column += 2;
                return Token(TokenTypes::Increment, "++", line, column, sourceFilePath);
            } else if (peek() == '=') {
                currentPosition += 2;
                column += 2;
                return Token(TokenTypes::PlusAssign, "+=", line, column, sourceFilePath);
            }
            currentPosition++;
            column++;
            return Token(TokenTypes::Plus, "+", line, column, sourceFilePath);
        case '-':
            if (peek() == '>') {
                currentPosition += 2;
                column += 2;
                return Token(TokenTypes::Arrow, "->", line, column, sourceFilePath);
            }
            if (peek() == '-') {
                if (peek(2) == '>') {
                    currentPosition += 3;
                    column += 3;
                    return Token(TokenTypes::Arrow, "-->", line, column, sourceFilePath);
                }
                currentPosition += 2;
                column += 2;
                return Token(TokenTypes::Decrement, "--", line, column, sourceFilePath);
            }
            if (peek() == '=') {
                currentPosition += 2;
                column += 2;
                return Token(TokenTypes::MinusAssign, "-=", line, column, sourceFilePath);
            }
            currentPosition++;
            column++;
            return Token(TokenTypes::Minus, "-", line, column, sourceFilePath);
        case '/':
            if (peek() == '=') {
                currentPosition += 2;
                column += 2;
                return Token(TokenTypes::DivideAssign, "/=", line, column, sourceFilePath);
            }
            currentPosition++;
            column++;
            return Token(TokenTypes::Divide, "/", line, column, sourceFilePath);
        case '*':
            if (peek() == '=') {
                currentPosition += 2;
                column += 2;
                return Token(TokenTypes::MultiplyAssign, "*=", line, column, sourceFilePath);
            }
            currentPosition++;
            column++;
            return Token(TokenTypes::Multiply, "*", line, column, sourceFilePath);
        case '%':
            currentPosition++;
            column++;
            return Token(TokenTypes::Modulo, "%", line, column, sourceFilePath);
        case '&':
            if (peek() == '&') {
                currentPosition += 2;
                column += 2;
                return Token(TokenTypes::LogicalAnd, "&&", line, column, sourceFilePath);
            } else if (peek() == '=') {
                currentPosition += 2;
                column += 2;
                return Token(TokenTypes::BitwiseAndAssign, "&=", line, column, sourceFilePath);
            }
            currentPosition++;
            column++;
            return Token(TokenTypes::BitwiseAnd, "&", line, column, sourceFilePath);
        case '|':
            if (peek() == '|') {
                currentPosition += 2;
                column += 2;
                return Token(TokenTypes::LogicalOr, "||", line, column, sourceFilePath);
            } else if (peek() == '=') {
                currentPosition += 2;
                column += 2;
                return Token(TokenTypes::BitwiseOrAssign, "|=", line, column, sourceFilePath);
            }
            currentPosition++;
            column++;
            return Token(TokenTypes::BitwiseOr, "|", line, column, sourceFilePath);
        case '^':
            if (peek() == '=') {
                currentPosition += 2;
                column += 2;
                return Token(TokenTypes::BitwiseXorAssign, "^=", line, column, sourceFilePath);
            }
            currentPosition++;
            column++;
            return Token(TokenTypes::BitwiseXor, "^", line, column, sourceFilePath);
        case '~':
            currentPosition++;
            column++;
            return Token(TokenTypes::Tilde, "~", line, column, sourceFilePath);
        case '=':
            if (peek() == '=') {
                currentPosition += 2;
                column += 2;
                return Token(TokenTypes::Equals, "==", line, column, sourceFilePath);
            } else if (peek() == '>') {
                currentPosition += 2;
                column += 2;
                return Token(TokenTypes::Arrow, "=>", line, column, sourceFilePath);
            }
            currentPosition++;
            column++;
            return Token(TokenTypes::Assign, "=", line, column, sourceFilePath);
        case '(':
            currentPosition++;
            column++;
            return Token(TokenTypes::LeftParen, "(", line, column, sourceFilePath);
        case ')':
            currentPosition++;
            column++;
            return Token(TokenTypes::RightParen, ")", line, column, sourceFilePath);
        case '{':
            currentPosition++;
            column++;
            return Token(TokenTypes::LeftBrace, "{", line, column, sourceFilePath);
        case '}':
            currentPosition++;
            column++;
            return Token(TokenTypes::RightBrace, "}", line, column, sourceFilePath);
        case '[':
            currentPosition++;
            column++;
            return Token(TokenTypes::LeftBracket, "[", line, column, sourceFilePath);
        case ']':
            currentPosition++;
            column++;
            return Token(TokenTypes::RightBracket, "]", line, column, sourceFilePath);
        case ';':
            currentPosition++;
            column++;
            return Token(TokenTypes::Semicolon, ";", line, column, sourceFilePath);
        case ',':
            currentPosition++;
            column++;
            return Token(TokenTypes::Comma, ",", line, column, sourceFilePath);
        case '.':
            if (peek() == '.' && peek(2) == '.') {
                currentPosition += 3;
                column += 3;
                return Token(TokenTypes::Ellipsis, "...", line, column, sourceFilePath);
            }
            currentPosition++;
            column++;
            return Token(TokenTypes::Dot, ".", line, column, sourceFilePath);
        case ':':
            if (peek() == ':') {
                currentPosition += 2;
                column += 2;
                return Token(TokenTypes::ScopeResolution, "::", line, column, sourceFilePath);
            }
            currentPosition++;
            column++;
            return Token(TokenTypes::Colon, ":", line, column, sourceFilePath);
        case '?':
            currentPosition++;
            column++;
            return Token(TokenTypes::QuestionMark, "?", line, column, sourceFilePath);
        case '!':
            if (peek() == '=') {
                currentPosition += 2;
                column += 2;
                return Token(TokenTypes::NotEquals, "!=", line, column, sourceFilePath);
            }
            currentPosition += 1;
            column += 1;
            return Token(TokenTypes::LogicalNot, "!", line, column, sourceFilePath);
        case '<':
            if (peek() == '<') {
                if (peek(1) == '=') {
                    currentPosition += 3;
                    column += 3;
                    return Token(TokenTypes::ShiftLeftAssign, "<<=", line, column, sourceFilePath);
                }
                currentPosition += 2;
                column += 2;
                return Token(TokenTypes::ShiftLeft, "<<", line, column, sourceFilePath);
            } else if (peek() == '=') {
                currentPosition += 2;
                return Token(TokenTypes::LessEqual, "<=", line, column, sourceFilePath);
            } 
            currentPosition++;
            return Token(TokenTypes::LessThan, "<", line, column, sourceFilePath);
        case '>':
            if (peek() == '>') {
                if (peek(1) == '=') {
                    currentPosition += 3;
                    column += 3;
                    return Token(TokenTypes::ShiftRightAssign, ">>=", line, column, sourceFilePath);
                }
                currentPosition += 2;
                column += 2;
                return Token(TokenTypes::ShiftRight, ">>", line, column, sourceFilePath);
            } else if (peek() == '=') {
                currentPosition += 2;
                return Token(TokenTypes::GreaterEqual, ">=", line, column, sourceFilePath);
            }
            currentPosition++;
            return Token(TokenTypes::GreaterThan, ">", line, column, sourceFilePath);
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