#include <omniscript/utils.h>
#include <omniscript/runtime/symboltable.h>
#include <omniscript/runtime/Statement.h>
#include <omniscript/runtime/Function.h>
#include <omniscript/omniscript_pch.h>


// Set the parent scope
void SymbolTable::setParent(SymbolTable* parentScope) {
    this->parent = parentScope;
}

void SymbolTable::set(const std::string &name, std::optional<SymbolTable::ValueType> value) {
    // Debug: Print the value being passed to set
    if (value.has_value()) {
        console.debug("Setting variable: " + name + " with value: " + valueToString(value.value()));
    } else {
        console.debug("Setting variable: " + name + " with value: null");
    }

    SymbolTable* currentTable = this;

    // Traverse the parent scopes to check if the variable exists
    while (currentTable != nullptr) {
        console.debug("Checking variable '" + name + "' in scope " + currentTable->name);

        if (currentTable->constants.find(name) != currentTable->constants.end()) {
            // If it's a constant, we should throw a more descriptive error
            console.error("Constant '" + name + "' has already been defined in the scope " + currentTable->name);
            throw std::runtime_error("Constant '" + name + "' cannot be redefined.");
        }
        
        if (currentTable->variables.find(name) != currentTable->variables.end()) {
            // Variable found in current scope, update its value
            console.debug("Setting the variable '" + name + "' on scope '" + currentTable->name + "' with value " +
                          (value.has_value() ? valueToString(value.value()) : "null"));
            currentTable->variables[name] = value;
            return; // Exit the function after updating
        }
        currentTable = currentTable->parent; // Move to the parent scope
    }

    // If the variable does not exist in any parent scope, add it to the current scope
    console.debug("Setting the variable '" + name + "' on the scope '" + this->name + "' with value " +
                  (value.has_value() ? valueToString(value.value()) : "null"));
    variables[name] = value;
}


// Set constants in the SymbolTable
void SymbolTable::setConstant(const std::string &name, std::optional<SymbolTable::ValueType> value) {
    SymbolTable* currentTable = this;

    // Traverse through all parent tables to check if the constant is already defined
    while (currentTable) {
        if (currentTable->constants.find(name) != currentTable->constants.end()) {
            console.error("Error: Constant " + name + " has already been defined");
            // throw std::runtime_error("");
        }
        currentTable = currentTable->parent;
    }

    // If the constant is not found in any scope, set it in the current scope
    constants[name] = value;
}

// get variables from the variables table
std::optional<SymbolTable::ValueType> SymbolTable::getConstant(const std::string &name) {
    //Check if the variable is in the current scope
    if (constants.find(name) != constants.end()) {
        return constants.at(name);
    }

    // Transverse the list up if the variable isn't found in the current scope
    SymbolTable* currentParent = parent;
    while (currentParent != nullptr) {
        if (currentParent->constants.find(name) != currentParent->constants.end()) {
            return currentParent->constants.at(name);
        }
        currentParent = currentParent->parent;
    }

    // Throw an error if the variable isn't found in any scope
    throw std::runtime_error("Variable '" + name + "' was not found");
}


void SymbolTable::globalSet(const std::string &name, std::optional<ValueType> value) {
    SymbolTable* currentParent = this;
    
    // Traverse up the list until we find the global scope (where parent is nullptr)
    while (currentParent->parent != nullptr) {
        currentParent = currentParent->parent;
    }
    currentParent->set(name, value);
}

