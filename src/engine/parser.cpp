#include <omniscript/Core.h>
#include <omniscript/utils.h>
#include <omniscript/runtime/object.h>
#include <omniscript/engine/parser.h>
#include <omniscript/engine/lexer.h>
#include <omniscript/engine/tokens.h>
#include <omniscript/runtime/Statement.h>
#include <omniscript/runtime/symboltable.h>
#include <omniscript/mainthreadrunner.h>
#include <omniscript/omniscript_pch.h>
#include <omniscript/engine/IRGenerator.h>

// built - in objects
#include <omniscript/runtime/Function.h>
#include <omniscript/runtime/Class.h>
#include <omniscript/runtime/Namespace.h>
#include <omniscript/runtime/Enum.h>
#include <omniscript/runtime/Number.h>
#include <omniscript/runtime/String.h>


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

// #include <llvm/IR/LLVMContext.h>
// #include <llvm/IR/Module.h>
// #include <llvm/IR/IRBuilder.h>
// #include <llvm/Support/raw_ostream.h>


// Entry point for parsing the program
void Parser::Parse() {
    initializeEnvironment();
    parseProgram(); // Start parsing the program
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
    if (debugMode) {
        showDebugSection("Parsing the script");
    }   

    while (currentToken.getType() != TokenTypes::EOI) {
        statements.push_back(parseStatement()); // Parse each statement in the program
    }


    showDebugSection("Done parsing the script");
    console.debug();
}

void Parser::Interprete() {
    console.debug();
    showDebugSection("Executing the script");
    if (executeStatements) {
        for (const auto &statement : statements) {
            if (statement) {
                Expression::evaluate(statement, scope);
            }
            PROCESS_MAIN_THREAD_EVENTS();
        }
    }

    checkAllThreadsCompleted();

    console.debug("***=================================***" );
    console.debug("   Done executing the script 😁😁😊" );
    console.debug("***=================================***" ); 
}

void Parser::Compile() {
    console.debug();
    showDebugSection("Compiling the script");
    
    // IRGenerator irGen;

    // for (const auto &statement : statements) {
    //     if (statement) {
    //         irGen.generateIR(statement);
    //     }
    // }

    // irGen.printIR();
    
    console.debug("***=================================***" );
    console.debug("   Done compiling the script 😁😁😊" );
    console.debug("***=================================***" ); 
}


void Parser::checkAllThreadsCompleted() {
    do {
         std::this_thread::sleep_for(std::chrono::milliseconds(1000/60));
    } while (!Omniscript::allThreadsDone);
    // Get the object from the scope as a shared_ptr to the base class
    // std::shared_ptr<Object> baseCanvas = std::get<std::shared_ptr<Object>>(scope.get("canvas").value());
    
    // // Cast the base class shared_ptr to the derived class shared_ptr
    // std::shared_ptr<CanvasObject> canvas = std::dynamic_pointer_cast<CanvasObject>(baseCanvas);

    // if (canvas) {
    //     // If the cast succeeded, check if the canvas is still running
    //     while (canvas->isRunning()) {
    //         PROCESS_MAIN_THREAD_EVENTS();
    //         // Optionally add a small delay here to avoid a busy-wait loop
    //         std::this_thread::sleep_for(std::chrono::milliseconds(20));
    //     }
    // } else {
    //     console.error("Failed to cast to CanvasObject.");
    // }
}

// Function to change the scope of the current parser
void Parser::setScope(const SymbolTable &otherScope) {
    this->scope = otherScope;
}

// Helper function to consume a token if it matches the expected type
void Parser::eat(TokenTypes expectedType, const std::string& err) {
    // Keep track of your position in the current file
    Omniscript::setPosition(currentToken.getLine(), currentToken.getColumn(), "main.os");
    if (currentToken.getType() == expectedType) {
        previousToken = currentToken;
        currentToken = lexer.getNextToken(); // Move to the next token
    } else {
        std::string errorMessage = "[Parser Error] \n Expected token type: " 
        + getTokenTypeName(expectedType) 
        + " at line: " + std::to_string(currentToken.getLine()) 
        + " column: " + std::to_string(currentToken.getColumn()) 
        + " got token type " + getTokenTypeName(currentToken.getType()) 
        + " instead. \n\n";

        
        if (err != "") {
            errorMessage += "\n\n" + err;
        }
        console.error(errorMessage);
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
        console.debug(message);  // Using console.debug to output the debug message
    }

    switch (currentToken.getType()) {
        case TokenTypes::Function:
            statement = parseFunctionDeclaration();
            break;
        case TokenTypes::Identifier:
            statement = parseIdentifier();
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
    std::shared_ptr<Statement> left = factor();

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

        left = std::make_shared<BinaryExpression>(left, op, factor());
    }

    return left;
}

