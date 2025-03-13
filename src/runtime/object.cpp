#include <omniscript/runtime/object.h>
#include <omniscript/runtime/symboltable.h>
#include <omniscript/utils.h>

#include <omniscript/omniscript_pch.h>

// #ifdef _WIN32
// #include <Windows.h>
// #endif

// #if defined(__linux__) || defined(__APPLE__)
// #include <sys/resource.h>
// #endif

std::atomic<unsigned long long> Object::objectCounter{0};

// convert the whole object and its properties to a string for printing
// if there is one property put everything on a line
std::string Object::objectString() const {
    std::ostringstream oss;
    // oss << name;
    oss << "{";

    // Iterate over properties
    for (const auto& [key, value] : properties) {
        oss << "\n\t" << key << ": ";
        if (value.index() != std::variant_npos) { // Check if value is valid
            oss << valueToString(value);
        } else {
            oss << "null"; // Handle uninitialized values
        }
        oss << ",";
    }

     // Todo:: remove the last comma
    // if (methods.size() != 1) {
        // oss << "\n\t" << "methods: [";
    for (const auto& method : methods) {
        // italicize f with ansi
        oss << "\n\t" << method.first << ": \x1B[3m" << "f" << "\x1B[0m" << "()" << ",";
    }
        // oss << "\n\t\t]\n";
    // }
    oss << "\n}";

    return oss.str();
}

std::string Object::toString(int indentLevel) const {
    std::ostringstream oss;
    std::string indentation(indentLevel, '\t');
    oss << "{";

    // Get an iterator to the beginning of the properties map
    auto it = properties.begin();
    // Iterate over properties
    while (it != properties.end()) {
        oss << "\n" << indentation << "\t" << it->first << ": ";
        if (it->second.index() != std::variant_npos) { // Check if value is valid
            oss << valueToString(it->second, indentLevel + 1);
        } else {
            oss << "null"; // Handle uninitialized values
        }
        ++it;
        if (it != properties.end()) {
            oss << ","; // Add comma if not the last element
        }
    }

    oss << "\n" << indentation;

    //Todo:: space the last bracket the lenth of the object property's name + one space
    /*
        {
            y: 0,
            x: {
                    x: 0
            #here}
        }
    */
    // if (indentLevel != 0) {

    // }

    oss << "}";
    return oss.str();
}

std::string Object::generateUniqueId() const {
    // Get current time as time_point
    auto now = std::chrono::system_clock::now();

    // Convert to time_t to extract time components
    std::time_t now_time_t = std::chrono::system_clock::to_time_t(now);

    // Convert to tm structure for local time
    std::tm now_tm;
    localtime_s(&now_tm, &now_time_t);

    // Extract milliseconds
    auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

    // Get the static counter value
    unsigned long long counterValue = objectCounter.fetch_add(1, std::memory_order_relaxed);

    // Format the timestamp as YYYYMMDDHHMMSSmmm + Counter
    std::ostringstream oss;
    oss << std::put_time(&now_tm, "%Y%m%d%H%M%S")
        << std::setw(3) << std::setfill('0') << now_ms.count()
        << "_" << counterValue;

    return oss.str();
}

/*
* Booleans
*/
Boolean::Boolean(bool value) : value(value) {
    name = "a Boolean";

    // Add methods to the methods map
    methods["getValue"] = [this](const ArgumentDefinition& args) -> SymbolTable::ValueType {
        return getValue();
    };

    methods["setValue"] = [this](const ArgumentDefinition& args) -> SymbolTable::ValueType {
        if (args.empty() || !std::holds_alternative<bool>(args[0])) {
            throw std::runtime_error("setValue requires a boolean argument.");
        }
        setValue(std::get<bool>(args[0]));
        return SymbolTable::SymbolTable::ValueType{}; // No return value
    };

    methods["toString"] = [this](const ArgumentDefinition&) -> SymbolTable::ValueType {
        return toString();
    };

    methods["toInt"] = [this](const ArgumentDefinition&) -> SymbolTable::ValueType {
        return toInt();
    };

    methods["isTrue"] = [this](const ArgumentDefinition&) -> SymbolTable::ValueType {
        return isTrue();
    };

    methods["isFalse"] = [this](const ArgumentDefinition&) -> SymbolTable::ValueType {
        return isFalse();
    };

    methods["negate"] = [this](const ArgumentDefinition&) -> SymbolTable::ValueType {
        return negate();
    };
}

//String object


/*
* Array objects
*/

