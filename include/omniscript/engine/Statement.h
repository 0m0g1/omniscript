#pragma once

#include <omniscript/runtime/object.h>
#include <omniscript/Core.h>
#include <omniscript/Core/Types.h>
#include <omniscript/Core/Expression.h>
#include <omniscript/utils.h>
#include <omniscript/omniscript_pch.h>
#include <omniscript/engine/tokens.h>
#include <omniscript/debuggingtools/console.h>
#include <omniscript/engine/Symboltable.h>

using SymbolTableType = std::shared_ptr<SymbolTable<std::shared_ptr<Omniscript::Expression>, std::shared_ptr<Omniscript::Type>>>;

// TODO: Add a formart error virtual method if needed
class Statement { // Base class for all statements
    public:
        // enum Type { // implement a statement type for each statement for speed
        //     Value,
        //     Assignment,
        //     ConstantAssignment,
        //     Return,

        // }
        // virtual std::unique_ptr<Statement> clone() const = 0; // clone method
        // virtual void execute(SymbolTableType scope) = 0; //Function to execute a statement
        ~Statement() = default;

        virtual std::shared_ptr<Statement> clone() const { return nullptr; }
        virtual std::shared_ptr<Statement> evaluate(SymbolTableType scope) { return nullptr; }
        virtual std::shared_ptr<Omniscript::Expression> express(SymbolTableType scope) { return nullptr; }
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
            pos.filePath = position.filePath;
        }

        inline Omniscript::filePosition getPosition() const {
            return pos;
        }

        virtual bool hasSideEffects() { return true; } // Default: assume side effects
        virtual bool isCompileTimeEvaluatable() {
            return !hasSideEffects(); // Default logic: only if no side effects
        }
        virtual std::string formatError(const std::string& msg) const { return msg; }

    protected:
        Omniscript::filePosition pos;
};

class Initializer : public Statement {
public:
    std::vector<std::shared_ptr<Statement>> body = {};
    Initializer() {
        initialize();
    }

    void initialize();
    std::shared_ptr<Statement> evaluate(SymbolTableType scope) override { return nullptr; }
    std::shared_ptr<Omniscript::Expression> express(SymbolTableType scope) override;
};

class NamedStatement: public virtual Statement {
public:
    ~NamedStatement() = default;
    virtual std::string getName() const { return name; };
    void setName(const std::string& newName) { name = newName; }
    virtual std::string toString() const override { return "NamedStatement"; }

protected:
    std::string name;
};

class TypedStatement : public virtual Statement {
public:
    explicit TypedStatement() :type(nullptr) {}
    explicit TypedStatement(const std::string& typeName) : typeName(typeName) {}
    virtual ~TypedStatement() = default;

    std::string typeName;
    std::shared_ptr<Omniscript::Type> getType() const { return type; }
    std::shared_ptr<Omniscript::Type> getRootType() const { return rootType; }
    void setRootType(std::shared_ptr<Omniscript::Type> newType) { if (!rootType) { rootType = std::move(newType); } }
    void setTypeName(const std::string& newTypeName) { typeName = newTypeName; }
    void setType(std::shared_ptr<Omniscript::Type> newType) { type = newType; }
    virtual std::string toString() const override { return "TypedStatement"; }

protected:
    std::shared_ptr<Omniscript::Type> type;
    std::shared_ptr<Omniscript::Type> rootType;
};
    
class Terminator: public virtual TypedStatement {
public:
    ~Terminator() = default;
    virtual std::string toString() const override { return "TerminatorStatement"; }
};

class Expression : public virtual Statement {
public:
    virtual ~Expression() = default;
    virtual bool isTruthy(SymbolTableType scope) const {
        return false;
    }
    std::string toString() const override {
        return "Expression";
    }
};

class Literal : public TypedStatement, public Expression {
public:
    virtual ~Literal() = default;

    virtual std::string toString() const override { return "LiteralStatement"; }

    // Override to indicate no side effects
    virtual bool hasSideEffects() override { return false; }

    // Override to indicate evaluatable at compile time
    virtual bool isCompileTimeEvaluatable() override { return true; }
    virtual std::shared_ptr<Literal> castTo(std::shared_ptr<Omniscript::Type> targetType) const { return nullptr; }
};



class GenericHolder {
public:
    // List of declared generic type names: e.g., `["T", "U"]`
    std::vector<std::shared_ptr<Statement>> body;
    std::vector<std::string> typeParams;

    // When instantiated: map from generic name to actual type
    std::unordered_map<std::string, std::shared_ptr<Omniscript::Type>> genericTypeMap;

    GenericHolder(const std::vector<std::shared_ptr<Statement>>& body = {}) : body(body) {}

    inline void addGenericParam(const std::string& name) {
        typeParams.push_back(name);
    }

    inline void bindGeneric(const std::string& name, const std::shared_ptr<Omniscript::Type>& type) {
        genericTypeMap[name] = type;
    }

    inline std::shared_ptr<Omniscript::Type> resolveGeneric(const std::string& name) const {
        auto it = genericTypeMap.find(name);
        return it != genericTypeMap.end() ? it->second : nullptr;
    }

    inline void inheritGenericsFrom(const GenericHolder& other) {
        for (const auto& name : other.typeParams) {
            this->addGenericParam(name);
        }
        for (const auto& [name, type] : other.genericTypeMap) {
            this->bindGeneric(name, type);
        }
    }

    inline void resolveGenerics() {
        for (const auto& stmt : body) {
            if (auto typed = std::dynamic_pointer_cast<TypedStatement>(stmt)) {
                auto t = typed->getType();
                if (t && t->isGeneric()) {
                    auto resolved = resolveGeneric(t->getName());
                    if (resolved) {
                        typed->setType(resolved);
                    }
                }
            }
    
            // Recursively resolve inner generic holders, if any
            if (auto innerHolder = std::dynamic_pointer_cast<GenericHolder>(stmt)) {
                innerHolder->inheritGenericsFrom(*this);
                innerHolder->resolveGenerics();
            }
        }
    }
    
    inline std::vector<std::shared_ptr<Omniscript::TypeExpression>> createTypeExpressionListFromBoundGenerics() {
        std::vector<std::shared_ptr<Omniscript::TypeExpression>> result;
        result.reserve(genericTypeMap.size());
    
        for (const auto& [name, typePtr] : genericTypeMap) {
            DEBUG_LOG("Creating generic '" + name + "' of kind '" + typePtr->kindName() + "'.");
            result.emplace_back(std::make_shared<Omniscript::TypeExpression>(name, typePtr));
        }
    
        return result;
    }
    

    virtual ~GenericHolder() = default;
};

class BlockStatement : public TypedStatement , public GenericHolder {
public:
    std::vector<std::shared_ptr<Statement>> statements;

    BlockStatement() = default;
    
    explicit BlockStatement(std::vector<std::shared_ptr<Statement>> statements)
        : GenericHolder(statements), statements(std::move(statements)) {}

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

    std::shared_ptr<Statement> clone() const override {
        DEBUG_LOG("Cloning a block statement");
        std::vector<std::shared_ptr<Statement>> clonedStatements;
        for (const auto& stmt : statements) {
            clonedStatements.push_back(stmt->clone());  // Assuming Statement has a `clone` method
        }
        return std::make_shared<BlockStatement>(std::move(clonedStatements));
    }

