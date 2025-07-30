#include <omniscript/Statements/Statement.h>
#include <omniscript/Statements/AccessStatements.h>
#include <omniscript/Statements/LiteralStatements.h>
#include <omniscript/Statements/ExpressionStatements.h>
#include <omniscript/Statements/AssignmentAndGetterStatements.h>

#include <omniscript/Core.h>
#include <omniscript/utils.h>
#include <omniscript/Parser.h>
#include <omniscript/Tokens.h>
#include <omniscript/omniscript_pch.h>
#include <omniscript/Statements/Statement.h>
#include <omniscript/Symboltable.h>

#include <quadmath.h>

std::shared_ptr<Statement> Parser::parseExpression() {
    return parseTernaryExpression();
}

std::shared_ptr<Statement> Parser::parseTernaryExpression() {
    std::shared_ptr<Statement> condition = logicalOrExpression(); // Fixed: should start with lowest precedence

    if (currentToken.getType() == TokenTypes::QuestionMark) {
        eat(TokenTypes::QuestionMark);
        std::shared_ptr<Statement> truthy = parseExpression();
        eat(TokenTypes::Colon);
        std::shared_ptr<Statement> falsey = parseExpression();
        return std::make_shared<ASTTernaryExpression>(condition, truthy, falsey);
    }

    return condition;
}

// Addition and Subtraction (lowest precedence binary operators)
std::shared_ptr<Statement> Parser::parseBinaryExpression() {
    std::shared_ptr<Statement> left = term(); // Fixed: should call higher precedence

    while (currentToken.getType() == TokenTypes::Plus || currentToken.getType() == TokenTypes::Minus) {
        Token opToken = currentToken;
        TokenTypes op = currentToken.getType();
        
        if (op == TokenTypes::Plus) {
            eat(TokenTypes::Plus);
        } else if (op == TokenTypes::Minus) {
            eat(TokenTypes::Minus);
        }

        std::shared_ptr<Statement> right = term(); // Fixed: call same level for left-associativity
        left = std::make_shared<ASTBinaryExpression>(left, opToken, right);
    }

    return left;
}

// Logical OR (lowest precedence)
std::shared_ptr<Statement> Parser::logicalOrExpression() {
    std::shared_ptr<Statement> left = logicalAndExpression();

    while (currentToken.getType() == TokenTypes::LogicalOr) {
        Token opToken = currentToken;
        eat(TokenTypes::LogicalOr);
        std::shared_ptr<Statement> right = logicalAndExpression(); // Fixed: correct for left-associativity
        left = std::make_shared<ASTBinaryExpression>(left, opToken, right);
    }

    return left;
}

// Logical AND
std::shared_ptr<Statement> Parser::logicalAndExpression() {
    std::shared_ptr<Statement> left = comparisonExpression();

    while (currentToken.getType() == TokenTypes::LogicalAnd) {
        Token opToken = currentToken;
        eat(TokenTypes::LogicalAnd);
        std::shared_ptr<Statement> right = comparisonExpression(); // Fixed: correct for left-associativity
        left = std::make_shared<ASTBinaryExpression>(left, opToken, right);
    }

    return left;
}

// Comparison operators
std::shared_ptr<Statement> Parser::comparisonExpression() {
    std::shared_ptr<Statement> left = parseBinaryExpression(); // Fixed: call addition/subtraction level

    while (currentToken.getType() == TokenTypes::Equals || currentToken.getType() == TokenTypes::NotEquals ||
           currentToken.getType() == TokenTypes::LessThan || currentToken.getType() == TokenTypes::LessEqual ||
           currentToken.getType() == TokenTypes::GreaterThan || currentToken.getType() == TokenTypes::GreaterEqual) {
        Token operationTok = currentToken;
        TokenTypes op = currentToken.getType();
        
        if (op == TokenTypes::Equals) {
            eat(TokenTypes::Equals);
        } else if (op == TokenTypes::NotEquals) {
            eat(TokenTypes::NotEquals);
        } else if (op == TokenTypes::LessThan) {
            eat(TokenTypes::LessThan);
        } else if (op == TokenTypes::LessEqual) {
            eat(TokenTypes::LessEqual);
        } else if (op == TokenTypes::GreaterThan) {
            eat(TokenTypes::GreaterThan);
        } else if (op == TokenTypes::GreaterEqual) {
            eat(TokenTypes::GreaterEqual);
        }

        std::shared_ptr<Statement> right = parseBinaryExpression(); // Fixed: call same level
        left = std::make_shared<ASTBinaryExpression>(left, operationTok, right);
    }

    return left;
}

