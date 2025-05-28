#include <llvm/IR/Verifier.h>
#include <omniscript/engine/Backends/LLVM/IRGenerator.h>

llvm::Function* IRGenerator::createExternFunction(
    const std::string& name,
    const std::string& language,
    llvm::Type* returnType,
    const std::vector<std::shared_ptr<Omniscript::Expression>>& params,
    bool isVarArg
) {
    DEBUG_LOG("Creating extern \"" + language + "\" function: " + name);

    std::vector<llvm::Type*> paramTypes;
    for (auto& param : params) {
        auto type = param->getType();
        auto llvmType = resolveLLVMType(type);
        paramTypes.push_back(llvmType);
        
        DEBUG_LOG("Resolved parameter type: " + type->description() + " to LLVM type: " + debugType(llvmType));
    }

    if (isVarArg) {
        DEBUG_LOG("The external function is variadic.");
    }

    llvm::FunctionType* funcType = llvm::FunctionType::get(returnType, paramTypes, isVarArg);

    llvm::Function* function = llvm::Function::Create(
        funcType,
        llvm::Function::ExternalLinkage,
        name,
        CurrentModule
    );

    // Optionally store it in the active scope
    activeScope->set(name, function);
    DEBUG_LOG("Registered extern function in scope: " + name);

    return function;
}

llvm::Function* IRGenerator::createIntrinsicFunction(
    const std::string& name,
    const std::vector<std::shared_ptr<Omniscript::Expression>>& params
) {
    DEBUG_LOG("Creating intrinsic function by name: " + name);

    // Look up the intrinsic ID by name
    llvm::Intrinsic::ID intrinsicID = llvm::Intrinsic::lookupIntrinsicID(name);
    if (intrinsicID == llvm::Intrinsic::not_intrinsic) {
        console.error("Unknown intrinsic function: " + name);
        return nullptr;
    }

    // Create function type
    std::vector<llvm::Type*> paramTypes;
    for (auto& param : params) {
        auto type = param->getType();
        auto llvmType = resolveLLVMType(type);
        paramTypes.push_back(llvmType);
        
        DEBUG_LOG("Resolved parameter type: " + type->description() + " to LLVM type: " + debugType(llvmType));
    }

    // Get the intrinsic function declaration
    llvm::Function* intrinsicFunc = llvm::Intrinsic::getOrInsertDeclaration(
        CurrentModule,
        intrinsicID,
        paramTypes
    );

    if (!intrinsicFunc) {
        console.error("Failed to declare intrinsic: " + name);
        return nullptr;
    }

    // Register it in the current scope (optional)
    activeScope->set(intrinsicFunc->getName().str(), intrinsicFunc);
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
        i++;
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
        CurrentModule
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
                    " of kind: " + param->getType()->description() + 
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

    if (name == "__main") {
        DEBUG_LOG("Inserting call to __top_level__ inside __main");
    
        std::vector<llvm::Value*> topLevelArgs; // no arguments
        llvm::Value* topLevelCall = createCall("__top_level__", topLevelArgs, entry);
        
        if (!topLevelCall) {
            console.error("Failed to insert call to __top_level__ in __main");
            Builder->CreateUnreachable();
            popScope();
            popActiveBlock();
            function->eraseFromParent();
            return;
        }
    
        DEBUG_LOG("Successfully inserted call to __top_level__ inside __main");
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

        if (param->getName() == "this" && index == 0) {
            activeScope->set(argName, &arg); // Use directly
        } else {
            llvm::AllocaInst* alloca = createEntryBlockAlloca(function, arg.getType(), argName);
            Builder->CreateStore(&arg, alloca);
            activeScope->set(argName, alloca);
        }        

        DEBUG_LOG("Stored parameter '" + argName + "' in scope.");

        index++;
    }

    if (countIndex >= 0) {
        // Get the variadic count from alloca
        auto countParamName = params[countIndex]->getName();

        llvm::Value* countAlloca = activeScope->get(countParamName);
        if (!countAlloca) {
            console.error("Could not find alloca for count param: " + countParamName);
            return;
        }

        llvm::Type* countType = llvm::Type::getInt32Ty(*Context);
        llvm::Value* varArgCountValue = Builder->CreateLoad(countType, countAlloca, countParamName + "_load");

        // Collect variadic args as llvm::Value*
        std::vector<llvm::Value*> varArgValues;
        llvm::Type* varArgType = nullptr;

        for (int i = countIndex + 1; i < (int)params.size(); ++i) {
            auto param = std::dynamic_pointer_cast<Omniscript::FunctionInputExpression>(params[i]);
            if (!param) {
                console.error("Expected a parameter for parameter " + std::to_string(i));
                return; // or return error
            }
            if (!param->isVariadic) continue;

            llvm::Argument* arg = function->getArg(i);
            if (!varArgType) varArgType = arg->getType();

            varArgValues.push_back(arg);
        }

        // Use your existing createFixedArray function
        if (varArgType && !varArgValues.empty()) {
            llvm::Value* fixedArray = createFixedArray(varArgType, varArgValues.size(), varArgValues);

            // override the variadic parameter with the array
            std::string varArgArrayName = params[countIndex]->getName();
            std::string varArgCountName = params[countIndex]->getName() + "_count";

            // Store fixedArray in the scope so your function body can access it
            activeScope->set(varArgArrayName, fixedArray);

            // Also store the count llvm::Value for convenience
            activeScope->set(varArgCountName, varArgCountValue);
        }
    }

    // Generate function body
    llvm::Value* retVal = nullptr;
    for (const auto& expr : body) {
        DEBUG_LOG("Generating code for body expression of kind: " + expr->getType()->description());
        retVal = codegen(expr, localScope);
        if (!retVal && !function->getReturnType()->isVoidTy()) {
            console.error("Expression returned null value in non-void function: " + function->getName().str());
        } else if (retVal) {
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
            console.error("Non-void function missing return: " + function->getName().str());
            return;
        }
    }

    popScope();  // Parameters + function body scope
    DEBUG_LOG("Popped function scope");

    // Verify the function for consistency
    if (llvm::verifyFunction(*function, &llvm::errs())) {
        console.error("Function verification failed: " + function->getName().str());
        function->eraseFromParent();
    } else {
        DEBUG_LOG("Function verified successfully: " + function->getName().str());
    }

    Builder->restoreIP(savedIP); 
}
