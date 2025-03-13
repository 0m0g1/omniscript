#ifndef CLASS_H
#define CLASS_H

#include <omniscript/runtime/object.h>
#include <omniscript/runtime/Function.h>
#include <omniscript/omniscript_pch.h>


struct ClassMemberModifiers {
    // Access modifiers
    enum class AccessModifier { Public, Protected, Private };
    AccessModifier access = AccessModifier::Public; // Default to private
    bool isInitialized = false;

    // Storage specifiers
    bool isStatic = false;
    bool isExtern = false;
    bool isMutable = false;
    bool isThreadLocal = false;

    // Function-specific modifiers
    bool isVirtual = false;
    bool shouldOverride = false;
    bool isFinal = false;
    bool isConst = false;
    bool isVolatile = false;
    bool isNoexcept = false;
    bool isPureVirtual = false; // Implies `= 0`
    bool isExplicit = false;
    bool isInline = false;
    bool isConstexpr = false;

    // Special member function specifiers
    bool isDefault = false; // Implies `= default`
    bool isDeleted = false; // Implies `= delete`

    // Attribute specifiers
    bool isNodiscard = false;
    bool isMaybeUnused = false;
    bool isDeprecated = false;
    bool isLikely = false;
    bool isUnlikely = false;

    // Convenience function to display modifiers as a string
    std::string toString() const {
        std::string result;

        // Access
        switch (access) {
            case AccessModifier::Public: result += "public "; break;
            case AccessModifier::Protected: result += "protected "; break;
            case AccessModifier::Private: result += "private "; break;
        }

        // Storage specifiers
        if (isStatic) result += "static ";
        if (isExtern) result += "extern ";
        if (isMutable) result += "mutable ";
        if (isThreadLocal) result += "thread_local ";

        // Function modifiers
        if (isVirtual) result += "virtual ";
        if (shouldOverride) result += "should_override ";
        if (isFinal) result += "final ";
        if (isConst) result += "const ";
        if (isVolatile) result += "volatile ";
        if (isNoexcept) result += "noexcept ";
        if (isPureVirtual) result += "= 0 (pure virtual) ";
        if (isExplicit) result += "explicit ";
        if (isInline) result += "inline ";
        if (isConstexpr) result += "constexpr ";

        // Special member function specifiers
        if (isDefault) result += "= default ";
        if (isDeleted) result += "= delete ";

        // Attributes
        if (isNodiscard) result += "[[nodiscard]] ";
        if (isMaybeUnused) result += "[[maybe_unused]] ";
        if (isDeprecated) result += "[[deprecated]] ";
        if (isLikely) result += "[[likely]] ";
        if (isUnlikely) result += "[[unlikely]] ";

        return result.empty() ? "none" : result;
    }
};

namespace std {
    template <>
    struct hash<ClassMemberModifiers> {
        size_t operator()(const ClassMemberModifiers& modifiers) const {
            size_t result = 0;

            // Hash the access modifier (enum class)
            result ^= static_cast<size_t>(modifiers.access) + 0x9e3779b9 + (result << 6) + (result >> 2);

            // Hash the bool flags for storage specifiers and function-specific modifiers
            result ^= modifiers.isStatic + 0x9e3779b9 + (result << 6) + (result >> 2);
            result ^= modifiers.isExtern + 0x9e3779b9 + (result << 6) + (result >> 2);
            result ^= modifiers.isMutable + 0x9e3779b9 + (result << 6) + (result >> 2);
            result ^= modifiers.isThreadLocal + 0x9e3779b9 + (result << 6) + (result >> 2);
            result ^= modifiers.isVirtual + 0x9e3779b9 + (result << 6) + (result >> 2);
            result ^= modifiers.shouldOverride + 0x9e3779b9 + (result << 6) + (result >> 2);
            result ^= modifiers.isFinal + 0x9e3779b9 + (result << 6) + (result >> 2);
            result ^= modifiers.isConst + 0x9e3779b9 + (result << 6) + (result >> 2);
            result ^= modifiers.isVolatile + 0x9e3779b9 + (result << 6) + (result >> 2);
            result ^= modifiers.isNoexcept + 0x9e3779b9 + (result << 6) + (result >> 2);
            result ^= modifiers.isPureVirtual + 0x9e3779b9 + (result << 6) + (result >> 2);
            result ^= modifiers.isExplicit + 0x9e3779b9 + (result << 6) + (result >> 2);
            result ^= modifiers.isInline + 0x9e3779b9 + (result << 6) + (result >> 2);
            result ^= modifiers.isConstexpr + 0x9e3779b9 + (result << 6) + (result >> 2);
            result ^= modifiers.isDefault + 0x9e3779b9 + (result << 6) + (result >> 2);
            result ^= modifiers.isDeleted + 0x9e3779b9 + (result << 6) + (result >> 2);
            result ^= modifiers.isNodiscard + 0x9e3779b9 + (result << 6) + (result >> 2);
            result ^= modifiers.isMaybeUnused + 0x9e3779b9 + (result << 6) + (result >> 2);
            result ^= modifiers.isDeprecated + 0x9e3779b9 + (result << 6) + (result >> 2);
            result ^= modifiers.isLikely + 0x9e3779b9 + (result << 6) + (result >> 2);
            result ^= modifiers.isUnlikely + 0x9e3779b9 + (result << 6) + (result >> 2);

            return result;
        }
    };
}