    std::shared_ptr<Statement> evaluate(SymbolTableType scope) override { return nullptr; }
    std::shared_ptr<Omniscript::Expression> express(SymbolTableType scope) override;
    std::vector<std::shared_ptr<Omniscript::Expression>> expressAsVector(SymbolTableType scope);
    
    std::string toString() const override {
        std::string result = "Block {\n";
        for (const auto& stmt : statements) {
            result += "    " + stmt->toString() + "\n";
        }
        return result + "}";
    }

    bool hasSideEffects() override;
    bool isCompileTimeEvaluatable() override;
    void recursiveUpdate();
};

template<typename T>
class MonomorphizedStatement : public virtual Statement {
public:
    std::unordered_map<std::string, std::shared_ptr<T>> specializations;

    void addSpecialization(const std::string& signature, std::shared_ptr<T> stmt) {
        specializations[signature] = stmt;
    }

    std::shared_ptr<T> getSpecialization(const std::string& signature) const {
        auto it = specializations.find(signature);
        return it != specializations.end() ? it->second : nullptr;
    }

    std::string toString() const override {
        std::string str = "MonomorphizedStatement<" + std::string(typeid(T).name()) + ">: {\n";
        for (const auto& [sig, val] : specializations) {
            str += "  " + sig + " => " + val->toString() + "\n";
        }
        return str + "}";
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
    
    std::shared_ptr<Statement> evaluate(SymbolTableType scope) override { return nullptr; }
    std::shared_ptr<Omniscript::Expression> express(SymbolTableType scope) override;
};
    
class CreateModule : public NamedStatement {
public:
    std::vector<std::shared_ptr<Statement>> statements;

    CreateModule(std::string moduleName, std::vector<std::shared_ptr<Statement>> stmts)
    : statements(std::move(stmts)) {
        setName(moduleName);
    }
    
    std::string getName() const override { return name; }
    std::vector<std::shared_ptr<Statement>> getStatements() { return statements; }
    std::shared_ptr<Statement> evaluate(SymbolTableType scope) override { return nullptr; }
    std::shared_ptr<Omniscript::Expression> express(SymbolTableType scope) override;
};


class PublicMember : public NamedStatement {
public:
    std::shared_ptr<Statement> value;

    PublicMember(std::string memberName, std::shared_ptr<Statement> value)
        : value(std::move(value)) {
            setName(memberName);
        }

    std::string getName() const override { return name; }
    std::shared_ptr<Statement> getValue() { return value; }
    std::shared_ptr<Statement> evaluate(SymbolTableType scope) override { return nullptr; }
    std::shared_ptr<Omniscript::Expression> express(SymbolTableType scope) override;
};

class PrivateMember : public NamedStatement {
public:
    std::shared_ptr<Statement> value;

    PrivateMember(std::string memberName, std::shared_ptr<Statement> value)
        : value(std::move(value)) {
            setName(memberName);
        }

    std::string getName() const override { return name; }
    std::string toString() const override { return "PrivateMemberStatement"; }
    std::shared_ptr<Statement> evaluate(SymbolTableType scope) override { return nullptr; }
    std::shared_ptr<Omniscript::Expression> express(SymbolTableType scope) override;
};


class AddressOf : public NamedStatement, public TypedStatement {
public:
    AddressOf(const std::string& value) {
        setName(value);
    }

    std::string getName() const override { return name; }
    std::shared_ptr<Omniscript::Expression> express(SymbolTableType scope);
    std::string toString() const override { return "Addressof:" + name; }
    std::shared_ptr<Statement> clone() const override {
        return std::make_shared<AddressOf>(name);
    }
    bool hasSideEffects() override { return true; };
    bool isCompileTimeEvaluatable() override { return false; };
};

class ReferenceTo : public NamedStatement, public TypedStatement {
public:
    ReferenceTo(const std::string& value) {
        setName(value);
    }

    std::string getName() const override { return name; }
    std::shared_ptr<Omniscript::Expression> express(SymbolTableType scope);
    std::string toString() const override { return "ReferenceTo: " + name; }
    std::shared_ptr<Statement> clone() const override {
        return std::make_shared<ReferenceTo>(name);
    }
};

class Cast : public virtual TypedStatement, public virtual Expression {
    std::shared_ptr<Statement> value;
    std::shared_ptr<Omniscript::Type> targetType;
public:
    Cast(std::shared_ptr<Statement> value, std::shared_ptr<Omniscript::Type> targetType)
        : value(value), targetType(targetType) {
        setType(targetType);
    }

    std::shared_ptr<Omniscript::Expression> express(SymbolTableType scope) override;

    std::string toString() const override {
        return "((" + targetType->kindName() + ") " + value->toString() + ")";
    }
};


class Invalid : public Literal {
public:
    explicit Invalid() {
        setRootType(Omniscript::Type::createInvalid());
        setType(Omniscript::Type::createInvalid());
    }

    std::shared_ptr<Statement> evaluate(SymbolTableType scope) override { return nullptr; }
    std::shared_ptr<Omniscript::Expression> express(SymbolTableType scope) override;
    std::string toString() const override { return "InvalidStatement"; }
    std::shared_ptr<Statement> clone() const override {
        return std::make_shared<Invalid>();  // Clone using copy constructor
    }
};

class NullLiteral : public Literal {
public:
    virtual ~NullLiteral() = default; // Fix destructor
    virtual std::string toString() const override { return "LiteralStatement"; }
};

// Represents nullptr (for pointers)
class Nullptr : public NullLiteral {
public:
    Nullptr(std::shared_ptr<Omniscript::Type> expectedType = nullptr) {
        setType(expectedType);
        setRootType(Omniscript::Type::createNullPointerType());
    };

    std::shared_ptr<Statement> evaluate(SymbolTableType scope) override { return nullptr; }
    std::shared_ptr<Omniscript::Expression> express(SymbolTableType scope) override;
    std::string toString() const override { return "NullpointerStatement"; }
    std::shared_ptr<Statement> clone() const override {
        return std::make_shared<Nullptr>();  // Clone using copy constructor
    }
};
    
// Represents null for generic types (like JavaScript)
class Null : public NullLiteral {
public:
    Null(std::shared_ptr<Omniscript::Type> expectedType = nullptr) {
        setType(expectedType);
        setRootType(Omniscript::Type::createNullType());
    }

    std::shared_ptr<Statement> evaluate(SymbolTableType scope) override { return nullptr; }
    std::shared_ptr<Omniscript::Expression> express(SymbolTableType scope) override;
    std::string toString() const override { return "NullLiteralStatement"; }
    std::shared_ptr<Statement> clone() const override {
        return std::make_shared<Null>();  // Clone using copy constructor
    }
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
        : value(val) {
            setRootType(Omniscript::Type::createPrimitiveType(Omniscript::Kind::Int8));
        }

