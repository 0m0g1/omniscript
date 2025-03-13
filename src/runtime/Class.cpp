#include <omniscript/runtime/Class.h>
#include <omniscript/omniscript_pch.h>
#include <omniscript/utils.h>

Class::Class(const std::string& className, std::vector<std::shared_ptr<Class>> parentClasses) {
    classNames.push_back(className);
    registerMethods();
    registerProperties();

    this->parentClasses = parentClasses;
}

Class::~Class() {

}

void Class::registerMethods() {
    for (const auto& parentClass : parentClasses) {
        if (parentClass && parentClass->constructor) {
            parentConstructors.push_back(parentClass->constructor->clone());
        }
        if (parentClass && parentClass->destructor) {
            parentDestructors.push_back(parentClass->destructor->clone());
        }
        
        // Share parent class methods with instance
        for (const auto& [methodName, methodPair] : parentClass->classMethods) {
            classMethods[methodName] = std::make_shared<std::pair<MethodDefinition, ClassMemberModifiers>>(
                methodPair->first, methodPair->second);
        }
    }
    
    addMethod("setProperty", [this](const ArgumentDefinition& args) -> SymbolTable::ValueType {
        if (args.size() < 2 || !std::holds_alternative<std::string>(args[0])) {
            console.error("A class property's name should be a string");
            return nullptr;
        }
        auto propertyName = std::get<std::string>(args[0]);
        if (args.size() == 3 && std::holds_alternative<std::shared_ptr<ClassMemberModifiers>>(args[2])) {
            auto propertyModifiers = *std::get<std::shared_ptr<ClassMemberModifiers>>(args[2]);
            this->setClassProperty(propertyName, args[1], propertyModifiers);
        } else {
            this->setClassProperty(propertyName, args[1], ClassMemberModifiers{});
        }
        return SymbolTable::ValueType{};
    });
    
    addMethod("setMethod", [this](const ArgumentDefinition& args) -> SymbolTable::ValueType {
        if (args.size() < 2 || !std::holds_alternative<std::string>(args[0]) || 
            !std::holds_alternative<std::shared_ptr<Statement>>(args[1])) {
            console.error("Invalid arguments for setMethod");
            return nullptr;
        }
        auto methodName = std::get<std::string>(args[0]);
        auto statement = std::get<std::shared_ptr<Statement>>(args[1]);
        auto method = std::dynamic_pointer_cast<Function>(statement);
        
        if (!method) {
            console.error("Failed to cast statement to Function");
            return nullptr;
        }
        
        if (args.size() == 3 && std::holds_alternative<std::shared_ptr<ClassMemberModifiers>>(args[2])) {
            auto methodModifiers = *std::get<std::shared_ptr<ClassMemberModifiers>>(args[2]);
            this->addClassMethod(methodName, method, methodModifiers);
        } else {
            this->addClassMethod(methodName, method, ClassMemberModifiers{});
        }
        return SymbolTable::ValueType{};
    });
    
    addMethod("getProperty", [this](const ArgumentDefinition& args) -> SymbolTable::ValueType {
        if (args.empty() || !std::holds_alternative<std::string>(args[0])) {
            console.error("A class property's name should be a string");
            return nullptr;
        }
        auto propertyName = std::get<std::string>(args[0]);
        auto [value, mods] = getClassProperty(propertyName);
        return std::holds_alternative<std::nullptr_t>(value) ? nullptr : value;
    });
    
    addMethod("getMethod", [this](const ArgumentDefinition& args) -> SymbolTable::ValueType {
        if (args.empty() || !std::holds_alternative<std::string>(args[0])) {
            console.error("A class method's name should be a string");
            return nullptr;
        }
        auto methodName = std::get<std::string>(args[0]);
        auto [value, mods] = getClassMethod(methodName);
        
        if (std::holds_alternative<std::nullptr_t>(value)) {
            return nullptr;
        }
        return std::get<std::shared_ptr<Function>>(value);
    });
}


void Class::registerProperties() {

}

// Add a method to the class
void Class::addClassMethod(const std::string& name, MethodDefinition method, const ClassMemberModifiers& modifiers) {
    console.debug("Added method: " + name);
    classMethods[name] = std::make_shared<std::pair<MethodDefinition, ClassMemberModifiers>>(method, modifiers);
}

// Retrieve a method from the class
std::pair<MethodDefinition, ClassMemberModifiers> Class::getClassMethod(const std::string& name) const {
    // Check in classMethods first
    auto it = classMethods.find(name);
    if (it != classMethods.end()) {
        return *(it->second); // Dereference shared_ptr to return the actual pair
    }

    // Check in methods map
    auto objMethodIt = methods.find(name);
    if (objMethodIt != methods.end()) {
        return {objMethodIt->second, ClassMemberModifiers()}; // Default modifiers
    }

    // Return empty method and default modifiers if not found
    return {MethodDefinition(), ClassMemberModifiers()};
}



