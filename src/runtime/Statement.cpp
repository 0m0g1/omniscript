#include <omniscript/Core.h>
#include <omniscript/utils.h>
#include <omniscript/runtime/Statement.h>
#include <omniscript/runtime/symboltable.h>
#include <omniscript/omniscript_pch.h>
#include <omniscript/engine/IRGenerator.h>
#include <omniscript/engine/lexer.h>
#include <omniscript/engine/Parser.h>
#include <omniscript/utils.h>
#include <omniscript/engine/IRGenerator.h>

// #include <omniscript/runtime/object.h>
// #include <omniscript/runtime/Class.h>
// #include <omniscript/runtime/Namespace.h>
// #include <omniscript/runtime/Enum.h>
// #include <omniscript/runtime/object.h>
// // #include <omniscript/runtime/Function.h>
// #include <omniscript/runtime/Number.h>
// #include <omniscript/runtime/String.h>
// #include <omniscript/runtime/Pointer.h>

llvm::Value* ImportModule::codegen(IRGenerator& irGen) {
    std::string sourceCode = readFile(path);
    Lexer lexer(sourceCode);
    Parser parser(lexer, irGen);

    parser.setScopeName(alias);
    std::vector<std::shared_ptr<Statement>> statements = parser.Parse();

    irGen.generateModule(alias, statements);

    return nullptr; // No direct IR generation
}

llvm::Value* CreateModule::codegen(IRGenerator& irGen) {
    irGen.importModule(moduleName);
    return nullptr; // Modules themselves don't return a value
}

llvm::Value* PublicMember::codegen(IRGenerator& irGen) {
    return value->codegen(irGen);
}

llvm::Value* Int8Bit::codegen(IRGenerator& generator) {
    console.debug("Creating an 8 bit integer " + std::to_string(value));
    return generator.create8BitInteger(this->value);
}

llvm::Value* Int16Bit::codegen(IRGenerator& generator) {
    return generator.create16BitInteger(this->value);
}

llvm::Value* Int32Bit::codegen(IRGenerator& generator) {
    console.debug("Creating an 8 bit integer with value" + std::to_string(value));
    return generator.create32BitInteger(this->value);
}

llvm::Value* Int64Bit::codegen(IRGenerator& generator) {
    return generator.create64BitInteger(this->value);
}

llvm::Value* Float32Bit::codegen(IRGenerator& generator) {
    return generator.create32BitFloat(this->value);
}

llvm::Value* Float64Bit::codegen(IRGenerator& generator) {
    return generator.create64BitFloat(this->value);
}

// Arbitrary-precision integer (BigInt)
llvm::Value* BigInt::codegen(IRGenerator& generator) {
    return generator.createBigInt(this->value); // Assuming IRGenerator has a method for BigInt
}

llvm::Value* BoolLiteral::codegen(IRGenerator& generator) {
    return nullptr;
}

llvm::Value* StringLiteral::codegen(IRGenerator& generator) {
    return nullptr;
}

// ======================= Assignments and Variable Getters ======================= //
// Assignment
createVariable::createVariable(const std::string &variable, llvm::Type* type, std::shared_ptr<Statement> value)
    : variable(variable), type(type), value(value) {}

llvm::Value* createVariable::codegen(IRGenerator& generator) {
    console.debug("Creating variable " + variable);
    llvm::Value* result = value->codegen(generator);
    console.debug("Created value for " + variable);
    return generator.createVariable(variable, type, result);
}

// Constant Assignment
createConstant::createConstant(const std::string &variable, llvm::Type* type, std::shared_ptr<Statement> value)
: variable(variable), type(type), value(value) {}

llvm::Value* createConstant::codegen(IRGenerator& generator) {
    return generator.createConstant(variable, type, value->codegen(generator));
}

// Dynamic Assignment
createDynamicVariable::createDynamicVariable(const std::string &variable, std::shared_ptr<Statement> value)
    : variable(variable), value(value) {}

llvm::Value* createDynamicVariable::codegen(IRGenerator& generator) {
    return generator.assignDynamicVariable(variable, value->codegen(generator));
}

// Get Variable
GetVariable::GetVariable(const std::string &variable) : variable(variable) {}

llvm::Value* GetVariable::codegen(IRGenerator& generator) {
    return generator.getVariable(variable);
}

// Get Dynamic Variable
GetDynamicVariable::GetDynamicVariable(const std::string &variable) : variable(variable) {}

llvm::Value* GetDynamicVariable::codegen(IRGenerator& generator) {
    return generator.getDynamicVariable(variable);
}


llvm::Value* BreakStatement::codegen(IRGenerator& generator) {
    return nullptr;
}

llvm::Value* ContinueStatement::codegen(IRGenerator& generator) {
    return nullptr;
}

llvm::Value* ObjectConstructorStatement::codegen(IRGenerator& generator) {
    return nullptr;
}

llvm::Value* ForLoop::codegen(IRGenerator& generator) {
    return nullptr;
}

llvm::Value* GetProperty::codegen(IRGenerator& generator) {
    return nullptr;
}

llvm::Value* CallMethod::codegen(IRGenerator& generator) {
    return nullptr;
}

llvm::Value* WhileStatement::codegen(IRGenerator& generator) {
    return nullptr;
}

llvm::Value* TenaryExpression::codegen(IRGenerator& generator) {
    return nullptr;
}

llvm::Value* BinaryExpression::codegen(IRGenerator& generator) {
    return nullptr;
}

llvm::Value* IfStatement::codegen(IRGenerator& generator) {
    return nullptr;
}

llvm::Value* FunctionCallStatement::codegen(IRGenerator& generator) {
    return nullptr;
}

llvm::Value* ReturnStatement::codegen(IRGenerator& generator) {
    return nullptr;
}


// // Helper function to extract values from statements
// std::optional<SymbolTable::ValueType> Expression::evaluate(const SymbolTable::ValueType& object, SymbolTable &scope) {
//     // Variable to store the result
//     SymbolTable::ValueType result;

//     // Check if the object holds a shared pointer to a Statement
//     if (auto statement = std::get_if<std::shared_ptr<Statement>>(&object)) {
//         std::shared_ptr<Statement> stmt = *statement;
//         Omniscript::setPosition(stmt->getPosition());

