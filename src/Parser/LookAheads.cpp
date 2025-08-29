#include <omniscript/Statements/Statement.h>
#include <omniscript/Statements/AccessStatements.h>
#include <omniscript/Statements/FunctionStatement.h>
#include <omniscript/Statements/CallableStatement.h>
#include <omniscript/Statements/LiteralStatements.h>
#include <omniscript/Statements/AssignmentAndGetterStatements.h>

#include <omniscript/Core.h>
#include <omniscript/utils.h>
#include <omniscript/Lexer.h>
#include <omniscript/Tokens.h>
#include <omniscript/Parser.h>
#include <omniscript/Symboltable.h>
#include <omniscript/Types/Types.h>
#include <omniscript/omniscript_pch.h>
#include <omniscript/LibraryPaths.h>

namespace Omniscript {

// Add these functions to your LookAheads.cpp file
bool Parser::checkIfLambdaExpression() {
    int i = 0;

    // Check if the expression starts with 'identifier' (optional) or '('
    auto prevTokenNow = currentToken;
    if (currentToken.getType() == TokenTypes::Identifier) {
        i++; // Consume the lambda name
    }

    // Check for parameter list: '(' ... ')'
    if (lexer.peekToken(i).getType() != TokenTypes::LeftParen) {
        return false;
    }
    i++;
    i = parseParameterListLookahead(i);
    if (i == -1) {
        return false;
    }

    // Check for lambda arrow '=>'
    if (lexer.peekToken(i).getType() != TokenTypes::Arrow) {
        return false;
    } else {
        i += 2; // Consume '=>' or '-->' or '->'
    }

    // Check for optional return type
    i = parseOptionalReturnTypeLookahead(i);
    if (i == -1) {
        return false;
    }

    // Check for body enclosed in braces '{ ... }'
    if (lexer.peekToken(i).getType() != TokenTypes::LeftBrace) {
        return false;
    }

    // If we reach here, the token sequence matches a lambda expression
    return true;
}

int Parser::parseParameterListLookahead(int startIndex) {
    int i = startIndex;
    
    // Handle empty parameter list
    if (lexer.peekToken(i).getType() == TokenTypes::RightParen) {
        i++; // consume ')'
        return i;
    }
    
    while (lexer.peekToken(i).getType() != TokenTypes::RightParen) {
        // Must have parameter name
        if (lexer.peekToken(i).getType() != TokenTypes::Identifier) {
            return -1; // Invalid parameter
        }
        i++; // consume parameter name
        
        // Optional type annotation
        if (lexer.peekToken(i).getType() == TokenTypes::Colon) {
            i++; // consume ':'
            
            // Skip over type (simplified lookahead)
            int typeEndIndex = parseTypeLookahead(i);
            if (typeEndIndex == -1) {
                return -1;
            }
            i = typeEndIndex;
        }
        
        // Optional default value
        if (lexer.peekToken(i).getType() == TokenTypes::Assign) {
            i++; // consume '='
            // Skip over default value (simplified)
            if (lexer.peekToken(i).getType() == TokenTypes::IntegerLiteral ||
                lexer.peekToken(i).getType() == TokenTypes::FloatLiteral ||
                lexer.peekToken(i).getType() == TokenTypes::StringLiteral ||
                lexer.peekToken(i).getType() == TokenTypes::Identifier) {
                i++;
            } else {
                return -1; // Invalid default value
            }
        }
        
        // Handle parameter separator
        if (lexer.peekToken(i).getType() == TokenTypes::Comma) {
            i++; // consume ','
        } else if (lexer.peekToken(i).getType() != TokenTypes::RightParen) {
            return -1; // Expected comma or closing paren
        }
    }
    
    if (lexer.peekToken(i).getType() == TokenTypes::RightParen) {
        i++; // consume ')'
        return i;
    }
    
    return -1;
}

int Parser::parseOptionalReturnTypeLookahead(int startIndex) {
    int i = startIndex;
    
    if (lexer.peekToken(i).getType() == TokenTypes::Colon) {
        i++; // consume ':'
        
        // Parse return type (simplified lookahead)
        int typeEndIndex = parseTypeLookahead(i);
        if (typeEndIndex == -1) {
            return -1;
        }
        i = typeEndIndex;
    }
    
    return i;
}

int Parser::parseTypeLookahead(int startIndex) {
    int i = startIndex;
    
    // Handle function types
    if (lexer.peekToken(i).getType() == TokenTypes::Function) {
        i++; // consume 'fn'
        
        // Skip function signature for lookahead
        if (lexer.peekToken(i).getType() == TokenTypes::LeftParen) {
            int parenCount = 1;
            i++; // consume '('
            
            while (parenCount > 0 && lexer.peekToken(i).getType() != TokenTypes::EOI) {
                if (lexer.peekToken(i).getType() == TokenTypes::LeftParen) {
                    parenCount++;
                } else if (lexer.peekToken(i).getType() == TokenTypes::RightParen) {
                    parenCount--;
                }
                i++;
            }
            
            // Skip arrow and return type
            if (lexer.peekToken(i).getType() == TokenTypes::Arrow) {
                i++; // consume '=>'
                return parseTypeLookahead(i); // Parse return type recursively
            }
        }
        
        return i;
    }
    
    // Handle type modifiers (*, &, ?)
    while (lexer.peekToken(i).getType() == TokenTypes::Multiply ||
           lexer.peekToken(i).getType() == TokenTypes::BitwiseAnd ||
           lexer.peekToken(i).getType() == TokenTypes::QuestionMark) {
        i++;
    }
    
    // Must have base type
    if (lexer.peekToken(i).getType() != TokenTypes::Identifier) {
        return -1;
    }
    i++; // consume base type
    
    // Handle dotted types (e.g., std.vector)
    while (lexer.peekToken(i).getType() == TokenTypes::Dot) {
        i++; // consume '.'
        if (lexer.peekToken(i).getType() != TokenTypes::Identifier) {
            return -1;
        }
        i++; // consume identifier
    }
    
    // Handle trailing modifiers
    while (lexer.peekToken(i).getType() == TokenTypes::Multiply ||
           lexer.peekToken(i).getType() == TokenTypes::BitwiseAnd ||
           lexer.peekToken(i).getType() == TokenTypes::QuestionMark) {
        i++;
    }
    
    // Handle array types
    while (lexer.peekToken(i).getType() == TokenTypes::LeftBracket) {
        i++; // consume '['
        
        // Skip array size
        if (lexer.peekToken(i).getType() == TokenTypes::IntegerLiteral ||
            lexer.peekToken(i).getType() == TokenTypes::Identifier) {
            i++;
        } else {
            return -1; // Invalid array size
        }
        
        if (lexer.peekToken(i).getType() != TokenTypes::RightBracket) {
            return -1; // Missing closing bracket
        }
        i++; // consume ']'
    }
    
    return i;
}

}
