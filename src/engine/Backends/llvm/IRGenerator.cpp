#include <omniscript/engine/Backends/llvm/IRGenerator.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/IR/PassManager.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Passes/StandardInstrumentations.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Linker/Linker.h>
#include <llvm/IR/DataLayout.h>
#include <llvm/Support/Alignment.h>
#include <llvm/ADT/StringMap.h>         // Needed for getHostCPUFeatures
#include <llvm/IR/Constants.h>  // Required for appendToGlobalCtors
#include <llvm/Support/TargetSelect.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/TargetParser/Host.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/IR/DataLayout.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/BasicBlock.h>

#include <omniscript/debuggingtools/console.h>

IRGenerator::IRGenerator(const std::string& mainModulePath) {
    Context = std::make_unique<llvm::LLVMContext>();
    Module = std::make_unique<llvm::Module>(mainModulePath, *Context);
    Builder = std::make_unique<llvm::IRBuilder<>>(*Context);
    initialize();
}

bool IRGenerator::supportsAVX512() {
    // llvm::StringMap<bool> Features;
    // llvm::sys::getHostCPUFeatures(Features);
    // return Features.lookup("avx512f"); // Check if AVX-512 is supported
    return false;
}

bool IRGenerator::supportsAVX2() {
    // llvm::StringMap<bool> Features;
    // llvm::sys::getHostCPUFeatures(Features);
    // return Features.lookup("avx2"); // Check if AVX2 is supported
    return false;
}

void IRGenerator::initialize() {
    // Create context, module, builder if not yet created
    if (!Context)
        Context = std::make_unique<llvm::LLVMContext>();

    if (!Module)
        Module = std::make_unique<llvm::Module>("OmniScript", *Context);

    if (!Builder)
        Builder = std::make_unique<llvm::IRBuilder<>>(*Context);

    // Initialize target
    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();

    std::string error;
    std::string triple = llvm::sys::getDefaultTargetTriple();

    Module->setTargetTriple(triple);

    const llvm::Target* target = llvm::TargetRegistry::lookupTarget(triple, error);
    if (!target) {
        llvm::errs() << "Target lookup failed: " << error << "\n";
        return;
    }

    llvm::TargetOptions options;
    auto targetMachine = std::unique_ptr<llvm::TargetMachine>(
        target->createTargetMachine(triple, "generic", "", options, std::nullopt)
    );

    Module->setDataLayout(targetMachine->createDataLayout());

    // Optional: Setup main or top-level function
    llvm::FunctionType* funcType = llvm::FunctionType::get(llvm::Type::getVoidTy(*Context), false);
    llvm::Function* topFunc = llvm::Function::Create(funcType, llvm::Function::ExternalLinkage, "__top_level__", Module.get());
    llvm::BasicBlock* entry = llvm::BasicBlock::Create(*Context, "entry", topFunc);
    Builder->SetInsertPoint(entry);
    // Builder->CreateRetVoid(); // placeholder

    currentModule = Module.get();
    addExternalResolver("C", std::make_unique<CStdLibResolver>());
}

void IRGenerator::finalize() {
    // Find the top-level function
    llvm::Function* topFunc = Module->getFunction("__top_level__");
    if (!topFunc) {
        llvm::errs() << "No top-level function found.\n";
        return;
    }

    // Get the last basic block (or current insert block)
    llvm::BasicBlock* lastBlock = &topFunc->back(); // last block in function

    // If the block has no terminator, add a `ret void`
    if (!lastBlock->getTerminator()) {
        Builder->SetInsertPoint(lastBlock);
        Builder->CreateRetVoid();
    }
}


void IRGenerator::printIR() {
    Module->print(llvm::outs(), nullptr);
}

void IRGenerator::printErrors() {
    if (llvm::verifyModule(*Module, &llvm::errs())) {
        llvm::errs() << "Module verification for '" << Module->getModuleIdentifier() << "' failed!\n";
    } 
    // else {
    //     llvm::errs() << "No errors found in '" << Module->getModuleIdentifier() << "'.\n";
    // }
}

void IRGenerator::printErrors(llvm::Module& module) {
    if (llvm::verifyModule(module, &llvm::errs())) {
        llvm::errs() << "Module verification for '" << module.getModuleIdentifier() << " failed! '\n";
        module.print(llvm::outs(), nullptr);
    } 
    // else {
    //     llvm::errs() << "No errors found in: '" << module.getModuleIdentifier() << "'.\n";
    // }
}

void IRGenerator::printAssembly(llvm::Module* module) {
    // Initialize targets
    llvm::InitializeAllTargetInfos();
    llvm::InitializeAllTargets();
    llvm::InitializeAllTargetMCs();
    llvm::InitializeAllAsmPrinters();

    auto targetTriple = llvm::sys::getDefaultTargetTriple();
    std::string error;
    const llvm::Target* target = llvm::TargetRegistry::lookupTarget(targetTriple, error);

    if (!target) {
        llvm::errs() << "Error: " << error << "\n";
        return;
    }

    auto cpu = "generic";
    auto features = "";

    llvm::TargetOptions opt;
    auto RM = std::optional<llvm::Reloc::Model>();
    std::unique_ptr<llvm::TargetMachine> targetMachine(
        target->createTargetMachine(targetTriple, cpu, features, opt, RM));

    module->setDataLayout(targetMachine->createDataLayout());
    module->setTargetTriple(targetTriple);

    // Verify module before emission
    if (llvm::verifyModule(*module, &llvm::errs())) {
        llvm::errs() << "LLVM module verification failed!\n";
        module->print(llvm::errs(), nullptr);
        return;
    }

    // Create the pass manager
    llvm::legacy::PassManager pass;
    llvm::SmallVector<char, 0> asmOutput;
    llvm::raw_svector_ostream outStream(asmOutput);

    // Configure for assembly output
    if (targetMachine->addPassesToEmitFile(pass, outStream, nullptr, 
                                          llvm::CodeGenFileType::AssemblyFile)) {
        llvm::errs() << "TargetMachine can't emit a file of this type\n";
        return;
    }

    // Run the passes
    pass.run(*module);

    // Print the assembly
    llvm::outs().write(asmOutput.data(), asmOutput.size());
    llvm::outs().flush();
}

std::string IRGenerator::debugType(llvm::Type* type) {
    std::string str;
    llvm::raw_string_ostream rso(str);
    type->print(rso);
    return rso.str();
}

void IRGenerator::optimizeModule(int level) {
    if (level == -1) {
        DEBUG_LOG("No optimization taking place");
        return;
    }
    
    DEBUG_LOG("Running module verification before optimization...");

    if (llvm::verifyModule(*Module, &llvm::errs())) {
        console.error("Module verification failed before optimization");
    }

    llvm::LoopAnalysisManager lam;
    llvm::FunctionAnalysisManager fam;
    llvm::CGSCCAnalysisManager cam;
    llvm::ModuleAnalysisManager mam;

    llvm::PassBuilder pb;

    // Register analyses with PassBuilder
    pb.registerModuleAnalyses(mam);
    pb.registerFunctionAnalyses(fam);
    pb.registerLoopAnalyses(lam);
    pb.registerCGSCCAnalyses(cam);

    // Link all the analysis managers together
    pb.crossRegisterProxies(lam, fam, cam, mam);

    // Choose optimization level
    llvm::OptimizationLevel optLevel = llvm::OptimizationLevel::O2;
    if (level == 0) optLevel = llvm::OptimizationLevel::O0;
    else if (level == 1) optLevel = llvm::OptimizationLevel::O1;
    else if (level == 2) optLevel = llvm::OptimizationLevel::O2;
    else if (level >= 3) optLevel = llvm::OptimizationLevel::O3;

    // Build pipeline
    llvm::ModulePassManager mpm = pb.buildPerModuleDefaultPipeline(optLevel);

    if (!Module) {
        console.error("Module is null before optimization");
    }

    // Run pipeline
     try {
        mpm.run(*Module, mam);
    } catch (const std::exception& ex) {
        console.error("Exception during optimization: " + std::string(ex.what()));
        throw;
    }
}

void IRGenerator::compileAllFunctionBodies(std::shared_ptr<SymbolTable<std::shared_ptr<Omniscript::Expression>, std::shared_ptr<Omniscript::Type>>> scope) {
    for (const auto& func : userDefinedFunctions) {
        DEBUG_LOG("Generating body for function: " + func->name + " (mangled: " + func->mangledName + ")");
        auto llvmFunc = currentModule->getFunction(func->mangledName);
        if (!llvmFunc) {
            console.error("Function not found in module during body generation: " + func->mangledName);
            continue;
        }

        generateFunctionBody(
            func->mangledName,
            llvmFunc,
            func->parameters,
            func->body,
            scope
        );
    }
}

