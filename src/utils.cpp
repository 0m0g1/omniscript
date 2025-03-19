#include <omniscript/utils.h>
#include <omniscript/omniscript_pch.h>

#include <omniscript/runtime/Namespace.h>
#include <omniscript/runtime/object.h>
// #include <omniscript/runtime/Function.h>
#include <omniscript/runtime/Number.h>
#include <omniscript/runtime/String.h>
#include <omniscript/runtime/Pointer.h>

//Converts a string to lower case
std::string toLowerCaseString(const std::string &source) {
    std::string lowerCaseString;

    for (char ch: source) {
        lowerCaseString += tolower(ch);
    }

    return lowerCaseString;
}

//Converts a string to uppercase
std::string toUpperCaseString(const std::string &source) {
    std::string upperCaseString;

    for (char ch: source) {
        upperCaseString += toupper(ch);
    }

    return upperCaseString;
}

// Show debug section using Console
void showDebugSection(std::string nameOfSection) {
    std::string separator(nameOfSection.length(), '=');

    console.debug("");  // Blank line for separation
    console.debug(nameOfSection);
    console.debug(separator);
}

// Show a debug statement
void showDebugStatement(std::string statement) {
    console.debug(statement);
}

// Get a statement showing the current line and column of the token for debugging
std::string getLineAndColumnStatementFor(Token token) {
    // Assuming the token has getLine() and getColumn() methods that return integers
    int line = token.getLine();
    int column = token.getColumn();
    
    // Format the output string with the line and column information
    return "at line: " + std::to_string(line) + ", column: " + std::to_string(column);
}

// Compare 
// Utility function to compare the SymbolVariant types



// A function to convert ValueType to a string representation
std::string valueToString(const SymbolTable::ValueType& value, int indentLevel) {
    return std::visit([indentLevel](const auto& arg) -> std::string {
        using T = std::decay_t<decltype(arg)>;

        if constexpr (std::is_same_v<T, std::string>) {
            return "\"" + arg + "\"";
        } // else if constexpr (std::is_same_v<T, std::shared_ptr<Function>>) {
            // return arg ? arg->toString(indentLevel) : "[Function]";
        // }
        else if constexpr (std::is_same_v<T, std::shared_ptr<Array>>) {
            return arg ? arg->toString(indentLevel) : "[null array]";
        } else if constexpr (std::is_same_v<T, std::shared_ptr<Object>>) {
            return arg ? arg->toString(indentLevel) : "[null object]";
        } else if constexpr (std::is_same_v<T, std::shared_ptr<Statement>>) {
            // Dereference the shared pointer to access the actual Statement object
            auto& stmt = *arg;

            // Check the exact type of the statement object using dynamic_cast
            if (dynamic_cast<Literal*>(&stmt)) {
                return "[Literal Statement]";
            } 
            // else if (dynamic_cast<Assignment*>(&stmt)) {
                // return "[Assignment Statement]";
            // } 
            // else if (dynamic_cast<ConstantAssignment*>(&stmt)) {
            //     return "[Constant Assignment Statement]";
            // }
            else if (dynamic_cast<GenericAssignment*>(&stmt)) {
                return "[Generic Assignment Statement]";
            } else if (dynamic_cast<Variable*>(&stmt)) {
                return "[Variable Statement]";
            } else if (dynamic_cast<BlockStatement*>(&stmt)) {
                return "[Block Statement]";
            } else if (dynamic_cast<ObjectConstructorStatement*>(&stmt)) {
                return "[Object Constructor Statement]";
            } else if (dynamic_cast<ObjectDestructorStatement*>(&stmt)) {
                return "[Object Destructor Statement]";
            } else if (dynamic_cast<ReturnStatement*>(&stmt)) {
                return "[Return Statement]";
            } else if (dynamic_cast<FunctionCallStatement*>(&stmt)) {
                return "[Function Call Statement]";
            } else if (dynamic_cast<IfStatement*>(&stmt)) {
                return "[If Statement]";
            } else if (dynamic_cast<UnaryExpression*>(&stmt)) {
                return "[Unary Expression Statement]";
            } else if (dynamic_cast<BinaryExpression*>(&stmt)) {
                return "[Binary Expression Statement]";
            } else if (dynamic_cast<TenaryExpression*>(&stmt)) {
                return "[Tenary Expression Statement]";
            } else if (dynamic_cast<BreakStatement*>(&stmt)) {
                return "[Break Statement]";
            } else if (dynamic_cast<ContinueStatement*>(&stmt)) {
                return "[Continue Statement]";
            } else if (dynamic_cast<WhileStatement*>(&stmt)) {
                return "[While Statement]";
            } else if (dynamic_cast<ForLoop*>(&stmt)) {
                return "[For loop]";
            } else if (dynamic_cast<CallMethod*>(&stmt)) {
                return "[Method call]";
            } else if (dynamic_cast<GetProperty*>(&stmt)) {
                return "[Property call]";
            } else {
                return "[Unknown Statement Type]";
            }
        } else if constexpr (std::is_same_v<T, bool>) {
            return arg ? "true" : "false";
        } else if constexpr (std::is_same_v<T, std::vector<SymbolTable::ValueType>>) {
            // Convert std::vector<ValueType> to string with indentation
            std::string result = "[";
            for (size_t i = 0; i < arg.size(); ++i) {
                result += valueToString(arg[i], indentLevel + 1);
                if (i < arg.size() - 1) {
                    result += ", ";
                }
            }
            result += "]";
            return result;
        } else if constexpr (std::is_same_v<T, std::vector<std::shared_ptr<Statement>>>) {
            // Convert std::vector<ValueType> to string with indentation
            std::string result = "[";
            for (size_t i = 0; i < arg.size(); ++i) {
                result += valueToString(arg[i], indentLevel + 1);
                if (i < arg.size() - 1) {
                    result += ", ";
                }
            }
            result += "]";
            return result;
        } else if constexpr (std::is_arithmetic_v<T>) {
            return std::to_string(arg);
        } else if constexpr (std::is_pointer_v<T>) {
            return "[Pointer: " + std::to_string(reinterpret_cast<intptr_t>(arg)) + "]";
        } else if constexpr (std::is_null_pointer_v<T>) {
            return "nullptr";
        } else {
            return "[Unknown Type]";
        }
    }, value);
}

