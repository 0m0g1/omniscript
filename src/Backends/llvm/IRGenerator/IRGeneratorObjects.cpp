// IRGeneratorObjects.cpp
#include <omniscript/Backends/llvm/IRGenerator.h>

#include <string>
#include <vector>
#include <unordered_map>

namespace Omniscript {

namespace {
// LLVM struct types do not store field names, so names are recorded here when a struct is
// built from named members (currently module objects). getStructMemberIndex() reads this.
std::unordered_map<std::string, std::vector<std::string>>& structMemberNames() {
    static std::unordered_map<std::string, std::vector<std::string>> names;
    return names;
}

// With opaque pointers a pointer value no longer carries its pointee type, so recover the
// struct type from the allocation / global that produced it.
llvm::StructType* structTypeOfPointer(llvm::Value* ptr) {
    if (auto* alloca = llvm::dyn_cast<llvm::AllocaInst>(ptr)) {
        return llvm::dyn_cast<llvm::StructType>(alloca->getAllocatedType());
    }
    if (auto* global = llvm::dyn_cast<llvm::GlobalVariable>(ptr)) {
        return llvm::dyn_cast<llvm::StructType>(global->getValueType());
    }
    return nullptr;
}
} // namespace

llvm::Value* IRGenerator::createObjectInstance(
    const std::string& typeName,
    const std::string& varName,
    const std::vector<llvm::Value*>& args,
    bool isGlobal
)
{
    DEBUG_LOG("Creating instance of type: " + typeName);
    
    if (llvm::StructType* structType = llvm::StructType::getTypeByName(*Context, typeName)) {
        DEBUG_LOG("Found struct type: " + typeName);
        return createStructInstance(typeName, varName, args, isGlobal);
    }

    console.reportError(Console::TYPE_ERROR, "Unknown type: " + typeName, FileSpan{});
    return nullptr;
}

void IRGenerator::createStructType(const std::string& name, const std::vector<llvm::Type*>& fieldTypes) {
    if (llvm::StructType::getTypeByName(*Context, name)) {
        DEBUG_LOG("Struct " + name + " already exists. Skipping creation.");
        return;
    }

    llvm::StructType* structType = llvm::StructType::create(*Context, fieldTypes, name);
    if (!structType) {
        console.reportError(Console::TYPE_ERROR, "Failed to create struct type: " + name, FileSpan{});
        return;
    }
    
    DEBUG_LOG("Created '" + debugType(structType) + "' struct type");
    activeScope->addType(name, structType);

    DEBUG_LOG("Created struct prototype: " + name);
}

llvm::Value* IRGenerator::createStructInstance(
    const std::string& structName,
    const std::string& varName,
    const std::vector<llvm::Value*>& args,
    bool isGlobal) {
    
    llvm::StructType* structType = llvm::StructType::getTypeByName(*Context, structName);
    if (!structType) {
        DEBUG_LOG("Struct type '" + structName + "' does not exist.");
        return nullptr;
    }

    size_t fieldCount = structType->getNumElements();
    if (args.size() != fieldCount) {
        console.reportError(Console::TYPE_ERROR, "Mismatch: struct '" + structName + "' expects " +
                      std::to_string(fieldCount) + " fields, but got " +
                      std::to_string(args.size()) + " arguments.", FileSpan{});
        return nullptr;
    }

    // At global scope there is no insert block, so fall back to the current module.
    llvm::Module* module = Builder->GetInsertBlock()
        ? Builder->GetInsertBlock()->getModule()
        : currentModule;

    // === Global Instance ===
    if (isGlobal) {
        std::vector<llvm::Constant*> initVals;
        
        for (size_t i = 0; i < args.size(); ++i) {
            llvm::Value* arg = args[i];
            llvm::Type* expectedType = structType->getElementType(i);
            
            // Handle the case where arg might be a pointer to a struct that needs to be loaded
            if (auto* ptrType = llvm::dyn_cast<llvm::PointerType>(arg->getType())) {
                if (auto* globalVar = llvm::dyn_cast<llvm::GlobalVariable>(arg)) {
                    // If it's a global variable, use its initializer directly
                    if (globalVar->hasInitializer()) {
                        auto* initializer = globalVar->getInitializer();
                        if (initializer->getType() == expectedType) {
                            initVals.push_back(initializer);
                            continue;
                        }
                    }
                }
            }
            
            // Try to cast to constant directly
            auto* c = llvm::dyn_cast<llvm::Constant>(arg);
            if (!c) {
                console.reportError(Console::TYPE_ERROR, "Non-constant used in global initializer for field " + std::to_string(i), FileSpan{});
                return nullptr;
            }
            
            // Verify type compatibility
            if (c->getType() != expectedType) {
                console.reportError(Console::TYPE_ERROR, "Type mismatch for field " + std::to_string(i) + 
                             " in struct '" + structName + "'", FileSpan{});
                return nullptr;
            }
            
            initVals.push_back(c);
        }

        llvm::Constant* initializer = llvm::ConstantStruct::get(structType, initVals);

        llvm::GlobalVariable* globalVar = new llvm::GlobalVariable(
            *module,
            structType,
            false,
            llvm::GlobalValue::ExternalLinkage,
            initializer,
            varName
        );

        activeScope->set(varName, globalVar);
        return globalVar;
    }

    // === Local Instance ===
    if (!Builder->GetInsertBlock()) {
        console.reportError(Console::TYPE_ERROR, "Cannot create local instance '" + varName + "' outside a function", FileSpan{});
        return nullptr;
    }
    llvm::Function* func = Builder->GetInsertBlock()->getParent();
    llvm::BasicBlock& entryBlock = func->getEntryBlock();

    llvm::IRBuilder<> entryBuilder(&entryBlock, entryBlock.begin());
    llvm::AllocaInst* localVar = entryBuilder.CreateAlloca(structType, nullptr, varName);

    for (size_t i = 0; i < fieldCount; ++i) {
        llvm::Value* fieldPtr = Builder->CreateStructGEP(structType, localVar, i, 
                                                         varName + "_field" + std::to_string(i));
        
        llvm::Value* valueToStore = args[i];
        llvm::Type* expectedType = structType->getElementType(i);
        
        // If the argument is a pointer and we expect a value type, load it first
        if (valueToStore->getType()->isPointerTy() && !expectedType->isPointerTy()) {
            // In modern LLVM with opaque pointers, we need to specify the type to load
            valueToStore = Builder->CreateLoad(expectedType, valueToStore, 
                                              varName + "_load_field" + std::to_string(i));
        }
        
        Builder->CreateStore(valueToStore, fieldPtr);
    }

    DEBUG_LOG("Created local struct instance: " + varName);
    activeScope->set(varName, localVar);
    return localVar;
}

llvm::Value* IRGenerator::loadMemberValue(const std::string& objectName, const std::string& memberName) { 
    llvm::Value* objectPtr = getVariable(objectName);
    if (!objectPtr) return nullptr;

    if (objectName.ends_with("_lookup")) {
        std::string enumName = objectName.substr(0, objectName.find("_lookup"));
        return getEnumValue(enumName, memberName);
    }

    if (!objectPtr->getType()->isPointerTy()) return nullptr;

    llvm::StructType* structType = structTypeOfPointer(objectPtr);
    if (!structType) return nullptr;

    return loadMemberFromStruct(objectPtr, structType, memberName);
}

llvm::Value* IRGenerator::loadMemberFromStruct(llvm::Value* structPtr, llvm::StructType* structType, const std::string& memberName) {
    int memberIndex = getStructMemberIndex(structType, memberName);
    if (memberIndex == -1) return nullptr;

    llvm::Value* memberPtr = Builder->CreateStructGEP(structType, structPtr, memberIndex);

    return Builder->CreateLoad(structType->getElementType(memberIndex), memberPtr);
}

llvm::Value* IRGenerator::loadMemberFromClass(llvm::Value* classPtr, llvm::StructType* classType, const std::string& memberName) {
    int memberIndex = getStructMemberIndex(classType, memberName);
    if (memberIndex == -1) return nullptr;

    llvm::Value* memberPtr = Builder->CreateStructGEP(classType, classPtr, memberIndex);

    return Builder->CreateLoad(classType->getElementType(memberIndex), memberPtr);
}

int IRGenerator::getStructMemberIndex(llvm::StructType* structType, const std::string& memberName) {
    if (!structType || !structType->hasName()) return -1;

    auto it = structMemberNames().find(structType->getName().str());
    if (it == structMemberNames().end()) return -1;

    const auto& names = it->second;
    for (size_t i = 0; i < names.size() && i < structType->getNumElements(); i++) {
        if (names[i] == memberName) {
            return static_cast<int>(i);
        }
    }

    return -1;
}

void IRGenerator::setMemberValue(
    llvm::Value* object, 
    const std::string& memberName, 
    llvm::Value* newValue
) {
    return;
}

llvm::Value* IRGenerator::createModuleObject(
    const std::string& moduleName,
    const std::unordered_map<std::string, llvm::Value*>& members
) {
    llvm::LLVMContext& ctx = Builder->getContext();
    llvm::Module* mod = currentModule;

    DEBUG_LOG("Creating module object: " + moduleName);

    std::vector<llvm::Type*> memberTypes;
    std::vector<llvm::Value*> memberValues;
    std::vector<std::string> memberNames;

    for (const auto& [key, val] : members) {
        memberTypes.push_back(val->getType());
        memberValues.push_back(val);
        memberNames.push_back(key);
    }

    if (llvm::StructType::getTypeByName(*Context, moduleName)) {
        console.reportError(Console::SEMANTIC_ERROR, "Cannot create module " + moduleName + " as a symbol with the name '" + moduleName + "' already exists in the scope.", FileSpan{});
        return nullptr;
    }
    
    createStructType(moduleName, memberTypes);
    structMemberNames()[moduleName] = memberNames;

    llvm::Value* moduleInstance = createObjectInstance(
        moduleName,
        moduleName,
        memberValues,
        true
    );

    DEBUG_LOG("Module struct instance created: " + moduleName);

    return moduleInstance;
}

// Integer widening is signed by default (Omniscript's Int8..Int64). LLVM integer types carry no
// signedness, so unsigned sources need the caller to say so: to support that, declare
// `generateCast(llvm::Value*, llvm::Type*, bool srcIsSigned)` in IRGenerator.h and turn this
// local into that parameter.
llvm::Value* IRGenerator::generateCast(llvm::Value* src, llvm::Type* destType) {
    const bool srcIsSigned = true;
    // Todo:: add a bitcast omniscript expression
    llvm::Type* srcType = src->getType();

    if (srcType == destType) {
        DEBUG_LOG("No cast needed; source and destination types match.");
        return src;
    }

    if (srcType->isIntegerTy() && destType->isIntegerTy()) {
        unsigned srcBits = srcType->getIntegerBitWidth();
        unsigned destBits = destType->getIntegerBitWidth();
        if (destBits > srcBits) {
            // i1 is a bool: it is never negative, so it always zero-extends.
            if (srcIsSigned && srcBits > 1) {
                return Builder->CreateSExt(src, destType, "sext");
            }
            return Builder->CreateZExt(src, destType, "zext");
        } else if (destBits < srcBits) {
            return Builder->CreateTrunc(src, destType, "trunc");
        } else {
            return src;
        }
    }

    if (srcType->isIntegerTy() && destType->isFloatingPointTy()) {
        return srcIsSigned ? Builder->CreateSIToFP(src, destType, "sitofp")
                           : Builder->CreateUIToFP(src, destType, "uitofp");
    }

    if (srcType->isFloatingPointTy() && destType->isIntegerTy()) {
        // Here "signed" describes the destination, but callers only know one flag; the
        // destination's signedness is not available, so this follows the same flag.
        return srcIsSigned ? Builder->CreateFPToSI(src, destType, "fptosi")
                           : Builder->CreateFPToUI(src, destType, "fptoui");
    }

    if (srcType->isFloatingPointTy() && destType->isFloatingPointTy()) {
        unsigned srcBits = srcType->getPrimitiveSizeInBits();
        unsigned destBits = destType->getPrimitiveSizeInBits();
        if (destBits > srcBits) {
            return Builder->CreateFPExt(src, destType, "fpext");
        } else {
            return Builder->CreateFPTrunc(src, destType, "fptrunc");
        }
    }

    if (srcType->isPointerTy() && destType->isPointerTy()) {
        if (srcType->getPointerAddressSpace() != destType->getPointerAddressSpace()) {
            console.reportError(Console::TYPE_ERROR, "Pointer cast across address spaces is invalid.", FileSpan{});
            return nullptr;
        }
        return Builder->CreateBitCast(src, destType, "ptrcast");
    }

    // ✅ NEW: Cast from integer to pointer (e.g., 12 as void*)
    if (srcType->isIntegerTy() && destType->isPointerTy()) {
        return Builder->CreateIntToPtr(src, destType, "inttoptr");
    }

    // ✅ NEW: Cast from pointer to integer (optional reverse)
    if (srcType->isPointerTy() && destType->isIntegerTy()) {
        return Builder->CreatePtrToInt(src, destType, "ptrtoint");
    }

    // Cast from void* (i8*) to function pointer type
    if (srcType->isPointerTy() && destType->isFunctionTy()) {
        llvm::PointerType* funcPtrType = llvm::PointerType::getUnqual(*Context);
        return Builder->CreateBitCast(src, funcPtrType, "void_to_func_ptrcast");
    }

    // Cast from function pointer to void*
    if (srcType->isFunctionTy() && destType->isPointerTy()) {
        llvm::PointerType* srcPtrType = llvm::PointerType::getUnqual(*Context);
        return Builder->CreateBitCast(src, destType, "func_to_void_ptrcast");
    }

    console.reportError(Console::TYPE_ERROR, "Unsupported cast from '" + debugType(srcType) + "' to '" + debugType(destType) + "'", FileSpan{});
    return nullptr;
}

} // namespace Omniscript