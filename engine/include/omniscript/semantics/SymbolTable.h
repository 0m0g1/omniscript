#pragma once
#include <unordered_map>
#include <string>
#include <optional>
#include <memory>

#include <omniscript/semantics/Type.h>
#include <omniscript/semantics/Diagnostics.h>
#include <omniscript/tokens/Tokens.h>

namespace Omniscript {

enum class SymbolKind : std::uint8_t {
    Variable,
    Function,
    Struct,
    ExternFunction,
    ExternVariable,
};

struct Symbol {
    SymbolKind kind{SymbolKind::Variable};
    std::string name;
    Type type{Type::Unknown()};
    bool isMutable{true}; // for const/let rules
    Token nameTok{};      // useful for span/diagnostics
};

class SymbolTable {
public:
    explicit SymbolTable(SymbolTable* parent = nullptr)
        : m_parent(parent) {}

    SymbolTable* parent() const { return m_parent; }

    bool define(const Symbol& sym, Diagnostics& diags) {
        auto it = m_symbols.find(sym.name);
        if (it != m_symbols.end()) {
            diags.error(sym.nameTok.span(), "redefinition of symbol '" + sym.name + "'");
            return false;
        }
        m_symbols.emplace(sym.name, sym);
        return true;
    }

    Symbol* lookupLocal(const std::string& name) {
        auto it = m_symbols.find(name);
        return it == m_symbols.end() ? nullptr : &it->second;
    }

    Symbol* lookup(const std::string& name) {
        for (SymbolTable* s = this; s; s = s->m_parent) {
            if (auto* hit = s->lookupLocal(name)) return hit;
        }
        return nullptr;
    }

private:
    SymbolTable* m_parent{};
    std::unordered_map<std::string, Symbol> m_symbols;
};

struct EvalContext {
    Diagnostics diags;

    // current function return type checking
    bool inFunction{false};
    Type currentReturn{Type::Unknown()};
};

} // namespace Omniscript