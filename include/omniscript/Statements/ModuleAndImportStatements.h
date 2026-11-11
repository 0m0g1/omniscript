// include/omniscript/Statements/ModuleAndImportStatements.h
#pragma once
#include <omniscript/Statements/Statement.h>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace Omniscript {

std::string canonicalTypeName(const std::string& name);

class IncludeStatement : public Statement {
public:
    std::string path;  // The path to the file to be included

    IncludeStatement(const std::string& includePath)
        : path(includePath) {}

    std::string getPath() const {
        return path;
    }

    // Runtime behavior — usually returns nullptr because includes are handled at parse/preprocess time
    std::shared_ptr<Statement> evaluate(SymbolTableType scope) override {
        return nullptr;
    }

    // Optional: Could represent this as a string literal in expression form
    std::shared_ptr<Expression> express(SymbolTableType scope) override;
    std::vector<std::shared_ptr<Statement>> getStatements();

    std::string toString() const override {
        return "Include \"" + path + "\";";
    }
    std::string formatError(const std::string& msg) const override {
        return "Error in '" + toString() + "'.\n" + msg;
    };
};

class ImportModule : public ContextAwareStatement {
public:
    std::string moduleName;
    std::string alias;
    std::unordered_map<std::string, std::string> importedAliases;
    std::string path;
    bool importAll;
    bool isPathImport = false; // true for import "./file.os" (explicit file), false for dotted logical names

    ImportModule(const std::string& modName, 
                 const std::string& aliasName, 
                 const std::unordered_map<std::string, std::string>& aliases, 
                 const std::string& modPath, 
                 bool wildcard)
        : moduleName(modName), alias(aliasName), importedAliases(aliases), path(modPath), importAll(wildcard) {}
    
    std::shared_ptr<Statement> evaluate(SymbolTableType scope) override { return nullptr; }
    std::shared_ptr<Expression> express(SymbolTableType scope) override;

    // Helper function to split the module path (e.g., "Math.Algebra.Matrix" -> {"Math", "Algebra", "Matrix"})
    std::vector<std::string> splitModulePath(const std::string& path);

    // Recursive function to resolve the module path in the scope
    std::shared_ptr<SymbolTableType> resolveModulePath(SymbolTableType scope, const std::vector<std::string>& modulePathComponents);

    std::string toString() const override {
        // Logical imports print unquoted/dotted; explicit file imports stay quoted.
        std::string target = isPathImport ? "\"" + moduleName + "\"" : moduleName;
        std::string result = "import ";
        if (importAll) {
            result += "* from " + target;
        } else if (!importedAliases.empty()) {
            result += "{ ";
            bool first = true;
            for (const auto& pair : importedAliases) {
                if (!first) result += ", ";
                // importedAliases maps alias -> original name
                result += pair.second;
                if (pair.first != pair.second) result += " as " + pair.first;
                first = false;
            }
            result += " } from " + target;
        } else if (!alias.empty()) {
            result += alias + " from " + target;
        } else {
            result += target;
        }
        return result;
    }

    std::string formatError(const std::string& msg) const override {
        return "Error in '" + toString() + "'.\n" + msg;
    }

    // Function to generate the module expression with member access (e.g., "Math.Algebra.Matrix" -> "Matrix")
    std::shared_ptr<Expression> generateModuleExpression(std::shared_ptr<SymbolTableType> module, const std::vector<std::string>& modulePathComponents);
};

class ModuleMember : public Member {
public:
    ModuleMember(const std::string& memberName, std::shared_ptr<Statement> value, MemberModifiers modifiers)
        : Member(memberName, value, modifiers) {}

    std::shared_ptr<Expression> express(SymbolTableType scope) override;
};

class CreateModule : public NamedStatement, public TypedStatement, public ContextAwareStatement {
private:
    std::string modulePath;

public:
    std::vector<std::shared_ptr<Statement>> statements;

    CreateModule(std::string moduleName, std::vector<std::shared_ptr<Statement>> stmts)
        : statements(stmts) {
        setName(moduleName);
    }
    
    std::string getName() const override { return name; }
    std::string getPath() const { return modulePath; }
    // Prefix under which this module's members are registered in the scope ("Outer.Inner."),
    // built exactly like reinterprateStatement() builds it. Valid after extendContextOf().
    std::string qualifiedPrefix() const {
        std::string context;
        for (size_t i = 0; i + 1 < accessContext.size(); ++i) context += accessContext[i] + ".";
        return context + name + ".";
    }
    void setPath(const std::string& newPath) { modulePath = newPath; }
    std::vector<std::shared_ptr<Statement>> getStatements() { return statements; }
    std::shared_ptr<Statement> evaluate(SymbolTableType scope) override { return nullptr; }
    std::shared_ptr<Expression> express(SymbolTableType scope) override;
    std::shared_ptr<Statement> reinterprateStatement(std::shared_ptr<Statement> statement);
    std::string toString() const override { return "Create module '" + name + "'."; }
    std::string formatError(const std::string& msg) const override {
        return "Error in '" + toString() + "'.\n" + msg;
    };
};

} // namespace Omniscript