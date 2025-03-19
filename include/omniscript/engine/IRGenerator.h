#ifndef IR_GENERATOR_H
#define IR_GENERATOR_H

#include <llvm/IR/Module.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/Transforms/IPO.h>
#include <llvm/IR/LegacyPassManager.h>
#include <omniscript/omniscript_pch.h>

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


public:
    // Constructor initializes context, builder, and module
    IRGenerator();

    std::unique_ptr<llvm::Module> getModule() { return std::move(Module); }
    std::unique_ptr<llvm::LLVMContext> getContext() { return std::move(Context); }
    llvm::IRBuilder<>* getBuilder() { return Builder.get(); }
    
    void printIR();
    void optimizeModule(); // Define optimization logic

    llvm::Type* resolveLLVMType(const std::vector<std::string>& dataTypes);

    // Generate IR for different types
    // Number types
    llvm::Value* create8BitInteger(int value);
    llvm::Value* create16BitInteger(int value);
    llvm::Value* create32BitInteger(int value);
    llvm::Value* create64BitInteger(int value);

    llvm::Value* create32BitFloat(float value);
    llvm::Value* create64BitFloat(double value);

    llvm::Value* createBigInt(const std::string& str); // Arbitrary precision integer

    // Assignments
    llvm::Value* createVariable(const std::string& name, llvm::Type* type, llvm::Value* initialValue);
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