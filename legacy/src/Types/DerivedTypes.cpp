#include <omniscript/Types/BaseType.h>
#include <omniscript/Types/DerivedTypes.h>
#include <omniscript/Types/TraitCapability.h>
#include <omniscript/Console.h>

namespace Omniscript {

bool Type::isChar(int bitwidth) const {
    if (bitwidth == -1) {
        return kind == Kind::Char || kind == Kind::Char16 || kind == Kind::Char32;
    }
    switch (bitwidth) {
        case 8: return kind == Kind::Char;
        case 16: return kind == Kind::Char16;
        case 32: return kind == Kind::Char32;
        default: return false;
    }
}

bool Type::isString(int bitwidth) const {
    if (bitwidth == -1) {
        return kind == Kind::String || kind == Kind::Utf8 || kind == Kind::Utf16 || kind == Kind::Utf32;
    }
    switch (bitwidth) {
        case 8: return kind == Kind::Utf8;
        case 16: return kind == Kind::Utf16;
        case 32: return kind == Kind::Utf32;
        default: return kind == Kind::String;
    }
}

bool Type::isInteger(int bitwidth) const {
    if (bitwidth == -1) {
        return isIntegerKind(kind);
    }
    switch (bitwidth) {
        case 8: return kind == Kind::Int8 || kind == Kind::UInt8;
        case 16: return kind == Kind::Int16 || kind == Kind::UInt16;
        case 32: return kind == Kind::Int32 || kind == Kind::UInt32;
        case 64: return kind == Kind::Int64 || kind == Kind::UInt64;
        case 128: return kind == Kind::Int128 || kind == Kind::UInt128;
        case 256: return kind == Kind::Int256 || kind == Kind::UInt256;
        case 512: return kind == Kind::Int512 || kind == Kind::UInt512;
        case 1024: return kind == Kind::Int1024 || kind == Kind::UInt1024;
        default: return false;
    }
}

bool Type::isFloat(int bitWidth) const {
    if (bitWidth == -1) {
        return isFloatKind(kind);
    }
    switch (bitWidth) {
        case 16: return kind == Kind::Half;
        case 32: return kind == Kind::Float;
        case 64: return kind == Kind::Double;
        case 80: return kind == Kind::X86_FP80;
        case 128: return kind == Kind::FP128 || kind == Kind::PPC_FP128;
        default: return false;
    }
}

bool Type::isSigned() const {
    return kind == Kind::Int8 || kind == Kind::Int16 || kind == Kind::Int32 || kind == Kind::Int64 ||
           kind == Kind::Int128 || kind == Kind::Int256 || kind == Kind::Int512 || kind == Kind::Int1024 ||
           kind == Kind::BigInt;
}

int Type::getSize() const {
    switch (kind) {
        case Kind::Bool: return 1;
        case Kind::Char: case Kind::Int8: case Kind::UInt8: return 1;
        case Kind::Char16: case Kind::Int16: case Kind::UInt16: case Kind::Half: return 2;
        case Kind::Char32: case Kind::Int32: case Kind::UInt32: case Kind::Float: return 4;
        case Kind::Int64: case Kind::UInt64: case Kind::Double: return 8;
        case Kind::Int128: case Kind::UInt128: case Kind::FP128: case Kind::PPC_FP128: return 16;
        case Kind::X86_FP80: return 10; // 80-bit extended precision
        case Kind::Size_t: return sizeof(size_t);
        case Kind::Pointer: case Kind::Nullptr: case Kind::UniquePtr: case Kind::SharedPtr: case Kind::WeakPtr:
            return getPointerBitWidth() / 8;
        case Kind::Reference: return getPointerBitWidth() / 8;
        case Kind::FixedArray: {
            auto arr = std::dynamic_pointer_cast<FixedArrayType>(std::make_shared<Type>(*this));
            return arr && arr->elementType ? arr->elementType->getSize() * arr->size : 0;
        }
        case Kind::DynamicArray: return getPointerBitWidth() / 8; // Pointer to data
        case Kind::HeterogeneousArray: {
            auto arr = std::dynamic_pointer_cast<HeterogeneousArrayType>(std::make_shared<Type>(*this));
            if (!arr) return 0;
            size_t total = 0;
            for (const auto& elem : arr->elementTypes) {
                total += elem ? elem->getSize() : 0;
            }
            return total;
        }
        case Kind::String: case Kind::Utf8: case Kind::Utf16: case Kind::Utf32:
            return getPointerBitWidth() / 8; // Pointer to string data
        case Kind::UserDefined: {
            auto udt = std::dynamic_pointer_cast<UserDefinedType>(std::make_shared<Type>(*this));
            return udt ? 0 : 0; // Size depends on fields, assume 0 for now
        }
        default: return 0;
    }
}

bool Type::canPerform(const std::string& capability) const {
    return TraitSystem::instance().canPerform(std::make_shared<Type>(*this), capability);
}

bool Type::implementsTrait(const std::string& traitName) const {
    return TraitSystem::instance().implementsTrait(std::make_shared<Type>(*this), traitName);
}

bool Type::implementsFunctionTrait(const std::string& functionTraitName) const {
    return TraitSystem::instance().implementsFunctionTrait(std::make_shared<Type>(*this), functionTraitName);
}

void Type::addTrait(std::shared_ptr<Trait> trait) {
    if (trait) {
        implementedTraits.push_back(trait);
        DEBUG_LOG("Added trait " + trait->name + " to type " + toString());
    }
}

void Type::addFunctionTrait(std::shared_ptr<FunctionTrait> functionTrait) {
    if (functionTrait) {
        implementedFunctionTraits.push_back(functionTrait);
        DEBUG_LOG("Added function trait " + functionTrait->name + " to type " + toString());
    }
}

void Type::addCapability(const std::string& capability) {
    capabilities.insert(capability);
    DEBUG_LOG("Added capability " + capability + " to type " + toString());
}

std::string Type::kindName() const {
    return kindToString(kind);
}

void Type::logTypeInfo(const std::string& message, Console::LogLevel level) const {
    console.log(message + " [Type: " + toString() + "]", true, level);
}

void Type::reportTypeError(const std::string& message, Console::ErrorType errorType) const {
    console.reportError(errorType, message + " [Type: " + toString() + "]");
}

std::shared_ptr<Type> Type::createInvalid() {
    return std::make_shared<Type>();
}

std::shared_ptr<Type> Type::createUnresolved(const std::vector<std::string>& types) {
    return std::make_shared<UnresolvedType>(types);
}

std::shared_ptr<Type> Type::createUndefined() {
    return std::make_shared<Type>(Kind::Undefined);
}

std::shared_ptr<Type> Type::createPrimitiveType(Kind kind) {
    return std::make_shared<PrimitiveType>(kind);
}

std::shared_ptr<Type> Type::createNullType(std::shared_ptr<Type> innerType) {
    return std::make_shared<NullType>(innerType);
}

std::shared_ptr<Type> Type::createNullPointerType(std::shared_ptr<Type> innerType) {
    return std::make_shared<NullPointerType>(innerType ? innerType : createPrimitiveType(Kind::Void));
}

std::shared_ptr<Type> Type::createNullableType(std::shared_ptr<Type> innerType) {
    return std::make_shared<NullableType>(innerType);
}

std::shared_ptr<Type> Type::createMetaType() {
    return std::make_shared<Type>(Kind::Metadata);
}

std::shared_ptr<Type> Type::createPointerType(std::shared_ptr<Type> pointee, bool isConst, bool isVolatile) {
    return std::make_shared<PointerType>(pointee ? pointee : createPrimitiveType(Kind::Void), isConst, isVolatile);
}

std::shared_ptr<Type> Type::createReferenceType(std::shared_ptr<Type> referent) {
    return std::make_shared<ReferenceType>(referent);
}

std::shared_ptr<Type> Type::createFunctionType(
    const std::string& name,
    const std::vector<std::shared_ptr<Type>>& paramTypes,
    std::shared_ptr<Type> returnType,
    bool isVarArg) {
    return std::make_shared<FunctionType>(name, returnType, paramTypes, isVarArg);
}

std::shared_ptr<Type> Type::createStringType(Kind stringKind) {
    return std::make_shared<PrimitiveType>(stringKind);
}

std::shared_ptr<Type> Type::createFixedArrayType(std::shared_ptr<Type> elementType, size_t size) {
    return std::make_shared<FixedArrayType>(elementType, size);
}

std::shared_ptr<Type> Type::createDynamicArrayType(std::shared_ptr<Type> elementType) {
    return std::make_shared<DynamicArrayType>(elementType);
}

std::shared_ptr<Type> Type::createHeterogeneousArrayType() {
    return std::make_shared<HeterogeneousArrayType>();
}

std::shared_ptr<Type> Type::createGenericType(const std::string& typeName) {
    return std::make_shared<GenericType>(typeName);
}

std::shared_ptr<Type> Type::createUserDefinedType(
    const std::string& name,
    Kind kind,
    const std::vector<std::shared_ptr<Type>>& paramTypes,
    const std::vector<std::shared_ptr<Type>>& typeParams,
    const std::vector<std::shared_ptr<Type>>& baseTypes) {
    auto t = std::make_shared<UserDefinedType>(name, kind);
    t->paramTypes = paramTypes;
    t->typeParams = typeParams;
    t->baseTypes = baseTypes;
    return t;
}

std::shared_ptr<Type> Type::createSmartPointerType(Kind pointerKind, std::shared_ptr<Type> pointeeType) {
    return std::make_shared<SmartPointerType>(pointerKind, pointeeType);
}

bool Type::isSame(const std::shared_ptr<Type>& from, const std::shared_ptr<Type>& to) {
    if (!from || !to) return false;
    if (from == to) return true;
    if (from->kind != to->kind) return false;

    if (from->isInteger() && to->isInteger()) {
        return from->getSize() == to->getSize();
    }
    if (from->isFloat() && to->isFloat()) {
        return from->getSize() == to->getSize();
    }
    if (from->isChar() && to->isChar()) {
        return from->getBitWidth() == to->getBitWidth();
    }
    if (from->isString() && to->isString()) {
        return from->getBitWidth() == to->getBitWidth();
    }
    if (from->isPointer() && to->isPointer()) {
        return isSame(from->getPointeeType(), to->getPointeeType());
    }
    if (from->isReference() && to->isReference()) {
        return isSame(from->getReferencedType(), to->getReferencedType());
    }
    if (from->isNullable() && to->isNullable()) {
        auto fromNullable = std::dynamic_pointer_cast<NullableType>(from);
        auto toNullable = std::dynamic_pointer_cast<NullableType>(to);
        return fromNullable && toNullable && isSame(fromNullable->innerType, toNullable->innerType);
    }
    if (from->isFunction() && to->isFunction()) {
        auto fromFunc = std::dynamic_pointer_cast<FunctionType>(from);
        auto toFunc = std::dynamic_pointer_cast<FunctionType>(to);
        if (!fromFunc || !toFunc) return false;
        if (!isSame(fromFunc->returnType, toFunc->returnType)) return false;
        if (fromFunc->parameterTypes.size() != toFunc->parameterTypes.size()) return false;
        if (fromFunc->isVarArg != toFunc->isVarArg) return false;
        for (size_t i = 0; i < fromFunc->parameterTypes.size(); ++i) {
            if (!isSame(fromFunc->parameterTypes[i], toFunc->parameterTypes[i])) return false;
        }
        return true;
    }
    if (from->isUserDefined() && to->isUserDefined()) {
        auto fromUDT = std::dynamic_pointer_cast<UserDefinedType>(from);
        auto toUDT = std::dynamic_pointer_cast<UserDefinedType>(to);
        return fromUDT && toUDT && fromUDT->name == toUDT->name;
    }
    return false;
}

bool Type::isSameOrCastableTo(const std::shared_ptr<Type>& from, const std::shared_ptr<Type>& to) {
    if (!from || !to) {
        TYPE_ERROR("Null type in cast check");
        return false;
    }
    if (isSame(from, to)) return true;
    if (from->isVoidLike() && !to->isPointer()) return true;
    if (from->isNull() && (to->isPointer() || to->isReference())) return true;
    if (from->isNullable() && to->isNull()) return true;
    if (to->isNullable()) {
        auto nullableTo = std::dynamic_pointer_cast<NullableType>(to);
        if (from->isNullable()) {
            auto nullableFrom = std::dynamic_pointer_cast<NullableType>(from);
            return isSameOrCastableTo(nullableFrom->innerType, nullableTo->innerType);
        }
        return isSameOrCastableTo(from, nullableTo->innerType);
    }
    if (from->isInteger() && to->isInteger()) return from->getSize() <= to->getSize();
    if (from->isInteger() && to->isFloat()) return true;
    if (from->isFloat() && to->isFloat()) return from->getSize() <= to->getSize();
    if ((from->isChar() && to->isString()) || (from->isString() && to->isChar())) return true;
    if (from->isChar() && to->isChar()) return from->getBitWidth() <= to->getBitWidth();
    if (from->isString() && to->isString()) return from->getBitWidth() <= to->getBitWidth();
    if (from->isPointer() && to->isPointer()) return isSameOrCastableTo(from->getPointeeType(), to->getPointeeType());
    if (from->isFunction() && to->isFunction()) {
        auto fromFunc = std::dynamic_pointer_cast<FunctionType>(from);
        auto toFunc = std::dynamic_pointer_cast<FunctionType>(to);
        if (!fromFunc || !toFunc) return false;
        if (!isSameOrCastableTo(fromFunc->returnType, toFunc->returnType)) return false;
        if (fromFunc->parameterTypes.size() != toFunc->parameterTypes.size()) return false;
        if (fromFunc->isVarArg != toFunc->isVarArg) return false;
        for (size_t i = 0; i < fromFunc->parameterTypes.size(); ++i) {
            if (!isSameOrCastableTo(fromFunc->parameterTypes[i], toFunc->parameterTypes[i])) return false;
        }
        return true;
    }
    if (from->isPointer() && to->isFunction()) {
        auto fromPointee = from->getPointeeType();
        return fromPointee && fromPointee->isVoidLike();
    }
    if (from->isFunction() && to->isPointer()) {
        auto toPointee = to->getPointeeType();
        return toPointee && toPointee->isVoidLike();
    }
    if (from->isUserDefined() && to->isUserDefined()) {
        auto fromUDT = std::dynamic_pointer_cast<UserDefinedType>(from);
        auto toUDT = std::dynamic_pointer_cast<UserDefinedType>(to);
        return fromUDT && toUDT && (fromUDT->name == toUDT->name || fromUDT->derivesFrom(toUDT));
    }
    return false;
}

bool Type::isCompatibleWith(const std::shared_ptr<Type>& from, const std::shared_ptr<Type>& to) {
    return isSameOrCastableTo(from, to);
}

bool Type::hasCommonTrait(const std::shared_ptr<Type>& type1, const std::shared_ptr<Type>& type2, const std::string& traitName) {
    return type1 && type2 && type1->implementsTrait(traitName) && type2->implementsTrait(traitName);
}

} // namespace Omniscript