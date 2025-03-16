#include <omniscript/engine/IRGenerator.h>
#include <llvm/Support/raw_ostream.h>

IRGenerator::IRGenerator() : Builder(Context), Module(std::make_unique<llvm::Module>("OmniScript Module", Context)) {}

void IRGenerator::printIR() {
    Module->print(llvm::errs(), nullptr);
}

llvm::Module* IRGenerator::getModule() {
    return Module.get();
}

// Generate IR for different types

// ============================== Generate IR for Numeric Types ============================== //
// Create an 8-bit integer (i8)
llvm::Value* IRGenerator::create8BitInteger(int value) {
    return llvm::ConstantInt::get(llvm::Type::getInt8Ty(Context), value, true);
}

// Create a 16-bit integer (i16)
llvm::Value* IRGenerator::create16BitInteger(int value) {
    return llvm::ConstantInt::get(llvm::Type::getInt16Ty(Context), value, true);
}

// Create a 32-bit integer (i32)
llvm::Value* IRGenerator::create32BitInteger(int value) {
    return llvm::ConstantInt::get(llvm::Type::getInt32Ty(Context), value, true);
}

// Create a 64-bit integer (i64)
llvm::Value* IRGenerator::create64BitInteger(int value) {
    return llvm::ConstantInt::get(llvm::Type::getInt64Ty(Context), value, true);
}

// Create a 32-bit floating-point (float)
llvm::Value* IRGenerator::create32BitFloat(float value) {
    return llvm::ConstantFP::get(llvm::Type::getFloatTy(Context), value);
}

// Create a 64-bit floating-point (double)
llvm::Value* IRGenerator::create64BitFloat(double value) {
    return llvm::ConstantFP::get(llvm::Type::getDoubleTy(Context), value);
}

// Create an arbitrary-precision integer (BigInt)
llvm::Value* IRGenerator::createBigInt(const std::string& str) {
    llvm::APInt bigIntValue(1024, str, 10); // 1024-bit integer from string (base 10)
    return llvm::ConstantInt::get(Context, bigIntValue);
}
