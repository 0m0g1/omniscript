#pragma once

#include <omniscript/Types/BaseType.h>
#include <unordered_map>

namespace Omniscript {

// ----- Primitive Type -----
class PrimitiveType : public Type {
public:
    template <typename T>
    static Kind get() {
        if constexpr (std::is_same_v<T, bool>) return Kind::Bool;
        if constexpr (std::is_same_v<T, char>) return Kind::Char;
        if constexpr (std::is_same_v<T, char8_t>) return Kind::Char;
        if constexpr (std::is_same_v<T, char16_t>) return Kind::Char16;
        if constexpr (std::is_same_v<T, char32_t>) return Kind::Char32;
        
        if constexpr (std::is_same_v<T, int8_t>) return Kind::Int8;
        if constexpr (std::is_same_v<T, int16_t>) return Kind::Int16;
        if constexpr (std::is_same_v<T, int32_t>) return Kind::Int32;
        if constexpr (std::is_same_v<T, int64_t>) return Kind::Int64;
        if constexpr (std::is_same_v<T, __int128>) return Kind::Int128;

        if constexpr (std::is_same_v<T, size_t>) return Kind::Size_t;
        if constexpr (std::is_same_v<T, uint8_t>) return Kind::UInt8;
        if constexpr (std::is_same_v<T, uint16_t>) return Kind::UInt16;
        if constexpr (std::is_same_v<T, uint32_t>) return Kind::UInt32;
        if constexpr (std::is_same_v<T, uint64_t>) return Kind::UInt64;
        if constexpr (std::is_same_v<T, unsigned __int128>) return Kind::UInt128;

        // Platform-specific float16 support
        #ifdef __ARM_ARCH
            if constexpr (std::is_same_v<T, __fp16>) return Kind::Half;
        #elif defined(__x86_64__) || defined(__i386__)
            if constexpr (std::is_same_v<T, _Float16>) return Kind::Half;
        #endif

        if constexpr (std::is_same_v<T, float>) return Kind::Float;
        if constexpr (std::is_same_v<T, double>) return Kind::Double;
        if constexpr (std::is_same_v<T, __float128>) return Kind::FP128;
        if constexpr (std::is_same_v<T, long double>) return Kind::X86_FP80;

        if constexpr (std::is_same_v<T, std::string>) return Kind::String;
        if constexpr (std::is_same_v<T, std::u16string>) return Kind::Utf16;
        if constexpr (std::is_same_v<T, std::u32string>) return Kind::Utf32;

        return Kind::Invalid;
    }

    Kind primitiveKind;

    explicit PrimitiveType(Kind kind_) : Type(kind_), primitiveKind(kind_) {}

    static std::shared_ptr<Type> create(Kind kind) {
        return std::make_shared<PrimitiveType>(kind);
    }

    std::shared_ptr<Type> clone() const override {
        return std::make_shared<PrimitiveType>(primitiveKind);
    }
};

// ----- Unresolved Type -----
class UnresolvedType : public Type {
public:
    std::vector<std::string> dataTypes;
    std::string joinedTypeString;

    UnresolvedType(const std::vector<std::string>& dataTypes)
        : Type(Kind::Unresolved), dataTypes(dataTypes) {
        joinedTypeString.reserve(64);
        for (const auto& s : dataTypes) {
            joinedTypeString += s;
        }
    }

    const std::string& getTypeString() const { return joinedTypeString; }
    
    std::string toString() const override { 
        return "Unresolved<" + joinedTypeString + ">"; 
    }

    std::shared_ptr<Type> clone() const override {
        return std::make_shared<UnresolvedType>(dataTypes);
    }
};

// ----- User Defined Type -----
class UserDefinedType : public Type {
public:
    std::string name;
    std::vector<std::shared_ptr<Type>> paramTypes;
    std::vector<std::shared_ptr<Type>> typeParams;
    std::vector<std::shared_ptr<Type>> baseTypes; // For inheritance

    UserDefinedType(const std::string& name, Kind kind = Kind::UserDefined)
        : Type(kind), name(name) {}

