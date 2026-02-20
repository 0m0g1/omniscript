#pragma once

#include <cstddef>
#include <cstdint>
#include <ostream>
#include <string>

#include <omniscript/FileSpan.h> // adjust include path/name as needed

namespace Omniscript {

enum class TokenType : std::int32_t {
    // Special
    EndOfInput = 0,
    Invalid,
    Error,

    // Keywords (control flow)
    If,
    ElseIf,
    Else,
    While,
    For,
    Continue,
    Break,
    Return,

    // Keywords (functions / decl)
    Function,
    Let,
    Var,
    Const,

    // Keywords (allocation)
    New,
    Delete,

    // Keywords (types / OOP)
    Class,
    Struct,
    Enum,
    Extends,
    Implements,   // NEW: from older reservedWords_
    Namespace,
    Using,
    Variant,
    Any,

    // Keywords (access / modifiers / linkage)
    Public,
    Private,
    Protected,
    Static,
    Virtual,
    Override,
    Final,
    Extern,       // from older
    Intrinsic,    // from older
    Volatile,     // from older

    // Keywords (modules)
    Import,
    Export,       // NEW: from older reservedWords_
    Include,
    From,
    Module,

    // Keywords (misc)
    As,
    Type,
    Super,        // NEW: from older reservedWords_
    Async,        // NEW: from older reservedWords_
    Await,        // NEW: from older reservedWords_
    Yield,        // NEW: from older reservedWords_
    Typeof,       // NEW: from older reservedWords_
    Instanceof,   // NEW: from older reservedWords_

    // Keywords (literals)
    True,
    False,
    Null,
    Nullptr,

    // Identifiers & literals
    Identifier,
    CharacterLiteral,
    IntegerLiteral,
    BigIntLiteral,
    BinaryLiteral,
    OctalLiteral,
    HexLiteral,
    FloatLiteral,
    StringLiteral,

    // Template literals (optional)
    TemplateHead,
    TemplateMiddle,
    TemplateTail,
    TemplateLiteral,

    // Operators / punctuation
    ScopeResolution,    // ::

    Plus, Minus, Star, Slash, Percent,
    FloorDivide,        // //
    Increment,          // ++
    Decrement,          // --
    Power,              // **

    Assign, PlusAssign, MinusAssign, StarAssign, SlashAssign, PercentAssign, PowerAssign,

    Equals, NotEquals, Less, Greater, LessEqual, GreaterEqual, Spaceship,

    LogicalAnd, LogicalOr, LogicalXor, LogicalNot,

    BitAnd, BitOr, BitXor, BitNot,
    ShiftLeft, ShiftRight,
    BitAndAssign, BitOrAssign, BitXorAssign,
    ShiftLeftAssign, ShiftRightAssign,

    LeftParen, RightParen,
    LeftBrace, RightBrace,
    LeftBracket, RightBracket,

    Semicolon, Comma, Dot, Colon,
    QuestionMark, Ellipsis,

    NullCoalescing,     // ??
    SafeNavigation,     // ?.
    Arrow,              // =>
    Pipe,               // |>
    At, Hash, Dollar,

    Newline
};

enum class Assoc : std::uint8_t { Left, Right };

class Token {
public:
    Token() = default;
    explicit Token(TokenType type);

    // Primary constructor using a span (preferred)
    Token(TokenType type,
          std::string lexeme,
          std::string value,
          FileSpan span = {});

    // Convenience ctor: single-point token location (span start=end)
    Token(TokenType type,
          std::string lexeme,
          std::string value,
          std::size_t line,
          std::size_t column,
          std::string filePath = {});

    // Convenience ctor: explicit start/end range in same file
    Token(TokenType type,
          std::string lexeme,
          std::string value,
          std::size_t sLine, std::size_t sCol,
          std::size_t eLine, std::size_t eCol,
          std::string filePath = {});

    // ----- getters -----
    TokenType type() const noexcept { return m_type; }
    const std::string& lexeme() const noexcept { return m_lexeme; }
    const std::string& value() const noexcept { return m_value; }

    // Span-based location API (preferred)
    const FileSpan& span() const noexcept { return m_span; }
    void setSpan(FileSpan s) { m_span = std::move(s); }

    // Back-compat style convenience getters
    std::size_t line() const noexcept { return m_span.start.line; }
    std::size_t column() const noexcept { return m_span.start.col; }
    const std::string& filePath() const noexcept { return m_span.start.filePath; }

    // ----- setters -----
    void setType(TokenType t) noexcept { m_type = t; }
    void setLexeme(std::string s) { m_lexeme = std::move(s); }
    void setValue(std::string s) { m_value = std::move(s); }

    // Convenience: mutate span start (and keep file consistent on end)
    void setLine(std::size_t v) noexcept { m_span.start.line = v; }
    void setColumn(std::size_t v) noexcept { m_span.start.col = v; }
    void setFilePath(std::string s) {
        m_span.start.filePath = std::move(s);
        m_span.end.filePath = m_span.start.filePath;
    }

    // ----- quick checks -----
    bool isValid() const noexcept { return m_type != TokenType::Invalid && m_type != TokenType::Error; }
    bool isEndOfInput() const noexcept { return m_type == TokenType::EndOfInput; }

    bool isKeyword() const noexcept;
    bool isLiteral() const noexcept;
    bool isNumericLiteral() const noexcept;
    bool isIdentifierOrKeyword() const noexcept { return m_type == TokenType::Identifier || isKeyword(); }

    bool isOpenBracket() const noexcept;
    bool isCloseBracket() const noexcept;
    TokenType matchingBracket() const noexcept;

    bool isComparisonOperator() const noexcept;
    bool isAssignmentOperator() const noexcept;
    bool isArithmeticOperator() const noexcept;
    bool isUnaryOperator() const noexcept;
    bool isLogicalOperator() const noexcept;
    bool isBitwiseOperator() const noexcept;

    int precedence() const noexcept;
    Assoc associativity() const noexcept;
    bool isRightAssociative() const noexcept { return associativity() == Assoc::Right; }

    std::string toString() const;
    std::string toDebugString() const;

    bool operator==(const Token& other) const noexcept;
    bool operator!=(const Token& other) const noexcept { return !(*this == other); }

private:
    TokenType m_type { TokenType::Invalid };
    std::string m_lexeme {};     // raw source substring (exact characters)
    std::string m_value {};      // interpreted value (decoded string / normalized number / etc.)
    FileSpan m_span {};          // start/end position (preferred over separate line/col/path)
};

const char* tokenTypeName(TokenType t) noexcept;
std::ostream& operator<<(std::ostream& os, const Token& token);

} // namespace Omniscript