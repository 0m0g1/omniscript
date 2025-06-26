#include <omniscript/Backends/LLVM/IRGenerator.h>

void IRGenerator::generateModule(
    const std::string& modulePath,
    const std::string& alias,
    const std::vector<std::shared_ptr<Statement>>& statements,
    const std::unordered_map<std::string, std::string>& importedAliases, // Alias -> Original
    bool importAll
) {
    
    DEBUG_LOG("Generating module: " + modulePath);
    
    // if (scope->moduleExists(modulePath)) {
    //     DEBUG_LOG("Module " + modulePath + " is already generated. Skipping.");
    //     return;
    // }

    // 🔹 Store public members in a new module symbol table
    // pushScope();

    // auto newModule = std::make_unique<llvm::Module>(modulePath, *Context);
    // llvm::Module* previousModule = currentModule;
    // currentModule = newModule.get();

    // std::unordered_map<std::string, llvm::Value*> publicMembers;

    // for (const auto& statement : statements) {
    //     if (auto moduleStatement = std::dynamic_pointer_cast<CreateModule>(statement)) {
    //         DEBUG_LOG("Generating submodule: " + moduleStatement->getName());
    //         generateModule(moduleStatement->getName(), moduleStatement->getName(), moduleStatement->getStatements(), importedAliases, false);

    //     } else if (auto publicStatement = std::dynamic_pointer_cast<PublicMember>(statement)) {
    //         if (auto moduleStatement = std::dynamic_pointer_cast<CreateModule>(publicStatement->getValue())) {
    //             DEBUG_LOG("Generating submodule: " + moduleStatement->getName());
    //             generateModule(moduleStatement->getName(), moduleStatement->getName(), moduleStatement->getStatements(), importedAliases, false);
    //             continue;
    //         }
    //         std::string originalName = publicStatement->getName();
    //         llvm::Value* value = publicStatement->codegen(*this);
    //         if (!value) {
    //             console.warn("Skipping public statement '" + originalName + "' - no valid IR generated.");
    //             continue;
    //         }

    //         if (llvm::GlobalValue* gv = llvm::dyn_cast<llvm::GlobalValue>(value)) {
    //             if (importAll) {
    //                 gv->setLinkage(llvm::GlobalValue::ExternalLinkage);
    //                 // activeScope->set(originalName, gv);
    //             } else {
    //                 auto it = importedAliases.find(originalName);
    //                 if (it != importedAliases.end()) {
    //                     std::string alias = it->second; // Aliased name
    //                     gv->setLinkage(llvm::GlobalValue::ExternalLinkage);
    //                     DEBUG_LOG("Aliased import: " + alias + " -> " + originalName);
    //                 } else {
    //                     gv->setLinkage(llvm::GlobalValue::InternalLinkage);
    //                 }
    //             }
    //         } else {
    //             console.warn("Warning: Public symbol '" + originalName +
    //                          "' is not a global value and cannot have linkage visibility set.");
    //         }
    //     } else if (auto privateStatement = std::dynamic_pointer_cast<PrivateMember>(statement)) {
    //         DEBUG_LOG("Creating private member " + privateStatement->getName());
    //         llvm::Value* value = privateStatement->codegen(*this);
    //         if (importedAliases.find(privateStatement->getName()) != importedAliases.end()) {
    //             console.warn("'" + privateStatement->getName() + "' is not a public member of '" + modulePath + "' and cannot be imported");
    //         }
    //         if (value) {
    //             if (llvm::GlobalValue* gv = llvm::dyn_cast<llvm::GlobalValue>(value)) {
    //                 gv->setLinkage(llvm::GlobalValue::InternalLinkage);
    //             } else {
    //                 console.warn("Warning: Private statement '" + privateStatement->getName() + "' is not a global value and cannot have linkage visibility set to private or public.");
    //             }
    //         }
    //     }
    // }

    // printErrors(*currentModule);
    // currentModule = previousModule;

    // popScope();
    
    // if (llvm::Linker::linkModules(*Module, std::move(newModule))) {
    //     llvm::errs() << "Error: Linking failed for module " << modulePath << "!\n";
    // }
}

bool IRGenerator::isLoadedModule(const std::string& modulePath) {
    if (generatedModules.find(modulePath) != generatedModules.end()) {
        return true;
    }
    return false;
}

bool IRGenerator::isLoadedModuleMember(const std::string& modulePath, const std::string& memberName) {
    return true;
}

void IRGenerator::activateModuleMembers(const std::vector<std::string>& members) {
    return;
}

void IRGenerator::linkModules() {

}

void IRGenerator::importModule(const std::string& modulePath, const std::vector<std::string>& members) {
    if (modulePublicSymbols.find(modulePath) == modulePublicSymbols.end()) {
        console.error("Module '" + modulePath + "' not found or has no public symbols.");
        return;
    }

    // Import only public members
    for (const auto& [name, value] : modulePublicSymbols[modulePath]) {
        activeScope->set(name, value);
    }
}