# slang
## Sintax
### Rules
#### Main rule
```
program     -> declaration* EOF
```

#### Type rules
```
typeExpr      -> functionType | primitiveType
functionType  -> "(" ( typeExpr ( "," typeExpr )* )? ")" "->" ( typeExpr | "none" )
primitiveType -> "fixed" | "float" | "bool" | "str"
```

#### Declaration and statement rules
```
declaration -> varDecl | statement
varDecl     -> ( typeExpr | "auto" ) IDENTIFIER ( "=" expression )? ";"
statement   -> exprStmt | ifStmt | whileStmt | forStmt | returnStmt | block | ";"
ifStmt      -> "if" "(" expression ")" statement ( "else" statement )?
whileStmt   -> "while" "(" expression ")" statement
forStmt     -> "for" "(" ( varDecl | exprStmt | ";" ) expression? ";"
                         expression? ")" statement
returnStmt  -> "return" expression? ";"
block       -> "{" declaration* "}"
exprStmt    -> expression ";"
```

#### Expression rules
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

### Operators Precedence
From highest to lowest:
1. Parentheses `()`
2. Unary operators: `-`, `not`
3. Factor: `*`, `/`
4. Term: `+`, `-`
5. Comparison: `>`, `>=`, `<`, `<=`
6. Equality: `==`, `!=`
7. Logic AND: `and`
8. Logic OR: `or`
9. Ternary: `? :`
7. Assignment: `=`

## Instruction Set
### Instruction Shapes
 - `Op`: `[opcode:8] [unused:24]`
 - `Opabc`: `[opcode:8] [a:8] [b:8] [c:8]`
 - `OpA`: `[opcode:8] [unused:8] [a:16]`
 - `OpsA`: `[opcode:8] [unused:8] [a:16]`
 - `OpAB`: `[opcode:8] [a:12] [b:12]`
 - `OpAb`: `[opcode:8] [a:16] [b:8]`

### Instruction Reference
| I| Instruction | Shape | Description                                         |
|--|-------------|-------|-----------------------------------------------------|
| 0| Return      | Op    | Return from a function                              |
| 1| Constant    | OpAb  | Load constant A into register b                     |

| I| Ins         | Op    |                                                     |


