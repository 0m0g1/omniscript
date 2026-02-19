#pragma once

#include <omniscript/Tokens.h>
#include <omniscript/Core.h>
#include <omniscript/omniscript_pch.h>
#include <omniscript/Target_config.h>
#include <omniscript/Console.h>
#include <omniscript/Types/TypeKind.h>
#include <memory>
#include <string>
#include <vector>
#include <unordered_set>

namespace Omniscript {

// Forward declarations
class Trait;
class FunctionTrait;
class TraitImplementation;

inline constexpr int getPointerBitWidth() {
    #if defined(TARGET_32BIT)
        return 32;
    #elif defined(TARGET_64BIT)
        return 64;
    #else
        return sizeof(void*) * 8;  
    #endif
}

// Base type class
class Type {
public:
    std::string parameterName;
    Kind kind = Kind::Invalid;
    std::shared_ptr<Type> returnType;
    std::shared_ptr<Type> elementType;
    std::shared_ptr<Type> pointeeType;
    size_t fixedSize = 0;
    
    // Trait system integration
    std::vector<std::shared_ptr<Trait>> implementedTraits;
    std::vector<std::shared_ptr<FunctionTrait>> implementedFunctionTraits;
    std::unordered_set<std::string> capabilities;

    virtual ~Type() = default;

    Type() {}
    explicit Type(Kind kind) : kind(kind) {}

    // ----- Basic type queries -----
    bool isUserDefined() const { return kind == Kind::UserDefined; }
    bool isInvalid() const { return kind == Kind::Invalid; }
    bool isVoidLike() const { return kind == Kind::Void || kind == Kind::Undefined; }
    bool isUnresolved() const { return kind == Kind::Unresolved; }
    bool isUndefined() const { return kind == Kind::Undefined; }
    bool isPointer() const { return kind == Kind::Pointer || kind == Kind::Nullptr; }
    bool isNullType() const { return kind == Kind::Null || kind == Kind::Nullptr; }
    bool isNull() const { return kind == Kind::Null; }
    bool isNullable() const { return kind == Kind::Nullable; }
    bool isNullPointer() const { return kind == Kind::Nullptr; }
    bool isReference() const { return kind == Kind::Reference; }
    bool isFunction() const { return kind == Kind::Function; }
    bool isPrimitive() const { return isPrimitiveKind(kind); }
    bool isArray() const { return isArrayKind(kind); }
    bool isFixedArray() const { return kind == Kind::FixedArray; }
    bool isDynamicArray() const { return kind == Kind::DynamicArray; }
    bool isHeterogeneousArray() const { return kind == Kind::HeterogeneousArray; }
    bool isGeneric() const { return kind == Kind::Generic; }
    bool isBlock() const { return kind == Kind::Block; }
    bool isBool() const { return kind == Kind::Bool; }
    bool isStruct() const { return kind == Kind::Struct; }
    bool isClass() const { return kind == Kind::Class; }
    bool isModule() const { return kind == Kind::Module; }
    bool isTrait() const { return kind == Kind::Trait; }
    bool isFunctionTrait() const { return kind == Kind::FunctionTrait; }
    bool isSmartPointer() const { return isSmartPointerKind(kind); }
    bool isUniquePtr() const { return kind == Kind::UniquePtr; }
    bool isSharedPtr() const { return kind == Kind::SharedPtr; }
    bool isWeakPtr() const { return kind == Kind::WeakPtr; }

    bool isChar(int bitwidth = -1) const;
    bool isString(int bitwidth = -1) const;
    bool isInteger(int bitwidth = -1) const;
    bool isFloat(int bitWidth = -1) const;
    bool isSigned() const;
    bool isSizeType() const { return kind == Kind::Size_t; }
    bool isNumericLiteral() const { return isInteger() || isFloat(); }

    // ----- Size and layout -----
    int getBitWidth() const { return getSize(); }
    int getSize() const;
    
    // ----- Trait system integration -----
    bool canPerform(const std::string& capability) const;
    bool implementsTrait(const std::string& traitName) const;
    bool implementsFunctionTrait(const std::string& functionTraitName) const;
    void addTrait(std::shared_ptr<Trait> trait);
    void addFunctionTrait(std::shared_ptr<FunctionTrait> functionTrait);
    void addCapability(const std::string& capability);

