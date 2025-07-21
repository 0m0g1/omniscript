#pragma once
#include <omniscript/Expression.h>

namespace Omniscript {
struct VariableAssignment : public Expression {
    bool isExtern = false;
    bool isStatic = false;
    bool isConstant = false;
    bool isGlobal = true;
    bool isReassignment = false;
    bool isVolatile = false;
    std::string variableName;
    std::shared_ptr<Expression> assignedValue;

    // External linkage information
    std::string windowsDynamic;    // .dll
    std::string windowsStatic;     // .lib/.a
    std::string linuxShared;       // .so
    std::string linuxStatic;       // .a
    std::string macosShared;       // .dylib
    std::string macosStatic;       // .a
    std::string genericDynamic;    // fallback dynamic
    std::string genericStatic;     // fallback static
        
    std::string externName;
    std::string intrinsicName;
    std::string section = "";

    VariableAssignment(std::string name, std::shared_ptr<Expression> value, bool isGlobal = false, bool isReassignment = false)
        : variableName(name), assignedValue(value), isGlobal(isGlobal), isReassignment(isReassignment) {
        type = assignedValue->type;  
    }

    std::shared_ptr<Expression> getValue() const { return assignedValue; }
    std::string toString() const override {
        return "Assign: " + variableName + " = " + assignedValue->toString();
    }
    std::string toDebugString() const {
        std::ostringstream out;
        out << "VariableAssignment Debug Info:\n";
        out << "  variableName   : " << variableName << "\n";
        out << "  isStatic       : " << (isStatic ? "true" : "false") << "\n";
        out << "  isConstant     : " << (isConstant ? "true" : "false") << "\n";
        out << "  isGlobal       : " << (isGlobal ? "true" : "false") << "\n";
        out << "  isReassignment : " << (isReassignment ? "true" : "false") << "\n";
        out << "  isVolatile     : " << (isVolatile ? "true" : "false") << "\n";

        out << "  assignedValue  : ";
        if (assignedValue) {
            out << assignedValue->toString() << "\n";
        } else {
            out << "null\n";
        }

        out << "  type           : ";
        if (type) {
            out << type->toString() << "\n";
        } else {
            out << "null\n";
        }

        return out.str();
    }
    
    std::shared_ptr<Expression> clone() const override {
        auto clone = std::make_shared<VariableAssignment>(
            variableName,
            assignedValue ? assignedValue->clone() : nullptr
        );
        clone->isStatic = isStatic;
        clone->isGlobal = isGlobal;
        clone->isConstant = isConstant;
        clone->isReassignment = isReassignment;
        clone->isVolatile = isVolatile;
        clone->windowsDynamic = windowsDynamic;    // .dll
        clone->windowsStatic = windowsStatic;     // .lib/.a
        clone->linuxShared = linuxShared;       // .so
        clone->linuxStatic = linuxStatic;       // .a
        clone->macosShared = macosShared;       // .dylib
        clone->macosStatic = macosStatic;       // .a
        clone->genericDynamic = genericDynamic;    // fallback dynamic
        clone->genericStatic = genericStatic;     // fallback static
            
        clone->externName = externName;
        clone->intrinsicName = intrinsicName;
        clone->section = section;
        clone->isExtern = isExtern;
        return clone;
    }
};
}