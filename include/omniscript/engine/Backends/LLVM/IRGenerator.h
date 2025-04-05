#ifndef IR_GENERATOR_H
#define IR_GENERATOR_H

#include <llvm/IR/Module.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/Transforms/IPO.h>
#include <llvm/IR/LegacyPassManager.h>
#include <omniscript/omniscript_pch.h>
#include <omniscript/engine/Symboltable.h>
#include <omniscript/utils.h>
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/Constants.h>
#include <omniscript/Core/Value.h>

class Statement;

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
    DynamicValue(int64_t val) : type(INT), intValue(static_cast<int>(val)) {}  // New constructor
    DynamicValue(double val) : type(FLOAT), floatValue(val) {}
    DynamicValue(std::string val) : type(STRING), strValue(std::move(val)) {}
};


class IRGenerator {
private:
    std::shared_ptr<SymbolTable> scope = std::make_shared<SymbolTable>();
    std::shared_ptr<SymbolTable> activeScope = scope;
    std::stack<llvm::BasicBlock*> insertionPointStack;
    std::vector<GlobalInit> globalInitializers;

    std::unique_ptr<llvm::LLVMContext> Context;
    std::unique_ptr<llvm::IRBuilder<>> Builder;
    std::unique_ptr<llvm::Module> Module;
    std::map<std::string, llvm::Value*> NamedValues;
    std::unordered_map<std::string, DynamicValue*> runtimeVariables;
    std::unordered_map<std::string, std::unique_ptr<llvm::Module>> loadedModules;
    std::unordered_map<std::string, std::unique_ptr<llvm::Module>> generatedModules;

    llvm::Module* CurrentModule = nullptr;
    std::unordered_map<std::string, std::unordered_map<std::string, llvm::Value*>> modulePublicSymbols;
    std::vector<std::pair<llvm::GlobalVariable*, llvm::Value*>> globalInitList;

    llvm::Value* createBigIntAVX512(const std::string& str, unsigned bitSize);
    llvm::Value* createBigIntAVX2(const std::string& str, unsigned bitSize);
    llvm::Value* createBigIntAVX(const std::string& str, unsigned bitSize);
    llvm::Value* loadMemberFromStruct(llvm::Value* structPtr, llvm::StructType* structType, const std::string& memberName);
    llvm::Value* loadMemberFromClass(llvm::Value* classPtr, llvm::StructType* classType, const std::string& memberName);
    int getStructMemberIndex(llvm::StructType* structType, const std::string& memberName);
      

public:
    // Constructor initializes context, builder, and module
    IRGenerator(const std::string& mainModulePath);

    std::unique_ptr<llvm::Module> getModule() { return std::move(Module); }
    std::unique_ptr<llvm::LLVMContext> getContext() { return std::move(Context); }
    llvm::IRBuilder<>* getBuilder() { return Builder.get(); }
    
    
    bool supportsAVX512();
    bool supportsAVX2();

    void initialize();

