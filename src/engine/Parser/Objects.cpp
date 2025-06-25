#include <omniscript/engine/Statement.h>
#include <omniscript/engine/Statements/CallableStatement.h>
#include <omniscript/engine/Statements/FunctionStatement.h>
#include <omniscript/engine/Statements/ModuleAndImportStatements.h>
#include <omniscript/engine/Statements/ClassConstructorStatement.h>
#include <omniscript/engine/Statements/StructConstructorStatement.h>

#include <omniscript/engine/Core.h>
#include <omniscript/utils.h>
#include <omniscript/engine/Parser.h>
#include <omniscript/engine/Tokens.h>
#include <omniscript/omniscript_pch.h>
#include <omniscript/engine/Statement.h>
#include <omniscript/engine/Symboltable.h>


std::shared_ptr<Statement> Parser::parseObject() {
    // eat(TokenTypes::LeftBrace);

    // bool isDictionary = true;
    // auto object = std::make_shared<Object>(); // Start with a generic Object

    // while (currentToken.getType() != TokenTypes::RightBrace) {
    //     std::string propertyName;
    //     std::shared_ptr<Statement> propertyValue;

    //     // Accept identifiers, string literals, and numbers as keys
    //     if (currentToken.getType() == TokenTypes::Identifier || 
    //         currentToken.getType() == TokenTypes::StringLiteral || 
    //         currentToken.getType() == TokenTypes::IntegerLiteral) {
            
    //         // Convert numbers to strings for consistent key handling
    //         if (currentToken.getType() == TokenTypes::IntegerLiteral) {
    //             propertyName = std::to_string(std::stoi(currentToken.getValue()));
    //         } else {
    //             propertyName = currentToken.getValue();
    //         }

    //         // If the key is an identifier, it cannot be part of a dictionary
    //         if (currentToken.getType() == TokenTypes::Identifier) {
    //             isDictionary = false;
    //         }
    
    //         eat(currentToken.getType());

    //         // Check for a colon to determine key-value pair
    //         if (currentToken.getType() == TokenTypes::Colon) {
    //             eat(TokenTypes::Colon);
    //             propertyValue = parseExpression(); // Parse the value expression
    //         } else {
    //             // If there's no colon, it behaves like an object property
    //             isDictionary = false;
    //             propertyValue = std::make_shared<GetVariable>(propertyName);
    //         }

    //         // Set the property on the object
    //         // object->setProperty(propertyName, propertyValue);
    //     } else {
    //         eat(TokenTypes::RightBrace, "Invalid property name in object or dictionary.");
    //     }

    //     // Handle commas or terminate on the right brace
    //     if (currentToken.getType() == TokenTypes::Comma) {
    //         eat(TokenTypes::Comma);
    //     } else if (currentToken.getType() != TokenTypes::RightBrace) {
    //         eat(TokenTypes::RightBrace, "Expected ',' or '}' in object or dictionary.");
    //     }
    // }
    
    // eat(TokenTypes::RightBrace);

    // // Convert to Dictionary if all keys are valid for dictionary
    // if (isDictionary) {
    //     auto dictionary = std::make_shared<Dictionary>();
    //     for (const auto& [key, value] : object->properties) {
    //         dictionary->setProperty(key, value);
    //     }
    //     return std::make_shared<ObjectConstructorStatement>(dictionary);
    // } 
    // return std::make_shared<ObjectConstructorStatement>(object);
    return nullptr;
}