    std::shared_ptr<Statement> evaluate(SymbolTableType scope) override { return nullptr; }
    std::shared_ptr<Omniscript::Expression> express(SymbolTableType scope) override;
    int getValue() const { return value; }
    std::string toString() const override { return "IntegerLiteralStatement"; }
    std::shared_ptr<Statement> clone() const override {
        return std::make_shared<IntegerLiteral>(value);  // Clone using copy constructor
    }
    std::shared_ptr<Literal> castTo(std::shared_ptr<Omniscript::Type> targetType) const override;

private:
    int64_t value;
};

class FloatLiteral : public NumericLiteral {
public:
    explicit FloatLiteral(double val) 
        : value(val) {
            setRootType(Omniscript::Type::createPrimitiveType(Omniscript::Kind::Half));
        }

    std::shared_ptr<Statement> evaluate(SymbolTableType scope) override { return nullptr; }
    std::shared_ptr<Omniscript::Expression> express(SymbolTableType scope) override;
    std::string toString() const override { return "FloatLiteralStatement"; }
    std::shared_ptr<Statement> clone() const override {
        return std::make_shared<FloatLiteral>(value);  // Clone using copy constructor
    }
    std::shared_ptr<Literal> castTo(std::shared_ptr<Omniscript::Type> targetType) const override;

private:
    double value;
};    

// Arbitrary-precision integer (BigInt)
class BigInt : public NumericLiteral {
public:
    BigInt(const std::string& value)
        : value(value) {
            setRootType(Omniscript::Type::createNullType());
        }

    std::shared_ptr<Statement> evaluate(SymbolTableType scope) override { return nullptr; }
    std::shared_ptr<Omniscript::Expression> express(SymbolTableType scope) override;

    static unsigned determineBitWidth(const std::string& value) {
        unsigned numBits = std::ceil(value.length() * 3.32); // log2(10) ≈ 3.32 bits per decimal digit

        if (numBits <= 128) return 128;
        if (numBits <= 256) return 256;
        if (numBits <= 512) return 512;
        return 1024;
    }

    std::string toString() const override { return "BigIntLiteralStatement"; }
    std::shared_ptr<Statement> clone() const override {
        return std::make_shared<BigInt>(value);  // Clone using copy constructor
    }

private:
    std::string value;
    unsigned bitWidth;  // e.g., 128, 256, 1024
};


// ============================== Other Literals ============================== //
class CharacterLiteral : public Literal {
public:
    char value;

    explicit CharacterLiteral(char val) : value(std::move(val)) {
        setRootType(Omniscript::Type::createPrimitiveType(Omniscript::Kind::Char));
    }
    std::shared_ptr<Statement> evaluate(SymbolTableType scope) override { return nullptr; }
    std::shared_ptr<Omniscript::Expression> express(SymbolTableType scope) override;
    std::string toString() const override { return "CharacterLiteralStatement"; }
    std::shared_ptr<Statement> clone() const override {
        return std::make_shared<CharacterLiteral>(value);  // Clone using copy constructor
    }
    std::shared_ptr<Literal> castTo(std::shared_ptr<Omniscript::Type> targetType) const override;
};
    

class StringLiteral : public Literal {
public:
    std::string value;

    explicit StringLiteral(std::string val) : value(std::move(val)) {
        setRootType(Omniscript::Type::createPrimitiveType(Omniscript::Kind::Utf32));
    }
    std::shared_ptr<Statement> evaluate(SymbolTableType scope) override { return nullptr; }
    std::shared_ptr<Omniscript::Expression> express(SymbolTableType scope) override;
    std::string toString() const override { return "StringLiteralStatement"; }
    std::shared_ptr<Statement> clone() const override {
        return std::make_shared<StringLiteral>(value);  // Clone using copy constructor
    }
    std::shared_ptr<Literal> castTo(std::shared_ptr<Omniscript::Type> targetType) const override;
};

class BoolLiteral : public Literal {
public:
    bool value;

    explicit BoolLiteral(bool val) : value(std::move(val)) {
        setRootType(Omniscript::Type::createPrimitiveType(Omniscript::Kind::Bool));
    }

    std::shared_ptr<Statement> evaluate(SymbolTableType scope) override { return nullptr; }
    std::shared_ptr<Omniscript::Expression> express(SymbolTableType scope) override;
    std::string toString() const override { return "BoolLiteralStatement"; }
    std::shared_ptr<Statement> clone() const override {
        return std::make_shared<BoolLiteral>(value);  // Clone using copy constructor
    }
    std::shared_ptr<Literal> castTo(std::shared_ptr<Omniscript::Type> targetType) const override;
};

class Array : public Literal {
public:
    size_t arraySize;
    std::vector<std::shared_ptr<Statement>> initialValues; // Optional initial values

    Array(std::vector<std::shared_ptr<Statement>> values = {})
        : initialValues(std::move(values)) {
            setRootType(Omniscript::Type::createPrimitiveType(Omniscript::Kind::Array));
        }

    std::shared_ptr<Statement> evaluate(SymbolTableType scope) override { return nullptr; }
    std::shared_ptr<Omniscript::Expression> express(SymbolTableType scope) override;
    std::string toString() const override { return "ArrayStatement"; }
    std::shared_ptr<Statement> clone() const override {
        std::vector<std::shared_ptr<Statement>> copiedValues;
        for (const auto& val : initialValues) {
            copiedValues.push_back(val->clone());
        }
        return std::make_shared<Array>(copiedValues);  // Clone using copy constructor
    }
};

        
// ============================== Assignments ============================== //
// Assignments
class Assignment : public virtual NamedStatement, public virtual TypedStatement {
public:
    void setGlobalVisibilityTo(bool state);
    bool isStatic = false;
    bool isGlobal = true;
    virtual std::string toString() const override { return "Assignment"; }
};

class AssignVariable : public Assignment {
public:
    AssignVariable(const std::string &variable, std::shared_ptr<Omniscript::Type> type, std::shared_ptr<Statement> value, bool isReassign = false)
    : variable(variable), type(std::move(type)), value(std::move(value)), isReassign(isReassign) {}

    std::string getName() const override {return variable;}
    std::shared_ptr<Statement> evaluate(SymbolTableType scope) override { return nullptr; }
    std::shared_ptr<Omniscript::Expression> express(SymbolTableType scope) override;
    std::string toString() const override { return "AssignVariableStatement"; }
    std::shared_ptr<Statement> clone() const override {
        return std::make_shared<AssignVariable>(variable, type, value->clone());  // Clone the value as well
    }
    std::shared_ptr<Statement> getValue() { return value; }

private:
    std::string variable;
    std::shared_ptr<Omniscript::Type> type;
    std::shared_ptr<Statement> value;
    bool isReassign;
};

class createConstant : public Assignment {
public:
    createConstant(const std::string &variable, std::shared_ptr<Omniscript::Type> type, std::shared_ptr<Statement> value)
    : variable(variable), type(type), value(value) {}
    std::string getName() const override {return variable;}
    std::shared_ptr<Statement> evaluate(SymbolTableType scope) override { return nullptr; }
    std::shared_ptr<Omniscript::Expression> express(SymbolTableType scope) override;
    std::string toString() const override { return "ConstantStatement"; }

