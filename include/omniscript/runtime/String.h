#pragma once
#include <omniscript/runtime/Statement.h>
#include <omniscript/runtime/object.h>
#include <omniscript/omniscript_pch.h>


class String : public Object {
private:
    std::string value;
    
public:
    explicit String(std::string value = "");

    // Get the raw string value
    std::string getValue() const { return value; }

    // Set value from an integer (as string)
    void setValue(int newValue) { value = std::to_string(newValue); }

    // Low-level operations
    void* getAddress() { return static_cast<void*>(&value); }

    // Return the string value
    std::string toString(int indentLevel = 0) const override;

    // Convert string to a number, return primitive Number (e.g., int, long long, or double)
    auto toNumber();

    // Check if string can be parsed as a valid number
    bool isNumber() const;

    // String length
    size_t length() const;

    // Slice the string and return a primitive string (not wrapped in String object)
    std::string slice(int start, int end) const;

    // Extract a substring and return a primitive string
    std::string substring(int start, int end) const;

    // Convert to lowercase and return a new string
    std::string toLowerCase() const;

    // Convert to uppercase and return a new string
    std::string toUpperCase() const;

    // Trim whitespace from both ends of the string
    std::string trim() const;

    // Replace a substring and return the modified string
    std::string replace(const std::string& from, const std::string& to) const;

    // Character at a specific index
    char charAt(int index) const;

    // Index of the first occurrence of a substring
    int indexOf(const std::string& substr) const;

    // Check if string contains a substring
    bool includes(const std::string& substr) const;

    // Check if the string starts with a given substring
    bool startsWith(const std::string& prefix) const;

    // Check if the string ends with a given substring
    bool endsWith(const std::string& suffix) const;

    // Split the string into an array of substrings
    std::vector<std::string> split(const std::string& delimiter) const;

    // Convert string to boolean (empty string is false)
    bool toBoolean() const;

    // Check if the string is empty
    bool isEmpty() const;

    // Check if the string consists of only whitespace
    bool isWhitespace() const;
};



