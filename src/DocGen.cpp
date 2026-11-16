// src/DocGen.cpp
#include <omniscript/DocGen.h>
#include <omniscript/Lexer.h>
#include <omniscript/Tokens.h>

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

namespace Omniscript {

namespace fs = std::filesystem;

namespace {

std::vector<std::string> splitLines(const std::string& s) {
    std::vector<std::string> lines;
    std::string cur;
    for (size_t i = 0; i < s.size(); ++i) {
        const char c = s[i];
        if (c == '\r') {
            if (i + 1 < s.size() && s[i + 1] == '\n') ++i;
            lines.push_back(cur);
            cur.clear();
        } else if (c == '\n') {
            lines.push_back(cur);
            cur.clear();
        } else {
            cur += c;
        }
    }
    lines.push_back(cur);
    return lines;
}

std::string trim(const std::string& s) {
    const char* ws = " \t\r\n";
    const size_t a = s.find_first_not_of(ws);
    if (a == std::string::npos) return "";
    const size_t b = s.find_last_not_of(ws);
    return s.substr(a, b - a + 1);
}

std::string toLower(std::string s) {
    for (char& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return s;
}

// The declaration as the author wrote it: from its first line up to (not including) the first '{' or ';'.
std::string signatureAt(const std::vector<std::string>& lines, size_t index) {
    std::string sig;
    for (size_t i = index; i < lines.size() && i < index + 8; ++i) {
        const std::string ln = trim(lines[i]);
        const size_t cut = ln.find_first_of("{;");
        const std::string part = trim(cut == std::string::npos ? ln : ln.substr(0, cut));
        if (!part.empty()) sig += (sig.empty() ? "" : " ") + part;
        if (cut != std::string::npos) break;
    }
    return sig;
}

std::string stemOf(const std::string& path) {
    size_t slash = path.find_last_of("/\\");
    std::string name = slash == std::string::npos ? path : path.substr(slash + 1);
    const size_t dot = name.find_last_of('.');
    if (dot != std::string::npos) name = name.substr(0, dot);
    return name;
}

// GitHub-style heading anchor: lowercase, keep letters/digits/'-'/'_', spaces become '-'.
std::string anchorOf(const std::string& title) {
    std::string a;
    for (char ch : title) {
        const unsigned char c = static_cast<unsigned char>(ch);
        if (std::isalnum(c) || c == '-' || c == '_') a += static_cast<char>(std::tolower(c));
        else if (c == ' ') a += '-';
    }
    return a;
}

std::string firstLine(const std::string& s) {
    const size_t nl = s.find('\n');
    return trim(nl == std::string::npos ? s : s.substr(0, nl));
}

// Renders one file. `shift` pushes every heading down that many levels (used when the file
// is embedded in a bigger document). `summary` receives the first line of the `//!` doc.
// `entries` receives the number of documented declarations.
bool renderFile(const std::string& path, const std::string& title, size_t shift,
                std::string& out, std::string& summary, size_t& entries, std::string& error) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        error = "Cannot open '" + path + "'";
        return false;
    }
    std::stringstream buffer;
    buffer << in.rdbuf();
    const std::string source = buffer.str();

    // Lex the whole file. This fills the lexer's doc table, and the braces tell us which
    // struct/class/enum body every line is in (so methods can be listed under their owner).
    Lexer lexer(source, path);
    std::map<size_t, std::string> ownerAtLine;
    std::vector<std::string> bodies;      // one entry per open '{': the owner's name, or "" for other blocks
    std::string pendingName;
    bool expectName = false;
    for (;;) {
        Token t = lexer.getNextToken();
        const TokenTypes type = t.getType();
        if (type == TokenTypes::EOI) break;
        if (type == TokenTypes::Newline) continue;

        ownerAtLine.emplace(t.getLine(), bodies.empty() ? std::string() : bodies.back());

        if (type == TokenTypes::Struct || type == TokenTypes::Class || type == TokenTypes::Enum) {
            expectName = true;
        } else if (type == TokenTypes::Identifier && expectName) {
            pendingName = t.getValue();
            expectName = false;
        } else if (type == TokenTypes::LeftBrace) {
            bodies.push_back(pendingName);
            pendingName.clear();
        } else if (type == TokenTypes::RightBrace) {
            if (!bodies.empty()) bodies.pop_back();
        } else if (type == TokenTypes::Semicolon) {
            pendingName.clear();
            expectName = false;
        }
    }

    auto heading = [&](size_t level) { return std::string(level + shift, '#') + " "; };

    const std::vector<std::string> lines = splitLines(source);
    std::ostringstream o;
    o << heading(1) << title << "\n\n";

    const std::string moduleDoc = lexer.moduleDoc();
    summary = firstLine(moduleDoc);
    if (!moduleDoc.empty()) o << moduleDoc << "\n\n";

