#include <omniscript/Backends/LLVM/IRGenerator.h>
#include <omniscript/Backends/LLVM/ExternalFunctionResolvers/StaticLibraryLLVMResolver.h>

namespace Omniscript {
StaticLibraryResolver::StaticLibraryResolver() = default;

StaticLibraryResolver::StaticLibraryResolver(const std::string& libPath) 
    : specifiedLibraryPath(libPath) {}

void StaticLibraryResolver::setLibraryPath(const std::string& libPath) {
    specifiedLibraryPath = libPath;
}

const std::string& StaticLibraryResolver::getLibraryPath() const {
    return specifiedLibraryPath;
}

llvm::Function* StaticLibraryResolver::resolve(IRGenerator& generator, const std::string& name,
                       llvm::FunctionType* funcType, LinkDependencies& deps) {
    // Create the function declaration
    llvm::Function* func = llvm::Function::Create(
        funcType, llvm::Function::ExternalLinkage, name, generator.getCurrentModule()
    );
    
    // If the user specified a static library path, split it into directory and lib name
    if (!specifiedLibraryPath.empty()) {
        std::filesystem::path fullPath(specifiedLibraryPath);
        
        std::string libDir = fullPath.parent_path().string();
        std::string libFile = fullPath.stem().string(); // e.g., "libglfw3"
        
        // Remove 'lib' prefix if present
        if (libFile.rfind("lib", 0) == 0) {
            libFile = libFile.substr(3); // drop "lib"
        }
        
        // Create library info with proper linker flags
        LinkDependencies::LibraryInfo info;
        info.name = libFile;
        info.isSystemLib = false;
        
        // Add both -L and -l flags as separate entries
        if (!libDir.empty()) {
            info.linkerFlags.push_back("-L" + libDir);
        }
        info.linkerFlags.push_back("-l" + libFile);
        
        deps.addRequiredLibrary(libFile, info);
    }
    
    return func;
}

bool StaticLibraryResolver::isValidLibraryPath(const std::string& libPath) const {
    if (libPath.empty()) return false;
    
    std::filesystem::path path(libPath);
    
    // Check if it's a valid path structure
    if (!path.has_filename()) return false;
    
    // Check common static library extensions
    std::string ext = path.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    
    return (ext == ".a" || ext == ".lib" || ext == ".o" || ext.empty());
}

std::string StaticLibraryResolver::generateLinkerCommand(const LinkDependencies& deps) const {
    std::string command = "ld ";
    
    auto flags = deps.getLinkerFlags();
    for (const auto& flag : flags) {
        command += flag + " ";
    }
    
    return command;
}

} //namespace Omniscript
