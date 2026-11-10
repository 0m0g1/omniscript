#pragma once
#include <omniscript/Expressions/Expression.h>

namespace Omniscript {
template <typename T>
struct Primitive : public Expression {
    explicit Primitive(T value) : value(value) {
        type = Type::createPrimitiveType(PrimitiveType::get<T>());

        if constexpr (std::is_same_v<T, std::string> ||
                    std::is_same_v<T, std::u16string> ||
                    std::is_same_v<T, std::u32string>
                ) {
            auto charType = Type::createPrimitiveType(Kind::Char);
            auto stringType = Type::createPointerType(this->type);
            this->type = stringType;
            this->rootType = Type::createPointerType(charType);
        }
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
            return "Primitive: " + std::to_string(value); 
        }
    }

    T getValue() const { return value; }

    
    std::shared_ptr<Expression> clone() const override {
        return std::make_shared<Primitive<T>>(value);
    }

    T value;
};

template <typename T>
class NumericExpression : public Primitive<T> {
public:
    NumericExpression(T value)
        : Primitive<T>(value) {}

    virtual ~NumericExpression() = default;
    
    std::shared_ptr<Expression> clone() const override {
        return std::make_shared<NumericExpression<T>>(this->value);
    }
};

template <typename T>
class Integer : public NumericExpression<T> {
public:
    Integer(T value)
        : NumericExpression<T>(value) {
            this->rootType = std::make_shared<Type>(Kind::Int8);
        }  

    ~Integer() override = default;
    
    std::shared_ptr<Expression> clone() const override {
        return std::make_shared<Integer<T>>(this->value);
    }
};

template <typename T>
class Float : public NumericExpression<T> {
public:
    Float(T value)
        : NumericExpression<T>(value) {
            this->rootType = std::make_shared<Type>(Kind::Half);
        }  

    ~Float() override = default;
        
    std::shared_ptr<Expression> clone() const override {
        return std::make_shared<Float<T>>(this->value);
    }
};

class BigInt : public NumericExpression<std::string> {
public:
    explicit BigInt(std::string value, unsigned bitWidth)
        : NumericExpression<std::string>(value), bitWidth(bitWidth) {
            rootType = Type::createPrimitiveType(Kind::Int8);
        }  

    ~BigInt() override = default;

    unsigned getBitWidth() const { return bitWidth; }

    
    std::shared_ptr<Expression> clone() const override {
        return std::make_shared<BigInt>(this->value, bitWidth);
    }

private:
    unsigned bitWidth;
};

struct PointerExpression : public Expression {
    std::shared_ptr<Expression> pointee;
    bool isConst;
    bool isVolatile;

    PointerExpression(std::shared_ptr<Expression> pointee, bool isConst = false, bool isVolatile = false)
        : pointee(pointee), isConst(isConst), isVolatile(isVolatile) {
        type = Type::createPointerType(this->pointee->type);
        rootType = std::make_shared<Type>(Kind::Pointer);
    }

    std::string toString() const override { return "Pointer"; } 

    
    std::shared_ptr<Expression> clone() const override {
        return std::make_shared<PointerExpression>(
            pointee ? pointee->clone() : nullptr,
            isConst,
            isVolatile
        );
    }
};

struct RawPointerExpression : public Expression {
    size_t address;
    
    RawPointerExpression(size_t addr, std::shared_ptr<Type> type)
        : address(addr) {
        this->type = type;
        this->rootType = type;
    }

    std::string toString() const override {
        return "RawPointer(" + std::to_string(address) + ")";
    }

    std::shared_ptr<Expression> clone() const override {
        return std::make_shared<RawPointerExpression>(address, type);
    }
};

struct InvalidExpression : public Expression {
    InvalidExpression() {
        type = Type::createInvalid();
        rootType = std::make_shared<Type>(Kind::Invalid);
    }

    std::string toString() const override { return "Invalid"; }
    
    std::shared_ptr<Expression> clone() const override {
        return std::make_shared<InvalidExpression>();
    }
};

struct NullExpression : public Expression {
    bool nullCaseHandled = false;
    bool extractValue = true;
    std::shared_ptr<Type> expectedType;
    NullExpression(std::shared_ptr<Type> expectedType = nullptr) : expectedType(expectedType) {
        type = expectedType;
        rootType = Type::createNullType();
    }
    
    std::string toString() const override { return "Null expects " + type->toString(); }
    
    std::shared_ptr<Expression> clone() const override {
        return std::make_shared<NullExpression>(expectedType ? expectedType : nullptr);
    }
};

struct NullPointerExpression : public Expression {
    bool nullCaseHandled = false;
    bool extractValue = true;
    std::shared_ptr<Type> expectedType;
    NullPointerExpression(std::shared_ptr<Type> expectedType = nullptr) : expectedType(expectedType) {
        type = Type::createPointerType(expectedType);
        rootType = Type::createNullPointerType();
    }

    std::string toString() const override { return "NullPointer expects " + type->toString(); } 
    
    std::shared_ptr<Expression> clone() const override {
        return std::make_shared<NullPointerExpression>(expectedType ? expectedType : nullptr);
    }
};

struct NullableExpression : public Expression {
    bool nullCaseHandled = false;
    bool extractValue = true;
    std::shared_ptr<Expression> inner;

    NullableExpression(std::shared_ptr<Expression> expr = nullptr)
        : inner(expr) {
        if (inner) {
            type = type->createNullableType(inner->getType());
            rootType = type;
        } else {
            type = Type::createUndefined();
            rootType = Type::createUndefined();
        }
    }

    std::string toString() const override {
        return inner ? "Nullable(" + inner->toString() + ")" : "Nullable(null)";
    }

    std::shared_ptr<Expression> clone() const override {
        auto clone = std::make_shared<NullableExpression>(inner ? inner->clone() : nullptr);
        clone->nullCaseHandled = nullCaseHandled;
        return clone;
    }

    bool isNull() const { return !inner; }
    std::shared_ptr<Expression> get() const {
        if (!inner || !nullCaseHandled) {
            throw std::runtime_error("Attempted to unwrap a nullable expression without null-checking it.");
        }
        return inner;
    }
};

template <typename T>
class StringExpression : public Primitive<T> {
public:
    StringExpression(T value)
        : Primitive<T>(value) {}

    virtual ~StringExpression() = default;
    
    std::shared_ptr<Expression> clone() const override {
        return std::make_shared<StringExpression<T>>(this->value);
    }
};

struct ArrayExpression : public Expression {
    bool isVariadicArray = false;
    std::vector<std::shared_ptr<Expression>> elements;

    explicit ArrayExpression(std::shared_ptr<Type> type, std::vector<std::shared_ptr<Expression>> elements = {}, bool isVariadic = false)
        : elements(elements), isVariadicArray(isVariadic) {
        this->type = Type::createFixedArrayType(type, elements.size());
        this->rootType = this->type;
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
        elements.push_back(val);
    }

    std::shared_ptr<Expression> get(size_t index) const {
        return index < elements.size() ? elements[index] : nullptr;
    }

    const std::vector<std::shared_ptr<Expression>>& getElements() const {
        return elements;
    }

    
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
        : elements(elems), elementType(elemType) {
            type = Type::createFixedArrayType(elementType, elements.size());
        }

    std::string typeName() const {
        return "FixedArray<" + (elementType ? elementType->toString() : "unknown") + ">";
    }

    std::string toString() const override {
        std::string result = "[";
        for (size_t i = 0; i < elements.size(); ++i) {
            result += elements[i]->toString();
            if (i < elements.size() - 1) result += ", ";
        }
        return result + "]";
    }
    
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
