#ifndef IR_GENERATOR_H
#define IR_GENERATOR_H

#include <llvm/IR/Module.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/Transforms/IPO.h>
#include <llvm/Support/DynamicLibrary.h>
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/Constants.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/TargetParser/Host.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/Support/TargetSelect.h>

#include <omniscript/utils.h>
#include <omniscript/Core/Types.h>
#include <omniscript/omniscript_pch.h>
#include <omniscript/Core/Expression.h>
#include <omniscript/engine/Statement.h>
#include <omniscript/engine/Symboltable.h>
#include <omniscript/engine/EngineConfigs.h>
#include <omniscript/debuggingtools/console.h>
#include <omniscript/Core/Expressions/ClassExpression.h>
#include <omniscript/Core/Expressions/StructExpression.h>
#include <omniscript/Core/Expressions/FunctionExpression.h>
#include <omniscript/Core/Expressions/AssignmentExpression.h>
#include <omniscript/Core/Expressions/FunctionInputExpression.h>
#include <omniscript/Core/Expressions/VariableAccessExpression.h>

class ExternalFunctionResolver;

struct GlobalInit {
    llvm::GlobalVariable* variable;
    llvm::Value* value;
};

struct DynamicValue {
    enum Type { INT, FLOAT, STRING };
    Type type;
    union {
        int intValue;
        double floatValue;
    };
    std::string strValue;

    DynamicValue(int val) : type(INT), intValue(val) {}
    DynamicValue(int64_t val) : type(INT), intValue(static_cast<int>(val)) {}  
    DynamicValue(double val) : type(FLOAT), floatValue(val) {}
    DynamicValue(std::string val) : type(STRING), strValue(std::move(val)) {}
};

using IRGenSymbolTableType = std::shared_ptr<SymbolTable<llvm::Value*, llvm::Type*>>;

class IRGenerator {
private:
    Config configs;
    IRGenSymbolTableType scope = std::make_shared<SymbolTable<llvm::Value*, llvm::Type*>>();
    IRGenSymbolTableType activeScope = scope;
    std::stack<llvm::BasicBlock*> insertionPointStack;
    std::vector<GlobalInit> globalInitializers;

    std::unique_ptr<llvm::LLVMContext> Context;
    std::unique_ptr<llvm::IRBuilder<>> Builder;
    std::unique_ptr<llvm::Module> Module;
    std::unordered_map<std::string, DynamicValue*> runtimeVariables;
    std::unordered_map<std::string, std::unique_ptr<llvm::Module>> loadedModules;
    std::unordered_map<std::string, std::unique_ptr<llvm::Module>> generatedModules;

    llvm::Module* currentModule = nullptr;
    std::unordered_map<std::string, std::unordered_map<std::string, llvm::Value*>> modulePublicSymbols;
    std::vector<std::pair<llvm::GlobalVariable*, llvm::Value*>> globalInitList;

    std::unordered_map<std::string, std::unique_ptr<ExternalFunctionResolver>> resolvers;

    llvm::Value* createBigIntAVX512(const std::string& str, unsigned bitSize);
    llvm::Value* createBigIntAVX2(const std::string& str, unsigned bitSize);
    llvm::Value* createBigIntAVX(const std::string& str, unsigned bitSize);
    llvm::Value* loadMemberFromStruct(llvm::Value* structPtr, llvm::StructType* structType, const std::string& memberName);
    llvm::Value* loadMemberFromClass(llvm::Value* classPtr, llvm::StructType* classType, const std::string& memberName);
    int getStructMemberIndex(llvm::StructType* structType, const std::string& memberName);
    inline std::string formatError(const std::string& msg) const {
        return "[IRGenerator] " + msg;
    }

    std::vector<std::shared_ptr<Omniscript::FunctionExpression>> userDefinedFunctions;

public:
    
    IRGenerator(const Config& configs);

    std::unique_ptr<llvm::Module> getModule() { return std::move(Module); }
    llvm::Module* getCurrentModule() { return currentModule; }
    std::unique_ptr<llvm::LLVMContext> getContext() { return std::move(Context); }
    llvm::IRBuilder<>* getBuilder() { return Builder.get(); }
    
    
    bool supportsAVX512();
    bool supportsAVX2();

