#pragma once
#include <map>
#include <string>
#include <vector>
#include <memory>

#include <omniscript/Statements/Statement.h>
#include <omniscript/Statements/StructConstructorStatement.h>
#include <omniscript/Expressions/BlockExpression.h>
#include <omniscript/Symboltable.h>
#include <omniscript/Types/Types.h>
#include <omniscript/Types/DerivedTypes.h>

namespace Omniscript {

// Canonical mangled spelling of a type, used in instance names: i64 -> "i64", T* -> "T_ptr",
// List<i64> -> "List_i64". Spelling aliases (i64 / int64) unify because it works on resolved types.
std::string mangleTypeName(const std::shared_ptr<Type>& type);

// "i64_f64" for List<i64, f64>-style argument lists.
std::string mangleTypeArgs(const std::vector<std::shared_ptr<Type>>& args);

// Replaces every use of type parameter `name` inside `type` with `concrete`, looking through
// ref/pointer/nullable/array/smart-pointer wrappers AND through generic arguments of
// instantiated user types (List<T>* -> List<i64>*, renaming the UDT accordingly).
std::shared_ptr<Type> substituteTypeParam(
    const std::shared_ptr<Type>& type,
    const std::string& name,
    const std::shared_ptr<Type>& concrete);

// One type-parameter binding used when substituting inside a cloned generic body.
//   name    : the parameter as written in the source ("T")
//   type    : the concrete type it is bound to
//   mangled : spelling used in specialized names ("i64", "ptr_i32", "List_i64"). Must equal what the
//             specialized declaration name was built from, otherwise `sort_T` -> `sort_<mangled>`
//             will not find the instance. Empty = mangleTypeName(type).
struct GenericBinding {
    std::string name;
    std::shared_ptr<Type> type;
    std::string mangled;
};

class GenericInstantiator {
public:
    // Walks a cloned generic body in place and substitutes every bound type parameter:
    // type/rootType on every typed node, Call callee names (`sort_T` -> `sort_i64`),
    // ObjectConstructorStatement::objectType, cast target types, nested blocks/branches/loops.
    static void substituteInStatement(const std::shared_ptr<Statement>& stmt,
                                      const std::vector<GenericBinding>& bindings);

    // `sort_T` -> `sort_i64` for {T: i64}. Single pass; matches "_<Param>" only at a boundary
    // (end of string, or followed by '_', '.', '(').
    static std::string applyNameSubstitution(const std::string& name,
                                             const std::vector<GenericBinding>& bindings);

    struct Template {
        std::string name;                                   // final (module-qualified) name
        std::vector<std::shared_ptr<Statement>> body;
        GenericParamList typeParams;
        SymbolTableType scope;                              // scope the template was declared in
    };

    // Called by ConstructStructPrototype::express for `struct X<T> {...}`.
    // Registered under the full name and under the bare name after the last '.'.
    static void registerTemplate(const std::string& name,
                                 const std::vector<std::shared_ptr<Statement>>& body,
                                 const GenericParamList& typeParams,
                                 SymbolTableType scope);

    static bool hasTemplate(const std::string& name);

    // Instantiates `udt` (a type produced by resolveType with templateName/typeArgs set) if its
    // template is known and it hasn't been instantiated yet. The concrete struct is expressed in
    // the template's declaring scope (so `scope->addType` registers it there) and its expression
    // is queued: whoever builds the program output must call drainPending() and emit it.
    // Idempotent. Returns true when the instance exists (new or already present).
    static bool ensureInstance(const std::shared_ptr<UserDefinedType>& udt);

    // Walks `type` (through pointers, refs, arrays...) and ensures every generic instance in it.
    static void ensureInstancesIn(const std::shared_ptr<Type>& type);

    // Expressions of instances created since the last call. Emit them before their first use.
    static std::vector<std::shared_ptr<Expression>> drainPending();

private:
    static std::map<std::string, Template>& templates();
    static std::map<std::string, bool>& instances();   // instance name -> created
    static std::vector<std::shared_ptr<Expression>>& pending();
};

} // namespace Omniscript