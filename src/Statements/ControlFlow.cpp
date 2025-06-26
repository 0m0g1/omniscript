<<<<<<< HEAD:src/Statements/ControlFlow.cpp
#include <omniscript/Statement.h>
#include <omniscript/Statements/ExpressionStatements.h>
#include <omniscript/Statements/ControlFlowStatements.h>
#include <omniscript/Statements/AssignmentAndGetterStatements.h>

#include <omniscript/Core.h>
#include <omniscript/utils.h>
#include <omniscript/Symboltable.h>
=======
#include <omniscript/Statement.h>
#include <omniscript/Statements/ExpressionStatements.h>
#include <omniscript/Statements/ControlFlowStatements.h>
#include <omniscript/Statements/AssignmentAndGetterStatements.h>

#include <omniscript/Core.h>
#include <omniscript/utils.h>
#include <omniscript/Symboltable.h>
>>>>>>> 7ccebff50dd27e70cffd4d578dcb358f4c9e1613:src/engine/Statements/ControlFlow.cpp
#include <omniscript/omniscript_pch.h>

// ============================== Control flow statements  ============================== //

std::shared_ptr<Omniscript::Expression> ReturnStatement::express(SymbolTableType scope) {
    Omniscript::setPosition(pos.line, pos.col, pos.filePath);
    if (type) {
        DEBUG_LOG("[Return] Creating a return value of kind '" + type->toString() + "'.");
    } else {
        DEBUG_LOG("[Return] The return statement has no type, setting its type to void");
        type = Omniscript::resolveType({"void"});
    }
    std::shared_ptr<Omniscript::Expression> result = nullptr;
    if (returnValue) {
        extendContextOf(returnValue);
        if (auto stmt = std::dynamic_pointer_cast<TypedStatement>(returnValue)) {
            stmt->setType(type);
        }
        result = returnValue->express(scope);

        DEBUG_LOG("[Return] The result of the return value is '" + result->toString() + "' of kind '" + result->getType()->toString() + "'.");
    } else {
        DEBUG_LOG("[Return] The result of the return value is 'void'.");
    }
    
    return std::make_shared<Omniscript::ReturnExpression>(result, type);
}

std::shared_ptr<Statement> ReturnStatement::evaluate(SymbolTableType scope) {
    DEBUG_LOG("[Evaluate Return Statement]");
    std::shared_ptr<Statement> result = returnValue->evaluate(scope);

    if (auto typed = std::dynamic_pointer_cast<TypedStatement>(result)) {
        return std::make_shared<ReturnStatement>(result, typed->getType());
    }
    return std::make_shared<ReturnStatement>(result);
}

bool ReturnStatement::hasSideEffects() {
    return !isCompileTimeEvaluatable();
}

bool ReturnStatement::isCompileTimeEvaluatable() {
    if (returnValue->isCompileTimeEvaluatable()) {
        return true;
    }
    return false;
}

std::shared_ptr<Omniscript::Expression> WhileStatement::express(SymbolTableType scope) {
    Omniscript::setPosition(pos.line, pos.col, pos.filePath);
    auto localScope = scope->createChildScope("whileloop");
    DEBUG_LOG("Creating a while loop expression");

    extendContextOf(condition);
    extendContextOf(body);

    std::shared_ptr<Omniscript::Expression> conditionExpr = condition ? condition->express(localScope) : nullptr;
    DEBUG_LOG("Created its condition expression: " + conditionExpr->toString() + "'.");

    if (body) {
        body->isGlobal = false;
    }
    std::shared_ptr<Omniscript::Expression> bodyExpr = body ? body->express(localScope) : nullptr;
    DEBUG_LOG("Created its body expression: " + bodyExpr->toString() + "'.");

    return std::make_shared<Omniscript::WhileLoopExpression>(conditionExpr, bodyExpr);
 }
 
 std::shared_ptr<Statement> IfStatement::evaluate(SymbolTableType scope) {
     // Iterate through the conditions and check which one is true
     
    for (size_t i = 0; i < conditions.size(); ++i) {
        // Assuming the condition evaluates to a boolean
        auto result = conditions[i]->evaluate(scope); // Evaluate condition

        // If the condition is true, execute the corresponding body
        // if (result->isTruthy(scope)) {
        //     return bodies[i]; // Return the body if condition is true
        // }
    }
 
     // If no condition matches, return the elseBody if present
     if (elseBody) {
         return elseBody;
     }
 
     return nullptr; // If no condition is met and no elseBody exists, return nullptr
 }
 
std::shared_ptr<Omniscript::Expression> IfStatement::express(SymbolTableType scope) {
    Omniscript::setPosition(pos.line, pos.col, pos.filePath);
    std::vector<std::shared_ptr<Omniscript::Expression>> exprConditions;
    std::vector<std::shared_ptr<Omniscript::Expression>> exprBranches;
 
    for (size_t i = 0; i < conditions.size(); ++i) {
        extendContextOf(conditions[i]);
        extendContextOf(bodies[i]);
        bodies[i]->setType(type);
        bodies[i]->isGlobal = false;
        auto localScope = scope->createChildScope("if " + conditions[i]->toString());
        exprConditions.push_back(conditions[i]->express(localScope)); // already Expression pointers
        exprBranches.push_back(bodies[i]->express(localScope)); // convert BlockStatement to Expression
    }
 
     std::shared_ptr<Omniscript::Expression> elseExpr = nullptr;
     if (elseBody) {
        elseBody->isGlobal = false;
        extendContextOf(elseBody);
        elseBody->setType(type);
        auto localScope = scope->createChildScope("else");
        elseExpr = elseBody->express(localScope); // convert BlockStatement to Expression
     }
 
     return std::make_shared<Omniscript::IfExpression>(
        exprConditions,
        exprBranches,
        elseExpr
     );
 }

std::shared_ptr<Omniscript::Expression> BreakStatement::express(SymbolTableType scope) {
    Omniscript::setPosition(pos.line, pos.col, pos.filePath);
    return nullptr;
}

std::shared_ptr<Omniscript::Expression> ContinueStatement::express(SymbolTableType scope) {
    Omniscript::setPosition(pos.line, pos.col, pos.filePath);
    return nullptr;
}

std::shared_ptr<Omniscript::Expression> ForLoop::express(SymbolTableType scope) {
    Omniscript::setPosition(pos.line, pos.col, pos.filePath);
    auto localScope = scope->createChildScope("forloop");
    DEBUG_LOG("Creating a for loop expression");
    std::shared_ptr<Omniscript::Expression> initializationExpr;
    if (initialization) {
        if (auto assign = std::dynamic_pointer_cast<Assignment>(initialization)) {
            assign->isGlobal = false;
        }
        initializationExpr = initialization->express(localScope);
    }
    DEBUG_LOG("Created its initialization expression");
    std::shared_ptr<Omniscript::Expression> conditionExpr = condition? condition->express(localScope) : nullptr;
    DEBUG_LOG("Created its condition expression");
    std::shared_ptr<Omniscript::Expression> increamentExpr = increment? increment->express(localScope) : nullptr;
    DEBUG_LOG("Created its update expression");
    body->isGlobal = false;
    std::shared_ptr<Omniscript::Expression> bodyExpr = body->express(localScope);
    DEBUG_LOG("Created its body");

    return std::make_shared<Omniscript::ForLoopExpression>(initializationExpr, conditionExpr, increamentExpr, bodyExpr);
}