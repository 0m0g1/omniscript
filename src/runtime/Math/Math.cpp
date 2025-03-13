#include <omniscript/runtime/Math/Math.h>
#include <omniscript/omniscript_pch.h>

Math::Math() {
    registerMethods();
    registerProperties();
}

void Math::registerMethods() {
    // Registering mathematical methods
    addMethod("abs", [this](const ArgumentDefinition& args) -> SymbolTable::ValueType {
        if (args.size() == 1) {
            return std::fabs(std::get<float>(args[0]));
        }
        return nullptr; // Handle invalid arguments
    });

    addMethod("acos", [this](const ArgumentDefinition& args) -> SymbolTable::ValueType {
        if (args.size() == 1) {
            return std::acos(std::get<float>(args[0]));
        }
        return nullptr; // Handle invalid arguments
    });

    addMethod("acosh", [this](const ArgumentDefinition& args) -> SymbolTable::ValueType {
        if (args.size() == 1) {
            return std::acosh(std::get<float>(args[0]));
        }
        return nullptr; // Handle invalid arguments
    });

    addMethod("asin", [this](const ArgumentDefinition& args) -> SymbolTable::ValueType {
        if (args.size() == 1) {
            return std::asin(std::get<float>(args[0]));
        }
        return nullptr; // Handle invalid arguments
    });

    addMethod("asinh", [this](const ArgumentDefinition& args) -> SymbolTable::ValueType {
        if (args.size() == 1) {
            return std::asinh(std::get<float>(args[0]));
        }
        return nullptr; // Handle invalid arguments
    });

    addMethod("atan", [this](const ArgumentDefinition& args) -> SymbolTable::ValueType {
        if (args.size() == 1) {
            return std::atan(std::get<float>(args[0]));
        }
        return nullptr; // Handle invalid arguments
    });

    addMethod("atan2", [this](const ArgumentDefinition& args) -> SymbolTable::ValueType {
        if (args.size() == 2) {
            return std::atan2(std::get<float>(args[0]), std::get<float>(args[1]));
        }
        return nullptr; // Handle invalid arguments
    });

    addMethod("atanh", [this](const ArgumentDefinition& args) -> SymbolTable::ValueType {
        if (args.size() == 1) {
            return std::atanh(std::get<float>(args[0]));
        }
        return nullptr; // Handle invalid arguments
    });

    addMethod("cbrt", [this](const ArgumentDefinition& args) -> SymbolTable::ValueType {
        if (args.size() == 1) {
            return std::cbrt(std::get<float>(args[0]));
        }
        return nullptr; // Handle invalid arguments
    });

    addMethod("ceil", [this](const ArgumentDefinition& args) -> SymbolTable::ValueType {
        if (args.size() == 1) {
            return std::ceil(std::get<float>(args[0]));
        }
        return nullptr; // Handle invalid arguments
    });

    addMethod("cos", [this](const ArgumentDefinition& args) -> SymbolTable::ValueType {
        if (args.size() == 1) {
            return std::cos(std::get<float>(args[0]));
        }
        return nullptr; // Handle invalid arguments
    });

    addMethod("cosh", [this](const ArgumentDefinition& args) -> SymbolTable::ValueType {
        if (args.size() == 1) {
            return std::cosh(std::get<float>(args[0]));
        }
        return nullptr; // Handle invalid arguments
    });

    addMethod("exp", [this](const ArgumentDefinition& args) -> SymbolTable::ValueType {
        if (args.size() == 1) {
            return std::exp(std::get<float>(args[0]));
        }
        return nullptr; // Handle invalid arguments
    });

    addMethod("floor", [this](const ArgumentDefinition& args) -> SymbolTable::ValueType {
        if (args.size() == 1) {
            return std::floor(std::get<float>(args[0]));
        }
        return nullptr; // Handle invalid arguments
    });

    addMethod("log", [this](const ArgumentDefinition& args) -> SymbolTable::ValueType {
        if (args.size() == 1) {
            return std::log(std::get<float>(args[0]));
        }
        return nullptr; // Handle invalid arguments
    });

    addMethod("max", [this](const ArgumentDefinition& args) -> SymbolTable::ValueType {
        if (args.size() >= 2) {
            return *std::max_element(args.begin(), args.end(), [](const SymbolTable::ValueType& a, const SymbolTable::ValueType& b) {
                return std::get<float>(a) < std::get<float>(b);
            });
        }
        return nullptr; // Handle invalid arguments
    });

    addMethod("min", [this](const ArgumentDefinition& args) -> SymbolTable::ValueType {
        if (args.size() >= 2) {
            return *std::min_element(args.begin(), args.end(), [](const SymbolTable::ValueType& a, const SymbolTable::ValueType& b) {
                return std::get<float>(a) < std::get<float>(b);
            });
        }
        return nullptr; // Handle invalid arguments
    });

    addMethod("pow", [this](const ArgumentDefinition& args) -> SymbolTable::ValueType {
        if (args.size() == 2) {
            return std::pow(std::get<float>(args[0]), std::get<float>(args[1]));
        }
        return nullptr; // Handle invalid arguments
    });

    addMethod("random", [this](const ArgumentDefinition& args) -> SymbolTable::ValueType {
        return static_cast<float>(rand()) / static_cast<float>(RAND_MAX); // Random value between 0 and 1
    });

    addMethod("round", [this](const ArgumentDefinition& args) -> SymbolTable::ValueType {
        if (args.size() == 1) {
            return std::round(std::get<float>(args[0]));
        }
        return nullptr; // Handle invalid arguments
    });

    addMethod("sin", [this](const ArgumentDefinition& args) -> SymbolTable::ValueType {
        if (args.size() == 1) {
            return std::sin(std::get<float>(args[0]));
        }
        return nullptr; // Handle invalid arguments
    });

    addMethod("sqrt", [this](const ArgumentDefinition& args) -> SymbolTable::ValueType {
        if (args.size() == 1) {
            return std::sqrt(std::get<float>(args[0]));
        }
        return nullptr; // Handle invalid arguments
    });

    addMethod("tan", [this](const ArgumentDefinition& args) -> SymbolTable::ValueType {
        if (args.size() == 1) {
            return std::tan(std::get<float>(args[0]));
        }
        return nullptr; // Handle invalid arguments
    });
    addMethod("randint", [this](const ArgumentDefinition& args) {
        if (args.size() != 2) {
            throw std::runtime_error("randint() expects two arguments: min and max.");
        }
        if (!std::holds_alternative<int>(args[0]) || !std::holds_alternative<int>(args[1])) {
            throw std::runtime_error("randint() expects both arguments to be integers.");
        }
        int min = std::get<int>(args[0]);
        int max = std::get<int>(args[1]);

        return randInt(min, max);
    });

    addMethod("randf", [this](const ArgumentDefinition& args) {
        if (args.size() != 2) {
            throw std::runtime_error("randf() expects two arguments: min and max.");
        }
        if (!std::holds_alternative<float>(args[0]) || !std::holds_alternative<float>(args[1])) {
            throw std::runtime_error("randf() expects both arguments to be floats.");
        }
        float min = std::get<float>(args[0]);
        float max = std::get<float>(args[1]);

        return randF(min, max);
    });

    addMethod("randrange", [this](const ArgumentDefinition& args) {
        if (args.size() != 2) {
            throw std::runtime_error("randrange() expects two arguments: start and stop.");
        }
        if (!std::holds_alternative<int>(args[0]) || !std::holds_alternative<int>(args[1])) {
            throw std::runtime_error("randrange() expects both arguments to be integers.");
        }
        int start = std::get<int>(args[0]);
        int stop = std::get<int>(args[1]);

        return randRange(start, stop);
    });

    addMethod("randfrange", [this](const ArgumentDefinition& args) {
        if (args.size() != 2) {
            throw std::runtime_error("randfrange() expects two arguments: start and stop.");
        }
        if (!std::holds_alternative<float>(args[0]) || !std::holds_alternative<float>(args[1])) {
            throw std::runtime_error("randfrange() expects both arguments to be floats.");
        }
        float start = std::get<float>(args[0]);
        float stop = std::get<float>(args[1]);

        return randFRange(start, stop);
    });
}

void Math::registerProperties() {
    // Registering constants as properties
    setProperty("E", 2.718281828459045);
    setProperty("PI", 3.141592653589793);
    setProperty("LN2", 0.6931471805599453);
    setProperty("LN10", 2.302585092994046);
    setProperty("LOG2E", 1.4426950408889634);
    setProperty("LOG10E", 0.4342944819032518);
    setProperty("SQRT1_2", 0.7071067811865476);
    setProperty("SQRT2", 1.4142135623730951);
}

std::string Math::toString(int indentLevel) const {
    return "[Math]"; // More detailed information can be added if needed
}

int Math::randInt(int min, int max) {
    return min + (rand() % (max - min + 1));
}

float Math::randF(float min, float max) {
    return min + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / (max - min)));
}

int Math::randRange(int start, int stop) {
    return start + rand() % (stop - start);
}

float Math::randFRange(float start, float stop) {
    return start + static_cast<float>(rand()) / static_cast<float>(RAND_MAX / (stop - start));
}
