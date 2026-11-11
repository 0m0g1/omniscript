#include <omniscript/Backends/llvm/IRGenerator.h>

namespace Omniscript {
// Nullable<T> is lowered as an *unnamed* literal struct { i1 hasValue, T value } (see resolveLLVMType).
// Checked structurally here so this does not depend on isNullableStruct()'s exact rules.
static bool looksNullable(llvm::Type* t) {
    auto* st = llvm::dyn_cast<llvm::StructType>(t);
    return st && !st->hasName() && st->getNumElements() == 2 && st->getElementType(0)->isIntegerTy(1);
}

llvm::Value* IRGenerator::createUnaryExpression(llvm::Value* operand, TokenTypes op, bool isPostfix) {
    if (!operand) {
        console.reportError(Console::SEMANTIC_ERROR, "Invalid unary operation", FileSpan{});
        return nullptr;
    };

    // Handle numeric operations (both integer and floating point)
    auto handleNumericUnary = [&](auto createOp) -> llvm::Value* {
        if (operand->getType()->isIntegerTy()) {
            return createOp(operand, "unarytmp");
        } else if (operand->getType()->isFloatingPointTy()) {
            return createOp(operand, "funarytmp");
        }
        return nullptr;
    };

    switch (op) {
        case TokenTypes::Plus:
            return operand; // +x is just x

        case TokenTypes::Minus:
            return handleNumericUnary([&](auto val, auto name) {
                return val->getType()->isIntegerTy() 
                    ? Builder->CreateNeg(val, name)
                    : Builder->CreateFNeg(val, name);
            });

        case TokenTypes::LogicalNot:
            return Builder->CreateNot(Builder->CreateICmpNE(
                operand, 
                llvm::ConstantInt::get(operand->getType(), 0),
                "booltmp"
            ), "nottmp");

        case TokenTypes::Tilde:
            return Builder->CreateNot(operand, "bwnottmp");

        case TokenTypes::Increment:
        case TokenTypes::Decrement: {
            llvm::Value* one = llvm::ConstantInt::get(operand->getType(), 1);
            llvm::Value* delta = (op == TokenTypes::Increment) ? one : Builder->CreateNeg(one, "deltatmp");
            llvm::Value* newVal = Builder->CreateAdd(operand, delta, "incdec");
            
            // For variables, store the new value
            if (auto* load = llvm::dyn_cast<llvm::LoadInst>(operand)) {
                Builder->CreateStore(newVal, load->getPointerOperand());
            }
            
            return isPostfix ? operand : newVal;
        }

        case TokenTypes::BitwiseAnd:
            return operand;

        default:
            console.reportError(Console::TYPE_ERROR, "Unknown unary operator '" + getTokenTypeName(op) + "'.", FileSpan{});
            return nullptr;
    }
    return nullptr;
}

llvm::Value* IRGenerator::createBinaryExpression(llvm::Value* left, TokenTypes op, llvm::Value* right) {
    if (!left || !right) {
        console.reportError(Console::SEMANTIC_ERROR, "Invalid binary operation.", FileSpan{});
        return nullptr;
    };

    // Nullable == null or null == Nullable
    if (op == TokenTypes::Equals || op == TokenTypes::NotEquals) {
        bool isEqualOp = (op == TokenTypes::Equals);

        // A `null` literal reaches here in several shapes depending on how it was generated:
        // void, a null pointer constant (Kind::Null lowers to ptr), or an all-zero / hasValue=false
        // Nullable constant. Nullable layout is { i1 hasValue, T value }.
        auto isNullLiteral = [&](llvm::Value* v) -> bool {
            if (v->getType()->isVoidTy()) return true;
            if (llvm::isa<llvm::ConstantPointerNull>(v)) return true;
            if (looksNullable(v->getType())) {
                if (llvm::isa<llvm::ConstantAggregateZero>(v)) return true;
                if (auto* cs = llvm::dyn_cast<llvm::ConstantStruct>(v)) {
                    auto* flag = llvm::dyn_cast<llvm::ConstantInt>(cs->getOperand(0));
                    return flag && flag->isZero();
                }
            }
            return false;
        };

        // Left is nullable, right is null
        if (looksNullable(left->getType()) && isNullLiteral(right)) {
            llvm::Value* hasValue = Builder->CreateExtractValue(left, {0}, "hasValue");
            return Builder->CreateICmpEQ(
                hasValue, 
                Builder->getInt1(!isEqualOp),  // Equal -> false (is null), NotEqual -> true (is not null)
                isEqualOp ? "nonnull" : "isnull"
            );
        }

        // Right is nullable, left is null
        if (looksNullable(right->getType()) && isNullLiteral(left)) {
            llvm::Value* hasValue = Builder->CreateExtractValue(right, {0}, "hasValue");
            return Builder->CreateICmpEQ(
                hasValue, 
                Builder->getInt1(!isEqualOp),
                isEqualOp ? "nonnull" : "isnull"
            );
        }

        // Nullable == Nullable (neither side is a null literal):
        // equal when both are null, or both have a value and the values match.
        if (looksNullable(left->getType()) && looksNullable(right->getType())) {
            llvm::Value* lHas = Builder->CreateExtractValue(left, {0}, "lhasValue");
            llvm::Value* rHas = Builder->CreateExtractValue(right, {0}, "rhasValue");
            llvm::Value* lVal = Builder->CreateExtractValue(left, {1}, "lvalue");
            llvm::Value* rVal = Builder->CreateExtractValue(right, {1}, "rvalue");
            if (!lVal->getType()->isIntOrIntVectorTy() && !lVal->getType()->isPtrOrPtrVectorTy()) {
                console.reportError(Console::TYPE_ERROR,
                    "Cannot compare two nullable values of type '" + debugType(lVal->getType()) +
                    "' with '=='; compare against null or compare the fields instead.", FileSpan{});
                return nullptr;
            }
            llvm::Value* bothNull = Builder->CreateAnd(Builder->CreateNot(lHas), Builder->CreateNot(rHas));
            llvm::Value* bothSame = Builder->CreateAnd(Builder->CreateAnd(lHas, rHas), Builder->CreateICmpEQ(lVal, rVal));
            llvm::Value* eq = Builder->CreateOr(bothNull, bothSame, "nullable_eq");
            return isEqualOp ? eq : Builder->CreateNot(eq, "nullable_ne");
        }

        // Nullable == Non-nullable
        if (looksNullable(left->getType()) && !looksNullable(right->getType()) && !right->getType()->isVoidTy()) {
            llvm::Value* leftHasValue = Builder->CreateExtractValue(left, {0}, "hasValue");
            llvm::Value* leftValue = Builder->CreateExtractValue(left, {1}, "value");

            // if (!left.hasValue) return false;
            llvm::Value* checkHasValue = Builder->CreateICmpEQ(leftHasValue, Builder->getInt1(true));
            llvm::Value* checkValue = Builder->CreateICmpEQ(leftValue, right);

            return Builder->CreateAnd(checkHasValue, checkValue);
        }

        // Non-nullable == Nullable
        if (!looksNullable(left->getType()) && looksNullable(right->getType()) && !left->getType()->isVoidTy()) {
            llvm::Value* rightHasValue = Builder->CreateExtractValue(right, {0}, "hasValue");
            llvm::Value* rightValue = Builder->CreateExtractValue(right, {1}, "value");

            llvm::Value* checkHasValue = Builder->CreateICmpEQ(rightHasValue, Builder->getInt1(true));
            llvm::Value* checkValue = Builder->CreateICmpEQ(left, rightValue);

            return Builder->CreateAnd(checkHasValue, checkValue);
        }
    }

    // LLVM requires both operands of an arithmetic / bitwise / comparison instruction to have the
    // same type. Mixed-width integers happen when a literal (i32) meets a wider value (e.g. u64 `len == 0`).
    if (op != TokenTypes::LogicalAnd && op != TokenTypes::LogicalOr &&
        left->getType()->isIntegerTy() && right->getType()->isIntegerTy() &&
        left->getType() != right->getType() &&
        left->getType()->getIntegerBitWidth() > 1 && right->getType()->getIntegerBitWidth() > 1) {
        unsigned lw = left->getType()->getIntegerBitWidth();
        unsigned rw = right->getType()->getIntegerBitWidth();
        if (llvm::isa<llvm::ConstantInt>(right)) {
            // Literal on the right: take the left operand's type.
            right = Builder->CreateSExtOrTrunc(right, left->getType(), "litcast");
        } else if (llvm::isa<llvm::ConstantInt>(left)) {
            left = Builder->CreateSExtOrTrunc(left, right->getType(), "litcast");
        } else if (lw < rw) {
            // NOTE: sign-extends; the backend has no signedness info here.
            left = Builder->CreateSExt(left, right->getType(), "widen");
        } else {
            right = Builder->CreateSExt(right, left->getType(), "widen");
        }
    }

    switch (op) {
        // Arithmetic
        case TokenTypes::Plus:
            return left->getType()->isFloatingPointTy()
                ? Builder->CreateFAdd(left, right, "faddtmp")
                : Builder->CreateAdd(left, right, "addtmp");
            
        case TokenTypes::Minus:
            return left->getType()->isFloatingPointTy()
                ? Builder->CreateFSub(left, right, "fsubtmp")
                : Builder->CreateSub(left, right, "subtmp");
            
        case TokenTypes::Multiply:
            return left->getType()->isFloatingPointTy()
                ? Builder->CreateFMul(left, right, "fmultmp")
                : Builder->CreateMul(left, right, "multmp");
            
        case TokenTypes::Divide:
            return left->getType()->isFloatingPointTy()
                ? Builder->CreateFDiv(left, right, "fdivtmp")
                : Builder->CreateSDiv(left, right, "divtmp");
            
        case TokenTypes::Modulo:
            return left->getType()->isFloatingPointTy()
                ? Builder->CreateFRem(left, right, "fmodtmp")
                : Builder->CreateSRem(left, right, "modtmp");
            
        // Comparisons
        case TokenTypes::Equals:
            return left->getType()->isFloatingPointTy()
                ? Builder->CreateFCmpOEQ(left, right, "feqtmp")
                : Builder->CreateICmpEQ(left, right, "eqtmp");
            
        case TokenTypes::NotEquals:
            return left->getType()->isFloatingPointTy()
                ? Builder->CreateFCmpUNE(left, right, "fnetmp")
                : Builder->CreateICmpNE(left, right, "netmp");
            
        case TokenTypes::LessThan:
            return left->getType()->isFloatingPointTy()
                ? Builder->CreateFCmpOLT(left, right, "flttmp")
                : Builder->CreateICmpSLT(left, right, "lttmp");
            
        case TokenTypes::LessEqual:
            return left->getType()->isFloatingPointTy()
                ? Builder->CreateFCmpOLE(left, right, "fletmp")
                : Builder->CreateICmpSLE(left, right, "letmp");
            
        case TokenTypes::GreaterThan:
            return left->getType()->isFloatingPointTy()
                ? Builder->CreateFCmpOGT(left, right, "fgttmp")
                : Builder->CreateICmpSGT(left, right, "gttmp");
            
        case TokenTypes::GreaterEqual:
            return left->getType()->isFloatingPointTy()
                ? Builder->CreateFCmpOGE(left, right, "fgetmp")
                : Builder->CreateICmpSGE(left, right, "getmp");
            
        // Bitwise
        case TokenTypes::BitwiseAnd:
            return Builder->CreateAnd(left, right, "andtmp");
            
        case TokenTypes::BitwiseOr:
            return Builder->CreateOr(left, right, "ortmp");
            
        case TokenTypes::BitwiseXor:
            return Builder->CreateXor(left, right, "xortmp");
            
        case TokenTypes::ShiftLeft:
            return Builder->CreateShl(left, right, "shltmp");
            
        case TokenTypes::ShiftRight: {
            if (!left->getType()->isIntegerTy())
                return nullptr; // Only valid for integers

            // Check if left is signed or unsigned
            bool isSigned = left->getType()->isIntegerTy(); // custom helper? or use your own type system

            // Use arithmetic shift for signed integers, logical for unsigned
            return isSigned
                ? Builder->CreateAShr(left, right, "ashrtmp")  // arithmetic shift right
                : Builder->CreateLShr(left, right, "lshrtmp"); // logical shift right
        }

        case TokenTypes::LogicalAnd: {
            // Ensure both operands are boolean-like (i1)
            llvm::Value* lhs = Builder->CreateICmpNE(left, llvm::ConstantInt::get(left->getType(), 0), "lhscond");

            llvm::Function* function = Builder->GetInsertBlock()->getParent();

            // Create blocks
            llvm::BasicBlock* rhsBlock = llvm::BasicBlock::Create(*Context, "rhs", function);
            llvm::BasicBlock* mergeBlock = llvm::BasicBlock::Create(*Context, "landmerge", function);

            // Result variable
            llvm::PHINode* phi = nullptr;

            llvm::BasicBlock* lhsBlock = Builder->GetInsertBlock(); // Save this before setting insert point

            Builder->CreateCondBr(lhs, rhsBlock, mergeBlock);

            // RHS block
            Builder->SetInsertPoint(rhsBlock);
            llvm::Value* rhs = Builder->CreateICmpNE(right, llvm::ConstantInt::get(right->getType(), 0), "rhscond");
            Builder->CreateBr(mergeBlock);

            // Merge block
            Builder->SetInsertPoint(mergeBlock);
            phi = Builder->CreatePHI(llvm::Type::getInt1Ty(*Context), 2, "landtmp");

            // Correct incoming blocks
            phi->addIncoming(llvm::ConstantInt::getFalse(*Context), lhsBlock);
            phi->addIncoming(rhs, rhsBlock);

            return phi;
        }

        case TokenTypes::LogicalOr: {
            // Ensure both operands are boolean-like (i1)
            llvm::Value* lhs = Builder->CreateICmpNE(left, llvm::ConstantInt::get(left->getType(), 0), "lhscond");

            llvm::Function* function = Builder->GetInsertBlock()->getParent();

            // Create blocks
            llvm::BasicBlock* rhsBlock = llvm::BasicBlock::Create(*Context, "rhs", function);
            llvm::BasicBlock* mergeBlock = llvm::BasicBlock::Create(*Context, "lormerge", function);

            // Result variable
            llvm::PHINode* phi = nullptr;

            llvm::BasicBlock* lhsBlock = Builder->GetInsertBlock(); // Save this before setting insert point

            Builder->CreateCondBr(lhs, mergeBlock, rhsBlock);  // Note: reversed compared to AND

            // RHS block
            Builder->SetInsertPoint(rhsBlock);
            llvm::Value* rhs = Builder->CreateICmpNE(right, llvm::ConstantInt::get(right->getType(), 0), "rhscond");
            Builder->CreateBr(mergeBlock);

            // Merge block
            Builder->SetInsertPoint(mergeBlock);
            phi = Builder->CreatePHI(llvm::Type::getInt1Ty(*Context), 2, "lortmp");

            // Correct incoming blocks - reversed compared to AND
            phi->addIncoming(llvm::ConstantInt::getTrue(*Context), lhsBlock);
            phi->addIncoming(rhs, rhsBlock);

            return phi;
        }

        default:
            console.reportError(Console::TYPE_ERROR, "Unknown binary operator '" + getTokenTypeName(op) + "'.", FileSpan{});
            return nullptr;
        }
    return nullptr;
}

llvm::Value* IRGenerator::createTernaryExpression(llvm::Value* cond, llvm::Value* truthy, llvm::Value* falsey) {
    if (!cond || !truthy || !falsey) {
        console.reportError(Console::SEMANTIC_ERROR, "Invalid ternary operation", FileSpan{});
        return nullptr;
    };

    // Convert condition to bool if needed
    if (!cond->getType()->isIntegerTy(1)) {
        cond = Builder->CreateICmpNE(
            cond, 
            llvm::ConstantInt::get(cond->getType(), 0),
            "booltmp"
        );
    }

    llvm::Function* fn = Builder->GetInsertBlock()->getParent();
    
    // Create blocks for the true/false/merge cases
    llvm::BasicBlock* trueBB = llvm::BasicBlock::Create(*Context, "true", fn);
    llvm::BasicBlock* falseBB = llvm::BasicBlock::Create(*Context, "false", fn);
    llvm::BasicBlock* mergeBB = llvm::BasicBlock::Create(*Context, "merge", fn);

    Builder->CreateCondBr(cond, trueBB, falseBB);

    // Emit true value
    Builder->SetInsertPoint(trueBB);
    Builder->CreateBr(mergeBB);
    trueBB = Builder->GetInsertBlock();

    // Emit false value
    Builder->SetInsertPoint(falseBB);
    Builder->CreateBr(mergeBB);
    falseBB = Builder->GetInsertBlock();

    // Create PHI node
    Builder->SetInsertPoint(mergeBB);
    llvm::PHINode* phi = Builder->CreatePHI(truthy->getType(), 2, "ternarytmp");
    phi->addIncoming(truthy, trueBB);
    phi->addIncoming(falsey, falseBB);

    return phi;
}

} // namespace Omniscript