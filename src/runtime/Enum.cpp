// #include <omniscript/omniscript_pch.h>
// #include <omniscript/runtime/Enum.h>
// #include <omniscript/utils.h>

// Enum::Enum(const std::string& name, const std::vector<std::string>& keys, const std::vector<std::shared_ptr<Statement>>& values) {
//     this->name = name;
    
//     for (size_t i = 0; i < keys.size(); ++i) {
//         properties[keys[i]] = values[i];
//     }

//     // for (size_t i = 0; i < values.size(); ++i) {
//     //     properties[values[i]] = static_cast<int>(i);
//     //     properties[values[i]] = i;
//     // }
// }

// void Enum::setProperty(std::string name, SymbolTable::ValueType value) {
//     throw std::runtime_error("Cannot modify an Enum value");
// }

// // SymbolTable::ValueType Enum::getProperty(const std::string& name) const {
// //     auto it = properties.find(name);
// //     if (it != properties.end()) {
// //         return it->second;
// //     }
// //     return Object::getProperty(name);
// // }

// std::string Enum::getPropertyName(int value) const {
//     for (const auto& [key, val] : properties) {
//         if (const int* valueInt = std::get_if<int>(&val)) { 
//             if (*valueInt == value) {
//                 return key;
//             }
//         }
//     }
//     throw std::runtime_error("Enum value not found");
// }


// std::string Enum::toString(int indentLevel) const {
//     std::string result = "Enum(" + name + ") {\n";
//     for (const auto& [key, value] : properties) {
//         result += std::string(indentLevel + 2, ' ') + key + " = " + valueToString(value) + "\n";
//     }
//     result += std::string(indentLevel, ' ') + "}";
//     return result;
// }