    std::string kindName() const override { return name; }
    std::string getName() const override { return name; }
    
    // Inheritance checking
    bool derivesFrom(const std::string& baseName) const {
        return checkInheritance(baseName, std::unordered_set<std::string>());
    }
    
    bool derivesFrom(const std::shared_ptr<UserDefinedType>& baseType) const {
        return baseType && derivesFrom(baseType->name);
    }

    std::shared_ptr<Type> clone() const override {
        auto cloned = std::make_shared<UserDefinedType>(name, kind);
        cloned->paramTypes = paramTypes;
        cloned->typeParams = typeParams;
        cloned->baseTypes = baseTypes;
        return cloned;
    }

private:
    bool checkInheritance(const std::string& baseName, std::unordered_set<std::string> visited) const {
        if (visited.find(name) != visited.end()) {
            console.warn("Circular inheritance detected in type: " + name);
            return false;
        }
        visited.insert(name);
        
        for (const auto& base : baseTypes) {
            auto baseUDT = std::dynamic_pointer_cast<UserDefinedType>(base);
            if (baseUDT) {
                if (baseUDT->name == baseName || baseUDT->checkInheritance(baseName, visited)) {
                    return true;
                }
            }
        }
        return false;
    }
};

// ----- Pointer Type -----
class PointerType : public Type {
public:
    bool nullCaseHandled = false;
    bool isConst;
    bool isVolatile;
    
    PointerType(std::shared_ptr<Type> pointeeType, bool isConst = false, bool isVolatile = false) 
        : Type(Kind::Pointer), isConst(isConst), isVolatile(isVolatile) {
        this->pointeeType = pointeeType;
    }

    std::shared_ptr<Type> getPointeeType() const override { return pointeeType; }

    int getPointerDepth() const override {
        int depth = 1;
        auto current = pointeeType;
        while (current && current->isPointer()) {
            depth++;
            current = current->getPointeeType();
        }
        return depth;
    }

    std::shared_ptr<Type> getBasePointeeType() const override {
        auto current = pointeeType;
        while (current && current->isPointer()) {
            current = current->getPointeeType();
        }
        return current;
    }

    std::string description() const override { return pointerDescription(); }
    
    std::string pointerDescription() const override {
        std::string desc = "*";
        if (isConst) desc += " const";
        if (isVolatile) desc += " volatile";
        desc += " " + (pointeeType ? pointeeType->toString() : "void");
        return desc;
    }
    
    std::shared_ptr<Type> clone() const override {
        return std::make_shared<PointerType>(
            pointeeType ? pointeeType->clone() : nullptr, 
            isConst, 
            isVolatile
        );
    }
};

// ----- Smart Pointer Types -----
class SmartPointerType : public Type {
public:
    std::shared_ptr<Type> pointeeType;
    
    SmartPointerType(Kind smartPtrKind, std::shared_ptr<Type> pointeeType)
        : Type(smartPtrKind), pointeeType(pointeeType) {
        if (!isSmartPointerKind(smartPtrKind)) {
            console.error("Invalid smart pointer kind provided");
            kind = Kind::Invalid;
        }
    }

    std::shared_ptr<Type> getPointeeType() const override { return pointeeType; }
    
    std::string toString() const override {
        std::string prefix;
        switch (kind) {
            case Kind::UniquePtr: prefix = "unique_ptr<"; break;
            case Kind::SharedPtr: prefix = "shared_ptr<"; break;
            case Kind::WeakPtr: prefix = "weak_ptr<"; break;
            default: prefix = "unknown_ptr<"; break;
        }
        return prefix + (pointeeType ? pointeeType->toString() : "void") + ">";
    }

    std::shared_ptr<Type> clone() const override {
        return std::make_shared<SmartPointerType>(kind, pointeeType ? pointeeType->clone() : nullptr);
    }
};

// ----- Null Types -----
class NullType : public Type {
public:
    bool nullCaseHandled = false;
    std::shared_ptr<Type> innerType;
    
