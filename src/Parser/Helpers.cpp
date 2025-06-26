<<<<<<< HEAD:src/Parser/Helpers.cpp
#include <omniscript/Statement.h>
#include <omniscript/Statements/CallableStatement.h>
#include <omniscript/Statements/LiteralStatements.h>

#include <omniscript/Core.h>
#include <omniscript/utils.h>
#include <omniscript/Lexer.h>
#include <omniscript/Parser.h>
#include <omniscript/Tokens.h>
#include <omniscript/runtime/object.h>
#include <omniscript/omniscript_pch.h>
#include <omniscript/Statement.h>
#include <omniscript/Symboltable.h>
=======
#include <omniscript/Statement.h>
#include <omniscript/Statements/CallableStatement.h>
#include <omniscript/Statements/LiteralStatements.h>

#include <omniscript/Core.h>
#include <omniscript/utils.h>
#include <omniscript/Lexer.h>
#include <omniscript/Parser.h>
#include <omniscript/Tokens.h>
#include <omniscript/runtime/object.h>
#include <omniscript/omniscript_pch.h>
#include <omniscript/Statement.h>
#include <omniscript/Symboltable.h>
>>>>>>> 7ccebff50dd27e70cffd4d578dcb358f4c9e1613:src/engine/Parser/Helpers.cpp


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

std::vector<std::shared_ptr<Statement>> Parser::parseParameters() {
    eat(TokenTypes::LeftParen);
    
    std::vector<std::shared_ptr<Statement>> parameters;
    
    while (currentToken.getType() != TokenTypes::RightParen && currentToken.getType() != TokenTypes::EOI) {
        Token startToken = currentToken;
        std::string paramName;
        bool isVariadic = false;
        std::shared_ptr<Omniscript::Type> paramType;
        std::shared_ptr<Statement> defaultValue = nullptr;

        if (currentToken.getType() == TokenTypes::Ellipsis) {
            isVariadic = true;
            eat(TokenTypes::Ellipsis);
        }

        if (currentToken.getType() == TokenTypes::Identifier) {
            paramName = currentToken.getValue();
            eat(TokenTypes::Identifier);
        } else {
            eat(TokenTypes::Identifier, "Expected a parameter name.");
        }

        if (currentToken.getType() == TokenTypes::Colon) {
            eat(TokenTypes::Colon);
            
            std::vector<std::string> types = parseType();
            paramType = Omniscript::resolveType(types);
        }

        if (currentToken.getType() == TokenTypes::Assign) {
            eat(TokenTypes::Assign);
            defaultValue = parseExpression();
        } else {
            defaultValue = std::make_shared<Invalid>();
        }


        auto parameter = std::make_shared<ParameterStatement>(paramName, defaultValue);
        parameter->isVariadic = isVariadic;
        parameter->setType(paramType);
        parameters.push_back(parameter);
        parameter->setPosition(startToken);

        if (currentToken.getType() == TokenTypes::Comma) {
            eat(TokenTypes::Comma);
        }
    }

    eat(TokenTypes::RightParen);

    return parameters;
}

std::string Parser::generateSpecializedNameForCall(
    const std::string &baseName, 
    const std::vector<std::string> &typeParams
) {
    std::ostringstream oss;
    oss << baseName;

    if (!typeParams.empty()) {
        oss << "_";

        for (size_t i = 0; i < typeParams.size(); ++i) {
            const auto& type = typeParams[i];

            oss << type;

            if (i < typeParams.size() - 1) {
                oss << "_";
            }
        }
    }

    return oss.str();
}

