#include <omniscript/Core.h>
#include <omniscript/utils.h>
#include <omniscript/runtime/object.h>
#include <omniscript/engine/parser.h>
#include <omniscript/engine/lexer.h>
#include <omniscript/engine/tokens.h>
#include <omniscript/engine/Statement.h>
#include <omniscript/engine/Symboltable.h>
#include <omniscript/mainthreadrunner.h>
#include <omniscript/omniscript_pch.h>

// built - in objects
// // #include <omniscript/runtime/Function.h>
// #include <omniscript/runtime/Class.h>
// #include <omniscript/runtime/Namespace.h>
// #include <omniscript/runtime/Enum.h>
// #include <omniscript/runtime/Number.h>
// #include <omniscript/runtime/String.h>


// Environment Objects
// #include <omniscript/runtime/graphics/canvas.h>
// // #include <omniscript/runtime/audio/AudioAccess.h>
// #include <omniscript/runtime/Http/Http.h>
// #include <omniscript/runtime/io/console.h>
// #include <omniscript/runtime/io/FileAccess.h>
// #include <omniscript/runtime/Math/Math.h>
// #include <omniscript/runtime/Time/Time.h>
// #include <omniscript/runtime/Json/Json.h>
// #include <omniscript/runtime/Date/Date.h>
// #include <omniscript/runtime/Path/Path.h>
// #include <omniscript/runtime/OS/OS.h>


// Entry point for parsing the program
std::vector<std::shared_ptr<Statement>> Parser::Parse() {
    initializeEnvironment();
    parseProgram(); // Start parsing the program
    return this->statements;
}

void Parser::initializeEnvironment() {
    initializeBuiltInObjects();
    initializeConstants();
    initializeFunctions();
}

void Parser::initializeBuiltInObjects() {
    // // scope.addObject("canvas", std::make_shared<CanvasObject>());
    // // scope.addObject("AudioAccess", std::make_shared<AudioAccess>());
    // scope.addObject("HTTP", std::make_shared<HTTP>());
    // scope.addObject("console", std::make_shared<ConsoleObject>());
    // scope.addObject("FileAccess", std::make_shared<FileAccess>());
    // scope.addObject("Math", std::make_shared<Math>());
    // scope.addObject("Time", std::make_shared<Time>());
    // scope.addObject("JSON", std::make_shared<JSON>());
    // scope.addObject("Date", std::make_shared<Date>());
    // scope.addObject("Path", std::make_shared<Path>());
    // scope.addObject("OS", std::make_shared<OS>());
}

void Parser::initializeConstants() {

}

void Parser::initializeFunctions() {

}


// Parse a complete program
void Parser::parseProgram() {
    DEBUG_LOG();
    DEBUG_LOG("Parsing the script");
    DEBUG_LOG("==================");
    DEBUG_LOG();
    
    while (currentToken.getType() != TokenTypes::EOI) {
        statements.push_back(parseStatement()); // Parse each statement in the program
    }
    
    DEBUG_LOG();
    DEBUG_LOG("Done parsing the script");
    DEBUG_LOG("=======================");
    DEBUG_LOG();
}


// Helper function to consume a token if it matches the expected type
void Parser::eat(TokenTypes expectedType, const std::string& err) {
    // Keep track of your position in the current file
    Omniscript::setPosition(currentToken.getLine(), currentToken.getColumn(), currentToken.getFilePath());
    if (currentToken.getType() == expectedType) {
        previousToken = currentToken;
        currentToken = lexer.getNextToken(); // Move to the next token
    } else {
        std::string errorMessage = "[Parser Error]\nExpected token type: " 
        + getTokenTypeName(expectedType) 
        + " at line: " + std::to_string(currentToken.getLine()) 
        + " column: " + std::to_string(currentToken.getColumn()) 
        + " got token type " + getTokenTypeName(currentToken.getType()) 
        + " instead.\n";

        
        if (err != "") {
            errorMessage += "\n\n" + err;
        }
        console.error(errorMessage);
    }
}

// TODO: Omniscript automatically skips all new lines
void Parser::expectSemicolonOrNewLine() {
    if (currentToken.getType() == TokenTypes::Semicolon) {
        eat(TokenTypes::Semicolon);
    } else {
        eat(TokenTypes::Newline);
    }
}

