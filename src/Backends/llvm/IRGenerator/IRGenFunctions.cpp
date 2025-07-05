#include <omniscript/Backends/LLVM/IRGenerator.h>
#include <omniscript/Backends/LLVM/LLVMExternalFunctionResolver.h>
#include <omniscript/Backends/LLVM/ExternalFunctionResolvers/CLLVMResolver.h>
#include <omniscript/Backends/LLVM/ExternalFunctionResolvers/LinuxLLVMResolver.h>
#include <omniscript/Backends/LLVM/ExternalFunctionResolvers/PosixLLVMResolver.h>
#include <omniscript/Backends/LLVM/ExternalFunctionResolvers/DarwinLLVMResolver.h>
#include <omniscript/Backends/LLVM/ExternalFunctionResolvers/AndroidLLVMResolver.h>
#include <omniscript/Backends/LLVM/ExternalFunctionResolvers/WindowsAPILLVMResolver.h>
#include <omniscript/Backends/LLVM/ExternalFunctionResolvers/WebAssemblyLLVMResolver.h>
#include <omniscript/Backends/LLVM/ExternalFunctionResolvers/SmartPlatformLLVMResolver.h>
#include <omniscript/Backends/LLVM/ExternalFunctionResolvers/StaticLibraryLLVMResolver.h>
#include <omniscript/Backends/LLVM/ExternalFunctionResolvers/DynamicLibraryLLVMResolver.h>

void IRGenerator::createEntryFunction() {
    DEBUG_LOG("creating the entry function '__top_level'.");
    // Always create __top_level__ as the entry point
    llvm::FunctionType* funcType = llvm::FunctionType::get(
        llvm::Type::getVoidTy(*Context), 
        false  // no parameters
    );
    
    llvm::Function* entryFunc = llvm::Function::Create(
        funcType,
        llvm::Function::ExternalLinkage,
        "__top_level__",
        Module.get()
    );
    
    // Set function attributes based on configuration
    if (configs.mode != CompileMode::JIT) {
        if (configs.security.enableStackProtection) {
            // entryFunc->addFnAttr(llvm::Attribute::StackProtectReq);
        }
    }
    
     if (configs.optimization.level == 0) {
        entryFunc->addFnAttr(llvm::Attribute::OptimizeForSize);     
    }
    
    // Create entry basic block
    llvm::BasicBlock* entry = llvm::BasicBlock::Create(*Context, "entry", entryFunc);
    Builder->SetInsertPoint(entry);
}

void IRGenerator::addMainFunction() {
    if (!Module) return;
    
    llvm::Function* mainFn = Module->getFunction("main");
    if (mainFn) return; // already present
    
    llvm::Function* topFunc = Module->getFunction("__top_level__");
    if (!topFunc) {
        // If top-level doesn't exist, create a dummy one
        llvm::FunctionType* topType = llvm::FunctionType::get(llvm::Type::getVoidTy(*Context), false);
        topFunc = llvm::Function::Create(topType, llvm::Function::ExternalLinkage, "__top_level__", Module.get());
        llvm::BasicBlock* topEntry = llvm::BasicBlock::Create(*Context, "entry", topFunc);
        Builder->SetInsertPoint(topEntry);
        Builder->CreateRetVoid();
    }
    
    // Create the standard `main` function with signature: int main(int argc, char** argv)
    llvm::Type* intType = llvm::Type::getInt32Ty(*Context);
    llvm::Type* charType = llvm::Type::getInt8Ty(*Context);
    llvm::Type* charPtrType = llvm::PointerType::getUnqual(charType);
    llvm::Type* charPtrPtrType = llvm::PointerType::getUnqual(charPtrType);

    llvm::FunctionType* mainType = llvm::FunctionType::get(
        intType, 
        {intType, charPtrPtrType}, 
        false
    );
    
    mainFn = llvm::Function::Create(mainType, llvm::Function::ExternalLinkage, "main", Module.get());
    
    // Set parameter names for clarity
    auto args = mainFn->arg_begin();
    args->setName("argc");
    (++args)->setName("argv");
    
    llvm::BasicBlock* mainEntry = llvm::BasicBlock::Create(*Context, "entry", mainFn);
    Builder->SetInsertPoint(mainEntry);
    
    // Call __top_level__() (ignoring argc/argv for now)
    Builder->CreateCall(topFunc);
    Builder->CreateRet(llvm::ConstantInt::get(llvm::Type::getInt32Ty(*Context), 0));
}

