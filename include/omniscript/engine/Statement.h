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
    virtual std::string toString() const override { return name; }

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

class ContextAwareStatement : public virtual Statement {
protected:
    std::vector<std::string> accessContext;
    
public:
    void setAccessContext(const std::vector<std::string>& context) {
        accessContext = context;
    }
    
    const std::vector<std::string>& getAccessContext() const {
        return accessContext;
    }
    
    void pushContext(const std::string& newContext) {
        accessContext.push_back(newContext);
    }
    
    void popContext() {
        if (!accessContext.empty()) {
            accessContext.pop_back();
        }
    }

    // Create a deep copy of the context
    std::vector<std::string> copyContext() const {
        return std::vector<std::string>(accessContext); // Explicit copy
    }

    // Create a new nested context
    std::vector<std::string> createChildContext() const {
        return copyContext(); // Returns a copy that can be modified independently
    }

    // For statements that need to extend context
    void extendContextOf(std::shared_ptr<Statement> statement) const {
        auto ctxStmt = std::dynamic_pointer_cast<ContextAwareStatement>(statement);
        
        if (!ctxStmt) {
            // Not all statements need context - this isn't necessarily an error
            // console.error("'" + statement->toString() + "' is not context aware.");
            return;
        }

        // Create a new independent copy of our context
        auto newContext = this->copyContext();
        
        // Merge with any existing context in the target statement
        const auto& existingContext = ctxStmt->getAccessContext();
        newContext.insert(newContext.end(), existingContext.begin(), existingContext.end());
        
        // Set the combined context
        ctxStmt->setAccessContext(newContext);
    }

    // Basic exact match check (case-sensitive)
    bool containsContext(const std::string& contextName) const {
        return std::find(accessContext.begin(), accessContext.end(), contextName) 
               != accessContext.end();
    }

    // Check with custom comparator (e.g., case-insensitive)
    template<typename Comparator>
    bool containsContext(const std::string& contextName, Comparator comp) const {
        return std::find_if(accessContext.begin(), accessContext.end(),
            [&](const std::string& ctx){ return comp(ctx, contextName); }) 
            != accessContext.end();
    }

    // Check if any context matches a predicate
    template<typename Predicate>
    bool containsContext(Predicate pred) const {
        return std::any_of(accessContext.begin(), accessContext.end(), pred);
    }

    // Debug version that returns a string instead of printing
    std::string getContextAsString(const std::string& header = "Access Context") const {
        std::ostringstream oss;
        oss << "=== " << header << " ===\n";
        
        if (accessContext.empty()) {
            oss << "  <empty>\n";
        } else {
            for (size_t i = 0; i < accessContext.size(); ++i) {
                oss << "  " << i << ": " << accessContext[i] << "\n";
            }
        }
        
        oss << "===================\n";
        return oss.str();
    }

    void validateAccessiblity(std::string baseTypeName, std::string memberName, SymbolTableType scope) {
        // Ensure base type is a class
        auto aggregateExpr = std::dynamic_pointer_cast<Omniscript::AggregateExpression>(scope->get(baseTypeName));
        if (!aggregateExpr) {
            auto structExpr = std::dynamic_pointer_cast<Omniscript::StructExpression>(scope->get(baseTypeName));
            auto classExpr = std::dynamic_pointer_cast<Omniscript::ClassExpression>(scope->get(baseTypeName));
            auto moduleExpr = std::dynamic_pointer_cast<Omniscript::ModuleExpression>(scope->get(baseTypeName));
            
            if (auto type = scope->get(baseTypeName)) {
                DEBUG_LOG("Accessing a member of type '" + type->toString() + "'.");
            } else {
                DEBUG_LOG("No type defined");
            }

            std::shared_ptr<Omniscript::MemberExpression> member;

            DEBUG_LOG(getContextAsString());
            if (structExpr) {
                // member = structExpr->getMember(memberName);
            } else if (classExpr) {
                member = classExpr->getMember(memberName);
            } else if (moduleExpr) {
                member = moduleExpr->getMember(memberName);
            } else {
                console.error("Type '" + baseTypeName + "' is not an aggregate Type (class, struct, module).");
                return;
            }

            if (!member) {
                console.error("Member '" + memberName + "' not found in type '" + baseTypeName + "'.");
                return;
            }
        
            
            if (!structExpr && !member->isPublic() && member->isPrivate() && !containsContext(classExpr->getName())) {
                if (classExpr) {
                    console.error("Cannot access private member '" + memberName + "' of class '" + classExpr->getName() + "'.");
                } else if (moduleExpr) {
                    console.error("Cannot access private member '" + memberName + "' of module '" + moduleExpr->getName() + "'.");
                } 
                return;
            }
        }
    }

};

