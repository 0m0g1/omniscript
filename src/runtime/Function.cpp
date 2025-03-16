// // #include <omniscript/runtime/Function.h>

// void Function::checkTypeConstraints(const std::vector<SymbolTable::ValueType>& args) const {
//     if (typeParameters.empty()) return;

//     if (args.size() != typeParameters.size()) {
//         throw std::runtime_error("Type parameter count mismatch.");
//     }

//     for (size_t i = 0; i < typeParameters.size(); i++) {
//         auto [typeName, constraint] = typeParameters[i];

//         // Example constraint checking (extend for more rules)
//         if (constraint == "Number" && !std::holds_alternative<double>(args[i]) && !std::holds_alternative<int>(args[i])) {
//             throw std::runtime_error("Argument does not satisfy type constraint: " + constraint);
//         }
//     }
// }

// SymbolTable::ValueType Function::evaluate(SymbolTable &scope, std::vector<SymbolTable::ValueType> args) {
//     SymbolTable localScope;
//     localScope.name = name;
//     localScope.setParent(&scope);  // Set parent scope

//     showDebugSection("Executing Function (" + name + ")");
//     console.debug("Function has " + std::to_string(parameters.size()) + " parameters");

//     // Initialize parameters with default values
//     for (const auto &param : parameters) {
//         console.log(valueToString(param));
//         Expression::evaluate(param, localScope);
//     }

//     // Pass arguments
//     int argPos = 0;
//     console.debug("Passing arguments");

//     for (const auto &arg : args) {
//         if (argPos >= paramNames.size()) {
//             console.error("Too many arguments passed to function '" + name + "'. Expected " + 
//                           valueToString(paramNames.size()) + ", got " + valueToString(args.size()));
//             throw std::runtime_error("Function called with too many arguments");
//         }

//         SymbolTable::ValueType evaluatedValue;
//         if (auto argPtr = std::get_if<std::shared_ptr<Statement>>(&arg)) {
//             if (auto assignment = std::dynamic_pointer_cast<Assignment>(*argPtr)) {
//                 console.debug("Argument is an assignment statement");
//                 auto clonedAssignment = assignment->clone();
//                 evaluatedValue = Expression::evaluate(clonedAssignment->getValue(), scope).value();

//                 clonedAssignment->setValue(evaluatedValue);
//                 Expression::evaluate(clonedAssignment, localScope);
//             } 
//             else if (auto assignment = std::dynamic_pointer_cast<ConstantAssignment>(*argPtr)) {
//                 console.debug("Argument is a constant assignment");
//                 auto clonedAssignment = assignment->clone();
//                 evaluatedValue = Expression::evaluate(clonedAssignment->getValue(), scope).value();

//                 clonedAssignment->setValue(evaluatedValue);
//                 Expression::evaluate(clonedAssignment, localScope);
//             }
//         } else {
//             console.debug("Argument is not an assignment statement");
//             evaluatedValue = Expression::evaluate(arg, scope).value();

//             console.debug("Setting parameter '" + paramNames[argPos] + "' to '" + valueToString(evaluatedValue) + "'");
//             localScope.set(paramNames[argPos], evaluatedValue);
//         }

//         argPos++;
//     }

//     // Execute function body
//     for (const auto &statement : body) {
//         if (auto returnStatement = std::dynamic_pointer_cast<ReturnStatement>(statement)) {
//             console.debug("Function '" + name + "' returning a value");
//             return Expression::evaluate(returnStatement, localScope).value();
//         }

//         auto value = Expression::evaluate(statement, localScope).value();
//         if (value != SymbolTable::ValueType{}) {
//             console.debug("Function '" + name + "' returning " + valueToString(value) + " from nested statement");
//             return value;
//         }
//     }

//     console.debug("Function '" + name + "' executed and returned void.\n");
//     return SymbolTable::ValueType{};  // Return void-equivalent value
// }
