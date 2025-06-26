#pragma once
#include <omniscript/Expression.h>

namespace Omniscript {
struct FunctionInputExpression : public Expression {
    // Parameter modifiers
    bool isVariadic = false;
    bool isConstant = false;
    bool isReference = false;
    bool isPointer = false;
    bool isMutable = false;
    bool isVolatile = false;
    bool isStatic = false;
    bool isInline = false;
    bool isVirtual = false;
    bool isOverride = false;
    bool isFinal = false;
    bool isExplicit = false;
    bool isConstexpr = false;
    bool isConsteval = false;
    bool isConstinit = false;
    bool isNoexcept = false;
    bool isDeleted = false;
    bool isDefaulted = false;
    
    // Access modifiers
    enum class AccessModifier {
        Public,
        Private,
        Protected,
        Internal,
        Package
    };
    AccessModifier accessModifier = AccessModifier::Public;
    
    // Parameter passing modes
    enum class PassingMode {
        ByValue,
        ByReference,
        ByPointer,
        ByConstReference,
        ByRValueReference,
        ByMove,
        InOut,
        Out,
        Ref,
        Copy,
        Lazy
    };
    PassingMode passingMode = PassingMode::ByValue;
    
    // Parameter direction
    enum class Direction {
        In,
        Out,
        InOut,
        Return
    };
    Direction direction = Direction::In;
    
    // Parameter kind
    enum class ParameterKind {
        Regular,
        Optional,
        Named,
        Positional,
        KeywordOnly,
        VariadicPositional,
        VariadicKeyword,
        PackExpansion,
        Template,
        Generic,
        Captured,
        This,
        Self
    };
    ParameterKind parameterKind = ParameterKind::Regular;
    
    // Memory management
    enum class MemoryModel {
        Stack,
        Heap,
        Managed,
        Unmanaged,
        Weak,
        Strong,
        Unique,
        Shared
    };
    MemoryModel memoryModel = MemoryModel::Stack;
    
    // Nullability
    enum class Nullability {
        NonNull,
        Nullable,
        Unknown
    };
    Nullability nullability = Nullability::Unknown;
    
    // Evaluation strategy
    enum class EvaluationStrategy {
        Eager,
        Lazy,
        CallByName,
        CallByNeed,
        CallByValue,
        CallByReference
    };
    EvaluationStrategy evaluationStrategy = EvaluationStrategy::Eager;
    
    // Core parameter data
    std::shared_ptr<Expression> defaultValue;
    std::shared_ptr<Expression> initializer;
    std::vector<std::shared_ptr<Expression>> constraints;
    std::vector<std::shared_ptr<Expression>> attributes;
    std::vector<std::shared_ptr<Expression>> annotations;
    std::vector<std::shared_ptr<Expression>> decorators;
    std::vector<std::shared_ptr<Type>> genericConstraints;
    std::vector<std::shared_ptr<Type>> typeParameters;
    
    // Ranges and bounds
    std::shared_ptr<Expression> minValue;
    std::shared_ptr<Expression> maxValue;
    std::shared_ptr<Expression> arraySize;
    std::vector<std::shared_ptr<Expression>> dimensions;
    
    // String and documentation
    std::string documentation;
    std::string aliasName;
    std::string internalName;
    std::string externalName;
    std::string parameterLabel;
    std::string callingConvention;
    std::string linkage;
    std::string storageClass;
    