// Constructor to initialize with elements (default is empty vector)
// Array::Array(std::vector<SymbolTable::ValueType> &elements) : elements(elements) {
Array::Array(std::vector<SymbolTable::ValueType> elements) : elements(elements) {
    name = "an Array";
    addMethod("push", [this](const ArgumentDefinition& args) -> SymbolTable::ValueType {
        if (args.size() != 1) {
            throw std::invalid_argument("push requires exactly 1 argument");
        }
        this->elements.push_back(args[0]);
        return SymbolTable::SymbolTable::ValueType{}; // Return a null/void-like value
    });

    addMethod("append", [&, this](const ArgumentDefinition& args) -> SymbolTable::ValueType {
        if (args.size() != 1) {
            throw std::invalid_argument("append requires exactly 1 argument");
        }
        this->elements.push_back(args[0]);
        return SymbolTable::SymbolTable::ValueType{};
    });

   addMethod("fill", [this](const ArgumentDefinition& args) -> SymbolTable::ValueType {
        // Ensure exactly two arguments are provided
        if (args.size() != 2) {
            throw std::invalid_argument("fill requires exactly 2 arguments: value to fill and number of times");
        }

        // Extract and validate the first argument (value to fill)
        SymbolTable::ValueType valueToFill = args[0];

        // Extract and validate the second argument (number of times to fill)
        if (!std::holds_alternative<int>(args[1])) {
            throw std::invalid_argument("Second argument must be an integer representing the number of times to fill");
        }
        int numTimes = std::get<int>(args[1]);

        if (numTimes < 0) {
            throw std::invalid_argument("Number of times to fill cannot be negative");
        }

        // Perform the fill operation
        this->elements.reserve(this->elements.size() + numTimes); // Optional: Optimize for large fills
        for (int i = 0; i < numTimes; ++i) {
            this->elements.push_back(valueToFill);
        }

        // Return the array object to allow for chaining or inspection
        return SymbolTable::ValueType{this};  
    });


    addMethod("prepend", [this](const ArgumentDefinition& args) -> SymbolTable::ValueType {
        if (args.size() != 1) {
            throw std::invalid_argument("prepend requires exactly 1 argument");
        }
        this->elements.insert(this->elements.begin(), args[0]);
        return SymbolTable::SymbolTable::ValueType{};
    });

    addMethod("pop", [this](const ArgumentDefinition& args) -> SymbolTable::ValueType {
        if (!this->elements.empty()) {
            SymbolTable::ValueType val = this->elements.back();
            this->elements.pop_back();
            return val;
        } else {
            throw std::out_of_range("Cannot pop from an empty array");
        }
    });

    addMethod("get", [this](const ArgumentDefinition& args) -> SymbolTable::ValueType {
        if (args.size() != 1 || !std::holds_alternative<int>(args[0])) {
            throw std::invalid_argument("get requires exactly 1 integer argument");
        }

        int index = std::get<int>(args[0]);

        // Handle negative indices like Python
        if (index < 0) {
            index += this->elements.size();
        }

        // Check if the index is still out of range after adjustment
        if (index < 0 || index >= this->elements.size()) {
            throw std::out_of_range("Index out of range");
        }

        return this->elements[index];
    });

    addMethod("set", [this](const ArgumentDefinition& args) -> SymbolTable::ValueType {
        if (args.size() != 2) {
            throw std::invalid_argument("Set requires 2 arguments, the position to change and the object to change the value to");
        }

        if (!std::holds_alternative<int>(args[0])) {
            throw std::invalid_argument("The first argument in set should be an integer representing the index to change");
        }
        int index = std::get<int>(args[0]);
        this->elements[index] = SymbolTable::ValueType(args[1]);
        return SymbolTable::ValueType{};
    });

    addMethod("length", [this](const ArgumentDefinition& args) -> SymbolTable::ValueType {
        return static_cast<int>(this->elements.size());
    });

    addMethod("toString", [this](const ArgumentDefinition& args) -> SymbolTable::ValueType {
        return this->toString();
    });
}

std::string Array::toString(int indentLevel) const {
    std::stringstream ss;
    ss << "[";
    for (size_t i = 0; i < elements.size(); ++i) {
        ss << valueToString(elements[i]);
        
        if (i != elements.size() - 1) {
            ss << ", ";
        }
    }
    ss << "]";
    return ss.str();
}


// Dictionary objects
Dictionary::Dictionary() {
    name = "a Dictionary";
}

// std::string Dictionary::toString(int indentLevel = 1) const {
//     return objectString();
// }

// Add a key-value pair to the dictionary
void Dictionary::set(const std::string& key, const SymbolTable::ValueType& value) {
    if (has(key)) {
        properties[key] = value;
    }
    console.error("The dictionary doesn't contain the key " + key);
}

// Get the value associated with a key
SymbolTable::ValueType Dictionary::get(const std::string& key) const {
    auto it = properties.find(key);
    if (it == properties.end()) {
        console.error("Key not found in properties");
    }
    return it->second; // Directly return the SymbolTable::ValueType
}

// Check if the dictionary has a certain key
bool Dictionary::has(const std::string& key) const {
    return properties.find(key) != properties.end();
}

// Remove a key-value pair from the dictionary
void Dictionary::remove(const std::string& key) {
    properties.erase(key);
}

// Get all the keys in the dictionary
// std::shared_ptr<Object> Dictionary::keys() const {
//     std::vector<SymbolTable::ValueType> keysList;
//     for (const auto& pair : properties) {
//         keysList.push_back(std::make_shared<String>(pair.first));  // Assuming String is a derived class from Object
//     }
//     return std::make_shared<Array>(keysList);  // Assuming List is a container class for Objects
// }

// Get all the values in the dictionary
// std::shared_ptr<Object> Dictionary::values() const {
//     std::vector<SymbolTable::ValueType> valuesList;
//     for (const auto& pair : properties) {
//         valuesList.push_back(pair.second);
//     }
//     return std::make_shared<Array>(valuesList);  // List holding values
// }

// Get all the entries in the dictionary as key-value pairs
// std::shared_ptr<Object> Dictionary::entries() const {
//     std::vector<SymbolTable::ValueType> entriesList;
//     for (const auto& pair : properties) {
//         std::shared_ptr<Dictionary> entry = std::make_shared<Dictionary>();
//         entry->set("key", pair.first);
//         entry->set("value", valueToString(pair.second));
//         entriesList.push_back(entry);
//     }
//     return std::make_shared<Array>(entriesList);
// }

// Override toString to represent a dictionary
// std::string Dictionary::toString(int indentLevel = 1) const {
//     return objectString();  // Uses the base object's objectString() method
// }


