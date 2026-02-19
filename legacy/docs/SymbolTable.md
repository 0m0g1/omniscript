# SymbolTable Documentation

## Overview

The `SymbolTable` class in the Omniscript library is a versatile, thread-safe, and hierarchical symbol table implementation designed for managing symbols, types, modules, and scopes in a programming language or scripting environment. It supports variables, constants, function overloads, type definitions, and module management, with features like symbol validation, event listeners, and serialization for debugging.

The class is templated to allow flexibility in the type of values (`T`) and optional type information (`TypeT`). It is particularly suited for applications requiring robust symbol management, such as compilers, interpreters, or runtime environments.

## Key Features

- **Hierarchical Scoping**: Supports nested scopes with parent-child relationships, allowing symbol lookup in parent scopes.
- **Symbol Types**: Manages variables, constants, function overloads, and custom types (when `TypeT` is not `void`).
- **Module Management**: Handles module definitions, dependencies, imports, and exports with aliasing support.
- **Thread Safety**: Optional thread-safe operations using `std::shared_mutex` for concurrent access.
- **Event Listeners**: Supports callbacks for symbol and scope changes.
- **Serialization**: Provides serialization for debugging and persistence.
- **Iterator Support**: Offers iterator-based access to symbols, including public-only ranges.
- **Performance Optimization**: Includes memory management features like `reserve`, `optimize`, and `clearUnusedSymbols`.

## Usage

### Including the Header

To use the `SymbolTable` class, include the header file:

```cpp
#include <omniscript/SymbolTable.h>
```

### Instantiating a SymbolTable

The `SymbolTable` is a template class. You must specify the value type (`T`) and optionally a type for type definitions (`TypeT`). For example:

```cpp
#include <omniscript/SymbolTable.h>
#include <string>

using namespace Omniscript;

// SymbolTable with int values, no type definitions
SymbolTable<int> table;

// SymbolTable with string values and custom TypeT
struct MyType { std::string name; };
SymbolTable<std::string, MyType> typedTable;
```

### Creating a SymbolTable

Create a symbol table with an optional parent and name:

```cpp
auto rootTable = std::make_shared<SymbolTable<int>>("global");
auto childTable = rootTable->createChildScope("function_scope");
```

### Managing Symbols

#### Setting Variables and Constants

Use `setVariable` and `setConstant` to add symbols to the table:

```cpp
// Set a variable
rootTable->setVariable("x", 42, true); // public variable
rootTable->setVariable("y", 100, false); // private variable

// Set a constant
rootTable->setConstant("PI", 3.14159, true);
```

#### Updating Variables

Update non-constant variables using `updateVariable`:

```cpp
if (rootTable->updateVariable("x", 99)) {
    std::cout << "Updated x to 99\n";
} else {
    std::cout << "Failed to update x\n";
}
```

#### Retrieving Values

Retrieve values using `getValue` or `lookup`:

```cpp
int x = rootTable->getValue("x"); // Returns 99
auto result = rootTable->lookup("PI");
if (result.isConstantValue()) {
    std::cout << "PI is a constant with value: " << result.value << "\n";
}
```

#### Managing Function Overloads

Add overloads for functions using `addOverloadable`:

```cpp
rootTable->addOverloadable("func", 1, true); // Overload 1
rootTable->addOverloadable("func", 2, true); // Overload 2
std::vector<int> overloads = rootTable->getOverloads("func"); // Returns {1, 2}
```

#### Removing Symbols

Remove symbols using `removeSymbol`:

```cpp
if (rootTable->removeSymbol("y")) {
    std::cout << "Removed symbol y\n";
}
```

### Type Management

When `TypeT` is not `void`, you can manage type definitions:

```cpp
struct MyType { std::string name; };
SymbolTable<std::string, MyType> table;

MyType stringType{"string"};
table.addType("String", stringType, true);

if (table.typeExists<MyType>("String")) {
    MyType type = table.getType<MyType>("String");
    std::cout << "Type found: " << type.name << "\n";
}
```

### Scope Management

#### Creating Child Scopes

Create nested scopes using `createChildScope`:

```cpp
auto funcScope = rootTable->createChildScope("myFunction");
funcScope->setVariable("localVar", 10, false);
```

#### Navigating Scopes

Access parent, root, or specific scopes:

```cpp
auto parent = funcScope->getParent(); // Returns rootTable
auto root = funcScope->getRoot(); // Returns rootTable
auto foundScope = rootTable->findScope("myFunction"); // Returns funcScope
```

#### Scope Properties

Get scope information:

```cpp
std::string path = funcScope->getFullPath(); // Returns "global/myFunction"
size_t depth = funcScope->getScopeDepth(); // Returns 1
bool isGlobal = rootTable->isGlobalScope(); // Returns true
```

### Module Management

#### Defining Modules

Define modules with a path and associated symbol table:

```cpp
auto moduleTable = std::make_shared<SymbolTable<int>>("module");
moduleTable->setVariable("moduleVar", 123, true);
SymbolTable<int>::defineModule("lib/math", moduleTable);
```

#### Importing Symbols

Import symbols from a module:

```cpp
rootTable->aliasModule("math", "lib/math");
rootTable->importSymbol("moduleVar", "lib/math", "mathVar");
int value = rootTable->getValue("mathVar"); // Returns 123
```

#### Importing All Public Symbols

Import all public symbols from a module:

```cpp
rootTable->importAllPublicSymbols("lib/math", "math");
```

