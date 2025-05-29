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
    if (conditions.empty() || conditions.size() != bodies.size()) return nullptr;

    llvm::Function* function = Builder->GetInsertBlock()->getParent();
    llvm::LLVMContext& context = Builder->getContext();

    auto localScope = scope->createChildScope("ifscope");

    llvm::BasicBlock* mergeBlock = llvm::BasicBlock::Create(context, "if.merge", function);
    llvm::BasicBlock* currentBlock = Builder->GetInsertBlock();
    llvm::BasicBlock* nextCondBlock = nullptr;

    // Keep track of where to jump after the current condition fails
    llvm::BasicBlock* afterBlock = nullptr;

    for (size_t i = 0; i < conditions.size(); ++i) {
        llvm::Value* condValue = codegen(conditions[i], localScope);
        if (!condValue) return nullptr;

        // Convert to i1 if needed
        if (condValue->getType()->isIntegerTy(32)) {
            condValue = Builder->CreateICmpNE(condValue, llvm::ConstantInt::get(condValue->getType(), 0), "ifcond");
        }

        llvm::BasicBlock* thenBlock = llvm::BasicBlock::Create(context, "if.then", function);
        nextCondBlock = (i + 1 < conditions.size()) 
        ? llvm::BasicBlock::Create(context, "if.nextcond", function)
        : (elseBody ? llvm::BasicBlock::Create(context, "if.else", function) : mergeBlock);
        
        Builder->CreateCondBr(condValue, thenBlock, nextCondBlock);

        // THEN block
        Builder->SetInsertPoint(thenBlock);
        if (!codegen(bodies[i], localScope)) {
            Builder->CreateBr(mergeBlock);
        }

        if (!Builder->GetInsertBlock()->getTerminator()) {
            Builder->CreateBr(mergeBlock);
        }
        
        // Prepare to emit the next condition or else block
        Builder->SetInsertPoint(nextCondBlock);
    }
    // Only create merge block if any branch will jump to it
    bool mergeUsed = false;

    // ELSE block if present
    if (elseBody) {
        DEBUG_LOG("There is an else body");
        if (!codegen(elseBody, localScope)) return nullptr;
        if (!Builder->GetInsertBlock()->getTerminator()) {
            Builder->CreateBr(mergeBlock);
            mergeUsed = true;
        }
    } else {
        DEBUG_LOG("No else body");
    }

    // If the last 'else if' condition didn't return or branch
    if (!Builder->GetInsertBlock()->getTerminator()) {
        DEBUG_LOG("Last else if has no terminator");
        Builder->CreateBr(mergeBlock);
        mergeUsed = true;
    } else {
        DEBUG_LOG("Last else if has a terminator");
    }

    // If mergeBlock was actually used, set insert point there
    if (mergeUsed) {
        DEBUG_LOG("Merge block used");
        Builder->SetInsertPoint(mergeBlock);
        if (!Builder->GetInsertBlock()->getTerminator()) {
            console.error("Merge has no terminator");
        }
    } else {
        // Erase the unused merge block from the function
        DEBUG_LOG("Merge block was not used");
        mergeBlock->eraseFromParent();
    }

    return nullptr;
}

