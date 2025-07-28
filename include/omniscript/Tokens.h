//Holds the token class and TokenTypes enum

#ifndef Tokens_H
#define Tokens_H

// #include <string>
// #include <unordered_map>
#include <omniscript/omniscript_pch.h>

enum class TokenTypes {
    // Special tokens
    EOI = -1,               // End of Input / File
    Invalid,                // Token to represent an invalid or unknown token
    Error,                  // Token to represent an invalid or unknown token

    // Keyowords
    If,              // "if" statement
    Else_if,          // "else if" statement
    Else,            // "else" statement
    While,           // "while" loop
    For,             // "for" loop
    Continue,             // "for" break
    Break,             // "for" continue
    Return,          // "return" statement
    Function,        // "function" declaration
    FunctionCall,    // "call()" a function
    Let,             // "let" or "var" for variable declaration
    Var,             // "var" for creating variants
    Const,           // "const" for constants
    New,             // "new" for creating new objects
    Delete,
    Class,           // "class" for creating new class
    Struct,           // "class" for creating new class
    Namespace,         // "namespace"
    Using,
    Public,          // "public"
    Private,         // "private"
    Protected,
    Override,        // "override"
    Virtual,        // "virtual"
    Static,         // "static"
    Final,         // "final"
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
    Identifier,             // To identify Variable and function names
    Character,
    IntegerLiteral,          // Numeric literals (e.g., 123, 4.56)
    BigInt,
    BinaryLiteral,
    FloatLiteral,          // Numeric literals (e.g., 123, 4.56)
    TemplateHead,          // string templates (e.g., `hello ${name}!`)
    TemplateMiddle,
    TemplateTail,
    TemplateLiteral,          // String literals (e.g., "hello")
    StringLiteral,          // String literals (e.g., "hello")
    HexLiteral,
    OctalLiteral,
    Arrow,                  // =>, ->, -->
    
    // Operators
    ScopeResolution,

    // Arithmetic Operators
    Plus,                   // "+"
    Minus,                  // "-"
    Multiply,               // "*"
    Divide,                 // "/"
    Modulo,                 // "%"
    FloorDivide,             // "//"
    Increment,              //++
    Decrement,              //--

    // Assignment Operators
    Assign,                 // "="
    PlusAssign,             // "+="
    MinusAssign,            // "-="
    MultiplyAssign,         // "*="
    DivideAssign,           // "/="
    
    // Comparison Operators
    Equals,                 // "=="
    NotEquals,              // "!="
    LessThan,               // "<"
    GreaterThan,            // ">"
    LessEqual,              // "<="
    GreaterEqual,           // ">="

    // Logical Operators
    LogicalAnd,             // "&&"
    LogicalOr,              // "||"
    LogicalXor,              // "|^"
    LogicalNot,             // "!"

    // Bitwise Operators
    BitwiseAnd,             // "&"
    BitwiseOr,              // "|"
    BitwiseXor,             // "^"
    Tilde,             // "~"
    ShiftLeft,              // "<<"
    ShiftRight,             // ">>"
    BitwiseXorAssign,             // "^="
    BitwiseAndAssign,   // "&="
    BitwiseOrAssign,    // "|="
    ShiftLeftAssign,    // "<<="
    ShiftRightAssign,   // ">>="


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
    Newline,                //  Newline token (for line breaks)

    // Other
    Colon,                  // ":"
    QuestionMark,            // "?" (for ternary operator)
    Ellipsis               // "..."
};

// Create a map from TokenTypes to string names so that you can get the name of an enum via its index
extern std::unordered_map<TokenTypes, std::string> tokenTypeNames;

// Function to get the name of a TokenType
std::string getTokenTypeName(TokenTypes type);

// Check if the current operator is a binary operator like '+' '-' etc
bool isBinaryOperator(TokenTypes tokenType);

bool isAssignmentOperator(TokenTypes tokenType);

class Token {
    public:
        //Constructor
        Token(TokenTypes type = TokenTypes::Invalid, const std::string &value="", int line = 0, int column = 0, const std::string& path = "") :
        type(type), value(value), line(line), column(column), filePath(path) {};