    entries = 0;
    for (const auto& block : lexer.docBlocks()) {
        const size_t declIndex = block.lastLine;          // 0-based index of the line after the block
        if (declIndex >= lines.size()) continue;
        const std::string first = trim(lines[declIndex]);
        if (first.empty() || first.rfind("//", 0) == 0) continue;   // not attached to a declaration

        const std::string sig = signatureAt(lines, declIndex);
        if (sig.empty()) continue;

        const auto ownerIt = ownerAtLine.find(block.lastLine + 1);
        const std::string owner = ownerIt == ownerAtLine.end() ? std::string() : ownerIt->second;

        o << heading(owner.empty() ? 2 : 3) << "`" << sig << "`\n\n";
        if (!owner.empty()) o << "*Member of `" << owner << "`.*\n\n";
        o << block.text << "\n\n";
        ++entries;
    }
    if (entries == 0 && moduleDoc.empty()) o << "_No documentation yet._\n\n";

    out = o.str();
    return true;
}

struct ModuleFile {
    std::string name;      // "std/Console", "std/math/Vector"
    std::string relMd;     // "Console.md", "math/Vector.md"
    fs::path source;
};

// ---- Following a library's imports ---------------------------------------------------------

struct ImportRef {
    std::string name;                      // declared name ("Console"); empty for `import "x"` / `include "x"`
    std::string path;                      // the string literal as written
    std::vector<std::string> enclosing;    // names of the `module X { ... }` bodies it sits in
};

struct ScanResult {
    std::vector<ImportRef> refs;
    std::string topModule;                 // name of the file's outermost `module X { ... }`, if any
};

bool isHeaderPath(const std::string& p) {
    const size_t dot = p.find_last_of('.');
    if (dot == std::string::npos) return false;
    const std::string ext = toLower(p.substr(dot));
    return ext == ".h" || ext == ".hpp" || ext == ".hh" || ext == ".hxx";
}

// Finds every file a source file pulls in, using the lexer (so comments and strings are handled
// exactly like the engine does):  module X = import "p";   import "p";   import * from "p";
// import { a } from "p";   include "p";
bool scanImports(const std::string& path, ScanResult& result, std::string& error) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        error = "Cannot open '" + path + "'";
        return false;
    }
    std::stringstream buffer;
    buffer << in.rdbuf();
    Lexer lexer(buffer.str(), path);

    enum class St { None, ModuleName, AfterName, ModuleAssign, ModuleImport, ImportScan, IncludePath };
    St st = St::None;
    std::string pending;
    std::vector<std::string> stack;        // one entry per open '{': module name, or "" for other blocks

    auto enclosing = [&]() {
        std::vector<std::string> v;
        for (const auto& s : stack) if (!s.empty()) v.push_back(s);
        return v;
    };

    for (;;) {
        Token t = lexer.getNextToken();
        const TokenTypes type = t.getType();
        if (type == TokenTypes::EOI) break;
        if (type == TokenTypes::Newline) continue;

        switch (st) {
        case St::ModuleName:
            if (type == TokenTypes::Identifier) { pending = t.getValue(); st = St::AfterName; continue; }
            st = St::None;
            break;
        case St::AfterName:
            if (type == TokenTypes::Assign) { st = St::ModuleAssign; continue; }
            if (type == TokenTypes::LeftBrace) {
                if (stack.empty() && result.topModule.empty()) result.topModule = pending;
                stack.push_back(pending);
                pending.clear();
                st = St::None;
                continue;
            }
            st = St::None;
            break;
        case St::ModuleAssign:
            if (type == TokenTypes::Import) { st = St::ModuleImport; continue; }
            st = St::None;
            break;
        case St::ModuleImport:
            if (type == TokenTypes::StringLiteral) result.refs.push_back({pending, t.getValue(), enclosing()});
            pending.clear();
            st = St::None;
            continue;
        case St::ImportScan:
            if (type == TokenTypes::StringLiteral) {
                result.refs.push_back({"", t.getValue(), enclosing()});
                st = St::None;
            } else if (type == TokenTypes::Semicolon) {
                st = St::None;
            }
            continue;                      // braces of `import { a } from` are not module bodies
        case St::IncludePath:
            if (type == TokenTypes::StringLiteral) result.refs.push_back({"", t.getValue(), enclosing()});
            st = St::None;
            continue;
        case St::None:
            break;
        }

        if (type == TokenTypes::Module) st = St::ModuleName;
        else if (type == TokenTypes::Import) st = St::ImportScan;
        else if (type == TokenTypes::Include) st = St::IncludePath;
        else if (type == TokenTypes::LeftBrace) stack.push_back("");
        else if (type == TokenTypes::RightBrace) { if (!stack.empty()) stack.pop_back(); }
    }
    return true;
}

