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
        console.error("ImportModule::codegen - Failed to read module: " + path);
    }

    Lexer lexer(sourceCode, path);
    Parser parser(lexer);

    // parser.setScopeName(alias.empty() ? moduleName : alias);

    std::vector<std::shared_ptr<Statement>> statements = parser.Parse();

    DEBUG_LOG("Importing " + (importAll ? "everything" : joinMapKeys(importedAliases)) + " from " + path + ".");

    // Ensure module is only loaded once
    // if (!generator.isLoadedModule(path)) {
    //     generator.generateModule(path, alias, statements, importedAliases, importAll);
    // }

    return nullptr; // No direct IR generation
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
