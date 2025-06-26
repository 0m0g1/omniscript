#pragma once
#include <omniscript/Expression.h>

namespace Omniscript {
struct VariableAccessExpression : public Expression {
    // Basic variable access properties
    std::string variableName;
    std::shared_ptr<Expression> value;
    
    // Optional/nullable type handling
    bool nullCaseHandled = false;
    bool extractValue = true;
    bool isNullableAccess = false;
    bool requiresNullCheck = false;
    
    // Memory access attributes
    bool isVolatileAccess = false;
    bool isAtomicAccess = false;
    bool isRestrictAccess = false;
    
    // Atomic memory ordering for reads
    enum class AtomicOrdering {
        NotAtomic,
        Unordered,
        Monotonic,
        Acquire,
        Release,
        AcquireRelease,
        SequentiallyConsistent
    } atomicOrdering = AtomicOrdering::NotAtomic;
    
    // Synchronization scope
    enum class SyncScope {
        System,
        SingleThread
    } syncScope = SyncScope::System;
    
    // Access type and behavior
    enum class AccessType {
        DirectAccess,      // Direct variable access
        PointerDereference, // Accessing through pointer
        ReferenceAccess,   // Accessing through reference
        ArrayElement,      // Accessing array element
        StructMember,      // Accessing struct/class member
        UnionMember,       // Accessing union member
        GlobalAccess,      // Accessing global variable
        ThreadLocalAccess  // Accessing thread-local variable
    } accessType = AccessType::DirectAccess;
    
    // Memory alignment hints
    unsigned expectedAlignment = 0;
    bool assumeAligned = false;
    
    // Bounds checking
    bool boundsCheckEnabled = true;
    bool assumeInBounds = false;
    
    // Optimization hints
    bool isLikelyRead = true;
    bool isUnlikelyRead = false;
    bool isHotPath = false;
    bool isColdPath = false;
    bool isPrefetchHint = false;
    
    // Thread safety
    bool isThreadSafe = false;
    bool requiresLocking = false;
    
    // Debugging and profiling
    bool enableDebugInfo = true;
    bool enableProfiling = false;
    std::string debugName = "";
    
    // Cache behavior hints
    enum class CacheHint {
        None,
        Temporal,        // Expected to be reused soon
        NonTemporal,     // Not expected to be reused
        Streaming,       // Sequential access pattern
        WriteThrough,    // Bypass cache on write
        WriteBack        // Use cache on write
    } cacheHint = CacheHint::None;
    
    // Address space (for targets with multiple address spaces)
    unsigned addressSpace = 0;
    
    // Const correctness
    bool isConstAccess = false;
    bool isMutableAccess = false;
    
    // Exception handling
    bool canThrowException = false;
    bool isNoexceptAccess = true;

    // Constructor
    VariableAccessExpression(
        const std::string& name, 
        std::shared_ptr<Expression> value,
        bool isVolatile = false,
        bool isAtomic = false
    ) : variableName(std::move(name)), 
        accessType(accessType),
        isVolatileAccess(isVolatile),
        isAtomicAccess(isAtomic) {
        
        this->type = type ? type : this->type;
        
        // Set atomic ordering if atomic
        if (isAtomic && atomicOrdering == AtomicOrdering::NotAtomic) {
            atomicOrdering = AtomicOrdering::Acquire; // Default for reads
        }
    }

    VariableAccessExpression(
        std::string name, 
        std::shared_ptr<Type> type = nullptr,
        AccessType accessType = AccessType::DirectAccess,
        bool isVolatile = false,
        bool isAtomic = false
    ) : variableName(std::move(name)), 
        accessType(accessType),
        isVolatileAccess(isVolatile),
        isAtomicAccess(isAtomic) {
        
        this->type = type ? type : this->type;
        
        // Set atomic ordering if atomic
        if (isAtomic && atomicOrdering == AtomicOrdering::NotAtomic) {
            atomicOrdering = AtomicOrdering::Acquire; // Default for reads
        }
    }
    
    // Builder pattern methods for fluent API
    VariableAccessExpression& setVolatile(bool value = true) { 
        isVolatileAccess = value; 
        return *this; 
    }
    
    VariableAccessExpression& setAtomic(bool value = true) { 
        isAtomicAccess = value; 
        if (value && atomicOrdering == AtomicOrdering::NotAtomic) {
            atomicOrdering = AtomicOrdering::Acquire;
        }
        return *this; 
    }
    
