#include <omniscript/Statements/Statement.h>
#include <omniscript/Statements/AccessStatements.h>
#include <omniscript/Statements/FunctionStatement.h>
#include <omniscript/Statements/CallableStatement.h>
#include <omniscript/Statements/LiteralStatements.h>
#include <omniscript/Statements/AssignmentAndGetterStatements.h>

#include <omniscript/Core.h>
#include <omniscript/utils.h>
#include <omniscript/Lexer.h>
#include <omniscript/Tokens.h>
#include <omniscript/Parser.h>
#include <omniscript/Symboltable.h>
#include <omniscript/Types/Types.h>
#include <omniscript/omniscript_pch.h>
#include <omniscript/LibraryPaths.h>

namespace Omniscript {

std::shared_ptr<Statement> Parser::parseExternFunction() {
    Token startToken = currentToken;
    FileSpan span;
    span.start.line = startToken.getLine();
    span.start.col = startToken.getColumn();
    span.start.filePath = startToken.getFilePath();

    eat(TokenTypes::Extern, [&]() {
        std::string suggestion = Console::formatString(
            "To resolve this:\n"
            "1. Start extern declaration with 'extern' keyword\n"
            "2. Check for correct syntax\n"
            "3. Expected token: 'extern', found '%s'",
            getTokenTypeName(currentToken.getType()).c_str()
        );
        console.reportError(
            Console::SYNTAX_ERROR,
            Console::formatString("Expected 'extern' keyword, found '%s'", 
                getTokenTypeName(currentToken.getType()).c_str()),
            suggestion,
            span
        );
    });

    LibraryPaths libraryPaths;
    std::vector<std::string> paths;

    // Parse first path
    if (currentToken.getType() != TokenTypes::StringLiteral) {
        std::string suggestion = Console::formatString(
            "To resolve this:\n"
            "1. Provide a valid library path as a string literal\n"
            "2. Check extern declaration syntax\n"
            "3. Expected token: string literal, found '%s'",
            getTokenTypeName(currentToken.getType()).c_str()
        );
        console.reportError(
            Console::SYNTAX_ERROR,
            Console::formatString("Expected string literal for library path, found '%s'", 
                getTokenTypeName(currentToken.getType()).c_str()),
            suggestion,
            span
        );
        return nullptr;
    }
    paths.push_back(currentToken.getValue());
    eat(TokenTypes::StringLiteral);

    // Parse additional paths
    while (currentToken.getType() == TokenTypes::Comma) {
        eat(TokenTypes::Comma);
        if (currentToken.getType() != TokenTypes::StringLiteral) {
            std::string suggestion = Console::formatString(
                "To resolve this:\n"
                "1. Provide a valid library path as a string literal after comma\n"
                "2. Check extern declaration syntax\n"
                "3. Expected token: string literal, found '%s'",
                getTokenTypeName(currentToken.getType()).c_str()
            );
            console.reportError(
                Console::SYNTAX_ERROR,
                Console::formatString("Expected string literal for library path, found '%s'", 
                    getTokenTypeName(currentToken.getType()).c_str()),
                suggestion,
                span
            );
            return nullptr;
        }
        paths.push_back(currentToken.getValue());
        eat(TokenTypes::StringLiteral);
    }

    // Classify and assign paths
    auto classifyPath = [](const std::string& path) -> std::pair<std::string, std::string> {
        std::string lowerPath = path;
        std::transform(lowerPath.begin(), lowerPath.end(), lowerPath.begin(), ::tolower);

        std::string libType = "unknown";
        if (lowerPath.ends_with(".dll")) libType = "dynamic";
        else if (lowerPath.ends_with(".so")) libType = "shared";
        else if (lowerPath.ends_with(".dylib")) libType = "dylib";
        else if (lowerPath.ends_with(".a") || lowerPath.ends_with(".lib")) libType = "static";
        else libType = "extensionless";

        std::string platform = "generic";
        if (lowerPath.find("win") != std::string::npos || 
            lowerPath.find("mingw") != std::string::npos ||
            lowerPath.find("windows") != std::string::npos ||
            lowerPath.ends_with(".dll") || lowerPath == "kernel32" || lowerPath == "user32") {
            platform = "windows";
        }
        else if (lowerPath.find("linux") != std::string::npos ||
                 lowerPath.find("unix") != std::string::npos ||
                 lowerPath.ends_with(".so") || lowerPath == "glfw" || lowerPath == "x11") {
            platform = "linux";
        }
        else if (lowerPath.find("mac") != std::string::npos ||
                 lowerPath.find("darwin") != std::string::npos ||
                 lowerPath.find("osx") != std::string::npos ||
                 lowerPath.ends_with(".dylib")) {
            platform = "macos";
        }

        return {platform, libType};
    };

    for (const auto& path : paths) {
        auto [platform, libType] = classifyPath(path);

        if (libType == "extensionless") {
            if (libraryPaths.genericDynamic.empty()) {
                libraryPaths.genericDynamic = path;
            }
        } else if (platform == "windows") {
            if (libType == "dynamic") {
                if (libraryPaths.windowsDynamic.empty()) libraryPaths.windowsDynamic = path;
            } else if (libType == "static") {
                if (libraryPaths.windowsStatic.empty()) libraryPaths.windowsStatic = path;
            }
        } else if (platform == "linux") {
            if (libType == "shared") {
                if (libraryPaths.linuxShared.empty()) libraryPaths.linuxShared = path;
            } else if (libType == "static") {
                if (libraryPaths.linuxStatic.empty()) libraryPaths.linuxStatic = path;
            }
        } else if (platform == "macos") {
            if (libType == "dylib") {
                if (libraryPaths.macosShared.empty()) libraryPaths.macosShared = path;
            } else if (libType == "static") {
                if (libraryPaths.macosStatic.empty()) libraryPaths.macosStatic = path;
            }
        } else {
            if ((libType == "dynamic" || libType == "shared" || libType == "dylib" || libType == "extensionless")
                && libraryPaths.genericDynamic.empty()) {
                libraryPaths.genericDynamic = path;
            }
            if (libType == "static" && libraryPaths.genericStatic.empty()) {
                libraryPaths.genericStatic = path;
            }
        }
    }

    if (libraryPaths.genericDynamic.empty() && libraryPaths.genericStatic.empty() &&
        libraryPaths.windowsDynamic.empty() && libraryPaths.windowsStatic.empty() &&
        libraryPaths.linuxShared.empty() && libraryPaths.linuxStatic.empty() &&
        libraryPaths.macosShared.empty() && libraryPaths.macosStatic.empty()) {
        console.reportError(
            Console::SYNTAX_ERROR,
            "No valid library paths found in extern declaration",
            "To resolve this:\n1. Provide at least one valid library path\n2. Ensure paths are string literals\n3. Check for supported extensions (.dll, .so, .dylib, .a, .lib)",
            span
        );
        return nullptr;
    }

    if (currentToken.getType() == TokenTypes::Function) {
        eat(TokenTypes::Function);
        std::string functionName = currentToken.getValue();
        eat(TokenTypes::Identifier, [&]() {
            std::string suggestion = Console::formatString(
                "To resolve this:\n"
                "1. Provide a valid function name after 'fn'\n"
                "2. Check extern function syntax\n"
                "3. Expected token: identifier, found '%s'",
                getTokenTypeName(currentToken.getType()).c_str()
            );
            console.reportError(
                Console::SYNTAX_ERROR,
                Console::formatString("Expected identifier for function name, found '%s'", 
                    getTokenTypeName(currentToken.getType()).c_str()),
                suggestion,
                span
            );
        });

        auto function = std::dynamic_pointer_cast<FunctionDeclaration>(
            parseLambdaFunction(functionName));
        if (!function) {
            console.reportError(
                Console::SYNTAX_ERROR,
                "Failed to parse extern function declaration",
                "To resolve this:\n1. Verify lambda function syntax\n2. Check parameter and return type syntax\n3. Ensure proper function body",
                span
            );
            return nullptr;
        }
        
        function->isExtern = true;
        function->libraryPaths = libraryPaths;
        function->externName = functionName;
        function->setPosition(startToken, currentToken);
        function->setSpan(span);

        span.end.line = previousToken.getLine();
        span.end.col = previousToken.getColumn();
        span.end.filePath = previousToken.getFilePath();

        return function;

    } else if (currentToken.getType() == TokenTypes::Let || currentToken.getType() == TokenTypes::Const) {
        auto assignment = std::dynamic_pointer_cast<AssignVariable>(parseAssignment());
        if (!assignment) {
            console.reportError(
                Console::SYNTAX_ERROR,
                "Invalid assignment after let/const in extern declaration",
                "To resolve this:\n1. Verify assignment syntax\n2. Check for valid variable declaration\n3. Ensure proper initialization",
                span
            );
            return nullptr;
        }
        assignment->isExtern = true;
        assignment->externName = assignment->getName();
        assignment->libraryPaths.windowsDynamic  = libraryPaths.windowsDynamic;   // e.g., "lib/foo.dll"
        assignment->libraryPaths.windowsStatic   = libraryPaths.windowsStatic;    // e.g., "lib/foo.lib"
        assignment->libraryPaths.linuxShared     = libraryPaths.linuxShared;      // e.g., "libfoo.so"
        assignment->libraryPaths.linuxStatic     = libraryPaths.linuxStatic;      // e.g., "libfoo.a"
        assignment->libraryPaths.macosShared     = libraryPaths.macosShared;      // e.g., "libfoo.dylib"
        assignment->libraryPaths.macosStatic     = libraryPaths.macosStatic;      // e.g., "libfoo.a"
        assignment->libraryPaths.genericDynamic  = libraryPaths.genericDynamic;   // fallback .so/.dll/.dylib
        assignment->libraryPaths.genericStatic   = libraryPaths.genericStatic;    // fallback .a/.lib

        assignment->setPosition(startToken, currentToken);
        assignment->setSpan(span);

        span.end.line = previousToken.getLine();
        span.end.col = previousToken.getColumn();
        span.end.filePath = previousToken.getFilePath();

        return assignment;
    } else if (currentToken.getType() == TokenTypes::LeftBrace) {
        eat(TokenTypes::LeftBrace);
        std::vector<std::shared_ptr<Statement>> functions;

        while (currentToken.getType() != TokenTypes::RightBrace) {
            if (currentToken.getType() == TokenTypes::Function) {
                eat(TokenTypes::Function);
                std::string functionName = currentToken.getValue();
                eat(TokenTypes::Identifier, [&]() {
                    std::string suggestion = Console::formatString(
                        "To resolve this:\n"
                        "1. Provide a valid function name after 'fn'\n"
                        "2. Check extern function syntax\n"
                        "3. Expected token: identifier, found '%s'",
                        getTokenTypeName(currentToken.getType()).c_str()
                    );
                    console.reportError(
                        Console::SYNTAX_ERROR,
                        Console::formatString("Expected identifier for function name, found '%s'", 
                            getTokenTypeName(currentToken.getType()).c_str()),
                        suggestion,
                        span
                    );
                });
    
                auto function = std::dynamic_pointer_cast<FunctionDeclaration>(
                    parseLambdaFunction(functionName));
                if (!function) {
                    console.reportError(
                        Console::SYNTAX_ERROR,
                        "Failed to parse extern function in block",
                        "To resolve this:\n1. Verify lambda function syntax\n2. Check parameter and return type syntax\n3. Ensure proper function body",
                        span
                    );
                    return nullptr;
                }
                
                function->isExtern = true;
                function->libraryPaths = libraryPaths;
                function->externName = functionName;
                function->setPosition(startToken, currentToken);
                function->setSpan(span);
    
                functions.push_back(function);
                
            } else if (currentToken.getType() == TokenTypes::Let || currentToken.getType() == TokenTypes::Const) {
                auto assignment = std::dynamic_pointer_cast<AssignVariable>(parseAssignment());
                if (!assignment) {
                    console.reportError(
                        Console::SYNTAX_ERROR,
                        "Invalid assignment after let/const in extern block",
                        "To resolve this:\n1. Verify assignment syntax\n2. Check for valid variable declaration\n3. Ensure proper initialization",
                        span
                    );
                    return nullptr;
                }
                assignment->isExtern = true;
                assignment->externName = assignment->getName();
                assignment->libraryPaths.windowsDynamic  = libraryPaths.windowsDynamic;   // e.g., "lib/foo.dll"
                assignment->libraryPaths.windowsStatic   = libraryPaths.windowsStatic;    // e.g., "lib/foo.lib"
                assignment->libraryPaths.linuxShared     = libraryPaths.linuxShared;      // e.g., "libfoo.so"
                assignment->libraryPaths.linuxStatic     = libraryPaths.linuxStatic;      // e.g., "libfoo.a"
                assignment->libraryPaths.macosShared     = libraryPaths.macosShared;      // e.g., "libfoo.dylib"
                assignment->libraryPaths.macosStatic     = libraryPaths.macosStatic;      // e.g., "libfoo.a"
                assignment->libraryPaths.genericDynamic  = libraryPaths.genericDynamic;   // fallback .so/.dll/.dylib
                assignment->libraryPaths.genericStatic   = libraryPaths.genericStatic;    // fallback .a/.lib

                assignment->setPosition(startToken, currentToken);
                assignment->setSpan(span);

                functions.push_back(assignment);
            } else {
                console.reportError(
                    Console::SYNTAX_ERROR,
                    Console::formatString("Expected 'fn', 'let', or 'const' in extern block, found '%s'", 
                        getTokenTypeName(currentToken.getType()).c_str()),
                    "To resolve this:\n1. Use 'fn' for function declarations or 'let'/'const' for variable declarations\n2. Check extern block syntax\n3. Ensure valid declarations",
                    span
                );
                return nullptr;
            }

            if (currentToken.getType() == TokenTypes::Semicolon) {
                eat(TokenTypes::Semicolon);
            }
        }

        eat(TokenTypes::RightBrace, [&]() {
            std::string suggestion = Console::formatString(
                "To resolve this:\n"
                "1. Close extern block with '}'\n"
                "2. Check for matching braces\n"
                "3. Expected token: '}', found '%s'",
                getTokenTypeName(currentToken.getType()).c_str()
            );
            console.reportError(
                Console::SYNTAX_ERROR,
                Console::formatString("Expected '}' to close extern block, found '%s'", 
                    getTokenTypeName(currentToken.getType()).c_str()),
                suggestion,
                span
            );
        });

        span.end.line = previousToken.getLine();
        span.end.col = previousToken.getColumn();
        span.end.filePath = previousToken.getFilePath();

        auto block = std::make_shared<BlockStatement>(functions);
        block->setPosition(startToken, currentToken);
        block->setSpan(span);
        return block;
    }

    console.reportError(
        Console::SYNTAX_ERROR,
        Console::formatString("Expected 'fn', 'let', 'const', or '{' after extern library paths, found '%s'", 
            getTokenTypeName(currentToken.getType()).c_str()),
        "To resolve this:\n1. Use 'fn' for function declarations, 'let'/'const' for variables, or '{' for a block\n2. Check extern declaration syntax\n3. Ensure valid declarations",
        span
    );
    return nullptr;
}

std::shared_ptr<Statement> Parser::parseIntrinsicFunction() {
    Token startToken = currentToken;
    FileSpan span;
    span.start.line = startToken.getLine();
    span.start.col = startToken.getColumn();
    span.start.filePath = startToken.getFilePath();

    eat(TokenTypes::Intrinsic, [&]() {
        std::string suggestion = Console::formatString(
            "To resolve this:\n"
            "1. Start intrinsic declaration with 'intrinsic' keyword\n"
            "2. Check for correct syntax\n"
            "3. Expected token: 'intrinsic', found '%s'",
            getTokenTypeName(currentToken.getType()).c_str()
        );
        console.reportError(
            Console::SYNTAX_ERROR,
            Console::formatString("Expected 'intrinsic' keyword, found '%s'", 
                getTokenTypeName(currentToken.getType()).c_str()),
            suggestion,
            span
        );
    });

    if (currentToken.getType() == TokenTypes::Function) {
        eat(TokenTypes::Function);
    }

    std::string functionName = currentToken.getValue();
    eat(TokenTypes::Identifier, [&]() {
        std::string suggestion = Console::formatString(
            "To resolve this:\n"
            "1. Provide a valid function name after 'intrinsic'\n"
            "2. Check intrinsic function syntax\n"
            "3. Expected token: identifier, found '%s'",
            getTokenTypeName(currentToken.getType()).c_str()
        );
        console.reportError(
            Console::SYNTAX_ERROR,
            Console::formatString("Expected identifier for intrinsic function name, found '%s'", 
                getTokenTypeName(currentToken.getType()).c_str()),
            suggestion,
            span
        );
    });

    std::shared_ptr<FunctionDeclaration> function = std::dynamic_pointer_cast<FunctionDeclaration>(
        parseLambdaFunction(functionName));
    if (!function) {
        console.reportError(
            Console::SYNTAX_ERROR,
            "Failed to parse intrinsic function declaration",
            "To resolve this:\n1. Verify lambda function syntax\n2. Check parameter and return type syntax\n3. Ensure proper function body",
            span
        );
        return nullptr;
    }

    function->isIntrinsic = true;
    function->intrinsicName = functionName;
    function->setPosition(startToken, currentToken);
    function->setSpan(span);

    span.end.line = previousToken.getLine();
    span.end.col = previousToken.getColumn();
    span.end.filePath = previousToken.getFilePath();

    return function;
}

std::shared_ptr<Statement> Parser::parseFunctionDeclaration(
    parameterType paramTypes,
    std::shared_ptr<Type> type
) {
    return parseFunctionDeclaration("", paramTypes, type);
}

std::shared_ptr<Statement> Parser::parseFunctionDeclaration(
    const std::string& definedName,
    parameterType paramTypes,
    std::shared_ptr<Type> type
) {
    Token startToken = currentToken;
    FileSpan span;
    span.start.line = startToken.getLine();
    span.start.col = startToken.getColumn();
    span.start.filePath = startToken.getFilePath();

    std::string name = definedName;
    
    if (name.empty()) {
        eat(TokenTypes::Function, [&]() {
            std::string suggestion = Console::formatString(
                "To resolve this:\n"
                "1. Start function declaration with 'fn' keyword\n"
                "2. Check for correct syntax\n"
                "3. Expected token: 'fn', found '%s'",
                getTokenTypeName(currentToken.getType()).c_str()
            );
            console.reportError(
                Console::SYNTAX_ERROR,
                Console::formatString("Expected 'fn' keyword, found '%s'", 
                    getTokenTypeName(currentToken.getType()).c_str()),
                suggestion,
                span
            );
        });
        name = currentToken.getValue();
        eat(TokenTypes::Identifier, [&]() {
            std::string suggestion = Console::formatString(
                "To resolve this:\n"
                "1. Provide a valid function name after 'fn'\n"
                "2. Check function declaration syntax\n"
                "3. Expected token: identifier, found '%s'",
                getTokenTypeName(currentToken.getType()).c_str()
            );
            console.reportError(
                Console::SYNTAX_ERROR,
                Console::formatString("Expected identifier for function name, found '%s'", 
                    getTokenTypeName(currentToken.getType()).c_str()),
                suggestion,
                span
            );
        });
    }
    
    DEBUG_LOG("Parsing function " + name);

    parameterType types;
    if (paramTypes.empty()) {
        if (currentToken.getType() == TokenTypes::LessThan) {
            types = parseTypeParametersForDeclaration();
        }
    } else {
        types = paramTypes;
    }
    
    std::vector<std::shared_ptr<Statement>> parameters = parseParameters();

    if (type) {
        auto param = std::make_shared<ParameterStatement>("this", nullptr, true);
        param->setType(Type::createPointerType(type));
        param->setSpan(span);
        parameters.insert(parameters.begin(), std::dynamic_pointer_cast<Statement>(param));
    }

    std::shared_ptr<Type> returnType = nullptr;
    std::vector<std::string> returnDataType;
    if (currentToken.getType() != TokenTypes::LeftBrace) {
        eat(TokenTypes::Arrow, [&]() {
            std::string suggestion = Console::formatString(
                "To resolve this:\n"
                "1. Use '=>' for return type\n"
                "2. Check function declaration syntax\n"
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
        returnDataType = parseType();
        returnType = resolveType(returnDataType);
    } else {
        returnType = resolveType({"void"});
    }

    std::shared_ptr<BlockStatement> body;

    if (currentToken.getType() == TokenTypes::LeftBrace) {
        body = std::dynamic_pointer_cast<BlockStatement>(parseBlock());
        if (!body) {
            console.reportError(
                Console::SYNTAX_ERROR,
                "Failed to parse function body",
                "To resolve this:\n1. Verify block syntax\n2. Check for valid block structure\n3. Ensure block starts with '{'",
                span
            );
            return nullptr;
        }
    } else {
        body = BlockStatement::create();
    }

    span.end.line = previousToken.getLine();
    span.end.col = previousToken.getColumn();
    span.end.filePath = previousToken.getFilePath();

    if (!types.empty()) {
        std::vector<std::shared_ptr<Statement>> monomorphizedFunctions;

        // Special case: Single type parameter with simple alternatives
        if (types.size() == 1 && !types[0].second.empty()) {
            const auto& typeParam = types[0];
            const auto& constraints = typeParam.second;

            bool allSimple = std::all_of(constraints.begin(), constraints.end(),
                [](const std::vector<std::string>& c) {
                    return c.size() == 1 && c[0] != "any" && c[0] != "variant";
                });

            if (allSimple) {
                for (const auto& constraint : constraints) {
                    std::vector<std::pair<std::string, std::vector<std::string>>> selectedTypes = {
                        {typeParam.first, constraint}
                    };

                    std::string specializedName = generateSpecializedNameForDecleration(name, selectedTypes);
                    
                    std::vector<std::shared_ptr<Statement>> clonedParameters;
                    for (const auto& param : parameters) {
                        clonedParameters.push_back(param->clone());
                    }
                    
                    auto func = std::make_shared<FunctionDeclaration>(
                        specializedName, clonedParameters, std::dynamic_pointer_cast<BlockStatement>(body->clone()), returnType);
                    
                    func->addGenericParam(typeParam.first);
                    func->bindGeneric(typeParam.first, resolveType(constraint));
                    func->setPosition(startToken, currentToken);
                    func->setSpan(span);
                    
                    monomorphizedFunctions.push_back(func);
                }
                auto block = std::make_shared<BlockStatement>(monomorphizedFunctions);
                block->setPosition(startToken, currentToken);
                block->setSpan(span);
                return block;
            }
        }

        // General case: Use cartesian product for multiple type parameters
        std::vector<size_t> indices(types.size(), 0);
        std::vector<size_t> sizes;
        for (const auto& type : types) {
            sizes.push_back(type.second.empty() ? 1 : type.second.size());
        }

        bool done = false;
        while (!done) {
            std::vector<std::pair<std::string, std::vector<std::string>>> selectedTypes;
            for (size_t i = 0; i < types.size(); ++i) {
                const auto& type = types[i];
                std::vector<std::string> selectedConstraint;
                if (!type.second.empty()) {
                    selectedConstraint = type.second[indices[i]];
                }
                selectedTypes.emplace_back(type.first, selectedConstraint);
            }

            std::string specializedName = generateSpecializedNameForDecleration(name, selectedTypes);
            auto func = std::make_shared<FunctionDeclaration>(
                specializedName, parameters, std::dynamic_pointer_cast<BlockStatement>(body->clone()), returnType);

            for (const auto& genericPair : types) {
                func->addGenericParam(genericPair.first);
            }

            for (const auto& selected : selectedTypes) {
                if (!selected.second.empty()) {
                    func->bindGeneric(selected.first, resolveType(selected.second));
                }
            }

            func->setPosition(startToken, currentToken);
            func->setSpan(span);
            monomorphizedFunctions.push_back(func);

            for (size_t i = types.size(); i-- > 0;) {
                indices[i]++;
                if (indices[i] < sizes[i]) {
                    break;
                } else {
                    indices[i] = 0;
                    if (i == 0) done = true;
                }
            }
        }

        auto block = std::make_shared<BlockStatement>(monomorphizedFunctions);
        block->setPosition(startToken, currentToken);
        block->setSpan(span);
        return block;
    }

    // Normal function without generics
    returnType = resolveType(returnDataType);
    auto function = std::make_shared<FunctionDeclaration>(name, parameters, body, returnType);
    function->setPosition(startToken, currentToken);
    function->setSpan(span);
    return function;
}

std::string Parser::generateSpecializedNameForDecleration( 
    const std::string& baseName,
    const std::vector<std::pair<std::string, std::vector<std::string>>>& types
) {
    if (types.empty()) return baseName;

    std::ostringstream oss;
    oss << baseName;

    for (size_t i = 0; i < types.size(); ++i) {
        oss << "_";
        const auto& [genericName, concreteType] = types[i];

        for (size_t j = 0; j < concreteType.size(); ++j) {
            const auto& part = concreteType[j];
            if (part == "*") {
                oss << "ptr";
            } else if (part == "&") {
                oss << "ref";
            } else if (part == ".") {
                oss << "_";
            } else if (part == "[") {
                oss << "arr";
            } else if (part == "]") {
                // skip or treat as end marker
            } else {
                oss << part;
            }

            if (j < concreteType.size() - 1)
                oss << "_";
        }

        if (i < types.size() - 1)
            oss << "__";
    }

    return oss.str();
}

bool Parser::checkIfLambdaExpression() {
    int i = 0;
    DEBUG_LOG(getTokenTypeName(currentToken.getType()));
    
    // Check for optional lambda name
    if (currentToken.getType() == TokenTypes::Identifier) {
        i++;
    }
    
    DEBUG_LOG(getTokenTypeName(lexer.peekToken(i).getType()));
    
    // Check for parameter list
    if ((i == 0 ? currentToken.getType() : lexer.peekToken(i).getType()) == TokenTypes::LeftParen) {
        i++; // consume '('
        bool hasValidArgument = false;
        
        while (lexer.peekToken(i).getType() != TokenTypes::RightParen) {
            DEBUG_LOG(getTokenTypeName(lexer.peekToken(i).getType()));
            
            // Parse parameter name (required)
            if (lexer.peekToken(i).getType() == TokenTypes::Identifier) {
                hasValidArgument = true;
                i++; // consume parameter name
                
                // Check for type annotation
                if (lexer.peekToken(i).getType() == TokenTypes::Colon) {
                    i++; // consume ':'
                    
                    // Parse type - handle function types first
                    if (lexer.peekToken(i).getType() == TokenTypes::Function) {
                        i++; // consume 'fn'
                        
                        if (lexer.peekToken(i).getType() == TokenTypes::LeftParen) {
                            i++; // consume '('
                            
                            // Parse function parameters
                            while (lexer.peekToken(i).getType() != TokenTypes::RightParen) {
                                // Parse parameter name (optional)
                                if (lexer.peekToken(i).getType() == TokenTypes::Identifier) {
                                    if (lexer.peekToken(i + 1).getType() == TokenTypes::Colon) {
                                        i++; // consume parameter name
                                        i++; // consume ':'
                                    }
                                }
                                
                                // Skip over parameter type
                                while (lexer.peekToken(i).getType() == TokenTypes::Identifier ||
                                       lexer.peekToken(i).getType() == TokenTypes::Multiply ||
                                       lexer.peekToken(i).getType() == TokenTypes::BitwiseAnd ||
                                       lexer.peekToken(i).getType() == TokenTypes::QuestionMark ||
                                       lexer.peekToken(i).getType() == TokenTypes::Dot ||
                                       lexer.peekToken(i).getType() == TokenTypes::LeftBracket) {
                                    
                                    if (lexer.peekToken(i).getType() == TokenTypes::LeftBracket) {
                                        i++; // consume '['
                                        if (lexer.peekToken(i).getType() == TokenTypes::IntegerLiteral ||
                                            lexer.peekToken(i).getType() == TokenTypes::Identifier) {
                                            i++; // consume size
                                        } else {
                                            std::string suggestion = "To resolve this:\n"
                                                                   "1. Provide a valid array size (integer or identifier)\n"
                                                                   "2. Check array syntax\n"
                                                                   "3. Ensure correct bracket usage";
                                            console.reportError(
                                                Console::SYNTAX_ERROR,
                                                Console::formatString("Expected integer or identifier for array size, got '%s'",
                                                                 getTokenTypeName(lexer.peekToken(i).getType()).c_str()),
                                                suggestion
                                            );
                                            return false;
                                        }
                                        if (lexer.peekToken(i).getType() == TokenTypes::RightBracket) {
                                            i++; // consume ']'
                                        } else {
                                            std::string suggestion = "To resolve this:\n"
                                                                   "1. Close array type with ']'\n"
                                                                   "2. Check for matching brackets\n"
                                                                   "3. Verify array syntax";
                                            console.reportError(
                                                Console::SYNTAX_ERROR,
                                                Console::formatString("Expected ']' to close array type, got '%s'",
                                                                 getTokenTypeName(lexer.peekToken(i).getType()).c_str()),
                                                suggestion
                                            );
                                            return false;
                                        }
                                    } else {
                                        i++;
                                    }
                                }
                                
                                // Handle comma-separated function parameters
                                if (lexer.peekToken(i).getType() == TokenTypes::Comma) {
                                    i++; // consume ','
                                } else if (lexer.peekToken(i).getType() != TokenTypes::RightParen) {
                                    std::string suggestion = "To resolve this:\n"
                                                           "1. Use ',' to separate parameters or ')' to close parameter list\n"
                                                           "2. Check function parameter syntax\n"
                                                           "3. Ensure valid parameter declarations";
                                    console.reportError(
                                        Console::SYNTAX_ERROR,
                                        Console::formatString("Expected ',' or ')', got '%s'",
                                                         getTokenTypeName(lexer.peekToken(i).getType()).c_str()),
                                        suggestion
                                    );
                                    return false;
                                }
                            }
                            
                            i++; // consume ')'
                            
                            // Parse arrow operator =>
                            if (lexer.peekToken(i).getType() == TokenTypes::Arrow || 
                                (lexer.peekToken(i).getType() == TokenTypes::Assign && 
                                 lexer.peekToken(i + 1).getType() == TokenTypes::GreaterThan)) {
                                
                                if (lexer.peekToken(i).getType() == TokenTypes::Arrow) {
                                    i++; // consume '=>'
                                } else {
                                    i += 2; // consume '=' and '>'
                                }
                                
                                // Skip over return type
                                while (lexer.peekToken(i).getType() == TokenTypes::Identifier ||
                                       lexer.peekToken(i).getType() == TokenTypes::Multiply ||
                                       lexer.peekToken(i).getType() == TokenTypes::BitwiseAnd ||
                                       lexer.peekToken(i).getType() == TokenTypes::QuestionMark ||
                                       lexer.peekToken(i).getType() == TokenTypes::Dot ||
                                       lexer.peekToken(i).getType() == TokenTypes::LeftBracket) {
                                    
                                    if (lexer.peekToken(i).getType() == TokenTypes::LeftBracket) {
                                        i++; // consume '['
                                        if (lexer.peekToken(i).getType() == TokenTypes::IntegerLiteral ||
                                            lexer.peekToken(i).getType() == TokenTypes::Identifier) {
                                            i++; // consume size
                                        } else {
                                            std::string suggestion = "To resolve this:\n"
                                                                   "1. Provide a valid array size (integer or identifier)\n"
                                                                   "2. Check array syntax\n"
                                                                   "3. Ensure correct bracket usage";
                                            console.reportError(
                                                Console::SYNTAX_ERROR,
                                                Console::formatString("Expected integer or identifier for array size, got '%s'",
                                                                 getTokenTypeName(lexer.peekToken(i).getType()).c_str()),
                                                suggestion
                                            );
                                            return false;
                                        }
                                        if (lexer.peekToken(i).getType() == TokenTypes::RightBracket) {
                                            i++; // consume ']'
                                        } else {
                                            std::string suggestion = "To resolve this:\n"
                                                                   "1. Close array type with ']'\n"
                                                                   "2. Check for matching brackets\n"
                                                                   "3. Verify array syntax";
                                            console.reportError(
                                                Console::SYNTAX_ERROR,
                                                Console::formatString("Expected ']' to close array type, got '%s'",
                                                                 getTokenTypeName(lexer.peekToken(i).getType()).c_str()),
                                                suggestion
                                            );
                                            return false;
                                        }
                                    } else {
                                        i++;
                                    }
                                }
                            }
                        }
                    } else {
                        // Parse regular types (non-function types)
                        while (lexer.peekToken(i).getType() == TokenTypes::Multiply ||
                               lexer.peekToken(i).getType() == TokenTypes::BitwiseAnd ||
                               lexer.peekToken(i).getType() == TokenTypes::QuestionMark) {
                            i++;
                        }
                        
                        if (lexer.peekToken(i).getType() == TokenTypes::Identifier) {
                            i++;
                            
                            while (lexer.peekToken(i).getType() == TokenTypes::Dot) {
                                i++; // consume '.'
                                if (lexer.peekToken(i).getType() == TokenTypes::Identifier) {
                                    i++; // consume identifier
                                } else {
                                    std::string suggestion = "To resolve this:\n"
                                                           "1. Provide a valid identifier after '.'\n"
                                                           "2. Check dotted type syntax (e.g., std.vector)\n"
                                                           "3. Ensure type is defined";
                                    console.reportError(
                                        Console::SYNTAX_ERROR,
                                        Console::formatString("Expected identifier after '.', got '%s'",
                                                         getTokenTypeName(lexer.peekToken(i).getType()).c_str()),
                                        suggestion
                                    );
                                    return false;
                                }
                            }
                        } else {
                            std::string suggestion = "To resolve this:\n"
                                                   "1. Provide a valid type identifier\n"
                                                   "2. Check type annotation syntax\n"
                                                   "3. Ensure type is defined";
                            console.reportError(
                                Console::SYNTAX_ERROR,
                                Console::formatString("Expected type identifier, got '%s'",
                                                 getTokenTypeName(lexer.peekToken(i).getType()).c_str()),
                                suggestion
                            );
                            return false;
                        }
                        
                        while (lexer.peekToken(i).getType() == TokenTypes::Multiply ||
                               lexer.peekToken(i).getType() == TokenTypes::BitwiseAnd ||
                               lexer.peekToken(i).getType() == TokenTypes::QuestionMark) {
                            i++;
                        }
                        
                        while (lexer.peekToken(i).getType() == TokenTypes::LeftBracket) {
                            i++; // consume '['
                            if (lexer.peekToken(i).getType() == TokenTypes::IntegerLiteral ||
                                lexer.peekToken(i).getType() == TokenTypes::Identifier) {
                                i++; // consume size
                            } else {
                                std::string suggestion = "To resolve this:\n"
                                                       "1. Provide a valid array size (integer or identifier)\n"
                                                       "2. Check array syntax\n"
                                                       "3. Ensure correct bracket usage";
                                console.reportError(
                                    Console::SYNTAX_ERROR,
                                    Console::formatString("Expected integer or identifier for array size, got '%s'",
                                                     getTokenTypeName(lexer.peekToken(i).getType()).c_str()),
                                    suggestion
                                );
                                return false;
                            }
                            if (lexer.peekToken(i).getType() == TokenTypes::RightBracket) {
                                i++; // consume ']'
                            } else {
                                std::string suggestion = "To resolve this:\n"
                                                       "1. Close array type with ']'\n"
                                                       "2. Check for matching brackets\n"
                                                       "3. Verify array syntax";
                                console.reportError(
                                    Console::SYNTAX_ERROR,
                                    Console::formatString("Expected ']' to close array type, got '%s'",
                                                     getTokenTypeName(lexer.peekToken(i).getType()).c_str()),
                                    suggestion
                                );
                                return false;
                            }
                        }
                    }
                }
                
                // Check for default value
                if (lexer.peekToken(i).getType() == TokenTypes::Assign) {
                    i++; // consume '='
                    if (lexer.peekToken(i).getType() == TokenTypes::IntegerLiteral ||
                        lexer.peekToken(i).getType() == TokenTypes::FloatLiteral ||
                        lexer.peekToken(i).getType() == TokenTypes::StringLiteral ||
                        lexer.peekToken(i).getType() == TokenTypes::Identifier) {
                        i++;
                    } else {
                        std::string suggestion = "To resolve this:\n"
                                               "1. Provide a valid default value (integer, float, string, or identifier)\n"
                                               "2. Check default value syntax\n"
                                               "3. Ensure valid expression";
                        console.reportError(
                            Console::SYNTAX_ERROR,
                            Console::formatString("Expected valid default value, got '%s'",
                                             getTokenTypeName(lexer.peekToken(i).getType()).c_str()),
                            suggestion
                        );
                        return false;
                    }
                }
                
                if (lexer.peekToken(i).getType() == TokenTypes::Comma) {
                    i++; // consume ','
                } else if (lexer.peekToken(i).getType() != TokenTypes::RightParen) {
                    std::string suggestion = "To resolve this:\n"
                                           "1. Use ',' to separate parameters or ')' to close parameter list\n"
                                           "2. Check parameter syntax\n"
                                           "3. Ensure valid parameter declarations";
                    console.reportError(
                        Console::SYNTAX_ERROR,
                        Console::formatString("Expected ',' or ')', got '%s'",
                                         getTokenTypeName(lexer.peekToken(i).getType()).c_str()),
                        suggestion
                    );
                    return false;
                }
            } else {
                std::string suggestion = "To resolve this:\n"
                                       "1. Provide a valid parameter name\n"
                                       "2. Check parameter syntax\n"
                                       "3. Ensure valid identifier";
                console.reportError(
                    Console::SYNTAX_ERROR,
                    Console::formatString("Expected parameter name, got '%s'",
                                     getTokenTypeName(lexer.peekToken(i).getType()).c_str()),
                    suggestion
                );
                return false;
            }
        }
        
        if (lexer.peekToken(i).getType() == TokenTypes::RightParen) {
            i++; // consume ')'
            
            // Check for optional return type annotation
            if (lexer.peekToken(i).getType() == TokenTypes::Colon) {
                i++; // consume ':'
                
                if (lexer.peekToken(i).getType() == TokenTypes::Function) {
                    while (lexer.peekToken(i).getType() != TokenTypes::Arrow &&
                           lexer.peekToken(i).getType() != TokenTypes::EOI &&
                           lexer.peekToken(i).getType() != TokenTypes::LeftBrace) {
                        i++;
                    }
                } else {
                    while (lexer.peekToken(i).getType() == TokenTypes::Multiply ||
                           lexer.peekToken(i).getType() == TokenTypes::BitwiseAnd ||
                           lexer.peekToken(i).getType() == TokenTypes::QuestionMark) {
                        i++;
                    }
                    
                    if (lexer.peekToken(i).getType() == TokenTypes::Identifier) {
                        i++;
                        while (lexer.peekToken(i).getType() == TokenTypes::Dot) {
                            i++; // consume '.'
                            if (lexer.peekToken(i).getType() == TokenTypes::Identifier) {
                                i++; // consume identifier
                            } else {
                                std::string suggestion = "To resolve this:\n"
                                                       "1. Provide a valid identifier after '.'\n"
                                                       "2. Check dotted type syntax (e.g., std.vector)\n"
                                                       "3. Ensure type is defined";
                                console.reportError(
                                    Console::SYNTAX_ERROR,
                                    Console::formatString("Expected identifier after '.', got '%s'",
                                                     getTokenTypeName(lexer.peekToken(i).getType()).c_str()),
                                    suggestion
                                );
                                return false;
                            }
                        }
                    } else {
                        std::string suggestion = "To resolve this:\n"
                                               "1. Provide a valid return type identifier\n"
                                               "2. Check return type syntax\n"
                                               "3. Ensure type is defined";
                        console.reportError(
                            Console::SYNTAX_ERROR,
                            Console::formatString("Expected return type identifier, got '%s'",
                                             getTokenTypeName(lexer.peekToken(i).getType()).c_str()),
                            suggestion
                        );
                        return false;
                    }
                    
                    while (lexer.peekToken(i).getType() == TokenTypes::Multiply ||
                           lexer.peekToken(i).getType() == TokenTypes::BitwiseAnd ||
                           lexer.peekToken(i).getType() == TokenTypes::QuestionMark) {
                        i++;
                    }
                    
                    while (lexer.peekToken(i).getType() == TokenTypes::LeftBracket) {
                        i++; // consume '['
                        if (lexer.peekToken(i).getType() == TokenTypes::IntegerLiteral ||
                            lexer.peekToken(i).getType() == TokenTypes::Identifier) {
                            i++; // consume size
                        } else {
                            std::string suggestion = "To resolve this:\n"
                                                   "1. Provide a valid array size (integer or identifier)\n"
                                                   "2. Check array syntax\n"
                                                   "3. Ensure correct bracket usage";
                            console.reportError(
                                Console::SYNTAX_ERROR,
                                Console::formatString("Expected integer or identifier for array size, got '%s'",
                                                 getTokenTypeName(lexer.peekToken(i).getType()).c_str()),
                                suggestion
                            );
                            return false;
                        }
                        if (lexer.peekToken(i).getType() == TokenTypes::RightBracket) {
                            i++; // consume ']'
                        } else {
                            std::string suggestion = "To resolve this:\n"
                                                   "1. Close array type with ']'\n"
                                                   "2. Check for matching brackets\n"
                                                   "3. Verify array syntax";
                            console.reportError(
                                Console::SYNTAX_ERROR,
                                Console::formatString("Expected ']' to close array type, got '%s'",
                                                 getTokenTypeName(lexer.peekToken(i).getType()).c_str()),
                                suggestion
                            );
                            return false;
                        }
                    }
                }
            }
            
            if (lexer.peekToken(i).getType() == TokenTypes::Arrow) {
                return true;
            }
        } else {
            std::string suggestion = "To resolve this:\n"
                                   "1. Close parameter list with ')'\n"
                                   "2. Check for matching parentheses\n"
                                   "3. Verify parameter syntax";
            console.reportError(
                Console::SYNTAX_ERROR,
                Console::formatString("Expected ')', got '%s'",
                                 getTokenTypeName(lexer.peekToken(i).getType()).c_str()),
                suggestion
            );
            return false;
        }
    }
    
    return false;
}

std::shared_ptr<Statement> Parser::parseLambdaFunction(
    const std::string& lambdaName,
    parameterType paramTypes,
    std::shared_ptr<Type> type
) {
    static int anonCounter = 0;
    if (lambdaName.empty()) {
        std::string name = "lambda_" + std::to_string(anonCounter++);
        return parseFunctionDeclaration(name, paramTypes, type);
    }
    return parseFunctionDeclaration(lambdaName, paramTypes, type);
}

} // namespace Omniscript
