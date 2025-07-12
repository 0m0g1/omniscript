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
            auto param = std::dynamic_pointer_cast<ParameterStatement>(parameters.back());
            if (!param) {
                console.error("You cannont have a stand alone variadic.\nAdd a parameter to capture the variadic arguments.");
            }
            param->isVariadic = true;
            eat(TokenTypes::Ellipsis);
            break;
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