    std::shared_ptr<Statement> clone() const override {
        return std::make_shared<createConstant>(variable, type, value->clone());  // Clone the value as well
    }

private:
    std::string variable;
    std::shared_ptr<Omniscript::Type> type;
    std::shared_ptr<Statement> value;
};

class createDynamicVariable : public Assignment {
public:
    createDynamicVariable(const std::string &variable, std::shared_ptr<Statement> value);
    std::string getName() const override {return variable;}
    std::shared_ptr<Statement> evaluate(SymbolTableType scope) override { return nullptr; }
    std::shared_ptr<Omniscript::Expression> express(SymbolTableType scope) override;
    std::string toString() const override { return "LiteralStatement"; }

private:
    std::string variable;
    std::shared_ptr<Statement> value;
};


// Variable Retrieval
class GetVariable : public NamedStatement, public TypedStatement, public Expression {
public:
    explicit GetVariable(const std::string &var) {
        setName(var);
    }

    std::string getName() const override { return name; }

    std::shared_ptr<Statement> evaluate(SymbolTableType scope) override { return nullptr; }
    std::shared_ptr<Omniscript::Expression> express(SymbolTableType scope) override;
    std::string toString() const override { return "GetVariable"; }
    std::shared_ptr<Statement> clone() const override {
        return std::make_shared<GetVariable>(name);
    }

private:
    std::string variable;
};
    

class GetDynamicVariable : public NamedStatement {
public:
    GetDynamicVariable(const std::string &variable);
    std::string getName() const override {return variable;}
    std::shared_ptr<Statement> evaluate(SymbolTableType scope) override { return nullptr; }
    std::shared_ptr<Omniscript::Expression> express(SymbolTableType scope) override;
    std::string toString() const override { return "LiteralStatement"; }

private:
    std::string variable;
};
    

class GenericAssignment : public NamedStatement {
public:
    GenericAssignment(const std::string &variable, std::shared_ptr<Statement> value) :
        variable(variable), value(value) {}
    std::string getName() const override {return variable;}
    std::shared_ptr<Statement> evaluate(SymbolTableType scope) override { return nullptr; }
    std::shared_ptr<Omniscript::Expression> express(SymbolTableType scope) override;
    std::string toString() const override { return "LiteralStatement"; }

private:
    std::string variable;
    std::shared_ptr<Statement> value;
    std::shared_ptr<Statement> tempValue;
};

class ReturnStatement : public Terminator {
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
    std::string toString() const override { return "ReturnStatement"; }
    std::shared_ptr<Statement> clone() const override {
        // Clone the returnValue if it's not nullptr, otherwise leave it as nullptr
        std::shared_ptr<Statement> clonedReturnValue = returnValue ? returnValue->clone() : nullptr;
        
        // Create and return a new ReturnStatement with the cloned returnValue and the returnType
        return std::make_shared<ReturnStatement>(clonedReturnValue, type);  // 'type' is the return type set earlier
    }
};

class CreateStruct : public NamedStatement {
public:
    std::vector<std::shared_ptr<Statement>> members; // Store all struct members

    CreateStruct(std::string structName, std::vector<std::shared_ptr<Statement>> members)
    : members(std::move(members)) {
        setName(name);
    }

    std::string getName() const override { return name; }
    std::shared_ptr<Statement> evaluate(SymbolTableType scope) override { return nullptr; }
    std::shared_ptr<Omniscript::Expression> express(SymbolTableType scope) override;
    std::string toString() const override { return "LiteralStatement"; }
};

class ParameterStatement : public NamedStatement, public TypedStatement {
public:
    bool isConstant;
    std::shared_ptr<Statement> defaultValue;

    ParameterStatement(
        std::string name,
        std::shared_ptr<Statement> defaultValue = nullptr,
        bool isConst = false
    ) : defaultValue(std::move(defaultValue)), isConstant(isConst) {
            setName(name);
        }
    
    ParameterStatement(const ParameterStatement& other)
        : NamedStatement(other), TypedStatement(other),
          defaultValue(other.defaultValue ? other.defaultValue->clone() : nullptr) 
    {
        setPosition(other.getPosition());
    }

    std::string getName() const override { return name; }
    std::shared_ptr<Statement> evaluate(SymbolTableType scope) override { return nullptr; }
    std::shared_ptr<Omniscript::Expression> express(SymbolTableType scope) override;
    std::string toString() const override { return "ParameterStatement"; }
    std::shared_ptr<Statement> getDefaultValue();

    std::shared_ptr<Statement> clone() const override {
        return std::make_shared<ParameterStatement>(*this);
    }
};

class ArgumentStatement : public NamedStatement, public TypedStatement {
public:
    std::string name;
    std::shared_ptr<Statement> value;
    bool isConstant;

    ArgumentStatement(std::string name, std::shared_ptr<Statement> value = nullptr, bool isConstant = false)
        : name(std::move(name)), value(std::move(value)), isConstant(isConstant) {}

    std::string getName() const override { return name; }
    std::shared_ptr<Statement> evaluate(SymbolTableType scope) override { return nullptr; }
    std::shared_ptr<Omniscript::Expression> express(SymbolTableType scope) override;
    std::string toString() const override { return "ArgumentStatement"; }
};

class Callable: public virtual NamedStatement {
public:
    std::vector<std::shared_ptr<Statement>> defaultParams;

    Callable(std::vector<std::shared_ptr<Statement>> params = {}) : defaultParams(params) {}
    ~Callable() = default;

    virtual std::string toString() const override { return "CallableStatement"; }
    
    std::vector<std::shared_ptr<ParameterStatement>> cloneParameters() {
        std::vector<std::shared_ptr<ParameterStatement>> clonedParams;

        for (const auto& param : defaultParams) {
            clonedParams.push_back(std::dynamic_pointer_cast<ParameterStatement>(param->clone()));
        }

        return clonedParams;
    }
};

// ============================== Prototypes ============================== //
class FunctionDeclaration : 
public Callable, 
public TypedStatement, 
public GenericHolder {
public:
    std::shared_ptr<Omniscript::Type> returnType;
    std::vector<std::pair<std::string, std::string>> typeParams; // Generic types
    std::vector<std::shared_ptr<Statement>> parameters;
    std::shared_ptr<BlockStatement> body;
    SymbolTableType localScope;

    FunctionDeclaration(
        const std::string& functionName,
        const std::vector<std::shared_ptr<Statement>>& parameters,
        std::shared_ptr<BlockStatement> body,
        std::shared_ptr<Omniscript::Type> returnType_ = nullptr // Default to nullptr if return type is unknown
    ) : parameters(parameters), body(body), Callable(parameters) {
        setType(std::move(returnType_)); // Store the return type using `TypedStatement`
        returnType = getType();
        setName(functionName);
    }

    std::string getName() const override { return name; }
    std::shared_ptr<Statement> evaluate(SymbolTableType scope) override { return nullptr; }
    std::shared_ptr<Omniscript::Expression> express(SymbolTableType scope) override;
    std::string toString() const override { return "FunctionDeclerationStatement"; }
    std::string generateMangledName() const;
    
    void setReturnTypes();
    void setReturnTypesInStatement(
        const std::shared_ptr<Statement>& stmt, 
        std::shared_ptr<Omniscript::Type> returnType
    );

    // Clone method for FunctionDeclaration
    std::shared_ptr<Statement> clone() const override {
        // Clone the parameters, body, and localScope (deep copy)
        std::vector<std::shared_ptr<Statement>> clonedParameters;
        for (const auto& param : parameters) {
            clonedParameters.push_back(param->clone());  // Assuming Statement has a clone method
        }

        std::shared_ptr<BlockStatement> clonedBody = body ? std::dynamic_pointer_cast<BlockStatement>(body->clone()) : nullptr;

        // Clone the FunctionDeclaration
        return std::make_shared<FunctionDeclaration>(name, clonedParameters, clonedBody, returnType);
    }
};
    
class ConstructStructPrototype : public NamedStatement, public TypedStatement {
public:
    ConstructStructPrototype(const std::string& structName, const std::vector<std::shared_ptr<Statement>>& structBody) :
    body(structBody) {
        setName(structName);
    }
    
