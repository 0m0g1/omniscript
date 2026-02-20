// engine/src/omniscript/extern/CHeaderParser.cpp
//
// C and C++ header parser for OmniScript's `extern` FFI system.
//
// Handles:
//   C:
//     Function prototypes, variable declarations, struct/union definitions,
//     enums, typedefs (including function-pointer typedefs), extern "C" blocks,
//     preprocessor lines (skipped), line/block comments.
//
//   C++:
//     Namespaces (nested, anonymous), classes/structs with base lists,
//     public/protected/private access specifiers, member functions,
//     constructors/destructors, virtual/override/pure-virtual, static members,
//     const/noexcept trailing qualifiers, operator overloads, templates
//     (declaration captured, body skipped), enum class / enum struct,
//     'using' type aliases, inline variables, constexpr, default arguments,
//     rvalue references (&&), deleted/defaulted functions (skipped).

#include <omniscript/extern/CHeaderParser.h>

#include <algorithm>
#include <cassert>
#include <cctype>
#include <iostream>
#include <sstream>

namespace Omniscript {
namespace extern_support {

// =============================================================================
// CType::toString
// =============================================================================
std::string CType::toString() const {
    std::string s;
    for (const auto& q : qualifiers) { s += q; s += ' '; }
    s += base;
    for (int i = 0; i < pointers; ++i) s += '*';
    if (isConstPtr) s += " const";
    if (isRef)      s += '&';
    if (isRvalRef)  s += "&&";
    if (isArray)    { s += '['; s += arraySize; s += ']'; }
    return s;
}

// =============================================================================
// qualifiedName
// =============================================================================
std::string CFunctionDecl::qualifiedName() const {
    std::string q;
    if (!ns.empty())        { q += ns; q += "::"; }
    if (!classOwner.empty()){ q += classOwner; q += "::"; }
    q += name;
    return q;
}

// =============================================================================
// Print helpers
// =============================================================================
static std::string ind(int n) { return std::string(static_cast<std::size_t>(n * 2), ' '); }

void CFunctionDecl::print(std::ostream& os, int indent) const {
    os << ind(indent);
    if (isTemplate)    os << "[template] ";
    if (isVirtual)     os << "virtual ";
    if (isStatic)      os << "static ";
    if (isInline)      os << "inline ";
    if (isConstexpr)   os << "constexpr ";
    if (isConstructor) os << "[ctor] ";
    else if (isDestructor) os << "[dtor] ";
    else if (isOperator)   os << "[op] ";
    else                   os << "[fn] ";
    os << qualifiedName() << '(';
    for (std::size_t i = 0; i < params.size(); ++i) {
        if (i) os << ", ";
        const auto& p = params[i];
        if (p.isVarArg) { os << "..."; continue; }
        os << p.type.toString();
        if (!p.name.empty()) os << ' ' << p.name;
        if (!p.defaultValue.empty()) os << " = " << p.defaultValue;
    }
    os << ')';
    if (!isConstructor && !isDestructor) os << " -> " << returnType.toString();
    if (isConst)       os << " const";
    if (isNoexcept)    os << " noexcept";
    if (isOverride)    os << " override";
    if (isPureVirtual) os << " = 0";
    os << "  [linkage: " << (linkage == Linkage::C ? "C" : "C++") << "]\n";
}

void CVarDecl::print(std::ostream& os, int indent) const {
    os << ind(indent) << "[var] ";
    if (!ns.empty())        os << ns << "::";
    if (!classOwner.empty()) os << classOwner << "::";
    if (isExtern)    os << "extern ";
    if (isStatic)    os << "static ";
    if (isConstexpr) os << "constexpr ";
    else if (isConst) os << "const ";
    if (isInline)    os << "inline ";
    os << type.toString() << ' ' << name;
    if (!defaultValue.empty()) os << " = " << defaultValue;
    os << '\n';
}

void CStructDecl::print(std::ostream& os, int indent) const {
    os << ind(indent);
    if (isTemplate) os << "[template] ";
    os << '[' << kind << "] ";
    if (!ns.empty()) os << ns << "::";
    os << name;
    if (!bases.empty()) {
        os << " : ";
        for (std::size_t i = 0; i < bases.size(); ++i) {
            if (i) os << ", ";
            os << bases[i];
        }
    }
    if (isDefinition && !fields.empty()) {
        os << " {\n";
        for (const auto& f : fields) {
            os << ind(indent + 1);
            if (f.isStatic) os << "static ";
            if (f.isConst)  os << "const ";
            os << f.type.toString() << ' ' << f.name;
            if (f.isBitField) os << " : " << f.bitWidth;
            if (!f.defaultValue.empty()) os << " = " << f.defaultValue;
            os << '\n';
        }
        os << ind(indent) << '}';
    }
    os << '\n';
}

void CEnumDecl::print(std::ostream& os, int indent) const {
    os << ind(indent) << "[enum";
    if (isClass) os << " class";
    os << "] ";
    if (!ns.empty()) os << ns << "::";
    os << name;
    if (!underlying.empty()) os << " : " << underlying;
    if (!enumerators.empty()) {
        os << " {\n";
        for (const auto& [k, v] : enumerators) {
            os << ind(indent + 1) << k;
            if (!v.empty()) os << " = " << v;
            os << '\n';
        }
        os << ind(indent) << '}';
    }
    os << '\n';
}

void CTypedef::print(std::ostream& os, int indent) const {
    os << ind(indent) << (isUsing ? "[using] " : "[typedef] ");
    if (!ns.empty()) os << ns << "::";
    os << alias << " = " << underlyingType.toString() << '\n';
}

void CHeaderResult::print(std::ostream& os) const {
    os << "=== CHeaderResult: " << filePath << " ===\n\n";

    if (!errors.empty()) {
        os << "-- Warnings / Errors --\n";
        for (const auto& e : errors) os << "  ! " << e << '\n';
        os << '\n';
    }

    os << "-- Structs / Classes (" << structs.size() << ") --\n";
    for (const auto& s : structs) s.print(os, 1);

    os << "\n-- Enums (" << enums.size() << ") --\n";
    for (const auto& e : enums) e.print(os, 1);

    os << "\n-- Typedefs / Using (" << typedefs.size() << ") --\n";
    for (const auto& t : typedefs) t.print(os, 1);

    os << "\n-- Variables (" << variables.size() << ") --\n";
    for (const auto& v : variables) v.print(os, 1);

    os << "\n-- Functions (" << functions.size() << ") --\n";
    for (const auto& f : functions) f.print(os, 1);

    os << '\n';
}

// =============================================================================
// Construction
// =============================================================================
CHeaderParser::CHeaderParser(std::string source, std::string filePath)
    : m_src(std::move(source)), m_filePath(std::move(filePath)) {}

// =============================================================================
// Low-level helpers
// =============================================================================
bool  CHeaderParser::eof()           const { return m_pos >= m_src.size(); }
bool  CHeaderParser::isIdentStart(char c) const { return std::isalpha((unsigned char)c) || c == '_'; }
bool  CHeaderParser::isIdentCont (char c) const { return std::isalnum((unsigned char)c) || c == '_'; }

char CHeaderParser::peek(int off) const {
    auto idx = m_pos + (std::size_t)off;
    return idx < m_src.size() ? m_src[idx] : '\0';
}

char CHeaderParser::consume() {
    char c = m_src[m_pos++];
    if (c == '\n') { ++m_line; m_col = 1; }
    else           { ++m_col; }
    return c;
}

void CHeaderParser::skipLineComment()  { while (!eof() && peek() != '\n') consume(); }
void CHeaderParser::skipBlockComment() {
    while (!eof()) {
        if (peek() == '*' && peek(1) == '/') { consume(); consume(); return; }
        consume();
    }
}

void CHeaderParser::skipWhitespace() {
    while (!eof()) {
        char c = peek();
        if (std::isspace((unsigned char)c)) { consume(); continue; }
        if (c == '/' && peek(1) == '/') { consume(); consume(); skipLineComment(); continue; }
        if (c == '/' && peek(1) == '*') { consume(); consume(); skipBlockComment(); continue; }
        break;
    }
}

bool CHeaderParser::skipPreprocessor() {
    if (peek() != '#') return false;
    consume();
    while (!eof()) {
        if (peek() == '\\' && peek(1) == '\n') { consume(); consume(); continue; }
        if (peek() == '\n') break;
        // skip string literals inside macros (rudimentary)
        if (peek() == '"') { consume(); while (!eof() && peek() != '"' && peek() != '\n') consume(); if (!eof() && peek() == '"') consume(); continue; }
        consume();
    }
    return true;
}

bool CHeaderParser::matchKw(const char* kw) {
    const std::size_t len = std::char_traits<char>::length(kw);
    if (m_pos + len > m_src.size()) return false;
    if (m_src.compare(m_pos, len, kw) != 0) return false;
    // Must be followed by a non-ident char (word boundary)
    char after = (m_pos + len < m_src.size()) ? m_src[m_pos + len] : '\0';
    if (isIdentCont(after)) return false;
    m_pos += len; m_col += (int)len;
    return true;
}

bool CHeaderParser::matchStr(const char* s) {
    const std::size_t len = std::char_traits<char>::length(s);
    if (m_pos + len > m_src.size()) return false;
    if (m_src.compare(m_pos, len, s) != 0) return false;
    m_pos += len; m_col += (int)len;
    return true;
}

std::string CHeaderParser::readIdentifier() {
    std::string id;
    if (eof() || !isIdentStart(peek())) return id;
    while (!eof() && isIdentCont(peek())) id += consume();
    return id;
}

std::string CHeaderParser::readScopedName() {
    std::string name = readIdentifier();
    if (name.empty()) return name;
    while (!eof()) {
        if (peek() == ':' && peek(1) == ':') {
            name += "::"; consume(); consume();
            skipWhitespace();
            name += readIdentifier();
        } else if (peek() == '<') {
            // Capture template args as part of type name: std::vector<int>
            std::string args;
            int depth = 0;
            std::size_t save = m_pos;
            bool ok = true;
            do {
                if (eof()) { ok = false; break; }
                char c = consume();
                args += c;
                if (c == '<') ++depth;
                else if (c == '>') { --depth; if (depth == 0) break; }
                else if (c == ';') { ok = false; break; }
            } while (depth > 0);
            if (ok) name += args;
            else { m_pos = save; break; }
        } else break;
    }
    return name;
}

std::string CHeaderParser::readStringLiteral() {
    if (peek() != '"') return {};
    consume();
    std::string s;
    while (!eof() && peek() != '"') {
        if (peek() == '\\') { consume(); if (!eof()) consume(); continue; }
        s += consume();
    }
    if (!eof()) consume();
    return s;
}

bool CHeaderParser::skipBalanced(char open, char close) {
    if (peek() != open) return false;
    consume();
    int depth = 1;
    while (!eof() && depth > 0) {
        char c = consume();
        if (c == open)  ++depth;
        if (c == close) --depth;
        if (c == '"') {
            while (!eof() && peek() != '"') {
                if (peek() == '\\') consume();
                consume();
            }
            if (!eof()) consume();
        }
        if (c == '/' && peek() == '/') { skipLineComment(); }
        if (c == '/' && peek() == '*') { consume(); skipBlockComment(); }
    }
    return true;
}

bool CHeaderParser::skipTemplateArgs() {
    if (peek() != '<') return false;
    int depth = 0;
    while (!eof()) {
        char c = consume();
        if (c == '<') ++depth;
        if (c == '>') { --depth; if (depth == 0) return true; }
        if (c == ';') return false; // malformed
    }
    return false;
}

std::string CHeaderParser::captureExpr(const char* sentinels) {
    std::string out;
    int parenDepth = 0, braceDepth = 0, bracketDepth = 0, angleDepth = 0;
    while (!eof()) {
        char c = peek();
        // Check sentinel only at depth 0
        if (parenDepth == 0 && braceDepth == 0 && bracketDepth == 0) {
            bool isSent = false;
            for (const char* p = sentinels; *p; ++p) if (c == *p) { isSent = true; break; }
            if (isSent) break;
        }
        consume();
        out += c;
        if      (c == '(') ++parenDepth;
        else if (c == ')') --parenDepth;
        else if (c == '{') ++braceDepth;
        else if (c == '}') --braceDepth;
        else if (c == '[') ++bracketDepth;
        else if (c == ']') --bracketDepth;
        // String literal inside expression
        else if (c == '"') {
            while (!eof() && peek() != '"') {
                if (peek() == '\\') { out += consume(); }
                out += consume();
            }
            if (!eof()) { out += consume(); } // closing "
        }
    }
    // Trim whitespace from captured expression
    while (!out.empty() && std::isspace((unsigned char)out.back())) out.pop_back();
    return out;
}

void CHeaderParser::emitError(CHeaderResult& result, const std::string& msg) {
    std::ostringstream os;
    os << m_filePath << ':' << m_line << ':' << m_col << ": " << msg;
    result.errors.push_back(os.str());
}

std::string CHeaderParser::joinNs(const std::string& a, const std::string& b) const {
    if (a.empty()) return b;
    if (b.empty()) return a;
    return a + "::" + b;
}

// =============================================================================
// Keyword classification
// =============================================================================
bool CHeaderParser::isTypeQualifier(const std::string& w) const {
    static const char* kw[] = { "const","volatile","restrict","__restrict","__restrict__",
                                 "signed","unsigned","__signed__", nullptr };
    for (int i = 0; kw[i]; ++i) if (w == kw[i]) return true;
    return false;
}
bool CHeaderParser::isStorageClass(const std::string& w) const {
    static const char* kw[] = { "static","extern","inline","__inline","__inline__",
                                 "register","auto","thread_local","_Thread_local",
                                 "constexpr","consteval","constinit","__declspec",
                                 "__attribute__","__cdecl","__stdcall","__fastcall",
                                 "__thiscall","__forceinline","__volatile__", nullptr };
    for (int i = 0; kw[i]; ++i) if (w == kw[i]) return true;
    return false;
}
bool CHeaderParser::isBuiltinType(const std::string& w) const {
    static const char* kw[] = {
        "void","char","short","int","long","float","double",
        "__int8","__int16","__int32","__int64",
        "int8_t","int16_t","int32_t","int64_t",
        "uint8_t","uint16_t","uint32_t","uint64_t",
        "size_t","ptrdiff_t","intptr_t","uintptr_t","ssize_t",
        "bool","_Bool","wchar_t","char8_t","char16_t","char32_t",
        "nullptr_t","uchar","ushort","uint","ulong",
        nullptr
    };
    for (int i = 0; kw[i]; ++i) if (w == kw[i]) return true;
    return false;
}
bool CHeaderParser::isFunctionSpecifier(const std::string& w) const {
    // These appear before a return type in a function decl
    static const char* kw[] = { "virtual","explicit","friend","override",nullptr };
    for (int i = 0; kw[i]; ++i) if (w == kw[i]) return true;
    return false;
}

// =============================================================================
// Type parsing
// =============================================================================
CType CHeaderParser::parseType(bool /*allowAuto*/) {
    CType t;
    bool seenBase = false;

    while (!eof()) {
        skipWhitespace();
        if (eof()) break;

        // Skip __attribute__((...)) / __declspec(...)
        if (peek() == '_' && peek(1) == '_') {
            std::size_t save = m_pos;
            std::string kw = readIdentifier();
            if (kw == "__attribute__" || kw == "__declspec" ||
                kw == "__cdecl" || kw == "__stdcall" || kw == "__fastcall" ||
                kw == "__thiscall" || kw == "__forceinline" || kw == "__volatile__") {
                skipWhitespace();
                if (peek() == '(') skipBalanced('(', ')');
                continue;
            }
            m_pos = save;
        }

        // Qualify keywords that are part of the type
        if (!seenBase) {
            std::size_t save = m_pos;
            std::string kw = readIdentifier();
            if (kw.empty()) break;

            if (isTypeQualifier(kw)) { t.qualifiers.push_back(kw); continue; }

            if (kw == "struct" || kw == "class" || kw == "union" || kw == "enum") {
                t.base = kw;
                skipWhitespace();
                // optional "class" sub-keyword for enum
                if (kw == "enum" && (peek() == 'c' || peek() == 's')) {
                    std::size_t s2 = m_pos;
                    std::string sub = readIdentifier();
                    if (sub == "class" || sub == "struct") { t.base += ' '; t.base += sub; skipWhitespace(); }
                    else m_pos = s2;
                }
                if (!eof() && isIdentStart(peek())) {
                    t.base += ' ';
                    t.base += readScopedName();
                }
                seenBase = true;
                break;
            }

            if (isStorageClass(kw) || isFunctionSpecifier(kw)) continue; // skip, handled by caller

            if (isBuiltinType(kw)) {
                if (t.base.empty()) t.base = kw;
                else { t.base += ' '; t.base += kw; } // e.g. "long long", "unsigned int"
                seenBase = true;
                // continue to allow "long long", "unsigned long", etc.
                continue;
            }

            // Identifier (typedef / class name / scoped name)
            // Put back and use readScopedName to capture "std::vector<int>" etc.
            m_pos = save;
            std::string name = readScopedName();
            if (name.empty()) break;
            t.base = name;
            seenBase = true;
            break;
        }

        // Already have a base — check for additional type keyword combinations
        // (long long, unsigned long, etc.)
        {
            std::size_t save = m_pos;
            std::string kw = readIdentifier();
            if (!kw.empty() && isBuiltinType(kw)) {
                // e.g. "long" + "long", "long" + "int"
                t.base += ' '; t.base += kw;
                continue;
            }
            m_pos = save;
            break;
        }
    }

    return t;
}

void CHeaderParser::parseDeclarator(CType& base, std::string& outName) {
    skipWhitespace();
    // Consume pointers and references
    while (!eof()) {
        if (peek() == '*') {
            consume(); ++base.pointers; skipWhitespace();
            // const after * (e.g. char * const)
            if (matchKw("const")) { base.isConstPtr = true; skipWhitespace(); }
            continue;
        }
        if (peek() == '&' && peek(1) == '&') {
            consume(); consume(); base.isRvalRef = true; skipWhitespace(); break;
        }
        if (peek() == '&') {
            consume(); base.isRef = true; skipWhitespace(); break;
        }
        break;
    }

    // Name (possibly skipped for anonymous / abstract declarators)
    if (!eof() && isIdentStart(peek())) {
        outName = readIdentifier();
    }

    skipWhitespace();

    // Array suffix
    while (!eof() && peek() == '[') {
        consume();
        base.isArray = true;
        base.arraySize = captureExpr("]");
        if (!eof() && peek() == ']') consume();
        skipWhitespace();
    }
}

// =============================================================================
// Parameter list
// =============================================================================
CParam CHeaderParser::parseParam() {
    CParam p;
    skipWhitespace();
    if (peek() == '.' && peek(1) == '.' && peek(2) == '.') {
        p.isVarArg = true; consume(); consume(); consume();
        return p;
    }
    p.type = parseType();
    // Function-pointer parameter: type (*name)(params)
    if (p.type.base.empty() || peek() == '(') {
        // try as function pointer: already consumed nothing useful
    }
    parseDeclarator(p.type, p.name);

    skipWhitespace();
    // Default argument
    if (peek() == '=') {
        consume();
        skipWhitespace();
        p.defaultValue = captureExpr(",)");
    }
    return p;
}

std::vector<CParam> CHeaderParser::parseParamList(bool& isVariadic) {
    std::vector<CParam> params;
    skipWhitespace();
    if (peek() == ')') return params; // ()

    // (void) -> empty params
    {
        std::size_t save = m_pos;
        std::string kw = readIdentifier();
        if (kw == "void") {
            skipWhitespace();
            if (peek() == ')') return params;
        }
        m_pos = save;
    }

    while (!eof() && peek() != ')') {
        skipWhitespace();
        if (peek() == ')') break;

        CParam p = parseParam();
        if (p.isVarArg) { isVariadic = true; params.push_back(std::move(p)); break; }
        params.push_back(std::move(p));

        skipWhitespace();
        if (peek() == ',') { consume(); skipWhitespace(); }
        else break;
    }
    return params;
}

// =============================================================================
// Trailing function qualifiers: const noexcept override = 0 = delete = default
// =============================================================================
void CHeaderParser::parseTrailingFnQualifiers(CFunctionDecl& fn) {
    while (!eof()) {
        skipWhitespace();
        if (matchKw("const"))    { fn.isConst    = true; continue; }
        if (matchKw("noexcept")) { fn.isNoexcept = true; skipWhitespace(); if (peek()=='(') skipBalanced('(',')'); continue; }
        if (matchKw("override")) { fn.isOverride = true; continue; }
        if (matchKw("final"))    { continue; }
        if (matchKw("throw"))    { skipWhitespace(); if (peek()=='(') skipBalanced('(',')'); continue; }
        if (peek() == '-' && peek(1) == '>') {
            // trailing return type: -> Type
            consume(); consume();
            skipWhitespace();
            fn.returnType = parseType();
            std::string dummy;
            parseDeclarator(fn.returnType, dummy);
            continue;
        }
        if (peek() == '=') {
            consume();
            skipWhitespace();
            if (matchKw("0"))       { fn.isPureVirtual = true; break; }
            if (matchKw("delete"))  { /* deleted fn */ break; }
            if (matchKw("default")) { /* defaulted fn */ break; }
            break;
        }
        break;
    }
}

// =============================================================================
// Struct / class body
// =============================================================================
void CHeaderParser::parseClassBody(CHeaderResult& result,
                                   CStructDecl& decl,
                                   const std::string& currentNs,
                                   Linkage linkage) {
    // Default access: struct/union=public, class=private (we don't enforce, just collect all)
    const std::string classNs = joinNs(currentNs, decl.name);

    while (!eof() && peek() != '}') {
        skipWhitespace();
        if (peek() == '}') break;
        if (peek() == '#') { skipPreprocessor(); continue; }
        if (peek() == ';') { consume(); continue; }

        // Access specifiers
        {
            std::size_t save = m_pos;
            std::string kw = readIdentifier();
            if (kw == "public" || kw == "private" || kw == "protected") {
                skipWhitespace();
                if (peek() == ':') { consume(); } // eat ':'
                continue;
            }
            m_pos = save;
        }

        // Nested struct/class/union
        {
            std::size_t save = m_pos;
            std::string kw = readIdentifier();
            if (kw == "struct" || kw == "class" || kw == "union") {
                m_pos = save; // re-parse via tryParseDecl
                tryParseDecl(result, classNs, decl.name, linkage);
                continue;
            }
            if (kw == "enum") {
                m_pos = save;
                tryParseDecl(result, classNs, decl.name, linkage);
                continue;
            }
            if (kw == "template") {
                m_pos = save;
                parseTemplate(result, classNs);
                continue;
            }
            if (kw == "using") {
                m_pos = save;
                parseUsing(result, classNs);
                continue;
            }
            if (kw == "typedef") {
                m_pos = save;
                parseTypedef(result, classNs);
                continue;
            }
            if (kw == "friend") {
                // Skip friend declarations
                while (!eof() && peek() != ';' && peek() != '{') consume();
                if (peek() == '{') skipBalanced('{', '}');
                if (peek() == ';') consume();
                continue;
            }
            m_pos = save;
        }

        // Try to parse a member declaration (field or method)
        bool isStatic    = false;
        bool isVirtual   = false;
        bool isInline    = false;
        bool isExplicit  = false;
        bool isConstexpr = false;
        bool isConst     = false;

        // Consume leading specifiers
        while (true) {
            skipWhitespace();
            std::size_t save = m_pos;
            std::string kw = readIdentifier();
            if      (kw == "static")    { isStatic    = true; continue; }
            else if (kw == "virtual")   { isVirtual   = true; continue; }
            else if (kw == "inline" || kw == "__inline" || kw == "__inline__") { isInline = true; continue; }
            else if (kw == "explicit")  { isExplicit  = true; continue; }
            else if (kw == "constexpr") { isConstexpr = true; continue; }
            else if (kw == "const")     { isConst     = true; continue; } // leading const
            else if (kw == "__declspec" || kw == "__attribute__") {
                skipWhitespace(); if (peek()=='(') skipBalanced('(',')'); continue;
            }
            else if (kw == "operator") {
                // operator overload — capture op symbol then parse as function
                skipWhitespace();
                std::string opSym;
                while (!eof() && peek() != '(' && !std::isspace((unsigned char)peek()))
                    opSym += consume();
                skipWhitespace();
                if (peek() == '(') {
                    consume();
                    bool isVar = false;
                    CFunctionDecl fn;
                    fn.ns        = currentNs;
                    fn.classOwner= decl.name;
                    fn.name      = "operator" + opSym;
                    fn.isOperator= true;
                    fn.isStatic  = isStatic;
                    fn.isVirtual = isVirtual;
                    fn.isInline  = isInline;
                    fn.isConstexpr=isConstexpr;
                    fn.linkage   = linkage;
                    fn.params    = parseParamList(isVar);
                    fn.isVariadic= isVar;
                    if (!eof() && peek()==')') consume();
                    parseTrailingFnQualifiers(fn);
                    if (peek() == '{') skipBalanced('{', '}'); // skip body
                    else if (peek() == ';') consume();
                    result.functions.push_back(std::move(fn));
                }
                break;
            }
            m_pos = save;
            break;
        }
        (void)isExplicit;

        skipWhitespace();

        // Constructor / destructor detection:
        // If the next identifier matches the class name → ctor
        // If '~' → dtor
        if (peek() == '~') {
            consume();
            skipWhitespace();
            std::string dname = readIdentifier();
            skipWhitespace();
            if (peek() == '(') {
                consume();
                bool isVar = false;
                CFunctionDecl fn;
                fn.ns          = currentNs;
                fn.classOwner  = decl.name;
                fn.name        = "~" + dname;
                fn.isDestructor= true;
                fn.isVirtual   = isVirtual;
                fn.isInline    = isInline;
                fn.linkage     = linkage;
                fn.params      = parseParamList(isVar);
                if (!eof() && peek()==')') consume();
                parseTrailingFnQualifiers(fn);
                if (peek() == '{') skipBalanced('{', '}');
                else if (peek() == ';') consume();
                result.functions.push_back(std::move(fn));
            }
            continue;
        }

        CType baseType;
        // Check for ctor: identifier == class name followed by '('
        {
            std::size_t save = m_pos;
            std::string id = readIdentifier();
            skipWhitespace();
            if (id == decl.name && peek() == '(') {
                consume();
                bool isVar = false;
                CFunctionDecl fn;
                fn.ns           = currentNs;
                fn.classOwner   = decl.name;
                fn.name         = id;
                fn.isConstructor= true;
                fn.isExplicit   = false; // already consumed 'explicit' above
                fn.isInline     = isInline;
                fn.isConstexpr  = isConstexpr;
                fn.linkage      = linkage;
                fn.params       = parseParamList(isVar);
                fn.isVariadic   = isVar;
                if (!eof() && peek()==')') consume();
                parseTrailingFnQualifiers(fn);
                if (peek() == '{') skipBalanced('{', '}');
                else if (peek() == ';') consume();
                result.functions.push_back(std::move(fn));
                continue;
            }
            m_pos = save;
        }

        baseType = parseType();
        if (isConst && !baseType.qualifiers.empty()) { /* already in qualifiers */ }
        if (isConst) baseType.qualifiers.push_back("const");

        if (baseType.base.empty()) {
            // Nothing parseable — skip line
            while (!eof() && peek() != ';' && peek() != '}') consume();
            if (peek() == ';') consume();
            continue;
        }

        skipWhitespace();

        // operator overload with return type already parsed
        if (matchKw("operator")) {
            skipWhitespace();
            std::string opSym;
            while (!eof() && peek() != '(' && !std::isspace((unsigned char)peek()))
                opSym += consume();
            skipWhitespace();
            if (peek() == '(') {
                consume();
                bool isVar = false;
                CFunctionDecl fn;
                fn.ns         = currentNs;
                fn.classOwner = decl.name;
                fn.name       = "operator" + opSym;
                fn.returnType = baseType;
                fn.isOperator = true;
                fn.isStatic   = isStatic;
                fn.isVirtual  = isVirtual;
                fn.isInline   = isInline;
                fn.isConstexpr= isConstexpr;
                fn.linkage    = linkage;
                fn.params     = parseParamList(isVar);
                fn.isVariadic = isVar;
                if (!eof() && peek()==')') consume();
                parseTrailingFnQualifiers(fn);
                if (peek() == '{') skipBalanced('{', '}');
                else if (peek() == ';') consume();
                result.functions.push_back(std::move(fn));
            }
            continue;
        }

        // Declarator
        CType declType = baseType;
        std::string name;
        parseDeclarator(declType, name);
        skipWhitespace();

        if (!eof() && peek() == '(') {
            // Method
            consume();
            bool isVar = false;
            CFunctionDecl fn;
            fn.ns         = currentNs;
            fn.classOwner = decl.name;
            fn.name       = name;
            fn.returnType = declType;
            fn.isStatic   = isStatic;
            fn.isVirtual  = isVirtual;
            fn.isInline   = isInline;
            fn.isConstexpr= isConstexpr;
            fn.linkage    = linkage;
            fn.params     = parseParamList(isVar);
            fn.isVariadic = isVar;
            if (!eof() && peek()==')') consume();
            parseTrailingFnQualifiers(fn);
            // Skip body if inline definition
            skipWhitespace();
            if (peek() == '{') skipBalanced('{', '}');
            else if (peek() == ';') consume();
            result.functions.push_back(std::move(fn));
        } else {
            // Field
            CField field;
            field.type     = declType;
            field.name     = name;
            field.isStatic = isStatic;
            field.isConst  = isConstexpr ||
                             std::find(declType.qualifiers.begin(), declType.qualifiers.end(), "const")
                                 != declType.qualifiers.end();
            // Bit field
            if (peek() == ':') {
                consume(); skipWhitespace();
                field.isBitField = true;
                std::string bw = captureExpr(";,}");
                try { field.bitWidth = std::stoi(bw); } catch (...) {}
            }
            // Default member initializer
            if (peek() == '=') {
                consume(); skipWhitespace();
                field.defaultValue = captureExpr(";,}");
            }
            if (!field.name.empty()) decl.fields.push_back(std::move(field));
            if (peek() == ';') consume();
        }
    }
}

// =============================================================================
// parseClassOrStruct
// =============================================================================
void CHeaderParser::parseClassOrStruct(CHeaderResult& result,
                                       const std::string& currentNs,
                                       const std::string& keyword) {
    CStructDecl decl;
    decl.ns   = currentNs;
    decl.kind = keyword;
    skipWhitespace();

    // Optional name
    if (!eof() && isIdentStart(peek()))
        decl.name = readIdentifier();

    skipWhitespace();

    // Base list: struct Foo : public Bar, Baz { ... }
    if (peek() == ':') {
        consume();
        while (!eof() && peek() != '{' && peek() != ';') {
            skipWhitespace();
            // skip access specifier
            std::size_t save = m_pos;
            std::string kw = readIdentifier();
            if (kw == "public" || kw == "private" || kw == "protected" || kw == "virtual") {
                skipWhitespace(); continue;
            }
            m_pos = save;
            std::string base = readScopedName();
            if (!base.empty()) decl.bases.push_back(base);
            skipWhitespace();
            if (peek() == ',') consume();
        }
        skipWhitespace();
    }

    if (peek() == ';') {
        // Forward declaration
        consume();
        result.structs.push_back(decl);
        return;
    }

    if (peek() != '{') {
        result.structs.push_back(decl);
        return;
    }
    consume(); // '{'
    decl.isDefinition = true;
    parseClassBody(result, decl, currentNs, m_linkage);
    if (!eof() && peek() == '}') consume();

    skipWhitespace();

    // Optional declarator after the body: struct Foo { ... } varName;
    if (!eof() && peek() != ';') {
        CType t; t.base = keyword + ' ' + decl.name;
        std::string varName;
        parseDeclarator(t, varName);
        if (!varName.empty()) {
            CVarDecl v; v.ns = currentNs; v.type = t; v.name = varName;
            result.variables.push_back(v);
        }
    }
    if (peek() == ';') consume();

    result.structs.push_back(std::move(decl));
}

// =============================================================================
// parseEnum
// =============================================================================
void CHeaderParser::parseEnum(CHeaderResult& result, const std::string& currentNs) {
    CEnumDecl decl;
    decl.ns = currentNs;
    skipWhitespace();

    // enum class / enum struct
    {
        std::size_t save = m_pos;
        std::string kw = readIdentifier();
        if (kw == "class" || kw == "struct") { decl.isClass = true; skipWhitespace(); }
        else m_pos = save;
    }

    if (!eof() && isIdentStart(peek())) {
        decl.name = readIdentifier();
        skipWhitespace();
    }

    // underlying type: enum class E : uint32_t
    if (peek() == ':') {
        consume();
        skipWhitespace();
        decl.underlying = readScopedName();
        skipWhitespace();
    }

    if (peek() == ';') { consume(); result.enums.push_back(decl); return; }
    if (peek() != '{') { result.enums.push_back(decl); return; }
    consume(); // '{'

    while (!eof() && peek() != '}') {
        skipWhitespace();
        if (peek() == '}') break;
        if (peek() == '#') { skipPreprocessor(); continue; }
        if (peek() == ';') { consume(); continue; }

        std::string ename = readIdentifier();
        if (ename.empty()) { consume(); continue; }

        skipWhitespace();
        std::string value;
        if (peek() == '=') {
            consume(); skipWhitespace();
            value = captureExpr(",}");
        }
        decl.enumerators.emplace_back(std::move(ename), std::move(value));

        skipWhitespace();
        if (peek() == ',') consume();
    }

    if (peek() == '}') consume();
    if (peek() == ';') consume();
    result.enums.push_back(std::move(decl));
}

// =============================================================================
// parseTemplate
// =============================================================================
void CHeaderParser::parseTemplate(CHeaderResult& result, const std::string& currentNs) {
    skipWhitespace();
    // Skip template parameter list
    if (peek() == '<') skipTemplateArgs();
    skipWhitespace();

    // Now parse whatever follows (class, function, variable decl)
    // We mark it as a template in tryParseDecl by flagging isTemplate
    // Simple approach: just parse it normally and set isTemplate on last added fn/struct
    std::size_t fnsBefore    = result.functions.size();
    std::size_t structsBefore= result.structs.size();

    tryParseDecl(result, currentNs, "", m_linkage);

    // Mark newly added declarations as templates
    for (std::size_t i = fnsBefore; i < result.functions.size(); ++i)
        result.functions[i].isTemplate = true;
    for (std::size_t i = structsBefore; i < result.structs.size(); ++i)
        result.structs[i].isTemplate = true;
}

// =============================================================================
// parseUsing
// =============================================================================
void CHeaderParser::parseUsing(CHeaderResult& result, const std::string& currentNs) {
    skipWhitespace();

    // 'using namespace Foo;' — skip
    {
        std::size_t save = m_pos;
        std::string kw = readIdentifier();
        if (kw == "namespace") {
            while (!eof() && peek() != ';') consume();
            if (peek() == ';') consume();
            return;
        }
        m_pos = save;
    }

    // 'using Alias = Type;'
    std::string alias = readIdentifier();
    skipWhitespace();

    if (peek() == '=') {
        consume(); skipWhitespace();
        CType t = parseType();
        std::string dummy;
        parseDeclarator(t, dummy);
        CTypedef td;
        td.ns              = currentNs;
        td.alias           = alias;
        td.underlyingType  = t;
        td.isUsing         = true;
        result.typedefs.push_back(std::move(td));
    }
    // else 'using Base::member;' — skip
    while (!eof() && peek() != ';') consume();
    if (peek() == ';') consume();
}

// =============================================================================
// parseTypedef
// =============================================================================
void CHeaderParser::parseTypedef(CHeaderResult& result, const std::string& currentNs) {
    skipWhitespace();

    // typedef struct/union/enum
    {
        std::size_t save = m_pos;
        std::string kw = readIdentifier();
        if (kw == "struct" || kw == "union") {
            // Parse the struct normally (adds to result.structs)
            std::size_t before = result.structs.size();
            parseClassOrStruct(result, currentNs, kw);
            // The last token consumed was '}' — now read optional alias
            skipWhitespace();
            if (peek() != ';') {
                std::string alias = readIdentifier();
                skipWhitespace();
                if (!alias.empty() && before < result.structs.size()) {
                    CTypedef td;
                    td.ns   = currentNs;
                    td.alias= alias;
                    auto& s = result.structs.back();
                    td.underlyingType.base = kw + ' ' + s.name;
                    result.typedefs.push_back(td);
                }
            }
            if (peek() == ';') consume();
            return;
        }
        if (kw == "enum") {
            parseEnum(result, currentNs);
            skipWhitespace();
            if (peek() != ';') {
                std::string alias = readIdentifier();
                if (!alias.empty() && !result.enums.empty()) {
                    CTypedef td;
                    td.ns   = currentNs;
                    td.alias= alias;
                    td.underlyingType.base = "enum " + result.enums.back().name;
                    result.typedefs.push_back(td);
                }
            }
            if (peek() == ';') consume();
            return;
        }
        m_pos = save;
    }

    CType t = parseType();
    skipWhitespace();

    // Function pointer typedef: typedef void (*Callback)(int, char*);
    if (peek() == '(') {
        consume(); skipWhitespace();
        if (peek() == '*') consume(); // '*'
        skipWhitespace();
        std::string alias = readIdentifier();
        skipWhitespace();
        if (peek() == ')') consume();
        skipWhitespace();
        bool isVar = false;
        if (peek() == '(') { consume(); parseParamList(isVar); if (peek()==')') consume(); }
        if (!alias.empty()) {
            CTypedef td;
            td.ns   = currentNs;
            td.alias= alias;
            t.base += "(*)"; // mark as fn ptr
            td.underlyingType = t;
            result.typedefs.push_back(std::move(td));
        }
        while (!eof() && peek() != ';') consume();
        if (peek() == ';') consume();
        return;
    }

    // typedef Type Alias;
    std::string alias;
    parseDeclarator(t, alias);
    if (!alias.empty()) {
        CTypedef td; td.ns = currentNs; td.alias = alias; td.underlyingType = t;
        result.typedefs.push_back(std::move(td));
    }
    if (peek() == ';') consume();
}

// =============================================================================
// parseNamespace
// =============================================================================
void CHeaderParser::parseNamespace(CHeaderResult& result, const std::string& parentNs) {
    skipWhitespace();
    std::string nsName;

    // Anonymous namespace
    if (peek() == '{') {
        nsName = "";
    } else if (isIdentStart(peek())) {
        nsName = readIdentifier();
        skipWhitespace();
        // Nested: namespace A::B (C++17)
        while (peek() == ':' && peek(1) == ':') {
            consume(); consume(); skipWhitespace();
            nsName += "::";
            nsName += readIdentifier();
            skipWhitespace();
        }
        // namespace alias: namespace X = Y::Z;
        if (peek() == '=') {
            while (!eof() && peek() != ';') consume();
            if (peek() == ';') consume();
            return;
        }
    }

    if (peek() != '{') {
        // malformed
        while (!eof() && peek() != '}' && peek() != ';') consume();
        return;
    }
    consume(); // '{'

    const std::string fullNs = joinNs(parentNs, nsName);
    parseScope(result, fullNs, "", m_linkage, '}');

    if (!eof() && peek() == '}') consume();
}

// =============================================================================
// parseExternSpec  — extern "C" { } or extern "C++" { }
// =============================================================================
void CHeaderParser::parseExternSpec(CHeaderResult& result,
                                    const std::string& currentNs,
                                    const std::string& currentClass) {
    skipWhitespace();
    if (peek() != '"') return;
    std::string lang = readStringLiteral();
    Linkage lnk = (lang == "C") ? Linkage::C : Linkage::Cpp;

    // Save and update m_linkage so nested methods that read it are consistent
    const Linkage savedLinkage = m_linkage;
    m_linkage = lnk;

    skipWhitespace();
    if (peek() == '{') {
        consume();
        parseScope(result, currentNs, currentClass, lnk, '}');
        if (!eof() && peek() == '}') consume();
    } else {
        // single declaration
        tryParseDecl(result, currentNs, currentClass, lnk);
    }

    m_linkage = savedLinkage;
}

// =============================================================================
// tryParseDecl  — try to parse one declaration at current position
// =============================================================================
bool CHeaderParser::tryParseDecl(CHeaderResult& result,
                                  const std::string& currentNs,
                                  const std::string& currentClass,
                                  Linkage linkage) {
    skipWhitespace();
    if (eof()) return false;

    if (peek() == '#') { skipPreprocessor(); return true; }
    if (peek() == ';') { consume(); return true; }

    // Collect leading specifiers (before knowing what kind of decl this is)
    bool isStatic    = false;
    bool isExtern    = false;
    bool isInline    = false;
    bool isVirtual   = false;
    bool isConstexpr = false;
    bool isConst     = false; // leading const (before type)
    bool isVolatile  = false;

    // ---- dispatch on leading keyword ----
    {
        std::size_t save = m_pos;
        std::string kw = readIdentifier();

        if (kw == "namespace") { parseNamespace(result, currentNs); return true; }
        if (kw == "extern")    { parseExternSpec(result, currentNs, currentClass); return true; }
        if (kw == "struct" || kw == "class") { parseClassOrStruct(result, currentNs, kw); return true; }
        if (kw == "union")     { parseClassOrStruct(result, currentNs, "union"); return true; }
        if (kw == "enum")      { parseEnum(result, currentNs); return true; }
        if (kw == "template")  { parseTemplate(result, currentNs); return true; }
        if (kw == "using")     { parseUsing(result, currentNs); return true; }
        if (kw == "typedef")   { parseTypedef(result, currentNs); return true; }

        if (kw == "static")    { isStatic    = true; }
        else if (kw == "inline" || kw == "__inline__") { isInline = true; }
        else if (kw == "virtual")   { isVirtual   = true; }
        else if (kw == "constexpr") { isConstexpr = true; }
        else if (kw == "const")     { isConst     = true; }
        else if (kw == "volatile")  { isVolatile  = true; }
        else { m_pos = save; } // not a recognised keyword — fall through to type parsing
    }
    (void)isVolatile;

    // Consume additional leading specifiers in any order
    bool keepGoing = true;
    while (keepGoing) {
        skipWhitespace();
        std::size_t save = m_pos;
        std::string kw = readIdentifier();
        if      (kw == "static")    isStatic    = true;
        else if (kw == "inline" || kw == "__inline__") isInline = true;
        else if (kw == "virtual")   isVirtual   = true;
        else if (kw == "constexpr") isConstexpr = true;
        else if (kw == "const")     isConst     = true;
        else if (kw == "extern")    isExtern    = true;
        else if (kw == "explicit")  { /* eat */ }
        else if (kw == "__declspec" || kw == "__attribute__") {
            skipWhitespace(); if (peek()=='(') skipBalanced('(',')');
        }
        else { m_pos = save; keepGoing = false; }
    }

    skipWhitespace();

    // Operator at top level (rare, but possible)
    if (matchKw("operator")) {
        skipWhitespace();
        std::string opSym;
        while (!eof() && peek() != '(' && !std::isspace((unsigned char)peek()))
            opSym += consume();
        if (peek() == '(') {
            consume();
            bool isVar = false;
            CFunctionDecl fn;
            fn.ns         = currentNs;
            fn.classOwner = currentClass;
            fn.name       = "operator" + opSym;
            fn.isOperator = true;
            fn.linkage    = linkage;
            fn.params     = parseParamList(isVar);
            fn.isVariadic = isVar;
            if (peek()==')') consume();
            parseTrailingFnQualifiers(fn);
            if (peek()=='{') skipBalanced('{','}');
            else if (peek()==';') consume();
            result.functions.push_back(std::move(fn));
        }
        return true;
    }

    CType baseType = parseType();
    if (isConst) baseType.qualifiers.push_back("const");

    if (baseType.base.empty()) {
        // Couldn't parse anything useful — skip to next ';' or '}'
        while (!eof() && peek() != ';' && peek() != '}') consume();
        if (peek() == ';') consume();
        return true;
    }

    skipWhitespace();

    // Possibly another 'operator' keyword after the return type
    if (matchKw("operator")) {
        skipWhitespace();
        std::string opSym;
        while (!eof() && peek() != '(' && !std::isspace((unsigned char)peek()))
            opSym += consume();
        if (peek() == '(') {
            consume();
            bool isVar = false;
            CFunctionDecl fn;
            fn.ns         = currentNs;
            fn.classOwner = currentClass;
            fn.name       = "operator" + opSym;
            fn.returnType = baseType;
            fn.isOperator = true;
            fn.isStatic   = isStatic;
            fn.isInline   = isInline;
            fn.isConstexpr= isConstexpr;
            fn.linkage    = linkage;
            fn.params     = parseParamList(isVar);
            fn.isVariadic = isVar;
            if (peek()==')') consume();
            parseTrailingFnQualifiers(fn);
            if (peek()=='{') skipBalanced('{','}');
            else if (peek()==';') consume();
            result.functions.push_back(std::move(fn));
        }
        return true;
    }

    // Parse declarator(s) — there may be multiple: int a, *b, c[4];
    bool first = true;
    while (true) {
        if (!first) {
            skipWhitespace();
            if (peek() != ',') break;
            consume(); skipWhitespace();
        }
        first = false;

        CType declType = baseType;
        std::string name;
        parseDeclarator(declType, name);
        skipWhitespace();

        // Function pointer declarator: type (*name)(params)
        if (name.empty() && peek() == '(') {
            std::size_t save = m_pos;
            consume(); skipWhitespace();
            if (peek() == '*') {
                consume(); skipWhitespace();
                name = readIdentifier(); skipWhitespace();
                if (peek() == ')') consume(); skipWhitespace();
                // This is a function pointer variable, not a function decl.
                // Fall through to variable handling below.
            } else {
                m_pos = save;
            }
        }

        if (!eof() && peek() == '(') {
            consume(); // consume '('
            bool isVar = false;
            auto params = parseParamList(isVar);
            skipWhitespace();
            if (peek() == ')') consume();
            skipWhitespace();

            // __attribute__((...)) after params
            while (!eof() && peek() == '_' && peek(1) == '_') {
                std::size_t save = m_pos;
                std::string kw2 = readIdentifier();
                if (kw2 == "__attribute__" || kw2 == "__declspec") {
                    skipWhitespace(); if (peek()=='(') skipBalanced('(',')');
                } else { m_pos = save; break; }
                skipWhitespace();
            }

            CFunctionDecl fn;
            fn.ns         = currentNs;
            fn.classOwner = currentClass;
            fn.name       = name;
            fn.returnType = declType;
            fn.params     = std::move(params);
            fn.isVariadic = isVar;
            fn.isStatic   = isStatic;
            fn.isVirtual  = isVirtual;
            fn.isInline   = isInline;
            fn.isConstexpr= isConstexpr;
            fn.linkage    = linkage;
            parseTrailingFnQualifiers(fn);

            skipWhitespace();
            if (peek() == '{') skipBalanced('{', '}'); // inline body
            else if (peek() == ';') consume();

            if (!name.empty()) result.functions.push_back(std::move(fn));
            return true; // function decl is never followed by another declarator
        }

        // Variable declaration
        CVarDecl v;
        v.ns         = currentNs;
        v.classOwner = currentClass;
        v.type       = declType;
        v.name       = name;
        v.isExtern   = isExtern;
        v.isStatic   = isStatic;
        v.isInline   = isInline;
        v.isConstexpr= isConstexpr;
        v.isConst    = isConst ||
                       std::find(declType.qualifiers.begin(), declType.qualifiers.end(), "const")
                           != declType.qualifiers.end();

        // Default value / initializer
        skipWhitespace();
        if (peek() == '=') {
            consume(); skipWhitespace();
            v.defaultValue = captureExpr(",;");
        }
        // Skip brace initializer
        if (peek() == '{') { skipBalanced('{', '}'); }

        if (!v.name.empty()) result.variables.push_back(std::move(v));

        skipWhitespace();
        if (peek() == ',') continue; // another declarator
        break;
    }

    if (peek() == ';') consume();
    return true;
}

// =============================================================================
// parseScope  — main loop
// =============================================================================
void CHeaderParser::parseScope(CHeaderResult& result,
                               const std::string& currentNs,
                               const std::string& currentClass,
                               Linkage linkage,
                               char terminator) {
    while (!eof()) {
        skipWhitespace();
        if (eof()) break;
        if (terminator != '\0' && peek() == (char)terminator) break;
        if (peek() == '}' && terminator == '}') break;

        tryParseDecl(result, currentNs, currentClass, linkage);
    }
}

// =============================================================================
// Public entry point
// =============================================================================
CHeaderResult CHeaderParser::parse() {
    CHeaderResult result;
    result.filePath = m_filePath;
    parseScope(result, "", "", Linkage::Cpp, '\0');
    return result;
}

} // namespace extern_support
} // namespace Omniscript

// =============================================================================
// Optional standalone driver  —  compile with -DCHEADERPARSER_STANDALONE
// =============================================================================
#ifdef CHEADERPARSER_STANDALONE
#include <fstream>
#include <sstream>

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: cheaderparser <header.h|hpp> [header2 ...]\n";
        return 1;
    }
    for (int i = 1; i < argc; ++i) {
        std::ifstream f(argv[i]);
        if (!f) { std::cerr << "Cannot open: " << argv[i] << '\n'; continue; }
        std::ostringstream ss; ss << f.rdbuf();
        Omniscript::extern_support::CHeaderParser p(ss.str(), argv[i]);
        auto r = p.parse();
        r.print(std::cout);
    }
    return 0;
}
#endif