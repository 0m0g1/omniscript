#include <omniscript/Engine.h>
#include <omniscript/Lexer.h>
#include <omniscript/Parser.h>
#include <omniscript/DocGen.h>
#include <omniscript/extern/ExternResolver.h>
#include <omniscript/Statements/ExternHeaderBlock.h>
#include <filesystem>
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <vector>

namespace Omniscript {

namespace fs = std::filesystem;

namespace {

// Checks the file a --doc* option was pointed at and explains what is wrong in plain words.
// Returns "" when the file can be read. `given` is what the user typed, `resolved` what it maps to.
std::string pathProblem(const std::string& option, const std::string& given, const std::string& resolved) {
    std::error_code ec;
    const fs::path p(resolved);
    std::string shown = "'" + resolved + "'";
    if (given != resolved) shown += " (from '" + given + "')";

    if (!fs::exists(p, ec)) {
        std::string msg = option + ": cannot open " + shown + " - file not found.";
        if (!resolved.empty() && (resolved[0] == '/' || resolved[0] == '\\')) {
            if (fs::exists(fs::path("." + resolved), ec)) {
                msg += "\n  Did you mean '." + resolved + "'? A leading '/' means the root of the drive, not this folder.";
            } else {
                msg += "\n  A leading '/' means the root of the drive. Use a relative path such as ./folder/file.os instead.";
            }
        } else {
            msg += "\n  Paths are relative to the folder you run from (" + fs::current_path(ec).string() + ").";
        }
        return msg;
    }
    if (fs::is_directory(p, ec)) {
        return option + ": " + shown + " is a folder, but a source file (like std.os) is needed.";
    }
    std::ifstream probe(resolved, std::ios::binary);
    if (!probe) return option + ": cannot read " + shown + " - permission denied or the file is locked.";
    return "";
}

// Number of single-character edits between two strings (for "did you mean" suggestions).
size_t editDistance(const std::string& a, const std::string& b) {
    std::vector<size_t> prev(b.size() + 1), cur(b.size() + 1);
    for (size_t j = 0; j <= b.size(); ++j) prev[j] = j;
    for (size_t i = 1; i <= a.size(); ++i) {
        cur[0] = i;
        for (size_t j = 1; j <= b.size(); ++j) {
            const size_t sub = prev[j - 1] + (a[i - 1] == b[j - 1] ? 0 : 1);
            cur[j] = std::min({prev[j] + 1, cur[j - 1] + 1, sub});
        }
        std::swap(prev, cur);
    }
    return prev[b.size()];
}

// The known option closest to what was typed, or "" if nothing is close.
std::string closestOption(const std::string& arg, const std::vector<std::string>& options) {
    std::string best;
    size_t bestDistance = arg.size();
    const size_t limit = std::max<size_t>(1, std::min<size_t>(3, arg.size() / 3));
    for (const auto& o : options) {
        if (arg.size() >= 4 && o.rfind(arg, 0) == 0) return o;              // "--doc-s" -> "--doc-search"
        const size_t d = editDistance(arg, o);
        if (d < bestDistance) { bestDistance = d; best = o; }
    }
    return bestDistance <= limit ? best : std::string();
}

// What an option expects as its value, for error messages.
std::string optionHint(const std::string& o) {
    if (o == "--target" || o == "--target-os") return "one of: linux, windows, macos, freebsd, android, ios, wasm";
    if (o == "--target-arch") return "one of: x86_64, arm64, x86_32, arm32, riscv64, wasm32, wasm64";
    if (o == "-O" || o == "--optimization-level") return "a number from 0 to 3, e.g. -O 2";
    if (o == "--heap-size" || o == "--stack-size") return "a size such as 64MB, 512K or 2GB";
    if (o == "--parallel-jobs") return "a whole number, e.g. --parallel-jobs 4";
    if (o == "--max-errors") return "a whole number, e.g. --max-errors 20";
    if (o == "--gc") return "one of: none, refcounting, marksweep, generational, incremental";
    if (o == "--safety") return "one of: unsafe, minimal, standard, paranoid";
    if (o == "--main-source" || o == "--source") return "a source file path";
    if (o == "--enable-modules" || o == "--cache-dir") return "a folder path";
    if (o == "--error-log") return "a file path for the log";
    if (o == "--toolchain") return "a folder path (for android: the NDK root)";
    if (o == "--target-triple") return "a target triple such as x86_64-pc-windows-msvc";
    if (o == "--profiler") return "a profiler type such as perf (optionally followed by an output path)";
    return "";
}

bool hasFlag(int argc, char* argv[], const char* flag) {
    for (int j = 1; j < argc; ++j) if (std::string_view(argv[j]) == flag) return true;
    return false;
}

std::string describeOptionError(const std::string& option, const std::string& value, bool hasValue, bool missing) {
    const std::string hint = optionHint(option);
    std::string msg = (missing || !hasValue) ? option + " needs a value" : "Invalid value '" + value + "' for " + option;
    if (!hint.empty()) msg += ": expected " + hint;
    return msg + ".";
}

} // namespace

thread_local Engine::ExecutionStats Engine::currentStats_;

const std::unordered_map<std::string_view, std::function<Engine::ParseResult(Config&, std::span<char*>&)>> 
Engine::ArgumentParser::argHandlers = {
    {"--main-source", [](Config& config, std::span<char*>& args) {
        if (args.empty()) return ParseResult::MissingValue;
        config.mainSourceFile = args[0];
        config.sourcePaths.push_back(args[0]);
        args = args.subspan(1);
        return ParseResult::Success;
    }},
    {"--source", [](Config& config, std::span<char*>& args) {
        if (args.empty()) return ParseResult::MissingValue;
        config.sourcePaths.push_back(args[0]);
        args = args.subspan(1);
        return ParseResult::Success;
    }},
    {"--enable-modules", [](Config& config, std::span<char*>& args) {
        if (args.empty()) return ParseResult::MissingValue;
        config.modules.enableModules = true;
        config.modules.modulePaths.push_back(args[0]);
        args = args.subspan(1);
        return ParseResult::Success;
    }},
    {"--incremental", [](Config& config, std::span<char*>&) {
        config.incremental.enabled = true;
        return ParseResult::Success;
    }},
    {"--cache-dir", [](Config& config, std::span<char*>& args) {
        if (args.empty()) return ParseResult::MissingValue;
        config.enableCaching = true;
        config.cacheDirectory = args[0];
        args = args.subspan(1);
        return ParseResult::Success;
    }},
    {"--parallel-jobs", [](Config& config, std::span<char*>& args) {
        if (args.empty()) return ParseResult::MissingValue;
        try {
            config.parallelJobs = std::stoi(args[0]);
            args = args.subspan(1);
            return ParseResult::Success;
        } catch (...) {
            return ParseResult::InvalidValue;
        }
    }},
    {"--profiler", [](Config& config, std::span<char*>& args) {
        if (args.empty()) return ParseResult::MissingValue;
        config.profiler.type = ProfilerType::Perf; // Simplified
        if (!args.subspan(1).empty() && args[1][0] != '-') {
            config.profiler.outputPath = args[1];
            args = args.subspan(2);
        } else {
            args = args.subspan(1);
        }
        config.diagnostics.enableProfiling = true;
        return ParseResult::Success;
    }},
    {"--error-log", [](Config& config, std::span<char*>& args) {
        if (args.empty()) return ParseResult::MissingValue;
        config.errorHandling.logToFile = true;
        config.errorHandling.errorLogPath = args[0];
        args = args.subspan(1);
        return ParseResult::Success;
    }},
    {"--max-errors", [](Config& config, std::span<char*>& args) {
        if (args.empty()) return ParseResult::MissingValue;
        try {
            config.errorHandling.maxErrorCount = std::stoi(args[0]);
            args = args.subspan(1);
            return ParseResult::Success;
        } catch (...) {
            return ParseResult::InvalidValue;
        }
    }},
    {"--debug", handleDebugArgs},
    {"-d", handleDebugArgs},
    // Handlers in this table receive `args` starting at the token AFTER the option, so args[0] is the
    // option's value. (The old handleTargetArgs/handleOptimizationArgs/... functions compared args[0]
    // against the option name with `==` on a char*, which compares pointers and is always false, so
    // --target-os, -O, --emit-sharedlib, --gc, --safety etc. silently did nothing.)
    {"--target", [](Config& config, std::span<char*>& args) {
        if (args.empty()) return ParseResult::MissingValue;
        const std::string_view name = args[0];
        const auto it = Engine::ArgumentParser::osMap.find(name);
        if (it == Engine::ArgumentParser::osMap.end()) return ParseResult::InvalidValue;
        config.targetOS = it->second;
        if (config.targetOS == TargetOS::Android) {
            // Android builds are 64-bit ARM shared libraries loaded by the app, not executables.
            if (config.targetArch == TargetArch::Auto) config.targetArch = TargetArch::ARM64;
            config.aot.outputFormat = OutputFormat::SharedLib;
        }
        config.mode = CompileMode::AOT;
        args = args.subspan(1);
        return ParseResult::Success;
    }},
    {"--toolchain", [](Config& config, std::span<char*>& args) {
        if (args.empty()) return ParseResult::MissingValue;
        config.toolchainPath = args[0];   // folder with the linker/compiler tools, or the Android NDK root
        args = args.subspan(1);
        return ParseResult::Success;
    }},
    {"--target-arch", [](Config& config, std::span<char*>& args) {
        if (args.empty()) return ParseResult::MissingValue;
        config.targetArch = parseTargetArch(args[0]);
        args = args.subspan(1);
        return ParseResult::Success;
    }},
    {"--target-os", [](Config& config, std::span<char*>& args) {
        if (args.empty()) return ParseResult::MissingValue;
        config.targetOS = parseTargetOS(args[0]);
        args = args.subspan(1);
        return ParseResult::Success;
    }},
    {"--target-triple", [](Config& config, std::span<char*>& args) {
        if (args.empty()) return ParseResult::MissingValue;
        config.targetTriple = args[0];   // the exact triple the user asked for wins over the generated one
        auto [arch, os] = parseTargetTriple(args[0]);
        config.targetArch = arch;
        config.targetOS = os;
        args = args.subspan(1);
        return ParseResult::Success;
    }},
    {"--optimization-level", [](Config& config, std::span<char*>& args) {
        if (args.empty()) return ParseResult::MissingValue;
        try {
            config.optimization.level = std::stoi(args[0]);
            args = args.subspan(1);
            return ParseResult::Success;
        } catch (...) {
            return ParseResult::InvalidValue;
        }
    }},
    {"-O", [](Config& config, std::span<char*>& args) {
        if (args.empty()) return ParseResult::MissingValue;
        try {
            config.optimization.level = std::stoi(args[0]);
            args = args.subspan(1);
            return ParseResult::Success;
        } catch (...) {
            return ParseResult::InvalidValue;
        }
    }},
    {"--gc", [](Config& config, std::span<char*>& args) {
        if (args.empty()) return ParseResult::MissingValue;
        config.runtime.gcStrategy = parseGCStrategy(args[0]);
        args = args.subspan(1);
        return ParseResult::Success;
    }},
    {"--safety", [](Config& config, std::span<char*>& args) {
        if (args.empty()) return ParseResult::MissingValue;
        config.runtime.safetyLevel = parseSafetyLevel(args[0]);
        args = args.subspan(1);
        return ParseResult::Success;
    }},
    {"--heap-size", [](Config& config, std::span<char*>& args) {
        if (args.empty()) return ParseResult::MissingValue;
        auto sizeResult = parseSizeString(args[0]);
        if (!sizeResult) return ParseResult::InvalidValue;
        config.runtime.heapSize = *sizeResult;
        args = args.subspan(1);
        return ParseResult::Success;
    }},
    {"--stack-size", [](Config& config, std::span<char*>& args) {
        if (args.empty()) return ParseResult::MissingValue;
        auto sizeResult = parseSizeString(args[0]);
        if (!sizeResult) return ParseResult::InvalidValue;
        config.runtime.stackSize = *sizeResult;
        args = args.subspan(1);
        return ParseResult::Success;
    }},
    {"--enable-stack-protection", [](Config& config, std::span<char*>&) {
        config.security.enableStackProtection = true;
        return ParseResult::Success;
    }},
    {"--enable-cfi", [](Config& config, std::span<char*>&) {
        config.security.enableControlFlowIntegrity = true;
        return ParseResult::Success;
    }},
    {"--enable-asan", [](Config& config, std::span<char*>&) {
        config.security.enableAddressSanitizer = true;
        return ParseResult::Success;
    }},
    {"--enable-msan", [](Config& config, std::span<char*>&) {
        config.security.enableMemorySanitizer = true;
        return ParseResult::Success;
    }},
    {"--enable-tsan", [](Config& config, std::span<char*>&) {
        config.security.enableThreadSanitizer = true;
        return ParseResult::Success;
    }},
    {"--enable-ubsan", [](Config& config, std::span<char*>&) {
        config.security.enableUndefinedBehaviorSanitizer = true;
        return ParseResult::Success;
    }},
    {"--enable-pic", [](Config& config, std::span<char*>&) {
        config.security.enablePositionIndependentCode = true;
        return ParseResult::Success;
    }},
    {"--enable-lto", [](Config& config, std::span<char*>&) {
        config.aot.lto.enabled = true;
        return ParseResult::Success;
    }},
    {"--enable-pgo", [](Config& config, std::span<char*>&) {
        config.optimization.pgo.enabled = true;
        return ParseResult::Success;
    }},
    {"--enable-vectorization", [](Config& config, std::span<char*>&) {
        config.optimization.enableVectorization = true;
        return ParseResult::Success;
    }},
    {"--enable-inlining", [](Config& config, std::span<char*>&) {
        config.optimization.enableFunctionInlining = true;
        return ParseResult::Success;
    }},
    {"--enable-tail-calls", [](Config& config, std::span<char*>&) {
        config.optimization.enableTailCallOptimization = true;
        return ParseResult::Success;
    }},
    {"--fast-math", [](Config& config, std::span<char*>&) {
        config.optimization.fastMath = true;
        return ParseResult::Success;
    }},
    {"--emit-staticlib", [](Config& config, std::span<char*>&) {
        config.mode = CompileMode::AOT;
        config.aot.outputFormat = OutputFormat::StaticLib;
        return ParseResult::Success;
    }},
    {"--emit-sharedlib", [](Config& config, std::span<char*>&) {
        config.mode = CompileMode::AOT;
        config.aot.outputFormat = OutputFormat::SharedLib;
        return ParseResult::Success;
    }},
    {"--emit-assembly", [](Config& config, std::span<char*>&) {
        config.mode = CompileMode::AOT;
        config.aot.outputFormat = OutputFormat::Assembly;
        return ParseResult::Success;
    }},
    {"--emit-ir", [](Config& config, std::span<char*>&) {
        config.mode = CompileMode::AOT;
        config.aot.outputFormat = OutputFormat::LLVM_IR;
        return ParseResult::Success;
    }},
    {"--emit-object", [](Config& config, std::span<char*>&) {
        config.mode = CompileMode::AOT;
        config.aot.outputFormat = OutputFormat::ObjectFile;
        return ParseResult::Success;
    }},
    {"--emit-bitcode", [](Config& config, std::span<char*>&) {
        config.mode = CompileMode::AOT;
        config.aot.outputFormat = OutputFormat::Bitcode;
        return ParseResult::Success;
    }},
    {"--emit-wasm", [](Config& config, std::span<char*>&) {
        config.mode = CompileMode::AOT;
        config.aot.outputFormat = OutputFormat::WebAssembly;
        return ParseResult::Success;
    }}
};

const std::unordered_map<std::string_view, TargetArch> Engine::ArgumentParser::archMap{
    {"x86_64", TargetArch::X86_64},
    {"arm64", TargetArch::ARM64},
    {"aarch64", TargetArch::ARM64},
    {"x86_32", TargetArch::X86_32},
    {"i386", TargetArch::X86_32},
    {"arm32", TargetArch::ARM32},
    {"arm", TargetArch::ARM32},
    {"riscv64", TargetArch::RISCV64},
    {"wasm32", TargetArch::WASM32},
    {"wasm64", TargetArch::WASM64},
    {"auto", TargetArch::Auto}
};

const std::unordered_map<std::string_view, TargetOS> Engine::ArgumentParser::osMap{
    {"linux", TargetOS::Linux},
    {"windows", TargetOS::Windows},
    {"win32", TargetOS::Windows},
    {"macos", TargetOS::MacOS},
    {"darwin", TargetOS::MacOS},
    {"freebsd", TargetOS::FreeBSD},
    {"android", TargetOS::Android},
    {"ios", TargetOS::iOS},
    {"wasm", TargetOS::WebAssembly},
    {"webassembly", TargetOS::WebAssembly},
    {"auto", TargetOS::Auto}
};

const std::unordered_map<std::string_view, GCStrategy> Engine::ArgumentParser::gcMap{
    {"none", GCStrategy::None},
    {"refcounting", GCStrategy::RefCounting},
    {"marksweep", GCStrategy::MarkSweep},
    {"generational", GCStrategy::Generational},
    {"incremental", GCStrategy::Incremental}
};

const std::unordered_map<std::string_view, SafetyLevel> Engine::ArgumentParser::safetyMap{
    {"unsafe", SafetyLevel::Unsafe},
    {"minimal", SafetyLevel::Minimal},
    {"standard", SafetyLevel::Standard},
    {"paranoid", SafetyLevel::Paranoid}
};

constexpr uint64_t Engine::ArgumentParser::hash(std::string_view str) noexcept {
    uint64_t hash = 14695981039346656037ULL;
    for (char c : str) {
        hash ^= static_cast<uint64_t>(c);
        hash *= 1099511628211ULL;
    }
    return hash;
}

std::expected<Config, std::string> 
Engine::parseArguments(int argc, char* argv[]) noexcept {
    OMNISCRIPT_PROFILE_FUNCTION();
    if (argc > MAX_ARGS) OMNISCRIPT_UNLIKELY {
        return std::unexpected("Too many arguments (" + std::to_string(argc - 1) + " given, the limit is " + std::to_string(MAX_ARGS) + ").");
    }

    Config config;
    bool fileSpecified = false;
    config.targetArch = TargetArch::Auto;
    config.targetOS = TargetOS::Auto;
    perf::ScopedTimer parseTimer(currentStats_.parseTime);

    for (int i = 1; i < argc; ++i) {
        std::string_view arg(argv[i]);
        auto handlerIt = ArgumentParser::argHandlers.find(arg);
        if (handlerIt != ArgumentParser::argHandlers.end()) {
            std::span<char*> args(argv + i + 1, argc - i - 1);
            auto result = handlerIt->second(config, args);
            if (result != ParseResult::Success) {
                currentStats_.result = result;
                const bool hasValue = i + 1 < argc;
                return std::unexpected(describeOptionError(std::string(arg), hasValue ? argv[i + 1] : "", hasValue,
                                                           result == ParseResult::MissingValue));
            }
            i += args.size() - (argc - i - 1);
        } else if (arg == "--verbose") {
            config.diagnostics.verbose = true;
        } else if (arg == "--execute") {
            config.mode = CompileMode::JIT;
        } else if (arg == "--make") {
            config.mode = CompileMode::AOT;
        } else if (arg == "--dry") {
            config.mode = CompileMode::DryCompile;
        } else if (arg == "--version") {
            printVersion();
            std::exit(0);
        } else if (arg == "--help") {
            printUsage();
            std::exit(0);
        } else if (arg == "--list-targets") {
            listTargets();
            std::exit(0);
        } else if (arg == "--show-host-info") {
            // Assuming TargetInfo::printHostInfo exists
            std::exit(0);
        } else if (arg == "--all" || arg == "--html" || arg == "--split") {
            // flags read by the --doc* options (they scan the whole command line); ignored otherwise
        } else if (arg == "--kind" || arg == "--limit") {
            ++i;   // value is read by the --doc* options
        } else if (arg == "--doc") {
            // --doc <module or file> [-o <file.md>]: print the file's /// docs as Markdown, then exit.
            // <module> may be written like an import: "std/Console" -> standard/1/Console.os
            if (i + 1 >= argc) return std::unexpected("--doc needs a module or file, e.g. --doc std/Console or --doc ./standard/1/Console.os");
            const std::string target = argv[++i];
            if (const auto problem = pathProblem("--doc", target, resolveDocTarget(target)); !problem.empty()) {
                return std::unexpected(problem);
            }
            std::string docOut;
            for (int j = 1; j + 1 < argc; ++j) {
                const std::string_view a(argv[j]);
                if (a == "-o" || a == "--output") docOut = argv[j + 1];
            }
            bool html = false;
            for (int j = 1; j < argc; ++j) {
                if (std::string_view(argv[j]) == "--html") html = true;
            }
            std::string markdown, docError;
            const bool docOk = html ? generateDocsHtml(resolveDocTarget(target), markdown, docError)
                                    : generateDocsMarkdown(resolveDocTarget(target), markdown, docError);
            if (!docOk) {
                return std::unexpected("--doc: " + docError);
            }
            if (docOut.empty()) {
                std::cout << markdown;
            } else {
                std::ofstream out(docOut, std::ios::binary);   // UTF-8, no BOM
                if (!out) return std::unexpected("Cannot write '" + docOut + "'");
                out << markdown;
            }
            std::exit(0);
        } else if (arg == "--doc-all") {
            // --doc-all <root file> [--split] [--html] [-o <file | dir>]: document a whole library, then exit.
            // Every file the root imports/includes is documented once, sorted alphabetically.
            //   --doc-all standard/1/std.os                 -> docs/std.md
            //   --doc-all standard/1/std.os --split         -> docs/std/index.md + one .md per module
            //   --doc-all standard/1/std.os --split -o out  -> out/index.md + ...
            //   add --html for web pages with a sidebar and search (docs/std.html, or docs/std/index.html with --split)
            if (i + 1 >= argc) return std::unexpected("--doc-all needs a root file, e.g. --doc-all ./standard/1/std.os");
            const std::string rootArg = argv[++i];
            const std::string rootFile = resolveDocTarget(rootArg);
            if (const auto problem = pathProblem("--doc-all", rootArg, rootFile); !problem.empty()) {
                return std::unexpected(problem);
            }
            bool split = false;
            bool html = false;
            std::string docOut;
            for (int j = 1; j < argc; ++j) {
                const std::string_view a(argv[j]);
                if (a == "--split") split = true;
                else if (a == "--html") html = true;
                else if ((a == "-o" || a == "--output") && j + 1 < argc) docOut = argv[j + 1];
            }
            if (docOut.empty()) {
                const std::string stem = std::filesystem::path(rootFile).stem().string();
                docOut = split ? "docs/" + stem : "docs/" + stem + (html ? ".html" : ".md");
            }
            std::string docError;
            if (!generateLibraryDocs(rootFile, docOut, split, html, docError)) {
                return std::unexpected("--doc-all: " + docError);
            }
            std::cout << "Wrote " << docOut << "\n";
            std::exit(0);
        } else if (arg == "--doc-search") {
            // --doc-search <root file> <query> [--kind <kind>] [--limit <n>]: find documented items, then exit.
            //   --doc-search standard/1/std.os "read file"
            //   --doc-search standard/1/std.os String.trim --kind method --limit 5
            // <kind>: module, function, method, struct, class, enum, field, constant, variable,
            //         type (struct/class/enum), member (method/field)
            if (i + 2 >= argc) return std::unexpected("--doc-search needs a root file and a query, e.g. --doc-search ./standard/1/std.os trim  (optional: --kind <kind> --limit <n>)");
            const std::string rootArg = argv[++i];
            const std::string rootFile = resolveDocTarget(rootArg);
            if (const auto problem = pathProblem("--doc-search", rootArg, rootFile); !problem.empty()) {
                return std::unexpected(problem);
            }
            const std::string query = argv[++i];
            std::string kind;
            size_t limit = 0;
            for (int j = 1; j + 1 < argc; ++j) {
                const std::string_view a(argv[j]);
                if (a == "--kind") kind = argv[j + 1];
                else if (a == "--limit") {
                    try { limit = std::stoul(argv[j + 1]); }
                    catch (...) { return std::unexpected("Invalid value for argument: --limit"); }
                }
            }
            std::string result, docError;
            if (!searchLibraryDocs(rootFile, query, kind, limit, result, docError, console.isColorEnabled(), hasFlag(argc, argv, "--all"))) {
                return std::unexpected("--doc-search: " + docError);
            }
            std::cout << result;
            std::exit(0);
        } else if (arg == "--doc-list") {
            // --doc-list <root file> [--kind <kind>] [-o <file.md>]: one Markdown file listing everything
            // that is documented (module by module, one line per item), then exit. Good for showing
            // people what a library offers.
            //   --doc-list standard/1/std.os                      -> docs/std-index.md
            //   --doc-list standard/1/std.os --kind function    -> only functions
            //   --doc-list standard/1/std.os -o -                 -> print to the console instead
            if (i + 1 >= argc) return std::unexpected("--doc-list needs a root file, e.g. --doc-list ./standard/1/std.os");
            const std::string rootArg = argv[++i];
            const std::string rootFile = resolveDocTarget(rootArg);
            if (const auto problem = pathProblem("--doc-list", rootArg, rootFile); !problem.empty()) {
                return std::unexpected(problem);
            }
            std::string kind, docOut;
            for (int j = 1; j + 1 < argc; ++j) {
                const std::string_view a(argv[j]);
                if (a == "--kind") kind = argv[j + 1];
                else if (a == "-o" || a == "--output") docOut = argv[j + 1];
            }
            if (docOut.empty()) {
                docOut = "docs/" + std::filesystem::path(rootFile).stem().string() + "-index.md";
            }
            std::string result, docError;
            if (!listLibraryDocs(rootFile, kind, result, docError, hasFlag(argc, argv, "--all"))) {
                return std::unexpected("--doc-list: " + docError);
            }
            if (docOut == "-") {
                std::cout << result;
            } else {
                const std::filesystem::path outFile(docOut);
                std::error_code ec;
                if (outFile.has_parent_path()) std::filesystem::create_directories(outFile.parent_path(), ec);
                std::ofstream out(outFile, std::ios::binary);   // UTF-8, no BOM
                if (!out) return std::unexpected("Cannot write '" + docOut + "'");
                out << result;
                std::cout << "Wrote " << docOut << "\n";
            }
            std::exit(0);
        } else if (arg == "--doc-show") {
            // --doc-show <module or file> [--kind <kind>]: list everything documented in ONE file on the
            // console (kind, signature, summary, line), then exit. <module> may be written like an import.
            //   --doc-show std/Console
            //   --doc-show ./standard/1/String.os --kind method
            if (i + 1 >= argc) return std::unexpected("--doc-show needs a module or file, e.g. --doc-show std/Console or --doc-show ./standard/1/Console.os");
            const std::string target = argv[++i];
            const std::string file = resolveDocTarget(target);
            if (const auto problem = pathProblem("--doc-show", target, file); !problem.empty()) {
                return std::unexpected(problem);
            }
            std::string kind;
            for (int j = 1; j + 1 < argc; ++j) {
                if (std::string_view(argv[j]) == "--kind") kind = argv[j + 1];
            }
            std::string result, docError;
            if (!showFileDocs(file, target, kind, hasFlag(argc, argv, "--all"), console.isColorEnabled(), result, docError)) {
                return std::unexpected("--doc-show: " + docError);
            }
            std::cout << result;
            std::exit(0);
        } else if (arg == "--entry") {
            if (i + 1 >= argc) return std::unexpected("--entry needs a function name, e.g. --entry main.");
            config.entry = argv[++i];
        } else if (arg == "--output" || arg == "-o") {
            if (i + 1 >= argc) return std::unexpected("--output needs a file path, e.g. -o out.exe.");
            config.outputPath = argv[++i];
        } else if (arg == "--keep-obj") {
            config.keepIntermediateFiles = true;
        } else if (arg == "--log-asm") {
            config.diagnostics.logAsm = true;
        } else if (arg == "--log-final-code") {
            config.diagnostics.logFinalCode = true;
        } else if (arg == "--show-metadata") {
            config.diagnostics.showMetadata = true;
        } else if (arg == "--enable-parallel-gc") {
            config.runtime.enableParallelGC = true;
        } else if (arg == "--enable-concurrent-gc") {
            config.runtime.enableConcurrentGC = true;
        } else if (arg == "-" || !arg.starts_with("-")) {
            if (!fileSpecified) OMNISCRIPT_LIKELY {
                config.filePath = std::string(arg);
                config.sourcePaths.push_back(std::string(arg));
                fileSpecified = true;
            } else {
                config.sourcePaths.push_back(std::string(arg));
                console.reportWarning("Multiple file paths provided. All will be processed.", FileSpan{});
            }
        } else {
            static const char* const otherOptions[] = {
                "--verbose", "--execute", "--make", "--dry", "--version", "--help", "--list-targets", "--show-host-info",
                "--doc", "--doc-all", "--doc-search", "--doc-list", "--doc-show", "--all", "--entry", "--output", "--keep-obj", "--log-asm",
                "--log-final-code", "--show-metadata", "--enable-parallel-gc", "--enable-concurrent-gc"};
            std::vector<std::string> known;
            for (const auto& entry : ArgumentParser::argHandlers) known.emplace_back(entry.first);
            for (const char* o : otherOptions) known.emplace_back(o);
            std::string msg = "Unknown option '" + std::string(arg) + "'.";
            const std::string suggestion = closestOption(std::string(arg), known);
            if (!suggestion.empty()) msg += " Did you mean '" + suggestion + "'?";
            msg += " Run with --help to see all options.";
            return std::unexpected(msg);
        }
    }

    if (!fileSpecified && config.sourcePaths.empty()) OMNISCRIPT_UNLIKELY {
        return std::unexpected("No source file given. Usage: omniscript [options] <file>  (use '-' to read from stdin, --help for all options).");
    }

    for (const auto& source : config.sourcePaths) {   // fail early, with a clear reason, if a source cannot be read
        if (source == "-") continue;
        if (const auto problem = pathProblem("Source file", source, source); !problem.empty()) {
            return std::unexpected(problem);
        }
    }

    if (config.mode == CompileMode::None) {
        config.mode = CompileMode::JIT;
    }

    if (config.mainSourceFile.empty() && !config.sourcePaths.empty()) {
        config.mainSourceFile = config.sourcePaths[0];
    }

    resolveTargetConfiguration(config);
    if ((config.outputPath.empty() || config.outputPath == "a.out") && config.isAOTMode()) {
        setDefaultOutputPath(config);   // sphere.os -> sphere.exe (Windows), sphere (Linux), ...
    }
    config.autoConfigureForTarget();
    std::string validationError;
    if (!config.validate(validationError)) OMNISCRIPT_UNLIKELY {
        return std::unexpected("Invalid combination of options: " + validationError);
    }
    printConfigDebugInfo(config);
    trackResourceUsage(currentStats_);
    return config;
}

std::expected<Engine::ExecutionStats, std::string>
Engine::run(const Config& config) noexcept {
    OMNISCRIPT_PROFILE_FUNCTION();
    currentStats_ = {};
    perf::ScopedTimer totalTimer(currentStats_.totalTime);

    std::vector<std::shared_ptr<Statement>> statements;
    {   // scope so parseTimer is destroyed exactly once, at the closing brace
    perf::ScopedTimer parseTimer(currentStats_.parseTime);
    for (const auto& source : config.sourcePaths.empty() ? std::vector<std::string>{config.filePath} : config.sourcePaths) {
        auto sourceResult = readSourceCode({.filePath = source, .mainSourceFile = source});
        if (!sourceResult) OMNISCRIPT_UNLIKELY {
            currentStats_.result = ParseResult::FileAccessError;
            error::globalErrorCollector.addError(error::Severity::Error, sourceResult.error(), "Source reading");
            return std::unexpected("Failed to read source: " + sourceResult.error());
        }
        Lexer lexer(*sourceResult, source);
        Parser parser(lexer);
        parser.setDebugMode(config.diagnostics.debugMode);
        auto parsed = parser.parse();

        // Expand `extern "lib.h" { }` blocks into real declarations
        // (headers are searched relative to this source file first).
        {
            extern_support::ResolverConfig resolverCfg;
            resolverCfg.debugPrint = config.diagnostics.debugMode;
            resolverCfg.debugOut   = &std::cout;
            extern_support::ExternResolver resolver(
                std::move(resolverCfg),
                fs::path(source).parent_path().string());
            resolver.expand(parsed);
        }

        statements.insert(statements.end(), parsed.begin(), parsed.end());
        // for (const auto& err : parser.getErrors()) {
        //     error::globalErrorCollector.addError(error::Severity::Error, err.message, err.context);
        // }
    }
    }   // parseTimer destroyed here

    // Gate 1: stop if lexing/parsing/extern expansion reported errors.
    // (Parser errors are reported through the console; if your Parser keeps
    // its own list, re-enable the getErrors() loop above instead.)
    if (console.getErrorCount() > 0) OMNISCRIPT_UNLIKELY {
        currentStats_.result = ParseResult::ConfigurationError;
        return std::unexpected("Parsing failed with " +
            std::to_string(console.getErrorCount()) + " error(s); nothing was compiled or executed.");
    }

    if (config.diagnostics.verbose) OMNISCRIPT_UNLIKELY {
        // parser.printParseTree();
        // parser.printTokenStream();
    }
    if (config.diagnostics.debugMode) OMNISCRIPT_LIKELY {
        printConfigDebugInfo(config);
    }

    perf::MemoryTracker memTracker;
    perf::ScopedTimer compileTimer(currentStats_.compileTime);
    if (config.isAOTMode() || config.isHybridMode()) {
        Compiler compiler;
        auto compileResult = config.parallelJobs > 1 ? 
            compiler.compileParallel(statements, config, config.parallelJobs) :
            compiler.compile(statements, config, 
                [](const std::string& phase, double progress) {
                    if (console.isDebugging()) {
                        DEBUG_LOG("Compilation progress: " + phase + " (" + 
                                std::to_string(static_cast<int>(progress)) + "%)");
                    }
                });
        if (!compileResult) OMNISCRIPT_UNLIKELY {
            currentStats_.result = ParseResult::ConfigurationError;
            error::globalErrorCollector.addError(error::Severity::Error, compileResult.error(), "Compilation");
            return std::unexpected("Compilation failed: " + compileResult.error());
        }
        if (console.getErrorCount() > 0) OMNISCRIPT_UNLIKELY {
            currentStats_.result = ParseResult::ConfigurationError;
            return std::unexpected("Compilation reported " +
                std::to_string(console.getErrorCount()) + " error(s); output is not valid.");
        }
        currentStats_.compileTime = compileResult->totalTime;
        DEBUG_LOG("Compilation progress: Done (100%)");
        if (config.mode == CompileMode::AOT) {
            console.log("Compilation done. Output emitted to: " + config.outputPath);
        } else if (config.isDryRun()) {
            console.log("Dry compilation complete. Output written to: " + config.outputPath);
        }
    } else if (config.isJITMode()) {
        if (config.diagnostics.debugMode) {
            DEBUG_LOG("JIT execution using target: " + config.getTargetSummary());
        }
        auto backend = std::make_shared<LLVMJITBackend>();
        backend->initialize();   // TODO: if initialize() returns a status, check it here
        auto jitStart = std::chrono::steady_clock::now();
        backend->execute(statements, config);
        auto jitEnd = std::chrono::steady_clock::now();
        if (console.getErrorCount() > 0) OMNISCRIPT_UNLIKELY {
            currentStats_.result = ParseResult::ConfigurationError;
            return std::unexpected("Execution reported " +
                std::to_string(console.getErrorCount()) + " error(s).");
        }
        currentStats_.compileTime = std::chrono::duration_cast<std::chrono::milliseconds>(jitEnd - jitStart);
    }

    currentStats_.memoryUsage = memTracker.getPeakUsage();
    currentStats_.result = ParseResult::Success;
    return currentStats_;
}

std::expected<size_t, std::string> Engine::parseSizeString(std::string_view sizeStr) noexcept {
    OMNISCRIPT_PROFILE_FUNCTION();
    if (sizeStr.empty()) {
        OMNISCRIPT_UNLIKELY
        return std::unexpected("Empty size string");
    }
    if (sizeStr.back() >= '0' && sizeStr.back() <= '9') {
        try {
            return std::stoull(std::string(sizeStr));
        } catch (...)  {
            OMNISCRIPT_UNLIKELY
            return std::unexpected("Invalid numeric value");
        }
    }
    auto suffixPos = sizeStr.find_first_not_of("0123456789.");
    if (suffixPos == 0) {
        OMNISCRIPT_UNLIKELY
        return std::unexpected("Invalid size string: no numeric part");
    }
    double value;
    try {
        value = std::stod(std::string(sizeStr.substr(0, suffixPos)));
    } catch (...) {
        OMNISCRIPT_UNLIKELY 
        return std::unexpected("Invalid numeric value");
    }
    if (value < 0) {
        OMNISCRIPT_UNLIKELY 
        return std::unexpected("Size cannot be negative");
    }
    size_t multiplier = 1;
    if (suffixPos != std::string_view::npos) {
        auto suffix = sizeStr.substr(suffixPos);
        std::string suffixUpper;
        suffixUpper.reserve(suffix.size());
        std::transform(suffix.begin(), suffix.end(), std::back_inserter(suffixUpper),
                      [](char c) { return std::toupper(c); });
        if (suffixUpper == "B" || suffixUpper.empty()) {
            multiplier = 1;
        } else if (suffixUpper == "KB" || suffixUpper == "K") {
            multiplier = 1024ULL;
        } else if (suffixUpper == "MB" || suffixUpper == "M") {
            multiplier = 1024ULL * 1024ULL;
        } else if (suffixUpper == "GB" || suffixUpper == "G") {
            multiplier = 1024ULL * 1024ULL * 1024ULL;
        } else if (suffixUpper == "TB" || suffixUpper == "T") {
            multiplier = 1024ULL * 1024ULL * 1024ULL * 1024ULL;
        } else {
            return std::unexpected("Unknown size suffix: " + std::string(suffix));
        }
    }
    double result = value * multiplier;
    if (result > static_cast<double>(SIZE_MAX)) {
        OMNISCRIPT_UNLIKELY
        return std::unexpected("Size value too large");
    }
    return static_cast<size_t>(result);
}

TargetArch Engine::parseTargetArch(std::string_view arch) noexcept {
    auto it = ArgumentParser::archMap.find(arch);
    if (it != ArgumentParser::archMap.end()) {
        OMNISCRIPT_LIKELY
        return it->second;
    }
    console.reportWarning("Unknown architecture '" + std::string(arch) + "', using auto-detection", FileSpan{});
    return TargetArch::Auto;
}

TargetOS Engine::parseTargetOS(std::string_view os) noexcept {
    auto it = ArgumentParser::osMap.find(os);
    if (it != ArgumentParser::osMap.end()) {
        OMNISCRIPT_LIKELY
        return it->second;
    }
    console.reportWarning("Unknown operating system '" + std::string(os) + "', using auto-detection", FileSpan{});
    return TargetOS::Auto;
}

std::pair<TargetArch, TargetOS> Engine::parseTargetTriple(std::string_view triple) noexcept {
    OMNISCRIPT_PROFILE_FUNCTION();
    auto parsed = TargetInfo::TargetTriple::parse(std::string(triple));
    TargetArch arch = TargetArch::Auto;
    auto archIt = ArgumentParser::archMap.find(parsed.arch);
    if (archIt != ArgumentParser::archMap.end()) {
        arch = archIt->second;
    }
    TargetOS os = TargetOS::Auto;
    auto osIt = ArgumentParser::osMap.find(parsed.os);
    if (osIt != ArgumentParser::osMap.end()) {
        os = osIt->second;
    }
    return {arch, os};
}

GCStrategy Engine::parseGCStrategy(std::string_view gc) noexcept {
    auto it = ArgumentParser::gcMap.find(gc);
    if (it != ArgumentParser::gcMap.end()) OMNISCRIPT_LIKELY {
        return it->second;
    }
    return GCStrategy::RefCounting;
}

SafetyLevel Engine::parseSafetyLevel(std::string_view safety) noexcept {
    auto it = ArgumentParser::safetyMap.find(safety);
    if (it != ArgumentParser::safetyMap.end()) {
        OMNISCRIPT_LIKELY
        return it->second;
    }
    return SafetyLevel::Standard;
}

std::expected<std::string, std::string> Engine::readSourceCode(const Config& config) noexcept {
    OMNISCRIPT_PROFILE_FUNCTION();
    try {
        auto filePath = config.mainSourceFile.empty() ? config.filePath : config.mainSourceFile;
        if (filePath == "-") {
            std::ostringstream buffer;
            buffer << std::cin.rdbuf();
            return buffer.str();
        } else {
            fs::path path(filePath);
            if (!fs::exists(path)) {
                OMNISCRIPT_UNLIKELY
                return std::unexpected("File not found: " + filePath);
            }
            auto fileSize = fs::file_size(path);
            if (fileSize == 0) {
                OMNISCRIPT_UNLIKELY 
                return std::string{};
            }
            if (fileSize < 1024 * 1024) {
                std::ifstream file(filePath, std::ios::binary);
                if (!file) {
                    OMNISCRIPT_UNLIKELY 
                    return std::unexpected("Cannot open file: " + filePath);
                }
                std::string content;
                content.reserve(fileSize);
                file.seekg(0, std::ios::end);
                content.resize(file.tellg());
                file.seekg(0, std::ios::beg);
                file.read(content.data(), content.size());
                return content;
            } else {
                std::ifstream file(filePath, std::ios::binary);
                if (!file) {
                    OMNISCRIPT_UNLIKELY 
                    return std::unexpected("Cannot open file: " + filePath);
                }
                std::ostringstream buffer;
                buffer << file.rdbuf();
                return buffer.str();
            }
        }
    } catch (const std::exception& e) {
        OMNISCRIPT_UNLIKELY
        return std::unexpected("I/O error: " + std::string(e.what()));
    }
}

void Engine::printUsage() noexcept {
    console.log("Usage: omniscript [options] <file>");
    console.log("Options:");
    console.log("  --main-source <file>     Specify main source file");
    console.log("  --source <file>          Add additional source file");
    console.log("  --enable-modules <path>  Enable module support");
    console.log("  --incremental            Enable incremental compilation");
    console.log("  --cache-dir <dir>        Set cache directory");
    console.log("  --parallel-jobs <n>      Set number of parallel jobs");
    console.log("  --profiler <type>        Enable profiling (gprof, perf)");
    console.log("  --error-log <file>       Log errors to file");
    console.log("  --max-errors <n>         Set maximum error count");
    console.log("  --debug, -d              Enable debug mode");
    console.log("  --dry                    Compile to object file only");
    console.log("  --entry <function>       Set entry function");
    console.log("  --doc <module|file>      Print the file's /// docs as Markdown (add -o <file> to save, --html for a web page)");
    console.log("  --doc-all <root.os>      Document a whole library from its root file (add --split, --html, -o <out>)");
    console.log("  --doc-search <root.os> <query>  Find functions, types, members (add --kind <kind>, --limit <n>, --all)");
    console.log("  --doc-show <module|file> List everything documented in one file (add --kind <kind>, --all)");
    console.log("  --doc-list <root.os>     List everything documented in one Markdown file (add --kind <kind>, --all, -o <file>)");
    console.log("  --emit-staticlib         Emit a static library");
    console.log("  --emit-sharedlib         Emit a shared library");
    console.log("  --emit-assembly          Emit assembly source");
    console.log("  --emit-ir                Emit LLVM IR");
    console.log("  --execute                Execute using JIT");
    console.log("  --keep-obj               Keep intermediate object files");
    console.log("  --log-asm                Log generated assembly code");
    console.log("  --log-final-code         Log final IR code");
    console.log("  --make                   Compile AOT");
    console.log("  --output, -o <file>      Set output file path");
    console.log("  --optimization-level, -O <n> Optimization level: 0-3");
    console.log("  --target <os>            Build for windows, linux, android, macos, ios or wasm");
    console.log("  --toolchain <dir>        Toolchain folder (for --target android: the NDK root; or set ANDROID_NDK_HOME)");
    console.log("  --target-arch <arch>     Target architecture: x86_64, arm64, etc.");
    console.log("  --target-os <os>         Target OS: linux, windows, macos, etc.");
    console.log("  --target-triple <triple> Set target triple");
    console.log("  --list-targets           List available targets");
    console.log("  --show-host-info         Show host system info");
    console.log("  --gc <strategy>          GC strategy: none, refcounting, etc.");
    console.log("  --safety <level>         Safety level: unsafe, minimal, standard, paranoid");
    console.log("  --enable-lto             Enable Link Time Optimization");
    console.log("  --enable-pgo             Enable Profile Guided Optimization");
    console.log("  --version                Display version");
    console.log("  --help                   Display this help");
    console.log("  --verbose                Show parse tree and token stream");
    console.log("  --show-metadata          Show metadata");
    console.log("Security Options:");
    console.log("  --enable-stack-protection   Enable stack protection");
    console.log("  --enable-cfi                Enable Control Flow Integrity");
    console.log("  --enable-asan               Enable AddressSanitizer");
    console.log("  --enable-msan               Enable MemorySanitizer");
    console.log("  --enable-tsan               Enable ThreadSanitizer");
    console.log("  --enable-ubsan              Enable UndefinedBehaviorSanitizer");
    console.log("  --enable-pic                Enable Position Independent Code");
    console.log("Optimization Options:");
    console.log("  --enable-vectorization      Enable vectorization");
    console.log("  --enable-inlining           Enable function inlining");
    console.log("  --enable-tail-calls         Enable tail call optimization");
    console.log("  --fast-math                 Enable fast math");
    console.log("Runtime Configuration:");
    console.log("  --heap-size <size>       Set heap size (e.g., 64MB)");
    console.log("  --stack-size <size>      Set stack size (e.g., 8MB)");
    console.log("  --enable-parallel-gc     Enable parallel GC");
    console.log("  --enable-concurrent-gc   Enable concurrent GC");
}

void Engine::listTargets() noexcept {
    console.log("Available Target Architectures:");
    console.log("  x86_64  - 64-bit x86 (Intel/AMD)");
    console.log("  arm64   - 64-bit ARM (Apple Silicon, etc.)");
    console.log("  x86_32  - 32-bit x86");
    console.log("  arm32   - 32-bit ARM");
    console.log("  riscv64 - 64-bit RISC-V");
    console.log("  wasm32  - 32-bit WebAssembly");
    console.log("  wasm64  - 64-bit WebAssembly");
    console.log("  auto    - Auto-detect (default)");
    console.log("");
    console.log("Available Target Operating Systems:");
    console.log("  linux   - Linux");
    console.log("  windows - Windows");
    console.log("  macos   - macOS");
    console.log("  freebsd - FreeBSD");
    console.log("  android - Android");
    console.log("  ios     - iOS");
    console.log("  wasm    - WebAssembly");
    console.log("  auto    - Auto-detect (default)");
}

void Engine::printVersion() noexcept {
    console.log(std::string("Omniscript version ") + std::string(VERSION));
}

void Engine::resolveTargetConfiguration(Config& config) noexcept {
    OMNISCRIPT_PROFILE_FUNCTION();
    if (config.targetArch == TargetArch::Auto) {
        config.targetArch = TargetInfo::detectHostArchitecture();
        DEBUG_LOG("Auto-detected target architecture: " + TargetInfo::getArchitectureName(config.targetArch));
    }
    if (config.targetOS == TargetOS::Auto) {
        config.targetOS = TargetInfo::detectHostOS();
        DEBUG_LOG("Auto-detected target OS: " + TargetInfo::getOSName(config.targetOS));
    }
}

void Engine::setDefaultOutputPath(Config& config) noexcept {
    OMNISCRIPT_PROFILE_FUNCTION();
    if (config.outputPath.empty() || config.outputPath == "a.out") {
        fs::path inputPath(config.mainSourceFile.empty() ? config.filePath : config.mainSourceFile);
        std::string baseName = inputPath.stem().string();
        std::string extension;
        switch (config.aot.outputFormat) {
            case OutputFormat::Executable:
                extension = config.getExecutableExtension();
                break;
            case OutputFormat::SharedLib:
                extension = config.getSharedLibExtension();
                break;
            case OutputFormat::StaticLib:
                extension = config.getStaticLibExtension();
                break;
            case OutputFormat::Assembly:
                extension = ".s";
                break;
            case OutputFormat::LLVM_IR:
                extension = ".ll";
                break;
            case OutputFormat::ObjectFile:
                extension = config.getObjectFileExtension();
                break;
            case OutputFormat::Bitcode:
                extension = ".bc";
                break;
            case OutputFormat::WebAssembly:
                extension = ".wasm";
                break;
        }
        config.outputPath = baseName + extension;
        DEBUG_LOG("Default output path set to: " + config.outputPath);
    }
}

void Engine::printConfigDebugInfo(const Config& config) noexcept {
    DEBUG_LOG("=== Configuration Debug Info ===");
    DEBUG_LOG("File: " + config.filePath);
    DEBUG_LOG("Mode: " + std::to_string(static_cast<int>(config.mode)));
    DEBUG_LOG("Entry: " + config.entry);
    DEBUG_LOG("Output Path: " + config.outputPath);
    DEBUG_LOG("Optimization Level: " + std::to_string(config.optimization.level));
    DEBUG_LOG("Debug Mode: " + std::string(config.diagnostics.debugMode ? "true" : "false"));
    DEBUG_LOG("Target Summary: " + config.getTargetSummary());
    DEBUG_LOG("Cross Compilation: " + std::string(config.isCrossCompilation() ? "yes" : "no"));
}

size_t Engine::getCurrentMemoryUsage() noexcept {
    OMNISCRIPT_PROFILE_FUNCTION();
#ifdef __linux__
    struct mallinfo mi = mallinfo();
    return mi.uordblks;
#elif _WIN32
    PROCESS_MEMORY_COUNTERS pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
        return pmc.WorkingSetSize;
    }
#elif __APPLE__
    struct mach_task_basic_info info;
    mach_msg_type_number_t count = MACH_TASK_BASIC_INFO_COUNT;
    if (task_info(mach_task_self(), MACH_TASK_BASIC_INFO, 
                  reinterpret_cast<task_info_t>(&info), &count) == KERN_SUCCESS) {
        return info.resident_size;
    }
#endif
    return 0;
}

