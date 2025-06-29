#include <omniscript/Core.h>
#include <omniscript/utils.h>
#include <omniscript/Lexer.h>
#include <omniscript/Parser.h>
#include <omniscript/omniscript_pch.h>
#include <omniscript/Statement.h>
#include <omniscript/Symboltable.h>

#include <omniscript/Statements/ControlFlowStatements.h>
#include <omniscript/Statements/AssignmentAndGetterStatements.h>

// #include <omniscript/runtime/object.h>
// #include <omniscript/runtime/Class.h>
// #include <omniscript/runtime/Namespace.h>
// #include <omniscript/runtime/Enum.h>
// #include <omniscript/runtime/object.h>
// // #include <omniscript/runtime/Function.h>
// #include <omniscript/runtime/Number.h>
// #include <omniscript/runtime/String.h>
// #include <omniscript/runtime/Pointer.h>

void Initializer::initialize() {
    
    
}

std::shared_ptr<Omniscript::Expression> Initializer::express(SymbolTableType scope) {
    Omniscript::setPosition(pos.line, pos.col, pos.filePath);
    return nullptr;
}  


std::shared_ptr<Omniscript::Expression> BlockStatement::express(SymbolTableType scope) {
    Omniscript::setPosition(pos.line, pos.col, pos.filePath);
    auto block = std::make_shared<Omniscript::BlockExpression>(expressAsVector(scope));
    block->isGlobal = isGlobal;
    block->setPosition(getPosition());
    return block;
}

std::vector<std::shared_ptr<Omniscript::Expression>> BlockStatement::expressAsVector(SymbolTableType scope) {
    Omniscript::setPosition(pos.line, pos.col, pos.filePath);
    recursiveInternalUpdate();
    
    std::vector<std::shared_ptr<Omniscript::Expression>> results = {};
    
    // // Generate code for each statement in order
    for (const auto& stmt : statements) {
        // Handle type propagation if needed
        DEBUG_LOG(stmt->toString());
        if (auto typed = std::dynamic_pointer_cast<ScopedStatement>(stmt)) {
            if (type) {
                typed->setType(type);
            }
        }

        if (auto typed = std::dynamic_pointer_cast<ReturnStatement>(stmt)) {
            if (type) {
                typed->setType(type);
            }
        } else if (auto assignment = std::dynamic_pointer_cast<Assignment>(stmt)) {
            assignment->setGlobalVisibilityTo(false);
        }

        results.push_back(stmt->express(scope));
        
        // // If the current block already has a terminator, stop generating
        // if (generator.currentBlockHasTerminator()) {
        //     break;
        // }
    }
    
    // // Pop the scope we created for this block
    // generator.popScope();
    
    // // Return the last computed value (may be nullptr for statements without values)
    // return lastValue;
    return results;
}

bool BlockStatement::hasSideEffects() {
    return !isCompileTimeEvaluatable();
}


bool BlockStatement::isCompileTimeEvaluatable() {
    for (const auto& stmt : statements) {
        if (!stmt->isCompileTimeEvaluatable()) {
            return false;
        }
    }
    return true;
}

void BlockStatement::recursiveInternalUpdate() {
    resolveGenerics();
    
    updateInternalContext();

    for (auto& stmt : statements) {
        if (auto assign = std::dynamic_pointer_cast<Assignment>(stmt)) {
            if (assign->isStatic) {
                assign->isGlobal = true;
            } else {
                assign->isGlobal = isGlobal;
            }
        }
        // Todo: work on for loop bodies, while loop etc
        if (auto innerBlock = std::dynamic_pointer_cast<BlockStatement>(stmt)) {
            innerBlock->isGlobal = isGlobal;
        }
    }
}

void BlockStatement::updateInternalContext() {
    for (auto& stmt : statements) {
        if (auto ctxAware = std::dynamic_pointer_cast<ContextAwareStatement>(stmt)) {
            extendContextOf(ctxAware);
        }
        // Todo: work on for loop bodies, while loop etc
        // Todo: Also in internal assignments that are not in bodies?
        // if (auto innerBlock = std::dynamic_pointer_cast<BlockStatement>(stmt)) {
        //     innerBlock->markAsInternal();
        // }
    }
}