llvm::Function* IRGenerator::getOrCreateGlobalInitFunction() {
    // const char* initName = "__startup__";
    const char* initName = "__top_level__";
    
    // First check if function already exists
    if (auto* existing = Module->getFunction(initName)) {
        return existing;
    }

    // Create function type (void -> void)
    auto* funcType = llvm::FunctionType::get(
        Builder->getVoidTy(), 
        false
    );

    // Create the function
    auto* func = llvm::Function::Create(
        funcType,
        llvm::Function::InternalLinkage,
        initName,
        Module.get()
    );

    // Create entry block
    auto* entry = llvm::BasicBlock::Create(
        Module->getContext(), 
        "entry", 
        func
    );

    Builder->SetInsertPoint(entry);
    Builder->CreateRetVoid(); // Ensure the function has a return

    // Get or create `llvm.global_ctors`
    llvm::GlobalVariable* globalCtors = Module->getNamedGlobal("llvm.global_ctors");
    
    llvm::StructType* ctorStructType = llvm::StructType::get(
        Builder->getInt32Ty(), // Priority
        func->getType(),       // Function pointer
        llvm::PointerType::getUnqual(Module->getContext()) // Data
    );

    llvm::Constant* ctorEntry = llvm::ConstantStruct::get(
        ctorStructType,
        {
            llvm::ConstantInt::get(Builder->getInt32Ty(), 0), // Priority = 0
            func,                                            // Function pointer
            llvm::Constant::getNullValue(
                llvm::PointerType::getUnqual(Module->getContext())
            ) // Data (nullptr)
        }
    );

    // If `llvm.global_ctors` exists, append the new function
    if (globalCtors) {
        auto* arrayType = llvm::dyn_cast<llvm::ArrayType>(globalCtors->getValueType());
        size_t existingSize = arrayType->getNumElements();
        
        std::vector<llvm::Constant*> ctorEntries;

        auto* existingInit = llvm::dyn_cast<llvm::ConstantArray>(globalCtors->getInitializer());
        for (size_t i = 0; i < existingSize; ++i) {
            ctorEntries.push_back(existingInit->getOperand(i));
        }

        // Add new constructor
        ctorEntries.push_back(ctorEntry);

        auto* newArrayType = llvm::ArrayType::get(ctorStructType, ctorEntries.size());
        auto* newInit = llvm::ConstantArray::get(newArrayType, ctorEntries);

        // Replace global variable with updated initializer
        globalCtors->setInitializer(newInit);
    } else {
        // If `llvm.global_ctors` doesn't exist, create it
        auto* arrayType = llvm::ArrayType::get(ctorStructType, 1);
        auto* globalCtorVar = new llvm::GlobalVariable(
            *Module,
            arrayType,
            false,
            llvm::GlobalValue::AppendingLinkage,
            llvm::ConstantArray::get(arrayType, {ctorEntry}),
            "llvm.global_ctors"
        );

        globalCtorVar->setAlignment(llvm::Align(8));
    }

    return func;
}

llvm::Value* IRGenerator::createCall(
    const std::string& callee,
    std::vector<llvm::Value*>& args,
    llvm::BasicBlock* activeBlock
) {
    return createCall(callee, args, "");
}

