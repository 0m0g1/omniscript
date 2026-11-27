// src/Backends/llvm/IRGenerator/IRGeneratorCodegen.cpp
#include <cctype>
#include <functional>
#include <map>
#include <set>
#include <omniscript/Expressions/Expressions.h>
#include <omniscript/Backends/LLVM/IRGenerator.h>
#include <omniscript/Semantic/GenericInstantiator.h>
#include <omniscript/Expressions/CastExpression.h>
#include <omniscript/Expressions/BlockExpression.h>
#include <omniscript/Expressions/TypeExpressions.h>
#include <omniscript/Expressions/GetterExpressions.h>
#include <omniscript/Expressions/EntityExpressions.h>
#include <omniscript/Expressions/AccessExpressions.h>
#include <omniscript/Expressions/CallableExpression.h>
#include <omniscript/Expressions/LiteralExpressions.h>
#include <omniscript/Expressions/AssignmentExpression.h>
#include <omniscript/Expressions/ControlFlowExpressions.h>
#include <omniscript/Expressions/NullableExpressions.h>

namespace Omniscript { std::string canonicalTypeName(const std::string& name); }

namespace Omniscript {
// Nullable<T> is lowered as an *unnamed* literal struct { i1 hasValue, T value } (see resolveLLVMType).
// Checked structurally here so this does not depend on isNullableStruct()'s exact rules.
static bool looksNullable(llvm::Type* t) {
    auto* st = llvm::dyn_cast<llvm::StructType>(t);
    return st && !st->hasName() && st->getNumElements() == 2 && st->getElementType(0)->isIntegerTy(1);
}

// ---------------------------------------------------------------------------
// Atomic intrinsics. `intrinsic fn atomic_<op>_<type>(...)` has no llvm.* intrinsic behind it:
// atomics are instructions, so the function is defined here with a one-block body that the
// optimizer inlines. Ops: add sub and or xor xchg (ptr, value) -> OLD value,
// cmpxchg (ptr, expected, desired) -> OLD value, load (ptr), store (ptr, value).
// Returns nullptr when `intrinsicName` is not an atomic op (caller falls back to llvm.*).
// ---------------------------------------------------------------------------
static llvm::Function* defineAtomicIntrinsic(
    llvm::Module* mod,
    const std::string& mangledName,
    std::string intrinsicName,
    llvm::Type* retTy,
    const std::vector<llvm::Type*>& paramTys
) {
    const std::string intrinsicPrefix = "intrinsic_";
    if (intrinsicName.rfind(intrinsicPrefix, 0) == 0) intrinsicName = intrinsicName.substr(intrinsicPrefix.size());
    // `intrinsic fn call_ptr1(f: void*, obj: void*) => void;` calls the function pointer `f` with one pointer
    // argument (used by Arena.destroy to run recorded destructors; OmniScript has no indirect calls).
    if (mod && intrinsicName == "call_ptr1") {
        if (paramTys.size() != 2 || !retTy->isVoidTy()) {
            console.reportError(Console::SEMANTIC_ERROR,
                "Intrinsic 'call_ptr1' must be declared as (f: void*, obj: void*) => void", FileSpan{});
            return nullptr;
        }
        llvm::Function* fn = mod->getFunction(mangledName);
        if (fn && !fn->empty()) return fn;   // already defined
        if (!fn) {
            auto* fnTy = llvm::FunctionType::get(retTy, paramTys, false);
            fn = llvm::Function::Create(fnTy, llvm::GlobalValue::InternalLinkage, mangledName, mod);
        }
        llvm::LLVMContext& cctx = mod->getContext();
        auto* centry = llvm::BasicBlock::Create(cctx, "entry", fn);
        llvm::IRBuilder<> cb(centry);
        auto cit = fn->arg_begin();
        llvm::Value* target = &*cit++;
        llvm::Value* obj = &*cit++;
        auto* calleeTy = llvm::FunctionType::get(llvm::Type::getVoidTy(cctx), { llvm::PointerType::getUnqual(cctx) }, false);
        cb.CreateCall(calleeTy, target, { obj });
        cb.CreateRetVoid();
        return fn;
    }
    const std::string atomicPrefix = "atomic_";
    if (!mod || intrinsicName.rfind(atomicPrefix, 0) != 0) return nullptr;

    std::string rest = intrinsicName.substr(atomicPrefix.size());
    std::string op = rest.substr(0, rest.find('_'));

    llvm::AtomicRMWInst::BinOp rmw;
    bool isRmw = true;
    if (op == "add") rmw = llvm::AtomicRMWInst::Add;
    else if (op == "sub") rmw = llvm::AtomicRMWInst::Sub;
    else if (op == "and") rmw = llvm::AtomicRMWInst::And;
    else if (op == "or")  rmw = llvm::AtomicRMWInst::Or;
    else if (op == "xor") rmw = llvm::AtomicRMWInst::Xor;
    else if (op == "xchg") rmw = llvm::AtomicRMWInst::Xchg;
    else { isRmw = false; }

    const size_t needed = isRmw ? 2 : (op == "cmpxchg" ? 3 : (op == "load" ? 1 : (op == "store" ? 2 : 0)));
    if (needed == 0 || paramTys.size() != needed) {
        console.reportError(Console::SEMANTIC_ERROR,
            "Atomic intrinsic '" + intrinsicName + "' has the wrong number of parameters or an unknown operation",
            FileSpan{});
        return nullptr;
    }

    llvm::Function* fn = mod->getFunction(mangledName);
    if (fn && !fn->empty()) return fn;   // already defined
    if (!fn) {
        auto* fnTy = llvm::FunctionType::get(retTy, paramTys, false);
        fn = llvm::Function::Create(fnTy, llvm::GlobalValue::InternalLinkage, mangledName, mod);
    }
    fn->addFnAttr(llvm::Attribute::AlwaysInline);

    llvm::LLVMContext& ctx = mod->getContext();
    auto* entry = llvm::BasicBlock::Create(ctx, "entry", fn);
    llvm::IRBuilder<> b(entry);   // own builder: leaves the generator's insertion point alone

    auto it = fn->arg_begin();
    llvm::Value* ptr = &*it++;
    const auto ordering = llvm::AtomicOrdering::SequentiallyConsistent;

    if (op == "load") {
        llvm::Type* valTy = retTy;
        llvm::Align align = llvm::Align(mod->getDataLayout().getTypeStoreSize(valTy).getFixedValue());
        auto* ld = b.CreateAlignedLoad(valTy, ptr, align);
        ld->setAtomic(ordering);
        b.CreateRet(ld);
    } else if (op == "store") {
        llvm::Value* val = &*it++;
        llvm::Align align = llvm::Align(mod->getDataLayout().getTypeStoreSize(val->getType()).getFixedValue());
        auto* st = b.CreateAlignedStore(val, ptr, align);
        st->setAtomic(ordering);
        if (retTy->isVoidTy()) b.CreateRetVoid(); else b.CreateRet(val);
    } else if (op == "cmpxchg") {
        llvm::Value* expected = &*it++;
        llvm::Value* desired = &*it++;
        llvm::Align align = llvm::Align(mod->getDataLayout().getTypeStoreSize(expected->getType()).getFixedValue());
        auto* x = b.CreateAtomicCmpXchg(ptr, expected, desired, llvm::MaybeAlign(align), ordering, ordering);
        b.CreateRet(b.CreateExtractValue(x, 0));   // old value, like InterlockedCompareExchange
    } else {
        llvm::Value* val = &*it++;
        llvm::Align align = llvm::Align(mod->getDataLayout().getTypeStoreSize(val->getType()).getFixedValue());
        auto* r = b.CreateAtomicRMW(rmw, ptr, val, llvm::MaybeAlign(align), ordering);
        if (retTy->isVoidTy()) b.CreateRetVoid(); else b.CreateRet(r);   // old value
    }
    return fn;
}

static bool isUnsignedOmniType(const std::shared_ptr<Type>& t) {
    if (!t) return false;
    // Decide from the type's Kind first (names in Types.cpp's baseTypeMap): independent of how toString() spells it.
    switch (t->getKind()) {
        case Kind::UInt8: case Kind::UInt16: case Kind::UInt32: case Kind::UInt64:
        case Kind::UInt128: case Kind::UInt256: case Kind::UInt512: case Kind::UInt1024:
        case Kind::Size_t:
            return true;
        default:
            break;
    }
    std::string s = t->toString();
    // Case-insensitive: the engine may spell the same type "UInt32", "uint32", "u32" or "unsigned int".
    for (auto& ch : s) ch = (char)std::tolower((unsigned char)ch);
    // Strip qualifiers that can prefix the spelling.
    for (const char* q : {"const ", "volatile "}) {
        const std::string qs(q);
        while (s.rfind(qs, 0) == 0) s = s.substr(qs.size());
    }
    const bool uns =
        s.rfind("uint", 0) == 0 || s.rfind("usize", 0) == 0 || s.rfind("unsigned", 0) == 0 ||
        s == "u8" || s == "u16" || s == "u32" || s == "u64" || s == "u128";
    DEBUG_LOG("[isUnsignedOmniType] '" + t->toString() + "' -> " + std::string(uns ? "unsigned" : "signed"));
    return uns;
}

llvm::Value* IRGenerator::codegen(std::shared_ptr<Expression> value, SymbolTableType scope) {
    DEBUG_LOG();
    if (!value) {
        console.reportError(Console::INTERNAL_ERROR, "There is no value to be codegened.", FileSpan{});
        return nullptr;
    }
    
    if (!scope) {
        console.reportError(Console::INTERNAL_ERROR, "There is no scope for codegen to perform its operations in.", FileSpan{});
        return nullptr;
    }
    
    setSpan(value->getSpan());

    // Generic instances (Iter_i32, Zip_i32_i64, ...) are only *expressed* by GenericInstantiator
    // and queued; nothing else emits them, so emit them here.
    //  - emitInstanceNamed: on demand, only the instance a type actually needs (so we never
    //    emit an instance whose own dependencies are not registered yet).
    //  - emitAllInstances: everything queued, in creation (dependency) order.
    //  - instance method bodies are deferred until the outermost block has registered
    //    every type and function, then flushed.
    static std::vector<std::shared_ptr<Expression>> genericEmitQueue;
    static std::vector<std::function<void()>> deferredBodies;
    static int emitDepth = 0;
    static int blockDepth = 0;
    static bool flushingBodies = false;
    // Struct declarations found anywhere in the program (instances sit in nested blocks, in
    // creation order, so a Chain_f64 can appear before the Iter_f64 its field needs).
    // A struct whose by-value field type is not registered yet pulls that declaration forward.
    static std::map<std::string, std::shared_ptr<StructExpression>> structDecls;
    static std::set<std::string> startedStructs;

    std::function<void(const std::shared_ptr<Expression>&)> collectStructs =
        [&](const std::shared_ptr<Expression>& e) {
            if (!e) return;
            if (auto st = std::dynamic_pointer_cast<StructExpression>(e)) {
                structDecls[st->name] = st;
            } else if (auto cl = std::dynamic_pointer_cast<ClassExpression>(e)) {
                if (cl->structExpr) structDecls[cl->structExpr->name] = cl->structExpr;
            } else if (auto b = std::dynamic_pointer_cast<BlockExpression>(e)) {
                for (const auto& v : b->values) collectStructs(v);
            }
        };
    std::function<std::shared_ptr<StructExpression>(const std::string&, int)> findStructDeclImpl =
        [&](const std::string& n, int depth) -> std::shared_ptr<StructExpression> {
        auto it = structDecls.find(n);
        if (it != structDecls.end()) return it->second;
        for (auto& [q, decl] : structDecls) {
            if (q.size() > n.size() && q.compare(q.size() - n.size() - 1, std::string::npos, "." + n) == 0)
                return decl;
        }
        if (depth > 0) return nullptr;
        // `Str.String` (alias-qualified name of an already loaded module's type): the decl is
        // registered under its canonical name (`String`), or only its last segment matches.
        const std::string canon = Omniscript::canonicalTypeName(n);
        if (canon != n) {
            if (auto d = findStructDeclImpl(canon, depth + 1)) return d;
        }
        const size_t dot = n.rfind('.');
        if (dot != std::string::npos) {
            if (auto d = findStructDeclImpl(n.substr(dot + 1), depth + 1)) return d;
        }
        return nullptr;
    };
    auto findStructDecl = [&](const std::string& n) -> std::shared_ptr<StructExpression> {
        if (auto d = findStructDeclImpl(n, 0)) return d;
        // Not in any block codegen has seen: a struct synthesised while expressing an expression
        // (the inferred type of `let w = { a: 1 }`) is only stored in the scope under its name.
        return std::dynamic_pointer_cast<StructExpression>(scope->get(n));
    };

    auto pullPending = [&]() {
        for (auto& inst : GenericInstantiator::drainPending()) genericEmitQueue.push_back(inst);
    };
    auto instanceName = [](const std::shared_ptr<Expression>& e) -> std::string {
        if (auto st = std::dynamic_pointer_cast<StructExpression>(e)) return st->name;
        if (auto cl = std::dynamic_pointer_cast<ClassExpression>(e)) return cl->structExpr ? cl->structExpr->name : "";
        return "";
    };
    auto emitOne = [&](size_t idx) {
        auto inst = genericEmitQueue[idx];
        genericEmitQueue.erase(genericEmitQueue.begin() + idx);   // erase first: re-entrancy safe
        ++emitDepth;
        codegen(inst, scope);
        --emitDepth;
    };
    auto emitAllInstances = [&]() {
        pullPending();
        while (!genericEmitQueue.empty()) {
            emitOne(0);
            pullPending();
        }
    };
    auto emitInstanceNamed = [&](const std::string& n) {
        pullPending();
        for (size_t i = 0; i < genericEmitQueue.size(); ++i) {
            const std::string q = instanceName(genericEmitQueue[i]);
            if (q == n || (q.size() > n.size() && q.compare(q.size() - n.size() - 1, std::string::npos, "." + n) == 0)) {
                emitOne(i);
                return;
            }
        }
    };
    std::function<void(const std::shared_ptr<Type>&)> prepareType = [&](const std::shared_ptr<Type>& t) {
        if (!t) return;
        GenericInstantiator::ensureInstancesIn(t);
        if (auto udt = std::dynamic_pointer_cast<UserDefinedType>(t)) {
            emitInstanceNamed(udt->getName());
            // Not in the generic queue and not registered: it is declared elsewhere in the
            // program (possibly later). Emit its struct now; bodies are deferred via emitDepth.
            if (!activeScope->getType(udt->getName())) {
                auto decl = findStructDecl(udt->getName());
                DEBUG_LOG("[prepareType] '" + udt->getName() + "' unregistered: decl " + std::string(decl ? "found" : "NOT FOUND"));
                if (decl && !startedStructs.count(decl->name)) {
                    ++emitDepth;
                    codegen(decl, scope);
                    --emitDepth;
                }
            }
        } else if (auto nullable = std::dynamic_pointer_cast<NullableType>(t)) {
            prepareType(nullable->innerType);
        } else if (auto fixedArr = std::dynamic_pointer_cast<FixedArrayType>(t)) {
            prepareType(fixedArr->elementType);
        }
        // pointers / references are opaque `ptr`: the pointee need not exist yet.
    };
    auto prepareSignature = [&](const std::shared_ptr<Type>& ret,
                                const std::vector<std::shared_ptr<Expression>>& params) {
        prepareType(ret);
        for (const auto& prm : params) if (prm) prepareType(prm->getType());
    };
    auto flushDeferredBodies = [&]() {
        if (flushingBodies) return;
        flushingBodies = true;
        emitAllInstances();
        while (!deferredBodies.empty()) {
            auto fn = deferredBodies.front();
            deferredBodies.erase(deferredBodies.begin());
            fn();
            emitAllInstances();   // anything a body instantiated lazily
        }
        flushingBodies = false;
    };

    DEBUG_LOG("Calling codegen on scope '" + scope->getName() + "' for '" + value->toString() + "'.");
    
    llvm::Value* result = codegenPrimitive(value, scope);

    if (result) {
        return result;
    }

    if (auto nv = std::dynamic_pointer_cast<NullableValueExpression>(value)) {
        llvm::Value* opt = codegen(nv->optional, scope);
        return opt ? createNullableValue(opt) : nullptr;
    }

    if (auto nvo = std::dynamic_pointer_cast<NullableValueOrExpression>(value)) {
        llvm::Value* opt = codegen(nvo->optional, scope);
        llvm::Value* fallback = codegen(nvo->fallback, scope);
        return (opt && fallback) ? createNullableValueOr(opt, fallback) : nullptr;
    }

    if (auto so = std::dynamic_pointer_cast<SizeOfExpression>(value)) {
        prepareType(so->measuredType);   // generic instances / later-declared structs must exist first
        // A by-value user type must be measured as its struct, not as the opaque `ptr`
        // resolveLLVMType can hand back for it: look the registered struct up by name first.
        // The type may still be an UnresolvedType (a struct name the parser could not resolve).
        llvm::Type* ty = nullptr;
        std::string sizeofName;
        if (auto udt = std::dynamic_pointer_cast<UserDefinedType>(so->measuredType)) sizeofName = udt->getName();
        else if (auto unres = std::dynamic_pointer_cast<UnresolvedType>(so->measuredType)) sizeofName = unres->getTypeString();
        if (!sizeofName.empty()) {
            std::vector<std::string> names = { sizeofName, canonicalTypeName(sizeofName) };
            const size_t dot = sizeofName.rfind('.');
            if (dot != std::string::npos) names.push_back(sizeofName.substr(dot + 1));
            for (const auto& n : names) {
                if (auto* t = activeScope->getType(n)) { ty = t; break; }
                if (auto* st = llvm::StructType::getTypeByName(*Context, n)) { ty = st; break; }
            }
        }
        if (!ty) ty = resolveLLVMType(so->measuredType);
        if (!ty || !ty->isSized()) {
            console.reportError(Console::TYPE_ERROR,
                "sizeof: cannot determine the size of '" +
                (so->measuredType ? so->measuredType->toString() : std::string("?")) + "'",
                so->getSpan());
            return nullptr;
        }
        llvm::Module* m = currentModule ? &*currentModule : &*Module;
        const uint64_t sz = m->getDataLayout().getTypeAllocSize(ty).getFixedValue();
        return llvm::ConstantInt::get(llvm::Type::getInt32Ty(*Context), sz);
    }

    // Handle VariableAssignment
    if (auto varAssign = std::dynamic_pointer_cast<VariableAssignment>(value)) {
        DEBUG_LOG("Assigning variable " + varAssign->variableName + " of type " + varAssign->getType()->toString());
        prepareType(varAssign->getType());   // emit a not-yet-declared struct type before it is used
        return assignVariable(varAssign, scope);
    }

    if (auto castExpr = std::dynamic_pointer_cast<CastExpression>(value)) {
        DEBUG_LOG("Generating cast from type " + castExpr->targetExpr->getType()->toString() + " to " + castExpr->type->toString());

        llvm::Value* src = codegen(castExpr->targetExpr, scope);
        if (!src) return nullptr;

        llvm::Type* destType = resolveLLVMType(castExpr->type);
        // generateCast reports "Unsupported cast" with no location. For the common bad case (a pointer
        // cast to a floating-point type) report it here, with the span, scope and expression.
        if (src->getType()->isPointerTy() && destType && destType->isFloatingPointTy()) {
            console.reportError(Console::TYPE_ERROR,
                "Unsupported cast from '" + debugType(src->getType()) + "' to '" + debugType(destType) +
                "' in '" + scope->getName() + "': " + castExpr->toString(),
                castExpr->getSpan());
            return nullptr;
        }
        // LLVM integers are signless: choose zext vs sext from the Omniscript source type,
        // otherwise a u32 widened to i64/u64 is sign-extended.
        if (src->getType()->isIntegerTy() && destType && destType->isIntegerTy()) {
            if (src->getType()->getIntegerBitWidth() == 1)
                return Builder->CreateZExtOrTrunc(src, destType, "bool.cast");
            const bool srcUns = isUnsignedOmniType(castExpr->targetExpr->getType());
            return Builder->CreateIntCast(src, destType, /*isSigned=*/!srcUns, "int.cast");
        }
        return generateCast(src, destType);
    }

    if (auto nullable = std::dynamic_pointer_cast<NullableExpression>(value)) {
        DEBUG_LOG("Generating nullable expression");

        if (nullable->isNull()) {
            DEBUG_LOG("Nullable expression has null value");
            // Return a 'null' representation
            // Assuming you use `{ i1, T }` style struct, set `is_null = true` and `value = undef`
            llvm::Type* innerType = resolveLLVMType(nullable->getType()); // Get full nullable type
            llvm::StructType* nullableType = llvm::StructType::get(*Context, {
                llvm::Type::getInt1Ty(*Context), // is_null
                innerType
            });

            llvm::Value* undefVal = llvm::UndefValue::get(innerType);
            llvm::Value* isNull = llvm::ConstantInt::getFalse(*Context);

            llvm::Value* result = llvm::UndefValue::get(nullableType);
            result = Builder->CreateInsertValue(result, isNull, {0});
            result = Builder->CreateInsertValue(result, undefVal, {1});
            return result;
        }

        // If value is not null
        // Generate the value
        llvm::Value* innerValue = codegen(nullable->inner, scope);

        llvm::Type* innerType = innerValue->getType();
        llvm::StructType* nullableType = llvm::StructType::get(*Context, {
            llvm::Type::getInt1Ty(*Context), // is_null
            innerType
        });

        llvm::Value* isNull = llvm::ConstantInt::getTrue(*Context);

        llvm::Value* result = llvm::UndefValue::get(nullableType);
        result = Builder->CreateInsertValue(result, isNull, {0});
        result = Builder->CreateInsertValue(result, innerValue, {1});

        return result;
    }

    // Handle ReferenceValue
    // Address of a variable's storage. For a Nullable<T> variable ({ i1 hasValue, T value }) the
    // address handed out is that of the wrapped T value (field 1), so `r.method()` on a `Stats?`
    // passes a real Stats* as `this` instead of a pointer to the nullable wrapper.
    auto refToVariableValue = [&](const std::string& varName) -> llvm::Value* {
        llvm::Value* ref = getReferenceToVariable(varName);
        if (!ref) return nullptr;
        llvm::Type* storage = nullptr;
        if (auto* alloca = llvm::dyn_cast<llvm::AllocaInst>(ref)) storage = alloca->getAllocatedType();
        else if (auto* global = llvm::dyn_cast<llvm::GlobalVariable>(ref)) storage = global->getValueType();
        if (storage && looksNullable(storage)) {
            DEBUG_LOG("Reference to nullable variable '" + varName + "': using the address of its value field");
            return Builder->CreateStructGEP(storage, ref, 1, varName + ".value");
        }
        return ref;
    };

    if (auto refValue = std::dynamic_pointer_cast<ReferenceExpression>(value)) {
        // Reference to an lvalue expression with no variable name (e.g. 'this.position'):
        // compute the address of the expression itself instead of looking up a name.
        // An index/member expression is always addressed through its own element/field, even when
        // the front end also recorded the base variable's name (`&sa[4]` must not decay to `&sa`).
        const bool referentIsElementOrField = refValue->referent &&
            (std::dynamic_pointer_cast<IndexAccessExpression>(refValue->referent) ||
             std::dynamic_pointer_cast<MemberAccessExpression>(refValue->referent));
        if (refValue->referent && (refValue->referentName.empty() || referentIsElementOrField)) {
            DEBUG_LOG("Creating reference to expression " + refValue->referent->toString());

            std::function<llvm::Value*(const std::shared_ptr<Expression>&)> addressOf;
            addressOf = [&](const std::shared_ptr<Expression>& e) -> llvm::Value* {
                if (auto var = std::dynamic_pointer_cast<VariableAccessExpression>(e)) {
                    // The scope holds the variable's storage (alloca / global / direct argument)
                    return refToVariableValue(var->variableName);
                }

                if (auto member = std::dynamic_pointer_cast<MemberAccessExpression>(e)) {
                    if (member->index < 0 || !member->expr) {
                        console.reportError(Console::SEMANTIC_ERROR, "Cannot take the address of member '" + member->member + "': unresolved member index", FileSpan{});
                        return nullptr;
                    }

                    auto baseOmniType = member->expr->getType();
                    if (!baseOmniType) {
                        console.reportError(Console::TYPE_ERROR, "Cannot take the address of member '" + member->member + "': base has no type", FileSpan{});
                        return nullptr;
                    }

                    llvm::Value* baseAddr = addressOf(member->expr);
                    if (!baseAddr) return nullptr;

                    std::shared_ptr<Type> structOmniType = baseOmniType;
                    if (baseOmniType->isPointer()) {
                        structOmniType = baseOmniType->getPointeeType();
                        // A pointer-typed base: 'baseAddr' is either the pointer itself (a direct
                        // function argument such as 'this') or the storage holding the pointer.
                        if (!llvm::isa<llvm::Argument>(baseAddr)) {
                            baseAddr = Builder->CreateLoad(
                                llvm::PointerType::get(*Context, 0), baseAddr, "ref.base");
                        }
                    }

                    auto* structType = llvm::dyn_cast_or_null<llvm::StructType>(resolveLLVMType(structOmniType));
                    if (!structType) {
                        console.reportError(Console::TYPE_ERROR, "Cannot take the address of member '" + member->member + "': base is not a struct", FileSpan{});
                        return nullptr;
                    }
                    if ((unsigned)member->index >= structType->getNumElements()) {
                        console.reportError(Console::SEMANTIC_ERROR, "Member index " + std::to_string(member->index) + " out of range for '" +
                                      member->member + "'", FileSpan{});
                        return nullptr;
                    }
                    return Builder->CreateStructGEP(structType, baseAddr, (unsigned)member->index,
                                                    "ref." + member->member);
                }

                if (auto idx = std::dynamic_pointer_cast<IndexAccessExpression>(e)) {
                    // &base[i]: GEP to the element, no load
                    if (!idx->expr || !idx->indexExpr || !idx->getType()) {
                        console.reportError(Console::SEMANTIC_ERROR, "Cannot take the address of an incomplete index expression", FileSpan{});
                        return nullptr;
                    }
                    // Fixed array (local, global or struct field): index into the array's storage,
                    // [N x T]* with indices {0, i}. codegen() of the base would yield its value
                    // (or the array itself), never an element pointer.
                    if (auto arrType = std::dynamic_pointer_cast<FixedArrayType>(idx->expr->getType())) {
                        llvm::Value* arrAddr = addressOf(idx->expr);
                        llvm::Value* arrIndex = codegen(idx->indexExpr, scope);
                        if (!arrAddr || !arrIndex) return nullptr;
                        llvm::Type* arrTy = resolveLLVMType(arrType);
                        llvm::Value* i64Index = Builder->CreateIntCast(
                            arrIndex, llvm::Type::getInt64Ty(*Context), true, "ref.idx");
                        return Builder->CreateInBoundsGEP(
                            arrTy, arrAddr,
                            {llvm::ConstantInt::get(llvm::Type::getInt64Ty(*Context), 0), i64Index},
                            "ref.elem");
                    }

                    llvm::Value* baseVal = codegen(idx->expr, scope);
                    llvm::Value* indexVal = codegen(idx->indexExpr, scope);
                    if (!baseVal || !indexVal) return nullptr;
                    llvm::Type* elemTy = resolveLLVMType(idx->getType());
                    return Builder->CreateGEP(elemTy, baseVal, indexVal, "ref.index");
                }

                // Anything else is an rvalue: a call result (`a.sub(b)` in `a.sub(b).normalize()`), a struct
                // literal, a cast, ... It has no storage of its own, so evaluate it once into a temporary stack
                // slot and hand out that slot's address. This is what lets a method be called on a
                // temporary (`this` = &temp) and lets a member be read from one (`f().x`).
                llvm::Value* rvalue = codegen(e, scope);
                if (!rvalue) {
                    console.reportError(Console::SEMANTIC_ERROR, "Cannot take a reference to expression: " + e->toString(), FileSpan{});
                    return nullptr;
                }
                if (rvalue->getType()->isVoidTy()) {
                    console.reportError(Console::SEMANTIC_ERROR, "Cannot take a reference to a void expression: " + e->toString(), FileSpan{});
                    return nullptr;
                }
                // Aggregates (struct/array values that are not pointers in the language) can already evaluate
                // to their address, e.g. `*p` or an object literal materialised in a local: use it as is.
                auto rvalueOmniType = e->getType();
                if (rvalueOmniType && !rvalueOmniType->isPointer() && rvalue->getType()->isPointerTy()) {
                    return rvalue;
                }
                // Entry-block alloca, like the by-value parameter spill below: one slot per call site no matter
                // how many times a loop runs it, and it stays valid until the function returns.
                llvm::Function* fn = Builder->GetInsertBlock()->getParent();
                llvm::IRBuilder<> entry(&fn->getEntryBlock(), fn->getEntryBlock().begin());
                llvm::AllocaInst* temp = entry.CreateAlloca(rvalue->getType(), nullptr, "rvalue.tmp");
                Builder->CreateStore(rvalue, temp);
                DEBUG_LOG("Spilled rvalue '" + e->toString() + "' to a temporary to take its address");
                return temp;
            };

            return addressOf(refValue->referent);
        }

        DEBUG_LOG("Creating reference to variable " + refValue->referentName);
        llvm::Value* varRef = refToVariableValue(refValue->referentName);

        // A by-value parameter (`f64(v: double)`) is an SSA argument with no storage, so `&v`
        // has nothing to point at (it produced a double that was then cast to ptr). Spill it into
        // a stack slot in the entry block and hand out that address. Reads of `v` keep using the
        // argument, so writes made through the returned pointer are not seen by later reads.
        if (auto* arg = llvm::dyn_cast_or_null<llvm::Argument>(varRef)) {
            if (!arg->getType()->isPointerTy()) {
                llvm::Function* fn = Builder->GetInsertBlock()->getParent();
                llvm::IRBuilder<> entry(&fn->getEntryBlock(), fn->getEntryBlock().begin());
                llvm::AllocaInst* slot = entry.CreateAlloca(arg->getType(), nullptr, refValue->referentName + ".addr");
                Builder->CreateStore(arg, slot);
                DEBUG_LOG("Spilled by-value parameter '" + refValue->referentName + "' to take its address");
                return slot;
            }
        }
        return varRef;
    }
    
    if (auto addressOf = std::dynamic_pointer_cast<AddressOfExpression>(value)) {
        DEBUG_LOG("Getting the address of variable " + addressOf->variableName);
        return getAddressOf(addressOf->variableName);
    }

    if (auto null = std::dynamic_pointer_cast<NullExpression>(value)) {
        DEBUG_LOG("Creating a null value");
        auto innerType = resolveLLVMType(null->getType());
        return createNullValue(innerType);
    }

    else if (auto rawPtr = std::dynamic_pointer_cast<RawPointerExpression>(value)) {
        DEBUG_LOG("Creating raw pointer from address: " + std::to_string(rawPtr->address));
        
        // Get the LLVM type for the pointee
        llvm::Type* pointeeType = resolveLLVMType(rawPtr->getType()->getPointeeType());
        
        // Handle null pointer case
        if (rawPtr->address == 0) {
            return llvm::ConstantPointerNull::get(
                llvm::PointerType::get(*Context, 0)
            );
        }
        
        // Create the raw pointer
        return createRawPointer(rawPtr->address, pointeeType);
    }
    
    if (auto block = std::dynamic_pointer_cast<BlockExpression>(value)) {
        DEBUG_LOG("Evaluating a block value — First pass (registration)");

        struct BlockGuard { std::function<void()> onExit; ~BlockGuard() { if (onExit) onExit(); } };
        ++blockDepth;
        BlockGuard blockGuard{[&]() { if (--blockDepth == 0) flushDeferredBodies(); }};

        collectStructs(block);   // before emitting: instances may need structs declared later in this block
        emitAllInstances();   // instances created while the program was expressed

        for (const auto& expr : block->values) {
            if (auto structExpr = std::dynamic_pointer_cast<StructExpression>(expr)) {
                codegen(expr, scope);
            } else if (auto func = std::dynamic_pointer_cast<FunctionExpression>(expr)) {
                DEBUG_LOG("Processing function declaration: " + func->name + " (mangled: " + func->mangledName + ")");
                prepareSignature(func->returnType, func->parameters);
                llvm::Type* returnType = resolveLLVMType(func->returnType);

                if (func->isExtern) {
                    createExternFunction(func, scope);
                } else if (func->isIntrinsic) {
                    std::string nameWithoutPrefix = func->intrinsicName;
                    const std::string prefix = "intrinsic_";
                    if (nameWithoutPrefix.rfind(prefix, 0) == 0)
                        nameWithoutPrefix = nameWithoutPrefix.substr(prefix.size());

                    std::vector<llvm::Type*> atomicParamTys;
                    for (const auto& p : func->parameters) atomicParamTys.push_back(resolveLLVMType(p->getType()));
                    llvm::Module* atomicMod = currentModule ? &*currentModule : &*Module;
                    if (auto atomicFn = defineAtomicIntrinsic(atomicMod, func->mangledName, func->intrinsicName, returnType, atomicParamTys)) {
                        activeScope->set(func->mangledName, atomicFn);   // same registration createIntrinsicFunction does
                    } else {
                        createIntrinsicFunction(
                            func->mangledName,
                            "llvm." + nameWithoutPrefix,
                            returnType
                        );
                    }
                } else {
                    registerFunction(
                        func->mangledName,
                        returnType,
                        func->parameters,
                        scope,
                        func->isVarArg
                    );
                    userDefinedFunctions.push_back(func);
                }
            }
        }

        for (const auto& expr : block->values) {
            if (std::dynamic_pointer_cast<StructExpression>(expr)) {
                continue;   // structs were emitted in the first pass
            }

            if (std::dynamic_pointer_cast<FunctionExpression>(expr)) {
                continue;
            }

            if (auto ret = std::dynamic_pointer_cast<ReturnExpression>(expr)) {
                if (ret->getType()) {
                    if (!ret->getType()->isVoidLike()) {
                        return codegen(expr, scope);
                    }
                } else {
                    console.reportError(Console::TYPE_ERROR, "The return type has no type", FileSpan{});
                }
            } 

            if (auto varAssign = std::dynamic_pointer_cast<VariableAssignment>(expr)) {
                if (!varAssign->isStatic) {
                    varAssign->isGlobal = block->isGlobal;
                }
            }

            codegen(expr, scope);
        }

        return nullptr;
    }

    if (auto nullpointer = std::dynamic_pointer_cast<NullPointerExpression>(value)) {
        DEBUG_LOG("Creating a null pointer of type " + nullpointer->getType()->toString());
        DEBUG_LOG("Creating a null pointer of root type " + nullpointer->getRootType()->toString());
        auto pointeeType = resolveLLVMType(nullpointer->getRootType()->getPointeeType());
        return createNullPointer(pointeeType);
    }

    if (auto func = std::dynamic_pointer_cast<FunctionExpression>(value)) {
        DEBUG_LOG("Creating an overload for function " + func->name + " with mangled name '" + func->mangledName + "'");
        prepareSignature(func->returnType, func->parameters);
        llvm::Type* returnType = resolveLLVMType(func->returnType);
        if (func->isExtern) {
            return createExternFunction(func, scope);
        } else if (func->isIntrinsic) {
            std::string nameWithoutPrefix = func->intrinsicName;
            const std::string prefix = "intrinsic_";

            // Strip 'intrinsic_' prefix if present
            if (nameWithoutPrefix.rfind(prefix, 0) == 0) {
                nameWithoutPrefix = nameWithoutPrefix.substr(prefix.size());
            }

            std::vector<llvm::Type*> atomicParamTys;
            for (const auto& p : func->parameters) atomicParamTys.push_back(resolveLLVMType(p->getType()));
            llvm::Module* atomicMod = currentModule ? &*currentModule : &*Module;
            if (auto atomicFn = defineAtomicIntrinsic(atomicMod, func->mangledName, func->intrinsicName, returnType, atomicParamTys)) {
                activeScope->set(func->mangledName, atomicFn);   // same registration createIntrinsicFunction does
                return atomicFn;
            }

            return createIntrinsicFunction(
                func->mangledName,
                "llvm." + nameWithoutPrefix,
                returnType
            );
        }

        if (configs.mode == CompileMode::JIT) {
            if (func->mangledName == "main") {
                func->mangledName = "__main";
            }
        }

        registerFunction(
            func->mangledName,
            returnType,
            func->parameters,
            scope,
            func->isVarArg
        );
        userDefinedFunctions.push_back(func);
    }

    if (auto ret = std::dynamic_pointer_cast<ReturnExpression>(value)) {
        DEBUG_LOG("Creating a return statement of kind '" + ret->getType()->toString() + "'.");

        llvm::Type* type = resolveLLVMType(ret->getType());
        if (!ret->value) {
            return createReturn(nullptr, nullptr);
        }
        llvm::Value* val = codegen(ret->value, scope);
        // Aggregates evaluate to their address (arr[i], this.field, a local struct): a by-value
        // return of a struct/array needs the value itself.
        if (val && type && val->getType()->isPointerTy() && (type->isStructTy() || type->isArrayTy())) {
            val = Builder->CreateLoad(type, val, "ret.load");
        }
        return createReturn(val, type);
    }

    if (auto unary = std::dynamic_pointer_cast<UnaryExpression>(value)) {
        DEBUG_LOG("Creating a unary expression");
        llvm::Value* operandVal = codegen(unary->operand, scope);
        if (!operandVal) {
            console.reportError(Console::SEMANTIC_ERROR, "The operand value is invalid", FileSpan{});
        }
        return createUnaryExpression(operandVal, unary->op.getType(), unary->isPrefix);
    }

    if (auto binary = std::dynamic_pointer_cast<BinaryExpression>(value)) {
        DEBUG_LOG("Creating a binary expression: " + binary->left->toString() + " " + binary->op.getValue() + " " + binary->right->toString());
        // `&&` / `||` must short-circuit: the right operand is evaluated only in its own block, so
        // `a != nullptr && a.f()` and `i < n && p[i] == x` never run the right side when the left decides.
        {
            const TokenTypes logicalOp = binary->op.getType();
            if (logicalOp == TokenTypes::LogicalAnd || logicalOp == TokenTypes::LogicalOr) {
                auto toBool = [&](llvm::Value* v, const char* name) -> llvm::Value* {
                    if (v->getType()->isPointerTy())
                        return Builder->CreateICmpNE(v, llvm::ConstantPointerNull::get(llvm::cast<llvm::PointerType>(v->getType())), name);
                    if (v->getType()->isIntegerTy())
                        return Builder->CreateICmpNE(v, llvm::ConstantInt::get(v->getType(), 0), name);
                    return nullptr;
                };
                llvm::Value* l = codegen(binary->left, scope);
                if (!l) {
                    console.reportError(Console::SEMANTIC_ERROR, "The lhs value is invalid", FileSpan{});
                    return nullptr;
                }
                llvm::Value* lcond = toBool(l, "lhscond");
                if (!lcond) {
                    console.reportError(Console::TYPE_ERROR, "Left operand of '" + binary->op.getValue() + "' is not boolean-like (got '" + debugType(l->getType()) + "')", FileSpan{});
                    return nullptr;
                }
                llvm::Function* scFn = Builder->GetInsertBlock()->getParent();
                llvm::BasicBlock* lhsEnd = Builder->GetInsertBlock();
                llvm::BasicBlock* rhsBlock = llvm::BasicBlock::Create(*Context, "sc.rhs", scFn);
                llvm::BasicBlock* mergeBlock = llvm::BasicBlock::Create(*Context, "sc.merge", scFn);
                if (logicalOp == TokenTypes::LogicalAnd) Builder->CreateCondBr(lcond, rhsBlock, mergeBlock);
                else Builder->CreateCondBr(lcond, mergeBlock, rhsBlock);

                Builder->SetInsertPoint(rhsBlock);
                llvm::Value* r = codegen(binary->right, scope);
                if (!r) {
                    console.reportError(Console::SEMANTIC_ERROR, "The rhs value is invalid", FileSpan{});
                    return nullptr;
                }
                llvm::Value* rcond = toBool(r, "rhscond");
                if (!rcond) {
                    console.reportError(Console::TYPE_ERROR, "Right operand of '" + binary->op.getValue() + "' is not boolean-like (got '" + debugType(r->getType()) + "')", FileSpan{});
                    return nullptr;
                }
                llvm::BasicBlock* rhsEnd = Builder->GetInsertBlock();   // evaluating the right side may have opened blocks
                Builder->CreateBr(mergeBlock);

                Builder->SetInsertPoint(mergeBlock);
                llvm::PHINode* phi = Builder->CreatePHI(llvm::Type::getInt1Ty(*Context), 2, logicalOp == TokenTypes::LogicalAnd ? "landtmp" : "lortmp");
                phi->addIncoming(logicalOp == TokenTypes::LogicalAnd ? llvm::ConstantInt::getFalse(*Context) : llvm::ConstantInt::getTrue(*Context), lhsEnd);
                phi->addIncoming(rcond, rhsEnd);
                return phi;
            }
        }

        llvm::Value* lhs = codegen(binary->left, scope);
        llvm::Value* rhs = codegen(binary->right, scope);
        if (!lhs) {
            console.reportError(Console::SEMANTIC_ERROR, "The lhs value is invalid", FileSpan{});
            return nullptr;
        }
        if (!rhs) {
            console.reportError(Console::SEMANTIC_ERROR, "The rhs value is invalid", FileSpan{});
            return nullptr;
        }
        DEBUG_LOG("lhs type: '" + debugType(lhs->getType()) + "' rhs type: '" + debugType(rhs->getType()) + "'.");

        // `nullable == null` / `nullable != null` / `null == nullable`: decide this from the AST, where a
        // null literal is unambiguous, rather than guessing from the LLVM value. Nullable layout is
        // { i1 hasValue, T value } (the null literal above is built with flag = false).
        if (binary->op.getType() == TokenTypes::Equals || binary->op.getType() == TokenTypes::NotEquals) {
            auto isNullAst = [](const std::shared_ptr<Expression>& e) {
                auto n = std::dynamic_pointer_cast<NullableExpression>(e);
                return n && n->isNull();
            };
            bool leftIsNull = isNullAst(binary->left);
            bool rightIsNull = isNullAst(binary->right);
            DEBUG_LOG(std::string("null-compare check: leftIsNull=") + (leftIsNull ? "1" : "0") +
                      " rightIsNull=" + (rightIsNull ? "1" : "0") +
                      " lhsNullable=" + (looksNullable(lhs->getType()) ? "1" : "0") +
                      " rhsNullable=" + (looksNullable(rhs->getType()) ? "1" : "0"));
            if (leftIsNull != rightIsNull) {
                llvm::Value* nullableVal = rightIsNull ? lhs : rhs;
                if (looksNullable(nullableVal->getType())) {
                    llvm::Value* hasValue = Builder->CreateExtractValue(nullableVal, {0}, "hasValue");
                    return binary->op.getType() == TokenTypes::Equals
                        ? Builder->CreateNot(hasValue, "isnull")
                        : hasValue;
                }
            }
        }

        // Pointer arithmetic is decided here, from the Omniscript types: the LLVM values are opaque
        // `ptr`s and cannot tell `char* + int` (valid) from `char* + char*` (a type error).
        {
            const TokenTypes bop = binary->op.getType();
            const bool isArith = bop == TokenTypes::Plus || bop == TokenTypes::Minus ||
                                 bop == TokenTypes::Multiply || bop == TokenTypes::Divide ||
                                 bop == TokenTypes::Modulo;
            auto lt = binary->left->getType();
            auto rt = binary->right->getType();
            const bool lPtr = lt && lt->isPointer();
            const bool rPtr = rt && rt->isPointer();
            if (isArith && (lPtr || rPtr)) {
                auto isInt = [](llvm::Value* v) {
                    return v->getType()->isIntegerTy() && v->getType()->getIntegerBitWidth() > 1;
                };
                auto elemTypeOf = [&](const std::shared_ptr<Type>& pt) -> llvm::Type* {
                    llvm::Type* e = resolveLLVMType(pt->getPointeeType());
                    // void* / unresolved pointees step by bytes
                    return (!e || e->isVoidTy()) ? llvm::Type::getInt8Ty(*Context) : e;
                };
                if (bop == TokenTypes::Plus && lPtr && !rPtr && isInt(rhs)) {
                    return Builder->CreateGEP(elemTypeOf(lt), lhs, rhs, "ptradd");
                }
                if (bop == TokenTypes::Plus && rPtr && !lPtr && isInt(lhs)) {
                    return Builder->CreateGEP(elemTypeOf(rt), rhs, lhs, "ptradd");
                }
                if (bop == TokenTypes::Minus && lPtr && !rPtr && isInt(rhs)) {
                    return Builder->CreateGEP(elemTypeOf(lt), lhs, Builder->CreateNeg(rhs, "ptroff"), "ptrsub");
                }
                if (bop == TokenTypes::Minus && lPtr && rPtr && lt->toString() == rt->toString()) {
                    llvm::Module* m = currentModule ? &*currentModule : &*Module;
                    llvm::Type* i64 = llvm::Type::getInt64Ty(*Context);
                    llvm::Type* elem = elemTypeOf(lt);
                    uint64_t sz = m->getDataLayout().getTypeAllocSize(elem).getFixedValue();
                    if (sz == 0) sz = 1;
                    llvm::Value* diff = Builder->CreateSub(Builder->CreatePtrToInt(lhs, i64), Builder->CreatePtrToInt(rhs, i64), "ptrdiff");
                    return Builder->CreateSDiv(diff, llvm::ConstantInt::get(i64, sz), "ptrdiff.elems");
                }
                console.reportError(Console::TYPE_ERROR,
                    "Operator '" + binary->op.getValue() + "' is not defined for '" + (lt ? lt->toString() : std::string("?")) +
                    "' and '" + (rt ? rt->toString() : std::string("?")) +
                    "'. Pointers support: pointer +/- integer, and pointer - pointer of the same type.",
                    binary->getSpan());
                return nullptr;
            }
        }

        bool uns = isUnsignedOmniType(binary->left->getType()) || isUnsignedOmniType(binary->right->getType());
        return createBinaryExpression(lhs, binary->op.getType(), rhs, uns);
    }

    if (auto ternary = std::dynamic_pointer_cast<TernaryExpression>(value)) {
        DEBUG_LOG("Creating a ternary expression");
        llvm::Value* cond = codegen(ternary->condition, scope);
        llvm::Value* truthy = codegen(ternary->truthy, scope);
        llvm::Value* falsey = codegen(ternary->falsey, scope);
        if (!cond || !truthy || !falsey) return nullptr;
        return createTernaryExpression(cond, truthy, falsey);
    }

    if (auto var = std::dynamic_pointer_cast<VariableAccessExpression>(value)) {
        DEBUG_LOG("Accessing variable: " + var->variableName);
        
        if (var->extractValue) {
            DEBUG_LOG("Extracting the value of a nullable");
        }

        // Module-level consts are registered under their qualified name (e.g. "Math.INF"), but the
        // front end can leave the short name ("INF") on accesses made inside the module.
        // If the short name is not in scope, retry qualified by the enclosing module, taken
        // from the scope name ("Math.isInf(Double)" -> "Math").
        std::string accessName = var->variableName;
        if (!activeScope->lookup(accessName).found()) {
            std::string scopeName = scope->getName();
            scopeName = scopeName.substr(0, scopeName.find('('));
            while (true) {
                size_t dot = scopeName.rfind('.');
                if (dot == std::string::npos) break;
                scopeName = scopeName.substr(0, dot);
                std::string candidate = scopeName + "." + var->variableName;
                if (activeScope->lookup(candidate).found()) {
                    DEBUG_LOG("Resolved '" + var->variableName + "' as '" + candidate + "'");
                    accessName = candidate;
                    break;
                }
            }
        }

        return getVariable(accessName, false);
    }

    if (auto instance = std::dynamic_pointer_cast<InstanceExpression>(value)) {
        DEBUG_LOG("Creating an instance of '" + instance->baseName + "'.");

        // Check if we already generated this instance
        if (!instance->instanceName.empty()) {
            // Try to get existing LLVM value if it exists
            auto existing = activeScope->lookup(instance->instanceName);
            if (existing.found() && existing.value) {
                return existing.value;
            }
        }
        
        std::vector<llvm::Value*> args;
        args.reserve(instance->members.size()); // Pre-reserve to avoid reallocations

        for (const auto& arg : instance->members) {
            DEBUG_LOG(arg->toString());

            if (auto member = std::dynamic_pointer_cast<MemberExpression>(arg)) {
                if (auto value = codegen(member->value, scope)) {
                    args.push_back(value);
                }
            } else {
                console.reportError(Console::TYPE_ERROR, formatError("Failed to generate code for constructor argument: " + arg->toString()), FileSpan{});
            }
            
        }

        DEBUG_LOG("Creating an object instance");
        auto result = createObjectInstance(instance->baseName, instance->instanceName, args, false);
        
        // Store the result if it has a name
        if (!instance->instanceName.empty()) {
            activeScope->setConstant(instance->instanceName, result);
        }
        
        return result;
    }

    if (auto call = std::dynamic_pointer_cast<CallExpression>(value)) {
        DEBUG_LOG("Calling " + call->calleeName);

        // NEW: call through an expression (function-pointer value).
        if (call->calleeExpr) {
            auto calleeVal = codegen(call->calleeExpr, scope);
            if (!calleeVal) return nullptr;

            std::vector<llvm::Value*> args;
            args.reserve(call->args.size());
            for (const auto& arg : call->args) {
                if (auto v = codegen(arg, scope)) args.push_back(v);
                else return nullptr;
            }

            llvm::Type* llvmFnTy = resolveLLVMType(call->calleeFnType);
            if (!llvmFnTy || !llvmFnTy->isFunctionTy()) {
                console.reportError(Console::TYPE_ERROR,
                    "Cannot resolve function type for expression callee", FileSpan{});
                return nullptr;
            }
            auto* fnTy = llvm::cast<llvm::FunctionType>(llvmFnTy);
            return Builder->CreateCall(fnTy, calleeVal, args);
        }
        
        std::vector<llvm::Value*> args;
        args.reserve(call->args.size()); // Pre-reserve to avoid reallocations

        for (const auto& arg : call->args) {
            DEBUG_LOG(arg->toString());

            if (auto arr = std::dynamic_pointer_cast<ArrayExpression>(arg); arr && arr->isVariadicArray) {
                // If variadic, reserve space ahead (optional perf tweak)
                args.reserve(args.size() + arr->elements.size());

                for (const auto& element : arr->elements) {
                    if (auto value = codegen(element, scope)) {
                        args.push_back(value);
                    } else {
                        console.reportError(Console::SEMANTIC_ERROR, formatError("Failed to generate code for variadic argument element."), FileSpan{});
                    }
                }
            } else {
                if (auto value = codegen(arg, scope)) {
                    args.push_back(value);
                } else {
                    console.reportError(Console::SEMANTIC_ERROR, formatError("Failed to generate code for argument: " + arg->toString()), FileSpan{});
                }
            }
        }


        if (call->instanceName.empty()) {
            DEBUG_LOG("Creating a normal call for " + call->calleeName);
            llvm::Value* callResult = createCall(call->calleeName, args, call->functionTypeName);
            // createCall reports argument mismatches without a location; name the call that failed.
            if (!callResult && call->getType() && !call->getType()->isVoidLike()) {
                console.reportError(Console::SEMANTIC_ERROR,
                    "Call to '" + call->calleeName + "' failed in '" + scope->getName() + "': " + call->toString(),
                    call->getSpan());
            }
            return callResult;
        }
        DEBUG_LOG("Creating an object instance");
        return createObjectInstance(call->calleeName, call->instanceName, args, call->isGlobal);
    }

    if (auto structExpr = std::dynamic_pointer_cast<StructExpression>(value)) {
        // Emit each struct once (it may already have been pulled forward by a dependent).
        if (!startedStructs.insert(structExpr->name).second) return nullptr;

        std::vector<llvm::Type*> fieldTypes;
        fieldTypes.reserve(structExpr->parameters.size());
        
        int methodsCount = structExpr->parameters.size();

        for (const auto& field : structExpr->parameters) {
            if (auto input = std::dynamic_pointer_cast<FunctionInputExpression>(field)) {
                prepareType(input->getType());
                // By-value field of a user type that is declared later in the program: emit it now.
                // Bodies are deferred (emitDepth) until every type and function is registered.
                std::function<void(const std::shared_ptr<Type>&)> ensureDeclared =
                    [&](const std::shared_ptr<Type>& t) {
                        if (!t) return;
                        if (auto udt = std::dynamic_pointer_cast<UserDefinedType>(t)) {
                            if (activeScope->getType(udt->getName())) return;
                            auto decl = findStructDecl(udt->getName());
                            DEBUG_LOG("[ensureDeclared] '" + udt->getName() + "' needed by '" + structExpr->name + "': decl " + std::string(decl ? "found" : "NOT FOUND") + " (known=" + std::to_string(structDecls.size()) + ")");
                            if (decl && !startedStructs.count(decl->name)) {
                                ++emitDepth;
                                codegen(decl, scope);
                                --emitDepth;
                            }
                        } else if (auto nl = std::dynamic_pointer_cast<NullableType>(t)) {
                            ensureDeclared(nl->innerType);
                        } else if (auto fa = std::dynamic_pointer_cast<FixedArrayType>(t)) {
                            ensureDeclared(fa->elementType);
                        }
                    };
                ensureDeclared(input->getType());
                llvm::Type* llvmFieldType = resolveLLVMType(input->getType());
                if (!llvmFieldType) {
                    console.reportError(Console::TYPE_ERROR, "Failed to generate type for field '" + input->name + "' in struct '" + structExpr->name + "'.", FileSpan{});
                    return nullptr;
                }
        
                // A function-typed field (e.g. `transform: fn(T) => U`) is stored as a pointer:
                // llvm::FunctionType is unsized and is not a valid struct element type.
                if (llvmFieldType->isFunctionTy()) {
                    llvmFieldType = llvm::PointerType::get(*Context, 0);
                }

                fieldTypes.push_back(llvmFieldType);
                methodsCount--;
            }
        }
        
        // Create the LLVM struct type (opaque or packed depending on your system)
        createStructType(structExpr->name, fieldTypes);

        // Instances nested in a module/struct are named qualified ("Iter.MapIter_i32_i32") but
        // referenced by their short name ("MapIter_i32_i32") in signatures and fields.
        // Register the short name too (never overriding an existing type of that name).
        {
            size_t dot = structExpr->name.rfind('.');
            if (dot != std::string::npos) {
                const std::string shortName = structExpr->name.substr(dot + 1);
                if (auto* st = llvm::StructType::getTypeByName(*Context, structExpr->name)) {
                    if (!activeScope->getType(shortName)) {
                        DEBUG_LOG("[struct] aliasing '" + shortName + "' -> '" + structExpr->name + "'");
                        activeScope->addType(shortName, st);
                    }
                }
            }
        }

        if (methodsCount > 0) {
            std::vector<llvm::Function*> methods;
    
            methods.reserve(methodsCount);
    
            for (const auto& field : structExpr->parameters) {
                if (auto method = std::dynamic_pointer_cast<FunctionExpression>(field)) {
                    prepareSignature(method->returnType, method->parameters);
                    llvm::Type* returnType = resolveLLVMType(method->returnType);
                    llvm::Function* methd = registerFunction(method->mangledName, returnType, method->parameters, scope);
                    methods.emplace_back(methd);
                }
            }
            
            int methodIndex = 0;
            for (const auto& field : structExpr->parameters) {
                if (auto method = std::dynamic_pointer_cast<FunctionExpression>(field)) {
                    auto genBody = [this, method, fnPtr = methods[methodIndex], scope]() {
                        generateFunctionBody(method->mangledName, fnPtr, method->parameters, method->body, scope);
                    };
                    if (emitDepth > 0) deferredBodies.push_back(genBody);   // generic instance: defer
                    else genBody();
                    methodIndex++;
                }
        }
        }

        return nullptr;
    }

    if (auto classExpr = std::dynamic_pointer_cast<ClassExpression>(value)) {
        return codegen(classExpr->structExpr, scope);
    }

    if (auto ifExpr = std::dynamic_pointer_cast<IfExpression>(value)) {
        DEBUG_LOG("Creating an if expression");
        return createIfStatement(ifExpr->conditions, ifExpr->bodies, ifExpr->elseBody, scope);
    }

    if (auto enumExpr = std::dynamic_pointer_cast<EnumExpression>(value)) {
        DEBUG_LOG("Processing EnumExpression for enum '" + enumExpr->enumName + "'");
    
        std::vector<std::string> names;
        std::vector<llvm::Value*> values;
    
        // Reserve space to avoid repeated allocations
        names.reserve(enumExpr->expressionMap.size());
        values.reserve(enumExpr->expressionMap.size());
    
        for (const auto& [name, val] : enumExpr->expressionMap) {
            llvm::Value* enumValue = codegen(val, scope); // Generate LLVM IR for each enum entry
    
            if (enumValue) {
                names.emplace_back(name);
                values.emplace_back(enumValue);
                DEBUG_LOG("Enum '" + enumExpr->enumName + "' has enumerator '" + name + "' with value " + val->toString());
            } else {
                DEBUG_LOG("Failed to generate IR for enumerator '" + name + "'");
            }
        }
    
        // Call the appropriate method depending on flags
        if (enumExpr->isEnumClass && enumExpr->hasLookup) {
            return createEnumClassWithLookup(names, values, enumExpr->enumName, /*isGlobal=*/true);
        } else if (enumExpr->isEnumClass) {
            return createEnumClass(names, values, enumExpr->enumName, /*isGlobal=*/true);
        } else if (enumExpr->hasLookup) {
            return createEnumWithLookup(names, values, enumExpr->enumName, /*isGlobal=*/true);
        } else {
            return createEnum(names, values, enumExpr->enumName, /*isGlobal=*/true);
        }
    }
    
    if (std::dynamic_pointer_cast<StringTemplateExpression>(value)) {
        return createStringTemplate(value, scope);
    }

    if (auto newExpr = std::dynamic_pointer_cast<NewExpression>(value)) {
        return createNew(newExpr, scope);
    }
    if (auto delExpr = std::dynamic_pointer_cast<DeleteExpression>(value)) {   // already there
        return createDelete(delExpr, scope);
    }

    if (auto deferExpr = std::dynamic_pointer_cast<DeferExpression>(value)) {
        DEBUG_LOG("Registering a deferred action");
        registerDefer(deferExpr->action, scope);   // emitted at scope exit, not here
        return nullptr;
    }

    if (std::dynamic_pointer_cast<BreakExpression>(value)) {
        return createLoopJump(true, value->getSpan());
    }

    if (std::dynamic_pointer_cast<ContinueExpression>(value)) {
        return createLoopJump(false, value->getSpan());
    }

    if (auto forExpr = std::dynamic_pointer_cast<ForLoopExpression>(value)) {
        DEBUG_LOG("Processing ForLoopExpression");
        return createForLoop(forExpr, scope);
    }

    if (auto whileExpr = std::dynamic_pointer_cast<WhileLoopExpression>(value)) {
        DEBUG_LOG("Processing WhileLoopExpression");
        return createWhileLoop(whileExpr, scope);
    }

    // Handle access expressions recursively
    if (auto accessExpr = std::dynamic_pointer_cast<AccessExpression>(value)) {
        return handleAccessExpression(accessExpr, scope);
    }

    if (auto moduleExpr = std::dynamic_pointer_cast<ModuleExpression>(value)) {
        DEBUG_LOG("Processing ModuleExpression: " + moduleExpr->name);
    
        // Create a new scope for the module
        auto moduleScope = std::make_shared<SymbolTable<std::shared_ptr<Expression>, std::shared_ptr<Type>>>(scope);
    
        // Store generated IR values for module members
        std::unordered_map<std::string, llvm::Value*> memberIRValues;
    
        for (const auto& member : moduleExpr->members) {
            std::string memberName = member->name;
            DEBUG_LOG("Generating IR for module member: " + memberName);
    
            llvm::Value* memberValue = codegen(member->value, moduleScope);
            if (!memberValue) {
                console.reportError(Console::SEMANTIC_ERROR, "Failed to generate IR for module member: " + memberName, FileSpan{});
                continue;
            }
    
            // Save the member value and type
            memberIRValues[memberName] = memberValue;
            // moduleScope->define(memberName, memberExpr->getType());
        }
    
        // Generate the actual module object
        llvm::Value* moduleInstance = createModuleObject(
            moduleExpr->name,
            memberIRValues
        );
    
        return moduleInstance;
    } 
    
    if (auto typeDecl = std::dynamic_pointer_cast<TypeDeclarationExpression>(value)) {
        if (!typeDecl->isAliasing()) {
            llvm::Type* resolvedType = resolveLLVMType(typeDecl->getType());
            activeScope->addType(typeDecl->typeName, resolvedType);
        } else {
            llvm::Type* originalType = activeScope->getType(typeDecl->originalTypeName);
            activeScope->addType(typeDecl->typeName, originalType);
        }
    }

    return nullptr;
}

llvm::Value* IRGenerator::createStringTemplate(const std::shared_ptr<Expression>& expr, SymbolTableType scope) {
    auto tmpl = std::dynamic_pointer_cast<StringTemplateExpression>(expr);
    if (!tmpl) return nullptr;

    llvm::Module* mod = currentModule ? &*currentModule : &*Module;
    llvm::LLVMContext& ctx = *Context;
    llvm::Type* i8Ty  = llvm::Type::getInt8Ty(ctx);
    llvm::Type* i32Ty = llvm::Type::getInt32Ty(ctx);
    llvm::Type* i64Ty = llvm::Type::getInt64Ty(ctx);
    llvm::Type* f64Ty = llvm::Type::getDoubleTy(ctx);
    llvm::Type* ptrTy = llvm::PointerType::getUnqual(ctx);

    constexpr uint64_t kSlots = 8;     // rotating scratch buffers (power of two)
    constexpr uint64_t kSize  = 512;   // bytes per buffer, including the terminating NUL

    // Build the printf format and the argument list, part by part, in source order.
    std::string fmt;
    std::vector<llvm::Value*> fmtArgs;
    for (const auto& part : tmpl->parts) {
        if (part.isText) {
            for (char c : part.text) {
                if (c == '%') fmt += "%%";
                else fmt += c;
            }
            continue;
        }

        llvm::Value* v = codegen(part.expr, scope);
        if (!v) {
            console.reportError(Console::SEMANTIC_ERROR,
                "Failed to generate code for template expression: " + (part.expr ? part.expr->toString() : std::string("?")),
                tmpl->getSpan());
            return nullptr;
        }

        llvm::Type* t = v->getType();
        if (t->isIntegerTy(1)) {
            fmt += "%s";
            fmtArgs.push_back(Builder->CreateSelect(v,
                Builder->CreateGlobalString("true", ".interp.true", 0, mod),
                Builder->CreateGlobalString("false", ".interp.false", 0, mod)));
        } else if (t->isIntegerTy()) {
            fmt += "%lld";
            fmtArgs.push_back(Builder->CreateSExtOrTrunc(v, i64Ty));
        } else if (t->isFloatingPointTy()) {
            fmt += "%g";
            fmtArgs.push_back(Builder->CreateFPCast(v, f64Ty));
        } else if (t->isPointerTy()) {
            auto et = part.expr->getType();
            auto pointee = (et && et->isPointer()) ? et->getPointeeType() : nullptr;
            if (pointee && (pointee->isChar(8) || pointee->isString(8))) {
                // char*: never hand NULL to %s
                llvm::Value* isNull = Builder->CreateICmpEQ(v, llvm::ConstantPointerNull::get(llvm::cast<llvm::PointerType>(t)));
                fmt += "%s";
                fmtArgs.push_back(Builder->CreateSelect(isNull,
                    Builder->CreateGlobalString("(null)", ".interp.null", 0, mod), v));
            } else {
                fmt += "%p";
                fmtArgs.push_back(v);
            }
        } else {
            console.reportError(Console::TYPE_ERROR,
                "Cannot interpolate a value of this type into a template string: " + part.expr->toString(),
                tmpl->getSpan());
            return nullptr;
        }
    }

    // Rotating scratch storage: one global byte array and a global slot counter per module.
    llvm::ArrayType* bufTy = llvm::ArrayType::get(i8Ty, kSlots * kSize);
    llvm::GlobalVariable* buf = mod->getNamedGlobal("__interp_buf");
    if (!buf) {
        buf = new llvm::GlobalVariable(*mod, bufTy, false, llvm::GlobalValue::InternalLinkage,
                                       llvm::ConstantAggregateZero::get(bufTy), "__interp_buf");
    }
    llvm::GlobalVariable* counter = mod->getNamedGlobal("__interp_idx");
    if (!counter) {
        counter = new llvm::GlobalVariable(*mod, i32Ty, false, llvm::GlobalValue::InternalLinkage,
                                           llvm::ConstantInt::get(i32Ty, 0), "__interp_idx");
    }

    llvm::Value* idx  = Builder->CreateLoad(i32Ty, counter, "interp.idx");
    Builder->CreateStore(Builder->CreateAdd(idx, llvm::ConstantInt::get(i32Ty, 1)), counter);
    llvm::Value* slot = Builder->CreateAnd(idx, llvm::ConstantInt::get(i32Ty, kSlots - 1), "interp.slot");
    llvm::Value* off  = Builder->CreateMul(Builder->CreateZExt(slot, i64Ty), llvm::ConstantInt::get(i64Ty, kSize));
    llvm::Value* dest = Builder->CreateGEP(i8Ty, buf, { off }, "interp.buf");

    // UCRT has no exported snprintf; the engine provides os_snprintf (see CLLVMFunctionResolver.cpp),
    // which is what the extern "C" name "snprintf" is mapped to. Declare that symbol directly.
    llvm::FunctionCallee snprintfFn = mod->getOrInsertFunction(
        "os_snprintf", llvm::FunctionType::get(i32Ty, { ptrTy, i64Ty, ptrTy }, /*isVarArg=*/true));

    std::vector<llvm::Value*> callArgs = {
        dest,
        llvm::ConstantInt::get(i64Ty, kSize),
        Builder->CreateGlobalString(fmt, ".interp.fmt", 0, mod)
    };
    callArgs.insert(callArgs.end(), fmtArgs.begin(), fmtArgs.end());
    Builder->CreateCall(snprintfFn, callArgs, "interp.len");

    return dest;
}

}