    void printIR();
    void printErrors();
    void printErrors(llvm::Module& module);
    void optimizeModule(int level = 2); // Define optimization logic

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
            // console.error("No active block to pop!");
            return;
        }
        llvm::BasicBlock* prevBlock = insertionPointStack.top();
        insertionPointStack.pop();
        if (prevBlock) {
            Builder->SetInsertPoint(prevBlock);
        }
        // If prevBlock is nullptr, the Builder remains unchanged
    }

    inline void pushScope() {
        activeScope = std::make_shared<SymbolTable>(activeScope);
    }
    
    inline void popScope() {
        if (activeScope->getParent()) {
            activeScope = activeScope->getParent();
        }
    }

    void generateModule (
        const std::string& modulePath,
        const std::string& alias,
        const std::vector<std::shared_ptr<Statement>>& statements,
        const std::unordered_map<std::string, std::string>& importedAliases,
        bool importAll
    );
    void importModule(const std::string& moduleName, const std::vector<std::string>& members);
    // bool isModuleUpdated(const std::string& moduleName);
    // void unloadModule(const std::string& moduleName);

    
    // Generate IR for different types
    llvm::Value* codegen(std::shared_ptr<Omniscript::Value> value, std::shared_ptr<SymbolTable> scope);
    llvm::Value* codegenPrimitive(std::shared_ptr<Omniscript::Value> value, std::shared_ptr<SymbolTable> scope);
    llvm::Type* resolveLLVMType(std::shared_ptr<Omniscript::Type> type);
    
    llvm::Value* createNullPointer();
    llvm::Value* createNullValue();
    
    // Number types
    llvm::Value* create8BitInteger(int8_t value);
    llvm::Value* create16BitInteger(int16_t value);
    llvm::Value* create32BitInteger(int32_t value);
    llvm::Value* create64BitInteger(int64_t value);
    llvm::Value* createUnsigned8BitInteger(uint8_t value);
    llvm::Value* createUnsigned16BitInteger(uint16_t value);
    llvm::Value* createUnsigned32BitInteger(uint32_t value);
    llvm::Value* createUnsigned64BitInteger(uint64_t value);
    
    llvm::Value* create32BitFloat(float value);
    llvm::Value* create64BitFloat(double value);
    
    llvm::Value* createBigInt(const std::string& str, unsigned bitWidth); // Arbitrary precision integer

    llvm::Value* createChar(char value);
    llvm::Value* createChar16(char16_t value);
    llvm::Value* createChar32(char32_t value);
    llvm::Value* createUTF8String(const std::string& str);
    llvm::Value* createUTF16String(const std::u16string& str);
    llvm::Value* createUTF32String(const std::u32string& str);
    
    llvm::Value* createBool(bool value);

    // Assignments
    llvm::Function* getOrCreateGlobalInitFunction();
    void scheduleGlobalInitialization(
        const std::string& name,
        llvm::GlobalVariable* gVar,
        llvm::Value* initialValue
    );
    void finalizeGlobalInitializers();
    llvm::GlobalVariable* createGlobalVariable(
        const std::string& name, 
        llvm::Type* type, 
        llvm::Value* initialValue, 
        llvm::GlobalValue::LinkageTypes linkage = llvm::GlobalValue::InternalLinkage // Default to internal
    );
    llvm::Value* createVariable(
        const std::string& name, 
        llvm::Type* type = nullptr, 
        llvm::Value* initialValue = nullptr, 
        bool isGlobal = true, 
        llvm::BasicBlock* activeBlock = nullptr
    );
    llvm::Value* createConstant(const std::string& name, llvm::Type* type, llvm::Value* value);
    llvm::Value* reassign(const std::string& name, llvm::Value* newValue);
    llvm::Value* getAddressOf(const std::string& varname);
    llvm::Value* getReferenceToVariable(const std::string& varname);
    llvm::Value* dereferenceValue(llvm::Value* value);
    llvm::Value* fullyDereferencePointer(llvm::Value* ptr);
    llvm::Value* getVariable(const std::string& varname);
    
    llvm::Value* createDynamicVariable(const std::string& name, llvm::Value* value);
    llvm::Value* createDynamicConstant(const std::string& name, llvm::Value* value);
    llvm::Value* assignDynamicVariable(const std::string& name, llvm::Value* newValue);
    llvm::Value* getDynamicVariable(const std::string& name);
    llvm::Value* generateOpaqueDynamicVariable(const std::string& name, llvm::Value* value);

    llvm::Value* createStaticFixedArray(llvm::Type* elementType, size_t arraySize, const std::vector<llvm::Value*>& elements);
    llvm::Function* createFunction(const FunctionDeclaration& funcDecl);
    void generateFunctionBody(llvm::Function* function, const FunctionDeclaration& funcDecl);
    llvm::AllocaInst* createEntryBlockAlloca(llvm::Function* function,llvm::Type* type, const std::string& name);
    // llvm::Value* createCall(const std::string& callee, std::vector<llvm::Value*>& args);
    llvm::Value* createCall(
        const std::string& callee, 
        std::vector<llvm::Value*>& args, 
        llvm::BasicBlock* activeBlock = nullptr
    );
    bool currentBlockHasTerminator() const;
    llvm::Value* castValue(llvm::Value* val, llvm::Type* targetType);
    std::string typeToString(llvm::Type* type);
    llvm::Value* createReturn(llvm::Value* returnValue, llvm::Type* expectedReturnType);

    llvm::Value* createUnaryExpression(llvm::Value* operand, TokenTypes op, bool isPostfix);
    llvm::Value* createBinaryExpression(llvm::Value* left, TokenTypes op, llvm::Value* right);
    llvm::Value* createTernaryExpression(llvm::Value* cond, llvm::Value* truthy, llvm::Value* falsey);

    llvm::Value* createObjectInstance(
        const std::string& objectType,
        const std::string& instanceName,
        const std::vector<llvm::Value*>& args = {}
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

    void createStructType(const ConstructStructPrototype& structProto);
    llvm::Value* createStructInstance(
        const std::string& structName,
        const std::string& varName,
        const std::vector<llvm::Value*>& args);
    
    llvm::Value* createClassInstance(
        const std::string& className,
        const std::string& varName,
        const std::vector<llvm::Value*>& args);
    
    llvm::Value* createPrimitiveInstance(
        llvm::Type* type,
        const std::string& varName,
        llvm::Value* initValue = nullptr);
    
    void createEnum(
        const std::string& enumName,
        const std::vector<std::string>& valueNames,
        const std::vector<int>& valueIndices
    );

    void createEnumWithLookup(
        const std::string& enumName,
        const std::vector<std::string>& valueNames,
        const std::vector<int>& valueIndices
    );

    llvm::Value* getEnumValue(const std::string& enumName, const std::string& memberName);
};

#endif