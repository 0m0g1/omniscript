// #include <omniscript/runtime/Number.h>
// #include <omniscript/runtime/Pointer.h>
// #include <omniscript/omniscript_pch.h>
// #include <omniscript/utils.h>

// void Number::initializeMethods() {
//     name = "a Number";
//     addMethod("setValue", [this](const ArgumentDefinition& args) -> SymbolTable::ValueType {
//         if (args.empty()) {
//             throw std::runtime_error("setValue requires at least one argument.");
//         }
//         if (!std::holds_alternative<int>(args[0]) && !std::holds_alternative<float>(args[0]) && !std::holds_alternative<double>(args[0])) {
//             throw std::runtime_error("setValue requires a numeric argument.");
//         }
//         if (std::holds_alternative<int>(args[0])) {
//             setValue(std::get<int>(args[0]));
//         } else if (std::holds_alternative<float>(args[0])) {
//             setValue(std::get<float>(args[0]));
//         } else if (std::holds_alternative<double>(args[0])) {
//             setValue(std::get<double>(args[0]));
//         }
//         return getValue(); 
//     });

//     addMethod("fromString", [](const ArgumentDefinition& args) -> SymbolTable::ValueType {
//         if (args.size() != 1 || !std::holds_alternative<std::string>(args[0])) {
//             throw std::runtime_error("fromString requires a single string argument.");
//         }
//         return fromString(std::get<std::string>(args[0])); 
//     });

//     addMethod("toString", [this](const ArgumentDefinition& args) -> SymbolTable::ValueType {
//         if (!args.empty()) {
//             throw std::runtime_error("toString does not take any arguments.");
//         }
//         return toString(); 
//     });

//     addMethod("toInt", [this](const ArgumentDefinition& args) -> SymbolTable::ValueType {
//         if (!args.empty()) {
//             throw std::runtime_error("toInt does not take any arguments.");
//         }
//         return toInt(); 
//     });

//     addMethod("toDouble", [this](const ArgumentDefinition& args) -> SymbolTable::ValueType {
//         if (!args.empty()) {
//             throw std::runtime_error("toDouble does not take any arguments.");
//         }
//         return toDouble(); 
//     });

//     addMethod("getMaxValue", [this](const ArgumentDefinition& args) -> SymbolTable::ValueType {
//         if (!args.empty()) {
//             throw std::runtime_error("getMaxValue does not take any arguments.");
//         }
//         return getMaxValue(); 
//     });

//     addMethod("getMaxValueFor", [](const ArgumentDefinition& args) -> SymbolTable::ValueType {
//         if (args.size() != 1 || !std::holds_alternative<int>(args[0])) {
//             throw std::runtime_error("getMaxValueFor requires one integer argument.");
//         }
//         return getMaxValueFor(std::get<int>(args[0])); 
//     });

//     addMethod("getType", [this](const ArgumentDefinition& args) -> SymbolTable::ValueType {
//         if (!args.empty()) {
//             throw std::runtime_error("getType does not take any arguments.");
//         }
//         return getType(); 
//     });

//     addMethod("isInteger", [this](const ArgumentDefinition& args) -> SymbolTable::ValueType {
//         if (!args.empty()) {
//             throw std::runtime_error("isInteger does not take any arguments.");
//         }
//         return isInteger(); 
//     });

//     addMethod("isFloat", [this](const ArgumentDefinition& args) -> SymbolTable::ValueType {
//         if (!args.empty()) {
//             throw std::runtime_error("isFloat does not take any arguments.");
//         }
//         return isFloat(); 
//     });

//     addMethod("isValidNumber", [this](const ArgumentDefinition& args) -> SymbolTable::ValueType {
//         if (args.size() != 1 || !std::holds_alternative<std::string>(args[0])) {
//             throw std::runtime_error("isValidNumber requires a single string argument.");
//         }
//         return isValidNumber(std::get<std::string>(args[0])); 
//     });

//     addMethod("isNumber", [this](const ArgumentDefinition& args) -> SymbolTable::ValueType {
//         if (args.size() != 1 || !std::holds_alternative<std::string>(args[0])) {
//             throw std::runtime_error("isNumber requires a single string argument.");
//         }
//         return isNumber(std::get<std::string>(args[0])); 
//     });

//     addMethod("isValidInteger", [this](const ArgumentDefinition& args) -> SymbolTable::ValueType {
//         if (args.size() != 1 || !std::holds_alternative<std::string>(args[0])) {
//             throw std::runtime_error("isValidInteger requires a single string argument.");
//         }
//         return isValidInteger(std::get<std::string>(args[0])); 
//     });

//     addMethod("isValidFloat", [this](const ArgumentDefinition& args) -> SymbolTable::ValueType {
//         if (args.size() != 1 || !std::holds_alternative<std::string>(args[0])) {
//             throw std::runtime_error("isValidFloat requires a single string argument.");
//         }
//         return isValidFloat(std::get<std::string>(args[0])); 
//     });

//     addMethod("asPointer", [this](const ArgumentDefinition& args) -> SymbolTable::ValueType {
//         if (args.size() != 0) {
//             throw std::runtime_error("asPointer does not take any arguments.");
//         }
//         return asPointer(); 
//     });
    
// }

// // Convert value to string
// std::string Number::toString(int indentLevel) const {
//     return valueToString(value);
// }

// // Convert string to the appropriate Number type
// SymbolTable::ValueType Number::fromString(const std::string& str) {
//     std::istringstream iss(str);
//     long double temp;

