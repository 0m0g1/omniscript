<<<<<<< HEAD:src/Parser/Functions.cpp
#include <omniscript/Statement.h>
#include <omniscript/Statements/AccessStatements.h>
#include <omniscript/Statements/FunctionStatement.h>
#include <omniscript/Statements/CallableStatement.h>
#include <omniscript/Statements/LiteralStatements.h>
#include <omniscript/Statements/AssignmentAndGetterStatements.h>

#include <omniscript/Core.h>
=======
#include <omniscript/engine/Statement.h>
#include <omniscript/engine/Statements/AccessStatements.h>
#include <omniscript/engine/Statements/FunctionStatement.h>
#include <omniscript/engine/Statements/CallableStatement.h>
#include <omniscript/engine/Statements/LiteralStatements.h>
#include <omniscript/engine/Statements/AssignmentAndGetterStatements.h>

#include <omniscript/engine/Core.h>
>>>>>>> 7ccebff50dd27e70cffd4d578dcb358f4c9e1613:src/engine/Parser/Functions.cpp
#include <omniscript/utils.h>
#include <omniscript/Lexer.h>
#include <omniscript/Tokens.h>
#include <omniscript/Parser.h>
#include <omniscript/runtime/object.h>
#include <omniscript/omniscript_pch.h>
<<<<<<< HEAD:src/Parser/Functions.cpp
#include <omniscript/Symboltable.h>

=======
#include <omniscript/engine/Symboltable.h>
>>>>>>> 7ccebff50dd27e70cffd4d578dcb358f4c9e1613:src/engine/Parser/Functions.cpp


std::shared_ptr<Statement> Parser::parseExternFunction() {
    Token startToken = currentToken;
    eat(TokenTypes::Extern);

    // Parse library paths (dynamic first, then static - order doesn't matter)
    std::string dynamicLibPath;
    std::string staticLibPath;
    
    std::string firstPath = currentToken.getValue();
    eat(TokenTypes::StringLiteral);
    
    if (currentToken.getType() == TokenTypes::Comma) {
        eat(TokenTypes::Comma);
        std::string secondPath = currentToken.getValue();
        eat(TokenTypes::StringLiteral);
        
        // Classify paths by extension
        auto classify = [](const std::string& path) {
            if (path.ends_with(".a") || path.ends_with(".lib")) return "static";
            if (path.ends_with(".dll") || path.ends_with(".so") || path.ends_with(".dylib")) return "dynamic";
            return "unknown";
        };
        
        std::string firstType = classify(firstPath);
        std::string secondType = classify(secondPath);
        
        if (firstType == "dynamic" && secondType == "static") {
            dynamicLibPath = firstPath;
            staticLibPath = secondPath;
        }
        else if (firstType == "static" && secondType == "dynamic") {
            dynamicLibPath = secondPath;
            staticLibPath = firstPath;
        }
        else {
            console.error("Extern declaration requires one dynamic and one static library path");
        }
    
    } else {
        if (firstPath.ends_with(".a") || firstPath.ends_with(".lib")) {
            staticLibPath = firstPath;
        }
        else {
            dynamicLibPath = firstPath;
        }
    }

    if (currentToken.getType() == TokenTypes::Function) {
        eat(TokenTypes::Function);
        std::string functionName = currentToken.getValue();
        eat(TokenTypes::Identifier);
        
        auto function = std::dynamic_pointer_cast<FunctionDeclaration>(
            parseLambdaFunction(functionName));
        
        function->isExtern = true;
        function->dynamicLibPath = dynamicLibPath;
        function->staticLibPath = staticLibPath;
        function->externName = functionName;
        function->setPosition(startToken);

        return function;
    
    } else if (currentToken.getType() == TokenTypes::LeftBrace) {
        eat(TokenTypes::LeftBrace);
        std::vector<std::shared_ptr<Statement>> functions;
        
        while (currentToken.getType() != TokenTypes::RightBrace) {
            if (currentToken.getType() == TokenTypes::Function) {
                eat(TokenTypes::Function);
            }
            
            std::string functionName = currentToken.getValue();
            eat(TokenTypes::Identifier);
            
            auto function = std::dynamic_pointer_cast<FunctionDeclaration>(
                parseLambdaFunction(functionName));
            
            function->isExtern = true;
            function->dynamicLibPath = dynamicLibPath;
            function->staticLibPath = staticLibPath;
            function->externName = functionName;
            
            functions.push_back(function);
            if (currentToken.getType() == TokenTypes::Semicolon) {
                eat(TokenTypes::Semicolon);
            }
        }
        
        eat(TokenTypes::RightBrace);
        auto block = std::make_shared<BlockStatement>(functions);
        block->setPosition(startToken);
        return block;
    }

    console.error("Expected function declaration or block after extern library path");
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
    function->setPosition(startToken);

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
                block->setPosition(startToken);
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
        block->setPosition(startToken);
        return block;
    }

    // Normal function without generics
    returnType = Omniscript::resolveType(returnDataType);
    auto function = std::make_shared<FunctionDeclaration>(name, parameters, body, returnType);
    function->setPosition(startToken);
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
    if (currentToken.getType() == TokenTypes::Identifier) {
        i++;
    }
    
    DEBUG_LOG(getTokenTypeName(lexer.peekToken(i).getType()));
    if ((i == 0 ? currentToken.getType() : lexer.peekToken(i).getType()) == TokenTypes::LeftParen) {
        i++;
        bool hasValidArgument = false;
        
        while (
            lexer.peekToken(i).getType() == TokenTypes::Identifier ||
            lexer.peekToken(i).getType() == TokenTypes::Comma || 
            lexer.peekToken(i).getType() == TokenTypes::Assign ||
            lexer.peekToken(i).getType() == TokenTypes::Colon
            ) {

            DEBUG_LOG(getTokenTypeName(lexer.peekToken(i).getType()));
            
            if (lexer.peekToken(i).getType() == TokenTypes::Identifier) {
                hasValidArgument = true;
                i++;
            }
            
            if (lexer.peekToken(i).getType() == TokenTypes::Colon) {
                i++;
                if (lexer.peekToken(i).getType() == TokenTypes::Identifier) {
                    i++;
                }
            }
            
            if (lexer.peekToken(i).getType() == TokenTypes::Assign) {
                i++;
                if (lexer.peekToken(i).getType() == TokenTypes::IntegerLiteral ||
                    lexer.peekToken(i).getType() == TokenTypes::FloatLiteral ||
                    lexer.peekToken(i).getType() == TokenTypes::StringLiteral) {
                    i++;
                }
            }
            
            if (lexer.peekToken(i).getType() == TokenTypes::Comma) {
                i++;
            }
        }

        if (lexer.peekToken(i).getType() == TokenTypes::RightParen && lexer.peekToken(i + 1).getType() == TokenTypes::Arrow) {
            return true;
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
