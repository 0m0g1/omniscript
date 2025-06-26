#include <omniscript/Backends/LLVM/IRGenerator.h>
#include <omniscript/Core/CPUFeatures.h>

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

// Use TargetInfo utilities to resolve CPU name properly
std::string IRGenerator::resolveCPUName(const std::string& triple) {
    if (configs.cpuFeatures == "native") {
        // Use LLVM's host CPU detection for native
        std::string hostCPU = llvm::sys::getHostCPUName().str();
        if (!hostCPU.empty() && hostCPU != "generic") {
            return hostCPU;
        }
        
        // Fallback to TargetInfo's default CPU for detected architecture
        TargetInfo::TargetTriple parsedTriple = TargetInfo::TargetTriple::parse(triple);
        TargetArch arch = getTargetArchFromTriple(parsedTriple);
        return TargetInfo::getDefaultCPUForArch(arch);
    } else if (!configs.cpuFeatures.empty()) {
        return configs.cpuFeatures;
    } else {
        // Use TargetInfo to get default CPU for the architecture
        TargetInfo::TargetTriple parsedTriple = TargetInfo::TargetTriple::parse(triple);
        TargetArch arch = getTargetArchFromTriple(parsedTriple);
        return TargetInfo::getDefaultCPUForArch(arch);
    }
}

// Build feature string using TargetInfo and user configuration
std::string IRGenerator::buildFeatureString(const std::string& triple) {
    DEBUG_LOG("Building the features string");

    std::string features;

    int enabledCount = 0;
    int disabledCount = 0;

    // Add explicitly enabled features
    for (const auto& feature : configs.enabledFeatures) {
        if (!features.empty()) features += ",";
        features += "+" + feature;
        DEBUG_LOG("Enabled feature: +" + feature);
        enabledCount++;
    }

    // Add explicitly disabled features
    for (const auto& feature : configs.disabledFeatures) {
        if (!features.empty()) features += ",";
        features += "-" + feature;
        DEBUG_LOG("Disabled feature: -" + feature);
        disabledCount++;
    }

    // Add host features only if CPU is native and no manual features specified
    if (configs.cpuFeatures == "native" &&
        configs.enabledFeatures.empty() &&
        configs.disabledFeatures.empty()) {

        DEBUG_LOG("Detecting native CPU features...");

        auto hostFeatures = CPUFeatures::getHostCPUFeatures();

        int nativeEnabledCount = 0;
        for (const auto& feature : hostFeatures) {
            const std::string& name = feature.first;
            bool isEnabled = feature.second;

            DEBUG_LOG("Host feature: " + name + " = " + (isEnabled ? "enabled" : "disabled"));

            if (isEnabled) {
                if (!features.empty()) features += ",";
                features += "+" + name;
                nativeEnabledCount++;
            }
        }

        DEBUG_LOG("Native enabled features count: " + std::to_string(nativeEnabledCount));
        DEBUG_LOG("Done getting the features");
    }

    DEBUG_LOG("Total explicitly enabled features: " + std::to_string(enabledCount));
    DEBUG_LOG("Total explicitly disabled features: " + std::to_string(disabledCount));
    DEBUG_LOG("Final feature string: " + features);

    return features;
}

// Convert LLVM triple to TargetInfo TargetArch enum
TargetArch IRGenerator::getTargetArchFromTriple(const TargetInfo::TargetTriple& triple) {
    std::string arch = triple.arch;
    std::transform(arch.begin(), arch.end(), arch.begin(), ::tolower);
    
    if (arch == "x86_64" || arch == "x86-64" || arch == "amd64") {
        return TargetArch::X86_64;
    } else if (arch == "aarch64" || arch == "arm64") {
        return TargetArch::ARM64;
    } else if (arch == "i386" || arch == "i486" || arch == "i586" || arch == "i686") {
        return TargetArch::X86_32;
    } else if (arch == "arm" || arch == "armv6" || arch == "armv7") {
        return TargetArch::ARM32;
    } else if (arch == "riscv64") {
        return TargetArch::RISCV64;
    } else if (arch == "wasm32") {
        return TargetArch::WASM32;
    } else if (arch == "wasm64") {
        return TargetArch::WASM64;
    } else {
        return TargetArch::Auto; // Will auto-detect
    }
}

// Validate target triple compatibility using TargetInfo
bool IRGenerator::validateTargetTripleCompatibility(const std::string& triple, const std::string& cpu) {
    // First, use TargetInfo to validate the triple format
    if (!TargetInfo::isValidTriple(triple)) {
        llvm::errs() << "Invalid target triple format: " << triple << "\n";
        return false;
    }
    
    // Parse and normalize the triple
    TargetInfo::TargetTriple normalized = TargetInfo::normalizeTriple(triple);
    TargetArch arch = getTargetArchFromTriple(normalized);
    
    // Check architecture-specific constraints
    if (arch == TargetArch::X86_64) {
        // For x86_64 targets, ensure we're not using 32-bit only CPUs
        static const std::unordered_set<std::string> incompatibleCPUs = {
            "i386", "i486", "i586"
        };
        
        if (incompatibleCPUs.find(cpu) != incompatibleCPUs.end()) {
            llvm::errs() << "Warning: CPU '" << cpu 
                         << "' may not support 64-bit mode for triple '" << triple << "'\n";
            return false;
        }
    }
    
    // Additional validation using TargetInfo
    TargetInfo::ArchitectureInfo archInfo = TargetInfo::getArchitectureInfo(arch);
    
    // Check if the triple requests 64-bit but architecture doesn't support it
    bool tripleIs64Bit = (normalized.arch.find("64") != std::string::npos);
    if (tripleIs64Bit && !archInfo.is64Bit) {
        llvm::errs() << "Target triple requests 64-bit but architecture '" 
                     << archInfo.name << "' doesn't support it\n";
        return false;
    }
    
    // Check if the triple requests 32-bit but we're on 64-bit only arch
    bool tripleIs32Bit = (normalized.arch.find("32") != std::string::npos || 
                         normalized.arch == "i386" || normalized.arch == "arm");
    if (tripleIs32Bit && archInfo.is64Bit && archInfo.name == "arm64") {
        // ARM64 can run ARM32, so this is OK
    } else if (tripleIs32Bit && archInfo.is64Bit && archInfo.name != "x86_64") {
        llvm::errs() << "Target triple requests 32-bit but architecture '" 
                     << archInfo.name << "' is 64-bit only\n";
        return false;
    }
    
    if (configs.diagnostics.verbose) {
        llvm::outs() << "Target validation passed:\n";
        llvm::outs() << "  Normalized triple: " << normalized.toString() << "\n";
        llvm::outs() << "  Architecture: " << archInfo.name << " (" << archInfo.pointerSize << "-byte pointers)\n";
        llvm::outs() << "  CPU: " << cpu << "\n";
    }
    
    return true;
}