#include <omniscript/Types/Casting.h>
#include <omniscript/Types/BaseType.h>
#include <omniscript/Types/DerivedTypes.h>
#include <omniscript/Core.h>

namespace Omniscript {

CastResult CastingSystem::canCast(const std::shared_ptr<Type>& from, const std::shared_ptr<Type>& to, CastKind castKind) {
    if (!from || !to) {
        TYPE_ERROR("Null type in cast operation");
        return CastResult::Invalid;
    }

    DEBUG_LOG("Checking cast from " + from->toString() + " to " + to->toString() + " with kind " + std::to_string(static_cast<int>(castKind)));

    // Same type
    if (Type::isSame(from, to)) {
        return CastResult::Success;
    }

    // Check implicit convertibility for SafeCast and OptionalCast
    if (castKind == CastKind::SafeCast || castKind == CastKind::OptionalCast) {
        if (isImplicitlyConvertible(from, to)) {
            return CastResult::Success;
        }
        return (castKind == CastKind::OptionalCast) ? CastResult::Failure : CastResult::Invalid;
    }

    // ForceCast: Allow if castable, but mark as unsafe
    if (castKind == CastKind::ForceCast) {
        if (Type::isSameOrCastableTo(from, to)) {
            return CastResult::Unsafe;
        }
        return CastResult::Invalid;
    }

    // BitCast: Check memory layout compatibility
    if (castKind == CastKind::BitCast) {
        if (isBitCastable(from, to)) {
            return CastResult::Unsafe;
        }
        return CastResult::Invalid;
    }

    return CastResult::Invalid;
}

bool CastingSystem::isImplicitlyConvertible(const std::shared_ptr<Type>& from, const std::shared_ptr<Type>& to) {
    if (!from || !to) return false;

    // Check if types are same or castable
    if (Type::isSameOrCastableTo(from, to)) {
        // Numeric casts
        if (from->isInteger() && to->isInteger()) {
            return checkNumericCast(from, to, CastKind::SafeCast);
        }
        if (from->isFloat() && to->isFloat()) {
            return checkNumericCast(from, to, CastKind::SafeCast);
        }
        if (from->isInteger() && to->isFloat()) {
            return true; // Int to float is generally safe
        }
        // Pointer casts
        if (from->isPointer() && to->isPointer()) {
            return checkPointerCast(from, to, CastKind::SafeCast);
        }
        // Inheritance-based casts
        if (from->isUserDefined() && to->isUserDefined()) {
            return checkInheritanceCast(from, to, CastKind::SafeCast);
        }
        // Null handling
        if (from->isNull() && (to->isPointer() || to->isReference())) {
            return true;
        }
        if (from->isNullable()) {
            auto nullableFrom = std::dynamic_pointer_cast<NullableType>(from);
            return isImplicitlyConvertible(nullableFrom->innerType, to);
        }
    }

    return false;
}

bool CastingSystem::isSafelyCastable(const std::shared_ptr<Type>& from, const std::shared_ptr<Type>& to) {
    return canCast(from, to, CastKind::SafeCast) == CastResult::Success;
}

bool CastingSystem::isBitCastable(const std::shared_ptr<Type>& from, const std::shared_ptr<Type>& to) {
    if (!from || !to) return false;

    // Bitcast requires same size and compatible memory layout
    if (from->getSize() != to->getSize()) {
        DEBUG_LOG("Bitcast failed: size mismatch (" + std::to_string(from->getSize()) + " vs " + std::to_string(to->getSize()) + ")");
        return false;
    }

    // Allow bitcast between pointers, numeric types, or compatible user-defined types
    if ((from->isPointer() && to->isPointer()) || (from->isNumericLiteral() && to->isNumericLiteral())) {
        return true;
    }

    if (from->isUserDefined() && to->isUserDefined()) {
        auto fromUDT = std::dynamic_pointer_cast<UserDefinedType>(from);
        auto toUDT = std::dynamic_pointer_cast<UserDefinedType>(to);
        return fromUDT && toUDT && fromUDT->name == toUDT->name;
    }

    return false;
}

std::shared_ptr<Type> CastingSystem::getCastResultType(const std::shared_ptr<Type>& from, const std::shared_ptr<Type>& to, CastKind castKind) {
    if (canCast(from, to, castKind) == CastResult::Success || canCast(from, to, castKind) == CastResult::Unsafe) {
        return to->clone();
    }
    if (castKind == CastKind::OptionalCast) {
        return Type::createNullableType(to);
    }
    return Type::createInvalid();
}

bool CastingSystem::checkNumericCast(const std::shared_ptr<Type>& from, const std::shared_ptr<Type>& to, CastKind castKind) {
    if (from->isInteger() && to->isInteger()) {
        return from->getSize() <= to->getSize(); // Allow widening conversions
    }
    if (from->isFloat() && to->isFloat()) {
        return from->getSize() <= to->getSize(); // Allow widening conversions
    }
    return false;
}

bool CastingSystem::checkPointerCast(const std::shared_ptr<Type>& from, const std::shared_ptr<Type>& to, CastKind castKind) {
    auto fromPtr = std::dynamic_pointer_cast<PointerType>(from);
    auto toPtr = std::dynamic_pointer_cast<PointerType>(to);
    if (!fromPtr || !toPtr) return false;

    auto fromPointee = fromPtr->getPointeeType();
    auto toPointee = toPtr->getPointeeType();
    if (!fromPointee || !toPointee) return false;

    // Allow casting to void* or from void*
    if (fromPointee->isVoidLike() || toPointee->isVoidLike()) {
        return true;
    }

    // Check inheritance for user-defined types
    if (fromPointee->isUserDefined() && toPointee->isUserDefined()) {
        return checkInheritanceCast(fromPointee, toPointee, castKind);
    }

    return Type::isSameOrCastableTo(fromPointee, toPointee);
}

bool CastingSystem::checkInheritanceCast(const std::shared_ptr<Type>& from, const std::shared_ptr<Type>& to, CastKind castKind) {
    auto fromUDT = std::dynamic_pointer_cast<UserDefinedType>(from);
    auto toUDT = std::dynamic_pointer_cast<UserDefinedType>(to);
    if (!fromUDT || !toUDT) return false;

    return fromUDT->derivesFrom(toUDT);
}

} // namespace Omniscript