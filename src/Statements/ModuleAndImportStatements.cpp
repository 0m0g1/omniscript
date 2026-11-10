// src/Statements/ModuleAndImportStatements.cpp
#include <algorithm>
#include <filesystem>
#include <functional>
#include <unordered_map>
#include <omniscript/Statements/Statement.h>
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
#include <omniscript/Statements/Statement.h>
#include <omniscript/Symboltable.h>

#include <omniscript/Expressions/AggregateExpressions.h>
#include <omniscript/Expressions/BlockExpression.h>
#include <omniscript/extern/ExternResolver.h>
#include <omniscript/Modules/ModuleResolver.h>

namespace Omniscript {

namespace {
// Finds the file for an include/import path. Tried in order: as given (working directory), next to the
// importing file, then the standard library ("std/x.os" -> "standard/1/x.os"). Returns the path unchanged
// if nothing matches so the caller can report the failure.
std::string resolveSourcePath(const std::string& path, const std::string& importerFile) {
    namespace fs = std::filesystem;
    std::error_code ec;
    std::vector<fs::path> candidates;
    candidates.emplace_back(path);
    fs::path importerDir = importerFile.empty() ? fs::path() : fs::path(importerFile).parent_path();
    if (!importerDir.empty()) candidates.emplace_back(importerDir / path);
    if (path.rfind("std/", 0) == 0 || path.rfind("std\\", 0) == 0) {
        fs::path rest = path.substr(4);
        candidates.emplace_back(fs::path("standard") / "1" / rest);
        if (!importerDir.empty()) candidates.emplace_back(importerDir / "standard" / "1" / rest);
    }
    for (const auto& c : candidates) {
        if (fs::is_regular_file(c, ec)) return c.string();
    }
    return path;
}

// readFile throws on a missing file; turn that into a failure the caller reports through the console.
bool tryReadSource(const std::string& path, std::string& out) {
    try {
        out = readFile(path);
        return true;
    } catch (const std::exception&) {
        out.clear();
        return false;
    }
}
} // namespace

std::shared_ptr<Expression> IncludeStatement::express(SymbolTableType scope) {
    setSpanFromPosition(span.start.line, span.start.col, span.start.filePath);

    // A top-level include used to be a no-op here (only CreateModule::express called
    // getStatements()), so nothing from the included file was ever declared.
    // Load the file and express its statements in the including scope.
    std::vector<std::shared_ptr<Expression>> expressions;
    for (const auto& stmt : getStatements()) {
        if (!stmt) continue;
        if (auto expr = stmt->express(scope)) {
            expressions.push_back(expr);
        }
    }

    auto block = std::make_shared<BlockExpression>(expressions);
    block->setSpan(this->getSpan());
    return block;
}

std::vector<std::shared_ptr<Statement>> IncludeStatement::getStatements() {
    if (path.empty()) {
        std::string suggestion = "To resolve this:\n"
                               "1. Ensure the include directive specifies a valid file path\n"
                               "2. Check for correct file path syntax\n"
                               "3. Verify the file exists in the include directory";
        console.reportError(
            Console::RUNTIME_ERROR,
            "Include path is empty",
            suggestion,
            getSpan()
        );
        return {};
    }

    const std::string resolvedPath = resolveSourcePath(path, getSpan().start.filePath);
    std::string sourceCode;
    const bool readOk = tryReadSource(resolvedPath, sourceCode);
    if (!readOk || sourceCode.empty()) {
        std::string suggestion = Console::formatString(
            "To resolve this:\n"
            "1. Verify the file '%s' exists\n"
            "2. Check file permissions\n"
            "3. Ensure correct file path is provided",
            path.c_str()
        );
        console.reportError(
            Console::RUNTIME_ERROR,
            Console::formatString("Failed to read file: '%s'", path.c_str()),
            suggestion,
            getSpan()
        );
        return {};
    }

    Lexer lexer(sourceCode, resolvedPath);
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

    // `extern "header.h", "lib.dll" { }` blocks are normally expanded after parsing the main
    // script; statements pulled in through an include never went through that step, so expand
    // them here (expand() skips blocks that were already resolved, so this is safe to repeat).
    {
        const std::string sourceDir = std::filesystem::path(resolvedPath).parent_path().string();
        extern_support::ExternResolver resolver(extern_support::ResolverConfig{}, sourceDir);
        resolver.expand(finalStatements);
    }

    return finalStatements;
}

std::shared_ptr<Expression> CreateModule::express(SymbolTableType scope) {
    setSpanFromPosition(span.start.line, span.start.col, span.start.filePath);
    DEBUG_LOG();
    DEBUG_LOG("Creating module '" + modulePath + "'.");

    std::vector<std::shared_ptr<Expression>> expressions;
    std::vector<std::shared_ptr<ModuleMemberExpression>> members;

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
            if (auto block = std::dynamic_pointer_cast<BlockExpression>(nestedExpr)) {
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
                std::string suggestion = Console::formatString(
                    "To resolve this:\n"
                    "1. Verify member '%s' in module '%s' is correctly defined\n"
                    "2. Check member expression validity\n"
                    "3. Add debug output for member evaluation",
                    member->getName().c_str(), modulePath.c_str()
                );
                console.reportError(
                    Console::RUNTIME_ERROR,
                    Console::formatString("Failed to evaluate module member '%s' in module '%s'",
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
            auto memberExpr = std::make_shared<ModuleMemberExpression>(
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
            std::string suggestion = Console::formatString(
                "To resolve this:\n"
                "1. Verify function '%s' in module '%s' is correctly defined\n"
                "2. Check function body and parameters\n"
                "3. Add debug output for function compilation",
                func->getName().c_str(), modulePath.c_str()
            );
            console.reportError(
                Console::RUNTIME_ERROR,
                Console::formatString("Failed to compile function '%s' in module '%s'",
                                 func->getName().c_str(), modulePath.c_str()),
                suggestion,
                func->getSpan()
            );
            return nullptr;
        }
        expressions.push_back(result);

        auto memberExpr = std::make_shared<ModuleMemberExpression>(
            expressions.back()->getName(),
            expressions.back(),
            funcsModifiers[i]
        );
        
        members.push_back(memberExpr);
    }

    auto module = std::make_shared<ModuleExpression>(name, members);
    scope->setConstant(name, module);
    scope->defineModule(modulePath, moduleScope);

    auto block = std::make_shared<BlockExpression>(expressions);
    block->setSpan(this->getSpan());
    return block;
}

std::shared_ptr<Statement> CreateModule::reinterprateStatement(std::shared_ptr<Statement> statement) {
    DEBUG_LOG("Reinterprating statement '" + statement->toString() + "'.");

    // Build the outer context prefix (all accessContext entries except the last).
    // `i + 1 < size()` avoids size_t underflow when accessContext is empty.
    std::string context;
    for (size_t i = 0; i + 1 < accessContext.size(); i++) {
        context += accessContext[i] + ".";
    }

    const std::string moduleName = getName();

    // If a member's name already equals the module name (e.g., `module String { struct String {...} }`),
    // don't double it. Otherwise `String` becomes `String.String`, which breaks method lookup on `this`.
    auto makeName = [&](const std::string& oldName) -> std::string {
        if (oldName == moduleName) return context + oldName;
        return context + moduleName + "." + oldName;
    };

    if (auto assignment = std::dynamic_pointer_cast<AssignVariable>(statement)) {
        // Extern variables keep their raw C linkage name.
        if (!assignment->isExtern) {
            assignment->setName(makeName(assignment->getName()));
        }
        return assignment;
    }

    if (auto function = std::dynamic_pointer_cast<FunctionDeclaration>(statement)) {
        // Extern functions keep their raw C linkage name (malloc, free, ...).
        if (!function->isExtern) {
            std::string oldName = function->getName();
            // A free function inside a module is just `name`; a method is `Struct.method`.
            // If it's a method whose struct name equals the module name, don't prefix with the module.
            auto dotPos = oldName.find('.');
            std::string structPrefix = (dotPos == std::string::npos) ? "" : oldName.substr(0, dotPos);
            if (!structPrefix.empty() && structPrefix == moduleName) {
                function->setName(context + oldName);
            } else {
                function->setName(context + moduleName + "." + oldName);
            }
        }
        return function;
    }

    if (auto structDeclr = std::dynamic_pointer_cast<ConstructStructPrototype>(statement)) {
        std::string oldName = structDeclr->getName();
        std::string newName = makeName(oldName);
        structDeclr->setName(newName);

        // Rename methods: `String.clear` → `<newName>.clear`.
        for (auto& member : structDeclr->getBody()) {
            if (auto method = std::dynamic_pointer_cast<FunctionDeclaration>(member)) {
                std::string mName = method->getName();
                if (mName.rfind(oldName + ".", 0) == 0) {
                    std::string suffix = mName.substr(oldName.length() + 1);
                    method->setName(newName + "." + suffix);
                }
            }
        }
        return structDeclr;
    }

    if (auto classDeclr = std::dynamic_pointer_cast<ConstructClassPrototype>(statement)) {
        std::string oldName = classDeclr->getName();
        std::string newName = makeName(oldName);
        classDeclr->setName(newName);

        // Class bodies store ClassMember wrappers; unwrap to reach FunctionDeclaration.
        for (auto& classMember : classDeclr->getBody()) {
            if (!classMember) continue;
            auto method = std::dynamic_pointer_cast<FunctionDeclaration>(classMember->getValue());
            if (!method) continue;

            std::string mName = method->getName();
            if (mName.rfind(oldName + ".", 0) == 0) {
                std::string suffix = mName.substr(oldName.length() + 1);
                method->setName(newName + "." + suffix);
            }
        }
        return classDeclr;
    }

    std::string suggestion = Console::formatString(
        "To resolve this:\n"
        "1. Ensure statement '%s' is a valid module member\n"
        "2. Check for supported statement types (assignment, function, struct, class)\n"
        "3. Verify statement syntax",
        statement->toString().c_str()
    );
    console.reportError(
        Console::RUNTIME_ERROR,
        Console::formatString("Invalid statement type for reinterpretation: '%s'",
                         statement->toString().c_str()),
        suggestion,
        statement->getSpan()
    );
    return nullptr;
}

namespace {

std::string lastComponent(const std::string& dotted) {
    const auto p = dotted.find_last_of('.');
    return p == std::string::npos ? dotted : dotted.substr(p + 1);
}

bool equalsIgnoreCase(const std::string& a, const std::string& b) {
    if (a.size() != b.size()) return false;
    for (size_t i = 0; i < a.size(); ++i) {
        if (std::tolower(static_cast<unsigned char>(a[i])) != std::tolower(static_cast<unsigned char>(b[i]))) return false;
    }
    return true;
}

// Releases the resolver's "currently loading" mark (cycle detection) on every exit path.
struct LoadGuard {
    ModuleResolver& resolver;
    ModuleId id;
    ~LoadGuard() { resolver.endLoad(id); }
};

// Flattens includes/blocks and keeps only named declarations. Returns false (and the offending
// statement's text in `bad`) when a top-level statement can't be a module member.
bool collectDeclarations(const std::vector<std::shared_ptr<Statement>>& stmts,
                         std::vector<std::shared_ptr<Statement>>& out, std::string& bad) {
    for (const auto& stmt : stmts) {
        if (!stmt) continue;
        if (auto inc = std::dynamic_pointer_cast<IncludeStatement>(stmt)) {
            if (!collectDeclarations(inc->getStatements(), out, bad)) return false;
        } else if (auto block = std::dynamic_pointer_cast<BlockStatement>(stmt)) {
            if (!collectDeclarations(block->statements, out, bad)) return false;
        } else if (std::dynamic_pointer_cast<NamedStatement>(stmt)) {
            out.push_back(stmt);
        } else {
            bad = stmt->toString();
            return false;
        }
    }
    return true;
}

// What a module looked like when it was first expressed into a scope (for import deduplication).
struct LoadedModuleInfo {
    std::string name;    // constant name of the ModuleExpression in the scope
    std::string prefix;  // "Mod." - members are registered in the scope as "Mod.member"
    bool flattened;      // loaded by `import * from` as unprefixed declarations (no namespace)
};

std::unordered_map<std::string, LoadedModuleInfo>& loadedModules() {
    static std::unordered_map<std::string, LoadedModuleInfo> loaded;
    return loaded;
}

// Re-registers the module member "<prefix><original>" in `scope` under `alias`
// (values, constants, overload sets and types).
bool bindMember(SymbolTableType scope, const std::string& prefix,
                const std::string& original, const std::string& alias) {
    const std::string qualified = prefix + original;
    bool bound = false;
    auto found = scope->lookup(qualified);
    if (found.found() && found.isPublic) {
        if (found.isOverloadSet()) {
            for (const auto& overload : found.overloads) scope->addOverloadable(alias, overload, true);
            bound = true;
        } else if (found.isConstantValue()) {
            bound = scope->setConstant(alias, found.value, true);
        } else if (found.isVariable()) {
            bound = scope->setVariable(alias, found.value, true);
        }
    }
    if (auto type = scope->getType(qualified)) {
        scope->addType(alias, type, true);
        bound = true;
    }
    return bound;
}

// Applies `import { a, b as c } from M;` and `import * from M;` after M is expressed.
bool bindImportedNames(SymbolTableType scope, const ImportModule& imp, const std::string& moduleName,
                       const std::string& prefix, const FileSpan& span) {
    auto moduleExpr = std::dynamic_pointer_cast<ModuleExpression>(scope->get(moduleName));
    auto memberNamed = [&](const std::string& original) -> std::shared_ptr<ModuleMemberExpression> {
        if (!moduleExpr) return nullptr;
        for (const auto& m : moduleExpr->members)
            if (m && (m->getName() == original || m->getName() == prefix + original)) return m;
        return nullptr;
    };
    auto stripPrefix = [&](const std::string& n) {
        return n.rfind(prefix, 0) == 0 ? n.substr(prefix.size()) : n;
    };

    if (!imp.importedAliases.empty()) {
        for (const auto& [aliasName, originalName] : imp.importedAliases) {
            auto member = memberNamed(originalName);
            if (member && member->isPrivate()) {
                console.reportError(
                    Console::RUNTIME_ERROR,
                    Console::formatString("Member '%s' of module '%s' is private.", originalName.c_str(), moduleName.c_str()),
                    "To resolve this:\n1. Make the member public in the module\n2. Import a different member",
                    span);
                return false;
            }
            if (!bindMember(scope, prefix, originalName, aliasName)) {
                std::string available;
                if (moduleExpr) {
                    for (const auto& m : moduleExpr->members)
                        if (m && !m->isPrivate()) available += "\n  " + stripPrefix(m->getName());
                }
                console.reportError(
                    Console::RUNTIME_ERROR,
                    Console::formatString("Module '%s' has no member '%s'.", moduleName.c_str(), originalName.c_str()),
                    "Public members:" + available + "\n\nTo resolve this:\n1. Check the member name and its casing",
                    span);
                return false;
            }
        }
        return true;
    }

    if (imp.importAll) {
        if (!moduleExpr) {
            console.reportError(
                Console::RUNTIME_ERROR,
                Console::formatString("Cannot enumerate the members of module '%s' for a wildcard import.", moduleName.c_str()),
                "To resolve this:\n1. Import the module by name instead of with '*'",
                span);
            return false;
        }
        for (const auto& m : moduleExpr->members) {
            if (!m || m->isPrivate()) continue;
            const std::string original = stripPrefix(m->getName());
            bindMember(scope, prefix, original, original); // members that can't be re-bound are skipped
        }
    }
    return true;
}

} // namespace

std::shared_ptr<Expression> ImportModule::express(SymbolTableType scope) {
    setSpanFromPosition(span.start.line, span.start.col, span.start.filePath);
    if (path.empty()) {
        std::string suggestion = "To resolve this:\n"
                               "1. Ensure the import directive specifies a valid module path\n"
                               "2. Check for correct module path syntax\n"
                               "3. Verify the module file exists";
        console.reportError(
            Console::RUNTIME_ERROR,
            "Module path is empty",
            suggestion,
            getSpan()
        );
        return nullptr;
    }

    // 1. Resolve the name to a concrete module (explicit path -> std -> own package -> dependencies).
    // A path that contains a directory separator or ends in `.os` is always a file path,
    // regardless of what `isPathImport` was set to (some construction sites — the parser's
    // `module X = import "..."` alias branch in particular — never set it).
    const bool pathLooksLikeFile =
        path.find('/') != std::string::npos ||
        path.find('\\') != std::string::npos ||
        (path.size() > 3 && path.compare(path.size() - 3, 3, ".os") == 0);

    ModuleRequest request;
    request.name = path;
    request.isPath = isPathImport || pathLooksLikeFile;
    request.importerFile = getSpan().start.filePath;

    ModuleResolver& resolver = ModuleResolver::global();
    ResolveResult found = resolver.resolve(request);
    if (!found.module) {
        std::string tried;
        for (const auto& line : found.tried) tried += "\n" + line;
        console.reportError(
            Console::RUNTIME_ERROR,
            Console::formatString("Module '%s' not found.", path.c_str()),
            "Searched:" + tried + "\n\nTo resolve this:\n"
            "1. Check the module name (e.g. 'std.console', 'std.vulkan')\n"
            "2. Use a quoted path for a file: import \"./helpers.os\";\n"
            "3. Check that the module file exists at one of the locations above",
            getSpan()
        );
        return nullptr;
    }
    const ResolvedModule& resolved = *found.module;

    if (resolved.sourceText.empty()) {
        console.reportError(
            Console::RUNTIME_ERROR,
            Console::formatString("Module file is empty: '%s'", resolved.displayPath.c_str()),
            "To resolve this:\n1. Ensure the module file has content\n2. Check file permissions",
            getSpan()
        );
        return nullptr;
    }

    // 2. Cycle detection by canonical module id.
    if (!resolver.beginLoad(resolved.id)) {
        console.reportError(
            Console::RUNTIME_ERROR,
            Console::formatString("Circular import of module '%s'.", resolved.id.toString().c_str()),
            "To resolve this:\n1. Move the shared declarations into a third module\n2. Remove one of the imports in the cycle",
            getSpan()
        );
        return nullptr;
    }
    LoadGuard guard{resolver, resolved.id};

    // 2b. Deduplicate: a module already expressed into this very scope is not expressed again;
    //     the import only (re)binds names. Registry: SymbolTable::defineModule(<registryKey>, scope).
    const std::string registryKey = resolved.id.registryKey();
    {
        auto cached = loadedModules().find(registryKey);
        bool sameScope = false;
        if (cached != loadedModules().end() && scope->moduleExists(registryKey)) {
            auto registered = scope->getModuleByPath(registryKey);
            sameScope = registered && &*registered == &*scope;
        }
        if (sameScope && (!cached->second.flattened || importAll)) {
            const LoadedModuleInfo info = cached->second;
            if (!info.flattened) {
                if (!alias.empty() && alias != info.name) {
                    if (auto existing = scope->get(info.name)) scope->setConstant(alias, existing);
                }
                if ((importAll || !importedAliases.empty()) &&
                    !bindImportedNames(scope, *this, info.name, info.prefix, getSpan())) {
                    return nullptr;
                }
            }
            std::vector<std::shared_ptr<Expression>> nothing;
            auto block = std::make_shared<BlockExpression>(nothing);
            block->setSpan(this->getSpan());
            return block;
        }
    }

    // 3. Parse.
    Lexer lexer(resolved.sourceText, resolved.displayPath);
    Parser parser(lexer);
    std::vector<std::shared_ptr<Statement>> statements = parser.parse();

    // 4. Find the module in the file. A file either wraps its declarations in `module X { }`
    //    or is itself the module (top-level declarations, no wrapper).
    const std::string wanted = lastComponent(moduleName);
    std::vector<std::shared_ptr<CreateModule>> declared;
    for (const auto& stmt : statements) {
        DEBUG_LOG("[ImportModule] Found statement '" + stmt->toString() + "' in file '" + resolved.displayPath + "'.");
        if (auto cm = std::dynamic_pointer_cast<CreateModule>(stmt)) declared.push_back(cm);
    }

    std::shared_ptr<CreateModule> createModule;
    for (const auto& cm : declared) {
        if (moduleName.empty() || equalsIgnoreCase(cm->getName(), wanted)) { createModule = cm; break; }
    }
    // A logical name maps to exactly one file, so a file with a single module is that module
    // even when the casing/name differs (std.console -> `module Console`).
    if (!createModule && declared.size() == 1) createModule = declared.front();

    if (!createModule && !declared.empty()) {
        console.reportError(
            Console::RUNTIME_ERROR,
            Console::formatString("Module '%s' not found in file '%s'", moduleName.c_str(), resolved.displayPath.c_str()),
            Console::formatString("To resolve this:\n1. Verify module '%s' is defined in file '%s'\n"
                                  "2. Check for correct module name", moduleName.c_str(), resolved.displayPath.c_str()),
            getSpan()
        );
        return nullptr;
    }

    if (!createModule) {
        // Unwrapped file: the file itself is the module.
        {
            const std::string sourceDir = std::filesystem::path(resolved.displayPath).parent_path().string();
            extern_support::ExternResolver externs(extern_support::ResolverConfig{}, sourceDir);
            externs.expand(statements); // `extern "x.h", "lib.dll" { }` blocks -> declarations
        }

        // NEW: Evaluate `import ...` statements in a module file BEFORE collecting
        // declarations. `import * from X` inside a file brings X's names into the
        // file's scope; the import itself is not a declaration and must be dropped
        // from the statement list so `collectDeclarations` doesn't reject it.
        //
        // This makes both of these work identically:
        //   Core.os with `import * from "c/console.os";` at top  → OK
        //   Core.os with `include "c/console.os";` at top        → OK (already worked)
        std::vector<std::shared_ptr<Statement>> remainingStatements;
        for (auto& stmt : statements) {
            if (!stmt) continue;
            if (auto imp = std::dynamic_pointer_cast<ImportModule>(stmt)) {
                // Express the import so its names land in `scope`, then drop it.
                imp->express(scope);
                continue;
            }
            remainingStatements.push_back(stmt);
        }
        statements = std::move(remainingStatements);

        std::vector<std::shared_ptr<Statement>> declarations;
        std::string bad;
        if (!collectDeclarations(statements, declarations, bad)) {
            console.reportError(
                Console::RUNTIME_ERROR,
                Console::formatString("Module file '%s' has a top-level statement that is not a declaration: %s",
                                      resolved.displayPath.c_str(), bad.c_str()),
                "To resolve this:\n1. Module files may only contain declarations (fn, const, struct, class, extern, include)\n"
                "2. Move executable statements into a function",
                getSpan()
            );
            return nullptr;
        }

        // `import * from std.vulkan;` -> declarations land in the importing scope, unprefixed.
        if (importAll) {
            std::vector<std::shared_ptr<Expression>> expressions;
            for (const auto& decl : declarations) {
                if (auto expr = decl->express(scope)) expressions.push_back(expr);
            }
            scope->defineModule(registryKey, scope);
            loadedModules()[registryKey] = LoadedModuleInfo{"", "", true};
            auto block = std::make_shared<BlockExpression>(expressions);
            block->setSpan(this->getSpan());
            return block;
        }

        std::vector<std::shared_ptr<Statement>> members;
        for (const auto& decl : declarations) {
            auto named = std::dynamic_pointer_cast<NamedStatement>(decl);
            members.push_back(std::make_shared<ModuleMember>(named->getName(), decl, MemberModifiers{}));
        }

        std::string implicitName = !alias.empty() ? alias
                                 : !moduleName.empty() ? wanted
                                 : std::filesystem::path(resolved.displayPath).stem().string();
        createModule = std::make_shared<CreateModule>(implicitName, members);
        createModule->setSpan(this->getSpan());
    }

    // `import x.y as z` / `module Z = import ...` names the module in the importing scope.
    // Must happen before extendContextOf(), which records the module's name.
    if (!alias.empty()) createModule->setName(alias);
    extendContextOf(createModule);
    createModule->setPath(resolved.displayPath);

    auto mod = createModule->express(scope);
    if (!mod) {
        std::string suggestion = Console::formatString(
            "To resolve this:\n"
            "1. Verify module '%s' contains valid expressions\n"
            "2. Check module body for errors\n"
            "3. Add debug output for module evaluation",
            createModule->getName().c_str()
        );
        console.reportError(
            Console::RUNTIME_ERROR,
            Console::formatString("Invalid module '%s': not a ModuleExpression", createModule->getName().c_str()),
            suggestion,
            createModule->getSpan()
        );
        return nullptr;
    }

    scope->defineModule(registryKey, scope);
    loadedModules()[registryKey] = LoadedModuleInfo{createModule->getName(), createModule->qualifiedPrefix(), false};
    if (importAll || !importedAliases.empty()) {
        if (!bindImportedNames(scope, *this, createModule->getName(), createModule->qualifiedPrefix(), getSpan())) {
            return nullptr;
        }
    }

    mod->setSpan(this->getSpan());
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

std::shared_ptr<Expression> ModuleMember::express(SymbolTableType scope) {
    setSpanFromPosition(span.start.line, span.start.col, span.start.filePath);
    if (auto assignment = std::dynamic_pointer_cast<Assignment>(value)) {
        assignment->setGlobalVisibilityTo(true);
    }
    auto val = value->express(scope);
    if (!val) {
        std::string suggestion = Console::formatString(
            "To resolve this:\n"
            "1. Verify member '%s' expression is valid\n"
            "2. Check member type and initialization\n"
            "3. Add debug output for member evaluation",
            getName().c_str()
        );
        console.reportError(
            Console::RUNTIME_ERROR,
            Console::formatString("Failed to evaluate module member '%s'",
                             getName().c_str()),
            suggestion,
            getSpan()
        );
        return nullptr;
    }
    val->setSpan(this->getSpan());
    return val;
}

} // namespace Omniscript