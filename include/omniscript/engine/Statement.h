#pragma once

#include <llvm/IR/Module.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <omniscript/runtime/object.h>
#include <omniscript/Core.h>
#include <omniscript/utils.h>
#include <omniscript/omniscript_pch.h>
#include <omniscript/engine/tokens.h>
#include <omniscript/debuggingtools/console.h>
#include <omniscript/engine/Symboltable.h>

class IRGenerator;

class Statement { // Base class for all statements
    public:
        // enum Type { // implement a statement type for each statement for speed
        //     Value,
        //     Assignment,
        //     ConstantAssignment,
        //     Return,

        // }
        // virtual std::unique_ptr<Statement> clone() const = 0; // clone method
        // virtual void execute(SymbolTable &scope) = 0; //Function to execute a statement
        ~Statement() = default;

        virtual llvm::Value* codegen(IRGenerator& generator) { return nullptr; }
        virtual std::string toString() const { return "Statement"; }

        inline void setPosition(int line, int column, const std::string& file, const std::string& path) {
            pos.line = line;
            pos.col = column;
            pos.fileName = file;
            pos.filePath = path;
        }

        inline void setPosition(Omniscript::filePosition position) {
            pos.line = position.line;
            pos.col = position.col;
            pos.fileName = position.fileName;
        }

        inline Omniscript::filePosition getPosition() {
            return pos;
        }

    protected:
        Omniscript::filePosition pos;

};

class NamedStatement: public virtual Statement {
public:
    ~NamedStatement() = default;
    virtual std::string getName() const = 0;
    virtual std::string toString() const override { return "NamedStatement"; }
};

class TypedStatement : public virtual Statement {
    public:
    explicit TypedStatement() : llvmType(nullptr) {}
    virtual ~TypedStatement() = default;

    llvm::Type* getType() const { return llvmType; }
    void setType(llvm::Type* newType) { llvmType = newType; }
    virtual std::string toString() const override { return "TypedStatement"; }

protected:
    llvm::Type* llvmType;  // Store the type at the base level
};
    
class Terminator: public virtual TypedStatement {
public:
    ~Terminator() = default;
    virtual std::string toString() const override { return "TerminatorStatement"; }
};

class Literal : public TypedStatement {
public:
    virtual ~Literal() = default;
    virtual std::string toString() const override { return "LiteralStatement"; }
};

class BlockStatement : public TypedStatement {
public:
    std::vector<std::shared_ptr<Statement>> statements;

    BlockStatement() = default;
    
    explicit BlockStatement(std::vector<std::shared_ptr<Statement>> statements)
        : statements(std::move(statements)) {}

    // For creating empty blocks
    static std::shared_ptr<BlockStatement> create() {
        return std::make_shared<BlockStatement>();
    }

    // For creating blocks with statements
    static std::shared_ptr<BlockStatement> create(std::vector<std::shared_ptr<Statement>> statements) {
        return std::make_shared<BlockStatement>(std::move(statements));
    }

    // Add a statement to the block
    void addStatement(std::shared_ptr<Statement> stmt) {
        statements.push_back(std::move(stmt));
    }

    llvm::Value* codegen(IRGenerator& generator) override;
    
    std::string toString() const override {
        std::string result = "Block {\n";
        for (const auto& stmt : statements) {
            result += "    " + stmt->toString() + "\n";
        }
        return result + "}";
    }
};


class ImportModule : public Statement {
public:
    std::string moduleName;
    std::string alias;
    std::unordered_map<std::string, std::string> importedAliases;
    std::string path;
    bool importAll;

    ImportModule(const std::string& modName, 
                    const std::string& aliasName, 
                    const std::unordered_map<std::string, std::string>& aliases, 
                    const std::string& modPath, 
                    bool wildcard)
        : moduleName(modName), alias(aliasName), importedAliases(aliases), path(modPath), importAll(wildcard) {}
    
    llvm::Value* codegen(IRGenerator& irGen) override;
};
    
class CreateModule : public NamedStatement {
public:
    std::string moduleName;
    std::vector<std::shared_ptr<Statement>> statements;

    CreateModule(std::string name, std::vector<std::shared_ptr<Statement>> stmts)
    : moduleName(std::move(name)), statements(std::move(stmts)) {}
    
    std::string getName() const override { return moduleName; }
    std::vector<std::shared_ptr<Statement>> getStatements() { return statements; }
    llvm::Value* codegen(IRGenerator& irGen) override;
};


class PublicMember : public NamedStatement {
public:
    std::string memberName;
    std::shared_ptr<Statement> value;

