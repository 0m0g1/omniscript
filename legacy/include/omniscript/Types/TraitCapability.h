#pragma once

#include <memory>
#include <vector>
#include <string>
#include <unordered_map>
#include <omniscript/Types/TypeKind.h>

namespace Omniscript {

class Type;
class FunctionType;

// Trait capability - represents what a type "can" do
struct TraitCapability {
    std::string name;
    std::shared_ptr<FunctionType> signature;
    bool isRequired = true;
    std::string documentation;
    
    TraitCapability(const std::string& name, std::shared_ptr<FunctionType> sig, bool required = true)
        : name(name), signature(sig), isRequired(required) {}
};

// Function trait - defines a specific function signature that types can implement
class FunctionTrait {
public:
    std::string name;
    std::vector<std::string> typeParameters; // Generic type parameters like <T>
    std::shared_ptr<FunctionType> signature;
    std::string documentation;
    
    FunctionTrait(const std::string& name, std::shared_ptr<FunctionType> sig)
        : name(name), signature(sig) {}
    
    // Check if a type implements this function trait
    bool isImplementedBy(const std::shared_ptr<Type>& type) const;
    
    // Get the concrete signature for a specific type
    std::shared_ptr<FunctionType> getConcreteSignature(
        const std::shared_ptr<Type>& type,
        const std::vector<std::shared_ptr<Type>>& typeArgs = {}
    ) const;
};

// Regular trait - collection of capabilities/functions
class Trait {
public:
    std::string name;
    std::vector<std::string> typeParameters;
    std::vector<TraitCapability> capabilities;
    std::vector<std::shared_ptr<Trait>> superTraits; // Trait inheritance
    std::string documentation;
    
    Trait(const std::string& name) : name(name) {}
    
    // Add a capability to this trait
    void addCapability(const TraitCapability& capability);
    
    // Check if a type implements this trait
    bool isImplementedBy(const std::shared_ptr<Type>& type) const;
    
    // Get all required capabilities (including from super traits)
    std::vector<TraitCapability> getAllCapabilities() const;
    
    // Check if this trait extends another trait
    bool extends(const std::shared_ptr<Trait>& other) const;
};

// Trait implementation for a specific type
struct TraitImplementation {
    std::shared_ptr<Type> implementingType;
    std::shared_ptr<Trait> trait;
    std::unordered_map<std::string, std::shared_ptr<FunctionType>> implementations;
    std::vector<std::shared_ptr<Type>> typeArguments; // For generic traits
    
    bool isComplete() const;
    std::shared_ptr<FunctionType> getImplementation(const std::string& capabilityName) const;
};

// Global trait system manager
class TraitSystem {
public:
    static TraitSystem& instance();
    
    // Register traits
    void registerTrait(std::shared_ptr<Trait> trait);
    void registerFunctionTrait(std::shared_ptr<FunctionTrait> functionTrait);
    
    // Register trait implementations
    void registerImplementation(const TraitImplementation& impl);
    
    // Query capabilities
    bool canPerform(const std::shared_ptr<Type>& type, const std::string& capability) const;
    std::shared_ptr<FunctionType> getCapabilitySignature(
        const std::shared_ptr<Type>& type, 
        const std::string& capability
    ) const;
    
    // Check trait relationships
    bool implementsTrait(const std::shared_ptr<Type>& type, const std::string& traitName) const;
    bool implementsFunctionTrait(const std::shared_ptr<Type>& type, const std::string& functionTraitName) const;
    
    // Get all traits implemented by a type
    std::vector<std::shared_ptr<Trait>> getImplementedTraits(const std::shared_ptr<Type>& type) const;
    std::vector<std::shared_ptr<FunctionTrait>> getImplementedFunctionTraits(const std::shared_ptr<Type>& type) const;
    
    // Trait resolution for generic contexts
    std::vector<std::shared_ptr<Type>> findTypesWithTrait(const std::string& traitName) const;
    std::vector<std::shared_ptr<Type>> findTypesWithCapability(const std::string& capability) const;

private:
    TraitSystem() = default;
    
    std::unordered_map<std::string, std::shared_ptr<Trait>> traits;
    std::unordered_map<std::string, std::shared_ptr<FunctionTrait>> functionTraits;
    std::vector<TraitImplementation> implementations;
    
    // Cache for performance
    mutable std::unordered_map<std::string, bool> implementationCache;
};

// Built-in trait definitions
namespace BuiltinTraits {
    // Basic traits
    extern std::shared_ptr<Trait> Copyable;
    extern std::shared_ptr<Trait> Movable;
    extern std::shared_ptr<Trait> Comparable;
    extern std::shared_ptr<Trait> Hashable;
    extern std::shared_ptr<Trait> Printable;
    
    // Arithmetic traits
    extern std::shared_ptr<Trait> Addable;
    extern std::shared_ptr<Trait> Numeric;
    extern std::shared_ptr<Trait> Orderable;
    
    // Function traits for operators
    extern std::shared_ptr<FunctionTrait> Add;
    extern std::shared_ptr<FunctionTrait> Subtract;
    extern std::shared_ptr<FunctionTrait> Multiply;
    extern std::shared_ptr<FunctionTrait> Divide;
    extern std::shared_ptr<FunctionTrait> Equal;
    extern std::shared_ptr<FunctionTrait> Compare;
    
    void initializeBuiltinTraits();
}

} // namespace Omniscript