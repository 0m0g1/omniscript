#pragma once
#ifndef Expression_H
#define Expression_H

#include <omniscript/engine/tokens.h>
#include <omniscript/Core.h>
#include <omniscript/omniscript_pch.h>
#include <omniscript/debuggingtools/console.h>
#include <omniscript/Core/Types.h>

namespace Omniscript {

// ====================================== Expressions ====================================== //
struct Expression {
public:
    virtual ~Expression() = default;  // Polymorphic base
    
    virtual std::shared_ptr<Expression> clone() const { return nullptr; }
    std::shared_ptr<Type> getType() const { return type; }
    std::shared_ptr<Type> getRootType() const { return rootType; }
    virtual std::string toString() const { return "Expression"; }

    std::string name;
    std::shared_ptr<Type> type = Type::createInvalid();  // Holds a full Type object now
    std::shared_ptr<Type> rootType = Type::createInvalid();  // Holds a full Type object now
};

template <typename T>
std::shared_ptr<T> make_expression(auto&&... args) {
    return std::make_shared<T>(std::forward<decltype(args)>(args)...);
}

struct UndefinedExpression : public Expression {
    UndefinedExpression() {
        type = Type::createUndefined();
        rootType = Type::createUndefined();
    }

    std::string toString() const override { return "Undefined"; }
    // Undefined
    std::shared_ptr<Expression> clone() const override {
        return std::make_shared<UndefinedExpression>();
    }
};

struct TypeExpression : public Expression {
    std::string name;
    std::shared_ptr<Type> actualType;  // This holds the real type being wrapped

    // Constructor from existing Type
    explicit TypeExpression(const std::string& typeName, std::shared_ptr<Type> type)
        : name(typeName), actualType(std::move(type)) {
        this->type = Type::createMetaType();  // Meta-type for this Expression
    }

    // Constructor from Kind (for primitive types)
    explicit TypeExpression(Kind kind)
        : actualType(Type::createPrimitiveType(kind)) {
        this->type = Type::createMetaType();
    }

    std::string toString() const override {
        return actualType ? actualType->getName() : "nulltype";
    }

    std::shared_ptr<Type> getTypeExpression() const {
        return actualType;
    }

    bool isSameTypeAs(const std::shared_ptr<TypeExpression>& other) const {
        if (!actualType || !other->actualType) return false;
        return actualType->getKind() == other->actualType->getKind();
    }

    // TypeExpression
    std::shared_ptr<Expression> clone() const override {
        return std::make_shared<TypeExpression>(name, actualType ? actualType->clone() : nullptr);
    }
};

// Template class for Primitive Types (e.g., Int8, Bool)
template <typename T>
struct Primitive : public Expression {
    explicit Primitive(T value) : value(value) {
        type = Type::createPrimitiveType(PrimitiveType::get<T>());
    }

    std::string toString() const override {
        if constexpr (std::is_same_v<T, std::string>) {
            if (PrimitiveType::get<T>() != Kind::Utf8)
                return "Primitive: " + value;
            else
                return "String: \"" + this->getValue() + "\"";
        } else if constexpr (std::is_same_v<T, std::u16string>) {
            return "UTF-16 String";
        } else if constexpr (std::is_same_v<T, std::u32string>) {
            return "UTF-32 String";
        } else if constexpr (std::is_same_v<T, bool>) {
            return std::string("Primitive: ") + (value ? "true" : "false");
        } else if constexpr (std::is_same_v<T, __float128>) {
            char buffer[128];
            snprintf(buffer, sizeof(buffer), "%.*Lf", 36, (long double)value);
            return std::string("Primitive: ") + buffer;
        } else if constexpr (std::is_same_v<T, _Float16>) {
            return "Primitive: (Float16 not yet printable)";
        } else {
            return "Primitive: " + std::to_string(value); // works for int, float, etc.
        }
    }

    T getValue() const { return value; }

    // Primitive (template)
    std::shared_ptr<Expression> clone() const override {
        return std::make_shared<Primitive<T>>(value);
    }

protected:
    T value;
};


// Base class for all numeric values
template <typename T>
class NumericExpression : public Primitive<T> {
public:
    NumericExpression(T value)
        : Primitive<T>(value) {}

