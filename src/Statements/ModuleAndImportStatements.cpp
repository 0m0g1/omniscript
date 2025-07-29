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

namespace Omniscript {
std::shared_ptr<Omniscript::Expression> IncludeStatement::express(SymbolTableType scope) {
    Omniscript::setSpanFromPosition(span.start.line, span.start.col, span.start.filePath);
    return nullptr;
}

std::vector<std::shared_ptr<Statement>> IncludeStatement::getStatements() {
    if (path.empty()) {
        std::string suggestion = "To resolve this:\n"
                               "1. Ensure the include directive specifies a valid file path\n"
                               "2. Check for correct file path syntax\n"
                               "3. Verify the file exists in the include directory";
        console.reportError(
            Omniscript::Console::RUNTIME_ERROR,
            "Include path is empty",
            suggestion,
            getSpan()
        );
        return {};
    }

    std::string sourceCode = readFile(path);
    if (sourceCode.empty()) {
        std::string suggestion = Omniscript::Console::formatString(
            "To resolve this:\n"
            "1. Verify the file '%s' exists\n"
            "2. Check file permissions\n"
            "3. Ensure correct file path is provided",
            path.c_str()
        );
        console.reportError(
            Omniscript::Console::RUNTIME_ERROR,
            Omniscript::Console::formatString("Failed to read file: '%s'", path.c_str()),
            suggestion,
            getSpan()
        );
        return {};
    }

    Lexer lexer(sourceCode, path);
    Parser parser(lexer);
    std::vector<std::shared_ptr<Statement>> parsedStatements = parser.parse();

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
            if (!memberExprValue) {
                std::string suggestion = Omniscript::Console::formatString(
                    "To resolve this:\n"
                    "1. Verify member '%s' in module '%s' is correctly defined\n"
                    "2. Check member expression validity\n"
                    "3. Add debug output for member evaluation",
                    member->getName().c_str(), modulePath.c_str()
                );
                console.reportError(
                    Omniscript::Console::RUNTIME_ERROR,
                    Omniscript::Console::formatString("Failed to evaluate module member '%s' in module '%s'",
                                     member->getName().c_str(), modulePath.c_str()),
                    suggestion,
                    member->getSpan()
                );
                return nullptr;
            }
            expressions.push_back(memberExprValue);
        }

        if (!std::dynamic_pointer_cast<FunctionDeclaration>(member->getValue()) && !expressions.empty()) {
            DEBUG_LOG("Appending module member '" + member->getName() + "'.");
            auto memberExpr = std::make_shared<Omniscript::ModuleMemberExpression>(
                member->getName(),
                expressions.back(),
                member->getModifiers()
            );
            members.push_back(memberExpr);
        }
    }

    for (int i = 0; i < funcs.size(); i++) {
        const auto& func = funcs[i];
        auto result = func->express(moduleScope);
        if (!result) {
            std::string suggestion = Omniscript::Console::formatString(
                "To resolve this:\n"
                "1. Verify function '%s' in module '%s' is correctly defined\n"
                "2. Check function body and parameters\n"
                "3. Add debug output for function compilation",
                func->getName().c_str(), modulePath.c_str()
            );
            console.reportError(
                Omniscript::Console::RUNTIME_ERROR,
                Omniscript::Console::formatString("Failed to compile function '%s' in module '%s'",
                                 func->getName().c_str(), modulePath.c_str()),
                suggestion,
                func->getSpan()
            );
            return nullptr;
        }
        expressions.push_back(result);

        auto memberExpr = std::make_shared<Omniscript::ModuleMemberExpression>(
            expressions.back()->getName(),
            expressions.back(),
            funcsModifiers[i]
        );
        
        members.push_back(memberExpr);
    }

    auto module = std::make_shared<Omniscript::ModuleExpression>(name, members);
    scope->setConstant(name, module);
    scope->defineModule(modulePath, moduleScope);

    auto block = std::make_shared<Omniscript::BlockExpression>(expressions);
    block->setSpan(getSpan());
    return block;
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

    std::string suggestion = Omniscript::Console::formatString(
        "To resolve this:\n"
        "1. Ensure statement '%s' is a valid module member\n"
        "2. Check for supported statement types (assignment, function, struct, class)\n"
        "3. Verify statement syntax",
        statement->toString().c_str()
    );
    console.reportError(
        Omniscript::Console::RUNTIME_ERROR,
        Omniscript::Console::formatString("Invalid statement type for reinterpretation: '%s'",
                         statement->toString().c_str()),
        suggestion,
        statement->getSpan()
    );
    return nullptr;
}

