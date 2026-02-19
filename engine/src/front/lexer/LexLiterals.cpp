#include <omniscript/lexer/Lexer.h>
#include <cctype>
#include <unordered_map>

namespace Omniscript {

static TokenType keywordType(const std::string& s) {
    static const std::unordered_map<std::string, TokenType> k = {
        {"if", TokenType::If},
        {"elseif", TokenType::ElseIf},
        {"else", TokenType::Else},
        {"while", TokenType::While},
        {"for", TokenType::For},
        {"continue", TokenType::Continue},
        {"break", TokenType::Break},
        {"return", TokenType::Return},

        {"function", TokenType::Function},
        {"let", TokenType::Let},
        {"var", TokenType::Var},
        {"const", TokenType::Const},

        {"class", TokenType::Class},
        {"struct", TokenType::Struct},
        {"namespace", TokenType::Namespace},
        {"using", TokenType::Using},

        {"public", TokenType::Public},
        {"private", TokenType::Private},
        {"protected", TokenType::Protected},

        {"static", TokenType::Static},
        {"virtual", TokenType::Virtual},
        {"override", TokenType::Override},
        {"final", TokenType::Final},

        {"true", TokenType::True},
        {"false", TokenType::False},
        {"null", TokenType::Null},
        {"nullptr", TokenType::Nullptr},

        {"import", TokenType::Import},
        {"include", TokenType::Include},
        {"from", TokenType::From},
        {"module", TokenType::Module},

        {"as", TokenType::As},
        {"type", TokenType::Type},
        // add remaining keywords as needed
    };

    auto it = k.find(s);
    return (it == k.end()) ? TokenType::Identifier : it->second;
}

Token Lexer::tokenizeIdentifierOrKeyword() {
    const std::size_t startPos = m_current_position;
    const std::size_t sLine = m_line;
    const std::size_t sCol  = m_col;

    while (!atEnd()) {
        const char c = current();
        if (std::isalnum(static_cast<unsigned char>(c)) || c == '_')
            advance();
        else
            break;
    }

    const std::size_t endPos = m_current_position;
    std::string text = m_source.substr(startPos, endPos - startPos);

    const TokenType tt = keywordType(text);
    return makeToken(tt, sLine, sCol, startPos, endPos, text);
}

} // namespace Omniscript
