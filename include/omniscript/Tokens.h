#pragma once
#include <omniscript/Core.h>
#include <omniscript/omniscript_pch.h>
#include <string>
#include <unordered_map>
#include <variant>
#include <memory>

// Forward declarations
namespace Omniscript {

enum class TokenTypes {
    // Special tokens
    EOI = -1,               // End of Input / File
    Invalid,                // Token to represent an invalid or unknown token
    Error,                  // Token to represent an invalid or unknown token

    // Keywords
    If,              // "if" statement
    Else_if,         // "else if" statement
    Else,            // "else" statement
    While,           // "while" loop
    For,             // "for" loop
    Continue,        // "continue" break
    Break,           // "break" continue
    Return,          // "return" statement
    Function,        // "function" declaration
    FunctionCall,    // "call()" a function
    Let,             // "let" or "var" for variable declaration
    Var,             // "var" for creating variants
    Const,           // "const" for constants
    New,             // "new" for creating new objects
    Delete,
    Class,           // "class" for creating new class
    Struct,          // "struct" for creating new struct
    Namespace,       // "namespace"
    Using,
    Public,          // "public"
    Private,         // "private"
    Protected,
    Override,        // "override"
    Virtual,         // "virtual"
    Static,          // "static"
    Final,           // "final"
    True,            // "true" literal
    Nullptr,
    False,           // "false" literal
    Null,            // "null" literal
    Enum,
    Extends,
    Variant,
    Any,
    Import,
    Include,
    From,
    Module,
    Extern,
    Intrinsic,
    Volatile,
    As,
    Type,
    
    // Literals
    Identifier,             // Variable and function names
    Character,
    IntegerLiteral,         // Integer literals (e.g., 123)
    BigInt,
    BinaryLiteral,
    FloatLiteral,           // Float literals (e.g., 4.56)
    TemplateHead,           // String templates (e.g., `hello ${name}!`)
    TemplateMiddle,
    TemplateTail,
    TemplateLiteral,        // Template literal
    StringLiteral,          // String literals (e.g., "hello")
    HexLiteral,
    OctalLiteral,
    Arrow,                  // =>, ->, -->
    
    // Operators
    ScopeResolution,        // ::

    // Arithmetic Operators
    Plus,                   // "+"
    Minus,                  // "-"
    Multiply,               // "*"
    Divide,                 // "/"
    Modulo,                 // "%"
    FloorDivide,            // "//"
    Increment,              // ++
    Decrement,              // --
    Power,                  // ** (exponentiation)

    // Assignment Operators
    Assign,                 // "="
    PlusAssign,             // "+="
    MinusAssign,            // "-="
    MultiplyAssign,         // "*="
    DivideAssign,           // "/="
    ModuloAssign,           // "%="
    PowerAssign,            // "**="
    
    // Comparison Operators
    Equals,                 // "=="
    NotEquals,              // "!="
    LessThan,               // "<"
    GreaterThan,            // ">"
    LessEqual,              // "<="
    GreaterEqual,           // ">="
    Spaceship,              // "<=>" (three-way comparison)

    // Logical Operators
    LogicalAnd,             // "&&"
    LogicalOr,              // "||"
    LogicalXor,             // "^^" or "xor"
    LogicalNot,             // "!"

    // Bitwise Operators
    BitwiseAnd,             // "&"
    BitwiseOr,              // "|"
    BitwiseXor,             // "^"
    Tilde,                  // "~"
    ShiftLeft,              // "<<"
    ShiftRight,             // ">>"
    BitwiseXorAssign,       // "^="
    BitwiseAndAssign,       // "&="
    BitwiseOrAssign,        // "|="
    ShiftLeftAssign,        // "<<="
    ShiftRightAssign,       // ">>="

