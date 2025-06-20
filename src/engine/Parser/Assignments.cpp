#include <omniscript/Core.h>
#include <omniscript/utils.h>
#include <omniscript/runtime/object.h>
#include <omniscript/engine/Parser.h>
#include <omniscript/engine/Lexer.h>
#include <omniscript/engine/Tokens.h>
#include <omniscript/engine/Statement.h>
#include <omniscript/engine/Symboltable.h>
#include <omniscript/omniscript_pch.h>

bool Parser::isAssignmentExpression(TokenTypes tokenType) {
    if (tokenType == TokenTypes::Assign || 
        tokenType == TokenTypes::PlusAssign || 
        tokenType == TokenTypes::MinusAssign || 
        tokenType == TokenTypes::DivideAssign || 
        tokenType == TokenTypes::MultiplyAssign || 
        tokenType == TokenTypes::Increment || 
        tokenType == TokenTypes::Decrement ||
        tokenType == TokenTypes::BitwiseXorAssign ||
        tokenType == TokenTypes::BitwiseAndAssign ||
        tokenType == TokenTypes::BitwiseOrAssign ||
        tokenType == TokenTypes::ShiftLeftAssign ||
        tokenType == TokenTypes::ShiftRightAssign  
    ) {
        return true;
    }
    return false;
}

std::shared_ptr<Statement> Parser::parseAssignment(parameterType paramTypes) {
    Token startToken = currentToken;

    TokenTypes variableType = TokenTypes::Let;
    std::string variableName;
    std::shared_ptr<Omniscript::Type> type = nullptr;

    if (currentToken.getType() == TokenTypes::Let) {
        eat(TokenTypes::Let);
        variableType = TokenTypes::Let;
    } else if (currentToken.getType() == TokenTypes::Const) {
        eat(TokenTypes::Const);
        variableType = TokenTypes::Const;
    }

    variableName = currentToken.getValue();
    eat(TokenTypes::Identifier);

    if (currentToken.getType() == TokenTypes::Colon) {
        eat(TokenTypes::Colon);
        std::vector<std::string> dataTypes = parseType();
        type = Omniscript::resolveType(dataTypes);
    }

    eat(TokenTypes::Assign);

    std::shared_ptr<Statement> lambda = parseLambdaFunction(variableName, paramTypes);

    if (auto funcDecl = std::dynamic_pointer_cast<FunctionDeclaration>(lambda)) {
        if (auto named = std::dynamic_pointer_cast<NamedStatement>(funcDecl)) {
            named->setName(variableName);
        }
    }

    if (variableType == TokenTypes::Const) {
        auto constant = std::make_shared<AssignVariable>(variableName, type, lambda);
        constant->markAsConstant();
        constant->setPosition(startToken);
        return constant;
    }

    auto assign = std::make_shared<AssignVariable>(variableName, type, lambda);
    assign->setPosition(startToken);

    return assign;
}


