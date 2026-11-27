// src\Parser\ParseObjects.cpp
#include <omniscript/Statements/Statement.h>
#include <omniscript/Statements/CallableStatement.h>
#include <omniscript/Statements/FunctionStatement.h>
#include <omniscript/Statements/ModuleAndImportStatements.h>
#include <omniscript/Statements/ClassConstructorStatement.h>
#include <omniscript/Statements/StructConstructorStatement.h>

#include <omniscript/Core.h>
#include <omniscript/utils.h>
#include <omniscript/Parser.h>
#include <omniscript/Tokens.h>
#include <omniscript/omniscript_pch.h>
#include <omniscript/Statements/Statement.h>
#include <omniscript/Types/Types.h>
#include <omniscript/Symboltable.h>

namespace Omniscript {

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

    std::shared_ptr<Type> thisType = Type::createUserDefinedType(className, Kind::Class);
    std::vector<std::shared_ptr<ClassMember>> members;

    eat(TokenTypes::LeftBrace);

    bool hasConstructor = false;
    bool hasDestructor = false;

    while (currentToken.getType() != TokenTypes::RightBrace) {
        MemberModifiers modifiers = parseMemberModifiers();

        // ---- TEXTUAL INCLUDE (class body) ----
        if (currentToken.getType() == TokenTypes::Include) {
            FileSpan incSpan;
            incSpan.start.line = currentToken.getLine();
            incSpan.start.col  = currentToken.getColumn();
            incSpan.start.filePath = currentToken.getFilePath();

            eat(TokenTypes::Include);
            if (currentToken.getType() != TokenTypes::StringLiteral) {
                console.reportError(Console::SYNTAX_ERROR,
                    "Expected string literal after 'include'.",
                    errorSpanAtCurrentToken());
                return nullptr;
            }
            const std::string incPath = currentToken.getValue();
            eat(TokenTypes::StringLiteral);
            if (currentToken.getType() == TokenTypes::Semicolon) eat(TokenTypes::Semicolon);

            incSpan.end.line = previousToken.getLine();
            incSpan.end.col  = previousToken.getColumn();
            incSpan.end.filePath = previousToken.getFilePath().empty()
                ? incSpan.start.filePath : previousToken.getFilePath();

            auto incStmt = std::make_shared<IncludeStatement>(incPath);
            incStmt->setSpan(incSpan);

            const std::string src = incStmt->getSourceText();
            if (src.empty()) continue;

            // Wrap in a synthetic class so the included contents are parsed as CLASS
            // MEMBERS (methods -> '<synName>.<method>'), not as a standalone program.
            // `__inc__` prefix makes the name astronomically unlikely to collide with
            // any user-written class. The `{`/`}` are placed flush against the source
            // so line numbers stay identical; only line 1's columns get a small shift.
            const std::string synName = "__inc__" + className;
            const std::string wrapped = "class " + synName + " {" + src + "}";
            Lexer incLexer(wrapped, incPath);
            Parser incParser(incLexer);
            auto parsed = incParser.parse();

            bool spliced = false;
            for (auto& s : parsed) {
                if (auto cp = std::dynamic_pointer_cast<ConstructClassPrototype>(s)) {
                    for (auto& cm : cp->getBody()) {
                        if (!cm) continue;
                        const std::string mName = cm->getName();

                        // parseClass auto-inserts a default ctor (ClassMember name =
                        // class name) and dtor (ClassMember name = "~<classname>")
                        // when none was found in its body. Skip those: the OUTER class
                        // will insert its own defaults if the include contributed none.
                        if (mName == synName || mName == "~" + synName) continue;

                        // A user-written ctor/dtor appears with the literal member
                        // names "constructor" / "destructor". Mark them so the outer
                        // class does not also auto-insert defaults on top of them.
                        if (mName == "constructor") hasConstructor = true;
                        if (mName == "destructor") hasDestructor = true;

                        // Rename the FunctionDeclaration from '<synName>.method' to
                        // '<className>.method'. The ClassMember's own name (e.g.
                        // "init") is already bare and matches what parseClass produces
                        // for inline methods, so it stays untouched.
                        if (auto fn = std::dynamic_pointer_cast<FunctionDeclaration>(cm->getValue())) {
                            const std::string fnName = fn->getName();
                            if (fnName.rfind(synName + ".", 0) == 0) {
                                fn->setName(className + fnName.substr(synName.size()));
                            }
                            fn->pushContext(className);
                        }
                        members.push_back(cm);
                    }
                    spliced = true;
                    break;
                }
            }
            if (!spliced) {
                console.reportError(Console::SYNTAX_ERROR,
                    "Included file '" + incPath + "' could not be parsed as class members.",
                    incSpan);
            }
            continue;
        }
                
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

        std::shared_ptr<Type> typeExpr = nullptr;
        if (currentToken.getType() == TokenTypes::Colon) {
            eat(TokenTypes::Colon);
            auto parsedType = parseType();
            typeExpr = resolveType(parsedType);
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
            if (!methodExpr) {
                console.reportError(Console::SYNTAX_ERROR,
                    "Could not parse method '" + memberName + "' of class '" + className + "'.",
                    FileSpan{});
                return nullptr;
            }
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
            resolveType({"void"})
        );
        auto ctorMember = std::make_shared<ClassMember>(
            className,
            resolveType({"void"}),
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
            resolveType({"void"})
        );
        auto dtorMember = std::make_shared<ClassMember>(
            "~" + className,
            resolveType({"void"}),
            defaultDtor,
            modifiers
        );
        members.push_back(dtorMember);
    }    