// Parse a single statement
std::shared_ptr<Statement> Parser::parseStatement(bool checkForTerminalChar) {
    std::shared_ptr<Statement> statement;
    // std::vector<std::shared_ptr<Statement>> statements;

    if (debugMode) { // If we are in debug mode, show all of the tokens being parsed
        std::string message = "The lexer got token '" + getTokenTypeName(currentToken.getType()) +
                            "' with value '" + currentToken.getValue() + 
                            "' at line: " + std::to_string(currentToken.getLine()) + 
                            " column: " + std::to_string(currentToken.getColumn());
        DEBUG_LOG(message);  // Using DEBUG_LOG to output the debug message
    }

    switch (currentToken.getType()) {
        case TokenTypes::Import:
            statement = parseModuleImport();
            break;
        case TokenTypes::Module:
            statement = parseModule();
            break;
        case TokenTypes::Function:
            statement = parseFunctionDeclaration("", {});
            break;
        case TokenTypes::Identifier: {
                if (lexer.peekToken(1).getType() == TokenTypes::Increment || lexer.peekToken(1).getType() == TokenTypes::Decrement) {
                    statement = parseExpression();
                } else {
                    statement = parseIdentifier();
                }
            }
            break;
        case TokenTypes::Increment:
        case TokenTypes::Decrement:
            statement = parseExpression();
            break;
        case TokenTypes::False:
            statement = parseExpression();
            break;
        case TokenTypes::True:
            statement = parseExpression();
            break;
        case TokenTypes::IntegerLiteral:
            statement = parseExpression();
            break;
        case TokenTypes::FloatLiteral:
            statement = parseExpression();
            break;
        case TokenTypes::StringLiteral:
            statement = parseExpression();
            break;
        case TokenTypes::If:
            statement = parseIfStatement();
            break;
        case TokenTypes::While:
            statement = parseWhileStatement();
            break;
        case TokenTypes::For:
            statement = parseForLoop();
            break;
        case TokenTypes::Continue:
            statement = parseContinue();
            break;
        case TokenTypes::Break:
            statement = parseBreak();
            break;
        case TokenTypes::Return:
            statement = parseReturnStatement();
            break;
        case TokenTypes::Struct:
            statement = parseStruct();
            break;
        case TokenTypes::Enum:
            statement = parseEnum();
            break;
        case TokenTypes::Namespace:
            statement = parseNamespace();
            break;
        case TokenTypes::Let:
            statement = parseAssignment();
            break;
        case TokenTypes::Const:
            statement = parseAssignment();
            break;
        case TokenTypes::Semicolon:
            statement = nullptr;
            break;
        case TokenTypes::Class:
            statement = parseClass();
            break;
        case TokenTypes::LessThan: {
            if (lexer.peekToken(1).getType() == TokenTypes::Identifier) {
                parameterType paramTypes = parseTypeParametersForDeclaration();
                if (currentToken.getType() == TokenTypes::Function) {
                    statement = parseFunctionDeclaration(paramTypes);
                    break;
                }
                else if (currentToken.getType() == TokenTypes::Let || currentToken.getType() == TokenTypes::Const) {
                    statement = parseAssignment(paramTypes);
                    break;
                }
            }
        }
        case TokenTypes::RightBrace:
            statement = nullptr; // add parse RightBrace method
            return statement;
        default:
           console.error(
                "[Parser Error]\nUnexpected token " + getTokenTypeName(currentToken.getType()) + " '" + currentToken.getValue() + "'" + " in statement"
            );
    }

    // Check for an optional semicolon
    if (checkForTerminalChar &&
        (currentToken.getType() == TokenTypes::Semicolon || currentToken.getType() == TokenTypes::Newline)) {
        eat(currentToken.getType()); 
    }

    statement->setPosition(Omniscript::getPosition());
    return statement;
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
    std::unordered_map<std::string, bool> publicMembers;

    eat(TokenTypes::Module);
    moduleName = currentToken.getValue();
    eat(TokenTypes::Identifier);
    eat(TokenTypes::LeftBrace);

    while (currentToken.getType() != TokenTypes::RightBrace) {
        bool isPublicMember = false;
        if (currentToken.getType() == TokenTypes::Public) {
            isPublicMember = true;
            eat(TokenTypes::Public);
        }

        // **Check for Nested Module Import Assignment**
        if (currentToken.getType() == TokenTypes::Module) {
            eat(TokenTypes::Module);
            std::string moduleAlias = currentToken.getValue();
            eat(TokenTypes::Identifier);

            eat(TokenTypes::Assign);
            eat(TokenTypes::Import);
            std::string modulePath = currentToken.getValue();
            eat(TokenTypes::StringLiteral);

            eat(TokenTypes::Semicolon);
            // expectSemicolonOrNewLine();

            // Parse the module immediately instead of treating it as an ImportModule
            std::string sourceCode = readFile(modulePath);
            if (sourceCode.empty()) {
                return nullptr;
                // throw std::runtime_error("Failed to read module: " + modulePath);
            }

            Lexer lexer(sourceCode);
            Parser parser(lexer);
            
            std::vector<std::shared_ptr<Statement>> moduleStatements = parser.Parse();
            auto moduleStmt = std::make_shared<CreateModule>(moduleAlias, moduleStatements);

            if (isPublicMember) {
                members.push_back(std::make_shared<PublicMember>(moduleAlias, moduleStmt));
            } else {
                members.push_back(std::make_shared<PrivateMember>(moduleAlias, moduleStmt));
            }

            continue;  // Skip further processing for this iteration
        }


        // **Handle Regular Module Members (Variables, Functions, etc.)**
        std::shared_ptr<Statement> member = parseStatement();
        std::string memberName;

        if (auto named = std::dynamic_pointer_cast<NamedStatement>(member)) {
            memberName = named->getName();
        } else {
            console.error("Cannot determine name of public member in module: " + moduleName);
            continue;
        }

        if (isPublicMember) {
            members.push_back(std::make_shared<PublicMember>(memberName, member));
        } else {
            members.push_back(std::make_shared<PrivateMember>(memberName, member));
        }
    }

    eat(TokenTypes::RightBrace);
    return std::make_shared<CreateModule>(moduleName, members);
}

std::shared_ptr<Statement> Parser::parseExpression() {
    return parseTernaryExpression(); // Delegate to ternary parsing first
}

std::shared_ptr<Statement> Parser::parseTernaryExpression() {
    std::shared_ptr<Statement> condition = parseBinaryExpression(); // Start with lower precedence

    if (currentToken.getType() == TokenTypes::QuestionMark) {
        eat(TokenTypes::QuestionMark);
        std::shared_ptr<Statement> truthy = parseExpression();
        eat(TokenTypes::Colon);
        std::shared_ptr<Statement> falsey = parseExpression();
        return std::make_shared<TernaryExpression>(condition, truthy, falsey);
    }

    return condition; // Return just the binary expression if no ternary is found
}

// Parse an expression, handling addition, subtraction, logical operators, and comparison operators
std::shared_ptr<Statement> Parser::parseBinaryExpression() {
    std::shared_ptr<Statement> left = logicalOrExpression(); // Start with logical OR (lowest precedence)

    while (currentToken.getType() == TokenTypes::Plus || currentToken.getType() == TokenTypes::Minus ||
           currentToken.getType() == TokenTypes::LogicalAnd || currentToken.getType() == TokenTypes::LogicalOr) {
        TokenTypes op = currentToken.getType();
        
        if (op == TokenTypes::Plus) {
            eat(TokenTypes::Plus);
        } else if (op == TokenTypes::Minus) {
            eat(TokenTypes::Minus);
        } else if (op == TokenTypes::LogicalAnd) {
            eat(TokenTypes::LogicalAnd);
        } else if (op == TokenTypes::LogicalOr) {
            eat(TokenTypes::LogicalOr);
        }

        left = std::make_shared<BinaryExpression>(left, op, logicalOrExpression()); // Chain with logical OR expression
    }

    return left;
}

// Parse a logical OR expression
std::shared_ptr<Statement> Parser::logicalOrExpression() {
    std::shared_ptr<Statement> left = logicalAndExpression();

    while (currentToken.getType() == TokenTypes::LogicalOr) {
        TokenTypes op = TokenTypes::LogicalOr;
        eat(TokenTypes::LogicalOr);
        left = std::make_shared<BinaryExpression>(left, op, logicalAndExpression());
    }

    return left;
}

// Parse a logical AND expression
std::shared_ptr<Statement> Parser::logicalAndExpression() {
    std::shared_ptr<Statement> left = comparisonExpression();

    while (currentToken.getType() == TokenTypes::LogicalAnd) {
        TokenTypes op = TokenTypes::LogicalAnd;
        eat(TokenTypes::LogicalAnd);
        left = std::make_shared<BinaryExpression>(left, op, comparisonExpression());
    }

    return left;
}

// Parse a comparison expression, handling ==, !=, <, <=, >, >=
std::shared_ptr<Statement> Parser::comparisonExpression() {
    std::shared_ptr<Statement> left = term();

    while (currentToken.getType() == TokenTypes::Equals || currentToken.getType() == TokenTypes::NotEquals ||
           currentToken.getType() == TokenTypes::LessThan || currentToken.getType() == TokenTypes::LessEqual ||
           currentToken.getType() == TokenTypes::GreaterThan || currentToken.getType() == TokenTypes::GreaterEqual) {
        TokenTypes op = currentToken.getType();
        
        if (op == TokenTypes::Equals) {
            eat(TokenTypes::Equals);
        } else if (op == TokenTypes::NotEquals) {
            eat(TokenTypes::NotEquals);
        } else if (op == TokenTypes::LessThan) {
            eat(TokenTypes::LessThan);
        } else if (op == TokenTypes::LessEqual) {
            eat(TokenTypes::LessEqual);
        } else if (op == TokenTypes::GreaterThan) {
            eat(TokenTypes::GreaterThan);
        } else if (op == TokenTypes::GreaterEqual) {
            eat(TokenTypes::GreaterEqual);
        }

        left = std::make_shared<BinaryExpression>(left, op, term());
    }

    return left;
}

