#ifndef PARSER_H
#define PARSER_H

//Includes
#include <omniscript/omniscript_pch.h>
#include <omniscript/engine/lexer.h>
#include <omniscript/engine/tokens.h>
#include <omniscript/engine/Statement.h>
#include <omniscript/engine/Symboltable.h>
#include <omniscript/runtime/Class.h>

//Include llvm headers
// #include <llvm-c/...

using parameterType = std::vector<std::pair<std::string, std::vector<std::vector<std::string>>>>;

class Parser {
    public:
        Parser(Lexer &lexer) : lexer(lexer), currentToken(lexer.getNextToken()) {}
    
        void setDebugMode(bool state) {
            debugMode = state;
        }
        void setExecution(bool state) {
            executeStatements = state;
        }

        std::vector<std::shared_ptr<Statement>> Parse();
        
    private:
        
        std::vector<std::shared_ptr<Statement>> statements;
        
        bool executeStatements = true;
        bool debugMode = false;
        Lexer& lexer;
        Token currentToken;
        Token previousToken;

        // Function declarations for parsing different token types
        void parseProgram();                                // To parse a complete program
        
        void initializeEnvironment();                       // To initialize constants, objects and utility functions
        void initializeFunctions();
        void initializeBuiltInObjects();
        void initializeConstants();

        void expectSemicolonOrNewLine();
        
        std::shared_ptr<Statement> parseStatement(bool checkForTerminalChar = true); // To parse a single statement
        
        std::shared_ptr<Statement> parseModuleImport();
        std::shared_ptr<Statement> parseModule();

        std::vector<std::shared_ptr<Statement>> parseParameters();
        std::vector<std::shared_ptr<Statement>> parseArguments(
            TokenTypes start = TokenTypes::LeftParen, 
            TokenTypes end = TokenTypes::RightParen,
            TokenTypes assignOp = TokenTypes::Assign
        ); // Parse code block
        std::shared_ptr<Statement> parseFunctionDeclaration(parameterType paramTypes = {}); // To parse function declarations
        std::shared_ptr<Statement> parseFunctionDeclaration(
            const std::string& definedName = "",
            parameterType paramTypes = {}
        ); 
        // std::shared_ptr<Statement> parseFunctionCall();      // To parse function calls
        std::shared_ptr<Statement> parseIdentifier();          // To parse an identifier / function call

        std::shared_ptr<ForLoop> parseForLoop();               // To parse for loops
        std::shared_ptr<BreakStatement> parseBreak();          // To parse break statements
        std::shared_ptr<ContinueStatement> parseContinue();    // To parse continue statements
        std::shared_ptr<Statement> parseIfStatement();         // To parse if statements
        std::shared_ptr<Statement> parseWhileStatement();      // To parse while loops
        std::shared_ptr<ReturnStatement> parseReturnStatement();// To parse return statements
        bool isAssignmentExpression(TokenTypes tokenType);
        std::shared_ptr<Statement> parseAssignment(std::shared_ptr<Statement> assignee = nullptr);          // To parse variable assignments
        std::shared_ptr<Statement> parseAssignment(parameterType paramType);          // To parse variable assignments
        std::string parseStringLiteral();                      // To parse string literals
        std::shared_ptr<Statement> parseBlock();
        std::shared_ptr<Statement> parseLambdaFunction(const std::string& name = "", parameterType paramTypes = {});
        bool checkIfLambdaExpression();
        bool isGenericCallOrConstructor();
        bool checkIfFunctionCall();
        // SymbolTable::ValueType parseFunctionArrow();
        std::shared_ptr<Statement> parseStruct();
        std::shared_ptr<Statement> parseEnum();
        std::shared_ptr<Statement> parseNamespace();
        
        std::string generateSpecializedNameForDecleration(
            const std::string &baseName, 
            const std::vector<std::pair<std::string, std::vector<std::string>>> &types
        );
        
        std::string generateSpecializedNameForCall(
            const std::string &baseName, 
            const std::vector<std::string> &typeParams
        );

        std::vector<std::string> parseType();
        std::vector<std::string> parseTypeParametersForCall();
        parameterType parseTypeParametersForDeclaration();
        
        // Parse Objects
        /*
        std::shared_ptr<lambda> parseLambda(); // To parse a lambda function
        */
        std::shared_ptr<Statement> parseObject();
        std::shared_ptr<Statement> parseClass();
        ClassMemberModifiers parseClassMemberModifiers();

        // Parse binary and operational expressions (e.g., mathematical expressions)
        std::shared_ptr<Statement> parseTernaryExpression();   // Parse a ternary expression
        std::shared_ptr<Statement> parseBinaryExpression();   // Parse a binary expression
        std::shared_ptr<Statement> parseUnaryExpression();

        std::shared_ptr<Statement> parseExpression();         // To parse a general expression
        std::shared_ptr<Statement> logicalOrExpression();
        std::shared_ptr<Statement> logicalAndExpression();
        std::shared_ptr<Statement> comparisonExpression();
        std::shared_ptr<Statement> term();                    // To parse multiplications and divisions
        std::shared_ptr<Statement> factor();                  // To parse bracketed expressions ()

        void eat(TokenTypes expectedType, const std::string& errorMessage = ""); // Helper function to consume a token if it matches the expected type
};

#endif