    void initialize();
    void addMainFunction();
    void finalize();

    void printIR();
    void printErrors();
    void printErrors(llvm::Module& module);
    void printAssembly(llvm::Module* module);

    std::string debugType(llvm::Type* type);
    void optimizeModule(int level = 2); 

    bool isLoadedModule(const std::string& modulePath);
    bool isLoadedModuleMember(const std::string& modulePath, const std::string& memberName);
    void activateModuleMembers(const std::vector<std::string>& members);
    void linkModules();

    inline void pushActiveBlock() {
        llvm::BasicBlock* currentBlock = Builder->GetInsertBlock();
        insertionPointStack.push(currentBlock);
    }

    inline void popActiveBlock() {
        if (insertionPointStack.empty()) {
            return;
        }
        llvm::BasicBlock* prevBlock = insertionPointStack.top();
        insertionPointStack.pop();
        if (prevBlock) {
            Builder->SetInsertPoint(prevBlock);
        }
        
    }

    inline void pushScope(const std::string& name = "") {
        activeScope = activeScope->createChildScope(name);
    }
    
    inline void popScope() {
        if (activeScope->getParent()) {
            activeScope = activeScope->getParent();
        }
    }

    inline bool fileExists(const std::string& path) {
        return !path.empty() && llvm::sys::fs::exists(llvm::Twine(path));
    }

    inline void addExternalResolver(const std::string& language, std::unique_ptr<ExternalFunctionResolver> resolver) {
        resolvers[language] = std::move(resolver);
    }

    void generateModule (
        const std::string& modulePath,
        const std::string& alias,
        const std::vector<std::shared_ptr<Statement>>& statements,
        const std::unordered_map<std::string, std::string>& importedAliases,
        bool importAll
    );

    void importModule(const std::string& moduleName, const std::vector<std::string>& members);
    bool symbolExistsInStaticLib(const std::string& libPath, const std::string& symbolName);

    llvm::Value* codegen(
        std::shared_ptr<Omniscript::Expression> value,
        SymbolTableType scope
    );
    llvm::Value* codegenPrimitive(
        std::shared_ptr<Omniscript::Expression> value,
        SymbolTableType scope);
    llvm::Type* resolveLLVMType(std::shared_ptr<Omniscript::Type> type);
    
    llvm::Value* createNullValue();
    llvm::Value* createNullPointer(llvm::Type* pointeeType);
    llvm::Value* createRawPointer(uintptr_t address, llvm::Type* pointeeType);
    
    
    llvm::Value* create8BitInteger(int8_t value);
    llvm::Value* create16BitInteger(int16_t value);
    llvm::Value* create32BitInteger(int32_t value);
    llvm::Value* create64BitInteger(int64_t value);
    llvm::Value* createUnsigned8BitInteger(uint8_t value);
    llvm::Value* createUnsigned16BitInteger(uint16_t value);
    llvm::Value* createUnsigned32BitInteger(uint32_t value);
    llvm::Value* createUnsigned64BitInteger(uint64_t value);
    
    
    #ifdef __ARM_ARCH
        llvm::Value* create16BitFloat(__fp16 value);
    #elif defined(__x86_64__) || defined(__i386__)
        llvm::Value* create16BitFloat(_Float16 value);
    #endif
        
    llvm::Value* create32BitFloat(float value);
    llvm::Value* create64BitFloat(double value);
    llvm::Value* create80BitFloat(long double value);
    llvm::Value* create128BitFloat(__float128 value);
    
    llvm::Value* createBigInt(const std::string& str, unsigned bitWidth); 

    llvm::Value* createChar(char value);
    llvm::Value* createChar16(char16_t value);
    llvm::Value* createChar32(char32_t value);
    llvm::Value* createUTF8String(const std::string& str);
    llvm::Value* createUTF16String(const std::u16string& str);
    llvm::Value* createUTF32String(const std::u32string& str);
    
    llvm::Value* createBool(bool value);
    
