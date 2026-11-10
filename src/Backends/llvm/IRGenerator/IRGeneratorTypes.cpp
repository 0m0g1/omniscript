#include <omniscript/Backends/LLVM/IRGenerator.h>

namespace Omniscript {
llvm::Type* IRGenerator::resolveLLVMType(std::shared_ptr<Type> type) {
    // Resolve the base type kind
    llvm::Type* llvmType = nullptr;

    if (!type) {
        console.reportError(Console::TYPE_ERROR, "Cannot resolve a null type", FileSpan{});
        return nullptr;
    }

    DEBUG_LOG("Resolving a '" + type->toString() + "'.");
    llvm::LLVMContext& context = *Context;
    auto typeError = [&](const std::string& msg) {
        console.reportError(Console::TYPE_ERROR, msg, FileSpan{});
    };

    if (auto customType = std::dynamic_pointer_cast<UserDefinedType>(type)) {
        const std::string& userTypeName = customType->getName();
        auto userType = activeScope->getType(userTypeName);
        if (!userType) {
            // Fallback: the struct may be registered with LLVM (named struct) but not visible from
            // the current scope (e.g. top-level statements, method/generic scopes that don't chain
            // to the scope the struct was declared in). Try the LLVM context's named types.
            for (const char* prefix : {"", "struct.", "class."}) {
                if (auto* named = llvm::StructType::getTypeByName(context, std::string(prefix) + userTypeName)) {
                    DEBUG_LOG("Type '" + userTypeName + "' not in activeScope; resolved via LLVM named struct '" +
                              std::string(prefix) + userTypeName + "'.");
                    return named;
                }
            }
            DEBUG_LOG("Type lookup failed for '" + userTypeName + "' (activeScope=" +
                      std::to_string(reinterpret_cast<uintptr_t>(&*activeScope)) + ")");
        }
        if (!userType) {
            console.reportError(Console::TYPE_ERROR,
                "User type '" + customType->getName() + "' is not registered in the active scope",
                "Declare the type before using it in a function signature or variable", FileSpan{});
            return nullptr;
        }
        return userType;
    }

    // Todo:: Possibly call the type nullable type
    if (auto nullable = std::dynamic_pointer_cast<NullableType>(type)) {
        DEBUG_LOG("Resolving nullable type: " + nullable->toString());

        // Nullable<T> is represented as { i1, T } (i1 = hasValue flag; null is hasValue=false / zeroinit)
        if (!nullable->innerType) {
            typeError("Nullable type has no inner type");
            return nullptr;
        }
        
        llvm::Type* innerLLVMType = resolveLLVMType(nullable->innerType);
        if (!innerLLVMType) {
            typeError("Failed to resolve inner type of nullable: " + nullable->innerType->toString());
            return nullptr;
        }

        // Create struct: { i1 has_value, T value }
        return llvm::StructType::get(*Context, {
            llvm::Type::getInt1Ty(*Context),  // hasValue flag
            innerLLVMType                     // actual value
        });
    }

    if (auto funcType = std::dynamic_pointer_cast<FunctionType>(type)) {
        DEBUG_LOG("Resolving function type: " + funcType->toString());
        
        llvm::Type* returnType = resolveLLVMType(funcType->returnType);
        if (!returnType) {
            typeError("Failed to resolve function return type: " + funcType->getReturnType()->toString());
            return nullptr;
        }

        std::vector<llvm::Type*> paramTypes;
        for (const auto& param : funcType->parameterTypes) {
            llvm::Type* paramLLVMType = resolveLLVMType(param);
            if (!paramLLVMType) {
                typeError("Failed to resolve function parameter type: " + param->toString());
                return nullptr;
            }
            paramTypes.push_back(paramLLVMType);
        }

        // Create the LLVM function type with vararg info
        llvm::FunctionType* llvmFuncType = llvm::FunctionType::get(returnType, paramTypes, funcType->isVarArg);
        
        // For function pointer variables (like OpenGL function pointers), return a pointer to the function type
        // This is what you'd use for: type GLCLEAR = fn(mask: uint) => void;
        // The variable GLCLEAR would be of type "pointer to function"
        llvmType = llvmFuncType;
        // return llvm::PointerType::get(llvmFuncType, 0);
    }

    // If the type is an array, resolve the base type first.
    // Note: the element type and size live on ArrayType/FixedArrayType. The base Type's
    // elementType/fixedSize fields are not set for them (null / 0), so reading them crashed here.
    if (type->isArray()) {
        auto elementSourceType = type->getElementType();
        if (!elementSourceType) {
            typeError("Array type '" + type->toString() + "' has no element type");
            return nullptr;
        }

        if (auto fixedArray = std::dynamic_pointer_cast<FixedArrayType>(type)) {
            uint64_t arraySize = fixedArray->size;
            DEBUG_LOG("The array is of size '" + std::to_string(arraySize) + "' and holds type '" + elementSourceType->toString() + "'.");
            auto elementType = resolveLLVMType(elementSourceType);
            if (!elementType) {
                typeError("Failed to resolve array element type: " + elementSourceType->toString());
                return nullptr;
            }
            return llvm::ArrayType::get(elementType, arraySize);
        }

        // Dynamic / other arrays: represented as a pointer to the data.
        return llvm::PointerType::get(context, 0);
    }

    // If the type is a pointer: opaque pointers, so the pointee is never needed.
    if (type->isPointer()) {
        if (auto nullpointer = std::dynamic_pointer_cast<NullPointerType>(type)) {
            return llvm::PointerType::getUnqual(context);
        }
        return llvm::PointerType::get(context, 0);
    }

    // If the type is a reference, treat it as a pointer (no need to resolve the referent).
    if (type->isReference()) {
        return llvm::PointerType::get(context, 0);
    }

    switch (type->getKind()) {
        case Kind::Function:
            break;  
        case Kind::Int8:
            DEBUG_LOG("Generating LLVM type: Int8");
            llvmType = llvm::Type::getInt8Ty(context);
            break;
        case Kind::Int16:
            DEBUG_LOG("Generating LLVM type: Int16");
            llvmType = llvm::Type::getInt16Ty(context);
            break;
        case Kind::Int32:
            DEBUG_LOG("Generating LLVM type: Int32");
            llvmType = llvm::Type::getInt32Ty(context);
            break;
        case Kind::Int64:
            DEBUG_LOG("Generating LLVM type: Int64");
            llvmType = llvm::Type::getInt64Ty(context);
            break;
        case Kind::Int128:
            DEBUG_LOG("Generating LLVM type: Int128");
            llvmType = llvm::IntegerType::get(context, 128);
            break;
        case Kind::Int256:
            DEBUG_LOG("Generating LLVM type: Int256");
            llvmType = llvm::IntegerType::get(context, 256);
            break;
        case Kind::Int512:
            DEBUG_LOG("Generating LLVM type: Int512");
            llvmType = llvm::IntegerType::get(context, 512);
            break;
        case Kind::Int1024:
            DEBUG_LOG("Generating LLVM type: Int1024");
            llvmType = llvm::IntegerType::get(context, 1024);
            break;
        case Kind::BigInt:
            DEBUG_LOG("Generating LLVM type: BigInt (treated as Int1024)");
            llvmType = llvm::IntegerType::get(context, 1024);
            break;
        case Kind::Size_t: {
            int pointerWidth = getPointerBitWidth();
            DEBUG_LOG("Generating LLVM type: Size_t (treated as Int " + std::to_string(pointerWidth) + ")");
            llvmType = llvm::IntegerType::get(context, pointerWidth);
            break;
        }
        case Kind::UInt8:
            DEBUG_LOG("Generating LLVM type: UInt8");
            llvmType = llvm::Type::getInt8Ty(context);
            break;
        case Kind::UInt16:
            DEBUG_LOG("Generating LLVM type: UInt16");
            llvmType = llvm::Type::getInt16Ty(context);
            break;
        case Kind::UInt32:
            DEBUG_LOG("Generating LLVM type: UInt32");
            llvmType = llvm::Type::getInt32Ty(context);
            break;
        case Kind::UInt64:
            DEBUG_LOG("Generating LLVM type: UInt64");
            llvmType = llvm::Type::getInt64Ty(context);
            break;
        case Kind::UInt128:
            DEBUG_LOG("Generating LLVM type: UInt128");
            llvmType = llvm::IntegerType::get(context, 128);
            break;
        case Kind::UInt256:
            DEBUG_LOG("Generating LLVM type: UInt256");
            llvmType = llvm::IntegerType::get(context, 256);
            break;
        case Kind::UInt512:
            DEBUG_LOG("Generating LLVM type: UInt512");
            llvmType = llvm::IntegerType::get(context, 512);
            break;
        case Kind::UInt1024:
            DEBUG_LOG("Generating LLVM type: UInt1024");
            llvmType = llvm::IntegerType::get(context, 1024);
            break;
        case Kind::Half:
            DEBUG_LOG("Generating LLVM type: Half");
            llvmType = llvm::Type::getHalfTy(context);
            break;
        case Kind::Float:
            DEBUG_LOG("Generating LLVM type: Float");
            llvmType = llvm::Type::getFloatTy(context);
            break;
        case Kind::Double:
            DEBUG_LOG("Generating LLVM type: Double");
            llvmType = llvm::Type::getDoubleTy(context);
            break;
        case Kind::FP128:
            DEBUG_LOG("Generating LLVM type: FP128");
            llvmType = llvm::Type::getFP128Ty(context);
            break;
        case Kind::X86_FP80:
            DEBUG_LOG("Generating LLVM type: X86_FP80");
            llvmType = llvm::Type::getX86_FP80Ty(context);
            break;
        case Kind::PPC_FP128:
            DEBUG_LOG("Generating LLVM type: PPC_FP128");
            llvmType = llvm::Type::getPPC_FP128Ty(context);
            break;
        case Kind::Char:
            DEBUG_LOG("Generating LLVM type: Char (as Int8)");
            llvmType = llvm::Type::getInt8Ty(context);
            break;
        case Kind::Char16:
            DEBUG_LOG("Generating LLVM type: Char16 (as Int16)");
            llvmType = llvm::Type::getInt16Ty(context);
            break;
        case Kind::Char32:
            DEBUG_LOG("Generating LLVM type: Char32 (as Int32)");
            llvmType = llvm::Type::getInt32Ty(context);
            break;
        case Kind::Bool:
            DEBUG_LOG("Generating LLVM type: Bool (as Int1)");
            llvmType = llvm::Type::getInt1Ty(context);
            break;
        case Kind::Void:
            DEBUG_LOG("Generating LLVM type: Void");
            llvmType = llvm::Type::getVoidTy(context);
            break;
        case Kind::Null:
            if (auto nullType = std::dynamic_pointer_cast<NullType>(type)) {
                DEBUG_LOG("Generating LLVM type: Null expects " + (nullType->innerType ? nullType->innerType->toString() : "void"));
                if (nullType->innerType) {
                    llvmType = llvm::PointerType::getUnqual(context);
                } else {
                    llvmType = llvm::PointerType::getUnqual(context);
                }
            } else {
                console.reportError(Console::INTERNAL_ERROR, "Type with kind null is not a valid null type", FileSpan{});
            }
            break;
        case Kind::String:
            DEBUG_LOG("Generating LLVM type: String (ptr)");
            llvmType = llvm::PointerType::get(context, 0);
            break;
        case Kind::Utf8:
            DEBUG_LOG("Generating LLVM type: Utf8 (i8*)");
            llvmType = llvm::PointerType::get(context, 0);
            break;
        case Kind::Utf16:
            DEBUG_LOG("Generating LLVM type: Utf16 (i16*)");
            llvmType = llvm::PointerType::get(context, 0);
            break;
        case Kind::Utf32:
            DEBUG_LOG("Generating LLVM type: Utf32 (i32*)");
            llvmType = llvm::PointerType::get(context, 0);
            break;
        default:
            typeError("Unknown type: " + type->toString());
            return nullptr;
    }
    

    return llvmType;
}

} // namespace Omniscript