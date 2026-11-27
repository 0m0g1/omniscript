// src\Semantic\Genericinstantiator.cpp
#include <omniscript/Semantic/GenericInstantiator.h>

#include <algorithm>
#include <omniscript/Core.h>
#include <omniscript/Console.h>
#include <omniscript/Statements/CallableStatement.h>
#include <omniscript/Statements/FunctionStatement.h>
#include <omniscript/Statements/ControlFlowStatements.h>
#include <omniscript/Statements/AccessStatements.h>
#include <omniscript/Statements/AssignmentAndGetterStatements.h>
#include <omniscript/Statements/ExpressionStatements.h>
#include <omniscript/Statements/SizeOfStatement.h>

namespace Omniscript {

std::string canonicalTypeName(const std::string& name);   // ModuleAndImportStatements.cpp

// ---------------------------------------------------------------------------
// Mangling
// ---------------------------------------------------------------------------
static std::string sanitizeForName(const std::string& s) {
    std::string out;
    for (char c : s) {
        bool ok = (c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
        if (ok) out += c;
        else if (!out.empty() && out.back() != '_') out += '_';
    }
    while (!out.empty() && out.back() == '_') out.pop_back();
    return out;
}

std::string mangleTypeArgs(const std::vector<std::shared_ptr<Type>>& args);

std::string mangleTypeName(const std::shared_ptr<Type>& type) {
    if (!type) return "void";
    if (auto udt = std::dynamic_pointer_cast<UserDefinedType>(type)) {
        auto dotsToUnderscores = [](const std::string& in) {
            std::string q;
            q.reserve(in.size() + 4);
            for (char c : in) { if (c == '.') q += "__"; else q += c; }
            return q;
        };
        // A generic instance used as a type argument (`List<List<Str.String>>`). Build the name
        // from the template and the MANGLED arguments, never from udt->name: that spelling carries
        // whatever the front end wrote (`Str.String` early, `String` once the import alias is
        // canonical), so the same type used to mangle two ways and produced two instances
        // (Iter.List_L__List__Str__String and Iter.List_L__List__String).
        if (!udt->typeArgs.empty() && !udt->templateName.empty()) {
            return sanitizeForName(dotsToUnderscores(udt->templateName) + "__" + mangleTypeArgs(udt->typeArgs));
        }
        if (!udt->typeArgs.empty()) {
            const std::string suffix = "_" + mangleTypeArgs(udt->typeArgs);
            if (udt->name.size() > suffix.size() &&
                udt->name.compare(udt->name.size() - suffix.size(), suffix.size(), suffix) == 0) {
                const std::string tmplName = udt->name.substr(0, udt->name.size() - suffix.size());
                return sanitizeForName(dotsToUnderscores(tmplName) + "__" + suffix.substr(1));
            }
        }
        // A qualified type argument is spelled with "__" for each '.' (Str.String -> Str__String),
        // after resolving an import alias to the canonical type name (Str.String -> String).
        return sanitizeForName(dotsToUnderscores(canonicalTypeName(udt->name)));
    }
    // Match mangleTypeTokens: use the source spelling, not toString().
    if (auto prim = std::dynamic_pointer_cast<PrimitiveType>(type)) {
        switch (prim->getKind()) {
            case Kind::Int8:   return "i8";
            case Kind::Int16:  return "i16";
            case Kind::Int32:  return "i32";
            case Kind::Int64:  return "i64";
            case Kind::UInt8:  return "u8";
            case Kind::UInt16: return "u16";
            case Kind::UInt32: return "u32";
            case Kind::UInt64: return "u64";
            case Kind::Half:   return "f16";
            case Kind::Float:  return "f32";
            case Kind::Double: return "f64";
            case Kind::Bool:   return "bool";
            case Kind::Char:   return "char";
            case Kind::Void:   return "void";
            default: break;
        }
    }
    if (auto ptr = std::dynamic_pointer_cast<PointerType>(type)) {
        return mangleTypeName(ptr->pointeeType) + "_ptr";
    }
    if (auto ref = std::dynamic_pointer_cast<ReferenceType>(type)) {
        return mangleTypeName(ref->referentType) + "_ref";
    }
    if (auto nullable = std::dynamic_pointer_cast<NullableType>(type)) {
        return mangleTypeName(nullable->innerType) + "_opt";
    }
    if (auto fixedArr = std::dynamic_pointer_cast<FixedArrayType>(type)) {
        return mangleTypeName(fixedArr->elementType) + "_arr" + std::to_string(fixedArr->size);
    }
    if (auto dynArr = std::dynamic_pointer_cast<DynamicArrayType>(type)) {
        return mangleTypeName(dynArr->elementType) + "_arr";
    }
    return sanitizeForName(type->toString());
}

std::string mangleTypeArgs(const std::vector<std::shared_ptr<Type>>& args) {
    std::string out;
    for (const auto& a : args) {
        if (!out.empty()) out += "_";
        out += mangleTypeName(a);
    }
    return out;
}

// ---------------------------------------------------------------------------
// Substitution (moved from ParseFunctions.cpp, extended for generic instances)
// ---------------------------------------------------------------------------
std::shared_ptr<Type> substituteTypeParam(
    const std::shared_ptr<Type>& type,
    const std::string& name,
    const std::shared_ptr<Type>& concrete
) {
    if (!type || !concrete) return type;

    if (auto udt = std::dynamic_pointer_cast<UserDefinedType>(type)) {
        if (udt->name == name) return concrete;
        if (!udt->typeArgs.empty()) {
            bool changed = false;
            std::vector<std::shared_ptr<Type>> newArgs;
            for (const auto& arg : udt->typeArgs) {
                auto sub = substituteTypeParam(arg, name, concrete);
                if (sub != arg) changed = true;
                newArgs.push_back(sub);
            }
            if (changed) {
                auto copy = std::dynamic_pointer_cast<UserDefinedType>(udt->clone());
                copy->typeArgs = newArgs;
                copy->name = udt->templateName + "_" + mangleTypeArgs(newArgs);
                return copy;
            }
        }
        return type;
    }
    if (auto ref = std::dynamic_pointer_cast<ReferenceType>(type)) {
        auto inner = substituteTypeParam(ref->referentType, name, concrete);
        return inner == ref->referentType ? type : Type::createReferenceType(inner);
    }
    if (auto ptr = std::dynamic_pointer_cast<PointerType>(type)) {
        auto inner = substituteTypeParam(ptr->pointeeType, name, concrete);
        return inner == ptr->pointeeType ? type : Type::createPointerType(inner, ptr->isConst, ptr->isVolatile);
    }
    if (auto nullable = std::dynamic_pointer_cast<NullableType>(type)) {
        auto inner = substituteTypeParam(nullable->innerType, name, concrete);
        return inner == nullable->innerType ? type : Type::createNullableType(inner);
    }
    if (auto fixedArr = std::dynamic_pointer_cast<FixedArrayType>(type)) {
        auto inner = substituteTypeParam(fixedArr->elementType, name, concrete);
        return inner == fixedArr->elementType ? type : Type::createFixedArrayType(inner, fixedArr->size);
    }
    if (auto dynArr = std::dynamic_pointer_cast<DynamicArrayType>(type)) {
        auto inner = substituteTypeParam(dynArr->elementType, name, concrete);
        return inner == dynArr->elementType ? type : Type::createDynamicArrayType(inner);
    }
    if (auto smart = std::dynamic_pointer_cast<SmartPointerType>(type)) {
        auto inner = substituteTypeParam(smart->pointeeType, name, concrete);
        return inner == smart->pointeeType ? type : Type::createSmartPointerType(smart->getKind(), inner);
    }
    if (auto fn = std::dynamic_pointer_cast<FunctionType>(type)) {
        // fn(T*, T*) => int  ->  fn(i32*, i32*) => int. Copy keeps the name/other fields as they were.
        bool changed = false;
        std::vector<std::shared_ptr<Type>> newParams;
        newParams.reserve(fn->parameterTypes.size());
        for (const auto& p : fn->parameterTypes) {
            auto sub = substituteTypeParam(p, name, concrete);
            if (sub != p) changed = true;
            newParams.push_back(sub);
        }
        auto newRet = substituteTypeParam(fn->returnType, name, concrete);
        if (newRet != fn->returnType) changed = true;
        if (!changed) return type;
        auto copy = std::make_shared<FunctionType>(*fn);
        copy->parameterTypes = newParams;
        copy->returnType = newRet;
        return copy;
    }
    if (auto g = std::dynamic_pointer_cast<GenericType>(type)) {
        if (g->name == name) return concrete;
        return type;
    }
    return type;
}


// ---------------------------------------------------------------------------
// Body substitution
// ---------------------------------------------------------------------------
std::string GenericInstantiator::applyNameSubstitution(const std::string& name,
                                                       const std::vector<GenericBinding>& bindings) {
    std::string out;
    out.reserve(name.size() + 8);
    size_t i = 0;
    while (i < name.size()) {
        bool replaced = false;
        if (name[i] == '_') {
            for (const auto& b : bindings) {
                if (b.name.empty()) continue;
                if (name.compare(i + 1, b.name.size(), b.name) != 0) continue;
                const size_t after = i + 1 + b.name.size();
                const bool boundary = after == name.size() || name[after] == '_' ||
                                      name[after] == '.' || name[after] == '(';
                if (!boundary) continue;
                const std::string spelled = b.mangled.empty() ? mangleTypeName(b.type) : b.mangled;
                out += "_" + spelled;
                i = after;
                replaced = true;
                break;
            }
        }
        if (!replaced) out += name[i++];
    }
    return out;
}

void GenericInstantiator::substituteInStatement(const std::shared_ptr<Statement>& stmt,
                                                const std::vector<GenericBinding>& bindings) {
    if (!stmt || bindings.empty()) return;

    auto applyType = [&](std::shared_ptr<Type> t) -> std::shared_ptr<Type> {
        if (!t) return t;
        for (const auto& b : bindings) t = substituteTypeParam(t, b.name, b.type);
        return t;
    };
    auto visit = [&](const std::shared_ptr<Statement>& s) { substituteInStatement(s, bindings); };

    // 1. Declared type / root type on every typed node.
    if (auto typed = std::dynamic_pointer_cast<TypedStatement>(stmt)) {
        if (auto t = typed->getType())      typed->setType(applyType(t));
        if (auto rt = typed->getRootType()) typed->replaceRootType(applyType(rt));
    }

    // `sizeof(T)` keeps its type in its own field (the TypedStatement type is overwritten by the
    // declared type of the target), so substitute that field explicitly.
    if (auto so = std::dynamic_pointer_cast<SizeOfStatement>(stmt)) {
        so->measuredType = applyType(so->measuredType);
        return;
    }

    // 2. Children.
    if (auto block = std::dynamic_pointer_cast<BlockStatement>(stmt)) {
        for (auto& s : block->statements) visit(s);
        return;
    }
    if (auto ifStmt = std::dynamic_pointer_cast<IfStatement>(stmt)) {
        for (auto& c : ifStmt->conditions) visit(c);
        for (auto& b : ifStmt->bodies)     visit(b);
        visit(ifStmt->elseBody);
        return;
    }
    if (auto w = std::dynamic_pointer_cast<WhileStatement>(stmt)) {
        visit(w->condition);
        visit(w->body);
        return;
    }
    if (auto f = std::dynamic_pointer_cast<ForLoop>(stmt)) {
        visit(f->initialization);
        visit(f->condition);
        visit(f->increment);
        visit(f->body);
        return;
    }
    if (auto r = std::dynamic_pointer_cast<ReturnStatement>(stmt)) {
        visit(r->returnValue);
        return;
    }
    if (auto c = std::dynamic_pointer_cast<Call>(stmt)) {
        c->callee = applyNameSubstitution(c->callee, bindings);
        c->setName(c->callee);
        visit(c->expr);
        for (auto& a : c->args) visit(a);
        return;
    }
    if (auto ca = std::dynamic_pointer_cast<ASTCast>(stmt)) {
        ca->targetType = applyType(ca->targetType);
        visit(ca->value);
        return;
    }
    if (auto ocs = std::dynamic_pointer_cast<ObjectConstructorStatement>(stmt)) {
        ocs->objectType = applyNameSubstitution(ocs->objectType, bindings);
        visit(ocs->expr);
        for (auto& a : ocs->constructorArgs) visit(a);
        return;
    }
    if (auto ps = std::dynamic_pointer_cast<ParameterStatement>(stmt)) {
        visit(ps->defaultValue);
        return;
    }
    if (auto as = std::dynamic_pointer_cast<ArgumentStatement>(stmt)) {
        visit(as->value);
        return;
    }

    if (auto asg = std::dynamic_pointer_cast<Assignment>(stmt)) {
        visit(asg->value);
        return;
    }
    if (auto b = std::dynamic_pointer_cast<ASTBinaryExpression>(stmt)) {
        visit(b->left);
        visit(b->right);
        return;
    }
    if (auto u = std::dynamic_pointer_cast<ASTUnaryExpression>(stmt)) {
        visit(u->getOperand());
        return;
    }
    if (auto t = std::dynamic_pointer_cast<ASTTernaryExpression>(stmt)) {
        visit(t->condition);
        visit(t->truthy);
        visit(t->falsey);
        return;
    }
    // `&a[i]` is a ReferenceTo with a referent expression.
    if (auto rt = std::dynamic_pointer_cast<ReferenceTo>(stmt)) {
        visit(rt->referent);
        return;
    }
    // Access family. Base expressions can alias (expr == object / pointer), so never visit one node twice.
    if (auto acc = std::dynamic_pointer_cast<Access>(stmt)) {
        visit(acc->expr);
        if (auto ma = std::dynamic_pointer_cast<MemberAccess>(stmt)) {
            if (ma->getObject() != acc->expr) visit(ma->getObject());
        }
        if (auto ia = std::dynamic_pointer_cast<IndexAccess>(stmt)) {
            visit(ia->getIndex());
        }
        for (const auto& a : acc->getArguments()) visit(a);
        visit(acc->getAssignmentValue());
        return;
    }

    // Still not recursed: nested FunctionExpression bodies (header not reviewed yet).
}

// ---------------------------------------------------------------------------
// Registry
// ---------------------------------------------------------------------------
std::map<std::string, GenericInstantiator::Template>& GenericInstantiator::templates() {
    static std::map<std::string, Template> t;
    return t;
}
std::map<std::string, bool>& GenericInstantiator::instances() {
    static std::map<std::string, bool> i;
    return i;
}
std::vector<std::shared_ptr<Expression>>& GenericInstantiator::pending() {
    static std::vector<std::shared_ptr<Expression>> p;
    return p;
}

// ---- out-of-struct operators on generic structs: `DynArray<T> can + (...) => R { ... }` ----
// The parser wraps such an operator in a ConstructStructPrototype of the same name and type parameters whose
// body holds only methods named "<name>.operator...". registerTemplate recognises that shape and APPENDS the
// methods to the template of the real struct instead of replacing it. If the operator is expressed before the
// struct (should not happen, but is harmless) it is parked and merged when the struct registers.
namespace {
std::map<std::string, std::vector<std::shared_ptr<Statement>>>& parkedOperators() {
    static std::map<std::string, std::vector<std::shared_ptr<Statement>>> p;
    return p;
}

bool isOperatorAddition(const std::string& name, const std::vector<std::shared_ptr<Statement>>& body) {
    if (body.empty()) return false;
    const std::string prefix = name + ".operator";
    for (const auto& m : body) {
        auto fn = std::dynamic_pointer_cast<FunctionDeclaration>(m);
        if (!fn || fn->getName().rfind(prefix, 0) != 0) return false;
    }
    return true;
}

std::string paramSignature(const std::shared_ptr<FunctionDeclaration>& fn) {
    std::string sig = fn->getName() + "(";
    for (const auto& p : fn->parameters) {
        auto ps = std::dynamic_pointer_cast<ParameterStatement>(p);
        if (ps && ps->getName() == "this") continue;
        sig += (ps && ps->getType()) ? ps->getType()->toString() : std::string("?");
        sig += ",";
    }
    return sig + ")";
}

// Appends `extra` to `into`, skipping a method that is already there (same name and parameter types) and
// renaming "<fromName>.x" to "<templateName>.x" so ensureInstance's prefix rename finds it (module-qualified templates).
void appendOperatorMembers(std::vector<std::shared_ptr<Statement>>& into,
                           const std::vector<std::shared_ptr<Statement>>& extra,
                           const std::string& fromName, const std::string& templateName) {
    for (const auto& m : extra) {
        auto fn = std::dynamic_pointer_cast<FunctionDeclaration>(m);
        if (!fn) continue;
        if (fromName != templateName && fn->getName().rfind(fromName + ".", 0) == 0) {
            fn->setName(templateName + "." + fn->getName().substr(fromName.size() + 1));
        }
        const std::string sig = paramSignature(fn);
        bool dup = false;
        for (const auto& e : into) {
            auto ef = std::dynamic_pointer_cast<FunctionDeclaration>(e);
            if (ef && paramSignature(ef) == sig) { dup = true; break; }
        }
        if (!dup) into.push_back(fn);
    }
}

void storeTemplateBothKeys(std::map<std::string, GenericInstantiator::Template>& all, const GenericInstantiator::Template& t) {
    all[t.name] = t;
    const auto dot = t.name.rfind('.');
    if (dot != std::string::npos) all[t.name.substr(dot + 1)] = t;
}
} // namespace

void GenericInstantiator::registerTemplate(const std::string& name,
                                           const std::vector<std::shared_ptr<Statement>>& body,
                                           const GenericParamList& typeParams,
                                           SymbolTableType scope) {
    auto& all = templates();

    if (isOperatorAddition(name, body)) {
        auto it = all.find(name);
        if (it == all.end()) {
            auto& parked = parkedOperators()[name];
            parked.insert(parked.end(), body.begin(), body.end());
            DEBUG_LOG("[GenericInstantiator] Parked operators for not yet declared template " + name);
            return;
        }
        Template t = it->second;

        // Same type parameter names, in the same order: the method body is written with them.
        bool sameParams = t.typeParams.size() == typeParams.size();
        for (size_t i = 0; sameParams && i < typeParams.size(); ++i) {
            sameParams = t.typeParams[i].first == typeParams[i].first;
        }
        if (!sameParams) {
            console.reportError(
                Console::SEMANTIC_ERROR,
                "An operator added to generic '" + name + "' must use the same type parameter names as the struct declaration",
                body.front()->getSpan());
            return;
        }
        // "<name>_T" is the placeholder instance made for the template's own signatures, not a real use.
        std::string placeholder = t.name + "_";
        for (size_t i = 0; i < t.typeParams.size(); ++i) {
            if (i) placeholder += "__";
            placeholder += t.typeParams[i].first;
        }
        for (const auto& kv : instances()) {
            if (kv.first == placeholder) continue;
            if (kv.first.rfind(t.name + "_", 0) == 0) {
                console.reportError(
                    Console::SEMANTIC_ERROR,
                    "Operator added to generic '" + name + "' after '" + kv.first +
                    "' was already instantiated; declare it before the first use of the generic type",
                    body.front()->getSpan());
                return;
            }
        }
        appendOperatorMembers(t.body, body, name, t.name);
        storeTemplateBothKeys(all, t);
        DEBUG_LOG("[GenericInstantiator] Added operators to template " + t.name);
        return;
    }

    Template t{name, body, typeParams, scope};
    auto parked = parkedOperators().find(name);
    if (parked != parkedOperators().end()) {
        appendOperatorMembers(t.body, parked->second, name, name);
        parkedOperators().erase(parked);
    }
    storeTemplateBothKeys(all, t);
    DEBUG_LOG("[GenericInstantiator] Registered template " + name);
}

bool GenericInstantiator::hasTemplate(const std::string& name) {
    if (templates().count(name) > 0) return true;
    // Same fallback ensureInstance uses: a module alias (L.List) that was never registered as a
    // template name still resolves through the short name registerTemplate always records.
    const auto dot = name.rfind('.');
    return dot != std::string::npos && templates().count(name.substr(dot + 1)) > 0;
}

std::vector<std::shared_ptr<Expression>> GenericInstantiator::drainPending() {
    std::vector<std::shared_ptr<Expression>> out;
    out.swap(pending());
    return out;
}

// ---------------------------------------------------------------------------
// Instantiation
// ---------------------------------------------------------------------------
// The front end spells an instance "<templateName>_<args>" with its own mangler, which can differ
// from the name built below when a type argument is dotted (Str.String -> "Str__String" vs
// "Str_String"). Expose the instance's type and methods under the spelling that was asked for, so
// both resolve to the SAME type / FunctionExpressions (nothing is copied).
static std::map<std::string, std::string>& instanceAliases() {
    static std::map<std::string, std::string> m;
    return m;
}

// Back-end lookup: the instance a front-end spelling (e.g. "L.List_Str__String") stands for,
// or "" when `name` is not a recorded alias.
std::string instanceAliasTarget(const std::string& name) {
    auto it = instanceAliases().find(name);
    return it == instanceAliases().end() ? std::string() : it->second;
}

static void aliasInstanceName(SymbolTableType scope, const std::string& instName, const std::string& alias) {
    if (alias.empty() || alias == instName) return;
    {
        auto prev = instanceAliases().find(alias);
        if (prev != instanceAliases().end() && prev->second != instName) {
            DEBUG_LOG("[ensureInstance] alias '" + alias + "' already -> '" + prev->second +
                      "'; keeping it over '" + instName + "'");
            return;
        }
    }
    instanceAliases()[alias] = instName;
    if (!scope) return;

    if (auto ty = scope->getType(instName)) {
        if (!scope->getType(alias)) {
            scope->addType(alias, ty, true);
            DEBUG_LOG("[ensureInstance] aliased type '" + alias + "' -> '" + instName + "'");
        }
    }

    const std::string oldPrefix = instName + ".";
    for (const auto& k : scope->getAllSymbolNames(false)) {
        if (k.rfind(oldPrefix, 0) != 0) continue;
        const std::string nn = alias + "." + k.substr(oldPrefix.size());
        if (!scope->getOverloads(nn).empty()) continue;   // already aliased
        auto found = scope->lookup(k);
        if (found.found() && found.isOverloadSet()) {
            for (const auto& o : found.overloads) scope->addOverloadable(nn, o, true);
            DEBUG_LOG("[ensureInstance] aliased method '" + nn + "' -> '" + k + "'");
        }
    }
}

// substituteTypeParam returns the SAME Type object for a bare `T`. ConstructStructPrototype::express writes the field
// name into the field's type (`type->parameterName = fieldName`), so `struct Pair<T> { a: T; b: T; }` ended up with two
// members both called 'b'. Give every field its own copy of such a shared type.
static std::shared_ptr<Type> copyTypeForField(const std::shared_ptr<Type>& t) {
    if (auto p = std::dynamic_pointer_cast<PrimitiveType>(t))   return std::make_shared<PrimitiveType>(*p);
    if (auto u = std::dynamic_pointer_cast<UserDefinedType>(t)) return std::make_shared<UserDefinedType>(*u);
    if (auto p = std::dynamic_pointer_cast<PointerType>(t))     return std::make_shared<PointerType>(*p);
    if (auto n = std::dynamic_pointer_cast<NullableType>(t))    return std::make_shared<NullableType>(*n);
    return t;
}

bool GenericInstantiator::ensureInstance(const std::shared_ptr<UserDefinedType>& udt) {
    if (!udt || udt->templateName.empty() || udt->typeArgs.empty()) return false;

    // Instantiate argument types first (List<List<i64>> needs List<i64>).
    for (const auto& arg : udt->typeArgs) ensureInstancesIn(arg);

    auto it = templates().find(udt->templateName);
    if (it == templates().end()) {
        // A module alias (L.List) that was never registered as a template name: fall back to the
        // short name, which registerTemplate always records.
        const auto dot = udt->templateName.rfind('.');
        if (dot != std::string::npos) it = templates().find(udt->templateName.substr(dot + 1));
    }
    if (it == templates().end()) {
        DEBUG_LOG("[ensureInstance] no template for '" + udt->templateName + "' (type '" + udt->name + "')");
        return false;
    }
    const Template& tmpl = it->second;

    if (tmpl.typeParams.size() != udt->typeArgs.size()) {
        console.reportError(
            Console::SEMANTIC_ERROR,
            "Generic '" + udt->templateName + "' expects " + std::to_string(tmpl.typeParams.size()) +
            " type argument(s) but got " + std::to_string(udt->typeArgs.size()),
            FileSpan{});
        return false;
    }

    // Instance name keeps the template's module prefix: Core.List + i64 -> Core.List_i64.
    const std::string suffix = "_" + mangleTypeArgs(udt->typeArgs);
    const std::string instName = tmpl.name + suffix;

    DEBUG_LOG("[ensureInstance] request type='" + udt->name + "' template='" + udt->templateName +
              "' -> instance='" + instName + "'" + (udt->name != instName ? " (NAME DIFFERS)" : ""));

    if (instances().count(instName)) {
        aliasInstanceName(tmpl.scope, instName, udt->name);
        return true;
    }
    instances()[instName] = true;               // mark first: guards self-referential templates

    DEBUG_LOG("[ensureInstance] Marked '" + instName + "' as in-progress; template='" +
          tmpl.name + "', scope='" + tmpl.scope->getName() + "'");

    // The mangled name resolveType gave the UDT is "<templateName>_<args>"; make sure both
    // spellings resolve to the instance type (bare alias is added by struct express for dotted names).
    std::vector<std::shared_ptr<Statement>> newBody;
    const std::string oldPrefix = tmpl.name + ".";

    auto substituteAll = [&](std::shared_ptr<Type> t) {
        for (size_t i = 0; i < tmpl.typeParams.size(); ++i) {
            t = substituteTypeParam(t, tmpl.typeParams[i].first, udt->typeArgs[i]);
        }
        return t;
    };

    std::vector<GenericBinding> bindings;
    for (size_t i = 0; i < tmpl.typeParams.size(); ++i) {
        bindings.push_back({tmpl.typeParams[i].first, udt->typeArgs[i], mangleTypeName(udt->typeArgs[i])});
    }

    for (const auto& member : tmpl.body) {
        if (!member) continue;

        if (auto field = std::dynamic_pointer_cast<ParameterStatement>(member)) {
            auto copy = std::make_shared<ParameterStatement>(*field);
            auto fieldType = substituteAll(field->getType());
            for (const auto& arg : udt->typeArgs) {
                if (fieldType == arg) { fieldType = copyTypeForField(fieldType); break; }   // bare `T`: shared object
            }
            copy->setType(fieldType);
            substituteInStatement(copy->defaultValue, bindings);   // e.g. `arr: List<T> = List<T>{}`
            newBody.push_back(copy);
            continue;
        }

        if (auto method = std::dynamic_pointer_cast<FunctionDeclaration>(member)) {
            auto fn = std::dynamic_pointer_cast<FunctionDeclaration>(method->clone());
            if (!fn) continue;

            std::string mName = method->getName();
            if (mName.rfind(oldPrefix, 0) == 0) {
                fn->setName(instName + "." + mName.substr(oldPrefix.size()));
            }

            // FunctionDeclaration::clone() drops these flags.
            fn->isStatic = method->isStatic;
            fn->isExtern = method->isExtern;
            fn->externName = method->externName;
            fn->isIntrinsic = method->isIntrinsic;
            fn->intrinsicName = method->intrinsicName;

            for (auto& p : fn->parameters) {
                if (auto ps = std::dynamic_pointer_cast<ParameterStatement>(p)) {
                    ps->setType(substituteAll(ps->getType()));
                }
            }
            fn->returnType = substituteAll(method->returnType);
            fn->setType(fn->returnType);

            // Bodies resolve T / sizeof(T) / `as T*` through the same generic binding that
            // parse-time generic functions use.
            for (size_t i = 0; i < tmpl.typeParams.size(); ++i) {
                fn->addGenericParam(tmpl.typeParams[i].first);
                fn->bindGeneric(tmpl.typeParams[i].first, udt->typeArgs[i]);
            }
            // Signature substitution above only covers param/return types; rewrite the body too
            // (`let p: T*`, `sort<T>(...)` -> `sort_i64(...)`, casts, constructors, nested blocks).
            for (auto& p : fn->parameters) substituteInStatement(p, bindings);
            substituteInStatement(fn->body, bindings);
            fn->setSpan(method->getSpan());
            newBody.push_back(fn);
            continue;
        }

        newBody.push_back(member->clone());
    }

    auto proto = std::make_shared<ConstructStructPrototype>(instName, newBody);   // no typeParams: concrete
    proto->setSpan(tmpl.body.empty() ? FileSpan{} : tmpl.body.front()->getSpan());

    auto expr = proto->express(tmpl.scope);
    if (!expr) {
        console.reportError(
            Console::SEMANTIC_ERROR,
            "Failed to instantiate generic '" + tmpl.name + "' as '" + instName + "'",
            FileSpan{});
        return false;
    }
    pending().push_back(expr);
    aliasInstanceName(tmpl.scope, instName, udt->name);
    DEBUG_LOG("[GenericInstantiator] Instantiated " + instName);
    DEBUG_LOG("[ensureInstance] Successfully registered '" + instName +
          "' in scope '" + tmpl.scope->getName() + "'");
    return true;
}

void GenericInstantiator::ensureInstancesIn(const std::shared_ptr<Type>& type) {
    if (!type) return;
    if (auto udt = std::dynamic_pointer_cast<UserDefinedType>(type)) {
        ensureInstance(udt);
        return;
    }
    if (auto ptr = std::dynamic_pointer_cast<PointerType>(type)) { ensureInstancesIn(ptr->pointeeType); return; }
    if (auto ref = std::dynamic_pointer_cast<ReferenceType>(type)) { ensureInstancesIn(ref->referentType); return; }
    if (auto n = std::dynamic_pointer_cast<NullableType>(type)) { ensureInstancesIn(n->innerType); return; }
    if (auto a = std::dynamic_pointer_cast<FixedArrayType>(type)) { ensureInstancesIn(a->elementType); return; }
    if (auto d = std::dynamic_pointer_cast<DynamicArrayType>(type)) { ensureInstancesIn(d->elementType); return; }
    if (auto s = std::dynamic_pointer_cast<SmartPointerType>(type)) { ensureInstancesIn(s->pointeeType); return; }
}


// ---------------------------------------------------------------------------
// Lazy function templates
// ---------------------------------------------------------------------------
namespace {
struct FunctionTemplate {
    std::string name;                                   // as declared (may be module-qualified)
    std::shared_ptr<FunctionDeclaration> decl;
    std::vector<std::string> paramNames;                // "T", "U", ...
    SymbolTableType scope;
};
std::map<std::string, FunctionTemplate>& functionTemplates() {   // keyed by bare name
    static std::map<std::string, FunctionTemplate> t;
    return t;
}
std::map<std::string, bool>& functionInstances() {
    static std::map<std::string, bool> i;
    return i;
}
std::string bareName(const std::string& n) {
    const auto dot = n.rfind('.');
    return dot == std::string::npos ? n : n.substr(dot + 1);
}
std::vector<std::string> splitOn(const std::string& s, const std::string& sep) {
    std::vector<std::string> out;
    size_t pos = 0;
    while (true) {
        size_t nx = s.find(sep, pos);
        if (nx == std::string::npos) { out.push_back(s.substr(pos)); break; }
        out.push_back(s.substr(pos, nx - pos));
        pos = nx + sep.size();
    }
    return out;
}
// Inverse of Parser::generateSpecializedNameForDecleration for one parameter: "i32_ptr" -> {"i32","*"}.
// Arrays and dotted names are not decodable from this spelling and are rejected.
bool decodeSpelling(const std::string& piece, SymbolTableType scope, std::vector<std::string>& tokens) {
    if (piece.empty()) return false;
    for (const auto& part : splitOn(piece, "_")) {
        if (part == "ptr") tokens.push_back("*");
        else if (part == "ref") tokens.push_back("&");
        else if (part.empty() || part == "arr" || std::isdigit(static_cast<unsigned char>(part[0]))) return false;
        else tokens.push_back(part);
    }
    // The first non-modifier token is the base type: it must be a known type, otherwise this is
    // not a type spelling at all (e.g. the "by_i64" of a function named `sort_by_i64`).
    for (const auto& t : tokens) {
        if (t == "*" || t == "&") continue;
        auto ty = resolveType({t});
        if (!ty || ty->isInvalid() || ty->isUnresolved()) return false;
        if (std::dynamic_pointer_cast<UserDefinedType>(ty) && !(scope && scope->getType(t))) return false;
        break;
    }
    return true;
}
} // namespace

void GenericInstantiator::registerFunctionTemplate(const std::string& name,
                                                   const std::shared_ptr<FunctionDeclaration>& decl,
                                                   SymbolTableType scope) {
    FunctionTemplate t;
    t.name = name;
    t.decl = decl;
    for (const auto& tp : decl->typeParams) t.paramNames.push_back(tp.first);
    t.scope = scope;
    functionTemplates()[bareName(name)] = t;
    DEBUG_LOG("[GenericInstantiator] Registered function template " + name);
}

bool GenericInstantiator::hasFunctionTemplates() { return !functionTemplates().empty(); }

bool GenericInstantiator::ensureFunctionInstanceByCallee(const std::string& callee) {
    const std::string bare = bareName(callee);

    // Longest template name that prefixes the callee (`sort_by` before `sort`).
    const FunctionTemplate* best = nullptr;
    for (const auto& [key, tmpl] : functionTemplates()) {
        if (bare.size() > key.size() + 1 && bare.compare(0, key.size(), key) == 0 && bare[key.size()] == '_') {
            if (!best || key.size() > bareName(best->name).size()) best = &tmpl;
        }
    }
    if (!best) return false;
    const FunctionTemplate& tmpl = *best;

    const std::string tail = bare.substr(bareName(tmpl.name).size() + 1);
    const auto pieces = splitOn(tail, "__");
    if (pieces.size() != tmpl.paramNames.size()) return false;

    std::vector<std::shared_ptr<Type>> typeArgs;
    for (const auto& piece : pieces) {
        std::vector<std::string> tokens;
        if (!decodeSpelling(piece, tmpl.scope, tokens)) return false;
        auto ty = resolveType(tokens);
        if (!ty || ty->isInvalid()) return false;
        typeArgs.push_back(ty);
    }

    const std::string instName = tmpl.name + "_" + tail;
    if (functionInstances().count(instName)) return true;
    functionInstances()[instName] = true;          // mark first: guards recursive generics

    std::vector<GenericBinding> bindings;
    for (size_t i = 0; i < tmpl.paramNames.size(); ++i) {
        bindings.push_back({tmpl.paramNames[i], typeArgs[i], pieces[i]});   // same spelling the callee used
    }
    auto substituteAll = [&](std::shared_ptr<Type> t) {
        for (const auto& b : bindings) t = substituteTypeParam(t, b.name, b.type);
        return t;
    };

    auto fn = std::dynamic_pointer_cast<FunctionDeclaration>(tmpl.decl->clone());
    if (!fn) return false;
    fn->setName(instName);
    fn->isStatic = tmpl.decl->isStatic;
    fn->isExtern = tmpl.decl->isExtern;
    fn->externName = tmpl.decl->externName;
    fn->isIntrinsic = tmpl.decl->isIntrinsic;
    fn->intrinsicName = tmpl.decl->intrinsicName;
    fn->isGenericTemplate = false;

    for (auto& p : fn->parameters) {
        if (auto ps = std::dynamic_pointer_cast<ParameterStatement>(p)) {
            ps->setType(substituteAll(ps->getType()));
        }
    }
    fn->returnType = substituteAll(tmpl.decl->returnType);
    fn->setType(fn->returnType);

    for (size_t i = 0; i < tmpl.paramNames.size(); ++i) {
        fn->addGenericParam(tmpl.paramNames[i]);
        fn->bindGeneric(tmpl.paramNames[i], typeArgs[i]);
    }
    for (auto& p : fn->parameters) substituteInStatement(p, bindings);
    substituteInStatement(fn->body, bindings);
    fn->setSpan(tmpl.decl->getSpan());

    for (const auto& t : typeArgs) ensureInstancesIn(t);

    auto expr = fn->express(tmpl.scope);
    if (!expr) {
        console.reportError(Console::SEMANTIC_ERROR,
            "Failed to instantiate generic function '" + tmpl.name + "' as '" + instName + "'",
            FileSpan{});
        return false;
    }
    pending().push_back(expr);
    DEBUG_LOG("[GenericInstantiator] Instantiated function " + instName);
    return true;
}

// ---------------------------------------------------------------------------
// Type-argument inference for bare calls of lazy function templates
// ---------------------------------------------------------------------------
namespace {
using BoundTypes = std::map<std::string, std::shared_ptr<Type>>;

// Matches a declared parameter type against an argument type and binds the type parameters it finds.
// `soft` (bare integer literals): never reports a conflict with an earlier binding.
// Returns false only on a conflict; shapes that do not line up are left for the normal argument check.
bool unifyType(const std::shared_ptr<Type>& param, const std::shared_ptr<Type>& arg,
               const std::vector<std::string>& names, BoundTypes& bound, bool soft, std::string& conflict) {
    if (!param || !arg) return true;

    if (auto udt = std::dynamic_pointer_cast<UserDefinedType>(param)) {
        if (udt->typeArgs.empty()) {
            if (std::find(names.begin(), names.end(), udt->name) == names.end()) return true;   // concrete type
            auto it = bound.find(udt->name);
            if (it == bound.end()) { bound[udt->name] = arg; return true; }
            if (soft || mangleTypeName(it->second) == mangleTypeName(arg)) return true;
            conflict = "'" + udt->name + "' would be both " + it->second->toString() + " and " + arg->toString();
            return false;
        }
        // List<T> against List<i64>
        auto a = std::dynamic_pointer_cast<UserDefinedType>(arg);
        if (!a || a->typeArgs.size() != udt->typeArgs.size()) return true;
        for (size_t i = 0; i < udt->typeArgs.size(); ++i) {
            if (!unifyType(udt->typeArgs[i], a->typeArgs[i], names, bound, soft, conflict)) return false;
        }
        return true;
    }
    if (auto p = std::dynamic_pointer_cast<PointerType>(param)) {
        if (auto a = std::dynamic_pointer_cast<PointerType>(arg)) return unifyType(p->pointeeType, a->pointeeType, names, bound, soft, conflict);
        if (auto fa = std::dynamic_pointer_cast<FixedArrayType>(arg)) return unifyType(p->pointeeType, fa->elementType, names, bound, soft, conflict);   // array decay
        return true;
    }
    if (auto r = std::dynamic_pointer_cast<ReferenceType>(param)) {
        // A `T&` parameter takes the object itself (its address is implicit), or an address already.
        if (auto ar = std::dynamic_pointer_cast<ReferenceType>(arg)) return unifyType(r->referentType, ar->referentType, names, bound, soft, conflict);
        if (auto ap = std::dynamic_pointer_cast<PointerType>(arg)) return unifyType(r->referentType, ap->pointeeType, names, bound, soft, conflict);
        return unifyType(r->referentType, arg, names, bound, soft, conflict);
    }
    if (auto n = std::dynamic_pointer_cast<NullableType>(param)) {
        if (auto an = std::dynamic_pointer_cast<NullableType>(arg)) return unifyType(n->innerType, an->innerType, names, bound, soft, conflict);
        return unifyType(n->innerType, arg, names, bound, soft, conflict);   // T converts to T?
    }
    if (auto fa = std::dynamic_pointer_cast<FixedArrayType>(param)) {
        if (auto a = std::dynamic_pointer_cast<FixedArrayType>(arg)) return unifyType(fa->elementType, a->elementType, names, bound, soft, conflict);
        return true;
    }
    if (auto da = std::dynamic_pointer_cast<DynamicArrayType>(param)) {
        if (auto a = std::dynamic_pointer_cast<DynamicArrayType>(arg)) return unifyType(da->elementType, a->elementType, names, bound, soft, conflict);
        return true;
    }
    if (auto sp = std::dynamic_pointer_cast<SmartPointerType>(param)) {
        if (auto a = std::dynamic_pointer_cast<SmartPointerType>(arg)) return unifyType(sp->pointeeType, a->pointeeType, names, bound, soft, conflict);
        return true;
    }
    if (auto fn = std::dynamic_pointer_cast<FunctionType>(param)) {
        auto a = std::dynamic_pointer_cast<FunctionType>(arg);
        if (!a || a->parameterTypes.size() != fn->parameterTypes.size()) return true;
        for (size_t i = 0; i < fn->parameterTypes.size(); ++i) {
            if (!unifyType(fn->parameterTypes[i], a->parameterTypes[i], names, bound, soft, conflict)) return false;
        }
        return unifyType(fn->returnType, a->returnType, names, bound, soft, conflict);
    }
    return true;   // concrete type: nothing to bind
}
} // namespace

bool GenericInstantiator::inferFunctionTypeArgs(const std::string& callee,
                                                const std::vector<std::shared_ptr<Type>>& argTypes,
                                                const std::vector<bool>& literalInts,
                                                std::string& suffix,
                                                std::string& error) {
    suffix.clear();
    error.clear();
    const auto it = functionTemplates().find(bareName(callee));
    if (it == functionTemplates().end()) return false;   // not a template: nothing to infer
    const FunctionTemplate& tmpl = it->second;
    if (tmpl.paramNames.empty()) return false;

    std::vector<std::shared_ptr<Type>> paramTypes;
    for (const auto& p : tmpl.decl->parameters) {
        if (auto ps = std::dynamic_pointer_cast<ParameterStatement>(p)) paramTypes.push_back(ps->getType());
    }

    BoundTypes bound;
    std::string conflict;
    const auto defaultInt = resolveType({"int"});

    // Pass 0: ordinary arguments fix the type parameters. Pass 1: bare integer literals default to `int`
    // only for a parameter nothing else bound, so `max(someI64, 5)` stays i64.
    for (int pass = 0; pass < 2; ++pass) {
        for (size_t i = 0; i < argTypes.size() && i < paramTypes.size(); ++i) {
            const bool literal = i < literalInts.size() && literalInts[i];
            if (literal != (pass == 1)) continue;
            auto argType = argTypes[i];
            if (literal && argType && mangleTypeName(argType) == "i8" && defaultInt && !defaultInt->isInvalid()) {
                argType = defaultInt;
            }
            if (!unifyType(paramTypes[i], argType, tmpl.paramNames, bound, literal, conflict)) {
                error = "Cannot infer the type arguments of '" + tmpl.name + "': " + conflict + ".";
                return false;
            }
        }
    }

    std::string joined;
    for (size_t i = 0; i < tmpl.paramNames.size(); ++i) {
        const auto b = bound.find(tmpl.paramNames[i]);
        if (b == bound.end() || !b->second) {
            error = "Cannot infer type parameter '" + tmpl.paramNames[i] + "' of '" + tmpl.name +
                    "' from the call's arguments.";
            return false;
        }
        if (i) joined += "__";
        joined += mangleTypeName(b->second);
    }
    suffix = joined;
    return true;
}

} // namespace Omniscript