// Import paths are used as the engine does (relative to the working directory);
// as a fallback they are tried relative to the importing file.
fs::path resolveImportPath(const std::string& p, const fs::path& importer) {
    std::error_code ec;
    const fs::path direct(p);
    if (fs::exists(direct, ec)) return direct;
    const fs::path sibling = importer.parent_path() / direct;
    if (fs::exists(sibling, ec)) return sibling;
    return {};
}

// Depth-first, in source order. Each file is documented once, under the first name it was reached by.
bool collectModules(const fs::path& file, const std::string& prefix, std::set<std::string>& visited,
                    std::vector<ModuleFile>& out, std::string& error) {
    ScanResult scan;
    if (!scanImports(file.string(), scan, error)) return false;

    const size_t slash = prefix.find_last_of('/');
    const std::string lastSegment = slash == std::string::npos ? prefix : prefix.substr(slash + 1);

    for (const auto& ref : scan.refs) {
        if (isHeaderPath(ref.path)) continue;

        std::string qualified = prefix;
        for (size_t i = 0; i < ref.enclosing.size(); ++i) {
            // a file's own `module Http { }` wrapper is the name we already reached it by
            if (i == 0 && !prefix.empty() && ref.enclosing[i] == lastSegment) continue;
            qualified += (qualified.empty() ? "" : "/") + ref.enclosing[i];
        }
        const std::string leaf = ref.name.empty() ? stemOf(ref.path) : ref.name;
        const std::string full = qualified.empty() ? leaf : qualified + "/" + leaf;

        const fs::path resolved = resolveImportPath(ref.path, file);
        if (resolved.empty()) {
            error = "Cannot find '" + ref.path + "' (imported from '" + file.string() + "')";
            return false;
        }

        std::error_code ec;
        const std::string key = toLower(fs::weakly_canonical(resolved, ec).string());
        if (!visited.insert(key).second) continue;

        std::string flat = full;
        std::replace(flat.begin(), flat.end(), '/', '.');
        out.push_back(ModuleFile{full, flat + ".md", resolved});

        if (!collectModules(resolved, full, visited, out, error)) return false;
    }
    return true;
}

// ---- HTML output ----------------------------------------------------------------------------
// A small Markdown-to-HTML converter for exactly what doc comments contain: headings, paragraphs,
// flat bullet/numbered lists, fenced code blocks, `code`, **bold**, *italic* and [links](url).
// Everything is HTML-escaped; raw HTML in a doc comment is shown as text, never executed.

struct HtmlHeading {
    int level = 2;
    std::string text;      // plain text of the heading (backticks removed)
    std::string id;        // anchor id
    std::string desc;      // first sentence-ish of the paragraph below it, for search
};

struct HtmlPage {
    std::string name, summary, href, anchor, body;
    std::vector<HtmlHeading> headings;
};

std::string esc(const std::string& s) {
    std::string o;
    o.reserve(s.size() + 8);
    for (char c : s) {
        switch (c) {
        case '&': o += "&amp;"; break;
        case '<': o += "&lt;"; break;
        case '>': o += "&gt;"; break;
        case '"': o += "&quot;"; break;
        case '\'': o += "&#39;"; break;
        default: o += c;
        }
    }
    return o;
}

std::string jsonEsc(const std::string& s) {
    std::string o;
    for (char ch : s) {
        const unsigned char c = static_cast<unsigned char>(ch);
        switch (ch) {
        case '"': o += "\\\""; break;
        case '\\': o += "\\\\"; break;
        case '\n': o += "\\n"; break;
        case '\r': o += "\\r"; break;
        case '\t': o += "\\t"; break;
        case '<': o += "\\u003c"; break;      // keeps "</script>" out of inline data
        default:
            if (c < 0x20) {
                char buf[8];
                std::snprintf(buf, sizeof buf, "\\u%04x", c);
                o += buf;
            } else {
                o += ch;
            }
        }
    }
    return o;
}

std::string plainOf(std::string s) {
    s.erase(std::remove(s.begin(), s.end(), '`'), s.end());
    return s;
}

// "function readLine(out: Str.String*) => bool" -> "readLine"; "Ownership rules" stays as is.
std::string labelOf(std::string t) {
    static const char* const keywords[] = {"public ", "private ", "static ", "extern ", "struct ", "class ",
                                           "enum ", "function ", "fn ", "let ", "const "};
    bool changed = true;
    while (changed) {
        changed = false;
        for (const char* kw : keywords) {
            const std::string k(kw);
            if (t.rfind(k, 0) == 0) { t = t.substr(k.size()); changed = true; }
        }
    }
    const size_t cut = t.find_first_of("(:=<{");
    if (cut != std::string::npos && cut > 0) t = t.substr(0, cut);
    t = trim(t);
    return t;
}

