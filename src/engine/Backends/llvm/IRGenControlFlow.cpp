#include <omniscript/engine/Backends/LLVM/IRGenerator.h>

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
            console.error("Expected local variable assignment in for initializer.");
        }

        loopVarName = varAssign->variableName;
        initialValue = codegen(forExpr->initializer, localScope);
        if (!initialValue) return nullptr;
    }

    llvm::Function* function = Builder->GetInsertBlock()->getParent();
    llvm::LLVMContext& context = Builder->getContext();

    llvm::BasicBlock* preheaderBlock = Builder->GetInsertBlock();
    llvm::BasicBlock* condBlock = llvm::BasicBlock::Create(context, "for.cond", function);
    llvm::BasicBlock* bodyBlock = llvm::BasicBlock::Create(context, "for.body", function);
    llvm::BasicBlock* incrementBlock = llvm::BasicBlock::Create(context, "for.inc", function);
    llvm::BasicBlock* afterBlock = llvm::BasicBlock::Create(context, "for.end", function);

    Builder->CreateBr(condBlock);

    // === Condition block ===
    Builder->SetInsertPoint(condBlock);

    // llvm::PHINode* phi = nullptr;
    // if (!loopVarName.empty()) {
    //     phi = Builder->CreatePHI(initialValue->getType(), 2, loopVarName);
    //     phi->addIncoming(initialValue, preheaderBlock);
    //     activeScope->set(loopVarName, phi);
    // }

    llvm::Value* condValue = nullptr;
    if (forExpr->condition) {
        condValue = codegen(forExpr->condition, localScope);
        if (!condValue) return nullptr;

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
        if (!incrementValue) return nullptr;
    }

    // if (phi && incrementValue) {
    //     phi->addIncoming(incrementValue, Builder->GetInsertBlock());
    // }

    Builder->CreateBr(condBlock);

    // === After block ===
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
        // Fallback or error
        console.error("Invalid condition type in while loop");
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
        if (!condValue) return nullptr;

        if (condValue->getType()->isIntegerTy(32)) {
            condValue = Builder->CreateICmpNE(
                condValue,
                llvm::ConstantInt::get(condValue->getType(), 0),
                "ifcond"
            );
        }

        llvm::BasicBlock* thenBlock = llvm::BasicBlock::Create(context, "then", function);
        llvm::BasicBlock* elseBlock;
        
        if (i == conditions.size() - 1 && !elseBody) {
            elseBlock = mergeBlock; // add if else body here
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
            // Possibly track that this branch returned and skip PHI creation later
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
                console.error("Mismatched PHI types in if-statement");
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
