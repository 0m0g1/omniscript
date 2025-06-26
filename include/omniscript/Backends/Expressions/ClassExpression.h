#pragma once
#include <omniscript/Expression.h>
#include <omniscript/Expressions/StructExpression.h>
#include <omniscript/Expressions/CallableExpression.h>
#include <omniscript/Expressions/FunctionExpression.h>

namespace Omniscript {
struct ClassExpression : 
public Callable,
public AggregateExpression {
    std::shared_ptr<StructExpression> structExpr;
    std::vector<std::shared_ptr<FunctionExpression>> constructors;
    std::shared_ptr<FunctionExpression> destructor;
    std::vector<std::shared_ptr<ClassMemberExpression>> members;

    ClassExpression(
        const std::string& name,
        std::shared_ptr<StructExpression> structExpr,
        std::vector<std::shared_ptr<FunctionExpression>> constructors = {},
        std::shared_ptr<FunctionExpression> destructor = nullptr,
        std::vector<std::shared_ptr<ClassMemberExpression>> members = {}
    )
        : Callable(name, name, {}, false),  
          structExpr(std::move(structExpr)),
          constructors(std::move(constructors)),
          destructor(std::move(destructor)),
          members(std::move(members))
    {
        type = this->structExpr->getType(); 
    }

    std::string toString() const override {
        std::string memberStr;
        for (const auto& member : members) {
            memberStr += "\n  " + member->toString();
        }

        return "Class: " + structExpr->structName +
               " [Constructors: " + std::to_string(constructors.size()) +
               ", Destructor: " + (destructor ? "yes" : "none") + "]" +
               (members.empty() ? "" : "\nMembers:" + memberStr);
    }

    std::shared_ptr<FunctionExpression> resolveConstructor(const std::vector<std::shared_ptr<Expression>>& args) const {
        for (const auto& ctor : constructors) {
            if (ctor->getParameters().size() == args.size()) {
                return ctor;
            }
        }
        return nullptr;
    }

    std::shared_ptr<Expression> clone() const override {
        auto clonedStruct = std::dynamic_pointer_cast<StructExpression>(structExpr->clone());

        std::vector<std::shared_ptr<FunctionExpression>> clonedCtors;
        for (const auto& ctor : constructors)
            clonedCtors.push_back(std::dynamic_pointer_cast<FunctionExpression>(ctor->clone()));

        auto clonedDtor = destructor ? std::dynamic_pointer_cast<FunctionExpression>(destructor->clone()) : nullptr;

        std::vector<std::shared_ptr<ClassMemberExpression>> clonedMembers;
        for (const auto& member : members)
            clonedMembers.push_back(std::dynamic_pointer_cast<ClassMemberExpression>(member->clone()));

        return std::make_shared<ClassExpression>(
            name, clonedStruct, clonedCtors, clonedDtor, clonedMembers
        );
    }

    std::shared_ptr<ClassMemberExpression> getMember(const std::string& name) {
        for (const auto& member : members) {
            if (member->getName() == name) {
                return member;
            }
        }
        console.error("Member '" + name + "' not found in class '" + this->getName() + "'.");
        return nullptr;
    }

    std::string serializeMembers() const {
        std::string result = "[\n";
        for (const auto& member : members) {
            result += "  { name: \"" + member->getName() + "\", ";
            result += "type: \"" + member->getType()->toString() + "\", ";
            result += "modifiers: \"" + member->getAccessString() + "\" },\n";
        }
        result += "]";
        return result;
    }
};
}

// namespace Omniscript {
// struct ClassExpression : public Callable, public AggregateExpression {
//     // Core class components
//     // std::shared_ptr<StructExpression> structExpr;
//     // std::vector<std::shared_ptr<FunctionExpression>> constructors;
//     // std::shared_ptr<FunctionExpression> destructor;
//     // std::shared_ptr<FunctionExpression> finalizer;
//     // std::vector<std::shared_ptr<ClassMemberExpression>> members;
    
//     // // Inheritance and polymorphism
//     // std::vector<std::shared_ptr<ClassExpression>> baseClasses;
//     // std::vector<std::shared_ptr<ClassExpression>> derivedClasses;
//     // // std::vector<std::shared_ptr<InterfaceExpression>> implementedInterfaces;
//     // // std::vector<std::shared_ptr<ProtocolExpression>> conformedProtocols;
//     // // std::vector<std::shared_ptr<TraitExpression>> implementedTraits;
//     // // std::vector<std::shared_ptr<MixinExpression>> includedMixins;
//     // // std::vector<std::shared_ptr<ExtensionExpression>> extensions;
    
//     // // Access control and modifiers
//     enum class AccessLevel {
//         Public,
//         Private,
//         Protected,
//         Internal,
//         Package,
//         Module,
//         Assembly,
//         Friend,
//         FilePrivate
//     };
//     // AccessLevel accessLevel = AccessLevel::Public;
    
//     // enum class ClassKind {
//     //     Class,
//     //     AbstractClass,
//     //     Interface,
//     //     Protocol,
//     //     Trait,
//     //     Mixin,
//     //     Record,
//     //     Struct,
//     //     Union,
//     //     Enum,
//     //     Delegate,
//     //     Component,
//     //     Service,
//     //     Controller,
//     //     Entity,
//     //     ValueObject,
//     //     DataClass,
//     //     SealedClass,
//     //     OpenClass,
//     //     FinalClass,
//     //     StaticClass,
//     //     Singleton,
//     //     Factory,
//     //     Builder,
//     //     Proxy,
//     //     Decorator,
//     //     Adapter,
//     //     Facade,
//     //     Observer,
//     //     Strategy,
//     //     Command,
//     //     State,
//     //     Template,
//     //     Generic,
//     //     Parametric,
//     //     Partial,
//     //     Anonymous,
//     //     Nested,
//     //     Inner,
//     //     Local,
//     //     Lambda,
//     //     Functional
//     // };
//     // ClassKind classKind = ClassKind::Class;
    
//     // // Class modifiers
//     // bool isAbstract = false;
//     // bool isFinal = false;
//     // bool isSealed = false;
//     // bool isOpen = false;
//     // bool isStatic = false;
//     // bool isPartial = false;
//     // bool isGeneric = false;
//     // bool isTemplate = false;
//     // bool isNested = false;
//     // bool isInner = false;
//     // bool isLocal = false;
//     // bool isAnonymous = false;
//     // bool isSingleton = false;
//     // bool isImmutable = false;
//     // bool isMutable = true;
//     // bool isThreadSafe = false;
//     // bool isSerializable = false;
//     // bool isClonable = false;
//     // bool isComparable = false;
//     // bool isEquatable = false;
//     // bool isHashable = false;
//     // bool isIterable = false;
//     // bool isDisposable = false;
//     // bool isAsyncDisposable = false;
//     // bool isObservable = false;
//     // bool isBindable = false;
//     // bool isNotifyPropertyChanged = false;
//     // bool isComponent = false;
//     // bool isService = false;
//     // bool isController = false;
//     // bool isRepository = false;
//     // bool isEntity = false;
//     // bool isValueObject = false;
//     // bool isAggregateRoot = false;
//     // bool isDomainService = false;
//     // bool isApplicationService = false;
//     // bool isInfrastructureService = false;
//     // bool isEventHandler = false;
//     // bool isCommandHandler = false;
//     // bool isQueryHandler = false;
//     // bool isValidator = false;
//     // bool isMapper = false;
//     // bool isConverter = false;
//     // bool isFactory = false;
//     // bool isBuilder = false;
//     // bool isProxy = false;
//     // bool isDecorator = false;
//     // bool isAdapter = false;
//     // bool isFacade = false;
//     // bool isBridge = false;
//     // bool isComposite = false;
//     // bool isFlyweight = false;
//     // bool isObserver = false;
//     // bool isSubject = false;
//     // bool isMediator = false;
//     // bool isChainOfResponsibility = false;
//     // bool isCommand = false;
//     // bool isInterpreter = false;
//     // bool isIterator = false;
//     // bool isMemento = false;
//     // bool isState = false;
//     // bool isStrategy = false;
//     // bool isTemplateMethod = false;
//     // bool isVisitor = false;
//     // bool isNull = false;
//     // bool isDeprecated = false;
//     // bool isObsolete = false;
//     // bool isExperimental = false;
//     // bool isBeta = false;
//     // bool isAlpha = false;
//     // bool isPreview = false;
//     // bool isLegacy = false;
//     // bool isInternal = false;
//     // bool isPublic = true;
//     // bool isExported = false;
//     // bool isImported = false;
//     // bool isNative = false;
//     // bool isManaged = false;
//     // bool isUnmanaged = false;
//     // bool isCOM = false;
//     // bool isPInvoke = false;
//     // bool isBlittable = false;
//     // bool isLayoutSequential = false;
//     // bool isLayoutExplicit = false;
//     // bool isLayoutAuto = false;
//     // bool isStructLayout = false;
//     // bool isPackingSize = false;
//     // bool isCharSet = false;
//     // bool isCallingConvention = false;
//     // bool isExactSpelling = false;
//     // bool isPreserveSig = false;
//     // bool isSetLastError = false;
//     // bool isThrowOnUnmappableChar = false;
//     // bool isBestFitMapping = false;
//     // bool isUnicode = false;
//     // bool isAnsi = false;
//     // bool isAuto = false;
//     // bool isCustom = false;
//     // bool isWinRT = false;
//     // bool isActivatable = false;
//     // bool isComposable = false;
//     // bool isExclusiveTo = false;
//     // bool isFlags = false;
//     // bool isGuid = false;
//     // bool isMarshaling = false;
//     // bool isOverloadable = false;
//     // bool isDefaultOverload = false;
//     // bool isWebHostHidden = false;
//     // bool isRemoteAsync = false;
//     // bool isCallbackContract = false;
//     // bool isServiceContract = false;
//     // bool isOperationContract = false;
//     // bool isDataContract = false;
//     // bool isDataMember = false;
//     // bool isMessageContract = false;
//     // bool isMessageHeader = false;
//     // bool isMessageBodyMember = false;
//     // bool isFaultContract = false;
//     // bool isServiceKnownType = false;
//     // bool isXmlSerializerFormat = false;
//     // bool isDataContractFormat = false;
//     // bool isXmlIgnore = false;
//     // bool isXmlElement = false;
//     // bool isXmlAttribute = false;
//     // bool isXmlText = false;
//     // bool isXmlRoot = false;
//     // bool isXmlType = false;
//     // bool isXmlInclude = false;
//     // bool isXmlArray = false;
//     // bool isXmlArrayItem = false;
//     // bool isJsonProperty = false;
//     // bool isJsonIgnore = false;
//     // bool isJsonConverter = false;
//     // bool isJsonObject = false;
//     // bool isJsonArray = false;
//     // bool isJsonExtensionData = false;
//     // bool isYamlMember = false;
//     // bool isYamlIgnore = false;
//     // bool isProtobufContract = false;
//     // bool isProtobufMember = false;
//     // bool isProtobufIgnore = false;
//     // bool isMessagePackObject = false;
//     // bool isMessagePackMember = false;
//     // bool isMessagePackIgnore = false;
//     // bool isBsonElement = false;
//     // bool isBsonIgnore = false;
//     // bool isBsonId = false;
//     // bool isBsonRepresentation = false;
//     // bool isBsonSerializer = false;
//     // bool isBsonKnownTypes = false;
//     // bool isAvroRecord = false;
//     // bool isAvroField = false;
//     // bool isAvroUnion = false;
//     // bool isAvroEnum = false;
//     // bool isAvroArray = false;
//     // bool isAvroMap = false;
//     // bool isAvroFixed = false;
//     // bool isThriftStruct = false;
//     // bool isThriftField = false;
//     // bool isThriftException = false;
//     // bool isThriftService = false;
//     // bool isThriftMethod = false;
//     // bool isCapnProtoStruct = false;
//     // bool isCapnProtoField = false;
//     // bool isCapnProtoUnion = false;
//     // bool isCapnProtoGroup = false;
//     // bool isCapnProtoInterface = false;
//     // bool isCapnProtoMethod = false;
//     // bool isFlatBuffersTable = false;
//     // bool isFlatBuffersStruct = false;
//     // bool isFlatBuffersUnion = false;
//     // bool isFlatBuffersEnum = false;
//     // bool isFlatBuffersVector = false;
//     // bool isFlatBuffersString = false;
//     // bool isFlatBuffersOffset = false;
//     // bool isGraphQLType = false;
//     // bool isGraphQLObject = false;
//     // bool isGraphQLInterface = false;
//     // bool isGraphQLUnion = false;
//     // bool isGraphQLEnum = false;
//     // bool isGraphQLScalar = false;
//     // bool isGraphQLInput = false;
//     // bool isGraphQLField = false;
//     // bool isGraphQLArgument = false;
//     // bool isGraphQLDirective = false;
//     // bool isRestResource = false;
//     // bool isRestController = false;
//     // bool isRestEndpoint = false;
//     // bool isRestPath = false;
//     // bool isRestQuery = false;
//     // bool isRestBody = false;
//     // bool isRestHeader = false;
//     // bool isRestCookie = false;
//     // bool isRestForm = false;
//     // bool isRestMultipart = false;
//     // bool isRestConsumes = false;
//     // bool isRestProduces = false;
//     // bool isRestSecurity = false;
//     // bool isRestSwagger = false;
//     // bool isRestOpenAPI = false;
//     // bool isGrpcService = false;
//     // bool isGrpcMethod = false;
//     // bool isGrpcMessage = false;
//     // bool isGrpcField = false;
//     // bool isGrpcEnum = false;
//     // bool isGrpcOneof = false;
//     // bool isGrpcMap = false;
//     // bool isGrpcRepeated = false;
//     // bool isGrpcOptional = false;
//     // bool isGrpcRequired = false;
//     // bool isGrpcStream = false;
//     // bool isGrpcUnary = false;
//     // bool isGrpcClientStreaming = false;
//     // bool isGrpcServerStreaming = false;
//     // bool isGrpcBidirectionalStreaming = false;
    
