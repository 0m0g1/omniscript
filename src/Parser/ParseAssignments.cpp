#include <omniscript/Statements/Statement.h>
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
#include <omniscript/Types/Types.h>
#include <omniscript/Statements/Statement.h>
#include <omniscript/Symboltable.h>
#include <omniscript/omniscript_pch.h>

namespace Omniscript {

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
    FileSpan span;
    span.start.line = startToken.getLine();
    span.start.col = startToken.getColumn();
    span.start.filePath = startToken.getFilePath();

    TokenTypes variableType = TokenTypes::Let;
    std::string variableName;
    std::shared_ptr<Type> type = nullptr;

    if (currentToken.getType() == TokenTypes::Let) {
        eat(TokenTypes::Let);
        variableType = TokenTypes::Let;
    } else if (currentToken.getType() == TokenTypes::Const) {
        eat(TokenTypes::Const);
        variableType = TokenTypes::Const;
    } else {
        std::string suggestion = Console::formatString(
            "To resolve this:\n"
            "1. Use 'let' or 'const' for variable declaration\n"
            "2. Check for correct syntax\n"
            "3. Expected token: 'let' or 'const', found '%s'",
            getTokenTypeName(currentToken.getType()).c_str()
        );
        console.reportError(
            Console::SYNTAX_ERROR,
            Console::formatString("Expected 'let' or 'const' for variable declaration, found '%s'", 
                getTokenTypeName(currentToken.getType()).c_str()),
            suggestion,
            span
        );
        return nullptr;
    }

    if (currentToken.getType() != TokenTypes::Identifier) {
        std::string suggestion = Console::formatString(
            "To resolve this:\n"
            "1. Provide a valid identifier name\n"
            "2. Check for correct variable naming syntax\n"
            "3. Expected token: Identifier, found '%s'",
            getTokenTypeName(currentToken.getType()).c_str()
        );
        console.reportError(
            Console::SYNTAX_ERROR,
            Console::formatString("Expected identifier for variable name, found '%s'", 
                getTokenTypeName(currentToken.getType()).c_str()),
            suggestion,
            span
        );
        return nullptr;
    }
    variableName = currentToken.getValue();
    eat(TokenTypes::Identifier);

    if (currentToken.getType() == TokenTypes::Colon) {
        eat(TokenTypes::Colon);
        std::vector<std::string> dataTypes = parseType();
        type = resolveType(dataTypes);
        if (!type) {
            std::string suggestion = "To resolve this:\n"
                                   "1. Verify type name is defined\n"
                                   "2. Check for correct type spelling\n"
                                   "3. Ensure type is imported or in scope";
            console.reportError(
                Console::SEMANTIC_ERROR,
                Console::formatString("Invalid type specification for variable '%s'", 
                    variableName.c_str()),
                suggestion,
                span
            );
            return nullptr;
        }
    }

    if (currentToken.getType() != TokenTypes::Assign) {
        if (type && !type->isNullable()) {
            std::string suggestion = Console::formatString(
                "To resolve this:\n"
                "1. Provide an explicit value for non-nullable type '%s'\n"
                "2. Use '=' followed by a valid expression\n"
                "3. Consider using a nullable type if no initial value is intended",
                type->toString().c_str()
            );
            console.reportError(
                Console::SEMANTIC_ERROR,
                Console::formatString("Non-nullable type '%s' for variable '%s' requires an explicit value", 
                    type->toString().c_str(), variableName.c_str()),
                suggestion,
                span
            );
            return nullptr;
        }
        std::string suggestion = Console::formatString(
            "To resolve this:\n"
            "1. Use '=' for assignment\n"
            "2. Check for correct assignment syntax\n"
            "3. Expected token: '=', found '%s'",
            getTokenTypeName(currentToken.getType()).c_str()
        );
        console.reportError(
            Console::SYNTAX_ERROR,
            Console::formatString("Expected '=' for assignment, found '%s'", 
                getTokenTypeName(currentToken.getType()).c_str()),
            suggestion,
            span
        );
        return nullptr;
    }
    eat(TokenTypes::Assign);

