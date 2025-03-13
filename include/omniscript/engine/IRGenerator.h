#ifndef IR_GENERATOR_H
#define IR_GENERATOR_H

#include <llvm/IR/Module.h>
#include <llvm/IR/IRBuilder.h> 
#include <llvm/IR/LLVMContext.h>
#include <omniscript/runtime/Statement.h>

class IRGenerator {
private:
    llvm::LLVMContext Context;
    llvm::IRBuilder<> Builder;
    std::unique_ptr<llvm::Module> Module;

public:
    IRGenerator();
    void generateIR(const std::vector<std::shared_ptr<Statement>>& statements);
    void printIR();
    llvm::Module* getModule();

    // Generate IR for different statement types
    llvm::Value* generate(Value& valueStmt);
    llvm::Value* generate(Variable& varStmt);
    llvm::Value* generate(ConstantAssignment& constAssign);
    llvm::Value* generate(Assignment& assign);
    llvm::Value* generate(ObjectConstructorStatement& constructor);
    llvm::Value* generate(ObjectDestructorStatement& destructor);
    llvm::Value* generate(FunctionCallStatement& functionCall);
    llvm::Value* generate(ReturnStatement& returnStmt);
    llvm::Value* generate(IfStatement& ifStmt);
    llvm::Value* generate(WhileStatement& whileStmt);
    llvm::Value* generate(ForLoop& forLoopStmt);
    llvm::Value* generate(UnaryExpression& unaryExpr);
    llvm::Value* generate(BinaryExpression& binaryExpr);
    llvm::Value* generate(TenaryExpression& ternaryExpr);
    llvm::Value* generate(CallMethod& methodCall);
    llvm::Value* generate(GetProperty& propertyCall);
};

#endif
