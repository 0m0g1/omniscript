#include <omniscript/engine/Backends/LLVM/IRGenerator.h>
#include <omniscript/engine/Backends/LLVM/LLVMExternalFunctionResolver.h>
#include <omniscript/engine/Backends/LLVM/ExternalFunctionResolvers/DarwinLLVMResolver.h>

llvm::Function* DarwinResolver::resolve(IRGenerator& generator, const std::string& name, 
                                      llvm::FunctionType* funcType, LinkDependencies& deps) {
    if (!PlatformInfo::isApple()) {
        return nullptr; // Only resolve on macOS/iOS
    }
    
    std::string requiredFramework = getRequiredFramework(name);
    if (requiredFramework.empty()) {
        return nullptr; // Unknown Darwin function
    }
    
    // Create the function
    llvm::Function* func = llvm::Function::Create(
        funcType, llvm::Function::ExternalLinkage, name, generator.getCurrentModule()
    );
    
    // Add framework dependency
    if (requiredFramework != "System") {
        LinkDependencies::LibraryInfo info;
        info.name = requiredFramework;
        info.linkerFlags = {"-framework", requiredFramework};
        info.isSystemLib = true;
        deps.addRequiredLibrary(requiredFramework, info);
    }
    
    return func;
}

bool DarwinResolver::isFoundationFunction(const std::string& name) {
    return name.find("NS") == 0 || name.find("CF") == 0;
}

bool DarwinResolver::isCoreFoundationFunction(const std::string& name) {
    return name.find("CF") == 0;
}

bool DarwinResolver::isCocoaFunction(const std::string& name) {
    return name.find("NS") == 0 && name != "NSLog"; // NSLog is special
}

std::string DarwinResolver::getRequiredFramework(const std::string& name) {
    if (isCoreFoundationFunction(name)) return "CoreFoundation";
    if (isFoundationFunction(name)) return "Foundation";
    if (name == "NSLog") return "Foundation";
    
    // Add more framework mappings
    static const std::unordered_map<std::string, std::string> functionToFramework = {
        {"CGContextDrawImage", "CoreGraphics"},
        {"CGImageCreateWithImageInRect", "CoreGraphics"},
        {"CALayer", "QuartzCore"},
        {"objc_msgSend", "System"}, // In libobjc, usually linked by default
        {"class_getName", "System"},
        {"sel_getName", "System"}
    };
    
    auto it = functionToFramework.find(name);
    if (it != functionToFramework.end()) {
        return it->second;
    }
    
    return "";
}