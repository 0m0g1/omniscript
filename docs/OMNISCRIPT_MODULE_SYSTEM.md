# OmniScript Module System

How `import` works today, what is designed but not built yet, and the rules the standard library follows.

Status legend: **Implemented** works in the compiler now. **Stub** has an interface and a TODO. **Planned** is design only.

---

## 1. Importing

```os
import std.console;                 // logical name, dotted, no quotes
import std.net.http as http;        // alias
import * from std.vulkan;           // flatten declarations into the current scope
import { sqrt, pow } from std.math; // parsed; see "Known limitations"
import std;                         // the std facade (standard/<edition>/std.os)
import "./helpers.os";              // explicit file path (quoted)
import "vulkan_min.h";              // C header: treated as `include`
```

Rules:

- **Dotted identifiers are logical names.** `std.net.http` never mentions the disk layout, the edition directory or `.os`.
- **Quoted strings are file paths.** Use them only for files you ship yourself. The parser records which form was used (`ImportModule::isPathImport`).
- **A dotted name is not a string.** `import "std.console"` is treated as a file path and will not find anything.

### `import` versus `include`

| | `include "x.os";` | `import x;` |
|---|---|---|
| Mechanism | Textual: declarations are expressed in the including scope | Namespaced: declarations live under the module name |
| Name prefix | none | `x.name` (or alias) |
| Loaded once | no | by canonical module id (see 3) |

`import * from x;` on a file with no `module` wrapper behaves like `include`: names are unprefixed, and a repeated wildcard import of the same file in the same scope is skipped.

`import { a, b as c } from x;` and `import * from x;` on a `module`-wrapped file load the module normally, then re-register the chosen public members (functions, constants, variables, types) under the unprefixed or aliased name. Unknown members give an error listing the public members; private members cannot be imported.

---

## 2. Module files

A file is a module in one of two shapes.

**Unwrapped (the standard-library convention).** The file *is* the module. Top-level declarations only; the module name comes from the import.

```os
// standard/1/vulkan.os
extern "dependencies/vk/vulkan_full.h", "vulkan-1.dll" { }
const GLFW_NO_API: int = 0;
```

```os
import std.vulkan;            // vulkan.GLFW_NO_API
import std.vulkan as vk;      // vk.GLFW_NO_API
import * from std.vulkan;     // GLFW_NO_API
```

Allowed at top level: `fn`, `const`/`let` declarations, `struct`, `class`, `extern` blocks, `include`. Anything else (a bare call, for example) is an error, because module files must not execute code on load.

**Wrapped.** The file contains `module Name { ... }`. Use this for facades and for several modules in one file.

```os
// standard/1/std.os
module std {
    public module Console = import "standard/1/console.os";
}
```

Why unwrapped is the default: the importer chooses the prefix. A wrapper would force `std.vulkan.Vk.vkCreateInstance`.

If a file has exactly one `module` block, it is the module regardless of the import's casing (`import std.console` finds `module Console`).

The nested form `module X = import "..."` still takes a quoted path. It stays namespaced under `X`.

---

## 3. Resolution (Implemented)

`ImportModule::express()` asks `ModuleResolver::global()` for the module. Sources are tried in order and every attempt is recorded for the error message:

| # | Source | Matches | Looks in |
|---|---|---|---|
| 1 | Explicit path | quoted imports | as given, then next to the importing file; appends `.os` when there is no extension |
| 2 | Standard library | `std`, `std.*`, and legacy `"std/x.os"` | `<toolchain>/standard/<edition>/<name>.os`, then `<name>/mod.os` |
| 3 | Own package | `<package-name>.*` | `<package-root>/src/<name>.os`, then `<name>/mod.os` |
| 4 | Dependencies | first segment equals a manifest dependency alias | `path` dependencies only today |

Std is checked before packages, so a dependency cannot shadow it.

**Naming.** Module names and file names are lowercase and identical (`std.console` is `console.os`). Types are PascalCase. Resolution first tries the exact spelling, then a lowercase fallback, so existing `Math.os` files keep working while you rename them.

**Canonical module id.** Every loaded module has `(package, version, modulePath)`, written `std@1/json`. Explicit path imports use package `path` and the canonical absolute path. The id is used for:

- cycle detection (`beginLoad` / `endLoad`): a cycle is reported as "Circular import of module ...";
- deduplication: the id is turned into a registry key (`ModuleId::registryKey()`, e.g. `std/v1/json`) and registered with `SymbolTable::defineModule()`. Importing a module that is already expressed into the *same scope* does not express it again; it only binds names. A different scope (for example inside a function) loads it again.

