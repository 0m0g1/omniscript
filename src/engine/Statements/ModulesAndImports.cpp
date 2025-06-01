#include <omniscript/Core.h>
#include <omniscript/utils.h>
#include <omniscript/engine/Statement.h>
#include <omniscript/engine/Symboltable.h>
#include <omniscript/omniscript_pch.h>
#include <omniscript/engine/lexer.h>
#include <omniscript/engine/Parser.h>
#include <omniscript/utils.h>

std::shared_ptr<Omniscript::Expression> IncludeStatement::express(SymbolTableType scope) {
    return nullptr;
}

std::vector<std::shared_ptr<Statement>> IncludeStatement::getStatements() {
    if (path.empty()) {
        console.error("IncludeStatement::express - Include path is empty.");
        return {};
    }

    std::string sourceCode = readFile(path);
    if (sourceCode.empty()) {
        console.error("IncludeStatement::express - Failed to read file: " + path);
        return {};
    }

    Lexer lexer(sourceCode, path);
    Parser parser(lexer);
    std::vector<std::shared_ptr<Statement>> parsedStatements = parser.Parse();

    std::vector<std::shared_ptr<Statement>> finalStatements;

    for (const auto& stmt : parsedStatements) {
        if (auto mod = std::dynamic_pointer_cast<CreateModule>(stmt)) {
            for (const auto& inner : mod->getStatements()) {
                finalStatements.push_back(inner);
            }
        } else {
            finalStatements.push_back(stmt);
        }
    }

    return finalStatements;
}

