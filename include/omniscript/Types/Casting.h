#pragma once

#include <memory>
#include <omniscript/Types/TypeKind.h>

namespace Omniscript {

class Type;

enum class CastKind {
    // `as` - Safe cast (with runtime check, returns value or throws/captures error if failed)
    SafeCast,

    // `as?` - Optional cast (returns nullable, i.e. null on failure instead of throwing)
    OptionalCast,

    // `as!` - Force cast (unsafe, no runtime check, panics or crashes on invalid cast)
    ForceCast,

    // `as*` - Bitcast (reinterpret memory layout directly, purely unsafe, like C++ reinterpret_cast)
    BitCast
};

enum class CastResult {
    Success,   // Cast is valid and safe
    Failure,   // Cast is invalid at runtime (for SafeCast/OptionalCast)
    Unsafe,    // ForceCast or BitCast used on invalid types
    Invalid    // Static rejection (e.g., incompatible types)
};

class CastingSystem {
public:
    // Check if a cast is valid
    static CastResult canCast(
        const std::shared_ptr<Type>& from, 
        const std::shared_ptr<Type>& to, 
        CastKind castKind
    );
    
    // Perform implicit conversion check
    static bool isImplicitlyConvertible(
        const std::shared_ptr<Type>& from, 
        const std::shared_ptr<Type>& to
    );
    
    // Check for safe explicit cast (`as`)
    static bool isSafelyCastable(
        const std::shared_ptr<Type>& from, 
        const std::shared_ptr<Type>& to
    );
    
    // Check if bitcast is valid (same size, compatible memory layout)
    static bool isBitCastable(
        const std::shared_ptr<Type>& from, 
        const std::shared_ptr<Type>& to
    );
    
    // Get the result type of a cast operation
    static std::shared_ptr<Type> getCastResultType(
        const std::shared_ptr<Type>& from, 
        const std::shared_ptr<Type>& to, 
        CastKind castKind
    );

private:
    // Helper methods for specific cast checks
    static bool checkNumericCast(
        const std::shared_ptr<Type>& from, 
        const std::shared_ptr<Type>& to, 
        CastKind castKind
    );
    
    static bool checkPointerCast(
        const std::shared_ptr<Type>& from, 
        const std::shared_ptr<Type>& to, 
        CastKind castKind
    );
    
    static bool checkInheritanceCast(
        const std::shared_ptr<Type>& from, 
        const std::shared_ptr<Type>& to, 
        CastKind castKind
    );
};

// Cast operation result for runtime execution
struct CastOperation {
    CastKind kind;
    std::shared_ptr<Type> fromType;
    std::shared_ptr<Type> toType;
    std::shared_ptr<Type> resultType;
    bool requiresRuntimeCheck; // True for SafeCast and OptionalCast
    bool isImplicit;           // True if cast is implicit
};

} // namespace Omniscript