    // Punctuation
    LeftParen,              // "("
    RightParen,             // ")"
    LeftBrace,              // "{"
    RightBrace,             // "}"
    LeftBracket,            // "["
    RightBracket,           // "]"
    Semicolon,              // ";"
    Comma,                  // ","
    Dot,                    // "."
    Newline,                // Newline token (for line breaks)

    // Other
    Colon,                  // ":"
    DoubleColon,            // "::" (scope resolution)
    QuestionMark,           // "?" (for ternary operator)
    Ellipsis,               // "..."
    NullCoalescing,         // "??" (null coalescing)
    SafeNavigation,         // "?." (safe navigation)
    Lambda,                 // "=>" (lambda/arrow function)
    Pipe,                   // "|>" (pipe operator)
    AtSymbol,               // "@" (for decorators/attributes)
    Hash,                   // "#" (for preprocessor or comments)
    Dollar                  // "$" (for string interpolation)
};

// Token value variant type for strongly typed token values
using TokenValue = std::variant<
    std::monostate,         // No value
    std::string,            // String values
    std::u32string,         // Unicode string values
    int64_t,                // Integer values
    double,                 // Float values
    bool,                   // Boolean values
    char                    // Character values
>;

// Create a map from TokenTypes to string names
extern std::unordered_map<TokenTypes, std::string> tokenTypeNames;

// Function to get the name of a TokenType
std::string getTokenTypeName(TokenTypes type);

// Check if the current operator is a binary operator
bool isBinaryOperator(TokenTypes tokenType);
bool isUnaryOperator(TokenTypes tokenType);
bool isAssignmentOperator(TokenTypes tokenType);
bool isComparisonOperator(TokenTypes tokenType);
bool isLogicalOperator(TokenTypes tokenType);
bool isBitwiseOperator(TokenTypes tokenType);
bool isArithmeticOperator(TokenTypes tokenType);

// Token precedence for parsing
int getOperatorPrecedence(TokenTypes tokenType);
bool isRightAssociative(TokenTypes tokenType);

// Enhanced Token class with better functionality
class Token {
public:
    // Enhanced constructors
    Token(TokenTypes type = TokenTypes::Invalid, 
          const std::string& value = "", 
          size_t line = 0, 
          size_t column = 0, 
          const std::string& path = "")
        : type_(type), stringValue_(value), line_(line), column_(column), filePath_(path) {}

    Token(TokenTypes type, 
          const TokenValue& value, 
          size_t line = 0, 
          size_t column = 0, 
          const std::string& path = "")
        : type_(type), value_(value), line_(line), column_(column), filePath_(path) {}

    // Copy constructor and assignment
    Token(const Token& other) = default;
    Token& operator=(const Token& other) = default;

    // Move constructor and assignment
    Token(Token&& other) noexcept = default;
    Token& operator=(Token&& other) noexcept = default;

    // Getters
    TokenTypes getType() const { return type_; }
    std::string getValue() const { 
        if (std::holds_alternative<std::string>(value_)) {
            return std::get<std::string>(value_);
        }
        return stringValue_; 
    }
    
    const TokenValue& getVariantValue() const { return value_; }
    
    template<typename T>
    T getTypedValue() const {
        if (std::holds_alternative<T>(value_)) {
            return std::get<T>(value_);
        }
        throw std::runtime_error("Token value type mismatch");
    }

    // Specialized getters for common types
    int64_t getIntValue() const { return getTypedValue<int64_t>(); }
    double getFloatValue() const { return getTypedValue<double>(); }
    bool getBoolValue() const { return getTypedValue<bool>(); }
    char getCharValue() const { return getTypedValue<char>(); }
    std::string getStringValue() const { 
        if (std::holds_alternative<std::string>(value_)) {
            return getTypedValue<std::string>();
        }
        return stringValue_;
    }
    std::u32string getU32Value() const { return getTypedValue<std::u32string>(); }

    // Position getters
    size_t getLine() const { return line_; }
    size_t getColumn() const { return column_; }
    std::string getFilePath() const { return filePath_; }

