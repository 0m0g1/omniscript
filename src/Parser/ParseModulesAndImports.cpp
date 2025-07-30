#include <omniscript/omniscript_pch.h>
#include <omniscript/Statement.h>
#include <omniscript/Statements/ModuleAndImportStatements.h>
#include <omniscript/Statements/AssignmentAndGetterStatements.h>
#include <omniscript/Core.h>
#include <omniscript/utils.h>
#include <omniscript/Lexer.h>
#include <omniscript/Parser.h>
#include <omniscript/Tokens.h>
#include <omniscript/Symboltable.h>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

std::shared_ptr<Statement> Parser::parseInclude() {
    Token startToken = currentToken;
    FileSpan span;
    span.start.line = startToken.getLine();
    span.start.col = startToken.getColumn();
    span.start.filePath = startToken.getFilePath();

    eat(TokenTypes::Include, [&]() {
        std::string suggestion = Console::formatString(
            "To resolve this:\n"
            "1. Start include statement with 'include' keyword\n"
            "2. Check include syntax\n"
            "3. Expected token: 'include', found '%s'",
            getTokenTypeName(currentToken.getType()).c_str()
        );
        console.reportError(
            Console::SYNTAX_ERROR,
            Console::formatString("Expected 'include' keyword, found '%s'", 
                getTokenTypeName(currentToken.getType()).c_str()),
            suggestion,
            span
        );
    });

    std::string includePath;
    if (currentToken.getType() != TokenTypes::StringLiteral) {
        console.reportError(
            Console::SYNTAX_ERROR,
            Console::formatString("Expected string literal after 'include', found '%s'", 
                getTokenTypeName(currentToken.getType()).c_str()),
            "To resolve this:\n1. Provide a valid file path as a string literal\n2. Check include syntax\n3. Ensure path is enclosed in quotes",
            span
        );
        return nullptr;
    }

    includePath = currentToken.getValue();
    eat(TokenTypes::StringLiteral);

    eat(TokenTypes::Semicolon, [&]() {
        std::string suggestion = Console::formatString(
            "To resolve this:\n"
            "1. End include statement with ';'\n"
            "2. Check for proper termination\n"
            "3. Expected token: ';', found '%s'",
            getTokenTypeName(currentToken.getType()).c_str()
        );
        console.reportError(
            Console::SYNTAX_ERROR,
            Console::formatString("Expected ';' after include path, found '%s'", 
                getTokenTypeName(currentToken.getType()).c_str()),
            suggestion,
            span
        );
    });

    span.end.line = previousToken.getLine();
    span.end.col = previousToken.getColumn();
    span.end.filePath = previousToken.getFilePath();

    return std::make_shared<IncludeStatement>(includePath);
}