//         // Process different types of statements
//         if (auto valueStatement = std::dynamic_pointer_cast<Value>(stmt)) {
//             result = valueStatement->evaluate(scope);
//         } else if (auto varStatement = std::dynamic_pointer_cast<Variable>(stmt)) {
//             result = varStatement->evaluate(scope);
//         } else if (auto constAssign = std::dynamic_pointer_cast<ConstantAssignment>(stmt)) {
//             constAssign->execute(scope);
//             result = SymbolTable::ValueType{};
//         } else if (auto constAssign = std::dynamic_pointer_cast<GenericAssignment>(stmt)) {
//             constAssign->execute(scope);
//             result = SymbolTable::ValueType{};
//         } else if (auto block = std::dynamic_pointer_cast<BlockStatement>(stmt)) {
//             block->execute(scope);
//             result = SymbolTable::ValueType{};
//         } else if (auto assign = std::dynamic_pointer_cast<Assignment>(stmt)) {
//             assign->execute(scope);
//             result = SymbolTable::ValueType{};
//         } else if (auto constructor = std::dynamic_pointer_cast<ObjectConstructorStatement>(stmt)) {
//             result = constructor->evaluate(scope);
//         } else if (auto destructor = std::dynamic_pointer_cast<ObjectDestructorStatement>(stmt)) {
//             destructor->execute(scope);
//             result = SymbolTable::ValueType{};
//         } else if (auto functionCall = std::dynamic_pointer_cast<FunctionCallStatement>(stmt)) {
//             result = functionCall->evaluate(scope);
//         } else if (auto returnStatement = std::dynamic_pointer_cast<ReturnStatement>(stmt)) {
//             result = returnStatement->evaluate(scope);
//         } else if (auto ifStatement = std::dynamic_pointer_cast<IfStatement>(stmt)) {
//             result = ifStatement->evaluate(scope);
//         } else if (auto whileLoopStatement = std::dynamic_pointer_cast<WhileStatement>(stmt)) {
//             result = whileLoopStatement->evaluate(scope);
//         } else if (auto forLoopStatement = std::dynamic_pointer_cast<ForLoop>(stmt)) {
//             result = forLoopStatement->evaluate(scope);
//         } else if (auto unaryExpr = std::dynamic_pointer_cast<UnaryExpression>(stmt)) {
//             result = unaryExpr->evaluate(scope).value();
//         } else if (auto binaryExpr = std::dynamic_pointer_cast<BinaryExpression>(stmt)) {
//             auto evalResult = binaryExpr->evaluate(scope);
//             if (evalResult.has_value()) {
//                 console.debug("The Expression's result is " + valueToString(evalResult.value()));
//                 result = evalResult.value();
//             } else {
//                 std::cerr << "Error: Binary expression returned an empty optional!" << std::endl;
//                 return std::nullopt;
//             }
//         } else if (auto ternaryExpression = std::dynamic_pointer_cast<TenaryExpression>(stmt)) {
//             result = ternaryExpression->evaluate(scope);
//         } else if (auto methodCall = std::dynamic_pointer_cast<CallMethod>(stmt)) {
//             result = methodCall->evaluate(scope);
//         } else if (auto propertyCall = std::dynamic_pointer_cast<GetProperty>(stmt)) {
//             result = propertyCall->evaluate(scope);
//         } else {
//             // Error handling for unknown statement types
//             std::cerr << "Error: Unknown statement type encountered." << std::endl;
//             return std::nullopt;
//         }
//     }  else {
//         return object;
//         // console.error("Error: Unsupported type encountered " + valueToString(object) + ".");
//     }

//     // Ensure the result does not contain a shared_ptr<Statement>
//     if (std::holds_alternative<std::shared_ptr<Statement>>(result)) {
//         auto stmt = std::get<std::shared_ptr<Statement>>(result);
//         return Expression::evaluate(stmt, scope).value();
//     }

//     return result;
// }

// SymbolTable::ValueType Value::evaluate(SymbolTable &scope) {

//     if (auto object = std::get_if<std::shared_ptr<Statement>>(&value)) {
//         return Expression::evaluate(object, scope).value();
//     }
//     return value;
// }

// void Assignment::execute(SymbolTable &scope) {
//     // Check if value is a shared_ptr to a Statement
//     if (value.has_value() && std::holds_alternative<std::shared_ptr<Statement>>(*value)) {
//         auto statementPtr = std::get<std::shared_ptr<Statement>>(*value);
//         auto result = Expression::evaluate(statementPtr, scope);
//         scope.set(variable, result);
//     } else {
//         scope.set(variable, value);
//     }

//     if (value.has_value()) { // Check if value is present
//         console.debug("Assigned variable '" + variable + "' with value " + valueToString(value.value()) + " in scope " + scope.name);
//     } else {
//         console.debug("Assigned variable " + variable + " with no value (nullopt) in scope " + scope.name);
//     }
// }

// void ConstantAssignment::execute(SymbolTable &scope) {
//     tempValue = value;
//     // Check if value is a shared_ptr to a Statement
//     if (value.has_value() && std::holds_alternative<std::shared_ptr<Statement>>(*value)) {
//         auto statementPtr = std::get<std::shared_ptr<Statement>>(*value);
//         value = Expression::evaluate(statementPtr, scope).value();
//     }
//     scope.setConstant(variable, value);
//     if (value.has_value()) { // Check if value is present
//         console.debug("Assigned constant '" + variable + "' with value " + valueToString(value.value()));
//     } else {
//         console.debug("Assigned constant " + variable + " with no value (nullopt)");
//     }
// }

// void GenericAssignment::execute(SymbolTable &scope) {
//     tempValue = value;
//     // Check if value is a shared_ptr to a Statement
//     if (value.has_value() && std::holds_alternative<std::shared_ptr<Statement>>(*value)) {
//         auto statementPtr = std::get<std::shared_ptr<Statement>>(*value);
//         value = Expression::evaluate(statementPtr, scope).value();
//     }

//     auto func = std::get<std::shared_ptr<Function>>(value.value());
//     scope.addGenericFunction(variable, func);
//     if (value.has_value()) { // Check if value is present
//         console.debug("Assigned generic function constant '" + variable + "' with value " + valueToString(value.value()));
//     } else {
//         console.debug("Assigned generic function constant " + variable + " with no value (nullopt)");
//     }
// }

// SymbolTable::ValueType Variable::evaluate(SymbolTable &scope) {
//     if (std::holds_alternative<std::string>(variable)) {
//         auto varName = std::get<std::string>(variable);
//         auto variableValue = scope.get(varName);

//         if (variableValue.has_value()) {
//             return variableValue.value();
//         }
//     }
//     // TODO::
//     // else {}

//     return SymbolTable::ValueType{}; // Safe after the check
// }

// SymbolTable::ValueType ReturnStatement::evaluate(SymbolTable &scope) {
//     // Retrieve the return value based on the type in variableReturnValues
//     SymbolTable::ValueType result;

//     // Check if there's a return value
//     if (returnValue.has_value()) {
//         result = returnValue.value();
//         // If the return value is a shared pointer to a statement (like Variable)
//         if (auto statementPtr = std::get_if<std::shared_ptr<Statement>>(&*returnValue)) {
//             console.debug("Returning value: " + valueToString(returnValue.value()));
//             result = Expression::evaluate(*statementPtr, scope).value();  // Use helper to evaluate expressions
//         }
//     } else {
//         console.debug("Returning no value (void)");  // Handle the void case
//         return SymbolTable::ValueType{};
//     }
    
//     return result;
// }


// SymbolTable::ValueType FunctionCallStatement::evaluate(SymbolTable &scope) {
//     showDebugSection("Evaluating a function call");

//     std::shared_ptr<Function> functionPtr = nullptr;
    
