#include <omniscript/Backends/LLVM/IRGenerator.h>

namespace Omniscript {
llvm::Type* IRGenerator::resolveLLVMType(std::shared_ptr<Type> type) {
    // Resolve the base type kind
    llvm::Type* llvmType = nullptr;

    if (!type) {
        std::cerr << "[ERROR] Type is null!" << std::endl;
        return nullptr;
    }

    DEBUG_LOG("Resolving a '" + type->toString() + "'.");
    llvm::LLVMContext& context = *Context;

    if (auto customType = std::dynamic_pointer_cast<UserDefinedType>(type)) {
        auto userType = activeScope->getType(customType->getName());
        if (!userType) {
            console.error("User type is null");
            return nullptr;
        }
        return userType;
    }

    // Todo:: Possibly call the type nullable type
    if (auto nullable = std::dynamic_pointer_cast<NullableType>(type)) {
        DEBUG_LOG("Resolving nullable type: " + nullable->toString());

        // Nullable<T> is represented as { i1, T } (i1 = isNull flag)
        if (!nullable->innerType) {
            console.error("[ERROR] Nullable has no inner type ");
        }
        
        llvm::Type* innerLLVMType = resolveLLVMType(nullable->innerType);
        if (!innerLLVMType) {
            console.error("[ERROR] Failed to resolve inner type of nullable: " + nullable->innerType->toString());
            return nullptr;
        }

        // Create struct: { i1 is_null, T value }
        return llvm::StructType::get(*Context, {
            llvm::Type::getInt1Ty(*Context),  // null flag
            innerLLVMType                     // actual value
        });
    }

    if (auto funcType = std::dynamic_pointer_cast<FunctionType>(type)) {
        DEBUG_LOG("Resolving function type: " + funcType->toString());
        
        llvm::Type* returnType = resolveLLVMType(funcType->returnType);
        if (!returnType) {
            console.error("[ERROR] Failed to resolve function return type: " + funcType->getReturnType()->toString());
            return nullptr;
        }

        std::vector<llvm::Type*> paramTypes;
        for (const auto& param : funcType->parameterTypes) {
            llvm::Type* paramLLVMType = resolveLLVMType(param);
            if (!paramLLVMType) {
                console.error("[ERROR] Failed to resolve function parameter type: " + param->toString());
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
    if (type->isArray()) {
        DEBUG_LOG("The array is of size '" + std::to_string(type->fixedSize) + "' and holds type " + type->elementType->toString() + "'.");
        auto elementType = resolveLLVMType(type->elementType);
        uint64_t arraySize = type->fixedSize;
        return llvm::ArrayType::get(elementType, arraySize);
    }

    // If the type is a pointer, resolve the base type and add pointer depth.
    if (type->isPointer()) {
        if (auto pointer = std::dynamic_pointer_cast<PointerType>(type)) {
            int pointerDepth;
            llvm::Type* pointeeType;
            pointeeType = resolveLLVMType(pointer->getPointeeType());
            pointerDepth = pointer->getPointerDepth();
            return llvm::PointerType::get(pointeeType, 0);
        }

        if (auto nullpointer = std::dynamic_pointer_cast<NullPointerType>(type)) {
            return llvm::PointerType::getUnqual(resolveLLVMType(type->getPointeeType()));
        }
        
    }

    // If the type is a reference, treat it as a pointer.
    if (type->isReference()) {
        auto referencedType = resolveLLVMType(type->getReferencedType());
        return llvm::PointerType::get(referencedType, 0);
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
                    llvmType = llvm::PointerType::getUnqual(resolveLLVMType(nullType->innerType));
                } else {
                    llvmType = llvm::PointerType::getUnqual(llvm::Type::getInt8Ty(context));
                }
            } else {
                console.error("Type with kind null is not a valid null type.");
            }
            break;
        case Kind::Utf8:
            DEBUG_LOG("Generating LLVM type: Utf8 (i8*)");
            llvmType = llvm::PointerType::get(llvm::Type::getInt8Ty(context), 0);
            break;
        case Kind::Utf16:
            DEBUG_LOG("Generating LLVM type: Utf16 (i16*)");
            llvmType = llvm::PointerType::get(llvm::Type::getInt16Ty(context), 0);
            break;
        case Kind::Utf32:
            DEBUG_LOG("Generating LLVM type: Utf32 (i32*)");
            llvmType = llvm::PointerType::get(llvm::Type::getInt32Ty(context), 0);
            break;
        default:
            console.error("[ERROR] Unknown type: " + type->toString());
            return nullptr;
    }
    

    return llvmType;
}

} // namespace Omniscript
