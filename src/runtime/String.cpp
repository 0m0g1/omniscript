// #include <omniscript/runtime/String.h>
// #include <omniscript/runtime/Number.h>
// #include <omniscript/omniscript_pch.h>
// #include <omniscript/utils.h>

// String::String(std::string value) : value(value) {
//     name = "a String";
//     // Add methods dynamically with lambdas
//     addMethod("getValue", [&, this](const ArgumentDefinition& args) -> std::string {
//         return value;  // Accessing member variable 'value'
//     });

//     // addMethod("setValue", [&, this](const ArgumentDefinition& args) {
//     //     if (args.size() != 1 || !std::holds_alternative<std::string>(args[0])) {
//     //         throw std::runtime_error("setValue requires a single string argument.");
//     //     }
//     //     value = std::get<std::string>(args[0]);  // Accessing member variable 'value'
//     // });

//     // addMethod("getAddress", [&, this](const ArgumentDefinition& args) -> void* {
//     //     return static_cast<void*>(&value);  // Accessing member variable 'value'
//     // });

//     addMethod("toString", [&, this](const ArgumentDefinition& args) -> std::string {
//         return this->value;  // Accessing member variable 'value'
//     });

//     addMethod("toNumber", [&, this](const ArgumentDefinition& args) -> SymbolTable::ValueType {
//         return Number::fromString(this->value);  // Accessing member variable 'value'
//     });

//     addMethod("isNumber", [&, this](const ArgumentDefinition& args) -> bool {
//         return Number::isValidNumber(this->value);  // Accessing member variable 'value'
//     });

//     addMethod("length", [this](const ArgumentDefinition& args) -> int {
//         if (this->value.length() > static_cast<size_t>(std::numeric_limits<int>::max())) {
//             return static_cast<long long>(this->value.length());
//         }
//         return static_cast<int>(this->value.length());
//     });

//     // Method to mimic String.fromCharCode
//     addMethod("fromCharCode", [&, this](const ArgumentDefinition& args) -> std::string {
//         // Check if the argument is an integer
//         if (args.size() == 1 && std::holds_alternative<int>(args[0])) {
//             char character = static_cast<char>(std::get<int>(args[0]));  // Convert integer to char
//             return std::string(1, character);  // Return a string with the single character
//         }
//         return "";  // Return empty string if argument is invalid
//     });


//     // Method to mimic String.prototype.charCodeAt
//     addMethod("charCodeAt", [&, this](const ArgumentDefinition& args) -> int {
//         // Check if the argument is a valid index (integer)
//         if (args.size() == 1 && std::holds_alternative<int>(args[0])) {
//             int index = std::get<int>(args[0]);  // Extract the index
//             if (index >= 0 && index < this->value.size()) {  // Ensure index is within bounds
//                 return static_cast<int>(this->value[index]);  // Return the character's char code
//             }
//         }
//         return -1;  // Return -1 if the index is out of bounds or invalid
//     });

//     addMethod("get", [this](const ArgumentDefinition& args) -> SymbolTable::ValueType {
//         // Check if there's exactly one argument and it's an integer
//         if (args.size() != 1 || !std::holds_alternative<int>(args[0])) {
//             throw std::invalid_argument("get requires exactly 1 integer argument");
//         }

//         int index = std::get<int>(args[0]);

//         // Handle negative indices like Python (i.e., -1 is the last character)
//         if (index < 0) {
//             index += this->value.length();
//         }

//         // Check if the index is within bounds after adjustment
//         if (index < 0 || index >= this->value.length()) {
//             throw std::out_of_range("Index out of range");
//         }

//         // Return the character at the specified index
//         return std::string(1, this->value[index]);  // Return a single character as a string
//     });

//     addMethod("slice", [&, this](const ArgumentDefinition& args) -> std::string {
//         if (args.size() != 2 ||
//             !std::holds_alternative<int>(args[0]) ||
//             !std::holds_alternative<int>(args[1])) {
//             throw std::runtime_error("slice requires two integer arguments.");
//         }
//         int start = std::get<int>(args[0]);
//         int end = std::get<int>(args[1]);
//         return slice(start, end);  // Accessing member variable 'value' indirectly
//     });

