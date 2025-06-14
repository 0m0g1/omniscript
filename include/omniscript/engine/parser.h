#ifndef PARSER_H
#define PARSER_H


#include <omniscript/omniscript_pch.h>
#include <omniscript/engine/lexer.h>
#include <omniscript/engine/tokens.h>
#include <omniscript/engine/Statement.h>
#include <omniscript/engine/Symboltable.h>
#include <omniscript/runtime/Class.h>

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

        
        void parseProgram();                                
        
        void initializeEnvironment();                       
        void initializeFunctions();
        void initializeBuiltInObjects();
        void initializeConstants();

        void expectSemicolonOrNewLine();
        
        std::shared_ptr<Statement> parseStatement(bool checkForTerminalChar = true); 
        
        std::shared_ptr<Statement> parseInclude();
        std::shared_ptr<Statement> parseModuleImport();
        std::shared_ptr<Statement> parseModule();
        std::shared_ptr<Statement> parseExternFunction();
        std::shared_ptr<Statement> parseIntrinsicFunction();

        std::vector<std::shared_ptr<Statement>> parseParameters();

        std::vector<std::shared_ptr<Statement>> parseArguments(
            TokenTypes start = TokenTypes::LeftParen, 
            TokenTypes end = TokenTypes::RightParen,
            TokenTypes assignOp = TokenTypes::Assign
        );

        std::shared_ptr<Statement> parseFunctionDeclaration(
            parameterType paramTypes = {},
            std::shared_ptr<Omniscript::Type> type = nullptr
        ); 

        std::shared_ptr<Statement> parseFunctionDeclaration(
            const std::string& definedName = "",
            parameterType paramTypes = {},
            std::shared_ptr<Omniscript::Type> type = nullptr
        ); 

        std::shared_ptr<Statement> parseIdentifier();          

        std::shared_ptr<ForLoop> parseForLoop();               
        std::shared_ptr<BreakStatement> parseBreak();          
        std::shared_ptr<ContinueStatement> parseContinue();    
        std::shared_ptr<Statement> parseIfStatement();         
        std::shared_ptr<Statement> parseWhileStatement();      
        std::shared_ptr<ReturnStatement> parseReturnStatement();
        bool isAssignmentExpression(TokenTypes tokenType);
        std::shared_ptr<Statement> parseAssignment(std::shared_ptr<Statement> assignee = nullptr);          
        std::shared_ptr<Statement> parseAssignment(parameterType paramType);      

        std::u32string parseStringLiteral();                   
        std::shared_ptr<Statement> parseStringTemplate();            

        std::shared_ptr<Statement> parseBlock();

        std::shared_ptr<Statement> parseLambdaFunction(
            const std::string& name = "",
            parameterType paramTypes = {},
            std::shared_ptr<Omniscript::Type> type = nullptr
        );

        bool tryParseTypeParametersLookahead(int& i);
        bool checkIfLambdaExpression();
        bool isGenericCallOrConstructor();
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
        
        std::shared_ptr<Statement> parseObject();
        std::shared_ptr<Statement> parseClass();
        MemberModifiers parseMemberModifiers();

        
        std::shared_ptr<Statement> parseTernaryExpression();   
        std::shared_ptr<Statement> parseBinaryExpression();   
        std::shared_ptr<Statement> parseUnaryExpression();

        std::shared_ptr<Statement> parseExpression();         
        std::shared_ptr<Statement> logicalOrExpression();
        std::shared_ptr<Statement> logicalAndExpression();
        std::shared_ptr<Statement> comparisonExpression();
        std::shared_ptr<Statement> term();                    
        std::shared_ptr<Statement> factor();                  
        std::shared_ptr<Statement> parseAs();                  

        void eat(TokenTypes expectedType, const std::string& errorMessage = ""); 
};

#endif
