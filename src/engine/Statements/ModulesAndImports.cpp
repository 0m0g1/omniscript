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

    Lexer lexer(sourceCode);
    Parser parser(lexer);
    std::vector<std::shared_ptr<Statement>> statements = parser.Parse();

    // Find the module we're importing
    std::shared_ptr<Omniscript::Expression> moduleValue;
    for (const auto& stmt : statements) {
        if (auto createModule = std::dynamic_pointer_cast<CreateModule>(stmt)) {
            if (createModule->getName() == moduleName || moduleName.empty()) {
                moduleValue = createModule->express(scope);
                break;
            }
        }
    }

    if (!moduleValue) {
        console.error("Module not found: " + moduleName);
        return nullptr;
    }

    auto moduleExpr = std::dynamic_pointer_cast<Omniscript::ModuleExpression>(moduleValue);
    if (!moduleExpr) {
        console.error("Invalid module: not a ModuleExpression");
        return nullptr;
    }

    if (importAll) {
        for (const auto& member : moduleExpr->members) {
            // Directly add module members to the scope
            scope->set(member->name, member->value);
        }
    } else if (!importedAliases.empty()) {
        for (const auto& [alias, original] : importedAliases) {
            // Search for the member with the name matching 'original'
            auto it = std::find_if(moduleExpr->members.begin(), moduleExpr->members.end(),
                [&original](const std::shared_ptr<Omniscript::ModuleMemberExpression>& member) {
                    return member->getName() == original;  // assuming you have a 'getName()' method
                });
    
            if (it != moduleExpr->members.end()) {
                scope->set(alias, (*it)->value);  // assuming you have a 'getValue()' method
            } else {
                console.error("Symbol not found in module: " + original);
            }
        }
    } else if (!alias.empty()) {
        // Full module import with alias
        scope->aliasModule(alias, moduleName);
        scope->set(alias, moduleExpr);
    } else {
        // Full module import without alias (optional — usually you'd assign it)
        scope->set(moduleName, moduleExpr);
    }

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


std::shared_ptr<Omniscript::Expression> CreateModule::express(SymbolTableType scope) {
    std::vector<std::shared_ptr<Omniscript::ModuleMemberExpression>> memberExpressions;

    for (auto& stmt : statements) {
        auto member = std::dynamic_pointer_cast<ModuleMember>(stmt);
        if (!member) continue;
    
        std::shared_ptr<Omniscript::Expression> result = member->express(scope);
        auto memberExpr = std::make_shared<Omniscript::ModuleMemberExpression>(
            member->getName(), result, member->getModifiers()
        );
        memberExpressions.push_back(memberExpr);
    }
    
    std::shared_ptr<Omniscript::Expression> moduleExpr = std::make_shared<Omniscript::ModuleExpression>(name, memberExpressions);
    return moduleExpr;
}

std::shared_ptr<Omniscript::Expression> ModuleMember::express(SymbolTableType scope) {
    if (auto assignment = std::dynamic_pointer_cast<Assignment>(value)) {
        assignment->setGlobalVisibilityTo(true);
    }
    return value->express(scope);
}