//     addMethod("substring", [&, this](const ArgumentDefinition& args) -> std::string {
//         if (args.size() != 2 ||
//             !std::holds_alternative<int>(args[0]) ||
//             !std::holds_alternative<int>(args[1])) {
//             throw std::runtime_error("substring requires two integer arguments.");
//         }
//         int start = std::get<int>(args[0]);
//         int end = std::get<int>(args[1]);
//         return substring(start, end);  // Accessing member variable 'value' indirectly
//     });

//     addMethod("toLowerCase", [&, this](const ArgumentDefinition& args) -> std::string {
//         return toLowerCase();  // Accessing member function 'toLowerCase'
//     });

//     addMethod("toUpperCase", [&, this](const ArgumentDefinition& args) -> std::string {
//         return toUpperCase();  // Accessing member function 'toUpperCase'
//     });

//     addMethod("trim", [&, this](const ArgumentDefinition& args) -> std::string {
//         return trim();  // Accessing member function 'trim'
//     });

//     addMethod("replace", [&, this](const ArgumentDefinition& args) -> std::string {
//         if (args.size() != 2 ||
//             !std::holds_alternative<std::string>(args[0]) ||
//             !std::holds_alternative<std::string>(args[1])) {
//             throw std::runtime_error("replace requires two string arguments.");
//         }
//         std::string from = std::get<std::string>(args[0]);
//         std::string to = std::get<std::string>(args[1]);
//         return replace(from, to);  // Accessing member function 'replace'
//     });

//     addMethod("charAt", [this](const ArgumentDefinition& args) -> char {
//         if (args.size() != 1 || !std::holds_alternative<int>(args[0])) {
//             throw std::runtime_error("charAt requires a single integer argument.");
//         }
//         int index = std::get<int>(args[0]);
//         return charAt(index);  // Accessing member function 'charAt'
//     });

//     addMethod("indexOf", [this](const ArgumentDefinition& args) -> int {
//         if (args.size() != 1 || !std::holds_alternative<std::string>(args[0])) {
//             throw std::runtime_error("indexOf requires a single string argument.");
//         }
//         std::string substr = std::get<std::string>(args[0]);
//         return indexOf(substr);  // Accessing member function 'indexOf'
//     });

//     addMethod("includes", [this](const ArgumentDefinition& args) -> bool {
//         if (args.size() != 1 || !std::holds_alternative<std::string>(args[0])) {
//             throw std::runtime_error("includes requires a single string argument.");
//         }
//         std::string substr = std::get<std::string>(args[0]);
//         return includes(substr);  // Accessing member function 'includes'
//     });

//     addMethod("startsWith", [this](const ArgumentDefinition& args) -> bool {
//         if (args.size() != 1 || !std::holds_alternative<std::string>(args[0])) {
//             throw std::runtime_error("startsWith requires a single string argument.");
//         }
//         std::string prefix = std::get<std::string>(args[0]);
//         return startsWith(prefix);  // Accessing member function 'startsWith'
//     });

//     addMethod("endsWith", [this](const ArgumentDefinition& args) -> bool {
//         if (args.size() != 1 || !std::holds_alternative<std::string>(args[0])) {
//             throw std::runtime_error("endsWith requires a single string argument.");
//         }
//         std::string suffix = std::get<std::string>(args[0]);
//         return endsWith(suffix);  // Accessing member function 'endsWith'
//     });

//     // addMethod("split", [&, this](const ArgumentDefinition& args) -> std::vector<std::string> {
//     //     if (args.size() != 1 || !std::holds_alternative<std::string>(args[0])) {
//     //         throw std::runtime_error("split requires a single string argument.");
//     //     }
//     //     std::string delimiter = std::get<std::string>(args[0]);
//     //     return split(delimiter);  // Accessing member function 'split'
//     // });

//     addMethod("toBoolean", [this](const ArgumentDefinition& args) -> bool {
//         return toBoolean();  // Accessing member function 'toBoolean'
//     });

//     addMethod("isEmpty", [this](const ArgumentDefinition& args) -> bool {
//         return isEmpty();  // Accessing member function 'isEmpty'
//     });