    NullType(std::shared_ptr<Type> innerType = nullptr) 
        : Type(Kind::Null), innerType(innerType) {}

    std::string toString() const override {
        return "Null<" + (innerType ? innerType->toString() : "Unknown") + ">";
    }

    std::shared_ptr<Type> clone() const override {
        return std::make_shared<NullType>(innerType ? innerType->clone() : nullptr);
    }
};

class NullableType : public Type {
public:
    bool nullCaseHandled = false;
    std::shared_ptr<Type> innerType;
    
    NullableType(std::shared_ptr<Type> innerType = nullptr) 
        : Type(Kind::Nullable), innerType(innerType) {}

    std::string toString() const override {
        return (innerType ? innerType->toString() : "Unknown") + "?";
    }

    std::shared_ptr<Type> clone() const override {
        return std::make_shared<NullableType>(innerType ? innerType->clone() : nullptr);
    }
};

class NullPointerType : public Type {
public:
    bool nullCaseHandled = false;
    bool isConst;
    bool isVolatile;

    NullPointerType(std::shared_ptr<Type> pointeeType, bool isConst = false, bool isVolatile = false)
        : Type(Kind::Nullptr), isConst(isConst), isVolatile(isVolatile) {
        this->pointeeType = pointeeType;
    }

    std::shared_ptr<Type> getPointeeType() const override { return pointeeType; }

    int getPointerDepth() const override {
        int depth = 1;
        auto current = pointeeType;
        while (current && current->isPointer()) {
            depth++;
            current = current->getPointeeType();
        }
        return depth;
    }

    std::shared_ptr<Type> getBasePointeeType() const override {
        auto current = pointeeType;
        while (current && current->isPointer()) {
            current = current->getPointeeType();
        }
        return current;
    }

    std::string description() const override { return pointerDescription(); }
    
    std::string pointerDescription() const override {
        std::string desc = "nullptr";
        if (pointeeType && !pointeeType->isVoidLike()) {
            desc += "<" + pointeeType->toString() + ">";
        }
        return desc;
    }
    
    std::shared_ptr<Type> clone() const override {
        return std::make_shared<NullPointerType>(
            pointeeType ? pointeeType->clone() : nullptr, 
            isConst, 
            isVolatile
        );
    }
};

// ----- Reference Type -----
class ReferenceType : public Type {
public:
    std::shared_ptr<Type> referentType;

    explicit ReferenceType(std::shared_ptr<Type> referentType)
        : Type(Kind::Reference), referentType(referentType) {}

    std::shared_ptr<Type> getReferencedType() const override { return referentType; }

    int getReferenceDepth() const override {
        int depth = 1;
        auto current = referentType;
        while (current && current->isReference()) {
            depth++;
            current = std::dynamic_pointer_cast<ReferenceType>(current)->getReferencedType();
        }
        return depth;
    }

    std::shared_ptr<Type> getBaseReferencedType() const override {
        auto current = referentType;
        while (current && current->isReference()) {
            current = std::dynamic_pointer_cast<ReferenceType>(current)->getReferencedType();
        }
        return current;
    }

    std::string toString() const override {
        return "&" + (referentType ? referentType->toString() : "void");
    }

    std::shared_ptr<Type> clone() const override {
        return std::make_shared<ReferenceType>(referentType ? referentType->clone() : nullptr);
    }
};

// ----- Function Type -----
class FunctionType : public Type {
public:
    std::string functionName;
    std::vector<std::shared_ptr<Type>> parameterTypes;
    std::vector<std::string> parameterNames;
    std::shared_ptr<Type> returnType;
    bool isVarArg = false;
    bool isGeneric = false;
    std::vector<std::string> genericParameters;

    FunctionType(
        const std::string& name,
        std::shared_ptr<Type> returnType,
        const std::vector<std::shared_ptr<Type>>& params,
        bool isVarArg = false
    ) : Type(Kind::Function), functionName(name), returnType(returnType), 
        parameterTypes(params), isVarArg(isVarArg) {}

    size_t getArity() const { return parameterTypes.size(); }