std::string slugOf(const std::string& label) {
    std::string a;
    for (char ch : label) {
        const unsigned char c = static_cast<unsigned char>(ch);
        if (std::isalnum(c) || c == '_') a += static_cast<char>(std::tolower(c));
        else if (c == ' ' || c == '-') { if (!a.empty() && a.back() != '-') a += '-'; }
    }
    while (!a.empty() && a.back() == '-') a.pop_back();
    return a.empty() ? "section" : a;
}

bool safeUrl(const std::string& u) {
    if (u.empty()) return false;
    if (u.rfind("http://", 0) == 0 || u.rfind("https://", 0) == 0 || u.rfind("mailto:", 0) == 0) return true;
    return u.find(':') == std::string::npos;      // relative link or #anchor; rejects javascript: and friends
}

// Relative links to other .md pages keep working in the HTML output.
std::string rewriteLink(const std::string& u) {
    if (u.find(':') != std::string::npos) return u;
    const size_t hash = u.find('#');
    const std::string path = hash == std::string::npos ? u : u.substr(0, hash);
    const std::string rest = hash == std::string::npos ? "" : u.substr(hash);
    if (path.size() > 3 && path.compare(path.size() - 3, 3, ".md") == 0) return path.substr(0, path.size() - 3) + ".html" + rest;
    return u;
}

// Index of the '*' that closes an italic run starting at `from`, skipping `code` spans and ** markers.
size_t findClosingStar(const std::string& s, size_t from) {
    for (size_t j = from; j < s.size(); ++j) {
        if (s[j] == '`') {
            const size_t k = s.find('`', j + 1);
            if (k == std::string::npos) return std::string::npos;
            j = k;
        } else if (s[j] == '*') {
            if (j + 1 < s.size() && s[j + 1] == '*') { ++j; continue; }
            if (j > 0 && s[j - 1] != ' ') return j;
        }
    }
    return std::string::npos;
}

std::string inlineHtml(const std::string& s) {
    std::string out;
    size_t i = 0;
    while (i < s.size()) {
        const char c = s[i];
        if (c == '`') {
            const size_t j = s.find('`', i + 1);
            if (j != std::string::npos) {
                out += "<code>" + esc(s.substr(i + 1, j - i - 1)) + "</code>";
                i = j + 1;
                continue;
            }
        } else if (c == '*' && i + 1 < s.size() && s[i + 1] == '*') {
            const size_t j = s.find("**", i + 2);
            if (j != std::string::npos && j > i + 2) {
                out += "<strong>" + inlineHtml(s.substr(i + 2, j - i - 2)) + "</strong>";
                i = j + 2;
                continue;
            }
        } else if (c == '*' && (i == 0 || !std::isalnum(static_cast<unsigned char>(s[i - 1]))) &&
                   i + 1 < s.size() && s[i + 1] != ' ') {
            const size_t j = findClosingStar(s, i + 1);
            if (j != std::string::npos && j > i + 1) {
                out += "<em>" + inlineHtml(s.substr(i + 1, j - i - 1)) + "</em>";
                i = j + 1;
                continue;
            }
        } else if (c == '[') {
            const size_t j = s.find("](", i + 1);
            const size_t k = j == std::string::npos ? std::string::npos : s.find(')', j + 2);
            if (k != std::string::npos) {
                const std::string url = s.substr(j + 2, k - j - 2);
                if (safeUrl(url)) {
                    out += "<a href=\"" + esc(rewriteLink(url)) + "\">" + inlineHtml(s.substr(i + 1, j - i - 1)) + "</a>";
                    i = k + 1;
                    continue;
                }
            }
        }
        out += esc(std::string(1, c));
        ++i;
    }
    return out;
}

bool isFenceLine(const std::string& ln) { return trim(ln).rfind("```", 0) == 0; }

size_t headingLevel(const std::string& ln) {
    size_t n = 0;
    while (n < ln.size() && ln[n] == '#') ++n;
    return (n >= 1 && n <= 6 && n < ln.size() && ln[n] == ' ') ? n : 0;
}

// "- text", "* text", "+ text" or "1. text" (indented at most 3 spaces).
bool listItem(const std::string& ln, bool& ordered, std::string& text) {
    size_t k = 0;
    while (k < ln.size() && k < 3 && ln[k] == ' ') ++k;
    if (k + 1 < ln.size() && (ln[k] == '-' || ln[k] == '*' || ln[k] == '+') && ln[k + 1] == ' ') {
        ordered = false;
        text = ln.substr(k + 2);
        return true;
    }
    size_t d = k;
    while (d < ln.size() && std::isdigit(static_cast<unsigned char>(ln[d]))) ++d;
    if (d > k && d + 1 < ln.size() && ln[d] == '.' && ln[d + 1] == ' ') {
        ordered = true;
        text = ln.substr(d + 2);
        return true;
    }
    return false;
}

