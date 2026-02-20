#include <omniscript/lexer/Lexer.h>
#include <cctype>
#include <unordered_map>

namespace Omniscript {

static TokenType keywordType(const std::string& s) {
    static const std::unordered_map<std::string, TokenType> k = {
        // control flow
        {"if", TokenType::If},
        {"elseif", TokenType::ElseIf},
        {"else", TokenType::Else},
        {"while", TokenType::While},
        {"for", TokenType::For},
        {"continue", TokenType::Continue},
        {"break", TokenType::Break},
        {"return", TokenType::Return},

        // functions / decl
        {"function", TokenType::Function},
        {"fn", TokenType::Function},          // alias from older
        {"let", TokenType::Let},
        {"var", TokenType::Var},              // you already distinguish Var; older mapped var->Let
        {"const", TokenType::Const},

        // types / OOP / modules
        {"class", TokenType::Class},
        {"struct", TokenType::Struct},
        {"enum", TokenType::Enum},            // older
        {"extends", TokenType::Extends},      // older
        {"implements", TokenType::Implements},// older reservedWords_
        {"namespace", TokenType::Namespace},
        {"using", TokenType::Using},

        // access
        {"public", TokenType::Public},
        {"private", TokenType::Private},
        {"protected", TokenType::Protected},

        // modifiers / linkage
        {"static", TokenType::Static},
        {"virtual", TokenType::Virtual},
        {"override", TokenType::Override},
        {"final", TokenType::Final},
        {"extern", TokenType::Extern},        // older
        {"intrinsic", TokenType::Intrinsic},  // older
        {"volatile", TokenType::Volatile},    // older

        // literals
        {"true", TokenType::True},
        {"false", TokenType::False},
        {"null", TokenType::Null},
        {"nullptr", TokenType::Nullptr},

        // operators / logical
        {"xor", TokenType::LogicalXor},       // older

        // allocation
        {"new", TokenType::New},              // older
        {"delete", TokenType::Delete},        // older

        // module/import system
        {"import", TokenType::Import},
        {"export", TokenType::Export},        // older reservedWords_
        {"include", TokenType::Include},
        {"from", TokenType::From},
        {"module", TokenType::Module},
        {"mod", TokenType::Module},           // alias from older

        // typing / casts
        {"as", TokenType::As},
        {"type", TokenType::Type},

        // misc reserved words from older list
        {"super", TokenType::Super},
        {"async", TokenType::Async},
        {"await", TokenType::Await},
        {"yield", TokenType::Yield},
        {"typeof", TokenType::Typeof},
        {"instanceof", TokenType::Instanceof},

        // older keywordMap_ had these too
        {"variant", TokenType::Variant},
        {"any", TokenType::Any},

        // (optional) older comment: "this" todo
        // {"this", TokenType::This},
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