//     if (specializedName != "") {
//         console.log("here 1");
//         std::shared_ptr<Function> tempFunctionPtr = scope.getFunction(specializedName);
//         if (tempFunctionPtr) {
//             console.log("here 2");
//             functionPtr = tempFunctionPtr;
//         } else {
//             console.log("here 3");
//             std::string modifiedBaseName = baseName;
//             for (const auto type : types) {
//                 modifiedBaseName += "_any";
//             }
//             console.log(modifiedBaseName);
//             std::shared_ptr<Function> baseGenericFunction = scope.getGenericFunction(modifiedBaseName);
//             if (baseGenericFunction) {
//                 auto genericFunction = baseGenericFunction->clone();
//                 baseGenericFunction->name = specializedName;
//                 scope.addFunction(specializedName, genericFunction);
//                 functionPtr = genericFunction;
//             }
//         }
//     } 
//     // Check if `func` is already a function
//     else if (auto tempFunc = std::get_if<std::shared_ptr<Function>>(&func)) {
//         functionPtr = *tempFunc;
//     }
//     // If `func` is a string, treat it as a function name and look it up
//     else if (auto funcName = std::get_if<std::string>(&func)) {
//         functionPtr = scope.getFunction(*funcName);
//     }
//     // If `func` is an expression, evaluate it and check if it resolves to a function
//     else {
//         auto evaluatedFunc = Expression::evaluate(func, scope);
//         if (evaluatedFunc) {
//             if (auto tempFunc = std::get_if<std::shared_ptr<Function>>(&evaluatedFunc.value())) {
//                 functionPtr = *tempFunc;
//             }
//         }
//     }
    
//     console.log("here 4");
    
//     // Ensure function exists before calling
//     if (!functionPtr) {
//         throw std::runtime_error("Function call failed: function not found or invalid.");
//     }
    
//     // Evaluate function arguments
//     console.log("here 5");
//     std::vector<SymbolTable::ValueType> evaluatedArgs;
//     for (const auto& arg : args) {
//         evaluatedArgs.push_back(Expression::evaluate(arg, scope).value());
//     }
//     console.log("here 6");

//     // Call function and return result
//     return functionPtr->evaluate(scope, evaluatedArgs);
// }

// void BlockStatement::execute(SymbolTable &scope) {

//     for (const auto& statement : statements) {
//         Expression::evaluate(statement, scope);
//     }
// }

// SymbolTable::ValueType IfStatement::evaluate(SymbolTable &scope) {
//     showDebugSection("Executing if statement");
//     SymbolTable localScope;
//     localScope.name = "An if statement's scope";
//     localScope.setParent(&scope);

//     if (conditionIsMet(scope)) {
//         for (const auto &stmnt : body) {
//             auto result = Expression::evaluate(stmnt, localScope);

//             if (auto returnStatement = std::dynamic_pointer_cast<ReturnStatement>(stmnt)) {
//                 return result.value(); // Propagate the return value
//             }

//             if (result != SymbolTable::ValueType{}) {
//                 return result.value();
//             }
//         }
//         return SymbolTable::ValueType{};
//     }

//     for (const auto &branch : branches) {
//         // Execute the appropriate branch based on the condition result
//         if (branch->conditionIsMet(scope)) {
//             for (const auto &stmnt : branch->body) {
//                 auto result = Expression::evaluate(stmnt, localScope);

//                 if (auto returnStatement = std::dynamic_pointer_cast<ReturnStatement>(stmnt)) {
//                     return result.value(); // Propagate the return value
//                 }

//                 if (result != SymbolTable::ValueType{}) {
//                     return result.value();
//                 }
//             }
//             return SymbolTable::ValueType{};
//         }
//     }

//     for (const auto &stmnt : falseBranch) {
//         auto result = Expression::evaluate(stmnt, localScope);

//         if (auto returnStatement = std::dynamic_pointer_cast<ReturnStatement>(stmnt)) {
//             return result.value(); // Propagate the return value
//         }

//         if (result != SymbolTable::ValueType{}) {
//             return result.value();
//         }
//     }

//     return SymbolTable::ValueType{};
// }

// SymbolTable::ValueType ForLoop::evaluate(SymbolTable& scope) {
//     SymbolTable localScope;
//     localScope.name = "a for loop's scope";
//     localScope.setParent(&scope);
//     showDebugSection("Executing a for loop");
//     // Execute initialization
//     Expression::evaluate(initialization, localScope);
//     // Loop while condition evaluates to true
//     while (std::get<bool>(Expression::evaluate(condition, localScope).value())) {
//         // Execute body of the loop
//         for (auto& stmnt : body) {
//             // Check if the statement is a ReturnStatement
//             auto result = Expression::evaluate(stmnt, localScope);
//             if (auto returnStatement = std::dynamic_pointer_cast<ReturnStatement>(stmnt)) {
//                 return result.value(); // Propagate the return value
//             }
//             // Check for 'break' statement
//             if (auto breakStmt = std::dynamic_pointer_cast<BreakStatement>(stmnt)) {
//                 return SymbolTable::ValueType{};  // Exit the loop and function
//             }
//             // Check for 'continue' statement
//             if (auto continueStmt = std::dynamic_pointer_cast<ContinueStatement>(stmnt)) {
//                 Expression::evaluate(increment, localScope); // Skip the current iteration and continue the loop
//                 continue;
//             }
//             // If a statement evaluates to a return value, exit the loop
//             if (result != SymbolTable::ValueType{}) {
//                 return result.value();
//             }
//         }
//         // Execute increment expression after each iteration
//         Expression::evaluate(increment, localScope);
//     }
//     // Return default value if no explicit return was encountered
//     return SymbolTable::ValueType{};
// }

// SymbolTable::ValueType WhileStatement::evaluate(SymbolTable &scope) {
//     // Use the Expression::evaluate() method to evaluate the condition
//     showDebugSection("Executing a while loop");
//     SymbolTable localScope;
//     localScope.name = "a while loop's scope";
//     localScope.setParent(&scope);

//     console.debug("Checking if the condition of the while loop was met");
//     // Continue executing the loop as long as the condition evaluates to true
//     while (std::get<bool>(Expression::evaluate(condition, scope).value())) {
//         console.debug("executing the statements in the while loop");
//         for (auto &stmnt : body) {
//             auto result = Expression::evaluate(stmnt, localScope);

//             if (auto returnStatement = std::dynamic_pointer_cast<ReturnStatement>(stmnt)) {
//                 return result.value(); // Propagate the return value
//             }

//             if (result != SymbolTable::ValueType{}) {
//                 return result.value();
//             }
//         }
//     }
//     console.debug("done executing the statements in the while loop");
//     return SymbolTable::ValueType{};
// }

// //If statements
// // Checks if the conditional in an if statement is met
// bool IfStatement::conditionIsMet(SymbolTable &scope) {
//     // Evaluate the condition, assuming `BinaryExpression` returns a boolean-like result
//     // auto conditionExpr = std::dynamic_pointer_cast<BinaryExpression>(condition);
//     auto conditionResult = Expression::evaluate(condition, scope); //conditionExpr->evaluate(scope);

