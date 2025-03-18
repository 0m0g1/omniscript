#ifndef IR_GENERATOR_H
#define IR_GENERATOR_H

#include <llvm/IR/Module.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/Transforms/IPO.h>
#include <llvm/IR/LegacyPassManager.h>

class IRGenerator {
private:
    std::unique_ptr<llvm::LLVMContext> Context;
    std::unique_ptr<llvm::IRBuilder<>> Builder;
    std::unique_ptr<llvm::Module> Module;

public:
    // Constructor initializes context, builder, and module
    IRGenerator();

    std::unique_ptr<llvm::Module> getModule() { return std::move(Module); }
    std::unique_ptr<llvm::LLVMContext> getContext() { return std::move(Context); }
    llvm::IRBuilder<>* getBuilder() { return Builder.get(); }
    
    void printIR();
    void optimizeModule(); // Define optimization logic

    // Generate IR for different types
    llvm::Value* create8BitInteger(int value);
    llvm::Value* create16BitInteger(int value);
    llvm::Value* create32BitInteger(int value);
    llvm::Value* create64BitInteger(int value);

    llvm::Value* create32BitFloat(float value);
    llvm::Value* create64BitFloat(double value);

    llvm::Value* createBigInt(const std::string& str); // Arbitrary precision integer
};

#endif