class Class : public Object {
public:
    Class(const std::string& className, std::vector<std::shared_ptr<Class>> parentClasses = {});
    ~Class();

    // Add a constructor to the class
    void addConstructor(std::shared_ptr<Function> new_constructor) {
        // constructors.push_back(constructor);
        constructor = new_constructor;
    }

    // Add a destructor to the class
    void addDestructor(std::shared_ptr<Function> new_destructor) {
        // destructors.push_back(new_destructor);
        destructor = new_destructor;
    }

    std::string toString(int indentLevel = 0) const override;

    void addClassMethod(const std::string& name, MethodDefinition method, const ClassMemberModifiers& modifiers);
    std::pair<MethodDefinition, ClassMemberModifiers> getClassMethod(const std::string& name) const;

    void setClassProperty(const std::string& name, SymbolTable::ValueType value, const ClassMemberModifiers& modifiers);
    std::pair<SymbolTable::ValueType, ClassMemberModifiers> getClassProperty(const std::string& name) const;

    std::unordered_map<std::string, std::shared_ptr<std::pair<MethodDefinition, ClassMemberModifiers>>> classMethods;
    std::unordered_map<std::string, std::pair<SymbolTable::ValueType, ClassMemberModifiers>> classProperties;
    std::vector<std::string> classNames;
    std::vector<std::shared_ptr<Class>> parentClasses;

    std::vector<std::shared_ptr<Function>> parentConstructors;
    std::vector<std::shared_ptr<Function>> parentDestructors;
    std::shared_ptr<Function> constructor;
    std::shared_ptr<Function> destructor;
private:
    void registerMethods();
    void registerProperties();

    // std::vector<std::shared_ptr<Function>> constructors;
    // std::vector<std::shared_ptr<Function>> destructors;
};

class ClassInstance : public Object {
public:
    // Constructor and Destructor
    explicit ClassInstance(std::shared_ptr<Class> parentClass);
    ~ClassInstance();

    // Method management
    void addClassInstanceMethod(const std::string& name, MethodDefinition method, const ClassMemberModifiers& modifiers);
    std::pair<MethodDefinition, ClassMemberModifiers> getClassInstanceMethod(const std::string& name) const;

    // Property management
    void setClassInstanceProperty(const std::string& name, SymbolTable::ValueType value, const ClassMemberModifiers& modifiers);
    std::pair<SymbolTable::ValueType, ClassMemberModifiers> getClassInstanceProperty(const std::string& name) const;

    // Override the `toString` method
    std::string toString(int indentLevel = 0) const override;
    std::shared_ptr<std::vector<std::string>> classNames;
    
    std::vector<std::shared_ptr<Function>> constructors;
    std::vector<std::shared_ptr<Function>> destructors;

private:
    void registerMethods();
    void registerProperties();
    std::shared_ptr<Class> parentClass;

    std::unordered_map<std::string, std::shared_ptr<std::pair<MethodDefinition, ClassMemberModifiers>>> ClassInstanceMethods;
    std::unordered_map<std::string, std::pair<SymbolTable::ValueType, ClassMemberModifiers>> ClassInstanceProperties;

};


#endif