    // Setters
    void setType(TokenTypes type) { type_ = type; }
    void setValue(const TokenValue& value) { value_ = value; }
    void setStringValue(const std::string& value) { 
        stringValue_ = value; 
        value_ = value;
    }
    void setU32Value(const std::u32string& value) { 
        value_ = value; 
    }
    void setPosition(size_t line, size_t column) { 
        line_ = line; 
        column_ = column; 
    }

    // Type checking methods (inline for performance)
    inline bool isValid() const { return type_ != TokenTypes::Invalid && type_ != TokenTypes::Error; }
    inline bool isEOI() const { return type_ == TokenTypes::EOI; }
    inline bool isKeyword() const { return isKeywordType(type_); }
    
    // Operator type checks
    inline bool isComparisonOperator() const {
        return type_ == TokenTypes::Equals ||
               type_ == TokenTypes::NotEquals ||
               type_ == TokenTypes::LessThan ||
               type_ == TokenTypes::GreaterThan ||
               type_ == TokenTypes::LessEqual ||
               type_ == TokenTypes::GreaterEqual ||
               type_ == TokenTypes::Spaceship;
    }

    inline bool isAssignmentOperator() const {
        return type_ == TokenTypes::Assign ||
               type_ == TokenTypes::PlusAssign ||
               type_ == TokenTypes::MinusAssign ||
               type_ == TokenTypes::MultiplyAssign ||
               type_ == TokenTypes::DivideAssign ||
               type_ == TokenTypes::ModuloAssign ||
               type_ == TokenTypes::PowerAssign ||
               type_ == TokenTypes::BitwiseAndAssign ||
               type_ == TokenTypes::BitwiseOrAssign ||
               type_ == TokenTypes::BitwiseXorAssign ||
               type_ == TokenTypes::ShiftLeftAssign ||
               type_ == TokenTypes::ShiftRightAssign;
    }

    inline bool isArithmeticOperator() const {
        return type_ == TokenTypes::Plus ||
               type_ == TokenTypes::Minus ||
               type_ == TokenTypes::Multiply ||
               type_ == TokenTypes::Divide ||
               type_ == TokenTypes::Modulo ||
               type_ == TokenTypes::Power ||
               type_ == TokenTypes::FloorDivide;
    }

    inline bool isUnaryOperator() const {
        return type_ == TokenTypes::Increment ||
               type_ == TokenTypes::Decrement ||
               type_ == TokenTypes::LogicalNot ||
               type_ == TokenTypes::Tilde ||
               type_ == TokenTypes::Plus ||
               type_ == TokenTypes::Minus;
    }

    inline bool isLogicalOperator() const {
        return type_ == TokenTypes::LogicalAnd ||
               type_ == TokenTypes::LogicalOr ||
               type_ == TokenTypes::LogicalXor ||
               type_ == TokenTypes::LogicalNot;
    }

    inline bool isBitwiseOperator() const {
        return type_ == TokenTypes::BitwiseAnd ||
               type_ == TokenTypes::BitwiseOr ||
               type_ == TokenTypes::BitwiseXor ||
               type_ == TokenTypes::Tilde ||
               type_ == TokenTypes::ShiftLeft ||
               type_ == TokenTypes::ShiftRight;
    }

    // Bracket type checks
    inline bool isOpenBracket() const {
        return type_ == TokenTypes::LeftParen ||
               type_ == TokenTypes::LeftBrace ||
               type_ == TokenTypes::LeftBracket;
    }

    inline bool isCloseBracket() const {
        return type_ == TokenTypes::RightParen ||
               type_ == TokenTypes::RightBrace ||
               type_ == TokenTypes::RightBracket;
    }

