#include <omniscript/Lexer.h>
#include <omniscript/Tokens.h>
#include <omniscript/Console.h>
#include <algorithm>

namespace Omniscript {

Token Lexer::parseIdentifierOrKeyword() {
    size_t startLine = line_;
    size_t startColumn = column_;
    std::string rawIdentifier;

    while (!isAtEnd() && isIdentifierContinuation(getCurrentChar())) {
        if (rawIdentifier.length() >= config_.maxIdentifierLength) {
            console.reportError(Console::ErrorType::SYNTAX_ERROR,
                "Identifier too long",
                "Maximum identifier length is " + std::to_string(config_.maxIdentifierLength),
                FileSpan{startLine, startColumn, line_, column_, sourceFilePath_});
            break;
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

    TokenTypes tokenType = getKeywordType(keywordCheck);
    if (tokenType != TokenTypes::Invalid) {
        stats_.keywordCount++;
        return TokenFactory::createKeyword(tokenType, startLine, startColumn, sourceFilePath_);
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

Token Lexer::handleSpecialKeywords(const std::string& identifier, size_t startLine, size_t startColumn) {
    if (identifier == "else") {
        size_t savedPosition = currentPosition_;
        size_t savedLine = line_;
        size_t savedColumn = column_;
        skipWhitespace();
        if (peekString(2) == "if") {
            advance(2);
            stats_.keywordCount++;
            return TokenFactory::createKeyword(TokenTypes::Else_if, startLine, startColumn, sourceFilePath_);
        }
        seekTo(savedPosition);
        line_ = savedLine;
        column_ = savedColumn;
        return TokenFactory::createKeyword(TokenTypes::Else, startLine, startColumn, sourceFilePath_);
    }
    if (identifier == "type" && peekToken(1).getType() == TokenTypes::Identifier) {
        return TokenFactory::createKeyword(TokenTypes::Type, startLine, startColumn, sourceFilePath_);
    }
    return TokenFactory::createInvalid("", startLine, startColumn, sourceFilePath_);
}

TokenTypes Lexer::getKeywordType(const std::string& identifier) {
    auto it = keywordMap_.find(identifier);
    return it != keywordMap_.end() ? it->second : TokenTypes::Invalid;
}

} // namespace Omniscript