    PublicMember(std::string name, std::shared_ptr<Statement> value)
        : memberName(std::move(name)), value(std::move(value)) {}

    std::string getName() const override { return memberName; }
    std::shared_ptr<Statement> getValue() { return value; }
    llvm::Value* codegen(IRGenerator& irGen) override;
};

class PrivateMember : public NamedStatement {
public:
    std::string memberName;
    std::shared_ptr<Statement> value;

    PrivateMember(std::string name, std::shared_ptr<Statement> value)
        : memberName(std::move(name)), value(std::move(value)) {}

    std::string getName() const override { return memberName; }
    std::string toString() const override { return "PrivateMemberStatement"; }
    llvm::Value* codegen(IRGenerator& irGen) override;
};



class AddressOf : public NamedStatement {
public:
   std::string variableName;

    AddressOf(const std::string& value) : variableName(value) {}

    std::string getName() const override { return variableName; }
    llvm::Value* codegen(IRGenerator& irGen);
    std::string toString() const override { return "Arddressof"; }
};

class ReferenceTo : public NamedStatement {
public:
    std::string variableName;

    ReferenceTo(const std::string& value) : variableName(value) {}

    std::string getName() const override { return variableName; }
    llvm::Value* codegen(IRGenerator& irGen);
    std::string toString() const override { return "PrivateMemberStatement"; }
};

class NullLiteral : public Literal {
public:
    virtual ~NullLiteral() = default; // Fix destructor
    virtual std::string toString() const override { return "LiteralStatement"; }
};

// Represents nullptr (for pointers)
class Nullptr : public NullLiteral {
public:
    Nullptr() {};

    llvm::Value* codegen(IRGenerator& generator) override;
    std::string toString() const override { return "LiteralStatement"; }
};
    
// Represents null for generic types (like JavaScript)
class Null : public NullLiteral {
public:
    Null() {};

    llvm::Value* codegen(IRGenerator& generator) override;
    std::string toString() const override { return "LiteralStatement"; }
};
    

// ============================== Numeric Literals ============================== //
class NumericLiteral : public Literal {
public:
    virtual ~NumericLiteral() = default;
    virtual std::string toString() const override { return "LiteralStatement"; }
};

class IntegerLiteral : public NumericLiteral {
public:
    explicit IntegerLiteral(int64_t val)
        : value(val) {}

    llvm::Value* codegen(IRGenerator& generator) override;
    std::string toString() const override { return "LiteralStatement"; }

private:
    int64_t value;
};

class FloatLiteral : public NumericLiteral {
public:
    explicit FloatLiteral(double val) 
        : value(val) {}

    llvm::Value* codegen(IRGenerator& generator) override;
    std::string toString() const override { return "LiteralStatement"; }
private:
    double value;
};    

// Arbitrary-precision integer (BigInt)
class BigInt : public NumericLiteral {
public:
    BigInt(const std::string& value)
        : value(value) {}

    llvm::Value* codegen(IRGenerator& generator) override;
    #include <cmath>

    static unsigned determineBitWidth(const std::string& value) {
        unsigned numBits = std::ceil(value.length() * 3.32); // log2(10) ≈ 3.32 bits per decimal digit

        if (numBits <= 128) return 128;
        if (numBits <= 256) return 256;
        if (numBits <= 512) return 512;
        return 1024;
    }

    std::string toString() const override { return "LiteralStatement"; }

private:
    std::string value;
    unsigned bitWidth;  // e.g., 128, 256, 1024
};


// ============================== Other Literals ============================== //
class CharacterLiteral : public Literal {
public:
    char value;

    explicit CharacterLiteral(char val) : value(std::move(val)) {}
    llvm::Value* codegen(IRGenerator& generator) override;
    std::string toString() const override { return "LiteralStatement"; }
};
    

class StringLiteral : public Literal {
public:
    std::string value;

    explicit StringLiteral(std::string val) : value(std::move(val)) {}
    llvm::Value* codegen(IRGenerator& generator) override;
    std::string toString() const override { return "LiteralStatement"; }
};

class BoolLiteral : public Literal {
public:
    bool value;

    explicit BoolLiteral(bool val) : value(std::move(val)) {}
    llvm::Value* codegen(IRGenerator& generator) override;
    std::string toString() const override { return "LiteralStatement"; }
};

class FixedArray : public Literal {
public:
    size_t arraySize;
    std::vector<std::shared_ptr<Statement>> initialValues; // Optional initial values

