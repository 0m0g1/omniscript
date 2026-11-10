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
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <ostream>
#include <set>
#include <sstream>
#include <unordered_map>

namespace Omniscript {
namespace extern_support {

namespace {

// Strip C numeric suffixes (1u, 10UL, 2.0f) and validate that `in` is a plain
// literal / simple constant expression that OmniScript can parse as-is.
// Returns false for anything else (identifiers other than enumerator names are
// allowed only when `allowIdents` is true).
bool cleanLiteral(const std::string& in, std::string& out, bool allowIdents) {
    std::string v = in;
    while (!v.empty() && std::isspace((unsigned char)v.back())) v.pop_back();
    std::size_t b = 0;
    while (b < v.size() && std::isspace((unsigned char)v[b])) ++b;
    v = v.substr(b);
    if (v.empty()) return false;

    if (v.size() >= 2 && v.front() == '"' && v.back() == '"' && v.find('"', 1) == v.size() - 1) { out = v; return true; }

    std::string r;
    for (std::size_t i = 0; i < v.size();) {
        const char c = v[i];
        if (std::isdigit((unsigned char)c) || (c == '.' && i + 1 < v.size() && std::isdigit((unsigned char)v[i + 1]))) {
            std::size_t s0 = i;
            while (i < v.size() && (std::isalnum((unsigned char)v[i]) || v[i] == '.' ||
                   ((v[i] == '+' || v[i] == '-') && i > s0 && (v[i - 1] == 'e' || v[i - 1] == 'E')
                    && !(v[s0] == '0' && s0 + 1 < v.size() && (v[s0 + 1] == 'x' || v[s0 + 1] == 'X')))))
                ++i;
            std::string tok = v.substr(s0, i - s0);
            const bool hex = tok.size() > 1 && tok[0] == '0' && (tok[1] == 'x' || tok[1] == 'X');
            const bool flt = !hex && tok.find_first_of(".eE") != std::string::npos;
            while (!tok.empty() && (tok.back() == 'u' || tok.back() == 'U' || tok.back() == 'l' || tok.back() == 'L' ||
                                    (flt && (tok.back() == 'f' || tok.back() == 'F'))))
                tok.pop_back();
            r += tok;
            continue;
        }
        if (allowIdents && (std::isalpha((unsigned char)c) || c == '_')) {
            while (i < v.size() && (std::isalnum((unsigned char)v[i]) || v[i] == '_')) r += v[i++];
            continue;
        }
        if (std::isspace((unsigned char)c) || std::strchr("+-*/%()<>|&^~", c)) { r += c; ++i; continue; }
        return false;
    }
    out = r;
    return true;
}

bool parseInteger(const std::string& s, long long& v) {
    if (s.empty()) return false;
    char* end = nullptr;
    v = std::strtoll(s.c_str(), &end, 0);
    return end && *end == '\0';
}

// Words the OmniScript lexer turns into keyword tokens (see Tokens.h); they
// cannot be used as identifiers.
bool isOsKeyword(const std::string& n) {
    static const std::set<std::string> k = {
        "if","else","while","for","continue","break","return","fn","function","let","var","const",
        "new","delete","class","struct","namespace","using","public","private","protected",
        "override","virtual","static","final","true","false","nullptr","null","enum","extends",
        "variant","any","import","include","from","module","extern","intrinsic","volatile","as","type"
    };
    return k.count(n) != 0;
}

// Parameter / field / constant names that collide get a trailing '_'.
std::string safeName(const std::string& n) { return isOsKeyword(n) ? n + "_" : n; }

bool isStdHeaderStem(const std::string& stem) {
    static const std::set<std::string> k = {
        "stdio","stdlib","string","math","ctype","time","stdint","stddef","stdarg","stdbool",
        "errno","signal","setjmp","locale","limits","float","assert","wchar","wctype","inttypes",
        "fenv","complex","tgmath","uchar","threads","stdatomic"
    };
    return k.count(stem) != 0;
}

} // anonymous namespace

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
        { "unsigned long long int", "u64" },
        { "long unsigned int",   "u64"    },
        { "long long unsigned int", "u64" },
        { "unsigned __int64",    "u64"    },
        { "unsigned __int32",    "uint"   },
        { "unsigned __int16",    "u16"    },
        { "unsigned __int8",     "u8"     },
        { "signed long",         "i64"    },
        { "signed long long",    "i64"    },
        { "signed short int",    "i16"    },
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

#ifdef _WIN32
    // LLP64: 'long' is 32-bit on Windows
    if (c == "long" || c == "long int" || c == "signed long")  return "i32";
    if (c == "unsigned long" || c == "unsigned long int" || c == "long unsigned int") return "u32";
#endif
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

