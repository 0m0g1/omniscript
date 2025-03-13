
#include <omniscript/runtime/object.h>
#include <omniscript/runtime/Json/Json.h>
#include <omniscript/omniscript_pch.h>
#include <omniscript/debuggingtools/console.h>

JSON::JSON() {
    registerMethods();
    registerProperties();
}


void JSON::registerMethods() {
    addMethod("stringify", [this](const ArgumentDefinition& args) {
        if (args.size() != 1) {
            throw std::runtime_error("stringify() expects exactly 1 argument: a JSON object or array.");
        }
        
        // Only check for Object, since Array is also an Object
        if (!std::holds_alternative<std::shared_ptr<Object>>(args[0])) {
            throw std::runtime_error("stringify() expects an object or array as an argument.");
        }
        
        auto obj = std::get<std::shared_ptr<Object>>(args[0]);
        
        return stringifyJSON(obj);
    });
}

void JSON::registerProperties() {
    // Example of registering properties if needed
    // addProperty("type", []() { return std::make_shared<String>("JSON"); });
}

// Todo : should return a dictionary object
SymbolTable::ValueType JSON::parseJSON(const std::string& jsonStr) {
    // Logic to parse the string into JSON objects/arrays
    // console.debug("Parsing JSON string: " + jsonStr);
    
    // // Example: assuming a simple JSON object for illustration
    // auto parsedObject = std::make_shared<Object>();
    // parsedObject->setProperty("example", std::make_shared<String>("parsed_value"));
    return "to implement parsing dictoinaries and objects";
}

std::string JSON::stringifyJSON(const std::shared_ptr<Object>& obj) {
    if (!obj) {
        throw std::runtime_error("stringifyJSON() expects a valid object.");
    }

    return obj->toString();
}
