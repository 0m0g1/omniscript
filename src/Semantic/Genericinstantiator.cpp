#include <omniscript/Semantic/GenericInstantiator.h>

#include <omniscript/Core.h>
#include <omniscript/Console.h>
#include <omniscript/Statements/CallableStatement.h>
#include <omniscript/Statements/FunctionStatement.h>
#include <omniscript/Statements/ControlFlowStatements.h>
#include <omniscript/Statements/AccessStatements.h>
#include <omniscript/Statements/AssignmentAndGetterStatements.h>
#include <omniscript/Statements/ExpressionStatements.h>

namespace Omniscript {

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

std::string mangleTypeName(const std::shared_ptr<Type>& type) {
    if (!type) return "void";
    if (auto udt = std::dynamic_pointer_cast<UserDefinedType>(type)) {
        // The front end spells a qualified type argument with "__" for each '.'
        // (Str.String -> Str__String). The instance name must use the same spelling, otherwise
        // `L.List_Str__String` and `L.List_Str_String` are two different types to the checker.
        std::string qualified;
        qualified.reserve(udt->name.size() + 4);
        for (char c : udt->name) {
            if (c == '.') qualified += "__";
            else qualified += c;
        }
        return sanitizeForName(qualified);
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

void GenericInstantiator::registerTemplate(const std::string& name,
                                           const std::vector<std::shared_ptr<Statement>>& body,
                                           const GenericParamList& typeParams,
                                           SymbolTableType scope) {
    Template t{name, body, typeParams, scope};
    templates()[name] = t;
    const auto dot = name.rfind('.');
    if (dot != std::string::npos) templates()[name.substr(dot + 1)] = t;
    DEBUG_LOG("[GenericInstantiator] Registered template " + name);
}

bool GenericInstantiator::hasTemplate(const std::string& name) {
    return templates().count(name) > 0;
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
            copy->setType(substituteAll(field->getType()));
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

} // namespace Omniscript