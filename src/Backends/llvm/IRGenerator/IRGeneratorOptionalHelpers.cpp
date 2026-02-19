#include <omniscript/Backends/LLVM/IRGenerator.h>

namespace Omniscript {
bool IRGenerator::isNullableStruct(llvm::Type* type) {
    // if (auto* structType = llvm::dyn_cast<llvm::StructType>(type)) {
    //     return structType->getNumElements() == 2 &&
    //            structType->getElementType(0)->isIntegerTy(1);  // i1
    // }
    return false;
}

} // namespace Omniscript