    virtual ~NumericExpression() = default;
    // NumericExpression (template)
    std::shared_ptr<Expression> clone() const override {
        return std::make_shared<NumericExpression<T>>(this->value);
    }
};

// Specialized Integer template class inheriting from NumericExpression
template <typename T>
class Integer : public NumericExpression<T> {
public:
    Integer(T value)
        : NumericExpression<T>(value) {
            this->rootType = std::make_shared<Type>(Kind::Int8);
        }  // Specify type

    ~Integer() override = default;
    // Integer (template)
    std::shared_ptr<Expression> clone() const override {
        return std::make_shared<Integer<T>>(this->value);
    }
};

// Specialized Float template class inheriting from NumericExpression
template <typename T>
class Float : public NumericExpression<T> {
public:
    Float(T value)
        : NumericExpression<T>(value) {
            this->rootType = std::make_shared<Type>(Kind::Half);
        }  // Specify type

    ~Float() override = default;
        // Float (template)
    std::shared_ptr<Expression> clone() const override {
        return std::make_shared<Float<T>>(this->value);
    }
};

// Specialized BigInt class inheriting from NumericExpression for handling large integers
class BigInt : public NumericExpression<std::string> {
public:
    explicit BigInt(std::string value, unsigned bitWidth)
        : NumericExpression<std::string>(value), bitWidth(bitWidth) {
            rootType = Type::createPrimitiveType(Kind::Int8);
        }  // Pass value to base class constructor

    ~BigInt() override = default;

    unsigned getBitWidth() const { return bitWidth; }

    // BigInt
    std::shared_ptr<Expression> clone() const override {
        return std::make_shared<BigInt>(this->value, bitWidth);
    }

private:
    unsigned bitWidth;
};


// Pointer Types
struct PointerExpression : public Expression {
    std::shared_ptr<Expression> pointee;
    bool isConst;
    bool isVolatile;

    PointerExpression(std::shared_ptr<Expression> pointee, bool isConst = false, bool isVolatile = false)
        : pointee(std::move(pointee)), isConst(isConst), isVolatile(isVolatile) {
        type = Type::createPointerType(this->pointee->type);
        rootType = std::make_shared<Type>(Kind::Pointer);
    }

    std::string toString() const override { return "Pointer"; } 

    // PointerExpression
    std::shared_ptr<Expression> clone() const override {
        return std::make_shared<PointerExpression>(
            pointee ? pointee->clone() : nullptr,
            isConst,
            isVolatile
        );
    }
};

struct InvalidExpression : public Expression {
    InvalidExpression() {
        type = Type::createInvalid();
        rootType = std::make_shared<Type>(Kind::Invalid);
    }

    std::string toString() const override { return "Invalid"; }
    // NullExpression
    std::shared_ptr<Expression> clone() const override {
        return std::make_shared<InvalidExpression>();
    }
};

struct NullExpression : public Expression {
    std::shared_ptr<Type> expectedType;
    NullExpression(std::shared_ptr<Type> expectedType = nullptr) : expectedType(expectedType) {
        type = Type::createNullType();
        rootType = Type::createNullType();
    }
    
    std::string toString() const override { return "NullPointer"; }
    // NullExpression
    std::shared_ptr<Expression> clone() const override {
        return std::make_shared<NullExpression>();
    }
};

struct NullPointerExpression : public Expression {
    std::shared_ptr<Type> expectedType;
    NullPointerExpression(std::shared_ptr<Type> expectedType = nullptr) : expectedType(std::move(expectedType)) {
        type = Type::createNullPointerType();
        rootType = Type::createNullPointerType();
    }

    std::string toString() const override { return "NullPointer"; } 
    // NullPointerExpression
    std::shared_ptr<Expression> clone() const override {
        return std::make_shared<NullPointerExpression>();
    }
};

struct AddressOfExpression : public Expression {
    std::shared_ptr<Expression> referent;  // The variable whose address is being stored
    std::string variableName;

    explicit AddressOfExpression(const std::string& variableName, std::shared_ptr<Expression> referent = nullptr)
        : variableName(variableName), referent(std::move(referent)) {
        // We assume AddressOf value is of type Pointer to the referent's type
        type = Type::createPointerType(this->referent->type); 
    }

    // Return a string representation of the AddressOf value
    std::string toString() const override {
        return "AddressOf(" + referent->toString() + ")";
    }
    // AddressOfExpression
    std::shared_ptr<Expression> clone() const override {
        return std::make_shared<AddressOfExpression>(
            variableName,
            referent ? referent->clone() : nullptr
        );
    }
};

// Reference Types
struct ReferenceExpression : public Expression {
    std::string referentName;
    std::shared_ptr<Expression>* referentPtr = nullptr;  // Pointer to a reference
    std::shared_ptr<Expression> referent = nullptr;     // Pointer to a value (for regular pointers)

