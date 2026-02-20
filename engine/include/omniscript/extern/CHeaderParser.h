#pragma once
// engine/include/omniscript/extern/CHeaderParser.h
//
// C and C++ header parser for OmniScript's `extern` FFI system.
// Handles both plain C headers and C++ headers (namespaces, classes,
// templates, overloads, default params, references, using, enum class, etc.).
//
// Usage:
//   CHeaderParser p(source, "mylib.hpp");
//   auto result = p.parse();
//   result.print(std::cout);

#include <cstdint>
#include <iosfwd>
#include <string>
#include <vector>

namespace Omniscript {
namespace extern_support {

// -----------------------------------------------------------------------
// Linkage mode
// -----------------------------------------------------------------------
enum class Linkage : std::uint8_t { C, Cpp };

// -----------------------------------------------------------------------
// Parsed C/C++ type
// -----------------------------------------------------------------------
struct CType {
    std::vector<std::string> qualifiers; // const, volatile, restrict, signed, unsigned
    std::string              base;       // int, char, MyClass, std::vector<int>, ...
    int                      pointers  = 0;
    bool                     isRef     = false; // &
    bool                     isRvalRef = false; // &&
    bool                     isArray   = false;
    std::string              arraySize;
    bool                     isConstPtr = false; // char * const (const after '*')

    std::string toString() const;
};

// -----------------------------------------------------------------------
// A single function parameter
// -----------------------------------------------------------------------
struct CParam {
    CType       type;
    std::string name;
    std::string defaultValue; // raw source text of default argument (C++)
    bool        isVarArg = false;
};

// -----------------------------------------------------------------------
// A parsed function / method prototype
// -----------------------------------------------------------------------
struct CFunctionDecl {
    std::string           ns;           // enclosing namespace, e.g. "std::chrono"
    std::string           classOwner;   // non-empty if it's a method / ctor / dtor
    std::string           name;
    CType                 returnType;
    std::vector<CParam>   params;
    bool                  isVariadic    = false;
    bool                  isStatic      = false;
    bool                  isVirtual     = false;
    bool                  isInline      = false;
    bool                  isConst       = false; // trailing const qualifier (method)
    bool                  isNoexcept    = false;
    bool                  isOverride    = false;
    bool                  isPureVirtual = false;
    bool                  isConstructor = false;
    bool                  isDestructor  = false;
    bool                  isOperator    = false;
    bool                  isTemplate    = false;
    bool                  isConstexpr   = false;
    bool                  isExplicit    = false;
    Linkage               linkage       = Linkage::Cpp;

    // "ns::classOwner::name" (omits empty parts)
    std::string qualifiedName() const;
    void print(std::ostream& os, int indent = 0) const;
};

// -----------------------------------------------------------------------
// A parsed variable / constant declaration
// -----------------------------------------------------------------------
struct CVarDecl {
    std::string ns;
    std::string classOwner;
    CType       type;
    std::string name;
    std::string defaultValue;
    bool        isExtern    = false;
    bool        isStatic    = false;
    bool        isConst     = false;
    bool        isConstexpr = false;
    bool        isInline    = false;

    void print(std::ostream& os, int indent = 0) const;
};

// -----------------------------------------------------------------------
// Struct / class / union field
// -----------------------------------------------------------------------
struct CField {
    CType       type;
    std::string name;
    std::string defaultValue;
    bool        isStatic   = false;
    bool        isConst    = false;
    bool        isBitField = false;
    int         bitWidth   = 0;
};

// -----------------------------------------------------------------------
// Struct / class / union declaration
// -----------------------------------------------------------------------
struct CStructDecl {
    std::string              ns;
    std::string              name;
    std::string              kind;       // "struct" | "class" | "union"
    std::vector<std::string> bases;     // base class/struct names
    std::vector<CField>      fields;
    bool                     isDefinition = false;
    bool                     isTemplate   = false;

    void print(std::ostream& os, int indent = 0) const;
};

// -----------------------------------------------------------------------
// Enum (C enum and C++ enum class / enum struct)
// -----------------------------------------------------------------------
struct CEnumDecl {
    std::string ns;
    std::string name;
    bool        isClass = false;     // enum class / enum struct
    std::string underlying;          // e.g. "uint32_t"
    std::vector<std::pair<std::string, std::string>> enumerators; // name -> raw value

    void print(std::ostream& os, int indent = 0) const;
};

// -----------------------------------------------------------------------
// Typedef / using alias
// -----------------------------------------------------------------------
struct CTypedef {
    std::string ns;
    CType       underlyingType;
    std::string alias;
    bool        isUsing = false; // 'using X = T'  vs  'typedef T X'

