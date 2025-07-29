#include <omniscript/Statement.h>
#include <omniscript/Statements/CallableStatement.h>
#include <omniscript/Statements/LiteralStatements.h>
#include <omniscript/Core.h>
#include <omniscript/utils.h>
#include <omniscript/Lexer.h>
#include <omniscript/Parser.h>
#include <omniscript/Tokens.h>
#include <omniscript/Symboltable.h>
#include <omniscript/omniscript_pch.h>

std::vector<std::shared_ptr<Statement>> Parser::parseParameters() {
    Token startToken = currentToken;
    Omniscript::FileSpan span;
    span.start.line = startToken.getLine();
    span.start.col = startToken.getColumn();
    span.start.filePath = startToken.getFilePath();

    eat(TokenTypes::LeftParen, [&]() {
        std::string suggestion = Omniscript::Console::formatString(
            "To resolve this:\n"
            "1. Start parameter list with '('\n"
            "2. Check function declaration syntax\n"
            "3. Expected token: '(', found '%s'",
            getTokenTypeName(currentToken.getType()).c_str()
        );
        console.reportError(
            Omniscript::Console::SYNTAX_ERROR,
            Omniscript::Console::formatString("Expected '(' for parameter list, found '%s'", 
                getTokenTypeName(currentToken.getType()).c_str()),
            suggestion,
            span
        );
    });
    
    std::vector<std::shared_ptr<Statement>> parameters;
    
    while (currentToken.getType() != TokenTypes::RightParen && currentToken.getType() != TokenTypes::EOI) {
        Token paramStartToken = currentToken;
        std::string paramName;
        bool isVariadic = false;
        std::shared_ptr<Omniscript::Type> paramType;
        std::shared_ptr<Statement> defaultValue = nullptr;

        if (currentToken.getType() == TokenTypes::Ellipsis) {
            if (parameters.empty()) {
                console.reportError(
                    Omniscript::Console::SYNTAX_ERROR,
                    "Cannot have a standalone variadic",
                    "To resolve this:\n1. Add a parameter to capture variadic arguments\n2. Ensure variadic follows a named parameter",
                    span
                );
                return parameters;
            }
            auto param = std::dynamic_pointer_cast<ParameterStatement>(parameters.back());
            if (!param) {
                console.reportError(
                    Omniscript::Console::SYNTAX_ERROR,
                    "Invalid variadic parameter",
                    "To resolve this:\n1. Ensure variadic follows a valid parameter\n2. Check parameter syntax",
                    span
                );
                return parameters;
            }
            param->isVariadic = true;
            eat(TokenTypes::Ellipsis);
            break;
        }

        if (currentToken.getType() == TokenTypes::Identifier) {
            paramName = currentToken.getValue();
            eat(TokenTypes::Identifier);
        } else {
            std::string suggestion = Omniscript::Console::formatString(
                "To resolve this:\n"
                "1. Provide a valid parameter name\n"
                "2. Check parameter syntax\n"
                "3. Expected token: identifier, found '%s'",
                getTokenTypeName(currentToken.getType()).c_str()
            );
            console.reportError(
                Omniscript::Console::SYNTAX_ERROR,
                Omniscript::Console::formatString("Expected parameter name, found '%s'", 
                    getTokenTypeName(currentToken.getType()).c_str()),
                suggestion,
                span
            );
            return parameters;
        }

        if (currentToken.getType() == TokenTypes::Colon) {
            eat(TokenTypes::Colon);
            std::vector<std::string> types = parseType();
            paramType = Omniscript::resolveType(types);
            if (!paramType) {
                console.reportError(
                    Omniscript::Console::SYNTAX_ERROR,
                    "Invalid parameter type",
                    "To resolve this:\n1. Verify type syntax\n2. Ensure type is defined\n3. Check for valid type identifiers",
                    span
                );
                return parameters;
            }
        }

        if (currentToken.getType() == TokenTypes::Assign) {
            eat(TokenTypes::Assign);
            defaultValue = parseExpression();
            if (!defaultValue) {
                console.reportError(
                    Omniscript::Console::SYNTAX_ERROR,
                    "Invalid default value expression",
                    "To resolve this:\n1. Provide a valid default value\n2. Check expression syntax\n3. Ensure valid literals or identifiers",
                    span
                );
                return parameters;
            }
        } else {
            defaultValue = std::make_shared<Invalid>();
        }

        auto parameter = std::make_shared<ParameterStatement>(paramName, defaultValue);
        parameter->isVariadic = isVariadic;
        parameter->setType(paramType);
        parameter->setPosition(paramStartToken, previousToken);
        parameter->setSpan(span);
        parameters.push_back(parameter);

        if (currentToken.getType() == TokenTypes::Comma) {
            eat(TokenTypes::Comma);
        } else if (currentToken.getType() != TokenTypes::RightParen) {
            console.reportError(
                Omniscript::Console::SYNTAX_ERROR,
                Omniscript::Console::formatString("Expected ',' or ')', found '%s'", 
                    getTokenTypeName(currentToken.getType()).c_str()),
                "To resolve this:\n1. Separate parameters with ',' or close with ')'\n2. Check parameter list syntax\n3. Ensure valid parameter declarations",
                span
            );
            return parameters;
        }
    }

    eat(TokenTypes::RightParen, [&]() {
        std::string suggestion = Omniscript::Console::formatString(
            "To resolve this:\n"
            "1. Close parameter list with ')'\n"
            "2. Check for matching parentheses\n"
            "3. Expected token: ')', found '%s'",
            getTokenTypeName(currentToken.getType()).c_str()
        );
        console.reportError(
            Omniscript::Console::SYNTAX_ERROR,
            Omniscript::Console::formatString("Expected ')' to close parameter list, found '%s'", 
                getTokenTypeName(currentToken.getType()).c_str()),
            suggestion,
            span
        );
    });

    span.end.line = previousToken.getLine();
    span.end.col = previousToken.getColumn();
    span.end.filePath = previousToken.getFilePath();

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
    Token startToken = currentToken;
    Omniscript::FileSpan span;
    span.start.line = startToken.getLine();
    span.start.col = startToken.getColumn();
    span.start.filePath = startToken.getFilePath();

    DEBUG_LOG("Parsing the arguments");
    std::vector<std::shared_ptr<Statement>> args;
    eat(start, [&]() {
        std::string suggestion = Omniscript::Console::formatString(
            "To resolve this:\n"
            "1. Start argument list with '%s'\n"
            "2. Check argument syntax\n"
            "3. Expected token: '%s', found '%s'",
            getTokenTypeName(start).c_str(), getTokenTypeName(start).c_str(),
            getTokenTypeName(currentToken.getType()).c_str()
        );
        console.reportError(
            Omniscript::Console::SYNTAX_ERROR,
            Omniscript::Console::formatString("Expected '%s' for argument list, found '%s'", 
                getTokenTypeName(start).c_str(), getTokenTypeName(currentToken.getType()).c_str()),
            suggestion,
            span
        );
    });

    int argCount = 0;

    while (currentToken.getType() != end && currentToken.getType() != TokenTypes::EOI) {
        Token argStartToken = currentToken;
        std::shared_ptr<Statement> arg;

        if (currentToken.getType() == TokenTypes::Identifier && lexer.peekToken(1).getType() == assignOp) {
            std::string paramName = currentToken.getValue();
            eat(TokenTypes::Identifier);
            eat(assignOp, [&]() {
                std::string suggestion = Omniscript::Console::formatString(
                    "To resolve this:\n"
                    "1. Use '%s' for named argument\n"
                    "2. Check argument syntax\n"
                    "3. Expected token: '%s', found '%s'",
                    getTokenTypeName(assignOp).c_str(), getTokenTypeName(assignOp).c_str(),
                    getTokenTypeName(currentToken.getType()).c_str()
                );
                console.reportError(
                    Omniscript::Console::SYNTAX_ERROR,
                    Omniscript::Console::formatString("Expected '%s' for named argument, found '%s'", 
                        getTokenTypeName(assignOp).c_str(), getTokenTypeName(currentToken.getType()).c_str()),
                    suggestion,
                    span
                );
            });
            arg = parseExpression();
            if (!arg) {
                console.reportError(
                    Omniscript::Console::SYNTAX_ERROR,
                    "Invalid named argument expression",
                    "To resolve this:\n1. Provide a valid expression for named argument\n2. Check expression syntax\n3. Ensure valid literals or identifiers",
                    span
                );
                return args;
            }
        } else {
            arg = parseExpression();
            if (!arg) {
                console.reportError(
                    Omniscript::Console::SYNTAX_ERROR,
                    "Invalid argument expression",
                    "To resolve this:\n1. Provide a valid expression\n2. Check expression syntax\n3. Ensure valid literals or identifiers",
                    span
                );
                return args;
            }
        }

        arg->setPosition(argStartToken, previousToken);
        arg->setSpan(span);
        args.push_back(arg);

        if (currentToken.getType() == TokenTypes::Comma) {
            eat(TokenTypes::Comma);
            if (currentToken.getType() == end) {
                console.reportError(
                    Omniscript::Console::SYNTAX_ERROR,
                    "Unexpected comma before closing parenthesis",
                    "To resolve this:\n1. Remove trailing comma\n2. Ensure arguments are properly separated\n3. Check argument list syntax",
                    span
                );
                return args;
            }
        } else if (currentToken.getType() != end) {
            console.reportError(
                Omniscript::Console::SYNTAX_ERROR,
                Omniscript::Console::formatString("Expected ',' or '%s', found '%s'", 
                    getTokenTypeName(end).c_str(), getTokenTypeName(currentToken.getType()).c_str()),
                "To resolve this:\n1. Separate arguments with ',' or close with '%s'\n2. Check argument list syntax\n3. Ensure valid argument declarations",
                span
            );
            return args;
        }
        argCount++;
    }

    eat(end, [&]() {
        std::string suggestion = Omniscript::Console::formatString(
            "To resolve this:\n"
            "1. Close argument list with '%s'\n"
            "2. Check for matching delimiters\n"
            "3. Expected token: '%s', found '%s'",
            getTokenTypeName(end).c_str(), getTokenTypeName(end).c_str(),
            getTokenTypeName(currentToken.getType()).c_str()
        );
        console.reportError(
            Omniscript::Console::SYNTAX_ERROR,
            Omniscript::Console::formatString("Expected '%s' to close argument list, found '%s'", 
                getTokenTypeName(end).c_str(), getTokenTypeName(currentToken.getType()).c_str()),
            suggestion,
            span
        );
    });

    span.end.line = previousToken.getLine();
    span.end.col = previousToken.getColumn();
    span.end.filePath = previousToken.getFilePath();

    DEBUG_LOG("Done parsing the arguments");
    return args;
}