std::vector<std::shared_ptr<Statement>> Parser::parseArguments(TokenTypes start, TokenTypes end, TokenTypes assignOp) {
    DEBUG_LOG("Parsing the arguments");
    std::vector<std::shared_ptr<Statement>> args;
    eat(start);

    int argCount = 0;

    while (currentToken.getType() != end && currentToken.getType() != TokenTypes::EOI) {
        Token startToken = currentToken;
        if (currentToken.getType() == TokenTypes::Identifier) {
            std::string paramName;
            if (lexer.peekToken(1).getType() == assignOp) {
                paramName = currentToken.getValue();
                eat(TokenTypes::Identifier);
                eat(assignOp);
                args.push_back(parseExpression());
            } else {
                args.push_back(parseExpression());
            }
        } else {
            args.push_back(parseExpression());
        }

        if (currentToken.getType() == TokenTypes::Comma) {
            eat(TokenTypes::Comma);
            if (currentToken.getType() == end) {
                console.error("Unexpected comma before closing parenthesis.");
                throw std::runtime_error("Syntax error: Trailing comma in argument list.");
            }
        } else {
            break;
        }
        args[argCount]->setPosition(startToken);
        argCount++;
    }

    eat(end, "Expected '"+ getTokenTypeName(end) + "' but found '" + getTokenTypeName(currentToken.getType()) + "' at end of argument list.");

    DEBUG_LOG("Done parsing the arguments");
    return args;
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

    // Parse prefix modifiers (only if no whitespace before identifier)
    std::vector<std::string> prefixModifiers;
    while ((currentToken.getType() == TokenTypes::Multiply ||
            currentToken.getType() == TokenTypes::BitwiseAnd ||
            currentToken.getType() == TokenTypes::QuestionMark) &&
           (dataTypes.empty() || !hasWhitespace(currentToken))) {
        
        if (currentToken.getType() == TokenTypes::Multiply) {
            dataTypes.push_back("*");
        } else if (currentToken.getType() == TokenTypes::BitwiseAnd) {
            dataTypes.push_back("&");   
        } else if (currentToken.getType() == TokenTypes::QuestionMark) {
            dataTypes.push_back("?");
        }

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
        
        if (currentToken.getType() == TokenTypes::Multiply) {
            dataTypes.push_back("*");
        } else if (currentToken.getType() == TokenTypes::BitwiseAnd) {
            dataTypes.push_back("&");   
        } else if (currentToken.getType() == TokenTypes::QuestionMark) {
            dataTypes.push_back("?");
        }

        prevColumn = currentToken.getColumn();
        eat(currentToken.getType());
    }

    // Parse array brackets
    if (currentToken.getType() == TokenTypes::LeftBracket) {
        dataTypes.push_back("[");
        prevColumn = currentToken.getColumn();
        eat(TokenTypes::LeftBracket);
        
        if (currentToken.getType() == TokenTypes::IntegerLiteral) {
            dataTypes.push_back(currentToken.getValue());
            prevColumn = currentToken.getColumn();
            eat(TokenTypes::IntegerLiteral);
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

MemberModifiers Parser::parseMemberModifiers() {
    MemberModifiers modifiers;

    while (currentToken.getType() == TokenTypes::Private || 
           currentToken.getType() == TokenTypes::Public || 
           currentToken.getType() == TokenTypes::Override ||
           currentToken.getType() == TokenTypes::Static ||
           currentToken.getType() == TokenTypes::Final ||
           currentToken.getType() == TokenTypes::Virtual
        //    ||
        //    (
        //     currentToken.getType() == TokenTypes::Const
        //     && (lexer.peekToken(1).getType() != TokenTypes::Identifier && 
        //         (lexer.peekToken(2).getType() != TokenTypes::Colon || 
        //         lexer.peekToken(2).getType() != TokenTypes::Equals)
        //     )
        //     ) 
        ) {

        modifiers.isInitialized = true;

        if (currentToken.getType() == TokenTypes::Private) {
            modifiers.access = MemberModifiers::AccessModifier::Private;
            eat(TokenTypes::Private);
        }

        if (currentToken.getType() == TokenTypes::Public) {
            modifiers.access = MemberModifiers::AccessModifier::Public;
            eat(TokenTypes::Public);
        }

        if (currentToken.getType() == TokenTypes::Protected) {
            modifiers.access = MemberModifiers::AccessModifier::Protected;
            eat(TokenTypes::Protected);
        }

        if (currentToken.getType() == TokenTypes::Override) {
            modifiers.shouldOverride = true;
            eat(TokenTypes::Override);
        }

        if (currentToken.getType() == TokenTypes::Static) {
            modifiers.isStatic = true;
            eat(TokenTypes::Static);
        }

        if (currentToken.getType() == TokenTypes::Final) {
            modifiers.isFinal = true;
            eat(TokenTypes::Final);
        }

        if (currentToken.getType() == TokenTypes::Virtual) {
            modifiers.isVirtual = true;
            eat(TokenTypes::Virtual);
        }

        // if (currentToken.getType() == TokenTypes::Const) {
        //     modifiers.isConst = true;
        //     eat(TokenTypes::Const);
        // }
    }

    return modifiers;
}
