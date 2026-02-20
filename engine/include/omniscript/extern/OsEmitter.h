#pragma once
// engine/include/omniscript/extern/OsEmitter.h
//
// Converts a CHeaderResult into .os `extern` block source text.
//
// Usage:
//   extern_support::OsEmitter emitter;
//   emitter.emit(std::cout, result, { "mylib.dll", "libmylib.a" });

#include <omniscript/extern/CHeaderParser.h>

#include <iosfwd>
#include <string>
#include <vector>

namespace Omniscript {
namespace extern_support {

// -----------------------------------------------------------------------
// Options
// -----------------------------------------------------------------------
struct EmitOptions {
    // Emit a // comment above methods showing their qualified C++ origin
    bool sourceComments    = true;

    // Skip names that start with '_'  (implementation details)
    bool skipPrivateNames  = true;

    // Skip static class members (not importable via plain FFI)
    bool skipStaticMembers = true;

    // Emit structs as opaque instead of expanding fields.
    // e.g.  // [struct Foo — use void*]
    bool structsAsOpaque   = false;

    // Indentation inside extern blocks
    std::string indent = "    ";
};

// -----------------------------------------------------------------------
// OsEmitter
// -----------------------------------------------------------------------
class OsEmitter {
public:
    explicit OsEmitter(EmitOptions opts = {});

    // Emit a complete extern block:
    //
    //   // Auto-generated from: mylib.hpp
    //   extern "mylib.dll", "libmylib.a" {
    //       fn foo(x: int) => void;
    //       ...
    //   }
    void emit(std::ostream& os,
              const CHeaderResult& result,
              const std::vector<std::string>& libraryPaths) const;

    // Emit only the body lines (no extern wrapper).
    // Useful for merging several headers into one extern block.
    void emitBody(std::ostream& os,
                  const CHeaderResult& result) const;

    // Convert a single CType to its OS type string.
    // e.g.  { base="uint32_t", pointers=1 }  ->  "u32*"
    static std::string toOsType(const CType& t);

private:
    EmitOptions m_opts;

    void emitFunction(std::ostream& os, const CFunctionDecl& fn) const;
    void emitVariable(std::ostream& os, const CVarDecl& var)     const;
    void emitStruct  (std::ostream& os, const CStructDecl& s)    const;
    void emitEnum    (std::ostream& os, const CEnumDecl& e)      const;
    void emitTypedef (std::ostream& os, const CTypedef& td)      const;

    bool shouldSkip(const std::string& name) const;

    // Map a C/C++ primitive name to an OS primitive.
    // Returns "" if no direct mapping exists.
    static std::string mapPrimitive(const std::string& cName);
};

} // namespace extern_support
} // namespace Omniscript