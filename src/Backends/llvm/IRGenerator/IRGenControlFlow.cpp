#include <omniscript/Backends/LLVM/IRGenerator.h>
#include <omniscript/Expressions/BlockExpression.h>

namespace Omniscript {
llvm::Value* IRGenerator::createReturn(llvm::Value* returnValue, llvm::Type* expectedReturnType) {
    llvm::Function* currentFunction = Builder->GetInsertBlock()->getParent();
    if (!currentFunction) {
        std::string suggestion = "To resolve this:\n"
                               "1. Ensure return statement is within a function body\n"
                               "2. Check for correct function declaration\n"
                               "3. Verify code structure";
        console.reportError(
            Omniscript::Console::RUNTIME_ERROR,
            "Return statement outside function",
            suggestion,
            FileSpan() // Default span as no statement context
        );
        return nullptr;
    }

    if (!Module) {
        std::string suggestion = "To resolve this:\n"
                               "1. Verify LLVM module is properly initialized\n"
                               "2. Check module creation in IRGenerator setup\n"
                               "3. Ensure module is not null before codegen";
        console.reportError(
            Omniscript::Console::RUNTIME_ERROR,
            "LLVM Module is null",
            suggestion,
            FileSpan() // Default span as no statement context
        );
        return nullptr;
    }
    
    if (activeScope->get("va_list")) {
        llvm::Function* vaEndFunc = llvm::Intrinsic::getOrInsertDeclaration(Module.get(), llvm::Intrinsic::vaend);
        llvm::Value* vaListAlloca = activeScope->get("va_list");
        Builder->CreateCall(vaEndFunc, { vaListAlloca });
    }

    if (currentFunction->getReturnType()->isVoidTy()) {
        if (returnValue) {
            std::string suggestion = Omniscript::Console::formatString(
                "To resolve this:\n"
                "1. Remove return value for void function '%s'\n"
                "2. Use return without a value\n"
                "3. Verify function signature",
                currentFunction->getName().str().c_str()
            );
            console.reportError(
                Omniscript::Console::TYPE_ERROR,
                "Void function cannot return a value",
                suggestion,
                FileSpan() // Default span as no statement context
            );
        }
        return Builder->CreateRetVoid();
    }

    if (!returnValue) {
        std::string suggestion = Omniscript::Console::formatString(
            "To resolve this:\n"
            "1. Provide a return value for non-void function '%s'\n"
            "2. Check function return type\n"
            "3. Ensure return expression is valid",
            currentFunction->getName().str().c_str()
        );
        console.reportError(
            Omniscript::Console::TYPE_ERROR,
            "Non-void function must return a value",
            suggestion,
            FileSpan() // Default span as no statement context
        );
        return nullptr;
    }

    if (returnValue->getType() != expectedReturnType) {
        std::string suggestion = Omniscript::Console::formatString(
            "To resolve this:\n"
            "1. Ensure return value type matches expected type '%s'\n"
            "2. Check expression type for '%s'\n"
            "3. Consider explicit type casting",
            debugType(expectedReturnType).c_str(), debugType(returnValue->getType()).c_str()
        );
        console.reportError(
            Omniscript::Console::TYPE_ERROR,
            Omniscript::Console::formatString("Return type mismatch: expected '%s', got '%s'",
                             debugType(expectedReturnType).c_str(), debugType(returnValue->getType()).c_str()),
            suggestion,
            FileSpan() // Default span as no statement context
        );
        return nullptr;
    }

    return Builder->CreateRet(returnValue);
}

llvm::Value* IRGenerator::createForLoop(
    const std::shared_ptr<Omniscript::ForLoopExpression>& forExpr,
    std::shared_ptr<SymbolTable<std::shared_ptr<Omniscript::Expression>, std::shared_ptr<Omniscript::Type>>> scope
) {
    pushScope();
    auto localScope = scope->createChildScope("forloop");

    std::string loopVarName;
    llvm::Value* initialValue = nullptr;

    if (forExpr->initializer) {
        auto varAssign = std::dynamic_pointer_cast<Omniscript::VariableAssignment>(forExpr->initializer);
        if (!varAssign || varAssign->isGlobal) {
            std::string suggestion = "To resolve this:\n"
                                   "1. Ensure for loop initializer is a local variable assignment\n"
                                   "2. Check initializer syntax\n"
                                   "3. Verify variable is not declared as global";
            console.reportError(
                Omniscript::Console::RUNTIME_ERROR,
                "Expected local variable assignment in for initializer",
                suggestion,
                forExpr->getSpan()
            );
            popScope();
            return nullptr;
        }

        loopVarName = varAssign->variableName;
        initialValue = codegen(forExpr->initializer, localScope);
        if (!initialValue) {
            std::string suggestion = Omniscript::Console::formatString(
                "To resolve this:\n"
                "1. Verify initializer for variable '%s' is valid\n"
                "2. Check initializer expression\n"
                "3. Add debug output for initializer codegen",
                loopVarName.c_str()
            );
            console.reportError(
                Omniscript::Console::RUNTIME_ERROR,
                Omniscript::Console::formatString("Failed to generate initializer for variable '%s'",
                                 loopVarName.c_str()),
                suggestion,
                forExpr->initializer->getSpan()
            );
            popScope();
            return nullptr;
        }
    }

    llvm::Function* function = Builder->GetInsertBlock()->getParent();
    llvm::LLVMContext& context = Builder->getContext();

    llvm::BasicBlock* preheaderBlock = Builder->GetInsertBlock();
    llvm::BasicBlock* condBlock = llvm::BasicBlock::Create(context, "for.cond", function);
    llvm::BasicBlock* bodyBlock = llvm::BasicBlock::Create(context, "for.body", function);
    llvm::BasicBlock* incrementBlock = llvm::BasicBlock::Create(context, "for.inc", function);
    llvm::BasicBlock* afterBlock = llvm::BasicBlock::Create(context, "for.end");

    Builder->CreateBr(condBlock);

    // === Condition block ===
    Builder->SetInsertPoint(condBlock);

    llvm::Value* condValue = nullptr;
    if (forExpr->condition) {
        condValue = codegen(forExpr->condition, localScope);
        if (!condValue) {
            std::string suggestion = "To resolve this:\n"
                                   "1. Verify for loop condition is valid\n"
                                   "2. Check condition expression syntax\n"
                                   "3. Add debug output for condition codegen";
            console.reportError(
                Omniscript::Console::RUNTIME_ERROR,
                "Failed to generate condition for for loop",
                suggestion,
                forExpr->condition->getSpan()
            );
            popScope();
            return nullptr;
        }

        if (condValue->getType()->isIntegerTy()) {
            condValue = Builder->CreateICmpNE(
                condValue,
                llvm::ConstantInt::get(condValue->getType(), 0),
                "forcond"
            );
        } else if (condValue->getType()->isFloatingPointTy()) {
            condValue = Builder->CreateFCmpONE(
                condValue,
                llvm::ConstantFP::get(condValue->getType(), 0.0),
                "forcond"
            );
        }
    } else {
        condValue = llvm::ConstantInt::getTrue(context);
    }

    Builder->CreateCondBr(condValue, bodyBlock, afterBlock);

    // === Body block ===
    Builder->SetInsertPoint(bodyBlock);
    auto block = std::dynamic_pointer_cast<Omniscript::BlockExpression>(forExpr->body);
    if (block) {
        for (auto& expr : block->values) {
            if (expr) {
                codegen(expr, localScope);
            }
        }
    }

    if (!Builder->GetInsertBlock()->getTerminator()) {
        Builder->CreateBr(incrementBlock);
    }

    // === Increment block ===
    Builder->SetInsertPoint(incrementBlock);

    llvm::Value* incrementValue = nullptr;
    if (forExpr->increment) {
        incrementValue = codegen(forExpr->increment, localScope);
        if (!incrementValue) {
            std::string suggestion = "To resolve this:\n"
                                   "1. Verify for loop increment is valid\n"
                                   "2. Check increment expression syntax\n"
                                   "3. Add debug output for increment codegen";
            console.reportError(
                Omniscript::Console::RUNTIME_ERROR,
                "Failed to generate increment for for loop",
                suggestion,
                forExpr->increment->getSpan()
            );
            popScope();
            return nullptr;
        }
    }

    Builder->CreateBr(condBlock);

    // === After block ===
    afterBlock->insertInto(function);
    Builder->SetInsertPoint(afterBlock);

    popScope();

    return nullptr;
}

llvm::Value* IRGenerator::createWhileLoop(
    const std::shared_ptr<Omniscript::WhileLoopExpression>& whileExpr,
    std::shared_ptr<SymbolTable<std::shared_ptr<Omniscript::Expression>, std::shared_ptr<Omniscript::Type>>> scope
) {
    llvm::Function* function = Builder->GetInsertBlock()->getParent();
    llvm::LLVMContext& context = Builder->getContext();

    auto localScope = scope->createChildScope("whileloop");

    llvm::BasicBlock* condBlock = llvm::BasicBlock::Create(context, "while.cond", function);
    llvm::BasicBlock* bodyBlock = llvm::BasicBlock::Create(context, "while.body");
    llvm::BasicBlock* afterBlock = llvm::BasicBlock::Create(context, "while.end");

    Builder->CreateBr(condBlock);
    Builder->SetInsertPoint(condBlock);

    llvm::Value* condValue = whileExpr->condition ? codegen(whileExpr->condition, localScope) : nullptr;

    if (!condValue) {
        condValue = llvm::ConstantInt::getTrue(context);
    } else if (condValue->getType()->isIntegerTy()) {
        condValue = Builder->CreateICmpNE(
            condValue,
            llvm::ConstantInt::get(condValue->getType(), 0),
            "whilecond"
        );
    } else if (condValue->getType()->isFloatingPointTy()) {
        condValue = Builder->CreateFCmpONE(
            condValue,
            llvm::ConstantFP::get(condValue->getType(), 0.0),
            "whilecond"
        );
    } else {
        std::string suggestion = "To resolve this:\n"
                               "1. Ensure while loop condition has a valid type (integer or floating-point)\n"
                               "2. Check condition expression\n"
                               "3. Verify condition type compatibility";
        console.reportError(
            Omniscript::Console::TYPE_ERROR,
            "Invalid condition type in while loop",
            suggestion,
            whileExpr->condition ? whileExpr->condition->getSpan() : FileSpan()
        );
        return nullptr;
    }

    Builder->CreateCondBr(condValue, bodyBlock, afterBlock);

    // Attach bodyBlock now
    bodyBlock->insertInto(function);
    Builder->SetInsertPoint(bodyBlock);

    bool bodyHasContent = false;
    if (auto block = std::dynamic_pointer_cast<Omniscript::BlockExpression>(whileExpr->body)) {
        if (!block->values.empty()) {
            bodyHasContent = true;
            codegen(whileExpr->body, localScope);
        }
    }

    // If the loop body doesn't end in a terminator (return, break, etc.), jump to condition again
    if (!Builder->GetInsertBlock()->getTerminator()) {
        Builder->CreateBr(condBlock);
    }

    // Attach afterBlock
    afterBlock->insertInto(function);
    Builder->SetInsertPoint(afterBlock);

    return nullptr;
}

llvm::Value* IRGenerator::createIfStatement( 
    const std::vector<std::shared_ptr<Omniscript::Expression>>& conditions,
    const std::vector<std::shared_ptr<Omniscript::Expression>>& bodies,
    const std::shared_ptr<Omniscript::Expression>& elseBody,
    std::shared_ptr<SymbolTable<std::shared_ptr<Omniscript::Expression>, std::shared_ptr<Omniscript::Type>>> scope)
{
    if (conditions.empty() || conditions.size() != bodies.size()) {
        std::string suggestion = "To resolve this:\n"
                               "1. Ensure if statement has at least one condition and matching body\n"
                               "2. Verify conditions and bodies vectors have equal size\n"
                               "3. Check if statement syntax";
        console.reportError(
            Omniscript::Console::RUNTIME_ERROR,
            Omniscript::Console::formatString("Invalid if statement: %d conditions, %d bodies",
                             conditions.size(), bodies.size()),
            suggestion,
            conditions.empty() ? FileSpan() : conditions[0]->getSpan()
        );
        return nullptr;
    }

    llvm::Function* function = Builder->GetInsertBlock()->getParent();
    llvm::LLVMContext& context = Builder->getContext();

    auto localScope = scope->createChildScope("ifscope");

    llvm::BasicBlock* mergeBlock = llvm::BasicBlock::Create(context, "ifcont");
    bool mergeBlockUsed = false;

    std::vector<llvm::BasicBlock*> incomingBlocks;
    std::vector<llvm::Value*> incomingValues;

    llvm::BasicBlock* nextBlock = nullptr;

    // Handle if/else-if chain
    for (size_t i = 0; i < conditions.size(); ++i) {
        llvm::BasicBlock* condBlock = Builder->GetInsertBlock();
        llvm::Value* condValue = codegen(conditions[i], localScope);
        if (!condValue) {
            std::string suggestion = Omniscript::Console::formatString(
                "To resolve this:\n"
                "1. Verify condition #%d is valid\n"
                "2. Check condition expression syntax\n"
                "3. Add debug output for condition codegen",
                i + 1
            );
            console.reportError(
                Omniscript::Console::RUNTIME_ERROR,
                Omniscript::Console::formatString("Failed to generate code for condition #%d", i + 1),
                suggestion,
                conditions[i]->getSpan()
            );
            return nullptr;
        }

        if (condValue->getType()->isIntegerTy()) {
            unsigned bitWidth = condValue->getType()->getIntegerBitWidth();
            condValue = Builder->CreateICmpNE(
                condValue,
                llvm::ConstantInt::get(condValue->getType(), 0, true),
                "ifcond"
            );
        } else if (!condValue->getType()->isIntegerTy(1)) {
            condValue = Builder->CreateIsNotNull(condValue, "ifcond");
        }

        llvm::BasicBlock* thenBlock = llvm::BasicBlock::Create(context, "then", function);
        llvm::BasicBlock* elseBlock;
        
        if (i == conditions.size() - 1 && !elseBody) {
            elseBlock = mergeBlock;
            mergeBlockUsed = true;
        } else {
            elseBlock = llvm::BasicBlock::Create(context, "else");
        }

        Builder->CreateCondBr(condValue, thenBlock, elseBlock);

        // Emit then block
        Builder->SetInsertPoint(thenBlock);
        llvm::Value* thenValue = codegen(bodies[i], localScope);
        DEBUG_LOG(bodies[i]->toString());

        bool hasTerminator = Builder->GetInsertBlock()->getTerminator();
        if (!hasTerminator) {
            Builder->CreateBr(mergeBlock);
            mergeBlockUsed = true;
            incomingBlocks.push_back(Builder->GetInsertBlock());
            if (thenValue) incomingValues.push_back(thenValue);
        } else if (thenValue) {
            // Do not push the block because it won’t reach merge
        }

        if (i == conditions.size() - 1 && !elseBody) {
            // No else clause, make sure we emit a branch to merge
            if (!Builder->GetInsertBlock()->getTerminator()) {
                Builder->CreateBr(mergeBlock);
                mergeBlockUsed = true;
                incomingBlocks.push_back(Builder->GetInsertBlock());
            }
        }

        if (elseBody) {
            if (elseBlock != mergeBlock) {
                nextBlock = elseBlock;
                elseBlock->insertInto(function);
                Builder->SetInsertPoint(elseBlock);
            }
        }
    }

    // Handle final else block if provided
    if (elseBody) {
        llvm::Value* elseValue = codegen(elseBody, localScope);
        if (!elseValue && !std::dynamic_pointer_cast<Omniscript::BlockExpression>(elseBody)) {
            std::string suggestion = "To resolve this:\n"
                                   "1. Verify else block expression is valid\n"
                                   "2. Check else body syntax\n"
                                   "3. Add debug output for else block codegen";
            console.reportError(
                Omniscript::Console::RUNTIME_ERROR,
                "Failed to generate code for else block",
                suggestion,
                elseBody->getSpan()
            );
            return nullptr;
        }

        // Set insert point to current block *before* emitting the branch
        llvm::BasicBlock* currentBlock = Builder->GetInsertBlock();
        if (!currentBlock->getTerminator()) {
            Builder->CreateBr(mergeBlock);
            mergeBlockUsed = true;
        }

        incomingBlocks.push_back(currentBlock);
        if (elseValue) {
            incomingValues.push_back(elseValue);
        }
    } else if (nextBlock && Builder->GetInsertBlock() == nextBlock) {
        // If else not provided, and last else block is still current block
        if (!Builder->GetInsertBlock()->getTerminator()) {
            Builder->CreateBr(mergeBlock);
            mergeBlockUsed = true;
        }
    }

    if (mergeBlockUsed) {
        mergeBlock->insertInto(function);
        Builder->SetInsertPoint(mergeBlock);
    }

    // Emit merge block if used
    if (!incomingValues.empty()) {
        if (incomingValues[0]->getType()->isVoidTy()) {
            return nullptr;
        }

        for (auto val : incomingValues) {
            if (val->getType() != incomingValues[0]->getType()) {
                std::string suggestion = "To resolve this:\n"
                                       "1. Ensure all branches return compatible types\n"
                                       "2. Check return expressions in each branch\n"
                                       "3. Verify type consistency across if/else blocks";
                console.reportError(
                    Omniscript::Console::TYPE_ERROR,
                    "Mismatched PHI types in if-statement",
                    suggestion,
                    conditions[0]->getSpan()
                );
                return nullptr;
            }
        }

        llvm::PHINode* phi = Builder->CreatePHI(incomingValues[0]->getType(), incomingValues.size(), "iftmp");
        for (size_t i = 0; i < incomingValues.size(); ++i) {
            phi->addIncoming(incomingValues[i], incomingBlocks[i]);
        }
        
        return phi;
    }

    return nullptr;
}

}