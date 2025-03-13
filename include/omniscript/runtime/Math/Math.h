#pragma once

#include <omniscript/runtime/object.h>
#include <omniscript/omniscript_pch.h>

class Math : public Object {
public:
    Math(); // Constructor to register methods and properties

    // Override toString to provide specific string representation for the Math Object
    std::string toString(int indentLevel = 0) const override;

private:
    void registerMethods(); // Method to register all mathematical methods
    void registerProperties(); // Method to register mathematical properties
    int randInt(int min, int max);
    float randF(float min, float max);
    int randRange(int start, int stop);
    float randFRange(float start, float stop);
};