    // Constructor for pointers (original)
    explicit ReferenceExpression(const std::string& referentName, std::shared_ptr<Expression> referent = nullptr)
        : referentName(referentName), referent(referent) {
        type = Type::createReferenceType(this->referent->type);
    }

    // Constructor for references (using a reference pointer)
    explicit ReferenceExpression(const std::string& name, std::shared_ptr<Expression>* referentPtr)
        : referentName(name), referentPtr(referentPtr) {
        if (referentPtr && *referentPtr) {
            type = Type::createReferenceType((*referentPtr)->type);
        }
    }

    // Getter for value, works for both pointers and references
    std::shared_ptr<Expression> getValue() const {
        if (referent) {
            return referent;  // Regular pointer, just return the referent
        }
        return (referentPtr && *referentPtr) ? *referentPtr : nullptr;  // Dereference reference pointer
    }

    // String representation for both pointers and references
    std::string toString() const override {
        if (referent) {
            return "Pointer to(" + referent->toString() + ")";
        }
        return "Reference to(" + (referentPtr && *referentPtr ? (*referentPtr)->toString() : "null") + ")";
    }

    // ReferenceExpression
    std::shared_ptr<Expression> clone() const override {
        if (referent) {
            return std::make_shared<ReferenceExpression>(
                referentName,
                referent->clone()
            );
        }
        return std::make_shared<ReferenceExpression>(
            referentName,
            referentPtr ? new std::shared_ptr<Expression>(*referentPtr) : nullptr
        );
    }
};


// Function Types
struct ReturnExpression : public Expression {
    std::shared_ptr<Expression> value;
    ReturnExpression(std::shared_ptr<Expression> value, std::shared_ptr<Type> returnType) : value(std::move(value)) {
        type = returnType;
    }
    // ReturnExpression
    std::shared_ptr<Expression> clone() const override {
        return std::make_shared<ReturnExpression>(
            value ? value->clone() : nullptr,
            type ? type->clone() : nullptr
        );
    }
};

struct BinaryExpression : public Expression {
    std::shared_ptr<Expression> left;
    std::shared_ptr<Expression> right;
    TokenTypes op;

    BinaryExpression(std::shared_ptr<Expression> lhs, TokenTypes op, std::shared_ptr<Expression> rhs, std::shared_ptr<Type> resultType)
        : left(std::move(lhs)), right(std::move(rhs)), op(std::move(op)) {
        this->type = resultType;
    }

    std::string toString() const override {
        // return "(" + left->toString() + " " + op + " " + right->toString() + ")";
        return "(" + left->toString() + " op " + right->toString() + ")";
        // return "(bin expr)";
    }
    // BinaryExpression
    std::shared_ptr<Expression> clone() const override {
        return std::make_shared<BinaryExpression>(
            left ? left->clone() : nullptr,
            op,
            right ? right->clone() : nullptr,
            type ? type->clone() : nullptr
        );
    }
};

struct TernaryExpression : public Expression {
    std::shared_ptr<Expression> condition;
    std::shared_ptr<Expression> truthy;
    std::shared_ptr<Expression> falsey;

    TernaryExpression(std::shared_ptr<Expression> cond,
                            std::shared_ptr<Expression> ifTrue,
                            std::shared_ptr<Expression> ifFalse,
                            std::shared_ptr<Type> resultType)
        : condition(std::move(cond)), truthy(std::move(ifTrue)), falsey(std::move(ifFalse)) {
        this->type = resultType;
    }

    std::string toString() const override {
        return "(" + condition->toString() + " ? " + truthy->toString() + " : " + falsey->toString() + ")";
    }

    // TernaryExpression
    std::shared_ptr<Expression> clone() const override {
        return std::make_shared<TernaryExpression>(
            condition ? condition->clone() : nullptr,
            truthy ? truthy->clone() : nullptr,
            falsey ? falsey->clone() : nullptr,
            type ? type->clone() : nullptr
        );
    }
};


struct UnaryExpression : public Expression {
    TokenTypes op;
    std::shared_ptr<Expression> operand;
    bool position;

