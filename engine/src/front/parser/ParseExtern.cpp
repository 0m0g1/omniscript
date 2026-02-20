// ==============================
// engine/src/omniscript/parser/Extern.cpp
// (UPDATED: extern language token "C"/"c++"/"cpp" + optional "allow-mangled")
// Works with your m_current_token/m_previous_token API
// ==============================
#include <omniscript/parser/Parser.h>

#include <algorithm>
#include <cctype>
#include <string>
#include <vector>

namespace Omniscript {
namespace {

// -------------------------
// string helpers
// -------------------------
static std::string toLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

static bool endsWith(const std::string& s, const char* suffix) {
    const size_t n = std::char_traits<char>::length(suffix);
    return s.size() >= n && s.compare(s.size() - n, n, suffix) == 0;
}

// -------------------------
// file classification
// -------------------------
static bool isHeaderPath(const std::string& raw) {
    const std::string p = toLower(raw);
    return endsWith(p, ".h") || endsWith(p, ".hpp") || endsWith(p, ".hh") ||
           endsWith(p, ".hxx") || endsWith(p, ".inl");
}

static bool isLibraryPath(const std::string& raw) {
    const std::string p = toLower(raw);
    return endsWith(p, ".dll") || endsWith(p, ".so") || endsWith(p, ".dylib") ||
           endsWith(p, ".a")  || endsWith(p, ".lib") || endsWith(p, ".wa");
}

// -------------------------
// extern mode tokens
// -------------------------
// You can now write:
//   extern "C", "mylib.h", "mylib.dll" { ... }     // parse header(s) as C
//   extern "c++", "mylib.hpp" { ... }             // parse header(s) as C++
//   extern "cpp", "mylib.hpp" { ... }             // same as c++
//
// Optional flag token for C++ parsing:
//   extern "c++", "allow-mangled", "mylib.hpp" { ... }
//
// Disambiguation rule:
//   - If ANY header path is present among inputs, then "C"/"c++"/"cpp" are treated
//     as language tokens (NOT system-lib shorthand).
//   - If NO header paths exist, then "C" may still be used as your libc shorthand
//     (handled in later semantic/ffi resolution), and we do NOT consume it here.
static bool isLangToken(const std::string& s) {
    const std::string p = toLower(s);
    return p == "c" || p == "c++" || p == "cpp";
}

static bool isAllowMangledToken(const std::string& s) {
    return toLower(s) == "allow-mangled";
}

static ExternLang langFromToken(const std::string& s) {
    const std::string p = toLower(s);
    if (p == "c") return ExternLang::C;
    if (p == "c++" || p == "cpp") return ExternLang::Cpp;
    return ExternLang::Auto;
}

} // namespace

bool Parser::isExternDeclStart() const {
    const TokenType t = current().type();
    return t == TokenType::Function || t == TokenType::Let || t == TokenType::Const || t == TokenType::Import;
}

void Parser::skipExternSeparators() {
    while (match(TokenType::Semicolon)) {}
    while (match(TokenType::Newline)) {}
}

StmtPtr Parser::parseImport() {
    // Precondition: 'import' already consumed
    const Token importTok = previous();
    const FileSpan start = importTok.span();

    // import *;
    if (match(TokenType::Star)) {
        const Token starTok = previous();
        FileSpan full = mergeSpans(start, starTok.span());
        return std::make_unique<ImportStmt>(ImportKind::All, Token{}, Token{}, full);
    }

    ImportKind kind{};
    if (match(TokenType::Function)) {
        kind = ImportKind::Fn;
    } else if (match(TokenType::Let)) {
        kind = ImportKind::Var;
    } else if (match(TokenType::Const)) {
        kind = ImportKind::Const;
    } else {
        throw ParseError("Expected 'fn', 'let', 'const', or '*' after 'import'.", current());
    }

    if (!check(TokenType::Identifier))
        throw ParseError("Expected identifier after 'import <kind>'.", current());
    Token name = advance();

    Token alias{};
    bool hasAlias = false;

    if (match(TokenType::As)) {
        if (!check(TokenType::Identifier))
            throw ParseError("Expected identifier after 'as'.", current());
        alias = advance();
        hasAlias = true;
    }

    FileSpan full = mergeSpans(start, (hasAlias ? alias.span() : name.span()));
    return std::make_unique<ImportStmt>(kind, std::move(name), std::move(alias), full);
}

StmtPtr Parser::parseExtern() {
    // Precondition: 'extern' already consumed in parseStatement()

    // 1) Parse extern inputs: 1+ string literals separated by commas
    std::vector<Token> toks;

    if (!check(TokenType::StringLiteral))
        throw ParseError("Expected string literal after 'extern'.", current());
    toks.push_back(advance());

    while (match(TokenType::Comma)) {
        if (!check(TokenType::StringLiteral))
            throw ParseError("Expected string literal after ','.", current());
        toks.push_back(advance());
    }

    const FileSpan start = toks.front().span();

    // 2) Detect whether any header paths are present (disambiguation)
    bool anyHeader = false;
    for (const auto& t : toks) {
        const std::string v = t.value();
        if (isHeaderPath(v)) { anyHeader = true; break; }
    }

    // 3) Consume optional mode tokens if headers exist
    //    - language token: "C" / "c++" / "cpp"
    //    - optional flag: "allow-mangled"
    ExternLang lang = ExternLang::Auto;
    bool allowMangled = false;

    std::vector<Token> remaining;
    remaining.reserve(toks.size());

    if (anyHeader) {
        for (const auto& t : toks) {
            const std::string v = t.value();
            if (isLangToken(v)) {
                // last one wins, but realistically users pass one
                lang = langFromToken(v);
                continue;
            }
            if (isAllowMangledToken(v)) {
                allowMangled = true;
                continue;
            }
            remaining.push_back(t);
        }
    } else {
        // no headers => keep tokens as-is; "C" may be libc shorthand downstream
        remaining = std::move(toks);
    }

    // 4) classify inputs (after removing mode tokens if applicable)
    std::vector<std::string> headers;
    std::vector<std::string> libs;
    headers.reserve(remaining.size());
    libs.reserve(remaining.size());

    for (auto& t : remaining) {
        const std::string path = t.value();
        if (isHeaderPath(path)) headers.push_back(path);
        else if (isLibraryPath(path)) libs.push_back(path);
        else libs.push_back(path); // extensionless shorthand => treat as library
    }

    // 5) extern "...","..." { ... }
    if (match(TokenType::LeftBrace)) {
        std::vector<StmtPtr> decls;

        skipExternSeparators();

        while (!check(TokenType::RightBrace) && !check(TokenType::EndOfInput)) {
            if (match(TokenType::Import)) {
                decls.push_back(parseImport());
            } else if (match(TokenType::Function)) {
                decls.push_back(parseFunctionDeclaration());
            } else if (match(TokenType::Let)) {
                decls.push_back(parseVarDecl(VarFlavor::Let));
            } else if (match(TokenType::Const)) {
                decls.push_back(parseVarDecl(VarFlavor::Const));
            } else {
                throw ParseError("Expected 'import', 'fn', 'let', or 'const' inside extern block.", current());
            }

            skipExternSeparators();
        }

        const Token rb = current();
        eat(TokenType::RightBrace, "Expected '}' to close extern block.");

        FileSpan full = mergeSpans(start, rb.span());

        // NOTE: we store the original authored tokens in inputTokens for debug/UX,
        // but we also store parsed mode + filtered header/lib lists.
        auto node = std::make_unique<ExternStmt>(
            /*inputs*/ std::move(remaining),
            /*headers*/ std::move(headers),
            /*libs*/ std::move(libs),
            /*stmts*/ std::move(decls),
            /*span*/ full
        );
        node->lang = lang;
        node->allowMangled = allowMangled;
        return node;
    }

    // 6) extern "...","..." fn/let/const ... (single decl form)
    if (match(TokenType::Function)) {
        auto d = parseFunctionDeclaration();
        FileSpan full = mergeSpans(start, d->span);
        std::vector<StmtPtr> decls;
        decls.push_back(std::move(d));

        auto node = std::make_unique<ExternStmt>(std::move(remaining), std::move(headers), std::move(libs), std::move(decls), full);
        node->lang = lang;
        node->allowMangled = allowMangled;
        return node;
    }

    if (match(TokenType::Let)) {
        auto d = parseVarDecl(VarFlavor::Let);
        FileSpan full = mergeSpans(start, d->span);
        std::vector<StmtPtr> decls;
        decls.push_back(std::move(d));

        auto node = std::make_unique<ExternStmt>(std::move(remaining), std::move(headers), std::move(libs), std::move(decls), full);
        node->lang = lang;
        node->allowMangled = allowMangled;
        return node;
    }

    if (match(TokenType::Const)) {
        auto d = parseVarDecl(VarFlavor::Const);
        FileSpan full = mergeSpans(start, d->span);
        std::vector<StmtPtr> decls;
        decls.push_back(std::move(d));

        auto node = std::make_unique<ExternStmt>(std::move(remaining), std::move(headers), std::move(libs), std::move(decls), full);
        node->lang = lang;
        node->allowMangled = allowMangled;
        return node;
    }

    // 7) extern "...","...";  (auto-import)
    eatStatementTerminator();

    FileSpan full = mergeSpans(start, remaining.back().span());
    auto node = std::make_unique<ExternStmt>(
        std::move(remaining),
        std::move(headers),
        std::move(libs),
        std::vector<StmtPtr>{},
        full
    );
    node->lang = lang;
    node->allowMangled = allowMangled;
    return node;
}

} // namespace Omniscript