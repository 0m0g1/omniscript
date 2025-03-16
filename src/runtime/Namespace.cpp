// #include<omniscript/omniscript_pch.h>
// #include<omniscript/runtime/Statement.h>
// #include<omniscript/runtime/Namespace.h>

// Namespace::Namespace(const std::string& name, std::vector<std::shared_ptr<Statement>> body) 
//     : name(name), body(std::move(body)) {
//     this->name = name;
//     localScope = std::make_shared<SymbolTable>();
//     // addClass("Namespace");
// }

// void Namespace::setProperty(std::string name, SymbolTable::ValueType value) {
//     localScope->set(name, value);
// }

// SymbolTable::ValueType Namespace::getProperty(const std::string& symbolName) const {
//     auto value = localScope->get(symbolName);
//     if (!value.has_value()) {
//         console.error("Symbol " + symbolName + "does not exist in scope " + name);
//     }
//     return value.value();
// }

// void Namespace::evaluate(SymbolTable& scope) {
//     localScope->setParent(&scope);
//     for (const auto& statement : body) {
//         Expression::evaluate(statement, *localScope).value();
//     }
// }

// std::string Namespace::toString(int indentLevel) const {
//     std::string result = "Namespace(" + name + ") {\n";
//     for (const auto& [key, value] : localScope->variables) {
//         result += std::string(indentLevel + 2, ' ') + key + "\n";
//     }
//     for (const auto& [key, value] : localScope->constants) {
//         result += std::string(indentLevel + 2, ' ') + key + "\n";
//     }
//     for (const auto& [key, value] : localScope->objects) {
//         result += std::string(indentLevel + 2, ' ') + key + "\n";
//     }
//     for (const auto& [key, value] : localScope->functions) {
//         result += std::string(indentLevel + 2, ' ') + key + "\n";
//     }
//     result += std::string(indentLevel, ' ') + "}";
//     return result;
// }
