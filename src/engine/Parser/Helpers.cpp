#include <omniscript/Core.h>
#include <omniscript/utils.h>
#include <omniscript/runtime/object.h>
#include <omniscript/engine/parser.h>
#include <omniscript/engine/lexer.h>
#include <omniscript/engine/tokens.h>
#include <omniscript/engine/Statement.h>
#include <omniscript/engine/Symboltable.h>
#include <omniscript/omniscript_pch.h>


bool Parser::tryParseTypeParametersLookahead(int& i) {
    if ((i == 0 ? currentToken.getType() : lexer.peekToken(i).getType()) != TokenTypes::LessThan)
        return false;

    i++; // Skip '<'

    while (lexer.peekToken(i).getType() == TokenTypes::Identifier) {
        i++; // type name

        if (lexer.peekToken(i).getType() == TokenTypes::Extends) {
            i++; // skip 'extends'

            // Parse constraints
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

    i++; // Skip '<'

    // Parse comma-separated type names
    while (true) {
        if (lexer.peekToken(i).getType() != TokenTypes::Identifier)
            return false;

        i++; // Type name

        // Support dotted generic types (e.g., ns.Type)
        while (lexer.peekToken(i).getType() == TokenTypes::Dot) {
            i++;
            if (lexer.peekToken(i).getType() != TokenTypes::Identifier)
                return false;
            i++;
        }

        if (lexer.peekToken(i).getType() == TokenTypes::Comma) {
            i++; // More types coming
        } else {
            break;
        }
    }

    // Check for closing '>'
    if (lexer.peekToken(i).getType() != TokenTypes::GreaterThan)
        return false;

    i++; // Skip '>'

    // Next must be a LeftParen — function call or constructor
    return lexer.peekToken(i).getType() == TokenTypes::LeftParen;
}

std::vector<std::shared_ptr<Statement>> Parser::parseParameters() {
    eat(TokenTypes::LeftParen); // Start of parameters

    std::vector<std::shared_ptr<Statement>> parameters;

    while (currentToken.getType() != TokenTypes::RightParen && currentToken.getType() != TokenTypes::EOI) {
        std::string paramName;
        bool isVariadic = false;
        std::shared_ptr<Omniscript::Type> paramType;
        std::shared_ptr<Statement> defaultValue = nullptr;

        if (currentToken.getType() == TokenTypes::Ellipsis) {
            isVariadic = true;
            eat(TokenTypes::Ellipsis);
        }

        // Parse parameter name
        if (currentToken.getType() == TokenTypes::Identifier) {
            paramName = currentToken.getValue();
            eat(TokenTypes::Identifier);
        } else {
            eat(TokenTypes::Identifier, "Expected a parameter name.");
        }

        // Expect colon for type annotation
        if (currentToken.getType() == TokenTypes::Colon) {
            eat(TokenTypes::Colon);
            
            std::vector<std::string> types = parseType();
            paramType = Omniscript::resolveType(types);
        }

        // Check for default value
        if (currentToken.getType() == TokenTypes::Assign) {
            eat(TokenTypes::Assign);
            defaultValue = parseExpression(); // Parse the default value
        } else {
            defaultValue = std::make_shared<Invalid>();
        }

        // Store as a ParameterStatement
        auto parameter = std::make_shared<ParameterStatement>(paramName, defaultValue);
        parameter->isVariadic = isVariadic;
        parameter->setType(paramType);
        parameters.push_back(parameter);

        // Consume comma if present
        if (currentToken.getType() == TokenTypes::Comma) {
            eat(TokenTypes::Comma);
        }
    }

    eat(TokenTypes::RightParen); // End of parameters

    return parameters;
}

std::string Parser::generateSpecializedNameForCall(
    const std::string &baseName, 
    const std::vector<std::string> &typeParams
) {
    std::ostringstream oss;
    oss << baseName;

    if (!typeParams.empty()) {
        oss << "_"; // separator between baseName and type params

        for (size_t i = 0; i < typeParams.size(); ++i) {
            const auto& type = typeParams[i];

            // Just append the type names for calls, no special transformation needed
            oss << type;

            if (i < typeParams.size() - 1) {
                oss << "_";  // separate multiple types with underscores
            }
        }
    }

    return oss.str();
}

std::vector<std::shared_ptr<Statement>> Parser::parseArguments(TokenTypes start, TokenTypes end, TokenTypes assignOp) {
    DEBUG_LOG("Parsing the arguments");
    std::vector<std::shared_ptr<Statement>> args;
    eat(start);

    while (currentToken.getType() != end && currentToken.getType() != TokenTypes::EOI) {
        // Ensure we don't get stuck in an infinite loop
        if (currentToken.getType() == TokenTypes::Identifier) {
            std::string paramName;  // Argument name (e.g., "b")

            // Check if there's an assignment
            if (lexer.peekToken(1).getType() == assignOp) {
                paramName = currentToken.getValue();
                eat(TokenTypes::Identifier);  // Consume identifier
                eat(assignOp);  // Consume the assignment token
                args.push_back(parseExpression());  // Parse the value of the argument
            } else {
                // If no assignment, treat the current token as a regular argument
                args.push_back(parseExpression());  // Parse the argument
            }
        } else {
            args.push_back(parseExpression());  // Handle positional arguments (no names)
        }

        // Ensure comma consumption is correct
        if (currentToken.getType() == TokenTypes::Comma) {
            eat(TokenTypes::Comma);
            if (currentToken.getType() == end) {
                console.error("Unexpected comma before closing parenthesis.");
                throw std::runtime_error("Syntax error: Trailing comma in argument list.");
            }
        } else {
            break;  // End of arguments
        }
    }

    // Ensure we actually close the argument list
    eat(end, "Expected ' "+ getTokenTypeName(end) + " ' but found '" + getTokenTypeName(currentToken.getType()) + "' at end of argument list.");

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

                // Parse multiple types separated by '|'
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

     // First, check for any pointer/reference symbols before the base type
     while (
            currentToken.getType() == TokenTypes::Multiply ||
            currentToken.getType() == TokenTypes::BitwiseAnd ||
            currentToken.getType() == TokenTypes::QuestionMark) {
        if (currentToken.getType() == TokenTypes::Multiply) {
            dataTypes.push_back("*");
        } else if (currentToken.getType() == TokenTypes::BitwiseAnd) {
            dataTypes.push_back("&");
        } else if (currentToken.getType() == TokenTypes::QuestionMark) {
            dataTypes.push_back("?");
        }
        eat(currentToken.getType());
    }

    // Now check for the base type (identifier or array)
    if (currentToken.getType() == TokenTypes::Identifier) {
        dataTypes.push_back(currentToken.getValue());
        eat(TokenTypes::Identifier);
        
        // Handle namespaced types like `Numbers.i8`
        while (currentToken.getType() == TokenTypes::Dot) {
            eat(TokenTypes::Dot);
            dataTypes.push_back(currentToken.getValue());
            eat(TokenTypes::Identifier);
        }
    }

    // After processing the base type, check for pointers or references after the type
    while (
            currentToken.getType() == TokenTypes::Multiply ||
            currentToken.getType() == TokenTypes::BitwiseAnd ||
            currentToken.getType() == TokenTypes::BitwiseAnd ||
            currentToken.getType() == TokenTypes::QuestionMark
        ) {
        if (currentToken.getType() == TokenTypes::Multiply) {
            dataTypes.push_back("*");
        } else if (currentToken.getType() == TokenTypes::BitwiseAnd) {
            dataTypes.push_back("&");
        } else if (currentToken.getType() == TokenTypes::QuestionMark) {
            dataTypes.push_back("?");
        }
        eat(currentToken.getType());
    }

    // Handle array notation (e.g., [N])
    if (currentToken.getType() == TokenTypes::LeftBracket) {
        dataTypes.push_back("[");
        eat(TokenTypes::LeftBracket);
        
        // Handle array size, either an identifier or an integer literal
        if (currentToken.getType() == TokenTypes::Identifier) {
            dataTypes.push_back(currentToken.getValue());
            eat(TokenTypes::Identifier);
        } else if (currentToken.getType() == TokenTypes::IntegerLiteral) {
            dataTypes.push_back(currentToken.getValue());
            eat(TokenTypes::IntegerLiteral);
        }

        dataTypes.push_back("]");
        eat(TokenTypes::RightBracket);

        if (currentToken.getType() == TokenTypes::Identifier) {
            dataTypes.push_back(currentToken.getValue());
            eat(TokenTypes::Identifier);
        }
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