std::string valueToString(const std::vector<SymbolTable::ValueType>& args, int indentLevel) {
    std::string result;
    for (const auto& arg : args) {
        result += valueToString(arg);
    }
    return result;
}

// Check if a variant's value is true
bool isTruthy(const SymbolTable::ValueType& value) {
    return std::visit([](const auto& val) -> bool {
        using T = std::decay_t<decltype(val)>;

        if constexpr (std::is_same_v<T, std::string>) {
            // Non-empty strings are truthy
            return !val.empty();
        } else if constexpr (std::is_same_v<T, bool>) {
            // Boolean values directly determine truthiness
            return val;
        } else if constexpr (std::is_same_v<T, std::shared_ptr<Array>>) {
            // Shared pointers to arrays are truthy if not null
            return static_cast<bool>(val);
        } else if constexpr (std::is_same_v<T, std::shared_ptr<Statement>>) {
            // Shared pointers to statements are truthy if not null
            return static_cast<bool>(val);
        } else if constexpr (std::is_same_v<T, std::vector<std::shared_ptr<Statement>>>) {
            // Vectors of statements are truthy if non-empty
            return !val.empty();
        } else if constexpr (std::is_same_v<T, std::shared_ptr<Object>>) {
            // Shared pointers to objects are truthy if not null
            return static_cast<bool>(val);
        } else if constexpr (std::is_same_v<T, std::vector<std::shared_ptr<Object>>>) {
            // Vectors of objects are truthy if non-empty
            return !val.empty();
        } else if constexpr (std::is_arithmetic_v<T>) {
            // Numeric values are truthy if non-zero
            return true;
        } else if constexpr (std::is_same_v<T, void*>) {
            // Void pointers are truthy if not null
            return val != nullptr;
        } else {
            // Default case: unsupported types are considered falsey
            return false;
        }
    }, value);
}

// Platform-independent utility for sleep
void sleep_ms(int milliseconds) {
    std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
}

// Utility function to convert std::any to ValueType
// SymbolTable::ValueType convertToValueType(const std::any& anyValue) {
//     try {
//         // Check type and cast appropriately
//         if (anyValue.type() == typeid(std::string)) {
//             return std::any_cast<std::string>(anyValue);
//         } else if (anyValue.type() == typeid(int)) {
//             return std::any_cast<int>(anyValue);
//         } else if (anyValue.type() == typeid(unsigned int)) {
//             return std::any_cast<unsigned int>(anyValue);
//         } else if (anyValue.type() == typeid(long)) {
//             return std::any_cast<long>(anyValue);
//         } else if (anyValue.type() == typeid(unsigned long)) {
//             return std::any_cast<unsigned long>(anyValue);
//         } else if (anyValue.type() == typeid(long long)) {
//             return std::any_cast<long long>(anyValue);
//         } else if (anyValue.type() == typeid(unsigned long long)) {
//             return std::any_cast<unsigned long long>(anyValue);
//         } else if (anyValue.type() == typeid(float)) {
//             return std::any_cast<float>(anyValue);
//         } else if (anyValue.type() == typeid(double)) {
//             return std::any_cast<double>(anyValue);
//         } else if (anyValue.type() == typeid(long double)) {
//             return std::any_cast<long double>(anyValue);
//         } else if (anyValue.type() == typeid(bool)) {
//             return std::any_cast<bool>(anyValue);
//         } else if (anyValue.type() == typeid(char)) {
//             return std::any_cast<char>(anyValue);
//         } else if (anyValue.type() == typeid(std::vector<char>)) {
//             return std::any_cast<std::vector<char>>(anyValue);
//         } else if (anyValue.type() == typeid(std::shared_ptr<std::vector<int>>)) {
//             return std::any_cast<std::shared_ptr<std::vector<int>>>(anyValue);
//         } else if (anyValue.type() == typeid(std::shared_ptr<std::string>)) {
//             return std::any_cast<std::shared_ptr<std::string>>(anyValue);
//         } else if (anyValue.type() == typeid(std::shared_ptr<void>)) {
//             return std::any_cast<std::shared_ptr<void>>(anyValue);
//         } else if (anyValue.type() == typeid(std::nullptr_t)) {
//             return nullptr;
//         }

