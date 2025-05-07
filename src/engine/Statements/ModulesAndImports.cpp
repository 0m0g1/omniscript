#include <omniscript/Core.h>
#include <omniscript/utils.h>
#include <omniscript/engine/Statement.h>
#include <omniscript/engine/Symboltable.h>
#include <omniscript/omniscript_pch.h>
#include <omniscript/engine/lexer.h>
#include <omniscript/engine/Parser.h>
#include <omniscript/utils.h>


std::shared_ptr<Omniscript::Expression> ImportModule::express(SymbolTableType scope) {
    if (path.empty()) {
        console.error("ImportModule::express - Module path is empty.");
    }

    if (path == "std") {
        path = "standard/1/std.os";
    }

    std::string sourceCode = readFile(path);
    if (sourceCode.empty()) {
        console.error("ImportModule::express - Failed to read module: " + path);
    }

    Lexer lexer(sourceCode, path);
    Parser parser(lexer);

    // Parse the module code into statements
    std::vector<std::shared_ptr<Statement>> statements = parser.Parse();

    DEBUG_LOG("Importing " + (importAll ? "everything" : joinMapKeys(importedAliases)) + " from " + path + ".");

    // Parse the module path into components (e.g., Math.Algebra.Matrix -> {"Math", "Algebra", "Matrix"})
    std::vector<std::string> modulePathComponents = splitModulePath(path);

    // Resolve the nested module path
    std::shared_ptr<SymbolTableType> resolvedModule = resolveModulePath(scope, modulePathComponents);

    if (!resolvedModule) {
        console.error("ImportModule::express - Module path '" + path + "' could not be resolved.");
        return nullptr; // Could not resolve module
    }

    // Generate the module expression with member access
    std::shared_ptr<Omniscript::Expression> moduleExpr = generateModuleExpression(resolvedModule, modulePathComponents);

    return moduleExpr;
}

// Helper function to split module path into components (e.g., Math.Algebra.Matrix -> {"Math", "Algebra", "Matrix"})
std::vector<std::string> ImportModule::splitModulePath(const std::string& path) {
    std::vector<std::string> components;
    size_t start = 0;
    size_t end = path.find('.');

    while (end != std::string::npos) {
        components.push_back(path.substr(start, end - start));
        start = end + 1;
        end = path.find('.', start);
    }

    components.push_back(path.substr(start)); // Add the last component
    return components;
}

// Recursive function to resolve the module path
std::shared_ptr<SymbolTableType> ImportModule::resolveModulePath(SymbolTableType scope, const std::vector<std::string>& modulePathComponents) {
    std::shared_ptr<SymbolTableType> currentScope = scope;

    for (const std::string& component : modulePathComponents) {
        if (!currentScope->exists(component)) {
            return nullptr; // Module member doesn't exist at this point
        }

        // Check if this component is an alias, otherwise, resolve the module
        auto module = currentScope->getModule(component);
        if (module) {
            currentScope = module; // Move into the next nested module
        } else {
            return nullptr; // No module found for this component
        }
    }

    return currentScope; // Return the resolved module
}

// Function to generate module expression with member access (e.g., Math.Algebra.Matrix -> Matrix)
std::shared_ptr<Omniscript::Expression> ImportModule::generateModuleExpression(std::shared_ptr<SymbolTableType> module, const std::vector<std::string>& modulePathComponents) {
    std::shared_ptr<Omniscript::Expression> expression = std::make_shared<Omniscript::ModuleExpression>(module);

    for (size_t i = 0; i < modulePathComponents.size(); ++i) {
        std::shared_ptr<Omniscript::Expression> memberExpr = std::make_shared<Omniscript::MemberExpression>(expression, modulePathComponents[i]);
        expression = memberExpr;
    }

    return expression; // Return the final expression representing the nested module
}


std::shared_ptr<Omniscript::Expression> CreateModule::express(SymbolTableType scope) {
    // generator.importModule(name);8
    return nullptr; // Modules themselves don't return a value
}

std::shared_ptr<Omniscript::Expression> ModuleMember::express(SymbolTableType scope) {
    if (auto assignment = std::dynamic_pointer_cast<Assignment>(value)) {
        assignment->setGlobalVisibilityTo(true);
    }
    return value->express(scope);
}
