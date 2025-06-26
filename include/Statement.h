#pragma once

#include <omniscript/Core/Core.h>
#include <omniscript/utils.h>
#include <omniscript/Core/Types.h>
#include <omniscript/tokens.h>
#include <omniscript/omniscript_pch.h>
#include <omniscript/runtime/object.h>
#include <omniscript/Core/Expression.h>
#include <omniscript/Symboltable.h>
#include <omniscript/debuggingtools/console.h>
#include <omniscript/Core/Expressions/FunctionInputExpression.h>

using SymbolTableType = std::shared_ptr<SymbolTable<std::shared_ptr<Omniscript::Expression>, std::shared_ptr<Omniscript::Type>>>;

class Statement {
    public:
        // enum Type { // implement a statement type for each statement for speed
        //     Value,
        //     Assignment,
        //     ConstantAssignment,
        //     Return,
        // }
    
        ~Statement() = default;

        virtual std::shared_ptr<Statement> clone() const { return nullptr; }

        virtual std::shared_ptr<Statement> evaluate(SymbolTableType scope) { return nullptr; }
        virtual std::shared_ptr<Omniscript::Expression> express(SymbolTableType scope) { return nullptr; }
        virtual std::string toString() const { return "Statement"; }

        inline void setPosition(Token startToken) {
            pos.line = startToken.getLine();
            pos.col = startToken.getColumn();
            pos.fileName = startToken.getFilePath();
            pos.filePath = startToken.getFilePath();
        }

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
    virtual void setName(const std::string& newName) { name = newName; }
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
    
class ScopedStatement: public virtual TypedStatement {
public:
    ~ScopedStatement() = default;
    virtual std::string toString() const override { return "ScopedStatement"; }
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

    std::vector<std::string> copyContext() const {
        return std::vector<std::string>(accessContext);
    }

    std::vector<std::string> createChildContext() const {
        return copyContext();
    }

    void extendContextOf(std::shared_ptr<Statement> statement) const {
        auto ctxStmt = std::dynamic_pointer_cast<ContextAwareStatement>(statement);
        
        if (!ctxStmt) {
            return;
        }

        auto newContext = this->copyContext();   
        const auto& existingContext = ctxStmt->getAccessContext();
        newContext.insert(newContext.end(), existingContext.begin(), existingContext.end());
        
        ctxStmt->setAccessContext(newContext);
    }

    bool containsContext(const std::string& contextName) const {
        return std::find(accessContext.begin(), accessContext.end(), contextName) 
               != accessContext.end();
    }

    template<typename Comparator>
    bool containsContext(const std::string& contextName, Comparator comp) const {
        return std::find_if(accessContext.begin(), accessContext.end(),
            [&](const std::string& ctx){ return comp(ctx, contextName); }) 
            != accessContext.end();
    }

    template<typename Predicate>
    bool containsContext(Predicate pred) const {
        return std::any_of(accessContext.begin(), accessContext.end(), pred);
    }

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

    void validateAccessiblity(std::string baseTypeName, std::string memberName, SymbolTableType scope);
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

    virtual bool hasSideEffects() override { return false; }

    virtual bool isCompileTimeEvaluatable() override { return true; }
    virtual std::shared_ptr<Literal> castTo(std::shared_ptr<Omniscript::Type> targetType) const { return nullptr; }
};

class GenericHolder {
public:
    std::vector<std::shared_ptr<Statement>> body;
    std::vector<std::string> typeParams;

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
            DEBUG_LOG("Creating generic '" + name + "' of kind '" + typePtr->toString() + "'.");
            result.emplace_back(std::make_shared<Omniscript::TypeExpression>(name, typePtr));
        }
    
        return result;
    }
    
    virtual ~GenericHolder() = default;
};

class BlockStatement : 
public ScopedStatement, 
public GenericHolder,
public ContextAwareStatement {
public:
    bool isGlobal = true;
    std::vector<std::shared_ptr<Statement>> statements;
    
    BlockStatement(std::vector<std::shared_ptr<Statement>> statements = {})
        : GenericHolder(statements), statements(std::move(statements)) {}

    static std::shared_ptr<BlockStatement> create() {
        return std::make_shared<BlockStatement>();
    }

    static std::shared_ptr<BlockStatement> create(std::vector<std::shared_ptr<Statement>> statements) {
        return std::make_shared<BlockStatement>(std::move(statements));
    }

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
    std::string formatError(const std::string& msg) const override {
        return "Error in code block.\n" + msg;
    };

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
        return "((" + targetType->toString() + ") " + value->toString() + ")";
    }

    std::shared_ptr<Statement> clone() const override {
        return std::make_shared<Cast>(value->clone(), type);  // Clone using copy constructor
    }
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

