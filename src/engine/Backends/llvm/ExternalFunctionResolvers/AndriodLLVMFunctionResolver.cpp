#include <omniscript/engine/Backends/LLVM/IRGenerator.h>
#include <omniscript/engine/Backends/LLVM/LLVMExternalFunctionResolver.h>

// AndroidResolver Implementation
llvm::Function* AndroidResolver::resolve(IRGenerator& generator, const std::string& name, llvm::FunctionType* funcType) {
    if (PlatformInfo::getCurrentPlatform() != PlatformInfo::Android) {
        return nullptr;
    }
    
    if (isAndroidFunction(name) || isBionicFunction(name) || isJNIFunction(name)) {
        auto func = llvm::Function::Create(
            funcType,
            llvm::Function::ExternalLinkage,
            name,
            generator.getCurrentModule()
        );
        
        func->setCallingConv(llvm::CallingConv::C);
        return func;
    }
    
    return nullptr;
}

bool AndroidResolver::isAndroidFunction(const std::string& name) {
    return name.find("Android") != std::string::npos || 
           name.find("AAsset") != std::string::npos ||
           name.find("ANativeWindow") != std::string::npos;
}

bool AndroidResolver::isBionicFunction(const std::string& name) {
    // Bionic-specific functions
    static const std::unordered_set<std::string> bionicFunctions = {
        "arc4random", "arc4random_uniform", "getauxval", "android_get_device_api_level"
    };
    
    return bionicFunctions.find(name) != bionicFunctions.end();
}

bool AndroidResolver::isJNIFunction(const std::string& name) {
    return name.find("JNI") != std::string::npos || name.find("Java_") == 0;
}

std::string AndroidResolver::getRequiredLibrary(const std::string& name) {
    if (isAndroidFunction(name)) return "android";
    if (isJNIFunction(name)) return "jni";
    return "c";
}