#include <omniscript/Core/Expression.h>
#include <omniscript/Core/Expressions/CallableExpression.h>

namespace Omniscript {
struct FunctionExpression : public Callable {
    // Basic function properties
    std::vector<std::shared_ptr<Expression>> body;
    std::shared_ptr<Type> returnType;
    std::vector<std::shared_ptr<Type>> paramTypes;
    
    // Storage and linkage
    bool isStatic = false;
    bool isExtern = false;
    bool isIntrinsic = false;
    bool isInline = false;
    bool isConstexpr = false;
    bool isConsteval = false;
    bool isExplicit = false;
    
    // External linkage information
    std::string libPath;
    std::string externName;
    std::string intrinsicName;
    std::string section = "";
    
    // Function attributes
    bool isNoReturn = false;
    bool isNoThrow = false;
    bool isPure = false;
    bool isConst = false;
    bool isVirtual = false;
    bool isOverride = false;
    bool isFinal = false;
    bool isAbstract = false;
    bool isDeleted = false;
    bool isDefaulted = false;
    
    // Optimization attributes
    bool isAlwaysInline = false;
    bool isNoInline = false;
    bool isOptimizeForSize = false;
    bool isOptimizeNone = false;
    bool isHot = false;
    bool isCold = false;
    bool isLikelyBranch = false;
    bool isUnlikelyBranch = false;
    
    // Calling convention
    enum class CallingConvention {
        Default,
        CDecl,
        StdCall,
        FastCall,
        VectorCall,
        ThisCall,
        RegCall,
        SwiftCall,
        PreserveMost,
        PreserveAll,
        CxxFastTls,
        Tail
    } callingConvention = CallingConvention::Default;
    
    // Linkage type
    enum class Linkage {
        Internal,
        External,
        Private,
        LinkerPrivate,
        LinkOnceAny,
        LinkOnceODR,
        WeakAny,
        WeakODR,
        Common,
        ExternalWeak,
        AvailableExternally
    } linkage = Linkage::Internal;
    
    // Visibility
    enum class Visibility {
        Default,
        Hidden,
        Protected
    } visibility = Visibility::Default;
    
    // Memory attributes
    unsigned stackAlignment = 0;
    bool hasIndirectBranch = false;
    bool usesStructReturn = false;
    bool returnsNonNull = false;
    bool isNoAlias = false;
    
    // Exception handling
    bool canUnwind = true;
    bool isNoExcept = false;
    std::vector<std::string> throwsExceptions;
    
    // Threading
    bool isThreadSafe = false;
    bool isReentrant = false;
    bool isSignalHandler = false;
    
    // Debug and profiling
    bool enableDebugInfo = true;
    bool enableProfiling = false;
    std::string debugName = "";
    unsigned lineNumber = 0;
    std::string sourceFile = "";
    
    // Security attributes
    bool isSecurityCritical = false;
    bool stackProtectorRequired = false;
    bool stackProtectorStrong = false;
    bool isSafeStack = false;
    bool isCfi = false; // Control Flow Integrity
    
    // Target-specific attributes
    std::string targetFeatures = "";
    std::string cpuSpecific = "";
    unsigned addressSpace = 0;
    
    // Function complexity metrics
    unsigned cyclomaticComplexity = 0;
    unsigned estimatedInstructionCount = 0;
    bool isLeafFunction = false;
    
    // Coroutine support
    bool isCoroutine = false;
    bool isGenerator = false;
    bool isAsync = false;
    
    // Template and generic support
    bool isTemplate = false;
    bool isGeneric = false;
    bool isInstantiation = false;
    std::string templateSignature = "";
    
    // Constructor
    FunctionExpression(
        const std::string& name,
        const std::string& mangledName,
        std::shared_ptr<Type> returnType,
        std::vector<std::shared_ptr<Expression>> body = {},
        std::vector<std::shared_ptr<Expression>> params = {},
        std::vector<std::shared_ptr<Type>> paramTypes = {},
        bool isVarArg = false,
        CallingConvention convention = CallingConvention::Default,
        Linkage linkageType = Linkage::Internal
    ) : Callable(name, mangledName, std::move(params), isVarArg),
        body(std::move(body)), 
        paramTypes(paramTypes),
        returnType(returnType),
        callingConvention(convention),
        linkage(linkageType) {
        
        type = Type::createFunctionType(name, paramTypes, returnType, isVarArg);
        if (type) {
            returnType = type->getReturnType();
        }
    }
    
