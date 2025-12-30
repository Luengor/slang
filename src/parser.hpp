#pragma once

#include "ast_core.hpp"
#include "scanner.hpp"
#include <memory>

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

    // typeExpr -> functionType | primitiveType
    std::unique_ptr<ASTNode> typeExpr();

    // functionType -> "(" ( typeExpr ( "," typeExpr )* )? ")" "->" typeExpr
    std::unique_ptr<ASTNode> functionType();

    // primitiveType -> "fixed" | "float" | "bool"
    std::unique_ptr<ASTNode> primitiveType();

    // declaration -> varDecl | funcDecl | statement
    std::unique_ptr<ASTNode> declaration();

    // varDecl -> ( typeExpr | "auto" ) IDENTIFIER ( "=" expression )? ";"
    std::unique_ptr<ASTNode> varDecl();

    // statement -> exprStmt | ifStmt | whileStmt | forStmt | block
    std::unique_ptr<ASTNode> statement();

    // exprStmt -> expression ";"
    std::unique_ptr<ASTNode> exprStmt();

    // ifStmt -> "if" "(" expression ")" statement ( "else" statement )?
    std::unique_ptr<ASTNode> ifStmt();

    // whileStmt -> "while" "(" expression ")" statement
    std::unique_ptr<ASTNode> whileStmt();

    // forStmt -> "for" "(" ( varDecl | exprStmt | ";" ) expression? ";"
    //                       expression? ")" statement
    std::unique_ptr<ASTNode> forStmt();

    // block -> "{" declaration* "}"
    std::unique_ptr<ASTNode> block();

    // expression -> assignment 
    std::unique_ptr<ASTNode> expression();

    // assignment  -> IDENTIFIER "=" assignment | logicOr
    std::unique_ptr<ASTNode> assignment();

    // logicOr -> logicAnd ( "or" logicAnd )*
    std::unique_ptr<ASTNode> logicOr();

    // logicAnd -> equality ( "and" equality )*
    std::unique_ptr<ASTNode> logicAnd();

    // equality -> comparison ( ( "==" | "!=" ) comparison )*
    std::unique_ptr<ASTNode> equality();

    // comparison -> term ( ( ">" | ">=" | "<" | "<=" ) term )*
    std::unique_ptr<ASTNode> comparison();

    // term -> factor ( ( "+" | "-" | "or" ) factor )*
    std::unique_ptr<ASTNode> term();

    // factor -> unary ( ( "*" | "/" | "and" ) unary )*
    std::unique_ptr<ASTNode> factor();

    // unary -> ( "-" | "not" ) unary | primary
    std::unique_ptr<ASTNode> unary();

    // primary -> NUMBER | STRING | "true" | "false" | "(" expression ")" |
    //            IDENTIFIER | function
    std::unique_ptr<ASTNode> primary();


    std::unique_ptr<ASTNode> function();

public:
    Parser(const std::vector<Token> &tokens);

    // Parse the tokens and return the root of the AST
    // program -> declaration* EOF
    std::unique_ptr<ASTNode> parse();
};