    // signed / unsigned live in CType::qualifiers
    {
        bool uns = false, sgn = false;
        for (const auto& q : t.qualifiers) {
            if (q == "unsigned") uns = true;
            else if (q == "signed" || q == "__signed__") sgn = true;
        }
        if (uns)      base = base.empty() ? "unsigned" : "unsigned " + base;
        else if (sgn && base == "char") base = "signed char";
    }

    // Map primitive
    const std::string mapped = mapPrimitive(base);
    std::string result = mapped.empty() ? base : mapped;

    // OmniScript cannot parse 'T*[N]' (array of pointers), e.g. Vulkan's
    // VkPhysicalDevice physicalDevices[32]. Pointers are 8 bytes on 64-bit targets,
    // so u64[N] has the same size and layout.
    if (t.isArray && t.pointers > 0 && !t.isRef && !t.isRvalRef) {
        return "u64[" + t.arraySize + "]";
    }

    // Pointer chain
    for (int i = 0; i < t.pointers; ++i) result += '*';
    // (a trailing C "const" on a pointer has no OmniScript equivalent: dropped)
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
    if (isOsKeyword(fn.name)) {
        os << m_opts.indent << "// skipped '" << fn.name << "': OmniScript keyword (cannot be bound by name)\n";
        return;
    }
    if (fn.isConstructor || fn.isDestructor) return;
    if (m_opts.skipStaticMembers && fn.isStatic && !fn.classOwner.empty()) return;

    // Source comment showing original qualified name for methods
    if (m_opts.sourceComments && !fn.classOwner.empty()) {
        os << m_opts.indent << "// " << fn.qualifiedName() << '\n';
    }

    os << m_opts.indent << "fn " << fn.name << '(';