    // Flags for various language features
    bool isOptional = false;
    bool hasDefaultValue = false;
    bool isNamed = false;
    bool isPositional = true;
    bool isKeywordOnly = false;
    bool isRequired = true;
    bool isImplicit = false;
    bool isExported = false;
    bool isInternal = false;
    bool isUnsafe = false;
    bool isAsync = false;
    bool isGenerator = false;
    bool isCoroutine = false;
    bool isParallel = false;
    bool isSynchronized = false;
    bool isTransient = false;
    bool isSerializable = true;
    bool isDeprecated = false;
    bool isObsolete = false;
    bool isExperimental = false;
    bool isBeta = false;
    bool isAlpha = false;
    bool isHidden = false;
    bool isReadOnly = false;
    bool isWriteOnly = false;
    bool isLazy = false;
    bool isComputed = false;
    bool isCached = false;
    bool isWeak = false;
    bool isUnowned = false;
    bool isEscaping = false;
    bool isNonEscaping = false;
    bool isAutoclosure = false;
    bool isRethrows = false;
    bool isDiscardableResult = false;
    bool isWarnUnusedResult = false;
    bool isUnavailable = false;
    bool isAvailable = true;
    bool isThreadSafe = false;
    bool isAtomic = false;
    bool isVolatileAccess = false;
    bool isRestricted = false;
    bool isSealed = false;
    bool isAbstract = false;
    bool isConcrete = true;
    bool isGeneric = false;
    bool isParameterized = false;
    bool isSpecialized = false;
    bool isInstantiated = false;
    bool isReified = false;
    bool isErased = false;
    bool isBounded = false;
    bool isUnbounded = false;
    bool isCovariant = false;
    bool isContravariant = false;
    bool isInvariant = true;
    bool isBivariant = false;
    
    // Numeric attributes
    int parameterIndex = -1;
    int priority = 0;
    int alignment = 0;
    size_t sizeInBytes = 0;
    int precision = -1;
    int scale = -1;
    
    // Version and compatibility
    std::string minVersion;
    std::string maxVersion;
    std::string introducedVersion;
    std::string deprecatedVersion;
    std::string removedVersion;
    std::vector<std::string> supportedPlatforms;
    std::vector<std::string> requiredFeatures;
    
    // Error handling
    std::vector<std::shared_ptr<Type>> throwsTypes;
    std::shared_ptr<Expression> errorHandler;
    bool canThrow = false;
    bool canFail = false;
    
    // Ownership and lifetime
    enum class Ownership {
        Owned,
        Borrowed,
        Shared,
        Weak,
        Unowned,
        Consuming,
        NonConsuming
    };
    Ownership ownership = Ownership::Owned;
    
    std::string lifetimeName;
    std::shared_ptr<Expression> lifetimeConstraint;
    
    // Constructors
    FunctionInputExpression(const std::string& name, 
                           std::shared_ptr<Type> type = nullptr, 
                           std::shared_ptr<Expression> defaultValue = nullptr, 
                           bool isConst = false,
                           PassingMode mode = PassingMode::ByValue) :
        defaultValue(std::move(defaultValue)), 
        isConstant(isConst),
        passingMode(mode),
        hasDefaultValue(defaultValue != nullptr) {
        this->name = name;
        this->type = std::move(type);
        this->isRequired = !hasDefaultValue;
    }
    
    // Factory methods for common parameter types
    static std::shared_ptr<FunctionInputExpression> createVariadic(const std::string& name, std::shared_ptr<Type> type = nullptr) {
        auto param = std::make_shared<FunctionInputExpression>(name, type);
        param->isVariadic = true;
        param->parameterKind = ParameterKind::VariadicPositional;
        return param;
    }
    
    static std::shared_ptr<FunctionInputExpression> createOptional(const std::string& name, std::shared_ptr<Type> type = nullptr, std::shared_ptr<Expression> defaultVal = nullptr) {
        auto param = std::make_shared<FunctionInputExpression>(name, type, defaultVal);
        param->isOptional = true;
        param->parameterKind = ParameterKind::Optional;
        return param;
    }
    
    static std::shared_ptr<FunctionInputExpression> createReference(const std::string& name, std::shared_ptr<Type> type = nullptr) {
        auto param = std::make_shared<FunctionInputExpression>(name, type, nullptr, false, PassingMode::ByReference);
        param->isReference = true;
        return param;
    }
    
    static std::shared_ptr<FunctionInputExpression> createInOut(const std::string& name, std::shared_ptr<Type> type = nullptr) {
        auto param = std::make_shared<FunctionInputExpression>(name, type, nullptr, false, PassingMode::InOut);
        param->direction = Direction::InOut;
        return param;
    }
    