llvm::Value* IRGenerator::codegen(std::shared_ptr<Omniscript::Expression> value, std::shared_ptr<SymbolTable<std::shared_ptr<Omniscript::Expression>, std::shared_ptr<Omniscript::Type>>> scope) {
    DEBUG_LOG();
    DEBUG_LOG("Calling codegen on scope '" + scope->getName() + "' for '" + value->toString() + "'.");
    llvm::Value* result = codegenPrimitive(value, scope);

    if (result) {
        return result;
    }

    // Handle VariableAssignment
    if (auto varAssign = std::dynamic_pointer_cast<Omniscript::VariableAssignment>(value)) {
        DEBUG_LOG("Assigning variable " + varAssign->variableName + " of type " + varAssign->getType()->toString());
        llvm::Type* type = resolveLLVMType(varAssign->getType());
        DEBUG_LOG("Variable '" + varAssign->variableName + "' has type '" + debugType(type) + "'.");
        llvm::Value* value = codegen(varAssign->getValue(), scope);
        DEBUG_LOG("Got variable '" + varAssign->variableName + "''s value.");
        return assignVariable(
            varAssign->variableName,
            type,
            value, 
            varAssign->isGlobal,
            varAssign->isConstant
        );
    }

    if (auto castExpr = std::dynamic_pointer_cast<Omniscript::CastExpression>(value)) {
        DEBUG_LOG("Generating cast from type " + castExpr->targetExpr->getType()->toString() + " to " + castExpr->type->toString());

        llvm::Value* src = codegen(castExpr->targetExpr, scope);
        if (!src) return nullptr;

        llvm::Type* destType = resolveLLVMType(castExpr->type);
        return generateCast(src, destType);
    }

    // Handle ReferenceValue
    if (auto refValue = std::dynamic_pointer_cast<Omniscript::ReferenceExpression>(value)) {
        DEBUG_LOG("Creating reference to variable " + refValue->referentName);
        return getReferenceToVariable(refValue->referentName);
    }
    
    if (auto addressOf = std::dynamic_pointer_cast<Omniscript::AddressOfExpression>(value)) {
        DEBUG_LOG("Getting the address of variable " + addressOf->variableName);
        return getAddressOf(addressOf->variableName);
    }

    if (auto null = std::dynamic_pointer_cast<Omniscript::NullExpression>(value)) {
        DEBUG_LOG("Creating a null value");
        return createNullValue();
    }
    
    if (auto block = std::dynamic_pointer_cast<Omniscript::BlockExpression>(value)) {
        DEBUG_LOG("Evaluating a block value — First pass (registration)");

        for (const auto& expr : block->values) {
            if (auto func = std::dynamic_pointer_cast<Omniscript::FunctionExpression>(expr)) {
                DEBUG_LOG("Processing function declaration: " + func->name + " (mangled: " + func->mangledName + ")");
                llvm::Type* returnType = resolveLLVMType(func->returnType);

                if (func->isExtern) {
                    createExternFunction(
                        func->mangledName,
                        func->externName,
                        func->libPath,
                        returnType,
                        func->parameters,
                        func->isVarArg,
                        func->isStatic
                    );
                } else if (func->isIntrinsic) {
                    std::string nameWithoutPrefix = func->intrinsicName;
                    const std::string prefix = "intrinsic_";
                    if (nameWithoutPrefix.rfind(prefix, 0) == 0)
                        nameWithoutPrefix = nameWithoutPrefix.substr(prefix.size());

                    createIntrinsicFunction(
                        func->mangledName,
                        "llvm." + nameWithoutPrefix,
                        returnType
                    );
                } else {
                    // Normal user-defined function: register only
                    registerFunction(
                        func->mangledName,
                        returnType,
                        func->parameters,
                        scope,
                        func->isVarArg
                    );
                    userDefinedFunctions.push_back(func);
                }
            }
        }

        // Codegen other non-function expressions in the block
        for (const auto& expr : block->values) {
            if (std::dynamic_pointer_cast<Omniscript::FunctionExpression>(expr)) {
                continue;
            }

            if (auto ret = std::dynamic_pointer_cast<Omniscript::ReturnExpression>(expr)) {
                if (ret->getType()) {
                    if (!ret->getType()->isVoidLike()) {
                        return codegen(expr, scope);
                    }
                } else {
                    console.error("The return type has no type");
                }
            } 

            if (auto varAssign = std::dynamic_pointer_cast<Omniscript::VariableAssignment>(expr)) {
                if (!varAssign->isStatic) {
                    varAssign->isGlobal = block->isGlobal;
                }
            }

            codegen(expr, scope);
        }

        return nullptr;
    }

    if (auto nullpointer = std::dynamic_pointer_cast<Omniscript::NullPointerExpression>(value)) {
        DEBUG_LOG("Creating a null pointer");
        return createNullPointer();
    }

    if (auto func = std::dynamic_pointer_cast<Omniscript::FunctionExpression>(value)) {
        DEBUG_LOG("Creating an overload for function " + func->name + " with mangled name '" + func->mangledName + "'");
        llvm::Type* returnType = resolveLLVMType(func->returnType);
        if (func->isExtern) {
            return createExternFunction(
                func->mangledName,
                func->externName,
                func->libPath,
                returnType,
                func->parameters,
                func->isVarArg,
                func->isStatic
            );
        } else if (func->isIntrinsic) {
            std::string nameWithoutPrefix = func->intrinsicName;
            const std::string prefix = "intrinsic_";

            // Strip 'intrinsic_' prefix if present
            if (nameWithoutPrefix.rfind(prefix, 0) == 0) {
                nameWithoutPrefix = nameWithoutPrefix.substr(prefix.size());
            }

            return createIntrinsicFunction(
                func->mangledName,
                "llvm." + nameWithoutPrefix,
                returnType
            );
        }
        registerFunction(
            func->mangledName,
            returnType,
            func->parameters,
            scope,
            func->isVarArg
        );
        userDefinedFunctions.push_back(func);
    }

    if (auto ret = std::dynamic_pointer_cast<Omniscript::ReturnExpression>(value)) {
        DEBUG_LOG("Creating a return statement of kind '" + ret->getType()->toString() + "'.");

        llvm::Type* type = resolveLLVMType(ret->getType());
        llvm::Value* val = codegen(ret->value, scope);
        return createReturn(val, type);
    }

    if (auto unary = std::dynamic_pointer_cast<Omniscript::UnaryExpression>(value)) {
        DEBUG_LOG("Creating a unary expression");
        llvm::Value* operandVal = codegen(unary->operand, scope);
        if (!operandVal) return nullptr;
        return createUnaryExpression(operandVal, unary->op, unary->position);
    }

    if (auto binary = std::dynamic_pointer_cast<Omniscript::BinaryExpression>(value)) {
        DEBUG_LOG("Creating a binary expression");
        llvm::Value* lhs = codegen(binary->left, scope);
        llvm::Value* rhs = codegen(binary->right, scope);
        DEBUG_LOG("lhs type: '" + debugType(lhs->getType()) + "' rhs type: '" + debugType(rhs->getType()) + "'.");
        if (!lhs || !rhs) return nullptr;
        return createBinaryExpression(lhs, binary->op, rhs);
    }

    if (auto ternary = std::dynamic_pointer_cast<Omniscript::TernaryExpression>(value)) {
        DEBUG_LOG("Creating a ternary expression");
        llvm::Value* cond = codegen(ternary->condition, scope);
        llvm::Value* truthy = codegen(ternary->truthy, scope);
        llvm::Value* falsey = codegen(ternary->falsey, scope);
        if (!cond || !truthy || !falsey) return nullptr;
        return createTernaryExpression(cond, truthy, falsey);
    }

    if (auto var = std::dynamic_pointer_cast<Omniscript::VariableAccess>(value)) {
        DEBUG_LOG("Accessing variable: " + var->variableName);
        return getVariable(var->variableName);
    }

    if (auto call = std::dynamic_pointer_cast<Omniscript::CallExpression>(value)) {
        DEBUG_LOG("Calling " + call->calleeName);
        
        std::vector<llvm::Value*> args;
        args.reserve(call->args.size()); // Pre-reserve to avoid reallocations

        for (const auto& arg : call->args) {
            DEBUG_LOG(arg->toString());

            if (auto arr = std::dynamic_pointer_cast<Omniscript::ArrayExpression>(arg); arr && arr->isVariadicArray) {
                // If variadic, reserve space ahead (optional perf tweak)
                args.reserve(args.size() + arr->elements.size());

                for (const auto& element : arr->elements) {
                    if (auto value = codegen(element, scope)) {
                        args.push_back(value);
                    } else {
                        console.error(formatError("Failed to generate code for variadic argument element."));
                    }
                }
            } else {
                if (auto value = codegen(arg, scope)) {
                    args.push_back(value);
                } else {
                    console.error(formatError("Failed to generate code for argument: " + arg->toString()));
                }
            }
        }


        if (call->instanceName.empty()) {
            DEBUG_LOG("Creating a normal call for " + call->calleeName);
            return createCall(call->calleeName, args);
        }
        DEBUG_LOG("Creating an object instance");
        return createObjectInstance(call->calleeName, call->instanceName, args, call->isGlobal);
    }

    if (auto structExpr = std::dynamic_pointer_cast<Omniscript::StructExpression>(value)) {
        std::vector<llvm::Type*> fieldTypes;
        fieldTypes.reserve(structExpr->parameters.size());
        
        int methodsCount = structExpr->parameters.size();

        for (const auto& field : structExpr->parameters) {
            if (auto input = std::dynamic_pointer_cast<Omniscript::FunctionInputExpression>(field)) {
                llvm::Type* llvmFieldType = resolveLLVMType(input->getType());
                if (!llvmFieldType) {
                    console.error("Failed to generate type for field '" + input->name + "' in struct '" + structExpr->name + "'.");
                    return nullptr;
                }
        
                fieldTypes.push_back(llvmFieldType);
                methodsCount--;
            }
        }
        
        // Create the LLVM struct type (opaque or packed depending on your system)
        createStructType(structExpr->name, fieldTypes);

        if (methodsCount > 0) {
            std::vector<llvm::Function*> methods;
    
            methods.reserve(methodsCount);
    
            for (const auto& field : structExpr->parameters) {
                if (auto method = std::dynamic_pointer_cast<Omniscript::FunctionExpression>(field)) {
                    llvm::Type* returnType = resolveLLVMType(method->returnType);
                    llvm::Function* methd = registerFunction(method->mangledName, returnType, method->parameters, scope);
                    methods.emplace_back(methd);
                }
            }
            
            int methodIndex = 0;
            for (const auto& field : structExpr->parameters) {
                if (auto method = std::dynamic_pointer_cast<Omniscript::FunctionExpression>(field)) {
                    generateFunctionBody(
                        method->mangledName,
                        methods[methodIndex],
                        method->parameters,
                        method->body,
                        scope
                    );
                    methodIndex++;
                }
        }
        }

        return nullptr;
    }

    if (auto classExpr = std::dynamic_pointer_cast<Omniscript::ClassExpression>(value)) {
        return codegen(classExpr->structExpr, scope);
    }

    if (auto ifExpr = std::dynamic_pointer_cast<Omniscript::IfExpression>(value)) {
        DEBUG_LOG("Creating an if expression");
        return createIfStatement(ifExpr->conditions, ifExpr->bodies, ifExpr->elseBody, scope);
    }

    if (auto enumExpr = std::dynamic_pointer_cast<Omniscript::EnumExpression>(value)) {
        DEBUG_LOG("Processing EnumExpression for enum '" + enumExpr->enumName + "'");
    
        std::vector<std::string> names;
        std::vector<llvm::Value*> values;
    
        // Reserve space to avoid repeated allocations
        names.reserve(enumExpr->expressionMap.size());
        values.reserve(enumExpr->expressionMap.size());
    
        for (const auto& [name, val] : enumExpr->expressionMap) {
            llvm::Value* enumValue = codegen(val, scope); // Generate LLVM IR for each enum entry
    
            if (enumValue) {
                names.emplace_back(name);
                values.emplace_back(enumValue);
                DEBUG_LOG("Enum '" + enumExpr->enumName + "' has enumerator '" + name + "' with value " + val->toString());
            } else {
                DEBUG_LOG("Failed to generate IR for enumerator '" + name + "'");
            }
        }
    
        // Call the appropriate method depending on flags
        if (enumExpr->isEnumClass && enumExpr->hasLookup) {
            return createEnumClassWithLookup(names, values, enumExpr->enumName, /*isGlobal=*/true);
        } else if (enumExpr->isEnumClass) {
            return createEnumClass(names, values, enumExpr->enumName, /*isGlobal=*/true);
        } else if (enumExpr->hasLookup) {
            return createEnumWithLookup(names, values, enumExpr->enumName, /*isGlobal=*/true);
        } else {
            return createEnum(names, values, enumExpr->enumName, /*isGlobal=*/true);
        }
    }
    
    if (auto forExpr = std::dynamic_pointer_cast<Omniscript::ForLoopExpression>(value)) {
        DEBUG_LOG("Processing ForLoopExpression");
        return createForLoop(forExpr, scope);
    }

    if (auto whileExpr = std::dynamic_pointer_cast<Omniscript::WhileLoopExpression>(value)) {
        DEBUG_LOG("Processing WhileLoopExpression");
        return createWhileLoop(whileExpr, scope);
    }

    // Handle access expressions recursively
    if (auto accessExpr = std::dynamic_pointer_cast<Omniscript::AccessExpression>(value)) {
        return handleAccessExpression(accessExpr, scope);
    }

    if (auto moduleExpr = std::dynamic_pointer_cast<Omniscript::ModuleExpression>(value)) {
        DEBUG_LOG("Processing ModuleExpression: " + moduleExpr->name);
    
        // Create a new scope for the module
        auto moduleScope = std::make_shared<SymbolTable<std::shared_ptr<Omniscript::Expression>, std::shared_ptr<Omniscript::Type>>>(scope);
    
        // Store generated IR values for module members
        std::unordered_map<std::string, llvm::Value*> memberIRValues;
    
        for (const auto& member : moduleExpr->members) {
            std::string memberName = member->name;
            DEBUG_LOG("Generating IR for module member: " + memberName);
    
            llvm::Value* memberValue = codegen(member->value, moduleScope);
            if (!memberValue) {
                console.error("Failed to generate IR for module member: " + memberName);
                continue;
            }
    
            // Save the member value and type
            memberIRValues[memberName] = memberValue;
            // moduleScope->define(memberName, memberExpr->getType());
        }
    
        // Generate the actual module object
        llvm::Value* moduleInstance = createModuleObject(
            moduleExpr->name,
            memberIRValues
        );
    
        return moduleInstance;
    }   

    console.error("Trying to call codegen with an unsupported expression '" + value->toString() + "'.");
    return nullptr;
}

