#include <omniscript/Backends/LLVM/IRGenerator.h>

namespace Omniscript {
// Nullable<T> is lowered as an unnamed literal struct { i1 hasValue, T value } (see resolveLLVMType).
// Named structs (user types such as Stats) are never nullables, even if their first field is a bool.
bool IRGenerator::isNullableStruct(llvm::Type* type) {
    if (!type) return false;
    if (auto* structType = llvm::dyn_cast<llvm::StructType>(type)) {
        return !structType->hasName() &&
               structType->getNumElements() == 2 &&
               structType->getElementType(0)->isIntegerTy(1);
    }
    return false;
}

} // namespace Omniscript