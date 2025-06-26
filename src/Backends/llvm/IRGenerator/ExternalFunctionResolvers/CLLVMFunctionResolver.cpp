#include <omniscript/engine/Backends/LLVM/IRGenerator.h>
#include <omniscript/engine/Backends/LLVM/LLVMExternalFunctionResolver.h>
#include <omniscript/engine/Backends/LLVM/ExternalFunctionResolvers/CLLVMResolver.h>

llvm::Function* CStdLibResolver::resolve(IRGenerator& generator, const std::string& name, 
                                       llvm::FunctionType* funcType, LinkDependencies& deps) {
    static const std::unordered_map<std::string, std::string> functionToLibrary = {
        // Math functions
        {"sin", "m"}, {"cos", "m"}, {"tan", "m"}, {"sqrt", "m"}, {"pow", "m"},
        {"exp", "m"}, {"log", "m"}, {"floor", "m"}, {"ceil", "m"}, {"fabs", "m"},
        {"asin", "m"}, {"acos", "m"}, {"atan", "m"}, {"atan2", "m"}, {"sinh", "m"},
        {"cosh", "m"}, {"tanh", "m"}, {"log10", "m"}, {"ldexp", "m"}, {"frexp", "m"},
        
        // Threading
        {"pthread_create", "pthread"}, {"pthread_join", "pthread"}, 
        {"pthread_mutex_init", "pthread"}, {"pthread_mutex_lock", "pthread"},
        {"pthread_mutex_unlock", "pthread"}, {"pthread_mutex_destroy", "pthread"},
        {"pthread_cond_init", "pthread"}, {"pthread_cond_wait", "pthread"},
        {"pthread_cond_signal", "pthread"}, {"pthread_cond_broadcast", "pthread"},
        
        // Dynamic loading
        {"dlopen", "dl"}, {"dlsym", "dl"}, {"dlclose", "dl"}, {"dlerror", "dl"},
        
        // Standard C functions (usually in libc, no explicit linking needed on most systems)
        {"printf", ""}, {"scanf", ""}, {"malloc", ""}, {"free", ""}, {"calloc", ""},
        {"realloc", ""}, {"strlen", ""}, {"strcpy", ""}, {"strcmp", ""}, {"strcat", ""},
        {"memcpy", ""}, {"memset", ""}, {"memcmp", ""}, {"exit", ""}, {"abort", ""},
        {"getenv", ""}, {"system", ""}, {"time", ""}, {"clock", ""}, {"rand", ""},
        {"srand", ""}, {"qsort", ""}, {"bsearch", ""}, {"atoi", ""}, {"atof", ""},
        {"strtol", ""}, {"strtod", ""}, {"fopen", ""}, {"fclose", ""}, {"fread", ""},
        {"fwrite", ""}, {"fseek", ""}, {"ftell", ""}, {"rewind", ""}, {"fflush", ""},
    };
    
    auto it = functionToLibrary.find(name);
    if (it == functionToLibrary.end()) {
        return nullptr; // Not a known C standard library function
    }
    
    // Create the function
    llvm::Function* func = llvm::Function::Create(
        funcType, llvm::Function::ExternalLinkage, name, generator.getCurrentModule()
    );
    
    // Apply platform-specific attributes
    applyPlatformSpecificAttributes(func, name);
    
    // Add library dependency if needed
    if (!it->second.empty()) {
        LinkDependencies::LibraryInfo info;
        info.name = it->second;
        info.isSystemLib = true;
        deps.addRequiredLibrary(it->second, info);
    }
    
    return func;
}

bool CStdLibResolver::isCStdLibFunction(const std::string& name) {
    static const std::unordered_set<std::string> cStdLibFuncs = {
        "printf", "scanf", "malloc", "free", "strlen", "strcpy",
        "memcpy", "memset", "fopen", "fclose", "fread", "fwrite"
    };
    return cStdLibFuncs.count(name) > 0;
}

void CStdLibResolver::applyPlatformSpecificAttributes(llvm::Function* func, const std::string& name) {
    // Set calling convention
    func->setCallingConv(getPlatformCallingConv(name));
    
    // Add common attributes for certain functions
    if (name == "malloc" || name == "calloc" || name == "realloc") {
        func->addFnAttr(llvm::Attribute::NoAlias);
    }
    
    if (name == "strlen" || name == "strcmp" || name == "memcmp") {
        func->addFnAttr(llvm::Attribute::ReadOnly);
        func->addFnAttr(llvm::Attribute::NoUnwind);
    }
    
    if (name == "memcpy" || name == "memset" || name == "strcpy") {
        func->addFnAttr(llvm::Attribute::NoUnwind);
    }
}

llvm::CallingConv::ID CStdLibResolver::getPlatformCallingConv(const std::string& name) {
    PlatformInfo::Platform platform = PlatformInfo::getCurrentPlatform();
    
    switch (platform) {
        case PlatformInfo::Windows:
            return llvm::CallingConv::C; // Use default C calling convention
        case PlatformInfo::WebAssembly:
            return llvm::CallingConv::C;
        default:
            return llvm::CallingConv::C;
    }
}