#include <omniscript/engine/Statement.h>

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
        return std::make_shared<Invalid>(); 
    }
};

class NullLiteral : public Literal {
public:
    virtual ~NullLiteral() = default;
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

class PointerLiteral : public Literal {
public:
    PointerLiteral(size_t address, 
                          std::shared_ptr<Omniscript::Type> pointeeType = nullptr,
                          bool isConst = false,
                          bool isVolatile = false)
        : address(address),
          isConst(isConst),
          isVolatile(isVolatile) {
        
        if (!pointeeType) {
            pointeeType = Omniscript::Type::createPrimitiveType(Omniscript::Kind::Void);
        }
        
        setType(Omniscript::Type::createPointerType(pointeeType, isConst, isVolatile));
        setRootType(Omniscript::Type::createPointerType(pointeeType, isConst, isVolatile));
    }

    std::shared_ptr<Statement> evaluate(SymbolTableType scope) override { 
        return nullptr; 
    }
    
    std::shared_ptr<Omniscript::Expression> express(SymbolTableType scope) override;
    std::shared_ptr<Literal> castTo(std::shared_ptr<Omniscript::Type> targetType) const override;
    
    std::string toString() const override { 
        std::string typeStr = type ? type->toString() : "unknown";
        return "PointerLiteral(" + std::to_string(address) + " as " + typeStr + ")"; 
    }
    
    std::shared_ptr<Statement> clone() const override {
        auto pointeeType = type ? type->getPointeeType() : nullptr;
        return std::make_shared<PointerLiteral>(
            address,
            pointeeType,
            isConst,
            isVolatile
        );
    }

    size_t getAddress() const { return address; }
    bool isConstPointer() const { return isConst; }
    bool isVolatilePointer() const { return isVolatile; }

private:
    size_t address;
    bool isConst;
    bool isVolatile;
};

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
    std::string toString() const override { return "IntegerLiteral: " + std::to_string(value); }
    std::shared_ptr<Statement> clone() const override {
        return std::make_shared<IntegerLiteral>(value);  // Clone using copy constructor
    }
    std::shared_ptr<Literal> castTo(std::shared_ptr<Omniscript::Type> targetType) const override;

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
    std::string toString() const override { return "FloatLiteral: " + std::to_string(static_cast<long double>(value)); }
    std::shared_ptr<Statement> clone() const override {
        return std::make_shared<FloatLiteral>(value);  // Clone using copy constructor
    }
    std::shared_ptr<Literal> castTo(std::shared_ptr<Omniscript::Type> targetType) const override;

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

    std::string toString() const override { return "BigIntLiteral: " + value; }
    std::shared_ptr<Statement> clone() const override {
        return std::make_shared<BigInt>(value);  // Clone using copy constructor
    }

private:
    std::string value;
    unsigned bitWidth;  // e.g., 128, 256, 1024
};

class CharacterLiteral : public Literal {
public:
    char32_t value;

    explicit CharacterLiteral(char32_t val) : value(std::move(val)) {
        setRootType(Omniscript::Type::createPrimitiveType(Omniscript::Kind::Char));
    }
    std::shared_ptr<Statement> evaluate(SymbolTableType scope) override { return nullptr; }
    std::shared_ptr<Omniscript::Expression> express(SymbolTableType scope) override;
    std::string toString() const override { return "CharacterLiteral: " + value; }
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
    std::string toString() const override { return "StringLiteral: " + utf32_to_utf8(value); }
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
    std::string toString() const override { return "BoolLiteral: " + std::to_string(value); }
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
    std::shared_ptr<Literal> castTo(std::shared_ptr<Omniscript::Type> targetType) const override;
};
