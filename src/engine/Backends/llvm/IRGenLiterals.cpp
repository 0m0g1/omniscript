#include <omniscript/engine/Backends/LLVM/IRGenerator.h>

llvm::Value* IRGenerator::createNullPointer() {
    return llvm::ConstantPointerNull::get(llvm::PointerType::getUnqual(llvm::Type::getInt8Ty(*Context)));
}

llvm::Value* IRGenerator::createNullValue() {
    return llvm::UndefValue::get(llvm::Type::getVoidTy(*Context));
}

// Generate IR for different types
// ============================== Generate IR for Numeric Types ============================== //
// Create an 8-bit integer (i8)
llvm::Value* IRGenerator::create8BitInteger(int8_t value) {
    return llvm::ConstantInt::get(llvm::Type::getInt8Ty(*Context), value, true);
}

// Create a 16-bit integer (i16)
llvm::Value* IRGenerator::create16BitInteger(int16_t value) {
    return llvm::ConstantInt::get(llvm::Type::getInt16Ty(*Context), value, true);
}

// Create a 32-bit integer (i32)
llvm::Value* IRGenerator::create32BitInteger(int32_t value) {
    return llvm::ConstantInt::get(llvm::Type::getInt32Ty(*Context), value, true);
}

// Create a 64-bit integer (i64)
llvm::Value* IRGenerator::create64BitInteger(int64_t value) {
    return llvm::ConstantInt::get(llvm::Type::getInt64Ty(*Context), value, true);
}

llvm::Value* IRGenerator::create80BitFloat(long double value) {
    llvm::LLVMContext& C = *Context;

    #if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
        // On x86/x64 GCC/Clang: long double is 80-bit (x87 extended precision)
        if (sizeof(long double) == 10 || sizeof(long double) == 12 || sizeof(long double) == 16) {
            llvm::Type* f80Type = llvm::Type::getX86_FP80Ty(C);

            // Zero out buffer to avoid garbage in high bits
            uint8_t buffer[16] = {};
            std::memcpy(buffer, &value, sizeof(long double));

            // Extract 80-bit value: low 64 bits + upper 16 bits
            uint64_t low = *reinterpret_cast<const uint64_t*>(buffer);
            uint16_t high = *reinterpret_cast<const uint16_t*>(buffer + 8);
            llvm::APInt api(80, {low, static_cast<uint64_t>(high)});
            llvm::APFloat apf(llvm::APFloat::x87DoubleExtended(), api);
            return llvm::ConstantFP::get(C, apf);
        }
    #endif

    #if defined(__aarch64__) || defined(__powerpc64__) || defined(__wasm__) || defined(_MSC_VER)
        // Common fallback: long double is 64-bit or 128-bit IEEE (same as double or quadfloat)
        if (sizeof(long double) == 8) {
            llvm::Type* f64Type = llvm::Type::getDoubleTy(C);
            return llvm::ConstantFP::get(f64Type, static_cast<double>(value));
        } else if (sizeof(long double) == 16) {
            llvm::Type* f128Type = llvm::Type::getFP128Ty(C);

            uint8_t buffer[16] = {};
            std::memcpy(buffer, &value, 16);
            llvm::APInt api(128, { 
                *reinterpret_cast<const uint64_t*>(&buffer[0]), 
                *reinterpret_cast<const uint64_t*>(&buffer[8]) 
            });
            llvm::APFloat apf(llvm::APFloat::IEEEquad(), api);
            return llvm::ConstantFP::get(C, apf);
        }
    #endif

    // If platform unknown or unsupported long double layout
    console.error("Unsupported long double format on this platform.\n");
    return nullptr;
    // llvm::errs() << "Unsupported long double format on this platform.\n";
    // std::abort();
    // return nullptr;
}

 // Target-specific 16-bit float handling
#ifdef __ARM_ARCH
llvm::Value* IRGenerator::create16BitFloat(__fp16 value) {
    llvm::Type* type = llvm::Type::getHalfTy(*Context); // LLVM 16-bit float type
    return llvm::ConstantFP::get(type, static_cast<double>(value)); // safe conversion
}

#elif defined(__x86_64__) || defined(__i386__)
llvm::Value* IRGenerator::create16BitFloat(_Float16 value) {
    llvm::Type* type = llvm::Type::getHalfTy(*Context);
    return llvm::ConstantFP::get(type, static_cast<double>(value));
}
#endif

// Create a 32-bit floating-point (float)
llvm::Value* IRGenerator::create32BitFloat(float value) {
    return llvm::ConstantFP::get(llvm::Type::getFloatTy(*Context), value);
}

// Create a 64-bit floating-point (double)
llvm::Value* IRGenerator::create64BitFloat(double value) {
    return llvm::ConstantFP::get(llvm::Type::getDoubleTy(*Context), value);
}

// Create a 128-bit floating-point (FP128)
llvm::Value* IRGenerator::create128BitFloat(__float128 value) {
    return llvm::ConstantFP::get(llvm::Type::getFP128Ty(*Context), static_cast<double>(value));  // FP128 is typically represented as a double in LLVM
}