llvm::Value* IRGenerator::createCall(
    const std::string& callee,
    std::vector<llvm::Value*>& args,
    const std::string& functionTypeName
) {
    llvm::Value* funcValue = nullptr;

    // Look for the function in the module or current scope
    if (auto* moduleFunc = Module->getFunction(callee)) {
        funcValue = moduleFunc;
    } else if (auto* value = activeScope->get(callee)) {
        funcValue = value;
    } else {
        console.error("Function or pointer '" + callee + "' not found in scope '" + activeScope->getName() + "'");
        return nullptr;
    }

    llvm::FunctionType* funcType = nullptr;

    // If it's a function, get the FunctionType directly
    if (auto* func = llvm::dyn_cast<llvm::Function>(funcValue)) {
        funcType = func->getFunctionType();
    }
    // If it's a function pointer (Value*), attempt to use a known signature
    else if (funcValue->getType()->isPointerTy()) {
        // NOTE: Opaque pointers don't expose element type anymore.
        // So you must already know or track the function signature elsewhere.
        if (!functionTypeName.empty()) {
            funcType = llvm::dyn_cast<llvm::FunctionType>(activeScope->getType(functionTypeName));
            if (!funcType) {
                console.error("Value in scope is not a FunctionType for: " + functionTypeName);
                return nullptr;
            }
        }
    } else {
        console.error("Value for '" + callee + "' is not callable");
        return nullptr;
    }

    // Validate and cast arguments
    bool isVarArg = funcType->isVarArg();
    size_t fixedParams = funcType->getNumParams();
    size_t givenArgs = args.size();

    if (!isVarArg && fixedParams != givenArgs) {
        console.error("Argument count mismatch for '" + callee + "', expected " +
                      std::to_string(fixedParams) + " but got " + std::to_string(givenArgs));
        return nullptr;
    }

    for (size_t i = 0; i < fixedParams; ++i) {
        llvm::Type* expected = funcType->getParamType(i);
        if (args[i]->getType() != expected) {
            llvm::Value* castedArg = generateCast(args[i], expected);
            if (!castedArg) {
                console.error("Failed to cast argument " + std::to_string(i) +
                              " in call to '" + callee + "'");
                return nullptr;
            }
            args[i] = castedArg;
        }
    }

    llvm::BasicBlock* insertBlock = Builder->GetInsertBlock();
    if (!insertBlock) {
        console.error("No valid insertion block available for call to '" + callee + "'");
        return nullptr;
    }

    if (insertBlock->getTerminator()) {
        Builder->SetInsertPoint(insertBlock->getTerminator());
    } else {
        Builder->SetInsertPoint(insertBlock);
    }

    // Direct call if Function*
    if (auto* func = llvm::dyn_cast<llvm::Function>(funcValue)) {
        return Builder->CreateCall(func, args);
    }

    // If it's a function pointer, cast it to known type and call
    llvm::PointerType* funcPtrType = llvm::PointerType::getUnqual(funcType);
    llvm::Value* castedFunc = Builder->CreateBitCast(funcValue, funcPtrType);
    return Builder->CreateCall(funcType, castedFunc, args);
}

