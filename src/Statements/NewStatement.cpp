// NewStatement.cpp
#include <omniscript/Statements/NewStatement.h>
#include <omniscript/Statements/CallableStatement.h>     // ObjectConstructorStatement
#include <omniscript/Expressions/NewExpression.h>
#include <omniscript/Expressions/BlockExpression.h>
#include <omniscript/Expressions/StructExpression.h>     // InstanceExpression
#include <omniscript/Core.h>
#include <omniscript/utils.h>
#include <omniscript/Types/Types.h>   // resolveType
#include <set>

namespace Omniscript {

std::shared_ptr<Expression> NewStatement::express(SymbolTableType scope) {
    static int counter = 0;
    const std::string hidden = "__new_" + std::to_string(counter++);

    // `new(a) T(...)`: the allocator expression (an Arena*), evaluated before the object is made
    std::shared_ptr<Expression> allocExpr;
    if (allocator) {
        allocExpr = allocator->express(scope);
        if (!allocExpr) return nullptr;
    }

    // `new u8[n]` / `new int[n]`: an array of a primitive type. There is no constructor to run, so it
    // skips ObjectConstructorStatement and yields a plain `T*` (see IRGenerator::createNew).
    if (count) {
        static const std::set<std::string> kPrimitiveNames = {
            "u8", "u16", "u32", "u64", "i8", "i16", "i32", "i64",
            "int", "int8", "int16", "int32", "int64", "uint8", "uint16", "uint32", "uint64",
            "float", "double", "f32", "f64", "char", "bool", "byte"
        };
        if (kPrimitiveNames.count(typeName)) {
            if (!args.empty()) {
                console.reportError(Console::SYNTAX_ERROR,
                    "new '" + typeName + "[...]': an array of a primitive type takes no constructor arguments", getSpan());
                return nullptr;
            }
            auto elemType = resolveType({ typeName });
            if (!elemType) {
                console.reportError(Console::TYPE_ERROR, "new: unknown type '" + typeName + "'", getSpan());
                return nullptr;
            }
            auto countExpr = count->express(scope);
            if (!countExpr) return nullptr;
            auto primitiveNew = std::make_shared<NewExpression>(
                typeName, hidden, std::vector<std::shared_ptr<MemberExpression>>{}, nullptr,
                Type::createPointerType(elemType), countExpr);
            primitiveNew->allocator = allocExpr;
            primitiveNew->setSpan(getSpan());
            return primitiveNew;
        }
    }

    auto ctor = std::make_shared<ObjectConstructorStatement>(typeName, hidden, args);
    ctor->setSpan(getSpan());
    auto inner = ctor->express(scope);
    if (!inner) return nullptr;   // ObjectConstructorStatement already reported the error

    std::shared_ptr<InstanceExpression> instance;
    std::shared_ptr<Expression> ctorCall;
    if (auto block = std::dynamic_pointer_cast<BlockExpression>(inner)) {
        if (!block->values.empty()) instance = std::dynamic_pointer_cast<InstanceExpression>(block->values[0]);
        if (block->values.size() > 1) ctorCall = block->values[1];
    } else {
        instance = std::dynamic_pointer_cast<InstanceExpression>(inner);
    }
    if (!instance || !instance->type) {
        console.reportError(Console::INTERNAL_ERROR,
            "new '" + typeName + "' did not produce an instance", getSpan());
        return nullptr;
    }

    std::shared_ptr<Expression> countExpr;
    if (count) {
        countExpr = count->express(scope);
        if (!countExpr) return nullptr;   // the element count already reported its error
    }

    auto ptrType = Type::createPointerType(instance->type);
    auto result = std::make_shared<NewExpression>(typeName, hidden, instance->members, ctorCall, ptrType, countExpr);
    result->allocator = allocExpr;
    result->setSpan(getSpan());
    return result;
}

} // namespace Omniscript