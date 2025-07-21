#include <omniscript/Statement.h>
#include <omniscript/Statements/CallableStatement.h>
#include <omniscript/Statements/FunctionStatement.h>
#include <omniscript/Statements/LiteralStatements.h>
#include <omniscript/Statements/TypeStatements.h>

#include <omniscript/Core.h>
#include <omniscript/utils.h>
#include <omniscript/Lexer.h>
#include <omniscript/Parser.h>
#include <omniscript/Tokens.h>
#include <omniscript/Statement.h>
#include <omniscript/Symboltable.h>
#include <omniscript/omniscript_pch.h>


bool Parser::tryParseTypeParametersLookahead(int& i) {
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
    int i = 0;

    // Check for <...> generic parameters
    if ((i == 0? currentToken.getType() : lexer.peekToken(i).getType()) != TokenTypes::LessThan)
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

    // Next must be a LeftParen — function call or constructor
    return lexer.peekToken(i).getType() == TokenTypes::LeftParen;
}


parameterType Parser::parseTypeParametersForDeclaration() {
    parameterType typeParams;

    if (currentToken.getType() == TokenTypes::LessThan) { // `<T>`
        eat(TokenTypes::LessThan);

        while (currentToken.getType() == TokenTypes::Identifier) {
            std::string typeName = currentToken.getValue();
            eat(TokenTypes::Identifier);

            std::vector<std::vector<std::string>> constraintList;

            if (currentToken.getType() == TokenTypes::Extends) {
                eat(TokenTypes::Extends);

                while (true) {
                    if (currentToken.getType() == TokenTypes::Variant) {
                        constraintList.push_back({ "variant" });
                        eat(currentToken.getType());
                    } else if (currentToken.getType() == TokenTypes::Any) {
                        constraintList.push_back({ "any" });
                        eat(currentToken.getType());
                    } else {
                        std::vector<std::string> parsedType = parseType();

                        // Instead of pushing the entire union as one element,
                        // push individual types as separate constraints
                        constraintList.push_back(parsedType);
                    }

                    if (currentToken.getType() == TokenTypes::BitwiseOr) {
                        eat(TokenTypes::BitwiseOr);
                    } else {
                        break;
                    }
                }
            }

            // For debug log
            std::string constraintStr = constraintList.empty() ? "none" :
                std::accumulate(std::next(constraintList.begin()), constraintList.end(),
                    join(constraintList[0], "."),
                    [](const std::string& acc, const std::vector<std::string>& typeVec) {
                    return acc + " , " + join(typeVec, ".");
                });

            DEBUG_LOG("TypeName: " + typeName + ", Constraint: [" + constraintStr + "]");

            typeParams.emplace_back(typeName, constraintList);

            if (currentToken.getType() == TokenTypes::Comma) {
                eat(TokenTypes::Comma);
            } else {
                break;
            }
        }

        eat(TokenTypes::GreaterThan); // `>`
    }

    return typeParams;
}