llvm::Value* IRGenerator::codegenPrimitive(std::shared_ptr<Omniscript::Expression> value, std::shared_ptr<SymbolTable<std::shared_ptr<Omniscript::Expression>, std::shared_ptr<Omniscript::Type>>> scope) {

    // Handle 8-bit integer (int8_t)
    if (auto integer8 = std::dynamic_pointer_cast<Omniscript::Integer<int8_t>>(value)) {
        return create8BitInteger(integer8->getValue());
    }
    // Handle 16-bit integer (int16_t)
    else if (auto integer16 = std::dynamic_pointer_cast<Omniscript::Integer<int16_t>>(value)) {
        return create16BitInteger(integer16->getValue());
    }
    // Handle 32-bit integer (int32_t)
    else if (auto integer32 = std::dynamic_pointer_cast<Omniscript::Integer<int32_t>>(value)) {
        return create32BitInteger(integer32->getValue());
    }
    // Handle 64-bit integer (int64_t)
    else if (auto integer64 = std::dynamic_pointer_cast<Omniscript::Integer<int64_t>>(value)) {
        return create64BitInteger(integer64->getValue());

    } else if (auto unsignedInteger8 = std::dynamic_pointer_cast<Omniscript::Integer<uint8_t>>(value)) {
        return createUnsigned8BitInteger(unsignedInteger8->getValue());
    }
    // Handle unsigned 16-bit integer
    else if (auto unsignedInteger16 = std::dynamic_pointer_cast<Omniscript::Integer<uint16_t>>(value)) {
        return createUnsigned16BitInteger(unsignedInteger16->getValue());
    }
    // Handle unsigned 32-bit integer
    else if (auto unsignedInteger32 = std::dynamic_pointer_cast<Omniscript::Integer<uint32_t>>(value)) {
        return createUnsigned32BitInteger(unsignedInteger32->getValue());
    }
    // Handle unsigned 64-bit integer
    else if (auto unsignedInteger64 = std::dynamic_pointer_cast<Omniscript::Integer<uint64_t>>(value)) {
        return createUnsigned64BitInteger(unsignedInteger64->getValue());
    }
    // Handle boolean (bool)
    else if (auto boolean = std::dynamic_pointer_cast<Omniscript::Primitive<bool>>(value)) {
        return createBool(boolean->getValue());
    }
    // Handle half (16-bit floating-point)
    // Target-specific 16-bit float handling
    #ifdef __ARM_ARCH
        else if (auto halfPrimitive = std::dynamic_pointer_cast<Omniscript::Float<__fp16>>(value)) {
            return create16BitFloat(halfPrimitive->getValue());
        }
    #elif defined(__x86_64__) || defined(__i386__)
        else if (auto halfPrimitive = std::dynamic_pointer_cast<Omniscript::Float<_Float16>>(value)) {
            return create16BitFloat(halfPrimitive->getValue());
        }
    #endif
    // Handle float (32-bit floating-point)
    else if (auto floatPrimitive = std::dynamic_pointer_cast<Omniscript::Float<float>>(value)) {
        return create32BitFloat(floatPrimitive->getValue());
    }
    // Handle double (64-bit floating-point)
    else if (auto doublePrimitive = std::dynamic_pointer_cast<Omniscript::Float<double>>(value)) {
        return create64BitFloat(doublePrimitive->getValue());
    }
    // Handle double (64-bit floating-point)
    else if (auto longDoublePrimitive = std::dynamic_pointer_cast<Omniscript::Float<long double>>(value)) {
        return create80BitFloat(longDoublePrimitive->getValue());
    }
    // Handle FP128 (128-bit floating-point)
    else if (auto fp128Primitive = std::dynamic_pointer_cast<Omniscript::Float<__float128>>(value)) {
        return create128BitFloat(fp128Primitive->getValue());
    }
    else if (auto charPrimitive = std::dynamic_pointer_cast<Omniscript::Primitive<char>>(value)) {
        DEBUG_LOG("Creating and int8 char from Primitive<char>");
        return createChar(charPrimitive->getValue());
    }
    // Handle string (std::string)
    else if (auto stringPrimitiveUTF8 = std::dynamic_pointer_cast<Omniscript::Primitive<std::string>>(value)) {
        DEBUG_LOG("Creating UTF-8 string from Primitive<std::string>");
        return createUTF8String(stringPrimitiveUTF8->getValue());
    } else if (auto stringPrimitiveUTF16 = std::dynamic_pointer_cast<Omniscript::Primitive<std::u16string>>(value)) {
        DEBUG_LOG("Creating UTF-16 string from Primitive<std::u16string>");
        return createUTF16String(stringPrimitiveUTF16->getValue());
    } else if (auto stringPrimitiveUTF32 = std::dynamic_pointer_cast<Omniscript::Primitive<std::u32string>>(value)) {
        DEBUG_LOG("Creating UTF-32 string from Primitive<std::u32string>");
        return createUTF32String(stringPrimitiveUTF32->getValue());
    }    
    // Handle BigInt (std::string for now)
    else if (auto bigInt = std::dynamic_pointer_cast<Omniscript::BigInt>(value)) {
        DEBUG_LOG("Creating a Big int");
        return createBigInt(bigInt->getValue(), bigInt->getBitWidth());
    
    } else if (auto arry = std::dynamic_pointer_cast<Omniscript::FixedArrayExpression>(value)) {
        DEBUG_LOG("Creating a fixed array");
        std::vector<llvm::Value*> elems;

        for (const auto elem : arry->elements) {
            DEBUG_LOG("Creating an array element");
            elems.push_back(this->codegen(elem, scope));
        }
        
        llvm::Type* elementType = resolveLLVMType(arry->elementType);

        return createFixedArray(elementType, elems.size(), elems);
    }

    // Return nullptr if no matching type was found
    return nullptr;
}

llvm::Value* IRGenerator::handleAccessExpression(
    std::shared_ptr<Omniscript::AccessExpression> expr, 
    SymbolTableType scope
) {
    // First evaluate the base expression recursively
    llvm::Value* baseValue = nullptr;

    // Handle variable access case
    if (auto varAcc = std::dynamic_pointer_cast<Omniscript::VariableAccess>(expr->expr)) {
        baseValue = activeScope->get(varAcc->variableName);
    } 
    // Handle nested member access in arrow access case (like std.Math->pi)
    else if (auto arrowAccess = std::dynamic_pointer_cast<Omniscript::ArrowAccessExpression>(expr)) {
        if (auto memberAccess = std::dynamic_pointer_cast<Omniscript::MemberAccessExpression>(arrowAccess->expr)) {
            // First get the base value for the member access
            llvm::Value* memberBaseValue = nullptr;
            if (auto innerVarAcc = std::dynamic_pointer_cast<Omniscript::VariableAccess>(memberAccess->expr)) {
                DEBUG_LOG("Getting " + innerVarAcc->variableName);
                memberBaseValue = activeScope->get(innerVarAcc->variableName);
            } else {
                memberBaseValue = codegen(memberAccess->expr, scope);
            }
            
            // Process member access with pointer preservation
            baseValue = handleMemberAccess(memberAccess, memberBaseValue, scope, true);
        } else {
            // Regular arrow access case (ptr->member)
            baseValue = codegen(arrowAccess->expr, scope);
        }
    }
    // All other cases
    else {
        baseValue = codegen(expr->expr, scope);
    }

    if (!baseValue) {
        return nullptr;
    }

    // Handle the current access expression
    if (auto memberAccess = std::dynamic_pointer_cast<Omniscript::MemberAccessExpression>(expr)) {
        return handleMemberAccess(memberAccess, baseValue, scope);
    }
    else if (auto arrowAccess = std::dynamic_pointer_cast<Omniscript::ArrowAccessExpression>(expr)) {
        return handleArrowAccess(arrowAccess, baseValue, scope);
    }
    else if (auto derefAccess = std::dynamic_pointer_cast<Omniscript::DereferenceExpression>(expr)) {
        return handleDereference(derefAccess, baseValue, scope);
    }
    else if (auto indexAccess = std::dynamic_pointer_cast<Omniscript::IndexAccessExpression>(expr)) {
        return handleIndexAccess(indexAccess, baseValue, scope);
    }

    console.error("Unknown access expression type");
    return nullptr;
}

void IRGenerator::generateModule(
    const std::string& modulePath,
    const std::string& alias,
    const std::vector<std::shared_ptr<Statement>>& statements,
    const std::unordered_map<std::string, std::string>& importedAliases, // Alias -> Original
    bool importAll
) {
    
    DEBUG_LOG("Generating module: " + modulePath);
    
    // if (scope->moduleExists(modulePath)) {
    //     DEBUG_LOG("Module " + modulePath + " is already generated. Skipping.");
    //     return;
    // }

    // 🔹 Store public members in a new module symbol table
    // pushScope();

    // auto newModule = std::make_unique<llvm::Module>(modulePath, *Context);
    // llvm::Module* previousModule = currentModule;
    // currentModule = newModule.get();

    // std::unordered_map<std::string, llvm::Value*> publicMembers;

    // for (const auto& statement : statements) {
    //     if (auto moduleStatement = std::dynamic_pointer_cast<CreateModule>(statement)) {
    //         DEBUG_LOG("Generating submodule: " + moduleStatement->getName());
    //         generateModule(moduleStatement->getName(), moduleStatement->getName(), moduleStatement->getStatements(), importedAliases, false);

    //     } else if (auto publicStatement = std::dynamic_pointer_cast<PublicMember>(statement)) {
    //         if (auto moduleStatement = std::dynamic_pointer_cast<CreateModule>(publicStatement->getValue())) {
    //             DEBUG_LOG("Generating submodule: " + moduleStatement->getName());
    //             generateModule(moduleStatement->getName(), moduleStatement->getName(), moduleStatement->getStatements(), importedAliases, false);
    //             continue;
    //         }
    //         std::string originalName = publicStatement->getName();
    //         llvm::Value* value = publicStatement->codegen(*this);
    //         if (!value) {
    //             console.warn("Skipping public statement '" + originalName + "' - no valid IR generated.");
    //             continue;
    //         }

    //         if (llvm::GlobalValue* gv = llvm::dyn_cast<llvm::GlobalValue>(value)) {
    //             if (importAll) {
    //                 gv->setLinkage(llvm::GlobalValue::ExternalLinkage);
    //                 // activeScope->set(originalName, gv);
    //             } else {
    //                 auto it = importedAliases.find(originalName);
    //                 if (it != importedAliases.end()) {
    //                     std::string alias = it->second; // Aliased name
    //                     gv->setLinkage(llvm::GlobalValue::ExternalLinkage);
    //                     DEBUG_LOG("Aliased import: " + alias + " -> " + originalName);
    //                 } else {
    //                     gv->setLinkage(llvm::GlobalValue::InternalLinkage);
    //                 }
    //             }
    //         } else {
    //             console.warn("Warning: Public symbol '" + originalName +
    //                          "' is not a global value and cannot have linkage visibility set.");
    //         }
    //     } else if (auto privateStatement = std::dynamic_pointer_cast<PrivateMember>(statement)) {
    //         DEBUG_LOG("Creating private member " + privateStatement->getName());
    //         llvm::Value* value = privateStatement->codegen(*this);
    //         if (importedAliases.find(privateStatement->getName()) != importedAliases.end()) {
    //             console.warn("'" + privateStatement->getName() + "' is not a public member of '" + modulePath + "' and cannot be imported");
    //         }
    //         if (value) {
    //             if (llvm::GlobalValue* gv = llvm::dyn_cast<llvm::GlobalValue>(value)) {
    //                 gv->setLinkage(llvm::GlobalValue::InternalLinkage);
    //             } else {
    //                 console.warn("Warning: Private statement '" + privateStatement->getName() + "' is not a global value and cannot have linkage visibility set to private or public.");
    //             }
    //         }
    //     }
    // }

    // printErrors(*currentModule);
    // currentModule = previousModule;

    // popScope();
    
    // if (llvm::Linker::linkModules(*Module, std::move(newModule))) {
    //     llvm::errs() << "Error: Linking failed for module " << modulePath << "!\n";
    // }
}

bool IRGenerator::isLoadedModule(const std::string& modulePath) {
    if (generatedModules.find(modulePath) != generatedModules.end()) {
        return true;
    }
    return false;
}

bool IRGenerator::isLoadedModuleMember(const std::string& modulePath, const std::string& memberName) {
    return true;
}

void IRGenerator::activateModuleMembers(const std::vector<std::string>& members) {
    return;
}

void IRGenerator::linkModules() {

}

void IRGenerator::importModule(const std::string& modulePath, const std::vector<std::string>& members) {
    if (modulePublicSymbols.find(modulePath) == modulePublicSymbols.end()) {
        console.error("Module '" + modulePath + "' not found or has no public symbols.");
        return;
    }

    // Import only public members
    for (const auto& [name, value] : modulePublicSymbols[modulePath]) {
        activeScope->set(name, value);
    }
}


