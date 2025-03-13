#include <omniscript/engine/IRGenerator.h>
#include <llvm/Support/raw_ostream.h>

IRGenerator::IRGenerator() 
    : Builder(Context), Module(std::make_unique<llvm::Module>("OmniScript Module", Context)) {}

void IRGenerator::generateIR(const std::vector<std::shared_ptr<Statement>>& statements) {
    for (const auto& stmt : statements) {
        if (!stmt) continue;

        if (auto valueStmt = std::dynamic_pointer_cast<Value>(stmt)) {
            generate(*valueStmt);
        } else if (auto varStmt = std::dynamic_pointer_cast<Variable>(stmt)) {
            generate(*varStmt);
        } else if (auto constAssign = std::dynamic_pointer_cast<ConstantAssignment>(stmt)) {
            generate(*constAssign);
        } else if (auto assign = std::dynamic_pointer_cast<Assignment>(stmt)) {
            generate(*assign);
        } else if (auto constructor = std::dynamic_pointer_cast<ObjectConstructorStatement>(stmt)) {
            generate(*constructor);
        } else if (auto destructor = std::dynamic_pointer_cast<ObjectDestructorStatement>(stmt)) {
            generate(*destructor);
        } else if (auto functionCall = std::dynamic_pointer_cast<FunctionCallStatement>(stmt)) {
            generate(*functionCall);
        } else if (auto returnStmt = std::dynamic_pointer_cast<ReturnStatement>(stmt)) {
            generate(*returnStmt);
        } else if (auto ifStmt = std::dynamic_pointer_cast<IfStatement>(stmt)) {
            generate(*ifStmt);
        } else if (auto whileStmt = std::dynamic_pointer_cast<WhileStatement>(stmt)) {
            generate(*whileStmt);
        } else if (auto forLoopStmt = std::dynamic_pointer_cast<ForLoop>(stmt)) {
            generate(*forLoopStmt);
        } else if (auto unaryExpr = std::dynamic_pointer_cast<UnaryExpression>(stmt)) {
            generate(*unaryExpr);
        } else if (auto binaryExpr = std::dynamic_pointer_cast<BinaryExpression>(stmt)) {
            generate(*binaryExpr);
        } else if (auto ternaryExpr = std::dynamic_pointer_cast<TenaryExpression>(stmt)) {
            generate(*ternaryExpr);
        } else if (auto methodCall = std::dynamic_pointer_cast<CallMethod>(stmt)) {
            generate(*methodCall);
        } else if (auto propertyCall = std::dynamic_pointer_cast<GetProperty>(stmt)) {
            generate(*propertyCall);
        } else {
            std::cerr << "Error: Unknown statement type in IR generation." << std::endl;
        }
    }
}

// === IR Generation for Each Statement Type (Stub Implementations) ===

llvm::Value* IRGenerator::generate(Value& valueStmt) {
    return nullptr;  // TODO: Implement IR for values
}

llvm::Value* IRGenerator::generate(Variable& varStmt) {
    return nullptr;  // TODO: Implement IR for variables
}

llvm::Value* IRGenerator::generate(ConstantAssignment& constAssign) {
    return nullptr;  // TODO: Implement IR for constant assignment
}

llvm::Value* IRGenerator::generate(Assignment& assign) {
    return nullptr;  // TODO: Implement IR for assignments
}

llvm::Value* IRGenerator::generate(ObjectConstructorStatement& constructor) {
    return nullptr;  // TODO: Implement IR for object constructors
}

llvm::Value* IRGenerator::generate(ObjectDestructorStatement& destructor) {
    return nullptr;  // TODO: Implement IR for object destructors
}

llvm::Value* IRGenerator::generate(FunctionCallStatement& functionCall) {
    return nullptr;  // TODO: Implement IR for function calls
}

llvm::Value* IRGenerator::generate(ReturnStatement& returnStmt) {
    return nullptr;  // TODO: Implement IR for return statements
}

llvm::Value* IRGenerator::generate(IfStatement& ifStmt) {
    return nullptr;  // TODO: Implement IR for if statements
}

llvm::Value* IRGenerator::generate(WhileStatement& whileStmt) {
    return nullptr;  // TODO: Implement IR for while loops
}

llvm::Value* IRGenerator::generate(ForLoop& forLoopStmt) {
    return nullptr;  // TODO: Implement IR for for loops
}

llvm::Value* IRGenerator::generate(UnaryExpression& unaryExpr) {
    return nullptr;  // TODO: Implement IR for unary expressions
}

llvm::Value* IRGenerator::generate(BinaryExpression& binaryExpr) {
    return nullptr;  // TODO: Implement IR for binary expressions
}

llvm::Value* IRGenerator::generate(TenaryExpression& ternaryExpr) {
    return nullptr;  // TODO: Implement IR for ternary expressions
}

llvm::Value* IRGenerator::generate(CallMethod& methodCall) {
    return nullptr;  // TODO: Implement IR for method calls
}

llvm::Value* IRGenerator::generate(GetProperty& propertyCall) {
    return nullptr;  // TODO: Implement IR for property access
}

void IRGenerator::printIR() {
    Module->print(llvm::errs(), nullptr);
}

// llvm::Module* IRGenerator::getModule() {
//     return *Module;
// }
