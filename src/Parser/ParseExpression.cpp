// ParseExpression.cpp
#include <cstdio>
#include <variant>
#include <omniscript/Statements/Statement.h>
#include <omniscript/Statements/AccessStatements.h>
#include <omniscript/Statements/LiteralStatements.h>
#include <omniscript/Statements/ExpressionStatements.h>
#include <omniscript/Statements/AssignmentAndGetterStatements.h>

#include <omniscript/Core.h>
#include <omniscript/utils.h>
#include <omniscript/Parser.h>
#include <omniscript/Tokens.h>
#include <omniscript/Types/Types.h>
#include <omniscript/Symboltable.h>
#include <omniscript/omniscript_pch.h>
#include <omniscript/Statements/Statement.h>

#include <quadmath.h>

namespace Omniscript {

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
    std::shared_ptr<Statement> left = parseAs();

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

        std::shared_ptr<Statement> right = parseAs(); // Fixed: call same level
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
                // &obj.field / &arr[i]: reference the access expression itself so that codegen
                // produces its address (GEP) instead of the loaded value.
                operand = std::make_shared<ReferenceTo>(access);
            } else {
                console.reportError(Console::SEMANTIC_ERROR, "Cannot get the reference of a symbol that isn't a variable '" + operand->toString() + "'.", FileSpan{});
            }
        }

        return std::make_shared<ASTUnaryExpression>(op, operand, ASTUnaryExpression::Position::Prefix);
    }

    auto expr = factor();

    while (currentToken.getType() == TokenTypes::Increment ||
           currentToken.getType() == TokenTypes::Decrement) {
        TokenTypes op = currentToken.getType();
        eat(op);
        expr = std::make_shared<ASTUnaryExpression>(op, expr, ASTUnaryExpression::Position::Postfix);
    }

    return expr;
}

// Handles `expr as T` — binds looser than unary operators (`&x as T` = `(&x) as T`)
// but tighter than binary (`a + b as T` = `a + (b as T)`). Rust/Swift/C++ all agree.
std::shared_ptr<Statement> Parser::parseAs() {
    std::shared_ptr<Statement> expr = parseUnaryExpression();

    while (currentToken.getType() == TokenTypes::As) {
        eat(TokenTypes::As);
        std::vector<std::string> typeToCastTo = parseType();
        std::shared_ptr<Type> type = resolveType(typeToCastTo);
        expr = std::make_shared<ASTCast>(expr, type);
    }

    return expr;
}

// Reads an integer literal token whether the lexer stored it in the variant or as text.
static int64_t literalToInt(const Token& t, int base) {
    try {
        if (auto v = std::get_if<int64_t>(&t.getVariantValue())) return *v;
        std::string s = t.getValue();
        if (s.size() > 2 && s[0] == '0' &&
            (s[1]=='x'||s[1]=='X'||s[1]=='b'||s[1]=='B'||s[1]=='o'||s[1]=='O'))
            s = s.substr(2);
        return static_cast<int64_t>(std::stoull(s, nullptr, base));
    } catch (const std::exception&) {
        console.reportError(Console::SYNTAX_ERROR, "Invalid numeric literal '" + t.getValue() + "'.", FileSpan{});
        return 0;
    }
}

std::shared_ptr<Statement> Parser::factor() {
    Token startToken = currentToken;
    DEBUG_LOG("Factoring a '" + getTokenTypeName(currentToken.getType()) + "' with value '" + currentToken.getValue() + "'.");

    std::shared_ptr<Statement> left;

    if (currentToken.getType() == TokenTypes::IntegerLiteral) {
        eat(TokenTypes::IntegerLiteral);
        left = std::make_shared<IntegerLiteral>(literalToInt(previousToken, 10));
    }


   else if (currentToken.getType() == TokenTypes::FloatLiteral) {
        eat(TokenTypes::FloatLiteral);
        // The lexer stores float literals as a double in the token's variant, not as text,
        // so getValue() would return an empty string (-> 0.0). Read the double instead.
        std::string value;
        if (std::holds_alternative<double>(previousToken.getVariantValue())) {
            char buf[64];
            std::snprintf(buf, sizeof(buf), "%.17g", previousToken.getFloatValue());
            value = buf;
        } else {
            value = previousToken.getValue();
        }

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

    else if (currentToken.getType() == TokenTypes::HexLiteral) {
        eat(TokenTypes::HexLiteral);
        left = std::make_shared<IntegerLiteral>(literalToInt(previousToken, 16));
    }

    else if (currentToken.getType() == TokenTypes::OctalLiteral) {
        eat(TokenTypes::OctalLiteral);
        left = std::make_shared<IntegerLiteral>(literalToInt(previousToken, 8));
    }

    else if (currentToken.getType() == TokenTypes::BinaryLiteral) {
        eat(TokenTypes::BinaryLiteral);
        left = std::make_shared<IntegerLiteral>(literalToInt(previousToken, 2));
    }

    else if (currentToken.getType() == TokenTypes::BigInt) {
        eat(TokenTypes::BigInt);
        left = std::make_shared<ASTBigInt>(previousToken.getValue()); // Assuming BigInt is your arbitrary-precision type
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

    if (!left) {
        console.reportError(Console::SYNTAX_ERROR, "Unexpected token '" + currentToken.getValue() + "' in expression.", FileSpan{});
        // consume it so the parser always makes progress
        if (currentToken.getType() != TokenTypes::EOI) {
            previousToken = currentToken;
            currentToken = lexer.getNextToken();
        }
        return nullptr;
    }

    left->setPosition(startToken, currentToken);

    return left;
}

} // namespace Omniscript