std::optional<SymbolTable::ValueType> SymbolTable::get(const std::string &name) {
    SymbolTable* currentScope = this;

    while (currentScope != nullptr) {
        console.debug("Currently looking for the symbol '" + name + "' in scope '" + currentScope->name + "'");

        // Check for the symbol in variables, constants, functions, and objects
        if (auto it = currentScope->variables.find(name); it != currentScope->variables.end()) {
            console.debug("Found the variable: " + name);
            return it->second; // Return the variable value
        }

        if (auto it = currentScope->constants.find(name); it != currentScope->constants.end()) {
            console.debug("Found the constant: " + name);
            return it->second; // Return the constant value
        }

        if (auto it = currentScope->functions.find(name); it != currentScope->functions.end()) {
            console.debug("Found the function: " + name);
            return it->second; // Return the function value
        }
        
        if (auto it = currentScope->genericFunctions.find(name); it != currentScope->genericFunctions.end()) {
            console.debug("Found the generic function: " + name);
            return it->second; // Return the function value
        }

        if (auto it = currentScope->objects.find(name); it != currentScope->objects.end()) {
            console.debug("Found the object: " + name);
            return it->second; // Return the object value
        }

        // Move to the parent scope
        currentScope = currentScope->parent;
    }

    // If the symbol is not found, log it
    console.debug("Did not find the symbol: " + name);
    return std::nullopt; // Not found
}


// add functions to the functions table
void SymbolTable::addFunction(const std::string &name, const std::shared_ptr<Function> &func) {
    functions[name] = func;
}

// Function to get functions from the functions table
std::shared_ptr<Function> SymbolTable::getFunction(const std::string &name) {
    //Check if the variable is in the current scope
    if (functions.find(name) != functions.end()) {
        return functions.at(name)->clone();
    }

    // Transverse the list up if the variable isn't found in the current scope
    SymbolTable* currentParent = parent;
    while (currentParent != nullptr) {
        if (currentParent->functions.find(name) != currentParent->functions.end()) {
            return currentParent->functions.at(name)->clone();
        }
        currentParent = currentParent->parent;
    }

    // Throw an error if the variable isn't found in any scope
    return nullptr;
}

void SymbolTable::addGenericFunction(const std::string &name, const std::shared_ptr<Function> &func) {
    genericFunctions[name] = func;
}

std::shared_ptr<Function> SymbolTable::getGenericFunction(const std::string &name) {
    //Check if the variable is in the current scope
    if (genericFunctions.find(name) != genericFunctions.end()) {
        return genericFunctions.at(name)->clone();
    }

    // Transverse the list up if the variable isn't found in the current scope
    SymbolTable* currentParent = parent;
    while (currentParent != nullptr) {
        if (currentParent->genericFunctions.find(name) != currentParent->genericFunctions.end()) {
            return currentParent->genericFunctions.at(name)->clone();
        }
        currentParent = currentParent->parent;
    }

    // Throw an error if the variable isn't found in any scope
    return nullptr;
}


void SymbolTable::addObject(const std::string &name, const std::shared_ptr<Object> &obj) {
    objects[name] = obj;
    console.debug("added object '" + name + "' to the scope");
}

std::shared_ptr<Object> SymbolTable::getObject(const std::string &name) {
    //Check if the variable is in the current scope
    if (objects.find(name) != objects.end()) {
        return objects.at(name);
    }

    // Transverse the list up if the variable isn't found in the current scope
    SymbolTable* currentParent = parent;
    while (currentParent != nullptr) {
        if (currentParent->objects.find(name) != currentParent->objects.end()) {
            return currentParent->objects.at(name);
        }
        currentParent = currentParent->parent;
    }

    // Throw an error if the variable isn't found in any scope
    throw std::runtime_error("Object '" + name + "' was not found");
}

void SymbolTable::unset(const std::string& name) {
    // Check and erase from variables
    if (variables.find(name) != variables.end()) {
        variables.erase(name);
        return;
    }

    // Check and erase from constants
    if (constants.find(name) != constants.end()) {
        constants.erase(name);
        return;
    }

    // Check and erase from functions
    if (functions.find(name) != functions.end()) {
        functions.erase(name);
        return;
    }

    // Check and erase from objects
    if (objects.find(name) != objects.end()) {
        objects.erase(name);
        return;
    }

    // If not found in any map, throw an error or handle it gracefully
    throw std::runtime_error("Symbol '" + name + "' not found in symbol table.");
}
