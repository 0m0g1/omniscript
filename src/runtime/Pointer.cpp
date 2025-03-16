// #include <omniscript/runtime/Pointer.h>
// #include <omniscript/omniscript_pch.h>
// #include <omniscript/utils.h>
// #include <omniscript/debuggingtools/console.h>

// PointerObj::PointerObj(uintptr_t addr) : address(addr) {
//     registerMethods();
//     registerProperties();
// }

// uintptr_t PointerObj::getAddress() const {
//     return address;
// }

// void PointerObj::registerMethods() {
//     addMethod("getValue", [this](const ArgumentDefinition& args) -> SymbolTable::ValueType {
//         if (args.empty()) {
//             throw std::runtime_error("getValue requires a type argument.");
//         }
//         throw std::runtime_error("Type-based getValue() not implemented at runtime.");
//     });
    
//     addMethod("setValue", [this](const ArgumentDefinition& args) -> SymbolTable::ValueType {
//         if (args.size() < 1) {
//             throw std::runtime_error("setValue requires 1 argument.");
//         }
//         throw std::runtime_error("Type-based setValue() not implemented at runtime.");
//     });
//     addMethod("getAddress", [this](const ArgumentDefinition&) -> SymbolTable::ValueType {
//         return static_cast<double>(address);
//     });
// }

// void PointerObj::registerProperties() {
//     setProperty("address", static_cast<double>(address));
//     setDefaultValue();
// }

// void PointerObj::setDefaultValue() {
//     setProperty("value", 0);
// }