    // Builder pattern methods
    FunctionExpression& setStatic(bool value = true) { isStatic = value; return *this; }
    FunctionExpression& setExtern(bool value = true) { isExtern = value; return *this; }
    FunctionExpression& setIntrinsic(bool value = true) { isIntrinsic = value; return *this; }
    FunctionExpression& setInline(bool value = true) { isInline = value; return *this; }
    FunctionExpression& setConstexpr(bool value = true) { isConstexpr = value; return *this; }
    FunctionExpression& setConsteval(bool value = true) { isConsteval = value; return *this; }
    FunctionExpression& setNoReturn(bool value = true) { isNoReturn = value; return *this; }
    FunctionExpression& setNoThrow(bool value = true) { isNoThrow = value; return *this; }
    FunctionExpression& setPure(bool value = true) { isPure = value; return *this; }
    FunctionExpression& setConst(bool value = true) { isConst = value; return *this; }
    FunctionExpression& setVirtual(bool value = true) { isVirtual = value; return *this; }
    FunctionExpression& setOverride(bool value = true) { isOverride = value; return *this; }
    FunctionExpression& setFinal(bool value = true) { isFinal = value; return *this; }
    FunctionExpression& setAbstract(bool value = true) { isAbstract = value; return *this; }
    FunctionExpression& setDeleted(bool value = true) { isDeleted = value; return *this; }
    FunctionExpression& setDefaulted(bool value = true) { isDefaulted = value; return *this; }
    FunctionExpression& setAlwaysInline(bool value = true) { isAlwaysInline = value; return *this; }
    FunctionExpression& setNoInline(bool value = true) { isNoInline = value; return *this; }
    FunctionExpression& setHot(bool value = true) { isHot = value; isCold = !value; return *this; }
    FunctionExpression& setCold(bool value = true) { isCold = value; isHot = !value; return *this; }
    FunctionExpression& setThreadSafe(bool value = true) { isThreadSafe = value; return *this; }
    FunctionExpression& setReentrant(bool value = true) { isReentrant = value; return *this; }
    FunctionExpression& setNoExcept(bool value = true) { isNoExcept = value; return *this; }
    FunctionExpression& setCoroutine(bool value = true) { isCoroutine = value; return *this; }
    FunctionExpression& setAsync(bool value = true) { isAsync = value; return *this; }
    FunctionExpression& setTemplate(bool value = true) { isTemplate = value; return *this; }
    
    FunctionExpression& setCallingConvention(CallingConvention convention) { 
        callingConvention = convention; 
        return *this; 
    }
    
    FunctionExpression& setLinkage(Linkage linkageType) { 
        linkage = linkageType; 
        return *this; 
    }
    
    FunctionExpression& setVisibility(Visibility vis) { 
        visibility = vis; 
        return *this; 
    }
    
    FunctionExpression& setSection(const std::string& sec) { 
        section = sec; 
        return *this; 
    }
    
    FunctionExpression& setExternName(const std::string& name) { 
        externName = name; 
        return *this; 
    }
    
    FunctionExpression& setIntrinsicName(const std::string& name) { 
        intrinsicName = name; 
        return *this; 
    }
    
    FunctionExpression& setLibPath(const std::string& path) { 
        libPath = path; 
        return *this; 
    }
    
    FunctionExpression& setStackAlignment(unsigned align) { 
        stackAlignment = align; 
        return *this; 
    }
    
    FunctionExpression& setAddressSpace(unsigned space) { 
        addressSpace = space; 
        return *this; 
    }
    
    FunctionExpression& setTargetFeatures(const std::string& features) { 
        targetFeatures = features; 
        return *this; 
    }
    
    FunctionExpression& addThrowsException(const std::string& exception) { 
        throwsExceptions.push_back(exception); 
        return *this; 
    }
    
    // Convenience methods for common patterns
    FunctionExpression& makeExternC(const std::string& name = "") {
        return setExtern()
            .setCallingConvention(CallingConvention::CDecl)
            .setExternName(name.empty() ? this->name : name);
    }
    
    FunctionExpression& makeInlineConstexpr() {
        return setInline().setConstexpr();
    }
    
    FunctionExpression& makeVirtualOverride() {
        return setVirtual().setOverride();
    }
    
    FunctionExpression& makePureVirtual() {
        return setVirtual().setPure().setAbstract();
    }
    
    FunctionExpression& makeNoThrowPure() {
        return setNoThrow().setPure().setNoExcept();
    }
    
    FunctionExpression& makeHotInline() {
        return setHot().setAlwaysInline();
    }
    
    FunctionExpression& makeColdNoInline() {
        return setCold().setNoInline();
    }
    
    FunctionExpression& makeThreadSafeReentrant() {
        return setThreadSafe().setReentrant();
    }
    
    FunctionExpression& makeAsyncCoroutine() {
        return setAsync().setCoroutine();
    }
    