    FixedArray(std::vector<std::shared_ptr<Statement>> values = {})
        : initialValues(std::move(values)) {}

    llvm::Value* codegen(IRGenerator& generator) override;
    std::string toString() const override { return "LiteralStatement"; }
};

        
// ============================== Assignments ============================== //
// Assignments
class Assignment : public NamedStatement {
public:
    void setGlobalVisibilityTo(bool state);
    bool isGlobal = true;
};

class createVariable : public Assignment {
public:
    createVariable(const std::string &variable, llvm::Type* type, std::shared_ptr<Statement> value);
    std::string getName() const override {return variable;}
    llvm::Value* codegen(IRGenerator& generator) override;
    std::string toString() const override { return "LiteralStatement"; }

private:
    std::string variable;
    llvm::Type* type;
    std::shared_ptr<Statement> value;
};

class createConstant : public Assignment {
public:
    createConstant(const std::string &variable, llvm::Type* type, std::shared_ptr<Statement> value);
    std::string getName() const override {return variable;}
    llvm::Value* codegen(IRGenerator& generator) override;
    std::string toString() const override { return "LiteralStatement"; }

private:
    std::string variable;
    llvm::Type* type;
    std::shared_ptr<Statement> value;
};

class createDynamicVariable : public Assignment {
public:
    createDynamicVariable(const std::string &variable, std::shared_ptr<Statement> value);
    std::string getName() const override {return variable;}
    llvm::Value* codegen(IRGenerator& generator) override;
    std::string toString() const override { return "LiteralStatement"; }

private:
    std::string variable;
    std::shared_ptr<Statement> value;
};


// Variable Retrieval
class GetVariable : public NamedStatement, public TypedStatement {
public:
    explicit GetVariable(const std::string &var) : variable(var) {}

    std::string getName() const override { return variable; }

    llvm::Value* codegen(IRGenerator& generator) override;
    std::string toString() const override { return "GetVariable"; }

private:
    std::string variable;
};
    

class GetDynamicVariable : public NamedStatement {
public:
    GetDynamicVariable(const std::string &variable);
    std::string getName() const override {return variable;}
    llvm::Value* codegen(IRGenerator& generator) override;
    std::string toString() const override { return "LiteralStatement"; }

private:
    std::string variable;
};
    

class GenericAssignment : public NamedStatement {
public:
    GenericAssignment(const std::string &variable, std::shared_ptr<Statement> value) :
        variable(variable), value(value) {}
    std::string getName() const override {return variable;}
    llvm::Value* codegen(IRGenerator& generator) override;
    std::string toString() const override { return "LiteralStatement"; }

private:
    std::string variable;
    std::shared_ptr<Statement> value;
    std::shared_ptr<Statement> tempValue;
};

class ReturnStatement : public Terminator {
public:
    ReturnStatement(std::shared_ptr<Statement> value = nullptr, llvm::Type* returnType = nullptr)
        : returnValue(value) {
            setType(returnType);
        }
    llvm::Value* codegen(IRGenerator& generator) override;
    std::shared_ptr<Statement> returnValue;
    std::string toString() const override { return "LiteralStatement"; }
};

class CreateStruct : public NamedStatement {
public:
    std::string structName;
    std::vector<std::shared_ptr<Statement>> members; // Store all struct members

    CreateStruct(std::string name, std::vector<std::shared_ptr<Statement>> members)
    : structName(std::move(name)), members(std::move(members)) {}

    std::string getName() const override { return structName; }
    llvm::Value* codegen(IRGenerator& irGen) override;
    std::string toString() const override { return "LiteralStatement"; }
};

class ParameterStatement : public NamedStatement, public TypedStatement {
public:
    std::string name;
    std::shared_ptr<Statement> defaultValue;

    ParameterStatement(std::string name, std::shared_ptr<Statement> defaultValue = nullptr)
        : name(std::move(name)), defaultValue(std::move(defaultValue)) {}

    std::string getName() const override { return name; }
    llvm::Value* codegen(IRGenerator& irGen) override;
    std::string toString() const override { return "LiteralStatement"; }
    std::shared_ptr<Statement> getDefaultValue();
};
    
class FunctionDeclaration : public NamedStatement, public TypedStatement {
public:
    std::string name;
    std::vector<std::pair<std::string, std::string>> typeParams; // Generic types
    std::vector<std::shared_ptr<Statement>> parameters;
    std::shared_ptr<BlockStatement> body;

