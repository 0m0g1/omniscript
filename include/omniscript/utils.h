//Utility functions
#ifndef Utility_H
#define Utility_H

//Includes
// #include <string>
#include <omniscript/omniscript_pch.h>
#include <omniscript/debuggingtools/console.h>
#include <omniscript/engine/tokens.h>

// Foward declarations
class Object;

// Utility function declerations
std::string toUpperCaseString(const std::string &source);
std::string toLowerCaseString(const std::string &source);
void showDebugSection(std::string nameOfSection);
void showDebugStatement(std::string statement);
// std::string valueToString(const SymbolTable::ValueType& value, int indentLevel = 0);
// std::string valueToString(const std::vector<SymbolTable::ValueType>& args, int indentLevel = 0);
std::string getLineAndColumnStatementFor(Token token);
// std::shared_ptr<Object> primitiveToObject(const SymbolTable::ValueType &obj);

// bool isTruthy(const SymbolTable::ValueType& value);
void sleep_ms(int milliseconds);
// float toFloat(const SymbolTable::ValueType& value);
// bool isNumber(const SymbolTable::ValueType& value);
// bool isString(const SymbolTable::ValueType& value);
// bool isPrimitive(const SymbolTable::ValueType& value);
// SymbolTable::ValueType convertToValueType(const std::any& anyValue);
std::string readFile(const std::string& path);
std::string joinSet(const std::unordered_set<std::string>& members);
std::string joinMapKeys(const std::unordered_map<std::string, std::string>& map);
std::string join(const std::vector<std::string>& vec, const std::string& delimiter);

std::u32string utf8_to_utf32(const std::string& utf8str);
std::string utf32_to_utf8(const std::u32string& utf32str);
std::u16string utf8_to_utf16(const std::string& utf8str);
std::string utf16_to_utf8(const std::u16string& utf16str);
std::u16string utf32_to_utf16(const std::u32string& input);
std::u32string utf16_to_utf32(const std::u16string& input);

#endif