std::shared_ptr<Statement> Parser::parseModuleImport() {
    Token startToken = currentToken;
    FileSpan span;
    span.start.line = startToken.getLine();
    span.start.col = startToken.getColumn();
    span.start.filePath = startToken.getFilePath();

    eat(TokenTypes::Import, [&]() {
        std::string suggestion = Console::formatString(
            "To resolve this:\n"
            "1. Start import statement with 'import' keyword\n"
            "2. Check import syntax\n"
            "3. Expected token: 'import', found '%s'",
            getTokenTypeName(currentToken.getType()).c_str()
        );
        console.reportError(
            Console::SYNTAX_ERROR,
            Console::formatString("Expected 'import' keyword, found '%s'", 
                getTokenTypeName(currentToken.getType()).c_str()),
            suggestion,
            span
        );
    });

    std::unordered_map<std::string, std::string> importedAliases;
    bool importAll = false;
    std::string moduleName;
    std::string alias;
    std::string path;

    // Handle selective import: `import { console } from "std";`
    if (currentToken.getType() == TokenTypes::LeftBrace) {
        eat(TokenTypes::LeftBrace, [&]() {
            std::string suggestion = Console::formatString(
                "To resolve this:\n"
                "1. Start selective import with '{'\n"
                "2. Check import syntax\n"
                "3. Expected token: '{', found '%s'",
                getTokenTypeName(currentToken.getType()).c_str()
            );
            console.reportError(
                Console::SYNTAX_ERROR,
                Console::formatString("Expected '{' for selective import, found '%s'", 
                    getTokenTypeName(currentToken.getType()).c_str()),
                suggestion,
                span
            );
        });
        while (currentToken.getType() == TokenTypes::Identifier) {
            std::string originalName = currentToken.getValue();
            eat(TokenTypes::Identifier, [&]() {
                std::string suggestion = Console::formatString(
                    "To resolve this:\n"
                    "1. Provide a valid identifier for import\n"
                    "2. Check selective import syntax\n"
                    "3. Expected token: identifier, found '%s'",
                    getTokenTypeName(currentToken.getType()).c_str()
                );
                console.reportError(
                    Console::SYNTAX_ERROR,
                    Console::formatString("Expected identifier for import, found '%s'", 
                        getTokenTypeName(currentToken.getType()).c_str()),
                    suggestion,
                    span
                );
            });

            std::string aliasName = originalName;
            if (currentToken.getType() == TokenTypes::As) {
                eat(TokenTypes::As, [&]() {
                    std::string suggestion = Console::formatString(
                        "To resolve this:\n"
                        "1. Use 'as' for aliasing\n"
                        "2. Check alias syntax\n"
                        "3. Expected token: 'as', found '%s'",
                        getTokenTypeName(currentToken.getType()).c_str()
                    );
                    console.reportError(
                        Console::SYNTAX_ERROR,
                        Console::formatString("Expected 'as' for alias, found '%s'", 
                            getTokenTypeName(currentToken.getType()).c_str()),
                        suggestion,
                        span
                    );
                });
                if (currentToken.getType() == TokenTypes::Identifier) {
                    aliasName = currentToken.getValue();
                    eat(TokenTypes::Identifier);
                } else {
                    console.reportError(
                        Console::SYNTAX_ERROR,
                        Console::formatString("Expected alias name after 'as', found '%s'", 
                            getTokenTypeName(currentToken.getType()).c_str()),
                        "To resolve this:\n1. Provide a valid identifier for alias\n2. Check alias syntax\n3. Ensure valid alias name",
                        span
                    );
                    return nullptr;
                }
            }

            importedAliases[aliasName] = originalName;

            if (currentToken.getType() == TokenTypes::Comma) {
                eat(TokenTypes::Comma);
            } else {
                break;
            }
        }
        eat(TokenTypes::RightBrace, [&]() {
            std::string suggestion = Console::formatString(
                "To resolve this:\n"
                "1. Close selective import with '}'\n"
                "2. Check for matching braces\n"
                "3. Expected token: '}', found '%s'",
                getTokenTypeName(currentToken.getType()).c_str()
            );
            console.reportError(
                Console::SYNTAX_ERROR,
                Console::formatString("Expected '}' to close selective import, found '%s'", 
                    getTokenTypeName(currentToken.getType()).c_str()),
                suggestion,
                span
            );
        });
        eat(TokenTypes::From, [&]() {
            std::string suggestion = Console::formatString(
                "To resolve this:\n"
                "1. Use 'from' after selective import\n"
                "2. Check import syntax\n"
                "3. Expected token: 'from', found '%s'",
                getTokenTypeName(currentToken.getType()).c_str()
            );
            console.reportError(
                Console::SYNTAX_ERROR,
                Console::formatString("Expected 'from' after selective import, found '%s'", 
                    getTokenTypeName(currentToken.getType()).c_str()),
                suggestion,
                span
            );
        });

        if (currentToken.getType() == TokenTypes::Identifier) {
            moduleName = currentToken.getValue();
            path = currentToken.getValue();
            eat(TokenTypes::Identifier);
        } else if (currentToken.getType() == TokenTypes::StringLiteral) {
            path = currentToken.getValue();
            eat(TokenTypes::StringLiteral);
        } else {
            console.reportError(
                Console::SYNTAX_ERROR,
                Console::formatString("Expected identifier or string literal for module path, found '%s'", 
                    getTokenTypeName(currentToken.getType()).c_str()),
                "To resolve this:\n1. Provide a valid module name or path\n2. Check import syntax\n3. Ensure valid identifier or string literal",
                span
            );
            return nullptr;
        }
    }
    // Handle wildcard import: `import * from "test.os";`
    else if (currentToken.getType() == TokenTypes::Multiply) {
        eat(TokenTypes::Multiply);
        eat(TokenTypes::From, [&]() {
            std::string suggestion = Console::formatString(
                "To resolve this:\n"
                "1. Use 'from' after wildcard import\n"
                "2. Check import syntax\n"
                "3. Expected token: 'from', found '%s'",
                getTokenTypeName(currentToken.getType()).c_str()
            );
            console.reportError(
                Console::SYNTAX_ERROR,
                Console::formatString("Expected 'from' after wildcard import, found '%s'", 
                    getTokenTypeName(currentToken.getType()).c_str()),
                suggestion,
                span
            );
        });
        if (currentToken.getType() == TokenTypes::Identifier) {
            moduleName = currentToken.getValue();
            path = currentToken.getValue();
            eat(TokenTypes::Identifier);
        } else if (currentToken.getType() == TokenTypes::StringLiteral) {
            path = currentToken.getValue();
            eat(TokenTypes::StringLiteral);
        } else {
            console.reportError(
                Console::SYNTAX_ERROR,
                Console::formatString("Expected identifier or string literal for module path, found '%s'", 
                    getTokenTypeName(currentToken.getType()).c_str()),
                "To resolve this:\n1. Provide a valid module name or path\n2. Check import syntax\n3. Ensure valid identifier or string literal",
                span
            );
            return nullptr;
        }
        importAll = true;
    }
    // Handle full module import: `import "test.os";` or `import std;`
    else if (currentToken.getType() == TokenTypes::Identifier || currentToken.getType() == TokenTypes::StringLiteral) {
        if (currentToken.getType() == TokenTypes::Identifier) {
            moduleName = currentToken.getValue();
            path = currentToken.getValue();
            eat(TokenTypes::Identifier);
        } else {
            path = currentToken.getValue();
            eat(TokenTypes::StringLiteral);
        }
    } else {
        console.reportError(
            Console::SYNTAX_ERROR,
            Console::formatString("Expected identifier or string literal for module path, found '%s'", 
                getTokenTypeName(currentToken.getType()).c_str()),
            "To resolve this:\n1. Provide a valid module name or path\n2. Check import syntax\n3. Ensure valid identifier or string literal",
            span
        );
        return nullptr;
    }

    if (currentToken.getType() == TokenTypes::As) {
        eat(TokenTypes::As, [&]() {
            std::string suggestion = Console::formatString(
                "To resolve this:\n"
                "1. Use 'as' for module alias\n"
                "2. Check import syntax\n"
                "3. Expected token: 'as', found '%s'",
                getTokenTypeName(currentToken.getType()).c_str()
            );
            console.reportError(
                Console::SYNTAX_ERROR,
                Console::formatString("Expected 'as' for module alias, found '%s'", 
                    getTokenTypeName(currentToken.getType()).c_str()),
                suggestion,
                span
            );
        });
        if (currentToken.getType() == TokenTypes::Identifier) {
            alias = currentToken.getValue();
            eat(TokenTypes::Identifier);
        } else {
            console.reportError(
                Console::SYNTAX_ERROR,
                Console::formatString("Expected identifier for module alias, found '%s'", 
                    getTokenTypeName(currentToken.getType()).c_str()),
                "To resolve this:\n1. Provide a valid identifier for alias\n2. Check alias syntax\n3. Ensure valid alias name",
                span
            );
            return nullptr;
        }
    }

    eat(TokenTypes::Semicolon, [&]() {
        std::string suggestion = Console::formatString(
            "To resolve this:\n"
            "1. End import statement with ';'\n"
            "2. Check for proper termination\n"
            "3. Expected token: ';', found '%s'",
            getTokenTypeName(currentToken.getType()).c_str()
        );
        console.reportError(
            Console::SYNTAX_ERROR,
            Console::formatString("Expected ';' after import, found '%s'", 
                getTokenTypeName(currentToken.getType()).c_str()),
            suggestion,
            span
        );
    });

    span.end.line = previousToken.getLine();
    span.end.col = previousToken.getColumn();
    span.end.filePath = previousToken.getFilePath();

    return std::make_shared<ImportModule>(moduleName, alias, importedAliases, path, importAll);
}

