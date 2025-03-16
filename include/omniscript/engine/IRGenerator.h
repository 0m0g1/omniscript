#ifndef IR_GENERATOR_H
#define IR_GENERATOR_H

#include <llvm/IR/Module.h>
#include <llvm/IR/IRBuilder.h> 
#include <llvm/IR/LLVMContext.h>
#include <llvm/ADT/APInt.h> // For BigInt

class IRGenerator {
private:
    llvm::LLVMContext Context;
    llvm::IRBuilder<> Builder;
    std::unique_ptr<llvm::Module> Module;

public:
    IRGenerator();
    void printIR();
    llvm::Module* getModule();

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