    UnaryExpression(TokenTypes op,
                            std::shared_ptr<Expression> operand,
                            std::shared_ptr<Type> resultType,
                            bool position)
        : op(op), operand(std::move(operand)), position(position) {
        this->type = resultType;
    }

    std::string toString() const override {
        TokenTypes opStr = op;
        return "(unaryexpr)";
        // return (position)
        //     ? (opStr + operand->toString())
        //     : (operand->toString() + opStr);
    }

    // UnaryExpression
    std::shared_ptr<Expression> clone() const override {
        return std::make_shared<UnaryExpression>(
            op,
            operand ? operand->clone() : nullptr,
            type ? type->clone() : nullptr,
            position
        );
    }
};


//params & args
struct FunctionInputExpression : public Expression {
    bool isConstant = false;
    std::shared_ptr<Expression> value;

    FunctionInputExpression(const std::string& name, std::shared_ptr<Type> type = nullptr, std::shared_ptr<Expression> value = nullptr, bool isConst = false) :
    value(std::move(value)), isConstant(isConst) {
        this->name = name;
        this->type = std::move(type);
    }
    
    std::string toString() const override { return "(FunctionInput: " + name + ", value: " + value->toString() + ")"; } 

    std::shared_ptr<Expression> clone() const override {
        return std::make_shared<FunctionInputExpression>(
            name,
            type ? type->clone() : nullptr,
            value ? value->clone() : nullptr,
            isConstant
        );
    }
};

struct CallExpression : public Expression {
    std::string calleeName;
    std::string instanceName;
    std::vector<std::shared_ptr<Expression>> args;
    bool isGlobal;

    CallExpression(const std::string& calleeName, const std::vector<std::shared_ptr<Expression>>& args = {}, std::shared_ptr<Type> returnType = nullptr)
    : calleeName(calleeName), args(std::move(args)) {
        type = std::move(returnType);
    }

    CallExpression(const std::string& objectName,
        const std::string& instanceName,
        const std::vector<std::shared_ptr<Expression>>& args = {},
        std::shared_ptr<Type> returnType = nullptr,
        bool isGlobal = true
    )
    : calleeName(objectName), instanceName(instanceName), args(std::move(args)), isGlobal(isGlobal) {
        type = std::move(returnType);
    }

    // CallExpression
    std::shared_ptr<Expression> clone() const override {
        std::vector<std::shared_ptr<Expression>> clonedArgs;
        for (const auto& arg : args) {
            clonedArgs.push_back(arg ? arg->clone() : nullptr);
        }
        return std::make_shared<CallExpression>(
            calleeName,
            instanceName,
            clonedArgs,
            type ? type->clone() : nullptr
        );
    }
    std::string toString() const override {
        if (instanceName.empty()) {
            return "Call: " + calleeName;
        }
        return "Call create instance '" + instanceName + "' of object '" + calleeName + "'.";
    }
};

struct Callable : public Expression {
    std::string mangledName;
    std::vector<std::shared_ptr<Expression>> parameters;
    bool isVarArg;

    Callable(
            const std::string& name,
            const std::string& mangledName,
            std::vector<std::shared_ptr<Expression>> params = {},
            bool isVarArg = false
        )
        : parameters(std::move(params)), 
            isVarArg(isVarArg) {
        this->name = name;
        this->mangledName = mangledName;
    }

    // Helper method to clone parameters
    std::vector<std::shared_ptr<FunctionInputExpression>> cloneParameters() const {
        std::vector<std::shared_ptr<FunctionInputExpression>> clonedParams;
        for (const auto& parameter : parameters) {
            auto param = std::dynamic_pointer_cast<FunctionInputExpression>(parameter);
            clonedParams.push_back(std::make_shared<FunctionInputExpression>(
                param->name,
                param->type,
                param->value ? param->value->clone() : nullptr,
                param->isConstant
            ));
        }
        return clonedParams;
    }

    std::vector<std::shared_ptr<Expression>> getParameters() const {
        return parameters;
    }

    std::string toString() const override {
        std::string paramsStr;
        for (const auto& param : parameters) {
            if (!paramsStr.empty()) paramsStr += ", ";
            paramsStr += param->toString();
        }
        return "Callable: " + name + "(" + paramsStr + ")";
    }