std::vector<std::string> Parser::parseType() {
    std::vector<std::string> dataTypes;
    int prevColumn = -1;

    // Helper function to check for whitespace between tokens
    auto hasWhitespace = [&](Token& token) {
        return prevColumn != -1 && token.getColumn() > prevColumn + 1;
    };

    // Check for function type syntax: fn(params) => returnType
    if (currentToken.getType() == TokenTypes::Function) {
        dataTypes.push_back("fn");
        prevColumn = currentToken.getColumn();
        eat(TokenTypes::Function);

        // Parse parameter list
        if (currentToken.getType() == TokenTypes::LeftParen) {
            dataTypes.push_back("(");
            prevColumn = currentToken.getColumn();
            eat(TokenTypes::LeftParen);

            // Parse parameters
            while (currentToken.getType() != TokenTypes::RightParen) {
                // Parse parameter name (optional)
                if (currentToken.getType() == TokenTypes::Identifier) {
                    // Check if next token is colon (parameter name)
                    Token nextToken = lexer.peekToken(1);
                    if (nextToken.getType() == TokenTypes::Colon) {
                        dataTypes.push_back(currentToken.getValue());
                        prevColumn = currentToken.getColumn();
                        eat(TokenTypes::Identifier);
                        
                        dataTypes.push_back(":");
                        prevColumn = currentToken.getColumn();
                        eat(TokenTypes::Colon);
                    }
                }

                // Parse parameter type (recursive call)
                std::vector<std::string> paramType = parseType();
                dataTypes.insert(dataTypes.end(), paramType.begin(), paramType.end());

                // Handle comma-separated parameters
                if (currentToken.getType() == TokenTypes::Comma) {
                    dataTypes.push_back(",");
                    prevColumn = currentToken.getColumn();
                    eat(TokenTypes::Comma);
                } else if (currentToken.getType() != TokenTypes::RightParen) {
                    // Error: expected comma or closing paren
                    break;
                }
            }

            dataTypes.push_back(")");
            prevColumn = currentToken.getColumn();
            eat(TokenTypes::RightParen);

            // Parse arrow operator =>
            if (currentToken.getType() == TokenTypes::Arrow || 
                (currentToken.getType() == TokenTypes::Assign && 
                 lexer.peekToken(1).getType() == TokenTypes::GreaterThan)) {
                
                if (currentToken.getType() == TokenTypes::Arrow) {
                    dataTypes.push_back("=>");
                    prevColumn = currentToken.getColumn();
                    eat(TokenTypes::Arrow);
                } else {
                    // Handle "=" followed by ">"
                    eat(TokenTypes::Assign);
                    dataTypes.push_back("=>");
                    prevColumn = currentToken.getColumn();
                    eat(TokenTypes::GreaterThan);
                }

                // Parse return type (recursive call)
                std::vector<std::string> returnType = parseType();
                dataTypes.insert(dataTypes.end(), returnType.begin(), returnType.end());
            }
        }

        return dataTypes;
    }

    // Parse prefix modifiers (only if no whitespace before identifier)
    std::vector<std::string> prefixModifiers;
    while ((currentToken.getType() == TokenTypes::Multiply ||
            currentToken.getType() == TokenTypes::BitwiseAnd ||
            currentToken.getType() == TokenTypes::QuestionMark) &&
           (dataTypes.empty() || !hasWhitespace(currentToken))) {
        
        dataTypes.push_back(currentToken.getValue());
        prevColumn = currentToken.getColumn();
        eat(currentToken.getType());
    }

    // Parse the main identifier (required)
    if (currentToken.getType() == TokenTypes::Identifier) {
        // Only keep prefix modifiers if they're adjacent to the identifier
        if (!prefixModifiers.empty() && !hasWhitespace(currentToken)) {
            dataTypes.insert(dataTypes.end(), prefixModifiers.begin(), prefixModifiers.end());
        }
        prefixModifiers.clear();

        dataTypes.push_back(currentToken.getValue());
        prevColumn = currentToken.getColumn();
        eat(TokenTypes::Identifier);
        
        // Handle dotted identifiers
        while (currentToken.getType() == TokenTypes::Dot) {
            eat(TokenTypes::Dot);
            dataTypes.push_back(".");
            dataTypes.push_back(currentToken.getValue());
            prevColumn = currentToken.getColumn();
            eat(TokenTypes::Identifier);
        }
    } else if (!prefixModifiers.empty()) {
        // We had modifiers but no identifier - treat as separate tokens
        return prefixModifiers;
    }

    // Parse suffix modifiers (only if no whitespace)
    while ((currentToken.getType() == TokenTypes::Multiply ||
            currentToken.getType() == TokenTypes::BitwiseAnd ||
            currentToken.getType() == TokenTypes::QuestionMark) &&
           !hasWhitespace(currentToken)) {
        
        dataTypes.push_back(currentToken.getValue());

        prevColumn = currentToken.getColumn();
        eat(currentToken.getType());
    }

    // Parse array brackets
    while (currentToken.getType() == TokenTypes::LeftBracket) {
        dataTypes.push_back("[");
        prevColumn = currentToken.getColumn();
        eat(TokenTypes::LeftBracket);

        if (currentToken.getType() == TokenTypes::IntegerLiteral ||
            currentToken.getType() == TokenTypes::Identifier) {

            dataTypes.push_back(currentToken.getValue());

            if (currentToken.getType() == TokenTypes::IntegerLiteral) {
                uint64_t size = std::stoull(currentToken.getValue());
                if (size == 0) {
                    console.error("Array size must be greater than 0");
                }
            }

            prevColumn = currentToken.getColumn();
            eat(currentToken.getType());

        } else {
            console.error("Expected an integer or constant identifier inside array brackets '[size]'");
        }

        dataTypes.push_back("]");
        prevColumn = currentToken.getColumn();
        eat(TokenTypes::RightBracket);
    }

    return dataTypes;
}

std::vector<std::string> Parser::parseTypeParametersForCall() {
    std::vector<std::string> typeParams;

    if (currentToken.getType() == TokenTypes::LessThan) { // `<T>`
        eat(TokenTypes::LessThan);

        while (currentToken.getType() == TokenTypes::Identifier) {
            typeParams.push_back(currentToken.getValue());
            eat(TokenTypes::Identifier);

            if (currentToken.getType() == TokenTypes::Comma) {
                eat(TokenTypes::Comma);
            } else {
                break;
            }
        }

        eat(TokenTypes::GreaterThan); // `>`
    }

    return typeParams;
}

std::shared_ptr<Statement> Parser::parseTypeDeclaration() {
    Token startToken = currentToken;
    eat(TokenTypes::Type);
    std::string typeName = currentToken.getValue();
    eat(TokenTypes::Identifier);
    std::shared_ptr<Omniscript::Type> type = nullptr;

    if (currentToken.getType() == TokenTypes::Semicolon) {
        eat(currentToken.getType());
    } else {
        eat(TokenTypes::Assign);
        std::vector<std::string> typeString = parseType();
        type = Omniscript::resolveType(typeString);
    }

    auto typeDecl = std::make_shared<TypeDeclaration>(typeName, type);
    typeDecl->setPosition(startToken);
    return typeDecl;
}

std::shared_ptr<Statement> Parser::parseUsingAlias() {
    Token startToken = currentToken;
    eat(TokenTypes::Using);

    return nullptr;
}