//     // // Compilation and language features
//     // bool isCompileTime = false;
//     // bool isRuntime = true;
//     // bool isReflectable = false;
//     // bool isMetaClass = false;
//     // bool isMetaObject = false;
//     // bool isMetaType = false;
//     // bool isRTTI = false;
//     // bool isDynamicType = false;
//     // bool isStaticType = true;
//     // bool isNominalType = true;
//     // bool isStructuralType = false;
//     // bool isDuckType = false;
//     // bool isProxyType = false;
//     // bool isWrapperType = false;
//     // bool isPointerType = false;
//     // bool isReferenceType = false;
//     // bool isValueType = false;
//     // bool isBoxedType = false;
//     // bool isUnboxedType = false;
//     // bool isNullableType = false;
//     // bool isNonNullType = false;
//     // bool isOptionalType = false;
//     // bool isRequiredType = false;
//     // bool isVariantType = false;
//     // bool isUnionType = false;
//     // bool isIntersectionType = false;
//     // bool isSumType = false;
//     // bool isProductType = false;
//     // bool isFunctionType = false;
//     // bool isTupleType = false;
//     // bool isRecordType = false;
//     // bool isArrayType = false;
//     // bool isListType = false;
//     // bool isSetType = false;
//     // bool isMapType = false;
//     // bool isDictionaryType = false;
//     // bool isCollectionType = false;
//     // bool isIteratorType = false;
//     // bool isEnumeratorType = false;
//     // bool isGeneratorType = false;
//     // bool isCoroutineType = false;
//     // bool isAsyncType = false;
//     // bool isPromiseType = false;
//     // bool isFutureType = false;
//     // bool isTaskType = false;
//     // bool isObservableType = false;
//     // bool isStreamType = false;
//     // bool isFlowType = false;
//     // bool isChannelType = false;
//     // bool isActorType = false;
//     // bool isAgentType = false;
//     // bool isProcessType = false;
//     // bool isThreadType = false;
//     // bool isFiberType = false;
//     // bool isGreenThreadType = false;
//     // bool isCoroutineFrameType = false;
//     // bool isAsyncFrameType = false;
//     // bool isStackFrameType = false;
//     // bool isHeapFrameType = false;
//     // bool isExecutionContextType = false;
//     // bool isCallSiteType = false;
//     // bool isClosureType = false;
//     // bool isLambdaType = false;
//     // bool isDelegateType = false;
//     // bool isFunctionPointerType = false;
//     // bool isMethodPointerType = false;
//     // bool isEventType = false;
//     // bool isPropertyType = false;
//     // bool isIndexerType = false;
//     // bool isOperatorType = false;
//     // bool isConversionType = false;
//     // bool isCastType = false;
//     // bool isCoercionType = false;
//     // bool isImplicitType = false;
//     // bool isExplicitType = false;
//     // bool isCheckedType = false;
//     // bool isUncheckedType = false;
//     // bool isSafeType = false;
//     // bool isUnsafeType = false;
//     // bool isFixedType = false;
//     // bool isVolatileType = false;
//     // bool isReadOnlyType = false;
//     // bool isWriteOnlyType = false;
//     // bool isConstType = false;
//     // bool isMutableType = false;
//     // bool isImmutableType = false;
//     // bool isPersistentType = false;
//     // bool isTransientType = false;
//     // bool isEphemeralType = false;
//     // bool isTemporaryType = false;
//     // bool isLocalType = false;
//     // bool isGlobalType = false;
//     // bool isStaticStorageType = false;
//     // bool isAutomaticStorageType = false;
//     // bool isDynamicStorageType = false;
//     // bool isThreadLocalStorageType = false;
//     // bool isRegisterStorageType = false;
//     // bool isExternStorageType = false;
//     // bool isInlineStorageType = false;
//     // bool isVirtualStorageType = false;
//     // bool isAbstractStorageType = false;
//     // bool isOverrideStorageType = false;
//     // bool isFinalStorageType = false;
//     // bool isSealedStorageType = false;
//     // bool isNewStorageType = false;
//     // bool isHideStorageType = false;
//     // bool isPartialStorageType = false;
//     // bool isAsyncStorageType = false;
//     // bool isAwaitStorageType = false;
//     // bool isYieldStorageType = false;
//     // bool isReturnStorageType = false;
//     // bool isThrowStorageType = false;
//     // bool isTryStorageType = false;
//     // bool isCatchStorageType = false;
//     // bool isFinallyStorageType = false;
//     // bool isUsingStorageType = false;
//     // bool isLockStorageType = false;
//     // bool isSynchronizedStorageType = false;
//     // bool isVolatileStorageType = false;
//     // bool isTransactionalStorageType = false;
//     // bool isAtomicStorageType = false;
//     // bool isNonAtomicStorageType = false;
//     // bool isSequentialStorageType = false;
//     // bool isRelaxedStorageType = false;
//     // bool isAcquireStorageType = false;
//     // bool isReleaseStorageType = false;
//     // bool isAcqRelStorageType = false;
//     // bool isSeqCstStorageType = false;
//     // bool isConsumeStorageType = false;
    
//     // // Generic and template parameters
//     // // std::vector<std::shared_ptr<TemplateParameterExpression>> templateParameters;
//     // // std::vector<std::shared_ptr<GenericParameterExpression>> genericParameters;
//     // // std::vector<std::shared_ptr<TypeParameterExpression>> typeParameters;
//     // // std::vector<std::shared_ptr<ConstraintExpression>> constraints;
//     // // std::vector<std::shared_ptr<ConceptExpression>> concepts;
//     // // std::vector<std::shared_ptr<RequirementsExpression>> requirements;
    
//     // // // Attributes and annotations
//     // // std::vector<std::shared_ptr<AttributeExpression>> attributes;
//     // // std::vector<std::shared_ptr<AnnotationExpression>> annotations;
//     // // std::vector<std::shared_ptr<DecoratorExpression>> decorators;
//     // // std::vector<std::shared_ptr<DirectiveExpression>> directives;
//     // // std::vector<std::shared_ptr<PragmaExpression>> pragmas;
//     // // std::vector<std::shared_ptr<MetadataExpression>> metadata;
//     // // std::vector<std::shared_ptr<DocumentationExpression>> documentation;
//     // // std::vector<std::shared_ptr<CommentExpression>> comments;
    
//     // // // Special members and methods
//     // // std::vector<std::shared_ptr<PropertyExpression>> properties;
//     // // std::vector<std::shared_ptr<IndexerExpression>> indexers;
//     // // std::vector<std::shared_ptr<EventExpression>> events;
//     // // std::vector<std::shared_ptr<OperatorExpression>> operators;
//     // // std::vector<std::shared_ptr<ConversionExpression>> conversions;
//     // // std::vector<std::shared_ptr<CastExpression>> casts;
//     // // std::vector<std::shared_ptr<StaticMemberExpression>> staticMembers;
//     // // std::vector<std::shared_ptr<ConstantExpression>> constants;
//     // // std::vector<std::shared_ptr<EnumMemberExpression>> enumMembers;
//     // // std::vector<std::shared_ptr<NestedTypeExpression>> nestedTypes;
//     // // std::vector<std::shared_ptr<FriendExpression>> friends;
//     // // std::vector<std::shared_ptr<UsingExpression>> usings;
//     // // std::vector<std::shared_ptr<TypedefExpression>> typedefs;
//     // // std::vector<std::shared_ptr<AliasExpression>> aliases;
    
//     // // // Memory management and lifecycle
//     // // std::shared_ptr<AllocatorExpression> allocator;
//     // // std::shared_ptr<DeleterExpression> deleter;
//     // // std::shared_ptr<CloneExpression> cloner;
//     // // std::shared_ptr<ComparerExpression> comparer;
//     // // std::shared_ptr<HasherExpression> hasher;
//     // // std::shared_ptr<SerializerExpression> serializer;
//     // // std::shared_ptr<DeserializerExpression> deserializer;
//     // // std::shared_ptr<ValidatorExpression> validator;
//     // // std::shared_ptr<FactoryExpression> factory;
//     // // std::shared_ptr<BuilderExpression> builder;
//     // // std::shared_ptr<ProxyExpression> proxy;
//     // // std::shared_ptr<DecoratorExpression> decorator;
//     // // std::shared_ptr<AdapterExpression> adapter;
//     // // std::shared_ptr<FacadeExpression> facade;
//     // // std::shared_ptr<BridgeExpression> bridge;
//     // // std::shared_ptr<CompositeExpression> composite;
//     // // std::shared_ptr<FlyweightExpression> flyweight;
//     // // std::shared_ptr<ObserverExpression> observer;
//     // // std::shared_ptr<SubjectExpression> subject;
//     // // std::shared_ptr<MediatorExpression> mediator;
//     // // std::shared_ptr<ChainOfResponsibilityExpression> chainOfResponsibility;
//     // // std::shared_ptr<CommandExpression> command;
//     // // std::shared_ptr<InterpreterExpression> interpreter;
//     // // std::shared_ptr<IteratorExpression> iterator;
//     // // std::shared_ptr<MementoExpression> memento;
//     // // std::shared_ptr<StateExpression> state;
//     // // std::shared_ptr<StrategyExpression> strategy;
//     // // std::shared_ptr<TemplateMethodExpression> templateMethod;
//     // // std::shared_ptr<VisitorExpression> visitor;
//     // // std::shared_ptr<NullObjectExpression> nullObject;
    
