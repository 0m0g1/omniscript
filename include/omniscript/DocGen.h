// include/omniscript/DocGen.h
#pragma once
#include <cstddef>
#include <string>
#include <vector>

namespace Omniscript {

// Reads an Omniscript source file and turns its `///` and `//!` comments into Markdown.
// Only the lexer is used, so it works without compiling or even fully parsing the file.
// Returns false and fills `error` if the file cannot be read.
bool generateDocsMarkdown(const std::string& path, std::string& markdown, std::string& error);

// Same as generateDocsMarkdown, but one self-contained HTML page (sidebar, search, no other files).
bool generateDocsHtml(const std::string& path, std::string& html, std::string& error);

// One search result.
struct DocHit {
    int score = 0;
    std::string kind;        // module, function, method, struct, class, enum, field, constant, variable, item
    std::string name;        // bare name ("trim")
    std::string module;      // "std/String"
    std::string signature;   // "String.trim() => void" (members are shown as Owner.member)
    std::string summary;     // first line of the doc
    std::string file;
    size_t line = 0;
    bool documented = true;  // false: declaration without a `///` comment
};

// Declarations WITHOUT a `///` comment are skipped, unless `all` is true or the whole scope (library for
// search/list, file for show) has no doc comments at all - then its declarations are shown instead, and
// `note` (search) / the output (show, list) says so.

// Finds declarations of a library (root file plus everything it imports). All words of `query` must match
// (name, owner, module, signature or doc text; "String.trim" and "std/IO" work too); better matches come
// first. `kindFilter` is "" or one of: module, function, method, struct, class, enum, field, constant,
// variable, type (struct/class/enum), member (method/field).
bool findLibraryDocs(const std::string& rootFile, const std::string& query, const std::string& kindFilter, bool all,
                     std::vector<DocHit>& hits, size_t& moduleCount, size_t& itemCount, std::string& note,
                     std::string& error);

// Turns search results into console text: a header, then per hit a kind badge + signature, the summary
// and the location. `limit` 0 means 20. `color` adds ANSI colors (pass console.isColorEnabled()).
std::string formatDocHits(const std::vector<DocHit>& hits, const std::string& query, const std::string& kindFilter,
                          size_t limit, size_t moduleCount, size_t itemCount, const std::string& note, bool color);

// findLibraryDocs + formatDocHits in one call; the result is ready to print.
bool searchLibraryDocs(const std::string& rootFile, const std::string& query, const std::string& kindFilter,
                       size_t limit, std::string& output, std::string& error, bool color = false, bool all = false);

// Console listing of everything in ONE file, in source order, members indented under their owner: kind
// badge, signature, first doc line and line number. `title` is shown in the header (the path as typed).
bool showFileDocs(const std::string& path, const std::string& title, const std::string& kindFilter, bool all,
                  bool color, std::string& output, std::string& error);

// Lists every declaration of a library (root file plus everything it imports) as one compact Markdown
// catalog in `output`: a section per module, one line per item (signature plus the first line of its doc).
bool listLibraryDocs(const std::string& rootFile, const std::string& kindFilter,
                     std::string& output, std::string& error, bool all = false);

// Maps a user-facing name to a file: "std/Console" -> "standard/1/Console.os".
// Anything that does not start with "std/" is returned unchanged (".os" is added if it has no extension).
std::string resolveDocTarget(const std::string& name);

// Documents a whole library: pass its root file (e.g. "standard/1/std.os") and every file it
// pulls in is documented too, following `module X = import "p";`, `import "p";` and `include "p";`
// recursively (each file once; C headers are skipped). Modules are named by their declared
// names ("std/Console") and sorted alphabetically (case-insensitive).
//  - split == false: `outPath` is a single file with an index at the top.
//  - split == true : `outPath` is a directory; one page per module plus index.
//  - html == false : Markdown (.md files).
//  - html == true  : self-contained HTML with a sidebar and search. Split output also gets
//                    style.css and search.js next to the pages; the single file needs nothing else.
// Returns false and fills `error` on the first file or directory problem.
bool generateLibraryDocs(const std::string& rootFile, const std::string& outPath,
                         bool split, bool html, std::string& error);

} // namespace Omniscript