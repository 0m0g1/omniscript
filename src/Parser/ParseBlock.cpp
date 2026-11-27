#include <omniscript/Core.h>
#include <omniscript/utils.h>
#include <omniscript/Parser.h>
#include <omniscript/Tokens.h>
#include <omniscript/Statements/Statement.h>
#include <omniscript/Statements/ModuleAndImportStatements.h>
#include <omniscript/Statements/FunctionStatement.h>
#include <omniscript/Symboltable.h>
#include <omniscript/omniscript_pch.h>

namespace Omniscript {

std::shared_ptr<Statement> Parser::parseBlock() {
    Token startToken = currentToken;

    std::vector<std::shared_ptr<Statement>> statements;

    auto pushIfValid = [&](std::shared_ptr<Statement> stmt) {
        if (stmt) statements.push_back(std::move(stmt));
    };

    // Splice an included file's statements directly into `out`.
    // Returns true if the current token was an include (errors are
    // reported internally; the caller should not fall through to
    // parseStatement in that case).
    auto spliceIncludeInto = [&](std::vector<std::shared_ptr<Statement>>& out) -> bool {
        if (currentToken.getType() != TokenTypes::Include) return false;

        FileSpan incSpan = errorSpanAtCurrentToken();
        eat(TokenTypes::Include);
        if (currentToken.getType() != TokenTypes::StringLiteral) {
            console.reportError(Console::SYNTAX_ERROR,
                "Expected string literal after 'include'.", incSpan);
            return true;
        }
        const std::string incPath = currentToken.getValue();
        eat(TokenTypes::StringLiteral);
        if (currentToken.getType() == TokenTypes::Semicolon) eat(TokenTypes::Semicolon);

        auto incStmt = std::make_shared<IncludeStatement>(incPath);
        incStmt->setSpan(incSpan);
        const std::string src = incStmt->getSourceText();
        if (src.empty()) return true;

        // Wrap in a synthetic fn so the file parses as STATEMENTS, not as a
        // standalone program. Braces are flush against the source so line
        // numbers in error messages stay aligned with the included file.
        const std::string wrapped = "fn __inc__() {" + src + "}";
        Lexer incLexer(wrapped, incPath);
        Parser incParser(incLexer);
        auto parsed = incParser.parse();

        bool spliced = false;
        for (auto& s : parsed) {
            if (auto fn = std::dynamic_pointer_cast<FunctionDeclaration>(s)) {
                // See note below: FunctionDeclaration needs a body getter.
                if (auto fnBody = fn->body) {
                    for (auto& inner : fnBody->statements) {
                        out.push_back(inner);
                    }
                    spliced = true;
                }
                break;
            }
        }
        if (!spliced) {
            console.reportError(Console::SYNTAX_ERROR,
                "Included file '" + incPath + "' could not be parsed as statements.",
                incSpan);
        }
        return true;
    };

    if (currentToken.getType() == TokenTypes::LeftBrace) {
        eat(TokenTypes::LeftBrace);
        while (currentToken.getType() != TokenTypes::RightBrace &&
               currentToken.getType() != TokenTypes::EOI) {
            if (!spliceIncludeInto(statements)) {
                pushIfValid(parseStatement());
            }
        }
        // Do not consume a trailing ';' here: it belongs to the enclosing statement
        // (e.g. `return function() { ... };`).
        eat(TokenTypes::RightBrace);
    } else if (currentToken.getType() == TokenTypes::Return) {
        pushIfValid(parseReturnStatement());
    } else {
        // Single-statement body (e.g. `if cond include "x";`): splice too,
        // so textual inclusion applies regardless of brace presence.
        if (!spliceIncludeInto(statements)) {
            pushIfValid(parseStatement());
        }
    }

    auto block = std::make_shared<BlockStatement>(statements);
    block->setPosition(startToken, currentToken);
    return block;
}

} // namespace Omniscript