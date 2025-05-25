#include <omniscript/Core.h>
#include <omniscript/utils.h>
#include <omniscript/engine/parser.h>
#include <omniscript/engine/tokens.h>
#include <omniscript/engine/Statement.h>
#include <omniscript/engine/Symboltable.h>
#include <omniscript/omniscript_pch.h>

std::shared_ptr<Statement> Parser::parseExpression() {
    return parseTernaryExpression(); // Delegate to ternary parsing first
}

std::shared_ptr<Statement> Parser::parseTernaryExpression() {
    std::shared_ptr<Statement> condition = parseBinaryExpression(); // Start with lower precedence

    if (currentToken.getType() == TokenTypes::QuestionMark) {
        eat(TokenTypes::QuestionMark);
        std::shared_ptr<Statement> truthy = parseExpression();
        eat(TokenTypes::Colon);
        std::shared_ptr<Statement> falsey = parseExpression();
        return std::make_shared<TernaryExpression>(condition, truthy, falsey);
    }

    return condition; // Return just the binary expression if no ternary is found
}

// Parse an expression, handling addition, subtraction, logical operators, and comparison operators
std::shared_ptr<Statement> Parser::parseBinaryExpression() {
    std::shared_ptr<Statement> left = logicalOrExpression(); // Start with logical OR (lowest precedence)

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

        left = std::make_shared<BinaryExpression>(left, opToken, logicalOrExpression()); // Chain with logical OR expression
    }

    return left;
}

// Parse a logical OR expression
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

// Parse a logical AND expression
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

// Parse a comparison expression, handling ==, !=, <, <=, >, >=
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

// Parse a term, handling multiplication, division, modulo, and bitwise operators
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
    // Handle prefix operators
    if (currentToken.getType() == TokenTypes::Plus ||
        currentToken.getType() == TokenTypes::Minus ||
        currentToken.getType() == TokenTypes::LogicalNot ||
        currentToken.getType() == TokenTypes::Tilde ||
        currentToken.getType() == TokenTypes::Increment ||
        currentToken.getType() == TokenTypes::Decrement) {
        TokenTypes op = currentToken.getType();
        eat(op);
        auto operand = parseUnaryExpression();
        return std::make_shared<UnaryExpression>(op, operand, UnaryExpression::Position::Prefix);
    }

    // Parse primary expression
    auto expr = factor();

    // Handle postfix operators
    while (currentToken.getType() == TokenTypes::Increment ||
           currentToken.getType() == TokenTypes::Decrement) {
        TokenTypes op = currentToken.getType();
        eat(op);
        expr = std::make_shared<UnaryExpression>(op, expr, UnaryExpression::Position::Postfix);
    }

    // Handle 'as' casting (lowest precedence postfix)
    if (currentToken.getType() == TokenTypes::As) {
        eat(TokenTypes::As);
        std::vector<std::string> typeToCastTo = parseType();
        std::shared_ptr<Omniscript::Type> type = Omniscript::resolveType(typeToCastTo);
        expr = std::make_shared<Cast>(expr, type);
    }

    return expr;
}