    VariableAccessExpression& setRestrict(bool value = true) { 
        isRestrictAccess = value; 
        return *this; 
    }
    
    VariableAccessExpression& setNullable(bool value = true) { 
        isNullableAccess = value; 
        requiresNullCheck = value;
        return *this; 
    }
    
    VariableAccessExpression& setConst(bool value = true) { 
        isConstAccess = value; 
        return *this; 
    }
    
    VariableAccessExpression& setMutable(bool value = true) { 
        isMutableAccess = value; 
        return *this; 
    }
    
    VariableAccessExpression& setAtomicOrdering(AtomicOrdering ordering) { 
        atomicOrdering = ordering; 
        if (ordering != AtomicOrdering::NotAtomic) {
            isAtomicAccess = true;
        }
        return *this; 
    }
    
    VariableAccessExpression& setSyncScope(SyncScope scope) { 
        syncScope = scope; 
        return *this; 
    }
    
    VariableAccessExpression& setAccessType(AccessType type) { 
        accessType = type; 
        return *this; 
    }
    
    VariableAccessExpression& setAlignment(unsigned align) { 
        expectedAlignment = align; 
        assumeAligned = (align > 0);
        return *this; 
    }
    
    VariableAccessExpression& setCacheHint(CacheHint hint) { 
        cacheHint = hint; 
        return *this; 
    }
    
    VariableAccessExpression& setAddressSpace(unsigned space) { 
        addressSpace = space; 
        return *this; 
    }
    
    VariableAccessExpression& setBoundsCheck(bool enabled) { 
        boundsCheckEnabled = enabled; 
        return *this; 
    }
    
    VariableAccessExpression& setThreadSafe(bool safe = true) { 
        isThreadSafe = safe; 
        return *this; 
    }
    
    VariableAccessExpression& setHotPath(bool hot = true) { 
        isHotPath = hot; 
        isColdPath = !hot;
        return *this; 
    }
    
    VariableAccessExpression& setColdPath(bool cold = true) { 
        isColdPath = cold; 
        isHotPath = !cold;
        return *this; 
    }
    
    VariableAccessExpression& setPrefetch(bool prefetch = true) { 
        isPrefetchHint = prefetch; 
        return *this; 
    }
    
    // Convenience methods for common patterns
    VariableAccessExpression& makeAtomicVolatile(AtomicOrdering ordering = AtomicOrdering::Acquire) {
        return setAtomic().setVolatile().setAtomicOrdering(ordering);
    }
    
    VariableAccessExpression& makeNullSafe() {
        return setNullable().setThreadSafe();
    }
    
    VariableAccessExpression& makeHighPerformance() {
        return setHotPath().setPrefetch().setCacheHint(CacheHint::Temporal);
    }
    
    VariableAccessExpression& makeStreamingAccess() {
        return setCacheHint(CacheHint::Streaming).setPrefetch();
    }
    
    VariableAccessExpression& makePointerAccess(bool boundsCheck = true) {
        return setAccessType(AccessType::PointerDereference).setBoundsCheck(boundsCheck);
    }
    
    VariableAccessExpression& makeArrayAccess(bool boundsCheck = true) {
        return setAccessType(AccessType::ArrayElement).setBoundsCheck(boundsCheck);
    }
    
    // Utility methods
    bool isMemoryAccess() const {
        return accessType == AccessType::PointerDereference || 
               accessType == AccessType::ArrayElement ||
               accessType == AccessType::StructMember ||
               accessType == AccessType::UnionMember;
    }
    
    bool isIndirectAccess() const {
        return accessType == AccessType::PointerDereference ||
               accessType == AccessType::ReferenceAccess;
    }
    
    bool isGlobalAccess() const {
        return accessType == AccessType::GlobalAccess ||
               accessType == AccessType::ThreadLocalAccess;
    }
    
    bool requiresSpecialHandling() const {
        return isVolatileAccess || isAtomicAccess || isNullableAccess || 
               requiresLocking || canThrowException;
    }
    
    bool isOptimizationFriendly() const {
        return !isVolatileAccess && !canThrowException && 
               !requiresLocking && assumeInBounds;
    }
    
    bool hasCustomAlignment() const {
        return expectedAlignment > 0;
    }
    
    bool needsNullCheck() const {
        return isNullableAccess && requiresNullCheck && !nullCaseHandled;
    }
    