// Create an arbitrary-precision integer (BigInt)
llvm::Value* IRGenerator::createBigInt(const std::string& str, unsigned bitSize) {
    // #if SIMD_OPTIMIZATION_LEVEL == 512
    //     return createBigIntAVX512(str, bitSize);
    // #elif SIMD_OPTIMIZATION_LEVEL == 256
    //     return createBigIntAVX2(str, bitSize);
    // #elif SIMD_OPTIMIZATION_LEVEL == 128
    //     return createBigIntAVX(str, bitSize);
    // #else
    //     llvm::APInt bigIntValue(bitSize, str, 10);
    //     return llvm::ConstantInt::get(llvm::Type::getIntNTy(*Context, bitSize), bigIntValue);
    // #endif
    llvm::APInt bigIntValue(bitSize, str, 10);
    return llvm::ConstantInt::get(llvm::Type::getIntNTy(*Context, bitSize), bigIntValue);
}

llvm::Value* IRGenerator::createBigIntAVX512(const std::string& str, unsigned bitSize) {
    // __m512i bigIntVec = _mm512_setzero_si512(); // Zero-initialize 512-bit register
    return createBigInt(str, 512);  // Use this as placeholder for now
}

llvm::Value* IRGenerator::createBigIntAVX2(const std::string& str, unsigned bitSize) {
    // __m256i bigIntVec = _mm256_setzero_si256(); // Zero-initialize 256-bit register
    return createBigInt(str, 256);
}

llvm::Value* IRGenerator::createBigIntAVX(const std::string& str, unsigned bitSize) {
    __m128i bigIntVec = _mm_setzero_si128(); // Zero-initialize 128-bit register
    return createBigInt(str, 128);
}

llvm::Value* IRGenerator::createUnsigned8BitInteger(uint8_t value) {
    return llvm::ConstantInt::get(llvm::Type::getInt8Ty(*Context), value, false);  // Unsigned 8-bit integer
}

llvm::Value* IRGenerator::createUnsigned16BitInteger(uint16_t value) {
    return llvm::ConstantInt::get(llvm::Type::getInt16Ty(*Context), value, false);  // Unsigned 16-bit integer
}

llvm::Value* IRGenerator::createUnsigned32BitInteger(uint32_t value) {
    return llvm::ConstantInt::get(llvm::Type::getInt32Ty(*Context), value, false);  // Unsigned 32-bit integer
}

llvm::Value* IRGenerator::createUnsigned64BitInteger(uint64_t value) {
    return llvm::ConstantInt::get(llvm::Type::getInt64Ty(*Context), value, false);  // Unsigned 64-bit integer
}


// Create an 8-bit character (char)
llvm::Value* IRGenerator::createChar(char value) {
    return llvm::ConstantInt::get(llvm::Type::getInt8Ty(*Context), value, true);
}

// Create a 16-bit character (char16_t)
llvm::Value* IRGenerator::createChar16(char16_t value) {
    return llvm::ConstantInt::get(llvm::Type::getInt16Ty(*Context), value, false);
}

// Create a 32-bit character (char32_t)
llvm::Value* IRGenerator::createChar32(char32_t value) {
    return llvm::ConstantInt::get(llvm::Type::getInt32Ty(*Context), value, false);
}

// Create an 8-bit (UTF-8) string
llvm::Value* IRGenerator::createUTF8String(const std::string& str) {
    return Builder->CreateGlobalString(str, ".utf8str");
}

// Create a 16-bit (UTF-16) string
llvm::Value* IRGenerator::createUTF16String(const std::u16string& str) {
    std::vector<llvm::Constant*> chars;
    for (char16_t c : str) {
        chars.push_back(llvm::ConstantInt::get(llvm::Type::getInt16Ty(*Context), c, false));
    }
    chars.push_back(llvm::ConstantInt::get(llvm::Type::getInt16Ty(*Context), 0)); // Null terminator

    llvm::ArrayType* arrayType = llvm::ArrayType::get(llvm::Type::getInt16Ty(*Context), chars.size());
    llvm::Constant* array = llvm::ConstantArray::get(arrayType, chars);

    auto global = new llvm::GlobalVariable(
        *Module, arrayType, true, llvm::GlobalValue::PrivateLinkage, array, ".utf16str");

    return global;
}

// Create a 32-bit (UTF-32) string
llvm::Value* IRGenerator::createUTF32String(const std::u32string& str) {
    std::vector<llvm::Constant*> chars;
    for (char32_t c : str) {
        chars.push_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(*Context), c, false));
    }
    chars.push_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(*Context), 0)); // Null terminator

    llvm::ArrayType* arrayType = llvm::ArrayType::get(llvm::Type::getInt32Ty(*Context), chars.size());
    llvm::Constant* array = llvm::ConstantArray::get(arrayType, chars);

    auto global = new llvm::GlobalVariable(
        *Module, arrayType, true, llvm::GlobalValue::PrivateLinkage, array, ".utf32str");

    return global;
}

llvm::Value* IRGenerator::createBool(bool value) {
    return llvm::ConstantInt::get(llvm::Type::getInt1Ty(*Context), value ? 1 : 0, false);
}

// Create a raw pointer from an integer address
llvm::Value* IRGenerator::createRawPointer(uintptr_t address, llvm::Type* pointeeType) {
    // 1. Convert address to integer
    llvm::Type* intPtrTy = llvm::Type::getIntNTy(*Context, sizeof(uintptr_t) * 8);
    llvm::Value* addrValue = llvm::ConstantInt::get(intPtrTy, address, false);
    
    // 2. Create pointer type
    llvm::PointerType* ptrTy = llvm::PointerType::get(pointeeType, 0); // Default address space
    
    // 3. Convert integer to pointer
    return Builder->CreateIntToPtr(addrValue, ptrTy);
}