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

// `opt.value`: the payload of a Nullable. No check: the caller tests `!= null` first (C-style freedom).
llvm::Value* IRGenerator::createNullableValue(llvm::Value* opt) {
    if (!opt || !isNullableStruct(opt->getType())) {
        console.reportError(Console::TYPE_ERROR, ".value used on a non-optional value", FileSpan{});
        return nullptr;
    }
    return Builder->CreateExtractValue(opt, {1}, "nullable_value");
}

// `opt.valueOr(d)`: the payload when hasValue, else `d` (evaluated eagerly).
llvm::Value* IRGenerator::createNullableValueOr(llvm::Value* opt, llvm::Value* fallback) {
    if (!opt || !fallback || !isNullableStruct(opt->getType())) {
        console.reportError(Console::TYPE_ERROR, ".valueOr used on a non-optional value", FileSpan{});
        return nullptr;
    }
    llvm::Value* has = Builder->CreateExtractValue(opt, {0}, "nullable_has");
    llvm::Value* val = Builder->CreateExtractValue(opt, {1}, "nullable_value");
    llvm::Value* alt = generateCast(fallback, val->getType());
    if (!alt) return nullptr;
    return Builder->CreateSelect(has, val, alt, "nullable_or");
}

} // namespace Omniscript