//     if (iss >> temp) {
//         if (temp == static_cast<int>(temp)) {
//             if (temp <= std::numeric_limits<int>::max() && temp >= std::numeric_limits<int>::min()) {
//                 return static_cast<int>(temp); 
//             } else if (temp <= std::numeric_limits<long long>::max() && temp >= std::numeric_limits<long long>::min()) {
//                 return static_cast<long long>(temp);
//             }
//         }
//         return temp; 
//     }
//     throw std::invalid_argument(valueToString(str) + " is an invalid number.");
// }

// // Convert value to int
// int Number::toInt() const {
//     return std::visit([](auto&& arg) -> int {
//         using T = std::decay_t<decltype(arg)>;
//         if constexpr (std::is_same_v<T, int>) {
//             return arg; // Already an int
//         } else if constexpr (std::is_same_v<T, float>) {
//             return static_cast<int>(arg); // Convert float to int
//         } else if constexpr (std::is_same_v<T, std::string>) {
//             try {
//                 return std::stoi(arg); // Attempt to parse string as int
//             } catch (...) {
//                 throw std::invalid_argument("Cannot convert string to int");
//             }
//         } else {
//             throw std::invalid_argument("Unsupported type for toInt()");
//         }
//     }, value);
// }

// // Convert value to double
// SymbolTable::ValueType Number::toDouble() const {
//     return std::visit([](auto&& arg) -> SymbolTable::ValueType {
//         using T = std::decay_t<decltype(arg)>;
//         if constexpr (std::is_same_v<T, double>) {
//             return SymbolTable::ValueType(arg); // Already a double
//         } else if constexpr (std::is_same_v<T, float>) {
//             return SymbolTable::ValueType(static_cast<double>(arg)); // Convert float to double
//         } else if constexpr (std::is_integral_v<T>) {
//             return SymbolTable::ValueType(static_cast<double>(arg)); // Convert integers to double
//         } else if constexpr (std::is_same_v<T, std::string>) {
//             try {
//                 return SymbolTable::ValueType(std::stod(arg.c_str())); // Convert string to double
//             } catch (const std::invalid_argument&) {
//                 throw std::runtime_error("Invalid string-to-double conversion: " + arg);
//             } catch (const std::out_of_range&) {
//                 throw std::runtime_error("String value out of range for double: " + arg);
//             }
//         } else {
//             throw std::runtime_error("Unsupported type for toDouble conversion.");
//         }
//     }, value);
// }

// // Return max value based on the type
// SymbolTable::ValueType Number::getMaxValue() const {
//     return std::visit([](auto&& arg) -> SymbolTable::ValueType {
//         using T = std::decay_t<decltype(arg)>;
//         if constexpr (std::is_arithmetic_v<T>) {
//             return std::numeric_limits<T>::max();
//         } else if constexpr (std::is_same_v<T, std::string>) {
//             throw std::runtime_error("No max value for std::string.");
//         } else {
//             throw std::runtime_error("Unsupported type for getMaxValue.");
//         }
//     }, value);
// }

// // Get max value for a specific type
// std::string Number::getMaxValueFor(int type) {
//     switch (type) {
//         case 0: return std::to_string(std::numeric_limits<int>::max());
//         case 1: return std::to_string(std::numeric_limits<unsigned int>::max());
//         case 2: return std::to_string(std::numeric_limits<long>::max());
//         case 3: return std::to_string(std::numeric_limits<unsigned long>::max());
//         case 4: return std::to_string(std::numeric_limits<long long>::max());
//         case 5: return std::to_string(std::numeric_limits<unsigned long long>::max());
//         case 6: return std::to_string(std::numeric_limits<float>::max());
//         case 7: return std::to_string(std::numeric_limits<double>::max());
//         case 8: return std::to_string(std::numeric_limits<long double>::max());
//         default: throw std::invalid_argument("Unknown type.");
//     }
// }

// // Return type of number as string
// std::string Number::getType() const {
//     return std::visit([](auto&& arg) -> std::string {
//         return typeid(arg).name(); 
//     }, value);
// }

// // Check if number is integer
// bool Number::isInteger() const {
//     return std::holds_alternative<int>(value) || std::holds_alternative<long>(value) ||
//             std::holds_alternative<long long>(value) || std::holds_alternative<unsigned int>(value) ||
//             std::holds_alternative<unsigned long>(value) || std::holds_alternative<unsigned long long>(value);
// }

// // Check if number is float
// bool Number::isFloat() const {
//     return std::holds_alternative<float>(value) || std::holds_alternative<double>(value) ||
//             std::holds_alternative<long double>(value);
// }

// // Check if string is a valid number
// bool Number::isValidNumber(const std::string& str) {
//     try {
//         fromString(str);
//         return true;
//     } catch (...) {
//         return false;
//     }
// }

// // Check if string is a number
// bool Number::isNumber(const std::string& str) {
//     return isValidNumber(str);
// }

// // Check if string is a valid integer
// bool Number::isValidInteger(const std::string& str) {
//     try {
//         std::stoi(str);
//         return true;
//     } catch (...) {
//         return false;
//     }
// }

// // Check if string is a valid float
// bool Number::isValidFloat(const std::string& str) {
//     try {
//         std::stof(str);
//         return true;
//     } catch (...) {
//         return false;
//     }
// }

// std::shared_ptr<PointerObj> Number::asPointer() {
//     return std::visit([](auto&& arg) -> std::shared_ptr<PointerObj> {
//         using ValueType = std::decay_t<decltype(arg)>;

//         // Only allow integer types
//         if constexpr (std::is_integral_v<ValueType>) {
//             return std::make_shared<PointerObj>(static_cast<intptr_t>(arg));
//         }

//         // This should never be reached if NumberVariant is correctly defined
//         throw std::runtime_error("Unexpected non-integer type in Number.");
//     }, value);
// }