    std::string getName() const override { return name; }
    std::shared_ptr<Statement> evaluate(SymbolTableType scope) override { return nullptr; }
    std::shared_ptr<Omniscript::Expression> express(SymbolTableType scope) override;
    std::string toString() const override { return "ConstructStructPrototype"; }

    std::vector<std::shared_ptr<Statement>> getBody() const { return body; }
private:
    std::vector<std::shared_ptr<Statement>> body;
};

struct ClassMemberModifiers {
    // Access modifiers
    enum class AccessModifier { Public, Protected, Private };
    AccessModifier access = AccessModifier::Public; // Default to private
    bool isInitialized = false;

    // Storage specifiers
    bool isStatic = false;
    bool isExtern = false;
    bool isMutable = false;
    bool isThreadLocal = false;

    // Function-specific modifiers
    bool isVirtual = false;
    bool shouldOverride = false;
    bool isFinal = false;
    bool isConst = false;
    bool isVolatile = false;
    bool isNoexcept = false;
    bool isPureVirtual = false; // Implies `= 0`
    bool isExplicit = false;
    bool isInline = false;
    bool isConstexpr = false;

    // Special member function specifiers
    bool isDefault = false; // Implies `= default`
    bool isDeleted = false; // Implies `= delete`

    // Attribute specifiers
    bool isNodiscard = false;
    bool isMaybeUnused = false;
    bool isDeprecated = false;
    bool isLikely = false;
    bool isUnlikely = false;

    // Convenience function to display modifiers as a string
    std::string toString() const {
        std::string result;

        // Access
        switch (access) {
            case AccessModifier::Public: result += "public "; break;
            case AccessModifier::Protected: result += "protected "; break;
            case AccessModifier::Private: result += "private "; break;
        }

        // Storage specifiers
        if (isStatic) result += "static ";
        if (isExtern) result += "extern ";
        if (isMutable) result += "mutable ";
        if (isThreadLocal) result += "thread_local ";

        // Function modifiers
        if (isVirtual) result += "virtual ";
        if (shouldOverride) result += "should_override ";
        if (isFinal) result += "final ";
        if (isConst) result += "const ";
        if (isVolatile) result += "volatile ";
        if (isNoexcept) result += "noexcept ";
        if (isPureVirtual) result += "= 0 (pure virtual) ";
        if (isExplicit) result += "explicit ";
        if (isInline) result += "inline ";
        if (isConstexpr) result += "constexpr ";

        // Special member function specifiers
        if (isDefault) result += "= default ";
        if (isDeleted) result += "= delete ";

        // Attributes
        if (isNodiscard) result += "[[nodiscard]] ";
        if (isMaybeUnused) result += "[[maybe_unused]] ";
        if (isDeprecated) result += "[[deprecated]] ";
        if (isLikely) result += "[[likely]] ";
        if (isUnlikely) result += "[[unlikely]] ";

        return result.empty() ? "none" : result;
    }
};

namespace std {
    template <>
    struct hash<ClassMemberModifiers> {
        size_t operator()(const ClassMemberModifiers& modifiers) const {
            size_t result = 0;

            // Hash the access modifier (enum class)
            result ^= static_cast<size_t>(modifiers.access) + 0x9e3779b9 + (result << 6) + (result >> 2);

            // Hash the bool flags for storage specifiers and function-specific modifiers
            result ^= modifiers.isStatic + 0x9e3779b9 + (result << 6) + (result >> 2);
            result ^= modifiers.isExtern + 0x9e3779b9 + (result << 6) + (result >> 2);
            result ^= modifiers.isMutable + 0x9e3779b9 + (result << 6) + (result >> 2);
            result ^= modifiers.isThreadLocal + 0x9e3779b9 + (result << 6) + (result >> 2);
            result ^= modifiers.isVirtual + 0x9e3779b9 + (result << 6) + (result >> 2);
            result ^= modifiers.shouldOverride + 0x9e3779b9 + (result << 6) + (result >> 2);
            result ^= modifiers.isFinal + 0x9e3779b9 + (result << 6) + (result >> 2);
            result ^= modifiers.isConst + 0x9e3779b9 + (result << 6) + (result >> 2);
            result ^= modifiers.isVolatile + 0x9e3779b9 + (result << 6) + (result >> 2);
            result ^= modifiers.isNoexcept + 0x9e3779b9 + (result << 6) + (result >> 2);
            result ^= modifiers.isPureVirtual + 0x9e3779b9 + (result << 6) + (result >> 2);
            result ^= modifiers.isExplicit + 0x9e3779b9 + (result << 6) + (result >> 2);
            result ^= modifiers.isInline + 0x9e3779b9 + (result << 6) + (result >> 2);
            result ^= modifiers.isConstexpr + 0x9e3779b9 + (result << 6) + (result >> 2);
            result ^= modifiers.isDefault + 0x9e3779b9 + (result << 6) + (result >> 2);
            result ^= modifiers.isDeleted + 0x9e3779b9 + (result << 6) + (result >> 2);
            result ^= modifiers.isNodiscard + 0x9e3779b9 + (result << 6) + (result >> 2);
            result ^= modifiers.isMaybeUnused + 0x9e3779b9 + (result << 6) + (result >> 2);
            result ^= modifiers.isDeprecated + 0x9e3779b9 + (result << 6) + (result >> 2);
            result ^= modifiers.isLikely + 0x9e3779b9 + (result << 6) + (result >> 2);
            result ^= modifiers.isUnlikely + 0x9e3779b9 + (result << 6) + (result >> 2);

            return result;
        }
    };
}

class ClassMember : public NamedStatement, public TypedStatement {
public:
    ClassMember(
        const std::string& memberName,
        std::shared_ptr<Omniscript::Type> memberType,
        std::shared_ptr<Statement> defaultValue,
        const ClassMemberModifiers& memberModifiers
    ) : type(memberType), value(defaultValue), modifiers(memberModifiers) {
        setName(memberName);
        setType(memberType);
    }

    std::string getName() const override { return name; }

    // std::shared_ptr<Statement> getType() const { return type; }
    std::shared_ptr<Statement> getDefaultValue() const { return value; }
    const ClassMemberModifiers& getModifiers() const { return modifiers; }

