// #pragma once
// #include <omniscript/runtime/Statement.h>
// #include <omniscript/runtime/object.h>
// #include <omniscript/runtime/Pointer.h>
// #include <omniscript/omniscript_pch.h>

// class Number : public Object {
// public:
//     using NumberVariant = std::variant<int, unsigned int, long, unsigned long, long long, unsigned long long, float, double, long double>;

// private:
//     SymbolTable::ValueType value;

//     // Helper function to initialize methods (assuming this is part of an existing framework)
//     void initializeMethods();

// public:
//     // Constructor for various numeric types
//     explicit Number(int value = 0) : value(value) { initializeMethods(); }
//     explicit Number(unsigned int value) : value(value) { initializeMethods(); }
//     explicit Number(long value) : value(value) { initializeMethods(); }
//     explicit Number(unsigned long value) : value(value) { initializeMethods(); }
//     explicit Number(long long value) : value(value) { initializeMethods(); }
//     explicit Number(unsigned long long value) : value(value) { initializeMethods(); }
//     explicit Number(float value) : value(value) { initializeMethods(); }
//     explicit Number(double value) : value(value) { initializeMethods(); }
//     explicit Number(long double value) : value(value) { initializeMethods(); }

//     // Get the value as a variant
//     SymbolTable::ValueType getValue() const {
//         return value;
//     }

//     // Set the value
//     void setValue(int newValue) { value = newValue; }
//     void setValue(unsigned int newValue) { value = newValue; }
//     void setValue(long newValue) { value = newValue; }
//     void setValue(unsigned long newValue) { value = newValue; }
//     void setValue(long long newValue) { value = newValue; }
//     void setValue(unsigned long long newValue) { value = newValue; }
//     void setValue(float newValue) { value = newValue; }
//     void setValue(double newValue) { value = newValue; }
//     void setValue(long double newValue) { value = newValue; }

//     // Convert string to the appropriate Number type
//     static SymbolTable::ValueType fromString(const std::string& str);

//     // Convert value to string
//     std::string toString(int indentLevel = 0) const override;


//     // Convert value to int
//     int toInt() const;


//     // Convert value to double
//     SymbolTable::ValueType toDouble() const;

//     // Return max value based on the type
//     SymbolTable::ValueType getMaxValue() const;


//     // Get max value for a specific type
//     static std::string getMaxValueFor(int type);

//     // Return type of number as string
//     std::string getType() const;

//     // Check if number is integer
//     bool isInteger() const;

//     // Check if number is float
//     bool isFloat() const;

//     // Check if string is a valid number
//     static bool isValidNumber(const std::string& str);

//     // Check if string is a number
//     static bool isNumber(const std::string& str) ;

//     // Check if string is a valid integer
//     static bool isValidInteger(const std::string& str);

//     // Check if string is a valid float
//     static bool isValidFloat(const std::string& str);

//     std::shared_ptr<PointerObj> asPointer();
// };
