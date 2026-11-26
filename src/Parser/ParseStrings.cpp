#include <omniscript/Core.h>
#include <omniscript/utils.h>
#include <omniscript/Lexer.h>
#include <omniscript/Tokens.h>
#include <omniscript/Parser.h>
#include <omniscript/Statements/Statement.h>
#include <omniscript/Symboltable.h>
#include <omniscript/Statements/LiteralStatements.h>
#include <omniscript/omniscript_pch.h>

namespace Omniscript {

std::u32string Parser::parseStringLiteral() {
    FileSpan span;
    span.start.line = currentToken.getLine();
    span.start.col = currentToken.getColumn();
    span.start.filePath = currentToken.getFilePath();

    std::u32string value = currentToken.getU32Value();

    eat(TokenTypes::StringLiteral, [&]() {
        std::string suggestion = Console::formatString(
            "To resolve this:\n"
            "1. Provide a valid string literal\n"
            "2. Check string syntax\n"
            "3. Expected token: string literal, found '%s'",
            getTokenTypeName(currentToken.getType()).c_str()
        );
        console.reportError(
            Console::SYNTAX_ERROR,
            Console::formatString("Expected string literal, found '%s'", 
                getTokenTypeName(currentToken.getType()).c_str()),
            suggestion,
            span
        );
    });

    // Fold "a" + "b" / "a" + 5 at parse time only when a literal follows the '+'.
    // Anything else ("a" + name, "a" + f(x)) is left for the expression parser's binary '+'.
    while (currentToken.getType() == TokenTypes::Plus) {
        TokenTypes next = lexer.peekToken().getType();   // ADAPT: your one-token lookahead
        if (next != TokenTypes::StringLiteral &&
            next != TokenTypes::IntegerLiteral &&
            next != TokenTypes::FloatLiteral) {
            break;
        }

        eat(TokenTypes::Plus);

        if (currentToken.getType() == TokenTypes::StringLiteral) {
            value += currentToken.getU32Value();
            eat(TokenTypes::StringLiteral);
        } else if (currentToken.getType() == TokenTypes::IntegerLiteral) {
            value += utf8_to_utf32(currentToken.getValue());
            eat(TokenTypes::IntegerLiteral);
        } else {
            value += utf8_to_utf32(currentToken.getValue());
            eat(TokenTypes::FloatLiteral);
        }
    }
    span.end.line = previousToken.getLine();
    span.end.col = previousToken.getColumn();
    span.end.filePath = previousToken.getFilePath();

    DEBUG_LOG("Parsed string literal: " + utf32_to_utf8(value));

    return value;
}

std::shared_ptr<Statement> Parser::parseStringTemplate() {
    FileSpan span;
    span.start.line = currentToken.getLine();
    span.start.col = currentToken.getColumn();
    span.start.filePath = currentToken.getFilePath();

    std::vector<std::shared_ptr<Statement>> parts;

    // Token shape from the lexer: Head(text) expr [Middle(text) expr]* Tail(text).
    // A template without any `${}` is a single Tail and never reaches this function.
    auto addText = [&]() {
        std::u32string text = currentToken.getU32Value();
        if (!text.empty()) {
            auto lit = std::make_shared<StringLiteral>(text);
            lit->setSpan(span);
            parts.push_back(lit);
        }
    };

    if (currentToken.getType() != TokenTypes::TemplateHead) {
        console.reportError(
            Console::SYNTAX_ERROR,
            Console::formatString("Expected template head to start template, found '%s'",
                getTokenTypeName(currentToken.getType()).c_str()),
            "To resolve this:\n1. Start the template with a backtick string\n2. Check template syntax",
            span
        );
        return nullptr;
    }

    addText();
    eat(TokenTypes::TemplateHead);

    while (true) {
        auto expr = parseExpression();
        if (!expr) {
            console.reportError(
                Console::SYNTAX_ERROR,
                "Invalid expression in template ${}",
                "To resolve this:\n1. Provide a valid expression within ${}\n2. Check expression syntax",
                span
            );
            return nullptr;
        }
        expr->setSpan(span);
        parts.push_back(expr);

        if (currentToken.getType() == TokenTypes::TemplateMiddle) {
            addText();
            eat(TokenTypes::TemplateMiddle);
            continue;
        }
        if (currentToken.getType() == TokenTypes::TemplateTail) {
            addText();
            eat(TokenTypes::TemplateTail);
            break;
        }
        console.reportError(
            Console::SYNTAX_ERROR,
            Console::formatString("Expected '}' closing the template expression, found '%s'",
                getTokenTypeName(currentToken.getType()).c_str()),
            "To resolve this:\n1. Close the ${ } expression with '}'\n2. Check the expression inside ${ }",
            span
        );
        return nullptr;
    }

    span.end.line = previousToken.getLine();
    span.end.col = previousToken.getColumn();
    span.end.filePath = previousToken.getFilePath();

    auto templateStmt = std::make_shared<StringTemplateStatement>(parts);
    templateStmt->setSpan(span);
    DEBUG_LOG("Parsed string template with " + std::to_string(parts.size()) + " parts");
    return templateStmt;
}

} // namespace Omniscript