    void print(std::ostream& os, int indent = 0) const;
};

// -----------------------------------------------------------------------
// Aggregated parse result
// -----------------------------------------------------------------------
struct CHeaderResult {
    std::string                filePath;
    std::vector<CFunctionDecl> functions;
    std::vector<CVarDecl>      variables;
    std::vector<CStructDecl>   structs;
    std::vector<CEnumDecl>     enums;
    std::vector<CTypedef>      typedefs;
    std::vector<std::string>   errors;

    void print(std::ostream& os) const;
};

// -----------------------------------------------------------------------
// The parser
// -----------------------------------------------------------------------
class CHeaderParser {
public:
    CHeaderParser(std::string source, std::string filePath);

    // Parse the full header and return a populated CHeaderResult.
    CHeaderResult parse();

private:
    // ---- source / cursor ----
    std::string m_src;
    std::string m_filePath;
    std::size_t m_pos  = 0;
    int         m_line = 1;
    int         m_col  = 1;

    // ---- current linkage (updated when entering extern "C" / "C++" blocks) ----
    Linkage     m_linkage = Linkage::Cpp;

    // ---- low-level helpers ----
    char        peek(int offset = 0) const;
    char        consume();
    bool        eof() const;
    void        skipWhitespace();
    void        skipLineComment();
    void        skipBlockComment();
    bool        skipPreprocessor();
    // Match a keyword/string at current pos. Returns true and advances on match.
    bool        matchKw(const char* kw);   // requires word boundary after kw
    bool        matchStr(const char* s);   // raw string match, no boundary check
    std::string readIdentifier();
    // Read a possibly-qualified name: Foo, std::vector, ns::A::B
    // Also reads trailing template args: std::vector<int>  →  "std::vector<int>"
    std::string readScopedName();
    std::string readStringLiteral();
    bool        isIdentStart(char c) const;
    bool        isIdentCont(char c)  const;
    // Skip balanced { } / ( ) / [ ] . Returns true if consumed.
    bool        skipBalanced(char open, char close);
    // Skip template angle-bracket arg list < ... > (handles nesting)
    bool        skipTemplateArgs();
    // Capture text up to (but not including) any of the sentinel chars,
    // respecting balanced (), [], {}, <>
    std::string captureExpr(const char* sentinels);

    // ---- grammar ----

    // Main dispatch loop; recurses for namespaces and class bodies.
    void parseScope(CHeaderResult& result,
                    const std::string& currentNs,
                    const std::string& currentClass,
                    Linkage linkage,
                    char terminator = '\0'); // '\0' = EOF, '}' = block end

    // Returns true if a declaration was consumed, false if nothing matched.
    bool tryParseDecl(CHeaderResult& result,
                      const std::string& currentNs,
                      const std::string& currentClass,
                      Linkage linkage);

    // C++ specific constructs
    void parseNamespace(CHeaderResult& result, const std::string& parentNs);
    void parseExternSpec(CHeaderResult& result,
                         const std::string& currentNs,
                         const std::string& currentClass);
    void parseClassOrStruct(CHeaderResult& result,
                            const std::string& currentNs,
                            const std::string& keyword);
    void parseClassBody(CHeaderResult& result,
                        CStructDecl& decl,
                        const std::string& currentNs,
                        Linkage linkage);
    void parseEnum(CHeaderResult& result, const std::string& currentNs);
    void parseTemplate(CHeaderResult& result, const std::string& currentNs);
    void parseUsing(CHeaderResult& result, const std::string& currentNs);
    void parseTypedef(CHeaderResult& result, const std::string& currentNs);

    // Parse a type specifier sequence (qualifiers + base type).
    // Stops before the declarator (name / '*' etc.).
    CType parseType(bool allowAuto = false);

    // Parse the declarator on top of a base type:
    // '*', '&', '&&', const-after-ptr, array [], name.
    void parseDeclarator(CType& base, std::string& outName);

    // Parse a full parameter list starting AFTER the opening '('.
    std::vector<CParam> parseParamList(bool& isVariadic);
    CParam parseParam();

    // Skip or record trailing function qualifiers: const noexcept override = 0 = delete
    void parseTrailingFnQualifiers(CFunctionDecl& fn);

    // Helpers
    void emitError(CHeaderResult& result, const std::string& msg);
    std::string joinNs(const std::string& a, const std::string& b) const;

    // Keyword classification
    bool isTypeQualifier(const std::string& w) const;
    bool isStorageClass(const std::string& w) const;
    bool isBuiltinType(const std::string& w) const;
    bool isFunctionSpecifier(const std::string& w) const;
};

} // namespace extern_support
} // namespace Omniscript