// Parse a term, handling multiplication, division, modulo, and bitwise operators
std::shared_ptr<Statement> Parser::term() {
    std::shared_ptr<Statement> left = parseUnaryExpression();

    while (currentToken.getType() == TokenTypes::Multiply || currentToken.getType() == TokenTypes::Divide ||
           currentToken.getType() == TokenTypes::Modulo || currentToken.getType() == TokenTypes::BitwiseAnd ||
           currentToken.getType() == TokenTypes::BitwiseOr || currentToken.getType() == TokenTypes::BitwiseXor ||
           currentToken.getType() == TokenTypes::ShiftLeft || currentToken.getType() == TokenTypes::ShiftRight) {

        TokenTypes op = currentToken.getType();

        if (op == TokenTypes::Multiply) {
            eat(TokenTypes::Multiply);
        } else if (op == TokenTypes::Divide) {
            eat(TokenTypes::Divide);
        } else if (op == TokenTypes::Modulo) {
            eat(TokenTypes::Modulo);
        } else if (op == TokenTypes::BitwiseAnd) {
            eat(TokenTypes::BitwiseAnd);
        } else if (op == TokenTypes::BitwiseOr) {
            eat(TokenTypes::BitwiseOr);
        } else if (op == TokenTypes::BitwiseXor) {
            eat(TokenTypes::BitwiseXor);
        } else if (op == TokenTypes::ShiftLeft) {
            eat(TokenTypes::ShiftLeft);
        } else if (op == TokenTypes::ShiftRight) {
            eat(TokenTypes::ShiftRight);
        } 

        left = std::make_shared<BinaryExpression>(left, op, parseUnaryExpression());
    }

    return left;
}

std::shared_ptr<Statement> Parser::parseUnaryExpression() {
    // Handle prefix operators
    if (currentToken.getType() == TokenTypes::Plus ||
        currentToken.getType() == TokenTypes::Minus ||
        currentToken.getType() == TokenTypes::LogicalNot ||
        currentToken.getType() == TokenTypes::Tilde ||
        currentToken.getType() == TokenTypes::Increment ||
        currentToken.getType() == TokenTypes::Decrement) {
        TokenTypes op = currentToken.getType();
        eat(op);
        auto operand = parseUnaryExpression();
        return std::make_shared<UnaryExpression>(op, operand, UnaryExpression::Position::Prefix);
    }

    // Parse primary expression
    auto expr = factor();

    // Handle postfix operators
    while (currentToken.getType() == TokenTypes::Increment ||
           currentToken.getType() == TokenTypes::Decrement) {
        TokenTypes op = currentToken.getType();
        eat(op);
        expr = std::make_shared<UnaryExpression>(op, expr, UnaryExpression::Position::Postfix);
    }

    // Handle 'as' casting (lowest precedence postfix)
    if (currentToken.getType() == TokenTypes::As) {
        eat(TokenTypes::As);
        std::vector<std::string> typeToCastTo = parseType();
        std::shared_ptr<Omniscript::Type> type = Omniscript::resolveType(typeToCastTo);
        if (auto typed = std::dynamic_pointer_cast<TypedStatement>(expr)) {
            typed->setType(type);
        } else {
            expr = std::make_shared<Cast>(expr, type);
        }
    }

    return expr;
}