    std::shared_ptr<Type> getParamType(size_t index) const {
        return (index < parameterTypes.size()) ? parameterTypes[index] : nullptr;
    }

    std::string getParamName(size_t index) const {
        return (index < parameterNames.size()) ? parameterNames[index] : "";
    }

    void setParamName(size_t index, const std::string& name) {
        if (index >= parameterNames.size()) {
            parameterNames.resize(index + 1);
        }
        parameterNames[index] = name;
    }

    std::shared_ptr<Type> getReturnType() const override { return returnType; }

    bool isCompatibleWith(const std::shared_ptr<FunctionType>& other) const {
        if (!other) return false;
        
        // Check parameter count
        if (parameterTypes.size() != other->parameterTypes.size()) return false;
        if (isVarArg != other->isVarArg) return false;
        
        // Check parameter types (contravariant)
        for (size_t i = 0; i < parameterTypes.size(); ++i) {
            if (!Type::isSameOrCastableTo(other->parameterTypes[i], parameterTypes[i])) {
                return false;
            }
        }
        
        // Check return type (covariant)
        return Type::isSameOrCastableTo(returnType, other->returnType);
    }

    std::string toString() const override {
        std::string result = "fn";
        if (!functionName.empty()) {
            result += " " + functionName;
        }
        
        result += "(";
        for (size_t i = 0; i < parameterTypes.size(); ++i) {
            if (i > 0) result += ", ";
            
            std::string paramName = getParamName(i);
            if (!paramName.empty()) {
                result += paramName + ": ";
            }
            result += parameterTypes[i] ? parameterTypes[i]->toString() : "void";
        }
        if (isVarArg) {
            if (!parameterTypes.empty()) result += ", ";
            result += "...";
        }
        result += ") => ";
        result += returnType ? returnType->toString() : "void";
        return result;
    }

    std::shared_ptr<Type> clone() const override {
        auto cloned = std::make_shared<FunctionType>(
            functionName, 
            returnType ? returnType->clone() : nullptr, 
            std::vector<std::shared_ptr<Type>>{}, 
            isVarArg
        );
        
        for (const auto& param : parameterTypes) {
            cloned->parameterTypes.push_back(param ? param->clone() : nullptr);
        }
        cloned->parameterNames = parameterNames;
        cloned->isGeneric = isGeneric;
        cloned->genericParameters = genericParameters;
        
        return cloned;
    }
};

// ----- Generic Type -----
class GenericType : public Type {
public:
    std::string name;
    std::vector<std::shared_ptr<Type>> constraints; // Trait constraints

    GenericType(const std::string& name_) : Type(Kind::Generic), name(name_) {}

    std::string getName() const override { return name; }
    
    void addConstraint(std::shared_ptr<Type> constraint) {
        constraints.push_back(constraint);
    }
    
    bool satisfiesConstraints(const std::shared_ptr<Type>& candidate) const {
        for (const auto& constraint : constraints) {
            if (!Type::isCompatibleWith(candidate, constraint)) {
                return false;
            }
        }
        return true;
    }

    std::string toString() const override {
        std::string result = name;
        if (!constraints.empty()) {
            result += " : ";
            for (size_t i = 0; i < constraints.size(); ++i) {
                if (i > 0) result += " + ";
                result += constraints[i]->toString();
            }
        }
        return result;
    }

    std::shared_ptr<Type> clone() const override {
        auto cloned = std::make_shared<GenericType>(name);
        for (const auto& constraint : constraints) {
            cloned->constraints.push_back(constraint ? constraint->clone() : nullptr);
        }
        return cloned;
    }
};

// ----- Array Types -----
class ArrayType : public Type {
public:
    std::shared_ptr<Type> elementType;
    
protected:
    ArrayType(Kind arrayKind, std::shared_ptr<Type> elementType)
        : Type(arrayKind), elementType(elementType) {}
        
public:
    std::shared_ptr<Type> getElementType() const override { return elementType; }
};

class FixedArrayType : public ArrayType {
public:
    size_t size;
    