//     // // String and identification
//     // std::string fullQualifiedName;
//     // std::string namespace_;
//     // std::string module;
//     // std::string package;
//     // std::string assembly;
//     // std::string library;
//     // std::string unit;
//     // std::string file;
//     // std::string guid;
//     // std::string uuid;
//     // std::string hash;
//     // std::string checksum;
//     // std::string signature;
//     // std::string fingerprint;
//     // std::string canonicalName;
//     // std::string displayName;
//     // std::string friendlyName;
//     // std::string shortName;
//     // std::string longName;
//     // std::string internalName;
//     // std::string externalName;
//     // std::string publicName;
//     // std::string privateName;
//     // std::string protectedName;
//     // std::string qualifiedName;
//     // std::string unqualifiedName;
//     // std::string simpleName;
//     // std::string complexName;
//     // std::string mangledName;
//     // std::string demangledName;
//     // std::string decoratedName;
//     // std::string undecoratedName;
//     // std::string localizedName;
//     // std::string cultureName;
//     // std::string languageName;
//     // std::string regionName;
//     // std::string countryName;
//     // std::string variantName;
//     // std::string dialectName;
//     // std::string scriptName;
//     // std::string encodingName;
//     // std::string charsetName;
//     // std::string collationName;
//     // std::string sortingName;
//     // std::string normalizationName;
//     // std::string caseMapName;
//     // std::string breakIteratorName;
//     // std::string numberFormatName;
//     // std::string dateFormatName;
//     // std::string timeFormatName;
//     // std::string currencyFormatName;
//     // std::string percentFormatName;
//     // std::string messageFormatName;
//     // std::string calendarName;
//     // std::string timeZoneName;
//     // std::string resourceBundleName;
//     // std::string propertyName;
//     // std::string configurationName;
//     // std::string profileName;
//     // std::string environmentName;
//     // std::string platformName;
//     // std::string architectureName;
//     // std::string versionName;
//     // std::string buildName;
//     // std::string releaseName;
//     // std::string branchName;
//     // std::string tagName;
//     // std::string commitName;
//     // std::string revisionName;
//     // std::string changesetName;
//     // std::string patchName;
//     // std::string hotfixName;
//     // std::string featureName;
//     // std::string bugfixName;
//     // std::string refactoringName;
//     // std::string optimizationName;
//     // std::string securityName;
//     // std::string performanceName;
//     // std::string scalabilityName;
//     // std::string reliabilityName;
//     // std::string maintainabilityName;
//     // std::string usabilityName;
//     // std::string accessibilityName;
//     // std::string compatibilityName;
//     // std::string interoperabilityName;
//     // std::string portabilityName;
//     // std::string testabilityName;
//     // std::string debuggabilityName;
//     // std::string monitorabilityName;
//     // std::string observabilityName;
//     // std::string traceabilityName;
//     // std::string auditabilityName;
//     // std::string complianceName;
//     // std::string governanceName;
//     // std::string policyName;
//     // std::string procedureName;
//     // std::string standardName;
//     // std::string specificationName;
//     // std::string requirementName;
//     // std::string designName;
//     // std::string architectureName2;
//     // std::string implementationName;
//     // std::string deploymentName;
//     // std::string operationName;
//     // std::string maintenanceName;
//     // std::string supportName;
//     // std::string serviceName;
//     // std::string productName;
//     // std::string projectName;
//     // std::string organizationName;
//     // std::string companyName;
//     // std::string departmentName;
//     // std::string teamName;
//     // std::string authorName;
//     // std::string contributorName;
//     // std::string maintainerName;
//     // std::string ownerName;
//     // std::string sponsorName;
//     // std::string licenseName;
//     // std::string copyrightName;
//     // std::string trademarkName;
//     // std::string patentName;
//     // std::string tradeName;
//     // std::string brandName;
//     // std::string logoName;
//     // std::string iconName;
//     // std::string imageName;
//     // std::string styleName;
//     // std::string themeName;
//     // std::string skinName;
//     // std::string templateName;
//     // std::string layoutName;
//     // std::string formatName;
//     // std::string patternName;
//     // std::string expressionName;
//     // std::string regularExpressionName;
//     // std::string wildcardName;
//     // std::string globName;
//     // std::string pathName;
//     // std::string urlName;
//     // std::string uriName;
//     // std::string urnName;
//     // std::string linkName;
//     // std::string referenceName;
//     // std::string pointerName;
//     // std::string handleName;
//     // std::string idName;
//     // std::string keyName;
//     // std::string valueName;
//     // std::string pairName;
//     // std::string tupleName;
//     // std::string recordName;
//     // std::string structName;
//     // std::string className2;
//     // std::string interfaceName;
//     // std::string protocolName;
//     // std::string traitName;
//     // std::string mixinName;
//     // std::string extensionName;
//     // std::string categoryName;
//     // std::string aspectName;
//     // std::string interceptorName;
//     // std::string filterName;
//     // std::string handlerName;
//     // std::string processorName;
//     // std::string transformerName;
//     // std::string converterName;
//     // std::string mapperName;
//     // std::string reducerName;
//     // std::string aggregatorName;
//     // std::string accumulatorName;
//     // std::string collectorName;
//     // std::string generatorName;
//     // std::string producerName;
//     // std::string consumerName;
//     // std::string supplierName;
//     // std::string functionName;
//     // std::string predicateName;
//     // std::string comparatorName;
//     // std::string operatorName;
//     // std::string selectorName;
//     // std::string extractorName;
//     // std::string injectorName;
//     // std::string providerName;
//     // std::string resolverName;
//     // std::string locatorName;
//     // std::string registryName;
//     // std::string repositoryName;
//     // std::string cacheName;
//     // std::string poolName;
//     // std::string queueName;
//     // std::string stackName;
//     // std::string listName;
//     // std::string setName;
//     // std::string mapName;
//     // std::string dictionaryName;
//     // std::string tableName;
//     // std::string indexName;
//     // std::string viewName;
//     // std::string queryName;
//     // std::string commandName2;
//     // std::string eventName;
//     // std::string messageName;
//     // std::string requestName;
//     // std::string responseName;
//     // std::string headerName;
//     // std::string bodyName;
//     // std::string footerName;
//     // std::string sectionName;
//     // std::string chapterName;
//     // std::string pageName;
//     // std::string paragraphName;
//     // std::string sentenceName;
//     // std::string wordName;
//     // std::string characterName;
//     // std::string byteName;
//     // std::string bitName;
//     // std::string flagName;
//     // std::string optionName;
//     // std::string parameterName;
//     // std::string argumentName;
//     // std::string variableName;
//     // std::string constantName;
//     // std::string literalName;
//     // std::string symbolName;
//     // std::string tokenName;
//     // std::string lexemeName;
//     // std::string morphemeName;
//     // std::string phonemeName;
//     // std::string graphemeName;
//     // std::string characterSetName;
//     // std::string alphabetName;
//     // std::string vocabularyName;
//     // std::string dictionaryName2;
//     // std::string thesaurusName;
//     // std::string ontologyName;
//     // std::string taxonomyName;
//     // std::string categoryName2;
//     // std::string classificationName;
//     // std::string typologyName;
//     // std::string hierarchyName;
//     // std::string genealogyName;
//     // std::string pedigreeName;
//     // std::string lineageName;
//     // std::string ancestryName;
//     // std::string descendancyName;
//     // std::string inheritanceName;
//     // std::string derivationName;
//     // std::string originName;
//     // std::string sourceName;
//     // std::string rootName;
//     // std::string baseName;
//     // std::string foundationName;
//     // std::string coreName;
//     // std::string kernelName;
//     // std::string nucleusName;
//     // std::string center​Name;
//     // std::string middleName;
//     // std::string heartName;
//     // std::string soulName;
//     // std::string spiritName;
//     // std::string essenceName;
//     // std::string substanceName;
//     // std::string matterName;
//     // std::string materialName;
//     // std::string elementName;
//     // std::string componentName;
//     // std::string partName;
//     // std::string pieceName;
//     // std::string fragmentName;
//     // std::string segmentName;
//     // std::string sectionName2;
//     // std::string divisionName;
//     // std::string subdivisionName;
//     // std::string categoryName3;
//     // std::string groupName;
//     // std::string setName2;
//     // std::string collectionName;
//     // std::string assemblage​Name;
//     // std::string aggregationName;
//     // std::string compositionName;
//     // std::string combinationName;
//     // std::string mixtureName;
//     // std::string blendName;
//     // std::string fusionName;
//     // std::string mergeName;
//     // std::string unionName;
//     // std::string intersectionName;
//     // std::string differenceName;
//     // std::string complementName;
//     // std::string subsetName;
//     // std::string supersetName;
//     // std::string universalSetName;
//     // std::string emptySetName;
//     // std::string nullSetName;
//     // std::string voidName;
//     // std::string nothingName;
//     // std::string noneName;
//     // std::string zeroName;
//     // std::string oneName;
//     // std::string trueName;
//     // std::string falseName;
//     // std::string yesName;
//     // std::string noName;
//     // std::string okName;
//     // std::string cancelName;
//     // std::string acceptName;
//     // std::string rejectName;
//     // std::string approveName;
//     // std::string denyName;
//     // std::string allowName;
//     // std::string forbidName;
//     // std::string permitName;
//     // std::string prohibitName;
//     // std::string enableName;
//     // std::string disableName;
    
//     // // Numeric and measurement properties
//     // int nestingLevel = 0;
//     // int inheritanceDepth = 0;
//     // int complexityScore = 0;
//     // int cohesionScore = 0;
//     // int couplingScore = 0;
//     // int maintainabilityIndex = 0;
//     // int cyclomaticComplexity = 0;
//     // int linesOfCode = 0;
//     // int numberOfMethods = 0;
//     // int numberOfProperties = 0;
//     // int numberOfFields = 0;
//     // int numberOfConstructors = 0;
//     // int numberOfDestructors = 0;
//     // int numberOfOperators = 0;
//     // int numberOfEvents = 0;
//     // int numberOfIndexers = 0;
//     // int numberOfNestedTypes = 0;
//     // int numberOfBaseClasses = 0;
//     // int numberOfDerivedClasses = 0;
//     // int numberOfInterfaces = 0;
//     // int numberOfProtocols = 0;
//     // int numberOfTraits = 0;
//     // int numberOfMixins = 0;
//     // int numberOfExtensions = 0;
//     // int numberOfAttributes = 0;
//     // int numberOfAnnotations = 0;
//     // int numberOfDecorators = 0;
//     // int memoryFootprint = 0;
//     // int alignmentRequirement = 0;
//     // int packingSize = 0;
//     // size_t sizeInBytes = 0;
//     // size_t minimumSize = 0;
//     // size_t maximumSize = 0;
//     // size_t preferredSize = 0;
//     // size_t allocationSize = 0;
    