**Errors.** A failed import lists each location searched:

```
Module 'std.consol' not found.
Searched:
[explicit path]
[std (edition 1)]
  standard/1/consol.os
  standard/1/consol/mod.os
```

---

## 4. Packages and manifest (Stub)

Interfaces exist in `Modules/PackageManifest.h`; `findManifest()` currently returns "no manifest", so every program runs in single-file script mode (std and explicit paths only).

Planned manifest, `omni.json` (comments and trailing commas allowed):

```json
{
  "name": "wordtop",
  "version": "0.3.0",
  "edition": "1",
  "main": "src/main.os",
  "dependencies": {
    "json2": { "package": "acme.json", "version": "^2" },
    "local": { "path": "../local-lib" }
  }
}
```

- One canonical format (JSON). A `ManifestReader` interface allows other formats later.
- `name` makes `import <name>.x` resolve to `src/x.os`.
- A dependency key is the alias used in source: `import json2.parse;`.

---

## 5. Versioning (Planned)

Three layers, kept separate:

| Layer | Pinned by | Notes |
|---|---|---|
| Edition | `edition` in the manifest | Selects `standard/<edition>/`. Within an edition std only adds; breaking changes need a new edition. |
| Std | the toolchain | Tied to the edition. |
| Packages | manifest + lockfile | Semver; one version per major, so several majors can coexist as different canonical ids. |

- **No versions in import statements.** `import std.json` is the same in every file.
- Types from different versions are distinct types.
- Single-file scripts with no manifest use edition `"1"`.

### Lockfile, `omni.lock.json` (Planned)

Machine-written, sorted keys, committed to version control. Each entry records `source`, `integrity` (verified on every load) and its resolved `dependencies`. See `Modules/Lockfile.h`.

### Remote sources (Planned)

URLs are **not** import targets. A URL is a dependency *source* in the manifest, pinned by an integrity hash in the lockfile. Source code still says `import json2.parse;`. Reserved dependency kinds: `registry`, `path` (works now), `url`, `git`.

---

## 6. Files

| File | Role |
|---|---|
| `Statements/ModuleAndImportStatements.h/.cpp` | `ImportModule`, `CreateModule`, `IncludeStatement`; `express()` uses the resolver |
| `Parser/ParseModulesAndImports.cpp` | dotted-name parsing, `as`, selective/wildcard forms |
| `Modules/ModuleId.h` | canonical id and registry key |
| `Modules/Sha256.h` | SHA-256 used for content hashes |
| `Modules/ModuleSource.h` | `ModuleSource` interface, request/result types |
| `Modules/ModuleResolver.h/.cpp` | resolver, path/std/package/path-dependency sources |
| `Modules/PackageManifest.h/.cpp` | manifest structs, reader interface, `findManifest` stub |
| `Modules/Lockfile.h` | lockfile structs |

`ModuleResolver::configureGlobal(toolchainRoot, manifest, lock)` lets the driver set the toolchain root before compiling. Without it the root is the working directory (`"."`), matching the old hardcoded `standard/1/`.

---

## 7. Known limitations

- **Selective and wildcard imports bind by name.** The module is still expressed completely under its own name; `import { a }` adds `a` as an extra name for `Mod.a`. Members that are not registered in the symbol table under `Mod.<name>` (for example some nested module imports) are skipped by `import *`.
- **Visibility:** declarations in unwrapped files get default `MemberModifiers` (public). `private` at the top level of a module file is not parsed yet, so every top-level item is importable.
- **Extern-heavy files** (such as `vulkan.os`) are safest used with `import * from` or `include`, which keep the declared names unprefixed. Used as a namespaced module, their functions are registered as `vulkan.vkCreateInstance`; check that the JIT still resolves the native symbol from the unprefixed name before relying on that.
- **`extern` paths** in std files (`dependencies/...`) resolve against the module file's own directory, not the package root.
- **Dedup is per scope and per process:** nothing is cached across runs, and a REPL needs `SymbolTable::unloadModule(registryKey)` to reload a changed file.
- **Package manager:** `omni.json` reading, the lockfile, and registry/url/git dependencies are interfaces only. `path` dependencies resolve but their own manifests are not read.
- **Parse-time module read:** the nested form `module X = import "..."` still reads and parses its file during parsing, by plain path, and does not use the resolver.