    FunctionDeclaration(
        const std::string& name,
        const std::vector<std::shared_ptr<Statement>>& parameters,
        std::shared_ptr<BlockStatement> body,
        llvm::Type* returnType = nullptr // Default to nullptr if return type is unknown
    ) : name(name), parameters(parameters), body(body) {
        setType(returnType); // Store the return type using `TypedStatement`
    }

    std::string getName() const override { return name; }
    llvm::Value* codegen(IRGenerator& irGen) override;
    std::string toString() const override { return "LiteralStatement"; }
    
    void setReturnTypes(IRGenerator& generator);
    void setReturnTypesInStatement(
        const std::shared_ptr<Statement>& stmt, 
        llvm::Type* returnType
    );
};
    
    

class Call : public TypedStatement, public NamedStatement {
public:
    Call(const std::string& calleeName, std::vector<std::shared_ptr<Statement>>& arguments) :
    callee(calleeName), args(arguments) {}

    llvm::Value* codegen(IRGenerator& generator) override;
    std::string toString() const override { return "CallStatement"; }
    std::string getName() const override { return callee; }

    private:
        std::string callee;
        std::vector<std::shared_ptr<Statement>> args;
};

// class BlockStatement : public Statement {
// public:
//     BlockStatement(std::vector<std::shared_ptr<Statement>> statements = {})
//     : statements(std::move(statements)) {}

//     llvm::Value* codegen(IRGenerator& generator) override;

// private:
//     std::vector<std::shared_ptr<Statement>> statements;
// };

class IfStatement : public Statement {
public:
    IfStatement(std::shared_ptr<Statement> condition,
                std::shared_ptr<BlockStatement> body = {},
                std::vector<std::shared_ptr<IfStatement>> branches = {}, 
                std::shared_ptr<BlockStatement> falseBranch = {}) 
        : condition(condition), body(body), branches(branches), falseBranch(falseBranch) {}
    
    llvm::Value* codegen(IRGenerator& generator) override;
    
    std::shared_ptr<Statement> condition;
    std::shared_ptr<BlockStatement> body;
    std::vector<std::shared_ptr<IfStatement>> branches;
    std::shared_ptr<BlockStatement> falseBranch;

    bool conditionIsMet(SymbolTable &scope);

    
};


class UnaryExpression : public TypedStatement {
public:
    enum class Position { Prefix, Postfix };

    UnaryExpression(TokenTypes op, std::shared_ptr<Statement> operand, Position pos = Position::Prefix)
        : op(op), operand(operand), position(pos) {
        // Validate that this is a valid unary operator
        if (getOperatorString(op) == "?") {
            throw std::invalid_argument("Invalid unary operator");
        }
    }

    // Get operator as a string (only for unary operators)
    static std::string getOperatorString(TokenTypes op) {
        switch (op) {
            case TokenTypes::Plus: return "+";
            case TokenTypes::Minus: return "-";
            case TokenTypes::LogicalNot: return "!";
            case TokenTypes::Tilde: return "~";
            case TokenTypes::Increment: return "++";
            case TokenTypes::Decrement: return "--";
            default: return "?";
        }
    }

    // Accessors
    TokenTypes getOperator() const { return op; }
    std::shared_ptr<Statement> getOperand() const { return operand; }
    Position getPosition() const { return position; }

    // Code generation method
    llvm::Value* codegen(IRGenerator& generator) override;
    std::string toString() const override { return "UnaryExpressionStatement"; }

private:
    TokenTypes op;
    std::shared_ptr<Statement> operand;
    Position position;  // For ++/-- to distinguish prefix/postfix
};
    


// Binary expression statement
class BinaryExpression : public TypedStatement {
public:
    BinaryExpression(std::shared_ptr<Statement> left = std::shared_ptr<Statement>{}, TokenTypes op = TokenTypes::Null, std::shared_ptr<Statement> right = std::shared_ptr<Statement>{})
        : left(left), op(op), right(right) {}

     // Helper function to get operator as a string
    static std::string getOperatorString(TokenTypes op) {
        switch (op) {
            case TokenTypes::Plus: return "+";
            case TokenTypes::Minus: return "-";
            case TokenTypes::Multiply: return "*";
            case TokenTypes::Divide: return "/";
            case TokenTypes::Modulo: return "%";
            case TokenTypes::Equals: return "==";
            case TokenTypes::NotEquals: return "!=";
            case TokenTypes::LessThan: return "<";
            case TokenTypes::GreaterThan: return ">";
            case TokenTypes::LessEqual: return "<=";
            case TokenTypes::GreaterEqual: return ">=";
            default: return "?";
        }
    }

    
    // Method to evaluate the binary expression
    llvm::Value* codegen(IRGenerator& generator) override;
    std::string toString() const override { return "BinaryStatement"; }
    
private:
    std::shared_ptr<Statement> left;
    TokenTypes op;
    std::shared_ptr<Statement> right;
};