// Parse a factor, handling literals, identifiers, and parentheses
std::shared_ptr<Statement> Parser::factor() {
    DEBUG_LOG("Factoring a '" + getTokenTypeName(currentToken.getType()) + "' with value '" + currentToken.getValue() + "'.");

    // Handle literals
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
                // Handle BigInt case
                left = std::make_shared<BigInt>(valueStr);
            }
        } catch (const std::out_of_range&) {
            left = std::make_shared<BigInt>(valueStr);
        }
    }

    // Handle float literals (32-bit and 64-bit)
    else if (currentToken.getType() == TokenTypes::FloatLiteral) {
        eat(TokenTypes::FloatLiteral);
        std::string value = previousToken.getValue();

        // Check for 'f' or 'd' suffix to determine float type
        if (!value.empty() && (value.back() == 'f' || value.back() == 'F')) {
            left = std::make_shared<FloatLiteral>(std::stof(value)); // Float32
        } else {
            left = std::make_shared<FloatLiteral>(std::stod(value)); // Default to Float64
        }
    }

    // Handle float literals
    else if (currentToken.getType() == TokenTypes::FloatLiteral) {
        eat(TokenTypes::FloatLiteral);
        left = std::make_shared<FloatLiteral>(std::stof(previousToken.getValue())); // Assuming Float32Bit is your float type
    }
    // Handle integer literals (decimal)
    else if (currentToken.getType() == TokenTypes::IntegerLiteral) {
        eat(TokenTypes::IntegerLiteral);
        left = std::make_shared<IntegerLiteral>(std::stoll(previousToken.getValue())); // Assuming Int32Bit is your integer type
    }
    // Handle hexadecimal literals
    else if (currentToken.getType() == TokenTypes::HexLiteral) {
        eat(TokenTypes::HexLiteral);
        left = std::make_shared<IntegerLiteral>(std::stoll(previousToken.getValue(), nullptr, 16)); // Base 16
    }
    // Handle octal literals
    else if (currentToken.getType() == TokenTypes::OctalLiteral) {
        eat(TokenTypes::OctalLiteral);
        left = std::make_shared<IntegerLiteral>(std::stoll(previousToken.getValue(), nullptr, 8)); // Base 8
    }
    // Handle binary literals
    else if (currentToken.getType() == TokenTypes::BinaryLiteral) {
        eat(TokenTypes::BinaryLiteral);
        left = std::make_shared<IntegerLiteral>(std::stoll(previousToken.getValue(), nullptr, 2)); // Base 2
    }
    // Handle big integers (arbitrary-precision)
    else if (currentToken.getType() == TokenTypes::BigInt) {
        eat(TokenTypes::BigInt);
        left = std::make_shared<BigInt>(previousToken.getValue()); // Assuming BigInt is your arbitrary-precision type
    }
    // Handle string literals
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
    // Handle identifiers (variables and functions)
    else if (currentToken.getType() == TokenTypes::Identifier || currentToken.getType() == TokenTypes::New) {
        if (currentToken.getType() == TokenTypes::New) {
            eat(TokenTypes::New);
        }
        left = parseIdentifier();
        DEBUG_LOG("Parsed the identifier and got " + left->toString() + "'.");
    }
    // Handle arrays (e.g., [1, 2, 3])
    else if (currentToken.getType() == TokenTypes::LeftBracket) {
        eat(TokenTypes::LeftBracket);  // Consume the opening bracket
        std::vector<std::shared_ptr<Statement>> items;  // Store the array items

        // Parse array items (comma-separated expressions)
        while (currentToken.getType() != TokenTypes::RightBracket) {
            items.push_back(parseExpression());  // Wrap each item in an Expression
            if (currentToken.getType() == TokenTypes::Comma) {
                eat(TokenTypes::Comma);  // Consume the comma if there are more items
            } else {
                break;
            }
        }

        eat(TokenTypes::RightBracket);  // Consume the closing bracket "]"

        // Store the array directly as a value
        left = std::make_shared<Array>(items);
    }

    // Handle expressions within parentheses
    else if (currentToken.getType() == TokenTypes::LeftParen || currentToken.getType() == TokenTypes::LessThan) {
        int i = 0;
        //Todo: create an overload for tryParseTypeParametersLookahead(i) that takes in no references
        if (tryParseTypeParametersLookahead(i)) {
            parameterType paramTypes = parseTypeParametersForDeclaration();
            if (checkIfLambdaExpression()) {
                left = parseLambdaFunction("", paramTypes);
            }
        } else if (checkIfLambdaExpression()) {
            left = parseLambdaFunction();
        } else {
            eat(TokenTypes::LeftParen);
            left = parseExpression();  // Parse the expression within parentheses
            eat(TokenTypes::RightParen);
        }
    } else if (currentToken.getType() == TokenTypes::Character) {
        char32_t value = currentToken.getU32Value()[0]; 
        eat(TokenTypes::Character);
        left = std::make_shared<CharacterLiteral>(value);
    } else if (currentToken.getType() == TokenTypes::False) {
        eat(TokenTypes::False);
        left = std::make_shared<BoolLiteral>(false);
    } else if (currentToken.getType() == TokenTypes::True) {
        eat(TokenTypes::True);
        left = std::make_shared<BoolLiteral>(true);
    
    } else if (currentToken.getType() == TokenTypes::BitwiseAnd) {
        eat(TokenTypes::BitwiseAnd);
        std::string varName = currentToken.getValue();
        eat(TokenTypes::Identifier);

        left = std::make_shared<AddressOf>(varName);
    }

    // Parse objects and dictionaries
    // {a = 0, b = 1}
    // {a, b} 
    else if (currentToken.getType() == TokenTypes::LeftBrace) {
        left = parseObject();
    }

    // Handle dot operator for method calls (e.g., object.method())
    while (currentToken.getType() == TokenTypes::Dot) {
        eat(TokenTypes::Dot);  // Consume the dot operator

        // Ensure the next token is an identifier (method name or property)
        if (currentToken.getType() != TokenTypes::Identifier) {
            throw std::runtime_error("Expected an identifier after the '.' operator.");
        }

        std::string methodName = currentToken.getValue();
        eat(TokenTypes::Identifier);  // Consume the method name

        if (currentToken.getType() == TokenTypes::LeftParen || currentToken.getType() == TokenTypes::LessThan) {
            DEBUG_LOG("Got method " + methodName);

            // Parse the method call arguments
            auto args = parseArguments();

            // Create call for method invocation
            left = std::make_shared<Call>(left, methodName, args);
        } 
        // else {
        //     DEBUG_LOG("Got property " + methodName);
        //     left = std::make_shared<MemberAccess>(left, methodName);
        // }
    }
    
    // Parse a parentheses after a string, dictionary, or an array to access a key
    while (currentToken.getType() == TokenTypes::LeftBracket) {
        eat(TokenTypes::LeftBracket);

        std::vector<std::shared_ptr<Statement>> args;    
        args.push_back(parseExpression());

        eat(TokenTypes::RightBracket);
        left = std::make_shared<Call>(left, "get", args);
    }

    // Return the final result wrapped in a BinaryExpression
    // return std::make_shared<BinaryExpression>(left);
    return left;
}