// Converts Markdown to HTML. Heading ids are `idPrefix` + a slug, made unique through `used`.
// Every heading below level 1 is also appended to `headings` (for the sidebar and the search index).
std::string markdownToHtml(const std::string& md, const std::string& idPrefix,
                           std::set<std::string>& used, std::vector<HtmlHeading>& headings) {
    const std::vector<std::string> lines = splitLines(md);
    std::ostringstream o;
    int pendingDesc = -1;
    size_t i = 0;
    while (i < lines.size()) {
        const std::string& ln = lines[i];
        if (trim(ln).empty()) { ++i; continue; }

        if (isFenceLine(ln)) {
            ++i;
            o << "<pre><code>";
            bool first = true;
            while (i < lines.size() && !isFenceLine(lines[i])) {
                if (!first) o << "\n";
                o << esc(lines[i]);
                first = false;
                ++i;
            }
            if (i < lines.size()) ++i;
            o << "</code></pre>\n";
            continue;
        }

        if (const size_t lvl = headingLevel(ln)) {
            const std::string text = trim(ln.substr(lvl + 1));
            if (lvl == 1) {
                o << "<h1>" << inlineHtml(text) << "</h1>\n";
            } else {
                const std::string plain = plainOf(text);
                const std::string base = idPrefix + slugOf(labelOf(plain));
                std::string id = base;
                int n = 2;
                while (!used.insert(id).second) id = base + "-" + std::to_string(n++);
                o << "<h" << lvl << " id=\"" << esc(id) << "\">" << inlineHtml(text) << "</h" << lvl << ">\n";
                HtmlHeading h;
                h.level = static_cast<int>(lvl);
                h.text = plain;
                h.id = id;
                headings.push_back(h);
                pendingDesc = static_cast<int>(headings.size()) - 1;
            }
            ++i;
            continue;
        }

        if (trim(ln) == "---") { o << "<hr>\n"; ++i; continue; }

        bool ordered = false;
        std::string item;
        if (listItem(ln, ordered, item)) {
            const bool ord = ordered;
            o << (ord ? "<ol>\n" : "<ul>\n");
            while (i < lines.size() && listItem(lines[i], ordered, item) && ordered == ord) {
                std::string text = item;
                ++i;
                // indented continuation lines belong to the same item
                while (i < lines.size() && !trim(lines[i]).empty() && !isFenceLine(lines[i]) &&
                       !headingLevel(lines[i]) && !listItem(lines[i], ordered, item)) {
                    text += " " + trim(lines[i]);
                    ++i;
                }
                o << "<li>" << inlineHtml(text) << "</li>\n";
            }
            o << (ord ? "</ol>\n" : "</ul>\n");
            continue;
        }

        std::string para = trim(ln);
        ++i;
        bool ord2 = false;
        std::string it2;
        while (i < lines.size() && !trim(lines[i]).empty() && !isFenceLine(lines[i]) &&
               !headingLevel(lines[i]) && !listItem(lines[i], ord2, it2)) {
            para += "\n" + trim(lines[i]);
            ++i;
        }
        o << "<p>" << inlineHtml(para) << "</p>\n";

        if (pendingDesc >= 0 && para.rfind("*Member of ", 0) != 0) {
            std::string d = plainOf(para);
            std::replace(d.begin(), d.end(), '\n', ' ');
            if (d.size() > 160) {
                size_t cut = 157;
                while (cut > 0 && (static_cast<unsigned char>(d[cut]) & 0xC0) == 0x80) --cut;
                d = d.substr(0, cut) + "...";
            }
            headings[static_cast<size_t>(pendingDesc)].desc = d;
            pendingDesc = -1;
        }
    }
    return o.str();
}

