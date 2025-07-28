#include <omniscript/Statement.h>
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
#include <omniscript/omniscript_pch.h>


std::shared_ptr<Statement> Parser::parseExternFunction() {
    Token startToken = currentToken;
    eat(TokenTypes::Extern);

    FunctionDeclaration::LibraryPaths libraryPaths;
    std::vector<std::string> paths;

    // Parse first path
    paths.push_back(currentToken.getValue());
    eat(TokenTypes::StringLiteral);

    // Parse additional paths
    while (currentToken.getType() == TokenTypes::Comma) {
        eat(TokenTypes::Comma);
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
        else libType = "extensionless"; // Accept "kernel32", "glfw", etc.

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
        console.error("No valid library paths found in extern declaration");
        return nullptr;
    }

    if (currentToken.getType() == TokenTypes::Function) {
        eat(TokenTypes::Function);
        std::string functionName = currentToken.getValue();
        eat(TokenTypes::Identifier);

        auto function = std::dynamic_pointer_cast<FunctionDeclaration>(
            parseLambdaFunction(functionName));
        
        function->isExtern = true;
        function->libraryPaths = libraryPaths;
        function->externName = functionName;
        function->setPosition(startToken, previousToken);

        return function;

    } else if (currentToken.getType() == TokenTypes::Let || currentToken.getType() == TokenTypes::Const) {
        auto assignment = std::dynamic_pointer_cast<AssignVariable>(parseAssignment());
        if (!assignment) {
            console.error("Invalid assignment after let / const in external declaration");
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

        return assignment;
    } else if (currentToken.getType() == TokenTypes::LeftBrace) {
        eat(TokenTypes::LeftBrace);
        std::vector<std::shared_ptr<Statement>> functions;

        while (currentToken.getType() != TokenTypes::RightBrace) {
            if (currentToken.getType() == TokenTypes::Function) {
                eat(TokenTypes::Function);
                std::string functionName = currentToken.getValue();
                eat(TokenTypes::Identifier);
    
                auto function = std::dynamic_pointer_cast<FunctionDeclaration>(
                    parseLambdaFunction(functionName));
                
                function->isExtern = true;
                function->libraryPaths = libraryPaths;
                function->externName = functionName;
    
                functions.push_back(function);
                
            } else if (currentToken.getType() == TokenTypes::Let || currentToken.getType() == TokenTypes::Const) {
                auto assignment = std::dynamic_pointer_cast<AssignVariable>(parseAssignment());
                if (!assignment) {
                    console.error("Invalid assignment after let / const in external declaration");
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

                functions.push_back(assignment);
            }

            if (currentToken.getType() == TokenTypes::Semicolon) {
                eat(TokenTypes::Semicolon);
            }
        }

        eat(TokenTypes::RightBrace);
        auto block = std::make_shared<BlockStatement>(functions);
        block->setPosition(startToken, previousToken);
        return block;
    }

    console.error("Expected function declaration or block after extern library paths");
    return nullptr;
}

std::shared_ptr<Statement> Parser::parseIntrinsicFunction() {
    Token startToken = currentToken;
    eat(TokenTypes::Intrinsic);
    if (currentToken.getType() == TokenTypes::Function) {
        eat(TokenTypes::Function);
    }
    std::string functionName = currentToken.getValue();
    eat(TokenTypes::Identifier);
    std::shared_ptr<FunctionDeclaration> function = std::dynamic_pointer_cast<FunctionDeclaration>(parseLambdaFunction(functionName));
    function->isIntrinsic = true;
    function->intrinsicName = functionName;
    function->setPosition(startToken, previousToken);

    return function;
}

std::shared_ptr<Statement> Parser::parseFunctionDeclaration(
    parameterType paramTypes,
    std::shared_ptr<Omniscript::Type> type
) {
    return parseFunctionDeclaration("", paramTypes, type);
}

std::shared_ptr<Statement> Parser::parseFunctionDeclaration(
    const std::string& definedName,
    parameterType paramTypes,
    std::shared_ptr<Omniscript::Type> type
) {
    Token startToken = currentToken;

    std::string name = definedName;
    
    if (name.empty()) {
        eat(TokenTypes::Function);
        name = currentToken.getValue();
        eat(TokenTypes::Identifier);
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
        param->setType(Omniscript::Type::createPointerType(type));
        parameters.insert(parameters.begin(), std::dynamic_pointer_cast<Statement>(param));
    }

    std::shared_ptr<Omniscript::Type> returnType = nullptr;
    std::vector<std::string> returnDataType;
    if (currentToken.getType() != TokenTypes::LeftBrace) {
        eat(TokenTypes::Arrow);
        returnDataType = parseType();
        returnType = Omniscript::resolveType(returnDataType);
    } else {
        returnType = Omniscript::resolveType({"void"});
    }

    std::shared_ptr<BlockStatement> body;

    if (currentToken.getType() == TokenTypes::LeftBrace) {
        body = std::dynamic_pointer_cast<BlockStatement>(parseBlock());
    } else {
        body = BlockStatement::create();
    }

    if (!types.empty()) {
        std::vector<std::shared_ptr<Statement>> monomorphizedFunctions;

        // Special case: Single type parameter with simple alternatives (like i8 | i32)
        if (types.size() == 1 && !types[0].second.empty()) {
            const auto& typeParam = types[0];
            const auto& constraints = typeParam.second;

            // Check if all constraints are simple types (not variant/any)
            bool allSimple = std::all_of(constraints.begin(), constraints.end(),
                [](const std::vector<std::string>& c) {
                    return c.size() == 1 && c[0] != "any" && c[0] != "variant";
                });

            if (allSimple) {
                // Generate one function for each constraint
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
                    func->bindGeneric(typeParam.first, Omniscript::resolveType(constraint));
                    
                    monomorphizedFunctions.push_back(func);
                }
                auto block = std::make_shared<BlockStatement>(monomorphizedFunctions);
                block->setPosition(startToken, previousToken);
                return block;
            }
        }

        // General case: Use cartesian product for multiple type parameters or complex constraints
        std::vector<size_t> indices(types.size(), 0);
        std::vector<size_t> sizes;
        for (const auto& type : types) {
            sizes.push_back(type.second.empty() ? 1 : type.second.size());
        }

        bool done = false;
        while (!done) {
            // Generate one combination
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
                    func->bindGeneric(selected.first, Omniscript::resolveType(selected.second));
                }
            }

            monomorphizedFunctions.push_back(func);

            // Increment the index vector
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
        block->setPosition(startToken, previousToken);
        return block;
    }

    // Normal function without generics
    returnType = Omniscript::resolveType(returnDataType);
    auto function = std::make_shared<FunctionDeclaration>(name, parameters, body, returnType);
    function->setPosition(startToken, previousToken);
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
                                
                                // Skip over parameter type (simplified - would need recursive parsing for complex types)
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
                                        }
                                        if (lexer.peekToken(i).getType() == TokenTypes::RightBracket) {
                                            i++; // consume ']'
                                        } else {
                                            return false; // Invalid array syntax
                                        }
                                    } else {
                                        i++;
                                    }
                                }
                                
                                // Handle comma-separated function parameters
                                if (lexer.peekToken(i).getType() == TokenTypes::Comma) {
                                    i++; // consume ','
                                } else if (lexer.peekToken(i).getType() != TokenTypes::RightParen) {
                                    return false; // Expected comma or closing paren
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
                                
                                // Skip over return type (simplified)
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
                                        }
                                        if (lexer.peekToken(i).getType() == TokenTypes::RightBracket) {
                                            i++; // consume ']'
                                        } else {
                                            return false; // Invalid array syntax
                                        }
                                    } else {
                                        i++;
                                    }
                                }
                            }
                        }
                    } else {
                        // Parse regular types (non-function types)
                        
                        // Parse prefix modifiers (*, &, ?)
                        while (lexer.peekToken(i).getType() == TokenTypes::Multiply ||
                               lexer.peekToken(i).getType() == TokenTypes::BitwiseAnd ||
                               lexer.peekToken(i).getType() == TokenTypes::QuestionMark) {
                            i++;
                        }
                        
                        // Parse the main identifier (required for basic types)
                        if (lexer.peekToken(i).getType() == TokenTypes::Identifier) {
                            i++;
                            
                            // Handle dotted identifiers (e.g., std.vector)
                            while (lexer.peekToken(i).getType() == TokenTypes::Dot) {
                                i++; // consume '.'
                                if (lexer.peekToken(i).getType() == TokenTypes::Identifier) {
                                    i++; // consume identifier
                                } else {
                                    return false; // Invalid dotted identifier
                                }
                            }
                        } else {
                            return false; // Expected type identifier
                        }
                        
                        // Parse suffix modifiers (*, &, ?)
                        while (lexer.peekToken(i).getType() == TokenTypes::Multiply ||
                               lexer.peekToken(i).getType() == TokenTypes::BitwiseAnd ||
                               lexer.peekToken(i).getType() == TokenTypes::QuestionMark) {
                            i++;
                        }
                        
                        // Parse array brackets
                        while (lexer.peekToken(i).getType() == TokenTypes::LeftBracket) {
                            i++; // consume '['
                            
                            if (lexer.peekToken(i).getType() == TokenTypes::IntegerLiteral ||
                                lexer.peekToken(i).getType() == TokenTypes::Identifier) {
                                i++; // consume size
                            } else {
                                return false; // Invalid array syntax
                            }
                            
                            if (lexer.peekToken(i).getType() == TokenTypes::RightBracket) {
                                i++; // consume ']'
                            } else {
                                return false; // Missing closing bracket
                            }
                        }
                    }
                }
                
                // Check for default value
                if (lexer.peekToken(i).getType() == TokenTypes::Assign) {
                    i++; // consume '='
                    
                    // Parse default value (simplified - could be more complex expressions)
                    if (lexer.peekToken(i).getType() == TokenTypes::IntegerLiteral ||
                        lexer.peekToken(i).getType() == TokenTypes::FloatLiteral ||
                        lexer.peekToken(i).getType() == TokenTypes::StringLiteral ||
                        lexer.peekToken(i).getType() == TokenTypes::Identifier) {
                        i++;
                    } else {
                        return false; // Invalid default value
                    }
                }
                
                // Handle comma-separated parameters
                if (lexer.peekToken(i).getType() == TokenTypes::Comma) {
                    i++; // consume ','
                } else if (lexer.peekToken(i).getType() != TokenTypes::RightParen) {
                    return false; // Expected comma or closing paren
                }
            } else {
                return false; // Expected parameter name
            }
        }
        
        // Check for closing paren and arrow
        if (lexer.peekToken(i).getType() == TokenTypes::RightParen) {
            i++; // consume ')'
            
            // Check for optional return type annotation
            if (lexer.peekToken(i).getType() == TokenTypes::Colon) {
                i++; // consume ':'
                
                // Parse return type (same logic as parameter types)
                if (lexer.peekToken(i).getType() == TokenTypes::Function) {
                    // Handle function return type (simplified)
                    while (lexer.peekToken(i).getType() != TokenTypes::Arrow &&
                           lexer.peekToken(i).getType() != TokenTypes::EOI &&
                           lexer.peekToken(i).getType() != TokenTypes::LeftBrace) {
                        i++;
                    }
                } else {
                    // Parse regular return type
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
                                return false; // Invalid dotted identifier
                            }
                        }
                    } else {
                        return false; // Expected return type identifier
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
                            return false; // Invalid array syntax
                        }
                        if (lexer.peekToken(i).getType() == TokenTypes::RightBracket) {
                            i++; // consume ']'
                        } else {
                            return false; // Missing closing bracket
                        }
                    }
                }
            }
            
            // Check for arrow operator
            if (lexer.peekToken(i).getType() == TokenTypes::Arrow) {
                return true;
            }
        }
    }
    
    return false;
}

std::shared_ptr<Statement> Parser::parseLambdaFunction(
    const std::string& lambdaName,
    parameterType paramTypes,
    std::shared_ptr<Omniscript::Type> type
) {
    static int anonCounter = 0;
    if (lambdaName.empty()) {
        std::string name = "lambda_" + std::to_string(anonCounter++);
        return parseFunctionDeclaration(name, paramTypes, type);
    }
    return parseFunctionDeclaration(lambdaName, paramTypes, type);
}
