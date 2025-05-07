#include <omniscript/Core.h>
#include <omniscript/utils.h>
#include <omniscript/engine/Statement.h>
#include <omniscript/engine/Symboltable.h>
#include <omniscript/omniscript_pch.h>
#include <omniscript/engine/lexer.h>
#include <omniscript/engine/Parser.h>
#include <omniscript/utils.h>
// #include "Statement.h"

// #include <omniscript/runtime/object.h>
// #include <omniscript/runtime/Class.h>
// #include <omniscript/runtime/Namespace.h>
// #include <omniscript/runtime/Enum.h>
// #include <omniscript/runtime/object.h>
// // #include <omniscript/runtime/Function.h>
// #include <omniscript/runtime/Number.h>
// #include <omniscript/runtime/String.h>
// #include <omniscript/runtime/Pointer.h>

void Initializer::initialize() {
    // std::vector<std::shared_ptr<Statement>> types = {};

    // for ()
    // auto type = std::make_shared<Omniscript::TypeExpression>();
}

std::shared_ptr<Omniscript::Expression> Initializer::express(SymbolTableType scope) {
    return nullptr;
}  


std::shared_ptr<Omniscript::Expression> BlockStatement::express(SymbolTableType scope) {
    return std::make_shared<Omniscript::BlockExpression>(expressAsVector(scope));
}

std::vector<std::shared_ptr<Omniscript::Expression>> BlockStatement::expressAsVector(SymbolTableType scope) {
    recursiveUpdate();
    
    std::vector<std::shared_ptr<Omniscript::Expression>> results = {};
    
    // // Generate code for each statement in order
    for (const auto& stmt : statements) {
        // Handle type propagation if needed
        DEBUG_LOG(stmt->toString());
        if (auto typed = std::dynamic_pointer_cast<TypedStatement>(stmt)) {
            if (type) {
                typed->setType(type);
            }
        }

        if (auto assignment = std::dynamic_pointer_cast<Assignment>(stmt)) {
            assignment->setGlobalVisibilityTo(false);
        }

        results.push_back(stmt->express(scope));
        
        // // If the current block already has a terminator, stop generating
        // if (generator.currentBlockHasTerminator()) {
        //     break;
        // }
    }
    
    // // Pop the scope we created for this block
    // generator.popScope();
    
    // // Return the last computed value (may be nullptr for statements without values)
    // return lastValue;
    return results;
}

bool BlockStatement::hasSideEffects() {
    return !isCompileTimeEvaluatable();
}


bool BlockStatement::isCompileTimeEvaluatable() {
    for (const auto& stmt : statements) {
        if (!stmt->isCompileTimeEvaluatable()) {
            return false;
        }
    }
    return true;
}

void BlockStatement::recursiveUpdate() {
    resolveGenerics();
    for (auto& stmt : statements) {
        if (auto assign = std::dynamic_pointer_cast<Assignment>(stmt)) {
            if (assign->isStatic) {
                assign->isGlobal = true;
            } else {
                assign->isGlobal = false;
            }
        }
        if (auto assign = std::dynamic_pointer_cast<BlockStatement>(stmt)) {
            recursiveUpdate();
        }
    }
}

std::shared_ptr<Omniscript::Expression> ImportModule::express(SymbolTableType scope) {
    // if (path.empty()) {
    //     console.error("ImportModule::codegen - Module path is empty.");
    // }

    // if (path == "std") {
    //     path = "standard/1/std.os";
    // }

    // std::string sourceCode = readFile(path);
    // if (sourceCode.empty()) {
    //     console.error("ImportModule::codegen - Failed to read module: " + path);
    // }

    // Lexer lexer(sourceCode, path);
    // Parser parser(lexer);

    // // parser.setScopeName(alias.empty() ? moduleName : alias);

    // std::vector<std::shared_ptr<Statement>> statements = parser.Parse();

    // console.log("Importing " + (importAll ? "everything" : joinMapKeys(importedAliases)) + " from " + path + ".");

    // // Ensure module is only loaded once
    // if (!generator.isLoadedModule(path)) {
    //     generator.generateModule(path, alias, statements, importedAliases, importAll);
    // }

    return nullptr; // No direct IR generation
}


std::shared_ptr<Omniscript::Expression> CreateModule::express(SymbolTableType scope) {
    // generator.importModule(name);8
    return nullptr; // Modules themselves don't return a value
}

std::shared_ptr<Omniscript::Expression> PublicMember::express(SymbolTableType scope) {
    if (auto assignment = std::dynamic_pointer_cast<Assignment>(value)) {
        assignment->setGlobalVisibilityTo(true);
    }
    return value->express(scope);
}

std::shared_ptr<Omniscript::Expression> PrivateMember::express(SymbolTableType scope) {
    if (auto assignment = std::dynamic_pointer_cast<Assignment>(value)) {
        assignment->setGlobalVisibilityTo(true);
    }
    return value->express(scope);
}