    std::shared_ptr<Statement> lambda = parseLambdaFunction(variableName, paramTypes);
    if (!lambda) {
        std::string suggestion = Console::formatString(
            "To resolve this:\n"
            "1. Verify lambda function syntax\n"
            "2. Check for valid expression\n"
            "3. Ensure parameters match expected types for '%s'",
            variableName.c_str()
        );
        console.reportError(
            Console::SYNTAX_ERROR,
            Console::formatString("Failed to parse lambda function for variable '%s'", 
                variableName.c_str()),
            suggestion,
            span
        );
        return nullptr;
    }

    if (auto funcDecl = std::dynamic_pointer_cast<FunctionDeclaration>(lambda)) {
        if (auto named = std::dynamic_pointer_cast<NamedStatement>(funcDecl)) {
            named->setName(variableName);
        }
    }

    span.end.line = previousToken.getLine();
    span.end.col = previousToken.getColumn();
    span.end.filePath = previousToken.getFilePath();

    if (variableType == TokenTypes::Const) {
        auto constant = std::make_shared<AssignVariable>(variableName, type, lambda);
        constant->markAsConstant();
        constant->setPosition(startToken, currentToken);
        constant->setSpan(span);
        return constant;
    }

    auto assign = std::make_shared<AssignVariable>(variableName, type, lambda);
    assign->setPosition(startToken, currentToken);
    assign->setSpan(span);
    return assign;
}