llvm::Function* IRGenerator::createExternFunction(
    std::shared_ptr<Omniscript::FunctionExpression> func,
    SymbolTableType scope) {
    
    std::string& name = func->mangledName;
    std::string& externName = func->externName;
    
    // Resolve platform-specific library paths
    auto targetOS = configs.resolveTargetOS();
    std::string genericStatic;
    std::string genericDynamic;
    
    // Select appropriate library paths based on target OS and compilation mode
    switch (targetOS) {
        case TargetOS::Windows:
            genericStatic = !func->windowsStatic.empty() ? func->windowsStatic : func->genericStatic;
            genericDynamic = !func->windowsDynamic.empty() ? func->windowsDynamic : func->genericDynamic;
            break;
            
        case TargetOS::Linux:
        case TargetOS::FreeBSD:
        case TargetOS::Android:
            genericStatic = !func->linuxStatic.empty() ? func->linuxStatic : func->genericStatic;
            genericDynamic = !func->linuxShared.empty() ? func->linuxShared : func->genericDynamic;
            break;
            
        case TargetOS::MacOS:
        case TargetOS::iOS:
            genericStatic = !func->macosStatic.empty() ? func->macosStatic : func->genericStatic;
            genericDynamic = !func->macosShared.empty() ? func->macosShared : func->genericDynamic;
            break;
            
        case TargetOS::WebAssembly:
            // WebAssembly typically uses static linking or imports
            genericStatic = func->genericStatic;
            genericDynamic = func->genericDynamic;
            break;
            
        default:
            // Fallback to generic paths
            genericStatic = func->genericStatic;
            genericDynamic = func->genericDynamic;
            break;
    }
    
    // Fallback to legacy fields if platform-specific ones are empty
    if (genericStatic.empty() && !func->genericStatic.empty()) {
        genericStatic = func->genericStatic;
    }
    if (genericDynamic.empty() && !func->genericDynamic.empty()) {
        genericDynamic = func->genericDynamic;
    }
    
    llvm::Type* returnType = resolveLLVMType(func->returnType);
    std::vector<std::shared_ptr<Omniscript::Expression>>& params = func->parameters;
    bool isVarArg = func->isVarArg;

    std::vector<llvm::Type*> paramTypes;
    for (const auto& param : params) {
        paramTypes.push_back(resolveLLVMType(param->getType()));
    }

    llvm::FunctionType* funcType = llvm::FunctionType::get(returnType, paramTypes, isVarArg);
    llvm::Function* function = nullptr;

    // Enhanced resolver selection logic
    auto tryAddResolver = [&](const std::string& libPath, auto&& resolverFactory) -> ExternalFunctionResolver* {
        if (resolvers.find(libPath) == resolvers.end()) {
            try {
                addExternalResolver(libPath, resolverFactory());
            } catch (const std::exception& e) {
                console.error("Failed to add resolver for " + libPath + ": " + e.what());
                return nullptr;
            }
        }
        auto it = resolvers.find(libPath);
        return (it != resolvers.end()) ? it->second.get() : nullptr;
    };
    
    if (configs.mode == CompileMode::JIT) {
        if (!fileExists(genericDynamic) && (resolvers.find(genericDynamic) == resolvers.end()) && genericDynamic != "C") {
            console.error("Dynamic library '" + genericDynamic + "' for function '" + name + "' was not found.\n" +
                            "Try using the full library path.");
            return nullptr;
        }

        auto resolver = tryAddResolver(genericDynamic, [&]() {
            return std::make_unique<DynamicLibraryResolver>(genericDynamic);
        });

        if (resolver) {
            function = resolver->resolve(*this, externName, funcType, linkerDependencies);
        }
        
    } else {
        // AOT mode - enhanced static/dynamic library handling with dynamic Windows API detection
        bool staticExists = !genericStatic.empty() && fileExists(genericStatic);
        bool dynamicExists = !genericDynamic.empty() && fileExists(genericDynamic);

        if (!staticExists && !dynamicExists) {
            // Try common system libraries as fallback - user should provide explicit paths
            if (CStdLibResolver::isCStdLibFunction(externName)) {
                if (targetOS == TargetOS::Windows) {
                    genericStatic = "msvcrt.lib";
                    genericDynamic = "msvcrt.dll";
                } else {
                    genericStatic = "libc.a";
                    genericDynamic = "libc.so";
                }
                staticExists = true;
                dynamicExists = true;
            }
            // For Windows API functions, try to auto-detect the library
            else if (targetOS == TargetOS::Windows && WindowsAPIResolver::isLikelyWindowsAPIFunction(externName)) {
                // Get the likely library name for this function
                std::string detectedLib = WindowsAPIResolver::getRequiredLibraryForFunction(externName);
                genericStatic = detectedLib + ".lib";
                genericDynamic = detectedLib + ".dll";
                staticExists = true;
                dynamicExists = true;
                console.info("Auto-detected Windows API function '" + externName + "' in library: " + detectedLib);
            }
        }

        if (staticExists) {
            // Skip symbol existence check for Windows system libraries
            bool isWindowsSystemLib = (targetOS == TargetOS::Windows) && 
                                     WindowsAPIResolver::isWindowsSystemLibrary(genericStatic);
            if (!isWindowsSystemLib && !ExternalFunctionResolver::isSystemLibrary(genericStatic) && 
                !symbolExistsInStaticLib(genericStatic, externName)) {
                console.error("Symbol '" + externName + "' not found in static library: " + genericStatic);
                return nullptr;
            }

            auto resolver = tryAddResolver(genericStatic, [&]() -> std::unique_ptr<ExternalFunctionResolver> {
                if (CStdLibResolver::isCStdLibFunction(externName)) {
                    return std::make_unique<CStdLibResolver>();
                } else if (targetOS == TargetOS::Windows && WindowsAPIResolver::isWindowsSystemLibrary(genericStatic)) {
                    // Create a resolver that knows about the specific library
                    return std::make_unique<WindowsAPIResolver>(genericStatic);
                } else {
                    return std::make_unique<StaticLibraryResolver>(genericStatic);
                }
            });

            if (resolver) {
                function = resolver->resolve(*this, externName, funcType, linkerDependencies);
            }
        }

        if (!function && dynamicExists) {
            auto resolver = tryAddResolver(genericDynamic, [&]() -> std::unique_ptr<ExternalFunctionResolver> {
                switch (targetOS) {
                    case TargetOS::Windows:
                        return std::make_unique<DynamicLibraryResolver>(genericDynamic);
                    case TargetOS::Linux:
                    case TargetOS::FreeBSD:
                    case TargetOS::Android:
                        return std::make_unique<DynamicLibraryResolver>(genericDynamic);
                    case TargetOS::MacOS:
                    case TargetOS::iOS:
                        return std::make_unique<DynamicLibraryResolver>(genericDynamic);
                    default:
                        return std::make_unique<DynamicLibraryResolver>(genericDynamic);
                }
            });

            if (resolver) {
                function = resolver->resolve(*this, externName, funcType, linkerDependencies);
            }
        }
    }

    if (!function) {
        console.error("Failed to resolve external function: " + externName + 
                     " (searched in static: '" + genericStatic + 
                     "', dynamic: '" + genericDynamic + "')");
        return nullptr;
    }

    activeScope->set(name, function);
    return function;
}

