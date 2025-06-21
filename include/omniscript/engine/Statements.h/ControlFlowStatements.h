#include <omniscript/engine/Statement.h>

class ReturnStatement : 
public Terminator,
public ContextAwareStatement {
public:
    ReturnStatement(std::shared_ptr<Statement> value = nullptr, std::shared_ptr<Omniscript::Type> returnType = nullptr)
        : returnValue(value) {
            setType(returnType);
        }
    std::shared_ptr<Statement> evaluate(SymbolTableType scope) override;
    bool hasSideEffects() override;
    bool isCompileTimeEvaluatable() override;
    std::shared_ptr<Omniscript::Expression> express(SymbolTableType scope) override;
    std::shared_ptr<Statement> returnValue;
    std::string toString() const override { return "Return: " + (returnValue? returnValue->toString() : "void;"); }
    std::string formatError(const std::string& msg) const override {
        return "Error in return statement.\n" + msg;
    };
    std::shared_ptr<Statement> clone() const override {
        std::shared_ptr<Statement> clonedReturnValue = returnValue ? returnValue->clone() : nullptr;
        return std::make_shared<ReturnStatement>(clonedReturnValue, type);
    }
};

class ControlFlowStatement :
public ScopedStatement,
public ContextAwareStatement,
public GenericHolder  {
public:
    ~ControlFlowStatement() = default;
    virtual std::string toString() const override { return "ControlFlowStatement"; }
};

class IfStatement : public ControlFlowStatement {
public:
    // Branches for if, else if, and an optional else part
    std::vector<std::shared_ptr<Statement>> conditions;  // conditions of if/else if
    std::vector<std::shared_ptr<BlockStatement>> bodies;      // corresponding bodies (blocks)
    std::shared_ptr<BlockStatement> elseBody;                 // optional else body

    IfStatement(
        std::vector<std::shared_ptr<Statement>> conditions, 
        std::vector<std::shared_ptr<BlockStatement>> bodies,
        std::shared_ptr<BlockStatement> elseBody = nullptr
    ) : conditions(std::move(conditions)),
        bodies(std::move(bodies)),
        elseBody(std::move(elseBody)) {
    }

    // Evaluate the conditions and bodies
    // Convert this IfStatement to an expression (for printing or debugging)
    std::shared_ptr<Statement> evaluate(SymbolTableType scope) override;
    std::shared_ptr<Omniscript::Expression> express(SymbolTableType scope) override;
    

    // String representation for debugging
    std::string toString() const override {
        std::string result = "IfStatement with " + std::to_string(conditions.size()) + " branches";

        for (size_t i = 0; i < conditions.size(); ++i) {
            result += "\n  if (" + conditions[i]->toString() + ") " + bodies[i]->toString();
        }

        if (elseBody) {
            result += "\n  else " + elseBody->toString();
        }

        return result;
    }  

    std::string formatError(const std::string& msg) const override {
        return "Error in if statement.\n" + msg;
    };

    // Clone the IfStatement (deep copy)
    std::shared_ptr<Statement> clone() const override {
        std::vector<std::shared_ptr<Statement>> clonedConditions;
        std::vector<std::shared_ptr<BlockStatement>> clonedBodies;
        std::shared_ptr<BlockStatement> clonedElseBody = nullptr;
    
        // Clone and cast conditions
        for (const auto& cond : conditions) {
            clonedConditions.push_back(cond->clone());    
        }
    
        // Clone and cast bodies
        for (const auto& body : bodies) {
            auto clonedBody = std::dynamic_pointer_cast<BlockStatement>(body->clone());
            if (clonedBody) {
                clonedBodies.push_back(clonedBody);
            } else {
                console.error("Failed to cast cloned body to BlockStatement");
            }
        }
    
        // Clone and cast elseBody if present
        if (elseBody) {
            clonedElseBody = std::dynamic_pointer_cast<BlockStatement>(elseBody->clone());
            if (!clonedElseBody) {
                console.error("Failed to cast cloned elseBody to BlockStatement");
            }
        }
    
        return std::make_shared<IfStatement>(clonedConditions, clonedBodies, clonedElseBody);
    }    
};


// A while statement
class WhileStatement : public ControlFlowStatement {
public:
    WhileStatement(std::shared_ptr<Statement> condition, std::shared_ptr<BlockStatement> body = {})
        : condition(condition), body(body) {}

    std::shared_ptr<Statement> evaluate(SymbolTableType scope) override { return nullptr; }
    std::shared_ptr<Omniscript::Expression> express(SymbolTableType scope) override;
    std::shared_ptr<Statement> condition;
    std::shared_ptr<BlockStatement> body;
    std::string toString() const override { 
        return "While: (" + (condition? condition->toString() : "no-condition") + ")"; 
    }
    std::string formatError(const std::string& msg) const override {
        return "Error in while statement.\n" + msg;
    };

private:
    // Helper function to evaluate the condition as a boolean
    bool evaluateCondition(SymbolTableType scope) {
        // auto result = Expression::evaluate(condition, scope);
        
        return false; // If the condition cannot be evaluated to a valid boolean, stop the loop
    }
};

class ForLoop : public ControlFlowStatement {
private:
    std::shared_ptr<Statement> initialization;
    std::shared_ptr<Statement> condition;
    std::shared_ptr<Statement> increment;
    std::shared_ptr<BlockStatement> body;

public:
    ForLoop(std::shared_ptr<Statement> init, std::shared_ptr<Statement> cond, 
            std::shared_ptr<Statement> incr, std::shared_ptr<BlockStatement> body)
        :   initialization(std::move(init)), condition(std::move(cond)), 
            increment(std::move(incr)), body(std::move(body)) {}
    std::shared_ptr<Statement> evaluate(SymbolTableType scope) override { return nullptr; }
    std::shared_ptr<Omniscript::Expression> express(SymbolTableType scope) override;
    std::string toString() const override { 
        return "For: (" + (condition? condition->toString() : "no-condition") + ")"; 
    } 
    std::string formatError(const std::string& msg) const override {
        return "Error in for loop.\n" + msg;
    };
};

class BreakStatement : public ControlFlowStatement {
public:
    std::shared_ptr<Statement> evaluate(SymbolTableType scope) override { return nullptr; }
    std::shared_ptr<Omniscript::Expression> express(SymbolTableType scope) override;
    std::string toString() const override { return "Break;"; }
    std::string formatError(const std::string& msg) const override {
        return "Error in break statement.\n" + msg;
    };
};

class ContinueStatement : public ControlFlowStatement {
public:
    std::shared_ptr<Statement> evaluate(SymbolTableType scope) override { return nullptr; }
    std::shared_ptr<Omniscript::Expression> express(SymbolTableType scope) override;
    std::string toString() const override { return "Continue;"; }
    std::string formatError(const std::string& msg) const override {
        return "Error in continue statement.\n" + msg;
    };
};