std::shared_ptr<Statement> Parser::parseAssignment(std::shared_ptr<Statement> assignee) {
    Token startToken = currentToken;
    FileSpan span;
    span.start.line = startToken.getLine();
    span.start.col = startToken.getColumn();
    span.start.filePath = startToken.getFilePath();

    TokenTypes variableType = TokenTypes::Let;
    std::string variableName;
    std::shared_ptr<Statement> value;
    std::shared_ptr<Type> type;
    bool isReference = false;
    bool isPointer = false;
    bool isArray = false;
    
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

        do {
            if (currentToken.getType() != TokenTypes::Identifier) {
                std::string suggestion = Console::formatString(
                    "To resolve this:\n"
                    "1. Provide a valid identifier name\n"
                    "2. Check for correct variable naming syntax\n"
                    "3. Expected token: Identifier, found '%s'",
                    getTokenTypeName(currentToken.getType()).c_str()
                );
                console.reportError(
                    Console::SYNTAX_ERROR,
                    Console::formatString("Expected identifier for variable name, found '%s'", 
                        getTokenTypeName(currentToken.getType()).c_str()),
                    suggestion,
                    span
                );
                return nullptr;
            }
            variableName = currentToken.getValue();
            eat(TokenTypes::Identifier);

            std::vector<std::string> dataTypes;
            std::shared_ptr<Type> currentType = nullptr;
            std::shared_ptr<Statement> currentValue = nullptr;

            if (currentToken.getType() == TokenTypes::Colon) {
                eat(TokenTypes::Colon);
                dataTypes = parseType();
                currentType = resolveType(dataTypes);
                if (!currentType) {
                    std::string suggestion = Console::formatString(
                        "To resolve this:\n"
                        "1. Verify type '%s' is defined\n"
                        "2. Check for correct type spelling\n"
                        "3. Ensure type is imported or in scope",
                        join(dataTypes, ".").c_str()
                    );
                    console.reportError(
                        Console::SEMANTIC_ERROR,
                        Console::formatString("Invalid type specification for variable '%s'", 
                            variableName.c_str()),
                        suggestion,
                        span
                    );
                    return nullptr;
                }
                
                DEBUG_LOG("Parsing assignment for " + getTokenTypeName(variableType) + " '" + variableName + "' with type '" + currentType->toString() + "'.");

                if (currentToken.getType() == TokenTypes::Assign) {
                    eat(TokenTypes::Assign);
                    currentValue = parseExpression();
                    if (!currentValue) {
                        std::string suggestion = Console::formatString(
                            "To resolve this:\n"
                            "1. Provide a valid expression\n"
                            "2. Check for correct syntax\n"
                            "3. Ensure expression is compatible with type '%s'",
                            currentType->toString().c_str()
                        );
                        console.reportError(
                            Console::SYNTAX_ERROR,
                            Console::formatString("Failed to parse expression for variable '%s'", 
                                variableName.c_str()),
                            suggestion,
                            span
                        );
                        return nullptr;
                    }
                    if (auto typed = std::dynamic_pointer_cast<TypedStatement>(currentValue)) {
                        typed->setType(currentType);
                        typed->setRootType(currentType);
                    }
                } else if (currentType && !currentType->isNullable()) {
                    std::string suggestion = Console::formatString(
                        "To resolve this:\n"
                        "1. Provide an explicit value for non-nullable type '%s'\n"
                        "2. Use '=' followed by a valid expression\n"
                        "3. Consider using a nullable type if no initial value is intended",
                        currentType->toString().c_str()
                    );
                    console.reportError(
                        Console::SEMANTIC_ERROR,
                        Console::formatString("Non-nullable type '%s' for variable '%s' requires an explicit value", 
                            currentType->toString().c_str(), variableName.c_str()),
                        suggestion,
                        span
                    );
                    return nullptr;
                } else {
                    currentValue = std::make_shared<Null>(currentType);
                }
            } else if (currentToken.getType() == TokenTypes::Assign) {
                eat(TokenTypes::Assign);
                std::string typeName = currentToken.getValue();
        
                std::shared_ptr<Statement> result = parseExpression();
                if (!result) {
                    std::string suggestion = Console::formatString(
                        "To resolve this:\n"
                        "1. Provide a valid expression\n"
                        "2. Check for correct syntax\n"
                        "3. Ensure expression is valid for variable '%s'",
                        variableName.c_str()
                    );
                    console.reportError(
                        Console::SYNTAX_ERROR,
                        Console::formatString("Failed to parse expression for variable '%s'", 
                            variableName.c_str()),
                        suggestion,
                        span
                    );
                    return nullptr;
                }
        
                if (auto objConstructor = std::dynamic_pointer_cast<ObjectConstructorStatement>(result)) {
                    objConstructor->setInstanceName(variableName);
                    objConstructor->setPosition(startToken, currentToken);
                    declarations.push_back(objConstructor);
                } else if (auto call = std::dynamic_pointer_cast<Call>(result)) {
                    call->setInstanceName(variableName);
                    call->isFromAssignment = true;
                    if (variableType == TokenTypes::Const) {
                        call->markAsConstant();
                    }
                    call->setPosition(startToken, currentToken);
                    declarations.push_back(call);
                } else if (auto funcDecl = std::dynamic_pointer_cast<FunctionDeclaration>(result)) {
                    if (auto named = std::dynamic_pointer_cast<NamedStatement>(funcDecl)) {
                        named->setName(variableName);
                    }
                    funcDecl->setPosition(startToken, currentToken);
                    declarations.push_back(funcDecl);
                } else {
                    currentValue = result;
                }
            } else {
                if (currentType && !currentType->isNullable()) {
                    std::string suggestion = Console::formatString(
                        "To resolve this:\n"
                        "1. Provide an explicit value for non-nullable type '%s'\n"
                        "2. Use '=' followed by a valid expression\n"
                        "3. Consider using a nullable type if no initial value is intended",
                        currentType ? currentType->toString().c_str() : "unknown"
                    );
                    console.reportError(
                        Console::SEMANTIC_ERROR,
                        Console::formatString("Non-nullable type '%s' for variable '%s' requires an explicit value", 
                            currentType ? currentType->toString().c_str() : "unknown", variableName.c_str()),
                        suggestion,
                        span
                    );
                    return nullptr;
                }
                currentValue = std::make_shared<Null>(nullptr);
            }
            
            if (currentValue) {
                if (variableType == TokenTypes::Const) {
                    auto constant = std::make_shared<AssignVariable>(variableName, currentType, currentValue);
                    constant->markAsConstant();
                    constant->setPosition(startToken, currentToken);
                    declarations.push_back(constant);
                } else {
                    auto assignment = std::make_shared<AssignVariable>(variableName, currentType, currentValue);
                    assignment->setPosition(startToken, currentToken);
                    declarations.push_back(assignment);
                }
            }
            
            if (currentToken.getType() == TokenTypes::Comma) {
                eat(TokenTypes::Comma);
            } else {
                break;
            }
            
        } while (true);
        
        span.end.line = previousToken.getLine();
        span.end.col = previousToken.getColumn();
        span.end.filePath = previousToken.getFilePath();

        if (declarations.size() > 1) {
            auto block = std::make_shared<BlockStatement>(declarations);
            block->setPosition(startToken, currentToken);
            block->setSpan(span);
            return block;
        } else if (declarations.size() == 1) {
            declarations[0]->setSpan(span);
            return declarations[0];
        }
    }
    
    if (currentToken.getType() != TokenTypes::Semicolon) {
        if (currentToken.getType() == TokenTypes::Increment || currentToken.getType() == TokenTypes::Decrement) {
            DEBUG_LOG("Assigning a unary statement");
            switch (currentToken.getType()) {
                case TokenTypes::Increment:
                    value = std::make_shared<ASTUnaryExpression>(TokenTypes::Increment, assignee, ASTUnaryExpression::Position::Postfix);
                    break;
                case TokenTypes::Decrement:
                    value = std::make_shared<ASTUnaryExpression>(TokenTypes::Decrement, assignee, ASTUnaryExpression::Position::Postfix);
                    break;
                default:
                    std::string suggestion = Console::formatString(
                        "To resolve this:\n"
                        "1. Use valid unary operator ('++' or '--')\n"
                        "2. Check for correct syntax\n"
                        "3. Expected token: '++' or '--', found '%s'",
                        getTokenTypeName(currentToken.getType()).c_str()
                    );
                    console.reportError(
                        Console::SYNTAX_ERROR,
                        Console::formatString("Invalid unary operator, found '%s'", 
                            getTokenTypeName(currentToken.getType()).c_str()),
                        suggestion,
                        span
                    );
                    return nullptr;
            }
            eat(currentToken.getType());
        } else if (isAssignmentExpression(currentToken.getType())) {
            DEBUG_LOG("Assigning a binary or ternary expression");
            Token currentAssignmentOperation = currentToken;
            eat(currentAssignmentOperation.getType());
            
            value = parseExpression();
            if (!value) {
                std::string suggestion = Console::formatString(
                    "To resolve this:\n"
                    "1. Provide a valid right-hand side expression\n"
                    "2. Check for correct syntax\n"
                    "3. Ensure expression is compatible with assignee"
                );
                console.reportError(
                    Console::SYNTAX_ERROR,
                    "Failed to parse right-hand side of assignment",
                    suggestion,
                    span
                );
                return nullptr;
            }
            
            switch (currentAssignmentOperation.getType()) {
                case TokenTypes::Assign:
                    break;
                case TokenTypes::PlusAssign:
                    value = std::make_shared<ASTBinaryExpression>(assignee, TokenTypes::Plus, value);
                    break;
                case TokenTypes::MinusAssign:
                    value = std::make_shared<ASTBinaryExpression>(assignee, TokenTypes::Minus, value);
                    break;
                case TokenTypes::DivideAssign:
                    value = std::make_shared<ASTBinaryExpression>(assignee, TokenTypes::Divide, value);
                    break;
                case TokenTypes::MultiplyAssign:
                    value = std::make_shared<ASTBinaryExpression>(assignee, TokenTypes::Multiply, value);
                    break;
                case TokenTypes::BitwiseXorAssign:
                    value = std::make_shared<ASTBinaryExpression>(assignee, TokenTypes::BitwiseXor, value);
                    break;
                case TokenTypes::BitwiseAndAssign:
                    value = std::make_shared<ASTBinaryExpression>(assignee, TokenTypes::BitwiseAnd, value);
                    break;
                case TokenTypes::BitwiseOrAssign:
                    value = std::make_shared<ASTBinaryExpression>(assignee, TokenTypes::BitwiseOr, value);
                    break;
                case TokenTypes::ShiftLeftAssign:
                    value = std::make_shared<ASTBinaryExpression>(assignee, TokenTypes::ShiftLeft, value);
                    break;
                case TokenTypes::ShiftRightAssign:
                    value = std::make_shared<ASTBinaryExpression>(assignee, TokenTypes::ShiftRight, value);
                    break;  
                default:
                    std::string suggestion = Console::formatString(
                        "To resolve this:\n"
                        "1. Use valid assignment operator\n"
                        "2. Valid operators: '=', '+=', '-=', '*=', '/=', '^=', '&=', '|=', '<<=', '>>='\n"
                        "3. Found invalid operator '%s'",
                        getTokenTypeName(currentAssignmentOperation.getType()).c_str()
                    );
                    console.reportError(
                        Console::SYNTAX_ERROR,
                        Console::formatString("Invalid assignment operator '%s'", 
                            getTokenTypeName(currentAssignmentOperation.getType()).c_str()),
                        suggestion,
                        span
                    );
                    return nullptr;
            }
        } else {
            std::string suggestion = Console::formatString(
                "To resolve this:\n"
                "1. Use valid assignment operator\n"
                "2. Valid operators: '=', '+=', '-=', '*=', '/=', '^=', '&=', '|=', '++', '--'\n"
                "3. Found invalid token '%s'",
                getTokenTypeName(currentToken.getType()).c_str()
            );
            console.reportError(
                Console::SYNTAX_ERROR,
                Console::formatString("Expected assignment operator, found '%s'", 
                    getTokenTypeName(currentToken.getType()).c_str()),
                suggestion,
                span
            );
            return nullptr;
        }
    } else {
        if (type && !type->isNullable()) {
            std::string suggestion = Console::formatString(
                "To resolve this:\n"
                "1. Provide an explicit value for non-nullable type '%s'\n"
                "2. Use '=' followed by a valid expression\n"
                "3. Consider using a nullable type if no initial value is intended",
                type->toString().c_str()
            );
            console.reportError(
                Console::SEMANTIC_ERROR,
                Console::formatString("Non-nullable type '%s' requires an explicit intilializer", 
                    type->toString().c_str()),
                suggestion,
                span
            );
            return nullptr;
        }
        value = std::make_shared<Null>(type);
        if (currentToken.getType() != TokenTypes::Newline &&
            currentToken.getType() != TokenTypes::Semicolon &&
            currentToken.getType() != TokenTypes::EOI) {
            eat(TokenTypes::Semicolon);
        }
    }

    span.end.line = previousToken.getLine();
    span.end.col = previousToken.getColumn();
    span.end.filePath = previousToken.getFilePath();

    if (!assignee) {
        if (variableType == TokenTypes::Const) {
            auto constant = std::make_shared<AssignVariable>(variableName, type, value);
            constant->markAsConstant();
            constant->setPosition(startToken, currentToken);
            constant->setSpan(span);
            return constant;
        }
        auto assignment = std::make_shared<AssignVariable>(variableName, type, value);
        assignment->setPosition(startToken, currentToken);
        assignment->setSpan(span);
        return assignment;
    }

    if (auto varGetter = std::dynamic_pointer_cast<GetVariable>(assignee)) {
        auto reassignment = std::make_shared<AssignVariable>(varGetter->getName(), type, value, true);
        reassignment->setPosition(startToken, currentToken);
        reassignment->setSpan(span);
        return reassignment;
    } else if (auto reassignAccess = std::dynamic_pointer_cast<Access>(assignee)) {
        auto accessClone = std::dynamic_pointer_cast<Access>(reassignAccess->clone());
        accessClone->setAssignmentValueTo(value);
        accessClone->setPosition(startToken, currentToken);
        accessClone->setSpan(span);
        return accessClone;
    }

    std::string suggestion = "To resolve this:\n"
                           "1. Ensure assignee is a variable or accessible member\n"
                           "2. Check for correct identifier or access expression\n"
                           "3. Verify scope and accessibility";
    console.reportError(
        Console::SEMANTIC_ERROR,
        "The assignee is unassignable",
        suggestion,
        span
    );
    return nullptr;
}

} // namespace Omniscript
