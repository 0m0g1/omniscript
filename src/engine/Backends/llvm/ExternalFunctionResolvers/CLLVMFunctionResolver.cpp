#include <omniscript/engine/Backends/LLVM/IRGenerator.h>
#include <omniscript/engine/Backends/LLVM/ExternalFunctionResolver.h>

// CStdLibResolver Implementation
llvm::Function* CStdLibResolver::resolve(IRGenerator& generator, const std::string& name, llvm::FunctionType* funcType) {
    // Common C standard library functions
    static const std::unordered_set<std::string> cStdLibFunctions = {
        // Memory management
        "malloc", "free", "calloc", "realloc", "aligned_alloc",
        // String functions
        "strlen", "strcpy", "strncpy", "strcmp", "strncmp", "strcat", "strncat",
        "strchr", "strrchr", "strstr", "strtok", "memcpy", "memmove", "memset",
        "memcmp", "memchr",
        // I/O functions
        "printf", "sprintf", "snprintf", "fprintf", "scanf", "sscanf", "fscanf",
        "fopen", "fclose", "fread", "fwrite", "fseek", "ftell", "fflush",
        "putchar", "getchar", "puts", "gets", "fgetc", "fputc", "fgets", "fputs",
        // Math functions
        "sin", "cos", "tan", "asin", "acos", "atan", "atan2", "sinh", "cosh", "tanh",
        "exp", "log", "log10", "pow", "sqrt", "floor", "ceil", "fabs", "fmod",
        // Conversion functions
        "atoi", "atof", "atol", "strtol", "strtod", "strtof",
        // Time functions
        "time", "clock", "difftime", "mktime", "strftime", "gmtime", "localtime",
        // Process control
        "exit", "abort", "atexit", "system", "getenv", "setenv",
        // Character classification
        "isalpha", "isdigit", "isalnum", "isspace", "isupper", "islower",
        "toupper", "tolower"
    };
    
    if (cStdLibFunctions.find(name) != cStdLibFunctions.end()) {
        auto func = llvm::Function::Create(
            funcType,
            llvm::Function::ExternalLinkage,
            name,
            generator.getCurrentModule()
        );
        
        applyPlatformSpecificAttributes(func, name);
        return func;
    }
    
    return nullptr;
}

void CStdLibResolver::applyPlatformSpecificAttributes(llvm::Function* func, const std::string& name) {
    // Apply calling convention
    func->setCallingConv(getPlatformCallingConv(name));
    
    // Apply common attributes for C standard library functions
    if (name == "malloc" || name == "calloc" || name == "realloc") {
        func->addFnAttr(llvm::Attribute::NoAlias);
    }
    
    // Memory functions that don't throw
    static const std::unordered_set<std::string> noThrowFunctions = {
        "memcpy", "memmove", "memset", "memcmp", "memchr",
        "strlen", "strcpy", "strcmp", "free"
    };
    
    if (noThrowFunctions.find(name) != noThrowFunctions.end()) {
        func->addFnAttr(llvm::Attribute::NoUnwind);
    }
}

llvm::CallingConv::ID CStdLibResolver::getPlatformCallingConv(const std::string& name) {
    switch (PlatformInfo::getCurrentPlatform()) {
        case PlatformInfo::Windows:
            // Windows uses different calling conventions
            if (PlatformInfo::getCurrentArchitecture() == PlatformInfo::x86) {
                return llvm::CallingConv::C; // cdecl on x86
            }
            return llvm::CallingConv::Win64; // x64 calling convention
            
        default:
            return llvm::CallingConv::C; // System V ABI for Unix-like systems
    }
}