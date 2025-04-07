#include <omniscript/Core.h>
#include <omniscript/Core/Value.h>
#include <omniscript/omniscript_pch.h>

namespace Omniscript {

std::string Type::kindName() const {
    switch (kind) {
        case Kind::Invalid:     return "Invalid";

        // Primitive Types
        case Kind::Primitive:   return "Primitive";
        case Kind::Void:        return "Void";
        case Kind::Nullptr:     return "Nullptr";
        case Kind::Null:        return "Null";
        case Kind::Bool:        return "Bool";
        case Kind::Char:        return "Char";
        case Kind::Int8:        return "Int8";
        case Kind::Int16:       return "Int16";
        case Kind::Int32:       return "Int32";
        case Kind::Int64:       return "Int64";
        case Kind::Int128:      return "Int128";
        case Kind::Int256:      return "Int256";
        case Kind::Int512:      return "Int512";
        case Kind::Int1024:     return "Int1024";
        case Kind::BigInt:      return "BigInt";

        case Kind::UInt8:       return "UInt8";
        case Kind::UInt16:      return "UInt16";
        case Kind::UInt32:      return "UInt32";
        case Kind::UInt64:      return "UInt64";
        case Kind::UInt128:     return "UInt128";
        case Kind::UInt256:     return "UInt256";
        case Kind::UInt512:     return "UInt512";
        case Kind::UInt1024:    return "UInt1024";

        case Kind::Half:        return "Half";
        case Kind::Float:       return "Float";
        case Kind::Double:      return "Double";
        case Kind::FP128:       return "FP128";
        case Kind::X86_FP80:    return "X86_FP80";
        case Kind::PPC_FP128:   return "PPC_FP128";

        // Special Types
        case Kind::Label:       return "Label";
        case Kind::Token:       return "Token";
        case Kind::Metadata:    return "Metadata";

        // Aggregate Types
        case Kind::Struct:      return "Struct";
        case Kind::Enum:        return "Enum";
        case Kind::Array:       return "Array";
        case Kind::Vector:      return "Vector";

        // Pointer/Reference Types
        case Kind::Pointer:     return "Pointer";
        case Kind::Reference:   return "Reference";

        // Function Types
        case Kind::Function:    return "Function";

        // Custom Types
        case Kind::String:      return "String";
        case Kind::WideString:  return "WideString";

        default:                return "Unknown";
    }
}



std::shared_ptr<Type> Type::createInvalid() {
    return std::make_shared<Type>();
}

std::shared_ptr<Type> Type::createPrimitiveType(Kind kind) {
    return std::make_shared<PrimitiveType>(kind);
}

std::shared_ptr<Type> Type::createPointerType(std::shared_ptr<Type> pointee) {
    return std::make_shared<PointerType>(std::move(pointee));
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