#ifndef Object_H
#define Object_H

#include <omniscript/omniscript_pch.h>
#include <omniscript/runtime/symboltable.h>

/*
* @author 0m0g1
*/

#include <atomic>

using ArgumentDefinition = std::vector<SymbolTable::ValueType>;
using MethodDefinition = std::variant<
                            std::nullptr_t,
                            // std::shared_ptr<Function>,
                            std::function<SymbolTable::ValueType(const ArgumentDefinition&)>
                            >;
                            
class Object {
protected:
    std::string name;
    std::shared_ptr<SymbolTable> scope;
    std::string id;
    std::vector<std::string> classes;
    std::string className = "Object";

    // Static counter for unique ID generation
    static std::atomic<unsigned long long> objectCounter;

public:
    std::unordered_map<std::string, MethodDefinition> methods;
    std::unordered_map<std::string, SymbolTable::ValueType> properties;

    Object() {
        // Generate unique ID based on current time and static counter
        id = generateUniqueId();

        // Initialize with default method
        addMethod("objectString", [&, this](const ArgumentDefinition& args) -> SymbolTable::ValueType {
            return objectString();
        });

        addMethod("setProperty", [this](const ArgumentDefinition& args) -> SymbolTable::ValueType {
            if (args.size() < 2) {
                throw std::runtime_error("setProperty requires 2 arguments: name and value");
            }
            std::string name = std::get<std::string>(args[0]); // Ensure args[0] is a string
            setProperty(name, args[1]);
            return SymbolTable::ValueType{}; // Return type can be adjusted based on requirements
        });

        addMethod("getProperty", [this](const ArgumentDefinition& args) -> SymbolTable::ValueType {
            if (args.size() < 1) {
                throw std::runtime_error("getProperty requires 1 argument: name");
            }
            std::string name = std::get<std::string>(args[0]); // Ensure args[0] is a string
            return getProperty(name);
        });
    }

    virtual ~Object() = default;

    std::string getName() {
        return name;
    }

    inline virtual void setProperty(std::string name, SymbolTable::ValueType value) {
        properties[name] = value;
    }

    inline virtual SymbolTable::ValueType getProperty(const std::string& name) const {
        // Check if the name exists in properties
        auto propIt = properties.find(name);
        if (propIt != properties.end()) {
            return propIt->second;
        }

        // Check if the name exists as a method
        // auto methodIt = methods.find(name);
        // if (methodIt != methods.end()) {
        //     auto method = methodIt->second;
        //     if (std::holds_alternative<std::shared_ptr<Function>>(method)) {
        //         return std::get<std::shared_ptr<Function>>(method);
        //     }
        // }

        // Return null if neither property nor method exists
        return nullptr;
    }


    inline virtual void addMethod(const std::string& name, MethodDefinition method) {
        methods[name] = method;
    }

    inline virtual MethodDefinition getMethod(const std::string& methodName) {
        auto it = methods.find(methodName);
        if (it != methods.end()) {
            return it->second; // Return the found method
        } 
        return nullptr;
    }

    // Convert the whole object and its properties to a string for printing
    std::string objectString() const;

    // Convert the value of the object to a string for printing
    virtual std::string toString(int indentLevel = 0) const;

private:
    std::string generateUniqueId() const;

    inline void addClass(const std::string& className) {
        classes.push_back(className);
    }
};

/*
* Console
* Canvas
*/

/*
* Booloens
*/

// Boolean class derived from Object
class Boolean : public Object {
private:
    bool value;  // The boolean value

public:
    explicit Boolean(bool value = false);

    // Additional member functions (example):
    bool getValue() const { return value; }
    void setValue(bool newValue) { value = newValue; }

    std::string toString(int indentLevel = 0) const override {
        return value ? "true" : "false";
    }

    int toInt() const {
        return value ? 1 : 0;
    }

    bool isTrue() const { return value; }
    bool isFalse() const { return !value; }
    bool negate() { return !value; }
};