const char* const kCss = R"CSS(:root{--bg:#fff;--fg:#1f2328;--muted:#656d76;--side:#f6f8fa;--line:#d0d7de;--accent:#0969da;--code:#eff1f3}
@media (prefers-color-scheme:dark){:root{--bg:#0d1117;--fg:#e6edf3;--muted:#8b949e;--side:#161b22;--line:#30363d;--accent:#58a6ff;--code:#1f2630}}
*{box-sizing:border-box}
html{scroll-behavior:smooth}
body{margin:0;background:var(--bg);color:var(--fg);font:16px/1.6 system-ui,-apple-system,"Segoe UI",Roboto,sans-serif;display:flex;min-height:100vh}
a{color:var(--accent)}
.side{position:sticky;top:0;align-self:flex-start;height:100vh;width:300px;flex:none;overflow:auto;padding:16px;background:var(--side);border-right:1px solid var(--line)}
.side .lib{display:block;font-weight:700;font-size:1.15rem;color:var(--fg);text-decoration:none;margin-bottom:12px}
#q{width:100%;padding:8px 10px;border:1px solid var(--line);border-radius:6px;background:var(--bg);color:var(--fg);font:inherit;font-size:.9rem}
#results[hidden],#nav[hidden]{display:none}
.side ul{list-style:none;margin:8px 0 0;padding:0}
.side li a,#results a{display:block;padding:3px 8px;border-radius:5px;color:var(--fg);text-decoration:none;font-size:.9rem}
.side li a:hover,#results a:hover{background:var(--line)}
.side li.active>a{font-weight:600;color:var(--accent)}
.side ul.toc{margin:2px 0 6px 10px;border-left:1px solid var(--line)}
.side ul.toc a{color:var(--muted);font-size:.84rem;padding:2px 8px}
.side ul.toc li.l3 a,.side ul.toc li.l4 a{padding-left:20px}
#results{margin-top:8px}
#results .m{display:block;color:var(--muted);font-size:.78rem}
main{flex:1;min-width:0;max-width:900px;padding:32px 40px 80px}
h1,h2,h3,h4{line-height:1.25}
h2{margin-top:2.2rem;padding-top:.8rem;border-top:1px solid var(--line)}
h3{margin-top:1.6rem}
code{background:var(--code);padding:.15em .35em;border-radius:4px;font:.88em ui-monospace,SFMono-Regular,Consolas,monospace}
pre{background:var(--code);padding:12px 14px;border-radius:8px;overflow:auto}
pre code{background:none;padding:0}
h2 code,h3 code,h4 code{background:none;padding:0;font-size:.95em}
section{margin-bottom:3rem}
.modlist{padding-left:1.2rem}
.modlist li{margin:.3rem 0}
.modlist .s{color:var(--muted)}
@media (max-width:800px){body{display:block}.side{position:static;height:auto;width:auto;border-right:0;border-bottom:1px solid var(--line)}main{padding:20px}}
)CSS";

const char* const kSearchLogic = R"JS((function(){
var q=document.getElementById('q'),nav=document.getElementById('nav'),res=document.getElementById('results');
if(!q||typeof DOCS==='undefined')return;
function run(){
  var s=q.value.trim().toLowerCase();
  if(!s){res.hidden=true;nav.hidden=false;return;}
  var terms=s.split(/\s+/),hits=[];
  for(var i=0;i<DOCS.length;i++){
    var e=DOCS[i],t=e.t.toLowerCase(),hay=t+' '+e.m.toLowerCase()+' '+e.s.toLowerCase(),ok=true,score=0;
    for(var j=0;j<terms.length;j++){
      if(hay.indexOf(terms[j])<0){ok=false;break;}
      if(t.indexOf(terms[j])===0)score+=3;else if(t.indexOf(terms[j])>=0)score+=2;else score+=1;
    }
    if(ok)hits.push({e:e,score:score});
  }
  hits.sort(function(a,b){return b.score-a.score;});
  res.textContent='';
  if(!hits.length){var p=document.createElement('p');p.textContent='No results';res.appendChild(p);}
  hits.slice(0,40).forEach(function(h){
    var a=document.createElement('a');a.href=h.e.u;
    var t=document.createElement('span');t.textContent=h.e.t;
    var m=document.createElement('span');m.className='m';m.textContent=h.e.m;
    a.appendChild(t);a.appendChild(m);
    a.addEventListener('click',function(){q.value='';run();});
    res.appendChild(a);
  });
  res.hidden=false;nav.hidden=true;
}
q.addEventListener('input',run);
document.addEventListener('keydown',function(ev){
  if(ev.key==='/'&&document.activeElement!==q){ev.preventDefault();q.focus();}
  if(ev.key==='Escape'){q.value='';run();q.blur();}
});
})();
)JS";

std::string searchData(const std::vector<HtmlPage>& pages) {
    std::ostringstream o;
    o << "var DOCS=[";
    bool first = true;
    auto add = [&](const std::string& t, const std::string& m, const std::string& u, const std::string& s) {
        if (!first) o << ",";
        first = false;
        o << "{\"t\":\"" << jsonEsc(t) << "\",\"m\":\"" << jsonEsc(m) << "\",\"u\":\"" << jsonEsc(u)
          << "\",\"s\":\"" << jsonEsc(s) << "\"}";
    };
    for (const auto& p : pages) {
        add(p.name, p.name, p.href, plainOf(p.summary));
        const std::string base = p.href.rfind('#', 0) == 0 ? std::string() : p.href;   // one-page output: href is "#anchor"
        for (const auto& h : p.headings) add(h.text, p.name, base + "#" + h.id, h.desc);
    }
    o << "];\n";
    return o.str();
}

std::string navList(const std::vector<HtmlPage>& pages, size_t active, bool withToc) {
    std::ostringstream o;
    o << "<ul>\n";
    for (size_t i = 0; i < pages.size(); ++i) {
        const auto& p = pages[i];
        o << "<li class=\"mod" << (i == active ? " active" : "") << "\"><a href=\"" << esc(p.href) << "\">"
          << esc(p.name) << "</a>";
        if (i == active && withToc && !p.headings.empty()) {
            o << "\n<ul class=\"toc\">\n";
            for (const auto& h : p.headings) {
                o << "<li class=\"l" << h.level << "\"><a href=\"#" << esc(h.id) << "\">"
                  << esc(labelOf(h.text)) << "</a></li>\n";
            }
            o << "</ul>\n";
        }
        o << "</li>\n";
    }
    o << "</ul>";
    return o.str();
}