MemberModifiers Parser::parseMemberModifiers() {
    Token startToken = currentToken;
    Omniscript::FileSpan span;
    span.start.line = startToken.getLine();
    span.start.col = startToken.getColumn();
    span.start.filePath = startToken.getFilePath();

    MemberModifiers modifiers;

    while (currentToken.getType() == TokenTypes::Private || 
           currentToken.getType() == TokenTypes::Public || 
           currentToken.getType() == TokenTypes::Protected ||
           currentToken.getType() == TokenTypes::Override ||
           currentToken.getType() == TokenTypes::Static ||
           currentToken.getType() == TokenTypes::Final ||
           currentToken.getType() == TokenTypes::Virtual ||
           (currentToken.getType() == TokenTypes::Const &&
            lexer.peekToken(1).getType() != TokenTypes::Identifier &&
            lexer.peekToken(1).getType() != TokenTypes::Colon &&
            lexer.peekToken(1).getType() != TokenTypes::Assign)) {

        modifiers.isInitialized = true;

        if (currentToken.getType() == TokenTypes::Private) {
            // if (modifiers.access != MemberModifiers::AccessModifier::None) {
            //     console.reportError(
            //         Omniscript::Console::SYNTAX_ERROR,
            //         "Multiple access modifiers specified",
            //         "To resolve this:\n1. Use only one access modifier (public, private, or protected)\n2. Check modifier syntax\n3. Remove duplicate access modifiers",
            //         span
            //     );
            //     return modifiers;
            // }
            modifiers.access = MemberModifiers::AccessModifier::Private;
            eat(TokenTypes::Private);
        } else if (currentToken.getType() == TokenTypes::Public) {
            // if (modifiers.access != MemberModifiers::AccessModifier::None) {
            //     console.reportError(
            //         Omniscript::Console::SYNTAX_ERROR,
            //         "Multiple access modifiers specified",
            //         "To resolve this:\n1. Use only one access modifier (public, private, or protected)\n2. Check modifier syntax\n3. Remove duplicate access modifiers",
            //         span
            //     );
            //     return modifiers;
            // }
            modifiers.access = MemberModifiers::AccessModifier::Public;
            eat(TokenTypes::Public);
        } else if (currentToken.getType() == TokenTypes::Protected) {
            // if (modifiers.access != MemberModifiers::AccessModifier::None) {
            //     console.reportError(
            //         Omniscript::Console::SYNTAX_ERROR,
            //         "Multiple access modifiers specified",
            //         "To resolve this:\n1. Use only one access modifier (public, private, or protected)\n2. Check modifier syntax\n3. Remove duplicate access modifiers",
            //         span
            //     );
            //     return modifiers;
            // }
            modifiers.access = MemberModifiers::AccessModifier::Protected;
            eat(TokenTypes::Protected);
        } else if (currentToken.getType() == TokenTypes::Override) {
            if (modifiers.shouldOverride) {
                console.reportError(
                    Omniscript::Console::SYNTAX_ERROR,
                    "Duplicate 'override' modifier",
                    "To resolve this:\n1. Use 'override' only once\n2. Check modifier syntax\n3. Remove duplicate modifiers",
                    span
                );
                return modifiers;
            }
            modifiers.shouldOverride = true;
            eat(TokenTypes::Override);
        } else if (currentToken.getType() == TokenTypes::Static) {
            if (modifiers.isStatic) {
                console.reportError(
                    Omniscript::Console::SYNTAX_ERROR,
                    "Duplicate 'static' modifier",
                    "To resolve this:\n1. Use 'static' only once\n2. Check modifier syntax\n3. Remove duplicate modifiers",
                    span
                );
                return modifiers;
            }
            modifiers.isStatic = true;
            eat(TokenTypes::Static);
        } else if (currentToken.getType() == TokenTypes::Final) {
            if (modifiers.isFinal) {
                console.reportError(
                    Omniscript::Console::SYNTAX_ERROR,
                    "Duplicate 'final' modifier",
                    "To resolve this:\n1. Use 'final' only once\n2. Check modifier syntax\n3. Remove duplicate modifiers",
                    span
                );
                return modifiers;
            }
            modifiers.isFinal = true;
            eat(TokenTypes::Final);
        } else if (currentToken.getType() == TokenTypes::Virtual) {
            if (modifiers.isVirtual) {
                console.reportError(
                    Omniscript::Console::SYNTAX_ERROR,
                    "Duplicate 'virtual' modifier",
                    "To resolve this:\n1. Use 'virtual' only once\n2. Check modifier syntax\n3. Remove duplicate modifiers",
                    span
                );
                return modifiers;
            }
            modifiers.isVirtual = true;
            eat(TokenTypes::Virtual);
        } else if (currentToken.getType() == TokenTypes::Const) {
            if (modifiers.isConst) {
                console.reportError(
                    Omniscript::Console::SYNTAX_ERROR,
                    "Duplicate 'const' modifier",
                    "To resolve this:\n1. Use 'const' only once\n2. Check modifier syntax\n3. Remove duplicate modifiers",
                    span
                );
                return modifiers;
            }
            modifiers.isConst = true;
            eat(TokenTypes::Const);
        }
    }

    span.end.line = previousToken.getLine();
    span.end.col = previousToken.getColumn();
    span.end.filePath = previousToken.getFilePath();

    return modifiers;
}