// Parse a factor, handling literals, identifiers, and parentheses
std::shared_ptr<Statement> Parser::factor() {
    DEBUG_LOG("Factoring a '" + getTokenTypeName(currentToken.getType()) + "' with value '" + currentToken.getValue() + "'.");

    // Handle literals
    std::shared_ptr<Statement> left;

    if (currentToken.getType() == TokenTypes::IntegerLiteral) {
        eat(TokenTypes::IntegerLiteral);
        std::string valueStr = previousToken.getValue();
    
        try {
            long long value = std::stoll(valueStr);
    
            if (value >= std::numeric_limits<int32_t>::min() && value <= std::numeric_limits<int32_t>::max()) {
                left = std::make_shared<IntegerLiteral>(static_cast<int64_t>(value));
            } else if (value >= std::numeric_limits<int64_t>::min() && value <= std::numeric_limits<int64_t>::max()) {
                left = std::make_shared<IntegerLiteral>(static_cast<int64_t>(value));
            } else {
                // Handle BigInt case
                left = std::make_shared<BigInt>(valueStr);
            }
        } catch (const std::out_of_range&) {
            left = std::make_shared<BigInt>(valueStr);
        }
    }

    // Handle float literals (32-bit and 64-bit)
    else if (currentToken.getType() == TokenTypes::FloatLiteral) {
        eat(TokenTypes::FloatLiteral);
        std::string value = previousToken.getValue();

        // Check for 'f' or 'd' suffix to determine float type
        if (!value.empty() && (value.back() == 'f' || value.back() == 'F')) {
            left = std::make_shared<FloatLiteral>(std::stof(value)); // Float32
        } else {
            left = std::make_shared<FloatLiteral>(std::stod(value)); // Default to Float64
        }
    }

    // Handle float literals
    else if (currentToken.getType() == TokenTypes::FloatLiteral) {
        eat(TokenTypes::FloatLiteral);
        left = std::make_shared<FloatLiteral>(std::stof(previousToken.getValue())); // Assuming Float32Bit is your float type
    }
    // Handle integer literals (decimal)
    else if (currentToken.getType() == TokenTypes::IntegerLiteral) {
        eat(TokenTypes::IntegerLiteral);
        left = std::make_shared<IntegerLiteral>(std::stoll(previousToken.getValue())); // Assuming Int32Bit is your integer type
    }
    // Handle hexadecimal literals
    else if (currentToken.getType() == TokenTypes::HexLiteral) {
        eat(TokenTypes::HexLiteral);
        left = std::make_shared<IntegerLiteral>(std::stoll(previousToken.getValue(), nullptr, 16)); // Base 16
    }
    // Handle octal literals
    else if (currentToken.getType() == TokenTypes::OctalLiteral) {
        eat(TokenTypes::OctalLiteral);
        left = std::make_shared<IntegerLiteral>(std::stoll(previousToken.getValue(), nullptr, 8)); // Base 8
    }
    // Handle binary literals
    else if (currentToken.getType() == TokenTypes::BinaryLiteral) {
        eat(TokenTypes::BinaryLiteral);
        left = std::make_shared<IntegerLiteral>(std::stoll(previousToken.getValue(), nullptr, 2)); // Base 2
    }
    // Handle big integers (arbitrary-precision)
    else if (currentToken.getType() == TokenTypes::BigInt) {
        eat(TokenTypes::BigInt);
        left = std::make_shared<BigInt>(previousToken.getValue()); // Assuming BigInt is your arbitrary-precision type
    }
    // Handle string literals
    else if (currentToken.getType() == TokenTypes::StringLiteral) {
        eat(TokenTypes::StringLiteral);
        left = std::make_shared<StringLiteral>(previousToken.getValue());
    } else if (currentToken.getType() == TokenTypes::BitwiseAnd) {
        eat(TokenTypes::BitwiseAnd);
        if (currentToken.getType() == TokenTypes::Identifier) {
            std::string varName = currentToken.getValue();
            eat(TokenTypes::Identifier);
            left = std::make_shared<AddressOf>(varName);
        }
    } else if (currentToken.getType() == TokenTypes::Nullptr) {
        eat(TokenTypes::Nullptr);
        left = std::make_shared<Nullptr>();
    } else if (currentToken.getType() == TokenTypes::Null) {
        eat(TokenTypes::Null);
        left = std::make_shared<Null>();
    }
    // Handle identifiers (variables and functions)
    else if (currentToken.getType() == TokenTypes::Identifier || currentToken.getType() == TokenTypes::New) {
        if (currentToken.getType() == TokenTypes::New) {
            eat(TokenTypes::New);
        }
        left = parseIdentifier();
        DEBUG_LOG("Parsed the identifier and got " + left->toString() + "'.");
    }
    // Handle arrays (e.g., [1, 2, 3])
    else if (currentToken.getType() == TokenTypes::LeftBracket) {
        eat(TokenTypes::LeftBracket);  // Consume the opening bracket
        std::vector<std::shared_ptr<Statement>> items;  // Store the array items

        // Parse array items (comma-separated expressions)
        while (currentToken.getType() != TokenTypes::RightBracket) {
            items.push_back(parseExpression());  // Wrap each item in an Expression
            if (currentToken.getType() == TokenTypes::Comma) {
                eat(TokenTypes::Comma);  // Consume the comma if there are more items
            } else {
                break;
            }
        }

        eat(TokenTypes::RightBracket);  // Consume the closing bracket "]"

        // Store the array directly as a value
        left = std::make_shared<Array>(items);
    }

    // Handle expressions within parentheses
    else if (currentToken.getType() == TokenTypes::LeftParen || currentToken.getType() == TokenTypes::LessThan) {
        int i = 0;
        //Todo: create an overload for tryParseTypeParametersLookahead(i) that takes in no references
        if (tryParseTypeParametersLookahead(i)) {
            parameterType paramTypes = parseTypeParametersForDeclaration();
            if (checkIfLambdaExpression()) {
                left = parseLambdaFunction("", paramTypes);
            }
        } else if (checkIfLambdaExpression()) {
            left = parseLambdaFunction();
        } else {
            eat(TokenTypes::LeftParen);
            left = parseExpression();  // Parse the expression within parentheses
            eat(TokenTypes::RightParen);
        }
    } else if (currentToken.getType() == TokenTypes::Character) {
        char value = currentToken.getValue()[0]; 
        eat(TokenTypes::Character);
        left = std::make_shared<CharacterLiteral>(value);
    } else if (currentToken.getType() == TokenTypes::False) {
        eat(TokenTypes::False);
        left = std::make_shared<BoolLiteral>(false);
    } else if (currentToken.getType() == TokenTypes::True) {
        eat(TokenTypes::True);
        left = std::make_shared<BoolLiteral>(true);
    
    } else if (currentToken.getType() == TokenTypes::BitwiseAnd) {
        eat(TokenTypes::BitwiseAnd);
        std::string varName = currentToken.getValue();
        eat(TokenTypes::Identifier);

        left = std::make_shared<AddressOf>(varName);
    }

    // Parse objects and dictionaries
    // {a = 0, b = 1}
    // {a, b} 
    else if (currentToken.getType() == TokenTypes::LeftBrace) {
        left = parseObject();
    }

    // Handle dot operator for method calls (e.g., object.method())
    while (currentToken.getType() == TokenTypes::Dot) {
        eat(TokenTypes::Dot);  // Consume the dot operator

        // Ensure the next token is an identifier (method name or property)
        if (currentToken.getType() != TokenTypes::Identifier) {
            throw std::runtime_error("Expected an identifier after the '.' operator.");
        }

        std::string methodName = currentToken.getValue();
        eat(TokenTypes::Identifier);  // Consume the method name

        if (currentToken.getType() == TokenTypes::LeftParen || currentToken.getType() == TokenTypes::LessThan) {
            DEBUG_LOG("Got method " + methodName);

            // Parse the method call arguments
            auto args = parseArguments();

            // Create CallMethod for method invocation
            left = std::make_shared<CallMethod>(left, methodName, args);
        } else {
            DEBUG_LOG("Got property " + methodName);
            left = std::make_shared<GetProperty>(left, methodName);
        }
    }
    
    // Parse a parentheses after a string, dictionary, or an array to access a key
    while (currentToken.getType() == TokenTypes::LeftBracket) {
        eat(TokenTypes::LeftBracket);

        std::vector<std::shared_ptr<Statement>> args;    
        args.push_back(parseExpression());

        eat(TokenTypes::RightBracket);
        left = std::make_shared<CallMethod>(left, "get", args);
    }

    // Return the final result wrapped in a BinaryExpression
    // return std::make_shared<BinaryExpression>(left);
    return left;
}

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

// std::shared_ptr<Statement> Parser::parseFunctionCall() {
    

// }

ClassMemberModifiers Parser::parseClassMemberModifiers() {
    ClassMemberModifiers modifiers;

    while (currentToken.getType() == TokenTypes::Private || 
           currentToken.getType() == TokenTypes::Public || 
           currentToken.getType() == TokenTypes::Override ||
           currentToken.getType() == TokenTypes::Static ||
           currentToken.getType() == TokenTypes::Final ||
           currentToken.getType() == TokenTypes::Virtual ||
           currentToken.getType() == TokenTypes::Const) {

        modifiers.isInitialized = true;

        if (currentToken.getType() == TokenTypes::Private) {
            modifiers.access = ClassMemberModifiers::AccessModifier::Private;
            eat(TokenTypes::Private);
        }

        if (currentToken.getType() == TokenTypes::Public) {
            modifiers.access = ClassMemberModifiers::AccessModifier::Public;
            eat(TokenTypes::Public);
        }

        // if (currentToken.getType() == TokenTypes::Protected) {
        //     modifiers.access = MemberModifiers::AccessModifier::Protected;
        //     eat(TokenTypes::Protected);
        // }

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

        if (currentToken.getType() == TokenTypes::Const) {
            modifiers.isConst = true;
            eat(TokenTypes::Const);
        }
    }

    return modifiers;
}


