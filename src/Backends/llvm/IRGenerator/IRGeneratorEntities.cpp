#include <omniscript/Backends/llvm/IRGenerator.h>

llvm::Value* IRGenerator::createEnum(
    const std::vector<std::string>& names,
    const std::vector<llvm::Value*>& values,
    const std::string& enumName,
    bool isGlobal
) {
    // for (size_t i = 0; i < names.size(); ++i) {
    //     assignVariable(enumName + "." + names[i], values[i]->getType(), values[i], isGlobal, true);
    // }
    return nullptr;
}


llvm::Value* IRGenerator::createEnumWithLookup(
    const std::vector<std::string>& names,
    const std::vector<llvm::Value*>& values,
    const std::string& enumName,
    bool isGlobal
) {
    // llvm::Type* valueType = values[0]->getType();
    // std::vector<llvm::Constant*> constValues;
    // std::vector<llvm::Constant*> nameConstants;

    // for (size_t i = 0; i < values.size(); ++i) {
    //     // Declare each enum value as a global/local variable
    //     assignVariable(enumName + "." + names[i], valueType, values[i], isGlobal, true);

    //     // Handle the constant value for the value array
    //     if (auto* constantVal = llvm::dyn_cast<llvm::Constant>(values[i])) {
    //         constValues.push_back(constantVal);
    //     } else {
    //         llvm::errs() << "Warning: Non-constant enum value for " << names[i] << "\n";
    //         constValues.push_back(llvm::Constant::getNullValue(valueType));
    //     }

    //     // Create a global string pointer for the name
    //     llvm::Constant* namePtr = Builder->CreateGlobalString(names[i], enumName + "_str_" + names[i]);
    //     nameConstants.push_back(namePtr);
    // }

    // // Create value lookup array
    // llvm::ArrayType* valueArrayType = llvm::ArrayType::get(valueType, constValues.size());
    // llvm::Constant* valueArray = llvm::ConstantArray::get(valueArrayType, constValues);
    // assignVariable(enumName + "_lookup", valueArrayType, valueArray, isGlobal, true);

    // // Create name (string) lookup array
    // llvm::Type* stringPtrType = nameConstants[0]->getType(); // i8*
    // llvm::ArrayType* nameArrayType = llvm::ArrayType::get(stringPtrType, nameConstants.size());
    // llvm::Constant* nameArray = llvm::ConstantArray::get(nameArrayType, nameConstants);
    // assignVariable(enumName + "_name_lookup", nameArrayType, nameArray, isGlobal, true);

    // return valueArray; // or nullptr if you don't need to return a value
    return nullptr;
}

llvm::Value* IRGenerator::createEnumClass(
    const std::vector<std::string>& names,
    const std::vector<llvm::Value*>& values,
    const std::string& className,
    bool isGlobal
) {
    // llvm::LLVMContext& ctx = Builder->getContext();
    // llvm::Type* fieldType = values[0]->getType();

    // std::vector<llvm::Type*> fieldTypes(values.size(), fieldType);
    // std::vector<llvm::Constant*> fieldValues;

    // for (auto* val : values) {
    //     if (auto* c = llvm::dyn_cast<llvm::Constant>(val)) {
    //         fieldValues.push_back(c);
    //     } else {
    //         llvm::errs() << "Warning: Non-constant value in enum class " << className << "\n";
    //         fieldValues.push_back(llvm::Constant::getNullValue(fieldType));
    //     }
    // }

    // llvm::StructType* structType = llvm::StructType::create(ctx, fieldTypes, className);
    // llvm::Constant* structConst = llvm::ConstantStruct::get(structType, fieldValues);

    // return assignVariable(className, structType, structConst, isGlobal, true);
    return nullptr;
}

