// src/Backends/llvm/IRGenerator/IRGenControlFlow.cpp
#include <omniscript/Backends/LLVM/IRGenerator.h>
#include <omniscript/Expressions/BlockExpression.h>
#include <algorithm>

namespace Omniscript { std::string canonicalTypeName(const std::string& name); }

namespace Omniscript {
llvm::Value* IRGenerator::createReturn(llvm::Value* returnValue, llvm::Type* expectedReturnType) {
    (void)expectedReturnType; // the function's own return type is authoritative
    llvm::Function* currentFunction = Builder->GetInsertBlock()->getParent();
    
    if (!currentFunction) {
        std::string suggestion = "To resolve this:\n"
                               "1. Ensure return statement is within a function body\n"
                               "2. Check for correct function declaration\n"
                               "3. Verify code structure";
        console.reportError(
            Console::RUNTIME_ERROR,
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
            Console::RUNTIME_ERROR,
            "LLVM Module is null",
            suggestion,
            FileSpan() // Default span as no statement context
        );
        return nullptr;
    }
    
    llvm::Value* moved = nullptr;
    if (auto* ld = llvm::dyn_cast_or_null<llvm::LoadInst>(returnValue)) moved = ld->getPointerOperand()->stripPointerCasts();
    else if (llvm::isa_and_nonnull<llvm::AllocaInst>(returnValue)) moved = returnValue;
    emitDtors(dtorBase_, moved);

    if (activeScope->get("va_list")) {
        llvm::Function* vaEndFunc = llvm::Intrinsic::getOrInsertDeclaration(Module.get(), llvm::Intrinsic::vaend);
        llvm::Value* vaListAlloca = activeScope->get("va_list");
        Builder->CreateCall(vaEndFunc, { vaListAlloca });
    }

    if (currentFunction->getReturnType()->isVoidTy()) {
        if (returnValue) {
            std::string suggestion = Console::formatString(
                "To resolve this:\n"
                "1. Remove return value for void function '%s'\n"
                "2. Use return without a value\n"
                "3. Verify function signature",
                currentFunction->getName().str().c_str()
            );
            console.reportError(
                Console::TYPE_ERROR,
                "Void function cannot return a value",
                suggestion,
                FileSpan() // Default span as no statement context
            );
        }
        return Builder->CreateRetVoid();
    }

    llvm::Type* funcRetTy = currentFunction->getReturnType();

    if (!returnValue) {
        // A bare `return` in the top-level script means "exit code 0"
        if (currentFunction->getName() == "__top_level__") {
            return Builder->CreateRet(llvm::Constant::getNullValue(funcRetTy));
        }
        std::string suggestion = Console::formatString(
            "To resolve this:\n"
            "1. Provide a return value for non-void function '%s'\n"
            "2. Check function return type\n"
            "3. Ensure return expression is valid",
            currentFunction->getName().str().c_str()
        );
        console.reportError(
            Console::TYPE_ERROR,
            "Non-void function must return a value",
            suggestion,
            FileSpan() // Default span as no statement context
        );
        return nullptr;
    }

    // Cast to the function's actual return type (e.g. `return 1` in __top_level__ -> i32)
    if (returnValue->getType() != funcRetTy) {
        llvm::Value* casted = generateCast(returnValue, funcRetTy);
        if (!casted) {
            std::string suggestion = Console::formatString(
                "To resolve this:\n"
                "1. Ensure return value type matches expected type '%s'\n"
                "2. Check expression type for '%s'\n"
                "3. Consider explicit type casting",
                debugType(funcRetTy).c_str(), debugType(returnValue->getType()).c_str()
            );
            console.reportError(
                Console::TYPE_ERROR,
                Console::formatString("Return type mismatch: expected '%s', got '%s'",
                                 debugType(funcRetTy).c_str(), debugType(returnValue->getType()).c_str()),
                suggestion,
                FileSpan() // Default span as no statement context
            );
            return nullptr;
        }
        returnValue = casted;
    }

    return Builder->CreateRet(returnValue);
}

llvm::Value* IRGenerator::createForLoop(
    const std::shared_ptr<ForLoopExpression>& forExpr,
    std::shared_ptr<SymbolTable<std::shared_ptr<Expression>, std::shared_ptr<Type>>> scope
) {
    pushScope();
    auto localScope = scope->createChildScope("forloop");

    std::string loopVarName;
    llvm::Value* initialValue = nullptr;

    if (forExpr->initializer) {
        auto varAssign = std::dynamic_pointer_cast<VariableAssignment>(forExpr->initializer);
        if (!varAssign || varAssign->isGlobal) {
            std::string suggestion = "To resolve this:\n"
                                   "1. Ensure for loop initializer is a local variable assignment\n"
                                   "2. Check initializer syntax\n"
                                   "3. Verify variable is not declared as global";
            console.reportError(
                Console::RUNTIME_ERROR,
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
            std::string suggestion = Console::formatString(
                "To resolve this:\n"
                "1. Verify initializer for variable '%s' is valid\n"
                "2. Check initializer expression\n"
                "3. Add debug output for initializer codegen",
                loopVarName.c_str()
            );
            console.reportError(
                Console::RUNTIME_ERROR,
                Console::formatString("Failed to generate initializer for variable '%s'",
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

    // break -> for.end, continue -> for.inc
    loopTargets_.push_back({afterBlock, incrementBlock, function, dtorScopes_.size()});
    struct LoopTargetGuard { std::vector<LoopTarget>& v; ~LoopTargetGuard() { v.pop_back(); } } loopTargetGuard{loopTargets_};

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
                Console::RUNTIME_ERROR,
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
    beginDtorScope();   // the body is its own scope: destroyed at the end of every iteration
    auto block = std::dynamic_pointer_cast<BlockExpression>(forExpr->body);
    if (block) {
        for (auto& expr : block->values) {
            if (expr) {
                codegen(expr, localScope);
            }
        }
    }
    endDtorScope();

    if (!currentBlockHasTerminator()) {
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
                Console::RUNTIME_ERROR,
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
    const std::shared_ptr<WhileLoopExpression>& whileExpr,
    std::shared_ptr<SymbolTable<std::shared_ptr<Expression>, std::shared_ptr<Type>>> scope
) {
    llvm::Function* function = Builder->GetInsertBlock()->getParent();
    llvm::LLVMContext& context = Builder->getContext();

    auto localScope = scope->createChildScope("whileloop");

    // Same as for/if: variables declared in the loop are dropped when the loop ends.
    pushScope();
    struct WhileScopeGuard { IRGenerator* gen; ~WhileScopeGuard() { gen->popScope(); } } whileScopeGuard{this};

    llvm::BasicBlock* condBlock = llvm::BasicBlock::Create(context, "while.cond", function);
    llvm::BasicBlock* bodyBlock = llvm::BasicBlock::Create(context, "while.body");
    llvm::BasicBlock* afterBlock = llvm::BasicBlock::Create(context, "while.end");

    // break -> while.end, continue -> while.cond
    loopTargets_.push_back({afterBlock, condBlock, function, dtorScopes_.size()});
    struct LoopTargetGuard { std::vector<LoopTarget>& v; ~LoopTargetGuard() { v.pop_back(); } } loopTargetGuard{loopTargets_};

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
            Console::TYPE_ERROR,
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
    beginDtorScope();   // per-iteration scope
    if (auto block = std::dynamic_pointer_cast<BlockExpression>(whileExpr->body)) {
        if (!block->values.empty()) {
            bodyHasContent = true;
            codegen(whileExpr->body, localScope);
        }
    }
    endDtorScope();

    // If the loop body doesn't end in a terminator (return, break, etc.), jump to condition again
    if (!currentBlockHasTerminator()) {
        Builder->CreateBr(condBlock);
    }

    // Attach afterBlock
    afterBlock->insertInto(function);
    Builder->SetInsertPoint(afterBlock);

    return nullptr;
}

void IRGenerator::registerDtorLocal(llvm::Value* alloca, llvm::Type* type) {
    if (dtorScopes_.empty() || !alloca) return;
    auto* st = llvm::dyn_cast_or_null<llvm::StructType>(type);
    if (!st || !st->hasName()) return;
    // The same local can be registered from more than one codegen path: never destroy it twice.
    for (const auto& scopeLocals : dtorScopes_)
        for (const auto& e : scopeLocals)
            if (e.ptr && e.ptr->stripPointerCasts() == alloca->stripPointerCasts()) return;
    dtorScopes_.back().push_back({alloca, st->getName().str()});
}

// The copy constructor is `T.constructor(this, o: T&)`. A reference is an opaque `ptr`, so it cannot be told
// apart from `constructor(p: SomeType*)` by its LLVM signature: registerFunction marks it "os.copyctor".
llvm::Function* IRGenerator::findCopyConstructor(const std::string& structName) {
    llvm::Module* mod = currentModule ? &*currentModule : &*Module;
    if (!mod) return nullptr;
    const std::string prefix = structName + ".constructor";
    for (llvm::Function& f : mod->functions())
        if (f.arg_size() == 2 && f.hasFnAttribute("os.copyctor") && f.getName().starts_with(prefix)) return &f;
    return nullptr;
}

// `member = Temp(...)` / `arr[i] = local` moves the object into its new owner, which destroys it.
// Drop the source from the scope-exit list so it is not destroyed twice.
void IRGenerator::unregisterDtorLocal(llvm::Value* ptr) {
    if (!ptr) return;
    llvm::Value* target = ptr->stripPointerCasts();
    for (auto& scopeLocals : dtorScopes_) {
        scopeLocals.erase(std::remove_if(scopeLocals.begin(), scopeLocals.end(),
            [&](const DtorLocal& e) { return e.ptr && !e.deferred && e.ptr->stripPointerCasts() == target; }),
            scopeLocals.end());
    }
}

// Destructors are emitted with a mangled suffix, like constructors (see createStructInstance),
// so an exact getFunction("<Struct>.destructor") misses them. Match by prefix and require the
// single `this` parameter.
llvm::Function* IRGenerator::findDestructor(const std::string& structName) {
    llvm::Module* mod = currentModule ? &*currentModule : &*Module;
    if (!mod) return nullptr;
    const std::string exact = structName + ".destructor";
    if (auto* f = mod->getFunction(exact); f && f->arg_size() == 1) return f;
    for (llvm::Function& f : mod->functions())
        if (f.arg_size() == 1 && f.getName().starts_with(exact)) return &f;
    return nullptr;
}

void IRGenerator::emitDtors(size_t fromDepth, llvm::Value* skip) {
    if (!Module || !Builder->GetInsertBlock() || currentBlockHasTerminator()) return;
    for (size_t s = dtorScopes_.size(); s-- > fromDepth;) {
        for (size_t i = dtorScopes_[s].size(); i-- > 0;) {  // reverse declaration order
            // Copy the entry: emitting a deferred action can register more locals and grow the vectors.
            DtorLocal entry = dtorScopes_[s][i];
            if (skip && entry.ptr && entry.ptr->stripPointerCasts() == skip) continue;
            if (entry.deferred) {                           // `defer`: runs in order with the destructors
                if (currentBlockHasTerminator()) return;
                codegen(entry.deferred, entry.deferScope);
                continue;
            }
            llvm::Function* dtor = findDestructor(entry.structName);
            if (!dtor) continue;   // type has no destructor
            Builder->CreateCall(dtor, { entry.ptr });
        }
    }
}

void IRGenerator::registerDefer(const std::shared_ptr<Expression>& action, SymbolTableType scope) {
    if (!action) return;
    if (dtorScopes_.empty()) {
        console.reportError(
            Console::SEMANTIC_ERROR,
            "defer outside of a function",
            "To resolve this:\n"
            "1. Use defer inside a function body, loop body or if branch",
            action->getSpan()
        );
        return;
    }
    DtorLocal entry;
    entry.deferred = action;
    entry.deferScope = scope;
    dtorScopes_.back().push_back(std::move(entry));
}

// Finds a function whose name starts with `prefix` and that takes `nparams` parameters (methods carry a
// mangled suffix, like destructors; see findDestructor).
static llvm::Function* findFnByPrefix(llvm::Module* mod, const std::string& prefix, unsigned nparams) {
    if (!mod) return nullptr;
    if (auto* f = mod->getFunction(prefix); f && f->arg_size() == nparams) return f;
    // `Arena.alloc` may be emitted as `A.Arena.alloc` (import alias) and with a mangled suffix:
    // accept the name at the start or right after a '.'.
    for (llvm::Function& f : mod->functions()) {
        if (f.arg_size() != nparams) continue;
        const std::string n = f.getName().str();
        size_t pos = n.find(prefix);
        while (pos != std::string::npos) {
            if (pos == 0 || n[pos - 1] == '.') return &f;
            pos = n.find(prefix, pos + 1);
        }
    }
    return nullptr;
}

llvm::Value* IRGenerator::createNew(const std::shared_ptr<NewExpression>& nw, SymbolTableType scope) {
    if (!nw || !Builder->GetInsertBlock() || currentBlockHasTerminator()) return nullptr;
    llvm::Module* mod = currentModule ? &*currentModule : &*Module;
    llvm::LLVMContext& ctx = *Context;
    llvm::Type* i64 = llvm::Type::getInt64Ty(ctx);
    llvm::Type* ptrTy = llvm::PointerType::getUnqual(ctx);
    llvm::FunctionCallee mallocFn = mod->getOrInsertFunction("malloc", llvm::FunctionType::get(ptrTy, { i64 }, false));

    // `new(a) T`: memory comes from `Arena.alloc(a, n)` (zeroed) instead of malloc; objects whose type has a
    // destructor are recorded with `Arena.record(a, obj, dtor)` so the arena can destroy them in reverse.
    llvm::Value* arenaV = nullptr;
    llvm::Function* arenaAlloc = nullptr;
    llvm::Function* arenaRecord = nullptr;
    if (nw->allocator) {
        arenaV = codegen(nw->allocator, scope);
        if (!arenaV) return nullptr;
        arenaAlloc = findFnByPrefix(mod, "Arena.alloc", 2);
        arenaRecord = findFnByPrefix(mod, "Arena.record", 3);
        if (!arenaAlloc || !arenaRecord) {
            std::string seen;
            int shown = 0;
            for (llvm::Function& f : mod->functions())
                if (f.getName().contains("Arena") && shown++ < 12) seen += "\n  " + f.getName().str() + " (" + std::to_string(f.arg_size()) + " params)";
            console.reportError(Console::SEMANTIC_ERROR, "new(...): Arena.alloc / Arena.record were not found",
                "To resolve this:\n1. Import standard/1/Arena.os (String.os imports it already)\n"
                "Functions with 'Arena' in their name:" + (seen.empty() ? std::string("\n  (none)") : seen), nw->getSpan());
            return nullptr;
        }
    }
    auto rawAlloc = [&](llvm::Value* bytes) -> llvm::Value* {
        if (!arenaAlloc) return Builder->CreateCall(mallocFn, { bytes }, "new.raw");
        return Builder->CreateCall(arenaAlloc, { arenaV, bytes }, "new.araw");
    };

    // `new u8[n]` / `new int[n]`: raw zeroed storage with no header, so a plain `delete p` and
    // `delete[] p` both free it. Only arrays of structs carry the count header (see below).
    if (nw->count && nw->type) {
        if (auto pointee = nw->type->getPointeeType()) {
            llvm::Type* elemLL = resolveLLVMType(pointee);
            if (elemLL && !elemLL->isStructTy() && elemLL->isSized()) {
                llvm::Value* countV = codegen(nw->count, scope);
                if (!countV) return nullptr;
                if (!countV->getType()->isIntegerTy()) {
                    console.reportError(Console::TYPE_ERROR, "new '" + nw->allocTypeName + "[...]': the element count must be an integer",
                                        nw->getSpan());
                    return nullptr;
                }
                countV = Builder->CreateIntCast(countV, i64, true, "new.count");
                uint64_t elemSize = mod->getDataLayout().getTypeAllocSize(elemLL).getFixedValue();
                if (elemSize == 0) elemSize = 1;
                llvm::Value* bytes = Builder->CreateMul(countV, llvm::ConstantInt::get(i64, elemSize), "new.bytes");
                llvm::Value* raw = rawAlloc(bytes);
                if (!raw) return nullptr;
                Builder->CreateMemSet(raw, llvm::ConstantInt::get(llvm::Type::getInt8Ty(ctx), 0), bytes, llvm::MaybeAlign(1));
                return raw;
            }
        }
    }

    llvm::StructType* st = nullptr;
    std::vector<std::string> names = { nw->allocTypeName, canonicalTypeName(nw->allocTypeName) };
    if (size_t d = nw->allocTypeName.rfind('.'); d != std::string::npos) names.push_back(nw->allocTypeName.substr(d + 1));
    for (const auto& n : names) {
        if (auto* t = llvm::dyn_cast_or_null<llvm::StructType>(activeScope->getType(n))) { st = t; break; }
        if (auto* t = llvm::StructType::getTypeByName(ctx, n)) { st = t; break; }
    }
    if (!st || !st->isSized()) {
        console.reportError(Console::TYPE_ERROR, "new: unknown or unsized type '" + nw->allocTypeName + "'", nw->getSpan());
        return nullptr;
    }
    if (nw->members.size() != st->getNumElements()) {
        console.reportError(Console::TYPE_ERROR, "new '" + nw->allocTypeName + "': expected " +
            std::to_string(st->getNumElements()) + " field values, got " + std::to_string(nw->members.size()), nw->getSpan());
        return nullptr;
    }

    uint64_t size = mod->getDataLayout().getTypeAllocSize(st).getFixedValue();
    if (size == 0) size = 1;

    // Arena objects with a destructor are recorded so Arena.destroy() runs it.
    auto recordDtor = [&](llvm::Value* obj) {
        if (!arenaV) return;
        if (llvm::Function* dtor = findDestructor(st->getName().str()))
            Builder->CreateCall(arenaRecord, { arenaV, obj, dtor });
    };

    // Stores the initial field values (defaults / field initialisers) of the object at `objPtr`.
    auto initFields = [&](llvm::Value* objPtr) -> bool {
        for (unsigned i = 0; i < st->getNumElements(); ++i) {
            llvm::Value* v = nw->members[i] ? codegen(nw->members[i]->value, scope) : nullptr;
            if (!v) return false;
            llvm::Type* want = st->getElementType(i);
            if (v->getType() != want) {
                v = generateCast(v, want);
                if (!v) return false;
            }
            Builder->CreateStore(v, Builder->CreateStructGEP(st, objPtr, i, "new.field" + std::to_string(i)));
        }
        return true;
    };

    // `new T[count](args)`: [ i64 count | pad to 16 | element 0 | element 1 | ... ]. The result points at
    // element 0; `delete[]` reads the count back from the 16 bytes in front of it.
    if (nw->count) {
        constexpr uint64_t kArrayHeader = 16;
        llvm::Value* countV = codegen(nw->count, scope);
        if (!countV) return nullptr;
        if (!countV->getType()->isIntegerTy()) {
            console.reportError(Console::TYPE_ERROR, "new '" + nw->allocTypeName + "[...]': the element count must be an integer",
                                nw->getSpan());
            return nullptr;
        }
        countV = Builder->CreateIntCast(countV, i64, true, "new.count");

        llvm::Value* bytes = Builder->CreateAdd(
            Builder->CreateMul(countV, llvm::ConstantInt::get(i64, size), "new.elembytes"),
            llvm::ConstantInt::get(i64, kArrayHeader), "new.bytes");
        llvm::Value* raw = rawAlloc(bytes);
        if (!raw) return nullptr;
        Builder->CreateStore(countV, raw);
        llvm::Value* base = Builder->CreateGEP(llvm::Type::getInt8Ty(ctx), raw,
                                               { llvm::ConstantInt::get(i64, kArrayHeader) }, "new.arr");

        llvm::Function* fn = Builder->GetInsertBlock()->getParent();
        llvm::BasicBlock* pre  = Builder->GetInsertBlock();
        llvm::BasicBlock* cond = llvm::BasicBlock::Create(ctx, "new.cond", fn);
        llvm::BasicBlock* body = llvm::BasicBlock::Create(ctx, "new.body", fn);
        llvm::BasicBlock* done = llvm::BasicBlock::Create(ctx, "new.done", fn);
        Builder->CreateBr(cond);

        Builder->SetInsertPoint(cond);
        llvm::PHINode* idx = Builder->CreatePHI(i64, 2, "new.i");
        idx->addIncoming(llvm::ConstantInt::get(i64, 0), pre);
        Builder->CreateCondBr(Builder->CreateICmpSLT(idx, countV, "new.more"), body, done);

        Builder->SetInsertPoint(body);
        llvm::Value* elem = Builder->CreateGEP(st, base, { idx }, "new.elem");
        if (!initFields(elem)) return nullptr;
        activeScope->set(nw->hiddenName, elem);   // the constructor's `this` is this element
        if (nw->ctorCall) codegen(nw->ctorCall, scope);
        recordDtor(elem);
        llvm::Value* next = Builder->CreateAdd(idx, llvm::ConstantInt::get(i64, 1), "new.next");
        idx->addIncoming(next, Builder->GetInsertBlock());
        Builder->CreateBr(cond);

        Builder->SetInsertPoint(done);
        return base;
    }

    llvm::Value* ptr = rawAlloc(llvm::ConstantInt::get(i64, size));
    if (!ptr) return nullptr;
    if (!initFields(ptr)) return nullptr;

    // The ctor's `this` is ReferenceTo(hiddenName); make that name resolve to the heap pointer.
    activeScope->set(nw->hiddenName, ptr);
    if (nw->ctorCall && !codegen(nw->ctorCall, scope) && !currentBlockHasTerminator()) {
        // void ctor returns nullptr legitimately; only real failures were reported by codegen
    }
    recordDtor(ptr);
    return ptr;   // heap object: never registered for scope-exit destruction
}

llvm::Value* IRGenerator::createDelete(const std::shared_ptr<DeleteExpression>& del, SymbolTableType scope) {
    if (!del || !del->operand) return nullptr;
    if (!Builder->GetInsertBlock() || currentBlockHasTerminator()) return nullptr;   // unreachable code

    llvm::Function* function = Builder->GetInsertBlock()->getParent();
    llvm::Module* mod = currentModule ? &*currentModule : &*Module;
    llvm::LLVMContext& ctx = *Context;

    auto omniType = del->operand->getType();
    if (!omniType || !omniType->isPointer()) {
        console.reportError(
            Console::TYPE_ERROR,
            Console::formatString("delete needs a pointer operand, got '%s'",
                                  omniType ? omniType->toString().c_str() : "unknown"),
            "To resolve this:\n"
            "1. Pass a pointer, for example the result of an allocation\n"
            "2. Do not delete values or locals; they are destroyed automatically at scope exit",
            del->getSpan()
        );
        return nullptr;
    }

    llvm::Value* ptr = codegen(del->operand, scope);
    if (!ptr || !ptr->getType()->isPointerTy()) {
        console.reportError(Console::SEMANTIC_ERROR, "The operand of delete is invalid", "", del->getSpan());
        return nullptr;
    }

    // `delete &local` would destroy the local twice (once here, once at scope exit).
    {
        llvm::Value* base = ptr->stripPointerCasts();
        for (const auto& scopeLocals : dtorScopes_) {
            for (const auto& local : scopeLocals) {
                if (local.ptr && local.ptr->stripPointerCasts() == base) {
                    console.reportError(
                        Console::SEMANTIC_ERROR,
                        "Cannot delete a local variable: it is destroyed automatically at scope exit",
                        "To resolve this:\n"
                        "1. Only delete pointers that own heap memory\n"
                        "2. Remove the delete",
                        del->getSpan()
                    );
                    return nullptr;
                }
            }
        }
    }

    // Pointers are opaque `ptr`, so the destructor is found through the Omniscript pointee type,
    // using the same `<Struct>.destructor` name that scope-exit cleanup uses.
    llvm::Function* dtor = nullptr;
    llvm::StructType* elemStruct = nullptr;
    if (auto pointee = omniType->getPointeeType()) {
        llvm::Type* pointeeLL = resolveLLVMType(pointee);
        auto* st = llvm::dyn_cast_or_null<llvm::StructType>(pointeeLL);
        if (st && st->hasName()) {
            dtor = findDestructor(st->getName().str());
            elemStruct = st;
        }
    }

    llvm::BasicBlock* doBlock  = llvm::BasicBlock::Create(ctx, "delete.do", function);
    llvm::BasicBlock* endBlock = llvm::BasicBlock::Create(ctx, "delete.end", function);

    // delete of a null pointer is a no-op
    llvm::Value* isNull = Builder->CreateICmpEQ(
        ptr, llvm::ConstantPointerNull::get(llvm::cast<llvm::PointerType>(ptr->getType())), "delete.isnull");
    Builder->CreateCondBr(isNull, endBlock, doBlock);

    Builder->SetInsertPoint(doBlock);
    llvm::Type* ptrTy = llvm::PointerType::getUnqual(ctx);
    llvm::FunctionCallee freeFn = mod->getOrInsertFunction(
        "free", llvm::FunctionType::get(llvm::Type::getVoidTy(ctx), { ptrTy }, false));

    if (del->isArray && elemStruct) {
        // `new T[n]` (T a struct) stores the count in the 16 bytes in front of element 0 (see createNew).
        constexpr int64_t kArrayHeader = 16;
        llvm::Type* i64 = llvm::Type::getInt64Ty(ctx);
        llvm::Type* i8  = llvm::Type::getInt8Ty(ctx);
        llvm::Value* raw = Builder->CreateGEP(i8, ptr, { llvm::ConstantInt::getSigned(i64, -kArrayHeader) }, "delete.raw");
        if (dtor && elemStruct) {
            llvm::Value* count = Builder->CreateLoad(i64, raw, "delete.count");
            llvm::BasicBlock* pre  = Builder->GetInsertBlock();
            llvm::BasicBlock* cond = llvm::BasicBlock::Create(ctx, "delete.cond", function);
            llvm::BasicBlock* body = llvm::BasicBlock::Create(ctx, "delete.body", function);
            llvm::BasicBlock* done = llvm::BasicBlock::Create(ctx, "delete.done", function);
            Builder->CreateBr(cond);

            Builder->SetInsertPoint(cond);
            llvm::PHINode* remaining = Builder->CreatePHI(i64, 2, "delete.n");
            remaining->addIncoming(count, pre);
            Builder->CreateCondBr(Builder->CreateICmpSGT(remaining, llvm::ConstantInt::get(i64, 0)), body, done);

            Builder->SetInsertPoint(body);   // last element first
            llvm::Value* last = Builder->CreateSub(remaining, llvm::ConstantInt::get(i64, 1), "delete.i");
            Builder->CreateCall(dtor, { Builder->CreateGEP(elemStruct, ptr, { last }, "delete.elem") });
            remaining->addIncoming(last, Builder->GetInsertBlock());
            Builder->CreateBr(cond);

            Builder->SetInsertPoint(done);
        }
        Builder->CreateCall(freeFn, { raw });   // the allocation starts at the header
    } else {
        if (dtor) {
            Builder->CreateCall(dtor, { ptr });
        }
        Builder->CreateCall(freeFn, { ptr });
    }
    Builder->CreateBr(endBlock);

    Builder->SetInsertPoint(endBlock);
    return nullptr;
}

llvm::Value* IRGenerator::createLoopJump(bool isBreak, const FileSpan& span) {
    const char* word = isBreak ? "break" : "continue";
    llvm::Function* function = Builder->GetInsertBlock()->getParent();

    if (loopTargets_.empty() || loopTargets_.back().function != function) {
        console.reportError(
            Console::SEMANTIC_ERROR,
            Console::formatString("'%s' outside of a loop", word),
            Console::formatString("To resolve this:\n"
                                  "1. Use '%s' only inside a for or while loop body\n"
                                  "2. 'continue' directly inside a switch case is not supported", word),
            span
        );
        return nullptr;
    }

    if (!currentBlockHasTerminator()) {
        const LoopTarget& target = loopTargets_.back();
        emitDtors(target.dtorDepth);   // locals of the loop body (and nested blocks) end here
        Builder->CreateBr(isBreak ? target.breakTarget : target.continueTarget);
    }

    // Anything emitted after the jump (the rest of the block, the if-merge branch) needs a
    // valid insertion point. This block has no predecessors, so it is dead code.
    llvm::BasicBlock* dead = llvm::BasicBlock::Create(
        Builder->getContext(), isBreak ? "break.dead" : "continue.dead", function);
    Builder->SetInsertPoint(dead);
    return nullptr;
}

llvm::Value* IRGenerator::createIfStatement( 
    const std::vector<std::shared_ptr<Expression>>& conditions,
    const std::vector<std::shared_ptr<Expression>>& bodies,
    const std::shared_ptr<Expression>& elseBody,
    std::shared_ptr<SymbolTable<std::shared_ptr<Expression>, std::shared_ptr<Type>>> scope)
{
    if (conditions.empty() || conditions.size() != bodies.size()) {
        std::string suggestion = "To resolve this:\n"
                               "1. Ensure if statement has at least one condition and matching body\n"
                               "2. Verify conditions and bodies vectors have equal size\n"
                               "3. Check if statement syntax";
        console.reportError(
            Console::RUNTIME_ERROR,
            Console::formatString("Invalid if statement: %d conditions, %d bodies",
                             conditions.size(), bodies.size()),
            suggestion,
            conditions.empty() ? FileSpan() : conditions[0]->getSpan()
        );
        return nullptr;
    }

    llvm::Function* function = Builder->GetInsertBlock()->getParent();
    llvm::LLVMContext& context = Builder->getContext();

    auto localScope = scope->createChildScope("ifscope");

    // Variables declared in the condition or in any branch live only until the end of the if statement.
    // Without this, a `let x` in one branch stayed in the function scope, so a later `let x` of another
    // type in a sibling block was treated as a reassignment of the first one.
    pushScope();
    struct IfScopeGuard { IRGenerator* gen; ~IfScopeGuard() { gen->popScope(); } } ifScopeGuard{this};

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
            std::string suggestion = Console::formatString(
                "To resolve this:\n"
                "1. Verify condition #%d is valid\n"
                "2. Check condition expression syntax\n"
                "3. Add debug output for condition codegen",
                i + 1
            );
            console.reportError(
                Console::RUNTIME_ERROR,
                Console::formatString("Failed to generate code for condition #%d", i + 1),
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
        beginDtorScope();
        llvm::Value* thenValue = codegen(bodies[i], localScope);
        endDtorScope();
        DEBUG_LOG(bodies[i]->toString());

        bool hasTerminator = currentBlockHasTerminator();
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
            if (!currentBlockHasTerminator()) {
                Builder->CreateBr(mergeBlock);
                mergeBlockUsed = true;
                incomingBlocks.push_back(Builder->GetInsertBlock());
            }
        }

        // Always move on to the else block (needed for "else if" chains that have no
        // final plain else; previously this only happened when elseBody was set, which
        // left the next condition being emitted after the then-block's terminator).
        if (elseBlock != mergeBlock) {
            nextBlock = elseBlock;
            elseBlock->insertInto(function);
            Builder->SetInsertPoint(elseBlock);
        }
    }

    // Handle final else block if provided
    if (elseBody) {
        beginDtorScope();
        llvm::Value* elseValue = codegen(elseBody, localScope);
        endDtorScope();
        if (!elseValue && !std::dynamic_pointer_cast<BlockExpression>(elseBody)) {
            std::string suggestion = "To resolve this:\n"
                                   "1. Verify else block expression is valid\n"
                                   "2. Check else body syntax\n"
                                   "3. Add debug output for else block codegen";
            console.reportError(
                Console::RUNTIME_ERROR,
                "Failed to generate code for else block",
                suggestion,
                elseBody->getSpan()
            );
            return nullptr;
        }

        // Set insert point to current block *before* emitting the branch
        llvm::BasicBlock* currentBlock = Builder->GetInsertBlock();
        if ((currentBlock->empty() || !currentBlock->back().isTerminator())) {
            Builder->CreateBr(mergeBlock);
            mergeBlockUsed = true;
        }

        incomingBlocks.push_back(currentBlock);
        if (elseValue) {
            incomingValues.push_back(elseValue);
        }
    } else if (nextBlock && Builder->GetInsertBlock() == nextBlock) {
        // If else not provided, and last else block is still current block
        if (!currentBlockHasTerminator()) {
            Builder->CreateBr(mergeBlock);
            mergeBlockUsed = true;
        }
    }

    if (mergeBlockUsed) {
        mergeBlock->insertInto(function);
        Builder->SetInsertPoint(mergeBlock);
    }

    // Emit merge block if used
    // A PHI is only valid when every path into the merge block supplies a value,
    // which requires a final else. Without one the if is a statement.
    if (!elseBody) {
        return nullptr;
    }
    if (!incomingValues.empty() && incomingValues.size() == incomingBlocks.size()) {
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
                    Console::TYPE_ERROR,
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