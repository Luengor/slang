#pragma once

#include "ast_core.hpp"
#include "scanner.hpp"

enum class Precedence {
    None,
    Assignment,
    Or,
    And,
    Equality,
    Comparison,
    Term,
    Factor,
    Unary,
    Call,
    Primary
};

class Parser {
    unsigned int current;
    const std::vector<Token> &tokens;

    // Increment current to point to the next token if not at the end
    void advance();

    // Return the current token without consuming it
    const Token &peek();

    // Return the previous token
    const Token &previous();

    // Check if we've reached the end of the token stream
    bool isAtEnd();

    // Check if the current token matches the given type
    bool check(Token::Type type);

    // If the current token matches any of the given types, consume it and
    // return true
    bool match(const std::initializer_list<Token::Type> &types);

    // Consume the current token if it matches the given type, else throw an
    // error with the provided message
    const Token &consume(Token::Type type, const char *message);

    // Error recovery: synchronize the parser state after an error
    void syncronize();

    ASTNodePtr finishCall(ASTNodePtr expr);
    ASTNodePtr finishIndex(ASTNodePtr expr);

    // typeExpr -> ( functionType | primitiveType ) ( "[" "]" )*
    ASTNodePtr typeExpr();

    // functionType -> "(" ( typeExpr ( "," typeExpr )* )? ")" "->" typeExpr
    ASTNodePtr functionType();

    // primitiveType -> "fixed" | "float" | "bool"
    ASTNodePtr primitiveType();

    // declaration -> varDecl | funcDecl | statement
    ASTNodePtr declaration();

    // varDecl -> ( typeExpr | "auto" ) IDENTIFIER ( "=" expression )? ";"
    ASTNodePtr varDecl();

    // statement -> exprStmt | ifStmt | whileStmt | forStmt | returnStmt | block
    ASTNodePtr statement();

    // exprStmt -> expression ";"
    ASTNodePtr exprStmt();

    // ifStmt -> "if" "(" expression ")" statement ( "else" statement )?
    ASTNodePtr ifStmt();

    // whileStmt -> "while" "(" expression ")" statement
    ASTNodePtr whileStmt();

    // forStmt -> "for" "(" ( varDecl | exprStmt | ";" ) expression? ";"
    //                       expression? ")" statement
    ASTNodePtr forStmt();

    // returnStmt -> "return" expression? ";"
    ASTNodePtr returnStmt();

    // block -> "{" declaration* "}"
    ASTNodePtr block();

    // expression -> assignment
    ASTNodePtr expression();

    // assignment  -> ( IDENTIFIER | call "[" expression "]" ) "=" assignment |
    // ternary
    ASTNodePtr assignment();

    // ternary -> logicOr ( "?" expression ":" ternary )?
    ASTNodePtr ternary();

    // logicOr -> logicAnd ( "or" logicAnd )*
    ASTNodePtr logicOr();

    // logicAnd -> equality ( "and" equality )*
    ASTNodePtr logicAnd();

    // equality -> comparison ( ( "==" | "!=" ) comparison )*
    ASTNodePtr equality();

    // comparison -> term ( ( ">" | ">=" | "<" | "<=" ) term )*
    ASTNodePtr comparison();

    // term -> factor ( ( "+" | "-" | "or" ) factor )*
    ASTNodePtr term();

    // factor -> unary ( ( "*" | "/" | "and" ) unary )*
    ASTNodePtr factor();

    // unary -> ( "-" | "not" ) unary | call
    ASTNodePtr unary();

    // call -> primary ( "(" arguments? ")" | "[" expression "]" )*
    ASTNodePtr call();

    // primary -> NUMBER | STRING | "true" | "false" | "(" expression ")" |
    //            IDENTIFIER | function | arrayLiteral

    // arrayLiteral -> "[" ( expression ( "," expression )* )? "]"
    ASTNodePtr primary();

    // function -> "(" parameters ")" "->" ( typeExpr | "none" ) block
    ASTNodePtr function();

  public:
    Parser(const std::vector<Token> &tokens);

    // Parse the tokens and return the root of the AST
    // program -> declaration* EOF
    ASTNodePtr parse();
};
