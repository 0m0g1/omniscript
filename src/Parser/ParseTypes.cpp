#include <omniscript/Core.h>
#include <omniscript/utils.h>
#include <omniscript/Lexer.h>
#include <omniscript/Tokens.h>
#include <omniscript/Parser.h>
#include <omniscript/Types/Types.h>
#include <omniscript/Symboltable.h>
#include <omniscript/omniscript_pch.h>
#include <omniscript/Statements/Statement.h>
#include <omniscript/Statements/CallableStatement.h>
#include <omniscript/Statements/FunctionStatement.h>
#include <omniscript/Statements/LiteralStatements.h>
#include <omniscript/Statements/TypeStatements.h>

namespace Omniscript {

bool Parser::tryParseTypeParametersLookahead(int& i) {
    // No span needed since this is a lookahead function
    if ((i == 0 ? currentToken.getType() : lexer.peekToken(i).getType()) != TokenTypes::LessThan)
        return false;

    i++; // Skip '<'

    while (lexer.peekToken(i).getType() == TokenTypes::Identifier) {
        i++;

        if (lexer.peekToken(i).getType() == TokenTypes::Extends) {
            i++;

            while (true) {
                TokenTypes t = lexer.peekToken(i).getType();
                if (t == TokenTypes::Variant || t == TokenTypes::Any) {
                    i++;
                } else if (t == TokenTypes::Identifier) {
                    while (lexer.peekToken(i).getType() == TokenTypes::Identifier || lexer.peekToken(i).getType() == TokenTypes::Dot) {
                        i++;
                    }
                } else {
                    return false;
                }

                if (lexer.peekToken(i).getType() == TokenTypes::BitwiseOr) {
                    i++;
                } else {
                    break;
                }
            }
        }

        if (lexer.peekToken(i).getType() == TokenTypes::Comma) {
            i++;
        } else {
            break;
        }
    }

    if (lexer.peekToken(i).getType() != TokenTypes::GreaterThan) {
        return false;
    }

    i++; // Skip '>'

    return true;
}

bool Parser::isGenericCallOrConstructor() {
    // No span needed since this is a lookahead function
    int i = 0;

    if ((i == 0 ? currentToken.getType() : lexer.peekToken(i).getType()) != TokenTypes::LessThan)
        return false;

    i++;

    while (true) {
        if (lexer.peekToken(i).getType() != TokenTypes::Identifier)
            return false;

        i++;

        while (lexer.peekToken(i).getType() == TokenTypes::Dot) {
            i++;
            if (lexer.peekToken(i).getType() != TokenTypes::Identifier)
                return false;
            i++;
        }

        if (lexer.peekToken(i).getType() == TokenTypes::Comma) {
            i++;
        } else {
            break;
        }
    }

    if (lexer.peekToken(i).getType() != TokenTypes::GreaterThan)
        return false;

    i++; // Skip '>'

    return lexer.peekToken(i).getType() == TokenTypes::LeftParen;
}

parameterType Parser::parseTypeParametersForDeclaration() {
    FileSpan span;
    span.start.line = currentToken.getLine();
    span.start.col = currentToken.getColumn();
    span.start.filePath = currentToken.getFilePath();

    parameterType typeParams;

    if (currentToken.getType() == TokenTypes::LessThan) {
        eat(TokenTypes::LessThan, [&]() {
            std::string suggestion = Console::formatString(
                "To resolve this:\n"
                "1. Start type parameters with '<'\n"
                "2. Check generic type syntax\n"
                "3. Expected token: '<', found '%s'",
                getTokenTypeName(currentToken.getType()).c_str()
            );
            console.reportError(
                Console::SYNTAX_ERROR,
                Console::formatString("Expected '<' for type parameters, found '%s'", 
                    getTokenTypeName(currentToken.getType()).c_str()),
                suggestion,
                span
            );
        });

        while (currentToken.getType() == TokenTypes::Identifier) {
            std::string typeName = currentToken.getValue();
            eat(TokenTypes::Identifier, [&]() {
                std::string suggestion = Console::formatString(
                    "To resolve this:\n"
                    "1. Provide a valid type parameter name\n"
                    "2. Check generic type syntax\n"
                    "3. Expected token: identifier, found '%s'",
                    getTokenTypeName(currentToken.getType()).c_str()
                );
                console.reportError(
                    Console::SYNTAX_ERROR,
                    Console::formatString("Expected identifier for type parameter, found '%s'", 
                        getTokenTypeName(currentToken.getType()).c_str()),
                    suggestion,
                    span
                );
            });

            std::vector<std::vector<std::string>> constraintList;

            if (currentToken.getType() == TokenTypes::Extends) {
                eat(TokenTypes::Extends, [&]() {
                    std::string suggestion = Console::formatString(
                        "To resolve this:\n"
                        "1. Use 'extends' for type constraints\n"
                        "2. Check constraint syntax\n"
                        "3. Expected token: 'extends', found '%s'",
                        getTokenTypeName(currentToken.getType()).c_str()
                    );
                    console.reportError(
                        Console::SYNTAX_ERROR,
                        Console::formatString("Expected 'extends' for type constraint, found '%s'", 
                            getTokenTypeName(currentToken.getType()).c_str()),
                        suggestion,
                        span
                    );
                });

                while (true) {
                    if (currentToken.getType() == TokenTypes::Variant) {
                        constraintList.push_back({ "variant" });
                        eat(TokenTypes::Variant);
                    } else if (currentToken.getType() == TokenTypes::Any) {
                        constraintList.push_back({ "any" });
                        eat(TokenTypes::Any);
                    } else {
                        std::vector<std::string> parsedType = parseType();
                        if (parsedType.empty()) {
                            console.reportError(
                                Console::SYNTAX_ERROR,
                                "Invalid type constraint",
                                "To resolve this:\n1. Provide a valid type constraint\n2. Check type syntax\n3. Ensure valid type or identifier",
                                span
                            );
                            return typeParams;
                        }
                        constraintList.push_back(parsedType);
                    }

                    if (currentToken.getType() == TokenTypes::BitwiseOr) {
                        eat(TokenTypes::BitwiseOr, [&]() {
                            std::string suggestion = Console::formatString(
                                "To resolve this:\n"
                                "1. Use '|' for multiple type constraints\n"
                                "2. Check constraint syntax\n"
                                "3. Expected token: '|', found '%s'",
                                getTokenTypeName(currentToken.getType()).c_str()
                            );
                            console.reportError(
                                Console::SYNTAX_ERROR,
                                Console::formatString("Expected '|' for type constraint union, found '%s'", 
                                    getTokenTypeName(currentToken.getType()).c_str()),
                                suggestion,
                                span
                            );
                        });
                    } else {
                        break;
                    }
                }
            }

            std::string constraintStr = constraintList.empty() ? "none" :
                std::accumulate(std::next(constraintList.begin()), constraintList.end(),
                    join(constraintList[0], "."),
                    [](const std::string& acc, const std::vector<std::string>& typeVec) {
                        return acc + " , " + join(typeVec, ".");
                    });

            DEBUG_LOG("TypeName: " + typeName + ", Constraint: [" + constraintStr + "]");

            typeParams.emplace_back(typeName, constraintList);

            if (currentToken.getType() == TokenTypes::Comma) {
                eat(TokenTypes::Comma, [&]() {
                    std::string suggestion = Console::formatString(
                        "To resolve this:\n"
                        "1. Use ',' to separate type parameters\n"
                        "2. Check generic type syntax\n"
                        "3. Expected token: ',', found '%s'",
                        getTokenTypeName(currentToken.getType()).c_str()
                    );
                    console.reportError(
                        Console::SYNTAX_ERROR,
                        Console::formatString("Expected ',' between type parameters, found '%s'", 
                            getTokenTypeName(currentToken.getType()).c_str()),
                        suggestion,
                        span
                    );
                });
            } else {
                break;
            }
        }

        eat(TokenTypes::GreaterThan, [&]() {
            std::string suggestion = Console::formatString(
                "To resolve this:\n"
                "1. Close type parameters with '>'\n"
                "2. Check generic type syntax\n"
                "3. Expected token: '>', found '%s'",
                getTokenTypeName(currentToken.getType()).c_str()
            );
            console.reportError(
                Console::SYNTAX_ERROR,
                Console::formatString("Expected '>' to close type parameters, found '%s'", 
                    getTokenTypeName(currentToken.getType()).c_str()),
                suggestion,
                span
            );
        });
    }

    span.end.line = previousToken.getLine();
    span.end.col = previousToken.getColumn();
    span.end.filePath = previousToken.getFilePath();

    return typeParams;
}

std::vector<std::string> Parser::parseType() {
    FileSpan span;
    span.start.line = currentToken.getLine();
    span.start.col = currentToken.getColumn();
    span.start.filePath = currentToken.getFilePath();

    std::vector<std::string> dataTypes;
    int prevColumn = -1;

    auto hasWhitespace = [&](Token& token) {
        return prevColumn != -1 && token.getColumn() > prevColumn + 1;
    };

    if (currentToken.getType() == TokenTypes::Function) {
        dataTypes.push_back("fn");
        prevColumn = currentToken.getColumn();
        eat(TokenTypes::Function, [&]() {
            std::string suggestion = Console::formatString(
                "To resolve this:\n"
                "1. Use 'fn' for function type\n"
                "2. Check function type syntax\n"
                "3. Expected token: 'fn', found '%s'",
                getTokenTypeName(currentToken.getType()).c_str()
            );
            console.reportError(
                Console::SYNTAX_ERROR,
                Console::formatString("Expected 'fn' for function type, found '%s'", 
                    getTokenTypeName(currentToken.getType()).c_str()),
                suggestion,
                span
            );
        });

        if (currentToken.getType() == TokenTypes::LeftParen) {
            dataTypes.push_back("(");
            prevColumn = currentToken.getColumn();
            eat(TokenTypes::LeftParen, [&]() {
                std::string suggestion = Console::formatString(
                    "To resolve this:\n"
                    "1. Start parameter list with '('\n"
                    "2. Check function type syntax\n"
                    "3. Expected token: '(', found '%s'",
                    getTokenTypeName(currentToken.getType()).c_str()
                );
                console.reportError(
                    Console::SYNTAX_ERROR,
                    Console::formatString("Expected '(' for function parameters, found '%s'", 
                        getTokenTypeName(currentToken.getType()).c_str()),
                    suggestion,
                    span
                );
            });

            while (currentToken.getType() != TokenTypes::RightParen) {
                if (currentToken.getType() == TokenTypes::Identifier) {
                    Token nextToken = lexer.peekToken(1);
                    if (nextToken.getType() == TokenTypes::Colon) {
                        dataTypes.push_back(currentToken.getValue());
                        prevColumn = currentToken.getColumn();
                        eat(TokenTypes::Identifier, [&]() {
                            std::string suggestion = Console::formatString(
                                "To resolve this:\n"
                                "1. Provide a valid parameter name\n"
                                "2. Check function parameter syntax\n"
                                "3. Expected token: identifier, found '%s'",
                                getTokenTypeName(currentToken.getType()).c_str()
                            );
                            console.reportError(
                                Console::SYNTAX_ERROR,
                                Console::formatString("Expected identifier for parameter name, found '%s'", 
                                    getTokenTypeName(currentToken.getType()).c_str()),
                                suggestion,
                                span
                            );
                        });

                        dataTypes.push_back(":");
                        prevColumn = currentToken.getColumn();
                        eat(TokenTypes::Colon, [&]() {
                            std::string suggestion = Console::formatString(
                                "To resolve this:\n"
                                "1. Use ':' after parameter name\n"
                                "2. Check function parameter syntax\n"
                                "3. Expected token: ':', found '%s'",
                                getTokenTypeName(currentToken.getType()).c_str()
                            );
                            console.reportError(
                                Console::SYNTAX_ERROR,
                                Console::formatString("Expected ':' after parameter name, found '%s'", 
                                    getTokenTypeName(currentToken.getType()).c_str()),
                                suggestion,
                                span
                            );
                        });
                    }
                }

                std::vector<std::string> paramType = parseType();
                if (paramType.empty()) {
                    console.reportError(
                        Console::SYNTAX_ERROR,
                        "Invalid parameter type",
                        "To resolve this:\n1. Provide a valid parameter type\n2. Check type syntax\n3. Ensure valid type or identifier",
                        span
                    );
                    return dataTypes;
                }
                dataTypes.insert(dataTypes.end(), paramType.begin(), paramType.end());

                if (currentToken.getType() == TokenTypes::Comma) {
                    dataTypes.push_back(",");
                    prevColumn = currentToken.getColumn();
                    eat(TokenTypes::Comma, [&]() {
                        std::string suggestion = Console::formatString(
                            "To resolve this:\n"
                            "1. Use ',' to separate parameters\n"
                            "2. Check function parameter syntax\n"
                            "3. Expected token: ',', found '%s'",
                            getTokenTypeName(currentToken.getType()).c_str()
                        );
                        console.reportError(
                            Console::SYNTAX_ERROR,
                            Console::formatString("Expected ',' between parameters, found '%s'", 
                                getTokenTypeName(currentToken.getType()).c_str()),
                            suggestion,
                            span
                        );
                    });
                } else if (currentToken.getType() != TokenTypes::RightParen) {
                    console.reportError(
                        Console::SYNTAX_ERROR,
                        Console::formatString("Expected ',' or ')' after parameter, found '%s'", 
                            getTokenTypeName(currentToken.getType()).c_str()),
                        "To resolve this:\n1. Separate parameters with ',' or close with ')'\n2. Check function parameter syntax\n3. Ensure valid termination",
                        span
                    );
                    return dataTypes;
                }
            }

            dataTypes.push_back(")");
            prevColumn = currentToken.getColumn();
            eat(TokenTypes::RightParen, [&]() {
                std::string suggestion = Console::formatString(
                    "To resolve this:\n"
                    "1. Close parameter list with ')'\n"
                    "2. Check function type syntax\n"
                    "3. Expected token: ')', found '%s'",
                    getTokenTypeName(currentToken.getType()).c_str()
                );
                console.reportError(
                    Console::SYNTAX_ERROR,
                    Console::formatString("Expected ')' to close function parameters, found '%s'", 
                        getTokenTypeName(currentToken.getType()).c_str()),
                    suggestion,
                    span
                );
            });

            if (currentToken.getType() == TokenTypes::Arrow || 
                (currentToken.getType() == TokenTypes::Assign && 
                 lexer.peekToken(1).getType() == TokenTypes::GreaterThan)) {
                
                if (currentToken.getType() == TokenTypes::Arrow) {
                    dataTypes.push_back("=>");
                    prevColumn = currentToken.getColumn();
                    eat(TokenTypes::Arrow, [&]() {
                        std::string suggestion = Console::formatString(
                            "To resolve this:\n"
                            "1. Use '=>' for return type\n"
                            "2. Check function type syntax\n"
                            "3. Expected token: '=>', found '%s'",
                            getTokenTypeName(currentToken.getType()).c_str()
                        );
                        console.reportError(
                            Console::SYNTAX_ERROR,
                            Console::formatString("Expected '=>' for return type, found '%s'", 
                                getTokenTypeName(currentToken.getType()).c_str()),
                            suggestion,
                            span
                        );
                    });
                } else {
                    eat(TokenTypes::Assign, [&]() {
                        std::string suggestion = Console::formatString(
                            "To resolve this:\n"
                            "1. Use '=' for return type arrow\n"
                            "2. Check function type syntax\n"
                            "3. Expected token: '=', found '%s'",
                            getTokenTypeName(currentToken.getType()).c_str()
                        );
                        console.reportError(
                            Console::SYNTAX_ERROR,
                            Console::formatString("Expected '=' for return type arrow, found '%s'", 
                                getTokenTypeName(currentToken.getType()).c_str()),
                            suggestion,
                            span
                        );
                    });
                    dataTypes.push_back("=>");
                    prevColumn = currentToken.getColumn();
                    eat(TokenTypes::GreaterThan, [&]() {
                        std::string suggestion = Console::formatString(
                            "To resolve this:\n"
                            "1. Use '>' to complete '=>' arrow\n"
                            "2. Check function type syntax\n"
                            "3. Expected token: '>', found '%s'",
                            getTokenTypeName(currentToken.getType()).c_str()
                        );
                        console.reportError(
                            Console::SYNTAX_ERROR,
                            Console::formatString("Expected '>' to complete '=>', found '%s'", 
                                getTokenTypeName(currentToken.getType()).c_str()),
                            suggestion,
                            span
                        );
                    });
                }

                std::vector<std::string> returnType = parseType();
                if (returnType.empty()) {
                    console.reportError(
                        Console::SYNTAX_ERROR,
                        "Invalid return type",
                        "To resolve this:\n1. Provide a valid return type\n2. Check type syntax\n3. Ensure valid type or identifier",
                        span
                    );
                    return dataTypes;
                }
                dataTypes.insert(dataTypes.end(), returnType.begin(), returnType.end());
            }
        }

        span.end.line = previousToken.getLine();
        span.end.col = previousToken.getColumn();
        span.end.filePath = previousToken.getFilePath();

        return dataTypes;
    }

    std::vector<std::string> prefixModifiers;
    while ((currentToken.getType() == TokenTypes::Multiply ||
            currentToken.getType() == TokenTypes::BitwiseAnd ||
            currentToken.getType() == TokenTypes::QuestionMark) &&
           (dataTypes.empty() || !hasWhitespace(currentToken))) {
        
        prefixModifiers.push_back(currentToken.getValue());
        prevColumn = currentToken.getColumn();
        eat(currentToken.getType(), [&]() {
            std::string suggestion = Console::formatString(
                "To resolve this:\n"
                "1. Use valid type modifier (*, &, ?)\n"
                "2. Check type syntax\n"
                "3. Expected token: modifier, found '%s'",
                getTokenTypeName(currentToken.getType()).c_str()
            );
            console.reportError(
                Console::SYNTAX_ERROR,
                Console::formatString("Expected type modifier (*, &, ?), found '%s'", 
                    getTokenTypeName(currentToken.getType()).c_str()),
                suggestion,
                span
            );
        });
    }

    if (currentToken.getType() == TokenTypes::Identifier) {
        if (!prefixModifiers.empty() && !hasWhitespace(currentToken)) {
            dataTypes.insert(dataTypes.end(), prefixModifiers.begin(), prefixModifiers.end());
        }
        prefixModifiers.clear();

        dataTypes.push_back(currentToken.getValue());
        prevColumn = currentToken.getColumn();
        eat(TokenTypes::Identifier, [&]() {
            std::string suggestion = Console::formatString(
                "To resolve this:\n"
                "1. Provide a valid type identifier\n"
                "2. Check type syntax\n"
                "3. Expected token: identifier, found '%s'",
                getTokenTypeName(currentToken.getType()).c_str()
            );
            console.reportError(
                Console::SYNTAX_ERROR,
                Console::formatString("Expected identifier for type, found '%s'", 
                    getTokenTypeName(currentToken.getType()).c_str()),
                suggestion,
                span
            );
        });
        
        while (currentToken.getType() == TokenTypes::Dot) {
            eat(TokenTypes::Dot, [&]() {
                std::string suggestion = Console::formatString(
                    "To resolve this:\n"
                    "1. Use '.' for qualified type names\n"
                    "2. Check type syntax\n"
                    "3. Expected token: '.', found '%s'",
                    getTokenTypeName(currentToken.getType()).c_str()
                );
                console.reportError(
                    Console::SYNTAX_ERROR,
                    Console::formatString("Expected '.' for qualified type, found '%s'", 
                        getTokenTypeName(currentToken.getType()).c_str()),
                    suggestion,
                    span
                );
            });
            dataTypes.push_back(".");
            dataTypes.push_back(currentToken.getValue());
            prevColumn = currentToken.getColumn();
            eat(TokenTypes::Identifier, [&]() {
                std::string suggestion = Console::formatString(
                    "To resolve this:\n"
                    "1. Provide a valid identifier after '.'\n"
                    "2. Check qualified type syntax\n"
                    "3. Expected token: identifier, found '%s'",
                    getTokenTypeName(currentToken.getType()).c_str()
                );
                console.reportError(
                    Console::SYNTAX_ERROR,
                    Console::formatString("Expected identifier after '.', found '%s'", 
                        getTokenTypeName(currentToken.getType()).c_str()),
                    suggestion,
                    span
                );
            });
        }
    } else if (!prefixModifiers.empty()) {
        return prefixModifiers;
    } else {
        console.reportError(
            Console::SYNTAX_ERROR,
            Console::formatString("Expected identifier for type, found '%s'", 
                getTokenTypeName(currentToken.getType()).c_str()),
            "To resolve this:\n1. Provide a valid type identifier\n2. Check type syntax\n3. Ensure valid type declaration",
            span
        );
        return dataTypes;
    }

    while (currentToken.getType() == TokenTypes::Multiply ||
            currentToken.getType() == TokenTypes::BitwiseAnd ||
            currentToken.getType() == TokenTypes::QuestionMark) {
        
        dataTypes.push_back(currentToken.getValue());
        prevColumn = currentToken.getColumn();
        eat(currentToken.getType(), [&]() {
            std::string suggestion = Console::formatString(
                "To resolve this:\n"
                "1. Use valid type modifier (*, &, ?)\n"
                "2. Check type syntax\n"
                "3. Expected token: modifier, found '%s'",
                getTokenTypeName(currentToken.getType()).c_str()
            );
            console.reportError(
                Console::SYNTAX_ERROR,
                Console::formatString("Expected type modifier (*, &, ?), found '%s'", 
                    getTokenTypeName(currentToken.getType()).c_str()),
                suggestion,
                span
            );
        });
    }

    while (currentToken.getType() == TokenTypes::LeftBracket) {
        dataTypes.push_back("[");
        prevColumn = currentToken.getColumn();
        eat(TokenTypes::LeftBracket, [&]() {
            std::string suggestion = Console::formatString(
                "To resolve this:\n"
                "1. Use '[' for array type\n"
                "2. Check array type syntax\n"
                "3. Expected token: '[', found '%s'",
                getTokenTypeName(currentToken.getType()).c_str()
            );
            console.reportError(
                Console::SYNTAX_ERROR,
                Console::formatString("Expected '[' for array type, found '%s'", 
                    getTokenTypeName(currentToken.getType()).c_str()),
                suggestion,
                span
            );
        });

        if (currentToken.getType() == TokenTypes::IntegerLiteral ||
            currentToken.getType() == TokenTypes::Identifier) {
            dataTypes.push_back(currentToken.getValue());

            if (currentToken.getType() == TokenTypes::IntegerLiteral) {
                try {
                    uint64_t size = std::stoull(currentToken.getValue());
                    if (size == 0) {
                        console.reportError(
                            Console::SYNTAX_ERROR,
                            "Array size must be greater than 0",
                            "To resolve this:\n1. Provide a positive integer for array size\n2. Check array syntax\n3. Ensure valid size",
                            span
                        );
                    }
                } catch (const std::exception& e) {
                    console.reportError(
                        Console::SYNTAX_ERROR,
                        "Invalid array size: " + std::string(e.what()),
                        "To resolve this:\n1. Provide a valid integer for array size\n2. Check array syntax\n3. Ensure numeric value",
                        span
                    );
                }
            }

            prevColumn = currentToken.getColumn();
            eat(currentToken.getType(), [&]() {
                std::string suggestion = Console::formatString(
                    "To resolve this:\n"
                    "1. Provide an integer or constant identifier inside array brackets\n"
                    "2. Check array type syntax\n"
                    "3. Expected token: integer or identifier, found '%s'",
                    getTokenTypeName(currentToken.getType()).c_str()
                );
                console.reportError(
                    Console::SYNTAX_ERROR,
                    Console::formatString("Expected integer or identifier inside array brackets '[size]', found '%s'", 
                        getTokenTypeName(currentToken.getType()).c_str()),
                    suggestion,
                    span
                );
            });
        } else {
            console.reportError(
                Console::SYNTAX_ERROR,
                Console::formatString("Expected integer or identifier inside array brackets '[size]', found '%s'", 
                    getTokenTypeName(currentToken.getType()).c_str()),
                "To resolve this:\n1. Provide an integer or constant identifier\n2. Check array type syntax\n3. Ensure valid size or identifier",
                span
            );
        }

        dataTypes.push_back("]");
        prevColumn = currentToken.getColumn();
        eat(TokenTypes::RightBracket, [&]() {
            std::string suggestion = Console::formatString(
                "To resolve this:\n"
                "1. Close array type with ']'\n"
                "2. Check array type syntax\n"
                "3. Expected token: ']', found '%s'",
                getTokenTypeName(currentToken.getType()).c_str()
            );
            console.reportError(
                Console::SYNTAX_ERROR,
                Console::formatString("Expected ']' to close array type, found '%s'", 
                    getTokenTypeName(currentToken.getType()).c_str()),
                suggestion,
                span
            );
        });
    }

    span.end.line = previousToken.getLine();
    span.end.col = previousToken.getColumn();
    span.end.filePath = previousToken.getFilePath();

    return dataTypes;
}

std::vector<std::string> Parser::parseTypeParametersForCall() {
    FileSpan span;
    span.start.line = currentToken.getLine();
    span.start.col = currentToken.getColumn();
    span.start.filePath = currentToken.getFilePath();

    std::vector<std::string> typeParams;

    if (currentToken.getType() == TokenTypes::LessThan) {
        eat(TokenTypes::LessThan, [&]() {
            std::string suggestion = Console::formatString(
                "To resolve this:\n"
                "1. Start type parameters with '<'\n"
                "2. Check generic call syntax\n"
                "3. Expected token: '<', found '%s'",
                getTokenTypeName(currentToken.getType()).c_str()
            );
            console.reportError(
                Console::SYNTAX_ERROR,
                Console::formatString("Expected '<' for type parameters, found '%s'", 
                    getTokenTypeName(currentToken.getType()).c_str()),
                suggestion,
                span
            );
        });

        while (currentToken.getType() == TokenTypes::Identifier) {
            typeParams.push_back(currentToken.getValue());
            eat(TokenTypes::Identifier, [&]() {
                std::string suggestion = Console::formatString(
                    "To resolve this:\n"
                    "1. Provide a valid type parameter name\n"
                    "2. Check generic call syntax\n"
                    "3. Expected token: identifier, found '%s'",
                    getTokenTypeName(currentToken.getType()).c_str()
                );
                console.reportError(
                    Console::SYNTAX_ERROR,
                    Console::formatString("Expected identifier for type parameter, found '%s'", 
                        getTokenTypeName(currentToken.getType()).c_str()),
                    suggestion,
                    span
                );
            });

            if (currentToken.getType() == TokenTypes::Comma) {
                eat(TokenTypes::Comma, [&]() {
                    std::string suggestion = Console::formatString(
                        "To resolve this:\n"
                        "1. Use ',' to separate type parameters\n"
                        "2. Check generic call syntax\n"
                        "3. Expected token: ',', found '%s'",
                        getTokenTypeName(currentToken.getType()).c_str()
                    );
                    console.reportError(
                        Console::SYNTAX_ERROR,
                        Console::formatString("Expected ',' between type parameters, found '%s'", 
                            getTokenTypeName(currentToken.getType()).c_str()),
                        suggestion,
                        span
                    );
                });
            } else {
                break;
            }
        }

        eat(TokenTypes::GreaterThan, [&]() {
            std::string suggestion = Console::formatString(
                "To resolve this:\n"
                "1. Close type parameters with '>'\n"
                "2. Check generic call syntax\n"
                "3. Expected token: '>', found '%s'",
                getTokenTypeName(currentToken.getType()).c_str()
            );
            console.reportError(
                Console::SYNTAX_ERROR,
                Console::formatString("Expected '>' to close type parameters, found '%s'", 
                    getTokenTypeName(currentToken.getType()).c_str()),
                suggestion,
                span
            );
        });
    }

    span.end.line = previousToken.getLine();
    span.end.col = previousToken.getColumn();
    span.end.filePath = previousToken.getFilePath();

    return typeParams;
}

std::shared_ptr<Statement> Parser::parseTypeDeclaration() {
    FileSpan span;
    span.start.line = currentToken.getLine();
    span.start.col = currentToken.getColumn();
    span.start.filePath = currentToken.getFilePath();

    Token startToken = currentToken;
    eat(TokenTypes::Type, [&]() {
        std::string suggestion = Console::formatString(
            "To resolve this:\n"
            "1. Start type declaration with 'type'\n"
            "2. Check type declaration syntax\n"
            "3. Expected token: 'type', found '%s'",
            getTokenTypeName(currentToken.getType()).c_str()
        );
        console.reportError(
            Console::SYNTAX_ERROR,
            Console::formatString("Expected 'type' keyword, found '%s'", 
                getTokenTypeName(currentToken.getType()).c_str()),
            suggestion,
            span
        );
    });

    std::string typeName = currentToken.getValue();
    eat(TokenTypes::Identifier, [&]() {
        std::string suggestion = Console::formatString(
            "To resolve this:\n"
            "1. Provide a valid type name\n"
            "2. Check type declaration syntax\n"
            "3. Expected token: identifier, found '%s'",
            getTokenTypeName(currentToken.getType()).c_str()
        );
        console.reportError(
            Console::SYNTAX_ERROR,
            Console::formatString("Expected identifier for type name, found '%s'", 
                getTokenTypeName(currentToken.getType()).c_str()),
            suggestion,
            span
        );
    });

    std::shared_ptr<Type> type = nullptr;
    if (currentToken.getType() != TokenTypes::Semicolon) {
        eat(TokenTypes::Assign, [&]() {
            std::string suggestion = Console::formatString(
                "To resolve this:\n"
                "1. Use '=' for type assignment\n"
                "2. Check type declaration syntax\n"
                "3. Expected token: '=', found '%s'",
                getTokenTypeName(currentToken.getType()).c_str()
            );
            console.reportError(
                Console::SYNTAX_ERROR,
                Console::formatString("Expected '=' for type assignment, found '%s'", 
                    getTokenTypeName(currentToken.getType()).c_str()),
                suggestion,
                span
            );
        });

        std::vector<std::string> typeString = parseType();
        if (typeString.empty()) {
            console.reportError(
                Console::SYNTAX_ERROR,
                "Invalid type definition",
                "To resolve this:\n1. Provide a valid type definition\n2. Check type syntax\n3. Ensure valid type or identifier",
                span
            );
            return nullptr;
        }
        type = resolveType(typeString);
        if (!type) {
            console.reportError(
                Console::SYNTAX_ERROR,
                "Failed to resolve type: " + join(typeString, "."),
                "To resolve this:\n1. Ensure type is defined\n2. Check type resolution\n3. Verify type string",
                span
            );
            return nullptr;
        }
    }

    if (currentToken.getType() == TokenTypes::Semicolon) {
        eat(TokenTypes::Semicolon, [&]() {
            std::string suggestion = Console::formatString(
                "To resolve this:\n"
                "1. End type declaration with ';'\n"
                "2. Check type declaration syntax\n"
                "3. Expected token: ';', found '%s'",
                getTokenTypeName(currentToken.getType()).c_str()
            );
            console.reportError(
                Console::SYNTAX_ERROR,
                Console::formatString("Expected ';' to end type declaration, found '%s'", 
                    getTokenTypeName(currentToken.getType()).c_str()),
                suggestion,
                span
            );
        });
    } else {
        console.reportError(
            Console::SYNTAX_ERROR,
            Console::formatString("Expected ';' to end type declaration, found '%s'", 
                getTokenTypeName(currentToken.getType()).c_str()),
            "To resolve this:\n1. End type declaration with ';'\n2. Check type declaration syntax\n3. Ensure proper termination",
            span
        );
    }

    span.end.line = previousToken.getLine();
    span.end.col = previousToken.getColumn();
    span.end.filePath = previousToken.getFilePath();

    auto typeDecl = std::make_shared<TypeDeclaration>(typeName, type);
    typeDecl->setPosition(startToken, previousToken);
    typeDecl->setSpan(span);
    return typeDecl;
}

std::shared_ptr<Statement> Parser::parseUsingAlias() {
    FileSpan span;
    span.start.line = currentToken.getLine();
    span.start.col = currentToken.getColumn();
    span.start.filePath = currentToken.getFilePath();

    Token startToken = currentToken;
    eat(TokenTypes::Using, [&]() {
        std::string suggestion = Console::formatString(
            "To resolve this:\n"
            "1. Start alias declaration with 'using'\n"
            "2. Check using alias syntax\n"
            "3. Expected token: 'using', found '%s'",
            getTokenTypeName(currentToken.getType()).c_str()
        );
        console.reportError(
            Console::SYNTAX_ERROR,
            Console::formatString("Expected 'using' keyword, found '%s'", 
                getTokenTypeName(currentToken.getType()).c_str()),
            suggestion,
            span
        );
    });

    std::string aliasName = currentToken.getValue();
    eat(TokenTypes::Identifier, [&]() {
        std::string suggestion = Console::formatString(
            "To resolve this:\n"
            "1. Provide a valid alias name\n"
            "2. Check using alias syntax\n"
            "3. Expected token: identifier, found '%s'",
            getTokenTypeName(currentToken.getType()).c_str()
        );
        console.reportError(
            Console::SYNTAX_ERROR,
            Console::formatString("Expected identifier for alias name, found '%s'", 
                getTokenTypeName(currentToken.getType()).c_str()),
            suggestion,
            span
        );
    });

    eat(TokenTypes::Assign, [&]() {
        std::string suggestion = Console::formatString(
            "To resolve this:\n"
            "1. Use '=' for alias assignment\n"
            "2. Check using alias syntax\n"
            "3. Expected token: '=', found '%s'",
            getTokenTypeName(currentToken.getType()).c_str()
        );
        console.reportError(
            Console::SYNTAX_ERROR,
            Console::formatString("Expected '=' for alias assignment, found '%s'", 
                getTokenTypeName(currentToken.getType()).c_str()),
            suggestion,
            span
        );
    });

    std::vector<std::string> typePath = parseType();
    if (typePath.empty()) {
        console.reportError(
            Console::SYNTAX_ERROR,
            "Invalid type path for alias",
            "To resolve this:\n1. Provide a valid type path\n2. Check type syntax\n3. Ensure valid type or identifier",
            span
        );
        return nullptr;
    }

    eat(TokenTypes::Semicolon, [&]() {
        std::string suggestion = Console::formatString(
            "To resolve this:\n"
            "1. End alias declaration with ';'\n"
            "2. Check using alias syntax\n"
            "3. Expected token: ';', found '%s'",
            getTokenTypeName(currentToken.getType()).c_str()
        );
        console.reportError(
            Console::SYNTAX_ERROR,
            Console::formatString("Expected ';' to end alias declaration, found '%s'", 
                getTokenTypeName(currentToken.getType()).c_str()),
            suggestion,
            span
        );
    });

    span.end.line = previousToken.getLine();
    span.end.col = previousToken.getColumn();
    span.end.filePath = previousToken.getFilePath();

    // auto usingAlias = std::make_shared<TypeDeclaration>(aliasName, typePath);
    // usingAlias->setPosition(startToken, previousToken);
    // usingAlias->setSpan(span);

    // DEBUG_LOG("Parsed using alias: " + aliasName + " = " + join(typePath, "."));

    // return usingAlias;
    return nullptr;
}

} // namespace Omniscript
