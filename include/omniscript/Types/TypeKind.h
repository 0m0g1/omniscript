#pragma once

namespace Omniscript {

enum class Kind {
    // Invalid/Special
    Invalid,
    Undefined,
    Unresolved,

    // Primitive Base
    Primitive,
    Void,
    
    // Null Types
    Nullptr, 
    Null, 
    Nullable,
    
    // Boolean
    Bool,
    
    // Character Types
    Char, 
    Char16, 
    Char32,
    
    // Size Type
    Size_t,
    
    // Signed Integer Types
    Int8, Int16, Int32, Int64, Int128, Int256, Int512, Int1024, BigInt,
    
    // Unsigned Integer Types
    UInt8, UInt16, UInt32, UInt64, UInt128, UInt256, UInt512, UInt1024,
    
    // Floating Point Types
    Half, Float, Double, FP128, X86_FP80, PPC_FP128,

    // Special Types
    Label,
    Token,
    Metadata,

    // Aggregate Types
    Module,
    Class,
    Struct,
    Enum,
    Array,
    Vector,
    
    // Pointer/Reference Types
    Pointer,
    Reference,

    // Function Types
    Function,

    // String Types
    String,
    Utf8,
    Utf16,
    Utf32,
    
    // Array Types
    FixedArray,
    DynamicArray,
    HeterogeneousArray,
    
    // User Defined
    UserDefined,
    Call,
    Generic,
    Block,
    
    // Trait Types
    Trait,
    FunctionTrait,
    
    // Smart Pointer Types
    UniquePtr,
    SharedPtr,
    WeakPtr,
};

// Helper functions for Kind
const char* kindToString(Kind kind);
bool isPrimitiveKind(Kind kind);
bool isIntegerKind(Kind kind);
bool isFloatKind(Kind kind);
bool isCharKind(Kind kind);
bool isStringKind(Kind kind);
bool isPointerKind(Kind kind);
bool isArrayKind(Kind kind);
bool isSmartPointerKind(Kind kind);

} // namespace Omniscript