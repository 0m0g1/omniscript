#include <omniscript/Statement.h>
#include <omniscript/Statements/ExpressionStatements.h>
#include <omniscript/Statements/ControlFlowStatements.h>
#include <omniscript/Statements/AssignmentAndGetterStatements.h>
#include <omniscript/Core.h>
#include <omniscript/utils.h>
#include <omniscript/Symboltable.h>
#include <omniscript/Expressions/ControlFlowExpressions.h>

namespace Omniscript {

std::shared_ptr<Expression> ReturnStatement::express(SymbolTableType scope) {
    if (type) {
        DEBUG_LOG("[Return] Creating a return value of kind '" + type->toString() + "'.");
    } else {
        DEBUG_LOG("[Return] The return statement has no type, setting its type to void");
        type = resolveType({"void"});
    }

    std::shared_ptr<Expression> result = nullptr;
    if (returnValue) {
        extendContextOf(returnValue);
        if (auto stmt = std::dynamic_pointer_cast<TypedStatement>(returnValue)) {
            stmt->setType(type);
        }
        result = returnValue->express(scope);

        if (!result) {
            console.reportError(
                Console::RUNTIME_ERROR,
                "Failed to evaluate return value expression",
                Console::formatString(
                    "To resolve this:\n"
                    "1. Check return value expression validity\n"
                    "2. Verify type matches function return type '%s'\n"
                    "3. Add debug output for the return value\n"
                    "4. Check for null or invalid expressions\n"
                    "5. Verify all variables in expression are initialized",
                    type->toString().c_str()
                ),
                returnValue->getSpan()
            );
            return nullptr;
        }

        DEBUG_LOG("[Return] The result of the return value is '" + result->toString() + "' of kind '" + result->getType()->toString() + "'.");
    } else {
        DEBUG_LOG("[Return] The result of the return value is 'void'.");
    }
    
    auto returnstmt = std::make_shared<ReturnExpression>(result, type);
    returnstmt->setSpan(this->getSpan());
    return returnstmt;
}

std::shared_ptr<Statement> ReturnStatement::evaluate(SymbolTableType scope) {
    DEBUG_LOG("[Evaluate Return Statement]");
    if (!returnValue) {
        return std::make_shared<ReturnStatement>(nullptr);
    }

    std::shared_ptr<Statement> result = returnValue->evaluate(scope);
    if (!result) {
        console.reportError(
            Console::RUNTIME_ERROR,
            "Failed to evaluate return value",
            "To resolve this:\n"
            "1. Check return value expression validity\n"
            "2. Verify all variables are initialized\n"
            "3. Add debug output before return\n"
            "4. Check for runtime errors in expression",
            returnValue->getSpan()
        );
        return nullptr;
    }

    if (auto typed = std::dynamic_pointer_cast<TypedStatement>(result)) {
        auto ret = std::make_shared<ReturnStatement>(result, typed->getType());
        ret->setSpan(this->getSpan());
        return ret;
    }
    
    auto ret = std::make_shared<ReturnStatement>(result);
    ret->setSpan(this->getSpan());
    return ret;
}

bool ReturnStatement::hasSideEffects() {
    return true; // Return statements alter control flow
}

bool ReturnStatement::isCompileTimeEvaluatable() {
    return false; // Return statements depend on runtime values
}

std::shared_ptr<Expression> WhileStatement::express(SymbolTableType scope) {
    auto localScope = scope->createChildScope("whileloop");
    DEBUG_LOG("Creating a while loop expression");

    if (!condition) {
        console.reportError(
            Console::SYNTAX_ERROR,
            "While statement missing condition",
            "To resolve this:\n"
            "1. Add a condition expression\n"
            "2. Check for syntax errors\n"
            "3. Verify proper while statement structure",
            getSpan()
        );
        return nullptr;
    }

    extendContextOf(condition);
    extendContextOf(body);

    std::shared_ptr<Expression> conditionExpr = condition->express(localScope);
    if (!conditionExpr) {
        console.reportError(
            Console::RUNTIME_ERROR,
            "Failed to evaluate while loop condition",
            "To resolve this:\n"
            "1. Check condition expression validity\n"
            "2. Verify condition evaluates to boolean\n"
            "3. Add debug output for condition\n"
            "4. Check for null or invalid expressions",
            condition->getSpan()
        );
        return nullptr;
    }
    DEBUG_LOG("Created its condition expression: " + conditionExpr->toString() + "'.");

    if (!body) {
        console.reportWarning(
            "While loop with empty body\n"
            "Consider adding a body or removing the loop if intentional\n",
            getSpan()
        );
    }

    std::shared_ptr<Expression> bodyExpr = nullptr;
    if (body) {
        body->isGlobal = false;
        bodyExpr = body->express(localScope);
        if (!bodyExpr) {
            console.reportError(
                Console::RUNTIME_ERROR,
                "Failed to evaluate while loop body",
                "To resolve this:\n"
                "1. Check body statement validity\n"
                "2. Verify no runtime errors in body\n"
                "3. Add debug output in body\n"
                "4. Check for infinite loops",
                body->getSpan()
            );
            return nullptr;
        }
        DEBUG_LOG("Created its body expression: " + bodyExpr->toString() + "'.");
    }

    auto whileLoop = std::make_shared<WhileLoopExpression>(conditionExpr, bodyExpr);
    whileLoop->setSpan(this->getSpan());
    return whileLoop;
}

std::shared_ptr<Expression> IfStatement::express(SymbolTableType scope) {
    if (conditions.empty()) {
        console.reportError(
            Console::SYNTAX_ERROR,
            "If statement with no conditions",
            "To resolve this:\n"
            "1. Add at least one condition\n"
            "2. Check for syntax errors\n"
            "3. Verify proper if statement structure",
            getSpan()
        );
        return nullptr;
    }

    std::vector<std::shared_ptr<Expression>> exprConditions;
    std::vector<std::shared_ptr<Expression>> exprBranches;

    for (size_t i = 0; i < conditions.size(); ++i) {
        if (!conditions[i] || !bodies[i]) {
            console.reportError(
                Console::SYNTAX_ERROR,
                "Invalid if statement branch",
                Console::formatString(
                    "To resolve this:\n"
                    "1. Check condition and body for branch %d\n"
                    "2. Verify neither is null\n"
                    "3. Check for syntax errors",
                    i + 1
                ),
                getSpan()
            );
            return nullptr;
        }

        extendContextOf(conditions[i]);
        extendContextOf(bodies[i]);
        bodies[i]->setType(type);
        bodies[i]->isGlobal = false;
        auto localScope = scope->createChildScope("if " + conditions[i]->toString());

        auto condExpr = conditions[i]->express(localScope);
        if (!condExpr) {
            console.reportError(
                Console::RUNTIME_ERROR,
                Console::formatString("Failed to evaluate if condition %d", i + 1),
                "To resolve this:\n"
                "1. Check condition expression validity\n"
                "2. Verify condition evaluates to boolean\n"
                "3. Add debug output for condition\n"
                "4. Check variable references",
                conditions[i]->getSpan()
            );
            return nullptr;
        }
        exprConditions.push_back(condExpr);

        auto bodyExpr = bodies[i]->express(localScope);
        if (!bodyExpr) {
            console.reportError(
                Console::RUNTIME_ERROR,
                Console::formatString("Failed to evaluate if body %d", i + 1),
                "To resolve this:\n"
                "1. Check body statement validity\n"
                "2. Verify no runtime errors in body\n"
                "3. Add debug output in body\n"
                "4. Check for missing returns",
                bodies[i]->getSpan()
            );
            return nullptr;
        }
        exprBranches.push_back(bodyExpr);
    }

    std::shared_ptr<Expression> elseExpr = nullptr;
    if (elseBody) {
        elseBody->isGlobal = false;
        extendContextOf(elseBody);
        elseBody->setType(type);
        auto localScope = scope->createChildScope("else");
        elseExpr = elseBody->express(localScope);
        if (!elseExpr) {
            console.reportError(
                Console::RUNTIME_ERROR,
                "Failed to evaluate else body",
                "To resolve this:\n"
                "1. Check else body statement validity\n"
                "2. Verify no runtime errors in body\n"
                "3. Add debug output in body\n"
                "4. Check for missing returns",
                elseBody->getSpan()
            );
            return nullptr;
        }
    }

    auto ifExpr = std::make_shared<IfExpression>(exprConditions, exprBranches, elseExpr);
    ifExpr->setSpan(this->getSpan());
    return ifExpr;
}

std::shared_ptr<Statement> IfStatement::evaluate(SymbolTableType scope) {
    auto localScope = scope->createChildScope("ifstmt");
    std::vector<std::shared_ptr<Statement>> evalConditions;
    std::vector<std::shared_ptr<BlockStatement>> evalBodies;
    std::shared_ptr<BlockStatement> evalElseBody = nullptr;

    for (const auto& cond : conditions) {
        auto evalCond = cond->evaluate(localScope);
        if (!evalCond) return nullptr;
        evalConditions.push_back(evalCond);
    }
    for (const auto& body : bodies) {
        auto evalBody = std::dynamic_pointer_cast<BlockStatement>(body->evaluate(localScope));
        if (!evalBody) return nullptr;
        evalBodies.push_back(evalBody);
    }
    if (elseBody) {
        evalElseBody = std::dynamic_pointer_cast<BlockStatement>(elseBody->evaluate(localScope));
        if (!evalElseBody) return nullptr;
    }

    return std::make_shared<IfStatement>(evalConditions, evalBodies, evalElseBody);
}

std::shared_ptr<Expression> ForLoop::express(SymbolTableType scope) {
    auto localScope = scope->createChildScope("forloop");
    DEBUG_LOG("Creating a for loop expression");

    if (!body) {
        console.reportError(
            Console::SYNTAX_ERROR,
            "For loop missing body",
            "To resolve this:\n"
            "1. Add a loop body\n"
            "2. Check for syntax errors\n"
            "3. Verify proper for loop structure",
            getSpan()
        );
        return nullptr;
    }

    std::shared_ptr<Expression> initializationExpr = nullptr;
    if (initialization) {
        if (auto assign = std::dynamic_pointer_cast<Assignment>(initialization)) {
            assign->isGlobal = false;
        }
        initializationExpr = initialization->express(localScope);
        if (!initializationExpr) {
            console.reportError(
                Console::RUNTIME_ERROR,
                "Failed to evaluate for loop initialization",
                "To resolve this:\n"
                "1. Check initialization expression validity\n"
                "2. Verify all variables are initialized\n"
                "3. Add debug output for initialization\n"
                "4. Check for syntax errors",
                initialization->getSpan()
            );
            return nullptr;
        }
    }
    DEBUG_LOG("Created its initialization expression");

    std::shared_ptr<Expression> conditionExpr = nullptr;
    if (condition) {
        conditionExpr = condition->express(localScope);
        if (!conditionExpr) {
            console.reportError(
                Console::RUNTIME_ERROR,
                "Failed to evaluate for loop condition",
                "To resolve this:\n"
                "1. Check condition expression validity\n"
                "2. Verify condition evaluates to boolean\n"
                "3. Add debug output for condition\n"
                "4. Check variable references",
                condition->getSpan()
            );
            return nullptr;
        }
    }
    DEBUG_LOG("Created its condition expression");

    std::shared_ptr<Expression> incrementExpr = nullptr;
    if (increment) {
        incrementExpr = increment->express(localScope);
        if (!incrementExpr) {
            console.reportError(
                Console::RUNTIME_ERROR,
                "Failed to evaluate for loop increment",
                "To resolve this:\n"
                "1. Check increment expression validity\n"
                "2. Verify all variables are initialized\n"
                "3. Add debug output for increment\n"
                "4. Check for syntax errors",
                increment->getSpan()
            );
            return nullptr;
        }
    }
    DEBUG_LOG("Created its update expression");

    body->isGlobal = false;
    std::shared_ptr<Expression> bodyExpr = body->express(localScope);
    if (!bodyExpr) {
        console.reportError(
            Console::RUNTIME_ERROR,
            "Failed to evaluate for loop body",
            "To resolve this:\n"
            "1. Check body statement validity\n"
            "2. Verify no runtime errors in body\n"
            "3. Add debug output in body\n"
            "4. Check for infinite loops",
            body->getSpan()
        );
        return nullptr;
    }
    DEBUG_LOG("Created its body");

    auto forExpr = std::make_shared<ForLoopExpression>(
        initializationExpr,
        conditionExpr,
        incrementExpr,
        bodyExpr
    );
    forExpr->setSpan(this->getSpan());
    return forExpr;
}

std::shared_ptr<Expression> BreakStatement::express(SymbolTableType scope) {
    DEBUG_LOG("Creating a break expression");
    // auto breakExpr = std::make_shared<BreakExpression>();
    // breakExpr->setSpan(this->getSpan());
    // return breakExpr;
    return nullptr;
}

std::shared_ptr<Expression> ContinueStatement::express(SymbolTableType scope) {
    DEBUG_LOG("Creating a continue expression");
    // auto continueExpr = std::make_shared<ContinueExpression>();
    // continueExpr->setSpan(this->getSpan());
    // return continueExpr;
    return nullptr;
}

} // namespace Omniscript
