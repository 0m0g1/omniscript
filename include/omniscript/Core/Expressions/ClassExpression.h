#include <omniscript/Core/Expression.h>
#include <omniscript/Core/Expressions/FunctionInputExpression.h>

struct ClassExpression : 
public Callable,
public AggregateExpression {
    std::shared_ptr<StructExpression> structExpr;
    std::vector<std::shared_ptr<FunctionExpression>> constructors;
    std::shared_ptr<FunctionExpression> destructor;
    std::vector<std::shared_ptr<ClassMemberExpression>> members;

    ClassExpression(
        const std::string& name,
        std::shared_ptr<StructExpression> structExpr,
        std::vector<std::shared_ptr<FunctionExpression>> constructors = {},
        std::shared_ptr<FunctionExpression> destructor = nullptr,
        std::vector<std::shared_ptr<ClassMemberExpression>> members = {}
    )
        : Callable(name, name, {}, false),  
          structExpr(std::move(structExpr)),
          constructors(std::move(constructors)),
          destructor(std::move(destructor)),
          members(std::move(members))
    {
        type = this->structExpr->getType(); 
    }

    std::string toString() const override {
        std::string memberStr;
        for (const auto& member : members) {
            memberStr += "\n  " + member->toString();
        }

        return "Class: " + structExpr->structName +
               " [Constructors: " + std::to_string(constructors.size()) +
               ", Destructor: " + (destructor ? "yes" : "none") + "]" +
               (members.empty() ? "" : "\nMembers:" + memberStr);
    }

    std::shared_ptr<FunctionExpression> resolveConstructor(const std::vector<std::shared_ptr<Expression>>& args) const {
        for (const auto& ctor : constructors) {
            if (ctor->getParameters().size() == args.size()) {
                return ctor;
            }
        }
        return nullptr;
    }

    std::shared_ptr<Expression> clone() const override {
        auto clonedStruct = std::dynamic_pointer_cast<StructExpression>(structExpr->clone());

        std::vector<std::shared_ptr<FunctionExpression>> clonedCtors;
        for (const auto& ctor : constructors)
            clonedCtors.push_back(std::dynamic_pointer_cast<FunctionExpression>(ctor->clone()));

        auto clonedDtor = destructor ? std::dynamic_pointer_cast<FunctionExpression>(destructor->clone()) : nullptr;

        std::vector<std::shared_ptr<ClassMemberExpression>> clonedMembers;
        for (const auto& member : members)
            clonedMembers.push_back(std::dynamic_pointer_cast<ClassMemberExpression>(member->clone()));

        return std::make_shared<ClassExpression>(
            name, clonedStruct, clonedCtors, clonedDtor, clonedMembers
        );
    }

    std::shared_ptr<ClassMemberExpression> getMember(const std::string& name) {
        for (const auto& member : members) {
            if (member->getName() == name) {
                return member;
            }
        }
        console.error("Member '" + name + "' not found in class '" + this->getName() + "'.");
        return nullptr;
    }

    std::string serializeMembers() const {
        std::string result = "[\n";
        for (const auto& member : members) {
            result += "  { name: \"" + member->getName() + "\", ";
            result += "type: \"" + member->getType()->toString() + "\", ";
            result += "modifiers: \"" + member->getAccessString() + "\" },\n";
        }
        result += "]";
        return result;
    }
};