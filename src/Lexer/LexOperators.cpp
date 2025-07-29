#include <omniscript/omniscript_pch.h>
#include <omniscript/Lexer.h>
#include <omniscript/Tokens.h>
#include <omniscript/utils.h>
#include <omniscript/Core.h>

Token Lexer::getOperator(char &currentChar) {
    Omniscript::FileSpan span;
    span.start.line = line;
    span.start.col = column;
    span.start.filePath = sourceFilePath;

    // Operator Tokens
    switch (currentChar) {
        case '+':
            if (peek() == '+') {
                currentPosition += 2;
                column += 2;
                span.end.line = line;
                span.end.col = column;
                span.end.filePath = sourceFilePath;
                return Token(TokenTypes::Increment, "++", line, column, sourceFilePath);
            } else if (peek() == '=') {
                currentPosition += 2;
                column += 2;
                span.end.line = line;
                span.end.col = column;
                span.end.filePath = sourceFilePath;
                return Token(TokenTypes::PlusAssign, "+=", line, column, sourceFilePath);
            }
            currentPosition++;
            column++;
            span.end.line = line;
            span.end.col = column;
            span.end.filePath = sourceFilePath;
            return Token(TokenTypes::Plus, "+", line, column, sourceFilePath);
        case '-':
            if (peek() == '>') {
                currentPosition += 2;
                column += 2;
                span.end.line = line;
                span.end.col = column;
                span.end.filePath = sourceFilePath;
                return Token(TokenTypes::Arrow, "->", line, column, sourceFilePath);
            }
            if (peek() == '-') {
                if (peek(2) == '>') {
                    currentPosition += 3;
                    column += 3;
                    span.end.line = line;
                    span.end.col = column;
                    span.end.filePath = sourceFilePath;
                    return Token(TokenTypes::Arrow, "-->", line, column, sourceFilePath);
                }
                currentPosition += 2;
                column += 2;
                span.end.line = line;
                span.end.col = column;
                span.end.filePath = sourceFilePath;
                return Token(TokenTypes::Decrement, "--", line, column, sourceFilePath);
            }
            if (peek() == '=') {
                currentPosition += 2;
                column += 2;
                span.end.line = line;
                span.end.col = column;
                span.end.filePath = sourceFilePath;
                return Token(TokenTypes::MinusAssign, "-=", line, column, sourceFilePath);
            }
            currentPosition++;
            column++;
            span.end.line = line;
            span.end.col = column;
            span.end.filePath = sourceFilePath;
            return Token(TokenTypes::Minus, "-", line, column, sourceFilePath);
        case '/':
            if (peek() == '=') {
                currentPosition += 2;
                column += 2;
                span.end.line = line;
                span.end.col = column;
                span.end.filePath = sourceFilePath;
                return Token(TokenTypes::DivideAssign, "/=", line, column, sourceFilePath);
            }
            currentPosition++;
            column++;
            span.end.line = line;
            span.end.col = column;
            span.end.filePath = sourceFilePath;
            return Token(TokenTypes::Divide, "/", line, column, sourceFilePath);
        case '*':
            if (peek() == '=') {
                currentPosition += 2;
                column += 2;
                span.end.line = line;
                span.end.col = column;
                span.end.filePath = sourceFilePath;
                return Token(TokenTypes::MultiplyAssign, "*=", line, column, sourceFilePath);
            }
            currentPosition++;
            column++;
            span.end.line = line;
            span.end.col = column;
            span.end.filePath = sourceFilePath;
            return Token(TokenTypes::Multiply, "*", line, column, sourceFilePath);
        case '%':
            currentPosition++;
            column++;
            span.end.line = line;
            span.end.col = column;
            span.end.filePath = sourceFilePath;
            return Token(TokenTypes::Modulo, "%", line, column, sourceFilePath);
        case '&':
            if (peek() == '&') {
                currentPosition += 2;
                column += 2;
                span.end.line = line;
                span.end.col = column;
                span.end.filePath = sourceFilePath;
                return Token(TokenTypes::LogicalAnd, "&&", line, column, sourceFilePath);
            } else if (peek() == '=') {
                currentPosition += 2;
                column += 2;
                span.end.line = line;
                span.end.col = column;
                span.end.filePath = sourceFilePath;
                return Token(TokenTypes::BitwiseAndAssign, "&=", line, column, sourceFilePath);
            }
            currentPosition++;
            column++;
            span.end.line = line;
            span.end.col = column;
            span.end.filePath = sourceFilePath;
            return Token(TokenTypes::BitwiseAnd, "&", line, column, sourceFilePath);
        case '|':
            if (peek() == '|') {
                currentPosition += 2;
                column += 2;
                span.end.line = line;
                span.end.col = column;
                span.end.filePath = sourceFilePath;
                return Token(TokenTypes::LogicalOr, "||", line, column, sourceFilePath);
            } else if (peek() == '=') {
                currentPosition += 2;
                column += 2;
                span.end.line = line;
                span.end.col = column;
                span.end.filePath = sourceFilePath;
                return Token(TokenTypes::BitwiseOrAssign, "|=", line, column, sourceFilePath);
            }
            currentPosition++;
            column++;
            span.end.line = line;
            span.end.col = column;
            span.end.filePath = sourceFilePath;
            return Token(TokenTypes::BitwiseOr, "|", line, column, sourceFilePath);
        case '^':
            if (peek() == '=') {
                currentPosition += 2;
                column += 2;
                span.end.line = line;
                span.end.col = column;
                span.end.filePath = sourceFilePath;
                return Token(TokenTypes::BitwiseXorAssign, "^=", line, column, sourceFilePath);
            }
            currentPosition++;
            column++;
            span.end.line = line;
            span.end.col = column;
            span.end.filePath = sourceFilePath;
            return Token(TokenTypes::BitwiseXor, "^", line, column, sourceFilePath);
        case '~':
            currentPosition++;
            column++;
            span.end.line = line;
            span.end.col = column;
            span.end.filePath = sourceFilePath;
            return Token(TokenTypes::Tilde, "~", line, column, sourceFilePath);
        case '=':
            if (peek() == '=') {
                currentPosition += 2;
                column += 2;
                span.end.line = line;
                span.end.col = column;
                span.end.filePath = sourceFilePath;
                return Token(TokenTypes::Equals, "==", line, column, sourceFilePath);
            } else if (peek() == '>') {
                currentPosition += 2;
                column += 2;
                span.end.line = line;
                span.end.col = column;
                span.end.filePath = sourceFilePath;
                return Token(TokenTypes::Arrow, "=>", line, column, sourceFilePath);
            }
            currentPosition++;
            column++;
            span.end.line = line;
            span.end.col = column;
            span.end.filePath = sourceFilePath;
            return Token(TokenTypes::Assign, "=", line, column, sourceFilePath);
        case '(':
            currentPosition++;
            column++;
            span.end.line = line;
            span.end.col = column;
            span.end.filePath = sourceFilePath;
            return Token(TokenTypes::LeftParen, "(", line, column, sourceFilePath);
        case ')':
            currentPosition++;
            column++;
            span.end.line = line;
            span.end.col = column;
            span.end.filePath = sourceFilePath;
            return Token(TokenTypes::RightParen, ")", line, column, sourceFilePath);
        case '{':
            currentPosition++;
            column++;
            span.end.line = line;
            span.end.col = column;
            span.end.filePath = sourceFilePath;
            return Token(TokenTypes::LeftBrace, "{", line, column, sourceFilePath);
        case '}':
            currentPosition++;
            column++;
            span.end.line = line;
            span.end.col = column;
            span.end.filePath = sourceFilePath;
            return Token(TokenTypes::RightBrace, "}", line, column, sourceFilePath);
        case '[':
            currentPosition++;
            column++;
            span.end.line = line;
            span.end.col = column;
            span.end.filePath = sourceFilePath;
            return Token(TokenTypes::LeftBracket, "[", line, column, sourceFilePath);
        case ']':
            currentPosition++;
            column++;
            span.end.line = line;
            span.end.col = column;
            span.end.filePath = sourceFilePath;
            return Token(TokenTypes::RightBracket, "]", line, column, sourceFilePath);
        case ';':
            currentPosition++;
            column++;
            span.end.line = line;
            span.end.col = column;
            span.end.filePath = sourceFilePath;
            return Token(TokenTypes::Semicolon, ";", line, column, sourceFilePath);
        case ',':
            currentPosition++;
            column++;
            span.end.line = line;
            span.end.col = column;
            span.end.filePath = sourceFilePath;
            return Token(TokenTypes::Comma, ",", line, column, sourceFilePath);
        case '.':
            if (peek() == '.' && peek(2) == '.') {
                currentPosition += 3;
                column += 3;
                span.end.line = line;
                span.end.col = column;
                span.end.filePath = sourceFilePath;
                return Token(TokenTypes::Ellipsis, "...", line, column, sourceFilePath);
            }
            currentPosition++;
            column++;
            span.end.line = line;
            span.end.col = column;
            span.end.filePath = sourceFilePath;
            return Token(TokenTypes::Dot, ".", line, column, sourceFilePath);
        case ':':
            if (peek() == ':') {
                currentPosition += 2;
                column += 2;
                span.end.line = line;
                span.end.col = column;
                span.end.filePath = sourceFilePath;
                return Token(TokenTypes::ScopeResolution, "::", line, column, sourceFilePath);
            }
            currentPosition++;
            column++;
            span.end.line = line;
            span.end.col = column;
            span.end.filePath = sourceFilePath;
            return Token(TokenTypes::Colon, ":", line, column, sourceFilePath);
        case '?':
            currentPosition++;
            column++;
            span.end.line = line;
            span.end.col = column;
            span.end.filePath = sourceFilePath;
            return Token(TokenTypes::QuestionMark, "?", line, column, sourceFilePath);
        case '!':
            if (peek() == '=') {
                currentPosition += 2;
                column += 2;
                span.end.line = line;
                span.end.col = column;
                span.end.filePath = sourceFilePath;
                return Token(TokenTypes::NotEquals, "!=", line, column, sourceFilePath);
            }
            currentPosition++;
            column++;
            span.end.line = line;
            span.end.col = column;
            span.end.filePath = sourceFilePath;
            return Token(TokenTypes::LogicalNot, "!", line, column, sourceFilePath);
        case '<':
            if (peek() == '<') {
                if (peek(2) == '=') {
                    currentPosition += 3;
                    column += 3;
                    span.end.line = line;
                    span.end.col = column;
                    span.end.filePath = sourceFilePath;
                    return Token(TokenTypes::ShiftLeftAssign, "<<=", line, column, sourceFilePath);
                }
                currentPosition += 2;
                column += 2;
                span.end.line = line;
                span.end.col = column;
                span.end.filePath = sourceFilePath;
                return Token(TokenTypes::ShiftLeft, "<<", line, column, sourceFilePath);
            } else if (peek() == '=') {
                currentPosition += 2;
                column += 2;
                span.end.line = line;
                span.end.col = column;
                span.end.filePath = sourceFilePath;
                return Token(TokenTypes::LessEqual, "<=", line, column, sourceFilePath);
            } 
            currentPosition++;
            column++;
            span.end.line = line;
            span.end.col = column;
            span.end.filePath = sourceFilePath;
            return Token(TokenTypes::LessThan, "<", line, column, sourceFilePath);
        case '>':
            if (peek() == '>') {
                if (peek(2) == '=') {
                    currentPosition += 3;
                    column += 3;
                    span.end.line = line;
                    span.end.col = column;
                    span.end.filePath = sourceFilePath;
                    return Token(TokenTypes::ShiftRightAssign, ">>=", line, column, sourceFilePath);
                }
                currentPosition += 2;
                column += 2;
                span.end.line = line;
                span.end.col = column;
                span.end.filePath = sourceFilePath;
                return Token(TokenTypes::ShiftRight, ">>", line, column, sourceFilePath);
            } else if (peek() == '=') {
                currentPosition += 2;
                column += 2;
                span.end.line = line;
                span.end.col = column;
                span.end.filePath = sourceFilePath;
                return Token(TokenTypes::GreaterEqual, ">=", line, column, sourceFilePath);
            }
            currentPosition++;
            column++;
            span.end.line = line;
            span.end.col = column;
            span.end.filePath = sourceFilePath;
            return Token(TokenTypes::GreaterThan, ">", line, column, sourceFilePath);
        case '\n':
            currentPosition++;
            line++;
            column = 1;
            span.end.line = line;
            span.end.col = column;
            span.end.filePath = sourceFilePath;
            return getNextToken();
        // Add other cases for additional operators or punctuation as needed
        default:
            currentPosition++;
            column++;
            span.end.line = line;
            span.end.col = column;
            span.end.filePath = sourceFilePath;
            console.reportError(
                Omniscript::Console::SYNTAX_ERROR,
                Omniscript::Console::formatString("Unrecognized character '%c' at line %zu, column %zu",
                    currentChar, line, column),
                "To resolve this:\n1. Check for valid operators or punctuation\n2. Ensure correct syntax\n3. Remove or correct unrecognized characters",
                span
            );
            return Token(TokenTypes::Invalid, std::string(1, currentChar), line, column, sourceFilePath);
    }
}