//     // Check if the condition has a value and convert it to bool
//     bool result = conditionResult.has_value() && std::holds_alternative<bool>(conditionResult.value()) 
//     ? std::get<bool>(conditionResult.value())
//     : false; // default to false if no value or invalid type

//     return result;
// }

// template <typename T>
// bool compare_symbol_variant(const SymbolTable::ValueType& variant, const T& rightElement) {
//     if constexpr (std::is_same_v<std::decay_t<T>, std::string>) {
//         if (auto* str = std::get_if<std::string>(&variant)) {
//             return *str == rightElement;
//         }
//     } else if constexpr (std::is_same_v<std::decay_t<T>, bool>) {
//         if (auto* b = std::get_if<bool>(&variant)) {
//             return *b == rightElement;
//         }
//     } else if constexpr (std::is_same_v<std::decay_t<T>, std::shared_ptr<Array>>) {
//         if (auto* arr = std::get_if<std::shared_ptr<Array>>(&variant)) {
//             return *arr == rightElement;
//         }
//     } else if constexpr (std::is_same_v<std::decay_t<T>, std::shared_ptr<Statement>>) {
//         if (auto* stmt = std::get_if<std::shared_ptr<Statement>>(&variant)) {
//             return *stmt == rightElement;
//         }
//     } else if constexpr (std::is_same_v<std::decay_t<T>, std::vector<std::shared_ptr<Statement>>>) {
//         if (auto* stmtVec = std::get_if<std::vector<std::shared_ptr<Statement>>>(&variant)) {
//             return *stmtVec == rightElement;
//         }
//     } else if constexpr (std::is_same_v<std::decay_t<T>, std::shared_ptr<Object>>) {
//         if (auto* obj = std::get_if<std::shared_ptr<Object>>(&variant)) {
//             return *obj == rightElement;
//         }
//     } else if constexpr (std::is_same_v<std::decay_t<T>, std::vector<std::shared_ptr<Object>>>) {
//         if (auto* objVec = std::get_if<std::vector<std::shared_ptr<Object>>>(&variant)) {
//             return *objVec == rightElement;
//         }
//     } else if constexpr (std::is_same_v<std::decay_t<T>, int>) {
//         if (auto* num = std::get_if<int>(&variant)) {
//             return *num == rightElement;
//         }
//     } else if constexpr (std::is_same_v<std::decay_t<T>, unsigned int>) {
//         if (auto* num = std::get_if<unsigned int>(&variant)) {
//             return *num == rightElement;
//         }
//     } else if constexpr (std::is_same_v<std::decay_t<T>, long>) {
//         if (auto* num = std::get_if<long>(&variant)) {
//             return *num == rightElement;
//         }
//     } else if constexpr (std::is_same_v<std::decay_t<T>, unsigned long>) {
//         if (auto* num = std::get_if<unsigned long>(&variant)) {
//             return *num == rightElement;
//         }
//     } else if constexpr (std::is_same_v<std::decay_t<T>, long long>) {
//         if (auto* num = std::get_if<long long>(&variant)) {
//             return *num == rightElement;
//         }
//     } else if constexpr (std::is_same_v<std::decay_t<T>, unsigned long long>) {
//         if (auto* num = std::get_if<unsigned long long>(&variant)) {
//             return *num == rightElement;
//         }
//     } else if constexpr (std::is_same_v<std::decay_t<T>, float>) {
//         if (auto* num = std::get_if<float>(&variant)) {
//             return *num == rightElement;
//         }
//     } else if constexpr (std::is_same_v<std::decay_t<T>, double>) {
//         if (auto* num = std::get_if<double>(&variant)) {
//             return *num == rightElement;
//         }
//     } else if constexpr (std::is_same_v<std::decay_t<T>, long double>) {
//         if (auto* num = std::get_if<long double>(&variant)) {
//             return *num == rightElement;
//         }
//     } else if constexpr (std::is_same_v<std::decay_t<T>, void*>) {
//         if (auto* ptr = std::get_if<void*>(&variant)) {
//             return *ptr == rightElement;
//         }
//     }

//     // If types don't match, return false
//     return false;
// }

// template <typename T>
// bool compare_element(const SymbolTable::ValueType& element, const T& rightElement) {
//     return compare_symbol_variant(element, rightElement);
// }

// std::optional<SymbolTable::ValueType> UnaryExpression::evaluate(SymbolTable &scope) {
//     return SymbolTable::ValueType{};
// }

// // Binary Expression methods
// std::optional<SymbolTable::ValueType> BinaryExpression::evaluate(SymbolTable &scope) {
//     // console.debug("The left is " + valueToString(isTruthy(left)) + " " + getOperatorString(op) + " " + valueToString(isTruthy(right)));
//     if (!isTruthy(left) && isTruthy(right)) { // If there is no left statement, treat it as unary (e.g., unary minus)
//         auto operandValue = Expression::evaluate(right, scope);  

//         // Check if the operand is present
//         if (!operandValue.has_value()) {
//             throw std::runtime_error("Unary operand missing value.");
//         }

//         // Handle unary minus (negation) -1
//         if (op == TokenTypes::Minus) {
//             if (std::holds_alternative<int>(operandValue.value())) {
//                 return -std::get<int>(operandValue.value());  // Apply unary minus
//             } else if (std::holds_alternative<float>(operandValue.value())) {
//                 return -std::get<float>(operandValue.value());  // Apply unary minus
//             } else {
//                 throw std::runtime_error("Unsupported operand type for unary minus.");
//             }
//         } else if (std::holds_alternative<float>(operandValue.value())) { // Handle if the operand is a float
//             return -std::get<float>(operandValue.value());  // Apply unary minus
//         } else {
//             throw std::runtime_error("Unsupported operand type for unary minus.");
//         }

//     // If there is no right statement, treat it as unary (e.g., increment or decrement)
//     } else if (isTruthy(left) && (op == TokenTypes::Increment || op == TokenTypes::Decrement) && !isTruthy(right)) {
//         auto leftValue = Expression::evaluate(left, scope);
//         if (!leftValue.has_value()) {
//             throw std::runtime_error("Unary expression operand missing value.");
//         }

//         // Apply the unary operator (either increment or decrement)
//         switch (op) {
//             case TokenTypes::Increment: {
//                 if (std::holds_alternative<int>(leftValue.value())) {
//                     int value = std::get<int>(leftValue.value());
//                     return value + 1;  // Increment operation
//                 }
//                 break;
//             }
//             case TokenTypes::Decrement: {
//                 if (std::holds_alternative<int>(leftValue.value())) {
//                     int value = std::get<int>(leftValue.value());
//                     return value - 1;  // Decrement operation
//                 }
//                 break;
//             }
//             default:
//                 throw std::runtime_error("Unsupported unary operator." + getTokenTypeName(op));
//         }
//     // Handle booleans
//     } else if (std::holds_alternative<bool>(left) && op == TokenTypes::Null && !isTruthy(right)) { 
//         if (std::get<bool>(left) == true) { // Extract the bool value and compare
//             return true;
//         }
//         return false;
//     }
//     else if (isTruthy(left) && op == TokenTypes::Null && !isTruthy(right)) {
//         auto result = Expression::evaluate(left, scope).value();
//         return result;
//     } else {
//         // Handle binary operations 
//         // Retrieve values from evaluated statements
//         auto leftValueOpt = Expression::evaluate(left, scope);
//         auto rightValueOpt = Expression::evaluate(right, scope);

