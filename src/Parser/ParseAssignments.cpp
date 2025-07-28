#include <omniscript/Statement.h>
#include <omniscript/Statements/AccessStatements.h>
#include <omniscript/Statements/FunctionStatement.h>
#include <omniscript/Statements/CallableStatement.h>
#include <omniscript/Statements/ExpressionStatements.h>
#include <omniscript/Statements/LiteralStatements.h>
#include <omniscript/Statements/AssignmentAndGetterStatements.h>

#include <omniscript/Core.h>
#include <omniscript/utils.h>
#include <omniscript/Lexer.h>
#include <omniscript/Tokens.h>
#include <omniscript/Parser.h>
#include <omniscript/Statement.h>
#include <omniscript/Symboltable.h>
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
        constant->setPosition(startToken, previousToken);
        return constant;
    }

    auto assign = std::make_shared<AssignVariable>(variableName, type, lambda);
    assign->setPosition(startToken, previousToken);

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
    
    // Vector to store multiple declarations
    std::vector<std::shared_ptr<Statement>> declarations;
    
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

        // Parse first variable declaration
        do {
            variableName = currentToken.getValue();
            eat(TokenTypes::Identifier);

            std::vector<std::string> dataTypes;
            std::shared_ptr<Omniscript::Type> currentType = nullptr;
            std::shared_ptr<Statement> currentValue = nullptr;

            if (currentToken.getType() == TokenTypes::Colon) {
                eat(TokenTypes::Colon);
                dataTypes = parseType();
                currentType = Omniscript::resolveType(dataTypes);
                
                DEBUG_LOG("Parsing assignment for " + getTokenTypeName(variableType) + " '" + variableName + "' with type '" + currentType->toString() + "'.");
                
                // Check if there's an assignment after type declaration
                if (currentToken.getType() == TokenTypes::Assign) {
                    eat(TokenTypes::Assign);
                    currentValue = parseExpression();
                    if (auto typed = std::dynamic_pointer_cast<TypedStatement>(currentValue)) {
                        typed->setType(type);
                        typed->setRootType(type);
                    }
                } else {
                    currentValue = std::make_shared<Null>(currentType);
                }
            
            } else if (currentToken.getType() == TokenTypes::Assign) {
                eat(TokenTypes::Assign);
                std::string typeName = currentToken.getValue();
        
                std::shared_ptr<Statement> result = parseExpression();
        
                if (auto objConstructor = std::dynamic_pointer_cast<ObjectConstructorStatement>(result)) {
                    objConstructor->setInstanceName(variableName);
                    objConstructor->setPosition(startToken, previousToken);
                    declarations.push_back(objConstructor);
                } else if (auto call = std::dynamic_pointer_cast<Call>(result)) {
                    call->setInstanceName(variableName);
                    call->isFromAssignment = true;
                    if (variableType == TokenTypes::Const) {
                        call->markAsConstant();
                    }
                    call->setPosition(startToken, previousToken);
                    declarations.push_back(call);
                } else if (auto funcDecl = std::dynamic_pointer_cast<FunctionDeclaration>(result)) {
                    if (auto named = std::dynamic_pointer_cast<NamedStatement>(funcDecl)) {
                        named->setName(variableName);
                    }
                    funcDecl->setPosition(startToken, previousToken);
                    declarations.push_back(funcDecl);
                } else {
                    currentValue = result;
                }
            } else {
                // No type annotation and no assignment - default initialization
                currentValue = std::make_shared<Null>(nullptr);
            }
            
            // Create the assignment if we haven't already added a special case
            if (currentValue) {
                if (variableType == TokenTypes::Const) {
                    auto constant = std::make_shared<AssignVariable>(variableName, currentType, currentValue);
                    constant->markAsConstant();
                    constant->setPosition(startToken, previousToken);
                    declarations.push_back(constant);
                } else {
                    auto assignment = std::make_shared<AssignVariable>(variableName, currentType, currentValue);
                    assignment->setPosition(startToken, previousToken);
                    declarations.push_back(assignment);
                }
            }
            
            // Check for comma to continue with next variable
            if (currentToken.getType() == TokenTypes::Comma) {
                eat(TokenTypes::Comma);
            } else {
                break; // No more variables to declare
            }
            
        } while (true);
        
        // If we have multiple declarations, return a block statement
        if (declarations.size() > 1) {
            auto block = std::make_shared<BlockStatement>(declarations);
            block->setPosition(startToken, previousToken);
            return block;
        } else if (declarations.size() == 1) {
            return declarations[0];
        }
    }
    
    // Handle reassignment cases (existing logic)
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
        value = std::make_shared<Null>(type); // Handle cases like `let a;`
        if (currentToken.getType() != TokenTypes::Newline &&
            currentToken.getType() != TokenTypes::Semicolon &&
            currentToken.getType() != TokenTypes::EOI) {
            eat(TokenTypes::Semicolon);
        }
    }

    if (!assignee) {
        if (variableType == TokenTypes::Const) {
            auto constant = std::make_shared<AssignVariable>(variableName, type, value);
            constant->markAsConstant();
            constant->setPosition(startToken, previousToken);
            return constant;
        }
        auto assignment = std::make_shared<AssignVariable>(variableName, type, value);
        assignment->setPosition(startToken, previousToken);
        return assignment;
    }

    if (auto varGetter = std::dynamic_pointer_cast<GetVariable>(assignee)) {
        auto reassignment = std::make_shared<AssignVariable>(varGetter->getName(), type, value, true);
        reassignment->setPosition(startToken, previousToken);
        return reassignment;

    } else if (auto reassignAccess = std::dynamic_pointer_cast<Access>(assignee)) {
        auto accessClone = std::dynamic_pointer_cast<Access>(reassignAccess->clone());
        accessClone->setAssignmentValueTo(value);
        accessClone->setPosition(startToken, previousToken);
        return accessClone;
    }

    console.error("The assignee is unnasignable");
    return nullptr;
}
