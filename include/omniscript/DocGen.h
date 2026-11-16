// include/omniscript/DocGen.h
#pragma once
#include <string>

namespace Omniscript {

// Reads an Omniscript source file and turns its `///` and `//!` comments into Markdown.
// Only the lexer is used, so it works without compiling or even fully parsing the file.
// Returns false and fills `error` if the file cannot be read.
bool generateDocsMarkdown(const std::string& path, std::string& markdown, std::string& error);

// Same as generateDocsMarkdown, but one self-contained HTML page (sidebar, search, no other files).
bool generateDocsHtml(const std::string& path, std::string& html, std::string& error);

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