llvm::Value* IRGenerator::createEnumClassWithLookup(
    const std::vector<std::string>& names,
    const std::vector<llvm::Value*>& values,
    const std::string& className,
    bool isGlobal
) {
    // llvm::LLVMContext& ctx = Builder->getContext();
    // llvm::Type* valueType = values[0]->getType();

    // std::vector<llvm::Type*> fieldTypes(values.size(), valueType);
    // std::vector<llvm::Constant*> fieldValues;
    // std::vector<llvm::Constant*> nameConstants;

    // // Step 1: Generate the enum struct values and name strings
    // for (size_t i = 0; i < values.size(); ++i) {
    //     llvm::Value* val = values[i];

    //     if (auto* c = llvm::dyn_cast<llvm::Constant>(val)) {
    //         fieldValues.push_back(c);
    //     } else {
    //         llvm::errs() << "Warning: Non-constant enum value in class " << className << "\n";
    //         fieldValues.push_back(llvm::Constant::getNullValue(valueType));
    //     }

    //     // Create a global string pointer for the name
    //     llvm::Constant* namePtr = Builder->CreateGlobalString(names[i], className + "_str_" + names[i]);
    //     nameConstants.push_back(namePtr);
    // }

    // // Step 2: Create the struct for the enum class
    // llvm::StructType* structType = llvm::StructType::create(ctx, fieldTypes, className);
    // llvm::Constant* structConst = llvm::ConstantStruct::get(structType, fieldValues);
    // llvm::Value* enumClass = assignVariable(className, structType, structConst, isGlobal, true);

    // // Step 3: Create value lookup array (flat version of struct)
    // llvm::ArrayType* lookupArrayType = llvm::ArrayType::get(valueType, fieldValues.size());
    // llvm::Constant* lookupArray = llvm::ConstantArray::get(lookupArrayType, fieldValues);
    // assignVariable(className + "_lookup", lookupArrayType, lookupArray, isGlobal, true);

    // // Step 4: Create name lookup array
    // llvm::Type* stringPtrType = nameConstants[0]->getType(); // i8*
    // llvm::ArrayType* nameArrayType = llvm::ArrayType::get(stringPtrType, nameConstants.size());
    // llvm::Constant* nameArray = llvm::ConstantArray::get(nameArrayType, nameConstants);
    // assignVariable(className + "_name_lookup", nameArrayType, nameArray, isGlobal, true);

    // return enumClass;
    return nullptr;
}

llvm::Value* IRGenerator::getEnumValue(const std::string& enumName, const std::string& memberName) {
    // Construct lookup variable name
    // std::string lookupTableName = enumName + "_lookup";

    // // Retrieve the enum lookup table from the symbol table
    // llvm::GlobalVariable* lookupTable = dynamic_cast<llvm::GlobalVariable*>(activeScope->get(lookupTableName));
    // if (!lookupTable) {
    //     console.error("Enum '" + enumName + "' not found.");
    //     return nullptr;
    // }

    // // Get the array type and its length
    // llvm::ArrayType* arrayType = llvm::dyn_cast<llvm::ArrayType>(lookupTable->getValueType());
    // if (!arrayType) {
    //     console.error("Invalid enum lookup table format for '" + enumName + "'.");
    //     return nullptr;
    // }

    // size_t numEntries = arrayType->getNumElements();
    // llvm::Type* structType = arrayType->getElementType(); // Struct { i32, i8* }

    // // Iterate through the lookup table to find the matching member
    // for (size_t i = 0; i < numEntries; ++i) {
    //     // Get pointer to entry i
    //     llvm::Value* entryPtr = Builder->CreateConstGEP2_32(arrayType, lookupTable, 0, i);

    //     // Extract the member name (char*)
    //     llvm::Value* namePtr = Builder->CreateStructGEP(structType, entryPtr, 1);
    //     llvm::Value* nameValue = Builder->CreateLoad(namePtr->getType()->getPointerElementType(), namePtr);

    //     // Compare with the requested memberName
    //     llvm::Value* cmp = Builder->CreateCall(getStrcmpFunction(), {nameValue, Builder->CreateGlobalStringPtr(memberName)});
    //     llvm::Value* isMatch = Builder->CreateICmpEQ(cmp, llvm::ConstantInt::get(llvm::Type::getInt32Ty(*Context), 0));

    //     // If matched, return the integer value
    //     llvm::Value* intPtr = Builder->CreateStructGEP(structType, entryPtr, 0);
    //     llvm::Value* intValue = Builder->CreateLoad(intPtr->getType()->getPointerElementType(), intPtr);

    //     llvm::BasicBlock* returnBlock = llvm::BasicBlock::Create(*Context, "return_enum", Builder->GetInsertBlock()->getParent());
    //     llvm::BasicBlock* continueBlock = llvm::BasicBlock::Create(*Context, "continue_enum", Builder->GetInsertBlock()->getParent());

    //     Builder->CreateCondBr(isMatch, returnBlock, continueBlock);

    //     // Set insert point for return
    //     Builder->SetInsertPoint(returnBlock);
    //     Builder->CreateRet(intValue);

    //     // Set insert point for continue
    //     Builder->SetInsertPoint(continueBlock);
    // }

    // // If no match found, return an error value (-1)
    // return Builder->CreateRet(llvm::ConstantInt::get(llvm::Type::getInt32Ty(*Context), -1));
    return nullptr;
}