// Set a property in the class
void Class::setClassProperty(const std::string& name, SymbolTable::ValueType value, const ClassMemberModifiers& modifiers) {
    console.debug("added property " + name);
    classProperties[name] = {value, modifiers};
}

// Get a property from the class
std::pair<SymbolTable::ValueType, ClassMemberModifiers> Class::getClassProperty(const std::string& name) const {
    // Check in classProperties first
    auto it = classProperties.find(name);
    if (it != classProperties.end()) {
        return it->second;
    }

    // Check in properties map
    auto objPropertyIt = properties.find(name);
    if (objPropertyIt != properties.end()) {
        return {objPropertyIt->second, ClassMemberModifiers()}; // Default modifiers
    }

    // Return empty if not found
    return {nullptr, ClassMemberModifiers()};
}

std::string Class::toString(int indentLevel) const {
    std::ostringstream oss;
    std::string indentation(indentLevel, '\t');
    oss << "{" << "\n";

    // Print properties
    for (const auto& [propertyName, propertyData] : classProperties) {
        const auto& [value, modifiers] = propertyData;
        oss << indentation << "\"" << propertyName << "\": " 
            << valueToString(value, indentLevel + 1)
            << " /* " << modifiers.toString() << " */\n";
    }

    // Print methods
    for (const auto& [methodName, methodData] : classMethods) {
        if (methodData) {  // Ensure the shared_ptr is not null
            const auto& [method, modifiers] = *methodData; // Dereference here
            oss << indentation << "\"" << methodName << "\": "
                << "/* Modifiers: " << modifiers.toString() << " */\n";
        }
    }

    oss << indentation << "}";
    return oss.str();
}

/*
Class insance
*/
ClassInstance::ClassInstance(std::shared_ptr<Class> parentClass) {
    this->parentClass = parentClass;
    this->constructors.push_back(parentClass->constructor->clone());
    classNames = std::make_shared<std::vector<std::string>>(parentClass->classNames);
    registerMethods();
    registerProperties();
}

ClassInstance::~ClassInstance() {
    console.warn("The class instance was destroyed here");
}

void ClassInstance::registerMethods() {
    if (!parentClass) {
        console.error("Parent class is null");
        return;
    }

    constructors.push_back(parentClass->constructor->clone());
    for (const auto& parent : parentClass->parentClasses) {
        constructors.push_back(parent->constructor->clone());
    }

    // Share class methods with instance
    for (const auto& [methodName, methodPair] : parentClass->classMethods) {
        ClassInstanceMethods[methodName] = std::make_shared<std::pair<MethodDefinition, ClassMemberModifiers>>(*methodPair);
    }

    addMethod("setProperty", [this](const ArgumentDefinition& args) -> SymbolTable::ValueType {
        if (!std::holds_alternative<std::string>(args[0])) {
            console.error("A class property's name should be a string");
            return SymbolTable::ValueType{};
        }
        auto propertyName = std::get<std::string>(args[0]);
        if (args.size() == 3 && std::holds_alternative<std::shared_ptr<ClassMemberModifiers>>(args[2])) {
            auto propertyModifiers = *(std::get<std::shared_ptr<ClassMemberModifiers>>(args[2]));
            this->setClassInstanceProperty(propertyName, args[1], propertyModifiers);
        } else {
            this->setClassInstanceProperty(propertyName, args[1], ClassMemberModifiers{});
        }
        return SymbolTable::ValueType{};
    });

    addMethod("setMethod", [this](const ArgumentDefinition& args) -> SymbolTable::ValueType {
        if (!std::holds_alternative<std::string>(args[0])) {
            console.error("A class method name should be a string");
            return SymbolTable::ValueType{};
        }
        if (!std::holds_alternative<std::shared_ptr<Function>>(args[1])) {
            console.error("A class instance method should be a valid statement");
            return SymbolTable::ValueType{};
        }
        auto methodName = std::get<std::string>(args[0]);
        auto method = std::get<std::shared_ptr<Function>>(args[1]);

        if (!method) {
            console.error("The provided method is not a valid function");
            return SymbolTable::ValueType{};
        }

        if (args.size() == 3 && std::holds_alternative<std::shared_ptr<ClassMemberModifiers>>(args[2])) {
            auto methodModifiers = *(std::get<std::shared_ptr<ClassMemberModifiers>>(args[2]));
            this->addClassInstanceMethod(methodName, method, methodModifiers);
        } else {
            this->addClassInstanceMethod(methodName, method, ClassMemberModifiers{});
        }
        return SymbolTable::ValueType{};
    });

    addMethod("getProperty", [this](const ArgumentDefinition& args) -> SymbolTable::ValueType {
        if (!std::holds_alternative<std::string>(args[0])) {
            console.error("A class property's name should be a string");
            return SymbolTable::ValueType{};
        }
        auto propertyName = std::get<std::string>(args[0]);
        auto [value, mods] = getClassInstanceProperty(propertyName);
        return std::holds_alternative<std::nullptr_t>(value) ? nullptr : value;
    });

    addMethod("getMethod", [this](const ArgumentDefinition& args) -> SymbolTable::ValueType {
        if (!std::holds_alternative<std::string>(args[0])) {
            console.error("A class method's name should be a string");
            return SymbolTable::ValueType{};
        }
        auto methodName = std::get<std::string>(args[0]);
        auto [value, mods] = getClassInstanceMethod(methodName);

        if (std::holds_alternative<std::nullptr_t>(value)) {
            return nullptr;
        }
        if (!std::holds_alternative<std::shared_ptr<Function>>(value)) {
            console.error("Requested method is not a valid function");
            return SymbolTable::ValueType{};
        }
        return std::get<std::shared_ptr<Function>>(value);
    });
}