    llvm::Function* getOrCreateGlobalInitFunction();
    void scheduleGlobalInitialization(
        const std::string& name,
        llvm::GlobalVariable* gVar,
        llvm::Value* initialValue
    );
    void finalizeGlobalInitializers();
    llvm::Value* assignVariable(
        std::shared_ptr<Omniscript::VariableAssignment> statement,
        SymbolTableType scope
    );
    llvm::Value* createConstant(const std::string& name, llvm::Type* type, llvm::Value* value);
    llvm::Value* reassign(const std::string& name, llvm::Value* newValue);
    llvm::Value* getAddressOf(const std::string& varname);
    llvm::Value* getReferenceToVariable(const std::string& varname);
    llvm::Value* getVariable(const std::string& varname, bool extractValue = false);
    
    llvm::Value* createDynamicVariable(const std::string& name, llvm::Value* value);
    llvm::Value* createDynamicConstant(const std::string& name, llvm::Value* value);
    llvm::Value* assignDynamicVariable(const std::string& name, llvm::Value* newValue);
    llvm::Value* getDynamicVariable(const std::string& name);
    llvm::Value* generateOpaqueDynamicVariable(const std::string& name, llvm::Value* value);

    llvm::Value* createFixedArray(llvm::Type* elementType, size_t arraySize, const std::vector<llvm::Value*>& elements);
    llvm::Function* createExternFunction(std::shared_ptr<Omniscript::FunctionExpression> func, SymbolTableType scope);
    llvm::Function* createIntrinsicFunction(
        const std::string& name,
        const std::string& intrinsicName,
        llvm::Type* returnType
    );
    llvm::Function* createFunction(
        const std::string& name,
        std::vector<std::shared_ptr<Omniscript::Expression>>& body,
        llvm::Type* returnType,
        std::vector<std::shared_ptr<Omniscript::Expression>>& params,
        SymbolTableType scope,
        bool isVarArg = false
    );
    void compileAllFunctionBodies(SymbolTableType scope);
    llvm::Function* registerFunction(
        const std::string& name,
        llvm::Type* returnType,
        std::vector<std::shared_ptr<Omniscript::Expression>>& params,
        SymbolTableType scope,
        bool isVarArg = false
    );
    void generateFunctionBody(
        const std::string& name,
        llvm::Function* function,
        std::vector<std::shared_ptr<Omniscript::Expression>>& params,
        std::vector<std::shared_ptr<Omniscript::Expression>>& funcBody,
        SymbolTableType scope
    );
    llvm::Value* createCall(
        const std::string& callee, 
        std::vector<llvm::Value*>& args, 
        llvm::BasicBlock* activeBlock = nullptr
    );
    bool currentBlockHasTerminator() const;
    llvm::Value* createReturn(llvm::Value* returnValue, llvm::Type* expectedReturnType);

    bool isNullableStruct(llvm::Type* type);
    llvm::Value* createUnaryExpression(llvm::Value* operand, TokenTypes op, bool isPostfix);
    llvm::Value* createBinaryExpression(llvm::Value* left, TokenTypes op, llvm::Value* right);
    llvm::Value* createTernaryExpression(llvm::Value* cond, llvm::Value* truthy, llvm::Value* falsey);

    llvm::Value* createObjectInstance(
        const std::string& objectType,
        const std::string& instanceName,
        const std::vector<llvm::Value*>& args = {},
        bool isGlobal = true
    );
    
    llvm::Value* loadMemberValue(
        const std::string& objectName, 
        const std::string& memberName
    );

    void setMemberValue(
        llvm::Value* object, 
        const std::string& memberName, 
        llvm::Value* newValue
    );

    void createStructType(const std::string& name, const std::vector<llvm::Type*>& fieldTypes);
    llvm::Value* createStructInstance(
        const std::string& structName,
        const std::string& varName,
        const std::vector<llvm::Value*>& args,
        bool isGlobal = true
    );
    
    llvm::Value* createEnum(
        const std::vector<std::string>& names,
        const std::vector<llvm::Value*>& values,
        const std::string& enumName,
        bool isGlobal
    );

    llvm::Value* createEnumWithLookup(
        const std::vector<std::string>& names,
        const std::vector<llvm::Value*>& values,
        const std::string& enumName,
        bool isGlobal
    );

