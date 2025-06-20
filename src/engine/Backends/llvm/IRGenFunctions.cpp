#include <llvm/IR/Verifier.h>
#include <omniscript/engine/Backends/LLVM/IRGenerator.h>

llvm::Function* IRGenerator::createExternFunction(
    std::shared_ptr<Omniscript::FunctionExpression> func,
    SymbolTableType scope) {

    std::string& name = func->mangledName;
    std::string& externName = func->externName;
    std::string& staticLibPath = func->staticLibPath;
    std::string& dynamicLibPath = func->dynamicLibPath;
    llvm::Type* returnType = resolveLLVMType(func->returnType);
    std::vector<std::shared_ptr<Omniscript::Expression>>& params = func->parameters;
    bool isVarArg = func->isVarArg;
    bool isStatic = func->isStatic;

    std::vector<llvm::Type*> paramTypes;
    for (const auto& param : params) {
        paramTypes.push_back(resolveLLVMType(param->getType()));
    }

    llvm::FunctionType* funcType = llvm::FunctionType::get(returnType, paramTypes, isVarArg);

    // std::string libPath = libPath.substr(1, libPath.length() - 2); // remove surrounding quotes
    // bool isDynamicPath = !libPath.empty() && (libPath.front() == '"' || libPath.front() == '\'');
    
    // if (isDynamicPath && resolvers.find(libPath) == resolvers.end()) {
        //     addExternalResolver(libPath, std::make_unique<DynamicLibraryResolver>(libPath));
    // }
    llvm::Function* function = nullptr;

    if (configs.mode == CompileMode::JIT) {
        if (!fileExists(dynamicLibPath)) {
            console.error("JIT compiled mode requires a valid dynamic library for function '" + name + "'.");
            return nullptr;
        }

        if (resolvers.find(dynamicLibPath) == resolvers.end()) {
            addExternalResolver(dynamicLibPath, std::make_unique<DynamicLibraryResolver>(dynamicLibPath));
        }

        auto it = resolvers.find(dynamicLibPath);
        if (it == resolvers.end()) {
            console.error("No resolver found for libPath: " + dynamicLibPath);
            return nullptr;
        }

        function = it->second->resolve(*this, externName, funcType);

    } else { // AOT mode
        bool usedStatic = false;

        if (fileExists(staticLibPath)) {
            if (resolvers.find(staticLibPath) == resolvers.end()) {
                addExternalResolver(staticLibPath, std::make_unique<StaticLibraryResolver>(staticLibPath));
            }

            auto it = resolvers.find(staticLibPath);
            if (it == resolvers.end()) {
                console.error("No resolver found for static libPath: " + staticLibPath);
                return nullptr;
            }

            function = it->second->resolve(*this, externName, funcType);
            usedStatic = true;
        }

        if (!usedStatic) {
            if (!fileExists(dynamicLibPath)) {
                console.error("Function '" + name + "' must have a valid static or dynamic lib path.");
                return nullptr;
            }

            if (resolvers.find(dynamicLibPath) == resolvers.end()) {
                addExternalResolver(dynamicLibPath, std::make_unique<DynamicLibraryResolver>(dynamicLibPath));
            }

            auto it = resolvers.find(dynamicLibPath);
            if (it == resolvers.end()) {
                console.error("No resolver found for fallback dynamic libPath: " + dynamicLibPath);
                return nullptr;
            }

            function = it->second->resolve(*this, externName, funcType);
        }
    }

    if (!function) {
        console.error("Failed to resolve external function: " + externName);
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
            llvm::AllocaInst* alloca = createEntryBlockAlloca(function, arg.getType(), argName);
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
                llvm::Value* castedRet = castValue(retVal, function->getReturnType());
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
