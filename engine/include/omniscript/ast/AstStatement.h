#pragma once

#include <utility>
#include <vector>

#include <omniscript/ast/Ast.h>

namespace Omniscript {

// ----------------- Statements -----------------
// ----------------- Extern / FFI support -----------------
//
// Extern inputs can mix libraries + headers, plus optional mode tokens:
//
//   extern "mylib.h", "mylib.dll" { ... }
//   extern "mylib.h", "mylib.dll";                 // auto-import (statements empty)
//   extern "mylib.h", "mylib.dll" fn ...           // single decl form (statements size=1)
//
// Language mode tokens (only meaningful when ANY header path is present):
//   extern "C",   "mylib.h",  "mylib.dll" { ... }  // parse headers as C
//   extern "c++", "mylib.hpp" { ... }              // parse headers as C++
//   extern "cpp", "mylib.hpp" { ... }              // alias for c++
//
// Optional flags:
//   extern "c++", "allow-mangled", "mylib.hpp" { ... }
//
// Disambiguation:
//   - If no header paths are present, "C" may still be treated as the libc shorthand
//     by the later FFI resolver (msvcrt/libc/libSystem depending on --target-os).
//
// Inside extern blocks you can write:
//   fn / let / const            (manual typed declarations)
//   import fn <name> [as <a>]   (auto signature from header(s))
//   import *;                   (import everything from header(s))
//
// The parser splits extern inputs into:
//   - headerPaths: *.h, *.hpp, *.hh, *.hxx, *.inl
//   - libraryPaths: *.dll, *.so, *.dylib, *.a, *.lib, *.wa, plus extensionless shorthands
//
// Header auto-import is implemented in a later semantic/FFI pass (not in the parser).

enum class ExternArtifactKind : std::uint8_t { Library, Header, Unknown };

struct ExternArtifact {
    Token pathTok;                // string literal token
    ExternArtifactKind kind;
};

// What to import from headers
enum class ImportKind : std::uint8_t { Fn, Var, Const, All };

struct ImportStmt final : Stmt {
    ImportKind kind;
    Token name;   // empty for All
    Token alias;  // optional

    ImportStmt(ImportKind k, Token n = {}, Token a = {}, FileSpan s = {})
      : Stmt(NodeKind::ImportStmt, std::move(s)), kind(k), name(std::move(n)), alias(std::move(a)) {}

    void accept(AstVisitor& v) override;
};

// Header parse mode for auto-import.
// Auto: infer from extension / fallback heuristics
// C:    parse headers as C
// Cpp:  parse headers as C++
enum class ExternLang : std::uint8_t { Auto, C, Cpp };

struct ExternStmt final : Stmt {
    // Raw extern inputs as authored (minus any consumed mode tokens if headers exist).
    // Example: ["mylib.h", "mylib.dll", "libmylib.a"]
    std::vector<Token> inputTokens;

    // Parsed mode/flags that influence header auto-import and symbol selection.
    ExternLang lang = ExternLang::Auto;

    // If lang==Cpp, default behavior should be "C-linkage only" to avoid mangled/overloaded symbols.
    // allowMangled relaxes that filter in the header importer.
    bool allowMangled = false;

    // Split views (filled by parser for convenience).
    std::vector<std::string> headerPaths;
    std::vector<std::string> libraryPaths;

    // Decls inside extern block (or single-decl extern form).
    // Empty means auto-import mode (e.g. extern "mylib.h", "mylib.dll";).
    std::vector<StmtPtr> statements;

    explicit ExternStmt(std::vector<Token> inputs = {},
                        std::vector<std::string> headers = {},
                        std::vector<std::string> libs = {},
                        std::vector<StmtPtr> stmts = {},
                        FileSpan s = {})
        : Stmt(NodeKind::ExternStmt, std::move(s)),
          inputTokens(std::move(inputs)),
          headerPaths(std::move(headers)),
          libraryPaths(std::move(libs)),
          statements(std::move(stmts)) {}

