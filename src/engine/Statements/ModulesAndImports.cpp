#include <omniscript/Core.h>
#include <omniscript/utils.h>
#include <omniscript/engine/Statement.h>
#include <omniscript/engine/Symboltable.h>
#include <omniscript/omniscript_pch.h>
#include <omniscript/engine/lexer.h>
#include <omniscript/engine/Parser.h>
#include <omniscript/utils.h>

std::shared_ptr<Omniscript::Expression> CreateModule::express(SymbolTableType scope) {
    std::vector<std::shared_ptr<Statement>> moduleStatements;
    std::vector<std::shared_ptr<Statement>> parameterStatements;

    // First, evaluate nested modules
    for (auto& stmt : statements) {
        auto member = std::dynamic_pointer_cast<ModuleMember>(stmt);
        if (!member) continue;

        if (auto nestedModule = std::dynamic_pointer_cast<CreateModule>(member->getValue())) {
            // Nested module should register its type and instance in the scope
            moduleStatements.push_back(nestedModule);

            // Now we can reference the nested module by name
            auto getModuleRef = std::make_shared<ReferenceTo>(member->getName());
            auto parameterStmt = std::make_shared<ParameterStatement>(member->getName(), getModuleRef);
            parameterStatements.push_back(parameterStmt);
        } else {
            auto parameterStmt = std::make_shared<ParameterStatement>(member->getName(), member->getValue());
            parameterStatements.push_back(parameterStmt);
        }
    }

    // Create struct type for this module
    auto structStmt = std::make_shared<ConstructStructPrototype>(getName() + "_type", parameterStatements);

    // Create module instance
    auto createModuleInstance = std::make_shared<ObjectConstructorStatement>(
        getName() + "_type",  // Type name
        getName(),            // Variable name
        std::vector<std::shared_ptr<Statement>>{}                    // Constructor args
    );

    // Add struct and constructor
    moduleStatements.push_back(structStmt);
    moduleStatements.push_back(createModuleInstance);

    // Block for all operations
    auto moduleBlock = std::make_shared<BlockStatement>(moduleStatements);

    // Evaluate block
    auto result = moduleBlock->express(scope);

    // Record the module's type
    setType(scope->getType(getName() + "_type"));

    // Wrap as BlockExpression so we return final result
    return std::make_shared<Omniscript::BlockExpression>(std::vector{result});
}


std::shared_ptr<Omniscript::Expression> ImportModule::express(SymbolTableType scope) { 
    if (path.empty()) {
        console.error("ImportModule::codegen - Module path is empty.");
    }

    if (path == "std") {
        path = "standard/1/std.os";
    }

    std::string sourceCode = readFile(path);
    if (sourceCode.empty()) {
        console.error("Failed to read module file: " + path);
        return nullptr;
    }

    Lexer lexer(sourceCode, path);
    Parser parser(lexer);
    std::vector<std::shared_ptr<Statement>> statements = parser.Parse();

    // Find the module we're importing
    std::shared_ptr<Statement> moduleStmt;
    std::shared_ptr<Omniscript::Expression> moduleValue;
    for (const auto& stmt : statements) {
        DEBUG_LOG("[ImportModule] Found statement '" + stmt->toString() + "' in file '" + path + "'.");
        if (auto createModule = std::dynamic_pointer_cast<CreateModule>(stmt)) {
            if (createModule->getName() == moduleName || moduleName.empty()) {
                createModule->setPath(path);
                moduleStmt = createModule;
                break;
            }
        }
    }

    if (!moduleStmt) {
        console.error("Invalid module: not a ModuleExpression");
        return nullptr;
    }

    if (!moduleStmt) {
        console.error("Module not found: " + moduleName);
        return nullptr;
    }
    
   
    // if (importAll) {
    //     for (const auto& member : moduleExpr->members) {
    //         // Directly add module members to the scope
    //         scope->set(member->name, member->value);
    //     }
    // } else if (!importedAliases.empty()) {
    //     for (const auto& [alias, original] : importedAliases) {
    //         // Search for the member with the name matching 'original'
    //         auto it = std::find_if(moduleExpr->members.begin(), moduleExpr->members.end(),
    //             [&original](const std::shared_ptr<Omniscript::ModuleMemberExpression>& member) {
    //                 return member->getName() == original;  // assuming you have a 'getName()' method
    //             });
    
    //         if (it != moduleExpr->members.end()) {
    //             scope->set(alias, (*it)->value);  // assuming you have a 'getValue()' method
    //         } else {
    //             console.error("Symbol not found in module: " + original);
    //         }
    //     }
    // } else if (!alias.empty()) {
    //     // Full module import with alias
    //     scope->aliasModule(alias, moduleName);
    //     scope->set(alias, moduleExpr);
    // } else {
    //     // Full module import without alias (optional — usually you'd assign it)
    //     scope->set(moduleName, moduleExpr);
    // }

    
    return moduleStmt->express(scope);
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
    // std::shared_ptr<SymbolTableType> currentScope = scope;

    // for (const std::string& component : modulePathComponents) {
    //     if (!currentScope->exists(component)) {
    //         return nullptr; // Module member doesn't exist at this point
    //     }

    //     // Check if this component is an alias, otherwise, resolve the module
    //     auto module = currentScope->getModule(component);
    //     if (module) {
    //         currentScope = module; // Move into the next nested module
    //     } else {
    //         return nullptr; // No module found for this component
    //     }
    // }

    // return currentScope; // Return the resolved module
    return nullptr;

}

// Function to generate module expression with member access (e.g., Math.Algebra.Matrix -> Matrix)
std::shared_ptr<Omniscript::Expression> ImportModule::generateModuleExpression(std::shared_ptr<SymbolTableType> module, const std::vector<std::string>& modulePathComponents) {
    // std::shared_ptr<Omniscript::Expression> expression = std::make_shared<Omniscript::ModuleExpression>(module);

    // for (size_t i = 0; i < modulePathComponents.size(); ++i) {
    //     std::shared_ptr<Omniscript::Expression> memberExpr = std::make_shared<Omniscript::MemberExpression>(expression, modulePathComponents[i]);
    //     expression = memberExpr;
    // }

    // return expression; // Return the final expression representing the nested module
    return nullptr;
}

std::shared_ptr<Omniscript::Expression> ModuleMember::express(SymbolTableType scope) {
    if (auto assignment = std::dynamic_pointer_cast<Assignment>(value)) {
        assignment->setGlobalVisibilityTo(true);
    }
    return value->express(scope);
}