    std::shared_ptr<Statement> evaluate(SymbolTableType scope) override {
        // No runtime evaluation needed during declaration
        return nullptr;
    }

    std::shared_ptr<Omniscript::Expression> express(SymbolTableType scope) override;

    std::string toString() const override {
        return "ClassMember(" + modifiers.toString() + name + ")";
    }

private:
    std::shared_ptr<Omniscript::Type> type;
    std::shared_ptr<Statement> value;
    ClassMemberModifiers modifiers;
};


class ConstructClassPrototype : public NamedStatement, public TypedStatement {
public:
    ConstructClassPrototype(const std::string& className, const std::vector<std::string>& parentClasses = {}, const std::vector<std::shared_ptr<ClassMember>>& structBody = {}) :
    parentClasses(parentClasses), body(structBody) {
        setName(className);
    }
    
    std::string getName() const override { return name; }
    std::shared_ptr<Statement> evaluate(SymbolTableType scope) override { return nullptr; }
    std::shared_ptr<Omniscript::Expression> express(SymbolTableType scope) override;
    std::string toString() const override { return "ConstructClassPrototype"; }

    std::vector<std::shared_ptr<ClassMember>> getBody() const { return body; }
private:
    std::vector<std::string> parentClasses;
    std::vector<std::shared_ptr<ClassMember>> body;
};

class Call : public TypedStatement, public NamedStatement {
public:
    Call(const std::string& calleeName, std::vector<std::shared_ptr<Statement>>& arguments) :
    callee(calleeName), args(arguments) {
        setName(calleeName);
    }
    Call(const std::string& objectType, const std::string& instanceName, std::vector<std::shared_ptr<Statement>>& arguments) :
    callee(objectType), instanceName(instanceName), args(arguments) {
        setName(objectType);
    }
    Call(std::shared_ptr<Statement> expr, const std::string& calleeName, std::vector<std::shared_ptr<Statement>>& arguments) :
    expr(expr), callee(calleeName), args(arguments) {
        setName(calleeName);
    }
    Call(std::shared_ptr<Statement> expr, const std::string& objectType, const std::string& instanceName, std::vector<std::shared_ptr<Statement>>& arguments) :
    expr(expr), callee(objectType), instanceName(instanceName), args(arguments) {
        setName(objectType);
    }

    std::shared_ptr<Statement> evaluate(SymbolTableType scope) override { return nullptr; }
    std::shared_ptr<Omniscript::Expression> express(SymbolTableType scope) override;
    std::string toString() const override { return "CallStatement"; }
    std::string getName() const override { return callee; }
    bool matchArgumentsToParameters(
        const std::vector<std::shared_ptr<Omniscript::FunctionInputExpression>>& args,
        const std::vector<std::shared_ptr<Omniscript::FunctionInputExpression>>& params,
        SymbolTableType scope
    );
    void setInstanceName(const std::string& name) { instanceName = name; }
    std::string formatError(const std::string& msg) const override {
        return (instanceName.empty() ? "" : instanceName + ".") + callee + ": " + msg;
    };
    
    private:
        std::string callee;
        std::string instanceName;
        std::vector<std::shared_ptr<Statement>> args;
        std::shared_ptr<Statement> expr;
};

// class BlockStatement : public Statement {
// public:
//     BlockStatement(std::vector<std::shared_ptr<Statement>> statements = {})
//     : statements(std::move(statements)) {}

//     std::shared_ptr<Statement> evaluate(SymbolTableType scope) override { return nullptr; }
    // std::shared_ptr<Omniscript::Expression> express(SymbolTableType scope) override;

// private:
//     std::vector<std::shared_ptr<Statement>> statements;
// };

// class IfStatement : public Statement {
// public:
//     IfStatement(std::shared_ptr<Statement> condition,
//                 std::shared_ptr<BlockStatement> body = {},
//                 std::vector<std::shared_ptr<IfStatement>> branches = {}, 
//                 std::shared_ptr<BlockStatement> falseBranch = {}) 
//         : condition(condition), body(body), branches(branches), falseBranch(falseBranch) {}
    
//     std::shared_ptr<Statement> evaluate(SymbolTableType scope) override { return nullptr; }
//     std::shared_ptr<Omniscript::Expression> express(SymbolTableType scope) override;
    
//     std::shared_ptr<Statement> condition;
//     std::shared_ptr<BlockStatement> body;
//     std::vector<std::shared_ptr<IfStatement>> branches;
//     std::shared_ptr<BlockStatement> falseBranch;

 

    
// };

class IfStatement : public Statement {
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


class UnaryExpression : public TypedStatement, public Expression {
public:
    enum class Position { Prefix, Postfix };

