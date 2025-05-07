#include <omniscript/Core.h>
#include <omniscript/utils.h>
#include <omniscript/engine/parser.h>
#include <omniscript/engine/tokens.h>
#include <omniscript/engine/Statement.h>
#include <omniscript/engine/Symboltable.h>
#include <omniscript/omniscript_pch.h>


std::shared_ptr<Statement> Parser::parseEnum() {
    eat(TokenTypes::Enum);
    
    bool hasLookup = false;
    bool isEnumClass = false;

    if (currentToken.getType() == TokenTypes::Class) {
        eat(TokenTypes::Class);
        isEnumClass = true;
    }
    
    std::string enumName = currentToken.getValue();
    eat(TokenTypes::Identifier);
    
    if (currentToken.getType() == TokenTypes::LeftParen) {
        eat(TokenTypes::LeftParen);
        if (currentToken.getValue() == "lookup") {
            hasLookup = true;
            eat(TokenTypes::Identifier);
        }
        eat(TokenTypes::RightParen);
    }

    std::vector<std::shared_ptr<EnumValue>> values;
    
    eat(TokenTypes::LeftBrace);
    int currentIndex = 0;
    
    while (currentToken.getType() != TokenTypes::RightBrace) {
        if (currentToken.getType() == TokenTypes::Comma) {
            eat(TokenTypes::Comma);
        }

        if (currentToken.getType() == TokenTypes::Identifier) {
            std::string valueName = currentToken.getValue();
            eat(TokenTypes::Identifier);

            int assignedIndex = currentIndex; // Default index
            
            if (currentToken.getType() == TokenTypes::Assign) {
                eat(TokenTypes::Assign);
                std::shared_ptr<Statement> valueExpr = parseExpression();
                
                // Try to evaluate the expression as an integer
                if (auto intLiteral = std::dynamic_pointer_cast<IntegerLiteral>(valueExpr)) {
                    assignedIndex = intLiteral->getValue();
                } else {
                    console.error("Enum values must be compile-time integers");
                }
            }

            values.push_back(std::make_shared<EnumValue>(valueName, assignedIndex));
            currentIndex = assignedIndex + 1; // Auto-increment for the next entry
        }
    }

    eat(TokenTypes::RightBrace);
    
    // Create the EnumConstructor with the lookup flag
    return std::make_shared<EnumConstructor>(enumName, values, hasLookup, isEnumClass);
}

std::shared_ptr<Statement> Parser::parseNamespace() {
    eat(TokenTypes::Namespace);
    std::string namespaceName = currentToken.getValue();
    eat(TokenTypes::Identifier);

    // std::vector<std::shared_ptr<Statement>> body = parseBlock();
    
    // auto namespaceObj = std::make_shared<Namespace>(namespaceName, body);
    // auto objectConstructor = std::make_shared<ObjectConstructorStatement>(namespaceObj);
    // return std::make_shared<ConstantAssignment>(namespaceName, objectConstructor);
    return nullptr;
}
