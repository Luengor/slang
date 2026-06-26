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
assignment  -> ternary ( "=" assignment )?
ternary     -> logicOr ( "?" expression ":" expresion )?
logicOr     -> logicAnd ( "or" logicAnd )*
logicAnd    -> equality ( "and" equality )*
equality    -> comparison ( ( "==" | "!=" ) comparison )*
comparison  -> term ( ( ">" | ">=" | "<" | "<=" ) term )*
term        -> factor ( ( "+" | "-" ) factor )*
factor      -> cast ( ( "*" | "/" ) cast )*
cast        -> (cast | unary ) ( "as" typeExpr )?
unary       -> ( "-" | "not" ) unary | call 
call        -> primary ( "(" arguments? ")" )*
arguments   -> expression ( "," expression )*
primary     -> NUMBER | STRING | "true" | "false" | "(" expression ")" | IDENTIFIER |
               function | "none"
function    -> "(" parameters ")" "->" ( typeExpr | "none" ) block 
parameters  -> ( typeExpr IDENTIFIER ( "," typeExpr IDENTIFIER )* )? 
```
## Operator precedence
From highest to lowest:
1. Parentheses `()`
2. Call
3. Unary operators: `-`, `not`
4. Cast: `as`
5. Factor: `*`, `/`
6. Term: `+`, `-`
7. Comparison: `>`, `>=`, `<`, `<=`
8. Equality: `==`, `!=`
9. Logic AND: `and`
10. Logic OR: `or`
11. Ternary: `? :`
12. Assignment: `=`

