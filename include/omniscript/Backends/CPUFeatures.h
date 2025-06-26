#pragma once
#include <omniscript/omniscript_pch.h>

#if defined(__i386__) || defined(_M_IX86) || defined(__x86_64__) || defined(_M_X64)
#ifdef _WIN32
#include <intrin.h>
// Forward declare to avoid header conflicts - but handle MinGW case
#ifdef __MINGW32__
// MinGW doesn't have __xgetbv, we'll implement it inline
#else
extern "C" unsigned __int64 __xgetbv(unsigned int);
#endif
#else
#include <cpuid.h>
#endif
#elif defined(__linux__) && (defined(__arm__) || defined(__aarch64__))
#include <fstream>
#include <sstream>
#include <vector>
#elif defined(_WIN32) && (defined(__aarch64__) || defined(_M_ARM64))
#include <windows.h>
#elif defined(__linux__) && defined(__loongarch__)
#include <sys/auxv.h>
#elif defined(__linux__) && defined(__riscv)
#include <sys/syscall.h>
#include <unistd.h>
#include <array>
#endif

class CPUFeatures {
public:
    static std::unordered_map<std::string, bool> getHostCPUFeatures() {
#if defined(__i386__) || defined(_M_IX86) || defined(__x86_64__) || defined(_M_X64)
        return getX86Features();
#elif defined(__linux__) && (defined(__arm__) || defined(__aarch64__))
        return getArmLinuxFeatures();
#elif defined(_WIN32) && (defined(__aarch64__) || defined(_M_ARM64))
        return getArmWindowsFeatures();
#elif defined(__linux__) && defined(__loongarch__)
        return getLoongArchFeatures();
#elif defined(__linux__) && defined(__riscv)
        return getRiscVFeatures();
#else
        return {};
#endif
    }

private:
#if defined(__i386__) || defined(_M_IX86) || defined(__x86_64__) || defined(_M_X64)
    static bool getX86CPUID(unsigned level, unsigned* eax, unsigned* ebx, unsigned* ecx, unsigned* edx) {
#ifdef _WIN32
        int regs[4];
        __cpuid(regs, level);
        *eax = regs[0]; *ebx = regs[1]; *ecx = regs[2]; *edx = regs[3];
        return true;
#else
        return __get_cpuid(level, eax, ebx, ecx, edx) != 0;
#endif
    }

    static bool getX86CPUIDEx(unsigned level, unsigned sublevel, unsigned* eax, unsigned* ebx, unsigned* ecx, unsigned* edx) {
#ifdef _WIN32
        int regs[4];
        __cpuidex(regs, level, sublevel);
        *eax = regs[0]; *ebx = regs[1]; *ecx = regs[2]; *edx = regs[3];
        return true;
#else
        return __get_cpuid_count(level, sublevel, eax, ebx, ecx, edx) != 0;
#endif
    }

    static bool getX86XCR0(unsigned* eax, unsigned* edx) {
#ifdef _WIN32
#ifdef __MINGW32__
        // MinGW implementation using inline assembly
        try {
            unsigned int xcr0_low, xcr0_high;
            __asm__ volatile(".byte 0x0f, 0x01, 0xd0" : "=a"(xcr0_low), "=d"(xcr0_high) : "c"(0));
            *eax = xcr0_low;
            *edx = xcr0_high;
            return true;
        } catch(...) {
            return false;
        }
#else
        // MSVC implementation
        try {
            unsigned __int64 xcr0 = __xgetbv(0);
            *eax = static_cast<unsigned>(xcr0 & 0xFFFFFFFF);
            *edx = static_cast<unsigned>((xcr0 >> 32) & 0xFFFFFFFF);
            return true;
        } catch(...) {
            return false;
        }
#endif
#else
        try {
            unsigned int xcr0_low, xcr0_high;
            __asm__ volatile(".byte 0x0f, 0x01, 0xd0" : "=a"(xcr0_low), "=d"(xcr0_high) : "c"(0));
            *eax = xcr0_low;
            *edx = xcr0_high;
            return true;
        } catch(...) {
            return false;
        }
#endif
    }