//         // Ensure both values are present
//         if (!leftValueOpt || !rightValueOpt) {
//             throw std::runtime_error("Binary expression operands are missing values.");
//         }

//         auto leftValue = leftValueOpt.value();
//         auto rightValue = rightValueOpt.value();

//         // Debug output for the entire statement
//         console.debug("The statement is '" + valueToString(leftValue) + " " + getOperatorString(op) + " " + valueToString(rightValue) + "'");

//         // Handle numeric operations using std::visit
//         auto handleNumericOperation = [&](auto&& left, auto&& right) -> SymbolTable::ValueType {
//             using LeftType = std::decay_t<decltype(left)>;
//             using RightType = std::decay_t<decltype(right)>;

//             // Ensure both are numeric
//             if constexpr (std::is_arithmetic_v<LeftType> && std::is_arithmetic_v<RightType>) {
//                 using CommonType = std::common_type_t<LeftType, RightType>;
//                 CommonType leftNum = static_cast<CommonType>(left);
//                 CommonType rightNum = static_cast<CommonType>(right);

//                 switch (op) {
//                     case TokenTypes::Plus: 
//                         return leftNum + rightNum;
//                     case TokenTypes::Minus: 
//                         return leftNum - rightNum;
//                     case TokenTypes::Multiply: 
//                         return leftNum * rightNum;
//                     case TokenTypes::Divide: 
//                         if (rightNum != 0) 
//                             return leftNum / rightNum;
//                         else 
//                             throw std::runtime_error("Division by zero.");
//                     case TokenTypes::Modulo: 
//                         if constexpr (std::is_integral_v<CommonType>)
//                             return static_cast<CommonType>(std::fmod(leftNum, rightNum));
//                         else
//                             throw std::runtime_error("Modulo not supported for non-integer types.");
//                     case TokenTypes::Equals: 
//                         return leftNum == rightNum;
//                     case TokenTypes::NotEquals: 
//                         return leftNum != rightNum;
//                     case TokenTypes::LessThan: 
//                         return leftNum < rightNum;
//                     case TokenTypes::GreaterThan: 
//                         return leftNum > rightNum;
//                     case TokenTypes::LessEqual: 
//                         return leftNum <= rightNum;
//                     case TokenTypes::GreaterEqual: 
//                         return leftNum >= rightNum;
                    
//                     case TokenTypes::BitwiseXor:
//                     case TokenTypes::BitwiseAnd:
//                     case TokenTypes::BitwiseOr:
//                     case TokenTypes::ShiftLeft:
//                     case TokenTypes::ShiftRight: {
//                         if constexpr (!std::is_integral_v<CommonType>)
//                             throw std::runtime_error("Bitwise operations require integer operands.");
                        
//                         int64_t leftInt = static_cast<int64_t>(leftNum);
//                         int64_t rightInt = static_cast<int64_t>(rightNum);

//                         switch (op) {
//                             case TokenTypes::BitwiseXor: return leftInt ^ rightInt;
//                             case TokenTypes::BitwiseAnd: return leftInt & rightInt;
//                             case TokenTypes::BitwiseOr:  return leftInt | rightInt;
//                             case TokenTypes::ShiftLeft:  return leftInt << rightInt;
//                             case TokenTypes::ShiftRight: return leftInt >> rightInt;
//                             default: break;
//                         }
//                     }
//                     default:
//                         throw std::runtime_error("Unsupported operator for numeric types.");
//                 }
//             } else {
//                 throw std::runtime_error("Invalid types for binary operation.");
//             }
//         };

//         // Use std::visit for numeric handling
//         if (std::holds_alternative<int>(leftValue) || std::holds_alternative<float>(leftValue) ||
//             std::holds_alternative<double>(leftValue) || std::holds_alternative<long>(leftValue) ||
//             std::holds_alternative<unsigned int>(leftValue) || std::holds_alternative<long long>(leftValue) ||
//             std::holds_alternative<unsigned long long>(leftValue)) {
//             return std::visit(handleNumericOperation, leftValue, rightValue);
//         }

//         if (std::holds_alternative<bool>(leftValue) && std::holds_alternative<bool>(rightValue)) {
//             bool leftBool = std::get<bool>(leftValue);
//             bool rightBool = std::get<bool>(rightValue);

//             switch (op) {
//                 case TokenTypes::LogicalAnd:
//                     return leftBool && rightBool;
//                 case TokenTypes::LogicalOr:
//                     return leftBool || rightBool;
//                 case TokenTypes::LogicalXor:
//                     return (leftBool || rightBool) && !(leftBool && rightBool);
//                 case TokenTypes::Equals:
//                     return leftBool == rightBool;
//                 case TokenTypes::NotEquals:
//                     return leftBool != rightBool;
//                 default:
//                     throw std::runtime_error("Unsupported operator for booleans.");
//             }
//         // String and string
//         } else if (std::holds_alternative<std::string>(leftValue) && std::holds_alternative<std::string>(rightValue)) {
//             const std::string& leftString = std::get<std::string>(leftValue);
//             const std::string& rightString = std::get<std::string>(rightValue);

//             // The resulting string of any operation
//             std::string result = leftString;
//             switch (op) {
//                 case TokenTypes::Plus:
//                     return leftString + rightString; // Concatenation
//                 case TokenTypes::Minus:
//                     if (rightString.empty()) return leftString; // Nothing to remove
//                     size_t pos;
//                     // Find and erase all occurrences of `toRemove` in `result`
//                     while ((pos = result.find(rightString)) != std::string::npos) {
//                         result.erase(pos, rightString.length());
//                     }
                    
//                     return result;
//                 case TokenTypes::Equals:
//                     return leftString == rightString;
//                 case TokenTypes::NotEquals:
//                     return leftString != rightString;
//                 case TokenTypes::LessThan:
//                     return leftString < rightString;
//                 case TokenTypes::GreaterThan:
//                     return leftString > rightString;
//                 case TokenTypes::LessEqual:
//                     return leftString <= rightString;
//                 case TokenTypes::GreaterEqual:
//                     return leftString >= rightString;
//                 default:
//                     throw std::runtime_error("Unsupported operator for strings.");
//             }

//         // string and number
//         } else if ((std::holds_alternative<std::string>(leftValue) && std::holds_alternative<int>(rightValue)) ||
//                     (std::holds_alternative<int>(leftValue) && std::holds_alternative<std::string>(rightValue))) {

