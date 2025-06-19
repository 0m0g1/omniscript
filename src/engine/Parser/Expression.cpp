#include <omniscript/Core.h>
#include <omniscript/utils.h>
#include <omniscript/engine/Parser.h>
#include <omniscript/engine/tokens.h>
#include <omniscript/engine/Statement.h>
#include <omniscript/engine/Symboltable.h>
#include <omniscript/omniscript_pch.h>

std::shared_ptr<Statement> Parser::parseExpression() {
    return parseTernaryExpression();
}

std::shared_ptr<Statement> Parser::parseTernaryExpression() {
    std::shared_ptr<Statement> condition = parseBinaryExpression();

    if (currentToken.getType() == TokenTypes::QuestionMark) {
        eat(TokenTypes::QuestionMark);
        std::shared_ptr<Statement> truthy = parseExpression();
        eat(TokenTypes::Colon);
        std::shared_ptr<Statement> falsey = parseExpression();
        return std::make_shared<TernaryExpression>(condition, truthy, falsey);
    }

    return condition;
}

std::shared_ptr<Statement> Parser::parseBinaryExpression() {
    std::shared_ptr<Statement> left = logicalOrExpression();

    while (currentToken.getType() == TokenTypes::Plus || currentToken.getType() == TokenTypes::Minus ||
           currentToken.getType() == TokenTypes::LogicalAnd || currentToken.getType() == TokenTypes::LogicalOr) {
        Token opToken = currentToken;
        TokenTypes op = currentToken.getType();
        
        if (op == TokenTypes::Plus) {
            eat(TokenTypes::Plus);
        } else if (op == TokenTypes::Minus) {
            eat(TokenTypes::Minus);
        } else if (op == TokenTypes::LogicalAnd) {
            eat(TokenTypes::LogicalAnd);
        } else if (op == TokenTypes::LogicalOr) {
            eat(TokenTypes::LogicalOr);
        }

        left = std::make_shared<BinaryExpression>(left, opToken, logicalOrExpression());
    }

    return left;
}

std::shared_ptr<Statement> Parser::logicalOrExpression() {
    std::shared_ptr<Statement> left = logicalAndExpression();

    while (currentToken.getType() == TokenTypes::LogicalOr) {
        Token opToken = currentToken;
        TokenTypes op = TokenTypes::LogicalOr;
        eat(TokenTypes::LogicalOr);
        left = std::make_shared<BinaryExpression>(left, currentToken, logicalAndExpression());
    }

    return left;
}

std::shared_ptr<Statement> Parser::logicalAndExpression() {
    std::shared_ptr<Statement> left = comparisonExpression();

    while (currentToken.getType() == TokenTypes::LogicalAnd) {
        Token opToken = currentToken;
        TokenTypes op = TokenTypes::LogicalAnd;
        eat(TokenTypes::LogicalAnd);
        left = std::make_shared<BinaryExpression>(left, opToken, comparisonExpression());
    }

    return left;
}

std::shared_ptr<Statement> Parser::comparisonExpression() {
    std::shared_ptr<Statement> left = term();

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

        left = std::make_shared<BinaryExpression>(left, operationTok, term());
    }

    return left;
}

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

        left = std::make_shared<BinaryExpression>(left, opToken, parseUnaryExpression());
    }

    return left;
}

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

        return std::make_shared<UnaryExpression>(op, operand, UnaryExpression::Position::Prefix);
    }

    auto expr = factor();

    while (currentToken.getType() == TokenTypes::Increment ||
           currentToken.getType() == TokenTypes::Decrement) {
        TokenTypes op = currentToken.getType();
        eat(op);
        expr = std::make_shared<UnaryExpression>(op, expr, UnaryExpression::Position::Postfix);
    }

    if (currentToken.getType() == TokenTypes::As) {
        eat(TokenTypes::As);
        std::vector<std::string> typeToCastTo = parseType();
        std::shared_ptr<Omniscript::Type> type = Omniscript::resolveType(typeToCastTo);
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
        eat(TokenTypes::StringLiteral);
        left = std::make_shared<StringLiteral>(previousToken.getU32Value());
    } else if (currentToken.getType() == TokenTypes::BitwiseAnd) {
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
