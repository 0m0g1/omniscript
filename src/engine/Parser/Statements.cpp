#include <omniscript/Core.h>
#include <omniscript/utils.h>
#include <omniscript/runtime/object.h>
#include <omniscript/engine/parser.h>
#include <omniscript/engine/lexer.h>
#include <omniscript/engine/tokens.h>
#include <omniscript/engine/Statement.h>
#include <omniscript/engine/Symboltable.h>
#include <omniscript/mainthreadrunner.h>
#include <omniscript/omniscript_pch.h>

// Parse a single statement
std::shared_ptr<Statement> Parser::parseStatement(bool checkForTerminalChar) {
    std::shared_ptr<Statement> statement;
    // std::vector<std::shared_ptr<Statement>> statements;

    if (debugMode) { // If we are in debug mode, show all of the tokens being parsed
        std::string message = "The lexer got token '" + getTokenTypeName(currentToken.getType()) +
                            "' with value '" + currentToken.getValue() + 
                            "' at line: " + std::to_string(currentToken.getLine()) + 
                            " column: " + std::to_string(currentToken.getColumn());
        DEBUG_LOG(message);  // Using DEBUG_LOG to output the debug message
    }

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
        case TokenTypes::Extern:
            statement = parseExternFunction();
            break;
        case TokenTypes::Intrinsic:
            statement = parseIntrinsicFunction();
            break;
        case TokenTypes::Function:
            statement = parseFunctionDeclaration("", {});
            break;
        case TokenTypes::Identifier: {
                if (lexer.peekToken(1).getType() == TokenTypes::Increment || lexer.peekToken(1).getType() == TokenTypes::Decrement) {
                    statement = parseExpression();
                } else {
                    statement = parseIdentifier();
                }
            }
            break;
        case TokenTypes::Increment:
        case TokenTypes::Decrement:
            statement = parseExpression();
            break;
        case TokenTypes::False:
            statement = parseExpression();
            break;
        case TokenTypes::True:
            statement = parseExpression();
            break;
        case TokenTypes::IntegerLiteral:
            statement = parseExpression();
            break;
        case TokenTypes::FloatLiteral:
            statement = parseExpression();
            break;
        case TokenTypes::StringLiteral:
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
            statement = parseAssignment();
            break;
        case TokenTypes::Const:
            statement = parseAssignment();
            break;
        case TokenTypes::Semicolon:
            statement = nullptr;
            break;
        case TokenTypes::Class:
            statement = parseClass();
            break;
        case TokenTypes::LessThan: {
            if (lexer.peekToken(1).getType() == TokenTypes::Identifier) {
                parameterType paramTypes = parseTypeParametersForDeclaration();
                if (currentToken.getType() == TokenTypes::Function) {
                    statement = parseFunctionDeclaration(paramTypes);
                    break;
                }
                else if (currentToken.getType() == TokenTypes::Let || currentToken.getType() == TokenTypes::Const) {
                    statement = parseAssignment(paramTypes);
                    break;
                }
            }
        }
        case TokenTypes::RightBrace:
            statement = nullptr; // add parse RightBrace method
            return statement;
        default:
           console.error(
                "[Parser Error]\nUnexpected token " + getTokenTypeName(currentToken.getType()) + " '" + currentToken.getValue() + "'" + " in statement"
            );
    }

    // Check for an optional semicolon
    if (checkForTerminalChar &&
        (currentToken.getType() == TokenTypes::Semicolon || currentToken.getType() == TokenTypes::Newline)) {
        
        if (currentToken.getType() == TokenTypes::Semicolon) {
            eat(currentToken.getType()); 
            if (currentToken.getType() == TokenTypes::Newline) {
                eat(TokenTypes::Newline);
            }
        } else {
            eat(currentToken.getType()); 
        }
    }

    statement->setPosition(Omniscript::getPosition());
    return statement;
}
