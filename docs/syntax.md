# Slang syntax
## Syntax rules
### Main rule
```
program     -> declaration* EOF
```

### Type rules
```
typeExpr      -> functionType | primitiveType | IDENTIFIER
functionType  -> "(" ( typeExpr ( "," typeExpr )* )? ")" "->" ( typeExpr | "none" )
primitiveType -> "fixed" | "float" | "bool" | "str"
```

### Declaration and statement rules
```
declaration -> varDecl | typeDecl | statement
varDecl     -> ( typeExpr | "auto" ) IDENTIFIER ( "=" expression )? ";"
typeDecl    -> "type" IDENTIFIER "=" typeExpr ";"
statement   -> exprStmt | ifStmt | whileStmt | forStmt | returnStmt | block | ";"
ifStmt      -> "if" "(" expression ")" statement ( "else" statement )?
whileStmt   -> "while" "(" expression ")" statement
forStmt     -> "for" "(" ( varDecl | exprStmt | ";" ) expression? ";"
                         expression? ")" statement
returnStmt  -> "return" expression? ";"
block       -> "{" declaration* "}"
exprStmt    -> expression ";"
```

### Expression rules
```
expression  -> assignment
assignment  -> IDENTIFIER "=" assignment | ternary 
ternary     -> logicOr ( "?" expression ":" ternary )?
logicOr     -> logicAnd ( "or" logicAnd )*
logicAnd    -> equality ( "and" equality )*
equality    -> comparison ( ( "==" | "!=" ) comparison )*
comparison  -> term ( ( ">" | ">=" | "<" | "<=" ) term )*
term        -> factor ( ( "+" | "-" ) factor )*
factor      -> unary ( ( "*" | "/" ) unary )*
unary       -> ( "-" | "not" ) unary | call 
call        -> primary ( "(" arguments? ")" )*
arguments   -> expression ( "," expression )*
primary     -> NUMBER | STRING | "true" | "false" | "(" expression ")" | IDENTIFIER |
               function
function    -> "(" parameters ")" "->" ( typeExpr | "none" ) block 
parameters  -> ( typeExpr IDENTIFIER ( "," typeExpr IDENTIFIER )* )? 
```
## Operator precedence
From highest to lowest:
1. Parentheses `()`
2. Call
3. Unary operators: `-`, `not`
4. Factor: `*`, `/`
5. Term: `+`, `-`
6. Comparison: `>`, `>=`, `<`, `<=`
7. Equality: `==`, `!=`
8. Logic AND: `and`
9. Logic OR: `or`
10. Ternary: `? :`
11. Assignment: `=`

