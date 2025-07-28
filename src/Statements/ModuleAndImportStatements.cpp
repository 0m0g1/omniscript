#include <omniscript/Statement.h>
#include <omniscript/Statements/FunctionStatement.h>
#include <omniscript/Statements/ExpressionStatements.h>
#include <omniscript/Statements/ModuleAndImportStatements.h>
#include <omniscript/Statements/ClassConstructorStatement.h>
#include <omniscript/Statements/StructConstructorStatement.h>
#include <omniscript/Statements/AssignmentAndGetterStatements.h>

#include <omniscript/Core.h>
#include <omniscript/utils.h>
#include <omniscript/Lexer.h>
#include <omniscript/Parser.h>
#include <omniscript/omniscript_pch.h>
#include <omniscript/Statement.h>
#include <omniscript/Symboltable.h>

#include <omniscript/Expressions/AggregateExpressions.h>
#include <omniscript/Expressions/BlockExpression.h>

std::shared_ptr<Omniscript::Expression> IncludeStatement::express(SymbolTableType scope) {
    Omniscript::setSpanFromPosition(span.start.line, span.start.col, span.start.filePath);
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
    Omniscript::setSpanFromPosition(span.start.line, span.start.col, span.start.filePath);
    DEBUG_LOG();
    DEBUG_LOG("Creating module '" + modulePath + "'.");

    std::vector<std::shared_ptr<Omniscript::Expression>> expressions;
    std::vector<std::shared_ptr<Omniscript::ModuleMemberExpression>> members;

    // auto moduleScope = std::make_shared<SymbolTable<std::shared_ptr<Omniscript::Expression>, std::shared_ptr<Omniscript::Type>>>(nullptr, modulePath);
    auto moduleScope = scope;

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
    std::vector<MemberModifiers> funcsModifiers = {};

    for (const auto& stmt : flattenedStatements) {
        auto member = std::dynamic_pointer_cast<ModuleMember>(stmt);
        if (!member) continue;
        
        extendContextOf(member->getValue());

        if (auto func = std::dynamic_pointer_cast<FunctionDeclaration>(member->getValue())) {
            DEBUG_LOG(func->toString());
            auto funcExpr = reinterprateStatement(func);
            func->registerInScope(moduleScope);
            funcs.push_back(func);
        }
    }

    for (const auto& stmt : flattenedStatements) {
        DEBUG_LOG(stmt->toString());
        
        auto member = std::dynamic_pointer_cast<ModuleMember>(stmt);
        if (!member) continue;
        
        auto val = member->getValue();
        
        DEBUG_LOG("Handling module member '" + member->toString() + "'."); 

        // Nested module handling
        if (auto nestedModule = std::dynamic_pointer_cast<ImportModule>(val)) {
            auto nestedExpr = nestedModule->express(moduleScope);
            if (auto block = std::dynamic_pointer_cast<Omniscript::BlockExpression>(nestedExpr)) {
                for (auto& e : block->values) {
                    expressions.push_back(e);
                }
            } else {
                expressions.push_back(nestedExpr);
            }
        } else if (auto func = std::dynamic_pointer_cast<FunctionDeclaration>(member->getValue())) {
            funcsModifiers.push_back(member->getModifiers());
        } else {
            // Direct value member parameter
            auto memberExprValue = reinterprateStatement(member->getValue())->express(moduleScope);
            expressions.push_back(memberExprValue);
        }

        if (!std::dynamic_pointer_cast<FunctionDeclaration>(member->getValue()) && !expressions.empty()) {
            DEBUG_LOG("Appending module member '" + member->getName() + "'.");
            auto memberExpr = std::make_shared<Omniscript::ModuleMemberExpression>(
                member->getName(),  // assuming expr is from express() and has a name
                expressions.back(),
                member->getModifiers()
            );
            members.push_back(memberExpr);
        }
    }


    for (int i = 0; i < funcs.size(); i++) {
        const auto& func = funcs[i];
        auto result = func->express(moduleScope);
        expressions.push_back(result);

        auto memberExpr = std::make_shared<Omniscript::ModuleMemberExpression>(
            expressions.back()->getName(),  // assuming expr is from express() and has a name
            expressions.back(),
            funcsModifiers[i]
        );
        
        members.push_back(memberExpr);
    }

    auto module = std::make_shared<Omniscript::ModuleExpression>(name, members);
    scope->setConstant(name, module);
    scope->defineModule(modulePath, moduleScope);

    return std::make_shared<Omniscript::BlockExpression>(expressions);
}

std::shared_ptr<Statement> CreateModule::reinterprateStatement(std::shared_ptr<Statement> statement) {
    DEBUG_LOG("Reinterprating statement '" + statement->toString() + "'.");
    std::string context;

    for (int i = 0; i < accessContext.size() - 1; i++) {
        std::string& ctx = accessContext[i];
        context += ctx + ".";
    }
 
    if (auto assignment = std::dynamic_pointer_cast<AssignVariable>(statement)) {
        assignment->setName(context + getName() + "." + assignment->getName());
        if (!assignment->getType()) {
            DEBUG_LOG("Assignment has no type");
        } else {
            DEBUG_LOG("Assignment has a type of '" + assignment->getType()->toString() + "'.");
        }
        return assignment;
    } else if (auto function = std::dynamic_pointer_cast<FunctionDeclaration>(statement)) {
        function->setName(context + getName() + "." + function->getName());
        return function;
    } else if (auto structDeclr = std::dynamic_pointer_cast<ConstructStructPrototype>(statement)) {
        structDeclr->setName(context + getName() + "." + structDeclr->getName());
        return structDeclr;
    } else if (auto classDeclr = std::dynamic_pointer_cast<ConstructClassPrototype>(statement)) {
        classDeclr->setName(context + getName() + "." + classDeclr->getName());
        return classDeclr;
    }

    return nullptr;
}

std::shared_ptr<Omniscript::Expression> ImportModule::express(SymbolTableType scope) { 
    Omniscript::setSpanFromPosition(span.start.line, span.start.col, span.start.filePath);
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
            extendContextOf(createModule);
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

    
    auto mod = moduleStmt->express(scope);
    mod->setSpan(getSpan());
    return mod;
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
    Omniscript::setSpanFromPosition(span.start.line, span.start.col, span.start.filePath);
    if (auto assignment = std::dynamic_pointer_cast<Assignment>(value)) {
        assignment->setGlobalVisibilityTo(true);
    }
    auto val = value->express(scope);
    val->setSpan(getSpan());
    return val;
}