llvm::Type* IRGenerator::resolveLLVMType(std::shared_ptr<Omniscript::Type> type) {
    if (!type) {
        std::cerr << "[ERROR] Type is null!" << std::endl;
        return nullptr;
    }

    DEBUG_LOG("Resolving a '" + type->toString() + "'.");
    llvm::LLVMContext& context = *Context;

    if (auto customType = std::dynamic_pointer_cast<Omniscript::UserDefinedType>(type)) {
        auto userType = activeScope->getType(customType->getName());
        if (!userType) {
            console.error("User type is null");
            return nullptr;
        }
        return userType;
    }

    // Todo:: Possibly call the type nullable type
    if (auto nullable = std::dynamic_pointer_cast<Omniscript::NullType>(type)) {
        DEBUG_LOG("Resolving nullable type: " + nullable->toString());

        // Nullable<T> is represented as { i1, T } (i1 = isNull flag)
        if (!nullable->innerType) {
            console.error("[ERROR] Nullable has no inner type ");
        }
        
        llvm::Type* innerLLVMType = resolveLLVMType(nullable->innerType);
        if (!innerLLVMType) {
            console.error("[ERROR] Failed to resolve inner type of nullable: " + nullable->innerType->toString());
            return nullptr;
        }

        // Create struct: { i1 is_null, T value }
        return llvm::StructType::get(*Context, {
            llvm::Type::getInt1Ty(*Context),  // null flag
            innerLLVMType                     // actual value
        });
    }

    if (auto funcType = std::dynamic_pointer_cast<Omniscript::FunctionType>(type)) {
        llvm::Type* returnType = resolveLLVMType(funcType->returnType);
        if (!returnType) {
            console.error("[ERROR] Failed to resolve function return type: " + funcType->getReturnType()->toString());
            return nullptr;
        }

        std::vector<llvm::Type*> paramTypes;
        for (const auto& param : funcType->parameterTypes) {
            llvm::Type* paramLLVMType = resolveLLVMType(param);
            if (!paramLLVMType) {
                console.error("[ERROR] Failed to resolve function parameter type: " + param->toString());
                return nullptr;
            }
            paramTypes.push_back(paramLLVMType);
        }

        // Create the LLVM function type with vararg info
        return llvm::FunctionType::get(returnType, paramTypes, funcType->isVarArg);
    }

    // If the type is an array, resolve the base type first.
    if (type->isArray()) {
        DEBUG_LOG("The array is of size '" + std::to_string(type->fixedSize) + "' and holds type " + type->elementType->toString() + "'.");
        auto elementType = resolveLLVMType(type->elementType);
        uint64_t arraySize = type->fixedSize;
        return llvm::ArrayType::get(elementType, arraySize);
    }

    // If the type is a pointer, resolve the base type and add pointer depth.
    if (type->isPointer()) {
        if (auto pointer = std::dynamic_pointer_cast<Omniscript::PointerType>(type)) {
            int pointerDepth;
            llvm::Type* pointeeType;
            pointeeType = resolveLLVMType(pointer->getBasePointeeType());
            pointerDepth = pointer->getPointerDepth();
            return llvm::PointerType::get(pointeeType, pointerDepth);
        }

        if (auto nullpointer = std::dynamic_pointer_cast<Omniscript::NullPointerType>(type)) {
            return llvm::PointerType::getUnqual(llvm::Type::getInt8Ty(context));
        }
        
    }

    // If the type is a reference, treat it as a pointer.
    if (type->isReference()) {
        auto referencedType = resolveLLVMType(type->getReferencedType());
        return llvm::PointerType::get(referencedType, 0);
    }

    // Resolve the base type kind
    llvm::Type* llvmType = nullptr;

    switch (type->getKind()) {
        case Omniscript::Kind::Int8:
            DEBUG_LOG("Generating LLVM type: Int8");
            llvmType = llvm::Type::getInt8Ty(context);
            break;
        case Omniscript::Kind::Int16:
            DEBUG_LOG("Generating LLVM type: Int16");
            llvmType = llvm::Type::getInt16Ty(context);
            break;
        case Omniscript::Kind::Int32:
            DEBUG_LOG("Generating LLVM type: Int32");
            llvmType = llvm::Type::getInt32Ty(context);
            break;
        case Omniscript::Kind::Int64:
            DEBUG_LOG("Generating LLVM type: Int64");
            llvmType = llvm::Type::getInt64Ty(context);
            break;
        case Omniscript::Kind::Int128:
            DEBUG_LOG("Generating LLVM type: Int128");
            llvmType = llvm::IntegerType::get(context, 128);
            break;
        case Omniscript::Kind::Int256:
            DEBUG_LOG("Generating LLVM type: Int256");
            llvmType = llvm::IntegerType::get(context, 256);
            break;
        case Omniscript::Kind::Int512:
            DEBUG_LOG("Generating LLVM type: Int512");
            llvmType = llvm::IntegerType::get(context, 512);
            break;
        case Omniscript::Kind::Int1024:
            DEBUG_LOG("Generating LLVM type: Int1024");
            llvmType = llvm::IntegerType::get(context, 1024);
            break;
        case Omniscript::Kind::BigInt:
            DEBUG_LOG("Generating LLVM type: BigInt (treated as Int1024)");
            llvmType = llvm::IntegerType::get(context, 1024);
            break;
        case Omniscript::Kind::Size_t: {
            int pointerWidth = Omniscript::getPointerBitWidth();
            DEBUG_LOG("Generating LLVM type: Size_t (treated as Int " + std::to_string(pointerWidth) + ")");
            llvmType = llvm::IntegerType::get(context, pointerWidth);
            break;
        }
        case Omniscript::Kind::UInt8:
            DEBUG_LOG("Generating LLVM type: UInt8");
            llvmType = llvm::Type::getInt8Ty(context);
            break;
        case Omniscript::Kind::UInt16:
            DEBUG_LOG("Generating LLVM type: UInt16");
            llvmType = llvm::Type::getInt16Ty(context);
            break;
        case Omniscript::Kind::UInt32:
            DEBUG_LOG("Generating LLVM type: UInt32");
            llvmType = llvm::Type::getInt32Ty(context);
            break;
        case Omniscript::Kind::UInt64:
            DEBUG_LOG("Generating LLVM type: UInt64");
            llvmType = llvm::Type::getInt64Ty(context);
            break;
        case Omniscript::Kind::UInt128:
            DEBUG_LOG("Generating LLVM type: UInt128");
            llvmType = llvm::IntegerType::get(context, 128);
            break;
        case Omniscript::Kind::UInt256:
            DEBUG_LOG("Generating LLVM type: UInt256");
            llvmType = llvm::IntegerType::get(context, 256);
            break;
        case Omniscript::Kind::UInt512:
            DEBUG_LOG("Generating LLVM type: UInt512");
            llvmType = llvm::IntegerType::get(context, 512);
            break;
        case Omniscript::Kind::UInt1024:
            DEBUG_LOG("Generating LLVM type: UInt1024");
            llvmType = llvm::IntegerType::get(context, 1024);
            break;
        case Omniscript::Kind::Half:
            DEBUG_LOG("Generating LLVM type: Half");
            llvmType = llvm::Type::getHalfTy(context);
            break;
        case Omniscript::Kind::Float:
            DEBUG_LOG("Generating LLVM type: Float");
            llvmType = llvm::Type::getFloatTy(context);
            break;
        case Omniscript::Kind::Double:
            DEBUG_LOG("Generating LLVM type: Double");
            llvmType = llvm::Type::getDoubleTy(context);
            break;
        case Omniscript::Kind::FP128:
            DEBUG_LOG("Generating LLVM type: FP128");
            llvmType = llvm::Type::getFP128Ty(context);
            break;
        case Omniscript::Kind::X86_FP80:
            DEBUG_LOG("Generating LLVM type: X86_FP80");
            llvmType = llvm::Type::getX86_FP80Ty(context);
            break;
        case Omniscript::Kind::PPC_FP128:
            DEBUG_LOG("Generating LLVM type: PPC_FP128");
            llvmType = llvm::Type::getPPC_FP128Ty(context);
            break;
        case Omniscript::Kind::Char:
            DEBUG_LOG("Generating LLVM type: Char (as Int8)");
            llvmType = llvm::Type::getInt8Ty(context);
            break;
        case Omniscript::Kind::Char16:
            DEBUG_LOG("Generating LLVM type: Char16 (as Int16)");
            llvmType = llvm::Type::getInt16Ty(context);
            break;
        case Omniscript::Kind::Char32:
            DEBUG_LOG("Generating LLVM type: Char32 (as Int32)");
            llvmType = llvm::Type::getInt32Ty(context);
            break;
        case Omniscript::Kind::Bool:
            DEBUG_LOG("Generating LLVM type: Bool (as Int1)");
            llvmType = llvm::Type::getInt1Ty(context);
            break;
        case Omniscript::Kind::Void:
            DEBUG_LOG("Generating LLVM type: Void");
            llvmType = llvm::Type::getVoidTy(context);
            break;
        case Omniscript::Kind::Null:
            DEBUG_LOG("Generating LLVM type: Null (as pointer to i8)");
            llvmType = llvm::PointerType::getUnqual(llvm::Type::getInt8Ty(context));
            break;
        case Omniscript::Kind::Utf8:
            DEBUG_LOG("Generating LLVM type: Utf8 (i8*)");
            llvmType = llvm::PointerType::get(llvm::Type::getInt8Ty(context), 0);
            break;
        case Omniscript::Kind::Utf16:
            DEBUG_LOG("Generating LLVM type: Utf16 (i16*)");
            llvmType = llvm::PointerType::get(llvm::Type::getInt16Ty(context), 0);
            break;
        case Omniscript::Kind::Utf32:
            DEBUG_LOG("Generating LLVM type: Utf32 (i32*)");
            llvmType = llvm::PointerType::get(llvm::Type::getInt32Ty(context), 0);
            break;
        default:
            console.error("[ERROR] Unknown type: " + type->toString());
            return nullptr;
    }
    

    return llvmType;
}

llvm::Value* IRGenerator::createNullPointer() {
    return llvm::ConstantPointerNull::get(llvm::PointerType::getUnqual(llvm::Type::getInt8Ty(*Context)));
}

llvm::Value* IRGenerator::createNullValue() {
    return llvm::UndefValue::get(llvm::Type::getVoidTy(*Context));
}

// Generate IR for different types
// ============================== Generate IR for Numeric Types ============================== //
// Create an 8-bit integer (i8)
llvm::Value* IRGenerator::create8BitInteger(int8_t value) {
    return llvm::ConstantInt::get(llvm::Type::getInt8Ty(*Context), value, true);
}

// Create a 16-bit integer (i16)
llvm::Value* IRGenerator::create16BitInteger(int16_t value) {
    return llvm::ConstantInt::get(llvm::Type::getInt16Ty(*Context), value, true);
}

// Create a 32-bit integer (i32)
llvm::Value* IRGenerator::create32BitInteger(int32_t value) {
    return llvm::ConstantInt::get(llvm::Type::getInt32Ty(*Context), value, true);
}

// Create a 64-bit integer (i64)
llvm::Value* IRGenerator::create64BitInteger(int64_t value) {
    return llvm::ConstantInt::get(llvm::Type::getInt64Ty(*Context), value, true);
}