std::shared_ptr<Omniscript::Expression> CreateModule::express(SymbolTableType scope) {
    DEBUG_LOG();
    DEBUG_LOG("Creating module '" + modulePath + "'.");

    std::vector<std::shared_ptr<Omniscript::Expression>> expressions;
    std::vector<std::shared_ptr<Statement>> parameterStatements;
    std::vector<std::shared_ptr<Statement>> constructorArgs;

    auto moduleType = std::make_shared<Omniscript::UserDefinedType>(name, Omniscript::Kind::Module);
    scope->addType(name, moduleType);

    // First process all includes (flatten the hierarchy)
    std::vector<std::shared_ptr<Statement>> flattenedStatements;
    for (const auto& stmt : statements) {
        if (auto include = std::dynamic_pointer_cast<IncludeStatement>(stmt)) {
            for (const auto& innerExpr : include->getStatements()) {
                flattenedStatements.push_back(innerExpr);
            }
        } else {
            flattenedStatements.push_back(stmt);
        }
    }


    // 1. Handle nested modules and prepare parameters
    std::vector<std::shared_ptr<FunctionDeclaration>> funcs = {};

    for (const auto& stmt : flattenedStatements) {
        auto member = std::dynamic_pointer_cast<ModuleMember>(stmt);
        if (!member) continue;

        if (auto func = std::dynamic_pointer_cast<FunctionDeclaration>(member->getValue())) {
            DEBUG_LOG(func->toString());
            auto funcExpr = reinterprateStatement(func);
            func->registerInScope(scope);
            funcs.push_back(func);
        }
    }

    for (const auto& stmt : flattenedStatements) {
        DEBUG_LOG(stmt->toString());

        auto member = std::dynamic_pointer_cast<ModuleMember>(stmt);
        if (!member) continue;
        
        extendContextOf(member->getValue());
        
        DEBUG_LOG("Passing '" + member->toString() + "' as a parameter to create module type '" + getName() + "_module_type'."); 

        if (auto nestedModule = std::dynamic_pointer_cast<ImportModule>(member->getValue())) {
            // Add nested module statement first
            auto nestedModuleExpressionBlock = std::dynamic_pointer_cast<Omniscript::BlockExpression>(nestedModule->express(scope));
            for (const auto& val : nestedModuleExpressionBlock->values) {
                expressions.push_back(val);
            }    

            // Create pointer-type parameter for the nested module
            auto paramStmt = std::make_shared<ParameterStatement>(member->getName());
            paramStmt->setType(Omniscript::Type::createPointerType(scope->getType(member->getName() + "_module_type")));
            parameterStatements.push_back(paramStmt);
            moduleType->paramTypes.push_back(scope->getType(member->getName() + "_module_type"));
        } else if (auto func = std::dynamic_pointer_cast<FunctionDeclaration>(member->getValue())) {
            continue;
        } else {
            // Direct value member parameter
            auto paramStmt = reinterprateStatement(member->getValue());
            moduleType->paramTypes.push_back(std::dynamic_pointer_cast<TypedStatement>(paramStmt)->getType());
            parameterStatements.push_back(paramStmt);
        }
    }

    for (const auto& func : funcs) {
        auto result = func->express(scope);
        expressions.push_back(result);

        // auto ref = std::make_shared<AddressOf>(result->getName());
        if (!func->isIntrinsic) {
            auto paramStmt = std::make_shared<ParameterStatement>(func->getName());
            paramStmt->setType(Omniscript::Type::createPointerType(result->getType()));
            moduleType->paramTypes.push_back(result->getType());
            parameterStatements.push_back(paramStmt);
        } else {
            expressions.push_back(result);
        }
    }

    // 2. Create the struct type for the module
    auto structStmt = std::make_shared<ConstructStructPrototype>(getName() + "_module_type", parameterStatements);
    expressions.push_back(structStmt->express(scope));

    // 3. Build arguments for constructor
    for (const auto& stmt : flattenedStatements) {
        auto member = std::dynamic_pointer_cast<ModuleMember>(stmt);
        if (!member) continue;

        DEBUG_LOG("Passing '" + member->toString() + "' as an argument to create an instance module type '" + getName() + "_module_type'.");
        
        if (auto nestedModule = std::dynamic_pointer_cast<ImportModule>(member->getValue())) {
            auto ref = std::make_shared<ReferenceTo>(member->getName());
            ref->setRootType(ref->getType());
            constructorArgs.push_back(std::make_shared<ArgumentStatement>(member->getName(), ref));
        } else if (auto func = std::dynamic_pointer_cast<FunctionDeclaration>(member->getValue())) {
            continue;
        } else if (auto classDeclr = std::dynamic_pointer_cast<ConstructClassPrototype>(member->getValue())) {
            continue;
        } else {
            DEBUG_LOG("Arg '" + member->getValue()->toString() + "' ");
            auto paramStmt = std::dynamic_pointer_cast<ParameterStatement>(reinterprateStatement(member->getValue()));
            // DEBUG_LOG("Arg '" + paramStmt->getDefaultValue()->toString() + "' ");
            constructorArgs.push_back(std::make_shared<ArgumentStatement>(member->getName(), paramStmt->getDefaultValue()));
        }
    }

    for (const auto& func : funcs) {
        if (!func->isIntrinsic) {
            auto ref = std::make_shared<AddressOf>(func->getName());
            constructorArgs.push_back(std::make_shared<ArgumentStatement>(func->getName(), ref));
        }
    }

    // 4. Create and evaluate the module instance
    auto instanceStmt = std::make_shared<ObjectConstructorStatement>(
        getName() + "_module_type",
        getName(),
        constructorArgs
    );
    expressions.push_back(instanceStmt->express(scope));

    // 5. Record the type of this module
    setType(scope->getType(getName() + "_module_type"));

    // 6. Return final BlockExpression wrapping all sub-expressions
    return std::make_shared<Omniscript::BlockExpression>(expressions);
}

std::shared_ptr<Statement> CreateModule::reinterprateStatement(std::shared_ptr<Statement> statement) {
    DEBUG_LOG("Reinterprating statement '" + statement->toString() + "'.");
    if (auto assignment = std::dynamic_pointer_cast<AssignVariable>(statement)) {
        auto memberStatement = std::make_shared<ParameterStatement>(assignment->getName(), assignment->getValue()->clone());
        if (!assignment->getType()) {
            DEBUG_LOG("Assignment has no type");
        } else {
            DEBUG_LOG("Assignment has a type of '" + assignment->getType()->toString() + "'.");
        }
        memberStatement->setType(assignment->getType());
        return memberStatement;
    } else if (auto function = std::dynamic_pointer_cast<FunctionDeclaration>(statement)) {
        function->setName(getName() + "." + function->getName());
        return function;
    } else if (auto classDeclr = std::dynamic_pointer_cast<ConstructClassPrototype>(statement)) {
        classDeclr->setName(getName() + "." + classDeclr->getName());
        return classDeclr;
    }

    return nullptr;
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