    // ----- Type relationships -----
    virtual std::shared_ptr<Type> getReturnType() const { return returnType; }
    virtual std::shared_ptr<Type> getElementType() const { return elementType; }
    virtual std::shared_ptr<Type> getPointeeType() const { return pointeeType; }
    virtual std::shared_ptr<Type> getBasePointeeType() const { return nullptr; }
    virtual std::shared_ptr<Type> getReferencedType() const { return nullptr; }
    virtual std::shared_ptr<Type> getBaseReferencedType() const { return nullptr; }
    
    virtual int getReferenceDepth() const { return 0; }
    virtual int getPointerDepth() const { return 0; }

    // ----- String representation -----
    virtual std::string kindName() const;
    virtual std::string description() const { return kindName(); }
    virtual std::string toString() const { return description(); }
    virtual std::string pointerDescription() const { return ""; }
    
    Kind getKind() const { return kind; }
    virtual std::string getName() const { return "type"; }
    virtual std::string getParameterName() const { return parameterName; }

    // ----- Factory methods -----
    static std::shared_ptr<Type> createInvalid();
    static std::shared_ptr<Type> createUnresolved(const std::vector<std::string>& types);
    static std::shared_ptr<Type> createUndefined();
    static std::shared_ptr<Type> createPrimitiveType(Kind kind);
    static std::shared_ptr<Type> createNullType(std::shared_ptr<Type> innerType = nullptr);
    static std::shared_ptr<Type> createNullPointerType(std::shared_ptr<Type> innerType = nullptr);
    static std::shared_ptr<Type> createNullableType(std::shared_ptr<Type> innerType = nullptr);
    static std::shared_ptr<Type> createMetaType();
    static std::shared_ptr<Type> createPointerType(
        std::shared_ptr<Type> pointee,
        bool isConst = false,
        bool isVolatile = false
    );
    static std::shared_ptr<Type> createReferenceType(std::shared_ptr<Type> referent);
    static std::shared_ptr<Type> createFunctionType(
        const std::string& name,
        const std::vector<std::shared_ptr<Type>>& paramTypes = {},
        std::shared_ptr<Type> returnType = nullptr,
        bool isVarArg = false
    );
    static std::shared_ptr<Type> createStringType(Kind stringKind = Kind::String);
    static std::shared_ptr<Type> createFixedArrayType(std::shared_ptr<Type> elementType, size_t size);
    static std::shared_ptr<Type> createDynamicArrayType(std::shared_ptr<Type> elementType);
    static std::shared_ptr<Type> createHeterogeneousArrayType();
    static std::shared_ptr<Type> createGenericType(const std::string& typeName);
    static std::shared_ptr<Type> createUserDefinedType(
        const std::string& name,
        Kind kind = Kind::UserDefined,
        const std::vector<std::shared_ptr<Type>>& paramTypes = {},
        const std::vector<std::shared_ptr<Type>>& typeParams = {},
        const std::vector<std::shared_ptr<Type>>& baseTypes = {}
    );
    static std::shared_ptr<Type> createSmartPointerType(
        Kind pointerKind,
        std::shared_ptr<Type> pointeeType
    );

    // ----- Type comparison -----
    static bool isSame(const std::shared_ptr<Type>& from, const std::shared_ptr<Type>& to);
    static bool isSameOrCastableTo(const std::shared_ptr<Type>& from, const std::shared_ptr<Type>& to);
    
    // Enhanced with trait-aware compatibility
    static bool isCompatibleWith(const std::shared_ptr<Type>& from, const std::shared_ptr<Type>& to);
    static bool hasCommonTrait(const std::shared_ptr<Type>& type1, const std::shared_ptr<Type>& type2, const std::string& traitName);

    virtual std::shared_ptr<Type> clone() const {
        auto cloned = std::make_shared<Type>(*this);
        // Deep copy trait implementations if needed
        return cloned;
    }

protected:
    // Helper for logging with enhanced console
    void logTypeInfo(const std::string& message, Console::LogLevel level = Console::DEBUG_LOG) const;
    void reportTypeError(const std::string& message, Console::ErrorType errorType = Console::TYPE_ERROR) const;
};

} // namespace Omniscript