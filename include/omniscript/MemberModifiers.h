#pragma once

// STL
#include <string>
#include <functional>

// Omniscript
#include <omniscript/Tokens.h>
#include <omniscript/Core.h>
#include <omniscript/Types/BaseType.h>

namespace Omniscript {
    
struct MemberModifiers {
    enum class AccessModifier { Public, Protected, Private };
    AccessModifier access = AccessModifier::Public;
    bool isInitialized = false;

    bool isStatic = false;
    bool isExtern = false;
    bool isMutable = false;
    bool isThreadLocal = false;

    bool isVirtual = false;
    bool isOverride = false;
    bool shouldOverride = false;
    bool isFinal = false;
    bool isConst = false;
    bool isVolatile = false;
    bool isNoexcept = false;
    bool isPureVirtual = false;
    bool isExplicit = false;
    bool isInline = false;
    bool isConstexpr = false;

    bool isDefault = false;
    bool isDeleted = false;

    bool isNodiscard = false;
    bool isMaybeUnused = false;
    bool isDeprecated = false;
    bool isLikely = false;
    bool isUnlikely = false;

    std::string toString() const {
        std::string result;
        switch (access) {
            case AccessModifier::Public: result += "public "; break;
            case AccessModifier::Protected: result += "protected "; break;
            case AccessModifier::Private: result += "private "; break;
        }

        if (isStatic) result += "static ";
        if (isExtern) result += "extern ";
        if (isMutable) result += "mutable ";
        if (isThreadLocal) result += "thread_local ";

        if (isVirtual) result += "virtual ";
        if (isOverride) result += "is_override ";
        if (shouldOverride) result += "should_override ";
        if (isFinal) result += "final ";
        if (isConst) result += "const ";
        if (isVolatile) result += "volatile ";
        if (isNoexcept) result += "noexcept ";
        if (isPureVirtual) result += "= 0 (pure virtual) ";
        if (isExplicit) result += "explicit ";
        if (isInline) result += "inline ";
        if (isConstexpr) result += "constexpr ";

        if (isDefault) result += "= default ";
        if (isDeleted) result += "= delete ";

        if (isNodiscard) result += "[[nodiscard]] ";
        if (isMaybeUnused) result += "[[maybe_unused]] ";
        if (isDeprecated) result += "[[deprecated]] ";
        if (isLikely) result += "[[likely]] ";
        if (isUnlikely) result += "[[unlikely]] ";

        return result.empty() ? "none" : result;
    }
};

} // namespace Omniscript

namespace std {
    template <>
    struct hash<Omniscript::MemberModifiers> {
        size_t operator()(const Omniscript::MemberModifiers& modifiers) const {
            size_t result = 0;
            auto hash_combine = [&](bool flag) {
                result ^= static_cast<size_t>(flag) + 0x9e3779b9 + (result << 6) + (result >> 2);
            };

            result ^= static_cast<size_t>(modifiers.access) + 0x9e3779b9 + (result << 6) + (result >> 2);
            hash_combine(modifiers.isStatic);
            hash_combine(modifiers.isExtern);
            hash_combine(modifiers.isMutable);
            hash_combine(modifiers.isThreadLocal);
            hash_combine(modifiers.isVirtual);
            hash_combine(modifiers.isOverride);
            hash_combine(modifiers.shouldOverride);
            hash_combine(modifiers.isFinal);
            hash_combine(modifiers.isConst);
            hash_combine(modifiers.isVolatile);
            hash_combine(modifiers.isNoexcept);
            hash_combine(modifiers.isPureVirtual);
            hash_combine(modifiers.isExplicit);
            hash_combine(modifiers.isInline);
            hash_combine(modifiers.isConstexpr);
            hash_combine(modifiers.isDefault);
            hash_combine(modifiers.isDeleted);
            hash_combine(modifiers.isNodiscard);
            hash_combine(modifiers.isMaybeUnused);
            hash_combine(modifiers.isDeprecated);
            hash_combine(modifiers.isLikely);
            hash_combine(modifiers.isUnlikely);

            return result;
        }
    };
}