    void accept(AstVisitor& v) override;
};

// ----------------- libclang C-linkage filter (for your importer) -----------------
//
// When ExternStmt.lang == ExternLang::Cpp and allowMangled == false,
// keep only "C linkage" declarations so names match the dynamic/static library symbol table.
//
// Exact checks you can use with libclang:
//
//   bool isCLanguageLinkage(CXCursor c) {
//     // Available in newer clangs:
//     //   CXLanguageKind lk = clang_getCursorLanguage(c);
//     // but that reports the cursor language, not linkage.
//
//     // Most robust approach:
//     // - Look at mangled name: C functions usually have empty/identical mangling.
//     // - Or use clang_getCursorLinkage(c) and exclude internal/unique external.
//
//     CXString mang = clang_Cursor_getMangling(c);
//     const char* m = clang_getCString(mang);
//     std::string ms = m ? m : "";
//     clang_disposeString(mang);
//
//     // Heuristic: C linkage typically yields either empty mangling or same as spelling.
//     std::string spelling = toStd(clang_getCursorSpelling(c));
//     if (ms.empty()) return true;
//     if (!spelling.empty() && ms == spelling) return true;
//     return false;
//   }
//
// Also ensure it’s an externally visible symbol:
//   auto lk = clang_getCursorLinkage(c);
//   keep if lk == CXLinkage_External (and maybe CXLinkage_UniqueExternal depending on your needs)
//
// In practice for FFI v1, you can use:
//   keep = (clang_getCursorLinkage(c) == CXLinkage_External) && isCLanguageLinkage(c);
//
// Then relax this when allowMangled == true.

// AstStatement.h (add this)
struct ParamDecl {
    Token name;                 // identifier token
    std::vector<Token> type;    // minimal "type tokens" (e.g. ["int"], or ["MyType","*"])
    std::vector<Token> typeToks;
    FileSpan span{};
    bool isVarArg = false;
};

struct FunctionDeclStmt final : Stmt {
    Token name;                      // identifier
    std::vector<ParamDecl> params;
    std::vector<Token> returnType;   // empty => infer void
    StmtPtr body;                    // BlockStmt or null for prototypes
    bool isPrototype = false;        // true if ends with ';' (or newline) without body

    FunctionDeclStmt(Token n,
                     std::vector<ParamDecl> ps,
                     std::vector<Token> ret,
                     StmtPtr b,
                     bool proto,
                     FileSpan s = {})
        : Stmt(NodeKind::FunctionDeclStmt, std::move(s)),
          name(std::move(n)),
          params(std::move(ps)),
          returnType(std::move(ret)),
          body(std::move(b)),
          isPrototype(proto) {
        if (span.start.filePath.empty()) span = this->name.span();
    }

    void accept(AstVisitor& v) override;
};

struct BlockStmt final : Stmt {
    std::vector<StmtPtr> statements;

    explicit BlockStmt(std::vector<StmtPtr> stmts = {}, FileSpan s = {})
        : Stmt(NodeKind::BlockStmt, std::move(s)), statements(std::move(stmts)) {}

    void accept(AstVisitor& v) override;
};

struct ExprStmt final : Stmt {
    ExprPtr expr;

    explicit ExprStmt(ExprPtr e, FileSpan s = {})
        : Stmt(NodeKind::ExprStmt, std::move(s)), expr(std::move(e)) {}

    void accept(AstVisitor& v) override;
};

struct VarDeclStmt final : Stmt {
    VarFlavor flavor;
    Token name;          // identifier token
    ExprPtr initializer; // may be null

    VarDeclStmt(VarFlavor f, Token n, ExprPtr init, FileSpan s = {})
        : Stmt(NodeKind::VarDeclStmt, std::move(s)),
          flavor(f),
          name(std::move(n)),
          initializer(std::move(init)) {
        if (span.start.filePath.empty()) span = name.span();
    }

    void accept(AstVisitor& v) override;
};

struct IfStmt final : Stmt {
    ExprPtr condition;
    StmtPtr thenBranch;
    StmtPtr elseBranch; // may be null

    IfStmt(ExprPtr c, StmtPtr t, StmtPtr e = nullptr, FileSpan s = {})
        : Stmt(NodeKind::IfStmt, std::move(s)),
          condition(std::move(c)),
          thenBranch(std::move(t)),
          elseBranch(std::move(e)) {}

    void accept(AstVisitor& v) override;
};

struct WhileStmt final : Stmt {
    ExprPtr condition;
    StmtPtr body;

    WhileStmt(ExprPtr c, StmtPtr b, FileSpan s = {})
        : Stmt(NodeKind::WhileStmt, std::move(s)),
          condition(std::move(c)),
          body(std::move(b)) {}

    void accept(AstVisitor& v) override;
};

struct ReturnStmt final : Stmt {
    ExprPtr value; // may be null

    explicit ReturnStmt(ExprPtr v = nullptr, FileSpan s = {})
        : Stmt(NodeKind::ReturnStmt, std::move(s)), value(std::move(v)) {}

    void accept(AstVisitor& v) override;
};

} // namespace Omniscript