// Multiplicative, bitwise, and shift operators (highest precedence binary)
std::shared_ptr<Statement> Parser::term() {
    std::shared_ptr<Statement> left = parseUnaryExpression();

    while (currentToken.getType() == TokenTypes::Multiply || currentToken.getType() == TokenTypes::Divide ||
           currentToken.getType() == TokenTypes::Modulo || currentToken.getType() == TokenTypes::BitwiseAnd ||
           currentToken.getType() == TokenTypes::BitwiseOr || currentToken.getType() == TokenTypes::BitwiseXor ||
           currentToken.getType() == TokenTypes::ShiftLeft || currentToken.getType() == TokenTypes::ShiftRight) {
        
        Token opToken = currentToken;
        TokenTypes op = currentToken.getType();

        if (op == TokenTypes::Multiply) {
            eat(TokenTypes::Multiply);
        } else if (op == TokenTypes::Divide) {
            eat(TokenTypes::Divide);
        } else if (op == TokenTypes::Modulo) {
            eat(TokenTypes::Modulo);
        } else if (op == TokenTypes::BitwiseAnd) {
            eat(TokenTypes::BitwiseAnd);
        } else if (op == TokenTypes::BitwiseOr) {
            eat(TokenTypes::BitwiseOr);
        } else if (op == TokenTypes::BitwiseXor) {
            eat(TokenTypes::BitwiseXor);
        } else if (op == TokenTypes::ShiftLeft) {
            eat(TokenTypes::ShiftLeft);
        } else if (op == TokenTypes::ShiftRight) {
            eat(TokenTypes::ShiftRight);
        } 

        std::shared_ptr<Statement> right = parseUnaryExpression(); // Fixed: call same level
        left = std::make_shared<ASTBinaryExpression>(left, opToken, right);
    }

    return left;
}

// Rest of the methods remain the same...
std::shared_ptr<Statement> Parser::parseUnaryExpression() {
    if (currentToken.getType() == TokenTypes::Plus ||
        currentToken.getType() == TokenTypes::Minus ||
        currentToken.getType() == TokenTypes::LogicalNot ||
        currentToken.getType() == TokenTypes::Tilde ||
        currentToken.getType() == TokenTypes::Increment ||
        currentToken.getType() == TokenTypes::Decrement ||
        currentToken.getType() == TokenTypes::Multiply ||
        currentToken.getType() == TokenTypes::BitwiseAnd
    ) {
        TokenTypes op = currentToken.getType();
        eat(op);
        auto operand = parseUnaryExpression();
        
        if (op == TokenTypes::Plus) {
            return operand;
        } else if (op == TokenTypes::Minus) {
            if (auto integerLiteral = std::dynamic_pointer_cast<IntegerLiteral>(operand)) {
                integerLiteral->value = -integerLiteral->value; 
                return operand;
            } else if (auto floatLiteral = std::dynamic_pointer_cast<FloatLiteral>(operand)) {
                floatLiteral->value = -floatLiteral->value; 
                return operand;
            }
        } else if (op == TokenTypes::LogicalNot) {
            if (auto boolLiteral = std::dynamic_pointer_cast<BoolLiteral>(operand)) {
                boolLiteral->value = !boolLiteral->value; 
                return operand;
            }
        } else if (op == TokenTypes::Tilde) {
            if (auto integerLiteral = std::dynamic_pointer_cast<IntegerLiteral>(operand)) {
                integerLiteral->value = ~integerLiteral->value;
                return operand;
            }
        } else if (op == TokenTypes::BitwiseAnd) {
            if (auto variable = std::dynamic_pointer_cast<GetVariable>(operand)) {
                operand = std::make_shared<ReferenceTo>(variable->getName());
            } else if (auto access = std::dynamic_pointer_cast<Access>(operand)) {
                // Todo:: create a referent to an access
                // operand = std::make_shared<ReferenceTo>(Access);
            } else {
                console.error("Cannot get the reference of a symbol that isn't a variable '" + operand->toString() + "'.");
            }
        }

        return std::make_shared<ASTUnaryExpression>(op, operand, UnaryExpression::Position::Prefix);
    }

    auto expr = factor();

    while (currentToken.getType() == TokenTypes::Increment ||
           currentToken.getType() == TokenTypes::Decrement) {
        TokenTypes op = currentToken.getType();
        eat(op);
        expr = std::make_shared<ASTUnaryExpression>(op, expr, UnaryExpression::Position::Postfix);
    }

    if (currentToken.getType() == TokenTypes::As) {
        eat(TokenTypes::As);
        std::vector<std::string> typeToCastTo = parseType();
        std::shared_ptr<Type> type = resolveType(typeToCastTo);
        expr = std::make_shared<Cast>(expr, type);
    }

    return expr;
}