//             // Initialize variables before the switch statement
//             std::string str = std::holds_alternative<std::string>(leftValue) ? std::get<std::string>(leftValue) : std::get<std::string>(rightValue);
//             int n = std::holds_alternative<int>(leftValue) ? std::get<int>(leftValue) : std::get<int>(rightValue);

//             // Initialize other variables
//             std::string result;
//             bool isNegative = n < 0;
//             if (isNegative) {
//                 // Reverse the string if multiplier is negative
//                 std::reverse(str.begin(), str.end());
//                 n = -n; // Make the multiplier positive for repetition
//             }

//             // Now perform the switch statement
//             switch (op) {
//                 case TokenTypes::Multiply:
//                     // Add full repetitions
//                     for (int i = 0; i < n; ++i) result += str;
//                     return result;

//                 case TokenTypes::Plus:
//                     // Handle string + integer or integer + string
//                     if (std::holds_alternative<std::string>(leftValue)) {
//                         // (string + integer) -> Append integer to string
//                         std::string numAsString = std::to_string(n);
//                         return str + numAsString;
//                     } else {
//                         // (integer + string) -> Prepend integer to string
//                         std::string numAsString = std::to_string(n);
//                         return numAsString + str;
//                     }
//                 case TokenTypes::Equals:
//                     return false;

//                 default:
//                     throw std::runtime_error("Unsupported operator for string and integer.");
//             }
//         } else if ((std::holds_alternative<std::string>(leftValue) && (std::holds_alternative<float>(rightValue) || std::holds_alternative<double>(rightValue))) ||
//                     (std::holds_alternative<float>(leftValue) && std::holds_alternative<std::string>(rightValue)) ||
//                     (std::holds_alternative<double>(leftValue) && std::holds_alternative<std::string>(rightValue))) {

//             // Initialize variables before the switch statement
//             std::string str = std::holds_alternative<std::string>(leftValue) ? std::get<std::string>(leftValue) : std::get<std::string>(rightValue);
//             double n = std::holds_alternative<float>(leftValue) ? std::get<float>(leftValue) :
//                     std::holds_alternative<double>(leftValue) ? std::get<double>(leftValue) : 
//                     std::holds_alternative<float>(rightValue) ? std::get<float>(rightValue) : 
//                     std::get<double>(rightValue);

//             // Initialize other variables
//             std::string result;
//             bool isNegative = n < 0;
//             double fractionalPart = isNegative ? -(n - static_cast<int>(n)) : n - static_cast<int>(n);  // Fractional part for partial repetition
//             int fullRepetitions = static_cast<int>(n);  // Full repetitions from the integer part

//             // Initialize partialLength before the switch statement
//             int partialLength = static_cast<int>(fractionalPart * str.size());

//             // // If negative, reverse the string and make the multiplier positive
//             // if (isNegative) {
//             //     // Reverse the string if multiplier is negative
//             //     std::reverse(str.begin(), str.end());
//             //     n = -n; // Make the multiplier positive for repetition
//             // }

//             // Now perform the switch statement
//             switch (op) {
//                 case TokenTypes::Multiply:
//                     if (n == 0) return std::string(""); // Handle zero multiplier

//                     // Add full repetitions
//                     for (int i = 0; i < fullRepetitions; ++i) {
//                         result += str;
//                     }

//                     // Add fractional repetition (take the portion of the string based on fractional part)
//                     result += str.substr(0, partialLength);

//                     // If the multiplier was negative, reverse the result string back
//                     if (isNegative) {
//                         std::reverse(result.begin(), result.end());
//                     }

//                     return result;

//                 case TokenTypes::Plus:
//                     // Handle string + float/double or float/double + string
//                     if (std::holds_alternative<std::string>(leftValue)) {
//                         // (string + float/double) -> Append float/double to string
//                         std::string numAsString = std::to_string(n);
//                         return str + numAsString;
//                     } else {
//                         // (float/double + string) -> Prepend float/double to string
//                         std::string numAsString = std::to_string(n);
//                         return numAsString + str;
//                     }

//                 default:
//                     throw std::runtime_error("Unsupported operator for string and float/double.");
//             }
//         } else if (auto leftArray = std::dynamic_pointer_cast<Array>(std::get<std::shared_ptr<Object>>(leftValue));
//             auto rightArray = std::dynamic_pointer_cast<Array>(std::get<std::shared_ptr<Object>>(rightValue))) {

//             switch (op) {
//                 case TokenTypes::Plus: {
//                     // Concatenate the arrays
//                     auto result = std::make_shared<Array>(leftArray->elements);
//                     result->elements.insert(result->elements.end(), rightArray->elements.begin(), rightArray->elements.end());
//                     return result;
//                 }
//                 case TokenTypes::Equals:
//                     return leftArray->elements == rightArray->elements; // Compare the elements directly
//                 case TokenTypes::NotEquals:
//                     return leftArray->elements != rightArray->elements; // Inequality
//                 default:
//                     throw std::runtime_error("Unsupported operator for Array.");
//             }
        
//         // Array (op) Value
//         } else if (std::holds_alternative<std::shared_ptr<Object>>(leftValue) &&
//                 isTruthy(rightValue)) {

//             auto leftObject = std::get<std::shared_ptr<Object>>(leftValue);
//             auto leftArray = std::dynamic_pointer_cast<Array>(leftObject);

//             auto rightElement = rightValue;

//             switch (op) {
//                 case TokenTypes::Plus: {
//                     // Append the rightValue to the array
//                     leftArray->elements.push_back(rightElement);
//                     return leftArray;
//                 }
//                 case TokenTypes::Minus: {
//                     // Remove all occurrences of rightValue from the array
//                    leftArray->elements.erase(
//                                 std::remove_if(
//                                     leftArray->elements.begin(),
//                                     leftArray->elements.end(),
//                                     [&rightElement](const auto& element) {
//                                         return compare_element(element, rightElement);
//                                     }),
//                                 leftArray->elements.end()
//                             );
//                     return leftArray;
//                 }
//                 default:
//                     throw std::runtime_error(
//                         "Unsupported operator between Array and Object. Supported Operations are '+' and '-'");
//             }
//         }

//         // Value (op) Array
//         else if (isTruthy(leftValue) &&
//         std::holds_alternative<std::shared_ptr<Array>>(rightValue)) {

//         auto leftElement = leftValue;
//         auto rightArray = std::get<std::shared_ptr<Array>>(rightValue);

//             switch (op) {
//                 case TokenTypes::Plus: {
//                     // Prepend the leftValue to the array
//                     rightArray->elements.insert(rightArray->elements.begin(), leftElement);
//                     return rightArray;
//                 }
//                 default:
//                     throw std::runtime_error("Unsupported operator for Object + Array.");
//             }
//         } else {
//             console.error("Unsupported operand types " + valueToString(leftValue) + " " + getOperatorString(op) + " " +
//             valueToString(rightValue) + " in binary expression.");
//         }

//     } 

//     return std::nullopt;
// }