//     // // Version and compatibility
//     // std::string version;
//     // std::string minVersion;
//     // std::string maxVersion;
//     // std::string targetVersion;
//     // std::string compatibilityVersion;
//     // std::string introducedVersion;
//     // std::string deprecatedVersion;
//     // std::string removedVersion;
//     // std::string lastModifiedVersion;
//     // std::vector<std::string> supportedVersions;
//     // std::vector<std::string> supportedPlatforms;
//     // std::vector<std::string> supportedLanguages;
//     // std::vector<std::string> supportedFrameworks;
//     // std::vector<std::string> supportedLibraries;
//     // std::vector<std::string> dependencies;
//     // std::vector<std::string> optionalDependencies;
//     // std::vector<std::string> conflicts;
//     // std::vector<std::string> replacements;
//     // std::vector<std::string> alternatives;
    
//     // // Quality and metrics
//     // double testCoverage = 0.0;
//     // double codeQuality = 0.0;
//     // double performance = 0.0;
//     // double reliability = 0.0;
//     // double security = 0.0;
//     // double maintainability = 0.0;
//     // double usability = 0.0;
//     // double portability = 0.0;
//     // double efficiency = 0.0;
//     // double functionality = 0.0;
//     // double interoperability = 0.0;
//     // double scalability = 0.0;
//     // double availability = 0.0;
//     // double durability = 0.0;
//     // double consistency = 0.0;
//     // double integrity = 0.0;
//     // double confidentiality = 0.0;
//     // double authenticity = 0.0;
//     // double authorization = 0.0;
//     // double accountability = 0.0;
//     // double nonRepudiation = 0.0;
    
//     // // Timing and performance
//     // std::chrono::nanoseconds constructionTime{0};
//     // std::chrono::nanoseconds destructionTime{0};
//     // std::chrono::nanoseconds initializationTime{0};
//     // std::chrono::nanoseconds finalizationTime{0};
//     // std::chrono::nanoseconds serializationTime{0};
//     // std::chrono::nanoseconds deserializationTime{0};
//     // std::chrono::nanoseconds cloneTime{0};
//     // std::chrono::nanoseconds copyTime{0};
//     // std::chrono::nanoseconds moveTime{0};
//     // std::chrono::nanoseconds swapTime{0};
//     // std::chrono::nanoseconds compareTime{0};
//     // std::chrono::nanoseconds hashTime{0};
//     // std::chrono::nanoseconds validateTime{0};
//     // std::chrono::nanoseconds transformTime{0};
//     // std::chrono::nanoseconds processTime{0};
//     // std::chrono::nanoseconds executeTime{0};
//     // std::chrono::nanoseconds invokeTime{0};
//     // std::chrono::nanoseconds callTime{0};
//     // std::chrono::nanoseconds returnTime{0};
//     // std::chrono::nanoseconds yieldTime{0};
//     // std::chrono::nanoseconds awaitTime{0};
//     // std::chrono::nanoseconds sleepTime{0};
//     // std::chrono::nanoseconds waitTime{0};
//     // std::chrono::nanoseconds lockTime{0};
//     // std::chrono::nanoseconds unlockTime{0};
//     // std::chrono::nanoseconds syncTime{0};
//     // std::chrono::nanoseconds asyncTime{0};
    
//     // // Constructors
//     ClassExpression(
//         const std::string& name,
//         std::shared_ptr<StructExpression> structExpr,
//         std::vector<std::shared_ptr<FunctionExpression>> constructors = {},
//         std::shared_ptr<FunctionExpression> destructor = nullptr,
//         std::vector<std::shared_ptr<ClassMemberExpression>> members = {},
//         ClassKind kind = ClassKind::Class,
//         AccessLevel access = AccessLevel::Public
//     )
//     //     : Callable(name, name, {}, false),
//     //       structExpr(std::move(structExpr)),
//     //       constructors(std::move(constructors)),
//     //       destructor(std::move(destructor)),
//     //       members(std::move(members)),
//     //       classKind(kind),
//     //       accessLevel(access)
//     // {
//     //     if (this->structExpr) {
//     //         type = this->structExpr->getType();
//     //     }
//     //     fullQualifiedName = name;
//     //     numberOfMethods = static_cast<int>(this->constructors.size());
//     //     if (this->destructor) numberOfDestructors = 1;
//     //     numberOfFields = static_cast<int>(this->members.size());
//     // }
    
//     // // Factory methods for different class types
//     // static std::shared_ptr<ClassExpression> createAbstractClass(const std::string& name, std::shared_ptr<StructExpression> structExpr = nullptr) {
//     //     auto cls = std::make_shared<ClassExpression>(name, structExpr, std::vector<std::shared_ptr<FunctionExpression>>{}, nullptr, std::vector<std::shared_ptr<ClassMemberExpression>>{}, ClassKind::AbstractClass);
//     //     cls->isAbstract = true;
//     //     return cls;
//     // }
    
//     // static std::shared_ptr<ClassExpression> createInterface(const std::string& name) {
//     //     auto cls = std::make_shared<ClassExpression>(name, nullptr, std::vector<std::shared_ptr<FunctionExpression>>{}, nullptr, std::vector<std::shared_ptr<ClassMemberExpression>>{}, ClassKind::Interface);
//     //     cls->isAbstract = true;
//     //     return cls;
//     // }
    
//     // static std::shared_ptr<ClassExpression> createSealedClass(const std::string& name, std::shared_ptr<StructExpression> structExpr = nullptr) {
//     //     auto cls = std::make_shared<ClassExpression>(name, structExpr, std::vector<std::shared_ptr<FunctionExpression>>{}, nullptr, std::vector<std::shared_ptr<ClassMemberExpression>>{}, ClassKind::SealedClass);
//     //     cls->isSealed = true;
//     //     cls->isFinal = true;
//     //     return cls;
//     // }
    
//     // static std::shared_ptr<ClassExpression> createStaticClass(const std::string& name) {
//     //     auto cls = std::make_shared<ClassExpression>(name, nullptr, std::vector<std::shared_ptr<FunctionExpression>>{}, nullptr, std::vector<std::shared_ptr<ClassMemberExpression>>{}, ClassKind::StaticClass);
//     //     cls->isStatic = true;
//     //     cls->isAbstract = true;
//     //     cls->isSealed = true;
//     //     return cls;
//     // }
    
//     // static std::shared_ptr<ClassExpression> createSingleton(const std::string& name, std::shared_ptr<StructExpression> structExpr = nullptr) {
//     //     auto cls = std::make_shared<ClassExpression>(name, structExpr, std::vector<std::shared_ptr<FunctionExpression>>{}, nullptr, std::vector<std::shared_ptr<ClassMemberExpression>>{}, ClassKind::Singleton);
//     //     cls->isSingleton = true;
//     //     cls->isSealed = true;
//     //     return cls;
//     // }
    
//     // static std::shared_ptr<ClassExpression> createGenericClass(const std::string& name, const std::vector<std::shared_ptr<GenericParameterExpression>>& genericParams) {
//     //     auto cls = std::make_shared<ClassExpression>(name, nullptr, std::vector<std::shared_ptr<FunctionExpression>>{}, nullptr, std::vector<std::shared_ptr<ClassMemberExpression>>{}, ClassKind::Generic);
//     //     cls->isGeneric = true;
//     //     cls->genericParameters = genericParams;
//     //     return cls;
//     // }
    
//     // static std::shared_ptr<ClassExpression> createRecord(const std::string& name, std::shared_ptr<StructExpression> structExpr = nullptr) {
//     //     auto cls = std::make_shared<ClassExpression>(name, structExpr, std::vector<std::shared_ptr<FunctionExpression>>{}, nullptr, std::vector<std::shared_ptr<ClassMemberExpression>>{}, ClassKind::Record);
//     //     cls->isImmutable = true;
//     //     cls->isValueObject = true;
//     //     return cls;
//     // }
    
//     // static std::shared_ptr<ClassExpression> createEntity(const std::string& name, std::shared_ptr<StructExpression> structExpr = nullptr) {
//     //     auto cls = std::make_shared<ClassExpression>(name, structExpr, std::vector<std::shared_ptr<FunctionExpression>>{}, nullptr, std::vector<std::shared_ptr<ClassMemberExpression>>{}, ClassKind::Entity);
//     //     cls->isEntity = true;
//     //     cls->isSerializable = true;
//     //     return cls;
//     // }
    
//     // static std::shared_ptr<ClassExpression> createService(const std::string& name) {
//     //     auto cls = std::make_shared<ClassExpression>(name, nullptr, std::vector<std::shared_ptr<FunctionExpression>>{}, nullptr, std::vector<std::shared_ptr<ClassMemberExpression>>{}, ClassKind::Service);
//     //     cls->isService = true;
//     //     cls->isStateless = true;
//     //     return cls;
//     // }
    
//     // static std::shared_ptr<ClassExpression> createController(const std::string& name) {
//     //     auto cls = std::make_shared<ClassExpression>(name, nullptr, std::vector<std::shared_ptr<FunctionExpression>>{}, nullptr, std::vector<std::shared_ptr<ClassMemberExpression>>{}, ClassKind::Controller);
//     //     cls->isController = true;
//     //     cls->isComponent = true;
//     //     return cls;
//     // }
    
//     // // Utility methods
//     // bool isInstantiable() const {
//     //     return !isAbstract && !isInterface() && !isStatic;
//     // }
    
//     // bool isInterface() const {
//     //     return classKind == ClassKind::Interface || classKind == ClassKind::Protocol;
//     // }
    
//     // bool canInheritFrom(const std::shared_ptr<ClassExpression>& baseClass) const {
//     //     if (!baseClass) return false;
//     //     if (baseClass->isSealed || baseClass->isFinal) return false;
//     //     if (isStatic || baseClass->isStatic) return false;
//     //     return true;
//     // }
    
//     // bool canImplement(const std::shared_ptr<ClassExpression>& interface) const {
//     //     if (!interface) return false;
//     //     return interface->isInterface();
//     // }
    
//     // bool hasVirtualMethods() const {
//     //     for (const auto& member : members) {
//     //         if (member && member->isVirtual()) return true;
//     //     }
//     //     return false;
//     // }
    
//     // bool hasAbstractMethods() const {
//     //     for (const auto& member : members) {
//     //         if (member && member->isAbstract()) return true;
//     //     }
//     //     return false;
//     // }
    
//     // bool isPolymorphic() const {
//     //     return hasVirtualMethods() || !baseClasses.empty() || !implementedInterfaces.empty();
//     // }
    
//     // bool isCompleteType() const {
//     //     return !isAbstract || !hasAbstractMethods();
//     // }
    
//     // bool isValueType() const {
//     //     return classKind == ClassKind::Record || classKind == ClassKind::Struct || isValueObject;
//     // }
    
//     // bool isReferenceType() const {
//     //     return !isValueType();
//     // }
    
//     // bool isNullable() const {
//     //     return isReferenceType() && !isNonNullType;
//     // }
    
//     // bool isThreadSafeType() const {
//     //     return isThreadSafe || isImmutable || isStateless;
//     // }
    
//     // bool isSerializableType() const {
//     //     return isSerializable || isValueType();
//     // }
    
//     // bool isDisposableType() const {
//     //     return isDisposable || destructor != nullptr;
//     // }
    
//     // bool isObservableType() const {
//     //     return isObservable || isBindable || isNotifyPropertyChanged;
//     // }
    
//     // bool isComparableType() const {
//     //     return isComparable || isEquatable;
//     // }
    