std::shared_ptr<Statement> Parser::parseClass() {
    Token startToken = currentToken;
    eat(TokenTypes::Class);
    std::string className = currentToken.getValue();
    eat(TokenTypes::Identifier);

    parameterType types;
    if (currentToken.getType() == TokenTypes::LessThan) {
        types = parseTypeParametersForDeclaration();
    }

    std::vector<std::string> parentClasses;
    if (currentToken.getType() == TokenTypes::Colon) {
        eat(TokenTypes::Colon);
        while (currentToken.getType() != TokenTypes::LeftBrace) {
            if (currentToken.getType() == TokenTypes::Comma) eat(TokenTypes::Comma);
            if (currentToken.getType() == TokenTypes::Public || currentToken.getType() == TokenTypes::Private)
                eat(currentToken.getType()); // Ignore for now
            std::string parent = currentToken.getValue();
            eat(TokenTypes::Identifier);
            parentClasses.push_back(parent);
        }
    }

    std::shared_ptr<Omniscript::Type> thisType = Omniscript::Type::createUserDefinedType(className, Omniscript::Kind::Class);
    std::vector<std::shared_ptr<ClassMember>> members;

    eat(TokenTypes::LeftBrace);

    bool hasConstructor = false;
    bool hasDestructor = false;

    while (currentToken.getType() != TokenTypes::RightBrace) {
        MemberModifiers modifiers = parseMemberModifiers();

        std::string memberName;
        bool isDestructor = false;
        if (currentToken.getType() == TokenTypes::Tilde) {
            eat(currentToken.getType());
            // memberName = "~";
            isDestructor = true;
        }

        if (currentToken.getType() != TokenTypes::Identifier) {
            eat(TokenTypes::Identifier, "Expected identifier in class body.");
            return nullptr;
        }

        memberName += currentToken.getValue();
        eat(TokenTypes::Identifier);

        if (memberName == "constructor") hasConstructor = true;
        if (isDestructor && memberName == "destructor") hasDestructor = true;

        std::shared_ptr<Omniscript::Type> typeExpr = nullptr;
        if (currentToken.getType() == TokenTypes::Colon) {
            eat(TokenTypes::Colon);
            auto parsedType = parseType();
            typeExpr = Omniscript::resolveType(parsedType);
        }

        std::shared_ptr<Statement> valueExpr = nullptr;
        if (currentToken.getType() == TokenTypes::Assign) {
            eat(TokenTypes::Assign);
            valueExpr = parseExpression();
            if (auto ctxAware = std::dynamic_pointer_cast<ContextAwareStatement>(valueExpr)) {
                ctxAware->pushContext(className);
            }
        } else if (currentToken.getType() == TokenTypes::LeftParen) {
            valueExpr = parseLambdaFunction(className + "." + memberName);
            auto methodExpr = std::dynamic_pointer_cast<FunctionDeclaration>(valueExpr);
            methodExpr->pushContext(className);
        }

        auto member = std::make_shared<ClassMember>(memberName, typeExpr, valueExpr, modifiers);
        members.push_back(member);

        if (currentToken.getType() == TokenTypes::Semicolon) {
            eat(TokenTypes::Semicolon);
        }
    }

    eat(TokenTypes::RightBrace);
    
    if (!hasConstructor) {
        MemberModifiers modifiers;
        auto emptyBody = BlockStatement::create();
        auto defaultCtor = std::make_shared<FunctionDeclaration>(
            className + ".constructor",
            std::vector<std::shared_ptr<Statement>>{},
            emptyBody,
            Omniscript::resolveType({"void"})
        );
        auto ctorMember = std::make_shared<ClassMember>(
            className,
            Omniscript::resolveType({"void"}),
            defaultCtor,
            modifiers
        );
        members.insert(members.begin(), ctorMember);
    }
    
    if (!hasDestructor) {
        MemberModifiers modifiers;
        auto emptyBody = BlockStatement::create();
        auto defaultDtor = std::make_shared<FunctionDeclaration>(
            className + ".destructor",
            std::vector<std::shared_ptr<Statement>>{},
            emptyBody,
            Omniscript::resolveType({"void"})
        );
        auto dtorMember = std::make_shared<ClassMember>(
            "~" + className,
            Omniscript::resolveType({"void"}),
            defaultDtor,
            modifiers
        );
        members.push_back(dtorMember);
    }    

    auto classStatement = std::make_shared<ConstructClassPrototype>(className, parentClasses, members);
    classStatement->setPosition(startToken);
    return classStatement;
}

std::shared_ptr<Statement> Parser::parseStruct() {
    Token startToken = currentToken;
    eat(TokenTypes::Struct);
    std::string structName = currentToken.getValue();
    eat(TokenTypes::Identifier);

    std::shared_ptr<Omniscript::Type> thisType = Omniscript::Type::createUserDefinedType(structName, Omniscript::Kind::Struct);
    std::vector<std::shared_ptr<Statement>> body;

    eat(TokenTypes::LeftBrace);

    while (currentToken.getType() != TokenTypes::RightBrace) {

        if (checkIfLambdaExpression()) {
            std::string methodName = structName + "." + currentToken.getValue();
            eat(TokenTypes::Identifier);
            
            auto func = parseLambdaFunction(methodName);
            body.push_back(func);
            
            if (currentToken.getType() == TokenTypes::Semicolon) {
                eat(TokenTypes::Semicolon);
            }

        } else if (currentToken.getType() == TokenTypes::Identifier) {
            std::string fieldName = currentToken.getValue();
            std::vector<std::string> type;
            std::shared_ptr<Statement> value = nullptr;

            eat(TokenTypes::Identifier);

            if (currentToken.getType() == TokenTypes::Colon) {
                eat(TokenTypes::Colon);
                type = parseType();
            }

            if (currentToken.getType() == TokenTypes::Assign) {
                eat(TokenTypes::Assign);
                value = parseExpression();
            }

            auto field = std::make_shared<ParameterStatement>(fieldName, value);
            field->setType(Omniscript::resolveType(type));
            body.push_back(field);
            eat(TokenTypes::Semicolon);

        } else {
            console.error("Unexpected token in struct body.");
        }

    }

    eat(TokenTypes::RightBrace);

    auto structStatement = std::make_shared<ConstructStructPrototype>(structName, body);
    structStatement->setPosition(startToken);
    return structStatement;
}