std::shared_ptr<Statement> Parser::parseAssignment(std::shared_ptr<Statement> assignee) {
    Token startToken = currentToken;
    TokenTypes variableType;
    std::string variableName;
    std::shared_ptr<Statement> value;
    std::shared_ptr<Omniscript::Type> type;
    bool isReference = false;
    bool isPointer = false;
    bool isArray = false;
    
    if (!assignee) {
        if (currentToken.getType() == TokenTypes::Let) {
            eat(TokenTypes::Let);
            variableType = TokenTypes::Let;

        } else if (currentToken.getType() == TokenTypes::Const) {
            eat(TokenTypes::Const);
            variableType = TokenTypes::Const;

        } else {
            variableName = previousToken.getValue();
        }

        variableName = currentToken.getValue();
        eat(TokenTypes::Identifier);

        std::vector<std::string> dataTypes;

        if (currentToken.getType() == TokenTypes::Colon) {
            eat(TokenTypes::Colon);
            dataTypes = parseType();
        
        } else if (currentToken.getType() == TokenTypes::Assign) {
            eat(TokenTypes::Assign);
            std::string typeName = currentToken.getValue();
    
            std::shared_ptr<Statement> result = parseExpression();
    
            result->setPosition(startToken);
            if (auto objConstructor = std::dynamic_pointer_cast<ObjectConstructorStatement>(result)) {
                objConstructor->setInstanceName(variableName);
                return result;
            } else if (auto call = std::dynamic_pointer_cast<Call>(result)) {
                call->setInstanceName(variableName);
                call->isFromAssignment = true;
                if (variableType == TokenTypes::Const) {
                    call->markAsConstant();
                }
                return result;
            }
            
            if (auto funcDecl = std::dynamic_pointer_cast<FunctionDeclaration>(result)) {
                if (auto named = std::dynamic_pointer_cast<NamedStatement>(funcDecl)) {
                    named->setName(variableName);
                }
                return funcDecl;
            }
    
            if (variableType == TokenTypes::Let) {
                return std::make_shared<AssignVariable>(variableName, nullptr, result);
            }

            auto constant = std::make_shared<AssignVariable>(variableName, nullptr, result);
            constant->markAsConstant();
            return constant;
        }    
    
        type = Omniscript::resolveType(dataTypes);
    
        DEBUG_LOG("Parsing assignment for " + getTokenTypeName(variableType) + " '" + variableName + "' with type '" + type->toString() + "'.");
    }
    
    if (currentToken.getType() != TokenTypes::Semicolon) {
        if (currentToken.getType() == TokenTypes::Increment || currentToken.getType() == TokenTypes::Decrement) {
            DEBUG_LOG("Assigning a unary statement");
            switch (currentToken.getType()) {
                case TokenTypes::Increment:
                    value = std::make_shared<UnaryExpression>(TokenTypes::Increment, assignee, UnaryExpression::Position::Postfix);
                    break;
                case TokenTypes::Decrement:
                    value = std::make_shared<UnaryExpression>(TokenTypes::Decrement, assignee, UnaryExpression::Position::Postfix);
                    break;
                default:
                    eat(TokenTypes::Semicolon);
            }
            eat(currentToken.getType());

        } else {
            DEBUG_LOG("Assigning a binary or ternary expression");
            Token currentAssignmentOperation = currentToken;
            eat(currentToken.getType());
            
            value = parseExpression(); // Parse right-hand side
            
            switch (currentAssignmentOperation.getType()) {
                case TokenTypes::Assign:
                    break;
                case TokenTypes::PlusAssign:
                    value = std::make_shared<BinaryExpression>(assignee, TokenTypes::Plus, value);
                    break;
                case TokenTypes::MinusAssign:
                    value = std::make_shared<BinaryExpression>(assignee, TokenTypes::Minus, value);
                    break;
                case TokenTypes::DivideAssign:
                    value = std::make_shared<BinaryExpression>(assignee, TokenTypes::Divide, value);
                    break;
                case TokenTypes::MultiplyAssign:
                    value = std::make_shared<BinaryExpression>(assignee, TokenTypes::Multiply, value);
                    break;
                case TokenTypes::BitwiseXorAssign:
                    value = std::make_shared<BinaryExpression>(assignee, TokenTypes::BitwiseXor, value);
                    break;
                case TokenTypes::BitwiseAndAssign:
                    value = std::make_shared<BinaryExpression>(assignee, TokenTypes::BitwiseAnd, value);
                    break;
                case TokenTypes::BitwiseOrAssign:
                    value = std::make_shared<BinaryExpression>(assignee, TokenTypes::BitwiseOr, value);
                    break;
                case TokenTypes::ShiftLeftAssign:
                    value = std::make_shared<BinaryExpression>(assignee, TokenTypes::ShiftLeft, value);
                    break;
                case TokenTypes::ShiftRightAssign:
                    value = std::make_shared<BinaryExpression>(assignee, TokenTypes::ShiftRight, value);
                    break;  
                default:
                    eat(TokenTypes::Assign, "Invalid assignment operator: " + getTokenTypeName(currentAssignmentOperation.getType()));
                    break;
            }
        }

    } else {
        value = nullptr; // Handle cases like `let a;`
        if (currentToken.getType() != TokenTypes::Newline &&
            currentToken.getType() != TokenTypes::Semicolon &&
            currentToken.getType() != TokenTypes::EOI) {
            eat(TokenTypes::Semicolon);
        }
    }

    value->setPosition(startToken);

    if (!assignee) {
        if (variableType == TokenTypes::Const) {
            auto constant = std::make_shared<AssignVariable>(variableName, type, value);
            constant->markAsConstant();
            return constant;
        }
        return std::make_shared<AssignVariable>(variableName, type, value);
    }

    if (auto varGetter = std::dynamic_pointer_cast<GetVariable>(assignee)) {
        return std::make_shared<AssignVariable>(varGetter->getName(), type, value, true);

    } else if (auto reassignAccess = std::dynamic_pointer_cast<Access>(assignee)) {
        auto accessClone = std::dynamic_pointer_cast<Access>(reassignAccess->clone());
        accessClone->setAssignmentValueTo(value);
        return accessClone;
    }

    console.error("The assignee is unnasignable");
    return nullptr;
}