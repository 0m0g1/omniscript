#include <string>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <cstring>
#include "target_config.h" // Include your target config

#ifdef _WIN32
#include <intrin.h>
#else
#include <cpuid.h>
#endif

class FeatureDetector {
private:
    struct CPUIDResult {
        uint32_t eax, ebx, ecx, edx;
    };
    
    static CPUIDResult getCPUID(uint32_t leaf, uint32_t subleaf = 0) {
        CPUIDResult result = {0, 0, 0, 0};
        
#ifdef _WIN32
        int info[4];
        __cpuidex(info, leaf, subleaf);
        result.eax = info[0];
        result.ebx = info[1];
        result.ecx = info[2];
        result.edx = info[3];
#else
        __cpuid_count(leaf, subleaf, result.eax, result.ebx, result.ecx, result.edx);
#endif
        return result;
    }
    
    static std::unordered_map<std::string, bool> detectX86Features() {
        std::unordered_map<std::string, bool> features;
        
        try {
            // Basic feature detection
            auto basic = getCPUID(1);
            
            // CPUID.01H:ECX features
            features["sse3"] = (basic.ecx & (1 << 0)) != 0;
            features["pclmul"] = (basic.ecx & (1 << 1)) != 0;
            features["ssse3"] = (basic.ecx & (1 << 9)) != 0;
            features["sse4.1"] = (basic.ecx & (1 << 19)) != 0;
            features["sse4.2"] = (basic.ecx & (1 << 20)) != 0;
            features["aes"] = (basic.ecx & (1 << 25)) != 0;
            features["avx"] = (basic.ecx & (1 << 28)) != 0;
            features["f16c"] = (basic.ecx & (1 << 29)) != 0;
            features["rdrand"] = (basic.ecx & (1 << 30)) != 0;
            
            // CPUID.01H:EDX features
            features["sse"] = (basic.edx & (1 << 25)) != 0;
            features["sse2"] = (basic.edx & (1 << 26)) != 0;
            
            // Extended features (CPUID.07H:EBX)
            auto extended = getCPUID(7);
            features["avx2"] = (extended.ebx & (1 << 5)) != 0;
            features["bmi"] = (extended.ebx & (1 << 3)) != 0;
            features["bmi2"] = (extended.ebx & (1 << 8)) != 0;
            features["avx512f"] = (extended.ebx & (1 << 16)) != 0;
            features["avx512dq"] = (extended.ebx & (1 << 17)) != 0;
            features["avx512cd"] = (extended.ebx & (1 << 28)) != 0;
            features["avx512bw"] = (extended.ebx & (1 << 30)) != 0;
            features["avx512vl"] = (extended.ebx & (1 << 31)) != 0;
            
            // CPUID.07H:ECX features
            features["avx512vbmi"] = (extended.ecx & (1 << 1)) != 0;
            features["avx512vbmi2"] = (extended.ecx & (1 << 6)) != 0;
            features["avx512vnni"] = (extended.ecx & (1 << 11)) != 0;
            features["avx512bitalg"] = (extended.ecx & (1 << 12)) != 0;
            features["avx512vpopcntdq"] = (extended.ecx & (1 << 14)) != 0;
            
        } catch (...) {
            // If CPUID fails, return empty map
            features.clear();
        }
        
        return features;
    }
    
    static std::unordered_map<std::string, bool> detectARMFeatures() {
        std::unordered_map<std::string, bool> features;
        
        // Use compile-time detection for ARM features
        #ifdef __ARM_NEON
            features["neon"] = true;
        #endif
        
        #ifdef __ARM_FEATURE_CRC32
            features["crc"] = true;
        #endif
        
        #ifdef __ARM_FEATURE_CRYPTO
            features["crypto"] = true;
        #endif
        
        #ifdef __ARM_FEATURE_FP16_VECTOR_ARITHMETIC
            features["fp16"] = true;
        #endif
        
        // Add more ARM-specific detections as needed
        return features;
    }
    
public:
    static std::unordered_map<std::string, bool> getHostFeatures() {
        std::unordered_map<std::string, bool> features;
        
        // Use TargetInfo to detect host architecture
        TargetArch hostArch = TargetInfo::detectHostArchitecture();
        
        switch (hostArch) {
            case TargetArch::X86_64:
            case TargetArch::X86_32:
                features = detectX86Features();
                break;
                
            case TargetArch::ARM64:
            case TargetArch::ARM32:
                features = detectARMFeatures();
                break;
                
            case TargetArch::RISCV64:
                // Add RISC-V feature detection if needed
                break;
                
            case TargetArch::WASM32:
            case TargetArch::WASM64:
                // WASM features are compile-time determined
                features["simd128"] = true; // Usually available
                break;
                
            default:
                break;
        }
        
        return features;
    }
    
    // Integration with TargetInfo: filter features by target architecture
    static std::unordered_map<std::string, bool> getFeaturesForTarget(TargetArch targetArch) {
        auto detectedFeatures = getHostFeatures();
        auto availableFeatures = TargetInfo::getAvailableFeatures(targetArch);
        
        std::unordered_map<std::string, bool> validFeatures;
        
        for (const auto& feature : detectedFeatures) {
            if (feature.second && // Feature is enabled
                std::find(availableFeatures.begin(), availableFeatures.end(), feature.first) != availableFeatures.end()) {
                validFeatures[feature.first] = true;
            }
        }
        
        return validFeatures;
    }
};