    // Callable
    std::shared_ptr<Expression> clone() const override {
        std::vector<std::shared_ptr<Expression>> clonedParams;
        for (const auto& param : parameters) {
            clonedParams.push_back(param ? param->clone() : nullptr);
        }
        return std::make_shared<Callable>(name, mangledName, clonedParams, isVarArg);
    }
};

// FunctionExpression inherits from Callable
struct FunctionExpression : public Callable {
    std::vector<std::shared_ptr<Expression>> body;
    std::shared_ptr<Type> returnType;

    FunctionExpression(
                        const std::string& name, 
                        const std::string& mangledName, 
                        std::shared_ptr<Type> returnType,
                        std::vector<std::shared_ptr<Expression>> body,
                        std::vector<std::shared_ptr<Expression>> params = {},
                        bool isVarArg = false)
        : Callable(name, mangledName, std::move(params), isVarArg),
            body(std::move(body)),
            returnType(returnType) {
        type = Type::createFunctionType(returnType, isVarArg);
        returnType = type->getReturnType();
    }

    std::string toString() const override {
        return "Function: " + name + " [Returns: " + (returnType ? returnType->kindName() : "void") + "]";
    }

    std::shared_ptr<Type> getReturnType() {
        return type->getReturnType();
    }

    // FunctionExpression
    std::shared_ptr<Expression> clone() const override {
        std::vector<std::shared_ptr<Expression>> clonedBody;
        for (const auto& expr : body) {
            clonedBody.push_back(expr ? expr->clone() : nullptr);
        }
        
        std::vector<std::shared_ptr<Expression>> clonedParams;
            for (const auto& param : parameters) {
                clonedParams.push_back(param ? param->clone() : nullptr);
            }
            
            return std::make_shared<FunctionExpression>(
                name,
                mangledName,
                returnType ? returnType->clone() : nullptr,
                clonedBody,
                clonedParams,
                isVarArg
            );
        }
};

struct BlockExpression : public Expression {
    std::vector<std::shared_ptr<Expression>> values;  // Store multiple values in a vector

    BlockExpression(std::vector<std::shared_ptr<Expression>> values)
        : values(std::move(values)) {
        type = Type::createInvalid();
    }

    std::string toString() const override {
        std::string result = "Block: [ ";
        for (const auto& val : values) {
            result += val ? val->toString() : "null";
            result += " ";
        }
        result += "]";
        return result;
    }

    // BlockExpression
    std::shared_ptr<Expression> clone() const override {
        std::vector<std::shared_ptr<Expression>> clonedValues;
        for (const auto& val : values) {
            clonedValues.push_back(val ? val->clone() : nullptr);
        }
        return std::make_shared<BlockExpression>(clonedValues);
    }
};

// Aggregate Types (e.g., Struct, Enum, Array)
struct AggregateExpression : public Expression {
    std::vector<std::shared_ptr<Expression>> elements;
    std::vector<std::string> elementNames;
    uint64_t count;

    AggregateExpression(std::vector<std::shared_ptr<Expression>> elements, std::vector<std::string> elementNames)
        : elements(std::move(elements)), elementNames(std::move(elementNames)), count(elements.size()) {
        std::vector<std::shared_ptr<Type>> elementTypes;
        for (auto& e : this->elements)
            elementTypes.push_back(e->type);

        // type = Type::createStructType(elementTypes, this->elementNames);
    }

    std::string toString() const override { return "Aggregate"; }

    // AggregateExpression
    std::shared_ptr<Expression> clone() const override {
        std::vector<std::shared_ptr<Expression>> clonedElements;
        for (const auto& elem : elements) {
            clonedElements.push_back(elem ? elem->clone() : nullptr);
        }
        return std::make_shared<AggregateExpression>(
            clonedElements,
            elementNames
        );
    }
};

struct StructExpression : public Callable {
    std::string structName;
    std::vector<std::string> elementNames;

    StructExpression(
        const std::string& structName,
        const std::string& mangledName,
        std::vector<std::shared_ptr<Expression>> fields,
        std::vector<std::string> fieldNames = {},
        bool isVarArg = false
    )
        : Callable(structName, mangledName, fields, isVarArg),
          structName(structName),
          elementNames(std::move(fieldNames))
    {
        // Define type as struct
        std::vector<std::shared_ptr<Type>> fieldTypes;
        for (auto& f : parameters)
            fieldTypes.push_back(f->type);

        type = std::make_shared<UserDefinedType>(name);
    }

    std::string toString() const override {
        return "Struct : " + structName;
    }