llvm::Function* IRGenerator::createIntrinsicFunction( 
    const std::string& name,
    const std::string& intrinsicName,
    llvm::Type* returnType
) {
    DEBUG_LOG("Creating intrinsic function by name: " + intrinsicName);

    // Look up the intrinsic ID by name
    llvm::Intrinsic::ID intrinsicID = llvm::Intrinsic::lookupIntrinsicID(intrinsicName);
    if (intrinsicID == llvm::Intrinsic::not_intrinsic) {
        console.error("Unknown intrinsic function: " + intrinsicName);
        return nullptr;
    }

    // Get the intrinsic function declaration passing only the return type
    llvm::Function* intrinsicFunc = llvm::Intrinsic::getOrInsertDeclaration(
        currentModule,
        intrinsicID,
        { returnType }
    );

    if (!intrinsicFunc) {
        console.error("Failed to declare intrinsic: " + name);
        return nullptr;
    }

    // Register it in the current scope (optional)
    activeScope->set(name, intrinsicFunc);
    DEBUG_LOG("Registered intrinsic in scope: " + intrinsicFunc->getName().str());

    return intrinsicFunc;
}

llvm::Function* IRGenerator::createFunction(
    const std::string& name,
    std::vector<std::shared_ptr<Omniscript::Expression>>& body,
    llvm::Type* returnType,
    std::vector<std::shared_ptr<Omniscript::Expression>>& params,
    std::shared_ptr<SymbolTable<std::shared_ptr<Omniscript::Expression>, std::shared_ptr<Omniscript::Type>>> scope,
    bool isVarArg
) {
    llvm::Function* function = registerFunction(name, returnType, params, scope, isVarArg);
    
    // Generate function body
    generateFunctionBody(name, function, params, body, scope);

    return function;
}