void ClassInstance::registerProperties() {
    if (!parentClass) {
        console.error("Parent class is null");
        return;
    }

    // Share class methods with instance
    for (const auto& [name, valueModifiers] : parentClass->classProperties) {
        const auto& [value, modifiers] = valueModifiers;
        ClassInstanceProperties[name] = {value, modifiers};
    }
}

// Add or override an instance-specific method
void ClassInstance::addClassInstanceMethod(const std::string& name, MethodDefinition method, const ClassMemberModifiers& modifiers) {
    ClassInstanceMethods[name] = std::make_shared<std::pair<MethodDefinition, ClassMemberModifiers>>(method, modifiers);
}

std::pair<MethodDefinition, ClassMemberModifiers> ClassInstance::getClassInstanceMethod(const std::string& name) const {
    // Check instance methods first
    auto it = ClassInstanceMethods.find(name);
    if (it != ClassInstanceMethods.end()) {
        return *(it->second);
    }

    // Check parent class methods if parentClass is not null
    if (parentClass) {
        auto classIt = parentClass->classMethods.find(name);
        if (classIt != parentClass->classMethods.end()) {
            return *(classIt->second);
        }
    }

    // Check object-level methods
    auto objectIt = methods.find(name);
    if (objectIt != methods.end()) {
        return {objectIt->second, ClassMemberModifiers{}};
    }

    // Return an empty MethodDefinition instead of nullptr
    return {MethodDefinition{}, ClassMemberModifiers{}};
}


// Set or override an instance-specific property
void ClassInstance::setClassInstanceProperty(const std::string& name, SymbolTable::ValueType value, const ClassMemberModifiers& modifiers) {
    auto it = ClassInstanceMethods.find(name);
    if (it != ClassInstanceMethods.end()) {
        ClassInstanceMethods.erase(name);
    }
    ClassInstanceProperties[name] = {value, modifiers};
}

// Retrieve a property for the instance, with fallback to the class's property
std::pair<SymbolTable::ValueType, ClassMemberModifiers> ClassInstance::getClassInstanceProperty(const std::string& name) const {
    auto it = ClassInstanceProperties.find(name);
    if (it != ClassInstanceProperties.end()) {
        return it->second; // Instance-specific property
    }

    // Fallback to class properties
    auto classIt = parentClass->classProperties.find(name);
    if (classIt != parentClass->classProperties.end()) {
        return classIt->second; // Class-level property
    }

    auto objectIt = properties.find(name);
    if (objectIt != properties.end()) {
        return {objectIt->second, ClassMemberModifiers{}};
    }

    return {nullptr, ClassMemberModifiers{}};
}

std::string ClassInstance::toString(int indentLevel) const {
    std::ostringstream oss;
    std::string indentation(indentLevel, '\t');

    for (size_t i = 0; i < classNames->size(); ++i) {
        oss << (*classNames)[i];
        if (i != classNames->size() - 1) {
            oss << ", ";
        } else {
            oss << " ";
        }
    }

    oss << "{" << "\n";

    // Print properties
    for (const auto& [propertyName, propertyData] : ClassInstanceProperties) {
        const auto& [value, modifiers] = propertyData;
        oss << indentation + '\t' << "\"" << propertyName << "\": " << valueToString(value, indentLevel + 1)
            << " /* " << modifiers.toString() << " */\n";
    }

    // Print methods
    for (const auto& entry : ClassInstanceMethods) {
        const auto& methodName = entry.first; // Get the key
        const auto& methodData = entry.second; // Get the value (shared_ptr)

        if (methodData) { // Ensure shared_ptr is not null
            const auto& methodPair = *methodData; // Dereference shared_ptr
            const auto& method = methodPair.first; // Unpack first element
            const auto& modifiers = methodPair.second; // Unpack second element

            oss << indentation + '\t' << "\"" << methodName << "\": " << "\033[3mf\033[0m() "
                << "/* Modifiers: " << modifiers.toString() << " */\n";
        }
    }


    oss << indentation << "}";
    return oss.str();
}