// 1. Core Concepts
// Everything is an Object: Even primitive types like integers, floats, and functions are objects. They can have properties and methods.
// Low-Level Access: Provide ways to interact directly with memory, CPU registers, pointers, or hardware, similar to C or C++.

  
        

// 2. Objects for Everything
// Primitives an other objects
// Implementation:
// 1. Wrap low-level constructs (like integers, arrays, or memory) in object abstractions.
// Allow objects to interact with low-level APIs or perform memory management explicitly.
// Define the Number class that can hold multiple numeric types

class Array : public Object {
public:
    std::vector<SymbolTable::ValueType> elements;
    // Constructor with an optional initializer list
    Array(std::vector<SymbolTable::ValueType> elements = {});
    // Array(std::vector<SymbolTable::ValueType>& elements);
    
    // Convert array to string (for printing)
    std::string toString(int indentLevel = 0) const override;
};

// Sets
// class Set : public Object {
// private:
//     std::unordered_set<std::shared_ptr<Object>> elements;

// public:
//     void add(const std::shared_ptr<Object>& element);
//     void remove(const std::shared_ptr<Object>& element);
//     bool has(const std::shared_ptr<Object>& element) const;
//     int size() const;
// };

//Tuples
class Tuple : public Object {
private:
    std::vector<std::shared_ptr<Object>> elements;

public:
    explicit Tuple(const std::vector<std::shared_ptr<Object>>& elements);
    int length() const;
    std::shared_ptr<Object> get(int index) const;
    std::string toString(int indentLevel = 0) const override;
};


//Null object
class Null : public Object {
public:
    std::string toString(int indentLevel = 0) const override {
        return "null";
    }
};


// Undefined
class Undefined : public Object {
public:
    std::string toString(int indentLevel = 0) const override {
        return "undefined";
    }
};

// Error object
class Error : public Object {
private:
    std::string message;

public:
    explicit Error(const std::string& message = "");
    std::string toString(int indentLevel = 0) const override;
};

//exception
class Exception : public Object {
private:
    std::string message;

public:
    explicit Exception(const std::string& message);
    std::string toString(int indentLevel = 0) const override;
};


// Date object
// class Date : public Object {
// private:
//     std::chrono::system_clock::time_point value;

// public:
//     explicit Date(const std::chrono::system_clock::time_point& value = std::chrono::system_clock::now());
//     std::string toString(int indentLevel = 0) const override;
// };


// Symbol object
class Symbol : public Object {
private:
    std::string description;

public:
    explicit Symbol(const std::string& description = "");
    std::string toString(int indentLevel = 0) const override;
};


// Iterator object
class Iterator : public Object {
private:
    std::vector<std::shared_ptr<Object>>::iterator current;
    std::vector<std::shared_ptr<Object>>::iterator end;

public:
    Iterator(std::vector<std::shared_ptr<Object>>::iterator start, std::vector<std::shared_ptr<Object>>::iterator end);
    std::shared_ptr<Object> next();
    bool hasNext();
};


//Iterable
class Iterable : public Object {
private:
    std::vector<std::shared_ptr<Object>> collection;
    size_t currentIndex = 0;

public:
    Iterable(const std::vector<std::shared_ptr<Object>>& collection);
    bool hasNext() const;
    std::shared_ptr<Object> next();
};


//Range
class Range : public Object {
private:
    int start, end, step;

public:
    Range(int start, int end, int step = 1);
    std::shared_ptr<Object> next();
    bool hasNext() const;
};


// Map object
class Map : public Object {
private:
    std::unordered_map<std::string, std::shared_ptr<Object>> keyValuePairs;

public:
    void set(const std::string& key, const std::shared_ptr<Object>& value);
    std::shared_ptr<Object> get(const std::string& key);
    bool has(const std::string& key) const;
};

// Dictionary object
class Dictionary : public Object {
public:
    Dictionary();
    
    // Add a key-value pair to the dictionary
    void set(const std::string& key, const SymbolTable::ValueType& value);