    FunctionExpression& makeSecure() {
        return setSecurityCritical(true)
            .setStackProtectorStrong(true)
            .setCfi(true);
    }
    
    // Utility methods
    bool isDeclarationOnly() const {
        return body.empty() && (isExtern || isAbstract || isDeleted);
    }
    
    bool isDefinition() const {
        return !body.empty() || isDefaulted;
    }
    
    bool isConstexprFunction() const {
        return isConstexpr || isConsteval;
    }
    
    bool isVirtualFunction() const {
        return isVirtual || isOverride || isFinal || isAbstract;
    }
    
    bool isOptimizationFriendly() const {
        return isPure && isNoThrow && !hasIndirectBranch && isLeafFunction;
    }
    
    bool isHighPerformance() const {
        return isHot || isAlwaysInline || isLeafFunction;
    }
    
    bool isExternallyVisible() const {
        return linkage == Linkage::External || 
               linkage == Linkage::WeakAny || 
               linkage == Linkage::WeakODR ||
               linkage == Linkage::Common;
    }
    
    bool requiresRuntimeSupport() const {
        return isCoroutine || canUnwind || !throwsExceptions.empty();
    }
    
    bool hasSecurityFeatures() const {
        return stackProtectorRequired || stackProtectorStrong || 
               isSafeStack || isCfi || isSecurityCritical;
    }
    
    // Get string representations for backend-agnostic handling
    std::string getCallingConventionString() const {
        switch (callingConvention) {
            case CallingConvention::Default: return "default";
            case CallingConvention::CDecl: return "cdecl";
            case CallingConvention::StdCall: return "stdcall";
            case CallingConvention::FastCall: return "fastcall";
            case CallingConvention::VectorCall: return "vectorcall";
            case CallingConvention::ThisCall: return "thiscall";
            case CallingConvention::RegCall: return "regcall";
            case CallingConvention::SwiftCall: return "swiftcall";
            case CallingConvention::PreserveMost: return "preserve_most";
            case CallingConvention::PreserveAll: return "preserve_all";
            case CallingConvention::CxxFastTls: return "cxx_fast_tls";
            case CallingConvention::Tail: return "tail";
        }
        return "default";
    }
    
    std::string getLinkageString() const {
        switch (linkage) {
            case Linkage::Internal: return "internal";
            case Linkage::External: return "external";
            case Linkage::Private: return "private";
            case Linkage::LinkerPrivate: return "linker_private";
            case Linkage::LinkOnceAny: return "linkonce_any";
            case Linkage::LinkOnceODR: return "linkonce_odr";
            case Linkage::WeakAny: return "weak_any";
            case Linkage::WeakODR: return "weak_odr";
            case Linkage::Common: return "common";
            case Linkage::ExternalWeak: return "external_weak";
            case Linkage::AvailableExternally: return "available_externally";
        }
        return "internal";
    }
    
    std::string getVisibilityString() const {
        switch (visibility) {
            case Visibility::Default: return "default";
            case Visibility::Hidden: return "hidden";
            case Visibility::Protected: return "protected";
        }
        return "default";
    }
    
    std::vector<std::string> getAllAttributes() const {
        std::vector<std::string> attrs;
        
        if (isStatic) attrs.push_back("static");
        if (isExtern) attrs.push_back("extern");
        if (isInline) attrs.push_back("inline");
        if (isConstexpr) attrs.push_back("constexpr");
        if (isConsteval) attrs.push_back("consteval");
        if (isNoReturn) attrs.push_back("noreturn");
        if (isNoThrow) attrs.push_back("nothrow");
        if (isPure) attrs.push_back("pure");
        if (isConst) attrs.push_back("const");
        if (isVirtual) attrs.push_back("virtual");
        if (isOverride) attrs.push_back("override");
        if (isFinal) attrs.push_back("final");
        if (isAbstract) attrs.push_back("abstract");
        if (isDeleted) attrs.push_back("deleted");
        if (isDefaulted) attrs.push_back("defaulted");
        if (isAlwaysInline) attrs.push_back("always_inline");
        if (isNoInline) attrs.push_back("noinline");
        if (isHot) attrs.push_back("hot");
        if (isCold) attrs.push_back("cold");
        if (isNoExcept) attrs.push_back("noexcept");
        if (isCoroutine) attrs.push_back("coroutine");
        if (isAsync) attrs.push_back("async");
        if (isTemplate) attrs.push_back("template");
        
        return attrs;
    }
    
