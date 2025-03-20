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
    This,            // "this"
    Struct,           // "class" for creating new class
    Namespace,         // "namespace"
    Using,
    Public,          // "public"
    Private,         // "private"
    Override,        // "override"
    Virtual,        // "virtual"
    Static,         // "static"
    Final,         // "final"
    True,            // "true" literal
    False,           // "false" literal
    Null,            // "null" literal
    Enum,
    Extends,
    Variant,
    Any,
    Import,
    From,
    Module,
    As,

    // Literals
    Identifier,             // To identify Variable and function names
    Character,
    IntegerLiteral,          // Numeric literals (e.g., 123, 4.56)
    BigInt,
    BinaryLiteral,
    FloatLiteral,          // Numeric literals (e.g., 123, 4.56)
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
    QuestionMark            // "?" (for ternary operator)
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
        Token(TokenTypes type = TokenTypes::Invalid, const std::string &value="", int line = 0, int column = 0, int position = 0) :
        type(type), value(value), line(line), column(column), position(position) {};

        TokenTypes getType() const {return type;}
        std::string getValue() const {return value;}
        TokenTypes type;
        int getLine() {return line;}
        int getColumn() {return column;}
        int getPosition() {return position;}
    private:
        // TokenTypes type;
        std::string value;
        int line;
        int column;
        int position;
};

#endif