    auto classStatement = std::make_shared<ConstructClassPrototype>(className, parentClasses, members, types);
    classStatement->setPosition(startToken, currentToken);
    return classStatement;
}

// ---- operator overloading helpers (shared by the in-struct and the out-of-struct form) ----

// `canIdx` = lookahead index of `can`. Recognises the operator spelled right after it and requires `(` after that.
bool Parser::matchOperatorAfterCan(int canIdx, std::string& opName, int& opTokens) {
    opName.clear();
    opTokens = 0;
    const int b = canIdx;
    switch (lexer.peekToken(b + 1).getType()) {
        case TokenTypes::Plus:         opName = "+";   opTokens = 1; break;
        case TokenTypes::Minus:        opName = "-";   opTokens = 1; break;
        case TokenTypes::Multiply:     opName = "*";   opTokens = 1; break;
        case TokenTypes::Divide:       opName = "/";   opTokens = 1; break;
        case TokenTypes::Modulo:       opName = "%";   opTokens = 1; break;
        case TokenTypes::Equals:       opName = "==";  opTokens = 1; break;
        case TokenTypes::NotEquals:    opName = "!=";  opTokens = 1; break;
        case TokenTypes::LessThan:     opName = "<";   opTokens = 1; break;
        case TokenTypes::GreaterThan:  opName = ">";   opTokens = 1; break;
        case TokenTypes::LessEqual:    opName = "<=";  opTokens = 1; break;
        case TokenTypes::GreaterEqual: opName = ">=";  opTokens = 1; break;
        case TokenTypes::BitwiseAnd:   opName = "&";   opTokens = 1; break;
        case TokenTypes::BitwiseOr:    opName = "|";   opTokens = 1; break;
        case TokenTypes::BitwiseXor:   opName = "^";   opTokens = 1; break;
        case TokenTypes::ShiftLeft:    opName = "<<";  opTokens = 1; break;
        case TokenTypes::ShiftRight:   opName = ">>";  opTokens = 1; break;
        case TokenTypes::LogicalNot:   opName = "!";   opTokens = 1; break;
        case TokenTypes::Tilde:        opName = "~";   opTokens = 1; break;
        case TokenTypes::Increment:    opName = "++";  opTokens = 1; break;
        case TokenTypes::Decrement:    opName = "--";  opTokens = 1; break;
        case TokenTypes::PlusAssign:     opName = "+="; opTokens = 1; break;
        case TokenTypes::MinusAssign:    opName = "-="; opTokens = 1; break;
        case TokenTypes::MultiplyAssign: opName = "*="; opTokens = 1; break;
        case TokenTypes::DivideAssign:   opName = "/="; opTokens = 1; break;
        case TokenTypes::ModuloAssign:   opName = "%="; opTokens = 1; break;
        case TokenTypes::LeftBracket:
            if (lexer.peekToken(b + 2).getType() == TokenTypes::RightBracket) {
                if (lexer.peekToken(b + 3).getType() == TokenTypes::Assign) { opName = "[]="; opTokens = 3; }
                else { opName = "[]"; opTokens = 2; }
            }
            break;
        default: break;
    }
    if (opTokens == 0 || lexer.peekToken(b + opTokens + 1).getType() != TokenTypes::LeftParen) {
        opName.clear();
        opTokens = 0;
        return false;
    }
    // `can - () => T` (no parameters) is the UNARY minus/plus; `can - (o: T) => R` is the binary one.
    if ((opName == "-" || opName == "+") && lexer.peekToken(b + opTokens + 2).getType() == TokenTypes::RightParen) {
        opName = "unary" + opName;
    }
    return true;
}

