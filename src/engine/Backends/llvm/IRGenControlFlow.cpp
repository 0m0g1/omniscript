#include <omniscript/engine/Backends/LLVM/IRGenerator.h>

llvm::Value* IRGenerator::createForLoop(
    const std::shared_ptr<Omniscript::ForLoopExpression>& forExpr,
    std::shared_ptr<SymbolTable<std::shared_ptr<Omniscript::Expression>, std::shared_ptr<Omniscript::Type>>> scope
) {
    llvm::Function* function = Builder->GetInsertBlock()->getParent();
    llvm::LLVMContext& context = Builder->getContext();

    // Create a local scope for loop
    pushScope();
    auto localScope = scope->createChildScope("forloop");

    // Emit initializer if present
    if (forExpr->initializer) {
        if (!codegen(forExpr->initializer, localScope)) return nullptr;
    }

    // Create basic blocks
    llvm::BasicBlock* condBlock = llvm::BasicBlock::Create(context, "for.cond", function);
    llvm::BasicBlock* bodyBlock = llvm::BasicBlock::Create(context, "for.body", function);
    llvm::BasicBlock* incrementBlock = llvm::BasicBlock::Create(context, "for.inc", function);
    llvm::BasicBlock* afterBlock = llvm::BasicBlock::Create(context, "for.end", function);

    // Jump to condition
    Builder->CreateBr(condBlock);
    Builder->SetInsertPoint(condBlock);

    // Emit condition (if not null)
    llvm::Value* condValue = nullptr;
    if (forExpr->condition) {
        condValue = codegen(forExpr->condition, localScope);
        if (!condValue) return nullptr;

        // Normalize to boolean
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
        // No condition → always true
        condValue = llvm::ConstantInt::getTrue(context);
    }

    Builder->CreateCondBr(condValue, bodyBlock, afterBlock);

    // Emit body
    Builder->SetInsertPoint(bodyBlock);
    bool bodyHasContent = false;
    auto block = std::dynamic_pointer_cast<Omniscript::BlockExpression>(forExpr->body);
    if (block) {
        for (auto& expr : block->values) {
            if (expr) {
                bodyHasContent = true;
                if (!codegen(expr, localScope)) return nullptr;
            }
        }
    }

    // If body has no terminator, jump to increment
    if (!bodyHasContent || !Builder->GetInsertBlock()->getTerminator()) {
        Builder->CreateBr(incrementBlock);
    }

    // Emit increment if present
    Builder->SetInsertPoint(incrementBlock);
    if (forExpr->increment) {
        if (!codegen(forExpr->increment, localScope)) return nullptr;
    }
    Builder->CreateBr(condBlock);

    // Final block
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
    llvm::BasicBlock* bodyBlock = llvm::BasicBlock::Create(context, "while.body", function);
    llvm::BasicBlock* afterBlock = llvm::BasicBlock::Create(context, "while.end", function);

    Builder->CreateBr(condBlock); // Jump to condition
    Builder->SetInsertPoint(condBlock);

    llvm::Value* condValue = whileExpr->condition ? codegen(whileExpr->condition, localScope) : nullptr;

    if (!condValue) {
        // Treat null condition as infinite loop
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
    }

    Builder->CreateCondBr(condValue, bodyBlock, afterBlock);

    // Loop body
    Builder->SetInsertPoint(bodyBlock);
    bool bodyHasContent = false;

    if (auto block = std::dynamic_pointer_cast<Omniscript::BlockExpression>(whileExpr->body)) {
        for (auto& expr : block->values) {
            if (expr) {
                bodyHasContent = true;
                if (!codegen(expr, localScope)) return nullptr;
            }
        }
    } else if (whileExpr->body) {
        bodyHasContent = true;
        if (!codegen(whileExpr->body, localScope)) return nullptr;
    }

    if (!bodyHasContent || !Builder->GetInsertBlock()->getTerminator()) {
        Builder->CreateBr(condBlock);
    }

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
        llvm::BasicBlock* elseBlock = llvm::BasicBlock::Create(context, "else");

        Builder->CreateCondBr(condValue, thenBlock, elseBlock);

        // Emit then block
        Builder->SetInsertPoint(thenBlock);
        llvm::Value* thenValue = codegen(bodies[i], localScope);
        DEBUG_LOG(bodies[i]->toString());

        bool hasTerminator = Builder->GetInsertBlock()->getTerminator();
        if (!hasTerminator) {
            Builder->CreateBr(mergeBlock);
            incomingBlocks.push_back(Builder->GetInsertBlock());
            if (thenValue) incomingValues.push_back(thenValue);
        } else if (thenValue) {
            // Do not push the block because it won’t reach merge
            // Possibly track that this branch returned and skip PHI creation later
        }

        elseBlock->insertInto(function);
        Builder->SetInsertPoint(elseBlock);
        
        if (i == conditions.size() - 1 && !elseBody) {
            // No else clause, make sure we emit a branch to merge
            if (!Builder->GetInsertBlock()->getTerminator()) {
                mergeBlock->insertInto(function);
                Builder->CreateBr(mergeBlock);
                incomingBlocks.push_back(Builder->GetInsertBlock());
            }
        }

        if (i < conditions.size() - 1) {
            nextBlock = elseBlock;
        }
    }

    // Handle final else block if provided
    if (elseBody) {
        llvm::Value* elseValue = codegen(elseBody, localScope);

        // Set insert point to current block *before* emitting the branch
        llvm::BasicBlock* currentBlock = Builder->GetInsertBlock();
        if (!currentBlock->getTerminator()) {
            mergeBlock->insertInto(function);
            Builder->CreateBr(mergeBlock);
        }

        incomingBlocks.push_back(currentBlock);
        if (elseValue) {
            incomingValues.push_back(elseValue);
        }
    } else if (nextBlock && Builder->GetInsertBlock() == nextBlock) {
        // If else not provided, and last else block is still current block
        if (!Builder->GetInsertBlock()->getTerminator()) {
            mergeBlock->insertInto(function);
            Builder->CreateBr(mergeBlock);
        }
    }

    // If mergeBlock is in the function and active
    // Set the insert point to mergeBlock so following code continues here
    if (mergeBlock->getParent()) {
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