//     // bool isHashableType() const {
//     //     return isHashable || isEquatable;
//     // }
    
//     // bool isIterableType() const {
//     //     return isIterable || isCollection();
//     // }
    
//     // bool isCollection() const {
//     //     return isCollectionType || isArrayType || isListType || isSetType || isMapType;
//     // }
    
//     // bool isContainer() const {
//     //     return isCollection() || isDictionaryType;
//     // }
    
//     // bool isStateless() const {
//     //     return members.empty() || std::all_of(members.begin(), members.end(), 
//     //         [](const auto& member) { return member && member->isStatic(); });
//     // }
    
//     // // Validation methods
//     // bool isValidClassDefinition() const {
//     //     if (name.empty()) return false;
//     //     if (isAbstract && isFinal) return false;
//     //     if (isStatic && (!constructors.empty() || !members.empty())) return false;
//     //     if (isInterface() && (!constructors.empty() || destructor)) return false;
        
//     //     // Check for circular inheritance
//     //     if (hasCircularInheritance()) return false;
        
//     //     // Check for abstract methods in non-abstract class
//     //     if (!isAbstract && hasAbstractMethods()) return false;
        
//     //     return true;
//     // }
    
//     // bool hasCircularInheritance() const {
//     //     std::set<std::string> visited;
//     //     return hasCircularInheritanceHelper(visited);
//     // }
    
//     // bool hasCircularInheritanceHelper(std::set<std::string>& visited) const {
//     //     if (visited.find(name) != visited.end()) return true;
        
//     //     visited.insert(name);
//     //     for (const auto& baseClass : baseClasses) {
//     //         if (baseClass && baseClass->hasCircularInheritanceHelper(visited)) {
//     //             return true;
//     //         }
//     //     }
//     //     visited.erase(name);
//     //     return false;
//     // }
    
//     // // Member access methods
//     // std::shared_ptr<ClassMemberExpression> getMember(const std::string& memberName) {
//     //     for (const auto& member : members) {
//     //         if (member && member->getName() == memberName) {
//     //             return member;
//     //         }
//     //     }
        
//     //     // Search in base classes
//     //     for (const auto& baseClass : baseClasses) {
//     //         if (baseClass) {
//     //             auto member = baseClass->getMember(memberName);
//     //             if (member && member->isAccessibleFrom(shared_from_this())) {
//     //                 return member;
//     //             }
//     //         }
//     //     }
        
//     //     return nullptr;
//     // }
    
//     // std::vector<std::shared_ptr<ClassMemberExpression>> getMembers(const std::string& memberName) {
//     //     std::vector<std::shared_ptr<ClassMemberExpression>> result;
        
//     //     for (const auto& member : members) {
//     //         if (member && member->getName() == memberName) {
//     //             result.push_back(member);
//     //         }
//     //     }
        
//     //     return result;
//     // }
    
//     // std::vector<std::shared_ptr<ClassMemberExpression>> getAllMembers() {
//     //     std::vector<std::shared_ptr<ClassMemberExpression>> result = members;
        
//     //     for (const auto& baseClass : baseClasses) {
//     //         if (baseClass) {
//     //             auto baseMembers = baseClass->getAllMembers();
//     //             for (const auto& member : baseMembers) {
//     //                 if (member && member->isAccessibleFrom(shared_from_this())) {
//     //                     result.push_back(member);
//     //                 }
//     //             }
//     //         }
//     //     }
        
//     //     return result;
//     // }
    
//     // std::shared_ptr<FunctionExpression> resolveConstructor(const std::vector<std::shared_ptr<Expression>>& args) const {
//     //     // Exact match first
//     //     for (const auto& ctor : constructors) {
//     //         if (ctor && ctor->getParameters().size() == args.size()) {
//     //             bool matches = true;
//     //             auto params = ctor->getParameters();
//     //             for (size_t i = 0; i < args.size(); ++i) {
//     //                 if (params[i] && args[i] && !params[i]->getType()->isCompatibleWith(args[i]->getType())) {
//     //                     matches = false;
//     //                     break;
//     //                 }
//     //             }
//     //             if (matches) return ctor;
//     //         }
//     //     }
        
//     //     // Try with implicit conversions
//     //     for (const auto& ctor : constructors) {
//     //         if (ctor && ctor->canAcceptArguments(args)) {
//     //             return ctor;
//     //         }
//     //     }
        
//     //     return nullptr;
//     // }
    
//     // std::vector<std::shared_ptr<FunctionExpression>> getConstructors() const {
//     //     return constructors;
//     // }
    
//     // std::shared_ptr<FunctionExpression> getDestructor() const {
//     //     return destructor;
//     // }
    
//     // std::shared_ptr<FunctionExpression> getFinalizer() const {
//     //     return finalizer;
//     // }
    
//     // // Inheritance methods
//     // void addBaseClass(std::shared_ptr<ClassExpression> baseClass) {
//     //     if (baseClass && canInheritFrom(baseClass)) {
//     //         baseClasses.push_back(baseClass);
//     //         baseClass->derivedClasses.push_back(shared_from_this());
//     //         inheritanceDepth = std::max(inheritanceDepth, baseClass->inheritanceDepth + 1);
//     //     }
//     // }
    
//     // void addInterface(std::shared_ptr<InterfaceExpression> interface) {
//     //     if (interface) {
//     //         implementedInterfaces.push_back(interface);
//     //     }
//     // }
    
//     // void addProtocol(std::shared_ptr<ProtocolExpression> protocol) {
//     //     if (protocol) {
//     //         conformedProtocols.push_back(protocol);
//     //     }
//     // }
    
//     // void addTrait(std::shared_ptr<TraitExpression> trait) {
//     //     if (trait) {
//     //         implementedTraits.push_back(trait);
//     //     }
//     // }
    
//     // void addMixin(std::shared_ptr<MixinExpression> mixin) {
//     //     if (mixin) {
//     //         includedMixins.push_back(mixin);
//     //     }
//     // }
    
//     // bool isInstanceOf(const std::shared_ptr<ClassExpression>& classType) const {
//     //     if (!classType) return false;
//     //     if (this == classType.get()) return true;
        
//     //     for (const auto& baseClass : baseClasses) {
//     //         if (baseClass && baseClass->isInstanceOf(classType)) {
//     //             return true;
//     //         }
//     //     }
        
//     //     return false;
//     // }
    
//     // bool isAssignableFrom(const std::shared_ptr<ClassExpression>& derivedClass) const {
//     //     return derivedClass && derivedClass->isInstanceOf(shared_from_this());
//     // }
    
//     // // Pattern matching and design patterns
//     // bool implementsPattern(const std::string& patternName) const {
//     //     if (patternName == "Singleton") return isSingleton;
//     //     if (patternName == "Factory") return isFactory;
//     //     if (patternName == "Builder") return isBuilder;
//     //     if (patternName == "Observer") return isObserver;
//     //     if (patternName == "Strategy") return isStrategy;
//     //     if (patternName == "Command") return isCommand;
//     //     if (patternName == "Decorator") return isDecorator;
//     //     if (patternName == "Adapter") return isAdapter;
//     //     if (patternName == "Facade") return isFacade;
//     //     if (patternName == "Proxy") return isProxy;
//     //     if (patternName == "Composite") return isComposite;
//     //     if (patternName == "Bridge") return isBridge;
//     //     if (patternName == "Flyweight") return isFlyweight;
//     //     if (patternName == "ChainOfResponsibility") return isChainOfResponsibility;
//     //     if (patternName == "Interpreter") return isInterpreter;
//     //     if (patternName == "Iterator") return isIterator;
//     //     if (patternName == "Mediator") return isMediator;
//     //     if (patternName == "Memento") return isMemento;
//     //     if (patternName == "State") return isState;
//     //     if (patternName == "TemplateMethod") return isTemplateMethod;
//     //     if (patternName == "Visitor") return isVisitor;
//     //     if (patternName == "NullObject") return isNull;
//     //     return false;
//     // }
    
//     // std::vector<std::string> getImplementedPatterns() const {
//     //     std::vector<std::string> patterns;
//     //     if (isSingleton) patterns.push_back("Singleton");
//     //     if (isFactory) patterns.push_back("Factory");
//     //     if (isBuilder) patterns.push_back("Builder");
//     //     if (isObserver) patterns.push_back("Observer");
//     //     if (isStrategy) patterns.push_back("Strategy");
//     //     if (isCommand) patterns.push_back("Command");
//     //     if (isDecorator) patterns.push_back("Decorator");
//     //     if (isAdapter) patterns.push_back("Adapter");
//     //     if (isFacade) patterns.push_back("Facade");
//     //     if (isProxy) patterns.push_back("Proxy");
//     //     if (isComposite) patterns.push_back("Composite");
//     //     if (isBridge) patterns.push_back("Bridge");
//     //     if (isFlyweight) patterns.push_back("Flyweight");
//     //     if (isChainOfResponsibility) patterns.push_back("ChainOfResponsibility");
//     //     if (isInterpreter) patterns.push_back("Interpreter");
//     //     if (isIterator) patterns.push_back("Iterator");
//     //     if (isMediator) patterns.push_back("Mediator");
//     //     if (isMemento) patterns.push_back("Memento");
//     //     if (isState) patterns.push_back("State");
//     //     if (isTemplateMethod) patterns.push_back("TemplateMethod");
//     //     if (isVisitor) patterns.push_back("Visitor");
//     //     if (isNull) patterns.push_back("NullObject");
//     //     return patterns;
//     // }
    
//     // // String representation and serialization
//     // std::string toString() const override {
//     //     std::string result = "Class: " + (fullQualifiedName.empty() ? name : fullQualifiedName);
        
//     //     // Add class kind
//     //     switch (classKind) {
//     //         case ClassKind::AbstractClass: result += " (abstract)"; break;
//     //         case ClassKind::Interface: result += " (interface)"; break;
//     //         case ClassKind::Protocol: result += " (protocol)"; break;
//     //         case ClassKind::Trait: result += " (trait)"; break;
//     //         case ClassKind::Mixin: result += " (mixin)"; break;
//     //         case ClassKind::Record: result += " (record)"; break;
//     //         case ClassKind::Struct: result += " (struct)"; break;
//     //         case ClassKind::Enum: result += " (enum)"; break;
//     //         case ClassKind::SealedClass: result += " (sealed)"; break;
//     //         case ClassKind::StaticClass: result += " (static)"; break;
//     //         case ClassKind::Singleton: result += " (singleton)"; break;
//     //         case ClassKind::Generic: result += " (generic)"; break;
//     //         default: break;
//     //     }
        
//     //     // Add access level
//     //     switch (accessLevel) {
//     //         case AccessLevel::Private: result += " [private]"; break;
//     //         case AccessLevel::Protected: result += " [protected]"; break;
//     //         case AccessLevel::Internal: result += " [internal]"; break;
//     //         case AccessLevel::Package: result += " [package]"; break;
//     //         default: break;
//     //     }
        
//     //     // Add modifiers
//     //     if (isFinal) result += " final";
//     //     if (isSealed) result += " sealed";
//     //     if (isStatic) result += " static";
//     //     if (isPartial) result += " partial";
//     //     if (isGeneric) result += " generic";
//     //     if (isThreadSafe) result += " thread-safe";
//     //     if (isImmutable) result += " immutable";
//     //     if (isSerializable) result += " serializable";
//     //     if (isDeprecated) result += " deprecated";
        