// Statement-level lookahead (current token is an identifier): `A can op (`, `A.B can op (`, `A<T, U> can op (`.
bool Parser::isOutOfStructOperatorAhead() {
    int k = 1;
    while (lexer.peekToken(k).getValue() == ".") {
        if (lexer.peekToken(k + 1).getType() != TokenTypes::Identifier) return false;
        k += 2;
    }
    if (lexer.peekToken(k).getType() == TokenTypes::LessThan) {
        int depth = 0;
        const int limit = k + 64;
        for (; k < limit; ++k) {
            const TokenTypes t = lexer.peekToken(k).getType();
            if (t == TokenTypes::LessThan) ++depth;
            else if (t == TokenTypes::GreaterThan) --depth;
            else if (t == TokenTypes::ShiftRight) depth -= 2;
            else if (t == TokenTypes::EOI || t == TokenTypes::Semicolon || t == TokenTypes::LeftBrace ||
                     t == TokenTypes::LeftParen || t == TokenTypes::Assign) return false;
            if (depth <= 0) { ++k; break; }
        }
        if (depth != 0) return false;
    }
    const Token canTok = lexer.peekToken(k);
    if (canTok.getType() != TokenTypes::Identifier || canTok.getValue() != "can") return false;
    std::string opName;
    int opTokens = 0;
    return matchOperatorAfterCan(k, opName, opTokens);
}

// Out-of-struct operator:
//     Num can * (o: Num*) => Num { ... }          non-generic: a free function "Num.operator*" with an explicit `this: Num*`
//     DynArray<T> can + (o: DynArray<T>*) => ... generic: handed to the template as an extra member
// For the generic form the method is wrapped in a ConstructStructPrototype of the same name and type parameters;
// GenericInstantiator::registerTemplate recognises that shape and appends the method to the existing template.
std::shared_ptr<Statement> Parser::parseOutOfStructOperator() {
    const Token startToken = currentToken;

    std::string typeName = currentToken.getValue();
    std::vector<std::string> typeTokens{typeName};
    eat(TokenTypes::Identifier);
    while (currentToken.getValue() == ".") {
        eat(currentToken.getType());
        typeName += "." + currentToken.getValue();
        typeTokens.push_back(".");
        typeTokens.push_back(currentToken.getValue());
        eat(TokenTypes::Identifier);
    }

    parameterType typeParams;
    if (currentToken.getType() == TokenTypes::LessThan) {
        typeParams = parseTypeParametersForDeclaration();
        if (typeParams.empty()) return nullptr;      // already reported
    }

    std::string opName;
    int opTokens = 0;
    if (currentToken.getType() != TokenTypes::Identifier || currentToken.getValue() != "can" ||
        !matchOperatorAfterCan(0, opName, opTokens)) {
        console.reportError(Console::SYNTAX_ERROR,
            "Expected 'can <operator> (' after type '" + typeName + "'.",
            errorSpanAtCurrentToken());
        return nullptr;
    }
    eat(TokenTypes::Identifier);                       // `can`
    for (int i = 0; i < opTokens; ++i) eat(currentToken.getType());

    const std::string methodName = typeName + ".operator" + opName;

    if (!typeParams.empty()) {
        // Plain method: `this` is added when the instantiated struct is expressed, T is substituted per instance.
        auto func = parseLambdaFunction(methodName);
        if (!func) return nullptr;
        std::vector<std::shared_ptr<Statement>> members{func};
        auto proto = std::make_shared<ConstructStructPrototype>(typeName, members, typeParams);
        proto->setPosition(startToken, previousToken);
        return proto;
    }

    std::shared_ptr<Type> thisType = resolveType(typeTokens);
    if (!thisType) thisType = Type::createUserDefinedType(typeName, Kind::Struct);
    // parseFunctionDeclaration inserts `this: T*` as the first parameter when it is given the type.
    auto func = parseLambdaFunction(methodName, {}, thisType);
    if (func) func->setPosition(startToken, previousToken);
    return func;
}

