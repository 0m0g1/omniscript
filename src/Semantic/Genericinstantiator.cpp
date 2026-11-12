#include <omniscript/Semantic/GenericInstantiator.h>

#include <omniscript/Core.h>
#include <omniscript/Console.h>
#include <omniscript/Statements/CallableStatement.h>
#include <omniscript/Statements/FunctionStatement.h>

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
        return sanitizeForName(udt->name);
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
    return type;
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
bool GenericInstantiator::ensureInstance(const std::shared_ptr<UserDefinedType>& udt) {
    if (!udt || udt->templateName.empty() || udt->typeArgs.empty()) return false;

    // Instantiate argument types first (List<List<i64>> needs List<i64>).
    for (const auto& arg : udt->typeArgs) ensureInstancesIn(arg);

    auto it = templates().find(udt->templateName);
    if (it == templates().end()) return false;
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

    if (instances().count(instName)) return true;
    instances()[instName] = true;               // mark first: guards self-referential templates

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

    for (const auto& member : tmpl.body) {
        if (!member) continue;

        if (auto field = std::dynamic_pointer_cast<ParameterStatement>(member)) {
            auto copy = std::make_shared<ParameterStatement>(*field);
            copy->setType(substituteAll(field->getType()));
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
    DEBUG_LOG("[GenericInstantiator] Instantiated " + instName);
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