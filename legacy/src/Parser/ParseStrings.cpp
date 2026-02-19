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

    while (currentToken.getType() == TokenTypes::Plus) {
        eat(TokenTypes::Plus, [&]() {
            std::string suggestion = Console::formatString(
                "To resolve this:\n"
                "1. Use '+' for string concatenation\n"
                "2. Check string concatenation syntax\n"
                "3. Expected token: '+', found '%s'",
                getTokenTypeName(currentToken.getType()).c_str()
            );
            console.reportError(
                Console::SYNTAX_ERROR,
                Console::formatString("Expected '+' for string concatenation, found '%s'", 
                    getTokenTypeName(currentToken.getType()).c_str()),
                suggestion,
                span
            );
        });
        
        if (currentToken.getType() == TokenTypes::StringLiteral) {
            value += currentToken.getU32Value();
            eat(TokenTypes::StringLiteral);
        } else if (currentToken.getType() == TokenTypes::IntegerLiteral) {
            value += utf8_to_utf32(currentToken.getValue());
            eat(TokenTypes::IntegerLiteral);
        } else if (currentToken.getType() == TokenTypes::FloatLiteral) {
            value += utf8_to_utf32(currentToken.getValue());
            eat(TokenTypes::FloatLiteral);
        } else {
            console.reportError(
                Console::SYNTAX_ERROR,
                Console::formatString("Expected string, integer, or float literal after '+', found '%s'", 
                    getTokenTypeName(currentToken.getType()).c_str()),
                "To resolve this:\n1. Provide a valid literal for concatenation\n2. Check concatenation syntax\n3. Ensure valid string, integer, or float literal",
                span
            );
            break;
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

    // Handle TemplateHead or StringLiteral
    if (currentToken.getType() == TokenTypes::TemplateHead || currentToken.getType() == TokenTypes::StringLiteral) {
        std::u32string literalValue = currentToken.getU32Value();
        auto literalStmt = std::make_shared<StringLiteral>(literalValue);
        literalStmt->setPosition(currentToken, currentToken);
        literalStmt->setSpan(span);
        parts.push_back(literalStmt);
        eat(currentToken.getType(), [&]() {
            std::string suggestion = Console::formatString(
                "To resolve this:\n"
                "1. Start template with a string literal or template head\n"
                "2. Check template syntax\n"
                "3. Expected token: string literal or template head, found '%s'",
                getTokenTypeName(currentToken.getType()).c_str()
            );
            console.reportError(
                Console::SYNTAX_ERROR,
                Console::formatString("Expected string literal or template head, found '%s'", 
                    getTokenTypeName(currentToken.getType()).c_str()),
                suggestion,
                span
            );
        });
    } else {
        console.reportError(
            Console::SYNTAX_ERROR,
            Console::formatString("Expected template head or string literal to start template, found '%s'", 
                getTokenTypeName(currentToken.getType()).c_str()),
            "To resolve this:\n1. Start template with a string literal or backtick template\n2. Check template syntax\n3. Ensure valid template start",
            span
        );
        return nullptr;
    }

    // Handle TemplateMiddle and embedded expressions
    while (currentToken.getType() == TokenTypes::TemplateMiddle) {
        // Parse the embedded expression within ${}
        auto expr = parseExpression();
        if (!expr) {
            console.reportError(
                Console::SYNTAX_ERROR,
                "Invalid expression in template ${}",
                "To resolve this:\n1. Provide a valid expression within ${}\n2. Check expression syntax\n3. Ensure valid literals or identifiers",
                span
            );
            return nullptr;
        }
        expr->setPosition(currentToken, previousToken);
        expr->setSpan(span);
        parts.push_back(expr);

        // Consume TemplateMiddle
        std::u32string literalValue = currentToken.getU32Value();
        auto literalStmt = std::make_shared<StringLiteral>(literalValue);
        literalStmt->setPosition(currentToken, currentToken);
        literalStmt->setSpan(span);
        parts.push_back(literalStmt);
        eat(TokenTypes::TemplateMiddle, [&]() {
            std::string suggestion = Console::formatString(
                "To resolve this:\n"
                "1. Continue template with valid middle part\n"
                "2. Check template syntax\n"
                "3. Expected token: template middle, found '%s'",
                getTokenTypeName(currentToken.getType()).c_str()
            );
            console.reportError(
                Console::SYNTAX_ERROR,
                Console::formatString("Expected template middle, found '%s'", 
                    getTokenTypeName(currentToken.getType()).c_str()),
                suggestion,
                span
            );
        });
    }

    // Handle TemplateTail
    if (currentToken.getType() == TokenTypes::TemplateTail) {
        std::u32string literalValue = currentToken.getU32Value();
        auto literalStmt = std::make_shared<StringLiteral>(literalValue);
        literalStmt->setPosition(currentToken, currentToken);
        literalStmt->setSpan(span);
        parts.push_back(literalStmt);
        eat(TokenTypes::TemplateTail, [&]() {
            std::string suggestion = Console::formatString(
                "To resolve this:\n"
                "1. End template with valid tail part\n"
                "2. Check template syntax\n"
                "3. Expected token: template tail, found '%s'",
                getTokenTypeName(currentToken.getType()).c_str()
            );
            console.reportError(
                Console::SYNTAX_ERROR,
                Console::formatString("Expected template tail, found '%s'", 
                    getTokenTypeName(currentToken.getType()).c_str()),
                suggestion,
                span
            );
        });
    } else if (currentToken.getType() != TokenTypes::Plus) {
        console.reportError(
            Console::SYNTAX_ERROR,
            Console::formatString("Expected template tail or '+' for concatenation, found '%s'", 
                getTokenTypeName(currentToken.getType()).c_str()),
            "To resolve this:\n1. Complete template with a tail or concatenate with '+'\n2. Check template syntax\n3. Ensure valid template closure",
            span
        );
        return nullptr;
    }

    // Handle concatenation with additional string literals or templates
    while (currentToken.getType() == TokenTypes::Plus) {
        eat(TokenTypes::Plus, [&]() {
            std::string suggestion = Console::formatString(
                "To resolve this:\n"
                "1. Use '+' for template concatenation\n"
                "2. Check concatenation syntax\n"
                "3. Expected token: '+', found '%s'",
                getTokenTypeName(currentToken.getType()).c_str()
            );
            console.reportError(
                Console::SYNTAX_ERROR,
                Console::formatString("Expected '+' for template concatenation, found '%s'", 
                    getTokenTypeName(currentToken.getType()).c_str()),
                suggestion,
                span
            );
        });

        if (currentToken.getType() == TokenTypes::StringLiteral || currentToken.getType() == TokenTypes::TemplateHead) {
            auto nextTemplate = parseStringTemplate();
            if (!nextTemplate) {
                console.reportError(
                    Console::SYNTAX_ERROR,
                    "Failed to parse concatenated string template or literal",
                    "To resolve this:\n1. Ensure valid string or template after '+'\n2. Check template syntax\n3. Verify concatenation",
                    span
                );
                return nullptr;
            }
            parts.push_back(nextTemplate);
        } else if (currentToken.getType() == TokenTypes::IntegerLiteral || currentToken.getType() == TokenTypes::FloatLiteral) {
            std::u32string literalValue = utf8_to_utf32(currentToken.getValue());
            auto literalStmt = std::make_shared<StringLiteral>(literalValue);
            literalStmt->setPosition(currentToken, currentToken);
            literalStmt->setSpan(span);
            parts.push_back(literalStmt);
            eat(currentToken.getType());
        } else {
            console.reportError(
                Console::SYNTAX_ERROR,
                Console::formatString("Expected string, template, integer, or float literal after '+', found '%s'", 
                    getTokenTypeName(currentToken.getType()).c_str()),
                "To resolve this:\n1. Provide a valid literal or template for concatenation\n2. Check concatenation syntax\n3. Ensure valid string, template, or number",
                span
            );
            return nullptr;
        }
    }

    span.end.line = previousToken.getLine();
    span.end.col = previousToken.getColumn();
    span.end.filePath = previousToken.getFilePath();

    // auto templateStmt = std::make_shared<StringTemplateStatement>(parts);
    // templateStmt->setPosition(currentToken, previousToken);
    // templateStmt->setSpan(span);

    // DEBUG_LOG("Parsed string template with " + std::to_string(parts.size()) + " parts");

    // return templateStmt;
    return nullptr;
}

} // namespace Omniscript