    // std::string toString(int indentLevel = 0) const override;

    // Get the value associated with a key
    SymbolTable::ValueType get(const std::string& key) const;

    // Check if the dictionary has a certain key
    bool has(const std::string& key) const;

    // Remove a key-value pair from the dictionary
    void remove(const std::string& key);

    // Get all the keys in the dictionary
    std::shared_ptr<Object> keys() const;

    // Get all the values in the dictionary
    std::shared_ptr<Object> values() const;

    // Get all the entries in the dictionary as key-value pairs
    std::shared_ptr<Object> entries() const;
};


//for async behaviour
class Coroutine : public Object {
private:
    std::function<SymbolTable::ValueType()> coroutineFn;
    bool isCompleted;

public:
    Coroutine(std::function<SymbolTable::ValueType()> coroutineFn);
    bool awaitNext();
    bool isDone() const;
    std::shared_ptr<Object> then(std::function<void()> successFn);
    std::shared_ptr<Object> catchError(std::function<void()> errorFn);
};

// //lambda
// class Lambda : public Function {
// public:
//     explicit Lambda(std::function<SymbolTable::ValueType(const ArgumentDefinition&)> fn);
// };



// 4. Inline Assembly or Hardware Access
// Allow embedding assembly or direct hardware access for extreme low-level operations while still being object-oriented.
// class CPU : public Object {
// public:
//     void executeNop() {
//         __asm("nop");
//     }
// };


// 5. Objects for OS/Hardware Interaction
// Use objects to represent OS-level resources like files, sockets, or devices.
// class File : public Object {
//     private:
//         FILE* file;

//     public:
//         File(const std::string& path, const std::string& mode) {
//             file = fopen(path.c_str(), mode.c_str());
//             if (!file) {
//                 throw std::runtime_error("Failed to open file");
//             }
//         }

//         ~File() {
//             if (file) fclose(file);
//         }

//         void write(const std::string& data) {
//             fwrite(data.c_str(), 1, data.size(), file);
//         }

//         std::string toString(int indentLevel = 0) const override {
//             return "[File Object]";
//         }
// };

// 6. Inline Low-Level Code in High-Level Objects
// Allow users to write low-level code in special blocks while keeping objects everywhere.
// class Memory: public Object {
//     private:
//         void* address;

//     public:
//         explicit Memory(size_t size) {
//             address = malloc(size);  // Allocate memory
//         }

//         ~Memory() {
//             free(address);  // Free allocated memory
//         }

//         void clear() {
//             // Use memset to zero out the memory
//             memset(address, 0, sizeof(address)); // Clears the memory
//         }
// };


// 7. Design Philosophy
// To ensure low-level power and object-oriented design coexist:

// Compile to Native Code: Use a backend like LLVM to enable direct interaction with hardware while abstracting it in objects.
// Optimize Object Overhead: Keep object abstractions lightweight. For instance, don’t wrap every primitive in a large class; instead, use thin wrappers or inline methods.
// Type System Flexibility: Allow casting between high-level objects and raw memory types.


// 8. Language Examples
// Languages that have achieved a similar balance:

// C++: Combines low-level control with OOP features.
// Rust: Safe low-level programming with high-level abstractions.
// D Language: High-level syntax with low-level control.


// class LowLevelExample {
// public:
//     static void showExample() {
//         // Integer as object
//         auto integer = std::make_shared<Integer>(42);
//         std::cout << "Integer value: " << integer->toString() << std::endl;

//         // Pointer to the integer
//         Pointer ptr(integer->getAddress());
//         std::cout << "Pointer address: " << ptr.toString(int indentLevel = 0) << std::endl;

//         // Modify via pointer
//         ptr.setValue<int>(100);
//         std::cout << "Modified Integer value: " << integer->toString() << std::endl;
//     }
// };

// int main() {
//     LowLevelExample::showExample();
//     return 0;
// }

// output
// Integer value: 42
// Pointer address: 0x7ffc61b8e9f8
// Modified Integer value: 100



#endif