//         // If the type is not recognized, throw an exception
//         throw std::runtime_error("Unsupported type in std::any.");
//     } catch (const std::bad_any_cast& e) {
//         std::cerr << "Failed to cast std::any: " << e.what() << std::endl;
//         throw; // rethrow the exception
//     }
// }

std::shared_ptr<Object> primitiveToObject(const SymbolTable::ValueType &objectValue) {
    std::shared_ptr<Object> baseObject;

    // Handle integer types
    // if (auto shortValue = std::get_if<short>(&objectValue)) {
    //     baseObject = std::make_shared<Number>(*shortValue);
    // } else if (auto ushortValue = std::get_if<unsigned short>(&objectValue)) {
    //     baseObject = std::make_shared<Number>(*ushortValue);
    // } else
    // if (auto intValue = std::get_if<int>(&objectValue)) {
    //     baseObject = std::make_shared<Number>(*intValue);
    // } else if (auto uintValue = std::get_if<unsigned int>(&objectValue)) {
    //     baseObject = std::make_shared<Number>(*uintValue);
    // } else if (auto longValue = std::get_if<long>(&objectValue)) {
    //     baseObject = std::make_shared<Number>(*longValue);
    // } else if (auto ulongValue = std::get_if<unsigned long>(&objectValue)) {
    //     baseObject = std::make_shared<Number>(*ulongValue);
    // } else if (auto longLongValue = std::get_if<long long>(&objectValue)) {
    //     baseObject = std::make_shared<Number>(*longLongValue);
    // } else if (auto ulongLongValue = std::get_if<unsigned long long>(&objectValue)) {
    //     baseObject = std::make_shared<Number>(*ulongLongValue);
    // }

    // // Handle floating-point types
    // else if (auto floatValue = std::get_if<float>(&objectValue)) {
    //     baseObject = std::make_shared<Number>(*floatValue);
    // } else if (auto doubleValue = std::get_if<double>(&objectValue)) {
    //     baseObject = std::make_shared<Number>(*doubleValue);
    // } else if (auto longDoubleValue = std::get_if<long double>(&objectValue)) {
    //     baseObject = std::make_shared<Number>(*longDoubleValue);
    // }

    // Handle string type
    // else if (auto stringValue = std::get_if<std::string>(&objectValue)) {
    //     baseObject = std::make_shared<String>(*stringValue);
    // }

    // Handle boolean type
    // else 
    if (auto boolValue = std::get_if<bool>(&objectValue)) {
        baseObject = std::make_shared<Boolean>(*boolValue);
    }

    // Handle objects
    else if (auto obj = std::get_if<std::shared_ptr<Object>>(&objectValue)) {
        return *obj;
    }

    // Handle Array
    else if (auto obj = std::get_if<std::shared_ptr<Array>>(&objectValue)) {
        return *obj;
    }

    else if (auto obj = std::get_if<std::shared_ptr<Namespace>>(&objectValue)) {
        return *obj;
    }

    // Handle unsupported types
    else {
        throw std::runtime_error("Invalid base object type for method call." + valueToString(objectValue));
    }

    return baseObject;
}


float toFloat(const SymbolTable::ValueType& value) {
    return std::visit([&value](auto&& arg) -> float { // Capture 'value' by reference
        using T = std::decay_t<decltype(arg)>;
        if constexpr (std::is_same_v<T, int>) {
            return static_cast<float>(arg); // Convert int to float
        } else if constexpr (std::is_same_v<T, float>) {
            return arg; // Return float as is
        } else {
            console.error("Unsupported type for toFloat: " + valueToString(value));
            return 0.0f; // Default fallback
        }
    }, value);
}


bool isEnum(const SymbolTable::ValueType& value) {
    return std::visit([](auto&& value) {
        using T = std::decay_t<decltype(value)>;
        if constexpr (std::is_enum<T>::value) {
            return true;
        }
        return false;
    }, value);
}


// Function to check if a ValueType is a Number
bool isNumber(const SymbolTable::ValueType& value) {
    return std::visit([](auto&& arg) -> bool {
        using T = std::decay_t<decltype(arg)>;
        return std::is_same_v<T, SymbolTable::Numbers>;
    }, value);
}

// Function to check if a ValueType is a String
bool isString(const SymbolTable::ValueType& value) {
    return std::visit([](auto&& arg) -> bool {
        using T = std::decay_t<decltype(arg)>;
        return std::is_same_v<T, SymbolTable::String>;
    }, value);
}

// Function to check if a ValueType is a Primitive
bool isPrimitive(const SymbolTable::ValueType& value) {
    return std::visit([](auto&& arg) -> bool {
        using T = std::decay_t<decltype(arg)>;
        return isNumber(arg) || isString(arg) || std::is_same_v<T, bool>;
    }, value);
}
