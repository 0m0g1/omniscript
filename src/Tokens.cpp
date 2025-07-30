#include <omniscript/Tokens.h>
#include <sstream>
#include <iomanip>

namespace Omniscript {

std::unordered_map<TokenTypes, std::string> tokenTypeNames = {
    // ... (previous tokenTypeNames map unchanged, included for context)
    {TokenTypes::EOI, "EOI"},
    {TokenTypes::Invalid, "Invalid"},
    {TokenTypes::Error, "Error"},
    {TokenTypes::If, "If"},
    {TokenTypes::Else_if, "Else_if"},
    {TokenTypes::Else, "Else"},
    {TokenTypes::While, "While"},
    {TokenTypes::For, "For"},
    {TokenTypes::Continue, "Continue"},
    {TokenTypes::Break, "Break"},
    {TokenTypes::Return, "Return"},
    {TokenTypes::Function, "Function"},
    {TokenTypes::FunctionCall, "FunctionCall"},
    {TokenTypes::Let, "Let"},
    {TokenTypes::Var, "Var"},
    {TokenTypes::Const, "Const"},
    {TokenTypes::Namespace, "Namespace"},
    {TokenTypes::Using, "Using"},
    {TokenTypes::New, "New"},
    {TokenTypes::Delete, "Delete"},
    {TokenTypes::Class, "Class"},
    {TokenTypes::Struct, "Struct"},
    {TokenTypes::Public, "Public"},
    {TokenTypes::Private, "Private"},
    {TokenTypes::Protected, "Protected"},
    {TokenTypes::Override, "Override"},
    {TokenTypes::Virtual, "Virtual"},
    {TokenTypes::Static, "Static"},
    {TokenTypes::Final, "Final"},
    {TokenTypes::True, "True"},
    {TokenTypes::False, "False"},
    {TokenTypes::Extends, "Extends"},
    {TokenTypes::Variant, "Variant"},
    {TokenTypes::Any, "Any"},
    {TokenTypes::Include, "Include"},
    {TokenTypes::Import, "Import"},
    {TokenTypes::From, "From"},
    {TokenTypes::Module, "Module"},
    {TokenTypes::Extern, "Extern"},
    {TokenTypes::Intrinsic, "Intrinsic"},
    {TokenTypes::Volatile, "Volatile"},
    {TokenTypes::As, "As"},
    {TokenTypes::Type, "Type"},
    {TokenTypes::Nullptr, "Nullptr"},
    {TokenTypes::Null, "Null"},
    {TokenTypes::Enum, "Enum"},
    {TokenTypes::Identifier, "Identifier"},
    {TokenTypes::Character, "Character"},
    {TokenTypes::IntegerLiteral, "IntegerLiteral"},
    {TokenTypes::BigInt, "BigInt"},
    {TokenTypes::FloatLiteral, "FloatLiteral"},
    {TokenTypes::BinaryLiteral, "BinaryLiteral"},
    {TokenTypes::OctalLiteral, "OctalLiteral"},
    {TokenTypes::HexLiteral, "HexLiteral"},
    {TokenTypes::TemplateHead, "TemplateHead"},
    {TokenTypes::TemplateMiddle, "TemplateMiddle"},
    {TokenTypes::TemplateTail, "TemplateTail"},
    {TokenTypes::TemplateLiteral, "TemplateLiteral"},
    {TokenTypes::StringLiteral, "StringLiteral"},
    {TokenTypes::Arrow, "Arrow"},
    {TokenTypes::ScopeResolution, "ScopeResolution"},
    {TokenTypes::Plus, "Plus"},
    {TokenTypes::Minus, "Minus"},
    {TokenTypes::Multiply, "Multiply"},
    {TokenTypes::Divide, "Divide"},
    {TokenTypes::Modulo, "Modulo"},
    {TokenTypes::Power, "Power"},
    {TokenTypes::Increment, "Increment"},
    {TokenTypes::Decrement, "Decrement"},
    {TokenTypes::FloorDivide, "FloorDivide"},
    {TokenTypes::Assign, "Assign"},
    {TokenTypes::PlusAssign, "PlusAssign"},
    {TokenTypes::MinusAssign, "MinusAssign"},
    {TokenTypes::MultiplyAssign, "MultiplyAssign"},
    {TokenTypes::DivideAssign, "DivideAssign"},
    {TokenTypes::ModuloAssign, "ModuloAssign"},
    {TokenTypes::PowerAssign, "PowerAssign"},
    {TokenTypes::Equals, "Equals"},
    {TokenTypes::NotEquals, "NotEquals"},
    {TokenTypes::LessThan, "LessThan"},
    {TokenTypes::GreaterThan, "GreaterThan"},
    {TokenTypes::LessEqual, "LessEqual"},
    {TokenTypes::GreaterEqual, "GreaterEqual"},
    {TokenTypes::Spaceship, "Spaceship"},
    {TokenTypes::LogicalAnd, "LogicalAnd"},
    {TokenTypes::LogicalOr, "LogicalOr"},
    {TokenTypes::LogicalXor, "LogicalXor"},
    {TokenTypes::LogicalNot, "LogicalNot"},
    {TokenTypes::BitwiseAnd, "BitwiseAnd"},
    {TokenTypes::BitwiseOr, "BitwiseOr"},
    {TokenTypes::BitwiseXor, "BitwiseXor"},
    {TokenTypes::BitwiseAndAssign, "BitwiseAndAssign"},
    {TokenTypes::BitwiseOrAssign, "BitwiseOrAssign"},
    {TokenTypes::Tilde, "Tilde"},
    {TokenTypes::ShiftLeftAssign, "ShiftLeftAssign"},
    {TokenTypes::ShiftRightAssign, "ShiftRightAssign"},
    {TokenTypes::BitwiseXorAssign, "BitwiseXorAssign"},
    {TokenTypes::ShiftLeft, "ShiftLeft"},
    {TokenTypes::ShiftRight, "ShiftRight"},
    {TokenTypes::LeftParen, "LeftParen"},
    {TokenTypes::RightParen, "RightParen"},
    {TokenTypes::LeftBrace, "LeftBrace"},
    {TokenTypes::RightBrace, "RightBrace"},
    {TokenTypes::LeftBracket, "LeftBracket"},
    {TokenTypes::RightBracket, "RightBracket"},
    {TokenTypes::Semicolon, "Semicolon"},
    {TokenTypes::Newline, "Newline"},
    {TokenTypes::Comma, "Comma"},
    {TokenTypes::Dot, "Dot"},
    {TokenTypes::Colon, "Colon"},
    {TokenTypes::DoubleColon, "DoubleColon"},
    {TokenTypes::QuestionMark, "QuestionMark"},
    {TokenTypes::Ellipsis, "Ellipsis"},
    {TokenTypes::NullCoalescing, "NullCoalescing"},
    {TokenTypes::SafeNavigation, "SafeNavigation"},
    {TokenTypes::Lambda, "Lambda"},
    {TokenTypes::Pipe, "Pipe"},
    {TokenTypes::AtSymbol, "AtSymbol"},
    {TokenTypes::Hash, "Hash"},
    {TokenTypes::Dollar, "Dollar"}
};

// ... (previous function implementations unchanged, included for context)
std::string getTokenTypeName(TokenTypes type) {
    auto it = tokenTypeNames.find(type);
    return (it != tokenTypeNames.end()) ? it->second : "Unknown";
}

bool isBinaryOperator(TokenTypes tokenType) {
    switch (tokenType) {
        case TokenTypes::Plus:
        case TokenTypes::Minus:
        case TokenTypes::Multiply:
        case TokenTypes::Divide:
        case TokenTypes::Modulo:
        case TokenTypes::Power:
        case TokenTypes::FloorDivide:
        case TokenTypes::Equals:
        case TokenTypes::NotEquals:
        case TokenTypes::LessThan:
        case TokenTypes::GreaterThan:
        case TokenTypes::LessEqual:
        case TokenTypes::GreaterEqual:
        case TokenTypes::Spaceship:
        case TokenTypes::LogicalAnd:
        case TokenTypes::LogicalOr:
        case TokenTypes::LogicalXor:
        case TokenTypes::BitwiseAnd:
        case TokenTypes::BitwiseOr:
        case TokenTypes::BitwiseXor:
        case TokenTypes::ShiftLeft:
        case TokenTypes::ShiftRight:
        case TokenTypes::NullCoalescing:
        case TokenTypes::Pipe:
            return true;
        default:
            return false;
    }
}

bool isUnaryOperator(TokenTypes tokenType) {
    switch (tokenType) {
        case TokenTypes::Plus:
        case TokenTypes::Minus:
        case TokenTypes::Increment:
        case TokenTypes::Decrement:
        case TokenTypes::LogicalNot:
        case TokenTypes::Tilde:
            return true;
        default:
            return false;
    }
}

bool isAssignmentOperator(TokenTypes tokenType) {
    switch (tokenType) {
        case TokenTypes::Assign:
        case TokenTypes::PlusAssign:
        case TokenTypes::MinusAssign:
        case TokenTypes::MultiplyAssign:
        case TokenTypes::DivideAssign:
        case TokenTypes::ModuloAssign:
        case TokenTypes::PowerAssign:
        case TokenTypes::BitwiseAndAssign:
        case TokenTypes::BitwiseOrAssign:
        case TokenTypes::BitwiseXorAssign:
        case TokenTypes::ShiftLeftAssign:
        case TokenTypes::ShiftRightAssign:
            return true;
        default:
            return false;
    }
}

bool isComparisonOperator(TokenTypes tokenType) {
    switch (tokenType) {
        case TokenTypes::Equals:
        case TokenTypes::NotEquals:
        case TokenTypes::LessThan:
        case TokenTypes::GreaterThan:
        case TokenTypes::LessEqual:
        case TokenTypes::GreaterEqual:
        case TokenTypes::Spaceship:
            return true;
        default:
            return false;
    }
}

bool isLogicalOperator(TokenTypes tokenType) {
    switch (tokenType) {
        case TokenTypes::LogicalAnd:
        case TokenTypes::LogicalOr:
        case TokenTypes::LogicalXor:
        case TokenTypes::LogicalNot:
            return true;
        default:
            return false;
    }
}

bool isBitwiseOperator(TokenTypes tokenType) {
    switch (tokenType) {
        case TokenTypes::BitwiseAnd:
        case TokenTypes::BitwiseOr:
        case TokenTypes::BitwiseXor:
        case TokenTypes::Tilde:
        case TokenTypes::ShiftLeft:
        case TokenTypes::ShiftRight:
            return true;
        default:
            return false;
    }
}

bool isArithmeticOperator(TokenTypes tokenType) {
    switch (tokenType) {
        case TokenTypes::Plus:
        case TokenTypes::Minus:
        case TokenTypes::Multiply:
        case TokenTypes::Divide:
        case TokenTypes::Modulo:
        case TokenTypes::Power:
        case TokenTypes::FloorDivide:
            return true;
        default:
            return false;
    }
}

int getOperatorPrecedence(TokenTypes tokenType) {
    switch (tokenType) {
        case TokenTypes::Assign:
        case TokenTypes::PlusAssign:
        case TokenTypes::MinusAssign:
        case TokenTypes::MultiplyAssign:
        case TokenTypes::DivideAssign:
        case TokenTypes::ModuloAssign:
        case TokenTypes::PowerAssign:
        case TokenTypes::BitwiseAndAssign:
        case TokenTypes::BitwiseOrAssign:
        case TokenTypes::BitwiseXorAssign:
        case TokenTypes::ShiftLeftAssign:
        case TokenTypes::ShiftRightAssign:
            return 1;
        case TokenTypes::QuestionMark:
            return 2;
        case TokenTypes::NullCoalescing:
            return 3;
        case TokenTypes::LogicalOr:
            return 4;
        case TokenTypes::LogicalXor:
            return 5;
        case TokenTypes::LogicalAnd:
            return 6;
        case TokenTypes::BitwiseOr:
            return 7;
        case TokenTypes::BitwiseXor:
            return 8;
        case TokenTypes::BitwiseAnd:
            return 9;
        case TokenTypes::Equals:
        case TokenTypes::NotEquals:
        case TokenTypes::LessThan:
        case TokenTypes::GreaterThan:
        case TokenTypes::LessEqual:
        case TokenTypes::GreaterEqual:
        case TokenTypes::Spaceship:
            return 10;
        case TokenTypes::ShiftLeft:
        case TokenTypes::ShiftRight:
            return 11;
        case TokenTypes::Plus:
        case TokenTypes::Minus:
            return 12;
        case TokenTypes::Multiply:
        case TokenTypes::Divide:
        case TokenTypes::Modulo:
        case TokenTypes::FloorDivide:
            return 13;
        case TokenTypes::Power:
            return 14;
        case TokenTypes::Increment:
        case TokenTypes::Decrement:
        case TokenTypes::LogicalNot:
        case TokenTypes::Tilde:
            return 15;
        case TokenTypes::Dot:
        case TokenTypes::SafeNavigation:
        case TokenTypes::ScopeResolution:
        case TokenTypes::DoubleColon:
            return 16;
        case TokenTypes::LeftParen:
        case TokenTypes::LeftBracket:
            return 17;
        default:
            return 0;
    }
}

bool isRightAssociative(TokenTypes tokenType) {
    switch (tokenType) {
        case TokenTypes::Assign:
        case TokenTypes::PlusAssign:
        case TokenTypes::MinusAssign:
        case TokenTypes::MultiplyAssign:
        case TokenTypes::DivideAssign:
        case TokenTypes::ModuloAssign:
        case TokenTypes::PowerAssign:
        case TokenTypes::BitwiseAndAssign:
        case TokenTypes::BitwiseOrAssign:
        case TokenTypes::BitwiseXorAssign:
        case TokenTypes::ShiftLeftAssign:
        case TokenTypes::ShiftRightAssign:
        case TokenTypes::Power:
        case TokenTypes::QuestionMark:
            return true;
        default:
            return false;
    }
}

std::string Token::toString() const {
    std::stringstream ss;
    ss << getTokenTypeName(type_);
    if (!stringValue_.empty()) {
        ss << "(\"" << stringValue_ << "\")";
    }
    return ss.str();
}

std::string Token::toDebugString() const {
    std::stringstream ss;
    ss << "[" << getTokenTypeName(type_) << "]"
       << " Value: \"" << getValue() << "\""
       << " at " << filePath_ << ":" << line_ << ":" << column_;
    return ss.str();
}

std::ostream& operator<<(std::ostream& os, const Token& token) {
    os << token.toString();
    return os;
}

namespace TokenFactory {

Token createIdentifier(const std::string& name, size_t line, size_t column, const std::string& path) {
    Token token(TokenTypes::Identifier, std::string(name), line, column, path);
    token.setStringValue(name);
    return token;
}

Token createInteger(int64_t value, size_t line, size_t column, const std::string& path) {
    return Token(TokenTypes::IntegerLiteral, TokenValue(value), line, column, path);
}

Token createFloat(double value, size_t line, size_t column, const std::string& path) {
    return Token(TokenTypes::FloatLiteral, TokenValue(value), line, column, path);
}

Token createString(const std::string& value, size_t line, size_t column, const std::string& path) {
    Token token(TokenTypes::StringLiteral, TokenValue(value), line, column, path);
    token.setStringValue(value);
    return token;
}

Token createBoolean(bool value, size_t line, size_t column, const std::string& path) {
    return Token(value ? TokenTypes::True : TokenTypes::False, TokenValue(value), line, column, path);
}

Token createOperator(TokenTypes type, size_t line, size_t column, const std::string& path) {
    if (!isBinaryOperator(type) && !isUnaryOperator(type) && 
        !isAssignmentOperator(type) && !isComparisonOperator(type) &&
        !isLogicalOperator(type) && !isBitwiseOperator(type)) {
        Token token(TokenTypes::Invalid, std::string("Invalid operator"), line, column, path);
        token.setStringValue("Invalid operator");
        return token;
    }
    Token token(type, std::string(getTokenTypeName(type)), line, column, path);
    token.setStringValue(getTokenTypeName(type));
    return token;
}

Token createKeyword(TokenTypes type, size_t line, size_t column, const std::string& path) {
    if (type < TokenTypes::If || type > TokenTypes::Type) {
        Token token(TokenTypes::Invalid, std::string("Invalid keyword"), line, column, path);
        token.setStringValue("Invalid keyword");
        return token;
    }
    Token token(type, std::string(getTokenTypeName(type)), line, column, path);
    token.setStringValue(getTokenTypeName(type));
    return token;
}

Token createEOI(size_t line, size_t column, const std::string& path) {
    Token token(TokenTypes::EOI, std::string(""), line, column, path);
    token.setStringValue("");
    return token;
}

Token createInvalid(const std::string& value, size_t line, size_t column, const std::string& path) {
    Token token(TokenTypes::Invalid, std::string(value), line, column, path);
    token.setStringValue(value);
    return token;
}

} // namespace TokenFactory

} // namespace Omniscript
