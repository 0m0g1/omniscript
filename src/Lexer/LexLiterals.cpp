#include <omniscript/Lexer.h>
#include <omniscript/Tokens.h>
#include <omniscript/Console.h>
#include <algorithm>

namespace Omniscript {

Token Lexer::parseIdentifierOrKeyword() {
    size_t startLine = line_;
    size_t startColumn = column_;
    std::string rawIdentifier;
    bool tooLong = false;

    while (!isAtEnd() && isIdentifierContinuation(getCurrentChar())) {
        if (rawIdentifier.length() >= config_.maxIdentifierLength) {
            if (!tooLong) {
                console.reportError(Console::ErrorType::SYNTAX_ERROR,
                    "Identifier too long",
                    "Maximum identifier length is " + std::to_string(config_.maxIdentifierLength),
                    FileSpan{startLine, startColumn, line_, column_, sourceFilePath_});
                tooLong = true;
            }
            // Keep consuming so the rest of the name is not lexed as a second identifier.
            advance();
            continue;
        }
        rawIdentifier += getCurrentChar();
        advance();
    }

    if (rawIdentifier.empty()) {
        console.reportError(Console::ErrorType::SYNTAX_ERROR,
            "Empty identifier parsed",
            FileSpan{startLine, startColumn, line_, column_, sourceFilePath_});
        return TokenFactory::createInvalid("", startLine, startColumn, sourceFilePath_);
    }

    std::string keywordCheck = config_.caseSensitiveKeywords ? rawIdentifier : LexerUtils::toLowerCaseString(rawIdentifier);

    Token specialKeyword = handleSpecialKeywords(keywordCheck, startLine, startColumn);
    if (specialKeyword.getType() != TokenTypes::Invalid) {
        stats_.keywordCount++;
        return specialKeyword;
    }

    // "type" is only a keyword when a name follows (type Foo = ...);
    // otherwise it stays usable as an ordinary identifier.
    bool contextualIdentifier = (keywordCheck == "type") && peekToken(1).getType() != TokenTypes::Identifier;

    if (!contextualIdentifier) {
        TokenTypes tokenType = getKeywordType(keywordCheck);
        if (tokenType != TokenTypes::Invalid) {
            stats_.keywordCount++;
            Token keywordToken = TokenFactory::createKeyword(tokenType, startLine, startColumn, sourceFilePath_);
            keywordToken.setStringValue(rawIdentifier); // keep the word the user typed (mod vs module)
            return keywordToken;
        }
    }

    if (!isValidIdentifier(rawIdentifier)) {
        console.reportError(Console::ErrorType::SYNTAX_ERROR,
            "Invalid identifier: " + rawIdentifier,
            "Identifiers must start with a letter or underscore and contain only letters, digits, or underscores",
            FileSpan{startLine, startColumn, line_, column_, sourceFilePath_});
        return TokenFactory::createInvalid(rawIdentifier, startLine, startColumn, sourceFilePath_);
    }

    stats_.identifierCount++;
    return TokenFactory::createIdentifier(rawIdentifier, startLine, startColumn, sourceFilePath_);
}

// Handles keywords that need lookahead. Returns an Invalid token when the
// identifier is not one of them; the caller counts keywordCount for real hits.
Token Lexer::handleSpecialKeywords(const std::string& identifier, size_t startLine, size_t startColumn) {
    if (identifier == "else") {
        size_t savedPosition = currentPosition_;
        size_t savedLine = line_;
        size_t savedColumn = column_;
        std::optional<Token> savedPendingNewline = pendingNewlineToken_;
        LexerStats savedStats = stats_;

        skipWhitespace();

        std::string next = peekString(2);
        if (!config_.caseSensitiveKeywords) {
            next = LexerUtils::toLowerCaseString(next);
        }
        // "else if" but not "else iffy" / "else if_x"
        if (next == "if" && !isIdentifierContinuation(peek(2))) {
            advance(2);
            return TokenFactory::createKeyword(TokenTypes::Else_if, startLine, startColumn, sourceFilePath_);
        }

        // Plain else: restore exactly (no O(n) seekTo, stats untouched)
        currentPosition_ = savedPosition;
        line_ = savedLine;
        column_ = savedColumn;
        pendingNewlineToken_ = savedPendingNewline;
        stats_ = savedStats;
        return TokenFactory::createKeyword(TokenTypes::Else, startLine, startColumn, sourceFilePath_);
    }
    return TokenFactory::createInvalid("", startLine, startColumn, sourceFilePath_);
}

TokenTypes Lexer::getKeywordType(const std::string& identifier) {
    auto it = keywordMap_.find(identifier);
    return it != keywordMap_.end() ? it->second : TokenTypes::Invalid;
}

} // namespace Omniscript