        TokenTypes getType() const {return type;}
        std::string getValue() const {return value;}
        void setU32Value(std::u32string newValue) {
            u32Value = newValue;
        }
        std::u32string getU32Value() const {return u32Value;}
        TokenTypes type;
        int getLine() const {return line;}
        int getColumn() const {return column;}
        int getPosition() const {return position;}
        std::string getFilePath() const { return filePath; }

        // Comparison operators
        inline bool isComparisonOperator() {
            return type == TokenTypes::Equals ||
                type == TokenTypes::NotEquals ||
                type == TokenTypes::LessThan ||
                type == TokenTypes::GreaterThan ||
                type == TokenTypes::LessEqual ||
                type == TokenTypes::GreaterEqual;
        }

        // Assignment operators
        inline bool isAssignmentOperator() {
            return type == TokenTypes::Assign ||
                type == TokenTypes::PlusAssign ||
                type == TokenTypes::MinusAssign ||
                type == TokenTypes::MultiplyAssign ||
                type == TokenTypes::DivideAssign ||
                type == TokenTypes::BitwiseAndAssign ||
                type == TokenTypes::BitwiseOrAssign ||
                type == TokenTypes::BitwiseXorAssign ||
                type == TokenTypes::ShiftLeftAssign ||
                type == TokenTypes::ShiftRightAssign;
        }

        // Arithmetic operators
        inline bool isArithmeticOperator() {
            return type == TokenTypes::Plus ||
                type == TokenTypes::Minus ||
                type == TokenTypes::Multiply ||
                type == TokenTypes::Divide ||
                type == TokenTypes::Modulo ||
                type == TokenTypes::Increment ||
                type == TokenTypes::Decrement ||
                type == TokenTypes::FloorDivide;
        }

        // Logical operators
        inline bool isLogicalOperator() {
            return type == TokenTypes::LogicalAnd ||
                type == TokenTypes::LogicalOr ||
                type == TokenTypes::LogicalXor ||
                type == TokenTypes::LogicalNot;
        }

        // Bitwise operators
        inline bool isBitwiseOperator() {
            return type == TokenTypes::BitwiseAnd ||
                type == TokenTypes::BitwiseOr ||
                type == TokenTypes::BitwiseXor ||
                type == TokenTypes::Tilde ||
                type == TokenTypes::ShiftLeft ||
                type == TokenTypes::ShiftRight;
        }

        // Bracket types
        inline bool isOpenBracket() {
            return type == TokenTypes::LeftParen ||
                type == TokenTypes::LeftBrace ||
                type == TokenTypes::LeftBracket;
        }

        inline bool isCloseBracket() {
            return type == TokenTypes::RightParen ||
                type == TokenTypes::RightBrace ||
                type == TokenTypes::RightBracket;
        }

        // Literal types
        inline bool isLiteral() {
            return type == TokenTypes::IntegerLiteral ||
                type == TokenTypes::FloatLiteral ||
                type == TokenTypes::BinaryLiteral ||
                type == TokenTypes::OctalLiteral ||
                type == TokenTypes::HexLiteral ||
                type == TokenTypes::StringLiteral ||
                type == TokenTypes::Character ||
                type == TokenTypes::True ||
                type == TokenTypes::False ||
                type == TokenTypes::Null ||
                type == TokenTypes::Nullptr;
        }

        // Identifier or keyword
        inline bool isIdentifierOrKeyword() {
            return type == TokenTypes::Identifier ||
                type == TokenTypes::If ||
                type == TokenTypes::Else_if ||
                type == TokenTypes::Else ||
                type == TokenTypes::While ||
                type == TokenTypes::For ||
                type == TokenTypes::Return ||
                type == TokenTypes::Break ||
                type == TokenTypes::Continue ||
                type == TokenTypes::Function ||
                type == TokenTypes::Let ||
                type == TokenTypes::Var ||
                type == TokenTypes::Const;
        }

    private:
        // TokenTypes type;
        std::string value;
        std::u32string u32Value;
        int line;
        int column;
        int position;
        std::string filePath;
};

#endif