class TernaryExpression : public TypedStatement {
    public:
        TernaryExpression(std::shared_ptr<Statement> condition, std::shared_ptr<Statement> truthy, std::shared_ptr<Statement> falsey) :
        condition(condition), truthy(truthy), falsey(falsey) {}

    llvm::Value* codegen(IRGenerator& generator) override;
    std::string toString() const override { return "TernaryExpressionStatement"; }

    private:
        std::shared_ptr<Statement> condition;
        std::shared_ptr<Statement> truthy;
        std::shared_ptr<Statement> falsey;
};

// A while statement
class WhileStatement : public Statement {
public:
    WhileStatement(std::shared_ptr<Statement> condition, std::shared_ptr<BlockStatement> body = {})
        : condition(condition), body(body) {}

    llvm::Value* codegen(IRGenerator& generator) override;
    std::shared_ptr<Statement> condition;
    std::shared_ptr<BlockStatement> body;
    std::string toString() const override { return "WhileStatement"; }

private:
    // Helper function to evaluate the condition as a boolean
    bool evaluateCondition(SymbolTable &scope) {
        // auto result = Expression::evaluate(condition, scope);
        
        return false; // If the condition cannot be evaluated to a valid boolean, stop the loop
    }
};


// A class to call methods on objects
class CallMethod : public Statement {
public:
    CallMethod(std::shared_ptr<Statement> object, const std::string& methodName, std::vector<std::shared_ptr<Statement>> args)
        : object(object), methodName(methodName), arguments(std::move(args)) {}
    
    
    llvm::Value* codegen(IRGenerator& generator) override;
    
private:
    std::shared_ptr<Statement> object; // The base object on which the method is called.
    std::string methodName; // The method name.
    std::vector<std::shared_ptr<Statement>> arguments; // The arguments to the method.
    std::string toString() const override { return "LiteralStatement"; }
};

class GetProperty : public Statement {
private:
    std::shared_ptr<Statement> object; // The base object on which the method is called.
    std::string propertyName; // The method name.
    std::vector<std::shared_ptr<Statement>> arguments; // The arguments to the method.

public:
    GetProperty(std::shared_ptr<Statement> object, const std::string& propertyName)
        : object(object), propertyName(propertyName) {}
    
    
    llvm::Value* codegen(IRGenerator& generator) override;
    std::string toString() const override { return "LiteralStatement"; }
};


class ForLoop : public Statement {
    std::shared_ptr<Statement> initialization;
    std::shared_ptr<Statement> condition;
    std::shared_ptr<Statement> increment;
    std::shared_ptr<BlockStatement> body;

public:
    ForLoop(std::shared_ptr<Statement> init, std::shared_ptr<Statement> cond, 
            std::shared_ptr<Statement> incr, std::shared_ptr<BlockStatement> body)
        :   initialization(std::move(init)), condition(std::move(cond)), 
            increment(std::move(incr)), body(std::move(body)) {}
    llvm::Value* codegen(IRGenerator& generator) override;
    std::string toString() const override { return "LiteralStatement"; }
    
};

class BreakStatement : public Statement {
    public:
        llvm::Value* codegen(IRGenerator& generator) override;
        std::string toString() const override { return "LiteralStatement"; }
};

class ContinueStatement : public Statement {
public:
    llvm::Value* codegen(IRGenerator& generator) override;
    std::string toString() const override { return "LiteralStatement"; }
};

// class ObjectConstructorStatement : public Statement {
// private:
//     std::shared_ptr<Object> obj;
//     std::vector<std::shared_ptr<Statement>> constructorArgs;

// public:
//     ObjectConstructorStatement(std::shared_ptr<Object> obj,
//                                std::vector<std::shared_ptr<Statement>> args = {})
//         : obj(obj), constructorArgs(std::move(args)) {}

//     llvm::Value* codegen(IRGenerator& generator) override;
//     std::string toString() const override { return "LiteralStatement"; }
// };

// class ObjectDestructorStatement : public Statement {
// private:
//     std::string variableName; // The variable holding the object reference

// public:
//     explicit ObjectDestructorStatement(const std::string& variableName)
//         : variableName(variableName) {}
//     llvm::Value* codegen(IRGenerator& generator) override;
//     std::string toString() const override { return "LiteralStatement"; }
// };