#include <omniscript/Backends/LLVM/IRGenerator.h>
#include <omniscript/Expressions/CastExpression.h>
#include <omniscript/Expressions/LiteralExpressions.h>

llvm::Value* IRGenerator::codegenPrimitive(std::shared_ptr<Omniscript::Expression> value, SymbolTableType scope) {
    if (auto integer8 = std::dynamic_pointer_cast<Omniscript::Integer<int8_t>>(value)) {
        return create8BitInteger(integer8->getValue());
    
    } else if (auto integer16 = std::dynamic_pointer_cast<Omniscript::Integer<int16_t>>(value)) {
        return create16BitInteger(integer16->getValue());
    
    } else if (auto integer32 = std::dynamic_pointer_cast<Omniscript::Integer<int32_t>>(value)) {
        return create32BitInteger(integer32->getValue());
    
    } else if (auto integer64 = std::dynamic_pointer_cast<Omniscript::Integer<int64_t>>(value)) {
        return create64BitInteger(integer64->getValue());

    } else if (auto unsignedInteger8 = std::dynamic_pointer_cast<Omniscript::Integer<uint8_t>>(value)) {
        return createUnsigned8BitInteger(unsignedInteger8->getValue());
    
    } else if (auto unsignedInteger16 = std::dynamic_pointer_cast<Omniscript::Integer<uint16_t>>(value)) {
        return createUnsigned16BitInteger(unsignedInteger16->getValue());
    
    } else if (auto unsignedInteger32 = std::dynamic_pointer_cast<Omniscript::Integer<uint32_t>>(value)) {
        return createUnsigned32BitInteger(unsignedInteger32->getValue());
    } else if (auto unsignedInteger64 = std::dynamic_pointer_cast<Omniscript::Integer<uint64_t>>(value)) {
        return createUnsigned64BitInteger(unsignedInteger64->getValue());
    } else if (auto boolean = std::dynamic_pointer_cast<Omniscript::Primitive<bool>>(value)) {
        return createBool(boolean->getValue());
    }

    #ifdef __ARM_ARCH
        else if (auto halfPrimitive = std::dynamic_pointer_cast<Omniscript::Float<__fp16>>(value)) {
            return create16BitFloat(halfPrimitive->getValue());
        }
    #elif defined(__x86_64__) || defined(__i386__)
        else if (auto halfPrimitive = std::dynamic_pointer_cast<Omniscript::Float<_Float16>>(value)) {
            return create16BitFloat(halfPrimitive->getValue());
        }
    #endif

    else if (auto floatPrimitive = std::dynamic_pointer_cast<Omniscript::Float<float>>(value)) {
        return create32BitFloat(floatPrimitive->getValue());
    
    } else if (auto doublePrimitive = std::dynamic_pointer_cast<Omniscript::Float<double>>(value)) {
        return create64BitFloat(doublePrimitive->getValue());
    
    } else if (auto longDoublePrimitive = std::dynamic_pointer_cast<Omniscript::Float<long double>>(value)) {
        return create80BitFloat(longDoublePrimitive->getValue());
    
    } else if (auto fp128Primitive = std::dynamic_pointer_cast<Omniscript::Float<__float128>>(value)) {
        return create128BitFloat(fp128Primitive->getValue());
    
    } else if (auto charPrimitive = std::dynamic_pointer_cast<Omniscript::Primitive<char>>(value)) {
        DEBUG_LOG("Creating and int8 char from Primitive<char>");
        return createChar(charPrimitive->getValue());
    
    } else if (auto stringPrimitiveUTF8 = std::dynamic_pointer_cast<Omniscript::Primitive<std::string>>(value)) {
        DEBUG_LOG("Creating UTF-8 string from Primitive<std::string>");
        return createUTF8String(stringPrimitiveUTF8->getValue());
    
    } else if (auto stringPrimitiveUTF16 = std::dynamic_pointer_cast<Omniscript::Primitive<std::u16string>>(value)) {
        DEBUG_LOG("Creating UTF-16 string from Primitive<std::u16string>");
        return createUTF16String(stringPrimitiveUTF16->getValue());
    
    } else if (auto stringPrimitiveUTF32 = std::dynamic_pointer_cast<Omniscript::Primitive<std::u32string>>(value)) {
        DEBUG_LOG("Creating UTF-32 string from Primitive<std::u32string>");
        return createUTF32String(stringPrimitiveUTF32->getValue());
    
    } else if (auto bigInt = std::dynamic_pointer_cast<Omniscript::BigInt>(value)) {
        DEBUG_LOG("Creating a Big int");
        return createBigInt(bigInt->getValue(), bigInt->getBitWidth());
    
    } else if (auto arry = std::dynamic_pointer_cast<Omniscript::FixedArrayExpression>(value)) {
        DEBUG_LOG("Creating a fixed array");
        std::vector<llvm::Value*> elems;

        for (const auto elem : arry->elements) {
            DEBUG_LOG("Creating an array element");
            elems.push_back(this->codegen(elem, scope));
        }
        
        llvm::Type* elementType = resolveLLVMType(arry->elementType);

        return createFixedArray(elementType, elems.size(), elems);
    }

    return nullptr;
}