    // Get matching bracket
    TokenTypes getMatchingBracket() const {
        switch (type_) {
            case TokenTypes::LeftParen: return TokenTypes::RightParen;
            case TokenTypes::RightParen: return TokenTypes::LeftParen;
            case TokenTypes::LeftBrace: return TokenTypes::RightBrace;
            case TokenTypes::RightBrace: return TokenTypes::LeftBrace;
            case TokenTypes::LeftBracket: return TokenTypes::RightBracket;
            case TokenTypes::RightBracket: return TokenTypes::LeftBracket;
            default: return TokenTypes::Invalid;
        }
    }

    // Literal type checks
    inline bool isLiteral() const {
        return type_ == TokenTypes::IntegerLiteral ||
               type_ == TokenTypes::FloatLiteral ||
               type_ == TokenTypes::BinaryLiteral ||
               type_ == TokenTypes::OctalLiteral ||
               type_ == TokenTypes::HexLiteral ||
               type_ == TokenTypes::StringLiteral ||
               type_ == TokenTypes::Character ||
               type_ == TokenTypes::True ||
               type_ == TokenTypes::False ||
               type_ == TokenTypes::Null ||
               type_ == TokenTypes::Nullptr;
    }

    inline bool isNumericLiteral() const {
        return type_ == TokenTypes::IntegerLiteral ||
               type_ == TokenTypes::FloatLiteral ||
               type_ == TokenTypes::BinaryLiteral ||
               type_ == TokenTypes::OctalLiteral ||
               type_ == TokenTypes::HexLiteral ||
               type_ == TokenTypes::BigInt;
    }

    inline bool isBooleanLiteral() const {
        return type_ == TokenTypes::True || type_ == TokenTypes::False;
    }

    inline bool isNullLiteral() const {
        return type_ == TokenTypes::Null || type_ == TokenTypes::Nullptr;
    }

    // Identifier or keyword check
    inline bool isIdentifierOrKeyword() const {
        return type_ == TokenTypes::Identifier || isKeyword();
    }

    // Operator precedence and associativity
    int getPrecedence() const { return getOperatorPrecedence(type_); }
    bool isRightAssociative() const { return isRightAssociative(type_); }

    // Utility methods
    std::string toString() const;
    std::string toDebugString() const;
    
    // Equality operators
    bool operator==(const Token& other) const {
        return type_ == other.type_ && 
               getValue() == other.getValue() &&
               line_ == other.line_ && 
               column_ == other.column_;
    }
    
    bool operator!=(const Token& other) const {
        return !(*this == other);
    }

    // Stream output operator
    friend std::ostream& operator<<(std::ostream& os, const Token& token);

private:
    TokenTypes type_;
    TokenValue value_;           // Strongly typed value
    std::string stringValue_;    // Fallback string value for compatibility
    size_t line_;
    size_t column_;
    std::string filePath_;

    // Helper method to check if a TokenType is a keyword
    static bool isKeywordType(TokenTypes type) {
        return type >= TokenTypes::If && type <= TokenTypes::Type;
    }
};

// Token factory functions for convenience
namespace TokenFactory {
    Token createIdentifier(const std::string& name, size_t line = 0, size_t column = 0, const std::string& path = "");
    Token createInteger(int64_t value, size_t line = 0, size_t column = 0, const std::string& path = "");
    Token createFloat(double value, size_t line = 0, size_t column = 0, const std::string& path = "");
    Token createString(const std::string& value, size_t line = 0, size_t column = 0, const std::string& path = "");
    Token createBoolean(bool value, size_t line = 0, size_t column = 0, const std::string& path = "");
    Token createOperator(TokenTypes type, size_t line = 0, size_t column = 0, const std::string& path = "");
    Token createKeyword(TokenTypes type, size_t line = 0, size_t column = 0, const std::string& path = "");
    Token createEOI(size_t line = 0, size_t column = 0, const std::string& path = "");
    Token createInvalid(const std::string& value = "", size_t line = 0, size_t column = 0, const std::string& path = "");
}

} // namespace Omniscript
