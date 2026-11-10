#pragma once
// engine/include/omniscript/Statements/ExternHeaderBlock.h
//
// Produced by the parser for   extern "mylib.h" { }   (any path ending in a C/C++
// header extension followed by '{').  It carries the header paths; the
// extern_support::ExternResolver replaces the need for a body by parsing the
// header and splicing the generated declarations right after this statement.
//
// The block itself stays in the statement list (linker metadata) and is inert
// once resolved.

#include <omniscript/Statements/Statement.h>

#include <memory>
#include <string>
#include <vector>

namespace Omniscript {

class ExternHeaderBlock : public Statement {
public:
    std::vector<std::string> headerPaths;
    std::vector<std::string> libraryPaths;   // reserved: explicit linker override
    bool                     resolved = false; // set by ExternResolver::expand()

    std::shared_ptr<Statement> evaluate(SymbolTableType) override { return nullptr; }

    std::shared_ptr<Expression> express(SymbolTableType) override {
        // After expansion the block is only kept as metadata, so it is a no-op.
        if (resolved) return nullptr;
        console.reportError(
            Console::INTERNAL_ERROR,
            "ExternHeaderBlock reached express() - ExternResolver was not run",
            "To resolve this:\n"
            "1. Call extern_support::ExternResolver::expand() between parsing and code generation",
            getSpan()
        );
        return nullptr;
    }

    std::shared_ptr<Statement> clone() const override {
        return std::make_shared<ExternHeaderBlock>(*this);
    }

    bool hasSideEffects() override { return false; }

    std::string toString() const override {
        return "ExternHeaderBlock(" + (headerPaths.empty() ? std::string() : headerPaths[0]) + " ...)";
    }

    std::string formatError(const std::string& msg) const override {
        return "Error in '" + toString() + "'.\n" + msg;
    }
};

} // namespace Omniscript