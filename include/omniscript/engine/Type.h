#include <llvm/IR/Type.h>

struct Type {
    Type(llvm::Type& newType) : baseType(newType) {}
    llvm::Type* baseType;
    llvm::Type* ponteeType;
}