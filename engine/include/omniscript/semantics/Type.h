#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <memory>
#include <utility>

namespace Omniscript {

enum class TypeKind : std::uint8_t {
    Unknown,
    Void,
    Bool,
    Int,
    Float,
    Char,

    Struct,
    Extern,
    Function,
    Pointer
};

struct Type {
    TypeKind kind{TypeKind::Unknown};

    // Used by Struct / Extern
    std::string name;

    // Function type
    std::vector<Type> params;
    std::vector<std::string> paramNames; // optional
    bool isVarArg{false};
    std::unique_ptr<Type> ret;           // nullptr => unknown/unset

    // Pointer type
    std::unique_ptr<Type> pointee;       // nullptr => unknown/unset

    Type() = default;
    explicit Type(TypeKind k) : kind(k) {}

    // Deep-copy ctor
    Type(const Type& other)
        : kind(other.kind)
        , name(other.name)
        , params(other.params)
        , paramNames(other.paramNames)
        , isVarArg(other.isVarArg)
        , ret(other.ret ? std::make_unique<Type>(*other.ret) : nullptr)
        , pointee(other.pointee ? std::make_unique<Type>(*other.pointee) : nullptr)
    {}

    // Deep-copy assign
    Type& operator=(const Type& other) {
        if (this == &other) return *this;

        kind       = other.kind;
        name       = other.name;
        params     = other.params;
        paramNames = other.paramNames;
        isVarArg   = other.isVarArg;

        ret     = other.ret ? std::make_unique<Type>(*other.ret) : nullptr;
        pointee = other.pointee ? std::make_unique<Type>(*other.pointee) : nullptr;

        return *this;
    }

    Type(Type&&) noexcept = default;
    Type& operator=(Type&&) noexcept = default;

    // ---- factories ----
    static Type Unknown() { return Type{TypeKind::Unknown}; }
    static Type Void()    { return Type{TypeKind::Void}; }
    static Type Bool()    { return Type{TypeKind::Bool}; }
    static Type Int()     { return Type{TypeKind::Int}; }
    static Type Float()   { return Type{TypeKind::Float}; }
    static Type Char()    { return Type{TypeKind::Char}; }

    static Type Struct(std::string n) {
        Type t{TypeKind::Struct};
        t.name = std::move(n);
        return t;
    }

    static Type Extern(std::string n) {
        Type t{TypeKind::Extern};
        t.name = std::move(n);
        return t;
    }

    static Type PointerTo(Type p) {
        Type t{TypeKind::Pointer};
        t.pointee = std::make_unique<Type>(std::move(p)); // <-- std::
        return t;
    }

    static Type Function(std::vector<Type> ps, Type r, bool vararg = false) {
        Type t{TypeKind::Function};
        t.params = std::move(ps);
        t.isVarArg = vararg;
        t.ret = std::make_unique<Type>(std::move(r));
        return t;
    }

    bool isKnown() const { return kind != TypeKind::Unknown; }

    const Type& returnTypeOrUnknown() const {
        static const Type unk = Type::Unknown();
        return ret ? *ret : unk;
    }

    const Type& pointeeOrUnknown() const {
        static const Type unk = Type::Unknown();
        return pointee ? *pointee : unk;
    }

    std::string toString() const {
        switch (kind) {
            case TypeKind::Unknown: return "unknown";
            case TypeKind::Void:    return "void";
            case TypeKind::Bool:    return "bool";
            case TypeKind::Int:     return "int";
            case TypeKind::Float:   return "float";
            case TypeKind::Char:    return "char";

            case TypeKind::Struct:
                return "struct " + name;

            case TypeKind::Extern:
                return "extern " + name;

            case TypeKind::Pointer:
                return pointeeOrUnknown().toString() + "*";

            case TypeKind::Function: {
                std::string s = "fn(";
                for (size_t i = 0; i < params.size(); ++i) {
                    if (i) s += ", ";
                    s += params[i].toString();
                }
                if (isVarArg) {
                    if (!params.empty()) s += ", ";
                    s += "...";
                }
                s += ") -> " + returnTypeOrUnknown().toString();
                return s;
            }
        }
        return "unknown";
    }
};

inline bool sameType(const Type& a, const Type& b) {
    if (a.kind != b.kind) return false;

    if ((a.kind == TypeKind::Struct || a.kind == TypeKind::Extern) && a.name != b.name)
        return false;

    if (a.kind == TypeKind::Pointer) {
        return sameType(a.pointeeOrUnknown(), b.pointeeOrUnknown());
    }

    if (a.kind == TypeKind::Function) {
        if (a.isVarArg != b.isVarArg) return false;

        if (!sameType(a.returnTypeOrUnknown(), b.returnTypeOrUnknown()))
            return false;

        if (a.params.size() != b.params.size()) return false;
        for (size_t i = 0; i < a.params.size(); ++i) {
            if (!sameType(a.params[i], b.params[i])) return false;
        }
    }

    return true;
}

} // namespace Omniscript