std::string moduleList(const std::vector<HtmlPage>& pages) {
    std::ostringstream o;
    o << "<ul class=\"modlist\">\n";
    for (const auto& p : pages) {
        o << "<li><a href=\"" << esc(p.href) << "\">" << esc(p.name) << "</a>";
        if (!p.summary.empty()) o << " <span class=\"s\">— " << inlineHtml(p.summary) << "</span>";
        o << "</li>\n";
    }
    o << "</ul>\n";
    return o.str();
}

std::string htmlPage(const std::string& title, const std::string& libName, const std::string& nav,
                     const std::string& mainHtml, bool inlineAssets, const std::string& searchJs,
                     const std::string& homeHref) {
    std::ostringstream o;
    o << "<!doctype html>\n<html lang=\"en\">\n<head>\n<meta charset=\"utf-8\">\n"
      << "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n"
      << "<title>" << esc(title) << "</title>\n";
    if (inlineAssets) o << "<style>\n" << kCss << "</style>\n";
    else o << "<link rel=\"stylesheet\" href=\"style.css\">\n";
    o << "</head>\n<body>\n<aside class=\"side\">\n"
      << "<a class=\"lib\" href=\"" << esc(homeHref) << "\">" << esc(libName) << "</a>\n"
      << "<input id=\"q\" type=\"search\" placeholder=\"Search (press /)\" autocomplete=\"off\">\n"
      << "<div id=\"results\" hidden></div>\n"
      << "<nav id=\"nav\">\n" << nav << "\n</nav>\n</aside>\n<main>\n" << mainHtml << "</main>\n";
    if (inlineAssets) o << "<script>\n" << searchJs << "</script>\n";
    else o << "<script src=\"search.js\"></script>\n";
    o << "</body>\n</html>\n";
    return o.str();
}

bool writeText(const fs::path& file, const std::string& text, std::string& error) {
    std::ofstream f(file, std::ios::binary);
    if (!f) {
        error = "Cannot write '" + file.string() + "'";
        return false;
    }
    f << text;
    return true;
}

// Renders every module to HTML. split: <outPath>/ gets index.html, one page per module,
// style.css and search.js. Otherwise <outPath> is one self-contained .html file.
bool writeLibraryHtml(const std::vector<ModuleFile>& modules, const std::string& libName,
                      const std::string& intro, const std::string& outPath, bool split, std::string& error) {
    std::vector<HtmlPage> pages;
    std::set<std::string> sharedIds;                  // one-page output: ids must be unique across the whole page
    for (const auto& m : modules) {
        std::string md, summary, err;
        size_t entries = 0;
        if (!renderFile(m.source.string(), m.name, 0, md, summary, entries, err)) {
            error = err;
            return false;
        }
        HtmlPage p;
        p.name = m.name;
        p.summary = summary;
        p.anchor = anchorOf(m.name);
        p.href = split ? m.relMd.substr(0, m.relMd.size() - 3) + ".html" : "#" + p.anchor;
        std::set<std::string> ownIds;
        p.body = markdownToHtml(md, split ? std::string() : p.anchor + "-", split ? ownIds : sharedIds, p.headings);
        pages.push_back(std::move(p));
    }

    const std::string data = searchData(pages);
    std::string head = "<h1>" + esc(libName) + "</h1>\n";
    if (!intro.empty()) head += "<p>" + inlineHtml(intro) + "</p>\n";
    head += moduleList(pages);

    std::error_code ec;
    if (split) {
        fs::create_directories(outPath, ec);
        const fs::path dir(outPath);
        if (!writeText(dir / "style.css", kCss, error)) return false;
        if (!writeText(dir / "search.js", data + kSearchLogic, error)) return false;
        const std::string title = libName + " documentation";
        if (!writeText(dir / "index.html",
                       htmlPage(title, libName, navList(pages, pages.size(), false), head, false, "", "index.html"), error))
            return false;
        for (size_t i = 0; i < pages.size(); ++i) {
            const std::string pageTitle = pages[i].name + " - " + libName;
            if (!writeText(dir / pages[i].href,
                           htmlPage(pageTitle, libName, navList(pages, i, true), pages[i].body, false, "", "index.html"), error))
                return false;
        }
        return true;
    }

    std::string all = head;
    for (const auto& p : pages) all += "<section id=\"" + esc(p.anchor) + "\">\n" + p.body + "</section>\n";
    const fs::path file(outPath);
    if (file.has_parent_path()) fs::create_directories(file.parent_path(), ec);
    return writeText(file,
                     htmlPage(libName + " documentation", libName, navList(pages, pages.size(), false), all, true,
                              data + kSearchLogic, "#"),
                     error);
}

} // namespace