// //Evaluate a Tenary expression
// SymbolTable::ValueType TenaryExpression::evaluate(SymbolTable& scope) {
//     auto conditionResult = Expression::evaluate(condition, scope);
//     // Check if the condition has a value and convert it to bool
//     bool result = conditionResult.has_value() && std::holds_alternative<bool>(conditionResult.value()) 
//                     ? std::get<bool>(conditionResult.value())
//                     : false; // default to false if no value or invalid type
//     if (result) {
//         return Expression::evaluate(truthy, scope).value();
//     }
//     return Expression::evaluate(falsey, scope).value();
// }

// SymbolTable::ValueType CallMethod::evaluate(SymbolTable &scope) {
//     showDebugSection("Calling the method '" + methodName + "' on object " + valueToString(object));

//     // Lambda to convert a SymbolTable::ValueType to a shared_ptr<Object>
//     auto resolveObjectValue = [&](const SymbolTable::ValueType &val) -> std::shared_ptr<Object> {
//         if (auto objPtr = std::get_if<std::shared_ptr<Object>>(&val)) {
//             return *objPtr;
//         }
//         return primitiveToObject(val);
//     };

//     std::shared_ptr<Object> baseObject;

//     // === 1. Try to resolve the target object from the 'object' variant ===
//     if (std::holds_alternative<std::shared_ptr<Statement>>(object)) {
//         auto stmtPtr = std::get<std::shared_ptr<Statement>>(object);

//         // If it's a Value statement, try to extract the underlying string name.
//         if (auto valueStmt = std::dynamic_pointer_cast<Value>(stmtPtr)) {
//             if (auto objectName = std::get_if<std::string>(&valueStmt->value)) {
//                 // Replace object with the string name.
//                 object = *objectName;
//             }
//         }
//         // If it's a Variable, evaluate it.
//         else if (auto varStmt = std::dynamic_pointer_cast<Variable>(stmtPtr)) {
//             auto objectValue = Expression::evaluate(varStmt, scope).value();
//             baseObject = resolveObjectValue(objectValue);
            
//         } else if (auto getMemberStmt = std::dynamic_pointer_cast<GetProperty>(stmtPtr)) {
//             auto objectMember = Expression::evaluate(getMemberStmt, scope).value();
//             baseObject = resolveObjectValue(objectMember);
//         }
//     }
//     // If object holds a string name, try resolving it from the SymbolTable.
//     else if (std::holds_alternative<std::string>(object)) {
//         std::string objectName = std::get<std::string>(object);
//         auto lookupResult = scope.get(objectName);
//         if (lookupResult.has_value()) {
//             auto objectValue = lookupResult.value();
//             console.debug("Got object '" + valueToString(objectValue) + "'");
//             baseObject = resolveObjectValue(objectValue);
//         }
//     }
//     // If object is still a Statement (another branch), evaluate it.
//     else if (std::holds_alternative<std::shared_ptr<Statement>>(object)) {
//         auto stmt = std::get<std::shared_ptr<Statement>>(object);
//         auto objectValue = Expression::evaluate(stmt, scope).value();
//         baseObject = resolveObjectValue(objectValue);
//     }

//     // === 2. Fallback: Evaluate 'object' directly if baseObject is not yet resolved ===
//     if (!baseObject) {
//         auto objectValue = Expression::evaluate(object, scope).value();
//         baseObject = resolveObjectValue(objectValue);
//     }

//     // === 3. Evaluate method arguments ===
//     std::vector<SymbolTable::ValueType> evaluatedArgs;
//     console.debug("Evaluating the args");
//     console.debug("The args are " + valueToString(arguments));
//     int argPosition = 0;
//     for (const auto &arg : arguments) {
//         console.debug("Arg " + std::to_string(argPosition) + " is a " + valueToString(arg));
//         auto value = Expression::evaluate(arg, scope).value();
//         console.debug(valueToString(arg) + " = " + valueToString(value));
//         evaluatedArgs.push_back(value);
//         argPosition++;
//     }

//     SymbolTable::ValueType result;

//     // === 4. Dispatch to the proper method (class method vs. object method) ===
//     if (auto instance = std::dynamic_pointer_cast<ClassInstance>(baseObject)) {
//         // Retrieve the class method (with any modifiers)
//         auto [methodVariant, modifiers] = instance->getClassInstanceMethod(methodName);
//         if (std::holds_alternative<std::nullptr_t>(methodVariant)) {
//             // Method not found on class instance: return a null value.
//             return nullptr;
//         }
//         // If the method is a C++ lambda wrapper:
//         if (auto func = std::get_if<std::function<SymbolTable::ValueType(const ArgumentDefinition&)>>(&methodVariant)) {
//             result = (*func)(evaluatedArgs);
//         }
//         // Otherwise, if it's a Function statement:
//         else if (auto funcStmt = std::get_if<std::shared_ptr<Function>>(&methodVariant)) {
//             evaluatedArgs.push_back(std::make_shared<ConstantAssignment>("this", instance));
//             (*funcStmt)->evaluate(scope, evaluatedArgs);
//         }
//     } else {
//         // Handle regular objects:
//         console.debug("Calling method '" + methodName + "' on object '" + baseObject->getName() + "'");
//         auto methodVariant = baseObject->getMethod(methodName);
//         if (std::holds_alternative<std::nullptr_t>(methodVariant)) {
//             throw std::runtime_error("Method '" + methodName + "' not found on object '" + baseObject->getName() + "'.");
//         }
//         if (auto func = std::get_if<std::function<SymbolTable::ValueType(const ArgumentDefinition&)>>(&methodVariant)) {
//             result = (*func)(evaluatedArgs);
//         } else if (auto funcStmt = std::get_if<std::shared_ptr<Function>>(&methodVariant)) {
//             result = (*funcStmt)->evaluate(scope, evaluatedArgs);
//         }
//     }

//     return result;
// }



// SymbolTable::ValueType GetProperty::evaluate(SymbolTable& scope) {
//     console.debug("Evaluating a property call");
//     std::shared_ptr<Object> baseObject;

//     // 1. Check if the object is a string identifier.
//     if (auto objectName = std::get_if<std::string>(&object)) {
//         auto result = scope.get(*objectName); // Retrieve from the symbol table
//         if (result.has_value()) { // Check if the value exists
//             auto objectValue = result.value();
//             console.debug("Found object '" + valueToString(objectValue) + "'");

//             // If it's already an object, use it
//             if (auto objPtr = std::get_if<std::shared_ptr<Object>>(&objectValue)) {
//                 baseObject = *objPtr;
//             } else {
//                 // Convert primitives to objects if needed
//                 baseObject = primitiveToObject(objectValue);
//             }
//         }
//     }

//     // 2. Evaluate the object if not resolved via SymbolTable.
//     if (!baseObject) {
//         auto objectValue = Expression::evaluate(object, scope).value();
//         baseObject = primitiveToObject(objectValue);
//     }