std::shared_ptr<Statement> Parser::parseStruct() {
    Token startToken = currentToken;
    eat(TokenTypes::Struct);
    std::string structName = currentToken.getValue();
    eat(TokenTypes::Identifier);

    parameterType typeParams;
    if (currentToken.getType() == TokenTypes::LessThan) {
        typeParams = parseTypeParametersForDeclaration();
    }

    std::shared_ptr<Type> thisType = Type::createUserDefinedType(structName, Kind::Struct);
    std::vector<std::shared_ptr<Statement>> body;

    eat(TokenTypes::LeftBrace);

    // After a bad member, skip to the end of that member (its ';', or the struct's '}') so one
    // mistake does not turn the rest of the body into a pile of top-level parse errors. The
    // struct is still returned: the error is already reported and the program will not run.
    auto skipBadMember = [&]() {
        int depth = 0;
        while (currentToken.getType() != TokenTypes::EOI) {
            const TokenTypes t = currentToken.getType();
            if (depth == 0 && t == TokenTypes::Semicolon) { eat(TokenTypes::Semicolon); return; }
            if (depth == 0 && t == TokenTypes::RightBrace) return;
            if (t == TokenTypes::LeftBrace) ++depth;
            else if (t == TokenTypes::RightBrace) --depth;
            eat(t);
        }
    };

    while (currentToken.getType() != TokenTypes::RightBrace &&
           currentToken.getType() != TokenTypes::EOI) {

        // ---- TEXTUAL INCLUDE (struct body) ----
        if (currentToken.getType() == TokenTypes::Include) {
            FileSpan incSpan;
            incSpan.start.line = currentToken.getLine();
            incSpan.start.col  = currentToken.getColumn();
            incSpan.start.filePath = currentToken.getFilePath();

            eat(TokenTypes::Include);
            if (currentToken.getType() != TokenTypes::StringLiteral) {
                console.reportError(Console::SYNTAX_ERROR,
                    "Expected string literal after 'include'.",
                    errorSpanAtCurrentToken());
                skipBadMember();
                continue;
            }
            const std::string incPath = currentToken.getValue();
            eat(TokenTypes::StringLiteral);
            if (currentToken.getType() == TokenTypes::Semicolon) eat(TokenTypes::Semicolon);

            incSpan.end.line = previousToken.getLine();
            incSpan.end.col  = previousToken.getColumn();
            incSpan.end.filePath = previousToken.getFilePath();

            auto incStmt = std::make_shared<IncludeStatement>(incPath);
            incStmt->setSpan(incSpan);

            const std::string src = incStmt->getSourceText();
            if (src.empty()) continue;

            // Parse the file's contents in STRUCT context by wrapping in a synthetic
            // struct of the SAME name. Methods come out named '<structName>.<method>'
            // (exactly what parseLambdaFunction(structName + "." + name) would have
            // produced inline). Spans point at incPath because the Lexer is told so.
            const std::string wrapped = "struct " + structName + " {\n" + src + "\n}\n";
            Lexer incLexer(wrapped, incPath);
            Parser incParser(incLexer);
            auto parsed = incParser.parse();

            bool spliced = false;
            for (auto& s : parsed) {
                if (auto cs = std::dynamic_pointer_cast<ConstructStructPrototype>(s)) {
                    for (auto& m : cs->getBody()) {
                        body.push_back(m);   // already named '<structName>.<method>'
                    }
                    spliced = true;
                    break;
                }
            }
            if (!spliced) {
                console.reportError(Console::SYNTAX_ERROR,
                    "Included file '" + incPath + "' could not be parsed as struct members.",
                    incSpan);
            }
            continue;
        }

        // ---- DESTRUCTOR: `~destructor() { ... }` ----
        // Parsed like any other method; it is registered as '<structName>.destructor' (the same
        // name parseClass gives class destructors) and gets the implicit `this` in
        // ConstructStructPrototype::express.
        if (currentToken.getType() == TokenTypes::Tilde) {
            eat(TokenTypes::Tilde);
            if (currentToken.getType() != TokenTypes::Identifier || currentToken.getValue() != "destructor") {
                console.reportError(Console::SYNTAX_ERROR,
                    "Expected 'destructor' after '~' in struct '" + structName + "' (write `~destructor() { ... }`).",
                    errorSpanAtCurrentToken());
                skipBadMember();
                continue;
            }
            eat(TokenTypes::Identifier);
            if (currentToken.getType() != TokenTypes::LeftParen) {
                console.reportError(Console::SYNTAX_ERROR,
                    "Expected '(' after '~destructor' in struct '" + structName + "'.",
                    errorSpanAtCurrentToken());
                skipBadMember();
                continue;
            }
            auto dtor = parseLambdaFunction(structName + ".destructor");
            if (!dtor) {
                skipBadMember(); // the lambda parser already reported the error
                continue;
            }
            body.push_back(dtor);
            if (currentToken.getType() == TokenTypes::Semicolon) {
                eat(TokenTypes::Semicolon);
            }
            continue;
        }

        // ---- OPERATOR METHOD: `can <op> (params) => ret { body }` ----
        // `can` is a contextual keyword: it only starts an operator when an operator token and then `(` follow.
        // The method is an ordinary method with the reserved name '<struct>.operator<op>', e.g. 'Vec.operator+',
        // 'Vec.operator[]', 'Vec.operator[]='. Unary vs binary is decided later by the parameter count.
        if (currentToken.getType() == TokenTypes::Identifier && currentToken.getValue() == "can") {
            std::string opName;
            int opTokens = 0;
            if (matchOperatorAfterCan(0, opName, opTokens)) {
                eat(TokenTypes::Identifier);                       // `can`
                for (int i = 0; i < opTokens; ++i) eat(currentToken.getType());
                auto func = parseLambdaFunction(structName + ".operator" + opName);
                if (!func) { skipBadMember(); continue; }
                body.push_back(func);
                if (currentToken.getType() == TokenTypes::Semicolon) eat(TokenTypes::Semicolon);
                continue;
            }
        }

        if (currentToken.getType() == TokenTypes::Identifier) {
            std::string fieldName = currentToken.getValue();
            eat(TokenTypes::Identifier);

            // `name(` is a method. Deciding this from the token right after the name (as
            // parseClass does) avoids checkIfLambdaExpression(), which misreads parameter
            // lists containing pointer types such as `char*` or `void**`.
            // `name<T>(` is a generic method: `symbolAs<F>(name: char*) => F { ... }`. The type
            // parameters are parsed first and handed to the lambda parser, exactly as for a generic
            // free function (see the `<`/`(` case in Parser::factor).
            parameterType methodTypeParams;
            bool genericMethod = false;
            if (currentToken.getType() == TokenTypes::LessThan) {
                int i = 0;
                if (tryParseTypeParametersLookahead(i) && lexer.peekToken(i).getType() == TokenTypes::LeftParen) {
                    methodTypeParams = parseTypeParametersForDeclaration();
                    genericMethod = true;
                }
            }

            if (currentToken.getType() == TokenTypes::LeftParen) {
                auto func = genericMethod
                    ? parseLambdaFunction(structName + "." + fieldName, methodTypeParams)
                    : parseLambdaFunction(structName + "." + fieldName);
                if (!func) {
                    skipBadMember(); // the lambda parser already reported the error
                    continue;
                }
                body.push_back(func);

                if (currentToken.getType() == TokenTypes::Semicolon) {
                    eat(TokenTypes::Semicolon);
                }
                continue;
            }

            std::vector<std::string> type;
            std::shared_ptr<Statement> value = nullptr;

            if (currentToken.getType() == TokenTypes::Colon) {
                eat(TokenTypes::Colon);
                type = parseType();
            }

            if (currentToken.getType() == TokenTypes::Assign) {
                eat(TokenTypes::Assign);
                value = parseExpression();
            }

            auto field = std::make_shared<ParameterStatement>(fieldName, value);

            if (!type.empty()) {
                field->setType(resolveType(type));
            } else if (!value) {
                console.reportError(
                    Console::SYNTAX_ERROR,
                    "Field '" + fieldName + "' in struct '" + structName +
                    "' needs a type (': Type') or an initial value.",
                    errorSpanAtCurrentToken());
                skipBadMember();
                continue;
            } else if (auto typed = std::dynamic_pointer_cast<TypedStatement>(value)) {
                if (typed->getType()) {
                    field->setType(typed->getType());
                }
            }
            // An initializer that is not a typed statement leaves the field type unset
            // here so a later stage can infer it, instead of dereferencing null.

            body.push_back(field);
            eat(TokenTypes::Semicolon);

        } else {
            // A reserved word used as a field name gets the "rename it" explanation.
            const std::string msg = Token::isKeywordType(currentToken.getType())
                ? expectedIdentifierMessage("field name")
                : "Unexpected token '" + currentToken.getValue() + "' in struct '" + structName + "' body.";
            console.reportError(Console::SYNTAX_ERROR, msg, errorSpanAtCurrentToken());
            skipBadMember();
        }

    }

    eat(TokenTypes::RightBrace);

    auto structStatement = std::make_shared<ConstructStructPrototype>(structName, body, typeParams);
    structStatement->setPosition(startToken, currentToken);
    return structStatement;
}

}