    static std::unordered_map<std::string, bool> getX86Features() {
        unsigned eax = 0, ebx = 0, ecx = 0, edx = 0;
        unsigned maxLevel;
        std::unordered_map<std::string, bool> features;

        if (!getX86CPUID(0, &maxLevel, &ebx, &ecx, &edx) || maxLevel < 1)
            return features;

        getX86CPUID(1, &eax, &ebx, &ecx, &edx);

        // Basic features from CPUID leaf 1
        features["cx8"] = (edx >> 8) & 1;
        features["cmov"] = (edx >> 15) & 1;
        features["mmx"] = (edx >> 23) & 1;
        features["fxsr"] = (edx >> 24) & 1;
        features["sse"] = (edx >> 25) & 1;
        features["sse2"] = (edx >> 26) & 1;

        features["sse3"] = (ecx >> 0) & 1;
        features["pclmul"] = (ecx >> 1) & 1;
        features["ssse3"] = (ecx >> 9) & 1;
        features["cx16"] = (ecx >> 13) & 1;
        features["sse4.1"] = (ecx >> 19) & 1;
        features["sse4.2"] = (ecx >> 20) & 1;
        features["crc32"] = features["sse4.2"];
        features["movbe"] = (ecx >> 22) & 1;
        features["popcnt"] = (ecx >> 23) & 1;
        features["aes"] = (ecx >> 25) & 1;
        features["rdrnd"] = (ecx >> 30) & 1;

        // Check XSAVE support for AVX
        bool hasXSave = ((ecx >> 27) & 1) && getX86XCR0(&eax, &edx);
        bool hasAVXSave = hasXSave && ((ecx >> 28) & 1) && ((eax & 0x6) == 0x6);

#if defined(__APPLE__)
        bool hasAVX512Save = true;
#else
        bool hasAVX512Save = hasAVXSave && ((eax & 0xe0) == 0xe0);
#endif
        const unsigned AMXBits = (1 << 17) | (1 << 18);
        bool hasAMXSave = hasXSave && ((eax & AMXBits) == AMXBits);

        features["avx"] = hasAVXSave;
        features["fma"] = ((ecx >> 12) & 1) && hasAVXSave;
        features["xsave"] = ((ecx >> 26) & 1) && hasAVXSave;
        features["f16c"] = ((ecx >> 29) & 1) && hasAVXSave;

        // Extended features
        unsigned maxExtLevel;
        getX86CPUID(0x80000000, &maxExtLevel, &ebx, &ecx, &edx);

        bool hasExtLeaf1 = maxExtLevel >= 0x80000001 && getX86CPUID(0x80000001, &eax, &ebx, &ecx, &edx);
        if (hasExtLeaf1) {
            features["sahf"] = (ecx >> 0) & 1;
            features["lzcnt"] = (ecx >> 5) & 1;
            features["sse4a"] = (ecx >> 6) & 1;
            features["prfchw"] = (ecx >> 8) & 1;
            features["xop"] = ((ecx >> 11) & 1) && hasAVXSave;
            features["lwp"] = (ecx >> 15) & 1;
            features["fma4"] = ((ecx >> 16) & 1) && hasAVXSave;
            features["tbm"] = (ecx >> 21) & 1;
            features["mwaitx"] = (ecx >> 29) & 1;
            features["64bit"] = (edx >> 29) & 1;
        }

        // Structured extended features (leaf 7)
        bool hasLeaf7 = maxLevel >= 7 && getX86CPUIDEx(0x7, 0x0, &eax, &ebx, &ecx, &edx);
        if (hasLeaf7) {
            features["fsgsbase"] = (ebx >> 0) & 1;
            features["sgx"] = (ebx >> 2) & 1;
            features["bmi"] = (ebx >> 3) & 1;
            features["avx2"] = ((ebx >> 5) & 1) && hasAVXSave;
            features["bmi2"] = (ebx >> 8) & 1;
            features["invpcid"] = (ebx >> 10) & 1;
            features["rtm"] = (ebx >> 11) & 1;
            features["avx512f"] = ((ebx >> 16) & 1) && hasAVX512Save;
            if (features["avx512f"])
                features["evex512"] = true;
            features["avx512dq"] = ((ebx >> 17) & 1) && hasAVX512Save;
            features["rdseed"] = (ebx >> 18) & 1;
            features["adx"] = (ebx >> 19) & 1;
            features["avx512ifma"] = ((ebx >> 21) & 1) && hasAVX512Save;
            features["clflushopt"] = (ebx >> 23) & 1;
            features["clwb"] = (ebx >> 24) & 1;
            features["avx512cd"] = ((ebx >> 28) & 1) && hasAVX512Save;
            features["sha"] = (ebx >> 29) & 1;
            features["avx512bw"] = ((ebx >> 30) & 1) && hasAVX512Save;
            features["avx512vl"] = ((ebx >> 31) & 1) && hasAVX512Save;

            features["avx512vbmi"] = ((ecx >> 1) & 1) && hasAVX512Save;
            features["pku"] = (ecx >> 4) & 1;
            features["waitpkg"] = (ecx >> 5) & 1;
            features["avx512vbmi2"] = ((ecx >> 6) & 1) && hasAVX512Save;
            features["shstk"] = (ecx >> 7) & 1;
            features["gfni"] = (ecx >> 8) & 1;
            features["vaes"] = ((ecx >> 9) & 1) && hasAVXSave;
            features["vpclmulqdq"] = ((ecx >> 10) & 1) && hasAVXSave;
            features["avx512vnni"] = ((ecx >> 11) & 1) && hasAVX512Save;
            features["avx512bitalg"] = ((ecx >> 12) & 1) && hasAVX512Save;
            features["avx512vpopcntdq"] = ((ecx >> 14) & 1) && hasAVX512Save;
            features["rdpid"] = (ecx >> 22) & 1;

            features["amx-bf16"] = ((edx >> 22) & 1) && hasAMXSave;
            features["avx512fp16"] = ((edx >> 23) & 1) && hasAVX512Save;
            features["amx-tile"] = ((edx >> 24) & 1) && hasAMXSave;
            features["amx-int8"] = ((edx >> 25) & 1) && hasAMXSave;
        }

        return features;
    }

#elif defined(__linux__) && (defined(__arm__) || defined(__aarch64__))
    static std::unordered_map<std::string, bool> getArmLinuxFeatures() {
        std::unordered_map<std::string, bool> features;
        std::ifstream cpuinfo("/proc/cpuinfo");
        if (!cpuinfo.is_open())
            return features;

        std::string line;
        std::vector<std::string> cpuFeatures;
        
        while (std::getline(cpuinfo, line)) {
            if (line.find("Features") == 0) {
                std::istringstream iss(line.substr(line.find(':') + 1));
                std::string feature;
                while (iss >> feature) {
                    cpuFeatures.push_back(feature);
                }
                break;
            }
        }

#if defined(__aarch64__)
        enum { CAP_AES = 0x1, CAP_PMULL = 0x2, CAP_SHA1 = 0x4, CAP_SHA2 = 0x8 };
        uint32_t crypto = 0;
#endif

        for (const auto& feature : cpuFeatures) {
#if defined(__aarch64__)
            if (feature == "asimd") features["neon"] = true;
            else if (feature == "fp") features["fp-armv8"] = true;
            else if (feature == "crc32") features["crc"] = true;
            else if (feature == "atomics") features["lse"] = true;
            else if (feature == "sve") features["sve"] = true;
            else if (feature == "sve2") features["sve2"] = true;
            else if (feature == "aes") crypto |= CAP_AES;
            else if (feature == "pmull") crypto |= CAP_PMULL;
            else if (feature == "sha1") crypto |= CAP_SHA1;
            else if (feature == "sha2") crypto |= CAP_SHA2;
#else
            if (feature == "half") features["fp16"] = true;
            else if (feature == "neon") features["neon"] = true;
            else if (feature == "vfpv3") features["vfp3"] = true;
            else if (feature == "vfpv3d16") features["vfp3d16"] = true;
            else if (feature == "vfpv4") features["vfp4"] = true;
            else if (feature == "idiva") features["hwdiv-arm"] = true;
            else if (feature == "idivt") features["hwdiv"] = true;
#endif
        }

#if defined(__aarch64__)
        uint32_t aes = CAP_AES | CAP_PMULL;
        uint32_t sha2 = CAP_SHA1 | CAP_SHA2;
        features["aes"] = (crypto & aes) == aes;
        features["sha2"] = (crypto & sha2) == sha2;
#endif

        return features;
    }

#elif defined(_WIN32) && (defined(__aarch64__) || defined(_M_ARM64))
    static std::unordered_map<std::string, bool> getArmWindowsFeatures() {
        std::unordered_map<std::string, bool> features;

        features["neon"] = IsProcessorFeaturePresent(PF_ARM_NEON_INSTRUCTIONS_AVAILABLE);
        features["crc"] = IsProcessorFeaturePresent(PF_ARM_V8_CRC32_INSTRUCTIONS_AVAILABLE);

        bool tradCrypto = IsProcessorFeaturePresent(PF_ARM_V8_CRYPTO_INSTRUCTIONS_AVAILABLE);
        features["aes"] = tradCrypto;
        features["sha2"] = tradCrypto;

        return features;
    }

#elif defined(__linux__) && defined(__loongarch__)
    static std::unordered_map<std::string, bool> getLoongArchFeatures() {
        std::unordered_map<std::string, bool> features;
        
        unsigned long hwcap = getauxval(AT_HWCAP);
        bool hasFPU = hwcap & (1UL << 3); // HWCAP_LOONGARCH_FPU
        
        uint32_t cpucfg2 = 0x2, cpucfg3 = 0x3;
        __asm__("cpucfg %[cpucfg2], %[cpucfg2]\n\t" : [cpucfg2] "+r"(cpucfg2));
        __asm__("cpucfg %[cpucfg3], %[cpucfg3]\n\t" : [cpucfg3] "+r"(cpucfg3));

        features["f"] = hasFPU && (cpucfg2 & (1U << 1));
        features["d"] = hasFPU && (cpucfg2 & (1U << 2));
        features["lsx"] = hwcap & (1UL << 4);
        features["lasx"] = hwcap & (1UL << 5);
        features["lvz"] = hwcap & (1UL << 9);

        return features;
    }

#elif defined(__linux__) && defined(__riscv)
    struct RISCVHwProbe {
        long long key;
        unsigned long long value;
    };

    static std::unordered_map<std::string, bool> getRiscVFeatures() {
        std::unordered_map<std::string, bool> features;
        
        std::array<RISCVHwProbe, 3> query = {{
            {3, 0}, // RISCV_HWPROBE_KEY_BASE_BEHAVIOR
            {4, 0}, // RISCV_HWPROBE_KEY_IMA_EXT_0
            {9, 0}  // RISCV_HWPROBE_KEY_MISALIGNED_SCALAR_PERF
        }};

        int ret = syscall(258, query.data(), query.size(), 0, 0, 0);
        if (ret != 0)
            return features;

        uint64_t baseMask = query[0].value;
        if (baseMask & 1) {
            features["i"] = true;
            features["m"] = true;
            features["a"] = true;
        }

        uint64_t extMask = query[1].value;
        features["f"] = extMask & (1 << 0);
        features["d"] = extMask & (1 << 0);
        features["c"] = extMask & (1 << 1);
        features["v"] = extMask & (1 << 2);
        features["zba"] = extMask & (1 << 3);
        features["zbb"] = extMask & (1 << 4);
        features["zbs"] = extMask & (1 << 5);

        return features;
    }
#endif
};