//     // 3. Handle property retrieval.
//     SymbolTable::ValueType result;
//     if (auto instance = std::dynamic_pointer_cast<ClassInstance>(baseObject)) {
//         console.debug("Accessing property '" + propertyName + "' on class instance");

//         // Retrieve property from the class instance
//         auto [pvalue, mods] = instance->getClassInstanceProperty(propertyName);
//         if (!std::holds_alternative<std::nullptr_t>(pvalue)) {
//            result = pvalue;
//         } else {
//             auto [mvalue, mods] = instance->getClassInstanceMethod(propertyName);
//             if (!std::holds_alternative<std::nullptr_t>(mvalue)) {
//                 result = "\033[3mf\033[0m() => {}";
//             } else {
//                 result = nullptr;
//             }
//         }

//         if (std::get_if<std::nullptr_t>(&result)) {
//             throw std::runtime_error("Property '" + propertyName + "' not found on class instance.");
//         }
//     } else {
//         console.debug("Accessing property '" + propertyName + "' on object '" + baseObject->getName() + "'");

//         // Retrieve property from the regular object
//         result = baseObject->getProperty(propertyName);

//         if (std::get_if<std::nullptr_t>(&result)) {
//             throw std::runtime_error("Property '" + propertyName + "' not found on object '" + baseObject->getName() + "'.");
//         }
//     }

//     console.debug("Retrieved property value: " + valueToString(result));
//     return result;
// }


// SymbolTable::ValueType ObjectConstructorStatement::evaluate(SymbolTable &scope) {
//     // Evaluate constructor arguments
//     showDebugSection("Constructing an object");
//     std::vector<SymbolTable::ValueType> evaluatedArgs;
//     console.debug("Parsing the arguments");
//     for (const auto& arg : constructorArgs) {
//         auto result = Expression::evaluate(arg, scope);
//         if (result.has_value()) {
//             evaluatedArgs.push_back(result.value());
//         } else {
//             console.error("Argument could not be evaluated");
//         }
//     }

//     // Handle an Array (which should be an Object)
//     if (auto arry = std::get_if<std::shared_ptr<Array>>(&obj)) {
//         console.debug("Constructing an array");
//         std::vector<SymbolTable::ValueType> evaluatedElements;
//         for (auto &elem : (*arry)->elements) {
//             auto evalResult = Expression::evaluate(elem, scope);
//             if (evalResult.has_value()) {
//                 evaluatedElements.push_back(evalResult.value());
//             } else {
//                 console.error("Failed to evaluate array element.");
//             }
//         }
//         auto newArray = std::make_shared<Array>(evaluatedElements);
//         return newArray; // Ensure it's returned as an Object
//     } 

//     // Handle an Object
//     if (auto object = std::get_if<std::shared_ptr<Object>>(&obj)) {
//         console.debug("Constructing an object");
//         if (*object) { // Ensure the shared pointer is valid

//             // Also check if the Object is an EnumObject
//             if (auto enumObj = std::dynamic_pointer_cast<Enum>(*object)) {
//                 console.debug("Constructing an enum");
//                 for (const auto& [key, value] : enumObj->properties) {
//                     auto result = Expression::evaluate(value, scope);
//                     if (result.has_value()) {
//                         enumObj->properties[key] = result.value();
//                     } else {
//                         console.error("Failed to evaluate enum value: " + key);
//                     }
//                 }
//             }  else {
//                 console.debug("Constructing another object type");
//                 for (const auto& [key, value] : (*object)->properties) {
//                     if (std::holds_alternative<std::shared_ptr<Statement>>(value)) {
//                         auto result = Expression::evaluate(value, scope);
//                         if (result.has_value()) {
//                             (*object)->setProperty(key, result.value());
//                         } else {
//                             console.error("Failed to evaluate property: " + key);
//                         }
//                     } else {
//                         (*object)->setProperty(key, value);
//                     }
//                 }
//             }
//             return *object; // Ensure we return the processed object
//         } else {
//             console.error("Null Object pointer during construction");
//         }
//     }


//     // Handle a Namespace: Evaluate the Namespace body
//     if (auto namespaceObj = std::get_if<std::shared_ptr<Namespace>>(&obj)) {
//         console.debug("Constructing a namespace");
//         if (*namespaceObj) {
//             console.debug("Evaluating Namespace body");
//             // Evaluate the body of the Namespace in the provided scope
//             (*namespaceObj)->evaluate(scope);
//             return *namespaceObj; // Return the shared_ptr to the Namespace
//         } else {
//             console.error("Null Namespace pointer during construction");
//         }
//     }


//     // Handle a Statement
//     if (auto statement = std::get_if<std::shared_ptr<Statement>>(&obj)) {
//         console.debug("Evaluating a Statement for construction");
//         auto result = Expression::evaluate(*statement, scope);
//         if (result.has_value()) {
//             // Ensure result is an Object
//             if (auto objPtr = std::get_if<std::shared_ptr<Object>>(&result.value())) {
//                 if (objPtr && *objPtr) { // Ensure valid shared pointer
//                     if (auto classType = std::dynamic_pointer_cast<Class>(*objPtr)) {
//                         console.debug("Constructing a ClassInstance");
//                         auto instance = std::make_shared<ClassInstance>(classType);
//                         int constructorPos = 0;
//                         for (const auto& constructor : instance->constructors) {
//                             if (constructorPos == 0) {
//                                 std::vector<SymbolTable::ValueType> convertedArgs;
//                                 for (const auto& arg : constructorArgs) {
//                                     convertedArgs.push_back(arg); // Convert each Statement to a ValueType
//                                 }

//                                 constructor->addParameter("this", std::make_shared<ConstantAssignment>("this", instance));
//                                 constructor->evaluate(scope, convertedArgs);
//                             }
//                         }
//                         return std::static_pointer_cast<Object>(instance); // Ensure consistent return type
//                     } else {
//                         console.error("Resulting Object is not a Class");
//                     }
//                 } else {
//                     console.error("Null Object pointer in evaluation");
//                 }
//             } else {
//                 console.error("Evaluation result is not an Object");
//             }
//         } else {
//             console.error("Failed to evaluate Statement");
//         }
//     }

//     console.error("Invalid object type for construction");
//     return nullptr; // Return nullptr explicitly instead of returning `obj` directly
// }



// void ObjectDestructorStatement::execute(SymbolTable &scope) {
//     // Retrieve the object from the scope
//     auto objectValue = scope.get(variableName);
//     if (!objectValue) {
//         throw std::runtime_error("Variable " + variableName + " not found");
//     }

//     // Attempt to extract a shared_ptr<Object> from the variant
//     auto objectPtr = std::get_if<std::shared_ptr<Object>>(&*objectValue);
//     if (!objectPtr || !*objectPtr) {
//         throw std::runtime_error(variableName + " is not an object");
//     }

//     auto object = *objectPtr;

//     // Call the destructor method if defined
//     // if (object->hasMethod("~destructor")) {
//     //     object->callMethod("~destructor", {});
//     // }

//     // Remove the object from the scope
//     scope.unset(variableName);
// }