    // Utility methods
    bool isPassedByValue() const { return passingMode == PassingMode::ByValue; }
    bool isPassedByReference() const { return passingMode == PassingMode::ByReference || passingMode == PassingMode::ByConstReference; }
    bool isOutput() const { return direction == Direction::Out || direction == Direction::InOut; }
    bool isInput() const { return direction == Direction::In || direction == Direction::InOut; }
    bool hasConstraints() const { return !constraints.empty(); }
    bool hasAttributes() const { return !attributes.empty(); }
    bool isModifiable() const { return !isConstant && !isReadOnly && (isReference || isPointer || direction != Direction::In); }
    
    // Validation
    bool isValid() const {
        if (name.empty()) return false;
        if (isRequired && hasDefaultValue) return false;
        if (isVariadic && hasDefaultValue) return false;
        if (isConstant && (direction == Direction::Out || direction == Direction::InOut)) return false;
        return true;
    }
    
    std::string toString() const override {
        std::string result = "(FunctionInput: " + name;
        
        if (type) result += ", type: " + type->toString();
        if (defaultValue) result += ", default: " + defaultValue->toString();
        if (isConstant) result += ", const";
        if (isReference) result += ", ref";
        if (isVariadic) result += ", variadic";
        if (isOptional) result += ", optional";
        if (isStatic) result += ", static";
        if (isMutable) result += ", mutable";
        if (isAsync) result += ", async";
        if (isDeprecated) result += ", deprecated";
        
        // Add passing mode
        switch (passingMode) {
            case PassingMode::ByReference: result += ", by-ref"; break;
            case PassingMode::ByPointer: result += ", by-ptr"; break;
            case PassingMode::ByMove: result += ", by-move"; break;
            case PassingMode::InOut: result += ", inout"; break;
            case PassingMode::Out: result += ", out"; break;
            default: break;
        }
        
        // Add parameter kind
        switch (parameterKind) {
            case ParameterKind::Optional: result += ", optional-param"; break;
            case ParameterKind::Named: result += ", named-param"; break;
            case ParameterKind::KeywordOnly: result += ", keyword-only"; break;
            case ParameterKind::VariadicPositional: result += ", *args"; break;
            case ParameterKind::VariadicKeyword: result += ", **kwargs"; break;
            case ParameterKind::Template: result += ", template"; break;
            case ParameterKind::Generic: result += ", generic"; break;
            default: break;
        }
        
        if (!documentation.empty()) result += ", doc: \"" + documentation + "\"";
        if (parameterIndex >= 0) result += ", index: " + std::to_string(parameterIndex);
        
        result += ")";
        return result;
    }
    