// Parse a factor, handling literals, identifiers, and parentheses
std::shared_ptr<Statement> Parser::factor() {
    console.debug("factoring a " + getTokenTypeName(currentToken.getType()));

    SymbolTable::ValueType current;
    
    // Handle unary minus (negation)
    if (currentToken.getType() == TokenTypes::Minus) {
        eat(TokenTypes::Minus);  // Consume the minus sign
        auto right = factor();   // Parse the following factor
        return std::make_shared<BinaryExpression>(nullptr, TokenTypes::Minus, right); // Apply unary minus
    }

    // Handle increment and decrement operators
    if (currentToken.getType() == TokenTypes::Increment || currentToken.getType() == TokenTypes::Decrement) {
        TokenTypes op = currentToken.getType();  // Capture the operator type
        eat(op);  // Consume the operator
        std::shared_ptr<Statement> operand = factor(); // Parse the operand (the variable or expression)
        return std::make_shared<BinaryExpression>(operand, op);  // Create a UnaryExpression
    }

    // Handle literals
    std::shared_ptr<Statement> left;

    if (currentToken.getType() == TokenTypes::IntegerLiteral) {
        eat(TokenTypes::IntegerLiteral);
        left = std::make_shared<Value>(std::stoi(previousToken.getValue()));
    }
    // Handle float literals
    else if (currentToken.getType() == TokenTypes::FloatLiteral) {
        eat(TokenTypes::FloatLiteral);
        left = std::make_shared<Value>(std::stof(previousToken.getValue()));
    }
    else if (currentToken.getType() == TokenTypes::HexLiteral) {
        eat(TokenTypes::HexLiteral);
        left = std::make_shared<Value>(std::stoll(previousToken.getValue(), nullptr, 16)); // Base 16
    }
    else if (currentToken.getType() == TokenTypes::OctalLiteral) {
        eat(TokenTypes::OctalLiteral);
        left = std::make_shared<Value>(std::stoll(previousToken.getValue(), nullptr, 8)); // Base 8
    }
    else if (currentToken.getType() == TokenTypes::BinaryLiteral) {
        eat(TokenTypes::BinaryLiteral);
        left = std::make_shared<Value>(std::stoll(previousToken.getValue(), nullptr, 2)); // Base 2
    }
    // Handle string literals
    else if (currentToken.getType() == TokenTypes::StringLiteral) {
        eat(TokenTypes::StringLiteral);
        left = std::make_shared<Value>(previousToken.getValue());
    }
    // Handle identifiers (variables and functions)
    else if (currentToken.getType() == TokenTypes::Identifier) {
        console.info(currentToken.getValue());
        left = parseIdentifier(); // This should return a Statement (already parsed)
        console.info(valueToString(left));
    }
    // Handle arrays (e.g., [1, 2, 3])
    else if (currentToken.getType() == TokenTypes::LeftBracket) {
        eat(TokenTypes::LeftBracket);  // Consume the opening bracket
        std::vector<SymbolTable::ValueType> items;  // Store the array items

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
        left = std::make_shared<ObjectConstructorStatement>(std::make_shared<Array>(items));
    }

    // Handle expressions within parentheses
    else if (currentToken.getType() == TokenTypes::LeftParen) {
        console.warn(valueToString(left));
        if (checkIfLambdaExpression()) {
            left = std::make_shared<Value>(parseLambdaFunction());
        } else {
            eat(TokenTypes::LeftParen);
            left = parseExpression();  // Parse the expression within parentheses
            eat(TokenTypes::RightParen);
        }
    } else if (currentToken.getType() == TokenTypes::False) {
        eat(TokenTypes::False);
        left = std::make_shared<Value>(false);
    } else if (currentToken.getType() == TokenTypes::True) {
        eat(TokenTypes::True);
        left = std::make_shared<Value>(true);
    
    // Parse objects and dictionaries
    // {a = 0, b = 1}
    // {a, b}
    } else if (currentToken.getType() == TokenTypes::LeftBrace) {
        left = parseObject();
    } else if (currentToken.getType() == TokenTypes::New) {
        eat(TokenTypes::New);
        std::string objectClassName = currentToken.getValue();
        eat(TokenTypes::Identifier);
        auto [types, args] = parseArguments();
        auto objectType = std::make_shared<Variable>(objectClassName);
        left = std::make_shared<ObjectConstructorStatement>(objectType, args);
    }  else if (currentToken.getType() == TokenTypes::Null) {
        left = nullptr;
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
            console.debug("Got method " + methodName);

            // Parse the method call arguments
            auto [types, args] = parseArguments();

            // Create CallMethod for method invocation
            left = std::make_shared<CallMethod>(left, methodName, args);
        } else {
            console.debug("Got property " + methodName);
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


// Parse a full binary expression
// Parse the logical operators
std::shared_ptr<Statement> Parser::parseExpression() {
    std::shared_ptr<Statement> left = parseBinaryExpression();

    if (currentToken.getType() == TokenTypes::QuestionMark) {
        eat(TokenTypes::QuestionMark);
        std::shared_ptr<Statement> truthy = parseExpression();
        eat(TokenTypes::Colon);
        std::shared_ptr<Statement> falsey = parseExpression();
        return std::make_shared<TenaryExpression>(left, truthy, falsey);
    }

    return left;
}

std::shared_ptr<Statement> Parser::parseTenaryExpression() {
    std::shared_ptr<Statement> condition = parseExpression();
    if (currentToken.getType() == TokenTypes::QuestionMark) {
        eat(TokenTypes::QuestionMark);
        std::shared_ptr<Statement> truthy = parseExpression();
        eat(TokenTypes::Colon);
        std::shared_ptr<Statement> falsey = parseExpression();
        return std::make_shared<TenaryExpression>(condition, truthy, falsey);
    }
    return std::make_shared<BinaryExpression>(condition);
}

std::shared_ptr<Statement> Parser::parseObject() {
    eat(TokenTypes::LeftBrace);

    bool isDictionary = true;
    auto object = std::make_shared<Object>(); // Start with a generic Object

    while (currentToken.getType() != TokenTypes::RightBrace) {
        std::string propertyName;
        SymbolTable::ValueType propertyValue;

        // Accept identifiers, string literals, and numbers as keys
        if (currentToken.getType() == TokenTypes::Identifier || 
            currentToken.getType() == TokenTypes::StringLiteral || 
            currentToken.getType() == TokenTypes::IntegerLiteral) {
            
            // Convert numbers to strings for consistent key handling
            if (currentToken.getType() == TokenTypes::IntegerLiteral) {
                propertyName = std::to_string(std::stoi(currentToken.getValue()));
            } else {
                propertyName = currentToken.getValue();
            }

            // If the key is an identifier, it cannot be part of a dictionary
            if (currentToken.getType() == TokenTypes::Identifier) {
                isDictionary = false;
            }
    
            eat(currentToken.getType());

            // Check for a colon to determine key-value pair
            if (currentToken.getType() == TokenTypes::Colon) {
                eat(TokenTypes::Colon);
                propertyValue = parseExpression(); // Parse the value expression
            } else {
                // If there's no colon, it behaves like an object property
                isDictionary = false;
                propertyValue = std::make_shared<Variable>(propertyName);
            }

            // Set the property on the object
            object->setProperty(propertyName, propertyValue);
        } else {
            eat(TokenTypes::RightBrace, "Invalid property name in object or dictionary.");
        }

        // Handle commas or terminate on the right brace
        if (currentToken.getType() == TokenTypes::Comma) {
            eat(TokenTypes::Comma);
        } else if (currentToken.getType() != TokenTypes::RightBrace) {
            eat(TokenTypes::RightBrace, "Expected ',' or '}' in object or dictionary.");
        }
    }
    
    eat(TokenTypes::RightBrace);

    // Convert to Dictionary if all keys are valid for dictionary
    if (isDictionary) {
        auto dictionary = std::make_shared<Dictionary>();
        for (const auto& [key, value] : object->properties) {
            dictionary->setProperty(key, value);
        }
        return std::make_shared<ObjectConstructorStatement>(dictionary);
    } 
    return std::make_shared<ObjectConstructorStatement>(object);
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
    auto new_Class = std::make_shared<Class>(className);
    
    eat(TokenTypes::Identifier);

    std::vector<std::pair<std::string, std::string>> types;

    if (currentToken.getType() == TokenTypes::LessThan) {
        types = parseTypeParametersForDeclaration();
    }

    console.debug("Parsing a class " + className);

    if (currentToken.getType() == TokenTypes::Colon) {
        eat(TokenTypes::Colon);

        while (currentToken.getType() != TokenTypes::LeftBrace) {
            if (currentToken.getType() == TokenTypes::Comma) {
                eat(TokenTypes::Comma);
            }
            if (currentToken.getType() == TokenTypes::Public || currentToken.getType() == TokenTypes::Private) {
                eat(currentToken.getType());
            }
            std::string parentClassName = currentToken.getValue();
            eat(TokenTypes::Identifier);
            new_Class->classNames.push_back(parentClassName);
        }
    }
    
    eat(TokenTypes::LeftBrace);

    bool hasConstructor = false;
    bool hasDestructor = false;

    while (currentToken.getType() != TokenTypes::RightBrace) {
        // Parse modifiers before method name
        ClassMemberModifiers preModifiers = parseClassMemberModifiers();

        if (currentToken.getType() == TokenTypes::Identifier && currentToken.getValue() == "constructor") {
            if (hasConstructor) {
                console.error("Class " + className + " has multiple constructors.");
                return nullptr;
            }
            hasConstructor = true;
            eat(TokenTypes::Identifier);

            auto [paramNames, parameters] = parseParameters();

            // Parse modifiers after parameters
            ClassMemberModifiers postModifiers = parseClassMemberModifiers();

            if (preModifiers.isInitialized || postModifiers.isInitialized) {
                console.error("The constructor takes in no modifiers.");
                return nullptr;
            }

            std::vector<std::shared_ptr<Statement>> body = parseBlock();

            auto constructor = std::make_shared<Function>("constructor", paramNames, parameters, body);
            constructor->addParameter("this", nullptr);

            new_Class->addConstructor(constructor);
        } else if (currentToken.getType() == TokenTypes::Identifier && currentToken.getValue() == "destructor") {
            if (hasDestructor) {
                console.error("Class " + className + " has multiple destructors.");
                return nullptr;
            }
            hasDestructor = true;
            eat(TokenTypes::Identifier);

            auto [paramNames, parameters] = parseParameters();

            // Parse modifiers after parameters
            ClassMemberModifiers postModifiers = parseClassMemberModifiers();

            if (preModifiers.isInitialized || postModifiers.isInitialized) {
                console.error("The destructor takes in no modifiers.");
                return nullptr;
            }

            auto body = parseBlock();
            auto destructor = std::make_shared<Function>("destructor", paramNames, parameters, body);
            destructor->addParameter("this", nullptr);
            new_Class->addDestructor(destructor);
        } else if (currentToken.getType() == TokenTypes::Identifier) {
            std::string methodName = currentToken.getValue();
            eat(TokenTypes::Identifier);

            auto [paramNames, parameters] = parseParameters();

            // Parse modifiers after parameters
            ClassMemberModifiers postModifiers = parseClassMemberModifiers();

            if (preModifiers.isInitialized && postModifiers.isInitialized) {
                console.error("Modifiers should appear only before or after the method name, not both.");
                return nullptr;
            }

            // Use the modifiers (either preModifiers or postModifiers)
            ClassMemberModifiers methodModifiers = preModifiers.isInitialized ? preModifiers : postModifiers;

            auto body = parseBlock();
            auto method = std::make_shared<Function>(methodName, paramNames, parameters, body);

            if (methodModifiers.shouldOverride) {
                auto [methodToOverride, parentModifiers] = new_Class->getClassMethod(methodName);

                if (std::holds_alternative<std::nullptr_t>(methodToOverride)) {
                    console.error("Method " + methodName + " marked as override but does not override any virtual method.");
                }
                if (parentModifiers.isFinal) {
                    console.error("Method " + methodName + " is a final member of its parent class and cannot be overridden. Remove the 'final' keyword from the parent class.");
                }
            }

            method->addParameter("this", nullptr);
            new_Class->addClassMethod(methodName, method, methodModifiers);
        } else {
            console.error("Unexpected token in class definition.");
            return nullptr;
        }
    }

    eat(TokenTypes::RightBrace); // End of class body

    return std::make_shared<ConstantAssignment>(className, std::make_shared<ObjectConstructorStatement>(new_Class));
}

SymbolTable::ValueType Parser::parseFunctionArrow() {
    eat(TokenTypes::Arrow);
    // if (currentToken.getType() == TokenTypes::Assign) {
    //     eat(TokenTypes::Assign);
    //     eat(TokenTypes::GreaterThan);
    // } else if (currentToken.getType() == TokenTypes::Minus) {
    //     if (currentToken.getType() == TokenTypes::Minus) {
    //         eat(TokenTypes::Minus);
    //     }
    //     eat(TokenTypes::GreaterThan);
    // } else {
    //     eat(TokenTypes::Assign, "Expected an arrow after lambda paramters '=>', '->' or '-->' followed by the return type or method body.");
    // }

    if (currentToken.getType() == TokenTypes::Identifier) {
        eat(currentToken.getType());
    }

    return SymbolTable::ValueType{};
}

std::shared_ptr<Function> Parser::parseLambdaFunction() {
    auto [paramNames, params] = parseParameters();
    
    SymbolTable::ValueType returnType = parseFunctionArrow();

    std::vector<std::shared_ptr<Statement>> body = parseBlock();
    
    return std::make_shared<Function>("lambda", paramNames, params, body);
}

bool Parser::checkIfLambdaExpression() {
    if (currentToken.getType() == TokenTypes::LeftParen) {
        int i = 1;
        while (
            lexer.peekToken(i).getType() == TokenTypes::Identifier ||
            lexer.peekToken(i).getType() == TokenTypes::Comma || 
            lexer.peekToken(i).getType() == TokenTypes::Assign ||
            lexer.peekToken(i).getType() == TokenTypes::StringLiteral ||
            lexer.peekToken(i).getType() == TokenTypes::IntegerLiteral ||
            lexer.peekToken(i).getType() == TokenTypes::FloatLiteral
            ) {
            i++; // Skip identifiers and commas
        }
        if (lexer.peekToken(i).getType() == TokenTypes::RightParen && lexer.peekToken(i + 1).getType() == TokenTypes::Arrow) {
            return true;
        }
    }
    return false;
}

bool Parser::checkIfFunctionCall() {
    int i = 0;
    
    console.info(currentToken.getValue());
    // Check if it's a valid function name (identifier)
    if (currentToken.getType() == TokenTypes::Identifier) {
        i++;
    }
    
    console.info(getTokenTypeName(lexer.peekToken(i).getType()) + ' ' + lexer.peekToken(i).getValue());
    // Check for optional type parameters `<T>`
    if (lexer.peekToken(i).getType() == TokenTypes::LessThan) {
        i++; // Move past '<'
        console.info(getTokenTypeName(lexer.peekToken(i).getType()) + ' ' + lexer.peekToken(i).getValue());
        while (true) {
            Token token = lexer.peekToken(i);
            
            console.info(getTokenTypeName(token.getType()) + ' ' + token.getValue());
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
    console.info(getTokenTypeName(lexer.peekToken(i).getType()) + ' ' + lexer.peekToken(i).getValue());
    if (lexer.peekToken(i).getType() != TokenTypes::LeftParen) {
        return false;
    }
    i++; // Consume '('
    
    // Check function arguments (optional)
    bool hasAtLeastOneArg = false;
    while (true) {
        Token token = lexer.peekToken(i);
        console.info(getTokenTypeName(token.getType()) + ' ' + token.getValue());
        
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


std::pair<std::vector<std::string>, std::vector<std::shared_ptr<Statement>>> Parser::parseParameters() {
    eat(TokenTypes::LeftParen); // Start of parameters

    std::vector<std::string> paramNames;
    std::vector<std::shared_ptr<Statement>> parameters;

    while (currentToken.getType() != TokenTypes::RightParen && currentToken.getType() != TokenTypes::EOI) {
        if (currentToken.getType() == TokenTypes::Identifier) {
            std::string paramName = currentToken.getValue();
            eat(TokenTypes::Identifier);
            paramNames.push_back(paramName);

            std::shared_ptr<Statement> defaultValue = nullptr;
            if (currentToken.getType() == TokenTypes::Assign) {
                eat(TokenTypes::Assign);
                defaultValue = parseExpression(); // Parse the default value
            }

            // Add assignment for default value or implicit null
            parameters.push_back(std::make_shared<Assignment>(paramName, defaultValue));
        }

        if (currentToken.getType() == TokenTypes::Comma) {
            eat(TokenTypes::Comma); // Consume comma between parameters
        }
    }

    console.debug("Got " + valueToString(paramNames.size()) + " parameters.");
    eat(TokenTypes::RightParen); // End of parameters

    return {paramNames, parameters};
}

// Parse function declarations
std::shared_ptr<Statement> Parser::parseFunctionDeclaration() {
    eat(TokenTypes::Function); // Consume the 'function' keyword

    std::string name = currentToken.getValue(); // Function name
    eat(TokenTypes::Identifier);

    std::vector<std::pair<std::string, std::string>> types; // Generic types

    if (currentToken.getType() == TokenTypes::LessThan) {
        types = parseTypeParametersForDeclaration();
    }

    auto [paramNames, parameters] = parseParameters();
    std::vector<std::shared_ptr<Statement>> body = parseBlock(); // Parse function body
    
    std::vector<std::shared_ptr<Statement>> monomorphizedFunctions;

    if (!types.empty()) {
        // Generate specialized function name
        std::string typeSuffix;
        bool isGeneric = false;
        for (const auto& type : types) {
            typeSuffix += "_" + type.second; // Append each type
            if (type.second == "any" || type.second == "variant") {
                isGeneric = true;
            }
        }

        std::string specializedName = name + typeSuffix; // Example: add_int_string_float

        auto func = std::make_shared<Function>(
            specializedName, paramNames, parameters, body, std::nullopt, types
        );

        if (isGeneric) {
            monomorphizedFunctions.push_back(std::make_shared<GenericAssignment>(specializedName, func));
        } else {
            monomorphizedFunctions.push_back(std::make_shared<ConstantAssignment>(specializedName, func));
        }

        // Return all monomorphized functions as a block
        return std::make_shared<BlockStatement>(monomorphizedFunctions);
    }

    // If no generics, just return a normal function assignment
    auto func = std::make_shared<Function>(name, paramNames, parameters, body);
    return std::make_shared<ConstantAssignment>(name, func);
}

std::string Parser::generateSpecializedNameForDecleration(
    const std::string &baseName, 
    const std::vector<std::pair<std::string, std::string>> &types
) {
    if (types.empty()) return baseName;

    std::ostringstream oss;
    oss << baseName << "_";
    for (size_t i = 0; i < types.size(); ++i) {
        oss << types[i].second; // Use type name
        if (i < types.size() - 1) oss << "_";
    }
    
    console.warn(oss.str());
    return oss.str();
}

std::string Parser::generateSpecializedNameForCall(
    const std::string &baseName, 
    const std::vector<std::string> &typeParams
) {
    if (typeParams.empty()) return baseName;

    std::ostringstream oss;
    oss << baseName << "_";
    for (size_t i = 0; i < typeParams.size(); ++i) {
        oss << typeParams[i]; // Append each type parameter
        if (i < typeParams.size() - 1) oss << "_";
    }

    return oss.str();
}


std::pair<std::vector<std::string>, std::vector<std::shared_ptr<Statement>>> 
Parser::parseArguments() {
    std::vector<std::string> types;
    console.log("Parsing type parameters for a function call");

    if (currentToken.getType() == TokenTypes::LessThan) {
        types = parseTypeParametersForCall();
    }
    console.log("Done parsing type parameters for a function call");

    console.log("Parsing the arguments");
    std::vector<std::shared_ptr<Statement>> args;
    eat(TokenTypes::LeftParen);

    while (currentToken.getType() != TokenTypes::RightParen && currentToken.getType() != TokenTypes::EOI) { 
        // Ensure we don't get stuck in an infinite loop
        if (currentToken.getType() == TokenTypes::Identifier && lexer.peekToken().getType() == TokenTypes::Assign) {
            args.push_back(parseStatement());
        } else if (checkIfFunctionCall()) {
            console.warn("parsing the function " + currentToken.getValue());
            args.push_back(parseIdentifier());
        }else {
            args.push_back(parseExpression());
        }

        // Ensure comma consumption is correct
        if (currentToken.getType() == TokenTypes::Comma) {
            eat(TokenTypes::Comma);
            if (currentToken.getType() == TokenTypes::RightParen) {
                console.error("Unexpected comma before closing parenthesis.");
                throw std::runtime_error("Syntax error: Trailing comma in argument list.");
            }
        } else if (currentToken.getType() != TokenTypes::RightParen) {
            console.log(previousToken.getValue());
            console.error("Expected ',' or ')' but found " + getTokenTypeName(currentToken.getType()));
            throw std::runtime_error("Syntax error: Expected ',' or ')' in argument list.");
        }
    }

    // Ensure we actually close the argument list
    if (currentToken.getType() != TokenTypes::RightParen) {
        console.error("Expected ')' but found " + currentToken.getValue());
        throw std::runtime_error("Syntax error: Expected ')' at end of argument list.");
    }
    eat(TokenTypes::RightParen);

    console.log("Done parsing the arguments");
    return std::make_pair(types, args);
}

std::vector<std::pair<std::string, std::string>> Parser::parseTypeParametersForDeclaration() {
    std::vector<std::pair<std::string, std::string>> typeParams; // (Type, Constraint)

    if (currentToken.getType() == TokenTypes::LessThan) { // `<T>`
        eat(TokenTypes::LessThan);
        while (currentToken.getType() == TokenTypes::Identifier) {
            std::string typeName = currentToken.getValue();
            eat(TokenTypes::Identifier);

            std::string constraint = "any"; // Default: no constraint

            if (currentToken.getType() == TokenTypes::Extends) { // `extends`
                eat(TokenTypes::Extends);

                // Check for `variant` and `any` token types
                if (currentToken.getType() == TokenTypes::Variant) {
                    constraint = "variant";
                } else if (currentToken.getType() == TokenTypes::Any) {
                    constraint = "any";
                } else {
                    constraint = currentToken.getValue(); // Regular type constraint
                }
                eat(currentToken.getType()); // Consume the token
            }

            typeParams.emplace_back(typeName, constraint);

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
    console.log(valueToString(typeParams));
    return typeParams;
}


std::shared_ptr<Statement> Parser::parseIdentifier() {
    // Parse the root identifier
    std::string rootIdentifier = currentToken.getValue();
    eat(TokenTypes::Identifier);
    
    // Start with the base identifier as the initial statement

    std::shared_ptr<Statement> previousStatement = std::make_shared<Variable>(rootIdentifier);
    std::shared_ptr<Statement> statement = std::make_shared<Variable>(rootIdentifier);

    if (currentToken.getType() == TokenTypes::Assign || 
        currentToken.getType() == TokenTypes::PlusAssign || 
        currentToken.getType() == TokenTypes::MinusAssign || 
        currentToken.getType() == TokenTypes::DivideAssign || 
        currentToken.getType() == TokenTypes::MultiplyAssign || 
        currentToken.getType() == TokenTypes::Increment || 
        currentToken.getType() == TokenTypes::Decrement) {
        return parseAssignment();
    }

    std::string previousIdentifier;
    std::string currentIdentifier = currentToken.getValue();
    std::string member;

    while (true) {
        if (currentToken.getType() == TokenTypes::Dot) {
            eat(TokenTypes::Dot);
            member = currentToken.getValue();
            eat(TokenTypes::Identifier);

            if (currentToken.getType() == TokenTypes::LeftParen || currentToken.getType() == TokenTypes::LessThan) {
                auto [types, args] = parseArguments();

                previousStatement = statement;

                if (!types.empty()) {
                    std::string specializedMethodName = generateSpecializedNameForCall(member, types);
                    statement = std::make_shared<CallMethod>(statement, specializedMethodName, args);
                } else {
                    statement = std::make_shared<CallMethod>(statement, member, args);
                }
            } else {
                previousStatement = statement;
                statement = std::make_shared<GetProperty>(statement, member);
            }
        } else if (currentToken.getType() == TokenTypes::Assign) {
            eat(TokenTypes::Assign);
            if (checkIfLambdaExpression()) {
                std::vector<std::shared_ptr<Statement>> args;
                auto propertyName = std::make_shared<Value>(member);
                std::shared_ptr<Function> value = parseLambdaFunction();
                args.push_back(propertyName);
                args.push_back(std::make_shared<Value>(value));
                statement = std::make_shared<CallMethod>(previousStatement, "setMethod", args);
            } else {
                std::vector<std::shared_ptr<Statement>> args;
                auto propertyName = std::make_shared<Value>(member);
                std::shared_ptr<Statement> value = parseExpression();
                args.push_back(propertyName);
                args.push_back(value);
                statement = std::make_shared<CallMethod>(previousStatement, "setProperty", args);
            }
        } else if (
            currentToken.getType() == TokenTypes::BitwiseXorAssign ||
            currentToken.getType() == TokenTypes::BitwiseAndAssign ||
            currentToken.getType() == TokenTypes::BitwiseOrAssign ||
            currentToken.getType() == TokenTypes::ShiftLeftAssign ||
            currentToken.getType() == TokenTypes::ShiftRightAssign
            ) {

            TokenTypes assignType; // Store the assignment operator
            if (currentToken.getType() == TokenTypes::BitwiseXorAssign) {
                assignType = TokenTypes::BitwiseXor;
            } else if (currentToken.getType() == TokenTypes::BitwiseXorAssign) {
                assignType = TokenTypes::BitwiseAnd;
            } else if (currentToken.getType() == TokenTypes::BitwiseXorAssign) {
                assignType = TokenTypes::BitwiseOr;
            } else if (currentToken.getType() == TokenTypes::ShiftLeftAssign) {
                assignType = TokenTypes::ShiftLeft;
            } else if (currentToken.getType() == TokenTypes::ShiftRightAssign) {
                assignType = TokenTypes::ShiftRight;
            }
            eat(currentToken.getType());
            
            std::vector<std::shared_ptr<Statement>> args;
            auto propertyName = std::make_shared<Value>(member);
            std::shared_ptr<Statement> value = parseExpression();

            // Get the current value of the variable
                auto currentValue = std::make_shared<CallMethod>(
                    previousStatement, "getProperty",
                    std::vector<std::shared_ptr<Statement>>{propertyName}
                );

                // Create an expression: property = property <op> value
                auto result = std::make_shared<BinaryExpression>(currentValue, assignType, value);

                args.push_back(propertyName);
                args.push_back(result);
                statement = std::make_shared<CallMethod>(previousStatement, "setProperty", args);
                }   else if (currentToken.getType() == TokenTypes::LeftBracket) {
            eat(TokenTypes::LeftBracket);
            std::vector<std::shared_ptr<Statement>> args;
            std::shared_ptr<Statement> index = parseExpression();
            previousStatement = statement;
            args.push_back(index);
            eat(TokenTypes::RightBracket);
            if (currentToken.getType() == TokenTypes::Assign) {
                eat(TokenTypes::Assign);
                std::shared_ptr<Statement> value = parseExpression();
                args.push_back(value);
                statement = std::make_shared<CallMethod>(previousStatement, "set", args);
            } else {
                statement = std::make_shared<CallMethod>(statement, "get", args);
            }
        } else if (currentToken.getType() == TokenTypes::LeftParen || currentToken.getType() == TokenTypes::LessThan) {
            auto [types, args] = parseArguments();
            if (!types.empty()) {
                std::string name = generateSpecializedNameForCall(rootIdentifier, types);
                if (rootIdentifier != name) {
                    console.log(rootIdentifier + "__" + name);
                    statement = std::make_shared<FunctionCallStatement>(previousStatement, args, rootIdentifier, name, types);
                } else {
                    statement = std::make_shared<FunctionCallStatement>(previousStatement, args);
                }
            } else {
                statement = std::make_shared<FunctionCallStatement>(previousStatement, args);
            }
        } else {
            break;
        }
        previousIdentifier = currentIdentifier;
    }
    
    return statement;
}



std::vector<std::shared_ptr<Statement>> Parser::parseBlock() {
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

    return statements;
}

std::shared_ptr<ForLoop> Parser::parseForLoop() {
    eat(TokenTypes::For);
    eat(TokenTypes::LeftParen);
    std::shared_ptr<Statement> initialization = parseAssignment();
    eat(TokenTypes::Semicolon);
    std::shared_ptr<Statement> condition = parseExpression();
    eat(TokenTypes::Semicolon);
    std::shared_ptr<Statement> increment = parseStatement();
    eat(TokenTypes::RightParen);
    std::vector<std::shared_ptr<Statement>> body = parseBlock();

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
    eat(TokenTypes::If); // Consume the 'if' keyword
    // Additional logic to parse the condition and body of the if statement would go here

    eat(TokenTypes::LeftParen);
    auto condition = parseExpression();
    eat(TokenTypes::RightParen);

    auto body = parseBlock();
    std::vector<std::shared_ptr<IfStatement>> branches;
    std::vector<std::shared_ptr<Statement>> falseBranch;

    // ParseBlock checks for the if statements body and also "{" and "}"
    while (currentToken.getType() == TokenTypes::Else_if) {
        eat(TokenTypes::Else_if);
        eat(TokenTypes::LeftParen);
        auto elseIfCondition = parseExpression();
        eat(TokenTypes::RightParen);

        auto elseIfBlock = parseBlock();

        // Add this 'else if' condition and block to the falseBranch vector
        branches.push_back(std::make_shared<IfStatement>(elseIfCondition, elseIfBlock));
    }

     // Handle a single 'else' branch
    if (currentToken.getType() == TokenTypes::Else) {
        eat(TokenTypes::Else);

        falseBranch = parseBlock();
    }

    auto statement = std::make_shared<IfStatement>(condition, body, branches, falseBranch);

    console.debug("Parsed if statement");
    return statement;
}



// Parse while loops
std::shared_ptr<Statement> Parser::parseWhileStatement() {
    eat(TokenTypes::While); // Consume the 'while' keyword
    // Additional logic to parse the condition and body of the while loop would go here
    eat(TokenTypes::LeftParen);
    auto condition = parseExpression();
    eat(TokenTypes::RightParen);

    auto body = parseBlock();

    console.debug("Parsed while statement");
    return std::make_shared<WhileStatement>(condition, body);
}

// Parse return statements
std::shared_ptr<ReturnStatement> Parser::parseReturnStatement() {
    eat(TokenTypes::Return); // Consume the 'return' keyword
    SymbolTable::ValueType value = parseExpression();
    return std::make_shared<ReturnStatement>(value);
}

std::shared_ptr<Statement> Parser::parseEnum() {
    eat(TokenTypes::Enum);
    std::string enumName = currentToken.getValue();
    eat(TokenTypes::Identifier);

    std::vector<std::string> keys;
    std::vector<std::shared_ptr<Statement>> values;

    eat(TokenTypes::LeftBrace);
    int i = 0;
    while(currentToken.getType() != TokenTypes::RightBrace) {
        if (currentToken.getType() == TokenTypes::Comma) {
            i++;
            eat(TokenTypes::Comma);
        }

        if (currentToken.getType() == TokenTypes::Identifier) {
            std::string valueName = currentToken.getValue();
            keys.push_back(valueName);
            eat(TokenTypes::Identifier);
            if (currentToken.getType() == TokenTypes::Assign) {
                eat(TokenTypes::Assign);
                std::shared_ptr<Statement> value = parseExpression();
                values.push_back(value);
            } else {
                values.push_back(std::make_shared<Value>(i));
            }
        }
    }
    eat(TokenTypes::RightBrace);
    auto newEnum = std::make_shared<Enum>(enumName, keys, values);
    auto constructor = std::make_shared<ObjectConstructorStatement>(newEnum);
    return std::make_shared<ConstantAssignment>(enumName, constructor);
}


std::shared_ptr<Statement> Parser::parseNamespace() {
    eat(TokenTypes::Namespace);
    std::string namespaceName = currentToken.getValue();
    eat(TokenTypes::Identifier);

    std::vector<std::shared_ptr<Statement>> body = parseBlock();
    
    auto namespaceObj = std::make_shared<Namespace>(namespaceName, body);
    auto objectConstructor = std::make_shared<ObjectConstructorStatement>(namespaceObj);
    return std::make_shared<ConstantAssignment>(namespaceName, objectConstructor);
}

// Parse variable assignments
std::shared_ptr<Statement> Parser::parseAssignment() {
    TokenTypes variableType;
    std::string variableName;

    // Parse variable declarations (let or const)
    if (currentToken.getType() == TokenTypes::Let) {
        eat(TokenTypes::Let);
        variableType = TokenTypes::Let;
        variableName = currentToken.getValue();
        eat(TokenTypes::Identifier);
    } else if (currentToken.getType() == TokenTypes::Const) {
        eat(TokenTypes::Const);
        variableType = TokenTypes::Const;
        variableName = currentToken.getValue();
        eat(TokenTypes::Identifier);
    } else {
        variableName = previousToken.getValue();
    }

    console.debug("Parsing an assignment for " + getTokenTypeName(variableType) + " " + variableName);

    SymbolTable::ValueType dataType;
    if (currentToken.getType() == TokenTypes::Colon) {
        eat(TokenTypes::Colon);
        dataType = currentToken.getValue();
        eat(TokenTypes::Identifier);
    }

    std::shared_ptr<Statement> value;
    if (currentToken.getType() != TokenTypes::Semicolon) {
        if (currentToken.getType() == TokenTypes::Increment ||
            currentToken.getType() == TokenTypes::Decrement) { // Check if the imediate next operator is an increment (++) or decrement operator (--)
            console.debug("Assigning a unary statement");
            switch (currentToken.getType()) {
                case TokenTypes::Increment:
                    value = std::make_shared<BinaryExpression>(std::make_shared<Variable>(variableName), TokenTypes::Increment);
                    break;
                case TokenTypes::Decrement:
                    value = std::make_shared<BinaryExpression>(std::make_shared<Variable>(variableName), TokenTypes::Decrement);
                    break;
                default:
                    eat(TokenTypes::Semicolon);
            }
            eat(currentToken.getType());
        } else {
            console.debug("Assigning a binary or tenary expression");
            Token currentAssignmentOperation = currentToken;
            eat(currentToken.getType());

            value = parseTenaryExpression();
            
            switch (currentAssignmentOperation.getType()) {
                case TokenTypes::Assign:
                    break;
                case TokenTypes::PlusAssign:
                    value = std::make_shared<BinaryExpression>(std::make_shared<Variable>(variableName), TokenTypes::Plus, value);
                    break;
                case TokenTypes::MinusAssign:
                    value = std::make_shared<BinaryExpression>(std::make_shared<Variable>(variableName), TokenTypes::Minus, value);
                    break;
                case TokenTypes::DivideAssign:
                    value = std::make_shared<BinaryExpression>(std::make_shared<Variable>(variableName), TokenTypes::Divide, value);
                    break;
                case TokenTypes::MultiplyAssign:
                    value = std::make_shared<BinaryExpression>(std::make_shared<Variable>(variableName), TokenTypes::Multiply, value);
                    break;
                default:
                    eat(TokenTypes::Assign, "The current operator " + getTokenTypeName(currentAssignmentOperation.getType()) +
                    " at line : " + std::to_string(currentAssignmentOperation.getLine()) + " column: " +
                    std::to_string(currentAssignmentOperation.getColumn()) +" is an invalid assigment operator. \n" +
                    "Accepted assignment operators are '=', '+=', '-=', '/=/, and '*='." );
                    break;
            }
        }

    } else {
        value = nullptr; // For statements like `let a;` or `const a;`
        if (currentToken.getType() != TokenTypes::Newline &&
            currentToken.getType() != TokenTypes::Semicolon &&
            currentToken.getType() != TokenTypes::EOI) {
            eat(TokenTypes::Semicolon);
        }
    }
    
    if (variableType == TokenTypes::Const) {
        return std::make_shared<ConstantAssignment>(variableName, value);
    }
    return std::make_shared<Assignment>(variableName, value);
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

    // console.debug("Parsed string literal");

    return value;
}

/*
std::shared_ptr<lambda> parseLambda()  {
    eat(tokenTypes::RightParen);
    eat(tokenTypes::LeftParen);
    eat(tokenTypes::Assign);
    eat(tokenTypes::Greater);
    std::vector<std::shared_ptr<Statement>> statements = parseBlock();

    return value;
}
*/
