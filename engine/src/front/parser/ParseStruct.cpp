// engine/src/omniscript/parser/Struct.cpp
//
// Parses OS struct declarations:
//
//   struct Vec2 {
//       x: f32;
//       y: f32 = 0.0;
//       name: char*;
//   }
//
// Field syntax:   name : <type-tokens> [= expr] ;|newline
// Type tokens:    anything up to '=', ';', newline, or '}'

#include <omniscript/parser/Parser.h>

namespace Omniscript {

StmtPtr Parser::parseStructDeclaration() {
    // 'struct' keyword already consumed by match(TokenType::Struct)
    // in parseStatement().

    if (!check(TokenType::Identifier))
        throw ParseError("Expected struct name after 'struct'.", current());

    Token name = advance();

    eat(TokenType::LeftBrace, "Expected '{' after struct name.");

    std::vector<StructField> fields;

    while (!check(TokenType::RightBrace) && !check(TokenType::EndOfInput)) {
        while (match(TokenType::Newline)) {}
        if (check(TokenType::RightBrace)) break;

        if (!check(TokenType::Identifier))
            throw ParseError("Expected field name inside struct.", current());

        Token fieldName = advance();
        const FileSpan fieldStart = fieldName.span();

        eat(TokenType::Colon, "Expected ':' after field name.");

        // Collect raw type tokens — same pattern as FunctionDeclStmt::returnType
        std::vector<Token> typeToks;
        while (!check(TokenType::Assign)
            && !check(TokenType::Semicolon)
            && !check(TokenType::Newline)
            && !check(TokenType::RightBrace)
            && !check(TokenType::EndOfInput))
        {
            typeToks.push_back(advance());
        }

        if (typeToks.empty())
            throw ParseError("Expected type after ':' in struct field.", current());

        ExprPtr defaultVal;
        if (match(TokenType::Assign))
            defaultVal = parseExpression();

        FileSpan fieldSpan = defaultVal
            ? mergeSpans(fieldStart, defaultVal->span)
            : fieldStart;

        StructField field;
        field.name         = std::move(fieldName);
        field.typeToks     = std::move(typeToks);
        field.defaultValue = std::move(defaultVal);
        field.span         = fieldSpan;
        fields.push_back(std::move(field));

        match(TokenType::Semicolon);
        while (match(TokenType::Newline)) {}
    }

    const Token rbrace = current();
    eat(TokenType::RightBrace, "Expected '}' to close struct.");

    FileSpan span = mergeSpans(name.span(), rbrace.span());
    return std::make_unique<StructDeclStmt>(std::move(name), std::move(fields), span);
}

} // namespace Omniscript