llvm::Function* IRGenerator::registerFunction(
    const std::string& name,
    llvm::Type* returnType,
    std::vector<std::shared_ptr<Omniscript::Expression>>& params,
    std::shared_ptr<SymbolTable<std::shared_ptr<Omniscript::Expression>, std::shared_ptr<Omniscript::Type>>> scope,
    bool isVarArg
) {
    DEBUG_LOG("Creating function: " + name + " with parameter size " + std::to_string(params.size()));
    
    // Create function type
    std::vector<llvm::Type*> paramTypes;
    for (int i = 0; i < params.size(); i++) {
        auto& param = params[i];
        auto type = param->getType();
        auto llvmType = resolveLLVMType(type);
        auto parameter = std::dynamic_pointer_cast<Omniscript::FunctionInputExpression>(param);
        if (parameter->isVariadic) {
            llvm::Type* countType = llvm::Type::getInt32Ty(*Context);
            paramTypes.push_back(countType);
            
            std::shared_ptr<Omniscript::Type> intType = Omniscript::resolveType({"int32"});
            auto countParam = std::make_shared<Omniscript::FunctionInputExpression>(parameter->name + "_count", intType);
            params.insert(params.begin() + i, countParam);
            DEBUG_LOG("Resolved parameter type: " + intType->toString() + " to LLVM type: " + debugType(llvmType));
            i++;
        }
        paramTypes.push_back(llvmType);
        
        DEBUG_LOG("Resolved parameter type: " + type->toString() + " to LLVM type: " + debugType(llvmType));
    }

    DEBUG_LOG("Resolved return type LLVM: " + debugType(returnType));
    
    llvm::FunctionType* funcType = llvm::FunctionType::get(
        returnType,
        paramTypes,
        isVarArg
    );

    // Create function
    llvm::Function* function = llvm::Function::Create(
        funcType,
        llvm::Function::ExternalLinkage,
        name,
        currentModule
    );

    // Set parameter names
    unsigned idx = 0;
    bool foundArgsCount = false;
    for (auto& arg : function->args()) {
        auto& param = params[idx];
        
        if (idx >= params.size()) {
            console.error("Parameter index out of bounds: " + std::to_string(idx));
        }

        arg.setName(param->name);
        
        if (auto inpt = std::dynamic_pointer_cast<Omniscript::FunctionInputExpression>(param)) {
            // if (inpt->isConstant) {
            //     arg.addAttr(llvm::Attribute::ReadOnly); // <--- Mark as readonly if constant
            // }
            DEBUG_LOG("Setting function argument: " + param->name + 
                    " of kind: " + param->getType()->toString() + 
                    (inpt->isConstant ? " [const]" : ""));
        }
        idx++;
    }

    // Store function in scope
    activeScope->set(name, function);
    DEBUG_LOG("Stored function: " + name + " in scope");

    return function;
}