//     //     // Add inheritance info
//     //     if (!baseClasses.empty()) {
//     //         result += "\nExtends: ";
//     //         for (size_t i = 0; i < baseClasses.size(); ++i) {
//     //             if (i > 0) result += ", ";
//     //             if (baseClasses[i]) result += baseClasses[i]->getName();
//     //         }
//     //     }
        
//     //     if (!implementedInterfaces.empty()) {
//     //         result += "\nImplements: ";
//     //         for (size_t i = 0; i < implementedInterfaces.size(); ++i) {
//     //             if (i > 0) result += ", ";
//     //             if (implementedInterfaces[i]) result += implementedInterfaces[i]->getName();
//     //         }
//     //     }
        
//     //     // Add constructor info
//     //     result += "\nConstructors: " + std::to_string(constructors.size());
//     //     if (destructor) result += ", Destructor: yes";
//     //     if (finalizer) result += ", Finalizer: yes";
        
//     //     // Add member info
//     //     if (!members.empty()) {
//     //         result += "\nMembers: " + std::to_string(members.size());
//     //         for (const auto& member : members) {
//     //             if (member) {
//     //                 result += "\n  " + member->toString();
//     //             }
//     //         }
//     //     }
        
//     //     // Add metrics
//     //     if (linesOfCode > 0) result += "\nLOC: " + std::to_string(linesOfCode);
//     //     if (cyclomaticComplexity > 0) result += ", Complexity: " + std::to_string(cyclomaticComplexity);
//     //     if (sizeInBytes > 0) result += ", Size: " + std::to_string(sizeInBytes) + " bytes";
        
//     //     return result;
//     // }
    
//     // std::string serializeMembers() const {
//     //     std::string result = "[\n";
//     //     for (const auto& member : members) {
//     //         if (member) {
//     //             result += "  { name: \"" + member->getName() + "\", ";
//     //             result += "type: \"" + (member->getType() ? member->getType()->toString() : "unknown") + "\", ";
//     //             result += "access: \"" + member->getAccessString() + "\", ";
//     //             result += "static: " + (member->isStatic() ? "true" : "false") + ", ";
//     //             result += "virtual: " + (member->isVirtual() ? "true" : "false") + ", ";
//     //             result += "abstract: " + (member->isAbstract() ? "true" : "false") + ", ";
//     //             result += "override: " + (member->isOverride() ? "true" : "false") + " },\n";
//     //         }
//     //     }
//     //     result += "]";
//     //     return result;
//     // }
    
//     // std::string toJson() const {
//     //     std::string json = "{\n";
//     //     json += "  \"name\": \"" + name + "\",\n";
//     //     json += "  \"fullQualifiedName\": \"" + fullQualifiedName + "\",\n";
//     //     json += "  \"classKind\": \"" + std::to_string(static_cast<int>(classKind)) + "\",\n";
//     //     json += "  \"accessLevel\": \"" + std::to_string(static_cast<int>(accessLevel)) + "\",\n";
//     //     json += "  \"isAbstract\": " + (isAbstract ? "true" : "false") + ",\n";
//     //     json += "  \"isFinal\": " + (isFinal ? "true" : "false") + ",\n";
//     //     json += "  \"isSealed\": " + (isSealed ? "true" : "false") + ",\n";
//     //     json += "  \"isStatic\": " + (isStatic ? "true" : "false") + ",\n";
//     //     json += "  \"isGeneric\": " + (isGeneric ? "true" : "false") + ",\n";
//     //     json += "  \"isThreadSafe\": " + (isThreadSafe ? "true" : "false") + ",\n";
//     //     json += "  \"isImmutable\": " + (isImmutable ? "true" : "false") + ",\n";
//     //     json += "  \"isSerializable\": " + (isSerializable ? "true" : "false") + ",\n";
//     //     json += "  \"numberOfConstructors\": " + std::to_string(constructors.size()) + ",\n";
//     //     json += "  \"hasDestructor\": " + (destructor ? "true" : "false") + ",\n";
//     //     json += "  \"numberOfMembers\": " + std::to_string(members.size()) + ",\n";
//     //     json += "  \"numberOfBaseClasses\": " + std::to_string(baseClasses.size()) + ",\n";
//     //     json += "  \"numberOfInterfaces\": " + std::to_string(implementedInterfaces.size()) + ",\n";
//     //     json += "  \"linesOfCode\": " + std::to_string(linesOfCode) + ",\n";
//     //     json += "  \"cyclomaticComplexity\": " + std::to_string(cyclomaticComplexity) + ",\n";
//     //     json += "  \"sizeInBytes\": " + std::to_string(sizeInBytes) + ",\n";
//     //     json += "  \"isDeprecated\": " + (isDeprecated ? "true" : "false") + "\n";
//     //     json += "}";
//     //     return json;
//     // }


//     // Cloning with full deep copy
//     // std::shared_ptr<Expression> clone() const override {
//     //     auto clonedStruct = structExpr ? std::dynamic_pointer_cast<StructExpression>(structEx

//     // Deep copy method for ClassExpression
//     std::shared_ptr<Expression> clone() const override {
//         return nullptr;
//         // auto copy = std::make_shared<ClassExpression>();
        
//         // // Copy base class data (assuming Callable and AggregateExpression have copy methods)
//         // // copy->copyCallableData(*this);
//         // // copy->copyAggregateExpressionData(*this);
        
//         // // Core class components
//         // copy->structExpr = structExpr ? structExpr->clone() : nullptr;
        
//         // copy->constructors.reserve(constructors.size());
//         // for (const auto& constructor : constructors) {
//         //     copy->constructors.push_back(constructor ? constructor->clone() : nullptr);
//         // }
        
//         // copy->destructor = destructor ? destructor->clone() : nullptr;
//         // copy->finalizer = finalizer ? finalizer->clone() : nullptr;
        
//         // copy->members.reserve(members.size());
//         // for (const auto& member : members) {
//         //     copy->members.push_back(member ? member->clone() : nullptr);
//         // }
        
//         // // Inheritance and polymorphism
//         // copy->baseClasses.reserve(baseClasses.size());
//         // for (const auto& baseClass : baseClasses) {
//         //     copy->baseClasses.push_back(baseClass ? baseClass->clone() : nullptr);
//         // }
        
//         // copy->derivedClasses.reserve(derivedClasses.size());
//         // for (const auto& derivedClass : derivedClasses) {
//         //     copy->derivedClasses.push_back(derivedClass ? derivedClass->clone() : nullptr);
//         // }
        
//         // copy->implementedInterfaces.reserve(implementedInterfaces.size());
//         // for (const auto& interface : implementedInterfaces) {
//         //     copy->implementedInterfaces.push_back(interface ? interface->clone() : nullptr);
//         // }
        
//         // copy->conformedProtocols.reserve(conformedProtocols.size());
//         // for (const auto& protocol : conformedProtocols) {
//         //     copy->conformedProtocols.push_back(protocol ? protocol->clone() : nullptr);
//         // }
        
//         // copy->implementedTraits.reserve(implementedTraits.size());
//         // for (const auto& trait : implementedTraits) {
//         //     copy->implementedTraits.push_back(trait ? trait->clone() : nullptr);
//         // }
        
//         // copy->includedMixins.reserve(includedMixins.size());
//         // for (const auto& mixin : includedMixins) {
//         //     copy->includedMixins.push_back(mixin ? mixin->clone() : nullptr);
//         // }
        
//         // copy->extensions.reserve(extensions.size());
//         // for (const auto& extension : extensions) {
//         //     copy->extensions.push_back(extension ? extension->clone() : nullptr);
//         // }
        
//         // // Access control and modifiers - copy all enum and boolean values
//         // copy->accessLevel = accessLevel;
//         // copy->classKind = classKind;
        