    bool first = true;
    int argIndex = 0;
    for (const auto& p : fn.params) {
        if (!first) os << ", ";
        first = false;

        if (p.isVarArg) { os << "..."; continue; }

        // C arrays decay to pointers in parameter position
        CType pt = p.type;
        if (pt.isArray) { pt.isArray = false; pt.arraySize.clear(); ++pt.pointers; }

        os << (p.name.empty() ? "arg" + std::to_string(argIndex) : safeName(p.name)) << ": ";
        ++argIndex;
        os << toOsType(pt);

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
    if (isOsKeyword(var.name)) {
        os << m_opts.indent << "// skipped '" << var.name << "': OmniScript keyword (cannot be bound by name)\n";
        return;
    }
    if (m_opts.skipStaticMembers && var.isStatic && !var.classOwner.empty()) return;

    const bool isConst = var.isConst || var.isConstexpr ||
        std::find(var.type.qualifiers.begin(), var.type.qualifiers.end(), "const")
            != var.type.qualifiers.end();

    // Symbol exported by the library (no initializer): binds by name.
    os << m_opts.indent << (isConst ? "const " : "let ") << var.name << ": " << toOsType(var.type) << ";\n";
}

// =============================================================================
// emitStruct  — emits a real OS struct declaration (OUTSIDE the extern block)
// =============================================================================

void OsEmitter::emitStruct(std::ostream& os, const CStructDecl& s) const {
    if (shouldSkip(s.name)) return;

    if (!s.isDefinition || s.fields.empty()) {
        // Opaque / forward-declared: just a comment, no code emitted
        os << "// [opaque " << s.kind << "] " << s.name << '\n';
        return;
    }

    if (m_opts.structsAsOpaque) {
        os << "// [" << s.kind << ' ' << s.name << " — passed as void*]\n";
        return;
    }

    // Real struct declaration in OS syntax
    // class / union are both represented as 'struct' for now
    os << "struct " << s.name;
    if (!s.bases.empty()) {
        // OS has no inheritance yet — note bases as a comment
        os << " /* :";
        for (const auto& b : s.bases) os << ' ' << b;
        os << " */";
    }
    os << " {\n";

    for (const auto& f : s.fields) {
        if (f.name.empty()) continue;
        if (m_opts.skipPrivateNames && f.name[0] == '_') continue;
        if (f.isStatic) continue; // static fields aren't layout members
        if (f.isBitField) {
            // OmniScript has no bitfields. Emitting the field would also put the ';' inside the
            // trailing comment (syntax error), so keep it as a comment only. Note: the struct's
            // size/layout then differs from the C original.
            os << m_opts.indent << "// bitfield '" << f.name << " : " << f.bitWidth
               << "' skipped (no bitfield support; struct layout differs from C)\n";
            continue;
        }

        const std::string fts = toOsType(f.type);
        os << m_opts.indent << safeName(f.name) << ": " << fts;
        // Zero-initialise scalars and pointers so a bare `let x: S;` is a zeroed C struct
        // (Vulkan-style *CreateInfo structs rely on this). Arrays and nested structs are left alone.
        if (!f.type.isArray && !f.type.isRef && !f.type.isRvalRef) {
            static const std::set<std::string> kInt = {
                "int","uint","i8","i16","i32","i64","u8","u16","u32","u64","char","wchar","char16","char32" };
            if (f.type.pointers > 0)                 os << " = nullptr";
            else if (fts == "float" || fts == "double") os << " = 0.0";
            else if (fts == "bool")                  os << " = false";
            else if (kInt.count(fts))                os << " = 0";
        }
        os << ";\n";
    }

    os << "}\n";
}

// =============================================================================
// emitEnum
// =============================================================================

void OsEmitter::emitEnum(std::ostream& os, const CEnumDecl& e) const {
    if (!e.name.empty() && shouldSkip(e.name)) return;
    if (e.enumerators.empty()) return;

    const std::string osType = e.underlying.empty()
        ? "int"
        : toOsType(CType{ {}, e.underlying, 0, false, false, false, "" });

    if (m_opts.sourceComments)
        os << m_opts.indent << "// enum" << (e.isClass ? " class " : " ") << (e.name.empty() ? "<anonymous>" : e.name) << '\n';

    long long next = 0;
    bool haveNum = true;
    std::string prev;

    for (const auto& [k, v] : e.enumerators) {
        if (m_opts.skipPrivateNames && !k.empty() && k[0] == '_') continue;

        // enum class members are scoped in C++; flatten as Enum_Member
        const std::string cname = safeName((e.isClass && !e.name.empty()) ? e.name + "_" + k : k);

        std::string value;
        if (!v.empty()) {
            std::string cleaned;
            if (cleanLiteral(v, cleaned, /*allowIdents=*/true)) {
                long long n = 0;
                if (parseInteger(cleaned, n)) { next = n + 1; haveNum = true; value = std::to_string(n); }
                else                          { haveNum = false; value = cleaned; }
            } else {
                os << m_opts.indent << "// " << cname << " = " << v << "  (not translated)\n";
                haveNum = false; prev = cname;
                continue;
            }
        } else if (haveNum) {
            value = std::to_string(next);
            ++next;
        } else {
            value = prev + " + 1";
        }

        os << m_opts.indent << "const " << cname << ": " << osType << " = " << value << ";\n";
        prev = cname;
    }
}

// =============================================================================
// emitConstant  — object-like #define macro (numeric / string)
// =============================================================================

void OsEmitter::emitConstant(std::ostream& os, const CVarDecl& c) const {
    if (shouldSkip(c.name)) return;
    std::string v;
    if (!cleanLiteral(c.defaultValue, v, /*allowIdents=*/false)) return;
    os << m_opts.indent << "const " << safeName(c.name) << ": " << toOsType(c.type) << " = " << v << ";\n";
}

// =============================================================================
// emitTypedef
// =============================================================================

void OsEmitter::emitTypedef(std::ostream& os, const CTypedef& td) const {
    if (shouldSkip(td.alias)) return;

    if (td.underlyingType.base.find("(*)") != std::string::npos) {
        os << m_opts.indent << "// [fn-ptr typedef] "
           << td.alias << " = " << td.underlyingType.toString() << '\n';
        return;
    }

    const std::string rhs = toOsType(td.underlyingType);

    os << m_opts.indent << "// "
       << (td.isUsing ? "using" : "typedef")
       << ' ' << td.alias << " = " << rhs << '\n';
}

// =============================================================================
// emitBody — typedef comments, enums, variables and functions without any
// wrapper (kept for callers that merge several headers into one block).
// =============================================================================

void OsEmitter::emitBody(std::ostream& os, const CHeaderResult& result) const {
    if (!result.typedefs.empty() && m_opts.sourceComments) {
        os << m_opts.indent << "// -- type aliases --\n";
        for (const auto& td : result.typedefs) emitTypedef(os, td);
        os << '\n';
    }

    if (!result.enums.empty()) {
        os << m_opts.indent << "// -- enumerations --\n";
        for (const auto& e : result.enums) emitEnum(os, e);
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
// emit  — flat output (see OsEmitter.h)
// =============================================================================

void OsEmitter::emit(std::ostream& os,
                     const CHeaderResult& result,
                     const std::vector<std::string>& libraryPaths) const {
    for (const auto& e : result.errors)
        os << "// WARNING: " << e << '\n';

    // Top-level declarations use no indentation.
    EmitOptions flatOpts = m_opts;
    flatOpts.indent.clear();
    const OsEmitter top(flatOpts);

    // 1. structs
    {
        bool any = false;
        for (const auto& s : result.structs) {
            if (!any && !result.structs.empty()) { os << "// -- structs from: " << result.filePath << " --\n"; any = true; }
            emitStruct(os, s);
        }
        if (any) os << '\n';
    }

    // 2. #define constants and enumerators (real OmniScript constants)
    {
        std::ostringstream tmp;
        for (const auto& c : result.constants) top.emitConstant(tmp, c);
        for (const auto& e : result.enums)     top.emitEnum(tmp, e);
        // header-level 'const T X = literal;' variables (static const / constexpr)
        for (const auto& v : result.variables) {
            const bool isConst = v.isConst || v.isConstexpr;
            std::string lit;
            if (isConst && !v.defaultValue.empty() && v.classOwner.empty() && !top.shouldSkip(v.name) &&
                cleanLiteral(v.defaultValue, lit, false))
                tmp << "const " << safeName(v.name) << ": " << toOsType(v.type) << " = " << lit << ";\n";
        }
        if (!tmp.str().empty()) os << tmp.str() << '\n';
    }

    // 3. typedef aliases as comments (already resolved in signatures)
    if (m_opts.sourceComments && !result.typedefs.empty()) {
        for (const auto& td : result.typedefs) top.emitTypedef(os, td);
        os << '\n';
    }

    // 4. collect what goes into the extern block
    std::ostringstream body;
    bool anyBody = false;

    for (const auto& fn : result.functions) {
        if (shouldSkip(fn.name)) continue;
        if (fn.isConstructor || fn.isDestructor || fn.isOperator) continue;
        if (fn.isTemplate) continue;
        if (!fn.classOwner.empty()) continue;   // methods need a C++ ABI / 'this': not plain FFI
        if (m_opts.skipInlineFunctions && fn.classOwner.empty() && (fn.isStatic || fn.isInline)) continue;
        emitFunction(body, fn);
        anyBody = true;
    }

    for (const auto& v : result.variables) {
        if (shouldSkip(v.name) || !v.classOwner.empty()) continue;
        const bool isConst = v.isConst || v.isConstexpr;
        if (isConst && !v.defaultValue.empty()) continue; // emitted above as a constant
        if (v.isStatic) continue;                         // internal linkage: not exported
        emitVariable(body, v);
        anyBody = true;
    }

    if (!anyBody) return;

    // 5. library names
    std::vector<std::string> libs = libraryPaths;
    bool cLibrary = false;
    if (libs.empty()) {
        const std::string stem = std::filesystem::path(result.filePath).stem().string();
        if (isStdHeaderStem(stem)) {
            cLibrary = true;
        } else {
            libs.push_back(stem + ".dll");
            libs.push_back("lib" + stem + ".so");
            libs.push_back("lib" + stem + ".a");
        }
    }

    os << "extern ";
    if (cLibrary) {
        os << "\"C\"";
    } else {
        for (std::size_t i = 0; i < libs.size(); ++i) {
            if (i) os << ", ";
            os << '"' << libs[i] << '"';
        }
    }
    os << " {\n" << body.str() << "}\n";
}

} // namespace extern_support
} // namespace Omniscript