std::shared_ptr<Omniscript::Expression> ImportModule::express(SymbolTableType scope) { 
    Omniscript::setSpanFromPosition(span.start.line, span.start.col, span.start.filePath);
    if (path.empty()) {
        std::string suggestion = "To resolve this:\n"
                               "1. Ensure the import directive specifies a valid module path\n"
                               "2. Check for correct module path syntax\n"
                               "3. Verify the module file exists";
        console.reportError(
            Omniscript::Console::RUNTIME_ERROR,
            "Module path is empty",
            suggestion,
            getSpan()
        );
        return nullptr;
    }

    if (path == "std") {
        path = "standard/1/std.os";
    }

    std::string sourceCode = readFile(path);
    if (sourceCode.empty()) {
        std::string suggestion = Omniscript::Console::formatString(
            "To resolve this:\n"
            "1. Verify the module file '%s' exists\n"
            "2. Check file permissions\n"
            "3. Ensure correct file path is provided",
            path.c_str()
        );
        console.reportError(
            Omniscript::Console::RUNTIME_ERROR,
            Omniscript::Console::formatString("Failed to read module file: '%s'", path.c_str()),
            suggestion,
            getSpan()
        );
        return nullptr;
    }

    Lexer lexer(sourceCode, path);
    Parser parser(lexer);
    std::vector<std::shared_ptr<Statement>> statements = parser.parse();

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
        std::string suggestion = Omniscript::Console::formatString(
            "To resolve this:\n"
            "1. Verify module '%s' is defined in file '%s'\n"
            "2. Check for correct module name\n"
            "3. Ensure module declaration exists",
            moduleName.c_str(), path.c_str()
        );
        console.reportError(
            Omniscript::Console::RUNTIME_ERROR,
            Omniscript::Console::formatString("Module '%s' not found in file '%s'",
                             moduleName.c_str(), path.c_str()),
            suggestion,
            getSpan()
        );
        return nullptr;
    }

    auto mod = moduleStmt->express(scope);
    if (!mod) {
        std::string suggestion = Omniscript::Console::formatString(
            "To resolve this:\n"
            "1. Verify module '%s' contains valid expressions\n"
            "2. Check module body for errors\n"
            "3. Add debug output for module evaluation",
            moduleName.c_str()
        );
        console.reportError(
            Omniscript::Console::RUNTIME_ERROR,
            Omniscript::Console::formatString("Invalid module '%s': not a ModuleExpression",
                             moduleName.c_str()),
            suggestion,
            moduleStmt->getSpan()
        );
        return nullptr;
    }

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
    return nullptr;
}

std::shared_ptr<Omniscript::Expression> ModuleMember::express(SymbolTableType scope) {
    Omniscript::setSpanFromPosition(span.start.line, span.start.col, span.start.filePath);
    if (auto assignment = std::dynamic_pointer_cast<Assignment>(value)) {
        assignment->setGlobalVisibilityTo(true);
    }
    auto val = value->express(scope);
    if (!val) {
        std::string suggestion = Omniscript::Console::formatString(
            "To resolve this:\n"
            "1. Verify member '%s' expression is valid\n"
            "2. Check member type and initialization\n"
            "3. Add debug output for member evaluation",
            getName().c_str()
        );
        console.reportError(
            Omniscript::Console::RUNTIME_ERROR,
            Omniscript::Console::formatString("Failed to evaluate module member '%s'",
                             getName().c_str()),
            suggestion,
            getSpan()
        );
        return nullptr;
    }
    val->setSpan(getSpan());
    return val;
}

}
