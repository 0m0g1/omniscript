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

#ifdef _WIN32
    #include <sys/stat.h>
#endif

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

// Generates a main function if none exists, ensuring no recursion
void IRGenerator::addMainFunction() {
    if (!Module) return;

    // Get or create the top-level function
    llvm::Function* topFunc = Module->getFunction("__top_level__");
    if (!topFunc) {
        llvm::FunctionType* topType = 
            llvm::FunctionType::get(llvm::Type::getVoidTy(*Context), false);
        topFunc = llvm::Function::Create(topType, 
                                       llvm::Function::ExternalLinkage, 
                                       "__top_level__", 
                                       Module.get());
        llvm::BasicBlock* topEntry = llvm::BasicBlock::Create(*Context, "entry", topFunc);
        Builder->SetInsertPoint(topEntry);
        Builder->CreateRetVoid();
    }

    // Create main with the simplest valid signature (main(void))
    llvm::Type* intType = llvm::Type::getInt32Ty(*Context);
    llvm::FunctionType* mainType = 
        llvm::FunctionType::get(intType, false);  // int main(void)
    
    llvm::Function* mainFn = llvm::Function::Create(mainType,
                                                  llvm::Function::ExternalLinkage,
                                                  "main",
                                                  Module.get());
    
    // Create function body
    llvm::BasicBlock* mainEntry = llvm::BasicBlock::Create(*Context, "entry", mainFn);
    Builder->SetInsertPoint(mainEntry);
    
    // Call the top-level function
    Builder->CreateCall(topFunc);
    
    // Return 0 (success)
    Builder->CreateRet(llvm::ConstantInt::get(intType, 0));
    
    // Verify the top-level function doesn't call main
    for (auto &BB : *topFunc) {
        for (auto &I : BB) {
            if (auto *Call = llvm::dyn_cast<llvm::CallInst>(&I)) {
                if (Call->getCalledFunction() == mainFn) {
                    console.error("__top_level__ calls main()");
                    mainFn->eraseFromParent();  // Remove the invalid main
                    return;
                }
            }
        }
    }
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
    // Get the current function properly
    llvm::Function* currentFn = Builder->GetInsertBlock() ? 
        Builder->GetInsertBlock()->getParent() : nullptr;

    // Prevent recursive calls from __top_level__ to main
    if (currentFn && currentFn->getName() == "__top_level__" && 
        (callee == "main" || callee == "__main")) {
        console.warn(
            "⚠️  Invalid call to main() from __top_level__\n"
            "┌──────────────────────────────────────────────────────────┐\n"
            "│ The compiler automatically generates a main() function   │\n"
            "│ that calls __top_level__.                                │\n"
            "│                                                          │\n"
            "│ Do not call main() manually from __top_level__ as this   │\n"
            "│ would create infinite recursion.                         │\n"
            "│                                                          │\n"
            "│ Solution: Remove this call to main() - your program      |\n"
            "│ entry point is already handled automatically.            │\n"
            "└──────────────────────────────────────────────────────────┘\n"
        );
        return Builder->getInt32(0);  // Return dummy value instead of nullptr
    }

    // 1. Find the callable value [original code unchanged]
    llvm::Value* funcValue = nullptr;
    bool isDynamicFunction = !functionTypeName.empty();
    
    if (auto* moduleFunc = Module->getFunction(callee)) {
        funcValue = moduleFunc;
    } else if (auto* globalVar = Module->getGlobalVariable(callee)) {
        funcValue = globalVar;
    } else if (auto* value = activeScope->get(callee)) {
        funcValue = value;
    } else {
        console.error("Callable '" + callee + "' not found");
        return nullptr;
    }

    // 2. Determine the function type [original code unchanged]
    llvm::FunctionType* funcType = nullptr;
    
    if (auto* func = llvm::dyn_cast<llvm::Function>(funcValue)) {
        funcType = func->getFunctionType();
    }
    else if (funcValue->getType()->isPointerTy()) {
        // Function pointer case - we need to check if it points to a function
        if (auto* funcPtrType = llvm::dyn_cast<llvm::PointerType>(funcValue->getType())) {
            // For opaque pointers, we need the function type from elsewhere
            if (!functionTypeName.empty()) {
                if (auto* type = activeScope->getType("*" + callee)) {
                    if (auto* ft = llvm::dyn_cast<llvm::FunctionType>(type)) {
                        funcType = ft;
                    }
                    else {
                        console.error("Type '" + functionTypeName + "' is not a function type");
                        return nullptr;
                    }
                }
                else {
                    console.error("Unknown type '" + functionTypeName + "'");
                    return nullptr;
                }
            }
            else {
                // With opaque pointers, we can't get the pointee type, so we need explicit type info
                console.error("Function pointer call requires explicit type for '" + callee + "'");
                return nullptr;
            }
        }
    }
    else if (!functionTypeName.empty()) {
        if (auto* type = activeScope->getType("*" + functionTypeName)) {
            if (auto* ft = llvm::dyn_cast<llvm::FunctionType>(type)) {
                funcType = ft;
            } else {
                console.error("Type '" + functionTypeName + "' is not a function type");
                return nullptr;
            }
        } else {
            console.error("Unknown type '" + functionTypeName + "'");
            return nullptr;
        }
    }
    else {
        console.error("Untyped function pointer call to '" + callee + "'");
        return nullptr;
    }

    // 3. Validate arguments [original code unchanged]
    bool isVarArg = funcType->isVarArg();
    size_t fixedParams = funcType->getNumParams();
    
    if (!isVarArg && args.size() != fixedParams) {
        console.error("Argument count mismatch for '" + callee + "'");
        return nullptr;
    }

    // 4. Cast arguments if needed [original code unchanged]
    for (size_t i = 0; i < std::min(args.size(), fixedParams); ++i) {
        if (args[i]->getType() != funcType->getParamType(i)) {
            args[i] = generateCast(args[i], funcType->getParamType(i));
            if (!args[i]) {
                console.error("Argument " + std::to_string(i) + " type mismatch");
                return nullptr;
            }
        }
    }

    // 5. Create the call [original code unchanged]
    llvm::CallInst* call = nullptr;
    
    if (auto* func = llvm::dyn_cast<llvm::Function>(funcValue)) {
        call = Builder->CreateCall(func, args);
    } else {
        llvm::Value* loadedFuncPtr = nullptr;
        
        if (auto* globalVar = llvm::dyn_cast<llvm::GlobalVariable>(funcValue)) {
            loadedFuncPtr = Builder->CreateLoad(
                llvm::PointerType::getUnqual(funcType), 
                globalVar
            );
        } else {
            loadedFuncPtr = funcValue;
        }
        
        call = Builder->CreateCall(funcType, loadedFuncPtr, args);
    }

    return call;
}

