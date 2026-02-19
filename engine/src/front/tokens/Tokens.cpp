#include <omniscript/tokens/Tokens.h>

#include <sstream>
#include <utility>

namespace Omniscript {

// ---------------- ctors ----------------

Token::Token(TokenType type)
    : m_type(type)
    , m_span() {}

Token::Token(TokenType type,
             std::string lexeme,
             std::string value,
             FileSpan span)
    : m_type(type)
    , m_lexeme(std::move(lexeme))
    , m_value(std::move(value))
    , m_span(std::move(span)) {}

Token::Token(TokenType type,
             std::string lexeme,
             std::string value,
             std::size_t line,
             std::size_t column,
             std::string filePath)
    : Token(type,
            std::move(lexeme),
            std::move(value),
            FileSpan{
                FilePosition{line, column, filePath},
                FilePosition{line, column, std::move(filePath)}
            }) {}

Token::Token(TokenType type,
             std::string lexeme,
             std::string value,
             std::size_t sLine, std::size_t sCol,
             std::size_t eLine, std::size_t eCol,
             std::string filePath)
    : Token(type,
            std::move(lexeme),
            std::move(value),
            FileSpan{
                FilePosition{sLine, sCol, filePath},
                FilePosition{eLine, eCol, std::move(filePath)}
            }) {}

// ---------------- classification ----------------

bool Token::isKeyword() const noexcept {
    switch (m_type) {
        case TokenType::If: case TokenType::ElseIf: case TokenType::Else:
        case TokenType::While: case TokenType::For:
        case TokenType::Continue: case TokenType::Break: case TokenType::Return:
        case TokenType::Function: case TokenType::Let: case TokenType::Var: case TokenType::Const:
        case TokenType::New: case TokenType::Delete:
        case TokenType::Class: case TokenType::Struct: case TokenType::Namespace: case TokenType::Using:
        case TokenType::Public: case TokenType::Private: case TokenType::Protected:
        case TokenType::Override: case TokenType::Virtual: case TokenType::Static: case TokenType::Final:
        case TokenType::True: case TokenType::False: case TokenType::Null: case TokenType::Nullptr:
        case TokenType::Enum: case TokenType::Extends: case TokenType::Variant: case TokenType::Any:
        case TokenType::Import: case TokenType::Include: case TokenType::From: case TokenType::Module:
        case TokenType::Extern: case TokenType::Intrinsic: case TokenType::Volatile:
        case TokenType::As: case TokenType::Type:
            return true;
        default:
            return false;
    }
}

bool Token::isLiteral() const noexcept {
    switch (m_type) {
        case TokenType::CharacterLiteral:
        case TokenType::IntegerLiteral:
        case TokenType::BigIntLiteral:
        case TokenType::BinaryLiteral:
        case TokenType::OctalLiteral:
        case TokenType::HexLiteral:
        case TokenType::FloatLiteral:
        case TokenType::StringLiteral:
        case TokenType::TemplateHead:
        case TokenType::TemplateMiddle:
        case TokenType::TemplateTail:
        case TokenType::TemplateLiteral:
        case TokenType::True:
        case TokenType::False:
        case TokenType::Null:
        case TokenType::Nullptr:
            return true;
        default:
            return false;
    }
}

bool Token::isNumericLiteral() const noexcept {
    switch (m_type) {
        case TokenType::IntegerLiteral:
        case TokenType::BigIntLiteral:
        case TokenType::BinaryLiteral:
        case TokenType::OctalLiteral:
        case TokenType::HexLiteral:
        case TokenType::FloatLiteral:
            return true;
        default:
            return false;
    }
}

bool Token::isOpenBracket() const noexcept {
    return m_type == TokenType::LeftParen ||
           m_type == TokenType::LeftBrace ||
           m_type == TokenType::LeftBracket;
}

bool Token::isCloseBracket() const noexcept {
    return m_type == TokenType::RightParen ||
           m_type == TokenType::RightBrace ||
           m_type == TokenType::RightBracket;
}

TokenType Token::matchingBracket() const noexcept {
    switch (m_type) {
        case TokenType::LeftParen:    return TokenType::RightParen;
        case TokenType::RightParen:   return TokenType::LeftParen;
        case TokenType::LeftBrace:    return TokenType::RightBrace;
        case TokenType::RightBrace:   return TokenType::LeftBrace;
        case TokenType::LeftBracket:  return TokenType::RightBracket;
        case TokenType::RightBracket: return TokenType::LeftBracket;
        default:                      return TokenType::Invalid;
    }
}

// ---------------- operator group checks ----------------

bool Token::isComparisonOperator() const noexcept {
    switch (m_type) {
        case TokenType::Equals:
        case TokenType::NotEquals:
        case TokenType::Less:
        case TokenType::Greater:
        case TokenType::LessEqual:
        case TokenType::GreaterEqual:
        case TokenType::Spaceship:
            return true;
        default:
            return false;
    }
}

bool Token::isAssignmentOperator() const noexcept {
    switch (m_type) {
        case TokenType::Assign:
        case TokenType::PlusAssign:
        case TokenType::MinusAssign:
        case TokenType::StarAssign:
        case TokenType::SlashAssign:
        case TokenType::PercentAssign:
        case TokenType::PowerAssign:
        case TokenType::BitAndAssign:
        case TokenType::BitOrAssign:
        case TokenType::BitXorAssign:
        case TokenType::ShiftLeftAssign:
        case TokenType::ShiftRightAssign:
            return true;
        default:
            return false;
    }
}

bool Token::isArithmeticOperator() const noexcept {
    switch (m_type) {
        case TokenType::Plus:
        case TokenType::Minus:
        case TokenType::Star:
        case TokenType::Slash:
        case TokenType::Percent:
        case TokenType::FloorDivide:
        case TokenType::Power:
            return true;
        default:
            return false;
    }
}

bool Token::isUnaryOperator() const noexcept {
    switch (m_type) {
        case TokenType::Increment:
        case TokenType::Decrement:
        case TokenType::LogicalNot:
        case TokenType::BitNot:
        case TokenType::Plus:
        case TokenType::Minus:
            return true;
        default:
            return false;
    }
}

bool Token::isLogicalOperator() const noexcept {
    switch (m_type) {
        case TokenType::LogicalAnd:
        case TokenType::LogicalOr:
        case TokenType::LogicalXor:
        case TokenType::LogicalNot:
            return true;
        default:
            return false;
    }
}

bool Token::isBitwiseOperator() const noexcept {
    switch (m_type) {
        case TokenType::BitAnd:
        case TokenType::BitOr:
        case TokenType::BitXor:
        case TokenType::BitNot:
        case TokenType::ShiftLeft:
        case TokenType::ShiftRight:
            return true;
        default:
            return false;
    }
}

// ---------------- precedence / associativity ----------------
// Higher number = binds tighter.

int Token::precedence() const noexcept {
    switch (m_type) {
        case TokenType::Pipe:           return 1;

        case TokenType::Assign:
        case TokenType::PlusAssign:
        case TokenType::MinusAssign:
        case TokenType::StarAssign:
        case TokenType::SlashAssign:
        case TokenType::PercentAssign:
        case TokenType::PowerAssign:
        case TokenType::BitAndAssign:
        case TokenType::BitOrAssign:
        case TokenType::BitXorAssign:
        case TokenType::ShiftLeftAssign:
        case TokenType::ShiftRightAssign:
            return 2;

        case TokenType::NullCoalescing: return 3;
        case TokenType::LogicalOr:      return 4;
        case TokenType::LogicalXor:     return 5;
        case TokenType::LogicalAnd:     return 6;

        case TokenType::BitOr:          return 7;
        case TokenType::BitXor:         return 8;
        case TokenType::BitAnd:         return 9;

        case TokenType::Equals:
        case TokenType::NotEquals:
            return 10;

        case TokenType::Less:
        case TokenType::Greater:
        case TokenType::LessEqual:
        case TokenType::GreaterEqual:
        case TokenType::Spaceship:
            return 11;

        case TokenType::ShiftLeft:
        case TokenType::ShiftRight:
            return 12;

        case TokenType::Plus:
        case TokenType::Minus:
            return 13;

        case TokenType::Star:
        case TokenType::Slash:
        case TokenType::Percent:
        case TokenType::FloorDivide:
            return 14;

        case TokenType::Power:
            return 15;

        default:
            return 0; // not a binary operator
    }
}

Assoc Token::associativity() const noexcept {
    switch (m_type) {
        // Right associative operators
        case TokenType::Assign:
        case TokenType::PlusAssign:
        case TokenType::MinusAssign:
        case TokenType::StarAssign:
        case TokenType::SlashAssign:
        case TokenType::PercentAssign:
        case TokenType::PowerAssign:
        case TokenType::BitAndAssign:
        case TokenType::BitOrAssign:
        case TokenType::BitXorAssign:
        case TokenType::ShiftLeftAssign:
        case TokenType::ShiftRightAssign:
        case TokenType::NullCoalescing:
        case TokenType::Power:
            return Assoc::Right;

        default:
            return Assoc::Left;
    }
}

// ---------------- strings / debug ----------------

const char* tokenTypeName(TokenType t) noexcept {
    switch (t) {
        case TokenType::EndOfInput: return "EndOfInput";
        case TokenType::Invalid: return "Invalid";
        case TokenType::Error: return "Error";
        case TokenType::If: return "If";
        case TokenType::ElseIf: return "ElseIf";
        case TokenType::Else: return "Else";
        case TokenType::While: return "While";
        case TokenType::For: return "For";
        case TokenType::Continue: return "Continue";
        case TokenType::Break: return "Break";
        case TokenType::Return: return "Return";
        case TokenType::Function: return "Function";
        case TokenType::Let: return "Let";
        case TokenType::Var: return "Var";
        case TokenType::Const: return "Const";
        case TokenType::New: return "New";
        case TokenType::Delete: return "Delete";
        case TokenType::Class: return "Class";
        case TokenType::Struct: return "Struct";
        case TokenType::Namespace: return "Namespace";
        case TokenType::Using: return "Using";
        case TokenType::Public: return "Public";
        case TokenType::Private: return "Private";
        case TokenType::Protected: return "Protected";
        case TokenType::Override: return "Override";
        case TokenType::Virtual: return "Virtual";
        case TokenType::Static: return "Static";
        case TokenType::Final: return "Final";
        case TokenType::True: return "True";
        case TokenType::False: return "False";
        case TokenType::Null: return "Null";
        case TokenType::Nullptr: return "Nullptr";
        case TokenType::Enum: return "Enum";
        case TokenType::Extends: return "Extends";
        case TokenType::Variant: return "Variant";
        case TokenType::Any: return "Any";
        case TokenType::Import: return "Import";
        case TokenType::Include: return "Include";
        case TokenType::From: return "From";
        case TokenType::Module: return "Module";
        case TokenType::Extern: return "Extern";
        case TokenType::Intrinsic: return "Intrinsic";
        case TokenType::Volatile: return "Volatile";
        case TokenType::As: return "As";
        case TokenType::Type: return "Type";
        case TokenType::Identifier: return "Identifier";
        case TokenType::CharacterLiteral: return "CharacterLiteral";
        case TokenType::IntegerLiteral: return "IntegerLiteral";
        case TokenType::BigIntLiteral: return "BigIntLiteral";
        case TokenType::BinaryLiteral: return "BinaryLiteral";
        case TokenType::OctalLiteral: return "OctalLiteral";
        case TokenType::HexLiteral: return "HexLiteral";
        case TokenType::FloatLiteral: return "FloatLiteral";
        case TokenType::StringLiteral: return "StringLiteral";
        case TokenType::TemplateHead: return "TemplateHead";
        case TokenType::TemplateMiddle: return "TemplateMiddle";
        case TokenType::TemplateTail: return "TemplateTail";
        case TokenType::TemplateLiteral: return "TemplateLiteral";
        case TokenType::ScopeResolution: return "ScopeResolution";
        case TokenType::Plus: return "Plus";
        case TokenType::Minus: return "Minus";
        case TokenType::Star: return "Star";
        case TokenType::Slash: return "Slash";
        case TokenType::Percent: return "Percent";
        case TokenType::FloorDivide: return "FloorDivide";
        case TokenType::Increment: return "Increment";
        case TokenType::Decrement: return "Decrement";
        case TokenType::Power: return "Power";
        case TokenType::Assign: return "Assign";
        case TokenType::PlusAssign: return "PlusAssign";
        case TokenType::MinusAssign: return "MinusAssign";
        case TokenType::StarAssign: return "StarAssign";
        case TokenType::SlashAssign: return "SlashAssign";
        case TokenType::PercentAssign: return "PercentAssign";
        case TokenType::PowerAssign: return "PowerAssign";
        case TokenType::Equals: return "Equals";
        case TokenType::NotEquals: return "NotEquals";
        case TokenType::Less: return "Less";
        case TokenType::Greater: return "Greater";
        case TokenType::LessEqual: return "LessEqual";
        case TokenType::GreaterEqual: return "GreaterEqual";
        case TokenType::Spaceship: return "Spaceship";
        case TokenType::LogicalAnd: return "LogicalAnd";
        case TokenType::LogicalOr: return "LogicalOr";
        case TokenType::LogicalXor: return "LogicalXor";
        case TokenType::LogicalNot: return "LogicalNot";
        case TokenType::BitAnd: return "BitAnd";
        case TokenType::BitOr: return "BitOr";
        case TokenType::BitXor: return "BitXor";
        case TokenType::BitNot: return "BitNot";
        case TokenType::ShiftLeft: return "ShiftLeft";
        case TokenType::ShiftRight: return "ShiftRight";
        case TokenType::BitAndAssign: return "BitAndAssign";
        case TokenType::BitOrAssign: return "BitOrAssign";
        case TokenType::BitXorAssign: return "BitXorAssign";
        case TokenType::ShiftLeftAssign: return "ShiftLeftAssign";
        case TokenType::ShiftRightAssign: return "ShiftRightAssign";
        case TokenType::LeftParen: return "LeftParen";
        case TokenType::RightParen: return "RightParen";
        case TokenType::LeftBrace: return "LeftBrace";
        case TokenType::RightBrace: return "RightBrace";
        case TokenType::LeftBracket: return "LeftBracket";
        case TokenType::RightBracket: return "RightBracket";
        case TokenType::Semicolon: return "Semicolon";
        case TokenType::Comma: return "Comma";
        case TokenType::Dot: return "Dot";
        case TokenType::Colon: return "Colon";
        case TokenType::QuestionMark: return "QuestionMark";
        case TokenType::Ellipsis: return "Ellipsis";
        case TokenType::NullCoalescing: return "NullCoalescing";
        case TokenType::SafeNavigation: return "SafeNavigation";
        case TokenType::Arrow: return "Arrow";
        case TokenType::Pipe: return "Pipe";
        case TokenType::At: return "At";
        case TokenType::Hash: return "Hash";
        case TokenType::Dollar: return "Dollar";
        case TokenType::Newline: return "Newline";
        default: return "UnknownTokenType";
    }
}

std::string Token::toString() const {
    return std::string(tokenTypeName(m_type));
}

std::string Token::toDebugString() const {
    std::ostringstream oss;
    oss << tokenTypeName(m_type);

    if (!m_lexeme.empty())
        oss << " lexeme=\"" << m_lexeme << "\"";
    if (!m_value.empty())
        oss << " value=\"" << m_value << "\"";

    if (m_span.isValid()) {
        oss << " @" << m_span.start.line << ":" << m_span.start.col;

        if (!m_span.start.filePath.empty())
            oss << " file=\"" << m_span.start.filePath << "\"";

        if (m_span.start.line != m_span.end.line || m_span.start.col != m_span.end.col) {
            oss << " .. " << m_span.end.line << ":" << m_span.end.col;
        }
    } else {
        oss << " @?:?";
    }

    return oss.str();
}

bool Token::operator==(const Token& other) const noexcept {
    return m_type == other.m_type &&
           m_lexeme == other.m_lexeme &&
           m_value == other.m_value &&
           m_span.start.line == other.m_span.start.line &&
           m_span.start.col == other.m_span.start.col &&
           m_span.start.filePath == other.m_span.start.filePath &&
           m_span.end.line == other.m_span.end.line &&
           m_span.end.col == other.m_span.end.col &&
           m_span.end.filePath == other.m_span.end.filePath;
}

std::ostream& operator<<(std::ostream& os, const Token& token) {
    os << token.toDebugString();
    return os;
}

} // namespace Omniscript
