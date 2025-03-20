#include <omniscript/engine/tokens.h>

// Create a map from TokenTypes to string names so that you can get the name of an enum via its index
std::unordered_map<TokenTypes, std::string> tokenTypeNames = {
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
    {TokenTypes::Var, "var"},
    {TokenTypes::Const, "Const"},
    {TokenTypes::Namespace, "Namespace"},
    {TokenTypes::Using, "Using"},
    {TokenTypes::New, "New"},
    {TokenTypes::Delete, "Delete"},
    {TokenTypes::Class, "Class"},
    {TokenTypes::Struct, "Struct"},
    {TokenTypes::This, "This"},
    {TokenTypes::Public, "public"},
    {TokenTypes::Private, "Private"},
    {TokenTypes::Override, "Override"},
    {TokenTypes::Virtual, "Virtual"},
    {TokenTypes::Static, "Static"},
    {TokenTypes::Final, "Final"},
    {TokenTypes::True, "True"},
    {TokenTypes::False, "False"},
    {TokenTypes::Extends, "Extends"},
    {TokenTypes::Variant, "Variant"},
    {TokenTypes::Any, "Any"},
    {TokenTypes::Import, "Import"},
    {TokenTypes::From, "From"},
    {TokenTypes::Module, "Module"},
    {TokenTypes::As, "As"},
    {TokenTypes::Null, "Null"},
    {TokenTypes::Enum, "Enum"},
    {TokenTypes::Identifier, "Identifier"},
    {TokenTypes::IntegerLiteral, "IntegerLiteral"},
    {TokenTypes::FloatLiteral, "FloatLiteral"},
    {TokenTypes::BinaryLiteral, "BinaryLiteral"},
    {TokenTypes::OctalLiteral, "OctalLiteral"},
    {TokenTypes::HexLiteral, "HexLiteral"},
    {TokenTypes::StringLiteral, "StringLiteral"},
    {TokenTypes::Arrow, "Arrow"},
    {TokenTypes::ScopeResolution, "ScopeResolutionOperator"},
    {TokenTypes::Plus, "Plus"},
    {TokenTypes::Minus, "Minus"},
    {TokenTypes::Multiply, "Multiply"},
    {TokenTypes::Divide, "Divide"},
    {TokenTypes::Modulo, "Modulo"},
    {TokenTypes::Increment, "Increment"},
    {TokenTypes::Decrement, "Decrement"},
    {TokenTypes::FloorDivide, "FloorDivide"},
    {TokenTypes::Assign, "Assign"},
    {TokenTypes::PlusAssign, "PlusAssign"},
    {TokenTypes::MinusAssign, "MinusAssign"},
    {TokenTypes::MultiplyAssign, "MultiplyAssign"},
    {TokenTypes::DivideAssign, "DivideAssign"},
    {TokenTypes::Equals, "Equals"},
    {TokenTypes::NotEquals, "NotEquals"},
    {TokenTypes::LessThan, "LessThan"},
    {TokenTypes::GreaterThan, "GreaterThan"},
    {TokenTypes::LessEqual, "LessEqual"},
    {TokenTypes::GreaterEqual, "GreaterEqual"},
    {TokenTypes::LogicalAnd, "LogicalAnd"},
    {TokenTypes::LogicalOr, "LogicalOr"},
    {TokenTypes::LogicalXor, "LogicalXor"},
    {TokenTypes::LogicalNot, "LogicalNot"},
    {TokenTypes::BitwiseAnd, "BitwiseAnd"},
    {TokenTypes::BitwiseOr, "BitwiseOr"},
    {TokenTypes::BitwiseXor, "BitwiseXor"},
    {TokenTypes::BitwiseAndAssign, "BitwiseAnd"},
    {TokenTypes::BitwiseOrAssign, "BitwiseOr"},
    {TokenTypes::Tilde, "Tilde"},
    {TokenTypes::ShiftLeftAssign, "ShiftLeftAssign"},
    {TokenTypes::ShiftRightAssign, "ShiftRightAssign"},
    {TokenTypes::BitwiseXorAssign, "BitwiseXor"},
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
    {TokenTypes::QuestionMark, "QuestionMark"}
};

// Function to get the name of a TokenType
std::string getTokenTypeName(TokenTypes type) {
    return tokenTypeNames.count(type) ? tokenTypeNames[type] : "Unknown";
}

bool isBinaryOperator(TokenTypes tokenType) {
    switch (tokenType) {
        case TokenTypes::Plus:
        case TokenTypes::Minus:
        case TokenTypes::Multiply:
        case TokenTypes::Divide:
        case TokenTypes::Modulo:
        case TokenTypes::FloorDivide:
        case TokenTypes::NotEquals:
        case TokenTypes::LessThan:
        case TokenTypes::GreaterThan:
        case TokenTypes::LessEqual:
        case TokenTypes::GreaterEqual:
        case TokenTypes::LogicalAnd:
        case TokenTypes::LogicalOr:
        case TokenTypes::LogicalXor:
        case TokenTypes::BitwiseAnd:
        case TokenTypes::BitwiseOr:
        case TokenTypes::BitwiseXor:
        case TokenTypes::ShiftLeft:
        case TokenTypes::ShiftRight:
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
        case TokenTypes::Increment:
        case TokenTypes::Decrement:
            return true;
        default:
            return false;
    }
}
