#include <omniscript/Core.h>
#include <omniscript/Core/Value.h>
#include <omniscript/omniscript_pch.h>

namespace Omniscript {

std::shared_ptr<Type> Type::createInvalid() {
    return std::make_shared<Type>();
}

std::shared_ptr<Type> Type::createPrimitiveType(Kind kind) {
    return std::make_shared<PrimitiveType>(kind);
}

std::shared_ptr<Type> Type::createPointerType(std::shared_ptr<Type> pointee, bool isConst, bool isVolatile) {
    return std::make_shared<PointerType>(std::move(pointee), isConst, isVolatile);
}

std::shared_ptr<Type> Type::createReferenceType(std::shared_ptr<Type> referent) {
    return std::make_shared<ReferenceType>(std::move(referent));
}

std::shared_ptr<Type> Type::createFunctionType(Kind returnKind, std::vector<std::shared_ptr<Type>> params, bool isVarArg) {
    return std::make_shared<FunctionType>(returnKind, std::move(params), isVarArg);
}

std::shared_ptr<Type> Type::createStringType() {
    auto t = std::make_shared<Type>();
    t->kind = Kind::String;
    return t;
}

std::shared_ptr<Type> Type::createWideStringType() {
    auto t = std::make_shared<Type>();
    t->kind = Kind::WideString;
    return t;
}

} // namespace Omniscript