std::shared_ptr<Statement> Parser::factor() {
    DEBUG_LOG("Factoring a '" + getTokenTypeName(currentToken.getType()) + "' with value '" + currentToken.getValue() + "'.");

    std::shared_ptr<Statement> left;

    if (currentToken.getType() == TokenTypes::IntegerLiteral) {
        eat(TokenTypes::IntegerLiteral);
        std::string valueStr = previousToken.getValue();
    
        try {
            long long value = std::stoll(valueStr);
    
            if (value >= std::numeric_limits<int32_t>::min() && value <= std::numeric_limits<int32_t>::max()) {
                left = std::make_shared<IntegerLiteral>(static_cast<int64_t>(value));
            } else if (value >= std::numeric_limits<int64_t>::min() && value <= std::numeric_limits<int64_t>::max()) {
                left = std::make_shared<IntegerLiteral>(static_cast<int64_t>(value));
            } else {
                left = std::make_shared<BigInt>(valueStr);
            }
        } catch (const std::out_of_range&) {
            left = std::make_shared<BigInt>(valueStr);
        }
    }

   else if (currentToken.getType() == TokenTypes::FloatLiteral) {
        eat(TokenTypes::FloatLiteral);
        std::string value = previousToken.getValue();

        bool isFloat = false;
        bool isDouble = false;
        bool isLong = false;

        if (!value.empty()) {
            char last = value.back();
            if (last == 'f' || last == 'F') {
                isFloat = true;
                value.pop_back();
            } else if (last == 'd' || last == 'D') {
                isDouble = true;
                value.pop_back();
            } else if (last == 'l' || last == 'L') {
                isLong = true;
                value.pop_back();
            }
        }

        __float128 f128_value = strtoflt128(value.c_str(), nullptr);
        auto floatStmt = std::make_shared<FloatLiteral>(f128_value);
        floatStmt->isFloat32 = isFloat;
        floatStmt->isFloat64 = isDouble;
        floatStmt->isFloat80 = isLong;

        left = floatStmt;
    }

    else if (currentToken.getType() == TokenTypes::IntegerLiteral) {
        eat(TokenTypes::IntegerLiteral);
        left = std::make_shared<IntegerLiteral>(std::stoll(previousToken.getValue())); // Assuming Int32Bit is your integer type
    }

    else if (currentToken.getType() == TokenTypes::HexLiteral) {
        eat(TokenTypes::HexLiteral);
        left = std::make_shared<IntegerLiteral>(std::stoll(previousToken.getValue(), nullptr, 16)); // Base 16
    }

    else if (currentToken.getType() == TokenTypes::OctalLiteral) {
        eat(TokenTypes::OctalLiteral);
        left = std::make_shared<IntegerLiteral>(std::stoll(previousToken.getValue(), nullptr, 8)); // Base 8
    }

    else if (currentToken.getType() == TokenTypes::BinaryLiteral) {
        eat(TokenTypes::BinaryLiteral);
        left = std::make_shared<IntegerLiteral>(std::stoll(previousToken.getValue(), nullptr, 2)); // Base 2
    }

    else if (currentToken.getType() == TokenTypes::BigInt) {
        eat(TokenTypes::BigInt);
        left = std::make_shared<BigInt>(previousToken.getValue()); // Assuming BigInt is your arbitrary-precision type
    }

    else if (currentToken.getType() == TokenTypes::StringLiteral) {
        left = std::make_shared<StringLiteral>(parseStringLiteral());
    }

    else if (currentToken.getType() == TokenTypes::TemplateTail) {
        eat(TokenTypes::TemplateTail);
        left = std::make_shared<StringLiteral>(previousToken.getU32Value());
    }
    
    else if (currentToken.getType() == TokenTypes::BitwiseAnd) {
        eat(TokenTypes::BitwiseAnd);
        if (currentToken.getType() == TokenTypes::Identifier) {
            std::string varName = currentToken.getValue();
            eat(TokenTypes::Identifier);
            left = std::make_shared<AddressOf>(varName);
        }
    } else if (currentToken.getType() == TokenTypes::Nullptr) {
        eat(TokenTypes::Nullptr);
        left = std::make_shared<Nullptr>();
    } else if (currentToken.getType() == TokenTypes::Null) {
        eat(TokenTypes::Null);
        left = std::make_shared<Null>();
    }

    else if (currentToken.getType() == TokenTypes::Identifier || currentToken.getType() == TokenTypes::New) {
        if (currentToken.getType() == TokenTypes::New) {
            eat(TokenTypes::New);
        }
        left = parseIdentifier();
        DEBUG_LOG("Parsed the identifier and got " + left->toString() + "'.");
    }

    else if (currentToken.getType() == TokenTypes::LeftBracket) {
        eat(TokenTypes::LeftBracket);  
        std::vector<std::shared_ptr<Statement>> items;

        while (currentToken.getType() != TokenTypes::RightBracket) {
            items.push_back(parseExpression());
            if (currentToken.getType() == TokenTypes::Comma) {
                eat(TokenTypes::Comma);
            } else {
                break;
            }
        }

        eat(TokenTypes::RightBracket);  
        left = std::make_shared<Array>(items);
    }

    else if (currentToken.getType() == TokenTypes::LeftParen || currentToken.getType() == TokenTypes::LessThan) {
        int i = 0;
        if (tryParseTypeParametersLookahead(i)) {
            parameterType paramTypes = parseTypeParametersForDeclaration();
            if (checkIfLambdaExpression()) {
                left = parseLambdaFunction("", paramTypes);
            }
        } else if (checkIfLambdaExpression()) {
            left = parseLambdaFunction();
        } else {
            eat(TokenTypes::LeftParen);
            left = parseExpression();
            eat(TokenTypes::RightParen);
        }
    } 
    
    else if (currentToken.getType() == TokenTypes::Character) {
        char32_t value = currentToken.getU32Value()[0]; 
        eat(TokenTypes::Character);
        left = std::make_shared<CharacterLiteral>(value);
    }
    
    else if (currentToken.getType() == TokenTypes::False) {
        eat(TokenTypes::False);
        left = std::make_shared<BoolLiteral>(false);
    }
    
    else if (currentToken.getType() == TokenTypes::True) {
        eat(TokenTypes::True);
        left = std::make_shared<BoolLiteral>(true);
    
    }
    
    else if (currentToken.getType() == TokenTypes::BitwiseAnd) {
        eat(TokenTypes::BitwiseAnd);
        std::string varName = currentToken.getValue();
        eat(TokenTypes::Identifier);

        left = std::make_shared<AddressOf>(varName);
    }

    return left;
}