llvm::Value* IRGenerator::createNullPointer(llvm::Type* pointeeType) {
    llvm::PointerType* ptrType = llvm::PointerType::getUnqual(pointeeType);
    return llvm::ConstantPointerNull::get(ptrType);
}

llvm::Value* IRGenerator::createNullValue(llvm::Type* type) {
    if (type->isPointerTy()) {
        return llvm::ConstantPointerNull::get(llvm::cast<llvm::PointerType>(type));
    } else if (type->isIntegerTy()) {
        return llvm::ConstantInt::get(type, 0);
    } else {
        return llvm::Constant::getNullValue(type);
    }
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
    // Use string literal as unique key
    std::string key = "__utf8str_" + str;

    if (activeScope->exists(key)) {
        return activeScope->get(key);
    }

    llvm::Constant* global = Builder->CreateGlobalString(str, "utf8str");
    activeScope->setConstant(key, global);

    return global;
}

// Create a 16-bit (UTF-16) string
llvm::Value* IRGenerator::createUTF16String(const std::u16string& str) {
    std::string key = "__utf16str_" + utf16_to_utf8(str);

    if (activeScope->exists(key)) {
        return activeScope->get(key);
    }

    std::vector<llvm::Constant*> chars;
    for (char16_t c : str) {
        chars.push_back(llvm::ConstantInt::get(llvm::Type::getInt16Ty(*Context), c, false));
    }
    chars.push_back(llvm::ConstantInt::get(llvm::Type::getInt16Ty(*Context), 0)); // Null terminator

    llvm::ArrayType* arrayType = llvm::ArrayType::get(llvm::Type::getInt16Ty(*Context), chars.size());
    llvm::Constant* array = llvm::ConstantArray::get(arrayType, chars);

    auto global = new llvm::GlobalVariable(
        *Module, arrayType, true, llvm::GlobalValue::PrivateLinkage, array, "utf16str");

    activeScope->setConstant(key, global);

    return global;
}

// Create a 32-bit (UTF-32) string
llvm::Value* IRGenerator::createUTF32String(const std::u32string& str) {
    std::string key = "__utf32str_" + utf32_to_utf8(str);

    if (activeScope->exists(key)) {
        return activeScope->get(key);
    }

    std::vector<llvm::Constant*> chars;
    for (char32_t c : str) {
        chars.push_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(*Context), c, false));
    }
    chars.push_back(llvm::ConstantInt::get(llvm::Type::getInt32Ty(*Context), 0)); // Null terminator

    llvm::ArrayType* arrayType = llvm::ArrayType::get(llvm::Type::getInt32Ty(*Context), chars.size());
    llvm::Constant* array = llvm::ConstantArray::get(arrayType, chars);

    auto global = new llvm::GlobalVariable(
        *Module, arrayType, true, llvm::GlobalValue::PrivateLinkage, array, "utf32str");

    activeScope->setConstant(key, global);

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