llvm::Value* IRGenerator::create80BitFloat(long double value) {
    llvm::LLVMContext& C = *Context;

    #if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
        // On x86/x64 GCC/Clang: long double is 80-bit (x87 extended precision)
        if (sizeof(long double) == 10 || sizeof(long double) == 12 || sizeof(long double) == 16) {
            llvm::Type* f80Type = llvm::Type::getX86_FP80Ty(C);

            // Zero out buffer to avoid garbage in high bits
            uint8_t buffer[16] = {};
            std::memcpy(buffer, &value, sizeof(long double));

            // Extract 80-bit value: low 64 bits + upper 16 bits
            uint64_t low = *reinterpret_cast<const uint64_t*>(buffer);
            uint16_t high = *reinterpret_cast<const uint16_t*>(buffer + 8);
            llvm::APInt api(80, {low, static_cast<uint64_t>(high)});
            llvm::APFloat apf(llvm::APFloat::x87DoubleExtended(), api);
            return llvm::ConstantFP::get(C, apf);
        }
    #endif

    #if defined(__aarch64__) || defined(__powerpc64__) || defined(__wasm__) || defined(_MSC_VER)
        // Common fallback: long double is 64-bit or 128-bit IEEE (same as double or quadfloat)
        if (sizeof(long double) == 8) {
            llvm::Type* f64Type = llvm::Type::getDoubleTy(C);
            return llvm::ConstantFP::get(f64Type, static_cast<double>(value));
        } else if (sizeof(long double) == 16) {
            llvm::Type* f128Type = llvm::Type::getFP128Ty(C);

            uint8_t buffer[16] = {};
            std::memcpy(buffer, &value, 16);
            llvm::APInt api(128, { 
                *reinterpret_cast<const uint64_t*>(&buffer[0]), 
                *reinterpret_cast<const uint64_t*>(&buffer[8]) 
            });
            llvm::APFloat apf(llvm::APFloat::IEEEquad(), api);
            return llvm::ConstantFP::get(C, apf);
        }
    #endif

    // If platform unknown or unsupported long double layout
    console.error("Unsupported long double format on this platform.\n");
    return nullptr;
    // llvm::errs() << "Unsupported long double format on this platform.\n";
    // std::abort();
    // return nullptr;
}

 // Target-specific 16-bit float handling
#ifdef __ARM_ARCH
llvm::Value* IRGenerator::create16BitFloat(__fp16 value) {
    llvm::Type* type = llvm::Type::getHalfTy(*Context); // LLVM 16-bit float type
    return llvm::ConstantFP::get(type, static_cast<double>(value)); // safe conversion
}

#elif defined(__x86_64__) || defined(__i386__)
llvm::Value* IRGenerator::create16BitFloat(_Float16 value) {
    llvm::Type* type = llvm::Type::getHalfTy(*Context);
    return llvm::ConstantFP::get(type, static_cast<double>(value));
}
#endif

// Create a 32-bit floating-point (float)
llvm::Value* IRGenerator::create32BitFloat(float value) {
    return llvm::ConstantFP::get(llvm::Type::getFloatTy(*Context), value);
}

// Create a 64-bit floating-point (double)
llvm::Value* IRGenerator::create64BitFloat(double value) {
    return llvm::ConstantFP::get(llvm::Type::getDoubleTy(*Context), value);
}

// Create a 128-bit floating-point (FP128)
llvm::Value* IRGenerator::create128BitFloat(__float128 value) {
    return llvm::ConstantFP::get(llvm::Type::getFP128Ty(*Context), static_cast<double>(value));  // FP128 is typically represented as a double in LLVM
}

// Create an arbitrary-precision integer (BigInt)
llvm::Value* IRGenerator::createBigInt(const std::string& str, unsigned bitSize) {
    // #if SIMD_OPTIMIZATION_LEVEL == 512
    //     return createBigIntAVX512(str, bitSize);
    // #elif SIMD_OPTIMIZATION_LEVEL == 256
    //     return createBigIntAVX2(str, bitSize);
    // #elif SIMD_OPTIMIZATION_LEVEL == 128
    //     return createBigIntAVX(str, bitSize);
    // #else
    //     llvm::APInt bigIntValue(bitSize, str, 10);
    //     return llvm::ConstantInt::get(llvm::Type::getIntNTy(*Context, bitSize), bigIntValue);
    // #endif
    llvm::APInt bigIntValue(bitSize, str, 10);
    return llvm::ConstantInt::get(llvm::Type::getIntNTy(*Context, bitSize), bigIntValue);
}

llvm::Value* IRGenerator::createBigIntAVX512(const std::string& str, unsigned bitSize) {
    // __m512i bigIntVec = _mm512_setzero_si512(); // Zero-initialize 512-bit register
    return createBigInt(str, 512);  // Use this as placeholder for now
}

llvm::Value* IRGenerator::createBigIntAVX2(const std::string& str, unsigned bitSize) {
    // __m256i bigIntVec = _mm256_setzero_si256(); // Zero-initialize 256-bit register
    return createBigInt(str, 256);
}

llvm::Value* IRGenerator::createBigIntAVX(const std::string& str, unsigned bitSize) {
    __m128i bigIntVec = _mm_setzero_si128(); // Zero-initialize 128-bit register
    return createBigInt(str, 128);
}

llvm::Value* IRGenerator::createUnsigned8BitInteger(uint8_t value) {
    return llvm::ConstantInt::get(llvm::Type::getInt8Ty(*Context), value, false);  // Unsigned 8-bit integer
}

llvm::Value* IRGenerator::createUnsigned16BitInteger(uint16_t value) {
    return llvm::ConstantInt::get(llvm::Type::getInt16Ty(*Context), value, false);  // Unsigned 16-bit integer
}

llvm::Value* IRGenerator::createUnsigned32BitInteger(uint32_t value) {
    return llvm::ConstantInt::get(llvm::Type::getInt32Ty(*Context), value, false);  // Unsigned 32-bit integer
}

llvm::Value* IRGenerator::createUnsigned64BitInteger(uint64_t value) {
    return llvm::ConstantInt::get(llvm::Type::getInt64Ty(*Context), value, false);  // Unsigned 64-bit integer
}


// Create an 8-bit character (char)
llvm::Value* IRGenerator::createChar(char value) {
    return llvm::ConstantInt::get(llvm::Type::getInt8Ty(*Context), value, true);
}

// Create a 16-bit character (char16_t)
llvm::Value* IRGenerator::createChar16(char16_t value) {
    return llvm::ConstantInt::get(llvm::Type::getInt16Ty(*Context), value, false);
}

// Create a 32-bit character (char32_t)
llvm::Value* IRGenerator::createChar32(char32_t value) {
    return llvm::ConstantInt::get(llvm::Type::getInt32Ty(*Context), value, false);
}

// Create an 8-bit (UTF-8) string
llvm::Value* IRGenerator::createUTF8String(const std::string& str) {
    return Builder->CreateGlobalString(str, ".utf8str");
}

// Create a 16-bit (UTF-16) string
llvm::Value* IRGenerator::createUTF16String(const std::u16string& str) {
    std::vector<llvm::Constant*> chars;
    for (char16_t c : str) {
        chars.push_back(llvm::ConstantInt::get(llvm::Type::getInt16Ty(*Context), c, false));
    }
    chars.push_back(llvm::ConstantInt::get(llvm::Type::getInt16Ty(*Context), 0)); // Null terminator

    llvm::ArrayType* arrayType = llvm::ArrayType::get(llvm::Type::getInt16Ty(*Context), chars.size());
    llvm::Constant* array = llvm::ConstantArray::get(arrayType, chars);

    auto global = new llvm::GlobalVariable(
        *Module, arrayType, true, llvm::GlobalValue::PrivateLinkage, array, ".utf16str");

    return global;
}

// Create a 32-bit (UTF-32) string
llvm::Value* IRGenerator::createUTF32String(const std::u32string& str) {
    std::vector<llvm::Constant*> chars;
    for (char32_t c : str) {
        chars.push_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(*Context), c, false));
    }
    chars.push_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(*Context), 0)); // Null terminator

    llvm::ArrayType* arrayType = llvm::ArrayType::get(llvm::Type::getInt32Ty(*Context), chars.size());
    llvm::Constant* array = llvm::ConstantArray::get(arrayType, chars);

    auto global = new llvm::GlobalVariable(
        *Module, arrayType, true, llvm::GlobalValue::PrivateLinkage, array, ".utf32str");

    return global;
}

