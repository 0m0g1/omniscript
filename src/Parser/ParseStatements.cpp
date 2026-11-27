#include <omniscript/Core.h>
#include <omniscript/utils.h>
#include <omniscript/Lexer.h>
#include <omniscript/Tokens.h>
#include <omniscript/Parser.h>
#include <omniscript/Statements/Statement.h>
#include <omniscript/Symboltable.h>
#include <omniscript/omniscript_pch.h>
#include <omniscript/Statements/AssignmentAndGetterStatements.h>
#include <omniscript/Statements/DeleteStatement.h>
#include <omniscript/Statements/DeferStatement.h>

namespace Omniscript {
// Parse a single statement
std::shared_ptr<Statement> Parser::parseStatement(bool checkForTerminalChar) {
    FileSpan span;
    span.start.line = currentToken.getLine();
    span.start.col = currentToken.getColumn();
    span.start.filePath = currentToken.getFilePath();

    std::shared_ptr<Statement> statement;

    #ifdef DEBUG
        // If we are in debug mode, show all of the tokens being parsed
        std::string message = "The lexer got token '" + getTokenTypeName(currentToken.getType()) +
                            "' with value '" + currentToken.getValue() + 
                            "' at line: " + std::to_string(currentToken.getLine()) + 
                            " column: " + std::to_string(currentToken.getColumn());
        DEBUG_LOG(message); 
    #endif

    switch (currentToken.getType()) {
        case TokenTypes::Include:
            statement = parseInclude();
            break;
        case TokenTypes::Import:
            statement = parseModuleImport();
            break;
        case TokenTypes::Module:
            statement = parseModule();
            break;
        case TokenTypes::Assign:
            statement = parseAssignment();
            break;
        case TokenTypes::Extern:
            statement = parseExternFunction();
            break;
        case TokenTypes::Intrinsic:
            statement = parseIntrinsicFunction();
            break;
        case TokenTypes::Volatile:
            eat(TokenTypes::Volatile, [&]() {
                SYNTAX_ERROR_F("Expected 'volatile' keyword, found '%s'", 
                              getTokenTypeName(currentToken.getType()).c_str());
            });
            statement = parseAssignment();
            if (auto assign = std::dynamic_pointer_cast<AssignVariable>(statement)) {
                assign->isVolatile = true;
            } else if (statement) {
                REPORT_ERROR_WITH_SPAN(
                    Console::SYNTAX_ERROR,
                    "Volatile keyword can only be applied to variable assignments",
                    span
                );
                statement = nullptr;
            }
            break;
        case TokenTypes::Function:
            statement = parseFunctionDeclaration("", {});
            break;
        case TokenTypes::Delete:
            statement = parseDelete();
            break;
        case TokenTypes::Identifier: {
            // `do` and `switch` are contextual keywords: only treated as statements when followed by
            // `{` / `(`, so existing code that uses them as ordinary names keeps working.
            const std::string word = currentToken.getValue();
            if (word == "do" && lexer.peekToken(1).getType() == TokenTypes::LeftBrace) {
                statement = parseDoWhile();
                break;
            }
            if (word == "switch" && lexer.peekToken(1).getType() == TokenTypes::LeftParen) {
                statement = parseSwitch();
                break;
            }
            // `defer` is a keyword only when a statement follows it (`defer f();`, `defer { }`,
            // `defer delete p;`); `defer = 1`, `defer(x)` and `defer.x` stay ordinary uses of the name.
            if (word == "defer") {
                const TokenTypes next = lexer.peekToken(1).getType();
                if (next == TokenTypes::Identifier || next == TokenTypes::LeftBrace || next == TokenTypes::Delete) {
                    statement = parseDefer();
                    break;
                }
            }
            // `arena a { ... }` is a keyword only for `arena <identifier> {`; any other use of the name stays ordinary.
            if (word == "arena" &&
                lexer.peekToken(1).getType() == TokenTypes::Identifier &&
                lexer.peekToken(2).getType() == TokenTypes::LeftBrace) {
                statement = parseArena();
                break;
            }
            if (lexer.peekToken(1).getType() == TokenTypes::Increment || 
                lexer.peekToken(1).getType() == TokenTypes::Decrement) {
                statement = parseExpression();
            } else {
                statement = parseIdentifier();
            }
            break;
        }
        case TokenTypes::Multiply:              // *ptr = value; or *foo()
        case TokenTypes::BitwiseAnd:            // &foo; (rare but legal)
        case TokenTypes::LogicalNot:            // !cond;
        case TokenTypes::Tilde:                 // ~bits;
        case TokenTypes::Minus:                 // -x;
        case TokenTypes::Plus:                  // +x;
        case TokenTypes::Increment:
        case TokenTypes::Decrement:
        case TokenTypes::False:
        case TokenTypes::True:
        case TokenTypes::IntegerLiteral:
        case TokenTypes::FloatLiteral:
        case TokenTypes::StringLiteral:
        case TokenTypes::TemplateTail:
            statement = parseExpression();
            break;
        case TokenTypes::If:
            statement = parseIfStatement();
            break;
        case TokenTypes::While:
            statement = parseWhileStatement();
            break;
        case TokenTypes::For:
            statement = parseForLoop();
            break;
        case TokenTypes::Continue:
            statement = parseContinue();
            break;
        case TokenTypes::Break:
            statement = parseBreak();
            break;
        case TokenTypes::Return:
            statement = parseReturnStatement();
            break;
        case TokenTypes::Struct:
            statement = parseStruct();
            break;
        case TokenTypes::Enum:
            statement = parseEnum();
            break;
        case TokenTypes::Namespace:
            statement = parseNamespace();
            break;
        case TokenTypes::Let:
        case TokenTypes::Const:
            statement = parseAssignment();
            break;
        case TokenTypes::Semicolon:
            // Empty statement - just consume the semicolon
            eat(TokenTypes::Semicolon);
            statement = nullptr;
            break;
        case TokenTypes::Class:
            statement = parseClass();
            break;
        case TokenTypes::Type:
            statement = parseTypeDeclaration();
            break;
        case TokenTypes::Using:
            statement = parseUsingAlias();
            break;
        case TokenTypes::LessThan: {
            if (lexer.peekToken(1).getType() == TokenTypes::Identifier) {
                parameterType paramTypes = parseTypeParametersForDeclaration();
                if (paramTypes.empty()) {
                    REPORT_ERROR_WITH_SPAN(
                        Console::SYNTAX_ERROR,
                        "Invalid generic type parameters",
                        span
                    );
                    statement = nullptr;
                    break;
                }
                if (currentToken.getType() == TokenTypes::Function) {
                    statement = parseFunctionDeclaration(paramTypes);
                } else if (currentToken.getType() == TokenTypes::Let || 
                          currentToken.getType() == TokenTypes::Const) {
                    statement = parseAssignment(paramTypes);
                } else {
                    SYNTAX_ERROR_F("Expected 'fn', 'let', or 'const' after type parameters, found '%s'", 
                                  getTokenTypeName(currentToken.getType()).c_str());
                    statement = nullptr;
                }
            } else {
                SYNTAX_ERROR_F("Expected identifier after '<' for type parameters, found '%s'", 
                              getTokenTypeName(lexer.peekToken(1).getType()).c_str());
                statement = nullptr;
            }
            break;
        }
        case TokenTypes::RightBrace:
            // End of block - return null to indicate end
            statement = nullptr;
            break;
        case TokenTypes::Newline:
            // CRITICAL FIX: Handle newlines properly to prevent infinite loop
            eat(TokenTypes::Newline);
            statement = nullptr; // Empty line
            break;
        case TokenTypes::EOI:
            // End of input
            statement = nullptr;
            break;
        default:
            // Enhanced error with source location
            SYNTAX_ERROR_F("Unexpected token '%s' with value '%s' in statement", 
                          getTokenTypeName(currentToken.getType()).c_str(), 
                          currentToken.getValue().c_str());
            
            // CRITICAL: Always advance the token to prevent infinite loops
            eat(currentToken.getType());
            statement = nullptr;
            break;
    }

    // Check for an optional semicolon or newline
    if (checkForTerminalChar && statement && 
        (currentToken.getType() == TokenTypes::Semicolon || 
         currentToken.getType() == TokenTypes::Newline)) {
        expectSemicolonOrNewLine();
    }

    span.end.line = previousToken.getLine();
    span.end.col = previousToken.getColumn();
    span.end.filePath = previousToken.getFilePath();

    // if (statement && !statement->span) {
    //     statement->setSpan(span);
    // }

    return statement;
}

// delete <expr>;
// The lexer emits TokenTypes::Delete for `delete` (after `.` or `::` it becomes a plain identifier,
// so `x.delete()` never reaches this function).
std::shared_ptr<Statement> Parser::parseDelete() {
    const Token startToken = currentToken;

    eat(TokenTypes::Delete);

    // `delete[] p;` destroys every element of an array made by `new T[n]`
    bool isArray = false;
    if (currentToken.getType() == TokenTypes::LeftBracket &&
        lexer.peekToken(1).getType() == TokenTypes::RightBracket) {
        eat(TokenTypes::LeftBracket);
        eat(TokenTypes::RightBracket);
        isArray = true;
    }

    switch (currentToken.getType()) {
        case TokenTypes::Semicolon:
        case TokenTypes::Newline:
        case TokenTypes::RightBrace:
        case TokenTypes::EOI:
            SYNTAX_ERROR_F("Expected an expression after 'delete', found '%s'",
                           getTokenTypeName(currentToken.getType()).c_str());
            return nullptr;
        default:
            break;
    }

    auto operand = parseExpression();
    if (!operand) {
        SYNTAX_ERROR_F("Expected an expression after 'delete', found '%s'",
                       getTokenTypeName(currentToken.getType()).c_str());
        return nullptr;
    }

    auto stmt = std::make_shared<DeleteStatement>(operand, isArray);
    stmt->setPosition(startToken, previousToken);
    return stmt;
}


// defer <statement>;   or   defer { ... }
// Runs when the enclosing scope exits (end of block, return, break, continue), last in first out.
std::shared_ptr<Statement> Parser::parseDefer() {
    const Token startToken = currentToken;

    eat(TokenTypes::Identifier);   // the contextual keyword `defer`

    std::shared_ptr<Statement> action;
    if (currentToken.getType() == TokenTypes::LeftBrace) {
        action = parseBlock();
    } else {
        action = parseStatement(false);   // the caller consumes the terminator
    }

    if (!action) {
        SYNTAX_ERROR_F("Expected a statement or block after 'defer', found '%s'",
                       getTokenTypeName(currentToken.getType()).c_str());
        return nullptr;
    }

    auto stmt = std::make_shared<DeferStatement>(action);
    stmt->setPosition(startToken, previousToken);
    return stmt;
}

// arena a { ... }
// Everything allocated from `a` (`new(a) T`, `Str.String(a)`) is released when the block exits: end of block,
// return, break and continue. Lowered to a block holding
//     let __arena_a: Arena;  let a: Arena* = &__arena_a;  defer __arena_a.destroy();  { body }
// The three lines are parsed by an ordinary Parser over that text, so they are exactly what the user could
// have written by hand, and the existing `defer` machinery does the rest.
std::shared_ptr<Statement> Parser::parseArena() {
    const Token startToken = currentToken;

    eat(TokenTypes::Identifier);                       // the contextual keyword `arena`
    const std::string name = currentToken.getValue();
    eat(TokenTypes::Identifier, expectedIdentifierMessage("arena name"));

    auto body = parseBlock();
    if (!body) {
        SYNTAX_ERROR_F("Expected a block after 'arena %s', found '%s'",
                       name.c_str(), getTokenTypeName(currentToken.getType()).c_str());
        return nullptr;
    }

    const std::string hidden = "__arena_" + name;
    const std::string prologue =
        "let " + hidden + ": Arena;\n"
        "let " + name + ": Arena* = &" + hidden + ";\n"
        "defer " + hidden + ".destroy();\n";

    Lexer prologueLexer(prologue, startToken.getFilePath());
    Parser prologueParser(prologueLexer);
    std::vector<std::shared_ptr<Statement>> stmts;
    for (int guard = 0; guard < 32 && prologueParser.currentToken.getType() != TokenTypes::EOI; ++guard) {
        auto st = prologueParser.parseStatement();
        if (st) {
            st->setPosition(startToken, startToken);   // errors point at the `arena` keyword, not at the helper text
            stmts.push_back(st);
        }
    }
    if (stmts.size() != 3) {
        REPORT_ERROR_WITH_SPAN(
            Console::SYNTAX_ERROR,
            "Could not set up the arena '" + name + "' (is the Arena type available? import standard/1/Arena.os)",
            errorSpanAtCurrentToken()
        );
        return nullptr;
    }
    stmts.push_back(body);

    auto block = std::make_shared<BlockStatement>(stmts);
    if (auto bodyBlock = std::dynamic_pointer_cast<BlockStatement>(body)) block->isGlobal = bodyBlock->isGlobal;
    block->setPosition(startToken, previousToken);
    return block;
}

} // namespace Omniscript