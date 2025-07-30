#include <omniscript/Lexer.h>
#include <omniscript/Tokens.h>
#include <omniscript/Console.h>

namespace Omniscript {

Token Lexer::getOperator(char currentChar) {
    size_t startLine = line_;
    size_t startColumn = column_;

    switch (currentChar) {
        case '+':
            if (peek() == '+') {
                advance(2);
                stats_.operatorCount++;
                return TokenFactory::createOperator(TokenTypes::Increment, startLine, startColumn, sourceFilePath_);
            }
            if (peek() == '=') {
                advance(2);
                stats_.operatorCount++;
                return TokenFactory::createOperator(TokenTypes::PlusAssign, startLine, startColumn, sourceFilePath_);
            }
            advance();
            stats_.operatorCount++;
            return TokenFactory::createOperator(TokenTypes::Plus, startLine, startColumn, sourceFilePath_);
        case '-':
            if (peek() == '>') {
                advance(2);
                stats_.operatorCount++;
                return TokenFactory::createOperator(TokenTypes::Arrow, startLine, startColumn, sourceFilePath_);
            }
            if (peek() == '-') {
                if (peek(2) == '>') {
                    advance(3);
                    stats_.operatorCount++;
                    return TokenFactory::createOperator(TokenTypes::Arrow, startLine, startColumn, sourceFilePath_);
                }
                advance(2);
                stats_.operatorCount++;
                return TokenFactory::createOperator(TokenTypes::Decrement, startLine, startColumn, sourceFilePath_);
            }
            if (peek() == '=') {
                advance(2);
                stats_.operatorCount++;
                return TokenFactory::createOperator(TokenTypes::MinusAssign, startLine, startColumn, sourceFilePath_);
            }
            advance();
            stats_.operatorCount++;
            return TokenFactory::createOperator(TokenTypes::Minus, startLine, startColumn, sourceFilePath_);
        case '*':
            if (peek() == '=') {
                advance(2);
                stats_.operatorCount++;
                return TokenFactory::createOperator(TokenTypes::MultiplyAssign, startLine, startColumn, sourceFilePath_);
            }
            if (peek() == '*') {
                advance(2);
                if (peek() == '=') {
                    advance();
                    stats_.operatorCount++;
                    return TokenFactory::createOperator(TokenTypes::PowerAssign, startLine, startColumn, sourceFilePath_);
                }
                stats_.operatorCount++;
                return TokenFactory::createOperator(TokenTypes::Power, startLine, startColumn, sourceFilePath_);
            }
            advance();
            stats_.operatorCount++;
            return TokenFactory::createOperator(TokenTypes::Multiply, startLine, startColumn, sourceFilePath_);
        case '/':
            if (peek() == '=') {
                advance(2);
                stats_.operatorCount++;
                return TokenFactory::createOperator(TokenTypes::DivideAssign, startLine, startColumn, sourceFilePath_);
            }
            if (peek() == '/') {
                advance(2);
                stats_.operatorCount++;
                return TokenFactory::createOperator(TokenTypes::FloorDivide, startLine, startColumn, sourceFilePath_);
            }
            advance();
            stats_.operatorCount++;
            return TokenFactory::createOperator(TokenTypes::Divide, startLine, startColumn, sourceFilePath_);
        case '%':
            if (peek() == '=') {
                advance(2);
                stats_.operatorCount++;
                return TokenFactory::createOperator(TokenTypes::ModuloAssign, startLine, startColumn, sourceFilePath_);
            }
            advance();
            stats_.operatorCount++;
            return TokenFactory::createOperator(TokenTypes::Modulo, startLine, startColumn, sourceFilePath_);
        case '&':
            if (peek() == '&') {
                advance(2);
                stats_.operatorCount++;
                return TokenFactory::createOperator(TokenTypes::LogicalAnd, startLine, startColumn, sourceFilePath_);
            }
            if (peek() == '=') {
                advance(2);
                stats_.operatorCount++;
                return TokenFactory::createOperator(TokenTypes::BitwiseAndAssign, startLine, startColumn, sourceFilePath_);
            }
            advance();
            stats_.operatorCount++;
            return TokenFactory::createOperator(TokenTypes::BitwiseAnd, startLine, startColumn, sourceFilePath_);
        case '|':
            if (peek() == '|') {
                advance(2);
                stats_.operatorCount++;
                return TokenFactory::createOperator(TokenTypes::LogicalOr, startLine, startColumn, sourceFilePath_);
            }
            if (peek() == '=') {
                advance(2);
                stats_.operatorCount++;
                return TokenFactory::createOperator(TokenTypes::BitwiseOrAssign, startLine, startColumn, sourceFilePath_);
            }
            advance();
            stats_.operatorCount++;
            return TokenFactory::createOperator(TokenTypes::BitwiseOr, startLine, startColumn, sourceFilePath_);
        case '^':
            if (peek() == '=') {
                advance(2);
                stats_.operatorCount++;
                return TokenFactory::createOperator(TokenTypes::BitwiseXorAssign, startLine, startColumn, sourceFilePath_);
            }
            advance();
            stats_.operatorCount++;
            return TokenFactory::createOperator(TokenTypes::BitwiseXor, startLine, startColumn, sourceFilePath_);
        case '~':
            advance();
            stats_.operatorCount++;
            return TokenFactory::createOperator(TokenTypes::Tilde, startLine, startColumn, sourceFilePath_);
        case '=':
            if (peek() == '=') {
                advance(2);
                stats_.operatorCount++;
                return TokenFactory::createOperator(TokenTypes::Equals, startLine, startColumn, sourceFilePath_);
            }
            if (peek() == '>') {
                advance(2);
                stats_.operatorCount++;
                return TokenFactory::createOperator(TokenTypes::Arrow, startLine, startColumn, sourceFilePath_);
            }
            advance();
            stats_.operatorCount++;
            return TokenFactory::createOperator(TokenTypes::Assign, startLine, startColumn, sourceFilePath_);
        case '!':
            if (peek() == '=') {
                advance(2);
                stats_.operatorCount++;
                return TokenFactory::createOperator(TokenTypes::NotEquals, startLine, startColumn, sourceFilePath_);
            }
            advance();
            stats_.operatorCount++;
            return TokenFactory::createOperator(TokenTypes::LogicalNot, startLine, startColumn, sourceFilePath_);
        case '<':
            if (peek() == '<') {
                if (peek(2) == '=') {
                    advance(3);
                    stats_.operatorCount++;
                    return TokenFactory::createOperator(TokenTypes::ShiftLeftAssign, startLine, startColumn, sourceFilePath_);
                }
                advance(2);
                stats_.operatorCount++;
                return TokenFactory::createOperator(TokenTypes::ShiftLeft, startLine, startColumn, sourceFilePath_);
            }
            if (peek() == '=') {
                advance(2);
                stats_.operatorCount++;
                return TokenFactory::createOperator(TokenTypes::LessEqual, startLine, startColumn, sourceFilePath_);
            }
            advance();
            stats_.operatorCount++;
            return TokenFactory::createOperator(TokenTypes::LessThan, startLine, startColumn, sourceFilePath_);
        case '>':
            if (peek() == '>') {
                if (peek(2) == '=') {
                    advance(3);
                    stats_.operatorCount++;
                    return TokenFactory::createOperator(TokenTypes::ShiftRightAssign, startLine, startColumn, sourceFilePath_);
                }
                advance(2);
                stats_.operatorCount++;
                return TokenFactory::createOperator(TokenTypes::ShiftRight, startLine, startColumn, sourceFilePath_);
            }
            if (peek() == '=') {
                advance(2);
                stats_.operatorCount++;
                return TokenFactory::createOperator(TokenTypes::GreaterEqual, startLine, startColumn, sourceFilePath_);
            }
            advance();
            stats_.operatorCount++;
            return TokenFactory::createOperator(TokenTypes::GreaterThan, startLine, startColumn, sourceFilePath_);
        case ':':
            if (peek() == ':') {
                advance(2);
                stats_.operatorCount++;
                return TokenFactory::createOperator(TokenTypes::ScopeResolution, startLine, startColumn, sourceFilePath_);
            }
            advance();
            stats_.operatorCount++;
            return TokenFactory::createOperator(TokenTypes::Colon, startLine, startColumn, sourceFilePath_);
        case '?':
            if (peek() == '?') {
                advance(2);
                stats_.operatorCount++;
                return TokenFactory::createOperator(TokenTypes::NullCoalescing, startLine, startColumn, sourceFilePath_);
            }
            advance();
            stats_.operatorCount++;
            return TokenFactory::createOperator(TokenTypes::QuestionMark, startLine, startColumn, sourceFilePath_);
        case '.':
            if (peek() == '.' && peek(2) == '.') {
                advance(3);
                stats_.operatorCount++;
                return TokenFactory::createOperator(TokenTypes::Ellipsis, startLine, startColumn, sourceFilePath_);
            }
            if (!isDigit(peek())) {
                advance();
                stats_.operatorCount++;
                return TokenFactory::createOperator(TokenTypes::Dot, startLine, startColumn, sourceFilePath_);
            }
            break;
        case '(':
            advance();
            stats_.operatorCount++;
            return TokenFactory::createOperator(TokenTypes::LeftParen, startLine, startColumn, sourceFilePath_);
        case ')':
            advance();
            stats_.operatorCount++;
            return TokenFactory::createOperator(TokenTypes::RightParen, startLine, startColumn, sourceFilePath_);
        case '{':
            advance();
            stats_.operatorCount++;
            return TokenFactory::createOperator(TokenTypes::LeftBrace, startLine, startColumn, sourceFilePath_);
        case '}':
            advance();
            stats_.operatorCount++;
            return TokenFactory::createOperator(TokenTypes::RightBrace, startLine, startColumn, sourceFilePath_);
        case '[':
            advance();
            stats_.operatorCount++;
            return TokenFactory::createOperator(TokenTypes::LeftBracket, startLine, startColumn, sourceFilePath_);
        case ']':
            advance();
            stats_.operatorCount++;
            return TokenFactory::createOperator(TokenTypes::RightBracket, startLine, startColumn, sourceFilePath_);
        case ';':
            advance();
            stats_.operatorCount++;
            return TokenFactory::createOperator(TokenTypes::Semicolon, startLine, startColumn, sourceFilePath_);
        case ',':
            advance();
            stats_.operatorCount++;
            return TokenFactory::createOperator(TokenTypes::Comma, startLine, startColumn, sourceFilePath_);
        case '@':
            advance();
            stats_.operatorCount++;
            return TokenFactory::createOperator(TokenTypes::AtSymbol, startLine, startColumn, sourceFilePath_);
        case '#':
            advance();
            stats_.operatorCount++;
            return TokenFactory::createOperator(TokenTypes::Hash, startLine, startColumn, sourceFilePath_);
        case '$':
            advance();
            stats_.operatorCount++;
            return TokenFactory::createOperator(TokenTypes::Dollar, startLine, startColumn, sourceFilePath_);
    }

    Token invalid = handleUnexpectedCharacter(currentChar);
    advance();
    return invalid;
}

Token Lexer::parseMultiCharOperator(char first, char second, char third) {
    // Handled in getOperator
    return TokenFactory::createInvalid(std::string(1, first), line_, column_, sourceFilePath_);
}

} // namespace Omniscript