    llvm::Value* createEnumClass(
        const std::vector<std::string>& names,
        const std::vector<llvm::Value*>& values,
        const std::string& className,
        bool isGlobal
    );

    llvm::Value* createEnumClassWithLookup(
        const std::vector<std::string>& names,
        const std::vector<llvm::Value*>& values,
        const std::string& className,
        bool isGlobal
    );

    llvm::Value* getEnumValue(const std::string& enumName, const std::string& memberName);

    llvm::Value* createIfStatement(
        const std::vector<std::shared_ptr<Omniscript::Expression>>& conditions,
        const std::vector<std::shared_ptr<Omniscript::Expression>>& bodies,
        const std::shared_ptr<Omniscript::Expression>& elseBody,
        SymbolTableType scope
    );
    llvm::Value* createForLoop(
        const std::shared_ptr<Omniscript::ForLoopExpression>& forExpr,
        SymbolTableType scope
    );
    llvm::Value* createWhileLoop(
        const std::shared_ptr<Omniscript::WhileLoopExpression>& whileExpr,
        SymbolTableType scope
    );
     
    llvm::Value* handleAccessExpression(std::shared_ptr<Omniscript::AccessExpression> expr, 
                                      SymbolTableType scope);
    
    
    llvm::Value* handleMemberAccess(std::shared_ptr<Omniscript::MemberAccessExpression> expr,
                                  llvm::Value* baseValue,
                                  SymbolTableType scope,
                                  bool preservePointer = false
                                );
    
    llvm::Value* handleArrowAccess(std::shared_ptr<Omniscript::ArrowAccessExpression> expr,
                                 llvm::Value* baseValue,
                                 SymbolTableType scope);
    
    llvm::Value* handleDereference(std::shared_ptr<Omniscript::DereferenceExpression> expr,
                                 llvm::Value* baseValue,
                                 SymbolTableType scope);
    
    llvm::Value* handleIndexAccess(std::shared_ptr<Omniscript::IndexAccessExpression> expr,
                                  llvm::Value* baseValue,
                                  SymbolTableType scope);
    llvm::Value* createModuleObject(
        const std::string& moduleName,
        const std::unordered_map<std::string, llvm::Value*>& members
    );
    llvm::Value* generateCast(llvm::Value* src, llvm::Type* destType);
};

class ExternalFunctionResolver {
public:
    virtual llvm::Function* resolve(
        IRGenerator& generator,
        const std::string& name,
        llvm::FunctionType* funcType
    ) = 0;

    virtual ~ExternalFunctionResolver() = default;
};

class CStdLibResolver : public ExternalFunctionResolver {
public:
    llvm::Function* resolve(
        IRGenerator& generator,
        const std::string& name,
        llvm::FunctionType* funcType
    ) override {
        return llvm::Function::Create(
            funcType,
            llvm::Function::ExternalLinkage,
            name,
            generator.getCurrentModule()
        );
    }
};

class DynamicLibraryResolver : public ExternalFunctionResolver {
public:
    llvm::sys::DynamicLibrary dynLib;

    DynamicLibraryResolver(const std::string& libPath) {
        std::string errMsg;
        dynLib = llvm::sys::DynamicLibrary::getPermanentLibrary(libPath.c_str(), &errMsg);
        if (!dynLib.isValid()) {
            throw std::runtime_error("Failed to load library: " + errMsg);
        }
    }

    llvm::Function* resolve(
        IRGenerator& generator,
        const std::string& name,
        llvm::FunctionType* funcType
    ) override {
        void* symbol = dynLib.getAddressOfSymbol(name.c_str());
        if (!symbol) {
            return nullptr;
        }

        return llvm::Function::Create(
            funcType,
            llvm::Function::ExternalLinkage,
            name,
            generator.getCurrentModule()
        );
    }
};


class StaticLibraryResolver : public ExternalFunctionResolver {
public:
    llvm::Function* resolve(
        IRGenerator& generator,
        const std::string& name,
        llvm::FunctionType* funcType
    ) override {
        return llvm::Function::Create(
            funcType,
            llvm::Function::ExternalLinkage,
            name,
            generator.getCurrentModule()
        );
    }
};

#endif