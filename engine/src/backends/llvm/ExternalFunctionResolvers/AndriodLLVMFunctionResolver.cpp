#include <omniscript/backends/llvm/IRGenerator.h>
#include <omniscript/backends/llvm/LLVMExternalFunctionResolver.h>
#include <omniscript/backends/llvm/ExternalFunctionResolvers/AndroidLLVMResolver.h>

namespace Omniscript {
llvm::Function* AndroidResolver::resolve(IRGenerator& generator, const std::string& name, 
                                       llvm::FunctionType* funcType, LinkDependencies& deps) {
    if (PlatformInfo::getCurrentPlatform() != PlatformInfo::Android) {
        return nullptr; // Only resolve on Android
    }
    
    std::string requiredLib = getRequiredLibrary(name);
    if (requiredLib.empty()) {
        return nullptr; // Unknown Android function
    }
    
    // Create the function
    llvm::Function* func = llvm::Function::Create(
        funcType, llvm::Function::ExternalLinkage, name, generator.getCurrentModule()
    );
    
    // Add library dependency
    if (requiredLib != "c") {
        LinkDependencies::LibraryInfo info;
        info.name = requiredLib;
        info.isSystemLib = true;
        deps.addRequiredLibrary(requiredLib, info);
    }
    
    return func;
}

bool AndroidResolver::isAndroidFunction(const std::string& name) {
    return name.find("android_") == 0 || name.find("ANeuralNetworks") == 0 ||
           name.find("AAsset") == 0 || name.find("AConfiguration") == 0;
}

bool AndroidResolver::isBionicFunction(const std::string& name) {
    static const std::unordered_set<std::string> bionicFunctions = {
        "__android_log_print", "__android_log_write", "__android_log_vprint",
        "arc4random", "arc4random_uniform", "getauxval", "pthread_gettid_np"
    };
    return bionicFunctions.find(name) != bionicFunctions.end();
}

bool AndroidResolver::isJNIFunction(const std::string& name) {
    return name.find("JNI_") == 0 || name.find("Java_") == 0;
}

std::string AndroidResolver::getRequiredLibrary(const std::string& name) {
    if (name.find("__android_log") == 0) return "log";
    if (name.find("ANeuralNetworks") == 0) return "neuralnetworks";
    if (name.find("AAsset") == 0) return "android";
    if (name.find("OpenSL") == 0) return "OpenSLES";
    if (name.find("EGL") == 0) return "EGL";
    if (name.find("gl") == 0) return "GLESv2";
    if (isBionicFunction(name)) return "c";
    if (isJNIFunction(name)) return ""; // Usually linked automatically
    
    return "";
}

} //namespace Omniscript