    std::shared_ptr<Expression> clone() const override {
        std::vector<std::shared_ptr<Expression>> clonedFields;
        for (const auto& field : parameters)
            clonedFields.push_back(field ? field->clone() : nullptr);

        return std::make_shared<StructExpression>(
            structName,
            mangledName,
            clonedFields,
            elementNames,
            isVarArg
        );
    }
};

class InstanceExpression : public Expression {
public:
    std::string baseName;
    std::string instanceName;
    std::shared_ptr<Type> instanceType;

    std::vector<std::shared_ptr<Expression>> constructorArgs;
    std::vector<std::shared_ptr<Expression>> publicMembers;
    std::vector<std::shared_ptr<Expression>> privateMembers;

    InstanceExpression(
        const std::string& baseName,
        const std::string& instanceName,
        std::vector<std::shared_ptr<Expression>> args = {},
        std::vector<std::shared_ptr<Expression>> publicMembers = {},
        std::vector<std::shared_ptr<Expression>> privateMembers = {}
    ) : baseName(baseName),
        instanceName(instanceName),
        constructorArgs(std::move(args)),
        publicMembers(std::move(publicMembers)),
        privateMembers(std::move(privateMembers)) {

        this->instanceType = std::make_shared<UserDefinedType>(baseName);
        this->type = instanceType; // inherited from Expression
    }

    std::string toString() const override {
        return "Instance<" + baseName + "> named " + instanceName;
    }

    std::shared_ptr<Expression> clone() const override {
        std::vector<std::shared_ptr<Expression>> clonedArgs;
        for (const auto& arg : constructorArgs) {
            clonedArgs.push_back(arg->clone());
        }

        std::vector<std::shared_ptr<Expression>> clonedPublic;
        for (const auto& pub : publicMembers) {
            clonedPublic.push_back(pub->clone());
        }

        std::vector<std::shared_ptr<Expression>> clonedPrivate;
        for (const auto& priv : privateMembers) {
            clonedPrivate.push_back(priv->clone());
        }

        return std::make_shared<InstanceExpression>(
            baseName,
            instanceName,
            clonedArgs,
            clonedPublic,
            clonedPrivate
        );
    }
};

struct EnumExpression : public Expression {
    EnumExpression(const std::string& enumName, bool hasLookup = false, bool isEnumClass = false)
        : enumName(enumName), hasLookup(hasLookup), isEnumClass(isEnumClass) {
        name = enumName;
    }

    // Add entry for int -> expression mapping
    void addEntry(int value, const std::string& valueName, std::shared_ptr<Expression> expression) {
        enumerators[valueName] = value;
        expressionMap[valueName] = expression;  // Store the corresponding expression
    }

    // Add entry for value name -> expression mapping
    void addEntry(const std::string& valueName, std::shared_ptr<Expression> expression) {
        enumerators[valueName] = -1; // or some default int for non-int entries
        expressionMap[valueName] = expression;
    }

    // Retrieve the integer value associated with the enum name
    int get(const std::string& enumeration) const {
        auto it = enumerators.find(enumeration);
        if (it != enumerators.end()) return it->second;
        console.error("Enum '" + enumName + "' does not have an entry " + enumeration);
        return -9999999;
    }

    // Retrieve the string name for a given integer value
    std::string getName(int value) const {
        for (const auto& [name, val] : enumerators)
            if (val == value) return name;
        return "";
    }

    // Retrieve the expression associated with a given name
    std::shared_ptr<Expression> getExpression(const std::string& valueName) const {
        auto it = expressionMap.find(valueName);
        if (it != expressionMap.end()) {
            return it->second;
        }
        console.error("Enum expression for '" + enumName + "' does not have an entry " + valueName);
        return nullptr;
    }

    // Clone this enum expression
    std::shared_ptr<Expression> clone() const override {
        auto copy = std::make_shared<EnumExpression>(enumName, hasLookup, isEnumClass);
        copy->enumerators = enumerators;
        copy->expressionMap = expressionMap;  // Also clone the expression map
        return copy;
    }

    // Convert the enum to a string representation
    std::string toString() const override {
        return (isEnumClass ? "enum class " : "enum ") + enumName;
    }

    // Member variables
    std::string enumName;
    bool hasLookup = false;
    bool isEnumClass = false;

