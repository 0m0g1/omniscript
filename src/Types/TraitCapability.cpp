#include <omniscript/Core.h>
#include <omniscript/Types/TraitCapability.h>
#include <omniscript/Types/BaseType.h>
#include <omniscript/Types/DerivedTypes.h>

namespace Omniscript {

bool FunctionTrait::isImplementedBy(const std::shared_ptr<Type>& type) const {
    if (!type) return false;
    return type->implementsFunctionTrait(name);
}

std::shared_ptr<FunctionType> FunctionTrait::getConcreteSignature(
    const std::shared_ptr<Type>& type,
    const std::vector<std::shared_ptr<Type>>& typeArgs) const {
    if (!isImplementedBy(type)) {
        TYPE_ERROR("Type " + type->toString() + " does not implement function trait " + name);
        return nullptr;
    }
    return std::dynamic_pointer_cast<FunctionType>(signature->clone());
}

void Trait::addCapability(const TraitCapability& capability) {
    capabilities.push_back(capability);
}

bool Trait::isImplementedBy(const std::shared_ptr<Type>& type) const {
    if (!type) return false;
    return type->implementsTrait(name);
}

std::vector<TraitCapability> Trait::getAllCapabilities() const {
    std::vector<TraitCapability> result = capabilities;
    for (const auto& superTrait : superTraits) {
        if (superTrait) {
            auto superCaps = superTrait->getAllCapabilities();
            result.insert(result.end(), superCaps.begin(), superCaps.end());
        }
    }
    return result;
}

bool Trait::extends(const std::shared_ptr<Trait>& other) const {
    if (!other) return false;
    if (name == other->name) return true;
    for (const auto& superTrait : superTraits) {
        if (superTrait && superTrait->extends(other)) {
            return true;
        }
    }
    return false;
}

bool TraitImplementation::isComplete() const {
    if (!trait || !implementingType) return false;
    auto allCaps = trait->getAllCapabilities();
    for (const auto& cap : allCaps) {
        if (cap.isRequired && implementations.find(cap.name) == implementations.end()) {
            return false;
        }
    }
    return true;
}

std::shared_ptr<FunctionType> TraitImplementation::getImplementation(const std::string& capabilityName) const {
    auto it = implementations.find(capabilityName);
    if (it != implementations.end()) {
        return it->second;
    }
    return nullptr;
}

TraitSystem& TraitSystem::instance() {
    static TraitSystem instance;
    return instance;
}

void TraitSystem::registerTrait(std::shared_ptr<Trait> trait) {
    if (trait) {
        traits[trait->name] = trait;
        DEBUG_LOG("Registered trait: " + trait->name);
    }
}

void TraitSystem::registerFunctionTrait(std::shared_ptr<FunctionTrait> functionTrait) {
    if (functionTrait) {
        functionTraits[functionTrait->name] = functionTrait;
        DEBUG_LOG("Registered function trait: " + functionTrait->name);
    }
}

void TraitSystem::registerImplementation(const TraitImplementation& impl) {
    if (impl.isComplete()) {
        implementations.push_back(impl);
        implementationCache.clear(); // Invalidate cache
        DEBUG_LOG("Registered implementation for type " + impl.implementingType->toString() + " and trait " + impl.trait->name);
    } else {
        TYPE_ERROR("Incomplete trait implementation for type " + impl.implementingType->toString());
    }
}

bool TraitSystem::canPerform(const std::shared_ptr<Type>& type, const std::string& capability) const {
    if (!type) return false;
    return type->canPerform(capability);
}

std::shared_ptr<FunctionType> TraitSystem::getCapabilitySignature(
    const std::shared_ptr<Type>& type, const std::string& capability) const {
    if (!type) return nullptr;
    for (const auto& impl : implementations) {
        if (Type::isSame(impl.implementingType, type)) {
            auto sig = impl.getImplementation(capability);
            if (sig) return std::dynamic_pointer_cast<FunctionType>(sig->clone());
        }
    }
    return nullptr;
}

bool TraitSystem::implementsTrait(const std::shared_ptr<Type>& type, const std::string& traitName) const {
    if (!type) return false;
    auto cacheKey = type->toString() + ":" + traitName;
    auto cacheIt = implementationCache.find(cacheKey);
    if (cacheIt != implementationCache.end()) {
        return cacheIt->second;
    }

    for (const auto& impl : implementations) {
        if (Type::isSame(impl.implementingType, type) && impl.trait->name == traitName) {
            implementationCache[cacheKey] = true;
            return true;
        }
    }

    auto traitIt = traits.find(traitName);
    if (traitIt != traits.end()) {
        for (const auto& superTrait : traitIt->second->superTraits) {
            if (implementsTrait(type, superTrait->name)) {
                implementationCache[cacheKey] = true;
                return true;
            }
        }
    }

    implementationCache[cacheKey] = false;
    return false;
}

bool TraitSystem::implementsFunctionTrait(const std::shared_ptr<Type>& type, const std::string& functionTraitName) const {
    if (!type) return false;
    auto cacheKey = type->toString() + ":" + functionTraitName;
    auto cacheIt = implementationCache.find(cacheKey);
    if (cacheIt != implementationCache.end()) {
        return cacheIt->second;
    }

    for (const auto& impl : implementations) {
        if (Type::isSame(impl.implementingType, type)) {
            for (const auto& cap : impl.implementations) {
                auto fnTrait = functionTraits.find(functionTraitName);
                if (fnTrait != functionTraits.end() && cap.first == fnTrait->second->name) {
                    implementationCache[cacheKey] = true;
                    return true;
                }
            }
        }
    }

    implementationCache[cacheKey] = false;
    return false;
}

std::vector<std::shared_ptr<Trait>> TraitSystem::getImplementedTraits(const std::shared_ptr<Type>& type) const {
    std::vector<std::shared_ptr<Trait>> result;
    if (!type) return result;

    for (const auto& impl : implementations) {
        if (Type::isSame(impl.implementingType, type)) {
            result.push_back(impl.trait);
        }
    }
    return result;
}

std::vector<std::shared_ptr<FunctionTrait>> TraitSystem::getImplementedFunctionTraits(const std::shared_ptr<Type>& type) const {
    std::vector<std::shared_ptr<FunctionTrait>> result;
    if (!type) return result;

    for (const auto& impl : implementations) {
        if (Type::isSame(impl.implementingType, type)) {
            for (const auto& cap : impl.implementations) {
                auto fnTrait = functionTraits.find(cap.first);
                if (fnTrait != functionTraits.end()) {
                    result.push_back(fnTrait->second);
                }
            }
        }
    }
    return result;
}

std::vector<std::shared_ptr<Type>> TraitSystem::findTypesWithTrait(const std::string& traitName) const {
    std::vector<std::shared_ptr<Type>> result;
    for (const auto& impl : implementations) {
        if (impl.trait->name == traitName) {
            result.push_back(impl.implementingType);
        }
    }
    return result;
}

std::vector<std::shared_ptr<Type>> TraitSystem::findTypesWithCapability(const std::string& capability) const {
    std::vector<std::shared_ptr<Type>> result;
    for (const auto& impl : implementations) {
        if (impl.implementations.find(capability) != impl.implementations.end()) {
            result.push_back(impl.implementingType);
        }
    }
    return result;
}

namespace BuiltinTraits {

std::shared_ptr<Trait> Copyable = std::make_shared<Trait>("Copyable");
std::shared_ptr<Trait> Movable = std::make_shared<Trait>("Movable");
std::shared_ptr<Trait> Comparable = std::make_shared<Trait>("Comparable");
std::shared_ptr<Trait> Hashable = std::make_shared<Trait>("Hashable");
std::shared_ptr<Trait> Printable = std::make_shared<Trait>("Printable");
std::shared_ptr<Trait> Addable = std::make_shared<Trait>("Addable");
std::shared_ptr<Trait> Numeric = std::make_shared<Trait>("Numeric");
std::shared_ptr<Trait> Orderable = std::make_shared<Trait>("Orderable");

std::shared_ptr<FunctionTrait> Add;
std::shared_ptr<FunctionTrait> Subtract;
std::shared_ptr<FunctionTrait> Multiply;
std::shared_ptr<FunctionTrait> Divide;
std::shared_ptr<FunctionTrait> Equal;
std::shared_ptr<FunctionTrait> Compare;

void initializeBuiltinTraits() {
    auto genericT = Type::createGenericType("T");
    auto genericFn = Type::createFunctionType("add", {genericT, genericT}, genericT);
    Add = std::make_shared<FunctionTrait>("add", std::dynamic_pointer_cast<FunctionType>(genericFn));
    
    genericFn = Type::createFunctionType("subtract", {genericT, genericT}, genericT);
    Subtract = std::make_shared<FunctionTrait>("subtract", std::dynamic_pointer_cast<FunctionType>(genericFn));
    
    genericFn = Type::createFunctionType("multiply", {genericT, genericT}, genericT);
    Multiply = std::make_shared<FunctionTrait>("multiply", std::dynamic_pointer_cast<FunctionType>(genericFn));
    
    genericFn = Type::createFunctionType("divide", {genericT, genericT}, genericT);
    Divide = std::make_shared<FunctionTrait>("divide", std::dynamic_pointer_cast<FunctionType>(genericFn));
    
    genericFn = Type::createFunctionType("equal", {genericT, genericT}, Type::createPrimitiveType(Kind::Bool));
    Equal = std::make_shared<FunctionTrait>("equal", std::dynamic_pointer_cast<FunctionType>(genericFn));
    
    genericFn = Type::createFunctionType("compare", {genericT, genericT}, Type::createPrimitiveType(Kind::Int32));
    Compare = std::make_shared<FunctionTrait>("compare", std::dynamic_pointer_cast<FunctionType>(genericFn));

    TraitSystem& ts = TraitSystem::instance();
    ts.registerTrait(Copyable);
    ts.registerTrait(Movable);
    ts.registerTrait(Comparable);
    ts.registerTrait(Hashable);
    ts.registerTrait(Printable);
    ts.registerTrait(Addable);
    ts.registerTrait(Numeric);
    ts.registerTrait(Orderable);
    ts.registerFunctionTrait(Add);
    ts.registerFunctionTrait(Subtract);
    ts.registerFunctionTrait(Multiply);
    ts.registerFunctionTrait(Divide);
    ts.registerFunctionTrait(Equal);
    ts.registerFunctionTrait(Compare);

    // Example: Register Vec2 and Vec3 implementations
    auto vec2Type = Type::createUserDefinedType("Vec2", Kind::Struct);
    auto vec3Type = Type::createUserDefinedType("Vec3", Kind::Struct);
    
    TraitImplementation vec2AddImpl;
    vec2AddImpl.implementingType = vec2Type;
    vec2AddImpl.trait = Addable;
    
    // Cast the returned Type to FunctionType explicitly
    vec2AddImpl.implementations["add"] = 
        std::dynamic_pointer_cast<FunctionType>(
            Type::createFunctionType("add", {vec2Type, vec2Type}, vec2Type)
        );
    
    vec2AddImpl.implementations["add_vec3"] = 
        std::dynamic_pointer_cast<FunctionType>(
            Type::createFunctionType("add", {vec2Type, vec3Type}, vec2Type)
        );
    
    ts.registerImplementation(vec2AddImpl);

    // Sprite implementations
    auto spriteType = Type::createUserDefinedType("Sprite", Kind::Struct);

    // Movable trait implementation
    TraitImplementation spriteImpl;
    spriteImpl.implementingType = spriteType;
    spriteImpl.trait = std::make_shared<Trait>("Movable");
    spriteImpl.implementations["move"] = 
        std::dynamic_pointer_cast<FunctionType>(
            Type::createFunctionType("move", {spriteType}, Type::createPrimitiveType(Kind::Void))
        );
    ts.registerImplementation(spriteImpl);

    // Drawable trait implementation
    spriteImpl.trait = std::make_shared<Trait>("Drawable");
    spriteImpl.implementations.clear();
    spriteImpl.implementations["draw"] = 
        std::dynamic_pointer_cast<FunctionType>(
            Type::createFunctionType("draw", {spriteType}, Type::createPrimitiveType(Kind::Void))
        );
    ts.registerImplementation(spriteImpl);

    // Scene implementations
    auto sceneType = Type::createUserDefinedType("Scene", Kind::Struct);
    TraitImplementation sceneImpl;
    sceneImpl.implementingType = sceneType;
    sceneImpl.trait = Addable;
    sceneImpl.implementations["add"] = 
        std::dynamic_pointer_cast<FunctionType>(
            Type::createFunctionType("add", {sceneType, spriteType}, Type::createPrimitiveType(Kind::Void))
        );
    ts.registerImplementation(sceneImpl);
}

} // namespace BuiltinTraits

} // namespace Omniscript