//         // // Copy all boolean flags (there are many of these)
//         // copy->isAbstract = isAbstract;
//         // copy->isFinal = isFinal;
//         // copy->isSealed = isSealed;
//         // copy->isOpen = isOpen;
//         // copy->isStatic = isStatic;
//         // copy->isPartial = isPartial;
//         // copy->isGeneric = isGeneric;
//         // copy->isTemplate = isTemplate;
//         // copy->isNested = isNested;
//         // copy->isInner = isInner;
//         // copy->isLocal = isLocal;
//         // copy->isAnonymous = isAnonymous;
//         // copy->isSingleton = isSingleton;
//         // copy->isImmutable = isImmutable;
//         // copy->isMutable = isMutable;
//         // copy->isThreadSafe = isThreadSafe;
//         // copy->isSerializable = isSerializable;
//         // copy->isClonable = isClonable;
//         // copy->isComparable = isComparable;
//         // copy->isEquatable = isEquatable;
//         // copy->isHashable = isHashable;
//         // copy->isIterable = isIterable;
//         // copy->isDisposable = isDisposable;
//         // copy->isAsyncDisposable = isAsyncDisposable;
//         // copy->isObservable = isObservable;
//         // copy->isBindable = isBindable;
//         // copy->isNotifyPropertyChanged = isNotifyPropertyChanged;
//         // copy->isComponent = isComponent;
//         // copy->isService = isService;
//         // copy->isController = isController;
//         // copy->isRepository = isRepository;
//         // copy->isEntity = isEntity;
//         // copy->isValueObject = isValueObject;
//         // copy->isAggregateRoot = isAggregateRoot;
//         // copy->isDomainService = isDomainService;
//         // copy->isApplicationService = isApplicationService;
//         // copy->isInfrastructureService = isInfrastructureService;
//         // copy->isEventHandler = isEventHandler;
//         // copy->isCommandHandler = isCommandHandler;
//         // copy->isQueryHandler = isQueryHandler;
//         // copy->isValidator = isValidator;
//         // copy->isMapper = isMapper;
//         // copy->isConverter = isConverter;
//         // copy->isFactory = isFactory;
//         // copy->isBuilder = isBuilder;
//         // copy->isProxy = isProxy;
//         // copy->isDecorator = isDecorator;
//         // copy->isAdapter = isAdapter;
//         // copy->isFacade = isFacade;
//         // copy->isBridge = isBridge;
//         // copy->isComposite = isComposite;
//         // copy->isFlyweight = isFlyweight;
//         // copy->isObserver = isObserver;
//         // copy->isSubject = isSubject;
//         // copy->isMediator = isMediator;
//         // copy->isChainOfResponsibility = isChainOfResponsibility;
//         // copy->isCommand = isCommand;
//         // copy->isInterpreter = isInterpreter;
//         // copy->isIterator = isIterator;
//         // copy->isMemento = isMemento;
//         // copy->isState = isState;
//         // copy->isStrategy = isStrategy;
//         // copy->isTemplateMethod = isTemplateMethod;
//         // copy->isVisitor = isVisitor;
//         // copy->isNull = isNull;
//         // copy->isDeprecated = isDeprecated;
//         // copy->isObsolete = isObsolete;
//         // copy->isExperimental = isExperimental;
//         // copy->isBeta = isBeta;
//         // copy->isAlpha = isAlpha;
//         // copy->isPreview = isPreview;
//         // copy->isLegacy = isLegacy;
//         // copy->isInternal = isInternal;
//         // copy->isPublic = isPublic;
//         // copy->isExported = isExported;
//         // copy->isImported = isImported;
//         // copy->isNative = isNative;
//         // copy->isManaged = isManaged;
//         // copy->isUnmanaged = isUnmanaged;
//         // copy->isCOM = isCOM;
//         // copy->isPInvoke = isPInvoke;
//         // copy->isBlittable = isBlittable;
//         // copy->isLayoutSequential = isLayoutSequential;
//         // copy->isLayoutExplicit = isLayoutExplicit;
//         // copy->isLayoutAuto = isLayoutAuto;
//         // copy->isStructLayout = isStructLayout;
//         // copy->isPackingSize = isPackingSize;
//         // copy->isCharSet = isCharSet;
//         // copy->isCallingConvention = isCallingConvention;
//         // copy->isExactSpelling = isExactSpelling;
//         // copy->isPreserveSig = isPreserveSig;
//         // copy->isSetLastError = isSetLastError;
//         // copy->isThrowOnUnmappableChar = isThrowOnUnmappableChar;
//         // copy->isBestFitMapping = isBestFitMapping;
//         // copy->isUnicode = isUnicode;
//         // copy->isAnsi = isAnsi;
//         // copy->isAuto = isAuto;
//         // copy->isCustom = isCustom;
//         // copy->isWinRT = isWinRT;
//         // copy->isActivatable = isActivatable;
//         // copy->isComposable = isComposable;
//         // copy->isExclusiveTo = isExclusiveTo;
//         // copy->isFlags = isFlags;
//         // copy->isGuid = isGuid;
//         // copy->isMarshaling = isMarshaling;
//         // copy->isOverloadable = isOverloadable;
//         // copy->isDefaultOverload = isDefaultOverload;
//         // copy->isWebHostHidden = isWebHostHidden;
//         // copy->isRemoteAsync = isRemoteAsync;
//         // copy->isCallbackContract = isCallbackContract;
//         // copy->isServiceContract = isServiceContract;
//         // copy->isOperationContract = isOperationContract;
//         // copy->isDataContract = isDataContract;
//         // copy->isDataMember = isDataMember;
//         // copy->isMessageContract = isMessageContract;
//         // copy->isMessageHeader = isMessageHeader;
//         // copy->isMessageBodyMember = isMessageBodyMember;
//         // copy->isFaultContract = isFaultContract;
//         // copy->isServiceKnownType = isServiceKnownType;
//         // copy->isXmlSerializerFormat = isXmlSerializerFormat;
//         // copy->isDataContractFormat = isDataContractFormat;
//         // copy->isXmlIgnore = isXmlIgnore;
//         // copy->isXmlElement = isXmlElement;
//         // copy->isXmlAttribute = isXmlAttribute;
//         // copy->isXmlText = isXmlText;
//         // copy->isXmlRoot = isXmlRoot;
//         // copy->isXmlType = isXmlType;
//         // copy->isXmlInclude = isXmlInclude;
//         // copy->isXmlArray = isXmlArray;
//         // copy->isXmlArrayItem = isXmlArrayItem;
//         // copy->isJsonProperty = isJsonProperty;
//         // copy->isJsonIgnore = isJsonIgnore;
//         // copy->isJsonConverter = isJsonConverter;
//         // copy->isJsonObject = isJsonObject;
//         // copy->isJsonArray = isJsonArray;
//         // copy->isJsonExtensionData = isJsonExtensionData;
//         // copy->isYamlMember = isYamlMember;
//         // copy->isYamlIgnore = isYamlIgnore;
//         // copy->isProtobufContract = isProtobufContract;
//         // copy->isProtobufMember = isProtobufMember;
//         // copy->isProtobufIgnore = isProtobufIgnore;
//         // copy->isMessagePackObject = isMessagePackObject;
//         // copy->isMessagePackMember = isMessagePackMember;
//         // copy->isMessagePackIgnore = isMessagePackIgnore;
//         // copy->isBsonElement = isBsonElement;
//         // copy->isBsonIgnore = isBsonIgnore;
//         // copy->isBsonId = isBsonId;
//         // copy->isBsonRepresentation = isBsonRepresentation;
//         // copy->isBsonSerializer = isBsonSerializer;
//         // copy->isBsonKnownTypes = isBsonKnownTypes;
//         // copy->isAvroRecord = isAvroRecord;
//         // copy->isAvroField = isAvroField;
//         // copy->isAvroUnion = isAvroUnion;
//         // copy->isAvroEnum = isAvroEnum;
//         // copy->isAvroArray = isAvroArray;
//         // copy->isAvroMap = isAvroMap;
//         // copy->isAvroFixed = isAvroFixed;
//         // copy->isThriftStruct = isThriftStruct;
//         // copy->isThriftField = isThriftField;
//         // copy->isThriftException = isThriftException;
//         // copy->isThriftService = isThriftService;
//         // copy->isThriftMethod = isThriftMethod;
//         // copy->isCapnProtoStruct = isCapnProtoStruct;
//         // copy->isCapnProtoField = isCapnProtoField;
//         // copy->isCapnProtoUnion = isCapnProtoUnion;
//         // copy->isCapnProtoGroup = isCapnProtoGroup;
//         // copy->isCapnProtoInterface = isCapnProtoInterface;
//         // copy->isCapnProtoMethod = isCapnProtoMethod;
//         // copy->isFlatBuffersTable = isFlatBuffersTable;
//         // copy->isFlatBuffersStruct = isFlatBuffersStruct;
//         // copy->isFlatBuffersUnion = isFlatBuffersUnion;
//         // copy->isFlatBuffersEnum = isFlatBuffersEnum;
//         // copy->isFlatBuffersVector = isFlatBuffersVector;
//         // copy->isFlatBuffersString = isFlatBuffersString;
//         // copy->isFlatBuffersOffset = isFlatBuffersOffset;
//         // copy->isGraphQLType = isGraphQLType;
//         // copy->isGraphQLObject = isGraphQLObject;
//         // copy->isGraphQLInterface = isGraphQLInterface;
//         // copy->isGraphQLUnion = isGraphQLUnion;
//         // copy->isGraphQLEnum = isGraphQLEnum;
//         // copy->isGraphQLScalar = isGraphQLScalar;
//         // copy->isGraphQLInput = isGraphQLInput;
//         // copy->isGraphQLField = isGraphQLField;
//         // copy->isGraphQLArgument = isGraphQLArgument;
//         // copy->isGraphQLDirective = isGraphQLDirective;
//         // copy->isRestResource = isRestResource;
//         // copy->isRestController = isRestController;
//         // copy->isRestEndpoint = isRestEndpoint;
//         // copy->isRestPath = isRestPath;
//         // copy->isRestQuery = isRestQuery;
//         // copy->isRestBody = isRestBody;
//         // copy->isRestHeader = isRestHeader;
//         // copy->isRestCookie = isRestCookie;
//         // copy->isRestForm = isRestForm;
//         // copy->isRestMultipart = isRestMultipart;
//         // copy->isRestConsumes = isRestConsumes;
//         // copy->isRestProduces = isRestProduces;
//         // copy->isRestSecurity = isRestSecurity;
//         // copy->isRestSwagger = isRestSwagger;
//         // copy->isRestOpenAPI = isRestOpenAPI;
//         // copy->isGrpcService = isGrpcService;
//         // copy->isGrpcMethod = isGrpcMethod;
//         // copy->isGrpcMessage = isGrpcMessage;
//         // copy->isGrpcField = isGrpcField;
//         // copy->isGrpcEnum = isGrpcEnum;
//         // copy->isGrpcOneof = isGrpcOneof;
//         // copy->isGrpcMap = isGrpcMap;
//         // copy->isGrpcRepeated = isGrpcRepeated;
//         // copy->isGrpcOptional = isGrpcOptional;
//         // copy->isGrpcRequired = isGrpcRequired;
//         // copy->isGrpcStream = isGrpcStream;
//         // copy->isGrpcUnary = isGrpcUnary;
//         // copy->isGrpcClientStreaming = isGrpcClientStreaming;
//         // copy->isGrpcServerStreaming = isGrpcServerStreaming;
//         // copy->isGrpcBidirectionalStreaming = isGrpcBidirectionalStreaming;
        