    // Maps for the enum entries
    std::unordered_map<std::string, int> enumerators;  // value name -> value
    std::unordered_map<std::string, std::shared_ptr<Expression>> expressionMap;  // value name -> expression
};


// Custom String and WideString Types
template <typename T>
class StringExpression : public Primitive<T> {
public:
    StringExpression(T value)
        : Primitive<T>(value) {
            this->rootType = std::make_shared<Type>(Kind::String);
        }

    virtual ~StringExpression() = default;
    // StringExpression (template)
    std::shared_ptr<Expression> clone() const override {
        return std::make_shared<StringExpression<T>>(this->value);
    }
};

struct VariableAssignment : public Expression {
    bool isGlobal;
    std::string variableName;
    std::shared_ptr<Expression> assignedValue;

    VariableAssignment(std::string name, std::shared_ptr<Expression> value, bool isGlobal = false)
        : variableName(std::move(name)), assignedValue(std::move(value)), isGlobal(isGlobal) {
        type = assignedValue->type;  // Same type as the assigned value
    }

    std::shared_ptr<Expression> getValue() const { return assignedValue; }
    std::string toString() const override {
        return "Assign: " + variableName + " = " + assignedValue->toString();
    }
    // VariableAssignment
    std::shared_ptr<Expression> clone() const override {
        return std::make_shared<VariableAssignment>(
            variableName,
            assignedValue ? assignedValue->clone() : nullptr
        );
    }
};

struct VariableAccess : public Expression {
    std::string variableName;
    std::shared_ptr<Type> type;

    explicit VariableAccess(std::string name, std::shared_ptr<Type> type = nullptr) 
        : variableName(std::move(name)), type(type) {}

    std::string toString() const override {
        return "Variable: " + variableName;
    }
    // VariableAccess
    std::shared_ptr<Expression> clone() const override {
        return std::make_shared<VariableAccess>(variableName, type ? type->clone() : nullptr);
    }
};

struct ArrayExpression : public Expression {
    std::vector<std::shared_ptr<Expression>> elements;

    explicit ArrayExpression(std::shared_ptr<Type> type, std::vector<std::shared_ptr<Expression>> elements)
        : elements(std::move(elements)) {
        this->type = std::move(type);
        rootType = std::make_shared<Type>(Kind::Array);
    }

    std::string toString() const override {
        std::string s = "[";
        for (size_t i = 0; i < elements.size(); ++i) {
            s += elements[i] ? elements[i]->toString() : "null";
            if (i + 1 < elements.size()) s += ", ";
        }
        return s + "]";
    }

    void push(std::shared_ptr<Expression> val) {
        elements.push_back(std::move(val));
    }

    std::shared_ptr<Expression> get(size_t index) const {
        return index < elements.size() ? elements[index] : nullptr;
    }

    const std::vector<std::shared_ptr<Expression>>& getElements() const {
        return elements;
    }

    // ArrayExpression
    std::shared_ptr<Expression> clone() const override {
        std::vector<std::shared_ptr<Expression>> clonedElements;
        for (const auto& elem : elements) {
            clonedElements.push_back(elem ? elem->clone() : nullptr);
        }
        return std::make_shared<ArrayExpression>(
            type ? type->clone() : nullptr,
            clonedElements
        );
    }
};

class FixedArrayExpression : public Expression {
public:
    std::vector<std::shared_ptr<Expression>> elements;
    std::shared_ptr<Type> elementType;

    FixedArrayExpression(std::vector<std::shared_ptr<Expression>> elems, std::shared_ptr<Type> elemType)
        : elements(std::move(elems)), elementType(std::move(elemType)) {
            type = Type::createFixedArrayType(elementType, elements.size());
        }

    std::string typeName() const {
        return "FixedArray<" + (elementType ? elementType->kindName() : "unknown") + ">";
    }

    std::string toString() const override {
        std::string result = "[";
        for (size_t i = 0; i < elements.size(); ++i) {
            result += elements[i]->toString();
            if (i < elements.size() - 1) result += ", ";
        }
        return result + "]";
    }
    // FixedArrayExpression
    std::shared_ptr<Expression> clone() const override {
        std::vector<std::shared_ptr<Expression>> clonedElements;
        for (const auto& elem : elements) {
            clonedElements.push_back(elem ? elem->clone() : nullptr);
        }
        return std::make_shared<FixedArrayExpression>(
            clonedElements,
            elementType ? elementType->clone() : nullptr
        );
    }
};

}

#endif