void Engine::trackResourceUsage(ExecutionStats& stats) noexcept {
    OMNISCRIPT_PROFILE_FUNCTION();
    stats.memoryUsage = getCurrentMemoryUsage();
}

std::string Engine::formatError(ParseResult result, std::string_view context) noexcept {
    switch (result) {
        case ParseResult::Success: return "No error";
        case ParseResult::InvalidArgument: return "Invalid argument: " + std::string(context);
        case ParseResult::MissingValue: return "Missing value for argument: " + std::string(context);
        case ParseResult::InvalidValue: return "Invalid value for argument: " + std::string(context);
        case ParseResult::ConfigurationError: return "Configuration error: " + std::string(context);
        case ParseResult::FileAccessError: return "File access error: " + std::string(context);
        case ParseResult::ResourceLimitExceeded: return "Resource limit exceeded: " + std::string(context);
        default: return "Unknown error: " + std::string(context);
    }
}

void Engine::handleCriticalError(std::string_view error) noexcept {
    console.reportError(Console::RUNTIME_ERROR, std::string(error), FileSpan{});
    std::exit(1);
}

Engine::ParseResult Engine::handleDebugArgs(Config& config, std::span<char*>& args) noexcept {
    config.diagnostics.debugMode = true;
    console.enableDebug();
    return ParseResult::Success;
}