//         // // Compilation and language features (continuing with all boolean flags)
//         // copy->isCompileTime = isCompileTime;
//         // copy->isRuntime = isRuntime;
//         // copy->isReflectable = isReflectable;
//         // copy->isMetaClass = isMetaClass;
//         // copy->isMetaObject = isMetaObject;
//         // copy->isMetaType = isMetaType;
//         // copy->isRTTI = isRTTI;
//         // copy->isDynamicType = isDynamicType;
//         // copy->isStaticType = isStaticType;
//         // copy->isNominalType = isNominalType;
//         // copy->isStructuralType = isStructuralType;
//         // copy->isDuckType = isDuckType;
//         // copy->isProxyType = isProxyType;
//         // copy->isWrapperType = isWrapperType;
//         // copy->isPointerType = isPointerType;
//         // copy->isReferenceType = isReferenceType;
//         // copy->isValueType = isValueType;
//         // copy->isBoxedType = isBoxedType;
//         // copy->isUnboxedType = isUnboxedType;
//         // copy->isNullableType = isNullableType;
//         // copy->isNonNullType = isNonNullType;
//         // copy->isOptionalType = isOptionalType;
//         // copy->isRequiredType = isRequiredType;
//         // copy->isVariantType = isVariantType;
//         // copy->isUnionType = isUnionType;
//         // copy->isIntersectionType = isIntersectionType;
//         // copy->isSumType = isSumType;
//         // copy->isProductType = isProductType;
//         // copy->isFunctionType = isFunctionType;
//         // copy->isTupleType = isTupleType;
//         // copy->isRecordType = isRecordType;
//         // copy->isArrayType = isArrayType;
//         // copy->isListType = isListType;
//         // copy->isSetType = isSetType;
//         // copy->isMapType = isMapType;
//         // copy->isDictionaryType = isDictionaryType;
//         // copy->isCollectionType = isCollectionType;
//         // copy->isIteratorType = isIteratorType;
//         // copy->isEnumeratorType = isEnumeratorType;
//         // copy->isGeneratorType = isGeneratorType;
//         // copy->isCoroutineType = isCoroutineType;
//         // copy->isAsyncType = isAsyncType;
//         // copy->isPromiseType = isPromiseType;
//         // copy->isFutureType = isFutureType;
//         // copy->isTaskType = isTaskType;
//         // copy->isObservableType = isObservableType;
//         // copy->isStreamType = isStreamType;
//         // copy->isFlowType = isFlowType;
//         // copy->isChannelType = isChannelType;
//         // copy->isActorType = isActorType;
//         // copy->isAgentType = isAgentType;
//         // copy->isProcessType = isProcessType;
//         // copy->isThreadType = isThreadType;
//         // copy->isFiberType = isFiberType;
//         // copy->isGreenThreadType = isGreenThreadType;
//         // copy->isCoroutineFrameType = isCoroutineFrameType;
//         // copy->isAsyncFrameType = isAsyncFrameType;
//         // copy->isStackFrameType = isStackFrameType;
//         // copy->isHeapFrameType = isHeapFrameType;
//         // copy->isExecutionContextType = isExecutionContextType;
//         // copy->isCallSiteType = isCallSiteType;
//         // copy->isClosureType = isClosureType;
//         // copy->isLambdaType = isLambdaType;
//         // copy->isDelegateType = isDelegateType;
//         // copy->isFunctionPointerType = isFunctionPointerType;
//         // copy->isMethodPointerType = isMethodPointerType;
//         // copy->isEventType = isEventType;
//         // copy->isPropertyType = isPropertyType;
//         // copy->isIndexerType = isIndexerType;
//         // copy->isOperatorType = isOperatorType;
//         // copy->isConversionType = isConversionType;
//         // copy->isCastType = isCastType;
//         // copy->isCoercionType = isCoercionType;
//         // copy->isImplicitType = isImplicitType;
//         // copy->isExplicitType = isExplicitType;
//         // copy->isCheckedType = isCheckedType;
//         // copy->isUncheckedType = isUncheckedType;
//         // copy->isSafeType = isSafeType;
//         // copy->isUnsafeType = isUnsafeType;
//         // copy->isFixedType = isFixedType;
//         // copy->isVolatileType = isVolatileType;
//         // copy->isReadOnlyType = isReadOnlyType;
//         // copy->isWriteOnlyType = isWriteOnlyType;
//         // copy->isConstType = isConstType;
//         // copy->isMutableType = isMutableType;
//         // copy->isImmutableType = isImmutableType;
//         // copy->isPersistentType = isPersistentType;
//         // copy->isTransientType = isTransientType;
//         // copy->isEphemeralType = isEphemeralType;
//         // copy->isTemporaryType = isTemporaryType;
//         // copy->isLocalType = isLocalType;
//         // copy->isGlobalType = isGlobalType;
//         // copy->isStaticStorageType = isStaticStorageType;
//         // copy->isAutomaticStorageType = isAutomaticStorageType;
//         // copy->isDynamicStorageType = isDynamicStorageType;
//         // copy->isThreadLocalStorageType = isThreadLocalStorageType;
//         // copy->isRegisterStorageType = isRegisterStorageType;
//         // copy->isExternStorageType = isExternStorageType;
//         // copy->isInlineStorageType = isInlineStorageType;
//         // copy->isVirtualStorageType = isVirtualStorageType;
//         // copy->isAbstractStorageType = isAbstractStorageType;
//         // copy->isOverrideStorageType = isOverrideStorageType;
//         // copy->isFinalStorageType = isFinalStorageType;
//         // copy->isSealedStorageType = isSealedStorageType;
//         // copy->isNewStorageType = isNewStorageType;
//         // copy->isHideStorageType = isHideStorageType;
//         // copy->isPartialStorageType = isPartialStorageType;
//         // copy->isAsyncStorageType = isAsyncStorageType;
//         // copy->isAwaitStorageType = isAwaitStorageType;
//         // copy->isYieldStorageType = isYieldStorageType;
//         // copy->isReturnStorageType = isReturnStorageType;
//         // copy->isThrowStorageType = isThrowStorageType;
//         // copy->isTryStorageType = isTryStorageType;
//         // copy->isCatchStorageType = isCatchStorageType;
//         // copy->isFinallyStorageType = isFinallyStorageType;
//         // copy->isUsingStorageType = isUsingStorageType;
//         // copy->isLockStorageType = isLockStorageType;
//         // copy->isSynchronizedStorageType = isSynchronizedStorageType;
//         // copy->isVolatileStorageType = isVolatileStorageType;
//         // copy->isTransactionalStorageType = isTransactionalStorageType;
//         // copy->isAtomicStorageType = isAtomicStorageType;
//         // copy->isNonAtomicStorageType = isNonAtomicStorageType;
//         // copy->isSequentialStorageType = isSequentialStorageType;
//         // copy->isRelaxedStorageType = isRelaxedStorageType;
//         // copy->isAcquireStorageType = isAcquireStorageType;
//         // copy->isReleaseStorageType = isReleaseStorageType;
//         // copy->isAcqRelStorageType = isAcqRelStorageType;
//         // copy->isSeqCstStorageType = isSeqCstStorageType;
//         // copy->isConsumeStorageType = isConsumeStorageType;
        
//         // Generic and template parameters
//         // copy->templateParameters.reserve(templateParameters.size());
//         // for (const auto& param : templateParameters) {
//         //     copy->templateParameters.push_back(param ? param->clone() : nullptr);
//         // }
        
//         // copy->genericParameters.reserve(genericParameters.size());
//         // for (const auto& param : genericParameters) {
//         //     copy->genericParameters.push_back(param ? param->clone() : nullptr);
//         // }
        
//         // copy->typeParameters.reserve(typeParameters.size());
//         // for (const auto& param : typeParameters) {
//         //     copy->typeParameters.push_back(param ? param->clone() : nullptr);
//         // }
        
//         // copy->constraints.reserve(constraints.size());
//         // for (const auto& constraint : constraints) {
//         //     copy->constraints.push_back(constraint ? constraint->clone() : nullptr);
//         // }
        
//         // copy->concepts.reserve(concepts.size());
//         // for (const auto& concept : concepts) {
//         //     copy->concepts.push_back(concept ? concept->clone() : nullptr);
//         // }
        
//         // copy->requirements.reserve(requirements.size());
//         // for (const auto& requirement : requirements) {
//         //     copy->requirements.push_back(requirement ? requirement->clone() : nullptr);
//         // }
        
//         // // Attributes and annotations
//         // copy->attributes.reserve(attributes.size());
//         // for (const auto& attr : attributes) {
//         //     copy->attributes.push_back(attr ? attr->clone() : nullptr);
//         // }
        
//         // copy->annotations.reserve(annotations.size());
//         // for (const auto& annotation : annotations) {
//         //     copy->annotations.push_back(annotation ? annotation->clone() : nullptr);
//         // }
        
//         // copy->decorators.reserve(decorators.size());
//         // for (const auto& decorator : decorators) {
//         //     copy->decorators.push_back(decorator ? decorator->clone() : nullptr);
//         // }
        
//         // copy->directives.reserve(directives.size());
//         // for (const auto& directive : directives) {
//         //     copy->directives.push_back(directive ? directive->clone() : nullptr);
//         // }
        
//         // copy->pragmas.reserve(pragmas.size());
//         // for (const auto& pragma : pragmas) {
//         //     copy->pragmas.push_back(pragma ? pragma->clone() : nullptr);
//         // }
        
//         // copy->metadata.reserve(metadata.size());
//         // for (const auto& meta : metadata) {
//         //     copy->metadata.push_back(meta ? meta->clone() : nullptr);
//         // }
        
//         // copy->documentation.reserve(documentation.size());
//         // for (const auto& doc : documentation) {
//         //     copy->documentation.push_back(doc ? doc->clone() : nullptr);
//         // }
        
//         // copy->comments.reserve(comments.size());
//         // for (const auto& comment : comments) {
//         //     copy->comments.push_back(comment ? comment->clone() : nullptr);
//         // }
        
//         // // Special members and methods
//         // copy->properties.reserve(properties.size());
//         // for (const auto& prop : properties) {
//         //     copy->properties.push_back(prop ? prop->clone() : nullptr);
//         // }
        
//         // copy->indexers.reserve(indexers.size());
//         // for (const auto& indexer : indexers) {
//         //     copy->indexers.push_back(indexer ? indexer->clone() : nullptr);
//         // }
        
//         // copy->events.reserve(events.size());
//         // for (const auto& event : events) {
//         //     copy->events.push_back(event ? event->clone() : nullptr);
//         // }
        
//         // copy->operators.reserve(operators.size());
//         // for (const auto& op : operators) {
//         //     copy->operators.push_back(op ? op->clone() : nullptr);
//         // }
        
//         // copy->conversions.reserve(conversions.size());
//         // for (const auto& conversion : conversions) {
//         //     copy->conversions.push_back(conversion ? conversion->clone() : nullptr);
//         // }
        
//         // copy->casts.reserve(casts.size());
//         // for (const auto& cast : casts) {
//         //     copy->casts.push_back(cast ? cast->clone() : nullptr);
//         // }
        
//         // copy->staticMembers.reserve(staticMembers.size());
//         // for (const auto& staticMember : staticMembers) {
//         //     copy->staticMembers.push_back(staticMember ? staticMember->clone() : nullptr);
//         // }
        
//         // copy->constants.reserve(constants.size());
//         // for (const auto& constant : constants) {
//         //     copy->constants.push_back(constant ? constant->clone() : nullptr);
//         // }
        
//         // copy->enumMembers.reserve(enumMembers.size());
//         // for (const auto& enumMember : enumMembers) {
//         //     copy->enumMembers.push_back(enumMember ? enumMember->clone() : nullptr);
//         // }
        
//         // copy->nestedTypes.reserve(nestedTypes.size());
//         // for (const auto& nestedType : nestedTypes) {
//         //     copy->nestedTypes.push_back(nestedType ? nestedType->clone() : nullptr);
//         // }
        
//         // copy->friends.reserve(friends.size());
//         // for (const auto& friendDecl : friends) {
//         //     copy->friends.push_back(friendDecl ? friendDecl->clone() : nullptr);
//         // }
        
//         // copy->usings.reserve(usings.size());
//         // for (const auto& using_ : usings) {
//         //     copy->usings.push_back(using_ ? using_->clone() : nullptr);
//         // }
        
//         // copy->typedefs.reserve(typedefs.size());
//         // for (const auto& typedef_ : typedefs) {
//         //     copy->typedefs.push_back(typedef_ ? typedef_->clone() : nullptr);
//         // }
        
//         // copy->aliases.reserve(aliases.size());
//         // for (const auto& alias : aliases) {
//         //     copy->aliases.push_back(alias ? alias->clone() : nullptr);
//         // }
        
//         // Memory management and lifecycle (deep copy shared_ptr members)
//         // copy->allocator = allocator ? allocator->clone() : nullptr;
//         // copy->deleter = deleter ? deleter->clone() : nullptr;
//         // copy->cloner = cloner ? cloner->clone() : nullptr;
//         // copy->comparer = comparer ? comparer->clone() : nullptr;
//         // copy->hasher = hasher ? hasher->clone() : nullptr;
//         // copy->serializer = serializer ? serializer->clone() : nullptr;
//         // copy->deserializer = deserializer ? deserializer->clone() : nullptr;
//         // copy->validator = validator ? validator->clone() : nullptr;
//         // copy->factory = factory ? factory->clone() : nullptr;
//         // copy->builder = builder ? builder->clone() : nullptr;
//         // copy->proxy = proxy ? proxy->clone() : nullptr;
//         // copy->decorator = decorator ? decorator->clone() : nullptr;
//         // copy->adapter = adapter ? adapter->clone() : nullptr;
//         // copy->facade = facade ? facade->clone() : nullptr;
//         // copy->bridge = bridge ? bridge->clone() : nullptr;
//         // copy->composite = composite ? composite->clone() : nullptr;
//         // copy->flyweight = flyweight ? flyweight->clone() : nullptr;
//         // copy->observer = observer ? observer->clone() : nullptr;
//         // copy->subject = subject ? subject->clone() : nullptr;
//         // copy->mediator = mediator ? mediator->clone() : nullptr;
//         // copy->chainOfResponsibility = chainOfResponsibility ? chainOfResponsibility->clone() : nullptr;
//         // copy->command = command ? command->clone() : nullptr;
//         // copy->interpreter = interpreter ? interpreter->clone() : nullptr;
//         // copy->iterator = iterator ? iterator->clone() : nullptr;
//         // copy->memento = memento ? memento->clone() : nullptr;
//         // copy->state = state ? state->clone() : nullptr;
//         // copy->strategy = strategy ? strategy->clone() : nullptr;
//         // copy->templateMethod = templateMethod ? templateMethod->clone() : nullptr;
//         // copy->visitor = visitor ? visitor->clone() : nullptr;
//         // copy->nullObject = nullObject ? nullObject->
//     }
// }
