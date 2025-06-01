#include <omniscript/Core.h>
#include <omniscript/utils.h>
#include <omniscript/runtime/object.h>
#include <omniscript/engine/parser.h>
#include <omniscript/engine/lexer.h>
#include <omniscript/engine/tokens.h>
#include <omniscript/engine/Statement.h>
#include <omniscript/engine/Symboltable.h>
#include <omniscript/omniscript_pch.h>

std::shared_ptr<Statement> Parser::parseInclude() {
    eat(TokenTypes::Include);

    std::string includePath;

    if (currentToken.getType() != TokenTypes::StringLiteral) {
        console.error("Syntax Error: Expected string literal after 'include'");
    }

    includePath = currentToken.getValue();
    eat(TokenTypes::StringLiteral);

    eat(TokenTypes::Semicolon);

    return std::make_shared<IncludeStatement>(includePath);
}


std::shared_ptr<Statement> Parser::parseModuleImport() {
    eat(TokenTypes::Import);

    std::unordered_map<std::string, std::string> importedAliases;
    bool importAll = false;
    std::string moduleName;
    std::string alias;
    std::string path; // Path of the module (if from a file)

    // Handle selective import: `import { console } from "std";`
    if (currentToken.getType() == TokenTypes::LeftBrace) {
        eat(TokenTypes::LeftBrace);
        while (currentToken.getType() == TokenTypes::Identifier) {
            std::string originalName = currentToken.getValue();
            eat(TokenTypes::Identifier);

            std::string aliasName = originalName; // Default alias is the same as the original

            // Handle `import { foreign as test }`
            if (currentToken.getType() == TokenTypes::As) {
                eat(TokenTypes::As);
                if (currentToken.getType() == TokenTypes::Identifier) {
                    aliasName = currentToken.getValue();
                    eat(TokenTypes::Identifier);
                } else {
                    throw std::runtime_error("Syntax Error: Expected alias name after 'as'");
                }
            }

            importedAliases[aliasName] = originalName;

            if (currentToken.getType() == TokenTypes::Comma) {
                eat(TokenTypes::Comma);
            } else {
                break;
            }
        }
        eat(TokenTypes::RightBrace);
        eat(TokenTypes::From);

        // Expect module name (either an identifier or a string path)
        if (currentToken.getType() == TokenTypes::Identifier) {
            moduleName = currentToken.getValue();
            path = currentToken.getValue();
            eat(TokenTypes::Identifier);
        } else if (currentToken.getType() == TokenTypes::StringLiteral) {
            path = currentToken.getValue();
            eat(TokenTypes::StringLiteral);
        }
    }
    // Handle wildcard import: `import * from "test.os";`
    else if (currentToken.getType() == TokenTypes::Multiply) {
        eat(TokenTypes::Multiply);
        eat(TokenTypes::From);
        if (currentToken.getType() == TokenTypes::Identifier) {
            moduleName = currentToken.getValue();
            path = currentToken.getValue();
            eat(TokenTypes::Identifier);
        } else if (currentToken.getType() == TokenTypes::StringLiteral) {
            path = currentToken.getValue();
            eat(TokenTypes::StringLiteral);
        }
        importAll = true;
    }
    // Handle full module import: `import "test.os";` or `import std;`
    else if (currentToken.getType() == TokenTypes::Identifier) {
        moduleName = currentToken.getValue();
        path = currentToken.getValue();
        eat(TokenTypes::Identifier);
    } else if (currentToken.getType() == TokenTypes::StringLiteral) {
        path = currentToken.getValue();
        eat(TokenTypes::StringLiteral);
    }

    // Handle aliasing: `import { console } from "std" as c;`
    if (currentToken.getType() == TokenTypes::As) {
        eat(TokenTypes::As);
        if (currentToken.getType() == TokenTypes::Identifier) {
            alias = currentToken.getValue();
            eat(TokenTypes::Identifier);
        } else {
            throw std::runtime_error("Syntax Error: Expected alias name after 'as'");
        }
    }

    eat(TokenTypes::Semicolon);

    return std::make_shared<ImportModule>(moduleName, alias, importedAliases, path, importAll);
}

std::shared_ptr<Statement> Parser::parseModule() {
    std::string moduleName;
    std::vector<std::shared_ptr<Statement>> members;

    eat(TokenTypes::Module);
    moduleName = currentToken.getValue();
    eat(TokenTypes::Identifier);
    eat(TokenTypes::LeftBrace);

    while (currentToken.getType() != TokenTypes::RightBrace) {
        if (currentToken.getType() == TokenTypes::Include) {
            eat(TokenTypes::Include);
            std::string filePath = currentToken.getValue();
            eat(TokenTypes::StringLiteral);
            auto includeStatement = std::make_shared<IncludeStatement>(filePath);
            members.push_back(includeStatement);
            expectSemicolonOrNewLine();
            continue;
        }

        MemberModifiers modifiers = parseMemberModifiers();

        // Handle nested module import assignment
        if (currentToken.getType() == TokenTypes::Module) {
            eat(TokenTypes::Module);
            std::string moduleAlias = currentToken.getValue();
            eat(TokenTypes::Identifier);

            eat(TokenTypes::Assign);
            eat(TokenTypes::Import);
            std::string modulePath = currentToken.getValue();
            eat(TokenTypes::StringLiteral);
            eat(TokenTypes::Semicolon);

            std::string sourceCode = readFile(modulePath);
            if (sourceCode.empty()) {
                console.error("Failed to read module file: " + modulePath);
                return nullptr;
            }

            Lexer lexer(sourceCode);
            Parser parser(lexer);
            std::vector<std::shared_ptr<Statement>> moduleStatements = parser.Parse();

            auto importStmt = std::make_shared<ImportModule>(
                /* moduleName */ moduleAlias,
                /* alias */ moduleAlias,
                /* importedAliases */ std::unordered_map<std::string, std::string>{},  // you can extend this for `import { x as y } from ...` later
                /* path */ modulePath,
                /* importAll */ true       // simple wildcard import for now
            );

            auto wrapped = std::make_shared<ModuleMember>(moduleAlias, importStmt, modifiers);


            members.push_back(wrapped);
            continue;
        }

        // Handle regular members (functions, variables, etc.)
        bool expectLambda = false;
        if (currentToken.getType() == TokenTypes::Function) {
            eat(TokenTypes::Function);
            expectLambda = true;
        }

        std::shared_ptr<Statement> member; 
        std::string memberName;
        if (expectLambda && !checkIfLambdaExpression()) {
            console.error("The function/fn keyword shows that the member we are expecting should be a function but it is not.");
        } else if (checkIfLambdaExpression()) {
            memberName = currentToken.getValue();
            eat(TokenTypes::Identifier);
            member = parseLambdaFunction(memberName);
        } else {
            member = parseStatement();
        }
        
        if (auto named = std::dynamic_pointer_cast<NamedStatement>(member)) {
            memberName = named->getName();
        } else {
            console.error("Cannot determine name of member in module: " + moduleName);
            continue;
        }

        auto wrapped = std::make_shared<ModuleMember>(memberName, member, modifiers);
        members.push_back(wrapped);
    }

    eat(TokenTypes::RightBrace);

    eat(TokenTypes::EOI, "There can only be one module per file and nothing declared after the module.");
    auto module = std::make_shared<CreateModule>(moduleName, members);
    if (auto ctxAware = std::dynamic_pointer_cast<ContextAwareStatement>(module)) {
        ctxAware->pushContext(moduleName);
    }
    return module;
}
