#include <omniscript/Core.h>
#include <omniscript/utils.h>
#include <omniscript/Lexer.h>
#include <omniscript/Tokens.h>
#include <omniscript/Parser.h>
#include <omniscript/Statements/Statement.h>
#include <omniscript/Symboltable.h>
#include <omniscript/omniscript_pch.h>
#include <omniscript/Statements/AssignmentAndGetterStatements.h>

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
        case TokenTypes::Identifier: {
            if (lexer.peekToken(1).getType() == TokenTypes::Increment || 
                lexer.peekToken(1).getType() == TokenTypes::Decrement) {
                statement = parseExpression();
            } else {
                statement = parseIdentifier();
            }
            break;
        }
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

} // namespace Omniscript