void IRGenerator::generateFunctionBody( 
    const std::string& name,
    llvm::Function* function,
    std::vector<std::shared_ptr<Omniscript::Expression>>& params,
    std::vector<std::shared_ptr<Omniscript::Expression>>& body,
    std::shared_ptr<SymbolTable<std::shared_ptr<Omniscript::Expression>, std::shared_ptr<Omniscript::Type>>> scope
) {
    DEBUG_LOG("Generating body for function: " + function->getName().str());
    DEBUG_LOG("Function return type: " + debugType(function->getReturnType()));

    auto savedIP = Builder->saveIP();  // Save current insertion point

    // Create entry block
    llvm::BasicBlock* entry = llvm::BasicBlock::Create(*Context, "entry", function);
    Builder->SetInsertPoint(entry);
    DEBUG_LOG("Created entry block for function: " + function->getName().str());

    if (name == "main") {
        DEBUG_LOG("Inserting call to __top_level__ inside main");
    
        std::vector<llvm::Value*> topLevelArgs; // no arguments
        llvm::Value* topLevelCall = createCall("__top_level__", topLevelArgs, entry);
        
        if (!topLevelCall) {
            console.error("Failed to insert call to __top_level__ in main");
            Builder->CreateUnreachable();
            popScope();
            popActiveBlock();
            function->eraseFromParent();
            return;
        }
    
        DEBUG_LOG("Successfully inserted call to __top_level__ inside main");
    }    
    
    // Create a new scope for function parameters + body
    pushScope(name);
    DEBUG_LOG("Pushed new scope for function");
    auto localScope = scope->createChildScope(name);

    // Find the count index as before
    int countIndex = -1;
    for (int i = 0; i < (int)params.size(); ++i) {
        auto param = std::dynamic_pointer_cast<Omniscript::FunctionInputExpression>(params[i]);
        if (!param) {
            console.error("Expected a parameter for parameter " + std::to_string(i));
            return; // or return error
        }
        if (param->isVariadic) {
            countIndex = i - 1;
            break;
        }
    }

    // Create allocas for parameters in the entry block
    int index = 0;
    bool foundArgsCount = false;
    for (auto& arg : function->args()) {
        std::string argName = arg.getName().str();
        DEBUG_LOG("Allocating parameter: " + argName + " with type: " + debugType(arg.getType()));

        auto param = std::dynamic_pointer_cast<Omniscript::FunctionInputExpression>(params[index]);

        // if there is a variadic parameter there is one extra parameter
        if ((countIndex >= 0 ? index >= params.size() + 1 : index >= params.size())) {
            console.error("Parameter index out of bounds: " + std::to_string(index));
            break;
        }

        if (/* arg escapes = false*/ true) {
            activeScope->set(argName, &arg); // Use directly
        } else {
            llvm::IRBuilder<> tmpBuilder(&function->getEntryBlock(), function->getEntryBlock().begin());
            llvm::AllocaInst* alloca = tmpBuilder.CreateAlloca(arg.getType(), nullptr, argName);
            Builder->CreateStore(&arg, alloca);
            activeScope->set(argName, alloca);
        }        

        DEBUG_LOG("Stored parameter '" + argName + "' in scope.");

        index++;
    }

    if (function->getFunctionType()->isVarArg()) {
        // 1. Allocate va_list variable (usually a pointer-sized alloca)
        // llvm::Type* i8Ty = llvm::Type::getInt8Ty(*Context);
        // llvm::Type* i8PtrTy = llvm::PointerType::getUnqual(i8Ty);
        // llvm::AllocaInst* vaListAlloca = createEntryBlockAlloca(function, i8PtrTy, "va_list");
        
        // 2. Insert call to llvm.va_start intrinsic with the va_list
        // llvm::FunctionCallee vaStartCallee = llvm::Intrinsic::getOrInsertDeclaration(Module.get(), llvm::Intrinsic::vastart);
        // llvm::Function* vaStartFunc = llvm::dyn_cast<llvm::Function>(vaStartCallee.getCallee());

        // Builder->CreateCall(vaStartFunc, { vaListAlloca });

        // Now, you can expose vaListAlloca in the scope so the function's body codegen
        // can generate llvm.va_arg calls as needed to fetch variadic arguments dynamically.

        // activeScope->set("va_list", vaListAlloca);

        // Note: You should also insert llvm.va_end before the function returns,
        // ideally right before every return instruction. You can either:
        // - Track all return points and insert va_end calls there, or
        // - Insert one before the function epilogue if you have a single return.
    }


    // Generate function body
    llvm::Value* retVal = nullptr;

    for (const auto& expr : body) {
        if (Builder->GetInsertBlock()->getTerminator()) {
            break; // Don't emit instructions after return
        }

        if (auto varAssign = std::dynamic_pointer_cast<Omniscript::VariableAssignment>(expr)) {
            if (!varAssign->isStatic) {
                varAssign->isGlobal = false;
            }
        }

        DEBUG_LOG("Generating code for body expression of kind: " + expr->getType()->toString());
        retVal = codegen(expr, localScope);

        if (retVal) {
            DEBUG_LOG("Body expression result type: " + debugType(retVal->getType()));
        } 
    }

    // Handle implicit return if needed
    if (!currentBlockHasTerminator()) {
        if (function->getReturnType()->isVoidTy()) {
            DEBUG_LOG("Creating void return for function: " + function->getName().str());
            Builder->CreateRetVoid();
        } else if (retVal) {
            DEBUG_LOG("Creating return with value type: " + debugType(retVal->getType()));
            DEBUG_LOG("Function expects return type: " + debugType(function->getReturnType()));

            // Ensure return value matches function type
            if (retVal->getType() != function->getReturnType()) {
                DEBUG_LOG("Return type mismatch: attempting cast");
                llvm::Value* castedRet = generateCast(retVal, function->getReturnType());
                if (!castedRet) {
                    console.error("Failed to cast return value in function: " + function->getName().str());
                    return;
                }
                retVal = castedRet;
                DEBUG_LOG("Cast successful. New return type: " + debugType(retVal->getType()));
            }
            Builder->CreateRet(retVal);
            DEBUG_LOG("Created return instruction for function: " + function->getName().str());
        } else {
            // Error: Non-void function missing return
            console.warn("Non-void function missing return: " + function->getName().str());
        }
    }

    popScope();  // Parameters + function body scope
    DEBUG_LOG("Popped function scope");

    Builder->restoreIP(savedIP); 
}
