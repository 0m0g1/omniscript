#pragma once

#include <omniscript/omniscript_pch.h>
#include <omniscript/Tokens.h>
#include <omniscript/utils.h>
#include <omniscript/Core.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <optional>
#include <memory>
#include <functional>

namespace Omniscript {

// Forward declarations
struct FileSpan;
struct LexerError;

// Lexer configuration struct
struct LexerConfig {
    bool trackNewlines = true;           // Whether to emit newline tokens
    bool allowRawStrings = true;         // Support for raw string literals
    bool allowTemplateStrings = true;    // Support for template strings
    bool allowBigInts = true;            // Support for BigInt literals
    bool allowBinaryLiterals = true;     // Support for 0b... literals
    bool allowOctalLiterals = true;      // Support for 0o... literals
    bool allowHexLiterals = true;        // Support for 0x... literals
    bool caseSensitiveKeywords = false;  // Whether keywords are case-sensitive
    bool allowUnicodeIdentifiers = true; // Support Unicode in identifiers
    bool allowNestedComments = true;     // Support nested /* */ comments
    size_t maxIdentifierLength = 1024;   // Maximum identifier length
    size_t maxStringLength = 65536;      // Maximum string literal length
    size_t maxCommentDepth = 32;         // Maximum nested comment depth
};

// Lexer error information
struct LexerError {
    std::string message;
    size_t line;
    size_t column;
    std::string filePath;
    std::string suggestion;
    
    LexerError(const std::string& msg, size_t ln, size_t col, 
               const std::string& path = "", const std::string& hint = "")
        : message(msg), line(ln), column(col), filePath(path), suggestion(hint) {}
};

// Enhanced Lexer class with better error handling and features
class Lexer {
public:
    // Constructor and destructor
    explicit Lexer(const std::string& source, const std::string& filePath = "", 
                   const LexerConfig& config = LexerConfig{});
    ~Lexer() = default;

    // Main tokenizing methods
    Token getNextToken();
    std::vector<Token> tokenizeAll();
    
    // Peek methods for lookahead
    Token peekToken(size_t offset = 1);
    std::vector<Token> peekTokens(size_t count);
    
    // Position and state management
    size_t getCurrentLine() const { return line_; }
    size_t getCurrentColumn() const { return column_; }
    size_t getCurrentPosition() const { return currentPosition_; }
    std::string getFilePath() const { return sourceFilePath_; }
    
    // Error handling
    std::vector<LexerError> getErrors() const { return errors_; }
    bool hasErrors() const { return !errors_.empty(); }
    void clearErrors() { errors_.clear(); }
    
    // State queries
    bool isAtEnd() const;
    bool isValid() const { return !source_.empty(); }
    double getProgress() const;
    
    // Configuration
    void setConfig(const LexerConfig& config) { config_ = config; }
    const LexerConfig& getConfig() const { return config_; }
    
    // Reset and rewind
    void reset();
    void seekTo(size_t position);
    void seekToLine(size_t line);
    
    // Advanced features
    void enableDebugMode(bool enable = true) { debugMode_ = enable; }
    void setErrorCallback(std::function<void(const LexerError&)> callback) { 
        errorCallback_ = callback; 
    }
    
    // Statistics
    struct LexerStats {
        size_t totalTokens = 0;
        size_t totalLines = 0;
        size_t totalCharacters = 0;
        size_t identifierCount = 0;
        size_t keywordCount = 0;
        size_t literalCount = 0;
        size_t operatorCount = 0;
        size_t commentCount = 0;
        std::unordered_map<TokenTypes, size_t> tokenFrequency;
    };
    
    LexerStats getStatistics() const { return stats_; }

private:
    // Core lexing methods
    void skipWhitespace();
    Token handleComments();
    Token handleSingleLineComment();
    Token handleMultiLineComment();
    Token parseIdentifierOrKeyword();
    Token handleSpecialKeywords(const std::string& identifier, size_t startLine, size_t startColumn);
    TokenTypes getKeywordType(const std::string& identifier);
    
    // String and character literal parsing
    Token getStringToken(char currentChar);
    Token parseStringLiteral();
    Token parseCharacterLiteral();
    Token parseRawString();
    Token parseTemplateString();
    
    // Numeric literal parsing
    Token getNumberLiterals(char currentChar);
    Token parseIntegerLiteral();
    Token parseFloatLiteral();
    Token parseBinaryLiteral();
    Token parseOctalLiteral();
    Token parseHexLiteral();
    Token parseBigIntLiteral();
    
    // Operator parsing
    Token getOperator(char currentChar);
    Token parseMultiCharOperator(char first, char second = '\0', char third = '\0');
    
    // Error handling and recovery
    Token handleUnexpectedCharacter(char currentChar);
    void reportError(const std::string& message, const std::string& suggestion = "");
    void reportErrorAt(const std::string& message, size_t line, size_t column, 
                      const std::string& suggestion = "");
    
    // Character and position utilities
    char getCurrentChar() const;
    char peek(int offset = 1) const;
    std::string peekString(size_t length) const;
    void advance(size_t count = 1);
    void advanceLine();
    void updatePositionFromToken(const Token& token);
    