std::shared_ptr<Statement> Parser::parseClass() {
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
        ClassMemberModifiers modifiers = parseClassMemberModifiers();

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
        } else if (currentToken.getType() == TokenTypes::LeftParen) {
            valueExpr = parseLambdaFunction(className + "." + memberName);
        }

        auto member = std::make_shared<ClassMember>(memberName, typeExpr, valueExpr, modifiers);
        members.push_back(member);

        if (currentToken.getType() == TokenTypes::Semicolon) {
            eat(TokenTypes::Semicolon);
        }
    }

    eat(TokenTypes::RightBrace);
    
    if (!hasConstructor) {
        ClassMemberModifiers modifiers;
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
        ClassMemberModifiers modifiers;
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

    return std::make_shared<ConstructClassPrototype>(className, parentClasses, members);
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
            lexer.peekToken(i).getType() == TokenTypes::StringLiteral ||
            lexer.peekToken(i).getType() == TokenTypes::IntegerLiteral ||
            lexer.peekToken(i).getType() == TokenTypes::FloatLiteral ||
            lexer.peekToken(i).getType() == TokenTypes::Colon // Argument type annotation
            ) {
            
            // Check for argument name (identifier)
            if (lexer.peekToken(i).getType() == TokenTypes::Identifier) {
                hasValidArgument = true;
                i++;
            }
            
            // Check for argument type annotation (e.g., a: int)
            if (lexer.peekToken(i).getType() == TokenTypes::Colon) {
                i++; // Skip over the colon
                if (lexer.peekToken(i).getType() == TokenTypes::Identifier) {
                    // Argument type is valid, so skip the type token
                    i++;
                }
            }
            
            // Check for default value (e.g., a: int = 1)
            if (lexer.peekToken(i).getType() == TokenTypes::Assign) {
                i++; // Skip over the =
                if (lexer.peekToken(i).getType() == TokenTypes::IntegerLiteral ||
                    lexer.peekToken(i).getType() == TokenTypes::FloatLiteral ||
                    lexer.peekToken(i).getType() == TokenTypes::StringLiteral) {
                    // Valid default value
                    i++;
                }
            }
            
            // Skip commas between arguments
            if (lexer.peekToken(i).getType() == TokenTypes::Comma) {
                i++;
            }
        }

        // Check if we have reached the closing parenthesis and arrow (=>)
        if (lexer.peekToken(i).getType() == TokenTypes::RightParen && lexer.peekToken(i + 1).getType() == TokenTypes::Arrow) {
            return true;
        }
    }
    return false;
}

bool Parser::checkIfFunctionCall() {
    int i = 0;
    
    DEBUG_LOG(currentToken.getValue());
    // Check if it's a valid function name (identifier)
    if (currentToken.getType() == TokenTypes::Identifier) {
        i++;
    }
    
    DEBUG_LOG(getTokenTypeName(lexer.peekToken(i).getType()) + ' ' + lexer.peekToken(i).getValue());
    // Check for optional type parameters `<T>`
    if (lexer.peekToken(i).getType() == TokenTypes::LessThan) {
        i++; // Move past '<'
        DEBUG_LOG(getTokenTypeName(lexer.peekToken(i).getType()) + ' ' + lexer.peekToken(i).getValue());
        while (true) {
            Token token = lexer.peekToken(i);
            
            DEBUG_LOG(getTokenTypeName(token.getType()) + ' ' + token.getValue());
            if (token.getType() == TokenTypes::Identifier) {
                i++; // Consume type identifier
            } else if (token.getType() == TokenTypes::Comma) {
                i++; // Consume ','
            } else if (token.getType() == TokenTypes::GreaterThan) {
                i++; // End of type parameters
                break;
            } else {
                return false; // Invalid token inside type parameters
            }
        }
    }
    
    // Ensure we have `(` after function name
    DEBUG_LOG(getTokenTypeName(lexer.peekToken(i).getType()) + ' ' + lexer.peekToken(i).getValue());
    if (lexer.peekToken(i).getType() != TokenTypes::LeftParen) {
        return false;
    }
    i++; // Consume '('
    
    // Check function arguments (optional)
    bool hasAtLeastOneArg = false;
    while (true) {
        Token token = lexer.peekToken(i);
        DEBUG_LOG(getTokenTypeName(token.getType()) + ' ' + token.getValue());
        
        if (token.getType() == TokenTypes::RightParen) {
            return true; // End of function call
        }

        if (token.getType() == TokenTypes::Identifier ||
            token.getType() == TokenTypes::StringLiteral ||
            token.getType() == TokenTypes::IntegerLiteral ||
            token.getType() == TokenTypes::FloatLiteral) {
            hasAtLeastOneArg = true;
            i++; // Consume argument
        } else if (token.getType() == TokenTypes::Colon || 
                   token.getType() == TokenTypes::Assign) {
            i++; // Consume type annotation or assignment operator
        } else if (token.getType() == TokenTypes::Comma) {
            if (!hasAtLeastOneArg) return false; // Comma without argument before
            i++; // Consume ','
        } else {
            return false; // Unexpected token
        }
    }
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
        std::shared_ptr<Omniscript::Type> paramType;
        std::shared_ptr<Statement> defaultValue = nullptr;

        // Parse parameter name
        if (currentToken.getType() == TokenTypes::Identifier) {
            paramName = currentToken.getValue();
            eat(TokenTypes::Identifier);
        } else {
            throw std::runtime_error("Expected parameter name.");
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

// Parse function declarations
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
    std::string name = definedName;
    
    if (name.empty()) {
        eat(TokenTypes::Function);
        name = currentToken.getValue();
        eat(TokenTypes::Identifier);
    }

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

    auto body = std::dynamic_pointer_cast<BlockStatement>(parseBlock());

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
                return std::make_shared<BlockStatement>(monomorphizedFunctions);
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

        return std::make_shared<BlockStatement>(monomorphizedFunctions);
    }

    // Normal function without generics
    returnType = Omniscript::resolveType(returnDataType);
    return std::make_shared<FunctionDeclaration>(name, parameters, body, returnType);
}