    // Original interface methods
    std::string toString() const override {
        std::string result = "Function: ";
        
        // Add attributes
        auto attrs = getAllAttributes();
        for (const auto& attr : attrs) {
            result += attr + " ";
        }
        
        result += name;
        
        // Add calling convention if not default
        if (callingConvention != CallingConvention::Default) {
            result += " [" + getCallingConventionString() + "]";
        }
        
        // Add return type
        result += " [Returns: " + (returnType ? returnType->toString() : "void") + "]";
        
        // Add linkage if not internal
        if (linkage != Linkage::Internal) {
            result += " [" + getLinkageString() + "]";
        }
        
        // Add external name if different
        if (!externName.empty() && externName != name) {
            result += " [extern: " + externName + "]";
        }
        
        // Add intrinsic name
        if (!intrinsicName.empty()) {
            result += " [intrinsic: " + intrinsicName + "]";
        }
        
        return result;
    }
    
    std::shared_ptr<Type> getReturnType() {
        return type ? type->getReturnType() : returnType;
    }
    
    std::shared_ptr<Expression> clone() const override {
        std::vector<std::shared_ptr<Expression>> clonedBody;
        for (const auto& expr : body) {
            clonedBody.push_back(expr ? expr->clone() : nullptr);
        }
        
        std::vector<std::shared_ptr<Expression>> clonedParams;
        for (const auto& param : parameters) {
            clonedParams.push_back(param ? param->clone() : nullptr);
        }
        
        auto cloned = std::make_shared<FunctionExpression>(
            name,
            mangledName,
            returnType ? returnType->clone() : nullptr,
            clonedBody,
            clonedParams,
            paramTypes,
            isVarArg,
            callingConvention,
            linkage
        );
        
        // Copy all attributes
        cloned->isStatic = isStatic;
        cloned->isExtern = isExtern;
        cloned->isIntrinsic = isIntrinsic;
        cloned->isInline = isInline;
        cloned->isConstexpr = isConstexpr;
        cloned->isConsteval = isConsteval;
        cloned->isExplicit = isExplicit;
        cloned->libPath = libPath;
        cloned->externName = externName;
        cloned->intrinsicName = intrinsicName;
        cloned->section = section;
        cloned->isNoReturn = isNoReturn;
        cloned->isNoThrow = isNoThrow;
        cloned->isPure = isPure;
        cloned->isConst = isConst;
        cloned->isVirtual = isVirtual;
        cloned->isOverride = isOverride;
        cloned->isFinal = isFinal;
        cloned->isAbstract = isAbstract;
        cloned->isDeleted = isDeleted;
        cloned->isDefaulted = isDefaulted;
        cloned->isAlwaysInline = isAlwaysInline;
        cloned->isNoInline = isNoInline;
        cloned->isOptimizeForSize = isOptimizeForSize;
        cloned->isOptimizeNone = isOptimizeNone;
        cloned->isHot = isHot;
        cloned->isCold = isCold;
        cloned->isLikelyBranch = isLikelyBranch;
        cloned->isUnlikelyBranch = isUnlikelyBranch;
        cloned->visibility = visibility;
        cloned->stackAlignment = stackAlignment;
        cloned->hasIndirectBranch = hasIndirectBranch;
        cloned->usesStructReturn = usesStructReturn;
        cloned->returnsNonNull = returnsNonNull;
        cloned->isNoAlias = isNoAlias;
        cloned->canUnwind = canUnwind;
        cloned->isNoExcept = isNoExcept;
        cloned->throwsExceptions = throwsExceptions;
        cloned->isThreadSafe = isThreadSafe;
        cloned->isReentrant = isReentrant;
        cloned->isSignalHandler = isSignalHandler;
        cloned->enableDebugInfo = enableDebugInfo;
        cloned->enableProfiling = enableProfiling;
        cloned->debugName = debugName;
        cloned->lineNumber = lineNumber;
        cloned->sourceFile = sourceFile;
        cloned->isSecurityCritical = isSecurityCritical;
        cloned->stackProtectorRequired = stackProtectorRequired;
        cloned->stackProtectorStrong = stackProtectorStrong;
        cloned->isSafeStack = isSafeStack;
        cloned->isCfi = isCfi;
        cloned->targetFeatures = targetFeatures;
        cloned->cpuSpecific = cpuSpecific;
        cloned->addressSpace = addressSpace;
        cloned->cyclomaticComplexity = cyclomaticComplexity;
        cloned->estimatedInstructionCount = estimatedInstructionCount;
        cloned->isLeafFunction = isLeafFunction;
        cloned->isCoroutine = isCoroutine;
        cloned->isGenerator = isGenerator;
        cloned->isAsync = isAsync;
        cloned->isTemplate = isTemplate;
        cloned->isGeneric = isGeneric;
        cloned->isInstantiation = isInstantiation;
        cloned->templateSignature = templateSignature;
        
        return cloned;
    }
};
}