#include <omniscript/parser/Parser.h>

namespace Omniscript {


IdentifierPath Parser::parseIdentifiers(const char* contextMsg) {
    if (!check(TokenType::Identifier))
        throw ParseError(contextMsg, m_current_token);

    IdentifierPath out;

    // first identifier
    out.parts.push_back(advance());
    out.span = out.parts.back().span();

    // more: ('.' | '::') Identifier
    while (check(TokenType::Dot) || check(TokenType::ScopeResolution)) {
        TokenType sepType = m_current_token.type();
        advance(); // consume '.' or '::'
        out.seps.push_back(sepType);

        if (!check(TokenType::Identifier))
            throw ParseError("Expected identifier after '.' or '::'.", m_current_token);

        out.parts.push_back(advance());
        out.span = mergeSpans(out.span, out.parts.back().span());
    }

    return out;
}
    
std::vector<Token> Parser::parseType() {
    std::vector<Token> tks;

    // support "..." as a special type (variadic marker) if you want
    if (check(TokenType::Ellipsis)) {
        tks.push_back(advance());
        return tks;
    }

    IdentifierPath path = parseIdentifiers("Expected type name.");

    // store only identifiers for now (you can reconstruct seps later if needed)
    for (auto& p : path.parts) tks.push_back(std::move(p));

    while (check(TokenType::Star)) {
        tks.push_back(advance());
    }

    return tks;
}

} // namespace Omniscript