    // Character classification
    bool isIdentifierStart(char c) const;
    bool isIdentifierContinuation(char c) const;
    bool isDigit(char c) const;
    bool isHexDigit(char c) const;
    bool isBinaryDigit(char c) const;
    bool isOctalDigit(char c) const;
    bool isWhitespace(char c) const;
    bool isNewline(char c) const;
    bool isQuote(char c) const;
    bool isRawStringStart(char currentChar) const;
    
    // Unicode support
    bool isUnicodeIdentifierStart(uint32_t codepoint) const;
    bool isUnicodeIdentifierContinuation(uint32_t codepoint) const;
    uint32_t parseUnicodeEscape();
    std::string utf32ToUtf8(uint32_t codepoint);
    
    // String processing utilities
    std::string unescapeString(const std::string& str);
    std::string escapeString(const std::string& str);
    bool isValidEscapeSequence(char c) const;
    
    // Error recovery methods
    void skipToNextStatement();
    void skipToToken(TokenTypes targetType);
    bool synchronize();
    void skipToMatchingBracket(TokenTypes openBracket);
    
    // Validation methods
    bool isValidIdentifier(const std::string& identifier) const;
    bool isValidNumber(const std::string& number) const;
    bool isValidStringLiteral(const std::string& literal) const;
    bool isReservedKeyword(const std::string& word) const;
    
    // Token caching for lookahead
    void cacheToken(const Token& token);
    Token getCachedToken(size_t offset);
    void clearTokenCache();
    
    // Debug and profiling
    void debugPrintToken(const Token& token) const;
    void debugPrintPosition() const;
    void updateStatistics(const Token& token);

private:
    // Core lexer state
    std::string source_;
    std::string sourceFilePath_;
    size_t currentPosition_;
    size_t line_;
    size_t column_;
    LexerConfig config_;
    
    // Token management
    std::optional<Token> pendingNewlineToken_;
    std::vector<Token> tokenCache_;
    size_t cachePosition_;
    
    // Error handling
    std::vector<LexerError> errors_;
    std::function<void(const LexerError&)> errorCallback_;
    bool debugMode_;
    
    // Statistics and profiling
    LexerStats stats_;
    
    // Keyword lookup table (static for performance)
    static const std::unordered_map<std::string, TokenTypes> keywordMap_;
    static const std::unordered_set<std::string> reservedWords_;
    
    // Character classification tables for performance
    static const bool isIdentifierStartTable_[256];
    static const bool isIdentifierContinuationTable_[256];
    static const bool isDigitTable_[256];
    static const bool isHexDigitTable_[256];
    static const bool isWhitespaceTable_[256];
};

// Utility namespace for lexer-related functions
namespace LexerUtils {
    // String utilities
    std::string escapeString(const std::string& str);
    std::string unescapeString(const std::string& str);
    std::string toLowerCaseString(const std::string& str);
    std::string toUpperCaseString(const std::string& str);
    
    // Character classification
    bool isWhitespace(char c);
    bool isNewline(char c);
    bool isIdentifierChar(char c);
    bool isDigitChar(char c);
    bool isHexDigitChar(char c);
    bool isBinaryDigitChar(char c);
    bool isOctalDigitChar(char c);
    
    // Validation utilities
    bool isValidStringEscape(char c);
    bool isValidIntegerSuffix(const std::string& suffix);
    bool isValidFloatSuffix(const std::string& suffix);
    bool isValidIdentifierName(const std::string& name);
    bool isReservedKeyword(const std::string& word);
    
    // Number parsing utilities
    int64_t parseInteger(const std::string& str, int base = 10);
    double parseFloat(const std::string& str);
    bool isValidNumber(const std::string& str);
    
    // Unicode utilities
    bool isValidUTF8(const std::string& str);
    std::u32string utf8ToUtf32(const std::string& str);
    std::string utf32ToUtf8(const std::u32string& str);
    uint32_t getUnicodeCategory(uint32_t codepoint);
    
    // File and path utilities
    std::string getFileExtension(const std::string& path);
    std::string getFileName(const std::string& path);
    std::string getDirectory(const std::string& path);
    bool isSourceFile(const std::string& path);
    
    // Performance utilities
    size_t hashString(const std::string& str);
    bool stringEquals(const std::string& a, const std::string& b);
    int stringCompare(const std::string& a, const std::string& b);
}

// Lexer iterator for range-based loops
class LexerIterator {
public:
    explicit LexerIterator(Lexer& lexer, bool atEnd = false);
    
    Token operator*();
    LexerIterator& operator++();
    LexerIterator operator++(int);
    bool operator==(const LexerIterator& other) const;
    bool operator!=(const LexerIterator& other) const;
    
private:
    Lexer* lexer_;
    Token currentToken_;
    bool atEnd_;
};

// Range adapter for lexer
class LexerRange {
public:
    explicit LexerRange(Lexer& lexer) : lexer_(lexer) {}
    
    LexerIterator begin() { return LexerIterator(lexer_); }
    LexerIterator end() { return LexerIterator(lexer_, true); }
    
private:
    Lexer& lexer_;
};

// Convenience function to create a range
inline LexerRange tokens(Lexer& lexer) {
    return LexerRange(lexer);
}

} // namespace Omniscript
