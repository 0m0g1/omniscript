#include <omniscript/Core.h>
#include <omniscript/Types/TypeKind.h>

namespace Omniscript {

const char* kindToString(Kind kind) {
    switch (kind) {
        case Kind::Invalid: return "Invalid";
        case Kind::Undefined: return "Undefined";
        case Kind::Unresolved: return "Unresolved";
        case Kind::Primitive: return "Primitive";
        case Kind::Void: return "Void";
        case Kind::Nullptr: return "Nullptr";
        case Kind::Null: return "Null";
        case Kind::Nullable: return "Nullable";
        case Kind::Bool: return "Bool";
        case Kind::Char: return "Char";
        case Kind::Char16: return "Char16";
        case Kind::Char32: return "Char32";
        case Kind::Size_t: return "Size_t";
        case Kind::Int8: return "Int8";
        case Kind::Int16: return "Int16";
        case Kind::Int32: return "Int32";
        case Kind::Int64: return "Int64";
        case Kind::Int128: return "Int128";
        case Kind::Int256: return "Int256";
        case Kind::Int512: return "Int512";
        case Kind::Int1024: return "Int1024";
        case Kind::BigInt: return "BigInt";
        case Kind::UInt8: return "UInt8";
        case Kind::UInt16: return "UInt16";
        case Kind::UInt32: return "UInt32";
        case Kind::UInt64: return "UInt64";
        case Kind::UInt128: return "UInt128";
        case Kind::UInt256: return "UInt256";
        case Kind::UInt512: return "UInt512";
        case Kind::UInt1024: return "UInt1024";
        case Kind::Half: return "Half";
        case Kind::Float: return "Float";
        case Kind::Double: return "Double";
        case Kind::FP128: return "FP128";
        case Kind::X86_FP80: return "X86_FP80";
        case Kind::PPC_FP128: return "PPC_FP128";
        case Kind::Label: return "Label";
        case Kind::Token: return "Token";
        case Kind::Metadata: return "Metadata";
        case Kind::Module: return "Module";
        case Kind::Class: return "Class";
        case Kind::Struct: return "Struct";
        case Kind::Enum: return "Enum";
        case Kind::Array: return "Array";
        case Kind::Vector: return "Vector";
        case Kind::Pointer: return "Pointer";
        case Kind::Reference: return "Reference";
        case Kind::Function: return "Function";
        case Kind::String: return "String";
        case Kind::Utf8: return "Utf8";
        case Kind::Utf16: return "Utf16";
        case Kind::Utf32: return "Utf32";
        case Kind::FixedArray: return "FixedArray";
        case Kind::DynamicArray: return "DynamicArray";
        case Kind::HeterogeneousArray: return "HeterogeneousArray";
        case Kind::UserDefined: return "UserDefined";
        case Kind::Call: return "Call";
        case Kind::Generic: return "Generic";
        case Kind::Block: return "Block";
        case Kind::Trait: return "Trait";
        case Kind::FunctionTrait: return "FunctionTrait";
        case Kind::UniquePtr: return "UniquePtr";
        case Kind::SharedPtr: return "SharedPtr";
        case Kind::WeakPtr: return "WeakPtr";
        default: return "Unknown";
    }
}

bool isPrimitiveKind(Kind kind) {
    return kind == Kind::Bool || kind == Kind::Char || kind == Kind::Char16 || kind == Kind::Char32 ||
           kind == Kind::Size_t || isIntegerKind(kind) || isFloatKind(kind);
}

bool isIntegerKind(Kind kind) {
    return kind == Kind::Int8 || kind == Kind::Int16 || kind == Kind::Int32 || kind == Kind::Int64 ||
           kind == Kind::Int128 || kind == Kind::Int256 || kind == Kind::Int512 || kind == Kind::Int1024 ||
           kind == Kind::BigInt || kind == Kind::UInt8 || kind == Kind::UInt16 || kind == Kind::UInt32 ||
           kind == Kind::UInt64 || kind == Kind::UInt128 || kind == Kind::UInt256 || kind == Kind::UInt512 ||
           kind == Kind::UInt1024 || kind == Kind::Size_t;
}

bool isFloatKind(Kind kind) {
    return kind == Kind::Half || kind == Kind::Float || kind == Kind::Double ||
           kind == Kind::FP128 || kind == Kind::X86_FP80 || kind == Kind::PPC_FP128;
}

bool isCharKind(Kind kind) {
    return kind == Kind::Char || kind == Kind::Char16 || kind == Kind::Char32;
}

bool isStringKind(Kind kind) {
    return kind == Kind::String || kind == Kind::Utf8 || kind == Kind::Utf16 || kind == Kind::Utf32;
}

bool isPointerKind(Kind kind) {
    return kind == Kind::Pointer || kind == Kind::Nullptr;
}

bool isArrayKind(Kind kind) {
    return kind == Kind::Array || kind == Kind::FixedArray || kind == Kind::DynamicArray ||
           kind == Kind::HeterogeneousArray;
}

bool isSmartPointerKind(Kind kind) {
    return kind == Kind::UniquePtr || kind == Kind::SharedPtr || kind == Kind::WeakPtr;
}

} // namespace Omniscript