std::shared_ptr<Statement> Parser::parseModule() {
    Token startToken = currentToken;
    FileSpan span;
    span.start.line = startToken.getLine();
    span.start.col = startToken.getColumn();
    span.start.filePath = startToken.getFilePath();

    eat(TokenTypes::Module, [&]() {
        std::string suggestion = Console::formatString(
            "To resolve this:\n"
            "1. Start module declaration with 'module' keyword\n"
            "2. Check module syntax\n"
            "3. Expected token: 'module', found '%s'",
            getTokenTypeName(currentToken.getType()).c_str()
        );
        console.reportError(
            Console::SYNTAX_ERROR,
            Console::formatString("Expected 'module' keyword, found '%s'", 
                getTokenTypeName(currentToken.getType()).c_str()),
            suggestion,
            span
        );
    });

    std::string moduleName = currentToken.getValue();
    eat(TokenTypes::Identifier, [&]() {
        std::string suggestion = Console::formatString(
            "To resolve this:\n"
            "1. Provide a valid module name\n"
            "2. Check module syntax\n"
            "3. Expected token: identifier, found '%s'",
            getTokenTypeName(currentToken.getType()).c_str()
        );
        console.reportError(
            Console::SYNTAX_ERROR,
            Console::formatString("Expected identifier for module name, found '%s'", 
                getTokenTypeName(currentToken.getType()).c_str()),
            suggestion,
            span
        );
    });

    eat(TokenTypes::LeftBrace, [&]() {
        std::string suggestion = Console::formatString(
            "To resolve this:\n"
            "1. Start module body with '{'\n"
            "2. Check for matching braces\n"
            "3. Expected token: '{', found '%s'",
            getTokenTypeName(currentToken.getType()).c_str()
        );
        console.reportError(
            Console::SYNTAX_ERROR,
            Console::formatString("Expected '{' to start module body, found '%s'", 
                getTokenTypeName(currentToken.getType()).c_str()),
            suggestion,
            span
        );
    });

    std::vector<std::shared_ptr<Statement>> members;

    while (currentToken.getType() != TokenTypes::RightBrace) {
        Token memberStartToken = currentToken;

        if (currentToken.getType() == TokenTypes::Include) {
            eat(TokenTypes::Include);
            if (currentToken.getType() != TokenTypes::StringLiteral) {
                console.reportError(
                    Console::SYNTAX_ERROR,
                    Console::formatString("Expected string literal for include path, found '%s'", 
                        getTokenTypeName(currentToken.getType()).c_str()),
                    "To resolve this:\n1. Provide a valid file path as a string literal\n2. Check include syntax\n3. Ensure path is enclosed in quotes",
                    span
                );
                return nullptr;
            }
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
            eat(TokenTypes::Identifier, [&]() {
                std::string suggestion = Console::formatString(
                    "To resolve this:\n"
                    "1. Provide a valid module alias\n"
                    "2. Check nested module syntax\n"
                    "3. Expected token: identifier, found '%s'",
                    getTokenTypeName(currentToken.getType()).c_str()
                );
                console.reportError(
                    Console::SYNTAX_ERROR,
                    Console::formatString("Expected identifier for module alias, found '%s'", 
                        getTokenTypeName(currentToken.getType()).c_str()),
                    suggestion,
                    span
                );
            });

            eat(TokenTypes::Assign, [&]() {
                std::string suggestion = Console::formatString(
                    "To resolve this:\n"
                    "1. Use '=' for module import assignment\n"
                    "2. Check nested module syntax\n"
                    "3. Expected token: '=', found '%s'",
                    getTokenTypeName(currentToken.getType()).c_str()
                );
                console.reportError(
                    Console::SYNTAX_ERROR,
                    Console::formatString("Expected '=' for module import, found '%s'", 
                        getTokenTypeName(currentToken.getType()).c_str()),
                    suggestion,
                    span
                );
            });
            eat(TokenTypes::Import, [&]() {
                std::string suggestion = Console::formatString(
                    "To resolve this:\n"
                    "1. Use 'import' for module import\n"
                    "2. Check nested module syntax\n"
                    "3. Expected token: 'import', found '%s'",
                    getTokenTypeName(currentToken.getType()).c_str()
                );
                console.reportError(
                    Console::SYNTAX_ERROR,
                    Console::formatString("Expected 'import' for module import, found '%s'", 
                        getTokenTypeName(currentToken.getType()).c_str()),
                    suggestion,
                    span
                );
            });
            if (currentToken.getType() != TokenTypes::StringLiteral) {
                console.reportError(
                    Console::SYNTAX_ERROR,
                    Console::formatString("Expected string literal for module path, found '%s'", 
                        getTokenTypeName(currentToken.getType()).c_str()),
                    "To resolve this:\n1. Provide a valid module path as a string literal\n2. Check import syntax\n3. Ensure path is enclosed in quotes",
                    span
                );
                return nullptr;
            }
            std::string modulePath = currentToken.getValue();
            eat(TokenTypes::StringLiteral);
            eat(TokenTypes::Semicolon, [&]() {
                std::string suggestion = Console::formatString(
                    "To resolve this:\n"
                    "1. End module import with ';'\n"
                    "2. Check for proper termination\n"
                    "3. Expected token: ';', found '%s'",
                    getTokenTypeName(currentToken.getType()).c_str()
                );
                console.reportError(
                    Console::SYNTAX_ERROR,
                    Console::formatString("Expected ';' after module import, found '%s'", 
                        getTokenTypeName(currentToken.getType()).c_str()),
                    suggestion,
                    span
                );
            });

            std::string sourceCode = readFile(modulePath);
            if (sourceCode.empty()) {
                console.reportError(
                    Console::SYNTAX_ERROR,
                    "Failed to read module file: " + modulePath,
                    "To resolve this:\n1. Verify the module file path\n2. Ensure the file exists and is readable\n3. Check file permissions",
                    span
                );
                return nullptr;
            }

            Lexer lexer(sourceCode);
            Parser parser(lexer);
            std::vector<std::shared_ptr<Statement>> moduleStatements = parser.parse();
            if (moduleStatements.empty()) {
                console.reportError(
                    Console::SYNTAX_ERROR,
                    "Empty or invalid module file: " + modulePath,
                    "To resolve this:\n1. Ensure the module file contains valid statements\n2. Check module syntax\n3. Verify file content",
                    span
                );
                return nullptr;
            }

            auto importStmt = std::make_shared<ImportModule>(moduleAlias, moduleAlias, std::unordered_map<std::string, std::string>{}, modulePath, true);
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
            console.reportError(
                Console::SYNTAX_ERROR,
                "Expected a function declaration after 'fn' keyword",
                "To resolve this:\n1. Ensure a valid lambda function follows 'fn'\n2. Check function syntax\n3. Verify parameter list and body",
                span
            );
            return nullptr;
        } else if (checkIfLambdaExpression()) {
            memberName = currentToken.getValue();
            eat(TokenTypes::Identifier, [&]() {
                std::string suggestion = Console::formatString(
                    "To resolve this:\n"
                    "1. Provide a valid function name\n"
                    "2. Check function syntax\n"
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
            member = parseLambdaFunction(memberName);
            if (!member) {
                console.reportError(
                    Console::SYNTAX_ERROR,
                    "Failed to parse lambda function",
                    "To resolve this:\n1. Verify lambda function syntax\n2. Check parameter and return type syntax\n3. Ensure proper function body",
                    span
                );
                return nullptr;
            }
        } else {
            member = parseStatement();
            if (!member) {
                console.reportError(
                    Console::SYNTAX_ERROR,
                    "Failed to parse module member",
                    "To resolve this:\n1. Verify member syntax\n2. Check for valid statement types\n3. Ensure proper declaration",
                    span
                );
                return nullptr;
            }
        }
        
        if (auto named = std::dynamic_pointer_cast<NamedStatement>(member)) {
            memberName = named->getName();
            auto wrapped = std::make_shared<ModuleMember>(memberName, member, modifiers);
            members.push_back(wrapped);
            continue;
        } else if (auto block = std::dynamic_pointer_cast<BlockStatement>(member)) {
            for (const auto& stmt : block->statements) {
                if (auto named = std::dynamic_pointer_cast<NamedStatement>(stmt)) {
                    std::string m_Name = named->getName();
                    auto wrapped = std::make_shared<ModuleMember>(m_Name, stmt, modifiers);
                    members.push_back(wrapped);
                } else {
                    console.reportError(
                        Console::SYNTAX_ERROR,
                        "Module member must have a name",
                        "To resolve this:\n1. Ensure all block members are named\n2. Check statement types\n3. Use valid function or variable declarations",
                        span
                    );
                    return nullptr;
                }
            }
            continue;
        }

        console.reportError(
            Console::SYNTAX_ERROR,
            "Cannot determine name of member in module: " + moduleName,
            "To resolve this:\n1. Ensure member is a named statement or block\n2. Check member declaration syntax\n3. Verify valid member types",
            span
        );
        return nullptr;
    }

    eat(TokenTypes::RightBrace, [&]() {
        std::string suggestion = Console::formatString(
            "To resolve this:\n"
            "1. Close module body with '}'\n"
            "2. Check for matching braces\n"
            "3. Expected token: '}', found '%s'",
            getTokenTypeName(currentToken.getType()).c_str()
        );
        console.reportError(
            Console::SYNTAX_ERROR,
            Console::formatString("Expected '}' to close module body, found '%s'", 
                getTokenTypeName(currentToken.getType()).c_str()),
            suggestion,
            span
        );
    });

    eat(TokenTypes::EOI, [&]() {
        std::string suggestion = Console::formatString(
            "To resolve this:\n"
            "1. Ensure module is the only declaration in the file\n"
            "2. Check for extraneous code after module\n"
            "3. Expected token: end of input, found '%s'",
            getTokenTypeName(currentToken.getType()).c_str()
        );
        console.reportError(
            Console::SYNTAX_ERROR,
            Console::formatString("Expected end of input after module, found '%s'", 
                getTokenTypeName(currentToken.getType()).c_str()),
            suggestion,
            span
        );
    });

    span.end.line = previousToken.getLine();
    span.end.col = previousToken.getColumn();
    span.end.filePath = previousToken.getFilePath();

    return std::make_shared<CreateModule>(moduleName, members);
}