std::string resolveDocTarget(const std::string& name) {
    std::string path = name;
    if (path.rfind("std/", 0) == 0 || path.rfind("std\\", 0) == 0) {
        path = "standard/1/" + path.substr(4);
    }
    const size_t slash = path.find_last_of("/\\");
    const size_t dot = path.find_last_of('.');
    if (dot == std::string::npos || (slash != std::string::npos && dot < slash)) path += ".os";
    return path;
}

bool generateDocsMarkdown(const std::string& path, std::string& markdown, std::string& error) {
    std::string summary;
    size_t entries = 0;
    return renderFile(path, stemOf(path), 0, markdown, summary, entries, error);
}

bool generateDocsHtml(const std::string& path, std::string& html, std::string& error) {
    const std::string name = stemOf(path);
    std::string md, summary;
    size_t entries = 0;
    if (!renderFile(path, name, 0, md, summary, entries, error)) return false;

    HtmlPage p;
    p.name = name;
    p.summary = summary;
    p.anchor = anchorOf(name);
    p.href = "#" + p.anchor;
    std::set<std::string> ids;
    p.body = markdownToHtml(md, p.anchor + "-", ids, p.headings);

    const std::vector<HtmlPage> pages{p};
    html = htmlPage(name, name, navList(pages, 0, true),
                    "<section id=\"" + esc(p.anchor) + "\">\n" + p.body + "</section>\n",
                    true, searchData(pages) + kSearchLogic, "#");
    return true;
}

bool generateLibraryDocs(const std::string& rootFile, const std::string& outPath,
                         bool split, bool html, std::string& error) {
    std::error_code ec;
    const fs::path root(rootFile);

    ScanResult rootScan;
    if (!scanImports(rootFile, rootScan, error)) return false;
    const std::string libName = rootScan.topModule.empty() ? stemOf(rootFile) : rootScan.topModule;

    // The root file itself: its `//!` line becomes the library's intro, and it is listed as a
    // module of its own only if it declares documented things.
    std::string rootBody, rootSummary;
    size_t rootEntries = 0;
    if (!renderFile(rootFile, libName, split ? 0 : 1, rootBody, rootSummary, rootEntries, error)) return false;

    // Everything reachable from the root through imports and includes.
    std::set<std::string> visited;
    visited.insert(toLower(fs::weakly_canonical(root, ec).string()));
    std::vector<ModuleFile> modules;
    if (!collectModules(root, "", visited, modules, error)) return false;
    if (rootEntries > 0) modules.push_back(ModuleFile{libName, libName + ".md", root});
    if (modules.empty()) {
        error = "'" + rootFile + "' imports no files and has no documented declarations";
        return false;
    }

    // Alphabetical, case-insensitive.
    std::sort(modules.begin(), modules.end(), [](const ModuleFile& a, const ModuleFile& b) {
        const std::string la = toLower(a.name), lb = toLower(b.name);
        return la != lb ? la < lb : a.name < b.name;
    });

    if (html) return writeLibraryHtml(modules, libName, rootSummary, outPath, split, error);

    std::ostringstream index;     // index lines, "- [name](target) — summary"
    std::ostringstream all;       // single-file mode: every module, one after the other
    if (split) fs::create_directories(outPath, ec);

    for (const auto& m : modules) {
        std::string body, summary, err;
        size_t entries = 0;
        if (!renderFile(m.source.string(), m.name, split ? 0 : 1, body, summary, entries, err)) {
            error = err;
            return false;
        }

        const std::string target = split ? m.relMd : "#" + anchorOf(m.name);
        index << "- [" << m.name << "](" << target << ")";
        if (!summary.empty()) index << " — " << summary;
        index << "\n";

        if (split) {
            const fs::path file = fs::path(outPath) / m.relMd;
            std::ofstream f(file, std::ios::binary);
            if (!f) {
                error = "Cannot write '" + file.string() + "'";
                return false;
            }
            f << body;
        } else {
            all << body;
        }
    }

    const std::string intro = "# " + libName + "\n\n" + (rootSummary.empty() ? "" : rootSummary + "\n\n");
    if (split) {
        const fs::path file = fs::path(outPath) / "index.md";
        std::ofstream f(file, std::ios::binary);
        if (!f) {
            error = "Cannot write '" + file.string() + "'";
            return false;
        }
        f << intro << index.str();
    } else {
        const fs::path file(outPath);
        if (file.has_parent_path()) fs::create_directories(file.parent_path(), ec);
        std::ofstream f(file, std::ios::binary);
        if (!f) {
            error = "Cannot write '" + outPath + "'";
            return false;
        }
        f << intro << "## Index\n\n" << index.str() << "\n" << all.str();
    }
    return true;
}

} // namespace Omniscript