    // Get string representations for backend-agnostic handling
    std::string getAtomicOrderingString() const {
        switch (atomicOrdering) {
            case AtomicOrdering::NotAtomic: return "not_atomic";
            case AtomicOrdering::Unordered: return "unordered";
            case AtomicOrdering::Monotonic: return "monotonic";
            case AtomicOrdering::Acquire: return "acquire";
            case AtomicOrdering::Release: return "release";
            case AtomicOrdering::AcquireRelease: return "acquire_release";
            case AtomicOrdering::SequentiallyConsistent: return "seq_cst";
        }
        return "not_atomic";
    }
    
    std::string getSyncScopeString() const {
        switch (syncScope) {
            case SyncScope::System: return "system";
            case SyncScope::SingleThread: return "single_thread";
        }
        return "system";
    }
    
    std::string getAccessTypeString() const {
        switch (accessType) {
            case AccessType::DirectAccess: return "direct";
            case AccessType::PointerDereference: return "pointer_deref";
            case AccessType::ReferenceAccess: return "reference";
            case AccessType::ArrayElement: return "array_element";
            case AccessType::StructMember: return "struct_member";
            case AccessType::UnionMember: return "union_member";
            case AccessType::GlobalAccess: return "global";
            case AccessType::ThreadLocalAccess: return "thread_local";
        }
        return "direct";
    }
    
    std::string getCacheHintString() const {
        switch (cacheHint) {
            case CacheHint::None: return "none";
            case CacheHint::Temporal: return "temporal";
            case CacheHint::NonTemporal: return "non_temporal";
            case CacheHint::Streaming: return "streaming";
            case CacheHint::WriteThrough: return "write_through";
            case CacheHint::WriteBack: return "write_back";
        }
        return "none";
    }
    
    // Original interface methods
    std::string toString() const override {
        std::string result = "Variable: ";
        
        // Add access qualifiers
        if (isConstAccess) result += "const ";
        if (isVolatileAccess) result += "volatile ";
        if (isAtomicAccess) result += "atomic ";
        if (isRestrictAccess) result += "restrict ";
        if (isMutableAccess) result += "mutable ";
        
        result += variableName;
        
        // Add access type info
        if (accessType != AccessType::DirectAccess) {
            result += " [" + getAccessTypeString() + "]";
        }
        
        // Add special attributes
        if (isNullableAccess) result += " ?";
        if (isHotPath) result += " [hot]";
        if (isColdPath) result += " [cold]";
        if (hasCustomAlignment()) {
            result += " [[align(" + std::to_string(expectedAlignment) + ")]]";
        }
        if (addressSpace != 0) {
            result += " [[address_space(" + std::to_string(addressSpace) + ")]]";
        }
        if (cacheHint != CacheHint::None) {
            result += " [[cache(" + getCacheHintString() + ")]]";
        }
        
        return result;
    }
    
    std::shared_ptr<Expression> clone() const override {
        auto cloned = std::make_shared<VariableAccessExpression>(
            variableName, 
            type ? type->clone() : nullptr,
            accessType,
            isVolatileAccess,
            isAtomicAccess
        );
        
        // Copy all attributes
        cloned->value = value;
        cloned->nullCaseHandled = nullCaseHandled;
        cloned->extractValue = extractValue;
        cloned->isNullableAccess = isNullableAccess;
        cloned->requiresNullCheck = requiresNullCheck;
        cloned->isRestrictAccess = isRestrictAccess;
        cloned->atomicOrdering = atomicOrdering;
        cloned->syncScope = syncScope;
        cloned->expectedAlignment = expectedAlignment;
        cloned->assumeAligned = assumeAligned;
        cloned->boundsCheckEnabled = boundsCheckEnabled;
        cloned->assumeInBounds = assumeInBounds;
        cloned->isLikelyRead = isLikelyRead;
        cloned->isUnlikelyRead = isUnlikelyRead;
        cloned->isHotPath = isHotPath;
        cloned->isColdPath = isColdPath;
        cloned->isPrefetchHint = isPrefetchHint;
        cloned->isThreadSafe = isThreadSafe;
        cloned->requiresLocking = requiresLocking;
        cloned->enableDebugInfo = enableDebugInfo;
        cloned->enableProfiling = enableProfiling;
        cloned->debugName = debugName;
        cloned->cacheHint = cacheHint;
        cloned->addressSpace = addressSpace;
        cloned->isConstAccess = isConstAccess;
        cloned->isMutableAccess = isMutableAccess;
        cloned->canThrowException = canThrowException;
        cloned->isNoexceptAccess = isNoexceptAccess;
        
        return cloned;
    }
};
}