llvm::Function* IRGenerator::createExternFunction(
    std::shared_ptr<Omniscript::FunctionExpression> func,
    SymbolTableType scope
) {
    
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
    
    // Helper function to check if a library is a system library that doesn't need copying
    auto isSystemLibrary = [&](const std::string& libPath) -> bool {
        if (libPath.empty() || libPath == "C") return true;
        
        // System libraries based on OS
        if (targetOS == TargetOS::Windows) {
            std::string filename = libPath;
            size_t lastSlash = libPath.find_last_of("/\\");
            if (lastSlash != std::string::npos) {
                filename = libPath.substr(lastSlash + 1);
            }
            
            // Convert to lowercase for comparison
            std::transform(filename.begin(), filename.end(), filename.begin(), ::tolower);
            
            // Windows system DLLs that are always available
            return filename == "kernel32.dll" || filename == "user32.dll" || 
                   filename == "gdi32.dll" || filename == "msvcrt.dll" ||
                   filename == "advapi32.dll" || filename == "shell32.dll" ||
                   filename == "ole32.dll" || filename == "oleaut32.dll" ||
                   filename == "winmm.dll" || filename == "ws2_32.dll";
        }
        
        // For Unix-like systems, check if it's a system library path
        return libPath.find("/lib/") != std::string::npos || 
               libPath.find("/usr/lib/") != std::string::npos ||
               libPath.find("/usr/local/lib/") != std::string::npos ||
               libPath == "libc.so" || libPath == "libm.so" || 
               libPath == "libpthread.so" || libPath == "libdl.so";
    };
    
    // Helper function to create directories recursively
    std::function<bool(const std::string&)> createDirectoryRecursive = [&](const std::string& path) -> bool {
        if (path.empty()) return true;
        
        // Check if directory already exists
        struct stat st;
        if (stat(path.c_str(), &st) == 0) {
        #ifdef _WIN32
            if (st.st_mode & _S_IFDIR) {
                return true;
            }
        #else
            if (S_ISDIR(st.st_mode)) {
                return true;
            }
        #endif
        }
        
        // Find parent directory
        size_t lastSlash = path.find_last_of("/\\");
        if (lastSlash != std::string::npos) {
            std::string parent = path.substr(0, lastSlash);
            if (!createDirectoryRecursive(parent)) {
                return false;
            }
        }
        
        // Create this directory
        #ifdef _WIN32
            return CreateDirectoryA(path.c_str(), NULL) != 0 || GetLastError() == ERROR_ALREADY_EXISTS;
        #else
            return mkdir(path.c_str(), 0755) == 0 || errno == EEXIST;
        #endif
    };
    
    // Helper function to copy DLL to output directory if needed
    auto copyDllToOutputDir = [&](const std::string& dllPath) -> std::string {
        if (isSystemLibrary(dllPath)) {
            return dllPath; // Don't copy system libraries
        }
        
        // Extract filename from full path
        std::string filename = dllPath;
        size_t lastSlash = dllPath.find_last_of("/\\");
        if (lastSlash != std::string::npos) {
            filename = dllPath.substr(lastSlash + 1);
        }
        
        // Get output directory from config
        std::string outputDir = configs.outputPath;
        if (outputDir.empty()) {
            outputDir = "."; // Default to current directory
        }
        
        // Remove filename from outputPath if it contains one
        size_t lastDot = outputDir.find_last_of(".");
        size_t lastSlashInOutput = outputDir.find_last_of("/\\");
        if (lastDot != std::string::npos && (lastSlashInOutput == std::string::npos || lastDot > lastSlashInOutput)) {
            // outputPath contains a filename, extract directory
            if (lastSlashInOutput != std::string::npos) {
                outputDir = outputDir.substr(0, lastSlashInOutput);
            } else {
                outputDir = ".";
            }
        }
        
        // Create output directory if it doesn't exist
        if (!createDirectoryRecursive(outputDir)) {
            console.error("Failed to create output directory: " + outputDir);
            return dllPath;
        }
        
        std::string destPath = outputDir + "/" + filename;
        
        // Check if DLL already exists in output directory
        if (fileExists(destPath)) {
            return destPath; // Already exists, use it
        }
        
        // Check if source DLL exists
        if (!fileExists(dllPath)) {
            return dllPath; // Return original path, let error handling deal with it
        }
        
        // Copy DLL to output directory
        try {
            std::ifstream src(dllPath, std::ios::binary);
            if (!src.is_open()) {
                console.error("Failed to open source DLL: " + dllPath);
                return dllPath;
            }
            
            std::ofstream dest(destPath, std::ios::binary);
            if (!dest.is_open()) {
                console.error("Failed to create destination DLL: " + destPath);
                return dllPath;
            }
            
            dest << src.rdbuf();
            src.close();
            dest.close();
            
            console.info("Copied DLL: " + dllPath + " -> " + destPath);
            return destPath;
            
        } catch (const std::exception& e) {
            console.error("Failed to copy DLL " + dllPath + " to " + destPath + ": " + e.what());
            return dllPath;
        }
    };
    
    // Helper function to delete copied DLL if it's not needed
    auto deleteUnusedDll = [&](const std::string& dllPath) -> void {
        if (isSystemLibrary(dllPath) || dllPath.empty()) {
            return; // Don't delete system libraries
        }
        
        // Extract filename from full path
        std::string filename = dllPath;
        size_t lastSlash = dllPath.find_last_of("/\\");
        if (lastSlash != std::string::npos) {
            filename = dllPath.substr(lastSlash + 1);
        }
        
        // Get output directory from config
        std::string outputDir = configs.outputPath;
        if (outputDir.empty()) {
            outputDir = ".";
        }
        
        // Remove filename from outputPath if it contains one
        size_t lastDot = outputDir.find_last_of(".");
        size_t lastSlashInOutput = outputDir.find_last_of("/\\");
        if (lastDot != std::string::npos && (lastSlashInOutput == std::string::npos || lastDot > lastSlashInOutput)) {
            if (lastSlashInOutput != std::string::npos) {
                outputDir = outputDir.substr(0, lastSlashInOutput);
            } else {
                outputDir = ".";
            }
        }
        
        std::string destPath = outputDir + "/" + filename;
        
        // Only delete if it exists and is different from the original path
        if (fileExists(destPath) && destPath != dllPath) {
            try {
                std::remove(destPath.c_str());
                console.info("Deleted unused DLL: " + destPath);
            } catch (const std::exception& e) {
                console.error("Failed to delete unused DLL " + destPath + ": " + e.what());
            }
        }
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
            // For JIT mode, use dynamic library resolver without linker dependencies
            function = resolver->resolve(*this, externName, funcType, linkerDependencies);
        }
        
    } else {
        // AOT mode - enhanced static/dynamic library handling with dynamic Windows API detection
        bool staticExists = !genericStatic.empty() && fileExists(genericStatic);
        bool dynamicExists = !genericDynamic.empty() && fileExists(genericDynamic);
        
        // Store original dynamic path for potential cleanup
        std::string originalDynamicPath = genericDynamic;
        std::string copiedDynamicPath;
        bool dynamicWasCopied = false;
        bool resolvedWithStatic = false;

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

        // Try static library first
        if (staticExists) {
            // Skip symbol existence check for Windows system libraries
            bool isWindowsSystemLib = (targetOS == TargetOS::Windows) && 
                                     WindowsAPIResolver::isWindowsSystemLibrary(genericStatic);
            if (!isWindowsSystemLib && !ExternalFunctionResolver::isSystemLibrary(genericStatic) && 
                !symbolExistsInStaticLib(genericStatic, externName)) {
                console.error("Symbol '" + externName + "' not found in static library: " + genericStatic);
                staticExists = false; // Mark as not usable
            } else {
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
                    if (function) {
                        resolvedWithStatic = true;
                    }
                }
            }
        }

        // If static resolution failed, try dynamic library
        if (!function && dynamicExists) {
            // Copy dynamic library to output directory for AOT mode
            copiedDynamicPath = copyDllToOutputDir(genericDynamic);
            dynamicWasCopied = (copiedDynamicPath != genericDynamic);
            
            auto resolver = tryAddResolver(originalDynamicPath, [&]() -> std::unique_ptr<ExternalFunctionResolver> {
                // Always use DynamicLibraryResolver for dynamic libraries
                return std::make_unique<DynamicLibraryResolver>(originalDynamicPath);
            });

            if (resolver) {
                // For dynamic libraries, don't add linker dependencies
                function = resolver->resolve(*this, externName, funcType, linkerDependencies);
                
                // If dynamic resolution failed after copying, clean up the copied DLL
                if (!function && dynamicWasCopied) {
                    deleteUnusedDll(originalDynamicPath);
                }
            } else if (dynamicWasCopied) {
                // If resolver creation failed after copying, clean up the copied DLL
                deleteUnusedDll(originalDynamicPath);
            }
        }
        
        // If we successfully resolved with static library but had copied a dynamic library earlier, clean it up
        if (resolvedWithStatic && dynamicWasCopied) {
            deleteUnusedDll(originalDynamicPath);
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

        if (expr) {
            DEBUG_LOG("Generating code for expression: " + expr->toString());
            retVal = codegen(expr, localScope);
        }

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
