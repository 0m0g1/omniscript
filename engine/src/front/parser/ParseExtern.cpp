// ==============================
// //engine/src/omniscript/parser/Extern.cpp  (REWRITTEN FOR m_current_token/m_previous_token API)
// ==============================
#include <omniscript/parser/Parser.h>

#include <algorithm>
#include <cctype>
#include <string>
#include <vector>

namespace Omniscript {
namespace {

static std::string toLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

static bool endsWith(const std::string& s, const char* suffix) {
    const size_t n = std::char_traits<char>::length(suffix);
    return s.size() >= n && s.compare(s.size() - n, n, suffix) == 0;
}

static bool isHeaderPath(const std::string& raw) {
    const std::string p = toLower(raw);
    return endsWith(p, ".h") || endsWith(p, ".hpp") || endsWith(p, ".hh") || endsWith(p, ".hxx") || endsWith(p, ".inl");
}

static bool isLibraryPath(const std::string& raw) {
    const std::string p = toLower(raw);
    return endsWith(p, ".dll") || endsWith(p, ".so") || endsWith(p, ".dylib") ||
           endsWith(p, ".a")  || endsWith(p, ".lib") || endsWith(p, ".wa");
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
    // Precondition: 'import' has already been consumed by match(TokenType::Import)
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
    Token name = advance(); // consume identifier

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
    // Precondition: 'extern' has already been consumed by match(TokenType::Extern) in parseStatement().
    // If you ever call parseExtern() directly, uncomment:
    // eat(TokenType::Extern, "Expected 'extern'.");

    // 1) Parse extern inputs: 1+ string literals separated by commas
    std::vector<Token> toks;

    if (!check(TokenType::StringLiteral))
        throw ParseError("Expected string literal after 'extern'.", current());
    toks.push_back(advance()); // consume first string literal

    while (match(TokenType::Comma)) {
        if (!check(TokenType::StringLiteral))
            throw ParseError("Expected string literal after ','.", current());
        toks.push_back(advance());
    }

    // classify inputs
    std::vector<std::string> headers;
    std::vector<std::string> libs;
    headers.reserve(toks.size());
    libs.reserve(toks.size());

    for (auto& t : toks) {
        const std::string path = t.value();
        if (isHeaderPath(path)) headers.push_back(path);
        else if (isLibraryPath(path)) libs.push_back(path);
        else libs.push_back(path); // extensionless shorthand => treat as library
    }

    const FileSpan start = toks.front().span();

    // 2) extern "..." { ... }
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

        const Token rb = current(); // capture for span before consuming
        eat(TokenType::RightBrace, "Expected '}' to close extern block.");

        FileSpan full = mergeSpans(start, rb.span());
        return std::make_unique<ExternStmt>(
            std::move(toks),
            std::move(headers),
            std::move(libs),
            std::move(decls),
            full
        );
    }

    // 3) extern "..." fn/let/const ...   (single decl form)
    if (match(TokenType::Function)) {
        auto d = parseFunctionDeclaration();
        FileSpan full = mergeSpans(start, d->span);
        std::vector<StmtPtr> decls;
        decls.push_back(std::move(d));
        return std::make_unique<ExternStmt>(std::move(toks), std::move(headers), std::move(libs), std::move(decls), full);
    }

    if (match(TokenType::Let)) {
        auto d = parseVarDecl(VarFlavor::Let);
        FileSpan full = mergeSpans(start, d->span);
        std::vector<StmtPtr> decls;
        decls.push_back(std::move(d));
        return std::make_unique<ExternStmt>(std::move(toks), std::move(headers), std::move(libs), std::move(decls), full);
    }

    if (match(TokenType::Const)) {
        auto d = parseVarDecl(VarFlavor::Const);
        FileSpan full = mergeSpans(start, d->span);
        std::vector<StmtPtr> decls;
        decls.push_back(std::move(d));
        return std::make_unique<ExternStmt>(std::move(toks), std::move(headers), std::move(libs), std::move(decls), full);
    }

    // 4) extern "mylib.h", "mylib.dll";   (auto-import)
    // Later semantic pass can:
    //   - if headers present: expand imports (or import-all) from headers into real decls
    //   - use libs for symbol verification/link args
    eatStatementTerminator();

    FileSpan full = mergeSpans(start, toks.back().span());
    return std::make_unique<ExternStmt>(
        std::move(toks),
        std::move(headers),
        std::move(libs),
        std::vector<StmtPtr>{},
        full
    );
}

} // namespace Omniscript