Engine::ParseResult Engine::handleTargetArgs(Config& config, std::span<char*>& args) noexcept {
    if (args.empty()) return ParseResult::MissingValue;
    if (args[0] == "--target-arch") {
        config.targetArch = parseTargetArch(args[0]);
        args = args.subspan(1);
    } else if (args[0] == "--target-os") {
        config.targetOS = parseTargetOS(args[0]);
        args = args.subspan(1);
    } else if (args[0] == "--target-triple") {
        auto [arch, os] = parseTargetTriple(args[0]);
        config.targetArch = arch;
        config.targetOS = os;
        args = args.subspan(1);
    }
    return ParseResult::Success;
}

Engine::ParseResult Engine::handleOptimizationArgs(Config& config, std::span<char*>& args) noexcept {
    if (args[0] == "--optimization-level" || args[0] == "-O") {
        if (args.size() < 2) return ParseResult::MissingValue;
        try {
            config.optimization.level = std::stoi(args[1]);
            args = args.subspan(2);
        } catch (...) {
            return ParseResult::InvalidValue;
        }
    } else if (args[0] == "--enable-lto") {
        config.aot.lto.enabled = true;
    } else if (args[0] == "--enable-pgo") {
        config.optimization.pgo.enabled = true;
    } else if (args[0] == "--enable-vectorization") {
        config.optimization.enableVectorization = true;
    } else if (args[0] == "--enable-inlining") {
        config.optimization.enableFunctionInlining = true;
    } else if (args[0] == "--enable-tail-calls") {
        config.optimization.enableTailCallOptimization = true;
    } else if (args[0] == "--fast-math") {
        config.optimization.fastMath = true;
    }
    return ParseResult::Success;
}