    UnaryExpression(TokenTypes op, std::shared_ptr<Statement> operand, Position pos = Position::Prefix)
        : op(op), operand(operand), position(pos) {
        // Validate that this is a valid unary operator
        if (getOperatorString(op) == "?") {
            console.error("Invalid unary operator");
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
    std::shared_ptr<Statement> evaluate(SymbolTableType scope) override { return nullptr; }
    std::shared_ptr<Omniscript::Expression> express(SymbolTableType scope) override;
    std::string toString() const override { return "UnaryExpressionStatement"; }
    std::shared_ptr<Statement> clone() const override {
        // Clone operand if it's not nullptr, otherwise leave it as nullptr
        std::shared_ptr<Statement> clonedOperand = operand ? operand->clone() : nullptr;

        // Return a new UnaryExpression with the cloned operand
        return std::make_shared<UnaryExpression>(op, clonedOperand, position);
    }

private:
    TokenTypes op;
    std::shared_ptr<Statement> operand;
    Position position;  // For ++/-- to distinguish prefix/postfix
};

// Binary expression statement
class BinaryExpression : public TypedStatement, public Expression {
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
    std::shared_ptr<Statement> evaluate(SymbolTableType scope) override { return nullptr; }
    std::shared_ptr<Omniscript::Expression> express(SymbolTableType scope) override;
    std::string toString() const override { return "BinaryStatement"; }
    std::shared_ptr<Statement> clone() const override {
        // Clone left and right operands
        std::shared_ptr<Statement> clonedLeft = left ? left->clone() : nullptr;
        std::shared_ptr<Statement> clonedRight = right ? right->clone() : nullptr;

        // Return a new BinaryExpression with the cloned operands
        return std::make_shared<BinaryExpression>(clonedLeft, op, clonedRight);
    }

    bool hasSideEffects() override;
    bool isCompileTimeEvaluatable() override;

private:
    std::shared_ptr<Statement> left;
    TokenTypes op;
    std::shared_ptr<Statement> right;
};

class TernaryExpression : public TypedStatement {
    public:
        TernaryExpression(std::shared_ptr<Statement> condition, std::shared_ptr<Statement> truthy, std::shared_ptr<Statement> falsey) :
        condition(condition), truthy(truthy), falsey(falsey) {}

    std::shared_ptr<Statement> evaluate(SymbolTableType scope) override { return nullptr; }
    std::shared_ptr<Omniscript::Expression> express(SymbolTableType scope) override;
    std::string toString() const override { return "TernaryExpressionStatement"; }
    std::shared_ptr<Statement> clone() const override {
        // Clone condition, truthy, and falsey operands
        std::shared_ptr<Statement> clonedCondition = condition ? condition->clone() : nullptr;
        std::shared_ptr<Statement> clonedTruthy = truthy ? truthy->clone() : nullptr;
        std::shared_ptr<Statement> clonedFalsey = falsey ? falsey->clone() : nullptr;

        // Return a new TernaryExpression with the cloned operands
        return std::make_shared<TernaryExpression>(clonedCondition, clonedTruthy, clonedFalsey);
    }
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

    std::shared_ptr<Statement> evaluate(SymbolTableType scope) override { return nullptr; }
    std::shared_ptr<Omniscript::Expression> express(SymbolTableType scope) override;
    std::shared_ptr<Statement> condition;
    std::shared_ptr<BlockStatement> body;
    std::string toString() const override { return "WhileStatement"; }

private:
    // Helper function to evaluate the condition as a boolean
    bool evaluateCondition(SymbolTableType scope) {
        // auto result = Expression::evaluate(condition, scope);
        
        return false; // If the condition cannot be evaluated to a valid boolean, stop the loop
    }
};


// A class to call methods on objects
class CallMethod : public Expression {
public:
    CallMethod(std::shared_ptr<Statement> object, const std::string& methodName, std::vector<std::shared_ptr<Statement>> args)
        : object(object), methodName(methodName), arguments(std::move(args)) {}
    
    
    std::shared_ptr<Statement> evaluate(SymbolTableType scope) override { return nullptr; }
    std::shared_ptr<Omniscript::Expression> express(SymbolTableType scope) override;
    
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
    
    
    std::shared_ptr<Statement> evaluate(SymbolTableType scope) override { return nullptr; }
    std::shared_ptr<Omniscript::Expression> express(SymbolTableType scope) override;
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
    std::shared_ptr<Statement> evaluate(SymbolTableType scope) override { return nullptr; }
    std::shared_ptr<Omniscript::Expression> express(SymbolTableType scope) override;
    std::string toString() const override { return "Forloop"; }  
};

class BreakStatement : public Statement {
    public:
        std::shared_ptr<Statement> evaluate(SymbolTableType scope) override { return nullptr; }
    std::shared_ptr<Omniscript::Expression> express(SymbolTableType scope) override;
        std::string toString() const override { return "LiteralStatement"; }
};

class ContinueStatement : public Statement {
public:
    std::shared_ptr<Statement> evaluate(SymbolTableType scope) override { return nullptr; }
    std::shared_ptr<Omniscript::Expression> express(SymbolTableType scope) override;
    std::string toString() const override { return "LiteralStatement"; }
};

class ObjectConstructorStatement : public TypedStatement {
private:
    std::shared_ptr<Statement> expr;
    std::string objectType;
    std::string instanceName;
    std::vector<std::shared_ptr<Statement>> constructorArgs;

public:
    ObjectConstructorStatement(
        const std::string& objectType,
        const std::string& instanceName,
        std::vector<std::shared_ptr<Statement>> args = {})
        : objectType(objectType),
            instanceName(instanceName),
            constructorArgs(std::move(args)) {}
    
    ObjectConstructorStatement(
        std::shared_ptr<Statement> expr,
        const std::string& objectType,
        const std::string& instanceName,
        std::vector<std::shared_ptr<Statement>> args = {})
        :   expr(expr),
            objectType(objectType),
            instanceName(instanceName),
            constructorArgs(std::move(args)) {}

    std::shared_ptr<Statement> evaluate(SymbolTableType scope) override { return nullptr; }
    std::shared_ptr<Omniscript::Expression> express(SymbolTableType scope) override;
    std::string toString() const override { 
        return "ObjectConstructor(" + objectType + " " + instanceName + ")"; 
    }
    void setInstanceName(const std::string& name) { instanceName = name; }
};

class Access : public virtual TypedStatement, public virtual Expression, public virtual NamedStatement {
protected:
    // std::shared_ptr<Statement> expr;
    std::string member;
    std::shared_ptr<Statement> assignmentValue = nullptr;
    std::vector<std::shared_ptr<Statement>> arguments;
    bool isCall = false;

public:
    std::shared_ptr<Statement> expr;
    virtual ~Access() = default;

    void setAssignmentValueTo(std::shared_ptr<Statement> newVal = nullptr) {
        assignmentValue = newVal;
    }

    bool isSetter() const {
        return assignmentValue != nullptr;
    }

    const std::shared_ptr<Statement>& getAssignmentValue() const {
        return assignmentValue;
    }

    void setArguments(const std::vector<std::shared_ptr<Statement>>& args) {
        arguments = args;
        isCall = true;
    }

    bool isMethodCall() const {
        return isCall;
    }

    const std::vector<std::shared_ptr<Statement>>& getArguments() const {
        return arguments;
    }

    std::string toString() const override {
        return "AccessStatement";
    }
};

class MemberAccess : public Access {
private:
    std::string objectName;
    std::vector<std::string> propertyPath;
    std::shared_ptr<Statement> object;

public:
    MemberAccess(std::shared_ptr<Statement> obj, const std::string& member, std::shared_ptr<Statement> assignVal = nullptr) {
        this->expr = obj;
        this->object = obj;
        this->member = member;
        auto named = std::dynamic_pointer_cast<NamedStatement>(obj);
        if (!named) {
            console.error("The object having member '" + member + "' should be named");
        }
        this->name = named->getName();
        setAssignmentValueTo(assignVal);
    }

    MemberAccess(const std::string& objName, const std::vector<std::string>& props, std::shared_ptr<Statement> assignVal = nullptr)
        : objectName(objName), propertyPath(props) {
        setAssignmentValueTo(assignVal);
    }

    const std::shared_ptr<Statement>& getObject() const { return object; }
    const std::string& getMember() const { return member; }

    std::shared_ptr<Statement> clone() const override {
        auto cloned = object
            ? std::make_shared<MemberAccess>(expr->clone(), member, assignmentValue ? assignmentValue->clone() : nullptr)
            : std::make_shared<MemberAccess>(objectName, propertyPath, assignmentValue ? assignmentValue->clone() : nullptr);

        cloned->arguments.reserve(arguments.size());
        for (const auto& arg : arguments) {
            cloned->arguments.push_back(arg->clone());
        }
        cloned->isCall = isCall;
        return cloned;
    }

    std::shared_ptr<Statement> evaluate(SymbolTableType scope) override {
        return nullptr; // Evaluation logic to be filled
    }

    std::shared_ptr<Omniscript::Expression> express(SymbolTableType scope) override;

    std::string toString() const override {
        std::string base = object ? "ObjectMember(" + object->toString() + ")." + member : objectName;

        for (const auto& prop : propertyPath) {
            base += "." + prop;
        }

        if (isCall) {
            std::string argsStr = "(";
            for (size_t i = 0; i < arguments.size(); ++i) {
                argsStr += arguments[i] ? arguments[i]->toString() : "null";
                if (i + 1 < arguments.size()) argsStr += ", ";
            }
            argsStr += ")";
            return "Call: " + base + argsStr;
        } else if (isSetter()) {
            return "Set: " + base + " = " + (assignmentValue ? assignmentValue->toString() : "null");
        } else {
            return "Get: " + base;
        }
    }
};

class Dereference : public Access {
private:
    std::shared_ptr<Statement> pointer;

public:
    Dereference(std::shared_ptr<Statement> ptr, const std::string& member)
        : pointer(ptr) {
        this->member = member;auto named = std::dynamic_pointer_cast<NamedStatement>(ptr);
        if (!named) {
            console.error("The object having member '" + member + "' should be named");
        }
        this->name = named->getName();

    }
    Dereference(std::shared_ptr<Statement> ptr, std::shared_ptr<Statement> assignVal = nullptr)
        : pointer(ptr) {
        auto named = std::dynamic_pointer_cast<NamedStatement>(ptr);
        if (!named) {
            console.error("The object having member '" + member + "' should be named");
        }
        this->name = named->getName();
        setAssignmentValueTo(assignVal);
    }

    const std::shared_ptr<Statement>& getPointer() const { return pointer; }

    std::shared_ptr<Statement> clone() const override {
        return std::make_shared<Dereference>(
            pointer ? pointer->clone() : nullptr,
            assignmentValue ? assignmentValue->clone() : nullptr
        );
    }

    std::shared_ptr<Statement> evaluate(SymbolTableType scope) override {
        return nullptr; // To be implemented
    }

    std::shared_ptr<Omniscript::Expression> express(SymbolTableType scope) override;

    std::string toString() const override {
        return isSetter()
            ? "(*" + (pointer ? pointer->toString() : "null") + " = " + (assignmentValue ? assignmentValue->toString() : "null") + ")"
            : "(*" + (pointer ? pointer->toString() : "null") + ")";
    }
};

class ArrowAccess : public Access {
private:
    std::shared_ptr<Statement> pointer;

public:
    ArrowAccess(std::shared_ptr<Statement> ptr, const std::string& member)
        : pointer(ptr) {
            this->member = member;
            auto named = std::dynamic_pointer_cast<NamedStatement>(ptr);
            if (!named) {
                console.error("The object having member '" + member + "' should be named");
            }
            this->name = named->getName();
    }
    ArrowAccess(std::shared_ptr<Statement> ptr, std::shared_ptr<Statement> assignVal = nullptr)
        : pointer(ptr) {
            auto named = std::dynamic_pointer_cast<NamedStatement>(ptr);
            if (!named) {
                console.error("The object having member '" + member + "' should be named");
            }
            this->name = named->getName();
            assignmentValue = assignVal;
    }

    std::shared_ptr<Statement> clone() const override {
        return std::make_shared<ArrowAccess>(
            pointer ? pointer->clone() : nullptr,
            assignmentValue ? assignmentValue->clone() : nullptr
        );
    }

    std::shared_ptr<Statement> evaluate(SymbolTableType scope) override { return nullptr; }

    std::shared_ptr<Omniscript::Expression> express(SymbolTableType scope) override;

    std::string toString() const override {
        return isSetter()
            ? "(*" + pointer->toString() + " = " + assignmentValue->toString() + ")"
            : "(*" + pointer->toString() + ")";
    }

    const std::shared_ptr<Statement>& getPointer() const { return pointer; }
};

class IndexAccess : public Access {
private:
    std::shared_ptr<Statement> baseExpression;
    std::shared_ptr<Statement> indexExpression;

public:
    IndexAccess(std::shared_ptr<Statement> base, std::shared_ptr<Statement> index, std::shared_ptr<Statement> assignVal = nullptr)
        : baseExpression(base), indexExpression(index) {
            auto named = std::dynamic_pointer_cast<NamedStatement>(base);
            if (!named) {
                console.error("The object having member '" + member + "' should be named");
            }
            this->name = named->getName();
            setAssignmentValueTo(assignVal);
    }

    const std::shared_ptr<Statement>& getBase() const { return baseExpression; }
    const std::shared_ptr<Statement>& getIndex() const { return indexExpression; }

    std::shared_ptr<Statement> clone() const override {
        return std::make_shared<IndexAccess>(
            baseExpression ? baseExpression->clone() : nullptr,
            indexExpression ? indexExpression->clone() : nullptr,
            assignmentValue ? assignmentValue->clone() : nullptr
        );
    }

    std::shared_ptr<Statement> evaluate(SymbolTableType scope) override {
        return nullptr; // Actual logic depends on runtime value resolution
    }

    std::shared_ptr<Omniscript::Expression> express(SymbolTableType scope) override;

    std::string toString() const override {
        std::string baseStr = baseExpression ? baseExpression->toString() : "null";
        std::string indexStr = indexExpression ? indexExpression->toString() : "null";

        if (isCall) {
            std::string argsStr = "(";
            for (size_t i = 0; i < arguments.size(); ++i) {
                argsStr += arguments[i] ? arguments[i]->toString() : "null";
                if (i + 1 < arguments.size()) argsStr += ", ";
            }
            argsStr += ")";
            return "Call: " + baseStr + "[" + indexStr + "]" + argsStr;
        } else if (isSetter()) {
            return "Set: " + baseStr + "[" + indexStr + "] = " + (assignmentValue ? assignmentValue->toString() : "null");
        } else {
            return "Get: " + baseStr + "[" + indexStr + "]";
        }
    }
};



// class ObjectDestructorStatement : public Statement {
// private:
//     std::string variableName; // The variable holding the object reference

// public:
//     explicit ObjectDestructorStatement(const std::string& variableName)
//         : variableName(variableName) {}
//     std::shared_ptr<Statement> evaluate(SymbolTableType scope) override { return nullptr; }
    // std::shared_ptr<Omniscript::Expression> express(SymbolTableType scope) override;
//     std::string toString() const override { return "LiteralStatement"; }
// };

class EnumValue : public NamedStatement, public TypedStatement {
public:
    EnumValue(const std::string& valueName, int& index) :
    valueIndex(index) {
        setName(valueName);
    }

    std::shared_ptr<Statement> evaluate(SymbolTableType scope) override { return nullptr; }
    std::shared_ptr<Omniscript::Expression> express(SymbolTableType scope) override;
    std::string toString() const override {
        return "{" + name + ":" + std::to_string(valueIndex) + "}";
    }
    
    int getIndex() const { return valueIndex; }
    std::string getName() const override { return name; }

private:
    int valueIndex;
};

class EnumConstructor : public NamedStatement {
private:
    std::vector<std::shared_ptr<EnumValue>> values;
    bool hasLookup;
    bool isEnumClass;

public:
    EnumConstructor(
        const std::string& enumName,
        const std::vector<std::shared_ptr<EnumValue>>& values,
        bool hasLookup = false,
        bool isEnumClass = false
    ) : values(values), hasLookup(hasLookup), isEnumClass(isEnumClass) {
        setName(enumName);
    }

    std::shared_ptr<Statement> evaluate(SymbolTableType scope) override { return nullptr; }
    std::shared_ptr<Omniscript::Expression> express(SymbolTableType scope) override;
    std::string toString() const override {
        return "EnumConstructor for " + name + (hasLookup ? " (with lookup)" : "");
    }
    std::string getName() const override { return name; }
};