class Expression : 
public virtual Statement,
public virtual ContextAwareStatement {
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
            DEBUG_LOG("Creating generic '" + name + "' of kind '" + typePtr->description() + "'.");
            result.emplace_back(std::make_shared<Omniscript::TypeExpression>(name, typePtr));
        }
    
        return result;
    }
    

    virtual ~GenericHolder() = default;
};

class BlockStatement : 
public TypedStatement , 
public GenericHolder,
public ContextAwareStatement {
public:
    std::vector<std::shared_ptr<Statement>> statements;
    
    BlockStatement(std::vector<std::shared_ptr<Statement>> statements = {})
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
    void recursiveInternalUpdate();
    void updateInternalContext();
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


class IncludeStatement : public Statement {
public:
    std::string path;  // The path to the file to be included

    IncludeStatement(const std::string& includePath)
        : path(includePath) {}

    std::string getPath() const {
        return path;
    }

    // Runtime behavior — usually returns nullptr because includes are handled at parse/preprocess time
    std::shared_ptr<Statement> evaluate(SymbolTableType scope) override {
        return nullptr;
    }

    // Optional: Could represent this as a string literal in expression form
    std::shared_ptr<Omniscript::Expression> express(SymbolTableType scope) override;
    std::vector<std::shared_ptr<Statement>> getStatements();

    std::string toString() const override {
        return "Include \"" + path + "\";";
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

    // Helper function to split the module path (e.g., "Math.Algebra.Matrix" -> {"Math", "Algebra", "Matrix"})
    std::vector<std::string> splitModulePath(const std::string& path);

    // Recursive function to resolve the module path in the scope
    std::shared_ptr<SymbolTableType> resolveModulePath(SymbolTableType scope, const std::vector<std::string>& modulePathComponents);

    // Function to generate the module expression with member access (e.g., "Math.Algebra.Matrix" -> "Matrix")
    std::shared_ptr<Omniscript::Expression> generateModuleExpression(std::shared_ptr<SymbolTableType> module, const std::vector<std::string>& modulePathComponents);

};
    
class CreateModule : 
public NamedStatement, 
public TypedStatement,
public ContextAwareStatement {
private:
    std::string modulePath;

public:
    std::vector<std::shared_ptr<Statement>> statements;

    CreateModule(std::string moduleName, std::vector<std::shared_ptr<Statement>> stmts)
    : statements(std::move(stmts)) {
        setName(moduleName);
    }
    
    std::string getName() const override { return name; }
    std::string getPath() const { return modulePath; }
    void setPath(const std::string& newPath) { modulePath = newPath; }
    std::vector<std::shared_ptr<Statement>> getStatements() { return statements; }
    std::shared_ptr<Statement> evaluate(SymbolTableType scope) override { return nullptr; }
    std::shared_ptr<Omniscript::Expression> express(SymbolTableType scope) override;
    std::shared_ptr<Statement> reinterprateStatement(std::shared_ptr<Statement> statement);
    std::string toString() const override { return "Create module '" + name + "'."; }
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

class Cast : public TypedStatement, public Expression {
    std::shared_ptr<Statement> value;
    std::shared_ptr<Omniscript::Type> targetType;
public:
    Cast(std::shared_ptr<Statement> value, std::shared_ptr<Omniscript::Type> targetType)
        : value(value), targetType(targetType) {
        setType(targetType);
    }

    std::shared_ptr<Omniscript::Expression> express(SymbolTableType scope) override;

    std::string toString() const override {
        return "((" + targetType->description() + ") " + value->toString() + ")";
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
    bool isFloat16 = false;
    bool isFloat32 = false;
    bool isFloat64 = false;
    bool isFloat80 = false;
    bool isFloat128 = false;

    explicit FloatLiteral(__float128 val) 
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
    __float128 value;
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
    char32_t value;

    explicit CharacterLiteral(char32_t val) : value(std::move(val)) {
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
    std::u32string value;

    explicit StringLiteral(std::u32string val) : value(std::move(val)) {
        auto charType = Omniscript::Type::createPrimitiveType(Omniscript::Kind::Char);
        auto stringType = Omniscript::Type::createPointerType(charType);
        setRootType(stringType);
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
class Assignment : 
public NamedStatement, 
public TypedStatement,
public ContextAwareStatement {
public:
    bool isStatic = false;
    bool isConstant = false;
    bool isGlobal = true;
    std::shared_ptr<Statement> value;

    void setGlobalVisibilityTo(bool state) {
        isGlobal = state;
    }
    void markAsConstant(bool state = true)  {
        isConstant = state;
    }
    std::shared_ptr<Statement> getValue() { return value; }
    virtual std::string toString() const override { return "Assignment"; }
};

class AssignVariable : public Assignment {
public:
    AssignVariable(const std::string &var, std::shared_ptr<Omniscript::Type> ty, std::shared_ptr<Statement> val, bool isReassign = false)
    : variable(var), isReassign(isReassign) {
        setType(ty);
        this->value = std::move(val);
    }

    std::string getName() const override { return variable; }
    std::shared_ptr<Statement> evaluate(SymbolTableType scope) override { return nullptr; }
    std::shared_ptr<Omniscript::Expression> express(SymbolTableType scope) override;
    std::string toString() const override {
        return (isConstant ? "AssignConstant" : "AssignVariableStatement") + std::string(": ") + (value ? value->toString() : "null");
    }
    std::shared_ptr<Statement> clone() const override {
        return std::make_shared<AssignVariable>(variable, type, value->clone(), isReassign);
    }

private:
    std::string variable;
    bool isReassign;
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

class ParameterStatement : 
public NamedStatement, 
public TypedStatement,
public ContextAwareStatement {
public:
    bool isVariadic = false;
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

class ArgumentStatement :
public NamedStatement, 
public TypedStatement,
public ContextAwareStatement {
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

class Callable: public NamedStatement {
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
public GenericHolder,
public ContextAwareStatement {
public:
    std::string mangledName;

    bool isRegistered = false;
    bool bodyCompiled = false;
    bool isIntrinsic = false;
    bool isExtern = false;
    bool isStatic = false;
    
    std::string libPath;
    std::string externName;
    std::string intrinsicName;
    
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

    void registerInScope(SymbolTableType scope);
    void compileBody(SymbolTableType scope);

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

class Member : 
public NamedStatement,
public ContextAwareStatement {
public:
    Member(const std::string& memberName, std::shared_ptr<Statement> value, const MemberModifiers& modifiers)
        : value(std::move(value)), modifiers(modifiers) {
        setName(memberName);
    }

    std::string getName() const override { return name; }
    std::shared_ptr<Statement> getValue() const { return value; }
    const MemberModifiers& getModifiers() const { return modifiers; }

    std::shared_ptr<Statement> evaluate(SymbolTableType scope) override {
        return nullptr; // Base members do not evaluate by default
    }

    // Keep express() abstract to enforce implementation in subclasses
    virtual std::shared_ptr<Omniscript::Expression> express(SymbolTableType scope) override = 0;

protected:
    std::shared_ptr<Statement> value;
    MemberModifiers modifiers;
};

class ModuleMember : public Member {
public:
    ModuleMember(const std::string& memberName, std::shared_ptr<Statement> value, MemberModifiers modifiers)
        : Member(memberName, std::move(value), modifiers) {}

    std::shared_ptr<Omniscript::Expression> express(SymbolTableType scope) override;
};
    

class ClassMember : public Member, public TypedStatement {
public:
    ClassMember(
        const std::string& memberName,
        std::shared_ptr<Omniscript::Type> memberType,
        std::shared_ptr<Statement> defaultValue,
        const MemberModifiers& memberModifiers
    ) : Member(memberName, std::move(defaultValue), memberModifiers) {
        setType(memberType);
    }

    std::shared_ptr<Statement> getDefaultValue() const { return value; }

    std::shared_ptr<Omniscript::Expression> express(SymbolTableType scope) override;

    std::string toString() const override {
        return "ClassMember(" + modifiers.toString() + name + ")";
    }
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

class Call : 
public TypedStatement, 
public NamedStatement,
public ContextAwareStatement {
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
    static bool matchArgumentsToParameters(
        const std::vector<std::shared_ptr<Omniscript::FunctionInputExpression>>& args,
        const std::vector<std::shared_ptr<Omniscript::FunctionInputExpression>>& params,
        SymbolTableType scope
    );
    void setInstanceName(const std::string& name) { instanceName = name; }
    std::string formatError(const std::string& msg) const override {
        return (instanceName.empty() ? "" : instanceName + ".") + callee + ": " + msg;
    };
    void markAsConstant() {
        isFromConstantAssignment = true;
    }
    static std::string resolveFunctionOverload(
        const std::string& calleeName,
        const std::vector<std::shared_ptr<Statement>>& args,
        const SymbolTableType& scope
    );
    
        bool isFromAssignment = false;
    private:
        bool isFromConstantAssignment = false;
        std::string callee;
        std::string instanceName;
        std::vector<std::shared_ptr<Statement>> args;
        std::shared_ptr<Statement> expr;
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
    BinaryExpression(std::shared_ptr<Statement> left = std::shared_ptr<Statement>{}, Token op = Token(), std::shared_ptr<Statement> right = std::shared_ptr<Statement>{})
        : left(left), op(op), right(right) {}

    
    // Method to evaluate the binary expression
    std::shared_ptr<Statement> evaluate(SymbolTableType scope) override { return nullptr; }
    std::shared_ptr<Omniscript::Expression> express(SymbolTableType scope) override;
    std::string toString() const override {
        std::string leftStr = left ? left->toString() : "null";
        std::string rightStr = right ? right->toString() : "null";
        std::string opStr = getTokenTypeName(op.getType());
        return "(" + leftStr + " " + opStr + " " + rightStr + ")";
    }
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
    Token op;
    std::shared_ptr<Statement> right;
};

class TernaryExpression : 
public Expression,
public TypedStatement {
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

class ControlFlowStatement :
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
    std::string toString() const override { return "WhileStatement"; }

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
    std::string toString() const override { return "Forloop"; }  
};

class BreakStatement : public ControlFlowStatement {
public:
    std::shared_ptr<Statement> evaluate(SymbolTableType scope) override { return nullptr; }
    std::shared_ptr<Omniscript::Expression> express(SymbolTableType scope) override;
    std::string toString() const override { return "LiteralStatement"; }
};

class ContinueStatement : public ControlFlowStatement {
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

class Access : 
public TypedStatement, 
public Expression, 
public NamedStatement {
protected:
    std::string memberName;
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

    void setMemberName(const std::string& name) {
        memberName = name;
    }

    std::string getMemberName() const {
        return memberName;
    }

    void verifyMemberAccessibility();

    std::string getFullMemberPath() const {
        return memberName;
    }

    std::string toString() const override {
        return "AccessStatement";
    }
};

class MemberAccess : public Access {
private:
    std::string objectName;
    std::shared_ptr<Statement> object;

public:
    MemberAccess(const std::string& obj, const std::string& member, std::shared_ptr<Statement> assignVal = nullptr) {
        this->objectName = obj;
        this->memberName = member;
        this->name = obj;
        setAssignmentValueTo(assignVal);
    }

    MemberAccess(std::shared_ptr<Statement> obj, const std::string& member, std::shared_ptr<Statement> assignVal = nullptr) {
        this->expr = obj;
        this->object = obj;
        this->memberName = member;
        auto named = std::dynamic_pointer_cast<NamedStatement>(obj);
        if (!named) {
            console.error("The object having members should be named");
        } else {
            this->name = named->getName();
        }
        setAssignmentValueTo(assignVal);
    }

    const std::shared_ptr<Statement>& getObject() const { return object; }

    std::shared_ptr<Statement> clone() const override {
        auto cloned = object
            ? std::make_shared<MemberAccess>(expr->clone(), memberName, assignmentValue ? assignmentValue->clone() : nullptr)
            : std::make_shared<MemberAccess>(objectName, memberName, assignmentValue ? assignmentValue->clone() : nullptr);

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
        std::string base = object ? "ObjectMember(" + object->toString() + ")" : objectName;
        base += "." + memberName;

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
    Dereference(std::shared_ptr<Statement> ptr, const std::string& member) : pointer(ptr) {
        memberName = member;
        expr = ptr;
    }

    std::shared_ptr<Statement> clone() const override {
        auto cloned = std::make_shared<Dereference>(pointer->clone(), memberName);
        cloned->assignmentValue = assignmentValue ? assignmentValue->clone() : nullptr;
        for (const auto& arg : arguments) {
            cloned->arguments.push_back(arg->clone());
        }
        cloned->isCall = isCall;
        return cloned;
    }

    std::shared_ptr<Omniscript::Expression> express(SymbolTableType scope) override;

    std::string toString() const override {
        std::string base = "(*" + (pointer ? pointer->toString() : "null") + "." + memberName + ")";

        if (isSetter()) {
            return base + " = " + (assignmentValue ? assignmentValue->toString() : "null");
        } else if (isCall) {
            std::string argsStr = "(";
            for (size_t i = 0; i < arguments.size(); ++i) {
                argsStr += arguments[i] ? arguments[i]->toString() : "null";
                if (i + 1 < arguments.size()) argsStr += ", ";
            }
            argsStr += ")";
            return "Call: " + base + argsStr;
        } else {
            return "Get: " + base;
        }
    }
};

class ArrowAccess : public Access {
private:
    std::shared_ptr<Statement> pointer;

public:
    ArrowAccess(std::shared_ptr<Statement> ptr, const std::string& member) : pointer(ptr) {
        memberName = member;
        expr = ptr;
    }

    std::shared_ptr<Statement> clone() const override {
        auto cloned = std::make_shared<ArrowAccess>(pointer->clone(), memberName);
        cloned->assignmentValue = assignmentValue ? assignmentValue->clone() : nullptr;
        for (const auto& arg : arguments) {
            cloned->arguments.push_back(arg->clone());
        }
        cloned->isCall = isCall;
        return cloned;
    }

    std::shared_ptr<Omniscript::Expression> express(SymbolTableType scope) override;

    std::string toString() const override {
        std::string base = "(" + (pointer ? pointer->toString() : "null") + "->" + memberName + ")";

        if (isSetter()) {
            return base + " = " + (assignmentValue ? assignmentValue->toString() : "null");
        } else if (isCall) {
            std::string argsStr = "(";
            for (size_t i = 0; i < arguments.size(); ++i) {
                argsStr += arguments[i] ? arguments[i]->toString() : "null";
                if (i + 1 < arguments.size()) argsStr += ", ";
            }
            argsStr += ")";
            return "Call: " + base + argsStr;
        } else {
            return "Get: " + base;
        }
    }
};


class IndexAccess : public Access {
public:
    IndexAccess(std::shared_ptr<Statement> expr, std::shared_ptr<Statement> index) {
        this->expr = expr;
        this->index = index;
    }

    std::shared_ptr<Statement> index;

    std::shared_ptr<Statement> clone() const override {
        auto cloned = std::make_shared<IndexAccess>(expr->clone(), index->clone());
        cloned->assignmentValue = assignmentValue ? assignmentValue->clone() : nullptr;
        for (const auto& arg : arguments) {
            cloned->arguments.push_back(arg->clone());
        }
        cloned->isCall = isCall;
        return cloned;
    }
    std::shared_ptr<Omniscript::Expression> express(SymbolTableType scope) override;
    std::string toString() const override {
        std::string base = "(" + (expr ? expr->toString() : "null") + "[" + (index ? index->toString() : "null") + "])";

        if (isSetter()) {
            return base + " = " + (assignmentValue ? assignmentValue->toString() : "null");
        } else if (isCall) {
            std::string argsStr = "(";
            for (size_t i = 0; i < arguments.size(); ++i) {
                argsStr += arguments[i] ? arguments[i]->toString() : "null";
                if (i + 1 < arguments.size()) argsStr += ", ";
            }
            argsStr += ")";
            return "Call: " + base + argsStr;
        } else {
            return "Get: " + base;
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