llvm::Value* IRGenerator::createBool(bool value) {
    return llvm::ConstantInt::get(llvm::Type::getInt1Ty(*Context), value ? 1 : 0, false);
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

void IRGenerator::scheduleGlobalInitialization(
    const std::string& name,
    llvm::GlobalVariable* gVar,
    llvm::Value* initialValue
) {
    // Create initialization in global constructor
    llvm::IRBuilder<> initBuilder(
        Module->getContext()
    );
    auto* initFunc = getOrCreateGlobalInitFunction();
    auto* entry = &initFunc->getEntryBlock();
    
    if (entry->empty()) {
        initBuilder.SetInsertPoint(entry);
    } else {
        initBuilder.SetInsertPoint(
            entry, 
            std::prev(entry->end())
        );
    }

    // Store the initial value
    initBuilder.CreateStore(initialValue, gVar);
}

void IRGenerator::finalizeGlobalInitializers() {
    if (globalInitializers.empty()) return;

    auto* func = getOrCreateGlobalInitFunction();
    auto* entry = &func->getEntryBlock();
    
    // Set insertion point at end of entry block
    Builder->SetInsertPoint(entry, entry->end());

    // Emit all initializers
    for (auto& init : globalInitializers) {
        if (init.value->getType() != init.variable->getValueType()) {
            init.value = Builder->CreateBitCast(init.value, init.variable->getValueType());
        }
        Builder->CreateStore(init.value, init.variable);
    }

    globalInitializers.clear();
}

llvm::Value* IRGenerator::createFixedArray(
    llvm::Type* elementType,
    size_t size,
    const std::vector<llvm::Value*>& elements
) {

    DEBUG_LOG("Creating fixed array of size " + std::to_string(size));
    DEBUG_LOG("Element LLVM type: " + debugType(elementType));

    // Set insert point before terminator if present
    llvm::BasicBlock* currentBlock = Builder->GetInsertBlock();
    if (currentBlock && !currentBlock->empty() && currentBlock->back().isTerminator()) {
        DEBUG_LOG("Found terminator in current block, inserting before it");
        auto termIt = currentBlock->getTerminator()->getIterator();
        Builder->SetInsertPoint(currentBlock, termIt);
    }

    const llvm::DataLayout& dataLayout = currentModule != nullptr 
        ? currentModule->getDataLayout() 
        : Module->getDataLayout();

    unsigned elementAlign = dataLayout.getABITypeAlign(elementType).value();
    DEBUG_LOG("Element alignment: " + std::to_string(elementAlign));

    llvm::ArrayType* arrayType = llvm::ArrayType::get(elementType, size);
    DEBUG_LOG("Array LLVM type: " + debugType(arrayType));

    llvm::Value* arrayAlloc = Builder->CreateAlloca(arrayType, nullptr, "static_array");
    DEBUG_LOG("Array allocation created");

    llvm::cast<llvm::AllocaInst>(arrayAlloc)->setAlignment(llvm::Align(elementAlign));
    DEBUG_LOG("Set alignment of allocation");

    for (size_t i = 0; i < elements.size(); ++i) {
        DEBUG_LOG("Inserting element at index " + std::to_string(i));

        llvm::Value* element = elements[i];

        if (element->getType() != elementType) {
            DEBUG_LOG("Type mismatch: expected " + debugType(elementType) +
                      ", got " + debugType(element->getType()));
            element = Builder->CreateBitCast(element, elementType);
            DEBUG_LOG("Bitcasted element to match element type");
        }

        llvm::Value* elementPtr = Builder->CreateGEP(
            arrayType,
            arrayAlloc,
            {Builder->getInt32(0), Builder->getInt32(i)}
        );
        DEBUG_LOG("Computed GEP for index " + std::to_string(i));

        llvm::StoreInst* store = Builder->CreateStore(element, elementPtr);
        store->setAlignment(llvm::Align(elementAlign));
        DEBUG_LOG("Stored element at index " + std::to_string(i));
    }

    DEBUG_LOG("Finished creating fixed array");
    return arrayAlloc;
}

// When processing a function call:
llvm::Value* IRGenerator::createCall(
    const std::string& callee, 
    std::vector<llvm::Value*>& args, 
    llvm::BasicBlock* activeBlock
) {
    // 1. Look up function
    llvm::Function* func = nullptr;

    if (auto moduleFunc = Module->getFunction(callee)) {
        func = moduleFunc;
    } else {
        if (auto value = activeScope->get(callee)) {
            func = llvm::dyn_cast<llvm::Function>(value);
        }
    }

    if (!func) {
        console.error("Function '" + callee + "' was not found in scope '" + activeScope->getName() + "'");
    }
    
    // 2. Verify argument count (allow extra args if the function is variadic)
    auto *funcType = func->getFunctionType();
    bool isVarArg = funcType->isVarArg();

    size_t fixedParams = funcType->getNumParams();
    size_t givenArgs  = args.size();

    if (!isVarArg) {
        // non-variadic: must match exactly
        if (fixedParams != givenArgs) {
            console.error("Argument count mismatch for '" + callee + "', expected " +
                        std::to_string(fixedParams) + " but got " +
                        std::to_string(givenArgs));
            return nullptr;
        }
    } else {
        // variadic: must have *at least* the fixed params
        if (givenArgs < fixedParams) {
            console.error("Argument count mismatch for variadic '" + callee +
                        "', expected at least " + std::to_string(fixedParams) +
                        " but got " + std::to_string(givenArgs));
            return nullptr;
        }
    }


    // 3. Type checking and casting
    for (size_t i = 0; i < fixedParams; ++i) {
        llvm::Type* expected = funcType->getParamType(i);
        if (args[i]->getType() != expected) {
            llvm::Value* castedArg = castValue(args[i], expected);
            if (!castedArg) {
                console.error("Type mismatch for argument " + std::to_string(i) +
                            " in call to '" + callee + "'");
                return nullptr;
            }
            args[i] = castedArg;
        }
    }

    // 4. Determine insertion point safely
    llvm::BasicBlock* insertBlock = activeBlock ? activeBlock : Builder->GetInsertBlock();
    if (!insertBlock) {
        console.error("No valid insert block found for function call");
        return nullptr;
    }

    // 5. Find safe insertion point (BEFORE any terminator)
    if (insertBlock->getTerminator()) {
        // If block already has a terminator, insert right before it
        Builder->SetInsertPoint(insertBlock->getTerminator());
    } else {
        // Otherwise insert at end of block
        Builder->SetInsertPoint(insertBlock);
    }

    // 6. Generate call
    llvm::Value* callInst = Builder->CreateCall(func, args);

    // 7. Ensure we didn't accidentally insert after terminator
    // #ifdef LLVM_DEBUG
    // if (insertBlock->getTerminator() && 
    //     callInst->getNextNode() != insertBlock->getTerminator()) {
    //     llvm::errs() << "WARNING: Call instruction inserted after terminator!\n";
    // }
    // #endif

    return callInst;
}

llvm::AllocaInst* IRGenerator::createEntryBlockAlloca(llvm::Function* function,llvm::Type* type, const std::string& name) {
    llvm::IRBuilder<> tmpBuilder(&function->getEntryBlock(),
    function->getEntryBlock().begin());
    return tmpBuilder.CreateAlloca(type, nullptr, name);
}

llvm::Value* IRGenerator::createReturn(llvm::Value* returnValue, llvm::Type* expectedReturnType) {
    // Get current function and verify we're in a function context
    llvm::Function* currentFunction = Builder->GetInsertBlock()->getParent();
    if (!currentFunction) {
        console.error("Return statement outside function");
        return nullptr;
    }

    if (!Module) {
        console.error("LLVM Module is null");
        return nullptr;
    }
    
    // Insert va_end call before return, only if va_list was created
    if (activeScope->get("va_list")) {
        llvm::Function* vaEndFunc = llvm::Intrinsic::getOrInsertDeclaration(Module.get(), llvm::Intrinsic::vaend);
        llvm::Value* vaListAlloca = activeScope->get("va_list");
        Builder->CreateCall(vaEndFunc, { vaListAlloca });
    }

    // Handle void returns
    if (currentFunction->getReturnType()->isVoidTy()) {
        if (returnValue) {
            console.error("Void function cannot return a value");
        }
        return Builder->CreateRetVoid();
    }

    // Handle value returns
    if (!returnValue) {
        console.error("Non-void function must return a value");
    }

    // Ensure type compatibility
    if (returnValue->getType() != expectedReturnType) {
        console.error("Return type mismatch: expected " + 
            debugType(expectedReturnType) + ", got " + 
            debugType(returnValue->getType()));
    }

    // Create the return instruction
    return Builder->CreateRet(returnValue);
}


bool IRGenerator::currentBlockHasTerminator() const {
    return Builder->GetInsertBlock()->getTerminator() != nullptr;
}

llvm::Value* IRGenerator::castValue(llvm::Value* val, llvm::Type* targetType) {
    return val;
    // if (val->getType() == targetType) return val;

    // if (val->getType()->isIntegerTy() && targetType->isIntegerTy()) {
    //     return Builder->CreateIntCast(val, targetType, /*isSigned=*/true);
    // }

    // if (val->getType()->isFloatingPointTy() && targetType->isFloatingPointTy()) {
    //     return Builder->CreateFPCast(val, targetType);
    // }

    // DEBUG_LOG("Unsupported cast from " + debugType(val->getType()) + " to " + debugType(targetType));
    // return nullptr;
}


std::string IRGenerator::typeToString(llvm::Type* type) {
    std::string typeStr;
    llvm::raw_string_ostream rso(typeStr);
    type->print(rso);
    return rso.str();
}

llvm::Value* IRGenerator::createUnaryExpression(llvm::Value* operand, TokenTypes op, bool isPostfix) {
    if (!operand) {
        console.error("Unknown unary operation");
        return nullptr;
    };

    // Handle numeric operations (both integer and floating point)
    auto handleNumericUnary = [&](auto createOp) -> llvm::Value* {
        if (operand->getType()->isIntegerTy()) {
            return createOp(operand, "unarytmp");
        } else if (operand->getType()->isFloatingPointTy()) {
            return createOp(operand, "funarytmp");
        }
        return nullptr;
    };

    switch (op) {
        case TokenTypes::Plus:
            return operand; // +x is just x

        case TokenTypes::Minus:
            return handleNumericUnary([&](auto val, auto name) {
                return val->getType()->isIntegerTy() 
                    ? Builder->CreateNeg(val, name)
                    : Builder->CreateFNeg(val, name);
            });

        case TokenTypes::LogicalNot:
            return Builder->CreateNot(Builder->CreateICmpNE(
                operand, 
                llvm::ConstantInt::get(operand->getType(), 0),
                "booltmp"
            ), "nottmp");

        case TokenTypes::Tilde:
            return Builder->CreateNot(operand, "bwnottmp");

        case TokenTypes::Increment:
        case TokenTypes::Decrement: {
            llvm::Value* one = llvm::ConstantInt::get(operand->getType(), 1);
            llvm::Value* delta = (op == TokenTypes::Increment) ? one : Builder->CreateNeg(one, "deltatmp");
            llvm::Value* newVal = Builder->CreateAdd(operand, delta, "incdec");
            
            // For variables, store the new value
            if (auto* load = llvm::dyn_cast<llvm::LoadInst>(operand)) {
                Builder->CreateStore(newVal, load->getPointerOperand());
            }
            
            return isPostfix ? operand : newVal;
        }

        default:
            console.error("Unknown unary operator");
            return nullptr;
    }
    return nullptr;
}

llvm::Value* IRGenerator::createBinaryExpression(llvm::Value* left, TokenTypes op, llvm::Value* right) {
    if (!left || !right) {
        console.error("Unknown binary operation");
        return nullptr;
    };

    switch (op) {
        // Arithmetic
        case TokenTypes::Plus:
            return left->getType()->isFloatingPointTy()
                ? Builder->CreateFAdd(left, right, "faddtmp")
                : Builder->CreateAdd(left, right, "addtmp");
            
        case TokenTypes::Minus:
            return left->getType()->isFloatingPointTy()
                ? Builder->CreateFSub(left, right, "fsubtmp")
                : Builder->CreateSub(left, right, "subtmp");
            
        case TokenTypes::Multiply:
            return left->getType()->isFloatingPointTy()
                ? Builder->CreateFMul(left, right, "fmultmp")
                : Builder->CreateMul(left, right, "multmp");
            
        case TokenTypes::Divide:
            return left->getType()->isFloatingPointTy()
                ? Builder->CreateFDiv(left, right, "fdivtmp")
                : Builder->CreateSDiv(left, right, "divtmp");
            
        case TokenTypes::Modulo:
            return left->getType()->isFloatingPointTy()
                ? Builder->CreateFRem(left, right, "fmodtmp")
                : Builder->CreateSRem(left, right, "modtmp");
            
        // Comparisons
        case TokenTypes::Equals:
            return left->getType()->isFloatingPointTy()
                ? Builder->CreateFCmpOEQ(left, right, "feqtmp")
                : Builder->CreateICmpEQ(left, right, "eqtmp");
            
        case TokenTypes::NotEquals:
            return left->getType()->isFloatingPointTy()
                ? Builder->CreateFCmpONE(left, right, "fnetmp")
                : Builder->CreateICmpNE(left, right, "netmp");
            
        case TokenTypes::LessThan:
            return left->getType()->isFloatingPointTy()
                ? Builder->CreateFCmpOLT(left, right, "flttmp")
                : Builder->CreateICmpSLT(left, right, "lttmp");
            
        case TokenTypes::LessEqual:
            return left->getType()->isFloatingPointTy()
                ? Builder->CreateFCmpOLE(left, right, "fletmp")
                : Builder->CreateICmpSLE(left, right, "letmp");
            
        case TokenTypes::GreaterThan:
            return left->getType()->isFloatingPointTy()
                ? Builder->CreateFCmpOGT(left, right, "fgttmp")
                : Builder->CreateICmpSGT(left, right, "gttmp");
            
        case TokenTypes::GreaterEqual:
            return left->getType()->isFloatingPointTy()
                ? Builder->CreateFCmpOGE(left, right, "fgetmp")
                : Builder->CreateICmpSGE(left, right, "getmp");
            
        // Bitwise
        case TokenTypes::BitwiseAnd:
            return Builder->CreateAnd(left, right, "andtmp");
            
        case TokenTypes::BitwiseOr:
            return Builder->CreateOr(left, right, "ortmp");
            
        case TokenTypes::BitwiseXor:
            return Builder->CreateXor(left, right, "xortmp");
            
        case TokenTypes::ShiftLeft:
            return Builder->CreateShl(left, right, "shltmp");
            
        // case TokenTypes::ShiftRight: {
        //     // For shift right, we need to know if it's arithmetic (signed) or logical (unsigned)
        //     if (left->getType()->isIntegerTy()) {
        //         // Check if the value being shifted is signed by looking at its uses
        //         bool isSigned = false;
        //         if (llvm::Instruction* inst = llvm::dyn_cast<llvm::Instruction>(left)) {
        //             if (inst->getOpcode() == llvm::Instruction::SExt) {
        //                 isSigned = true;
        //             }
        //             // You might need to add more cases here depending on your IR
        //         }
        //         return isSigned ? Builder->CreateAShr(left, right, "ashrtmp")
        //                         : Builder->CreateLShr(left, right, "lshrtmp");
        //     }
        //     return nullptr; // Shift right only valid for integer types
        // }
        case TokenTypes::ShiftRight:
            return left->getType()->isIntegerTy()
                ? Builder->CreateAShr(left, right, "shrtmp")
                : nullptr;
        default:
            console.error("Unknown binary operator");
            return nullptr;
        }
    return nullptr;
}

llvm::Value* IRGenerator::createTernaryExpression(llvm::Value* cond, llvm::Value* truthy, llvm::Value* falsey) {
    if (!cond || !truthy || !falsey) {
        console.error("Unknown ternary operation");
        return nullptr;
    };

    // Convert condition to bool if needed
    if (!cond->getType()->isIntegerTy(1)) {
        cond = Builder->CreateICmpNE(
            cond, 
            llvm::ConstantInt::get(cond->getType(), 0),
            "booltmp"
        );
    }

    llvm::Function* fn = Builder->GetInsertBlock()->getParent();
    
    // Create blocks for the true/false/merge cases
    llvm::BasicBlock* trueBB = llvm::BasicBlock::Create(*Context, "true", fn);
    llvm::BasicBlock* falseBB = llvm::BasicBlock::Create(*Context, "false", fn);
    llvm::BasicBlock* mergeBB = llvm::BasicBlock::Create(*Context, "merge", fn);

    Builder->CreateCondBr(cond, trueBB, falseBB);

    // Emit true value
    Builder->SetInsertPoint(trueBB);
    Builder->CreateBr(mergeBB);
    trueBB = Builder->GetInsertBlock();

    // Emit false value
    Builder->SetInsertPoint(falseBB);
    Builder->CreateBr(mergeBB);
    falseBB = Builder->GetInsertBlock();

    // Create PHI node
    Builder->SetInsertPoint(mergeBB);
    llvm::PHINode* phi = Builder->CreatePHI(truthy->getType(), 2, "ternarytmp");
    phi->addIncoming(truthy, trueBB);
    phi->addIncoming(falsey, falseBB);

    return phi;
}

llvm::Value* IRGenerator::createObjectInstance(
    const std::string& typeName,
    const std::string& varName,
    const std::vector<llvm::Value*>& args,
    bool isGlobal
)
{
    DEBUG_LOG("Creating instance of type: " + typeName);
    
    // 1. Check for value types first
    // (your primitive type checks here)

    // 2. Check for struct types
    if (llvm::StructType* structType = llvm::StructType::getTypeByName(*Context, typeName)) {
        DEBUG_LOG("Found struct type: " + typeName);
        return createStructInstance(typeName, varName, args);
    }

    // 3. Check for class types
    // (your class type checks here)

    console.error("Unknown type: " + typeName);
    return nullptr;
}

void IRGenerator::createStructType(const std::string& name, const std::vector<llvm::Type*>& fieldTypes) {
    if (llvm::StructType::getTypeByName(*Context, name)) {
        DEBUG_LOG("Struct " + name + " already exists. Skipping creation.");
        return;
    }

    llvm::StructType* structType = llvm::StructType::create(*Context, fieldTypes, name);
    if (!structType) {
        console.error("Failed to create struct type: " + name);
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
    bool isGlobal)
{
    llvm::StructType* structType = llvm::StructType::getTypeByName(*Context, structName);
    if (!structType) {
        DEBUG_LOG("Struct type '" + structName + "' does not exist.");
        return nullptr;
    }

    // Ensure the number of arguments matches the number of fields in the struct
    if (args.size() != structType->getNumElements()) {
        console.error("Mismatch between number of fields '" + std::to_string(args.size()) + "' and constructor arguments '" + std::to_string(structType->getNumElements()) + "' for struct: " + structName);
        return nullptr;
    }

    // For initializing fields
    std::vector<llvm::Constant*> constants;
    for (size_t i = 0; i < args.size(); ++i) {
        if (auto* constVal = llvm::dyn_cast<llvm::Constant>(args[i])) {
            constants.push_back(constVal);
        } else {
            // If not constant, still allow the argument to be used in the struct
            constants.push_back(llvm::Constant::getNullValue(args[i]->getType())); // Fallback null value
        }
    }

    llvm::Constant* initializer = llvm::ConstantStruct::get(structType, constants);

    if (isGlobal) {
        // Creating a global variable
        llvm::Module* module = Builder->GetInsertBlock()->getModule();  // Get module from the builder's block
        llvm::GlobalVariable* globalVar = new llvm::GlobalVariable(
            *module,
            structType,
            false,  // isConstant
            llvm::GlobalValue::ExternalLinkage,
            initializer,
            varName
        );
        
        DEBUG_LOG("Created global struct instance: " + varName);
        activeScope->set(varName, globalVar);  // Register in active scope
        return globalVar;

    } else {
        // Creating a local variable (on the stack)
        llvm::Function* currentFunc = Builder->GetInsertBlock()->getParent();
        llvm::BasicBlock* entryBlock = &currentFunc->getEntryBlock();

        llvm::IRBuilder<> entryBuilder(entryBlock);
        if (!entryBlock->empty() && entryBlock->getTerminator()) {
            entryBuilder.SetInsertPoint(entryBlock->getTerminator());
        } else {
            entryBuilder.SetInsertPoint(entryBlock);
        }

        // Create alloca for local variable
        llvm::AllocaInst* localVar = entryBuilder.CreateAlloca(structType, nullptr, varName);

        // Initialize the fields using the current builder
        for (size_t i = 0; i < args.size() && i < structType->getNumElements(); ++i) {
            if (Builder->GetInsertBlock()->getTerminator()) {
                Builder->SetInsertPoint(Builder->GetInsertBlock()->getTerminator());
            }

            llvm::Value* fieldPtr = Builder->CreateStructGEP(structType, localVar, i, varName + "_field" + std::to_string(i));
            Builder->CreateStore(args[i], fieldPtr);
        }

        DEBUG_LOG("Created local struct instance: " + varName);
        activeScope->set(varName, localVar);  // Register struct instance in scope
        return localVar;
    }
}

llvm::Value* IRGenerator::handleMemberAccess(
    std::shared_ptr<Omniscript::MemberAccessExpression> expr,
    llvm::Value* baseValue,
    SymbolTableType scope,
    bool preservePointer
) {
    llvm::Type* currentType = activeScope->getType(expr->baseType);
    llvm::Value* currentPtr = baseValue;

    int fieldIndex = expr->index;

    if (!currentType->isStructTy()) {
        console.error("Member access requires an aggregate type (struct or class), not a '" + debugType(currentType) + "'.");
        return nullptr;
    }

    auto* structType = llvm::cast<llvm::StructType>(currentType);
    currentPtr = Builder->CreateStructGEP(structType, currentPtr, fieldIndex);
    currentType = structType->getElementType(fieldIndex);
    
    if (expr->isSetter()) {
        llvm::Value* valueToStore = codegen(expr->assignmentValue, scope);
        Builder->CreateStore(valueToStore, currentPtr);
        return valueToStore;
    }

    // // Only load if we're not preserving pointers AND it's not a setter
    // if (!preservePointer) {
    //     return currentPtr;  // Return the pointer if preserving
    // }
    
    return Builder->CreateLoad(currentType, currentPtr);
}

llvm::Value* IRGenerator::handleArrowAccess(
    std::shared_ptr<Omniscript::ArrowAccessExpression> expr,
    llvm::Value* baseValue,
    SymbolTableType scope
) {
    // Validate base value is a pointer
    if (!baseValue->getType()->isPointerTy()) {
        console.error("Arrow access requires pointer type");
        return nullptr;
    }

    // Get the actual pointee type (what the pointer points to)
    llvm::Type* pointeeType = resolveLLVMType(expr->expr->getType()->getPointeeType());
    
    // Handle case where we have a pointer-to-pointer
    if (pointeeType->isPointerTy()) {
        baseValue = Builder->CreateLoad(pointeeType, baseValue);
        pointeeType = resolveLLVMType(expr->expr->getType()->getPointeeType());
    }

    // Validate we're accessing a struct
    if (!pointeeType->isStructTy()) {
        console.error("Arrow access requires pointer to struct type");
        return nullptr;
    }

    auto* structType = llvm::cast<llvm::StructType>(pointeeType);
    int fieldIndex = expr->index;

    // Validate field index
    if (fieldIndex < 0 || fieldIndex >= (int)structType->getNumElements()) {
        console.error("Invalid struct field index");
        return nullptr;
    }

    // Get pointer to the field
    llvm::Value* fieldPtr = Builder->CreateStructGEP(structType, baseValue, fieldIndex);
    llvm::Type* fieldType = structType->getElementType(fieldIndex);

    // Handle setter case
    if (expr->isSetter()) {
        llvm::Value* valueToStore = codegen(expr->assignmentValue, scope);
        // Verify type compatibility
        if (valueToStore->getType() != fieldType) {
            valueToStore = Builder->CreateBitOrPointerCast(valueToStore, fieldType);
        }
        Builder->CreateStore(valueToStore, fieldPtr);
        return valueToStore;
    }

    // Handle getter case
    return Builder->CreateLoad(fieldType, fieldPtr);
}

llvm::Value* IRGenerator::handleDereference(
    std::shared_ptr<Omniscript::DereferenceExpression> expr,
    llvm::Value* baseValue,
    SymbolTableType scope
) {
    llvm::Type* ptrType = baseValue->getType();
    if (!ptrType->isPointerTy()) {
        console.error("Dereference requires pointer type");
        return nullptr;
    }

    llvm::PointerType* pointerType = llvm::dyn_cast<llvm::PointerType>(ptrType);
    if (!pointerType) {
        console.error("Base value is not a pointer.");
        return nullptr;
    }

    llvm::Type* pointeeType = resolveLLVMType(expr->getType());  // Get the type pointed to by the pointer
    llvm::Value* loadedPtr = baseValue;

    if (expr->valueExpr) {
        // This is a dereference assignment (*ptr = value)
        llvm::Value* valueToStore = codegen(expr->valueExpr, scope);
        if (!valueToStore) return nullptr;
        
        if (valueToStore->getType() != pointeeType) {
            valueToStore = Builder->CreateBitCast(valueToStore, pointeeType, "bitcast.store");
        }
        Builder->CreateStore(valueToStore, loadedPtr);
        return valueToStore;
    }

    // Regular dereference (*ptr)
    return Builder->CreateLoad(pointeeType, loadedPtr, "load.deref");
}

llvm::Value* IRGenerator::handleIndexAccess(
    std::shared_ptr<Omniscript::IndexAccessExpression> expr,
    llvm::Value* baseValue,
    SymbolTableType scope
) {
    // Evaluate the index expression
    llvm::Value* indexValue = codegen(expr->indexExpr, scope);
    if (!indexValue) return nullptr;

    llvm::Type* baseType = baseValue->getType();
    if (!baseType->isPointerTy() && !baseType->isArrayTy()) {
        console.error("Index access requires pointer or array type");
        return nullptr;
    }

    llvm::PointerType* pointerType = llvm::dyn_cast<llvm::PointerType>(baseType);
    if (!pointerType) {
        console.error("Base value is neither pointer nor array.");
        return nullptr;
    }

    // Get pointer to element
    llvm::Value* elementPtr = Builder->CreateGEP(
        resolveLLVMType(expr->getType()),  // The type pointed to by the pointer
        baseValue, 
        indexValue, 
        "index.ptr"
    );

    if (expr->isSetter()) {
        llvm::Value* valueToStore = codegen(expr->assignmentValue, scope);
        if (!valueToStore) return nullptr;
        
        Builder->CreateStore(valueToStore, elementPtr);
        return valueToStore;
    }

    // Load the value
    return Builder->CreateLoad(resolveLLVMType(expr->getType()), elementPtr, "load.index");
}

// Getter: Load a member variable from an object
llvm::Value* IRGenerator::loadMemberValue(const std::string& objectName, const std::string& memberName) { 
    // Retrieve the object pointer
    llvm::Value* objectPtr = getVariable(objectName);
    if (!objectPtr) return nullptr; // Variable not found

    // If this is an enum lookup table, use `getEnumValue` instead
    if (objectName.ends_with("_lookup")) {
        std::string enumName = objectName.substr(0, objectName.find("_lookup"));
        return getEnumValue(enumName, memberName);
    }

    // Ensure it's a pointer
    llvm::PointerType* pointerType = llvm::dyn_cast<llvm::PointerType>(objectPtr->getType());
    if (!pointerType) return nullptr; // Not a pointer

    // Get the struct/class type
    llvm::Type* elementType = pointerType->getContainedType(0);
    llvm::StructType* structType = llvm::dyn_cast<llvm::StructType>(elementType);
    if (!structType) return nullptr; // Not a struct/class

    // Get the index of the member
    int memberIndex = getStructMemberIndex(structType, memberName);
    if (memberIndex == -1) return nullptr; // Member not found

    // Generate GEP
    llvm::Value* memberPtr = Builder->CreateStructGEP(structType, objectPtr, memberIndex);

    // Load the value
    llvm::Type* loadedType = memberPtr->getType()->getContainedType(0);
    return Builder->CreateLoad(loadedType, memberPtr);
}


llvm::Value* IRGenerator::loadMemberFromStruct(llvm::Value* structPtr, llvm::StructType* structType, const std::string& memberName) {
    // Get the index of the member
    int memberIndex = getStructMemberIndex(structType, memberName);
    if (memberIndex == -1) return nullptr;

    // Generate a GEP instruction
    llvm::Value* memberPtr = Builder->CreateStructGEP(structType, structPtr, memberIndex);

    // Load the member value
    llvm::Type* loadedType = memberPtr->getType()->getContainedType(0);  // Get the type of the element being pointed to
    return Builder->CreateLoad(loadedType, memberPtr);  // Create the load with the correct type
}

llvm::Value* IRGenerator::loadMemberFromClass(llvm::Value* classPtr, llvm::StructType* classType, const std::string& memberName) {
    // Get the index of the member in the class
    int memberIndex = getStructMemberIndex(classType, memberName);
    if (memberIndex == -1) return nullptr;

    // Generate GEP instruction to access member
    llvm::Value* memberPtr = Builder->CreateStructGEP(classType, classPtr, memberIndex);

    // Load the member value
    llvm::Type* loadedType = memberPtr->getType()->getContainedType(0);  // Get the type of the element being pointed to
    return Builder->CreateLoad(loadedType, memberPtr);  // Create the load with the correct type
}


int IRGenerator::getStructMemberIndex(llvm::StructType* structType, const std::string& memberName) {
    const llvm::StructLayout* layout = Module->getDataLayout().getStructLayout(structType);
    auto structMembers = structType->elements();

    // Check if the member name matches any known struct/class member
    for (size_t i = 0; i < structMembers.size(); i++) {
        if (memberName == typeToString(structMembers[i])) {
            return i;
        }
    }

    return -1; // Not found
}


// Setter: Store a new value in a member variable
void IRGenerator::setMemberValue(
    llvm::Value* object, 
    const std::string& memberName, 
    llvm::Value* newValue
) {
    return;
}

llvm::Value* IRGenerator::createEnum(
    const std::vector<std::string>& names,
    const std::vector<llvm::Value*>& values,
    const std::string& enumName,
    bool isGlobal
) {
    for (size_t i = 0; i < names.size(); ++i) {
        assignVariable(enumName + "." + names[i], values[i]->getType(), values[i], isGlobal, true);
    }
    return nullptr;
}


llvm::Value* IRGenerator::createEnumWithLookup(
    const std::vector<std::string>& names,
    const std::vector<llvm::Value*>& values,
    const std::string& enumName,
    bool isGlobal
) {
    llvm::Type* valueType = values[0]->getType();
    std::vector<llvm::Constant*> constValues;
    std::vector<llvm::Constant*> nameConstants;

    for (size_t i = 0; i < values.size(); ++i) {
        // Declare each enum value as a global/local variable
        assignVariable(enumName + "." + names[i], valueType, values[i], isGlobal, true);

        // Handle the constant value for the value array
        if (auto* constantVal = llvm::dyn_cast<llvm::Constant>(values[i])) {
            constValues.push_back(constantVal);
        } else {
            llvm::errs() << "Warning: Non-constant enum value for " << names[i] << "\n";
            constValues.push_back(llvm::Constant::getNullValue(valueType));
        }

        // Create a global string pointer for the name
        llvm::Constant* namePtr = Builder->CreateGlobalString(names[i], enumName + "_str_" + names[i]);
        nameConstants.push_back(namePtr);
    }

    // Create value lookup array
    llvm::ArrayType* valueArrayType = llvm::ArrayType::get(valueType, constValues.size());
    llvm::Constant* valueArray = llvm::ConstantArray::get(valueArrayType, constValues);
    assignVariable(enumName + "_lookup", valueArrayType, valueArray, isGlobal, true);

    // Create name (string) lookup array
    llvm::Type* stringPtrType = nameConstants[0]->getType(); // i8*
    llvm::ArrayType* nameArrayType = llvm::ArrayType::get(stringPtrType, nameConstants.size());
    llvm::Constant* nameArray = llvm::ConstantArray::get(nameArrayType, nameConstants);
    assignVariable(enumName + "_name_lookup", nameArrayType, nameArray, isGlobal, true);

    return valueArray; // or nullptr if you don't need to return a value
}

llvm::Value* IRGenerator::createEnumClass(
    const std::vector<std::string>& names,
    const std::vector<llvm::Value*>& values,
    const std::string& className,
    bool isGlobal
) {
    llvm::LLVMContext& ctx = Builder->getContext();
    llvm::Type* fieldType = values[0]->getType();

    std::vector<llvm::Type*> fieldTypes(values.size(), fieldType);
    std::vector<llvm::Constant*> fieldValues;

    for (auto* val : values) {
        if (auto* c = llvm::dyn_cast<llvm::Constant>(val)) {
            fieldValues.push_back(c);
        } else {
            llvm::errs() << "Warning: Non-constant value in enum class " << className << "\n";
            fieldValues.push_back(llvm::Constant::getNullValue(fieldType));
        }
    }

    llvm::StructType* structType = llvm::StructType::create(ctx, fieldTypes, className);
    llvm::Constant* structConst = llvm::ConstantStruct::get(structType, fieldValues);

    return assignVariable(className, structType, structConst, isGlobal, true);
}

llvm::Value* IRGenerator::createEnumClassWithLookup(
    const std::vector<std::string>& names,
    const std::vector<llvm::Value*>& values,
    const std::string& className,
    bool isGlobal
) {
    llvm::LLVMContext& ctx = Builder->getContext();
    llvm::Type* valueType = values[0]->getType();

    std::vector<llvm::Type*> fieldTypes(values.size(), valueType);
    std::vector<llvm::Constant*> fieldValues;
    std::vector<llvm::Constant*> nameConstants;

    // Step 1: Generate the enum struct values and name strings
    for (size_t i = 0; i < values.size(); ++i) {
        llvm::Value* val = values[i];

        if (auto* c = llvm::dyn_cast<llvm::Constant>(val)) {
            fieldValues.push_back(c);
        } else {
            llvm::errs() << "Warning: Non-constant enum value in class " << className << "\n";
            fieldValues.push_back(llvm::Constant::getNullValue(valueType));
        }

        // Create a global string pointer for the name
        llvm::Constant* namePtr = Builder->CreateGlobalString(names[i], className + "_str_" + names[i]);
        nameConstants.push_back(namePtr);
    }

    // Step 2: Create the struct for the enum class
    llvm::StructType* structType = llvm::StructType::create(ctx, fieldTypes, className);
    llvm::Constant* structConst = llvm::ConstantStruct::get(structType, fieldValues);
    llvm::Value* enumClass = assignVariable(className, structType, structConst, isGlobal, true);

    // Step 3: Create value lookup array (flat version of struct)
    llvm::ArrayType* lookupArrayType = llvm::ArrayType::get(valueType, fieldValues.size());
    llvm::Constant* lookupArray = llvm::ConstantArray::get(lookupArrayType, fieldValues);
    assignVariable(className + "_lookup", lookupArrayType, lookupArray, isGlobal, true);

    // Step 4: Create name lookup array
    llvm::Type* stringPtrType = nameConstants[0]->getType(); // i8*
    llvm::ArrayType* nameArrayType = llvm::ArrayType::get(stringPtrType, nameConstants.size());
    llvm::Constant* nameArray = llvm::ConstantArray::get(nameArrayType, nameConstants);
    assignVariable(className + "_name_lookup", nameArrayType, nameArray, isGlobal, true);

    return enumClass;
}

llvm::Value* IRGenerator::getEnumValue(const std::string& enumName, const std::string& memberName) {
    // Construct lookup variable name
    // std::string lookupTableName = enumName + "_lookup";

    // // Retrieve the enum lookup table from the symbol table
    // llvm::GlobalVariable* lookupTable = dynamic_cast<llvm::GlobalVariable*>(activeScope->get(lookupTableName));
    // if (!lookupTable) {
    //     console.error("Enum '" + enumName + "' not found.");
    //     return nullptr;
    // }

    // // Get the array type and its length
    // llvm::ArrayType* arrayType = llvm::dyn_cast<llvm::ArrayType>(lookupTable->getValueType());
    // if (!arrayType) {
    //     console.error("Invalid enum lookup table format for '" + enumName + "'.");
    //     return nullptr;
    // }

    // size_t numEntries = arrayType->getNumElements();
    // llvm::Type* structType = arrayType->getElementType(); // Struct { i32, i8* }

    // // Iterate through the lookup table to find the matching member
    // for (size_t i = 0; i < numEntries; ++i) {
    //     // Get pointer to entry i
    //     llvm::Value* entryPtr = Builder->CreateConstGEP2_32(arrayType, lookupTable, 0, i);

    //     // Extract the member name (char*)
    //     llvm::Value* namePtr = Builder->CreateStructGEP(structType, entryPtr, 1);
    //     llvm::Value* nameValue = Builder->CreateLoad(namePtr->getType()->getPointerElementType(), namePtr);

    //     // Compare with the requested memberName
    //     llvm::Value* cmp = Builder->CreateCall(getStrcmpFunction(), {nameValue, Builder->CreateGlobalStringPtr(memberName)});
    //     llvm::Value* isMatch = Builder->CreateICmpEQ(cmp, llvm::ConstantInt::get(llvm::Type::getInt32Ty(*Context), 0));

    //     // If matched, return the integer value
    //     llvm::Value* intPtr = Builder->CreateStructGEP(structType, entryPtr, 0);
    //     llvm::Value* intValue = Builder->CreateLoad(intPtr->getType()->getPointerElementType(), intPtr);

    //     llvm::BasicBlock* returnBlock = llvm::BasicBlock::Create(*Context, "return_enum", Builder->GetInsertBlock()->getParent());
    //     llvm::BasicBlock* continueBlock = llvm::BasicBlock::Create(*Context, "continue_enum", Builder->GetInsertBlock()->getParent());

    //     Builder->CreateCondBr(isMatch, returnBlock, continueBlock);

    //     // Set insert point for return
    //     Builder->SetInsertPoint(returnBlock);
    //     Builder->CreateRet(intValue);

    //     // Set insert point for continue
    //     Builder->SetInsertPoint(continueBlock);
    // }

    // // If no match found, return an error value (-1)
    // return Builder->CreateRet(llvm::ConstantInt::get(llvm::Type::getInt32Ty(*Context), -1));
    return nullptr;
}

llvm::Value* IRGenerator::createModuleObject(
    const std::string& moduleName,
    const std::unordered_map<std::string, llvm::Value*>& members
) {
    llvm::LLVMContext& ctx = Builder->getContext();
    llvm::Module* mod = currentModule;

    DEBUG_LOG("Creating module object: " + moduleName);

    // Step 1: Create the struct type for the module using the member types
    std::vector<llvm::Type*> memberTypes;
    std::vector<std::string> memberNames;

    for (const auto& [key, val] : members) {
        memberTypes.push_back(val->getType());
        memberNames.push_back(key);
    }

    // Create the struct type using `createStructType`
    if (llvm::StructType::getTypeByName(*Context, moduleName)) {
        console.error("Cannot create module " + moduleName + "as a symbol with the name '" + moduleName + "' already exists in the scope.");
        return nullptr;
    }
    
    createStructType(moduleName, memberTypes);

    // // Step 2: Create an instance of the struct
    // llvm::Function* function = Builder->GetInsertBlock()->getParent();
    // llvm::IRBuilder<> tempBuilder(&function->getEntryBlock(), function->getEntryBlock().begin());

    // Create the struct instance using `createObjectInstance`
    llvm::Value* moduleInstance = createObjectInstance(
        moduleName,
        moduleName,
        std::vector<llvm::Value*> {},
        true
    );

    DEBUG_LOG("Module struct instance created: " + moduleName);

    return moduleInstance;
}

llvm::Value* IRGenerator::generateCast(llvm::Value* src, llvm::Type* destType) {
    llvm::Type* srcType = src->getType();

    if (srcType == destType) {
        DEBUG_LOG("No cast needed; source and destination types match.");
        return src;
    }

    if (srcType->isIntegerTy() && destType->isIntegerTy()) {
        unsigned srcBits = srcType->getIntegerBitWidth();
        unsigned destBits = destType->getIntegerBitWidth();
        if (destBits > srcBits) {
            return Builder->CreateZExt(src, destType, "zext");
        } else if (destBits < srcBits) {
            return Builder->CreateTrunc(src, destType, "trunc");
        } else {
            return src; // same bit width
        }
    }

    if (srcType->isIntegerTy() && destType->isFloatingPointTy()) {
        return Builder->CreateSIToFP(src, destType, "sitofp");
    }

    if (srcType->isFloatingPointTy() && destType->isIntegerTy()) {
        return Builder->CreateFPToSI(src, destType, "fptosi");
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
        return Builder->CreateBitCast(src, destType, "ptrcast");
    }

    console.error("Unsupported cast from '" + debugType(srcType) + "' to '" + debugType(destType) + "'");
    return nullptr;
}
