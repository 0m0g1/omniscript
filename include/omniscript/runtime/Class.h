#ifndef CLASS_H
#define CLASS_H

// #include <omniscript/runtime/object.h>
// #include <omniscript/runtime/Function.h>
// #include <omniscript/omniscript_pch.h>

// class Class : public Object {
// public:
//     Class(const std::string& className, std::vector<std::shared_ptr<Class>> parentClasses = {});
//     ~Class();

//     // Add a constructor to the class
//     // void addConstructor(std::shared_ptr<Function> new_constructor) {
//     //     // constructors.push_back(constructor);
//     //     constructor = new_constructor;
//     // }

//     // Add a destructor to the class
//     // void addDestructor(std::shared_ptr<Function> new_destructor) {
//     //     // destructors.push_back(new_destructor);
//     //     destructor = new_destructor;
//     // }

//     std::string toString(int indentLevel = 0) const override;

//     void addClassMethod(const std::string& name, MethodDefinition method, const ClassMemberModifiers& modifiers);
//     std::pair<MethodDefinition, ClassMemberModifiers> getClassMethod(const std::string& name) const;

//     void setClassProperty(const std::string& name, SymbolTable::ValueType value, const ClassMemberModifiers& modifiers);
//     std::pair<SymbolTable::ValueType, ClassMemberModifiers> getClassProperty(const std::string& name) const;

//     std::unordered_map<std::string, std::shared_ptr<std::pair<MethodDefinition, ClassMemberModifiers>>> classMethods;
//     std::unordered_map<std::string, std::pair<SymbolTable::ValueType, ClassMemberModifiers>> classProperties;
//     std::vector<std::string> classNames;
//     std::vector<std::shared_ptr<Class>> parentClasses;

//     // std::vector<std::shared_ptr<Function>> parentConstructors;
//     // std::vector<std::shared_ptr<Function>> parentDestructors;
//     // std::shared_ptr<Function> constructor;
//     // std::shared_ptr<Function> destructor;
// private:
//     void registerMethods();
//     void registerProperties();

//     // std::vector<std::shared_ptr<Function>> constructors;
//     // std::vector<std::shared_ptr<Function>> destructors;
// };

// class ClassInstance : public Object {
// public:
//     // Constructor and Destructor
//     explicit ClassInstance(std::shared_ptr<Class> parentClass);
//     ~ClassInstance();

//     // Method management
//     void addClassInstanceMethod(const std::string& name, MethodDefinition method, const ClassMemberModifiers& modifiers);
//     std::pair<MethodDefinition, ClassMemberModifiers> getClassInstanceMethod(const std::string& name) const;

//     // Property management
//     void setClassInstanceProperty(const std::string& name, SymbolTable::ValueType value, const ClassMemberModifiers& modifiers);
//     std::pair<SymbolTable::ValueType, ClassMemberModifiers> getClassInstanceProperty(const std::string& name) const;

//     // Override the `toString` method
//     std::string toString(int indentLevel = 0) const override;
//     std::shared_ptr<std::vector<std::string>> classNames;
    
//     // std::vector<std::shared_ptr<Function>> constructors;
//     // std::vector<std::shared_ptr<Function>> destructors;

// private:
//     void registerMethods();
//     void registerProperties();
//     std::shared_ptr<Class> parentClass;

//     std::unordered_map<std::string, std::shared_ptr<std::pair<MethodDefinition, ClassMemberModifiers>>> ClassInstanceMethods;
//     std::unordered_map<std::string, std::pair<SymbolTable::ValueType, ClassMemberModifiers>> ClassInstanceProperties;

// };


#endif
