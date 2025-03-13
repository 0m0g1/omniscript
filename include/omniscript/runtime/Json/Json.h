#pragma once

#include <omniscript/runtime/object.h>
#include <omniscript/omniscript_pch.h>

class JSON : public Object {
public:
    JSON();

    void registerMethods();
    void registerProperties();

private:
    SymbolTable::ValueType parseJSON(const std::string& jsonStr);
    std::string stringifyJSON(const std::shared_ptr<Object>& obj);
};