//     addMethod("isWhitespace", [this](const ArgumentDefinition& args) -> bool {
//         return isWhitespace();  // Accessing member function 'isWhitespace'
//     });
// }

// // Return the string value
// std::string String::toString(int indentLevel) const { 
//     return value; 
// }

// // Convert string to a number, return primitive Number (e.g., int, long long, or double)
// auto String::String::toNumber() {
//     return Number::fromString(value);  // Returns primitive type based on the string
// }

// // Check if string can be parsed as a valid number
// bool String::isNumber() const {
//     return Number::isValidNumber(value);
// }

// // String length
// size_t String::length() const {
//     return value.length();
// }

// // Slice the string and return a primitive string (not wrapped in String object)
// std::string String::slice(int start, int end) const {
//     if (start < 0) start = 0;
//     if (end > value.length()) end = value.length();
//     return value.substr(start, end - start);
// }

// // Extract a substring and return a primitive string
// std::string String::substring(int start, int end) const {
//     if (start < 0) start = 0;
//     if (end > value.length()) end = value.length();
//     return value.substr(start, end - start);
// }

// // Convert to lowercase and return a new string
// std::string String::toLowerCase() const {
//     std::string result = value;
//     std::transform(result.begin(), result.end(), result.begin(), ::tolower);
//     return result;
// }

// // Convert to uppercase and return a new string
// std::string String::toUpperCase() const {
//     std::string result = value;
//     std::transform(result.begin(), result.end(), result.begin(), ::toupper);
//     return result;
// }

// // Trim whitespace from both ends of the string
// std::string String::trim() const {
//     std::string result = value;
//     result.erase(result.begin(), std::find_if(result.begin(), result.end(), [](unsigned char ch) {
//         return !std::isspace(ch);
//     }));
//     result.erase(std::find_if(result.rbegin(), result.rend(), [](unsigned char ch) {
//         return !std::isspace(ch);
//     }).base(), result.end());
//     return result;
// }

// // Replace a substring and return the modified string
// std::string String::replace(const std::string& from, const std::string& to) const {
//     std::string result = value;
//     size_t pos = 0;
//     while ((pos = result.find(from, pos)) != std::string::npos) {
//         result.replace(pos, from.length(), to);
//         pos += to.length();
//     }
//     return result;
// }

// // Character at a specific index
// char String::charAt(int index) const {
//     if (index >= 0 && index < value.size()) {
//         return value[index];
//     }
//     throw std::out_of_range("Index out of range");
// }

// // Index of the first occurrence of a substring
// int String::indexOf(const std::string& substr) const {
//     size_t pos = value.find(substr);
//     return (pos != std::string::npos) ? static_cast<int>(pos) : -1;
// }

// // Check if string contains a substring
// bool String::includes(const std::string& substr) const {
//     return value.find(substr) != std::string::npos;
// }

// // Check if the string starts with a given substring
// bool String::startsWith(const std::string& prefix) const {
//     return value.rfind(prefix, 0) == 0;
// }

// // Check if the string ends with a given substring
// bool String::endsWith(const std::string& suffix) const {
//     return value.size() >= suffix.size() &&
//             value.compare(value.size() - suffix.size(), suffix.size(), suffix) == 0;
// }

// // Split the string into an array of substrings
// std::vector<std::string> String::split(const std::string& delimiter) const {
//     std::vector<std::string> result;
//     std::string tempValue = value;
//     size_t pos = 0;
//     while ((pos = tempValue.find(delimiter)) != std::string::npos) {
//         result.push_back(tempValue.substr(0, pos));
//         tempValue.erase(0, pos + delimiter.length());
//     }
//     result.push_back(tempValue);  // Add remaining part
//     return result;
// }

// // Convert string to boolean (empty string is false)
// bool String::toBoolean() const {
//     return !value.empty();
// }

// // Check if the string is empty
// bool String::isEmpty() const {
//     return value.empty();
// }

// // Check if the string consists of only whitespace
// bool String::isWhitespace() const {
//     return std::all_of(value.begin(), value.end(), ::isspace);
// }