Engine::ParseResult Engine::handleRuntimeArgs(Config& config, std::span<char*>& args) noexcept {
    if (args.empty()) return ParseResult::MissingValue;
    if (args[0] == "--gc") {
        config.runtime.gcStrategy = parseGCStrategy(args[0]);
        args = args.subspan(1);
    } else if (args[0] == "--heap-size") {
        auto sizeResult = parseSizeString(args[0]);
        if (!sizeResult) return ParseResult::InvalidValue;
        config.runtime.heapSize = *sizeResult;
        args = args.subspan(1);
    } else if (args[0] == "--stack-size") {
        auto sizeResult = parseSizeString(args[0]);
        if (!sizeResult) return ParseResult::InvalidValue;
        config.runtime.stackSize = *sizeResult;
        args = args.subspan(1);
    }
    return ParseResult::Success;
}

Engine::ParseResult Engine::handleSecurityArgs(Config& config, std::span<char*>& args) noexcept {
    if (args[0] == "--safety") {
        if (args.size() < 2) return ParseResult::MissingValue;
        config.runtime.safetyLevel = parseSafetyLevel(args[1]);
        args = args.subspan(2);
    } else if (args[0] == "--enable-stack-protection") {
        config.security.enableStackProtection = true;
    } else if (args[0] == "--enable-cfi") {
        config.security.enableControlFlowIntegrity = true;
    } else if (args[0] == "--enable-asan") {
        config.security.enableAddressSanitizer = true;
    } else if (args[0] == "--enable-msan") {
        config.security.enableMemorySanitizer = true;
    } else if (args[0] == "--enable-tsan") {
        config.security.enableThreadSanitizer = true;
    } else if (args[0] == "--enable-ubsan") {
        config.security.enableUndefinedBehaviorSanitizer = true;
    } else if (args[0] == "--enable-pic") {
        config.security.enablePositionIndependentCode = true;
    }
    return ParseResult::Success;
}

Engine::ParseResult Engine::handleLinkingArgs(Config& config, std::span<char*>& args) noexcept {
    config.mode = CompileMode::AOT;
    if (args[0] == "--emit-staticlib") {
        config.aot.outputFormat = OutputFormat::StaticLib;
    } else if (args[0] == "--emit-sharedlib") {
        config.aot.outputFormat = OutputFormat::SharedLib;
    } else if (args[0] == "--emit-assembly") {
        config.aot.outputFormat = OutputFormat::Assembly;
    } else if (args[0] == "--emit-ir") {
        config.aot.outputFormat = OutputFormat::LLVM_IR;
    } else if (args[0] == "--emit-object") {
        config.aot.outputFormat = OutputFormat::ObjectFile;
    } else if (args[0] == "--emit-bitcode") {
        config.aot.outputFormat = OutputFormat::Bitcode;
    } else if (args[0] == "--emit-wasm") {
        config.aot.outputFormat = OutputFormat::WebAssembly;
    }
    return ParseResult::Success;
}

} // namespace Omniscript