llvm::Value* IRGenerator::createFixedArray(
    llvm::Type* elementType,
    size_t size,
    const std::vector<llvm::Value*>& elements
) {

    DEBUG_LOG("Creating fixed array of size " + std::to_string(size));
    DEBUG_LOG("Element LLVM type: " + debugType(elementType));

    // Set insert point before terminator if present
    llvm::BasicBlock* currentBlock = Builder->GetInsertBlock();
    if (currentBlock && !currentBlock->empty() && currentBlock->back().isTerminator()) {
        DEBUG_LOG("Found terminator in current block, inserting before it");
        auto termIt = currentBlock->getTerminator()->getIterator();
        Builder->SetInsertPoint(currentBlock, termIt);
    }

    const llvm::DataLayout& dataLayout = currentModule != nullptr 
        ? currentModule->getDataLayout() 
        : Module->getDataLayout();

    unsigned elementAlign = dataLayout.getABITypeAlign(elementType).value();
    DEBUG_LOG("Element alignment: " + std::to_string(elementAlign));

    llvm::ArrayType* arrayType = llvm::ArrayType::get(elementType, size);
    DEBUG_LOG("Array LLVM type: " + debugType(arrayType));

    llvm::Value* arrayAlloc = Builder->CreateAlloca(arrayType, nullptr, "static_array");
    DEBUG_LOG("Array allocation created");

    llvm::cast<llvm::AllocaInst>(arrayAlloc)->setAlignment(llvm::Align(elementAlign));
    DEBUG_LOG("Set alignment of allocation");

    for (size_t i = 0; i < elements.size(); ++i) {
        DEBUG_LOG("Inserting element at index " + std::to_string(i));

        llvm::Value* element = elements[i];

        if (element->getType() != elementType) {
            DEBUG_LOG("Type mismatch: expected " + debugType(elementType) +
                      ", got " + debugType(element->getType()));
            element = generateCast(element, elementType);
            DEBUG_LOG("Casted element to match element type");
        }

        llvm::Value* elementPtr = Builder->CreateGEP(
            arrayType,
            arrayAlloc,
            {Builder->getInt32(0), Builder->getInt32(i)}
        );
        DEBUG_LOG("Computed GEP for index " + std::to_string(i));

        llvm::StoreInst* store = Builder->CreateStore(element, elementPtr);
        store->setAlignment(llvm::Align(elementAlign));
        DEBUG_LOG("Stored element at index " + std::to_string(i));
    }

    DEBUG_LOG("Finished creating fixed array");
    return arrayAlloc;
}

void IRGenerator::createFixedArrayInPlace(
    llvm::Value* destination,
    llvm::ArrayType* arrayType,
    const std::vector<llvm::Value*>& elements
) {
    llvm::Type* elementType = arrayType->getElementType();

    // DEBUG_LOG("Creating fixed array in-place at " + debugValue(destination));

    const llvm::DataLayout& dataLayout = currentModule
        ? currentModule->getDataLayout()
        : Module->getDataLayout();

    unsigned elementAlign = dataLayout.getABITypeAlign(elementType).value();

    for (size_t i = 0; i < elements.size(); ++i) {
        llvm::Value* element = elements[i];

        if (element->getType() != elementType) {
            DEBUG_LOG("Casting element at index " + std::to_string(i));
            element = generateCast(element, elementType);
        }

        llvm::Value* elementPtr = Builder->CreateGEP(arrayType, destination, {
            Builder->getInt32(0),
            Builder->getInt32(i)
        });

        llvm::StoreInst* store = Builder->CreateStore(element, elementPtr);
        store->setAlignment(llvm::Align(elementAlign));
    }

    DEBUG_LOG("Finished creating fixed array in-place");
}