std::string Parser::generateSpecializedNameForDecleration( 
    const std::string& baseName,
    const std::vector<std::pair<std::string, std::vector<std::string>>>& types
) {
    if (types.empty()) return baseName;

    std::ostringstream oss;
    oss << baseName;

    for (size_t i = 0; i < types.size(); ++i) {
        oss << "_"; // separator after baseName or previous type group
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

            // Only add underscore between parts, not after last
            if (j < concreteType.size() - 1)
                oss << "_";
        }

        // Add double underscore between groups, not after the last group
        if (i < types.size() - 1)
            oss << "__";
    }

    return oss.str();
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
            std::string paramName = currentToken.getValue();  // Argument name (e.g., "b")
            eat(TokenTypes::Identifier);  // Consume identifier

            // Check if there's an assignment
            if (currentToken.getType() == assignOp) {
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
     while (currentToken.getType() == TokenTypes::Multiply || currentToken.getType() == TokenTypes::BitwiseAnd) {
        if (currentToken.getType() == TokenTypes::Multiply) {
            dataTypes.push_back("*");
        } else if (currentToken.getType() == TokenTypes::BitwiseAnd) {
            dataTypes.push_back("&");
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
    while (currentToken.getType() == TokenTypes::Multiply || currentToken.getType() == TokenTypes::BitwiseAnd) {
        if (currentToken.getType() == TokenTypes::Multiply) {
            dataTypes.push_back("*");
        } else if (currentToken.getType() == TokenTypes::BitwiseAnd) {
            dataTypes.push_back("&");
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


std::shared_ptr<Statement> Parser::parseIdentifier() {
    // Parse the root identifier
    std::string rootIdentifier = currentToken.getValue();
    eat(TokenTypes::Identifier);
    
    // Start with the base identifier as the initial statement
    std::shared_ptr<Statement> expr = std::make_shared<GetVariable>(rootIdentifier);
    std::string member = rootIdentifier;

    // Loop to handle member accesses and function calls
    while (true) {
        if (currentToken.getType() == TokenTypes::LeftParen || currentToken.getType() == TokenTypes::LessThan) {
            // Handle function calls with generics
            if (isGenericCallOrConstructor()) {
                std::vector<std::string> typeParams = parseTypeParametersForCall();
                std::string specializedName = generateSpecializedNameForCall(member, typeParams);
                DEBUG_LOG("Generated Specialized Name: " + specializedName);
                
                std::vector<std::shared_ptr<Statement>> args = parseArguments();
                expr = std::make_shared<Call>(expr, specializedName, args);  // Use the specialized name
            } else {
                // Normal function call
                std::vector<std::shared_ptr<Statement>> args = parseArguments();
                expr = std::make_shared<Call>(expr, member, args);
            }
        }
        
        // Object constructor
        else if (currentToken.getType() == TokenTypes::LeftBrace) {
            std::vector<std::shared_ptr<Statement>> args = parseArguments(TokenTypes::LeftBrace, TokenTypes::RightBrace, TokenTypes::Colon);
            expr = std::make_shared<ObjectConstructorStatement>(expr, member, "", args);
        }
        
        // Member access (.)
        else if (currentToken.getType() == TokenTypes::Dot || currentToken.getType() == TokenTypes::ScopeResolution) {
            eat(currentToken.getType()); // Consume the dot or scope resolution operator
            member = currentToken.getValue();
            eat(TokenTypes::Identifier);  // Eat the member name
            DEBUG_LOG(expr->toString());
            expr = std::make_shared<MemberAccess>(expr, member);  // Add a member access
        }

        // Pointer member access (->)
        else if (currentToken.getType() == TokenTypes::Minus && lexer.peekToken(1).getType() == TokenTypes::GreaterThan) {
            eat(TokenTypes::Minus);
            eat(TokenTypes::GreaterThan);
            member = currentToken.getValue();
            eat(TokenTypes::Identifier);  // Eat the member name
            expr = std::make_shared<ArrowAccess>(expr, member);  // Add a pointer member access
        }

        // Index access ([])
        else if (currentToken.getType() == TokenTypes::LeftBracket) {
            eat(TokenTypes::LeftBracket);
            auto index = parseExpression();
            eat(TokenTypes::RightBracket);
            expr = std::make_shared<IndexAccess>(expr, index);  // Add an index access
        }

        // Assignment handling
        else if (isAssignmentExpression(currentToken.getType())) {
            return parseAssignment(expr);
        }

        // Break if no more valid tokens
        else {
            break;
        }
    }

    return expr;
}


std::shared_ptr<Statement> Parser::parseBlock() {
    std::vector<std::shared_ptr<Statement>> statements;

    if (currentToken.getType() == TokenTypes::LeftBrace) {
        eat(TokenTypes::LeftBrace);

        while (currentToken.getType() != TokenTypes::RightBrace && currentToken.getType() != TokenTypes::EOI) {
            statements.push_back(parseStatement());
        }

        eat(TokenTypes::RightBrace);
        if (currentToken.getType() == TokenTypes::Semicolon) {
            eat(TokenTypes::Semicolon);
        }
    } else if (currentToken.getType() == TokenTypes::Return) {
        statements.push_back(parseReturnStatement());

        if (currentToken.getType() != TokenTypes::Semicolon || currentToken.getType() != TokenTypes::Newline) {
            eat(TokenTypes::Semicolon);
        } else {
            eat(currentToken.getType());
        }
    } else {
        statements.push_back(parseStatement());
    }

    return std::make_shared<BlockStatement>(statements);
}

//Todo:: Add proper error messages for various exceptions
std::shared_ptr<ForLoop> Parser::parseForLoop() {
    eat(TokenTypes::For);
    eat(TokenTypes::LeftParen);
    std::shared_ptr<Statement> initialization;
    if (currentToken.getType() == TokenTypes::Semicolon) {
        eat(TokenTypes::Semicolon);
    } else {
        DEBUG_LOG("Parsed the for assignment expression");
        initialization = parseAssignment();
        eat(TokenTypes::Semicolon); 
    }
    std::shared_ptr<Statement> condition;
    if (currentToken.getType() != TokenTypes::Semicolon) {
        condition = parseExpression();
        DEBUG_LOG("Parsed the for loop's condition " + condition->toString());
    }
    eat(TokenTypes::Semicolon);
    std::shared_ptr<Statement> increment;
    if (currentToken.getType() != TokenTypes::RightParen) {
        increment = parseExpression();
        DEBUG_LOG("Parsed the for loop's update expression " + increment->toString());
    }
    DEBUG_LOG("Parsed the for loops update expression");
    eat(TokenTypes::RightParen);
    auto body = std::dynamic_pointer_cast<BlockStatement>(parseBlock());
    DEBUG_LOG("Parsed the for loops body");

    return std::make_shared<ForLoop>(initialization, condition, increment, body);
}

std::shared_ptr<ContinueStatement> Parser::parseContinue() {
    eat(TokenTypes::Continue);
    if (currentToken.getType() != TokenTypes::Semicolon || currentToken.getType() != TokenTypes::Newline) {
        eat(TokenTypes::Semicolon);
    }
    eat(currentToken.getType());
    return std::make_shared<ContinueStatement>();
}

std::shared_ptr<BreakStatement> Parser::parseBreak() {
    eat(TokenTypes::Break);
    if (currentToken.getType() != TokenTypes::Semicolon || currentToken.getType() != TokenTypes::Newline) {
        eat(TokenTypes::Semicolon);
    }
    eat(currentToken.getType());
    return std::make_shared<BreakStatement>();
}

// Parse if statements
std::shared_ptr<Statement> Parser::parseIfStatement() {
    std::vector<std::shared_ptr<Statement>> conditions;
    std::vector<std::shared_ptr<BlockStatement>> bodies;
    std::shared_ptr<BlockStatement> elseBody = nullptr;

    // Parse initial 'if'
    eat(TokenTypes::If);
    eat(TokenTypes::LeftParen);
    auto condition = parseExpression();
    eat(TokenTypes::RightParen);
    auto body = parseBlock();

    conditions.push_back(condition);
    bodies.push_back(std::dynamic_pointer_cast<BlockStatement>(body));

    // Parse any number of 'else if' branches
    while (currentToken.getType() == TokenTypes::Else_if) {
        eat(TokenTypes::Else_if);
        eat(TokenTypes::LeftParen);
        auto elseIfCondition = parseExpression();
        eat(TokenTypes::RightParen);
        auto elseIfBlock = parseBlock();

        conditions.push_back(elseIfCondition);
        bodies.push_back(std::dynamic_pointer_cast<BlockStatement>(elseIfBlock));
    }

    // Optional 'else'
    if (currentToken.getType() == TokenTypes::Else) {
        eat(TokenTypes::Else);
        elseBody = std::dynamic_pointer_cast<BlockStatement>(parseBlock());
    }

    auto statement = std::make_shared<IfStatement>(conditions, bodies, elseBody);
    DEBUG_LOG("Parsed IfStatement with " + std::to_string(conditions.size()) + " branches");
    return statement;
}

// Parse while loops
std::shared_ptr<Statement> Parser::parseWhileStatement() {
    eat(TokenTypes::While); // Consume the 'while' keyword
    // Additional logic to parse the condition and body of the while loop would go here
    eat(TokenTypes::LeftParen);
    auto condition = parseExpression();
    eat(TokenTypes::RightParen);

    auto body = std::dynamic_pointer_cast<BlockStatement>(parseBlock());

    DEBUG_LOG("Parsed while statement");
    return std::make_shared<WhileStatement>(condition, body);
}

// Parse return statements
std::shared_ptr<ReturnStatement> Parser::parseReturnStatement() {
    eat(TokenTypes::Return); // Consume the 'return' keyword
    if (currentToken.getType() != TokenTypes::Semicolon) {
        std::shared_ptr<Statement> value = parseExpression();
        return std::make_shared<ReturnStatement>(value);
        // if (auto stmt = std::dynamic_pointer_cast<TypedStatement>(value)) {
        //     return std::make_shared<ReturnStatement>(value);
        // } else {
        //     console.error("Unable to determine the return type");
        // }
    }
    return std::make_shared<ReturnStatement>();
}

std::shared_ptr<Statement> Parser::parseEnum() {
    eat(TokenTypes::Enum);
    
    bool hasLookup = false;
    bool isEnumClass = false;

    if (currentToken.getType() == TokenTypes::Class) {
        eat(TokenTypes::Class);
        isEnumClass = true;
    }
    
    std::string enumName = currentToken.getValue();
    eat(TokenTypes::Identifier);
    
    if (currentToken.getType() == TokenTypes::LeftParen) {
        eat(TokenTypes::LeftParen);
        if (currentToken.getValue() == "lookup") {
            hasLookup = true;
            eat(TokenTypes::Identifier);
        }
        eat(TokenTypes::RightParen);
    }

    std::vector<std::shared_ptr<EnumValue>> values;
    
    eat(TokenTypes::LeftBrace);
    int currentIndex = 0;
    
    while (currentToken.getType() != TokenTypes::RightBrace) {
        if (currentToken.getType() == TokenTypes::Comma) {
            eat(TokenTypes::Comma);
        }

        if (currentToken.getType() == TokenTypes::Identifier) {
            std::string valueName = currentToken.getValue();
            eat(TokenTypes::Identifier);

            int assignedIndex = currentIndex; // Default index
            
            if (currentToken.getType() == TokenTypes::Assign) {
                eat(TokenTypes::Assign);
                std::shared_ptr<Statement> valueExpr = parseExpression();
                
                // Try to evaluate the expression as an integer
                if (auto intLiteral = std::dynamic_pointer_cast<IntegerLiteral>(valueExpr)) {
                    assignedIndex = intLiteral->getValue();
                } else {
                    console.error("Enum values must be compile-time integers");
                }
            }

            values.push_back(std::make_shared<EnumValue>(valueName, assignedIndex));
            currentIndex = assignedIndex + 1; // Auto-increment for the next entry
        }
    }

    eat(TokenTypes::RightBrace);
    
    // Create the EnumConstructor with the lookup flag
    return std::make_shared<EnumConstructor>(enumName, values, hasLookup, isEnumClass);
}

std::shared_ptr<Statement> Parser::parseStruct() {
    eat(TokenTypes::Struct);
    std::string structName = currentToken.getValue();
    eat(TokenTypes::Identifier);

    std::shared_ptr<Omniscript::Type> thisType = Omniscript::Type::createUserDefinedType(structName, Omniscript::Kind::Struct);
    std::vector<std::shared_ptr<Statement>> body;

    eat(TokenTypes::LeftBrace);
    while (currentToken.getType() != TokenTypes::RightBrace) {
        // Parse method or field
        if (checkIfLambdaExpression()) {
            std::string methodName = structName + "." + currentToken.getValue();
            eat(TokenTypes::Identifier);
            auto func = parseLambdaFunction(methodName); // You can pass structName here for naming
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
            // Unexpected token, maybe throw error or recover
            console.error("Unexpected token in struct body.");
        }
    }
    eat(TokenTypes::RightBrace);
    return std::make_shared<ConstructStructPrototype>(structName, body);
}

std::shared_ptr<Statement> Parser::parseNamespace() {
    eat(TokenTypes::Namespace);
    std::string namespaceName = currentToken.getValue();
    eat(TokenTypes::Identifier);

    // std::vector<std::shared_ptr<Statement>> body = parseBlock();
    
    // auto namespaceObj = std::make_shared<Namespace>(namespaceName, body);
    // auto objectConstructor = std::make_shared<ObjectConstructorStatement>(namespaceObj);
    // return std::make_shared<ConstantAssignment>(namespaceName, objectConstructor);
    return nullptr;
}

// Parse variable assignments
bool Parser::isAssignmentExpression(TokenTypes tokenType) {
    if (tokenType == TokenTypes::Assign || 
        tokenType == TokenTypes::PlusAssign || 
        tokenType == TokenTypes::MinusAssign || 
        tokenType == TokenTypes::DivideAssign || 
        tokenType == TokenTypes::MultiplyAssign || 
        tokenType == TokenTypes::Increment || 
        tokenType == TokenTypes::Decrement) {
        return true;
    }
    return false;
}
std::shared_ptr<Statement> Parser::parseAssignment(parameterType paramTypes) {
    // We assume this is a lambda assignment like: let fn = (x: int) => x * 2;

    TokenTypes variableType = TokenTypes::Let; // Default to let, or adapt as needed
    std::string variableName;
    std::shared_ptr<Omniscript::Type> type = nullptr;

    // Parse `let` or `const`
    if (currentToken.getType() == TokenTypes::Let) {
        eat(TokenTypes::Let);
        variableType = TokenTypes::Let;
    } else if (currentToken.getType() == TokenTypes::Const) {
        eat(TokenTypes::Const);
        variableType = TokenTypes::Const;
    }

    // Parse variable name
    variableName = currentToken.getValue();
    eat(TokenTypes::Identifier);

    // Parse optional type annotation
    if (currentToken.getType() == TokenTypes::Colon) {
        eat(TokenTypes::Colon);
        std::vector<std::string> dataTypes = parseType();
        type = Omniscript::resolveType(dataTypes);
    }

    // Parse assignment
    eat(TokenTypes::Assign);

    // Parse lambda using provided paramTypes
    std::shared_ptr<Statement> lambda = parseLambdaFunction(variableName, paramTypes);

    // Set lambda's name if it's a FunctionDeclaration
    if (auto funcDecl = std::dynamic_pointer_cast<FunctionDeclaration>(lambda)) {
        if (auto named = std::dynamic_pointer_cast<NamedStatement>(funcDecl)) {
            named->setName(variableName);
        }
    }

    if (variableType == TokenTypes::Const) {
        return std::make_shared<createConstant>(variableName, type, lambda);
    }

    return std::make_shared<AssignVariable>(variableName, type, lambda);
}


std::shared_ptr<Statement> Parser::parseAssignment(std::shared_ptr<Statement> assignee) {
    TokenTypes variableType;
    std::string variableName;
    std::shared_ptr<Statement> value;
    std::shared_ptr<Omniscript::Type> type;
    bool isReference = false;
    bool isPointer = false;
    bool isArray = false;
    
    // Parse variable declarations (let or const)
    if (!assignee) {
        if (currentToken.getType() == TokenTypes::Let) {
            eat(TokenTypes::Let);
            variableType = TokenTypes::Let;
        } else if (currentToken.getType() == TokenTypes::Const) {
            eat(TokenTypes::Const);
            variableType = TokenTypes::Const;
        } else {
            variableName = previousToken.getValue();
        }
        variableName = currentToken.getValue();
        eat(TokenTypes::Identifier);
        // Handle type annotation before variable name
        std::vector<std::string> dataTypes;
        if (currentToken.getType() == TokenTypes::Colon) {
            eat(TokenTypes::Colon);
    
            dataTypes = parseType();
        
        } else if (currentToken.getType() == TokenTypes::Assign) {
            eat(TokenTypes::Assign);
            std::string typeName = currentToken.getValue();
    
            std::shared_ptr<Statement> result = parseExpression();
    
            if (auto objConstructor = std::dynamic_pointer_cast<ObjectConstructorStatement>(result)) {
                objConstructor->setInstanceName(variableName);
                return result;
            } else if (auto call = std::dynamic_pointer_cast<Call>(result)) {
                call->setInstanceName(variableName);
                return result;
            }
            
            if (auto funcDecl = std::dynamic_pointer_cast<FunctionDeclaration>(result)) {
                if (auto named = std::dynamic_pointer_cast<NamedStatement>(funcDecl)) {
                    named->setName(variableName);
                }
                return funcDecl;  // Return the function declaration
            }
    
            if (variableType == TokenTypes::Let) {
                return std::make_shared<AssignVariable>(variableName, nullptr, result);
            }
            return std::make_shared<createConstant>(variableName, nullptr, result);
        }    
    
        type = Omniscript::resolveType(dataTypes);
    
        DEBUG_LOG("Parsing assignment for " + getTokenTypeName(variableType) + " '" + variableName + "' with type '" + type->kindName() + "'.");
    }
    
    if (currentToken.getType() != TokenTypes::Semicolon) {
        if (currentToken.getType() == TokenTypes::Increment || currentToken.getType() == TokenTypes::Decrement) {
            DEBUG_LOG("Assigning a unary statement");
            switch (currentToken.getType()) {
                case TokenTypes::Increment:
                    value = std::make_shared<BinaryExpression>(assignee, TokenTypes::Increment);
                    break;
                case TokenTypes::Decrement:
                    value = std::make_shared<BinaryExpression>(assignee, TokenTypes::Decrement);
                    break;
                default:
                    eat(TokenTypes::Semicolon);
            }
            eat(currentToken.getType());
        } else {
            DEBUG_LOG("Assigning a binary or ternary expression");
            Token currentAssignmentOperation = currentToken;
            eat(currentToken.getType());
            
            value = parseExpression(); // Parse right-hand side
            
            switch (currentAssignmentOperation.getType()) {
                case TokenTypes::Assign:
                    break;
                case TokenTypes::PlusAssign:
                    value = std::make_shared<BinaryExpression>(assignee, TokenTypes::Plus, value);
                    break;
                case TokenTypes::MinusAssign:
                    value = std::make_shared<BinaryExpression>(assignee, TokenTypes::Minus, value);
                    break;
                case TokenTypes::DivideAssign:
                    value = std::make_shared<BinaryExpression>(assignee, TokenTypes::Divide, value);
                    break;
                case TokenTypes::MultiplyAssign:
                    value = std::make_shared<BinaryExpression>(assignee, TokenTypes::Multiply, value);
                    break;
                default:
                    eat(TokenTypes::Assign, "Invalid assignment operator: " + getTokenTypeName(currentAssignmentOperation.getType()));
                    break;
            }
        }
    } else {
        value = nullptr; // Handle cases like `let a;`
        if (currentToken.getType() != TokenTypes::Newline &&
            currentToken.getType() != TokenTypes::Semicolon &&
            currentToken.getType() != TokenTypes::EOI) {
            eat(TokenTypes::Semicolon);
        }
    }

    if (!assignee) {
        if (variableType == TokenTypes::Const) {
            return std::make_shared<createConstant>(variableName, type, value);
        }
        return std::make_shared<AssignVariable>(variableName, type, value);
    }
    if (auto varGetter = std::dynamic_pointer_cast<GetVariable>(assignee)) {
        return std::make_shared<AssignVariable>(varGetter->getName(), type, value, true);
    } else if (auto reassignAccess = std::dynamic_pointer_cast<Access>(assignee)) {
        auto accessClone = std::dynamic_pointer_cast<Access>(reassignAccess->clone());
        accessClone->setAssignmentValueTo(value);
        return accessClone;
    }

    console.error("The assignee is unnasignable");
    return nullptr;
}


std::string Parser::parseStringLiteral() {
    std::string value = currentToken.getValue();
    eat(TokenTypes::StringLiteral);

    while (currentToken.getType() == TokenTypes::Plus) {
        eat(TokenTypes::Plus);
        
        if (currentToken.getType() == TokenTypes::StringLiteral) {
            eat(TokenTypes::StringLiteral);
            value += previousToken.getValue();
        } else if (currentToken.getType() == TokenTypes::IntegerLiteral) {
            value += currentToken.getValue();  // Convert number to string
            eat(TokenTypes::IntegerLiteral);
        } else if (currentToken.getType() == TokenTypes::FloatLiteral) {
            value += currentToken.getValue();  // Convert number to string
            eat(TokenTypes::FloatLiteral);
        }
    }

    // DEBUG_LOG("Parsed string literal");

    return value;
}