    std::shared_ptr<Expression> clone() const override {
        auto input = std::make_shared<FunctionInputExpression>(
            name,
            type ? type->clone() : nullptr,
            defaultValue ? defaultValue->clone() : nullptr,
            isConstant,
            passingMode
        );
        
        // Copy all flags and properties
        input->isVariadic = isVariadic;
        input->isReference = isReference;
        input->isPointer = isPointer;
        input->isMutable = isMutable;
        input->isVolatile = isVolatile;
        input->isStatic = isStatic;
        input->isInline = isInline;
        input->isVirtual = isVirtual;
        input->isOverride = isOverride;
        input->isFinal = isFinal;
        input->isExplicit = isExplicit;
        input->isConstexpr = isConstexpr;
        input->isConsteval = isConsteval;
        input->isConstinit = isConstinit;
        input->isNoexcept = isNoexcept;
        input->isDeleted = isDeleted;
        input->isDefaulted = isDefaulted;
        input->accessModifier = accessModifier;
        input->direction = direction;
        input->parameterKind = parameterKind;
        input->memoryModel = memoryModel;
        input->nullability = nullability;
        input->evaluationStrategy = evaluationStrategy;
        input->isOptional = isOptional;
        input->hasDefaultValue = hasDefaultValue;
        input->isNamed = isNamed;
        input->isPositional = isPositional;
        input->isKeywordOnly = isKeywordOnly;
        input->isRequired = isRequired;
        input->isImplicit = isImplicit;
        input->isExported = isExported;
        input->isInternal = isInternal;
        input->isUnsafe = isUnsafe;
        input->isAsync = isAsync;
        input->isGenerator = isGenerator;
        input->isCoroutine = isCoroutine;
        input->isParallel = isParallel;
        input->isSynchronized = isSynchronized;
        input->isTransient = isTransient;
        input->isSerializable = isSerializable;
        input->isDeprecated = isDeprecated;
        input->isObsolete = isObsolete;
        input->isExperimental = isExperimental;
        input->isBeta = isBeta;
        input->isAlpha = isAlpha;
        input->isHidden = isHidden;
        input->isReadOnly = isReadOnly;
        input->isWriteOnly = isWriteOnly;
        input->isLazy = isLazy;
        input->isComputed = isComputed;
        input->isCached = isCached;
        input->isWeak = isWeak;
        input->isUnowned = isUnowned;
        input->isEscaping = isEscaping;
        input->isNonEscaping = isNonEscaping;
        input->isAutoclosure = isAutoclosure;
        input->isRethrows = isRethrows;
        input->isDiscardableResult = isDiscardableResult;
        input->isWarnUnusedResult = isWarnUnusedResult;
        input->isUnavailable = isUnavailable;
        input->isAvailable = isAvailable;
        input->isThreadSafe = isThreadSafe;
        input->isAtomic = isAtomic;
        input->isVolatileAccess = isVolatileAccess;
        input->isRestricted = isRestricted;
        input->isSealed = isSealed;
        input->isAbstract = isAbstract;
        input->isConcrete = isConcrete;
        input->isGeneric = isGeneric;
        input->isParameterized = isParameterized;
        input->isSpecialized = isSpecialized;
        input->isInstantiated = isInstantiated;
        input->isReified = isReified;
        input->isErased = isErased;
        input->isBounded = isBounded;
        input->isUnbounded = isUnbounded;
        input->isCovariant = isCovariant;
        input->isContravariant = isContravariant;
        input->isInvariant = isInvariant;
        input->isBivariant = isBivariant;
        input->parameterIndex = parameterIndex;
        input->priority = priority;
        input->alignment = alignment;
        input->sizeInBytes = sizeInBytes;
        input->precision = precision;
        input->scale = scale;
        input->ownership = ownership;
        input->documentation = documentation;
        input->aliasName = aliasName;
        input->internalName = internalName;
        input->externalName = externalName;
        input->parameterLabel = parameterLabel;
        input->callingConvention = callingConvention;
        input->linkage = linkage;
        input->storageClass = storageClass;
        input->minVersion = minVersion;
        input->maxVersion = maxVersion;
        input->introducedVersion = introducedVersion;
        input->deprecatedVersion = deprecatedVersion;
        input->removedVersion = removedVersion;
        input->supportedPlatforms = supportedPlatforms;
        input->requiredFeatures = requiredFeatures;
        input->canThrow = canThrow;
        input->canFail = canFail;
        input->lifetimeName = lifetimeName;
        
        // Clone complex objects
        if (initializer) input->initializer = initializer->clone();
        if (minValue) input->minValue = minValue->clone();
        if (maxValue) input->maxValue = maxValue->clone();
        if (arraySize) input->arraySize = arraySize->clone();
        if (errorHandler) input->errorHandler = errorHandler->clone();
        if (lifetimeConstraint) input->lifetimeConstraint = lifetimeConstraint->clone();
        
        // Clone vectors
        for (auto& constraint : constraints) {
            if (constraint) input->constraints.push_back(constraint->clone());
        }
        for (auto& attr : attributes) {
            if (attr) input->attributes.push_back(attr->clone());
        }
        for (auto& annotation : annotations) {
            if (annotation) input->annotations.push_back(annotation->clone());
        }
        for (auto& decorator : decorators) {
            if (decorator) input->decorators.push_back(decorator->clone());
        }
        for (auto& genericConstraint : genericConstraints) {
            if (genericConstraint) input->genericConstraints.push_back(genericConstraint->clone());
        }
        for (auto& typeParam : typeParameters) {
            if (typeParam) input->typeParameters.push_back(typeParam->clone());
        }
        for (auto& dim : dimensions) {
            if (dim) input->dimensions.push_back(dim->clone());
        }
        for (auto& throwsType : throwsTypes) {
            if (throwsType) input->throwsTypes.push_back(throwsType->clone());
        }
        
        return input;
    }
};
}