#### Exporting Symbols

Export symbols for use by other modules:

```cpp
rootTable->exportSymbol("x");
rootTable->exportAllSymbols(); // Exports all symbols in the scope
```

#### Managing Dependencies

Add and check module dependencies:

```cpp
SymbolTable<int>::addModuleDependency("lib/math", "lib/core");
if (SymbolTable<int>::hasCircularDependency("lib/math")) {
    std::cout << "Circular dependency detected\n";
}
```

### Advanced Features

#### Thread Safety

Enable thread-safe operations:

```cpp
rootTable->enableThreadSafety(true);
```

#### Symbol Validation

Set a validation function for symbols:

```cpp
rootTable->setValidationFunction([](const std::string& name, const int& value) {
    return value >= 0; // Only allow non-negative integers
});
rootTable->setVariable("x", -1, true); // Fails due to validation
```

#### Event Listeners

Add listeners for symbol or scope changes:

```cpp
rootTable->addSymbolChangeListener("x", [](const std::string& name, const int& value) {
    std::cout << "Symbol " << name << " changed to " << value << "\n";
});
rootTable->addScopeChangeListener([](const std::string& operation) {
    std::cout << "Scope changed: " << operation << "\n";
});
```

#### Serialization

Serialize the symbol table for debugging:

```cpp
std::string serialized = rootTable->serialize(true); // Include private symbols
std::cout << serialized;
```

#### Iterating Over Symbols

Use iterators to access symbols:

```cpp
for (const auto& [name, value] : rootTable->publicSymbols()) {
    std::cout << "Public symbol: " << name << " = " << value << "\n";
}
```

#### Performance Optimization

Optimize memory usage or reserve space:

```cpp
rootTable->reserve(100); // Reserve space for 100 symbols
rootTable->optimize(); // Shrink containers to fit
rootTable->clearUnusedSymbols(); // Remove non-constant, non-exported symbols
```

#### Statistics

Get statistics about the symbol table:

```cpp
auto stats = rootTable->getStatistics();
std::cout << "Total symbols: " << stats.totalSymbols << "\n";
std::cout << "Memory usage: " << stats.memoryUsage << " bytes\n";
```

## Example: Building a Simple Interpreter Scope

```cpp
#include <omniscript/SymbolTable.h>
#include <iostream>

int main() {
    using namespace Omniscript;
    
    // Create a global scope
    auto global = std::make_shared<SymbolTable<int>>("global");
    
    // Define some variables
    global->setVariable("x", 10, true);
    global->setConstant("MAX", 100, true);
    
    // Create a function scope
    auto funcScope = global->createChildScope("myFunc");
    funcScope->setVariable("y", 20, false);
    
    // Add a module
    auto mathModule = std::make_shared<SymbolTable<int>>("math");
    mathModule->setVariable("pi", 314, true);
    SymbolTable<int>::defineModule("lib/math", mathModule);
    
    // Import module and symbols
    global->aliasModule("math", "lib/math");
    global->importSymbol("pi", "lib/math", "math_pi");
    
    // Print values
    std::cout << "x = " << global->getValue("x") << "\n"; // 10
    std::cout << "math_pi = " << global->getValue("math_pi") << "\n"; // 314
    
    // Serialize for debugging
    std::cout << global->serialize(true);
    
    return 0;
}
```

## Thread Safety Considerations

- Enable thread safety with `enableThreadSafety(true)` for concurrent access.
- Be cautious with `getPointerToValue`, as it is not thread-safe even in thread-safe mode.
- Module management operations (e.g., `defineModule`, `addModuleDependency`) are protected by a global mutex (`modulesMutex_`).

## Best Practices

- Use `shared_ptr` for managing `SymbolTable` instances to ensure proper lifetime management.
- Reserve capacity (`reserve`) for large symbol tables to improve performance.
- Use validation functions to enforce constraints on symbol values.
- Regularly optimize (`optimize`) and clear unused symbols (`clearUnusedSymbols`) in long-running applications.
- Use `lookup` for comprehensive symbol information instead of direct `getValue` when type information is needed.

## Limitations

- `TypeT` must be explicitly defined if type management is needed; otherwise, use `void`.
- Direct pointer access (`getPointerToValue`) should be avoided in thread-safe mode.
- Circular dependencies in modules are detected but not automatically resolved.
- Serialization output is basic and may need customization for specific use cases.

## Dependencies

- Requires C++17 or later for `std::shared_mutex` and other features.
- Depends on `omniscript/Console.h` for logging (DEBUG_LOG, console.debug, console.error).
- Uses standard library containers (`unordered_map`, `vector`, `set`) and synchronization primitives.

## API Reference

For a complete list of methods and their signatures, refer to the `SymbolTable.h` header file. Key methods include:

- **Symbol Management**: `setVariable`, `setConstant`, `addOverloadable`, `getValue`, `lookup`, `removeSymbol`
- **Type Management**: `addType`, `getType`, `typeExists`, `removeType`, `getAllTypeNames`
- **Scope Management**: `createChildScope`, `getParent`, `getRoot`, `findScope`, `getFullPath`
- **Module Management**: `defineModule`, `getModuleByPath`, `importSymbol`, `exportSymbol`
- **Advanced Features**: `setValidationFunction`, `addSymbolChangeListener`, `serialize`, `getStatistics`
- **Iterators**: `begin`, `end`, `publicSymbols`