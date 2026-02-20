// engine/src/omniscript/extern/OsEmitter.cpp
//
// Converts parsed C/C++ declarations into OmniScript `extern` block source.
//
// OS type syntax recap:
//   fn foo(x: int, ptr: char*) => void;
//   fn bar(fmt: char*, ...) => int;
//   const MY_CONST: int = 42;
//   let g_counter: int;

#include <omniscript/extern/OsEmitter.h>

#include <algorithm>
#include <ostream>
#include <sstream>
#include <unordered_map>

namespace Omniscript {
namespace extern_support {

// =============================================================================
// Construction
// =============================================================================

OsEmitter::OsEmitter(EmitOptions opts) : m_opts(std::move(opts)) {}

// =============================================================================
// C/C++ type name  ->  OS primitive
// =============================================================================

std::string OsEmitter::mapPrimitive(const std::string& c) {
    static const std::unordered_map<std::string, std::string> kMap = {
        // void
        { "void",                "void"   },
        // bool
        { "bool",                "bool"   },
        { "_Bool",               "bool"   },
        // char
        { "char",                "char"   },
        { "wchar_t",             "wchar"  },
        { "char8_t",             "char"   },
        { "char16_t",            "char16" },
        { "char32_t",            "char32" },
        // signed integers
        { "signed char",         "i8"     },
        { "short",               "i16"    },
        { "signed short",        "i16"    },
        { "short int",           "i16"    },
        { "int",                 "int"    },
        { "signed",              "int"    },
        { "signed int",          "int"    },
        { "long",                "i64"    },
        { "long int",            "i64"    },
        { "long long",           "i64"    },
        { "long long int",       "i64"    },
        { "__int64",             "i64"    },
        { "__int32",             "int"    },
        { "__int16",             "i16"    },
        { "__int8",              "i8"     },
        // unsigned integers
        { "unsigned char",       "u8"     },
        { "unsigned short",      "u16"    },
        { "unsigned short int",  "u16"    },
        { "unsigned",            "uint"   },
        { "unsigned int",        "uint"   },
        { "unsigned long",       "u64"    },
        { "unsigned long int",   "u64"    },
        { "unsigned long long",  "u64"    },
        // stdint.h
        { "int8_t",              "i8"     },
        { "int16_t",             "i16"    },
        { "int32_t",             "i32"    },
        { "int64_t",             "i64"    },
        { "uint8_t",             "u8"     },
        { "uint16_t",            "u16"    },
        { "uint32_t",            "u32"    },
        { "uint64_t",            "u64"    },
        // size / ptr
        { "size_t",              "u64"    },
        { "ssize_t",             "i64"    },
        { "ptrdiff_t",           "i64"    },
        { "intptr_t",            "i64"    },
        { "uintptr_t",           "u64"    },
        // float
        { "float",               "float"  },
        { "double",              "double" },
        { "long double",         "double" },
    };

    auto it = kMap.find(c);
    return it != kMap.end() ? it->second : "";
}

// =============================================================================
// CType  ->  OS type string
// =============================================================================

std::string OsEmitter::toOsType(const CType& t) {
    std::string base = t.base;

    // Strip struct/class/union/enum prefix if present
    // e.g. "struct Vec2" -> "Vec2",  "enum class Mode" -> "Mode"
    for (const char* prefix : { "struct ", "union ", "class ", "enum class ", "enum struct ", "enum " }) {
        const std::size_t plen = std::char_traits<char>::length(prefix);
        if (base.size() > plen && base.compare(0, plen, prefix) == 0) {
            base = base.substr(plen);
            break;
        }
    }

    // Map primitive
    const std::string mapped = mapPrimitive(base);
    std::string result = mapped.empty() ? base : mapped;

    // Pointer chain
    for (int i = 0; i < t.pointers; ++i) result += '*';
    if (t.isConstPtr) result += " const";
    if (t.isRef)      result += '&';
    if (t.isRvalRef)  result += "&&";
    if (t.isArray) {
        result += '[';
        result += t.arraySize;
        result += ']';
    }

    return result;
}

// =============================================================================
// shouldSkip
// =============================================================================

bool OsEmitter::shouldSkip(const std::string& name) const {
    if (name.empty()) return true;
    if (m_opts.skipPrivateNames && name[0] == '_') return true;
    return false;
}

// =============================================================================
// emitFunction
// =============================================================================

void OsEmitter::emitFunction(std::ostream& os, const CFunctionDecl& fn) const {
    if (shouldSkip(fn.name)) return;
    if (fn.isConstructor || fn.isDestructor) return;
    if (m_opts.skipStaticMembers && fn.isStatic && !fn.classOwner.empty()) return;

    // Source comment showing original qualified name for methods
    if (m_opts.sourceComments && !fn.classOwner.empty()) {
        os << m_opts.indent << "// " << fn.qualifiedName() << '\n';
    }

    os << m_opts.indent << "fn " << fn.name << '(';

    bool first = true;
    for (const auto& p : fn.params) {
        if (!first) os << ", ";
        first = false;

        if (p.isVarArg) { os << "..."; continue; }

        if (!p.name.empty()) os << p.name << ": ";
        os << toOsType(p.type);

        // Default args are C++ only; keep as a comment so users know
        if (!p.defaultValue.empty())
            os << " /* = " << p.defaultValue << " */";
    }
    if (fn.isVariadic && (fn.params.empty() || !fn.params.back().isVarArg)) {
        if (!first) os << ", ";
        os << "...";
    }

    os << ") => " << toOsType(fn.returnType) << ";\n";
}

// =============================================================================
// emitVariable
// =============================================================================

void OsEmitter::emitVariable(std::ostream& os, const CVarDecl& var) const {
    if (shouldSkip(var.name)) return;
    if (m_opts.skipStaticMembers && var.isStatic && !var.classOwner.empty()) return;

    const bool isConst = var.isConst || var.isConstexpr ||
        std::find(var.type.qualifiers.begin(), var.type.qualifiers.end(), "const")
            != var.type.qualifiers.end();

    if (isConst) {
        os << m_opts.indent << "const " << var.name << ": " << toOsType(var.type);
        if (!var.defaultValue.empty()) os << " = " << var.defaultValue;
    } else {
        os << m_opts.indent << "let " << var.name << ": " << toOsType(var.type);
    }
    os << ";\n";
}

// =============================================================================
// emitStruct
// =============================================================================

void OsEmitter::emitStruct(std::ostream& os, const CStructDecl& s) const {
    if (shouldSkip(s.name)) return;

    if (!s.isDefinition) {
        os << m_opts.indent << "// [opaque " << s.kind << "] " << s.name << '\n';
        return;
    }

    if (m_opts.structsAsOpaque) {
        os << m_opts.indent << "// [" << s.kind << ' ' << s.name
           << " — use void* for FFI]\n";
        return;
    }

    os << m_opts.indent << "// " << s.kind << ' ' << s.name;
    if (!s.bases.empty()) {
        os << " :";
        for (const auto& b : s.bases) os << ' ' << b;
    }
    os << " {\n";

    for (const auto& f : s.fields) {
        if (f.name.empty()) continue;
        if (m_opts.skipPrivateNames && f.name[0] == '_') continue;

        os << m_opts.indent << m_opts.indent;
        if (f.isStatic) os << "static ";
        os << f.name << ": " << toOsType(f.type);
        if (f.isBitField) os << "  // bitfield:" << f.bitWidth;
        os << '\n';
    }
    os << m_opts.indent << "// }\n";
}

// =============================================================================
// emitEnum
// =============================================================================

void OsEmitter::emitEnum(std::ostream& os, const CEnumDecl& e) const {
    if (shouldSkip(e.name)) return;
    if (e.enumerators.empty()) return;

    os << m_opts.indent << "// enum" << (e.isClass ? " class " : " ") << e.name << '\n';

    for (const auto& [k, v] : e.enumerators) {
        if (m_opts.skipPrivateNames && !k.empty() && k[0] == '_') continue;

        // Underlying type -> OS type; default to int
        const std::string osType = e.underlying.empty()
            ? "int"
            : toOsType(CType{ {}, e.underlying, 0, false, false, false, "" });

        os << m_opts.indent << "const " << k << ": " << osType;
        if (!v.empty()) os << " = " << v;
        os << ";\n";
    }
}

// =============================================================================
// emitTypedef
// =============================================================================

void OsEmitter::emitTypedef(std::ostream& os, const CTypedef& td) const {
    if (shouldSkip(td.alias)) return;

    // Function-pointer typedef
    if (td.underlyingType.base.find("(*)") != std::string::npos) {
        os << m_opts.indent << "// [fn-ptr typedef] "
           << td.alias << " = " << td.underlyingType.toString() << '\n';
        return;
    }

    const std::string mapped = mapPrimitive(td.underlyingType.base);
    const std::string rhs    = mapped.empty()
        ? toOsType(td.underlyingType)
        : mapped;

    os << m_opts.indent << "// "
       << (td.isUsing ? "using" : "typedef")
       << ' ' << td.alias << " = " << rhs << '\n';
}

// =============================================================================
// emitBody
// =============================================================================

void OsEmitter::emitBody(std::ostream& os, const CHeaderResult& result) const {
    if (!result.typedefs.empty()) {
        os << m_opts.indent << "// -- type aliases --\n";
        for (const auto& td : result.typedefs) emitTypedef(os, td);
        os << '\n';
    }

    if (!result.enums.empty()) {
        os << m_opts.indent << "// -- enumerations --\n";
        for (const auto& e : result.enums) emitEnum(os, e);
        os << '\n';
    }

    if (!result.structs.empty()) {
        os << m_opts.indent << "// -- structs / classes --\n";
        for (const auto& s : result.structs) emitStruct(os, s);
        os << '\n';
    }

    if (!result.variables.empty()) {
        os << m_opts.indent << "// -- variables --\n";
        for (const auto& v : result.variables) emitVariable(os, v);
        os << '\n';
    }

    if (!result.functions.empty()) {
        os << m_opts.indent << "// -- functions --\n";
        for (const auto& fn : result.functions) emitFunction(os, fn);
    }
}

// =============================================================================
// emit  (full extern block)
// =============================================================================

void OsEmitter::emit(std::ostream& os,
                     const CHeaderResult& result,
                     const std::vector<std::string>& libraryPaths) const {
    // Warnings
    if (!result.errors.empty()) {
        for (const auto& e : result.errors)
            os << "// WARNING: " << e << '\n';
    }

    // extern "path1",\n       "path2" {
    os << "extern ";
    if (libraryPaths.empty()) {
        os << '"' << result.filePath << '"';
    } else {
        for (std::size_t i = 0; i < libraryPaths.size(); ++i) {
            if (i) os << ",\n       ";
            os << '"' << libraryPaths[i] << '"';
        }
    }
    os << " {\n";

    emitBody(os, result);

    os << "}\n";
}

} // namespace extern_support
} // namespace Omniscript