    FixedArrayType(std::shared_ptr<Type> elementType, size_t size)
        : ArrayType(Kind::FixedArray, elementType), size(size) {}
        
    std::string toString() const override {
        return (elementType ? elementType->toString() : "unknown") + "[" + std::to_string(size) + "]";
    }
    
    std::shared_ptr<Type> clone() const override {
        return std::make_shared<FixedArrayType>(
            elementType ? elementType->clone() : nullptr, 
            size
        );
    }
};

class DynamicArrayType : public ArrayType {
public:
    DynamicArrayType(std::shared_ptr<Type> elementType)
        : ArrayType(Kind::DynamicArray, elementType) {}
        
    std::string toString() const override {
        return (elementType ? elementType->toString() : "unknown") + "[]";
    }
    
    std::shared_ptr<Type> clone() const override {
        return std::make_shared<DynamicArrayType>(
            elementType ? elementType->clone() : nullptr
        );
    }
};

class HeterogeneousArrayType : public ArrayType {
public:
    std::vector<std::shared_ptr<Type>> elementTypes;
    
    HeterogeneousArrayType() : ArrayType(Kind::HeterogeneousArray, nullptr) {}
    
    void addElementType(std::shared_ptr<Type> type) {
        elementTypes.push_back(type);
    }
    
    std::shared_ptr<Type> getElementType(size_t index) const {
        return (index < elementTypes.size()) ? elementTypes[index] : nullptr;
    }
    
    size_t getElementCount() const { return elementTypes.size(); }
    
    std::string toString() const override {
        std::string result = "[";
        for (size_t i = 0; i < elementTypes.size(); ++i) {
            if (i > 0) result += ", ";
            result += elementTypes[i] ? elementTypes[i]->toString() : "unknown";
        }
        result += "]";
        return result;
    }
    
    std::shared_ptr<Type> clone() const override {
        auto cloned = std::make_shared<HeterogeneousArrayType>();
        for (const auto& type : elementTypes) {
            cloned->elementTypes.push_back(type ? type->clone() : nullptr);
        }
        return cloned;
    }
};

// ----- Trait Types -----
class TraitType : public Type {
public:
    std::string name;
    std::vector<std::string> capabilities;
    std::vector<std::shared_ptr<TraitType>> superTraits;
    
    TraitType(const std::string& name) : Type(Kind::Trait), name(name) {}
    
    std::string getName() const override { return name; }
    
    void addCapability(const std::string& capability) {
        capabilities.push_back(capability);
    }
    
    void addSuperTrait(std::shared_ptr<TraitType> superTrait) {
        superTraits.push_back(superTrait);
    }
    
    bool hasCapability(const std::string& capability) const {
        // Check direct capabilities
        for (const auto& cap : capabilities) {
            if (cap == capability) return true;
        }
        
        // Check inherited capabilities
        for (const auto& superTrait : superTraits) {
            if (superTrait && superTrait->hasCapability(capability)) {
                return true;
            }
        }
        
        return false;
    }
    
    std::string toString() const override {
        return "trait " + name;
    }
    
    std::shared_ptr<Type> clone() const override {
        auto cloned = std::make_shared<TraitType>(name);
        cloned->capabilities = capabilities;
        cloned->superTraits = superTraits; // Shallow copy is fine for traits
        return cloned;
    }
};

class FunctionTraitType : public Type {
public:
    std::string name;
    std::shared_ptr<FunctionType> signature;
    
    FunctionTraitType(const std::string& name, std::shared_ptr<FunctionType> signature)
        : Type(Kind::FunctionTrait), name(name), signature(signature) {}
        
    std::string getName() const override { return name; }
    
    std::string toString() const override {
        return "trait fn " + name + (signature ? signature->toString().substr(2) : "()"); // Remove "fn" prefix
    }
    
    std::shared_ptr<Type> clone() const override {
        return std::make_shared<FunctionTraitType>(
            name, 
            signature ? std::dynamic_pointer_cast<FunctionType>(signature->clone()) : nullptr
        );
    }
};

} // namespace Omniscript