#ifndef IR_GENERATOR_H
#define IR_GENERATOR_H

#include <llvm/IR/Module.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/Transforms/IPO.h>
#include <llvm/IR/LegacyPassManager.h>
#include <omniscript/omniscript_pch.h>
#include <omniscript/utils.h>

class Statement;

struct DynamicValue {
    enum Type { INT, FLOAT, STRING };
    Type type;
    union {
        int intValue;
        double floatValue;
    };
    std::string strValue;

    DynamicValue(int val) : type(INT), intValue(val) {}
    DynamicValue(int64_t val) : type(INT), intValue(static_cast<int>(val)) {}  // New constructor
    DynamicValue(double val) : type(FLOAT), floatValue(val) {}
    DynamicValue(std::string val) : type(STRING), strValue(std::move(val)) {}
};


class IRGenerator {
private:
    std::unique_ptr<llvm::LLVMContext> Context;
    std::unique_ptr<llvm::IRBuilder<>> Builder;
    std::unique_ptr<llvm::Module> Module;
    std::map<std::string, llvm::Value*> NamedValues;
    std::unordered_map<std::string, DynamicValue*> runtimeVariables;
    std::unordered_map<std::string, std::unique_ptr<llvm::Module>> loadedModules;
    std::unordered_map<std::string, std::unique_ptr<llvm::Module>> generatedModules;

    llvm::Module* CurrentModule = nullptr;
    std::unordered_map<std::string, std::unordered_map<std::string, llvm::Value*>> modulePublicSymbols;
    std::vector<std::pair<llvm::GlobalVariable*, llvm::Value*>> globalInitList;

public:
    // Constructor initializes context, builder, and module
    IRGenerator(const std::string& mainModulePath);

    std::unique_ptr<llvm::Module> getModule() { return std::move(Module); }
    std::unique_ptr<llvm::LLVMContext> getContext() { return std::move(Context); }
    llvm::IRBuilder<>* getBuilder() { return Builder.get(); }
    
    void initialize();

    void printIR();
    void printErrors();
    void printErrors(llvm::Module& module);
    void optimizeModule(int level = 2); // Define optimization logic

    bool isLoadedModule(const std::string& modulePath);
    bool isLoadedModuleMember(const std::string& modulePath, const std::string& memberName);
    void activateModuleMembers(const std::vector<std::string>& members);
    void linkModules();

    void generateModule(const std::string& moduleName, const std::vector<std::shared_ptr<Statement>>& statements);
    void importModule(const std::string& moduleName, const std::vector<std::string>& members);
    // bool isModuleUpdated(const std::string& moduleName);
    // void unloadModule(const std::string& moduleName);

    llvm::Type* resolveLLVMType(const std::vector<std::string>& dataTypes);

    // Generate IR for different types
    // Number types
    llvm::Value* create8BitInteger(int value);
    llvm::Value* create16BitInteger(int value);
    llvm::Value* create32BitInteger(int value);
    llvm::Value* create64BitInteger(int value);

    llvm::Value* create32BitFloat(float value);
    llvm::Value* create64BitFloat(double value);

    llvm::Value* create128BitBigInt(const std::string& str); // Arbitrary precision integer
    llvm::Value* create256BitBigInt(const std::string& str);
    llvm::Value* create1024BitBigInt(const std::string& str);

    // Assignments
    llvm::GlobalVariable* createGlobalVariable(
        const std::string& name, 
        llvm::Type* type, 
        llvm::Value* initialValue, 
        llvm::GlobalValue::LinkageTypes linkage = llvm::GlobalValue::InternalLinkage // Default to internal
    );
    llvm::Value* createVariable(const std::string& name, llvm::Type* type, llvm::Value* initialValue, bool isGlobal = false);
    llvm::Value* createConstant(const std::string& name, llvm::Type* type, llvm::Value* value);
    llvm::Value* reassign(const std::string& name, llvm::Value* newValue);
    llvm::Value* getVariable(const std::string& name);
    
    llvm::Value* createDynamicVariable(const std::string& name, llvm::Value* value);
    llvm::Value* createDynamicConstant(const std::string& name, llvm::Value* value);
    llvm::Value* assignDynamicVariable(const std::string& name, llvm::Value* newValue);
    llvm::Value* getDynamicVariable(const std::string& name);
    llvm::Value* generateOpaqueDynamicVariable(const std::string& name, llvm::Value* value);
};

#endif